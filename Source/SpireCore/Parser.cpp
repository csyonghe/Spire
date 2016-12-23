#include "Parser.h"

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
			List<RefPtr<Scope>> scopeStack;
            TokenReader tokenReader;
            DiagnosticSink * sink;
			String fileName;
			HashSet<String> typeNames;
			HashSet<String> classNames;
			bool isInImportOperator = false;

            // Is the parser in a "recovering" state?
            // During recovery we don't emit additional errors, until we find
            // a token that we expected, when we exit recovery.
            bool isRecovering = false;

			void FillPosition(SyntaxNode * node)
			{
				node->Position = tokenReader.PeekLoc();
				node->Scope = scopeStack.Last();
			}
			void PushScope()
			{
				scopeStack.Add(new Scope());
				if (scopeStack.Count() > 1)
					scopeStack.Last()->Parent = scopeStack[scopeStack.Count() - 2].Ptr();
			}
			void PopScope()
			{
				scopeStack.Last() = 0;
				scopeStack.RemoveAt(scopeStack.Count() - 1);
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
				typeNames.Add("sampler_state");
				typeNames.Add("Uniform");
				typeNames.Add("StructuredBuffer");
				typeNames.Add("RWStructuredBuffer");
				typeNames.Add("PackedBuffer");
				typeNames.Add("StorageBuffer");
				typeNames.Add("Patch");
			}
			RefPtr<ProgramSyntaxNode> Parse();

			Token ReadToken();
			Token ReadToken(CoreLib::Text::TokenType type);
			Token ReadToken(const char * string);
			bool LookAheadToken(CoreLib::Text::TokenType type, int offset = 0);
			bool LookAheadToken(const char * string, int offset = 0);
			Token ReadTypeKeyword();
			bool IsTypeKeyword();
			RefPtr<ProgramSyntaxNode>				ParseProgram();
			RefPtr<ShaderSyntaxNode>				ParseShader();
			RefPtr<InterfaceSyntaxNode>				ParseInterface();
			RefPtr<PipelineSyntaxNode>				ParsePipeline();
			RefPtr<StageSyntaxNode>					ParseStage();
			RefPtr<WorldSyntaxNode>					ParseWorld();
			RefPtr<RateSyntaxNode>					ParseRate();
			RefPtr<ImportSyntaxNode>				ParseImportInner();
			RefPtr<ImportStatementSyntaxNode>		ParseImportStatement();
			RefPtr<ImportOperatorDefSyntaxNode>		ParseImportOperator();
			RefPtr<FunctionSyntaxNode>				ParseFunction(bool parseBody = true);
			RefPtr<StructSyntaxNode>				ParseStruct();
			RefPtr<StatementSyntaxNode>				ParseStatement();
			RefPtr<BlockStatementSyntaxNode>		ParseBlockStatement();
			RefPtr<VarDeclrStatementSyntaxNode>		ParseVarDeclrStatement();
			RefPtr<IfStatementSyntaxNode>			ParseIfStatement();
			RefPtr<ForStatementSyntaxNode>			ParseForStatement();
			RefPtr<WhileStatementSyntaxNode>		ParseWhileStatement();
			RefPtr<DoWhileStatementSyntaxNode>		ParseDoWhileStatement();
			RefPtr<BreakStatementSyntaxNode>		ParseBreakStatement();
			RefPtr<ContinueStatementSyntaxNode>		ParseContinueStatement();
			RefPtr<ReturnStatementSyntaxNode>		ParseReturnStatement();
			RefPtr<ExpressionStatementSyntaxNode>	ParseExpressionStatement();
			RefPtr<ExpressionSyntaxNode>			ParseExpression(int level = 0);
			RefPtr<ExpressionSyntaxNode>			ParseLeafExpression();
			RefPtr<ParameterSyntaxNode>				ParseParameter();
			RefPtr<TypeSyntaxNode>					ParseType();

			Parser & operator = (const Parser &) = delete;
		};

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

		RefPtr<ProgramSyntaxNode> Parser::Parse()
		{
			return ParseProgram();
		}

        RefPtr<TypeDefDecl> ParseTypeDef(Parser* parser)
        {
            // Consume the `typedef` keyword
            parser->ReadToken("typedef");

            // TODO(tfoley): parse an actual declarator
            auto type = parser->ParseType();

            auto nameToken = parser->ReadToken(TokenType::Identifier);

            RefPtr<TypeDefDecl> typeDefDecl = new TypeDefDecl();
            typeDefDecl->Name = nameToken;
            typeDefDecl->TypeNode = type;

			parser->typeNames.Add(nameToken.Content);

            return typeDefDecl;
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
                else if (AdvanceIf(parser, "uniform"))
                {
                    modifiers.flags |= ModifierFlag::Uniform;
                }
                else if (AdvanceIf(parser, "parameter"))
                {
                    modifiers.flags |= ModifierFlag::Parameter;
                }
                else if (AdvanceIf(parser, "const"))
                {
                    modifiers.flags |= ModifierFlag::Const;
                }
                else if (AdvanceIf(parser, "centroid"))
                {
                    modifiers.flags |= ModifierFlag::Centroid;
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

                    *modifierLink = modifier;
                    modifierLink = &modifier->next;
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
                else if (AdvanceIf(parser, TokenType::LBracket))
                {
                    auto name = parser->ReadToken(TokenType::Identifier).Content;
                    Token valueToken;
                    if (AdvanceIf(parser, TokenType::Colon))
                    {
                        valueToken = parser->ReadToken(TokenType::StringLiterial);
                    }
                    parser->ReadToken(TokenType::RBracket);

                    RefPtr<SimpleAttribute> modifier = new SimpleAttribute();
                    modifier->Key = name;
                    modifier->Value = valueToken;

                    *modifierLink = modifier;
                    modifierLink = &modifier->next;
                }
                else if (AdvanceIf(parser, "__intrinsic"))
                {
                    modifiers.flags |= ModifierFlag::Intrinsic;
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

        struct DeclaratorInfo
        {
            RefPtr<RateSyntaxNode>  rate;
            RefPtr<TypeSyntaxNode>  typeSpec;
            Token                   nameToken;
        };

        static void ParseFuncDeclHeader(
            Parser*                     parser,
            DeclaratorInfo const&       declaratorInfo,
            RefPtr<FunctionSyntaxNode>  decl)
        {
            parser->PushScope();

            parser->anonymousParamCounter = 0;
            parser->FillPosition(decl.Ptr());
            decl->Position = declaratorInfo.nameToken.Position;

            decl->Name = declaratorInfo.nameToken;
            decl->ReturnTypeNode = declaratorInfo.typeSpec;
            parser->ReadToken(TokenType::LParent);
            while (!AdvanceIfMatch(parser, TokenType::RParent))
            {
                decl->Parameters.Add(parser->ParseParameter());
                if (AdvanceIf(parser, TokenType::RParent))
                    break;
                parser->ReadToken(TokenType::Comma);
            }
        }

        static void ParseFuncDeclHeader(
            Parser*                     parser,
            DeclaratorInfo const&       declaratorInfo,
            RefPtr<ComponentSyntaxNode> decl)
        {
            parser->PushScope();

            parser->anonymousParamCounter = 0;
            parser->FillPosition(decl.Ptr());
            decl->Position = declaratorInfo.nameToken.Position;

            decl->Name = declaratorInfo.nameToken;
            decl->TypeNode = declaratorInfo.typeSpec;
            parser->ReadToken(TokenType::LParent);
            while (!AdvanceIfMatch(parser, TokenType::RParent))
            {
                decl->Parameters.Add(parser->ParseParameter());
                if (AdvanceIf(parser, TokenType::RParent))
                    break;
                parser->ReadToken(TokenType::Comma);
            }
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
                decl->TypeNode = declaratorInfo.typeSpec;

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
                decl->TypeNode = declaratorInfo.typeSpec;

                if (AdvanceIf(parser, TokenType::OpAssign))
                {
                    decl->Expr = parser->ParseExpression();
                }
                parser->ReadToken(TokenType::Semicolon);

                return decl;
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
            declaratorInfo.nameToken = ParseDeclName(parser);

            // TODO(tfoley): handle array-ness here...

            // Look at the token after the name to disambiguate
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

        static RefPtr<Decl> ParseDeclWithModifiers(
            Parser*             parser,
            ContainerDecl*      containerDecl,
            Modifiers const&    modifiers )
        {
            RefPtr<Decl> decl;

            // TODO: actual dispatch!
			if (parser->LookAheadToken("shader") || parser->LookAheadToken("module"))
				decl = parser->ParseShader();
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
            else if (AdvanceIf(parser, TokenType::Semicolon))
            {
                // empty declaration
            }
            else
                decl = ParseDeclaratorDecl(parser, containerDecl);

            if (decl)
            {
                decl->modifiers = modifiers;
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
                    containerDecl->Members.Add(decl);
                }
                TryRecover(parser);
            }
        }

		RefPtr<ProgramSyntaxNode> Parser::ParseProgram()
		{
			scopeStack.Add(new Scope());
			RefPtr<ProgramSyntaxNode> program = new ProgramSyntaxNode();
			program->Position = CodePosition(0, 0, 0, fileName);
			program->Scope = scopeStack.Last();
            ParseDeclBody(this, program.Ptr(), TokenType::EndOfFile);
			scopeStack.Clear();
			return program;
		}

		RefPtr<InterfaceSyntaxNode> Parser::ParseInterface()
		{
			RefPtr<InterfaceSyntaxNode> node = new InterfaceSyntaxNode();
			ReadToken("interface");
			PushScope();
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
			PushScope();
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

		RefPtr<PipelineSyntaxNode> Parser::ParsePipeline()
		{
			RefPtr<PipelineSyntaxNode> pipeline = new PipelineSyntaxNode();
			ReadToken("pipeline");
			PushScope();
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
			PushScope();
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
				op->Parameters.Add(ParseParameter());
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

		RefPtr<FunctionSyntaxNode> Parser::ParseFunction(bool parseBody)
		{
			anonymousParamCounter = 0;
			RefPtr<FunctionSyntaxNode> function = new FunctionSyntaxNode();
            function->modifiers = ParseModifiers(this);
			
			PushScope();
			function->ReturnTypeNode = ParseType();
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
				function->Parameters.Add(ParseParameter());
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

		RefPtr<StatementSyntaxNode> Parser::ParseStatement()
		{
			RefPtr<StatementSyntaxNode> statement;
			if (LookAheadToken(TokenType::LBrace))
				statement = ParseBlockStatement();
			else if (IsTypeKeyword() || LookAheadToken("const"))
				statement = ParseVarDeclrStatement();
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
			else if (LookAheadToken(TokenType::Identifier))
			{
				Token* startPos = tokenReader.mCursor;
				bool isVarDeclr = false;
				RefPtr<TypeSyntaxNode> type = ParseType();
				if (LookAheadToken(TokenType::Identifier))
				{
					type = nullptr;
					tokenReader.mCursor = startPos;
					statement = ParseVarDeclrStatement();
					isVarDeclr = true;
				}
				if (!isVarDeclr)
				{
                    tokenReader.mCursor = startPos;
					statement = ParseExpressionStatement();
				}
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
			return statement;
		}

		RefPtr<BlockStatementSyntaxNode> Parser::ParseBlockStatement()
		{
			RefPtr<BlockStatementSyntaxNode> blockStatement = new BlockStatementSyntaxNode();
			PushScope();
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

		RefPtr<VarDeclrStatementSyntaxNode> Parser::ParseVarDeclrStatement()
		{
			RefPtr<VarDeclrStatementSyntaxNode>varDeclrStatement = new VarDeclrStatementSyntaxNode();
		
			FillPosition(varDeclrStatement.Ptr());
            varDeclrStatement->decl = ParseDecl(this, nullptr);
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
			RefPtr<ForStatementSyntaxNode> stmt = new ForStatementSyntaxNode();
			PushScope();
			FillPosition(stmt.Ptr());
			ReadToken("for");
			ReadToken(TokenType::LParent);
			if (IsTypeKeyword())
			{
                stmt->InitialStatement = ParseVarDeclrStatement();
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
			PushScope();
			FillPosition(whileStatement.Ptr());
			ReadToken("while");
			ReadToken(TokenType::LParent);
			whileStatement->Predicate = ParseExpression();
			ReadToken(TokenType::RParent);
			whileStatement->Statement = ParseStatement();
			PopScope();
			return whileStatement;
		}

		RefPtr<DoWhileStatementSyntaxNode> Parser::ParseDoWhileStatement()
		{
			RefPtr<DoWhileStatementSyntaxNode> doWhileStatement = new DoWhileStatementSyntaxNode();
			PushScope();
			FillPosition(doWhileStatement.Ptr());
			ReadToken("do");
			doWhileStatement->Statement = ParseStatement();
			ReadToken("while");
			ReadToken(TokenType::LParent);
			doWhileStatement->Predicate = ParseExpression();
			ReadToken(TokenType::RParent);
			ReadToken(TokenType::Semicolon);
			PopScope();
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
			RefPtr<ParameterSyntaxNode> parameter = new ParameterSyntaxNode();
            parameter->modifiers = ParseModifiers(this);
			parameter->TypeNode = ParseType();
			if (LookAheadToken(TokenType::Identifier))
			{
				Token name = ReadToken(TokenType::Identifier);
				parameter->Name = name;
			}
			else
				parameter->Name.Content = "_anonymousParam" + String(anonymousParamCounter++);
			FillPosition(parameter.Ptr());
			return parameter;
		}

		RefPtr<TypeSyntaxNode> Parser::ParseType()
		{
			Token typeName;
			if (LookAheadToken(TokenType::Identifier))
				typeName = ReadToken(TokenType::Identifier);
			else
				typeName = ReadTypeKeyword();
			RefPtr<TypeSyntaxNode> rs;
			if (LookAheadToken(TokenType::OpLess))
			{
				RefPtr<GenericTypeSyntaxNode> gtype = new GenericTypeSyntaxNode();
				gtype->Position = typeName.Position;
				gtype->GenericTypeName = typeName.Content;
				ReadToken(TokenType::OpLess);
				gtype->BaseType = ParseType();
				ReadToken(TokenType::OpGreater);
				rs = gtype;
			}
			else
			{
				auto basicType = new BasicTypeSyntaxNode();
				basicType->Position = typeName.Position;
				basicType->TypeName = typeName.Content;
				rs = basicType;
			}
			while (LookAheadToken(TokenType::LBracket))
			{
				RefPtr<ArrayTypeSyntaxNode> arrType = new ArrayTypeSyntaxNode();
				arrType->Position = rs->Position;
				arrType->BaseType = rs;
				ReadToken(TokenType::LBracket);
				if (LookAheadToken(TokenType::IntLiterial))
					arrType->ArrayLength = StringToInt(ReadToken(TokenType::IntLiterial).Content);
				else
					arrType->ArrayLength = 0;
				ReadToken(TokenType::RBracket);
				rs = arrType;
			}
			return rs;
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

		int GetOpLevel(CoreLib::Text::TokenType type)
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
			case TokenType::OpGeq:
			case TokenType::OpLeq:
			case TokenType::OpGreater:
			case TokenType::OpLess:
				return 8;
			case TokenType::OpLsh:
			case TokenType::OpRsh:
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
					while (GetOpLevel(tokenReader.PeekTokenType()) == level)
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
					if (GetOpLevel(tokenReader.PeekTokenType()) == level)
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
					tcexpr->TargetType = ParseType();
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
            String const&       fileName)
        {
            Parser parser(tokens, sink, fileName);
            return parser.Parse();
        }

	}
}