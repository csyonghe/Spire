#include "Parser.h"

namespace Spire
{
	namespace Compiler
	{
		Token & Parser::ReadToken(const char * string)
		{
			if (pos >= tokens.Count())
			{
				sink->diagnose(CodePosition(0, 0, 0, fileName), Diagnostics::tokenNameExpectedButEOF2, string);
				throw 0;
			}
			else if (tokens[pos].Content != string)
			{
				sink->diagnose(tokens[pos].Position, Diagnostics::tokenNameExpected, string);
				throw 20001;
			}
			return tokens[pos++];
		}

		Token & Parser::ReadToken()
		{
			if (pos >= tokens.Count())
			{
				sink->diagnose(CodePosition(0, 0, 0, fileName), Diagnostics::unexpectedEOF);
				throw 0;
			}
			return tokens[pos++];
		}

		Token & Parser::ReadToken(CoreLib::Text::TokenType type)
		{
			if (pos >= tokens.Count())
			{
				sink->diagnose(CodePosition(0, 0, 0, fileName), Diagnostics::tokenTypeExpectedButEOF2, type);
				throw 0;
			}
			else if(tokens[pos].Type != type)
			{
				sink->diagnose(tokens[pos].Position, Diagnostics::tokenTypeExpected, type);
				throw 20001;
			}
			return tokens[pos++];
		}

		bool Parser::LookAheadToken(const char * string, int offset)
		{
			if (pos + offset >= tokens.Count())
			{
				sink->diagnose(CodePosition(0, 0, 0, fileName), Diagnostics::tokenNameExpectedButEOF2, string);
				return false;
			}
			else
			{
				if (tokens[pos + offset].Content == string)
					return true;
				else
					return false;
			}
		}

		bool Parser::LookAheadToken(CoreLib::Text::TokenType type, int offset)
		{
			if (pos + offset >= tokens.Count())
			{
				sink->diagnose(CodePosition(0, 0, 0, fileName), Diagnostics::tokenTypeExpectedButEOF2, type);
				return false;
			}
			else
			{
				if(tokens[pos + offset].Type == type)
					return true;
				else
					return false;
			}
		}

		Token & Parser::ReadTypeKeyword()
		{
			if (pos >= tokens.Count())
			{
				sink->diagnose(CodePosition(0, 0, 0, fileName), Diagnostics::typeNameExpectedButEOF);
				throw 0;
			}
			if(!IsTypeKeyword())
			{
				sink->diagnose(tokens[pos].Position, Diagnostics::typeNameExpectedBut, tokens[pos].Content);
				throw 20001;
			}
			return tokens[pos++];
		}

		bool Parser::IsTypeKeyword()
		{
			if (pos >= tokens.Count())
			{
				sink->diagnose(tokens[pos].Position, Diagnostics::unexpectedEOF);
				throw 0;
			}

			return typeNames.Contains(tokens[pos].Content);
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
				int lastPosBeforeError = 0;
				while (pos < tokens.Count())
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
							if (lastPosBeforeError == 0 && pos < tokens.Count())
								sink->diagnose(tokens[pos].Position, Diagnostics::unexpectedToken, tokens[pos].Content);
							throw 0;
						}
					}
					catch (int)
					{
						if (pos == lastPosBeforeError)
							pos++;
						lastPosBeforeError = pos;
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
			int lastErrorPos = 0;
			while (!LookAheadToken(TokenType::RBrace))
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
						if (lastErrorPos == 0 && pos < tokens.Count())
							sink->diagnose(tokens[pos].Position, Diagnostics::unexpectedTokenExpectedComponentDefinition, tokens[pos].Content);
						throw 0;
					}
				}
				catch (int)
				{
					if (pos == lastErrorPos)
						pos++;
					lastErrorPos = pos;
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
							sink->diagnose(pos < tokens.Count() ? tokens[pos].Position : CodePosition(0, 0, 0, fileName), Diagnostics::unexpectedColon);
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
				pos++;
			}
			else if (LookAheadToken("extern"))
			{
				function->IsExtern = true;
				pos++;
			}
			else
				function->IsExtern = false;
			function->IsInline = true;
			if (LookAheadToken("inline"))
			{
				function->IsInline = true;
				pos++;
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
				while(pos < tokens.Count() && tokens[pos].Type != TokenType::RParent)
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
				while (pos < tokens.Count() && tokens[pos].Type != TokenType::LBrace)
				{
					pos++;
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
			while (!LookAheadToken("}") && pos < tokens.Count())
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
				} while (pos < tokens.Count());
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
				int startPos = pos;
				bool isVarDeclr = false;
				try
				{
					RefPtr<TypeSyntaxNode> type = ParseType();
					if (LookAheadToken(TokenType::Identifier))
					{
						type = nullptr;
						pos = startPos;
						statement = ParseVarDeclrStatement();
						isVarDeclr = true;
					}
				}
				catch (...)
				{
				}
				if (!isVarDeclr)
				{
					pos = startPos;
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
				sink->diagnose(tokens[pos].Position, Diagnostics::syntaxError);
				throw 20002;
			}
			return statement;
		}

		RefPtr<BlockStatementSyntaxNode> Parser::ParseBlockStatement()
		{
			RefPtr<BlockStatementSyntaxNode> blockStatement = new BlockStatementSyntaxNode();
			PushScope();
			ReadToken(TokenType::LBrace);
			if(pos < tokens.Count())
			{
				FillPosition(blockStatement.Ptr());
			}
			int lastErrorPos = 0;
			while (pos < tokens.Count() && !LookAheadToken(TokenType::RBrace))
			{
				try
				{
					blockStatement->Statements.Add(ParseStatement());
				}
				catch (int)
				{
					if (pos == lastErrorPos)
						pos++;
					lastErrorPos = pos;
				}
			}
			ReadToken(TokenType::RBrace);
			PopScope();
			return blockStatement;
		}

		VariableModifier Parser::ReadVariableModifier()
		{
			auto & token = ReadToken(TokenType::Identifier);
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
		
			if (pos < tokens.Count())
				FillPosition(varDeclrStatement.Ptr());
			while (pos < tokens.Count())
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
			while (pos < tokens.Count())
			{
				RefPtr<Variable> var = new Variable();
				FillPosition(var.Ptr());
				Token & name = ReadToken(TokenType::Identifier);
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
				Token & name = ReadToken(TokenType::Identifier);
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
					while (pos < tokens.Count() && GetOpLevel(tokens[pos].Type) == level)
					{
						RefPtr<BinaryExpressionSyntaxNode> tmp = new BinaryExpressionSyntaxNode();
						tmp->LeftExpression = left;
						FillPosition(tmp.Ptr());
						Token & opToken = ReadToken(tokens[pos].Type);
						tmp->Operator = GetOpFromToken(opToken);
						tmp->RightExpression = ParseExpression(level + 1);
						left = tmp;
					}
					return left;
				}
				else
				{
					auto left = ParseExpression(level + 1);
					if (pos < tokens.Count() && GetOpLevel(tokens[pos].Type) == level)
					{
						RefPtr<BinaryExpressionSyntaxNode> tmp = new BinaryExpressionSyntaxNode();
						tmp->LeftExpression = left;
						FillPosition(tmp.Ptr());
						Token & opToken = ReadToken(tokens[pos].Type);
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
				Token & token = tokens[pos++];
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
				if (IsTypeKeyword() && pos + 1 < tokens.Count() && tokens[pos+1].Type == TokenType::RParent)
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
				auto token = tokens[pos++];
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
				auto token = tokens[pos++];
				FillPosition(constExpr.Ptr());
				constExpr->ConstType = ConstantExpressionSyntaxNode::ConstantType::Bool;
				constExpr->IntValue = token.Content == "true" ? 1 : 0;
				rs = constExpr;
			}
			else if (LookAheadToken(TokenType::Identifier))
			{
				RefPtr<VarExpressionSyntaxNode> varExpr = new VarExpressionSyntaxNode();
				FillPosition(varExpr.Ptr());
				auto & token = ReadToken(TokenType::Identifier);
				varExpr->Variable = token.Content;
				rs = varExpr;
			}

			while (pos < tokens.Count() &&
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
					while (pos < tokens.Count())
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
				if (pos < tokens.Count())
				{
					codePos = tokens[pos].Position;
				}
				sink->diagnose(codePos, Diagnostics::syntaxError);
				throw 20005;
			}
			return rs;
		}
	}
}