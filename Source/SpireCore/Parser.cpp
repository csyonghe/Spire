#include "Parser.h"

namespace Spire
{
	namespace Compiler
	{
		const int MaxExprLevel = 12;

		// TODO: implement two pass parsing for file reference and struct type recognition

		class Parser
		{
		private:
			int anonymousParamCounter = 0;
			List<RefPtr<Scope>> scopeStack;
            TokenReader tokenReader;
            DiagnosticSink * sink;
			String fileName;
			HashSet<String> typeNames;
			HashSet<String> classNames;
			bool isInImportOperator = false;
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
		public:
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
		private:
			Token ReadToken();
			Token ReadToken(CoreLib::Text::TokenType type);
			Token ReadToken(const char * string);
			bool LookAheadToken(CoreLib::Text::TokenType type, int offset = 0);
			bool LookAheadToken(const char * string, int offset = 0);
			Token ReadTypeKeyword();
			VariableModifier ReadVariableModifier();
			bool IsTypeKeyword();
			EnumerableDictionary<String, String>	ParseAttribute();
			RefPtr<ProgramSyntaxNode>				ParseProgram();
			RefPtr<ShaderSyntaxNode>				ParseShader();
			RefPtr<PipelineSyntaxNode>				ParsePipeline();
			RefPtr<StageSyntaxNode>					ParseStage();
			RefPtr<ComponentSyntaxNode>				ParseComponent();
			RefPtr<WorldSyntaxNode>					ParseWorld();
			RefPtr<RateSyntaxNode>					ParseRate();
			RefPtr<ImportSyntaxNode>				ParseImport();
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

		Token Parser::ReadToken(const char * string)
		{
			if (tokenReader.PeekTokenType() == TokenType::EndOfFile)
			{
				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::tokenNameExpectedButEOF2, string);
				throw 0;
			}
			else if (tokenReader.PeekToken().Content != string)
			{
				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::tokenNameExpected, string);
				throw 20001;
			}
			return tokenReader.AdvanceToken();
		}

		Token Parser::ReadToken()
		{
			if (tokenReader.PeekTokenType() == TokenType::EndOfFile)
			{
				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::unexpectedEOF);
				throw 0;
			}
			return tokenReader.AdvanceToken();
		}

		Token Parser::ReadToken(CoreLib::Text::TokenType type)
		{
			if (tokenReader.PeekTokenType() == TokenType::EndOfFile)
			{
				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::tokenTypeExpectedButEOF2, type);
				throw 0;
			}
			else if(tokenReader.PeekTokenType() != type)
			{
				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::tokenTypeExpected, type);
				throw 20001;
			}
			return tokenReader.AdvanceToken();
		}

		bool Parser::LookAheadToken(const char * string, int offset)
		{
            TokenReader r = tokenReader;
            for (int ii = 0; ii < offset; ++ii)
                r.AdvanceToken();

			if (r.PeekTokenType() == TokenType::EndOfFile)
			{
				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::tokenNameExpectedButEOF2, string);
				return false;
			}
			else
			{
				if (r.PeekToken().Content == string)
					return true;
				else
					return false;
			}
		}

		bool Parser::LookAheadToken(CoreLib::Text::TokenType type, int offset)
		{
            TokenReader r = tokenReader;
            for (int ii = 0; ii < offset; ++ii)
                r.AdvanceToken();

			if (r.PeekTokenType() == TokenType::EndOfFile)
			{
				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::tokenTypeExpectedButEOF2, type);
				return false;
			}
			else
			{
				if(r.PeekTokenType() == type)
					return true;
				else
					return false;
			}
		}

		Token Parser::ReadTypeKeyword()
		{
			if (tokenReader.PeekTokenType() == TokenType::EndOfFile)
			{
				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::typeNameExpectedButEOF);
				throw 0;
			}
			if(!IsTypeKeyword())
			{
				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::typeNameExpectedBut, tokenReader.PeekTokenType());
				throw 20001;
			}
			return tokenReader.AdvanceToken();
		}

		bool Parser::IsTypeKeyword()
		{
			if (tokenReader.PeekTokenType() == TokenType::EndOfFile)
			{
				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::unexpectedEOF);
				throw 0;
			}

			return typeNames.Contains(tokenReader.PeekToken().Content);
		}

		RefPtr<ProgramSyntaxNode> Parser::Parse()
		{
			return ParseProgram();
		}

		EnumerableDictionary<String, String> Parser::ParseAttribute()
		{
			EnumerableDictionary<String, String> rs;
			while (LookAheadToken(TokenType::LBracket))
			{
				ReadToken(TokenType::LBracket);
				auto name = ReadToken(TokenType::Identifier).Content;
				String value;
				if (LookAheadToken(":"))
				{
					ReadToken(":");
					value = ReadToken(TokenType::StringLiterial).Content;
				}
				rs[name] = value;
				ReadToken(TokenType::RBracket);
			}
			return rs;
		}

		RefPtr<ProgramSyntaxNode> Parser::ParseProgram()
		{
			scopeStack.Add(new Scope());
			RefPtr<ProgramSyntaxNode> program = new ProgramSyntaxNode();
			program->Position = CodePosition(0, 0, 0, fileName);
			program->Scope = scopeStack.Last();
			try
			{
                Token* lastPosBeforeError = NULL;
				while (!tokenReader.IsAtEnd())
				{
					try
					{
						if (LookAheadToken("shader") || LookAheadToken("module"))
							program->Shaders.Add(ParseShader());
						else if (LookAheadToken("pipeline"))
							program->Pipelines.Add(ParsePipeline());
						else if (LookAheadToken("struct"))
							program->Structs.Add(ParseStruct());
						else if (LookAheadToken("using"))
						{
							ReadToken("using");
							program->Usings.Add(ReadToken(TokenType::StringLiterial));
							ReadToken(TokenType::Semicolon);
						}
						else if (IsTypeKeyword() || LookAheadToken("inline") || LookAheadToken("extern")
							|| LookAheadToken("__intrinsic") || LookAheadToken(TokenType::Identifier))
							program->Functions.Add(ParseFunction());
						else if (LookAheadToken(TokenType::Semicolon))
							ReadToken(TokenType::Semicolon);
						else
						{
							if (!lastPosBeforeError)
								sink->diagnose(tokenReader.PeekLoc(), Diagnostics::unexpectedToken, tokenReader.PeekTokenType());
							throw 0;
						}
					}
					catch (int)
					{
                        // TODO(tfoley): add proper error recovery strategy here.

						if (tokenReader.mCursor == lastPosBeforeError)
							tokenReader.AdvanceToken();
						lastPosBeforeError = tokenReader.mCursor;
					}
				}
			}
			catch(int)
			{}
			scopeStack.Clear();
			return program;
		}

		RefPtr<ShaderSyntaxNode> Parser::ParseShader()
		{
			RefPtr<ShaderSyntaxNode> shader = new ShaderSyntaxNode();
			if (LookAheadToken("module"))
			{
				shader->IsModule = true;
				ReadToken("module");
			}
			else
				ReadToken("shader");
			PushScope();
			FillPosition(shader.Ptr());
			shader->Name = ReadToken(TokenType::Identifier);
			try
			{
				if (LookAheadToken(":"))
				{
					ReadToken(":");
					shader->Pipeline = ReadToken(TokenType::Identifier);
				}
			}
			catch (int)
			{
			}
			
			ReadToken(TokenType::LBrace);
			Token* lastErrorPos = 0;
			while (!tokenReader.IsAtEnd() && !LookAheadToken(TokenType::RBrace))
			{
				try
				{
					if (LookAheadToken("inline") || (LookAheadToken("public") && !LookAheadToken("using", 1)) ||
						LookAheadToken("out") || LookAheadToken("@") || IsTypeKeyword()
						|| LookAheadToken("[") || LookAheadToken("require") || LookAheadToken("extern"))
					{
						auto comp = ParseComponent();
						comp->ParentModuleName = shader->Name;
						shader->Members.Add(comp);
					}
					else if (LookAheadToken("using") || (LookAheadToken("public") && LookAheadToken("using", 1)))
					{
						auto imp = ParseImport();
						imp->ParentModuleName = shader->Name;
						shader->Members.Add(imp);
					}
					else
					{
						if (!lastErrorPos)
							sink->diagnose(tokenReader.PeekLoc(), Diagnostics::unexpectedTokenExpectedComponentDefinition, tokenReader.PeekTokenType());
						throw 0;
					}
				}
				catch (int)
				{
					if (tokenReader.mCursor == lastErrorPos)
						tokenReader.AdvanceToken();
					lastErrorPos = tokenReader.mCursor;
				}
			}
			ReadToken(TokenType::RBrace);
			
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
			if (LookAheadToken(TokenType::Colon))
			{
				ReadToken(TokenType::Colon);
				pipeline->ParentPipeline = ReadToken(TokenType::Identifier);
			}
			ReadToken(TokenType::LBrace);
			while (!LookAheadToken(TokenType::RBrace))
			{
				auto attribs = ParseAttribute();
				if (LookAheadToken("input") || LookAheadToken("world"))
				{
					auto w = ParseWorld();
					w->LayoutAttributes = attribs;
					pipeline->Worlds.Add(w);
				}
				else if (LookAheadToken("import"))
				{
					auto op = ParseImportOperator();
					op->LayoutAttributes = attribs;
					pipeline->ImportOperators.Add(op);
				}
				else if (LookAheadToken("stage"))
				{
					pipeline->Stages.Add(ParseStage());
				}
				else
				{
					auto comp = ParseComponent();
					comp->LayoutAttributes = attribs;
					pipeline->AbstractComponents.Add(comp);
				}
			}
			ReadToken(TokenType::RBrace);
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
			while (!LookAheadToken(TokenType::RBrace))
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
			ReadToken(TokenType::RBrace);
			return stage;
		}

		RefPtr<ComponentSyntaxNode> Parser::ParseComponent()
		{
			RefPtr<ComponentSyntaxNode> component = new ComponentSyntaxNode();
			PushScope();
			component->LayoutAttributes = ParseAttribute();
			while (LookAheadToken("inline") || LookAheadToken("out") || LookAheadToken("require") || LookAheadToken("public") ||
				LookAheadToken("extern"))
			{
				if (LookAheadToken("inline"))
				{
					component->IsInline = true;
					ReadToken("inline");
				}
				else if (LookAheadToken("out"))
				{
					component->IsOutput = true;
					ReadToken("out");
				}
				else if (LookAheadToken("public"))
				{
					component->IsPublic = true;
					ReadToken("public");
				}
				else if (LookAheadToken("require"))
				{
					component->IsParam = true;
					ReadToken("require");
				}
				else if (LookAheadToken("extern"))
				{
					component->IsInput = true;
					ReadToken("extern");
				}
				else
					break;
			}
			if (LookAheadToken("@"))
				component->Rate = ParseRate();
			component->TypeNode = ParseType();
			FillPosition(component.Ptr());
			component->Name = ReadToken(TokenType::Identifier);
			if (LookAheadToken(":"))
			{
				ReadToken(":");
				component->AlternateName = ReadToken(TokenType::Identifier);
			}
			if (LookAheadToken(TokenType::LParent))
			{
				ReadToken(TokenType::LParent);
				while (!LookAheadToken(TokenType::RParent))
				{
					component->Parameters.Add(ParseParameter());
					if (LookAheadToken(TokenType::Comma))
						ReadToken(TokenType::Comma);
					else
						break;
				}
				ReadToken(TokenType::RParent);
			}
			if (LookAheadToken(TokenType::OpAssign))
			{
				ReadToken(TokenType::OpAssign);
				component->Expression = ParseExpression();
				ReadToken(TokenType::Semicolon);
			}
			else if (LookAheadToken(TokenType::LBrace))
			{
				component->BlockStatement = ParseBlockStatement();
			}
			else
				ReadToken(TokenType::Semicolon);
			PopScope();
			return component;
		}

		RefPtr<WorldSyntaxNode> Parser::ParseWorld()
		{
			RefPtr<WorldSyntaxNode> world = new WorldSyntaxNode();
			world->LayoutAttributes = ParseAttribute();
			world->IsAbstract = LookAheadToken("input");
			if (world->IsAbstract)
				ReadToken("input");
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
				if (LookAheadToken(TokenType::OpMul))
				{
					ReadToken(TokenType::OpMul);
					rw.Pinned = true;
				}
				return rw;
			};
			if (LookAheadToken(TokenType::LParent))
			{
				ReadToken(TokenType::LParent);
				while (!LookAheadToken(TokenType::RParent))
				{
					RateWorld rw = readWorldRate();
					rate->Worlds.Add(rw);
					if (LookAheadToken(TokenType::Comma))
					{
						ReadToken(TokenType::Comma);
					}
					else
						break;
				}
				ReadToken(TokenType::RParent);
			}
			else
				rate->Worlds.Add(readWorldRate());
			return rate;
		}

		RefPtr<ImportSyntaxNode> Parser::ParseImport()
		{
			RefPtr<ImportSyntaxNode> rs = new ImportSyntaxNode();
			if (LookAheadToken("public"))
			{
				rs->IsPublic = true;
				ReadToken("public");
			}
			ReadToken("using");
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
				while (!LookAheadToken(TokenType::RParent))
				{
					RefPtr<ImportArgumentSyntaxNode> arg = new ImportArgumentSyntaxNode();
					FillPosition(arg.Ptr());
					auto expr = ParseExpression();
					if (LookAheadToken(":"))
					{
						if (auto varExpr = dynamic_cast<VarExpressionSyntaxNode*>(expr.Ptr()))
						{
							arg->ArgumentName.Content = varExpr->Variable;
							arg->ArgumentName.Position = varExpr->Position;
						}
						else
							sink->diagnose(tokenReader.PeekLoc(), Diagnostics::unexpectedColon);
						ReadToken(":");
						arg->Expression = ParseExpression();
					}
					else
						arg->Expression = expr;
					rs->Arguments.Add(arg);
					if (LookAheadToken(TokenType::Comma))
						ReadToken(TokenType::Comma);
					else
						break;
				}
				ReadToken(TokenType::RParent);
				ReadToken(TokenType::Semicolon);
			}
			return rs;
		}

		RefPtr<ImportStatementSyntaxNode> Parser::ParseImportStatement()
		{
			RefPtr<ImportStatementSyntaxNode> rs = new ImportStatementSyntaxNode();
			FillPosition(rs.Ptr());
			rs->Import = ParseImport();
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
			while (!LookAheadToken(TokenType::RParent))
			{
				op->Parameters.Add(ParseParameter());
				if (LookAheadToken(TokenType::Comma))
					ReadToken(TokenType::Comma);
				else
					break;
			}
			ReadToken(TokenType::RParent);
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
			if (LookAheadToken("__intrinsic"))
			{
				function->HasSideEffect = false;
				function->IsExtern = true;
                tokenReader.AdvanceToken();
			}
			else if (LookAheadToken("extern"))
			{
				function->IsExtern = true;
                tokenReader.AdvanceToken();
			}
			else
				function->IsExtern = false;
			function->IsInline = true;
			if (LookAheadToken("inline"))
			{
				function->IsInline = true;
                tokenReader.AdvanceToken();
			}
			
			PushScope();
			function->ReturnTypeNode = ParseType();
			try
			{
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
				function->Name = name.Content;
				ReadToken(TokenType::LParent);
				while(!tokenReader.IsAtEnd() && tokenReader.PeekTokenType() != TokenType::RParent)
				{
					function->Parameters.Add(ParseParameter());
					if (LookAheadToken(TokenType::Comma))
						ReadToken(TokenType::Comma);
					else
						break;
				}
				ReadToken(TokenType::RParent);
			}
			catch(int e)
			{
				if (e == 0)
					return function;
				while (!tokenReader.IsAtEnd() && tokenReader.PeekTokenType() != TokenType::LBrace)
				{
                    tokenReader.AdvanceToken();
				}
			}
			if (parseBody)
			{
				if (!function->IsExtern)
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
			ReadToken("{");
			while (!LookAheadToken("}") && !tokenReader.IsAtEnd())
			{
				RefPtr<TypeSyntaxNode> type = ParseType();
				do
				{
					RefPtr<StructField> field = new StructField();
					FillPosition(field.Ptr());
					field->TypeNode = type;
					field->Name = ReadToken(TokenType::Identifier);
					rs->Fields.Add(field);
					if (!LookAheadToken(TokenType::Comma))
						break;
					ReadToken(TokenType::Comma);
				} while (!tokenReader.IsAtEnd());
				ReadToken(TokenType::Semicolon);
			}
			ReadToken("}");
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
				try
				{
					RefPtr<TypeSyntaxNode> type = ParseType();
					if (LookAheadToken(TokenType::Identifier))
					{
						type = nullptr;
						tokenReader.mCursor = startPos;
						statement = ParseVarDeclrStatement();
						isVarDeclr = true;
					}
				}
				catch (...)
				{
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
				sink->diagnose(tokenReader.PeekLoc(), Diagnostics::syntaxError);
				throw 20002;
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
			Token* lastErrorPos = 0;
			while (!tokenReader.IsAtEnd() && !LookAheadToken(TokenType::RBrace))
			{
				try
				{
					blockStatement->Statements.Add(ParseStatement());
				}
				catch (int)
				{
					if (tokenReader.mCursor == lastErrorPos)
						tokenReader.AdvanceToken();
					lastErrorPos = tokenReader.mCursor;
				}
			}
			ReadToken(TokenType::RBrace);
			PopScope();
			return blockStatement;
		}

		VariableModifier Parser::ReadVariableModifier()
		{
			auto token = ReadToken(TokenType::Identifier);
			if (token.Content == "in")
				return VariableModifier::In;
			else if (token.Content == "out")
				return VariableModifier::Out;
			else if (token.Content == "uniform")
				return VariableModifier::Uniform;
			else if (token.Content == "parameter")
				return VariableModifier::Parameter;
			else if (token.Content == "const")
				return VariableModifier::Const;
			else if (token.Content == "centroid")
				return VariableModifier::Centroid;
			else if (token.Content == "instance")
				return VariableModifier::Instance;
			else if (token.Content == "__builtin")
				return VariableModifier::Builtin;
			return VariableModifier::None; 
		}

		RefPtr<VarDeclrStatementSyntaxNode> Parser::ParseVarDeclrStatement()
		{
			RefPtr<VarDeclrStatementSyntaxNode>varDeclrStatement = new VarDeclrStatementSyntaxNode();
		
			FillPosition(varDeclrStatement.Ptr());
			while (!tokenReader.IsAtEnd())
			{
				if (LookAheadToken("layout"))
				{
					ReadToken("layout");
					ReadToken(TokenType::LParent);
					StringBuilder layoutSB;
					while (!LookAheadToken(TokenType::RParent))
					{
						layoutSB.Append(ReadToken(TokenType::Identifier).Content);
						if (LookAheadToken(TokenType::OpAssign))
						{
							layoutSB.Append(ReadToken(TokenType::OpAssign).Content);
							layoutSB.Append(ReadToken(TokenType::IntLiterial).Content);
						}
						if (!LookAheadToken(TokenType::Comma))
							break;
						else
							layoutSB.Append(", ");
					}
					ReadToken(TokenType::RParent);
					varDeclrStatement->LayoutString = layoutSB.ProduceString();
				}
				else
					break;
			}
			varDeclrStatement->TypeNode = ParseType();
			while (!tokenReader.IsAtEnd())
			{
				RefPtr<Variable> var = new Variable();
				FillPosition(var.Ptr());
				Token name = ReadToken(TokenType::Identifier);
				var->Name = name.Content;
				if (LookAheadToken(TokenType::OpAssign))
				{
					ReadToken(TokenType::OpAssign);
					var->Expression = ParseExpression();
				}

				varDeclrStatement->Variables.Add(var);
				if (LookAheadToken(TokenType::Comma))
					ReadToken(TokenType::Comma);
				else
					break;
			}
			ReadToken(TokenType::Semicolon);
			
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
				stmt->TypeDef = ParseType();
				stmt->IterationVariable = ReadToken(TokenType::Identifier);
				ReadToken(TokenType::OpAssign);
				stmt->InitialExpression = ParseExpression();
				RefPtr<BinaryExpressionSyntaxNode> assignment = new BinaryExpressionSyntaxNode();
				assignment->Operator = Operator::Assign;
				FillPosition(assignment.Ptr());
				assignment->Position = stmt->IterationVariable.Position;
				RefPtr<VarExpressionSyntaxNode> varExpr = new VarExpressionSyntaxNode();
				FillPosition(varExpr.Ptr());
				varExpr->Position = stmt->IterationVariable.Position;
				varExpr->Variable = stmt->IterationVariable.Content;
				assignment->LeftExpression = varExpr;
				assignment->RightExpression = stmt->InitialExpression;
				stmt->InitialExpression = assignment;
			}
			else
			{
				if (!LookAheadToken(TokenType::Semicolon))
					stmt->InitialExpression = ParseExpression();
			}
			ReadToken(TokenType::Semicolon);
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
			if (LookAheadToken("in"))
			{
				parameter->Qualifier = ParameterQualifier::In;
				ReadToken("in");
			}
			else if (LookAheadToken("inout"))
			{
				parameter->Qualifier = ParameterQualifier::InOut;
				ReadToken("inout");
			}
			else if (LookAheadToken("out"))
			{
				parameter->Qualifier = ParameterQualifier::Out;
				ReadToken("out");
			}
			else if (LookAheadToken("uniform"))
			{
				parameter->Qualifier = ParameterQualifier::Uniform;
				ReadToken("uniform");
				if (LookAheadToken("in"))
					ReadToken("in");
			}
			parameter->TypeNode = ParseType();
			if (LookAheadToken(TokenType::Identifier))
			{
				Token name = ReadToken(TokenType::Identifier);
				parameter->Name = name.Content;
			}
			else
				parameter->Name = "_anonymousParam" + String(anonymousParamCounter++);
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
				CodePosition codePos;
				if (!tokenReader.IsAtEnd())
				{
					codePos = tokenReader.PeekLoc();
				}
				sink->diagnose(codePos, Diagnostics::syntaxError);
				throw 20005;
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