#include "Parser.h"

namespace Spire
{
	namespace Compiler
	{
		Token & Parser::ReadToken(const wchar_t * string)
		{
			if (pos >= tokens.Count())
			{
				errors.Add(CompileError(String(L"\"") + string + String(L"\" expected but end of file encountered."), 20001, CodePosition(0, 0, fileName)));
				throw 0;
			}
			else if (tokens[pos].Content != string)
			{
				errors.Add(CompileError(String(L"\"") + string + String(L"\" expected"), 20001, tokens[pos].Position));
				throw 20001;
			}
			return tokens[pos++];
		}

		Token & Parser::ReadToken()
		{
			if (pos >= tokens.Count())
			{
				errors.Add(CompileError(String(L" Unexpected end of file."), 20001, CodePosition(0, 0, fileName)));
				throw 0;
			}
			return tokens[pos++];
		}

		Token & Parser::ReadToken(TokenType type)
		{
			if (pos >= tokens.Count())
			{
				errors.Add(CompileError(TokenTypeToString(type) + String(L" expected but end of file encountered."), 20001, CodePosition(0, 0, fileName)));
				throw 0;
			}
			else if(tokens[pos].Type != type)
			{
				errors.Add(CompileError(TokenTypeToString(type) + String(L" expected"), 20001, tokens[pos].Position));
				throw 20001;
			}
			return tokens[pos++];
		}

		bool Parser::LookAheadToken(const wchar_t * string, int offset)
		{
			if (pos + offset >= tokens.Count())
			{
				errors.Add(CompileError(String(L"\'") + string + String(L"\' expected but end of file encountered."), 20001, CodePosition(0, 0, fileName)));
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

		bool Parser::LookAheadToken(TokenType type, int offset)
		{
			if (pos + offset >= tokens.Count())
			{
				errors.Add(CompileError(TokenTypeToString(type) + String(L" expected but end of file encountered."), 20001, CodePosition(0, 0, fileName)));
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
				errors.Add(CompileError(String(L"type name expected but end of file encountered."), 20001, CodePosition(0, 0, fileName)));
				throw 0;
			}
			if(!IsTypeKeyword())
			{
				errors.Add(CompileError(String(L"type name expected but '" + tokens[pos].Content + L"' encountered."), 20001, tokens[pos].Position));
				throw 20001;
			}
			return tokens[pos++];
		}

		bool Parser::IsTypeKeyword()
		{
			if (pos >= tokens.Count())
			{
				errors.Add(CompileError(String(L"Unexpected end of file."), 20001, tokens[pos].Position));
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
				if (LookAheadToken(L":"))
				{
					ReadToken(L":");
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
			program->Position = CodePosition(0, 0, fileName);
			program->Scope = scopeStack.Last();
			try
			{
				int lastPosBeforeError = 0;
				while (pos < tokens.Count())
				{
					try
					{
						if (LookAheadToken(L"shader") || LookAheadToken(L"module"))
							program->Shaders.Add(ParseShader());
						else if (LookAheadToken(L"pipeline"))
							program->Pipelines.Add(ParsePipeline());
						else if (LookAheadToken(L"struct"))
							program->Structs.Add(ParseStruct());
						else if (LookAheadToken(L"using"))
						{
							ReadToken(L"using");
							program->Usings.Add(ReadToken(TokenType::StringLiterial));
							ReadToken(TokenType::Semicolon);
						}
						else if (IsTypeKeyword() || LookAheadToken(L"inline") || LookAheadToken(L"extern")
							|| LookAheadToken(L"__intrinsic") || LookAheadToken(TokenType::Identifier))
							program->Functions.Add(ParseFunction());
						else if (LookAheadToken(TokenType::Semicolon))
							ReadToken(TokenType::Semicolon);
						else
						{
							if (lastPosBeforeError == 0 && pos < tokens.Count())
								errors.Add(CompileError(L"unexpected token \'" + tokens[pos].Content + L"\'.", 20003, tokens[pos].Position));
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
			if (LookAheadToken(L"module"))
			{
				shader->IsModule = true;
				ReadToken(L"module");
			}
			else
				ReadToken(L"shader");
			PushScope();
			FillPosition(shader.Ptr());
			shader->Name = ReadToken(TokenType::Identifier);
			try
			{
				if (LookAheadToken(L":"))
				{
					ReadToken(L":");
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
					if (LookAheadToken(L"inline") || (LookAheadToken(L"public") && !LookAheadToken(L"using", 1)) ||
						LookAheadToken(L"out") || LookAheadToken(L"@") || IsTypeKeyword()
						|| LookAheadToken(L"[") || LookAheadToken(L"require"))
						shader->Members.Add(ParseComponent());
					else if (LookAheadToken(L"using") || (LookAheadToken(L"public") && LookAheadToken(L"using", 1)))
					{
						shader->Members.Add(ParseImport());
					}
					else
					{
						if (lastErrorPos == 0 && pos < tokens.Count())
							errors.Add(CompileError(L"unexpected token \'" + tokens[pos].Content + L"\', only component definitions are allowed in a shader scope.", 
								20004, tokens[pos].Position));
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
			ReadToken(L"pipeline");
			FillPosition(pipeline.Ptr());
			pipeline->Name = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::LBrace);
			while (!LookAheadToken(TokenType::RBrace))
			{
				auto attribs = ParseAttribute();
				if (LookAheadToken(L"input") || LookAheadToken(L"world"))
				{
					auto w = ParseWorld();
					w->LayoutAttributes = attribs;
					pipeline->Worlds.Add(w);
				}
				else if (LookAheadToken(L"import"))
				{
					auto op = ParseImportOperator();
					op->LayoutAttributes = attribs;
					pipeline->ImportOperators.Add(op);
				}
				else
				{
					auto comp = ParseComponent();
					comp->LayoutAttributes = attribs;
					pipeline->AbstractComponents.Add(comp);
				}
			}
			ReadToken(TokenType::RBrace);
			return pipeline;
		}

		RefPtr<ComponentSyntaxNode> Parser::ParseComponent()
		{
			RefPtr<ComponentSyntaxNode> component = new ComponentSyntaxNode();
			component->LayoutAttributes = ParseAttribute();
			while (LookAheadToken(L"inline") || LookAheadToken(L"out") || LookAheadToken(L"require") || LookAheadToken(L"public"))
			{
				if (LookAheadToken(L"inline"))
				{
					component->IsInline = true;
					ReadToken(L"inline");
				}
				else if (LookAheadToken(L"out"))
				{
					component->IsOutput = true;
					ReadToken(L"out");
				}
				else if (LookAheadToken(L"public"))
				{
					component->IsPublic = true;
					ReadToken(L"public");
				}
				else if (LookAheadToken(L"require"))
				{
					component->IsParam = true;
					ReadToken(L"require");
				}
				else
					break;
			}
			if (LookAheadToken(L"@"))
				component->Rate = ParseRate();
			component->Type = ParseType();
			FillPosition(component.Ptr());
			component->Name = ReadToken(TokenType::Identifier);
			if (LookAheadToken(L":"))
			{
				ReadToken(L":");
				component->AlternateName = ReadToken(TokenType::Identifier);
			}
			if (!component->IsParam && LookAheadToken(TokenType::OpAssign))
			{
				ReadToken(TokenType::OpAssign);
				component->Expression = ParseExpression();
				ReadToken(TokenType::Semicolon);
			}
			else if (!component->IsParam && LookAheadToken(TokenType::LBrace))
			{
				component->BlockStatement = ParseBlockStatement();
			}
			else
				ReadToken(TokenType::Semicolon);
			return component;
		}

		RefPtr<WorldSyntaxNode> Parser::ParseWorld()
		{
			RefPtr<WorldSyntaxNode> world = new WorldSyntaxNode();
			world->LayoutAttributes = ParseAttribute();
			world->IsAbstract = LookAheadToken(L"input");
			if (world->IsAbstract)
				ReadToken(L"input");
			ReadToken(L"world");
			FillPosition(world.Ptr());
			world->Name = ReadToken(TokenType::Identifier);
			if (LookAheadToken(TokenType::Colon))
			{
				ReadToken(TokenType::Colon);
				world->TargetMachine = ReadToken(TokenType::StringLiterial).Content;
			}
			if (LookAheadToken(L"using"))
			{
				ReadToken(L"using");
				while (LookAheadToken(TokenType::Identifier))
				{
					world->Usings.Add(ReadToken(TokenType::Identifier));
					if (LookAheadToken(TokenType::Comma))
						ReadToken(TokenType::Comma);
					else
						break;
				}
			}
			if (!world->IsAbstract)
			{
				ReadToken(L"export");
				world->ExportOperator = ReadToken(TokenType::Identifier);
			}
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
			if (LookAheadToken(L"public"))
			{
				rs->IsPublic = true;
				ReadToken(L"public");
			}
			ReadToken(L"using");
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
					if (LookAheadToken(L":"))
					{
						if (auto varExpr = dynamic_cast<VarExpressionSyntaxNode*>(expr.Ptr()))
						{
							arg->ArgumentName.Content = varExpr->Variable;
							arg->ArgumentName.Position = varExpr->Position;
						}
						else
							errors.Add(CompileError(L"unexpected ':'.", 20011, pos < tokens.Count() ? tokens[pos].Position : CodePosition(0, 0, fileName)));
						ReadToken(L":");
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
			ReadToken(L"import");
			FillPosition(op.Ptr());
			op->Name = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::LParent);
			op->SourceWorld = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::RightArrow);
			op->DestWorld = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::RParent);
			if (LookAheadToken(L"using"))
			{
				ReadToken(L"using");
				do
				{
					op->Usings.Add(ReadToken(TokenType::Identifier));
					if (LookAheadToken(TokenType::Comma))
						ReadToken(TokenType::Comma);
					else
						break;
				} while (LookAheadToken(TokenType::Identifier));
			}
			if (LookAheadToken(TokenType::LBrace))
			{
				ReadToken(TokenType::LBrace);
				while (!LookAheadToken(TokenType::RBrace))
				{
					auto name = ReadToken(TokenType::Identifier);
					ReadToken(TokenType::Colon);
					auto value = ReadToken(TokenType::StringLiterial);
					ReadToken(TokenType::Semicolon);
					op->Arguments[name.Content] = value.Content;
					if (LookAheadToken(TokenType::Comma))
						ReadToken(TokenType::Comma);
					else
						break;

				}
				ReadToken(TokenType::RBrace);
				if (LookAheadToken(TokenType::Semicolon))
					ReadToken(TokenType::Semicolon);
			}
			else 
				ReadToken(TokenType::Semicolon);
			return op;
		}

		RefPtr<FunctionSyntaxNode> Parser::ParseFunction()
		{
			RefPtr<FunctionSyntaxNode> function = new FunctionSyntaxNode();
			if (LookAheadToken(L"__intrinsic"))
			{
				function->HasSideEffect = false;
				function->IsExtern = true;
				pos++;
			}
			else if (LookAheadToken(L"extern"))
			{
				function->IsExtern = true;
				pos++;
			}
			else
				function->IsExtern = false;
			function->IsInline = true;
			if (LookAheadToken(L"inline"))
			{
				function->IsInline = true;
				pos++;
			}
			
			PushScope();
			function->ReturnType = ParseType();
			try
			{
				FillPosition(function.Ptr());
				Token name = ReadToken();
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
			if (!function->IsExtern)
				function->Body = ParseBlockStatement();
			else
				ReadToken(TokenType::Semicolon);
			PopScope();
			return function;
		}

		RefPtr<StructSyntaxNode> Parser::ParseStruct()
		{
			RefPtr<StructSyntaxNode> rs = new StructSyntaxNode();
			FillPosition(rs.Ptr());
			ReadToken(L"struct");
			rs->Name = ReadToken(TokenType::Identifier);
			ReadToken(L"{");
			while (!LookAheadToken(L"}") && pos < tokens.Count())
			{
				RefPtr<TypeSyntaxNode> type = ParseType();
				do
				{
					RefPtr<StructField> field = new StructField();
					FillPosition(field.Ptr());
					field->Type = type;
					field->Name = ReadToken(TokenType::Identifier);
					rs->Fields.Add(field);
					if (!LookAheadToken(TokenType::Comma))
						break;
					ReadToken(TokenType::Comma);
				} while (pos < tokens.Count());
				ReadToken(TokenType::Semicolon);
			}
			ReadToken(L"}");
			return rs;
		}

		RefPtr<StatementSyntaxNode> Parser::ParseStatement()
		{
			RefPtr<StatementSyntaxNode> statement;
			if (LookAheadToken(TokenType::LBrace))
				statement = ParseBlockStatement();
			else if (IsTypeKeyword() || LookAheadToken(L"const"))
				statement = ParseVarDeclrStatement();
			else if (LookAheadToken(TokenType::KeywordIf))
				statement = ParseIfStatement();
			else if (LookAheadToken(TokenType::KeywordFor))
				statement = ParseForStatement();
			else if (LookAheadToken(TokenType::KeywordWhile))
				statement = ParseWhileStatement();
			else if (LookAheadToken(TokenType::KeywordDo))
				statement = ParseDoWhileStatement();
			else if (LookAheadToken(TokenType::KeywordBreak))
				statement = ParseBreakStatement();
			else if (LookAheadToken(TokenType::KeywordContinue))
				statement = ParseContinueStatement();
			else if (LookAheadToken(TokenType::KeywordReturn))
				statement = ParseReturnStatement();
			else if (LookAheadToken(L"using") || (LookAheadToken(L"public") && LookAheadToken(L"using", 1)))
				statement = ParseImportStatement();
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
				errors.Add(CompileError(String(L"syntax error."), 20002, tokens[pos].Position));
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
			if (token.Content == L"in")
				return VariableModifier::In;
			else if (token.Content == L"out")
				return VariableModifier::Out;
			else if (token.Content == L"uniform")
				return VariableModifier::Uniform;
			else if (token.Content == L"parameter")
				return VariableModifier::Parameter;
			else if (token.Content == L"const")
				return VariableModifier::Const;
			else if (token.Content == L"centroid")
				return VariableModifier::Centroid;
			else if (token.Content == L"instance")
				return VariableModifier::Instance;
			else if (token.Content == L"__builtin")
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
				if (LookAheadToken(L"layout"))
				{
					ReadToken(L"layout");
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
							layoutSB.Append(L", ");
					}
					ReadToken(TokenType::RParent);
					varDeclrStatement->LayoutString = layoutSB.ProduceString();
				}
				else
					break;
			}
			varDeclrStatement->Type = ParseType();
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
			ReadToken(TokenType::KeywordIf);
			ReadToken(TokenType::LParent);
			ifStatement->Predicate = ParseExpression();
			ReadToken(TokenType::RParent);
			ifStatement->PositiveStatement = ParseStatement();
			if (LookAheadToken(TokenType::KeywordElse))
			{
				ReadToken(TokenType::KeywordElse);
				ifStatement->NegativeStatement = ParseStatement();
			}
			return ifStatement;
		}

		RefPtr<ForStatementSyntaxNode> Parser::ParseForStatement()
		{
			RefPtr<ForStatementSyntaxNode> stmt = new ForStatementSyntaxNode();
			PushScope();
			FillPosition(stmt.Ptr());
			ReadToken(TokenType::KeywordFor);
			ReadToken(TokenType::LParent);
			if (IsTypeKeyword())
				stmt->TypeDef = ParseType();
			stmt->IterationVariable = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::OpAssign);
			stmt->InitialExpression = ParseExpression();
			ReadToken(TokenType::Colon);
			stmt->EndExpression = ParseExpression();
			if (LookAheadToken(TokenType::Colon))
			{
				stmt->StepExpression = stmt->EndExpression;
				ReadToken(TokenType::Colon);
				stmt->EndExpression = ParseExpression();
			}
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
			ReadToken(TokenType::KeywordWhile);
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
			ReadToken(TokenType::KeywordDo);
			doWhileStatement->Statement = ParseStatement();
			ReadToken(TokenType::KeywordWhile);
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
			ReadToken(TokenType::KeywordBreak);
			ReadToken(TokenType::Semicolon);
			return breakStatement;
		}

		RefPtr<ContinueStatementSyntaxNode>	Parser::ParseContinueStatement()
		{
			RefPtr<ContinueStatementSyntaxNode> continueStatement = new ContinueStatementSyntaxNode();
			FillPosition(continueStatement.Ptr());
			ReadToken(TokenType::KeywordContinue);
			ReadToken(TokenType::Semicolon);
			return continueStatement;
		}

		RefPtr<ReturnStatementSyntaxNode> Parser::ParseReturnStatement()
		{
			RefPtr<ReturnStatementSyntaxNode> returnStatement = new ReturnStatementSyntaxNode();
			FillPosition(returnStatement.Ptr());
			ReadToken(TokenType::KeywordReturn);
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
			
			parameter->Type = ParseType();
			Token & name = ReadToken(TokenType::Identifier);
			parameter->Name = name.Content;
			FillPosition(parameter.Ptr());
			
			return parameter;
		}

		RefPtr<TypeSyntaxNode> Parser::ParseType()
		{
			RefPtr<TypeSyntaxNode> type = new TypeSyntaxNode();
		
			FillPosition(type.Ptr());
			if (LookAheadToken(TokenType::Identifier))
				type->TypeName = ReadToken(TokenType::Identifier).Content;
			else
				type->TypeName = ReadTypeKeyword().Content;
	
			if (LookAheadToken(TokenType::OpLess))
			{
				ReadToken(TokenType::OpLess);
				type->GenericBaseType = ReadToken(TokenType::Identifier).Content;
				ReadToken(TokenType::OpGreater);
			}
			if(LookAheadToken(TokenType::LBracket))
			{
				ReadToken(TokenType::LBracket);
				type->IsArray = true;
				if (LookAheadToken(TokenType::IntLiterial))
					type->ArrayLength = atoi(ReadToken(TokenType::IntLiterial).Content.ToMultiByteString());
				else
					type->ArrayLength = 0;
				ReadToken(TokenType::RBracket);
			}
			return type;
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

		int GetOpLevel(TokenType type)
		{
			switch(type)
			{
			case TokenType::OpAssign:
			case TokenType::OpMulAssign:
			case TokenType::OpDivAssign:
			case TokenType::OpAddAssign:
			case TokenType::OpSubAssign:
			case TokenType::OpModAssign:
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
				throw L"Illegal TokenType.";
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
					if (!invokeExpr->FunctionExpr)
					{
						errors.Add(CompileError(L"syntax error.", 20002, tokens[pos].Position));
					}
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
				errors.Add(CompileError(String(L"syntax error."), 20002, codePos));
				throw 20005;
			}
			return rs;
		}
	}
}