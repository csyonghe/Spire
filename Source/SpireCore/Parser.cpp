#include "Parser.h"

#include <assert.h>

namespace Spire
{
	namespace Compiler
	{
		const int MaxExprLevel = 12;

		// TODO: implement two pass parsing for file reference and struct type recognition

		class Parser
		{
        public:
			int anonymousParamCounter = 0;
			RefPtr<ContainerDecl> currentScope;
            TokenReader tokenReader;
            DiagnosticSink * sink;
			String fileName;
			HashSet<String> typeNames;
			HashSet<String> classNames;
			bool isInImportOperator = false;
			int genericDepth = 0;

            // Is the parser in a "recovering" state?
            // During recovery we don't emit additional errors, until we find
            // a token that we expected, when we exit recovery.
            bool isRecovering = false;

			void FillPosition(SyntaxNode * node)
			{
				node->Position = tokenReader.PeekLoc();
			}
			void PushScope(ContainerDecl* containerDecl)
			{
				containerDecl->ParentDecl = currentScope.Ptr();
				currentScope = containerDecl;
			}
			void PopScope()
			{
				currentScope = currentScope->ParentDecl;
			}
			Parser(TokenSpan const& _tokens, DiagnosticSink * sink, String _fileName)
				: tokenReader(_tokens), sink(sink), fileName(_fileName)
			{
				typeNames.Add("int");
				typeNames.Add("uint");
				typeNames.Add("bool");
				typeNames.Add("float");
				typeNames.Add("half");
				typeNames.Add("void");
				typeNames.Add("ivec2");
				typeNames.Add("ivec3");
				typeNames.Add("ivec4");
				typeNames.Add("uvec2");
				typeNames.Add("uvec3");
				typeNames.Add("uvec4");
				typeNames.Add("vec2");
				typeNames.Add("vec3");
				typeNames.Add("vec4");
				typeNames.Add("mat3");
				typeNames.Add("mat4");
				typeNames.Add("mat4x4");
				typeNames.Add("mat3x3");
				typeNames.Add("int2");
				typeNames.Add("int3");
				typeNames.Add("int4");
				typeNames.Add("uint2");
				typeNames.Add("uint3");
				typeNames.Add("uint4");
				typeNames.Add("float2");
				typeNames.Add("float3");
				typeNames.Add("float4");
				typeNames.Add("half2");
				typeNames.Add("half3");
				typeNames.Add("half4");
				typeNames.Add("float3x3");
				typeNames.Add("float4x4");
				typeNames.Add("half3x3");
				typeNames.Add("half4x4");
				typeNames.Add("Texture1D");
				typeNames.Add("Texture2D");
				typeNames.Add("Texture2DArray");
				typeNames.Add("Texture2DArrayShadow");
				typeNames.Add("TextureCube");
				typeNames.Add("TextureCubeShadow");
				typeNames.Add("Texture3D");
				typeNames.Add("texture");
				typeNames.Add("Texture");
				typeNames.Add("sampler");
				typeNames.Add("SamplerState");
				typeNames.Add("SamplerComparisonState");
				typeNames.Add("sampler_state");
				typeNames.Add("Uniform");
				typeNames.Add("StructuredBuffer");
				typeNames.Add("RWStructuredBuffer");
				typeNames.Add("PackedBuffer");
				typeNames.Add("StorageBuffer");
				typeNames.Add("Patch");
			}
			RefPtr<ProgramSyntaxNode> Parse(ProgramSyntaxNode*	predefUnit);

			Token ReadToken();
			Token ReadToken(CoreLib::Text::TokenType type);
			Token ReadToken(const char * string);
			bool LookAheadToken(CoreLib::Text::TokenType type, int offset = 0);
			bool LookAheadToken(const char * string, int offset = 0);
			Token ReadTypeKeyword();
			bool IsTypeKeyword();
			RefPtr<ProgramSyntaxNode>					ParseProgram(ProgramSyntaxNode*	predefUnit);
			RefPtr<ShaderSyntaxNode>					ParseShader();
			RefPtr<TemplateShaderSyntaxNode>			ParseTemplateShader();
			RefPtr<TemplateShaderParameterSyntaxNode>	ParseTemplateShaderParameter();
			RefPtr<InterfaceSyntaxNode>					ParseInterface();
			RefPtr<PipelineSyntaxNode>					ParsePipeline();
			RefPtr<StageSyntaxNode>						ParseStage();
			RefPtr<WorldSyntaxNode>						ParseWorld();
			RefPtr<RateSyntaxNode>						ParseRate();
			RefPtr<ImportSyntaxNode>					ParseImportInner();
			RefPtr<ImportStatementSyntaxNode>			ParseImportStatement();
			RefPtr<ImportOperatorDefSyntaxNode>			ParseImportOperator();
			RefPtr<FunctionSyntaxNode>					ParseFunction(bool parseBody = true);
			RefPtr<StructSyntaxNode>					ParseStruct();
			RefPtr<StatementSyntaxNode>					ParseStatement();
			RefPtr<BlockStatementSyntaxNode>			ParseBlockStatement();
			RefPtr<VarDeclrStatementSyntaxNode>			ParseVarDeclrStatement(Modifiers modifiers);
			RefPtr<IfStatementSyntaxNode>				ParseIfStatement();
			RefPtr<ForStatementSyntaxNode>				ParseForStatement();
			RefPtr<WhileStatementSyntaxNode>			ParseWhileStatement();
			RefPtr<DoWhileStatementSyntaxNode>			ParseDoWhileStatement();
			RefPtr<BreakStatementSyntaxNode>			ParseBreakStatement();
			RefPtr<ContinueStatementSyntaxNode>			ParseContinueStatement();
			RefPtr<ReturnStatementSyntaxNode>			ParseReturnStatement();
			RefPtr<ExpressionStatementSyntaxNode>		ParseExpressionStatement();
			RefPtr<ExpressionSyntaxNode>				ParseExpression(int level = 0);
			RefPtr<ExpressionSyntaxNode>				ParseLeafExpression();
			RefPtr<ParameterSyntaxNode>					ParseParameter();
			RefPtr<ExpressionSyntaxNode>				ParseType();
			TypeExp										ParseTypeExp();

			Parser & operator = (const Parser &) = delete;
		};

		// Forward Declarations

		static void ParseDeclBody(
			Parser*						parser,
			ContainerDecl*				containerDecl,
			CoreLib::Text::TokenType	closingToken);

		static void ParseOptSemantics(
			Parser* parser,
			Decl*	decl);

		static RefPtr<Decl> ParseDecl(
			Parser*			parser,
			ContainerDecl*	containerDecl);

		//

        static void Unexpected(
            Parser*     parser)
        {
            // Don't emit "unexpected token" errors if we are in recovering mode
            if (!parser->isRecovering)
            {
                parser->sink->diagnose(parser->tokenReader.PeekLoc(), Diagnostics::unexpectedToken,
                    parser->tokenReader.PeekTokenType());

                // Switch into recovery mode, to suppress additional errors
                parser->isRecovering = true;
            }
        }

        static void Unexpected(
            Parser*     parser,
            char const* expected)
        {
            // Don't emit "unexpected token" errors if we are in recovering mode
            if (!parser->isRecovering)
            {
                parser->sink->diagnose(parser->tokenReader.PeekLoc(), Diagnostics::unexpectedTokenExpectedTokenName,
                    parser->tokenReader.PeekTokenType(),
                    expected);

                // Switch into recovery mode, to suppress additional errors
                parser->isRecovering = true;
            }
        }

        static void Unexpected(
            Parser*                     parser,
            CoreLib::Text::TokenType    expected)
        {
            // Don't emit "unexpected token" errors if we are in recovering mode
            if (!parser->isRecovering)
            {
                parser->sink->diagnose(parser->tokenReader.PeekLoc(), Diagnostics::unexpectedTokenExpectedTokenType,
                    parser->tokenReader.PeekTokenType(),
                    expected);

                // Switch into recovery mode, to suppress additional errors
                parser->isRecovering = true;
            }
        }

        static CoreLib::Text::TokenType SkipToMatchingToken(TokenReader* reader, CoreLib::Text::TokenType tokenType);

        // Skip a singel balanced token, which is either a single token in
        // the common case, or a matched pair of tokens for `()`, `[]`, and `{}`
        static CoreLib::Text::TokenType SkipBalancedToken(
            TokenReader* reader)
        {
            CoreLib::Text::TokenType tokenType = reader->AdvanceToken().Type;
            switch (tokenType)
            {
            default:
                break;

            case TokenType::LParent:    tokenType = SkipToMatchingToken(reader, TokenType::RParent);    break;
            case TokenType::LBrace:     tokenType = SkipToMatchingToken(reader, TokenType::RBrace);     break;
            case TokenType::LBracket:   tokenType = SkipToMatchingToken(reader, TokenType::RBracket);   break;
            }
            return tokenType;
        }

        // Skip balanced 
        static CoreLib::Text::TokenType SkipToMatchingToken(
            TokenReader*                reader,
            CoreLib::Text::TokenType    tokenType)
        {
            for (;;)
            {
                if (reader->IsAtEnd()) return TokenType::EndOfFile;
                if (reader->PeekTokenType() == tokenType)
                {
                    reader->AdvanceToken();
                    return tokenType;
                }
                SkipBalancedToken(reader);
            }
        }

        // Is the given token type one that is used to "close" a
        // balanced construct.
        static bool IsClosingToken(CoreLib::Text::TokenType tokenType)
        {
            switch (tokenType)
            {
            case TokenType::EndOfFile:
            case TokenType::RBracket:
            case TokenType::RParent:
            case TokenType::RBrace:
                return true;

            default:
                return false;
            }
        }


        // Expect an identifier token with the given content, and consume it.
		Token Parser::ReadToken(const char* expected)
		{
            if (tokenReader.PeekTokenType() == TokenType::Identifier
                    && tokenReader.PeekToken().Content == expected)
            {
                isRecovering = false;
                return tokenReader.AdvanceToken();
            }

            if (!isRecovering)
            {
                Unexpected(this, expected);
                return tokenReader.PeekToken();
            }
            else
            {
                // Try to find a place to recover
                for (;;)
                {
                    // The token we expected?
                    // Then exit recovery mode and pretend like all is well.
                    if (tokenReader.PeekTokenType() == TokenType::Identifier
                        && tokenReader.PeekToken().Content == expected)
                    {
                        isRecovering = false;
                        return tokenReader.AdvanceToken();
                    }


                    // Don't skip past any "closing" tokens.
                    if (IsClosingToken(tokenReader.PeekTokenType()))
                    {
                        return tokenReader.PeekToken();
                    }

                    // Skip balanced tokens and try again.
                    SkipBalancedToken(&tokenReader);
                }
            }
		}

		Token Parser::ReadToken()
		{
			return tokenReader.AdvanceToken();
		}

        static bool TryRecover(
            Parser*                         parser,
            CoreLib::Text::TokenType const* recoverBefore,
            int                             recoverBeforeCount,
            CoreLib::Text::TokenType const* recoverAfter,
            int                             recoverAfterCount)
        {
            if (!parser->isRecovering)
                return true;

            // Determine if we are looking for a closing token at all...
            bool lookingForClose = false;
            for (int ii = 0; ii < recoverBeforeCount; ++ii)
            {
                if (IsClosingToken(recoverBefore[ii]))
                    lookingForClose = true;
            }
            for (int ii = 0; ii < recoverAfterCount; ++ii)
            {
                if (IsClosingToken(recoverAfter[ii]))
                    lookingForClose = true;
            }

            TokenReader* tokenReader = &parser->tokenReader;
            for (;;)
            {
                CoreLib::Text::TokenType peek = tokenReader->PeekTokenType();

                // Is the next token in our recover-before set?
                // If so, then we have recovered successfully!
                for (int ii = 0; ii < recoverBeforeCount; ++ii)
                {
                    if (peek == recoverBefore[ii])
                    {
                        parser->isRecovering = false;
                        return true;
                    }
                }

                // If we are looking at a token in our recover-after set,
                // then consume it and recover
                for (int ii = 0; ii < recoverAfterCount; ++ii)
                {
                    if (peek == recoverAfter[ii])
                    {
                        tokenReader->AdvanceToken();
                        parser->isRecovering = false;
                        return true;
                    }
                }

                // Don't try to skip past end of file
                if (peek == TokenType::EndOfFile)
                    return false;

                switch (peek)
                {
                // Don't skip past simple "closing" tokens, *unless*
                // we are looking for a closing token
                case TokenType::RParent:
                case TokenType::RBracket:
                    if (!lookingForClose)
                        return false;
                    break;

                // never skip a `}`, to avoid spurious errors
                case TokenType::RBrace:
                    return false;
                }

                // Skip balanced tokens and try again.
                CoreLib::Text::TokenType skipped = SkipBalancedToken(tokenReader);
                
                // If we happened to find a matched pair of tokens, and
                // the end of it was a token we were looking for,
                // then recover here
                for (int ii = 0; ii < recoverAfterCount; ++ii)
                {
                    if (skipped == recoverAfter[ii])
                    {
                        parser->isRecovering = false;
                        return true;
                    }
                }
            }
        }

        static bool TryRecoverBefore(
            Parser*                     parser,
            CoreLib::Text::TokenType    before0)
        {
            CoreLib::Text::TokenType recoverBefore[] = { before0 };
            return TryRecover(parser, recoverBefore, 1, nullptr, 0);
        }

        // Default recovery strategy, to use inside `{}`-delimeted blocks.
        static bool TryRecover(
            Parser*                     parser)
        {
            CoreLib::Text::TokenType recoverBefore[] = { TokenType::RBrace };
            CoreLib::Text::TokenType recoverAfter[] = { TokenType::Semicolon };
            return TryRecover(parser, recoverBefore, 1, recoverAfter, 1);
        }

		Token Parser::ReadToken(CoreLib::Text::TokenType expected)
		{
            if (tokenReader.PeekTokenType() == expected)
            {
                isRecovering = false;
                return tokenReader.AdvanceToken();
            }

            if (!isRecovering)
            {
                Unexpected(this, expected);
                return tokenReader.PeekToken();
            }
            else
            {
                // Try to find a place to recover
                if (TryRecoverBefore(this, expected))
                {
                    isRecovering = false;
                    return tokenReader.AdvanceToken();
                }

                return tokenReader.PeekToken();
            }
		}

		bool Parser::LookAheadToken(const char * string, int offset)
		{
            TokenReader r = tokenReader;
            for (int ii = 0; ii < offset; ++ii)
                r.AdvanceToken();

            return r.PeekTokenType() == TokenType::Identifier
                && r.PeekToken().Content == string;
	}

		bool Parser::LookAheadToken(CoreLib::Text::TokenType type, int offset)
		{
            TokenReader r = tokenReader;
            for (int ii = 0; ii < offset; ++ii)
                r.AdvanceToken();

            return r.PeekTokenType() == type;
		}

        // Consume a token and return true it if matches, otherwise false
        bool AdvanceIf(Parser* parser, CoreLib::Text::TokenType tokenType)
        {
            if (parser->LookAheadToken(tokenType))
            {
                parser->ReadToken();
                return true;
            }
            return false;
        }

        // Consume a token and return true it if matches, otherwise false
        bool AdvanceIf(Parser* parser, char const* text)
        {
            if (parser->LookAheadToken(text))
            {
                parser->ReadToken();
                return true;
            }
            return false;
        }

        // Consume a token and return true if it matches, otherwise check
        // for end-of-file and expect that token (potentially producing
        // an error) and return true to maintain forward progress.
        // Otherwise return false.
        bool AdvanceIfMatch(Parser* parser, CoreLib::Text::TokenType tokenType)
        {
            // If we've run into a syntax error, but haven't recovered inside
            // the block, then try to recover here.
            if (parser->isRecovering)
            {
                TryRecoverBefore(parser, tokenType);
            }
            if (AdvanceIf(parser, tokenType))
                return true;
            if (parser->tokenReader.PeekTokenType() == TokenType::EndOfFile)
            {
                parser->ReadToken(tokenType);
                return true;
            }
            return false;
        }

		Token Parser::ReadTypeKeyword()
		{
			if(!IsTypeKeyword())
			{
                if (!isRecovering)
                {
    				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::typeNameExpectedBut, tokenReader.PeekTokenType());
                }
                return tokenReader.PeekToken();
			}
			return tokenReader.AdvanceToken();
		}

		bool Parser::IsTypeKeyword()
		{
			return tokenReader.PeekTokenType() == TokenType::Identifier
                && typeNames.Contains(tokenReader.PeekToken().Content);
		}

		RefPtr<ProgramSyntaxNode> Parser::Parse(ProgramSyntaxNode*	predefUnit)
		{
			return ParseProgram(predefUnit);
		}

        RefPtr<TypeDefDecl> ParseTypeDef(Parser* parser)
        {
            // Consume the `typedef` keyword
            parser->ReadToken("typedef");

            // TODO(tfoley): parse an actual declarator
            auto type = parser->ParseTypeExp();

            auto nameToken = parser->ReadToken(TokenType::Identifier);

            RefPtr<TypeDefDecl> typeDefDecl = new TypeDefDecl();
            typeDefDecl->Name = nameToken;
            typeDefDecl->Type = type;

			parser->typeNames.Add(nameToken.Content);

            return typeDefDecl;
        }

		// Add a modifier to a list of modifiers being built
		static void AddModifier(RefPtr<Modifier>** ioModifierLink, RefPtr<Modifier> modifier)
		{
			RefPtr<Modifier>*& modifierLink = *ioModifierLink;

			*modifierLink = modifier;
			modifierLink = &modifier->next;
		}

		static void ParseSquareBracketAttributes(Parser* parser, RefPtr<Modifier>** ioModifierLink)
		{
			parser->ReadToken(TokenType::LBracket);
			for(;;)
			{
				auto nameToken = parser->ReadToken(TokenType::Identifier);
				if (AdvanceIf(parser, TokenType::Colon))
				{
					// Spire-style `[key:value]` attribute
					Token valueToken = parser->ReadToken(TokenType::StringLiterial);

					RefPtr<SimpleAttribute> modifier = new SimpleAttribute();
					modifier->Key = nameToken.Content;
					modifier->Value = valueToken;

					AddModifier(ioModifierLink, modifier);
				}
				else if (AdvanceIf(parser, TokenType::LParent))
				{
					// HLSL-style `[name(arg0, ...)]` attribute
					RefPtr<HLSLUncheckedAttribute> modifier = new HLSLUncheckedAttribute();
					modifier->nameToken = nameToken;

					while (!AdvanceIfMatch(parser, TokenType::RParent))
					{
						auto arg = parser->ParseExpression();
						if (arg)
						{
							modifier->args.Add(arg);
						}

						if (AdvanceIfMatch(parser, TokenType::RParent))
							break;

						parser->ReadToken(TokenType::Comma);
					}

					AddModifier(ioModifierLink, modifier);
				}
				else
				{
					// default case, just `[key]`

					// For now we parse this into the Spire-defined AST node,
					// but we might eventually want to make the HLSL case the default
					RefPtr<SimpleAttribute> modifier = new SimpleAttribute();
					modifier->Key = nameToken.Content;
					AddModifier(ioModifierLink, modifier);
				}


				if (AdvanceIfMatch(parser, TokenType::RBracket))
					break;

				parser->ReadToken(TokenType::Comma);
			}
		}

        static Modifiers ParseModifiers(Parser* parser)
        {
            Modifiers modifiers;
            RefPtr<Modifier>* modifierLink = &modifiers.first;
            for (;;)
            {
                if (AdvanceIf(parser, "in"))
                {
                    modifiers.flags |= ModifierFlag::In;
                }
                else if (AdvanceIf(parser, "input"))
                {
                    modifiers.flags |= ModifierFlag::Input;
                }
                else if (AdvanceIf(parser, "out"))
                {
                    modifiers.flags |= ModifierFlag::Out;
                }
                else if (AdvanceIf(parser, "inout"))
                {
                    modifiers.flags |= ModifierFlag::InOut;
                }
                else if (AdvanceIf(parser, "uniform"))
                {
                    modifiers.flags |= ModifierFlag::Uniform;
                }
                else if (AdvanceIf(parser, "const"))
                {
                    modifiers.flags |= ModifierFlag::Const;
                }
                else if (AdvanceIf(parser, "instance"))
                {
                    modifiers.flags |= ModifierFlag::Instance;
                }
                else if (AdvanceIf(parser, "__builtin"))
                {
                    modifiers.flags |= ModifierFlag::Builtin;
                }
                else if (AdvanceIf(parser, "layout"))
				{
					parser->ReadToken(TokenType::LParent);
					StringBuilder layoutSB;
					while (!AdvanceIfMatch(parser, TokenType::RParent))
					{
						layoutSB.Append(parser->ReadToken(TokenType::Identifier).Content);
						if (parser->LookAheadToken(TokenType::OpAssign))
						{
							layoutSB.Append(parser->ReadToken(TokenType::OpAssign).Content);
							layoutSB.Append(parser->ReadToken(TokenType::IntLiterial).Content);
						}
						if (AdvanceIf(parser, TokenType::RParent))
							break;
						parser->ReadToken(TokenType::Comma);
						layoutSB.Append(", ");
					}

                    RefPtr<LayoutModifier> modifier = new LayoutModifier();
                    modifier->LayoutString = layoutSB.ProduceString();

					AddModifier(&modifierLink, modifier);
				}
				else if (AdvanceIf(parser, "specialize"))
				{
					RefPtr<SpecializeModifier> modifier = new SpecializeModifier();
					if (AdvanceIf(parser, TokenType::LParent))
					{
						while (!AdvanceIfMatch(parser, TokenType::RParent))
						{
							auto expr = parser->ParseExpression();
							modifier->Values.Add(expr);
							if (AdvanceIf(parser, TokenType::RParent))
								break;
							parser->ReadToken(TokenType::Comma);
						}
					}
					AddModifier(&modifierLink, modifier);
				}
                else if (AdvanceIf(parser, "inline"))
				{
					modifiers.flags |= ModifierFlag::Inline;
				}
				else if (AdvanceIf(parser, "public"))
				{
					modifiers.flags |= ModifierFlag::Public;
				}
				else if (AdvanceIf(parser, "require"))
				{
					modifiers.flags |= ModifierFlag::Require;
				}
				else if (AdvanceIf(parser, "param"))
				{
					modifiers.flags |= ModifierFlag::Param;
				}
				else if (AdvanceIf(parser, "extern"))
				{
					modifiers.flags |= ModifierFlag::Extern;
				}
				else if (parser->tokenReader.PeekTokenType() == TokenType::LBracket)
				{
					ParseSquareBracketAttributes(parser, &modifierLink);
				}
                else if (AdvanceIf(parser, "__intrinsic"))
                {
                    modifiers.flags |= ModifierFlag::Intrinsic;
                }
				else if (AdvanceIf(parser,"__builtin_type"))
				{
					RefPtr<BuiltinTypeModifier> modifier = new BuiltinTypeModifier();
					parser->ReadToken(TokenType::LParent);
					modifier->tag = BaseType(StringToInt(parser->ReadToken(TokenType::IntLiterial).Content));
					parser->ReadToken(TokenType::RParent);

					AddModifier(&modifierLink, modifier);
				}
				else if (AdvanceIf(parser,"__magic_type"))
				{
					RefPtr<MagicTypeModifier> modifier = new MagicTypeModifier();
					parser->ReadToken(TokenType::LParent);
					modifier->name = parser->ReadToken(TokenType::Identifier).Content;
					if (AdvanceIf(parser, TokenType::Comma))
					{
						modifier->tag = uint32_t(StringToInt(parser->ReadToken(TokenType::IntLiterial).Content));
					}
					parser->ReadToken(TokenType::RParent);

					AddModifier(&modifierLink, modifier);
				}

				#define CASE(KEYWORD, TYPE)						\
					else if(AdvanceIf(parser, #KEYWORD)) do {	\
						AddModifier(&modifierLink, new TYPE());	\
					} while(0)

				CASE(row_major, HLSLRowMajorLayoutModifier);
				CASE(column_major, HLSLColumnMajorLayoutModifier);

				CASE(nointerpolation, HLSLNoInterpolationModifier);
				CASE(precise, HLSLPreciseModifier);
				CASE(shared, HLSLEffectSharedModifier);
				CASE(groupshared, HLSLGroupSharedModifier);
				CASE(static, HLSLStaticModifier);
				CASE(uniform, HLSLUniformModifier);
				CASE(volatile, HLSLVolatileModifier);

				#undef CASE

				else if (AdvanceIf(parser, "row_major"))
				{
					RefPtr<HLSLRowMajorLayoutModifier> modifier = new HLSLRowMajorLayoutModifier();
					AddModifier(&modifierLink, modifier);
				}
				else if (AdvanceIf(parser, "column_major"))
				{
					RefPtr<HLSLColumnMajorLayoutModifier> modifier = new HLSLColumnMajorLayoutModifier();
					AddModifier(&modifierLink, modifier);
				}

                else
                {
                    // Done with modifier list
                    return modifiers;
                }
            }
        }

        static RefPtr<Decl> ParseUsing(
            Parser* parser)
        {
            parser->ReadToken("using");
            if (parser->tokenReader.PeekTokenType() == TokenType::StringLiterial)
            {
                auto usingDecl = new UsingFileDecl();
                usingDecl->fileName = parser->ReadToken(TokenType::StringLiterial);
                parser->ReadToken(TokenType::Semicolon);
                return usingDecl;
            }
            else
            {
                // This is an import decl
                return parser->ParseImportInner();
            }
        }

        static Token ParseDeclName(
            Parser* parser)
        {
            Token name;
            if (AdvanceIf(parser, "operator"))
			{
				name = parser->ReadToken();
				switch (name.Type)
				{
				case TokenType::OpAdd: case TokenType::OpSub: case TokenType::OpMul: case TokenType::OpDiv:
				case TokenType::OpMod: case TokenType::OpNot: case TokenType::OpBitNot: case TokenType::OpLsh: case TokenType::OpRsh:
				case TokenType::OpEql: case TokenType::OpNeq: case TokenType::OpGreater: case TokenType::OpLess: case TokenType::OpGeq:
				case TokenType::OpLeq: case TokenType::OpAnd: case TokenType::OpOr: case TokenType::OpBitXor: case TokenType::OpBitAnd:
				case TokenType::OpBitOr: case TokenType::OpInc: case TokenType::OpDec:
					break;
				default:
					parser->sink->diagnose(name.Position, Diagnostics::invalidOperator, name.Content);
					break;
				}
			}
			else
			{
				name = parser->ReadToken(TokenType::Identifier);
			}
            return name;
        }

		// A "declarator" as used in C-style languages
		struct Declarator : RefObject
		{
			// Different cases of declarator appear as "flavors" here
			enum class Flavor
			{
				Name,
				Pointer,
				Array,
			};
			Flavor flavor;
		};

		// The most common case of declarator uses a simple name
		struct NameDeclarator : Declarator
		{
			Token nameToken;
		};

		// A declarator that declares a pointer type
		struct PointerDeclarator : Declarator
		{
			// location of the `*` token
			CodePosition starLoc;

			RefPtr<Declarator>				inner;
		};

		// A declarator that declares an array type
		struct ArrayDeclarator : Declarator
		{
			RefPtr<Declarator>				inner;

			// location of the `[` token
			CodePosition openBracketLoc;

			// The expression that yields the element count, or NULL
			RefPtr<ExpressionSyntaxNode>	elementCountExpr;
		};

		// "Unwrapped" information about a declarator
        struct DeclaratorInfo
        {
            RefPtr<RateSyntaxNode>			rate;
            RefPtr<ExpressionSyntaxNode>	typeSpec;
            Token							nameToken;
        };

		// Add a member declaration to its container, and ensure that its
		// parent link is set up correctly.
		static void AddMember(RefPtr<ContainerDecl> container, RefPtr<Decl> member)
		{
			if (container)
			{
				member->ParentDecl = container.Ptr();
				container->Members.Add(member);
			}
		}

        static void ParseFuncDeclHeader(
            Parser*                     parser,
            DeclaratorInfo const&       declaratorInfo,
            RefPtr<FunctionSyntaxNode>  decl)
        {
            parser->PushScope(decl.Ptr());

            parser->anonymousParamCounter = 0;
            parser->FillPosition(decl.Ptr());
            decl->Position = declaratorInfo.nameToken.Position;

            decl->Name = declaratorInfo.nameToken;
            decl->ReturnType = TypeExp(declaratorInfo.typeSpec);
            parser->ReadToken(TokenType::LParent);
            while (!AdvanceIfMatch(parser, TokenType::RParent))
            {
				AddMember(decl, parser->ParseParameter());
                if (AdvanceIf(parser, TokenType::RParent))
                    break;
                parser->ReadToken(TokenType::Comma);
            }
			ParseOptSemantics(parser, decl.Ptr());
        }

        static void ParseFuncDeclHeader(
            Parser*                     parser,
            DeclaratorInfo const&       declaratorInfo,
            RefPtr<ComponentSyntaxNode> decl)
        {
            parser->PushScope(decl.Ptr());

            parser->anonymousParamCounter = 0;
            parser->FillPosition(decl.Ptr());
            decl->Position = declaratorInfo.nameToken.Position;

            decl->Name = declaratorInfo.nameToken;
            decl->Type = TypeExp(declaratorInfo.typeSpec);
            parser->ReadToken(TokenType::LParent);
            while (!AdvanceIfMatch(parser, TokenType::RParent))
            {
				AddMember(decl, parser->ParseParameter());
                if (AdvanceIf(parser, TokenType::RParent))
                    break;
                parser->ReadToken(TokenType::Comma);
            }
			ParseOptSemantics(parser, decl.Ptr());
        }

        static RefPtr<Decl> ParseFuncDecl(
            Parser*                 parser,
            ContainerDecl*          containerDecl,
            DeclaratorInfo const&   declaratorInfo)
        {
            if (dynamic_cast<ShaderDeclBase*>(containerDecl))
            {
                // inside a shader, we create a component decl
                RefPtr<ComponentSyntaxNode> decl = new ComponentSyntaxNode();
                ParseFuncDeclHeader(parser, declaratorInfo, decl);

                //
                decl->Rate = declaratorInfo.rate;
                //

                if (AdvanceIf(parser, TokenType::Semicolon))
                {
                    // empty body
                }
                else
                {
                    decl->BlockStatement = parser->ParseBlockStatement();
                }

                parser->PopScope();
                return decl;
            }
            else
            {
                // everywhere else, we create an ordinary `FunctionSyntaxNode`

                RefPtr<FunctionSyntaxNode> decl = new FunctionSyntaxNode();
                ParseFuncDeclHeader(parser, declaratorInfo, decl);

                if (AdvanceIf(parser, TokenType::Semicolon))
                {
                    // empty body
                }
                else
                {
                    decl->Body = parser->ParseBlockStatement();
                }

                parser->PopScope();
                return decl;
            }
        }

        static RefPtr<VarDeclBase> CreateVarDeclForContext(
            ContainerDecl*  containerDecl )
        {
            if (dynamic_cast<StructSyntaxNode*>(containerDecl))
            {
                return new StructField();
            }
            else if (dynamic_cast<FunctionDeclBase*>(containerDecl))
            {
                return new ParameterSyntaxNode();
            }
            else
            {
                return new Variable();
            }
        }

        static RefPtr<Decl> ParseVarDecl(
            Parser*                 parser,
            ContainerDecl*          containerDecl,
            DeclaratorInfo const&   declaratorInfo)
        {
            if (dynamic_cast<ShaderDeclBase*>(containerDecl))
            {
                // inside a shader, we create a component decl
                RefPtr<ComponentSyntaxNode> decl = new ComponentSyntaxNode();
                parser->FillPosition(decl.Ptr());
                decl->Position = declaratorInfo.nameToken.Position;

                //
                decl->Rate = declaratorInfo.rate;
                //

                decl->Name = declaratorInfo.nameToken;
                decl->Type = TypeExp(declaratorInfo.typeSpec);

				ParseOptSemantics(parser, decl.Ptr());

                // Note(tfoley): this case is the one place where a component
                // declaration differents in any meaningful way from an
                // ordinary variable declaration.
                if (parser->tokenReader.PeekTokenType() == TokenType::LBrace)
                {
                    decl->BlockStatement = parser->ParseBlockStatement();
                }
                else
                {
                    if (AdvanceIf(parser, TokenType::OpAssign))
                    {
                        decl->Expression = parser->ParseExpression();
                    }
                    // TODO(tfoley): support the block case here
                    parser->ReadToken(TokenType::Semicolon);
                }

                return decl;
            }
            else
            {
                // everywhere else, we create an ordinary `VarDeclBase`
                RefPtr<VarDeclBase> decl = CreateVarDeclForContext(containerDecl);
                parser->FillPosition(decl.Ptr());
                decl->Position = declaratorInfo.nameToken.Position;

                decl->Name = declaratorInfo.nameToken;
                decl->Type = TypeExp(declaratorInfo.typeSpec);

				ParseOptSemantics(parser, decl.Ptr());

                if (AdvanceIf(parser, TokenType::OpAssign))
                {
                    decl->Expr = parser->ParseExpression();
                }
                parser->ReadToken(TokenType::Semicolon);

                return decl;
            }
        }

		static RefPtr<Declarator> ParseSimpleDeclarator(
			Parser* parser)
		{
			switch( parser->tokenReader.PeekTokenType() )
			{
			case TokenType::Identifier:
				{
					auto nameDeclarator = new NameDeclarator();
					nameDeclarator->flavor = Declarator::Flavor::Name;
					nameDeclarator->nameToken = ParseDeclName(parser);
					return nameDeclarator;
				}
				break;

			default:
				// an empty declarator is allowed
				return nullptr;
			}
		}

		static RefPtr<Declarator> ParsePointerDeclarator(
			Parser* parser)
		{
			if( parser->tokenReader.PeekTokenType() == TokenType::OpMul )
			{
				auto ptrDeclarator = new PointerDeclarator();
				ptrDeclarator->starLoc = parser->tokenReader.PeekLoc();
				ptrDeclarator->flavor = Declarator::Flavor::Pointer;

				parser->ReadToken(TokenType::OpMul);
				ptrDeclarator->inner = ParsePointerDeclarator(parser);
				return ptrDeclarator;
			}
			else
			{
				return ParseSimpleDeclarator(parser);
			}
		}

		static RefPtr<Declarator> ParseArrayDeclarator(
			Parser* parser)
		{
			RefPtr<Declarator> declarator = ParseSimpleDeclarator(parser);
			while(parser->tokenReader.PeekTokenType() == TokenType::LBracket)
			{

				auto arrayDeclarator = new ArrayDeclarator();
				arrayDeclarator->openBracketLoc = parser->tokenReader.PeekLoc();
				arrayDeclarator->flavor = Declarator::Flavor::Array;
				arrayDeclarator->inner = declarator;

				parser->ReadToken(TokenType::LBracket);
				if( parser->tokenReader.PeekTokenType() != TokenType::RBracket )
				{
					arrayDeclarator->elementCountExpr = parser->ParseExpression();
				}
				parser->ReadToken(TokenType::RBracket);

				declarator = arrayDeclarator;
			}
			return declarator;
		}

		// Parse a declarator (or at least as much of one as we support)
		static RefPtr<Declarator> ParseDeclarator(
			Parser* parser)
		{
			return ParseArrayDeclarator(parser);
		}

		static void UnwrapDeclarator(
			RefPtr<Declarator>	declarator,
			DeclaratorInfo*		ioInfo)
		{
			while( declarator )
			{
				switch(declarator->flavor)
				{
				case Declarator::Flavor::Name:
					{
						auto nameDeclarator = (NameDeclarator*) declarator.Ptr();
						ioInfo->nameToken = nameDeclarator->nameToken;
						return;
					}
					break;

				case Declarator::Flavor::Pointer:
					{
						auto ptrDeclarator = (PointerDeclarator*) declarator.Ptr();

						// TODO(tfoley): we don't support pointers for now
						// ioInfo->typeSpec = new PointerTypeExpr(ioInfo->typeSpec);

						declarator = ptrDeclarator->inner;
					}
					break;

				case Declarator::Flavor::Array:
					{
						// TODO(tfoley): we don't support pointers for now
						auto arrayDeclarator = (ArrayDeclarator*) declarator.Ptr();

						auto arrayTypeExpr = new IndexExpressionSyntaxNode();
						arrayTypeExpr->Position = arrayDeclarator->openBracketLoc;
						arrayTypeExpr->BaseExpression = ioInfo->typeSpec;
						arrayTypeExpr->IndexExpression = arrayDeclarator->elementCountExpr;
						ioInfo->typeSpec = arrayTypeExpr;

						declarator = arrayDeclarator->inner;
					}
					break;

				default:
					SPIRE_UNREACHABLE("all cases handled");
					break;
				}
			}
		}


        static RefPtr<Decl> ParseDeclaratorDecl(
            Parser*         parser,
            ContainerDecl*  containerDecl)
        {
            DeclaratorInfo declaratorInfo;

            // For now we just parse <type-spec> <decl-name>
            //
            // TODO(tfoley): Actual C-style declarator-based parsed.
            //
            if (parser->tokenReader.PeekTokenType() == TokenType::At)
            {
                declaratorInfo.rate = parser->ParseRate();
            }
            declaratorInfo.typeSpec = parser->ParseType();


			RefPtr<Declarator> declarator = ParseDeclarator(parser);
			UnwrapDeclarator(declarator, &declaratorInfo);

            // Look at the token after the declarator to disambiguate
			//
			// Note(tfoley): the correct approach would be to parse function parameters
			// as part of a function declarator, and then disambiguate on that result.
            switch (parser->tokenReader.PeekTokenType())
            {
            case TokenType::LParent:
                // It must be a function
                return ParseFuncDecl(parser, containerDecl, declaratorInfo);

            default:
                // Assume it is a variable-like declaration
                return ParseVarDecl(parser, containerDecl, declaratorInfo);
            }
        }

		//
		// layout-semantic ::= (register | packoffset) '(' register-name component-mask? ')'
		// register-name ::= identifier
		// component-mask ::= '.' identifier
		//
		static void ParseHLSLLayoutSemantic(
			Parser*				parser,
			HLSLLayoutSemantic*	semantic)
		{
			semantic->name = parser->ReadToken(TokenType::Identifier);

			parser->ReadToken(TokenType::LParent);
			semantic->registerName = parser->ReadToken(TokenType::Identifier);
			if (AdvanceIf(parser, TokenType::Dot))
			{
				semantic->componentMask = parser->ReadToken(TokenType::Identifier);
			}
			parser->ReadToken(TokenType::RParent);
		}

		//
		// semantic ::= identifier ( '(' args ')' )?
		//
		static RefPtr<Modifier> ParseSemantic(
			Parser* parser)
		{
			if (parser->LookAheadToken("register"))
			{
				RefPtr<HLSLRegisterSemantic> semantic = new HLSLRegisterSemantic();
				ParseHLSLLayoutSemantic(parser, semantic.Ptr());
				return semantic;
			}
			else if (parser->LookAheadToken("packoffset"))
			{
				RefPtr<HLSLPackOffsetSemantic> semantic = new HLSLPackOffsetSemantic();
				ParseHLSLLayoutSemantic(parser, semantic.Ptr());
				return semantic;
			}
			else
			{
				RefPtr<HLSLSimpleSemantic> semantic = new HLSLSimpleSemantic();
				semantic->name = parser->ReadToken(TokenType::Identifier);
				return semantic;
			}
		}

		//
		// opt-semantics ::= (':' semantic)*
		//
		static void ParseOptSemantics(
			Parser* parser,
			Decl*	decl)
		{
			if (!AdvanceIf(parser, TokenType::Colon))
				return;

			RefPtr<Modifier>* link = &decl->modifiers.first;
			assert(!*link);

			for (;;)
			{
				RefPtr<Modifier> semantic = ParseSemantic(parser);
				if (semantic)
				{
					*link = semantic;
					link = &semantic->next;
				}

				switch (parser->tokenReader.PeekTokenType())
				{
				case TokenType::LBrace:
				case TokenType::Semicolon:
				case TokenType::Comma:
				case TokenType::RParent:
				case TokenType::EndOfFile:
					return;

				default:
					break;
				}

				parser->ReadToken(TokenType::Colon);
			}
		}

		static RefPtr<Decl> ParseHLSLBufferDecl(
			Parser*	parser)
		{
			// An HLSL declaration of a constant buffer like this:
			//
			//     cbuffer Foo : register(b0) { int a; float b; };
			//
			// is treated as syntax sugar for a type declaration
			// and then a global variable declaration using that type:
			//
			//     struct Foo { int a; float b; };
			//     ConstantBuffer<Foo> $anonymous;
			//
			// where `$anonymous` is a fresh name, and the variable
			// declaration is made to be "transparent" so that lookup
			// will see through it to the members inside.

			// We first look at the declaration keywrod to determine
			// the type of buffer to declare:
			String bufferWrapperTypeName;
			CodePosition bufferWrapperTypeNamePos = parser->tokenReader.PeekLoc();
			if (AdvanceIf(parser, "cbuffer"))
			{
				bufferWrapperTypeName = "ConstantBuffer";
			}
			else if (AdvanceIf(parser, "tbuffer"))
			{
				bufferWrapperTypeName = "TextureBuffer";
			}
			else
			{
				Unexpected(parser);
			}

			// We are going to represent each buffer as a pair of declarations.
			// The first is a type declaration that holds all the members, while
			// the second is a variable declaration that uses the buffer type.
			RefPtr<StructSyntaxNode> bufferDataTypeDecl = new StructSyntaxNode();
			RefPtr<Variable> bufferVarDecl = new Variable();

			// Both declarations will have a location that points to the name
			parser->FillPosition(bufferDataTypeDecl.Ptr());
			parser->FillPosition(bufferVarDecl.Ptr());

			// Only the type declaration will actually be named, and it will
			// use the given name to identify itself.
			bufferDataTypeDecl->Name = parser->ReadToken(TokenType::Identifier);

			// TODO(tfoley): We end up constructing unchecked syntax here that
			// is expected to type check into the right form, but it might be
			// cleaner to have a more explicit desugaring pass where we parse
			// these constructs directly into the AST and *then* desugar them.

			// Construct a type expression to reference the buffer data type
			auto bufferDataTypeExpr = new VarExpressionSyntaxNode();
			bufferDataTypeExpr->Position = bufferDataTypeDecl->Position;
			bufferDataTypeExpr->Variable = bufferDataTypeDecl->Name.Content;
			bufferDataTypeExpr->scope = parser->currentScope.Ptr();

			// Construct a type exrpession to reference the type constructor
			auto bufferWrapperTypeExpr = new VarExpressionSyntaxNode();
			bufferWrapperTypeExpr->Position = bufferWrapperTypeNamePos;
			bufferWrapperTypeExpr->Variable = bufferWrapperTypeName;
			bufferWrapperTypeExpr->scope = parser->currentScope.Ptr();

			// Construct a type expression that represents the type for the variable,
			// which is the wrapper type applied to the data type
			auto bufferVarTypeExpr = new GenericAppExpr();
			bufferVarTypeExpr->Position = bufferVarDecl->Position;
			bufferVarTypeExpr->FunctionExpr = bufferWrapperTypeExpr;
			bufferVarTypeExpr->Arguments.Add(bufferDataTypeExpr);

			bufferVarDecl->Type.exp = bufferVarTypeExpr;

			// Any semantics applied to the bufer declaration are taken as applying
			// to the variable instead.
			ParseOptSemantics(parser, bufferVarDecl.Ptr());

			// The declarations in the body belong to the data type.
			parser->ReadToken(TokenType::LBrace);
			ParseDeclBody(parser, bufferDataTypeDecl.Ptr(), TokenType::RBrace);

			// All HLSL buffer declarations are "transparent" in that their
			// members are implicitly made visible in the parent scope.
			// We achieve this by applying the transparent modifier to the variable.
			bufferVarDecl->modifiers.flags |= ModifierFlag::Transparent;

			// Because we are constructing two declarations, we have a thorny
			// issue that were are only supposed to return one.
			// For now we handle this by adding the type declaration to
			// the current scope manually, and then returning the variable
			// declaration.
			//
			// Note: this means that any modifiers that have already been parsed
			// will get attached to the variable declaration, not the type.
			// There might be cases where we need to shuffle things around.

			AddMember(parser->currentScope, bufferDataTypeDecl);

			return bufferVarDecl;
		}

		static RefPtr<Decl> ParseGenericParamDecl(
			Parser* parser)
		{
			// simple syntax to introduce a value parameter
			if (AdvanceIf(parser, "let"))
			{
				// default case is a type parameter
				auto paramDecl = new GenericValueParamDecl();
				paramDecl->Name = parser->ReadToken(TokenType::Identifier);
				if (AdvanceIf(parser, TokenType::Colon))
				{
					paramDecl->Type = parser->ParseTypeExp();
				}
				if (AdvanceIf(parser, TokenType::OpAssign))
				{
					paramDecl->Expr = parser->ParseExpression();
				}
				return paramDecl;
			}
			else
			{
				// default case is a type parameter
				auto paramDecl = new GenericTypeParamDecl();
				paramDecl->Name = parser->ReadToken(TokenType::Identifier);
				if (AdvanceIf(parser, TokenType::Colon))
				{
					paramDecl->bound = parser->ParseTypeExp();
				}
				if (AdvanceIf(parser, TokenType::OpAssign))
				{
					paramDecl->initType = parser->ParseTypeExp();
				}
				return paramDecl;
			}
		}

		static RefPtr<Decl> ParseGenericDecl(
			Parser* parser)
		{
			RefPtr<GenericDecl> decl = new GenericDecl();
			parser->PushScope(decl.Ptr());
			parser->ReadToken("__generic");
			parser->ReadToken(TokenType::OpLess);
			parser->genericDepth++;
			while (!parser->LookAheadToken(TokenType::OpGreater))
			{
				AddMember(decl, ParseGenericParamDecl(parser));

				if (parser->LookAheadToken(TokenType::OpGreater))
					break;

				parser->ReadToken(TokenType::Comma);
			}
			parser->genericDepth--;
			parser->ReadToken(TokenType::OpGreater);

			decl->inner = ParseDecl(parser, decl.Ptr());

			// A generic decl hijacks the name of the declaration
			// it wraps, so that lookup can find it.
			decl->Name = decl->inner->Name;

			parser->PopScope();
			return decl;
		}

		static RefPtr<ExtensionDecl> ParseExtensionDecl(Parser* parser)
		{
			RefPtr<ExtensionDecl> decl = new ExtensionDecl();
			parser->FillPosition(decl.Ptr());
			parser->ReadToken("__extension");
			decl->targetType = parser->ParseTypeExp();
			parser->ReadToken(TokenType::LBrace);
            ParseDeclBody(parser, decl.Ptr(), TokenType::RBrace);
			return decl;
		}

		static RefPtr<TraitDecl> ParseTraitDecl(Parser* parser)
		{
			RefPtr<TraitDecl> decl = new TraitDecl();
			parser->FillPosition(decl.Ptr());
			parser->ReadToken("__trait");
			decl->Name = parser->ReadToken(TokenType::Identifier);

			if (AdvanceIf(parser, TokenType::Colon))
			{
				do
				{
					auto base = parser->ParseTypeExp();
					decl->bases.Add(base);
				} while (AdvanceIf(parser, TokenType::Comma));
			}

			parser->ReadToken(TokenType::LBrace);
			ParseDeclBody(parser, decl.Ptr(), TokenType::RBrace);
			return decl;
		}

		static RefPtr<ConstructorDecl> ParseConstructorDecl(Parser* parser)
		{
			RefPtr<ConstructorDecl> decl = new ConstructorDecl();
			parser->FillPosition(decl.Ptr());
			parser->ReadToken("__init");

            parser->ReadToken(TokenType::LParent);
            while (!AdvanceIfMatch(parser, TokenType::RParent))
            {
				AddMember(decl, parser->ParseParameter());
                if (AdvanceIf(parser, TokenType::RParent))
                    break;
                parser->ReadToken(TokenType::Comma);
            }

			if (AdvanceIf(parser, TokenType::Semicolon))
            {
                // empty body
            }
            else
            {
                decl->Body = parser->ParseBlockStatement();
            }
			return decl;
		}

        static RefPtr<Decl> ParseDeclWithModifiers(
            Parser*             parser,
            ContainerDecl*      containerDecl,
            Modifiers			modifiers )
        {
            RefPtr<Decl> decl;

            // TODO: actual dispatch!
			if (parser->LookAheadToken("shader") || parser->LookAheadToken("module"))
				decl = parser->ParseShader();
			else if (parser->LookAheadToken("template"))
				decl = parser->ParseTemplateShader();
			else if (parser->LookAheadToken("pipeline"))
				decl = parser->ParsePipeline();
			else if (parser->LookAheadToken("struct"))
				decl = parser->ParseStruct();
			else if (parser->LookAheadToken("typedef"))
				decl = ParseTypeDef(parser);
			else if (parser->LookAheadToken("using"))
				decl = ParseUsing(parser);
			else if (parser->LookAheadToken("world"))
				decl = parser->ParseWorld();
			else if (parser->LookAheadToken("import"))
				decl = parser->ParseImportOperator();
			else if (parser->LookAheadToken("stage"))
				decl = parser->ParseStage();
			else if (parser->LookAheadToken("interface"))
				decl = parser->ParseInterface();
			else if (parser->LookAheadToken("cbuffer") || parser->LookAheadToken("tbuffer"))
				decl = ParseHLSLBufferDecl(parser);
			else if (parser->LookAheadToken("__generic"))
				decl = ParseGenericDecl(parser);
			else if (parser->LookAheadToken("__extension"))
				decl = ParseExtensionDecl(parser);
			else if (parser->LookAheadToken("__init"))
				decl = ParseConstructorDecl(parser);
			else if (parser->LookAheadToken("__trait"))
				decl = ParseTraitDecl(parser);
            else if (AdvanceIf(parser, TokenType::Semicolon))
            {
                // empty declaration
            }
            else
                decl = ParseDeclaratorDecl(parser, containerDecl);

            if (decl)
            {
				// Combine the explicit modifiers with any extra stuff that came from the parsed declaration
				decl->modifiers.flags |= modifiers.flags;

				RefPtr<Modifier>* modifierLink = &modifiers.first;
				while (*modifierLink)
				{
					modifierLink = &(*modifierLink)->next;
				}

				*modifierLink = decl->modifiers.first;
				decl->modifiers.first = modifiers.first;

				if (containerDecl)
                {
					AddMember(containerDecl, decl);
                }
            }
            return decl;
        }

        static RefPtr<Decl> ParseDecl(
            Parser*         parser,
            ContainerDecl*  containerDecl)
        {
            Modifiers modifiers = ParseModifiers(parser);
            return ParseDeclWithModifiers(parser, containerDecl, modifiers);
        }

        // Parse a body consisting of declarations
        static void ParseDeclBody(
            Parser*         parser,
            ContainerDecl*  containerDecl,
            CoreLib::Text::TokenType       closingToken)
        {
            while(!AdvanceIfMatch(parser, closingToken))
            {
                RefPtr<Decl> decl = ParseDecl(parser, containerDecl);
                if (decl)
                {
                    decl->ParentDecl = containerDecl;
                    //containerDecl->Members.Add(decl);
                }
                TryRecover(parser);
            }
        }

		RefPtr<ProgramSyntaxNode> Parser::ParseProgram(ProgramSyntaxNode*	predefUnit)
		{
			if (predefUnit)
			{
				PushScope(predefUnit);
			}

			RefPtr<ProgramSyntaxNode> program = new ProgramSyntaxNode();
			if (predefUnit)
			{
				program->ParentDecl = predefUnit;
			}

			PushScope(program.Ptr());
			program->Position = CodePosition(0, 0, 0, fileName);
            ParseDeclBody(this, program.Ptr(), TokenType::EndOfFile);
			PopScope();

			if (predefUnit)
			{
				PopScope();
			}

			assert(!currentScope.Ptr());
			currentScope = nullptr;

			// HACK(tfoley): mark all declarations in the "stdlib" so
			// that we can detect them later (e.g., so we don't emit them)
			if (!predefUnit)
			{
				for (auto m : program->Members)
				{
					m->modifiers.flags |= ModifierFlag::FromStdlib;
				}
			}


			return program;
		}

		RefPtr<InterfaceSyntaxNode> Parser::ParseInterface()
		{
			RefPtr<InterfaceSyntaxNode> node = new InterfaceSyntaxNode();
			ReadToken("interface");
			PushScope(node.Ptr());
			FillPosition(node.Ptr());
			node->Name = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::LBrace);
			ParseDeclBody(this, node.Ptr(), TokenType::RBrace);
			PopScope();
			return node;
		}

		RefPtr<ShaderSyntaxNode> Parser::ParseShader()
		{
			RefPtr<ShaderSyntaxNode> shader = new ShaderSyntaxNode();
			if (AdvanceIf(this, "module"))
			{
				shader->IsModule = true;
			}
			else
				ReadToken("shader");
			PushScope(shader.Ptr());
			FillPosition(shader.Ptr());
			shader->Name = ReadToken(TokenType::Identifier);
			while (LookAheadToken("targets") || LookAheadToken("implements"))
			{
				if (AdvanceIf(this, "targets"))
				{
					shader->ParentPipelineName = ReadToken(TokenType::Identifier);
				}
				if (AdvanceIf(this, "implements"))
				{
					while (!LookAheadToken("implements") && !LookAheadToken("targets") && !LookAheadToken(TokenType::LBrace))
					{
						shader->InterfaceNames.Add(ReadToken(TokenType::Identifier));
						if (!AdvanceIf(this, TokenType::Comma))
							break;
					}
				}
			}
			
			ReadToken(TokenType::LBrace);
            ParseDeclBody(this, shader.Ptr(), TokenType::RBrace);
			PopScope();
			return shader;
		}

		RefPtr<TemplateShaderSyntaxNode> Parser::ParseTemplateShader()
		{
			RefPtr<TemplateShaderSyntaxNode> shader = new TemplateShaderSyntaxNode();
			ReadToken("template");
			ReadToken("shader");
			PushScope(shader.Ptr());
			FillPosition(shader.Ptr());
			shader->Name = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::LParent);
			while (!AdvanceIf(this, TokenType::RParent))
			{
				shader->Parameters.Add(ParseTemplateShaderParameter());
				if (!AdvanceIf(this, TokenType::Comma))
				{
					ReadToken(TokenType::RParent);
					break;
				}
			}
			if (AdvanceIf(this, "targets"))
				shader->ParentPipelineName = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::LBrace);
			ParseDeclBody(this, shader.Ptr(), TokenType::RBrace);
			PopScope();
			return shader;
		}

		RefPtr<TemplateShaderParameterSyntaxNode> Parser::ParseTemplateShaderParameter()
		{
			RefPtr<TemplateShaderParameterSyntaxNode> param = new TemplateShaderParameterSyntaxNode();
			FillPosition(param.Ptr());
			param->ModuleName = ReadToken(TokenType::Identifier);
			if (AdvanceIf(this, TokenType::Colon))
				param->InterfaceName = ReadToken(TokenType::Identifier);
			return param;
		}

		RefPtr<PipelineSyntaxNode> Parser::ParsePipeline()
		{
			RefPtr<PipelineSyntaxNode> pipeline = new PipelineSyntaxNode();
			ReadToken("pipeline");
			PushScope(pipeline.Ptr());
			FillPosition(pipeline.Ptr());
			pipeline->Name = ReadToken(TokenType::Identifier);
			if (AdvanceIf(this, TokenType::Colon))
			{
				pipeline->ParentPipelineName = ReadToken(TokenType::Identifier);
			}
			ReadToken(TokenType::LBrace);
            ParseDeclBody(this, pipeline.Ptr(), TokenType::RBrace);
			PopScope();
			return pipeline;
		}

		RefPtr<StageSyntaxNode> Parser::ParseStage()
		{
			RefPtr<StageSyntaxNode> stage = new StageSyntaxNode();
			ReadToken("stage");
			stage->Name = ReadToken(TokenType::Identifier);
			FillPosition(stage.Ptr());
			ReadToken(TokenType::Colon);
			stage->StageType = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::LBrace);
			while (!AdvanceIfMatch(this, TokenType::RBrace))
			{
				auto attribName = ReadToken(TokenType::Identifier);
				ReadToken(TokenType::Colon);
				Token attribValue;
				if (LookAheadToken(TokenType::StringLiterial) || LookAheadToken(TokenType::DoubleLiterial) || LookAheadToken(TokenType::IntLiterial))
					attribValue = ReadToken();
				else
					attribValue = ReadToken(TokenType::Identifier);
				stage->Attributes[attribName.Content] = attribValue;
				ReadToken(TokenType::Semicolon);
			}
			return stage;
		}

		RefPtr<WorldSyntaxNode> Parser::ParseWorld()
		{
			RefPtr<WorldSyntaxNode> world = new WorldSyntaxNode();
            world->modifiers = ParseModifiers(this);
			ReadToken("world");
			FillPosition(world.Ptr());
			world->Name = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::Semicolon);
			return world;
		}

		RefPtr<RateSyntaxNode> Parser::ParseRate()
		{
			RefPtr<RateSyntaxNode> rate = new RateSyntaxNode();
			FillPosition(rate.Ptr());
			ReadToken(TokenType::At);
			auto readWorldRate = [this]()
			{
				RateWorld rw;
				rw.World = ReadToken(TokenType::Identifier);
				if (AdvanceIf(this, TokenType::OpMul))
				{
					rw.Pinned = true;
				}
				return rw;
			};
			if (AdvanceIf(this, TokenType::LParent))
			{
				while (!AdvanceIfMatch(this, TokenType::RParent))
				{
					RateWorld rw = readWorldRate();
					rate->Worlds.Add(rw);
                    if (AdvanceIf(this, TokenType::RParent))
                        break;
                    ReadToken(TokenType::Comma);
				}
			}
			else
				rate->Worlds.Add(readWorldRate());
			return rate;
		}

		RefPtr<ImportSyntaxNode> Parser::ParseImportInner()
		{
			RefPtr<ImportSyntaxNode> rs = new ImportSyntaxNode();
			rs->IsInplace = !LookAheadToken(TokenType::OpAssign, 1);
			if (!rs->IsInplace)
			{
				rs->ObjectName = ReadToken(TokenType::Identifier);
				ReadToken(TokenType::OpAssign);
			}
			FillPosition(rs.Ptr());
			rs->ShaderName = ReadToken(TokenType::Identifier);
			if (LookAheadToken(TokenType::Semicolon))
				ReadToken(TokenType::Semicolon);
			else
			{
				ReadToken(TokenType::LParent);
				while (!AdvanceIfMatch(this, TokenType::RParent))
				{
					RefPtr<ImportArgumentSyntaxNode> arg = new ImportArgumentSyntaxNode();
					FillPosition(arg.Ptr());
					auto expr = ParseExpression();
					if (LookAheadToken(TokenType::Colon))
					{
						if (auto varExpr = dynamic_cast<VarExpressionSyntaxNode*>(expr.Ptr()))
						{
							arg->ArgumentName.Content = varExpr->Variable;
							arg->ArgumentName.Position = varExpr->Position;
						}
						else
							sink->diagnose(tokenReader.PeekLoc(), Diagnostics::unexpectedColon);
						ReadToken(TokenType::Colon);
						arg->Expression = ParseExpression();
					}
					else
						arg->Expression = expr;
					rs->Arguments.Add(arg);
					if (AdvanceIf(this, TokenType::RParent))
						break;
                    ReadToken(TokenType::Comma);
				}
				ReadToken(TokenType::Semicolon);
			}
			return rs;
		}

		RefPtr<ImportStatementSyntaxNode> Parser::ParseImportStatement()
		{
			RefPtr<ImportStatementSyntaxNode> rs = new ImportStatementSyntaxNode();
			FillPosition(rs.Ptr());
            ReadToken("import");
			rs->Import = ParseImportInner();
			return rs;
		}

		RefPtr<ImportOperatorDefSyntaxNode> Parser::ParseImportOperator()
		{
			RefPtr<ImportOperatorDefSyntaxNode> op = new ImportOperatorDefSyntaxNode();
			PushScope(op.Ptr());
			FillPosition(op.Ptr());
			ReadToken("import");
			ReadToken(TokenType::LParent);
			op->SourceWorld = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::RightArrow);
			op->DestWorld = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::RParent);
			FillPosition(op.Ptr());
			op->Name = ReadToken(TokenType::Identifier);
			if (LookAheadToken(TokenType::OpLess))
			{
				ReadToken(TokenType::OpLess);
				op->TypeName = ReadToken(TokenType::Identifier);
				ReadToken(TokenType::OpGreater);
			}
			else
			{
				op->TypeName.Position = op->Name.Position;
				op->TypeName.Content = "TComponentType";
			}
			ReadToken(TokenType::LParent);
			while (!AdvanceIf(this, TokenType::RParent))
			{
				AddMember(op, ParseParameter());
				if (AdvanceIf(this, TokenType::RParent))
					break;
                ReadToken(TokenType::Comma);
			}
			while (LookAheadToken("require"))
			{
				ReadToken("require");
				op->Requirements.Add(ParseFunction(false));
			}
			isInImportOperator = true;
			op->Body = ParseBlockStatement();
			isInImportOperator = false;
			PopScope();
			return op;
		}

        // TODO(tfoley): this definition is now largely redundant (only
        // used to parse requirements for import operators)
		RefPtr<FunctionSyntaxNode> Parser::ParseFunction(bool parseBody)
		{
			anonymousParamCounter = 0;
			RefPtr<FunctionSyntaxNode> function = new FunctionSyntaxNode();
            function->modifiers = ParseModifiers(this);
			
			PushScope(function.Ptr());
			function->ReturnType = ParseTypeExp();
			FillPosition(function.Ptr());
			Token name;
			if (LookAheadToken("operator"))
			{
				ReadToken();
				name = ReadToken();
				switch (name.Type)
				{
				case TokenType::OpAdd: case TokenType::OpSub: case TokenType::OpMul: case TokenType::OpDiv:
				case TokenType::OpMod: case TokenType::OpNot: case TokenType::OpBitNot: case TokenType::OpLsh: case TokenType::OpRsh:
				case TokenType::OpEql: case TokenType::OpNeq: case TokenType::OpGreater: case TokenType::OpLess: case TokenType::OpGeq:
				case TokenType::OpLeq: case TokenType::OpAnd: case TokenType::OpOr: case TokenType::OpBitXor: case TokenType::OpBitAnd:
				case TokenType::OpBitOr: case TokenType::OpInc: case TokenType::OpDec:
					break;
				default:
					sink->diagnose(name.Position, Diagnostics::invalidOperator, name.Content);
					break;
				}
			}
			else
			{
				name = ReadToken(TokenType::Identifier);
			}
			function->Name = name;
			ReadToken(TokenType::LParent);
			while(!AdvanceIfMatch(this, TokenType::RParent))
			{
				AddMember(function, ParseParameter());
				if (AdvanceIf(this, TokenType::RParent))
					break;
				ReadToken(TokenType::Comma);
			}
			if (parseBody)
			{
				if (!function->IsExtern())
					function->Body = ParseBlockStatement();
				else
					ReadToken(TokenType::Semicolon);
			}
			PopScope();
			return function;
		}

		RefPtr<StructSyntaxNode> Parser::ParseStruct()
		{
			RefPtr<StructSyntaxNode> rs = new StructSyntaxNode();
			FillPosition(rs.Ptr());
			ReadToken("struct");
			rs->Name = ReadToken(TokenType::Identifier);
			if (LookAheadToken("__intrinsic"))
			{
				ReadToken();
				rs->IsIntrinsic = true;
			}
			typeNames.Add(rs->Name.Content);
			ReadToken(TokenType::LBrace);
            ParseDeclBody(this, rs.Ptr(), TokenType::RBrace);
			return rs;
		}

		static RefPtr<StatementSyntaxNode> ParseSwitchStmt(Parser* parser)
		{
			RefPtr<SwitchStmt> stmt = new SwitchStmt();
			parser->FillPosition(stmt.Ptr());
			parser->ReadToken("switch");
			parser->ReadToken(TokenType::LParent);
			stmt->condition = parser->ParseExpression();
			parser->ReadToken(TokenType::RParent);
			stmt->body = parser->ParseBlockStatement();
			return stmt;
		}

		static RefPtr<StatementSyntaxNode> ParseCaseStmt(Parser* parser)
		{
			RefPtr<CaseStmt> stmt = new CaseStmt();
			parser->FillPosition(stmt.Ptr());
			parser->ReadToken("case");
			stmt->expr = parser->ParseExpression();
			parser->ReadToken(TokenType::Colon);
			return stmt;
		}

		static RefPtr<StatementSyntaxNode> ParseDefaultStmt(Parser* parser)
		{
			RefPtr<DefaultStmt> stmt = new DefaultStmt();
			parser->FillPosition(stmt.Ptr());
			parser->ReadToken("default");
			parser->ReadToken(TokenType::Colon);
			return stmt;
		}

		RefPtr<StatementSyntaxNode> Parser::ParseStatement()
		{
			auto modifiers = ParseModifiers(this);

			RefPtr<StatementSyntaxNode> statement;
			if (LookAheadToken(TokenType::LBrace))
				statement = ParseBlockStatement();
			else if (IsTypeKeyword())
				statement = ParseVarDeclrStatement(modifiers);
			else if (LookAheadToken("if"))
				statement = ParseIfStatement();
			else if (LookAheadToken("for"))
				statement = ParseForStatement();
			else if (LookAheadToken("while"))
				statement = ParseWhileStatement();
			else if (LookAheadToken("do"))
				statement = ParseDoWhileStatement();
			else if (LookAheadToken("break"))
				statement = ParseBreakStatement();
			else if (LookAheadToken("continue"))
				statement = ParseContinueStatement();
			else if (LookAheadToken("return"))
				statement = ParseReturnStatement();
			else if (LookAheadToken("using") || (LookAheadToken("public") && LookAheadToken("using", 1)))
				statement = ParseImportStatement();
			else if (LookAheadToken("discard"))
			{
				statement = new DiscardStatementSyntaxNode();
				FillPosition(statement.Ptr());
				ReadToken("discard");
				ReadToken(TokenType::Semicolon);
			}
			else if (LookAheadToken("switch"))
				statement = ParseSwitchStmt(this);
			else if (LookAheadToken("case"))
				statement = ParseCaseStmt(this);
			else if (LookAheadToken("default"))
				statement = ParseDefaultStmt(this);
			else if (LookAheadToken(TokenType::Identifier))
			{
				// We might be looking at a local declaration, or an
				// expression statement, and we need to figure out which.
				//
				// We'll solve this with backtracking for now.

				Token* startPos = tokenReader.mCursor;

				// Try to parse a type (knowing that the type grammar is
				// a subset of the expression grammar, and so this should
				// always succeed).
				RefPtr<ExpressionSyntaxNode> type = ParseType();
				// We don't actually care about the type, though, so
				// don't retain it
				type = nullptr;

				// If the next token after we parsed a type looks like
				// we are going to declare a variable, then lets guess
				// that this is a declaration.
				//
				// TODO(tfoley): this wouldn't be robust for more
				// general kinds of declarators (notably pointer declarators),
				// so we'll need to be careful about this.
				if (LookAheadToken(TokenType::Identifier))
				{
					// Reset the cursor and try to parse a declaration now.
					// Note: the declaration will consume any modifiers
					// that had been in place on the statement.
					tokenReader.mCursor = startPos;
					statement = ParseVarDeclrStatement(modifiers);
					return statement;
				}

				// Fallback: reset and parse an expression
				tokenReader.mCursor = startPos;
				statement = ParseExpressionStatement();
			}
			else if (LookAheadToken(TokenType::Semicolon))
			{
				statement = new EmptyStatementSyntaxNode();
				FillPosition(statement.Ptr());
				ReadToken(TokenType::Semicolon);
			}
			else
			{
                Unexpected(this);
			}

			if (statement)
			{
				// Install any modifiers onto the statement.
				// Note: this path is bypassed in the case of a
				// declaration statement, so we don't end up
				// doubling up the modifiers.
				statement->modifiers = modifiers;
			}

			return statement;
		}

		RefPtr<BlockStatementSyntaxNode> Parser::ParseBlockStatement()
		{
			RefPtr<ScopeDecl> scopeDecl = new ScopeDecl();
			RefPtr<BlockStatementSyntaxNode> blockStatement = new BlockStatementSyntaxNode();
            blockStatement->scopeDecl = scopeDecl;
			PushScope(scopeDecl.Ptr());
			ReadToken(TokenType::LBrace);
			if(!tokenReader.IsAtEnd())
			{
				FillPosition(blockStatement.Ptr());
			}
			while (!AdvanceIfMatch(this, TokenType::RBrace))
			{
                auto stmt = ParseStatement();
                if(stmt)
                {
    				blockStatement->Statements.Add(stmt);
				}
                TryRecover(this);
			}
			PopScope();
			return blockStatement;
		}

		RefPtr<VarDeclrStatementSyntaxNode> Parser::ParseVarDeclrStatement(
			Modifiers modifiers)
		{
			RefPtr<VarDeclrStatementSyntaxNode>varDeclrStatement = new VarDeclrStatementSyntaxNode();
		
			FillPosition(varDeclrStatement.Ptr());
            auto decl = ParseDeclWithModifiers(this, currentScope.Ptr(), modifiers);
            varDeclrStatement->decl = decl;
			return varDeclrStatement;
		}

		RefPtr<IfStatementSyntaxNode> Parser::ParseIfStatement()
		{
			RefPtr<IfStatementSyntaxNode> ifStatement = new IfStatementSyntaxNode();
			FillPosition(ifStatement.Ptr());
			ReadToken("if");
			ReadToken(TokenType::LParent);
			ifStatement->Predicate = ParseExpression();
			ReadToken(TokenType::RParent);
			ifStatement->PositiveStatement = ParseStatement();
			if (LookAheadToken("else"))
			{
				ReadToken("else");
				ifStatement->NegativeStatement = ParseStatement();
			}
			return ifStatement;
		}

		RefPtr<ForStatementSyntaxNode> Parser::ParseForStatement()
		{
			RefPtr<ScopeDecl> scopeDecl = new ScopeDecl();
			RefPtr<ForStatementSyntaxNode> stmt = new ForStatementSyntaxNode();
            stmt->scopeDecl = scopeDecl;
			PushScope(scopeDecl.Ptr());
			FillPosition(stmt.Ptr());
			ReadToken("for");
			ReadToken(TokenType::LParent);
			if (IsTypeKeyword())
			{
                stmt->InitialStatement = ParseVarDeclrStatement(Modifiers());
			}
			else
			{
                if (!LookAheadToken(TokenType::Semicolon))
                {
					stmt->InitialStatement = ParseExpressionStatement();
                }
                else
                {
			        ReadToken(TokenType::Semicolon);
                }
			}
			if (!LookAheadToken(TokenType::Semicolon))
				stmt->PredicateExpression = ParseExpression();
			ReadToken(TokenType::Semicolon);
			if (!LookAheadToken(TokenType::RParent))
				stmt->SideEffectExpression = ParseExpression();
			ReadToken(TokenType::RParent);
			stmt->Statement = ParseStatement();
			PopScope();
			return stmt;
		}

		RefPtr<WhileStatementSyntaxNode> Parser::ParseWhileStatement()
		{
			RefPtr<WhileStatementSyntaxNode> whileStatement = new WhileStatementSyntaxNode();
			FillPosition(whileStatement.Ptr());
			ReadToken("while");
			ReadToken(TokenType::LParent);
			whileStatement->Predicate = ParseExpression();
			ReadToken(TokenType::RParent);
			whileStatement->Statement = ParseStatement();
			return whileStatement;
		}

		RefPtr<DoWhileStatementSyntaxNode> Parser::ParseDoWhileStatement()
		{
			RefPtr<DoWhileStatementSyntaxNode> doWhileStatement = new DoWhileStatementSyntaxNode();
			FillPosition(doWhileStatement.Ptr());
			ReadToken("do");
			doWhileStatement->Statement = ParseStatement();
			ReadToken("while");
			ReadToken(TokenType::LParent);
			doWhileStatement->Predicate = ParseExpression();
			ReadToken(TokenType::RParent);
			ReadToken(TokenType::Semicolon);
			return doWhileStatement;
		}

		RefPtr<BreakStatementSyntaxNode> Parser::ParseBreakStatement()
		{
			RefPtr<BreakStatementSyntaxNode> breakStatement = new BreakStatementSyntaxNode();
			FillPosition(breakStatement.Ptr());
			ReadToken("break");
			ReadToken(TokenType::Semicolon);
			return breakStatement;
		}

		RefPtr<ContinueStatementSyntaxNode>	Parser::ParseContinueStatement()
		{
			RefPtr<ContinueStatementSyntaxNode> continueStatement = new ContinueStatementSyntaxNode();
			FillPosition(continueStatement.Ptr());
			ReadToken("continue");
			ReadToken(TokenType::Semicolon);
			return continueStatement;
		}

		RefPtr<ReturnStatementSyntaxNode> Parser::ParseReturnStatement()
		{
			RefPtr<ReturnStatementSyntaxNode> returnStatement = new ReturnStatementSyntaxNode();
			FillPosition(returnStatement.Ptr());
			ReadToken("return");
			if (!LookAheadToken(TokenType::Semicolon))
				returnStatement->Expression = ParseExpression();
			ReadToken(TokenType::Semicolon);
			return returnStatement;
		}

		RefPtr<ExpressionStatementSyntaxNode> Parser::ParseExpressionStatement()
		{
			RefPtr<ExpressionStatementSyntaxNode> statement = new ExpressionStatementSyntaxNode();
			
			FillPosition(statement.Ptr());
			statement->Expression = ParseExpression();
			
			ReadToken(TokenType::Semicolon);
			return statement;
		}

		RefPtr<ParameterSyntaxNode> Parser::ParseParameter()
		{
			// TODO(tfoley): This really ought to just use the same
			// logic as other variable-parsing code...

			RefPtr<ParameterSyntaxNode> parameter = new ParameterSyntaxNode();
            parameter->modifiers = ParseModifiers(this);
			parameter->Type = ParseTypeExp();
			if (LookAheadToken(TokenType::Identifier))
			{
				Token name = ReadToken(TokenType::Identifier);
				parameter->Name = name;
			}
			else
				parameter->Name.Content = "_anonymousParam" + String(anonymousParamCounter++);
			FillPosition(parameter.Ptr());
			ParseOptSemantics(this, parameter.Ptr());
			if (AdvanceIf(this, TokenType::OpAssign))
			{
				parameter->Expr = ParseExpression();
			}
			return parameter;
		}

		RefPtr<ExpressionSyntaxNode> ParseGenericArg(Parser* parser)
		{
			return parser->ParseExpression();
		}

		RefPtr<ExpressionSyntaxNode> Parser::ParseType()
		{
			Token typeName;
			if (LookAheadToken(TokenType::Identifier))
				typeName = ReadToken(TokenType::Identifier);
			else
				typeName = ReadTypeKeyword();

			auto basicType = new VarExpressionSyntaxNode();
			basicType->scope = currentScope.Ptr();
			basicType->Position = typeName.Position;
			basicType->Variable = typeName.Content;

			RefPtr<ExpressionSyntaxNode> rs = basicType;

			if (LookAheadToken(TokenType::OpLess))
			{
				RefPtr<GenericAppExpr> gtype = new GenericAppExpr();
				FillPosition(gtype.Ptr()); // set up scope for lookup
				gtype->Position = typeName.Position;
				gtype->FunctionExpr = rs;
				ReadToken(TokenType::OpLess);
				this->genericDepth++;
				// For now assume all generics have at least one argument
				gtype->Arguments.Add(ParseGenericArg(this));
				while (AdvanceIf(this, TokenType::Comma))
				{
					gtype->Arguments.Add(ParseGenericArg(this));
				}
				this->genericDepth--;
				ReadToken(TokenType::OpGreater);
				rs = gtype;
			}
			while (LookAheadToken(TokenType::LBracket))
			{
				RefPtr<IndexExpressionSyntaxNode> arrType = new IndexExpressionSyntaxNode();
				arrType->Position = rs->Position;
				arrType->BaseExpression = rs;
				ReadToken(TokenType::LBracket);
				if (!LookAheadToken(TokenType::RBracket))
				{
					arrType->IndexExpression = ParseExpression();
				}
				ReadToken(TokenType::RBracket);
				rs = arrType;
			}
			return rs;
		}

		TypeExp Parser::ParseTypeExp()
		{
			return TypeExp(ParseType());
		}

		enum class Associativity
		{
			Left, Right
		};

		Associativity GetAssociativityFromLevel(int level)
		{
			if (level == 0)
				return Associativity::Right;
			else
				return Associativity::Left;
		}

		int GetOpLevel(Parser* parser, CoreLib::Text::TokenType type)
		{
			switch(type)
			{
			case TokenType::OpAssign:
			case TokenType::OpMulAssign:
			case TokenType::OpDivAssign:
			case TokenType::OpAddAssign:
			case TokenType::OpSubAssign:
			case TokenType::OpModAssign:
			case TokenType::OpShlAssign:
			case TokenType::OpShrAssign:
			case TokenType::OpOrAssign:
			case TokenType::OpAndAssign:
			case TokenType::OpXorAssign:
				return 0;
			case TokenType::OpOr:
				return 2;
			case TokenType::OpAnd:
				return 3;
			case TokenType::OpBitOr:
				return 4;
			case TokenType::OpBitXor:
				return 5;
			case TokenType::OpBitAnd:
				return 6;
			case TokenType::OpEql:
			case TokenType::OpNeq:
				return 7;
			case TokenType::OpGreater:
			case TokenType::OpGeq:
				// Don't allow these ops inside a generic argument
				if (parser->genericDepth > 0) return -1;
			case TokenType::OpLeq:
			case TokenType::OpLess:
				return 8;
			case TokenType::OpRsh:
				// Don't allow this op inside a generic argument
				if (parser->genericDepth > 0) return -1;
			case TokenType::OpLsh:
				return 9;
			case TokenType::OpAdd:
			case TokenType::OpSub:
				return 10;
			case TokenType::OpMul:
			case TokenType::OpDiv:
			case TokenType::OpMod:
				return 11;
			default:
				return -1;
			}
		}

		Operator GetOpFromToken(Token & token)
		{
			switch(token.Type)
			{
			case TokenType::OpAssign:
				return Operator::Assign;
			case TokenType::OpAddAssign:
				return Operator::AddAssign;
			case TokenType::OpSubAssign:
				return Operator::SubAssign;
			case TokenType::OpMulAssign:
				return Operator::MulAssign;
			case TokenType::OpDivAssign:
				return Operator::DivAssign;
			case TokenType::OpModAssign:
				return Operator::ModAssign;
			case TokenType::OpShlAssign:
				return Operator::LshAssign;
			case TokenType::OpShrAssign:
				return Operator::RshAssign;
			case TokenType::OpOrAssign:
				return Operator::OrAssign;
			case TokenType::OpAndAssign:
				return Operator::AddAssign;
			case TokenType::OpXorAssign:
				return Operator::XorAssign;
			case TokenType::OpOr:
				return Operator::Or;
			case TokenType::OpAnd:
				return Operator::And;
			case TokenType::OpBitOr:
				return Operator::BitOr;
			case TokenType::OpBitXor:
				return Operator::BitXor;
			case TokenType::OpBitAnd:
				return Operator::BitAnd;
			case TokenType::OpEql:
				return Operator::Eql;
			case TokenType::OpNeq:
				return Operator::Neq;
			case TokenType::OpGeq:
				return Operator::Geq;
			case TokenType::OpLeq:
				return Operator::Leq;
			case TokenType::OpGreater:
				return Operator::Greater;
			case TokenType::OpLess:
				return Operator::Less;
			case TokenType::OpLsh:
				return Operator::Lsh;
			case TokenType::OpRsh:
				return Operator::Rsh;
			case TokenType::OpAdd:
				return Operator::Add;
			case TokenType::OpSub:
				return Operator::Sub;
			case TokenType::OpMul:
				return Operator::Mul;
			case TokenType::OpDiv:
				return Operator::Div;
			case TokenType::OpMod:
				return Operator::Mod;
			case TokenType::OpInc:
				return Operator::PostInc;
			case TokenType::OpDec:
				return Operator::PostDec;
			case TokenType::OpNot:
				return Operator::Not;
			case TokenType::OpBitNot:
				return Operator::BitNot;
			default:
				throw "Illegal TokenType.";
			}
		}

		RefPtr<ExpressionSyntaxNode> Parser::ParseExpression(int level)
		{
			if (level == MaxExprLevel)
				return ParseLeafExpression();
			if (level == 1)
			{
				// parse select clause
				auto condition = ParseExpression(level + 1);
				if (LookAheadToken(TokenType::QuestionMark))
				{
					RefPtr<SelectExpressionSyntaxNode> select = new SelectExpressionSyntaxNode();
					FillPosition(select.Ptr());
					ReadToken(TokenType::QuestionMark);
					select->SelectorExpr = condition;
					select->Expr0 = ParseExpression(level);
					ReadToken(TokenType::Colon);
					select->Expr1 = ParseExpression(level);
					return select;
				}
				else
					return condition;
			}
			else
			{
				if (GetAssociativityFromLevel(level) == Associativity::Left)
				{
					auto left = ParseExpression(level + 1);
					while (GetOpLevel(this, tokenReader.PeekTokenType()) == level)
					{
						RefPtr<BinaryExpressionSyntaxNode> tmp = new BinaryExpressionSyntaxNode();
						tmp->LeftExpression = left;
						FillPosition(tmp.Ptr());
						Token opToken = tokenReader.AdvanceToken();
						tmp->Operator = GetOpFromToken(opToken);
						tmp->RightExpression = ParseExpression(level + 1);
						left = tmp;
					}
					return left;
				}
				else
				{
					auto left = ParseExpression(level + 1);
					if (GetOpLevel(this, tokenReader.PeekTokenType()) == level)
					{
						RefPtr<BinaryExpressionSyntaxNode> tmp = new BinaryExpressionSyntaxNode();
						tmp->LeftExpression = left;
						FillPosition(tmp.Ptr());
						Token opToken = tokenReader.AdvanceToken();
						tmp->Operator = GetOpFromToken(opToken);
						tmp->RightExpression = ParseExpression(level);
						left = tmp;
					}
					return left;
				}
			}
		}

		RefPtr<ExpressionSyntaxNode> Parser::ParseLeafExpression()
		{
			RefPtr<ExpressionSyntaxNode> rs;
			if (LookAheadToken("project"))
			{
				RefPtr<ProjectExpressionSyntaxNode> project = new ProjectExpressionSyntaxNode();
				FillPosition(project.Ptr());
				ReadToken("project");
				ReadToken(TokenType::LParent);
				project->BaseExpression = ParseExpression();
				ReadToken(TokenType::RParent);
				return project;
			}
			if (LookAheadToken(TokenType::OpInc) ||
				LookAheadToken(TokenType::OpDec) ||
				LookAheadToken(TokenType::OpNot) ||
				LookAheadToken(TokenType::OpBitNot) ||
				LookAheadToken(TokenType::OpSub))
			{
				RefPtr<UnaryExpressionSyntaxNode> unaryExpr = new UnaryExpressionSyntaxNode();
				Token token = tokenReader.AdvanceToken();
				FillPosition(unaryExpr.Ptr());
				unaryExpr->Operator = GetOpFromToken(token);
				if (unaryExpr->Operator == Operator::PostInc)
					unaryExpr->Operator = Operator::PreInc;
				else if (unaryExpr->Operator == Operator::PostDec)
					unaryExpr->Operator = Operator::PreDec;
				else if (unaryExpr->Operator == Operator::Sub)
					unaryExpr->Operator = Operator::Neg;

				unaryExpr->Expression = ParseLeafExpression();
				rs = unaryExpr;
				return rs;
			}

			if (LookAheadToken(TokenType::LParent))
			{
				ReadToken(TokenType::LParent);
				RefPtr<ExpressionSyntaxNode> expr;
				if (IsTypeKeyword() && LookAheadToken(TokenType::RParent, 1))
				{
					RefPtr<TypeCastExpressionSyntaxNode> tcexpr = new TypeCastExpressionSyntaxNode();
					FillPosition(tcexpr.Ptr());
					tcexpr->TargetType = ParseTypeExp();
					ReadToken(TokenType::RParent);
					tcexpr->Expression = ParseExpression();
					expr = tcexpr;
				}
				else
				{
					expr = ParseExpression();
					ReadToken(TokenType::RParent);
				}
				rs = expr;
			}
			else if (LookAheadToken(TokenType::IntLiterial) ||
				LookAheadToken(TokenType::DoubleLiterial))
			{
				RefPtr<ConstantExpressionSyntaxNode> constExpr = new ConstantExpressionSyntaxNode();
				auto token = tokenReader.AdvanceToken();
				FillPosition(constExpr.Ptr());
				if (token.Type == TokenType::IntLiterial)
				{
					constExpr->ConstType = ConstantExpressionSyntaxNode::ConstantType::Int;
					constExpr->IntValue = StringToInt(token.Content);
				}
				else if (token.Type == TokenType::DoubleLiterial)
				{
					constExpr->ConstType = ConstantExpressionSyntaxNode::ConstantType::Float;
					constExpr->FloatValue = (float)StringToDouble(token.Content);
				}
				rs = constExpr;
			}
			else if (LookAheadToken("true") || LookAheadToken("false"))
			{
				RefPtr<ConstantExpressionSyntaxNode> constExpr = new ConstantExpressionSyntaxNode();
				auto token = tokenReader.AdvanceToken();
				FillPosition(constExpr.Ptr());
				constExpr->ConstType = ConstantExpressionSyntaxNode::ConstantType::Bool;
				constExpr->IntValue = token.Content == "true" ? 1 : 0;
				rs = constExpr;
			}
			else if (LookAheadToken(TokenType::Identifier))
			{
				RefPtr<VarExpressionSyntaxNode> varExpr = new VarExpressionSyntaxNode();
				varExpr->scope = currentScope.Ptr();
				FillPosition(varExpr.Ptr());
				auto token = ReadToken(TokenType::Identifier);
				varExpr->Variable = token.Content;
				rs = varExpr;
			}

			while (!tokenReader.IsAtEnd() &&
				(LookAheadToken(TokenType::OpInc) ||
				LookAheadToken(TokenType::OpDec) ||
				LookAheadToken(TokenType::Dot) ||
				LookAheadToken(TokenType::LBracket) ||
				LookAheadToken(TokenType::LParent)))
			{
				if (LookAheadToken(TokenType::OpInc))
				{
					RefPtr<UnaryExpressionSyntaxNode> unaryExpr = new UnaryExpressionSyntaxNode();
					FillPosition(unaryExpr.Ptr());
					ReadToken(TokenType::OpInc);
					unaryExpr->Operator = Operator::PostInc;
					unaryExpr->Expression = rs;
					rs = unaryExpr;
				}
				else if (LookAheadToken(TokenType::OpDec))
				{
					RefPtr<UnaryExpressionSyntaxNode> unaryExpr = new UnaryExpressionSyntaxNode();
					FillPosition(unaryExpr.Ptr());
					ReadToken(TokenType::OpDec);
					unaryExpr->Operator = Operator::PostDec;
					unaryExpr->Expression = rs;
					rs = unaryExpr;
				}
				else if (LookAheadToken(TokenType::LBracket))
				{
					RefPtr<IndexExpressionSyntaxNode> indexExpr = new IndexExpressionSyntaxNode();
					indexExpr->BaseExpression = rs;
					FillPosition(indexExpr.Ptr());
					ReadToken(TokenType::LBracket);
					indexExpr->IndexExpression = ParseExpression();
					ReadToken(TokenType::RBracket);
					rs = indexExpr;
				}
				else if (LookAheadToken(TokenType::LParent))
				{
					RefPtr<InvokeExpressionSyntaxNode> invokeExpr = new InvokeExpressionSyntaxNode();
					invokeExpr->FunctionExpr = rs;
					FillPosition(invokeExpr.Ptr());
					ReadToken(TokenType::LParent);
					while (!tokenReader.IsAtEnd())
					{
						if (!LookAheadToken(TokenType::RParent))
							invokeExpr->Arguments.Add(ParseExpression());
						else
						{
							break;
						}
						if (!LookAheadToken(TokenType::Comma))
							break;
						ReadToken(TokenType::Comma);
					}
					ReadToken(TokenType::RParent);
					rs = invokeExpr;
				}
				else if (LookAheadToken(TokenType::Dot))
				{
					RefPtr<MemberExpressionSyntaxNode> memberExpr = new MemberExpressionSyntaxNode();
					memberExpr->scope = currentScope.Ptr();
					FillPosition(memberExpr.Ptr());
					memberExpr->BaseExpression = rs;
					ReadToken(TokenType::Dot); 
					memberExpr->MemberName = ReadToken(TokenType::Identifier).Content;
					rs = memberExpr;
				}
			}
			if (!rs)
			{
				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::syntaxError);
			}
			return rs;
		}

        RefPtr<ProgramSyntaxNode> ParseProgram(
            TokenSpan const&    tokens,
            DiagnosticSink*     sink,
            String const&       fileName,
			ProgramSyntaxNode*	predefUnit)
        {
            Parser parser(tokens, sink, fileName);
            return parser.Parse(predefUnit);
        }

	}
}