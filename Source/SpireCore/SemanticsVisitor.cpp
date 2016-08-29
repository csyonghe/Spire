#include "SyntaxVisitors.h"
#include "IL.h"

namespace Spire
{
	namespace Compiler
	{
		bool IsNumeric(BaseType t)
		{
			return t == BaseType::Int || t == BaseType::Float || t == BaseType::UInt;
		}
		class SemanticsVisitor : public SyntaxVisitor
		{
			ProgramSyntaxNode * program = nullptr;
			FunctionSyntaxNode * function = nullptr;
			FunctionSymbol * currentFunc = nullptr;
			ShaderSymbol * currentShader = nullptr;
			ShaderComponentSymbol * currentComp = nullptr;
			ComponentSyntaxNode * currentCompNode = nullptr;
			List<SyntaxNode *> loops;
			SymbolTable * symbolTable;
		public:
			SemanticsVisitor(SymbolTable * symbols, ErrorWriter * pErr)
				:SyntaxVisitor(pErr), symbolTable(symbols)
			{
			}
			// return true if world0 depends on world1 (there exists a series of import operators that converts world1 variables to world0)
			bool IsWorldDependent(PipelineSymbol * pipeline, String world0, String world1)
			{
				HashSet<String> depWorldsSet;
				List<String> depWorlds;
				depWorlds.Add(world0);
				for (int i = 0; i < depWorlds.Count(); i++)
				{
					auto & dep = pipeline->WorldDependency[world0].GetValue();
					if (dep.Contains(world1))
						return true;
					else
					{
						for (auto w : dep)
							if (depWorldsSet.Add(w))
								depWorlds.Add(w);
					}
				}
				return false;
			}

			void VisitPipeline(PipelineSyntaxNode * pipeline)
			{
				RefPtr<PipelineSymbol> psymbol = new PipelineSymbol();
				psymbol->SyntaxNode = pipeline;
				symbolTable->Pipelines.Add(pipeline->Name.Content, psymbol);
				for (auto world : pipeline->Worlds)
				{
					WorldSymbol worldSym;
					worldSym.IsAbstract = world->IsAbstract;
					worldSym.SyntaxNode = world.Ptr();
					if (!psymbol->Worlds.ContainsKey(world->Name.Content))
					{
						psymbol->Worlds.Add(world->Name.Content, worldSym);
						psymbol->WorldDependency.Add(world->Name.Content, EnumerableHashSet<String>());
						psymbol->ReachableWorlds.Add(world->Name.Content, EnumerableHashSet<String>());
					}
					else
					{
						Error(33001, L"world \'" + world->Name.Content + L"\' is already defined.", world.Ptr());
					}
				}
				for (auto comp : pipeline->AbstractComponents)
				{
					if (comp->IsParam || (comp->Rate && comp->Rate->Worlds.Count() == 1
						&& psymbol->IsAbstractWorld(comp->Rate->Worlds.First().World.Content)))
						AddNewComponentSymbol(psymbol->Components, comp);
					else
						Error(33003, L"cannot define components in a pipeline.",
							comp.Ptr());
				}
				for (auto world : pipeline->Worlds)
				{
					for (auto & varUsing : world->Usings)
					{
						if (!psymbol->Components.ContainsKey(varUsing.Content))
							Error(33043, L"'using': unknown component '" + varUsing.Content + L"\'.", varUsing);
					}
				}
				// add initial world dependency edges
				for (auto op : pipeline->ImportOperators)
				{
					if (!psymbol->WorldDependency.ContainsKey(op->DestWorld.Content))
						Error(33004, L"undefined world name '" + op->DestWorld.Content + L"'.", op->DestWorld);
					else
					{
						if (psymbol->Worlds[op->DestWorld.Content].GetValue().IsAbstract)
							Error(33005, L"abstract world cannot appear as target as an import operator.", op->DestWorld);
						else if (!psymbol->WorldDependency.ContainsKey(op->SourceWorld.Content))
							Error(33006, L"undefined world name '" + op->SourceWorld.Content + L"'.", op->SourceWorld);
						else
						{
							if (IsWorldDependent(psymbol.Ptr(), op->SourceWorld.Content, op->DestWorld.Content))
							{
								Error(33007, L"import operator '" + op->Name.Content + L"' creates a circular dependency between world '" + op->SourceWorld.Content + L"' and '" + op->DestWorld.Content + L"'",
									op->Name);
							}
							else
								psymbol->WorldDependency[op->DestWorld.Content].GetValue().Add(op->SourceWorld.Content);
						}
					}
				
				}
				// propagate world dependency graph
				bool changed = true;
				while (changed)
				{
					changed = false;
					for (auto world : pipeline->Worlds)
					{
						EnumerableHashSet<String> & dependentWorlds = psymbol->WorldDependency[world->Name.Content].GetValue();
						List<String> loopRange;
						for (auto w : dependentWorlds)
							loopRange.Add(w);
						for (auto w : loopRange)
						{
							EnumerableHashSet<String> & ddw = psymbol->WorldDependency[w].GetValue();
							for (auto ww : ddw)
							{
								if (!dependentWorlds.Contains(ww))
								{
									dependentWorlds.Add(ww);
									changed = true;
								}
							}
						}
					}
				}
				// fill in reachable worlds
				for (auto world : psymbol->Worlds)
				{
					if (auto depWorlds = psymbol->WorldDependency.TryGetValue(world.Key))
					{
						for (auto & dep : *depWorlds)
						{
							psymbol->ReachableWorlds[dep].GetValue().Add(world.Key);
						}
					}
				}

				for (auto & op : pipeline->ImportOperators)
				{
					for (auto & dep : op->Usings)
					{
						RefPtr<ShaderComponentSymbol> refComp;
						if (psymbol->Components.TryGetValue(dep.Content, refComp))
						{
							bool invalid = true;
							for (auto & depImpl : refComp->Implementations)
							{
								for (auto & w : depImpl->Worlds)
								{
									if (psymbol->IsWorldReachable(w, op->DestWorld.Content))
									{
										invalid = false;
									}
								}
							}
							if (invalid)
							{
								Error(30039, L"import operator '" + op->Name.Content + L"': none of the definitions for '" + dep.Content + L"' is available to destination world '" + op->DestWorld.Content + L"'.", dep);
							}
						}
						else
							Error(30034, L"import operator reference '" + dep.Content + L"' is not a defined shader component.", dep);

					}
				}
			}

			virtual void VisitImport(ImportSyntaxNode * import) override
			{
				RefPtr<ShaderSymbol> refShader;
				symbolTable->Shaders.TryGetValue(import->ShaderName.Content, refShader);
				if (refShader)
				{
					// type check
					List<ShaderComponentSymbol*> paramList;
					for (auto & comp : refShader->Components)
						if (comp.Value->IsParam())
							paramList.Add(comp.Value.Ptr());
					int position = 0;
					bool namedArgumentAppeared = false;
					for (auto & arg : import->Arguments)
					{
						if (arg->ArgumentName.Content.Length())
							namedArgumentAppeared = true;
						else
						{
							if (namedArgumentAppeared)
							{
								Error(33030, L"positional argument cannot appear after a named argument.", arg->Expression.Ptr());
								break;
							}
							if (position >= paramList.Count())
							{
								Error(33031, L"too many arguments.", arg->Expression.Ptr());
								break;
							}
							arg->ArgumentName.Content = paramList[position]->Name;
							arg->ArgumentName.Position = arg->Position;
						}
						position++;
						arg->Accept(this);
						RefPtr<ShaderComponentSymbol> refComp;
						if (refShader->Components.TryGetValue(arg->ArgumentName.Content, refComp))
						{
							if (refComp->Type->DataType != arg->Expression->Type)
							{
								Error(33027, L"argument type (" + arg->Expression->Type.ToString() + L") does not match parameter type (" + refComp->Type->DataType.ToString() + L")", arg->Expression.Ptr());
							}
							if (!refComp->IsParam())
								Error(33028, L"'" + arg->ArgumentName.Content + L"' is not a parameter of '" + import->ShaderName.Content + L"'.", arg->ArgumentName);
						}
						else
							Error(33028, L"'" + arg->ArgumentName.Content + L"' is not a parameter of '" + import->ShaderName.Content + L"'.", arg->ArgumentName);
					}
				}
			}

			class ShaderImportVisitor : public SyntaxVisitor
			{
			private:
				SymbolTable * symbolTable = nullptr;
				ShaderSymbol * currentShader = nullptr;
				ShaderComponentSymbol * currentComp = nullptr;
			public:
				ShaderImportVisitor(ErrorWriter * writer, SymbolTable * symTable)
					: SyntaxVisitor(writer), symbolTable(symTable)
				{}
				virtual void VisitShader(ShaderSyntaxNode * shader) override
				{
					currentShader = symbolTable->Shaders[shader->Name.Content].GetValue().Ptr();
					SyntaxVisitor::VisitShader(shader);
					currentShader = nullptr;
				}
				virtual void VisitComponent(ComponentSyntaxNode * comp) override
				{
					RefPtr<ShaderComponentSymbol> compSym;
					currentShader->Components.TryGetValue(comp->Name.Content, compSym);
					currentComp = compSym.Ptr();
					SyntaxVisitor::VisitComponent(comp);
					if (compSym->Type->DataType.Struct != nullptr || compSym->Type->DataType.IsArray ||
						compSym->Type->DataType.IsTextureType())
					{
						bool valid = true;
						bool isInStorageBuffer = true;
						if (comp->Rate)
						{
							for (auto & w : comp->Rate->Worlds)
							{
								auto world = currentShader->Pipeline->Worlds.TryGetValue(w.World.Content);
								if (world)
								{
									if (!world->IsAbstract)
										valid = false;
									isInStorageBuffer = isInStorageBuffer && world->SyntaxNode->LayoutAttributes.ContainsKey(L"ShaderStorageBlock");
								}
							}
						}
						else 
							valid = false;
						if (!valid)
						{
							Error(33035, L"\'" + compSym->Name + L"\': sampler, struct and array types only allowed in input worlds.", comp->Name);
						}
						else
						{
							if (!isInStorageBuffer && compSym->Type->DataType.Struct != nullptr)
							{
								Error(33036, L"\'" + compSym->Name + L"\': struct must only be defined in a input world with [ShaderStorageBlock] attribute.", comp->Name);
							}
						}
					}
					currentComp = nullptr;
				}
				virtual void VisitImport(ImportSyntaxNode * import) override
				{
					RefPtr<ShaderSymbol> refShader;
					symbolTable->Shaders.TryGetValue(import->ShaderName.Content, refShader);
					if (!refShader)
						Error(33015, L"undefined identifier \'" + import->ShaderName.Content + L"\'.", import->ShaderName);
					currentShader->DependentShaders.Add(refShader.Ptr());
					if (!currentComp)
					{
						ShaderUsing su;
						su.Shader = refShader.Ptr();
						su.IsPublic = import->IsPublic;
						if (import->IsInplace)
						{
							currentShader->ShaderUsings.Add(su);
						}
						else
						{
							if (currentShader->ShaderObjects.ContainsKey(import->ObjectName.Content) ||
								currentShader->Components.ContainsKey(import->ObjectName.Content))
							{
								Error(33018, L"\'" + import->ShaderName.Content + L"\' is already defined.", import->ShaderName);
							}
							currentShader->ShaderObjects[import->ObjectName.Content] = su;
						}
					}
					if (currentComp)
						Error(33016, L"'using': importing not allowed in component definition.", import->ShaderName);
				}
			};

			// pass 1: fill components in shader symbol table
			void VisitShaderPass1(ShaderSyntaxNode * shader)
			{
				HashSet<String> inheritanceSet;
				auto curShader = shader;
				inheritanceSet.Add(curShader->Name.Content);
				auto & shaderSymbol = symbolTable->Shaders[curShader->Name.Content].GetValue();
				this->currentShader = shaderSymbol.Ptr();
				if (shader->Pipeline.Content.Length() == 0) // implicit pipeline
				{
					if (program->Pipelines.Count() == 1)
					{
						shader->Pipeline = shader->Name; // get line and col from shader name
						shader->Pipeline.Content = program->Pipelines.First()->Name.Content;
					}
					else
					{
						// current compilation context has more than one pipeline defined,
						// in which case we do not allow implicit pipeline specification
						Error(33002, L"explicit pipeline specification required for shader '" +
							shader->Name.Content + L"' because multiple pipelines are defined in current context.", curShader->Name);
					}
				}
				
				auto pipelineName = shader->Pipeline.Content;
				auto pipeline = symbolTable->Pipelines.TryGetValue(pipelineName);
				if (pipeline)
					shaderSymbol->Pipeline = pipeline->Ptr();
				else
				{
					Error(33010, L"pipeline \'" + pipelineName + L"' is not defined.", shader->Pipeline);
					throw 0;
				}
				if (shader->IsModule)
					shaderSymbol->IsAbstract = true;
				// add components to symbol table
				for (auto & mbr : shader->Members)
				{
					if (auto comp = dynamic_cast<ComponentSyntaxNode*>(mbr.Ptr()))
					{
						if (comp->IsParam)
						{
							shaderSymbol->IsAbstract = true;
							if (!shaderSymbol->SyntaxNode->IsModule)
							{
								Error(33009, L"parameters can only be defined in modules.", shaderSymbol->SyntaxNode);
							}
						}
						AddNewComponentSymbol(shaderSymbol->Components, mbr);
					}
				}
				// add shader objects to symbol table
				ShaderImportVisitor importVisitor(err, symbolTable);
				shader->Accept(&importVisitor);

				for (auto & comp : shaderSymbol->Components)
				{
					for (auto & impl : comp.Value->Implementations)
					{
						bool inAbstractWorld = false;
						if (impl->SyntaxNode->Rate)
						{
							auto & userSpecifiedWorlds = impl->SyntaxNode->Rate->Worlds;
							for (auto & world : userSpecifiedWorlds)
							{
								if (!shaderSymbol->Pipeline->WorldDependency.ContainsKey(world.World.Content))
									Error(33012, L"\'" + world.World.Content + L"' is not a defined world in '" +
										pipelineName + L"'.", world.World);
								WorldSymbol worldSym;

								if (shaderSymbol->Pipeline->Worlds.TryGetValue(world.World.Content, worldSym))
								{
									if (worldSym.IsAbstract)
									{
										inAbstractWorld = true;
										if (userSpecifiedWorlds.Count() > 1)
										{
											Error(33013, L"abstract world cannot appear with other worlds.",
												world.World);
										}
									}
								}
							}
						}
						if (!inAbstractWorld && !impl->SyntaxNode->IsParam
							&& !impl->SyntaxNode->Expression && !impl->SyntaxNode->BlockStatement)
						{
							Error(33014, L"non-abstract component must have an implementation.",
								impl->SyntaxNode.Ptr());
						}
					}
				}
				this->currentShader = nullptr;
			}
			// pass 2: type checking component definitions
			void VisitShaderPass2(ShaderSyntaxNode * shaderNode)
			{
				RefPtr<ShaderSymbol> shaderSym;
				if (!symbolTable->Shaders.TryGetValue(shaderNode->Name.Content, shaderSym))
					return;
				this->currentShader = shaderSym.Ptr();
				for (auto & comp : shaderNode->Members)
				{
					comp->Accept(this);
				}
				this->currentShader = nullptr;
			}

			bool MatchType_ValueReceiver(ExpressionType receiverType, ExpressionType valueType)
			{
				if (receiverType == valueType)
					return true;
				if (receiverType.IsIntegral() && valueType == ExpressionType::Int)
					return true;
				if (receiverType == ExpressionType::Float && valueType.IsIntegral())
					return true;
				if (receiverType.IsVectorType() && valueType.IsVectorType())
				{
					if (GetVectorBaseType(receiverType.BaseType) == BaseType::Float &&
						GetVectorSize(receiverType.BaseType) == GetVectorSize(valueType.BaseType))
						return true;
					if (GetVectorBaseType(receiverType.BaseType) == BaseType::UInt &&
						GetVectorBaseType(valueType.BaseType) == BaseType::Int &&
						GetVectorSize(receiverType.BaseType) == GetVectorSize(valueType.BaseType))
						return true;
				}
				return false;
			}
			virtual void VisitComponent(ComponentSyntaxNode * comp) override
			{
				this->currentCompNode = comp;
				RefPtr<ShaderComponentSymbol> compSym;
				currentShader->Components.TryGetValue(comp->Name.Content, compSym);
				this->currentComp = compSym.Ptr();
				if (comp->Expression)
				{
					comp->Expression->Accept(this);
					if (!MatchType_ValueReceiver(compSym->Type->DataType, comp->Expression->Type) && comp->Expression->Type != ExpressionType::Error)
						Error(30019, L"type mismatch \'" + comp->Expression->Type.ToString() + L"\' and \'" +
							currentComp->Type->DataType.ToString() + L"\'", comp->Name);
				}
				if (comp->BlockStatement)
					comp->BlockStatement->Accept(this);
				this->currentComp = nullptr;
				this->currentCompNode = nullptr;
			}
			virtual void VisitImportStatement(ImportStatementSyntaxNode * importStmt) override
			{
				importStmt->Import->Accept(this);
			}
			void AddNewComponentSymbol(EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>> & components, RefPtr<ComponentSyntaxNode> comp)
			{
				RefPtr<ShaderComponentSymbol> compSym;
				RefPtr<ShaderComponentImplSymbol> compImpl = new ShaderComponentImplSymbol();
				if (comp->Rate)
					for (auto w : comp->Rate->Worlds)
						compImpl->Worlds.Add(w.World.Content);
				compImpl->SyntaxNode = comp;
				if (compImpl->SyntaxNode->Rate)
				{
					for (auto & w : compImpl->SyntaxNode->Rate->Worlds)
						if (w.Pinned)
							compImpl->SrcPinnedWorlds.Add(w.World.Content);
				}
				if (compImpl->SyntaxNode->AlternateName.Type == TokenType::Identifier)
				{
					compImpl->AlternateName = compImpl->SyntaxNode->AlternateName.Content;
				}
				if (compImpl->SyntaxNode->IsOutput)
				{
					if (compImpl->SyntaxNode->Rate)
					{
						for (auto & w : compImpl->SyntaxNode->Rate->Worlds)
							compImpl->ExportWorlds.Add(w.World.Content);
					}
					else
					{
						Error(33019, L"component \'" + compImpl->SyntaxNode->Name.Content + L"\': definition marked as 'export' must have an explicitly specified world.",
							compImpl->SyntaxNode.Ptr());
					}
				}
				if (!components.TryGetValue(comp->Name.Content, compSym))
				{
					compSym = new ShaderComponentSymbol();
					compSym->Type = new Type();
					compSym->Name = comp->Name.Content;
					compSym->Type->DataType = comp->Type->ToExpressionType(symbolTable, err);
					components.Add(comp->Name.Content, compSym);
				}
				else
				{
					if (comp->IsParam)
						Error(33029, L"\'" + compImpl->SyntaxNode->Name.Content + L"\': requirement clash with previous definition.",
							compImpl->SyntaxNode.Ptr());
					symbolTable->CheckComponentImplementationConsistency(err, compSym.Ptr(), compImpl.Ptr());
				}
				compSym->Implementations.Add(compImpl);
			}
			virtual void VisitProgram(ProgramSyntaxNode * programNode) override
			{
				HashSet<String> funcNames;
				this->program = programNode;
				this->function = nullptr;
				for (auto & s : program->Structs)
				{
					RefPtr<StructSymbol> ssym = new StructSymbol();
					ssym->Name = s->Name.Content;
					ssym->SyntaxNode = s;
					ssym->Type = new ILStructType();
					symbolTable->Structs.Add(s->Name.Content, ssym);
				}
				for (auto & s : program->Structs)
					VisitStruct(s.Ptr());
				for (auto & pipeline : program->Pipelines)
				{
					VisitPipeline(pipeline.Ptr());
				}
				for (auto & func : program->Functions)
				{
					VisitFunctionDeclaration(func.Ptr());
					if (funcNames.Contains(func->InternalName))
					{
						StringBuilder argList;
						argList << L"(";
						for (auto & param : func->Parameters)
						{
							argList << param->Type->ToExpressionType(symbolTable, err).ToString();
							if (param != func->Parameters.Last())
								argList << L", ";
						}
						argList << L")";
						Error(30001, L"function \'" + func->Name + argList.ProduceString() + L"\' redefinition.", func.Ptr());
					}
					else
						funcNames.Add(func->InternalName);
				}
				for (auto & func : program->Functions)
				{
					func->Accept(this);
				}
				// build initial symbol table for shaders
				for (auto & shader : program->Shaders)
				{
					RefPtr<ShaderSymbol> shaderSym = new ShaderSymbol();
					shaderSym->SyntaxNode = shader.Ptr();
					if (symbolTable->Shaders.ContainsKey(shader->Name.Content))
					{
						Error(33018, L"shader '" + shader->Name.Content + "' has already been defined.", shader->Name);
					}
					symbolTable->Shaders[shader->Name.Content] = shaderSym;
				}
				HashSet<ShaderSyntaxNode*> validShaders;
				for (auto & shader : program->Shaders)
				{
					int lastErrorCount = err->GetErrorCount();
					VisitShaderPass1(shader.Ptr());
					if (err->GetErrorCount() == lastErrorCount)
						validShaders.Add(shader.Ptr());
				}
				if (err->GetErrorCount() != 0)
					return;
				// shader dependency is discovered in pass 1, we can now sort the shaders
				if (!symbolTable->SortShaders())
				{
					HashSet<ShaderSymbol*> sortedShaders;
					for (auto & shader : symbolTable->ShaderDependenceOrder)
						sortedShaders.Add(shader);
					for (auto & shader : symbolTable->Shaders)
						if (!sortedShaders.Contains(shader.Value.Ptr()))
						{
							Error(33011, L"shader '" + shader.Key + L"' involves circular reference.", shader.Value->SyntaxNode->Name);
						}
				}

				for (auto & shader : symbolTable->ShaderDependenceOrder)
				{
					if (!validShaders.Contains(shader->SyntaxNode))
						continue;
					int lastErrorCount = err->GetErrorCount();
					VisitShaderPass2(shader->SyntaxNode);
					if (err->GetErrorCount() != lastErrorCount)
						validShaders.Remove(shader->SyntaxNode);
				}
				// update symbol table with only valid shaders
				EnumerableDictionary<String, RefPtr<ShaderSymbol>> newShaderSymbols;
				for (auto & shader : symbolTable->Shaders)
				{
					if (validShaders.Contains(shader.Value->SyntaxNode))
						newShaderSymbols.AddIfNotExists(shader.Key, shader.Value);
				}
				symbolTable->Shaders = _Move(newShaderSymbols);
			}

			virtual void VisitStruct(StructSyntaxNode * structNode) override
			{
				RefPtr<StructSymbol> st;
				if (symbolTable->Structs.TryGetValue(structNode->Name.Content, st))
				{
					st->Type->TypeName = structNode->Name.Content;
					for (auto node : structNode->Fields)
					{
						ILStructType::ILStructField f;
						f.FieldName = node->Name.Content;
						f.Type = TranslateExpressionType(node->Type->ToExpressionType(symbolTable, err));
						st->Type->Members.Add(f);
					}
				}
			}

			virtual void VisitFunction(FunctionSyntaxNode *functionNode) override
			{
				if (!functionNode->IsExtern)
				{
					currentFunc = symbolTable->Functions.TryGetValue(functionNode->InternalName)->Ptr();
					this->function = functionNode;
					functionNode->Body->Accept(this);
					this->function = NULL;
					currentFunc = nullptr;
				}
				
			}
			void VisitFunctionDeclaration(FunctionSyntaxNode *functionNode)
			{
				this->function = functionNode;
				auto returnType = functionNode->ReturnType->ToExpressionType(symbolTable, err);
				if(returnType.BaseType == BaseType::Void && returnType.IsArray)
					Error(30024, L"function return type can not be 'void' array.", functionNode->ReturnType.Ptr());
				StringBuilder internalName;
				internalName << functionNode->Name;
				HashSet<String> paraNames;
				for (auto & para : functionNode->Parameters)
				{
					if (paraNames.Contains(para->Name))
						Error(30002, L"parameter \'" + para->Name + L"\' already defined.", para.Ptr());
					else
						paraNames.Add(para->Name);
					VariableEntry varEntry;
					varEntry.Name = para->Name;
					varEntry.Type.DataType = para->Type->ToExpressionType(symbolTable, err);
					functionNode->Scope->Variables.AddIfNotExists(varEntry.Name, varEntry);
					if (varEntry.Type.DataType.BaseType == BaseType::Void)
						Error(30016, L"'void' can not be parameter type.", para.Ptr());
					internalName << L"@" << varEntry.Type.DataType.ToString();
				}
				functionNode->InternalName = internalName.ProduceString();	
				RefPtr<FunctionSymbol> symbol = new FunctionSymbol();
				symbol->SyntaxNode = functionNode;
				symbolTable->Functions[functionNode->InternalName] = symbol;
				this->function = NULL;
			}
			
			virtual void VisitBlockStatement(BlockStatementSyntaxNode *stmt) override
			{
				for (auto & node : stmt->Statements)
				{
					node->Accept(this);
				}
			}
			virtual void VisitBreakStatement(BreakStatementSyntaxNode *stmt) override
			{
				if (!loops.Count())
					Error(30003, L"'break' must appear inside loop constructs.", stmt);
			}
			virtual void VisitContinueStatement(ContinueStatementSyntaxNode *stmt) override
			{
				if (!loops.Count())
					Error(30004, L"'continue' must appear inside loop constructs.", stmt);
			}
			virtual void VisitDoWhileStatement(DoWhileStatementSyntaxNode *stmt) override
			{
				loops.Add(stmt);
				if (stmt->Predicate != NULL)
					stmt->Predicate->Accept(this);
				if (stmt->Predicate->Type != ExpressionType::Error && stmt->Predicate->Type != ExpressionType::Int && stmt->Predicate->Type != ExpressionType::Bool)
					Error(30005, L"'while': expression must evaluate to int.", stmt);
				stmt->Statement->Accept(this);

				loops.RemoveAt(loops.Count() - 1);
			}
			virtual void VisitEmptyStatement(EmptyStatementSyntaxNode *) override {}
			virtual void VisitForStatement(ForStatementSyntaxNode *stmt) override
			{
				loops.Add(stmt);
				VariableEntry iterVar;
				if (stmt->TypeDef != nullptr)
				{
					VariableEntry varEntry;
					varEntry.IsComponent = false;
					varEntry.Name = stmt->IterationVariable.Content;
					varEntry.Type.DataType = stmt->TypeDef->ToExpressionType(symbolTable, err);
					stmt->Scope->Variables.AddIfNotExists(stmt->IterationVariable.Content, varEntry);
				}
				if (!stmt->Scope->FindVariable(stmt->IterationVariable.Content, iterVar))
					Error(30015, L"undefined identifier \'" + stmt->IterationVariable.Content + L"\'", stmt->IterationVariable);
				else
				{
					if (iterVar.Type.DataType != ExpressionType::Float && iterVar.Type.DataType != ExpressionType::Int)
						Error(30035, L"iteration variable \'" + stmt->IterationVariable.Content + L"\' can only be a int or float", stmt->IterationVariable);
					stmt->InitialExpression->Accept(this);
					if (stmt->InitialExpression->Type != iterVar.Type.DataType)
						Error(30019, L"type mismatch \'" + stmt->InitialExpression->Type.ToString() + L"\' and \'" +
							iterVar.Type.DataType.ToString() + L"\'", stmt->InitialExpression.Ptr());
					stmt->EndExpression->Accept(this);
					if (stmt->EndExpression->Type != iterVar.Type.DataType)
						Error(30019, L"type mismatch \'" + stmt->EndExpression->Type.ToString() + L"\' and \'" +
							iterVar.Type.DataType.ToString() + L"\'", stmt->EndExpression.Ptr());
					if (stmt->StepExpression != nullptr)
					{
						stmt->StepExpression->Accept(this);
						if (stmt->StepExpression->Type != iterVar.Type.DataType)
							Error(30019, L"type mismatch \'" + stmt->StepExpression->Type.ToString() + L"\' and \'" +
								iterVar.Type.DataType.ToString() + L"\'", stmt->StepExpression.Ptr());
					}
				}

				stmt->Statement->Accept(this);

				loops.RemoveAt(loops.Count() - 1);
			}
			virtual void VisitIfStatement(IfStatementSyntaxNode *stmt) override
			{
				if (stmt->Predicate != NULL)
					stmt->Predicate->Accept(this);
				if (stmt->Predicate->Type != ExpressionType::Error && (stmt->Predicate->Type != ExpressionType::Int && stmt->Predicate->Type != ExpressionType::Bool))
					Error(30006, L"'if': expression must evaluate to int.", stmt);

				if (stmt->PositiveStatement != NULL)
					stmt->PositiveStatement->Accept(this);
				
				if (stmt->NegativeStatement != NULL)
					stmt->NegativeStatement->Accept(this);
			}
			virtual void VisitReturnStatement(ReturnStatementSyntaxNode *stmt) override
			{
				if (currentCompNode && currentCompNode->BlockStatement->Statements.Count() &&
					stmt != currentCompNode->BlockStatement->Statements.Last().Ptr())
				{
					Error(30026, L"'return' can only appear as the last statement in component definition.", stmt);
				}
				if (!stmt->Expression)
				{
					if (function && function->ReturnType->ToExpressionType(symbolTable, err) != ExpressionType::Void)
						Error(30006, L"'return' should have an expression.", stmt);
				}
				else
				{
					stmt->Expression->Accept(this);
					if (stmt->Expression->Type != ExpressionType::Error)
					{
						if (function && !MatchType_ValueReceiver(function->ReturnType->ToExpressionType(symbolTable, err), stmt->Expression->Type))
							Error(30007, L"expression type '" + stmt->Expression->Type.ToString()
								+ L"' does not match function's return type '"
								+ function->ReturnType->ToExpressionType(symbolTable, err).ToString() + L"'", stmt);
						if (currentComp && !MatchType_ValueReceiver(currentComp->Type->DataType, stmt->Expression->Type))
						{
							Error(30007, L"expression type '" + stmt->Expression->Type.ToString()
								+ L"' does not match component's type '"
								+ currentComp->Type->DataType.ToString() + L"'", stmt);
						}
					}
				}
			}
			virtual void VisitVarDeclrStatement(VarDeclrStatementSyntaxNode *stmt) override
			{
				if (stmt->Type->ToExpressionType(symbolTable, err).IsTextureType())
				{
					Error(30033, L"cannot declare a local variable of 'texture' type.", stmt);
				}
				for (auto & para : stmt->Variables)
				{
					VariableEntry varDeclr;
					varDeclr.Name = para->Name;
					if (stmt->Scope->Variables.ContainsKey(para->Name))
						Error(30008, L"variable " + para->Name + L" already defined.", para.Ptr());

					varDeclr.Type.DataType = stmt->Type->ToExpressionType(symbolTable, err);
					if (varDeclr.Type.DataType.BaseType == BaseType::Void)
						Error(30009, L"invalid type 'void'.", stmt);
					if (varDeclr.Type.DataType.IsArray && varDeclr.Type.DataType.ArrayLength <= 0)
						Error(30025, L"array size must be larger than zero.", stmt);

					stmt->Scope->Variables.AddIfNotExists(para->Name, varDeclr);
					if (para->Expression != NULL)
					{
						para->Expression->Accept(this);
						if (!MatchType_ValueReceiver(varDeclr.Type.DataType, para->Expression->Type)
							&& para->Expression->Type != ExpressionType::Error)
						{
							Error(30019, L"type mismatch \'" + para->Expression->Type.ToString() + L"\' and \'" +
								varDeclr.Type.DataType.ToString() + L"\'", para.Ptr());
						}
					}
				}
			}
			virtual void VisitWhileStatement(WhileStatementSyntaxNode *stmt) override
			{
				loops.Add(stmt);
				stmt->Predicate->Accept(this);
				if (stmt->Predicate->Type != ExpressionType::Error && stmt->Predicate->Type != ExpressionType::Int && stmt->Predicate->Type != ExpressionType::Bool)
					Error(30010, L"'while': expression must evaluate to int.", stmt);

				stmt->Statement->Accept(this);
				loops.RemoveAt(loops.Count() - 1);
			}
			virtual void VisitExpressionStatement(ExpressionStatementSyntaxNode *stmt) override
			{
				stmt->Expression->Accept(this);
			}
			bool MatchType_BinaryImplicit(ExpressionType & resultType, ExpressionType leftType, ExpressionType rightType)
			{
				if (leftType == rightType && !leftType.IsTextureType())
				{
					resultType = leftType;
					return true;
				}
				else if (leftType.IsVectorType() && rightType == GetVectorBaseType(leftType.BaseType))
				{
					resultType = leftType;
					return true;
				}
				else if (rightType.IsVectorType() && leftType == GetVectorBaseType(rightType.BaseType))
				{
					resultType = rightType;
					return true;
				}
				else if ((rightType == ExpressionType::Float && leftType == ExpressionType::Int) ||
					(leftType == ExpressionType::Float && rightType == ExpressionType::Int))
				{
					resultType = ExpressionType::Float;
					return true;
				}
				else if ((rightType == ExpressionType::Float2 && leftType == ExpressionType::Int2) ||
					(leftType == ExpressionType::Float2 && rightType == ExpressionType::Int2))
				{
					resultType = ExpressionType::Float2;
					return true;
				}
				else if ((rightType == ExpressionType::Float3 && leftType == ExpressionType::Int3) ||
					(leftType == ExpressionType::Float3 && rightType == ExpressionType::Int3))
				{
					resultType = ExpressionType::Float3;
					return true;
				}
				else if ((rightType == ExpressionType::Float4 && leftType == ExpressionType::Int4) ||
					(leftType == ExpressionType::Float4 && rightType == ExpressionType::Int4))
				{
					resultType = ExpressionType::Float4;
					return true;
				}
				else if ((rightType == ExpressionType::Float && leftType == ExpressionType::UInt) ||
					(leftType == ExpressionType::Float && rightType == ExpressionType::UInt))
				{
					resultType = ExpressionType::Float;
					return true;
				}
				else if ((rightType == ExpressionType::Float2 && leftType == ExpressionType::UInt2) ||
					(leftType == ExpressionType::Float2 && rightType == ExpressionType::UInt2))
				{
					resultType = ExpressionType::Float2;
					return true;
				}
				else if ((rightType == ExpressionType::Float3 && leftType == ExpressionType::UInt3) ||
					(leftType == ExpressionType::Float3 && rightType == ExpressionType::UInt3))
				{
					resultType = ExpressionType::Float3;
					return true;
				}
				else if ((rightType == ExpressionType::Float4 && leftType == ExpressionType::UInt4) ||
					(leftType == ExpressionType::Float4 && rightType == ExpressionType::UInt4))
				{
					resultType = ExpressionType::Float4;
					return true;
				}
				else if ((rightType == ExpressionType::Int && leftType == ExpressionType::UInt) ||
					(leftType == ExpressionType::Int && rightType == ExpressionType::UInt))
				{
					resultType = ExpressionType::Int;
					return true;
				}
				else if ((rightType == ExpressionType::Int2 && leftType == ExpressionType::UInt2) ||
					(leftType == ExpressionType::Int2 && rightType == ExpressionType::UInt2))
				{
					resultType = ExpressionType::Int2;
					return true;
				}
				else if ((rightType == ExpressionType::Int3 && leftType == ExpressionType::UInt3) ||
					(leftType == ExpressionType::Int3 && rightType == ExpressionType::UInt3))
				{
					resultType = ExpressionType::Int3;
					return true;
				}
				else if ((rightType == ExpressionType::Int4 && leftType == ExpressionType::UInt4) ||
					(leftType == ExpressionType::Int4 && rightType == ExpressionType::UInt4))
				{
					resultType = ExpressionType::Int4;
					return true;
				}
				return false;
			}
			virtual void VisitBinaryExpression(BinaryExpressionSyntaxNode *expr) override
			{
				expr->LeftExpression->Accept(this);
				expr->RightExpression->Accept(this);
				auto & leftType = expr->LeftExpression->Type;
				auto & rightType = expr->RightExpression->Type;
				ExpressionType matchedType;
				switch (expr->Operator)
				{
				case Operator::Add:
				case Operator::Sub:
				case Operator::Div:
					if (MatchType_BinaryImplicit(matchedType, leftType, rightType))
						expr->Type = matchedType;
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Mul:
					if (!leftType.IsArray && leftType.BaseType != BaseType::Shader)
					{
						if (MatchType_BinaryImplicit(matchedType, leftType, rightType))
							expr->Type = matchedType;
						else if ((leftType.BaseType == BaseType::Float3x3 && rightType == ExpressionType::Float3) ||
							(leftType.BaseType == BaseType::Float3 && rightType.BaseType == BaseType::Float3x3))
							expr->Type = ExpressionType::Float3;
						else if ((leftType.BaseType == BaseType::Float4x4 && rightType == ExpressionType::Float4) ||
							(leftType.BaseType == BaseType::Float4 && rightType.BaseType == BaseType::Float4x4))
							expr->Type = ExpressionType::Float4;
						else
							expr->Type = ExpressionType::Error;
					}
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Mod:
				case Operator::Rsh:
				case Operator::Lsh:
				case Operator::BitAnd:
				case Operator::BitOr:
				case Operator::BitXor:
				case Operator::And:
				case Operator::Or:
					if (leftType == rightType && !leftType.IsArray && !IsTextureType(GetVectorBaseType(leftType.BaseType))
						&& leftType.BaseType != BaseType::Shader &&
						GetVectorBaseType(leftType.BaseType) != BaseType::Float)
						expr->Type = (expr->Operator == Operator::And || expr->Operator == Operator::Or ? ExpressionType::Bool : leftType);
					else if (leftType.IsIntegral() && rightType.IsIntegral())
						expr->Type = leftType;
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Neq:
				case Operator::Eql:
					if (leftType == rightType && !leftType.IsArray && !leftType.IsTextureType() && leftType.BaseType != BaseType::Shader)
						expr->Type = ExpressionType::Bool;
					else if ((leftType == ExpressionType::Int || leftType == ExpressionType::UInt) &&
						(rightType == ExpressionType::Int || rightType == ExpressionType::UInt))
						expr->Type = ExpressionType::Bool;
					else if (leftType.IsIntegral() && rightType.IsIntegral())
						expr->Type = ExpressionType::Bool;
					else if (leftType == ExpressionType::Float && rightType.IsIntegral() ||
						leftType.IsIntegral() && rightType == ExpressionType::Float)
						expr->Type = ExpressionType::Bool;
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Greater:
				case Operator::Geq:
				case Operator::Less:
				case Operator::Leq:
					if ((leftType == ExpressionType::Int || leftType == ExpressionType::UInt) && 
						(rightType == ExpressionType::Int || rightType == ExpressionType::UInt))
						expr->Type = ExpressionType::Bool;
					else if (leftType == ExpressionType::Float && rightType == ExpressionType::Float)
						expr->Type = ExpressionType::Bool;
					else if (leftType == ExpressionType::Float && rightType.IsIntegral() ||
						leftType.IsIntegral() && rightType == ExpressionType::Float)
						expr->Type = ExpressionType::Bool;
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Assign:
				case Operator::AddAssign:
				case Operator::MulAssign:
				case Operator::DivAssign:
				case Operator::SubAssign:
				case Operator::ModAssign:
				case Operator::AndAssign:
				case Operator::OrAssign:
				case Operator::XorAssign:
				case Operator::LshAssign:
				case Operator::RshAssign:
					if (!leftType.IsLeftValue && leftType != ExpressionType::Error)
						Error(30011, L"left of '=' is not an l-value.", expr->LeftExpression.Ptr());
					if (expr->Operator == Operator::AndAssign ||
						expr->Operator == Operator::OrAssign ||
						expr->Operator == Operator::XorAssign ||
						expr->Operator == Operator::LshAssign ||
						expr->Operator == Operator::RshAssign)
					{
						if (!(leftType.IsIntegral() && rightType.IsIntegral()))
						{
							Error(30041, L"bit operation: operand must be integral type.", expr);
						}
					}
					expr->LeftExpression->Access = ExpressionAccess::Write;
					if (MatchType_ValueReceiver(leftType, rightType))
						expr->Type = ExpressionType::Void;
					else
						expr->Type = ExpressionType::Error;
					break;
				default:
						expr->Type = ExpressionType::Error;
					break;
				}
				
				if (expr->Type == ExpressionType::Error &&
					leftType != ExpressionType::Error && rightType != ExpressionType::Error)
					Error(30012, L"no overload found for operator " + OperatorToString(expr->Operator)  + L" (" + leftType.ToString() + L", " + rightType.ToString() + L").", expr);
			}
			virtual void VisitConstantExpression(ConstantExpressionSyntaxNode *expr) override
			{
				switch (expr->ConstType)
				{
				case ConstantExpressionSyntaxNode::ConstantType::Int:
					expr->Type = ExpressionType::Int;
					break;
				case ConstantExpressionSyntaxNode::ConstantType::Float:
					expr->Type = ExpressionType::Float;
					break;
				default:
					expr->Type = ExpressionType::Error;
					throw "Invalid constant type.";
					break;
				}
			}
			virtual void VisitIndexExpression(IndexExpressionSyntaxNode *expr) override
			{
				expr->BaseExpression->Accept(this);
				expr->IndexExpression->Accept(this);
				if (expr->BaseExpression->Type == ExpressionType::Error)
					expr->Type = ExpressionType::Error;
				else
				{
					if (expr->BaseExpression->Type.IsArray &&
						GetVectorSize(expr->BaseExpression->Type.BaseType) == 0)
					{
						Error(30013, L"'[]' can only index on arrays and strings.", expr);
						expr->Type = ExpressionType::Error;
					}
					if (expr->IndexExpression->Type != ExpressionType::Int && expr->IndexExpression->Type != ExpressionType::UInt)
					{
						Error(30014, L"index expression must evaluate to int.", expr);
						expr->Type = ExpressionType::Error;
					}
				}
				if (expr->BaseExpression->Type.IsArray)
				{
					expr->Type = expr->BaseExpression->Type;
					expr->Type.IsArray = false;
					expr->Type.ArrayLength = 0;
				}
				else
				{
					if (expr->BaseExpression->Type.BaseType == BaseType::Float3x3)
						expr->Type.BaseType = BaseType::Float3;
					else if (expr->BaseExpression->Type.BaseType == BaseType::Float4x4)
						expr->Type.BaseType = BaseType::Float4;
					else
						expr->Type.BaseType = GetVectorBaseType(expr->BaseExpression->Type.BaseType);
				}
				expr->Type.IsLeftValue = true;
				expr->Type.IsReference = true;
			}
			bool MatchArguments(FunctionSyntaxNode * functionNode, List <RefPtr<ExpressionSyntaxNode>> &args)
			{
				if (functionNode->Parameters.Count() != args.Count())
					return false;
				for (int i = 0; i < functionNode->Parameters.Count(); i++)
				{
					if (functionNode->Parameters[i]->Type->ToExpressionType(symbolTable, err) != args[i]->Type)
						return false;
				}
				return true;
			}
			virtual void VisitInvokeExpression(InvokeExpressionSyntaxNode *expr) override
			{
				StringBuilder internalName;
				StringBuilder argList;
				internalName << expr->FunctionExpr->Variable;
				argList << L"(";
				for (int i = 0; i < expr->Arguments.Count(); i++)
				{
					expr->Arguments[i]->Accept(this);
					internalName << L"@" << expr->Arguments[i]->Type.ToString();
					argList << expr->Arguments[i]->Type.ToString();
					if (i != expr->Arguments.Count() - 1)
					{
						argList << L", ";
					}
					if (expr->Arguments[i]->Type == ExpressionType::Error)
					{
						expr->Type = ExpressionType::Error;
						return;
					}
				}
				argList << L")";
				String funcName = internalName.ProduceString();
				RefPtr<FunctionSymbol> func;
				bool found = symbolTable->Functions.TryGetValue(funcName, func);
				if (!found)
				{
					// find function overload with explicit conversions from int -> float
					auto namePrefix = expr->FunctionExpr->Variable + L"@";
					for (auto & f : symbolTable->Functions)
					{
						if (f.Key.StartsWith(namePrefix))
						{
							if (f.Value->SyntaxNode->Parameters.Count() == expr->Arguments.Count())
							{
								bool match = true;
								for (int i = 0; i < expr->Arguments.Count(); i++)
								{
									auto argType = expr->Arguments[i]->Type;
									auto paramType = f.Value->SyntaxNode->Parameters[i]->Type->ToExpressionType(symbolTable, err);
									if (argType == paramType)
										continue;
									else if (MatchType_ValueReceiver(paramType, argType))
										continue;
									else
									{
										match = false;
										break;
									}
								}
								if (match)
								{
									func = f.Value;
									funcName = f.Key;
									found = true;
								}
							}
						}
					}
				}

				if (!found)
				{
					expr->Type = ExpressionType::Error;
					Error(30021, expr->FunctionExpr->Variable + L": no overload takes arguments " + argList.ProduceString(), expr);
				}
				else
				{
					if (!func->SyntaxNode->IsExtern)
					{
						expr->FunctionExpr->Variable = funcName;
						if (currentFunc)
							currentFunc->ReferencedFunctions.Add(funcName);
					}
					expr->Type = func->SyntaxNode->ReturnType->ToExpressionType(symbolTable, err);
				}
			}

			String OperatorToString(Operator op)
			{
				switch (op)
				{
				case Spire::Compiler::Operator::Neg:
					return L"-";
				case Spire::Compiler::Operator::Not:
					return L"!";
				case Spire::Compiler::Operator::PreInc:
					return L"++";
				case Spire::Compiler::Operator::PreDec:
					return L"--";
				case Spire::Compiler::Operator::PostInc:
					return L"++";
				case Spire::Compiler::Operator::PostDec:
					return L"--";
				case Spire::Compiler::Operator::Mul:
					return L"*";
				case Spire::Compiler::Operator::Div:
					return L"/";
				case Spire::Compiler::Operator::Mod:
					return L"%";
				case Spire::Compiler::Operator::Add:
					return L"+";
				case Spire::Compiler::Operator::Sub:
					return L"-";
				case Spire::Compiler::Operator::Lsh:
					return L"<<";
				case Spire::Compiler::Operator::Rsh:
					return L">>";
				case Spire::Compiler::Operator::Eql:
					return L"==";
				case Spire::Compiler::Operator::Neq:
					return L"!=";
				case Spire::Compiler::Operator::Greater:
					return L">";
				case Spire::Compiler::Operator::Less:
					return L"<";
				case Spire::Compiler::Operator::Geq:
					return L">=";
				case Spire::Compiler::Operator::Leq:
					return L"<=";
				case Spire::Compiler::Operator::BitAnd:
					return L"&";
				case Spire::Compiler::Operator::BitXor:
					return L"^";
				case Spire::Compiler::Operator::BitOr:
					return L"|";
				case Spire::Compiler::Operator::And:
					return L"&&";
				case Spire::Compiler::Operator::Or:
					return L"||";
				case Spire::Compiler::Operator::Assign:
					return L"=";
				default:
					return L"ERROR";
				}
			}
			virtual void VisitUnaryExpression(UnaryExpressionSyntaxNode *expr) override
			{
				expr->Expression->Accept(this);
				
				switch (expr->Operator)
				{
				case Operator::Neg:
					if (expr->Expression->Type == ExpressionType::Int ||
						expr->Expression->Type == ExpressionType::Bool ||
						expr->Expression->Type == ExpressionType::Float ||
						expr->Expression->Type.IsVectorType())
						expr->Type = expr->Expression->Type;
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Not:
				case Operator::BitNot:
					if (expr->Expression->Type == ExpressionType::Int || expr->Expression->Type == ExpressionType::Bool ||
						expr->Expression->Type == ExpressionType::Int2
						|| expr->Expression->Type == ExpressionType::Int3 || expr->Expression->Type == ExpressionType::Int4)
						expr->Type = (expr->Operator == Operator::Not ? ExpressionType::Bool : expr->Expression->Type);
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::PostDec:
				case Operator::PostInc:
				case Operator::PreDec:
				case Operator::PreInc:
					if (expr->Expression->Type == ExpressionType::Int)
						expr->Type = ExpressionType::Int;
					else
						expr->Type = ExpressionType::Error;
					break;
				default:
					expr->Type = ExpressionType::Error;
					break;
				}

				if(expr->Type == ExpressionType::Error && expr->Expression->Type != ExpressionType::Error)
					Error(30020, L"operator " + OperatorToString(expr->Operator) + L" can not be applied to " + expr->Expression->Type.ToString(), expr);
			}
			virtual void VisitVarExpression(VarExpressionSyntaxNode *expr) override
			{
				VariableEntry variable;
				ShaderUsing shaderObj;
				if (expr->Scope->FindVariable(expr->Variable, variable))
				{
					expr->Type = variable.Type.DataType;
					expr->Type.IsLeftValue = !variable.IsComponent;
				}
				else if (currentShader && currentShader->ShaderObjects.TryGetValue(expr->Variable, shaderObj))
				{
					expr->Type.BaseType = BaseType::Shader;
					expr->Type.Shader = shaderObj.Shader;
					expr->Type.IsLeftValue = false;
				}
				else
				{
					if (currentShader)
					{
						auto compRef = currentShader->ResolveComponentReference(expr->Variable);
						if (compRef.IsAccessible)
						{
							expr->Type = compRef.Component->Type->DataType;
							expr->Type.IsLeftValue = false;
						}
						else if (compRef.Component)
						{
							Error(30017, L"component \'" + expr->Variable + L"\' is not accessible from shader '" + currentShader->SyntaxNode->Name.Content + L"'.", expr);
						}
						else
							Error(30015, L"undefined identifier \'" + expr->Variable + L"\'", expr);
					}
					else
						Error(30015, L"undefined identifier \'" + expr->Variable + L"\'", expr);
				}
			}
			virtual void VisitTypeCastExpression(TypeCastExpressionSyntaxNode * expr) override
			{
				expr->Expression->Accept(this);
				auto targetType = expr->TargetType->ToExpressionType(symbolTable, err);
				
				if (expr->Expression->Type != ExpressionType::Error)
				{
					if (expr->Expression->Type.IsArray)
						expr->Type = ExpressionType::Error;
					else if (!IsNumeric(GetVectorBaseType(expr->Expression->Type.BaseType)) || !IsNumeric(GetVectorBaseType(targetType.BaseType)))
						expr->Type = ExpressionType::Error;
					else if (targetType.BaseType == BaseType::Void || expr->Expression->Type.BaseType == BaseType::Void)
						expr->Type = ExpressionType::Error;
					else
						expr->Type = targetType;
				}
				else
					expr->Type = ExpressionType::Error;
				if (expr->Type == ExpressionType::Error && expr->Expression->Type != ExpressionType::Error)
				{
					Error(30022, L"invalid type cast between \"" + expr->Expression->Type.ToString() + L"\" and \"" +
						targetType.ToString() + L"\".", expr);
				}
			}
			virtual void VisitSelectExpression(SelectExpressionSyntaxNode * expr) override
			{
				expr->SelectorExpr->Accept(this);
				if ((expr->SelectorExpr->Type != ExpressionType::Int && expr->SelectorExpr->Type != ExpressionType::Bool) && expr->SelectorExpr->Type != ExpressionType::Error)
				{
					expr->Type = ExpressionType::Error;
					Error(30079, L"selector must evaluate to int.", expr);
				}
				expr->Expr0->Accept(this);
				expr->Expr1->Accept(this);
				if (expr->Expr0->Type != expr->Expr1->Type)
				{
					Error(30080, L"the two value expressions in a select clause must evaluate to same type.", expr);
				}
				expr->Type = expr->Expr0->Type;
			}
			virtual void VisitMemberExpression(MemberExpressionSyntaxNode * expr) override
			{
				expr->BaseExpression->Accept(this);
				auto & baseType = expr->BaseExpression->Type;
				if (baseType.IsArray)
					expr->Type = ExpressionType::Error;
				else if (IsVector(baseType.BaseType))
				{
					Array<int, 4> children;
					if (expr->MemberName.Length() > 4)
						expr->Type = ExpressionType::Error;
					else
					{
						bool error = false;

						for (int i = 0; i < expr->MemberName.Length(); i++)
						{
							auto ch = expr->MemberName[i];
							switch (ch)
							{
							case L'x':
							case L'r':
								children.Add(0);
								break;
							case L'y':
							case L'g':
								children.Add(1);
								break;
							case L'z':
							case L'b':
								children.Add(2);
								break;
							case L'w':
							case L'a':
								children.Add(3);
								break;
							default:
								error = true;
								expr->Type = ExpressionType::Error;
								break;
							}
						}
						int vecLen = GetVectorSize(baseType.BaseType);
						for (auto m : children)
						{
							if (m >= vecLen)
							{
								error = true;
								expr->Type = ExpressionType::Error;
								break;
							}
						}
						if ((vecLen == 9 || vecLen == 16) && children.Count() > 1)
						{
							error = true;
							expr->Type = ExpressionType::Error;
						}
						if (!error)
						{
							expr->Type = baseType;
							if (vecLen == 9)
								expr->Type.BaseType = (BaseType)((int)GetVectorBaseType(baseType.BaseType) + 2);
							else if (vecLen == 16)
								expr->Type.BaseType = (BaseType)((int)GetVectorBaseType(baseType.BaseType) + 3);
							else
							{
								expr->Type.BaseType = (BaseType)((int)GetVectorBaseType(baseType.BaseType) + children.Count() - 1);
							}
						}
						expr->Type.IsLeftValue = true;
					}
				}
				else if (baseType.BaseType == BaseType::Shader)
				{
					ShaderUsing shaderObj;
					auto refComp = baseType.Shader->ResolveComponentReference(expr->MemberName);
					if (refComp.IsAccessible)
						expr->Type = refComp.Component->Type->DataType;
					else if (baseType.Shader->ShaderObjects.TryGetValue(expr->MemberName, shaderObj))
					{
						if (shaderObj.IsPublic)
						{
							expr->Type.BaseType = BaseType::Shader;
							expr->Type.Shader = shaderObj.Shader;
						}
						else
							expr->Type = ExpressionType::Error;
					}
					else
						expr->Type = ExpressionType::Error;
				}
				else if (baseType.BaseType == BaseType::Struct && baseType.Struct)
				{
					int id = baseType.Struct->SyntaxNode->FindField(expr->MemberName);
					if (id == -1)
					{
						expr->Type = ExpressionType::Error;
						Error(30027, L"\'" + expr->MemberName + L"\' is not a member of \'" +
							baseType.Struct->Name + L"\'.", expr);
					}
					else
						expr->Type = baseType.Struct->SyntaxNode->Fields[id]->Type->ToExpressionType(symbolTable, err);
				}
				else
					expr->Type = ExpressionType::Error;
				if (baseType != ExpressionType::Error &&
					expr->Type == ExpressionType::Error)
				{
					Error(30023, L"\"" + baseType.ToString() + L"\" does not have public member \"" +
						expr->MemberName + L"\".", expr);
				}
			}
			virtual void VisitParameter(ParameterSyntaxNode *) override {}
			virtual void VisitType(TypeSyntaxNode *) override {}
			virtual void VisitDeclrVariable(Variable *) override {}
			SemanticsVisitor & operator = (const SemanticsVisitor &) = delete;
		};

		SyntaxVisitor * CreateSemanticsVisitor(SymbolTable * symbols, ErrorWriter * err)
		{
			return new SemanticsVisitor(symbols, err);
		}
		
	}
}