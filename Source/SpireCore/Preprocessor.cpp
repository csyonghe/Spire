// Preprocessor.cpp
#include "Preprocessor.h"

#include "Diagnostics.h"
#include "Lexer.h"

#include <assert.h>

using namespace CoreLib;
using namespace CoreLib::Text;


// This file provides an implementation of a simple C-style preprocessor.
// It does not aim for 100% compatibility with any particular preprocessor
// specification, but the goal is to have it accept the most common
// idioms for using the preprocessor, found in shader code in the wild.


namespace Spire{ namespace Compiler {

// State of a preprocessor conditional, which can change when
// we encounter directives like `#elif` or `#endif`
enum PreprocessorConditionalState
{
    Before, // We have not yet seen a branch with a `true` condition.
    During, // We are inside the branch with a `true` condition.
    After,  // We have already seen the branch with a `true` condition.
};

// Represents a preprocessor conditional that we are currently
// nested inside.
struct PreprocessorConditional
{
    // The next outer conditional in the current file/stream, or NULL.
    PreprocessorConditional*        parent;

    // The directive token that started the conditional (an `#if` or `#ifdef`)
    Token                           ifToken;

    // The `#else` directive token, if one has been seen (otherwise `TokenType::Unknown`)
    Token                           elseToken;

    // The state of the conditional
    PreprocessorConditionalState    state;
};

// Input tokens can either come from source text, or from macro expansion.
// In general, input streams can be nested, so we have to keep a conceptual
// stack of input.

// An enumeration for the diferent types of token input streams.
enum PreprocessorInputStreamFlavor
{
    SourceText,
    ObjectLikeMacro,
    FunctionLikeMacro,
};

// A stream of input tokens to be consumed
struct PreprocessorInputStream
{
    // The next input stream up the stack, if any.
    PreprocessorInputStream*        parent;

    // The deepest preprocessor conditional active for this stream.
    PreprocessorConditional*        conditional;

    // Pre-tokenized input for this stream
    List<Token>                     tokens;

    // Index of the "current" token in the stream.
    int                             tokenIndex;

    // The flavor of input stream (e.g., `SourceText`)
    PreprocessorInputStreamFlavor   flavor;
};

// In the current design (which we may want to re-consider),
// a macro is a specialized flavor of input stream, that
// captures the token list in its expansion, and then
// can be "played back."
struct PreprocessorMacro : PreprocessorInputStream
{
    // The name under which the macro was `#define`d
    Token nameToken;
};

// State of the preprocessor
struct Preprocessor
{
    // diagnostics sink to use when writing messages
    DiagnosticSink*                         sink;

    // An external callback interface to use when looking
    // for files in a `#include` directive
    IncludeHandler*                         includeHandler;

    // Current input stream (top of the stack of input)
    PreprocessorInputStream*                inputStream;

    // Currently-defined macros
    Dictionary<String, PreprocessorMacro*>  macros;

    // A pre-allocated token that can be returned to
    // represent end-of-input situations.
    Token                                   endOfFileToken;
};

// Convenience routine to access the diagnostic sink
static DiagnosticSink* GetSink(Preprocessor* preprocessor)
{
    return preprocessor->sink;
}

//
// Forward declarations
//

static void DestroyConditional(PreprocessorConditional* conditional);

//
// Basic Input Handling
//

// Create an input stream to represent a pre-tokenized input file.
// TODO(tfoley): pre-tokenizing files isn't going to work in the long run.
static PreprocessorInputStream* CreateInputStreamForSource(Preprocessor* preprocessor, CoreLib::String const& source, CoreLib::String const& fileName)
{
    Lexer lexer;

    PreprocessorInputStream* inputStream = new PreprocessorInputStream();

    // Use existing `Lexer` to generate a token stream.
    inputStream->tokens = lexer.Parse(fileName, source, GetSink(preprocessor));
    inputStream->parent = NULL;
    inputStream->conditional = NULL;
    inputStream->tokenIndex = 0;
    inputStream->flavor = PreprocessorInputStreamFlavor::SourceText;

    return inputStream;
}

// Called when we reach the end of an input stream.
// Performs some validation and then destroys the input stream if required.
static void EndInputStream(Preprocessor* preprocessor, PreprocessorInputStream* inputStream)
{
    // If there are any conditionals that weren't completed, then it is an error
    if (inputStream->conditional)
    {
        PreprocessorConditional* conditional = inputStream->conditional;

        GetSink(preprocessor)->diagnose(conditional->ifToken.Position, Diagnostics::endOfFileInPreprocessorConditional);

        while (conditional)
        {
            PreprocessorConditional* parent = conditional->parent;
            DestroyConditional(conditional);
            conditional = parent;
        }
    }

    switch (inputStream->flavor)
    {
    case PreprocessorInputStreamFlavor::SourceText:
        delete inputStream;
        break;

    case PreprocessorInputStreamFlavor::ObjectLikeMacro:
    case PreprocessorInputStreamFlavor::FunctionLikeMacro :
        // don't delete a macro, just make it non-busy
        inputStream->parent = NULL;
        break;
    }
}

// Read one token in "raw" mode (meaning don't expand macros)
static Token const& AdvanceRawToken(Preprocessor* preprocessor)
{
    for (;;)
    {
        // Look at the input stream on top of the stack
        PreprocessorInputStream* inputStream = preprocessor->inputStream;

        // If there isn't one, then there is no more input left to read.
        if (!inputStream)
        {
            return preprocessor->endOfFileToken;
        }

        // The top-most input stream may be at its end, in which
        // case we need to pop it from the stack and try again
        int tokenIndex = inputStream->tokenIndex;
        if (tokenIndex >= inputStream->tokens.Count())
        {
            preprocessor->inputStream = inputStream->parent;
            EndInputStream(preprocessor, inputStream);
            continue;
        }

        // Everything worked, so read a token from the top-most stream
        inputStream->tokenIndex = tokenIndex + 1;
        return inputStream->tokens[tokenIndex];
    }
}

// Return the next token in "raw" mode, but don't advance the
// current token state.
static Token const& PeekRawToken(Preprocessor* preprocessor)
{
    // We need to find the strema that `advanceRawToken` would read from.
    PreprocessorInputStream* inputStream = preprocessor->inputStream;
    for (;;)
    {
        if (!inputStream)
        {
            // No more input streams left to read
            return preprocessor->endOfFileToken;
        }

        // The top-most input stream may be at its end, so
        // look one entry up the stack (don't actually pop
        // here, since we are just peeking)
        int tokenIndex = inputStream->tokenIndex;
        if (tokenIndex >= inputStream->tokens.Count())
        {
            inputStream = inputStream->parent;
            continue;
        }

        // Everything worked, so peek a token from the top-most stream
        return inputStream->tokens[tokenIndex];
    }
}

// Get the location of the current (raw) token
static CodePosition PeekLoc(Preprocessor* preprocessor)
{
    return PeekRawToken(preprocessor).Position;
}

// Get the `TokenType` of the current (raw) token
static CoreLib::Text::TokenType PeekRawTokenType(Preprocessor* preprocessor)
{
    return PeekRawToken(preprocessor).Type;
}

//
// Macros
//

// Create a macro
static PreprocessorMacro* CreateMacro(Preprocessor* /*preprocessor*/)
{
    // TODO(tfoley): Allocate these more intelligently.
    // For example, consider pooling them on the preprocessor.

    PreprocessorMacro* macro = new PreprocessorMacro();
    macro->flavor = PreprocessorInputStreamFlavor::ObjectLikeMacro;
    return macro;
}

// Destroy a macro
static void DestroyMacro(Preprocessor* /*preprocessor*/, PreprocessorMacro* macro)
{
    delete macro;
}


// Find the currently-defined macro of the given name, or return NULL
static PreprocessorMacro* LookupMacro(Preprocessor* preprocessor, String const& name)
{
    PreprocessorMacro* macro = NULL;
    if (preprocessor->macros.TryGetValue(name, macro))
        return macro;
    return NULL;
}

// A macro is "busy" if it is currently being used for expansion.
// A macro cannot be expanded again while busy, to avoid infinite recursion.
static bool IsMacroBusy(PreprocessorMacro* macro)
{
    // We implement the "busy" state by simply checking if the
    // macro is currently on the stack of input streams (which
    // would mean if has a non-`NULL` parent).
    return macro->parent != NULL;
}

//
// Reading Tokens With Expansion
//

// Check whether the current token on the given input stream should be
// treated as a macro invocation, and if so set up state for expanding
// that macro.
static void MaybeBeginMacroExpansion(
    Preprocessor*               preprocessor )
{
    // We iterate because the first token in the expansion of one
    // macro may be another macro invocation.
    for (;;)
    {
        // Look at the next token ahead of us
        Token const& token = PeekRawToken(preprocessor);

        // Not an identifier? Can't be a macro.
        if (token.Type != TokenType::Identifier)
            return;

        // Look for a macro with the given name.
        String name = token.Content;
        PreprocessorMacro* macro = LookupMacro(preprocessor, name);

        // Not a macro? Can't be an invocation.
        if (!macro)
            return;

        // If the macro is busy (already being expanded),
        // don't try to trigger recursive expansion
        if (IsMacroBusy(macro))
            return;

        // TODO: handle expansion for function-like macros

        // Consume the token that triggered macro expansion
        AdvanceRawToken(preprocessor);

        macro->tokenIndex = 0;
        macro->parent = preprocessor->inputStream;
        preprocessor->inputStream = macro;
    }
}

// Read one token with macro-expansion enabled.
static Token const& AdvanceToken(Preprocessor* preprocessor)
{
    // Check whether we need to macro expand at the cursor.
    MaybeBeginMacroExpansion(preprocessor);

    // Read a raw token (now that expansion has been triggered)
    return AdvanceRawToken(preprocessor);

    // TODO: handle token pasting here, since we have just
    // read a token, and can now peek to see if the next
    // raw token is `##`.
}

// Read one token with macro-expansion enabled.
//
// Note that because triggering macro expansion may
// involve changing the input-stream state, this
// operation *can* have side effects.
static Token const& PeekToken(Preprocessor* preprocessor)
{
    // Check whether we need to macro expand at the cursor.
    MaybeBeginMacroExpansion(preprocessor);

    // Peek a raw token (now that expansion has been triggered)
    return PeekRawToken(preprocessor);

    // TODO: need a plan for how to handle token pasting
    // here without it being onerous. Would be nice if we
    // didn't have to re-do pasting on a "peek"...
}

// Peek the type of the next token, including macro expansion.
static CoreLib::Text::TokenType PeekTokenType(Preprocessor* preprocessor)
{
    return PeekToken(preprocessor).Type;
}

//
// Preprocessor Directives
//

// When reading a preprocessor directive, we use a context
// to wrap the direct preprocessor routines defines so far.
//
// One of the most important things the directive context
// does is give us a convenient way to read tokens with
// a guarantee that we won't read past the end of a line.
struct PreprocessorDirectiveContext
{
    // The preprocessor that is parsing the directive.
    Preprocessor*   preprocessor;

    // The directive token (e.g., the `if` in `#if`).
    // Useful for reference in diagnostic messages.
    Token           directiveToken;

    // Has any kind of parse error been encountered in
    // the directive so far?
    bool            parseError;
};

// Get the token for  the preprocessor directive being parsed.
inline Token const& GetDirective(PreprocessorDirectiveContext* context)
{
    return context->directiveToken;
}

// Get the name of the directive being parsed.
inline String const& GetDirectiveName(PreprocessorDirectiveContext* context)
{
    return context->directiveToken.Content;
}

// Get the location of the directive being parsed.
inline CodePosition const& GetDirectiveLoc(PreprocessorDirectiveContext* context)
{
    return context->directiveToken.Position;
}

// Wrapper to get the diagnostic sink in the context of a directive.
static inline DiagnosticSink* GetSink(PreprocessorDirectiveContext* context)
{
    return GetSink(context->preprocessor);
}

// Wrapper to get a "current" location when parsing a directive
static CodePosition PeekLoc(PreprocessorDirectiveContext* context)
{
    return PeekLoc(context->preprocessor);
}

// Wrapper to look up a macro in the context of a directive.
static PreprocessorMacro* LookupMacro(PreprocessorDirectiveContext* context, String const& name)
{
    return LookupMacro(context->preprocessor, name);
}

// Determine if we have read everthing on the directive's line.
static bool IsEndOfLine(PreprocessorDirectiveContext* context)
{
    // Is the next token at the start of its own line?
    // If so, we have read to the end of the directive.
    return (PeekRawToken(context->preprocessor).flags & TokenFlag::AtStartOfLine) != 0;
}

// Read one raw token in a directive, without going past the end of the line.
static Token const& AdvanceRawToken(PreprocessorDirectiveContext* context)
{
    if (IsEndOfLine(context))
        return context->preprocessor->endOfFileToken;
    return AdvanceRawToken(context->preprocessor);
}

// Peek one raw token in a directive, without going past the end of the line.
static Token const& PeekRawToken(PreprocessorDirectiveContext* context)
{
    if (IsEndOfLine(context))
        return context->preprocessor->endOfFileToken;
    return PeekRawToken(context->preprocessor);
}

// Peek next raw token type, without going past the end of the line.
static CoreLib::Text::TokenType PeekRawTokenType(PreprocessorDirectiveContext* context)
{
    if (IsEndOfLine(context))
        return TokenType::EndOfFile;
    return PeekRawTokenType(context->preprocessor);
}

// Read one token, with macro-expansion, without going past the end of the line.
static Token const& AdvanceToken(PreprocessorDirectiveContext* context)
{
    if (IsEndOfLine(context))
        context->preprocessor->endOfFileToken;
    return AdvanceToken(context->preprocessor);
}

// Peek one token, with macro-expansion, without going past the end of the line.
static Token const& PeekToken(PreprocessorDirectiveContext* context)
{
    if (IsEndOfLine(context))
        context->preprocessor->endOfFileToken;
    return PeekToken(context->preprocessor);
}

// Peek next token type, with macro-expansion, without going past the end of the line.
static CoreLib::Text::TokenType PeekTokenType(PreprocessorDirectiveContext* context)
{
    if (IsEndOfLine(context))
        return TokenType::EndOfFile;
    return PeekTokenType(context->preprocessor);
}

// Skip to the end of the line (useful for recovering from errors in a directive)
static void SkipToEndOfLine(PreprocessorDirectiveContext* context)
{
    while(!IsEndOfLine(context))
    {
        AdvanceRawToken(context);
    }
}

static bool ExpectRaw(PreprocessorDirectiveContext* context, CoreLib::Text::TokenType tokenType, DiagnosticInfo const& diagnostic, Token* outToken = NULL)
{
    if (PeekRawTokenType(context) != tokenType)
    {
        // Only report the first parse error within a directive
        if (!context->parseError)
        {
            GetSink(context)->diagnose(PeekLoc(context), diagnostic, tokenType, GetDirectiveName(context));
        }
        context->parseError = true;
        return false;
    }
    Token const& token = AdvanceRawToken(context);
    if (outToken)
        *outToken = token;
    return true;
}

static bool Expect(PreprocessorDirectiveContext* context, CoreLib::Text::TokenType tokenType, DiagnosticInfo const& diagnostic, Token* outToken = NULL)
{
    if (PeekTokenType(context) != tokenType)
    {
        // Only report the first parse error within a directive
        if (!context->parseError)
        {
            GetSink(context)->diagnose(PeekLoc(context), diagnostic, tokenType, GetDirectiveName(context));
            context->parseError = true;
        }
        return false;
    }
    Token const& token = AdvanceToken(context);
    if (outToken)
        *outToken = token;
    return true;
}



//
// Preprocessor Conditionals
//

// Determine whether the current preprocessor state means we
// should be skipping tokens.
static bool IsSkipping(Preprocessor* preprocessor)
{
    PreprocessorInputStream* inputStream = preprocessor->inputStream;
    if (!inputStream) return false;

    // If we are not inside a preprocessor conditional, then don't skip
    PreprocessorConditional* conditional = inputStream->conditional;
    if (!conditional) return false;

    // skip tokens unless the conditional is inside its `true` case
    return conditional->state != PreprocessorConditionalState::During;
}

// Wrapper for use inside directives
static inline bool IsSkipping(PreprocessorDirectiveContext* context)
{
    return IsSkipping(context->preprocessor);
}

// Create a preprocessor conditional
static PreprocessorConditional* CreateConditional(Preprocessor* /*preprocessor*/)
{
    // TODO(tfoley): allocate these more intelligently (for example,
    // pool them on the `Preprocessor`.
    return new PreprocessorConditional();
}

// Destroy a preprocessor conditional.
static void DestroyConditional(PreprocessorConditional* conditional)
{
    delete conditional;
}

// Start a preprocessor conditional, with an initial enable/disable state.
static void BeginConditional(PreprocessorDirectiveContext* context, bool enable)
{
    Preprocessor* preprocessor = context->preprocessor;
    PreprocessorInputStream* inputStream = preprocessor->inputStream;
    assert(inputStream);

    PreprocessorConditional* conditional = CreateConditional(preprocessor);

    conditional->ifToken = context->directiveToken;

    // Set state of this condition appropriately.
    //
    // Default to the "haven't yet seen a `true` branch" state.
    PreprocessorConditionalState state = PreprocessorConditionalState::Before;
    //
    // If we are nested inside a `false` branch of another condition, then
    // we never want to enable, so we act as if we already *saw* the `true` branch.
    //
    if (IsSkipping(preprocessor)) state = PreprocessorConditionalState::After;
    //
    // Similarly, if we ran into any parse errors when dealing with the
    // opening directive, then things are probably screwy and we should just
    // skip all the branches.
    if (IsSkipping(preprocessor)) state = PreprocessorConditionalState::After;
    //
    // Otherwise, if our condition was true, then set us to be inside the `true` branch
    else if (enable) state = PreprocessorConditionalState::During;

    conditional->state = state;

    // Push conditional onto the stack
    conditional->parent = inputStream->conditional;
    inputStream->conditional = conditional;
}

//
// Preprocessor Conditional Expressions
//

// Conditional expressions are always of type `int`
typedef int PreprocessorExpressionValue;

// Forward-declaretion
static PreprocessorExpressionValue ParseAndEvaluateExpression(PreprocessorDirectiveContext* context);

// Parse a unary (prefix) expression inside of a preprocessor directive.
static PreprocessorExpressionValue ParseAndEvaluateUnaryExpression(PreprocessorDirectiveContext* context)
{
    switch (PeekTokenType(context))
    {
    // handle prefix unary ops
    case TokenType::OpSub:
        AdvanceToken(context);
        return -ParseAndEvaluateUnaryExpression(context);
    case TokenType::OpNot:
        AdvanceToken(context);
        return !ParseAndEvaluateUnaryExpression(context);
    case TokenType::OpBitNot:
        AdvanceToken(context);
        return ~ParseAndEvaluateUnaryExpression(context);

    // handle parenthized sub-expression
    case TokenType::LParent:
        {
            Token leftParen = AdvanceToken(context);
            PreprocessorExpressionValue value = ParseAndEvaluateExpression(context);
            if (!Expect(context, TokenType::RParent, Diagnostics::expectedTokenInPreprocessorExpression))
            {
                GetSink(context)->diagnose(leftParen.Position, Diagnostics::seeOpeningToken, leftParen);
            }
            return value;
        }

    case TokenType::IntLiterial:
        return StringToInt(AdvanceToken(context).Content);

    case TokenType::Identifier:
        {
            Token token = AdvanceToken(context);
            if (token.Content == "defined")
            {
                // handle `defined(someName)`

                // Expect a `(`
                Token leftParen;
                if (!ExpectRaw(context, TokenType::LParent, Diagnostics::expectedTokenInDefinedExpression, &leftParen))
                {
                    return 0;
                }

                // Expect an identifier
                Token nameToken;
                if (!ExpectRaw(context, TokenType::Identifier, Diagnostics::expectedTokenInDefinedExpression, &nameToken))
                {
                    return 0;
                }
                String name = nameToken.Content;

                // Expect a `)`
                if(!ExpectRaw(context, TokenType::RParent, Diagnostics::expectedTokenInDefinedExpression))
                {
                    GetSink(context)->diagnose(leftParen.Position, Diagnostics::seeOpeningToken, leftParen);
                    return 0;
                }

                return LookupMacro(context, name) != NULL;
            }

            // An identifier here means it was not defined as a macro (or
            // it is defined, but as a function-like macro. These should
            // just evaluate to zero (possibly with a warning)
            return 0;
        }

    default:
        GetSink(context)->diagnose(PeekLoc(context), Diagnostics::syntaxErrorInPreprocessorExpression);
        return 0;
    }
}

// Determine the precedence level of an infix operator
// for use in parsing preprocessor conditionals.
static int GetInfixOpPrecedence(Token const& opToken)
{
    // If token is on another line, it is not part of the
    // expression
    if (opToken.flags & TokenFlag::AtStartOfLine)
        return -1;

    // otherwise we look at the token type to figure
    // out what precednece it should be parse with
    switch (opToken.Type)
    {
    default:
        // tokens that aren't infix operators should
        // cause us to stop parsing an expression
        return -1;

    case TokenType::OpMul:     return 10;
    case TokenType::OpDiv:     return 10;
    case TokenType::OpMod:     return 10;

    case TokenType::OpAdd:     return 9;
    case TokenType::OpSub:     return 9;

    case TokenType::OpLsh:     return 8;
    case TokenType::OpRsh:     return 8;

    case TokenType::OpLess:    return 7;
    case TokenType::OpGreater: return 7;
    case TokenType::OpLeq:     return 7;
    case TokenType::OpGeq:     return 7;

    case TokenType::OpEql:     return 6;
    case TokenType::OpNeq:     return 6;

    case TokenType::OpBitAnd:  return 5;
    case TokenType::OpBitOr:   return 4;
    case TokenType::OpBitXor:  return 3;
    case TokenType::OpAnd:     return 2;
    case TokenType::OpOr:      return 1;
    }
};

// Evaluate one infix operation in a preprocessor
// conditional expression
static PreprocessorExpressionValue EvaluateInfixOp(
    PreprocessorDirectiveContext*   context,
    Token const&                    opToken,
    PreprocessorExpressionValue     left,
    PreprocessorExpressionValue     right)
{
    switch (opToken.Type)
    {
    default:
//        SPIRE_INTERNAL_ERROR(getSink(preprocessor), opToken);
        return 0;
        break;

    case TokenType::OpMul:     return left * right;
    case TokenType::OpDiv:
    {
        if (right == 0)
        {
            if (!context->parseError)
            {
                GetSink(context)->diagnose(opToken.Position, Diagnostics::divideByZeroInPreprocessorExpression);
            }
            return 0;
        }
        return left / right;
    }
    case TokenType::OpMod:
    {
        if (right == 0)
        {
            if (!context->parseError)
            {
                GetSink(context)->diagnose(opToken.Position, Diagnostics::divideByZeroInPreprocessorExpression);
            }
            return 0;
        }
        return left % right;
    }
    case TokenType::OpAdd:      return left +  right;
    case TokenType::OpSub:      return left -  right;
    case TokenType::OpLsh:      return left << right;
    case TokenType::OpRsh:      return left >> right;
    case TokenType::OpLess:     return left <  right ? 1 : 0;
    case TokenType::OpGreater:  return left >  right ? 1 : 0;
    case TokenType::OpLeq:      return left <= right ? 1 : 0;
    case TokenType::OpGeq:      return left <= right ? 1 : 0;
    case TokenType::OpEql:      return left == right ? 1 : 0;
    case TokenType::OpNeq:      return left != right ? 1 : 0;
    case TokenType::OpBitAnd:   return left & right;
    case TokenType::OpBitOr:    return left | right;
    case TokenType::OpBitXor:   return left ^ right;
    case TokenType::OpAnd:      return left && right;
    case TokenType::OpOr:       return left || right;
    }
}

// Parse the rest of an infix preprocessor expression with
// precedence greater than or equal to the given `precedence` argument.
// The value of the left-hand-side expression is provided as
// an argument.
// This is used to form a simple recursive-descent expression parser.
static PreprocessorExpressionValue ParseAndEvaluateInfixExpressionWithPrecedence(
    PreprocessorDirectiveContext* context,
    PreprocessorExpressionValue left,
    int precedence)
{
    for (;;)
    {
        // Look at the next token, and see if it is an operator of
        // high enough precedence to be included in our expression
        Token opToken = PeekToken(context);
        int opPrecedence = GetInfixOpPrecedence(opToken);

        // If it isn't an operator of high enough precendece, we are done.
        if(opPrecedence < precedence)
            break;

        // Otherwise we need to consume the operator token.
        AdvanceToken(context);

        // Next we parse a right-hand-side expression by starting with
        // a unary expression and absorbing and many infix operators
        // as possible with strictly higher precedence than the operator
        // we found above.
        PreprocessorExpressionValue right = ParseAndEvaluateUnaryExpression(context);
        for (;;)
        {
            // Look for an operator token
            Token rightOpToken = PeekToken(context);
            int rightOpPrecedence = GetInfixOpPrecedence(rightOpToken);

            // If no operator was found, or the operator wasn't high
            // enough precedence to fold into the right-hand-side,
            // exit this loop.
            if (rightOpPrecedence <= opPrecedence)
                break;

            // Now invoke the parser recursively, passing in our
            // existing right-hand side to form an even larger one.
            right = ParseAndEvaluateInfixExpressionWithPrecedence(
                context,
                right,
                rightOpPrecedence);
        }

        // Now combine the left- and right-hand sides using
        // the operator we found above.
        left = EvaluateInfixOp(context, opToken, left, right);
    }
    return left;
}

// Parse a complete (infix) preprocessor expression, and return its value
static PreprocessorExpressionValue ParseAndEvaluateExpression(PreprocessorDirectiveContext* context)
{
    // First read in the left-hand side (or the whole expression in the unary case)
    PreprocessorExpressionValue value = ParseAndEvaluateUnaryExpression(context);

    // Try to read in trailing infix operators with correct precedence
    return ParseAndEvaluateInfixExpressionWithPrecedence(context, value, 0);
}

// Handle a `#if` directive
static void HandleIfDirective(PreprocessorDirectiveContext* context)
{
    // Parse a preprocessor expression.
    PreprocessorExpressionValue value = ParseAndEvaluateExpression(context);

    // Begin a preprocessor block, enabled based on the expression.
    BeginConditional(context, value != 0);
}

// Handle a `#ifdef` directive
static void HandleIfDefDirective(PreprocessorDirectiveContext* context)
{
    // Expect a raw identifier, so we can check if it is defined
    Token nameToken;
    if(!ExpectRaw(context, TokenType::Identifier, Diagnostics::expectedTokenInPreprocessorDirective, &nameToken))
        return;
    String name = nameToken.Content;

    // Check if the name is defined.
    BeginConditional(context, LookupMacro(context, name) != NULL);
}

// Handle a `#ifndef` directive
static void HandleIfNDefDirective(PreprocessorDirectiveContext* context)
{
    // Expect a raw identifier, so we can check if it is defined
    Token nameToken;
    if(!ExpectRaw(context, TokenType::Identifier, Diagnostics::expectedTokenInPreprocessorDirective, &nameToken))
        return;
    String name = nameToken.Content;

    // Check if the name is defined.
    BeginConditional(context, LookupMacro(context, name) == NULL);
}

// Handle a `#else` directive
static void HandleElseDirective(PreprocessorDirectiveContext* context)
{
    PreprocessorInputStream* inputStream = context->preprocessor->inputStream;
    assert(inputStream);

    // if we aren't inside a conditional, then error
    PreprocessorConditional* conditional = inputStream->conditional;
    if (!conditional)
    {
        GetSink(context)->diagnose(GetDirectiveLoc(context), Diagnostics::directiveWithoutIf, GetDirectiveName(context));
        return;
    }

    // if we've already seen a `#else`, then it is an error
    if (conditional->elseToken.Type != TokenType::Unknown)
    {
        GetSink(context)->diagnose(GetDirectiveLoc(context), Diagnostics::directiveAfterElse, GetDirectiveName(context));
        GetSink(context)->diagnose(conditional->elseToken.Position, Diagnostics::seeDirective);
        return;
    }
    conditional->elseToken = context->directiveToken;

    switch (conditional->state)
    {
    case PreprocessorConditionalState::Before:
        conditional->state = PreprocessorConditionalState::During;
        break;

    case PreprocessorConditionalState::During:
        conditional->state = PreprocessorConditionalState::After;
        break;

    default:
        break;
    }
}

// Handle a `#elif` directive
static void HandleElifDirective(PreprocessorDirectiveContext* context)
{
    PreprocessorExpressionValue value = ParseAndEvaluateExpression(context);

    PreprocessorInputStream* inputStream = context->preprocessor->inputStream;
    assert(inputStream);

    // if we aren't inside a conditional, then error
    PreprocessorConditional* conditional = inputStream->conditional;
    if (!conditional)
    {
        GetSink(context)->diagnose(GetDirectiveLoc(context), Diagnostics::directiveWithoutIf, GetDirectiveName(context));
        return;
    }

    // if we've already seen a `#else`, then it is an error
    if (conditional->elseToken.Type != TokenType::Unknown)
    {
        GetSink(context)->diagnose(GetDirectiveLoc(context), Diagnostics::directiveAfterElse, GetDirectiveName(context));
        GetSink(context)->diagnose(conditional->elseToken.Position, Diagnostics::seeDirective);
        return;
    }

    switch (conditional->state)
    {
    case PreprocessorConditionalState::Before:
        if(value)
            conditional->state = PreprocessorConditionalState::During;
        break;

    case PreprocessorConditionalState::During:
        conditional->state = PreprocessorConditionalState::After;
        break;

    default:
        break;
    }
}

// Handle a `#endif` directive
static void HandleEndIfDirective(PreprocessorDirectiveContext* context)
{
    PreprocessorInputStream* inputStream = context->preprocessor->inputStream;
    assert(inputStream);

    // if we aren't inside a conditional, then error
    PreprocessorConditional* conditional = inputStream->conditional;
    if (!conditional)
    {
        GetSink(context)->diagnose(GetDirectiveLoc(context), Diagnostics::directiveWithoutIf, GetDirectiveName(context));
        return;
    }

    inputStream->conditional = conditional->parent;
    DestroyConditional(conditional);
}

// Handle a `#include` directive
static void HandleIncludeDirective(PreprocessorDirectiveContext* context)
{
    Token pathToken;
    if(!Expect(context, TokenType::StringLiterial, Diagnostics::expectedTokenInPreprocessorDirective, &pathToken))
        return;
    String path = pathToken.Content;

    // TODO(tfoley): make this robust in presence of `#line`
    String pathIncludedFrom = GetDirectiveLoc(context).FileName;
    String foundPath;
    String foundSource;


    IncludeHandler* includeHandler = context->preprocessor->includeHandler;
    if (!includeHandler)
    {
        GetSink(context)->diagnose(pathToken.Position, Diagnostics::includeFailed, path);
        GetSink(context)->diagnose(pathToken.Position, Diagnostics::noIncludeHandlerSpecified);
        return;
    }
    if (!includeHandler->TryToFindIncludeFile(path, pathIncludedFrom, &foundPath, &foundSource))
    {
        GetSink(context)->diagnose(pathToken.Position, Diagnostics::includeFailed, path);
        return;
    }

    // Push the new file onto our stack of input streams
    // TODO(tfoley): check if we have made our include stack too deep
    PreprocessorInputStream* inputStream = CreateInputStreamForSource(context->preprocessor, foundSource, foundPath);
    inputStream->parent = context->preprocessor->inputStream;
    context->preprocessor->inputStream = inputStream;
}

// Handle a `#define` directive
static void HandleDefineDirective(PreprocessorDirectiveContext* context)
{
    Token nameToken;
    if (!Expect(context, TokenType::Identifier, Diagnostics::expectedTokenInPreprocessorDirective, &nameToken))
        return;
    String name = nameToken.Content;

    PreprocessorMacro* macro = CreateMacro(context->preprocessor);
    macro->nameToken = nameToken;

    PreprocessorMacro* oldMacro = LookupMacro(context, name);
    if (oldMacro)
    {
        GetSink(context)->diagnose(nameToken.Position, Diagnostics::macroRedefinition, name);
        GetSink(context)->diagnose(oldMacro->nameToken.Position, Diagnostics::seePreviousDefinitionOf, name);

        DestroyMacro(context->preprocessor, oldMacro);
    }
    context->preprocessor->macros[name] = macro;

    if (PeekRawTokenType(context) == TokenType::LParent)
    {
        if (!(PeekRawToken(context).flags & TokenFlag::AfterWhitespace))
        {
            // function-style macro
            SPIRE_UNIMPLEMENTED(GetSink(context), GetDirectiveLoc(context), "function-style macros");
            return;
        }
    }

    // consume tokens until end-of-line
    for(;;)
    {
        Token token = AdvanceRawToken(context);
        if (token.Type == TokenType::EndOfFile)
            break;
        macro->tokens.Add(token);
    }
}

// Handle a `#undef` directive
static void HandleUndefDirective(PreprocessorDirectiveContext* context)
{
    Token nameToken;
    if (!Expect(context, TokenType::Identifier, Diagnostics::expectedTokenInPreprocessorDirective, &nameToken))
        return;
    String name = nameToken.Content;

    PreprocessorMacro* macro = LookupMacro(context, name);
    if (macro != NULL)
    {
        // name was defined, so remove it
        context->preprocessor->macros.Remove(name);

        DestroyMacro(context->preprocessor, macro);
    }
    else
    {
        // name wasn't defined
        GetSink(context)->diagnose(nameToken.Position, Diagnostics::macroNotDefined, name);
    }
}

// Handle a `#warning` directive
static void HandleWarningDirective(PreprocessorDirectiveContext* context)
{
    // TODO: read rest of line without actual tokenization
    GetSink(context)->diagnose(GetDirectiveLoc(context), Diagnostics::userDefinedWarning, "user-defined warning");
    SkipToEndOfLine(context);
}

// Handle a `#error` directive
static void HandleErrorDirective(PreprocessorDirectiveContext* context)
{
    // TODO: read rest of line without actual tokenization
    GetSink(context)->diagnose(GetDirectiveLoc(context), Diagnostics::userDefinedError, "user-defined warning");
    SkipToEndOfLine(context);
}

// Handle a `#line` directive
static void HandleLineDirective(PreprocessorDirectiveContext* context)
{
    int line = 0;
    if (PeekTokenType(context) == TokenType::IntLiterial)
    {
        line = StringToInt(AdvanceToken(context).Content);
    }
    else if (PeekTokenType(context) == TokenType::Identifier
        && PeekToken(context).Content == "default")
    {
        AdvanceToken(context);

        // TODO(tfoley): reset line numbering here
        return;
    }
    else
    {
        GetSink(context)->diagnose(PeekLoc(context), Diagnostics::expected2TokensInPreprocessorDirective,
            TokenType::IntLiterial,
            "default",
            GetDirectiveName(context));
        context->parseError = true;
        return;
    }

    if (PeekTokenType(context) == TokenType::EndOfFile)
    {
        // TODO(tfoley): set line number, but not file
        return;
    }

    String file;
    if (PeekTokenType(context) == TokenType::StringLiterial)
    {
        file = AdvanceToken(context).Content;
    }
    else if (PeekTokenType(context) == TokenType::IntLiterial)
    {
        // Note(tfoley): GLSL allows the "source string" to be indicated by an integer
        // TODO(tfoley): Figure out a better way to handle this, if it matters
        file = AdvanceToken(context).Content;
    }
    else
    {
        Expect(context, TokenType::StringLiterial, Diagnostics::expectedTokenInPreprocessorDirective);
        return;
    }

    // TODO(tfoley): set line number and file here
}

// Handle a `#pragma` directive
static void HandlePragmaDirective(PreprocessorDirectiveContext* context)
{
    // TODO(tfoley): figure out which pragmas to parse,
    // and which to pass along
    SkipToEndOfLine(context);
}

// Callback interface used by preprocessor directives
typedef void (*PreprocessorDirectiveCallback)(PreprocessorDirectiveContext* context);

enum PreprocessorDirectiveFlag : unsigned int
{
    // Should this directive be handled even when skipping disbaled code?
    ProcessWhenSkipping = 1 << 0,
};

// Information about a specific directive
struct PreprocessorDirective
{
    // Name of the directive
    char const*                     name;

    // Callback to handle the directive
    PreprocessorDirectiveCallback   callback;

    unsigned int                    flags;
};

// A simple array of all the directives we know how to handle.
// TODO(tfoley): considering making this into a real hash map,
// and then make it easy-ish for users of the codebase to add
// their own directives as desired.
static const PreprocessorDirective kDirectives[] =
{
    { "if",         &HandleIfDirective,        ProcessWhenSkipping },
    { "ifdef",      &HandleIfDefDirective,     ProcessWhenSkipping },
    { "ifndef",     &HandleIfNDefDirective,    ProcessWhenSkipping },
    { "else",       &HandleElseDirective,      ProcessWhenSkipping },
    { "elif",       &HandleElifDirective,      ProcessWhenSkipping },
    { "endif",      &HandleEndIfDirective,     ProcessWhenSkipping },

    { "include",    &HandleIncludeDirective,   0 },
    { "define",     &HandleDefineDirective,    0 },
    { "undef",      &HandleUndefDirective,     0 },
    { "warning",    &HandleWarningDirective,   0 },
    { "error",      &HandleErrorDirective,     0 },
    { "line",       &HandleLineDirective,      0 },
    { "pragma",     &HandlePragmaDirective,    0 },
    { NULL, NULL },
};

// Look up the directive with the given name.
static PreprocessorDirective const* FindDirective(String const& name)
{
    char const* nameStr = name.Buffer();
    for (int ii = 0; kDirectives[ii].name; ++ii)
    {
        if (strcmp(kDirectives[ii].name, nameStr) != 0)
            continue;

        return &kDirectives[ii];
    }

    return NULL;
}

// Process a directive, where the preprocessor has already consumed the
// `#` token that started the directive line.
static void HandleDirective(PreprocessorDirectiveContext* context)
{
    // Try to read the directive name.
    context->directiveToken = PeekRawToken(context);

    CoreLib::Text::TokenType directiveTokenType = GetDirective(context).Type;

    // An empty directive is allowed, and ignored.
    if (directiveTokenType == TokenType::EndOfFile)
    {
        return;
    }
    // Otherwise the directive name had better be an identifier
    else if (directiveTokenType != TokenType::Identifier)
    {
        GetSink(context)->diagnose(GetDirectiveLoc(context), Diagnostics::expectedPreprocessorDirectiveName);
        SkipToEndOfLine(context);
        return;
    }

    // Consume the directive name token.
    AdvanceRawToken(context);

    // Look up the handler for the directive.
    PreprocessorDirective const* directive = FindDirective(GetDirectiveName(context));
    if (!directive)
    {
        GetSink(context)->diagnose(GetDirectiveLoc(context), Diagnostics::unknownPreprocessorDirective, GetDirectiveName(context));
        SkipToEndOfLine(context);
        return;
    }

    // If we are skipping disabled code, and the directive is not one
    // of the small number that need to run even in that case, skip it.
    if (IsSkipping(context) && !(directive->flags & PreprocessorDirectiveFlag::ProcessWhenSkipping))
    {
        SkipToEndOfLine(context);
        return;
    }

    // Apply the directive-specific callback
    (directive->callback)(context);

    // We expect the directive to consume the entire line, so if
    // it hasn't that is a parse error.
    if (!IsEndOfLine(context))
    {
        // If we already saw a previous parse error, then don't
        // emit another one for the same directive.
        if (!context->parseError)
        {
            GetSink(context)->diagnose(PeekLoc(context), Diagnostics::unexpectedTokensAfterDirective, GetDirectiveName(context));
        }
        SkipToEndOfLine(context);
    }
}

// Read one token using the full preprocessor, with all its behaviors.
static Token ReadToken(Preprocessor* preprocessor)
{
    for (;;)
    {
        // Look at the next raw token in the input.
        Token const& token = PeekRawToken(preprocessor);

        // If we have a directive (`#` at start of line) then handle it
        if ((token.Type == TokenType::Pound) && (token.flags & TokenFlag::AtStartOfLine))
        {
            // Skip the `#`
            AdvanceRawToken(preprocessor);

            // Create a context for parsing the directive
            PreprocessorDirectiveContext directiveContext;
            directiveContext.preprocessor = preprocessor;
            directiveContext.parseError = false;

            // Parse and handle the directive
            HandleDirective(&directiveContext);
            continue;
        }

        // otherwise, if we are currently in a skipping mode, then skip tokens
        if (IsSkipping(preprocessor))
        {
            AdvanceRawToken(preprocessor);
            continue;
        }

        // otherwise read a token, which may involve macro expansion
        return AdvanceToken(preprocessor);
    }
}

// Take a string of source code and preprocess it into a list of tokens.
CoreLib::List<CoreLib::Text::Token> PreprocessSource(
    CoreLib::String const& source,
    CoreLib::String const& fileName,
    DiagnosticSink* sink,
    IncludeHandler* includeHandler)
{
    Preprocessor preprocessor;
    preprocessor.sink = sink;
    preprocessor.includeHandler = includeHandler;
    preprocessor.endOfFileToken.Type = TokenType::EndOfFile;
    preprocessor.endOfFileToken.flags = TokenFlag::AtStartOfLine;

    // create an initial input stream based on the provided buffer
    preprocessor.inputStream = CreateInputStreamForSource(&preprocessor, source, fileName);

    List<Token> tokens;
    for (;;)
    {
        Token token = ReadToken(&preprocessor);
        // TODO(tfoley): should just include EOF token in output
        if (token.Type == TokenType::EndOfFile)
            break;

        tokens.Add(token);
    }

    // clean up any macros that were allocated
    for (auto pair : preprocessor.macros)
    {
        DestroyMacro(&preprocessor, pair.Value);
    }

    return tokens;
}

}}
