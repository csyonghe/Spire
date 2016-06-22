#include "SyntaxVisitors.h"
#include "ScopeDictionary.h"
#include "CodeWriter.h"
#include "../CoreLib/Parser.h"
#include <assert.h>

namespace Spire
{
	namespace Compiler
	{
		class CodeGenerator : public ICodeGenerator
		{
		private:
			SymbolTable * symTable;
			CompiledWorld * currentWorld = nullptr;
			ShaderComponentSymbol * currentComponent = nullptr;
			ShaderComponentImplSymbol * currentComponentImpl = nullptr;
			ShaderClosure * currentShader = nullptr;
			CompileResult & result;
			List<ILOperand*> exprStack;
			CodeWriter codeWriter;
			ScopeDictionary<String, ILOperand*> variables;
			ILType * TranslateExpressionType(const ExpressionType & type)
			{
				ILType * resultType = 0;
				auto base = new ILBasicType();
				base->Type = (ILBaseType)type.BaseType;
				resultType = base;
				if (type.BaseType == BaseType::Bool)
				{
					base->Type = ILBaseType::Int;
				}

				if (type.IsArray)
				{
					ILArrayType * arrType = dynamic_cast<ILArrayType*>(resultType);
					if (resultType)
					{
						arrType->ArrayLength *= type.ArrayLength;
					}
					else
					{
						auto nArrType = new ILArrayType();
						nArrType->BaseType = resultType;
						nArrType->ArrayLength = type.ArrayLength;
						resultType = nArrType;
					}
				}
				return resultType;
			}
			void PushStack(ILOperand * op)
			{
				exprStack.Add(op);
			}
			ILOperand * PopStack()
			{
				auto rs = exprStack.Last();
				exprStack.SetSize(exprStack.Count() - 1);
				return rs;
			}
			AllocVarInstruction * AllocVar(const ExpressionType & etype)
			{
				AllocVarInstruction * varOp = 0;
				RefPtr<ILType> type = TranslateExpressionType(etype);
				auto arrType = dynamic_cast<ILArrayType*>(type.Ptr());

				if (arrType)
				{
					varOp = codeWriter.AllocVar(arrType->BaseType, result.Program->ConstantPool->CreateConstant(arrType->ArrayLength));
				}
				else
				{
					assert(type);
					varOp = codeWriter.AllocVar(type, result.Program->ConstantPool->CreateConstant(0));
				}
				return varOp;
			}
			FetchArgInstruction * FetchArg(const ExpressionType & etype, int argId)
			{
				auto type = TranslateExpressionType(etype);
				auto arrType = dynamic_cast<ILArrayType*>(type);
				FetchArgInstruction * varOp = 0;
				if (arrType)
				{
					auto baseType = arrType->BaseType.Release();
					varOp = codeWriter.FetchArg(baseType, argId);
					delete type;
				}
				else
				{
					varOp = codeWriter.FetchArg(type, argId);
				}
				return varOp;
			}
			void SortInterfaceBlock(InterfaceBlock * block)
			{
				List<KeyValuePair<String, ComponentDefinition>> entries;
				for (auto & kv : block->Entries)
					entries.Add(kv);
				entries.Sort([](const auto & v0, const auto & v1) {return v0.Value.OrderingStr < v1.Value.OrderingStr; });
				block->Entries.Clear();
				for (auto & kv : entries)
					block->Entries.Add(kv.Key, kv.Value);
			}
		public:
			virtual void VisitProgram(ProgramSyntaxNode *) override
			{
			}
			virtual void ProcessFunction(FunctionSyntaxNode * func) override
			{
				VisitFunction(func);
			}
			virtual void ProcessShader(ShaderClosure * shader) override
			{
				currentShader = shader;
				RefPtr<CompiledShader> compiledShader = new CompiledShader();
				compiledShader->MetaData.ShaderName = shader->Name;
				result.Program->Shaders.Add(compiledShader);
				for (auto & world : shader->Pipeline->Worlds)
				{
					auto w = new CompiledWorld();
					w->ExportOperator = world.Value.SyntaxNode->ExportOperator;
					auto outputBlock = new InterfaceBlock();
					outputBlock->Name = world.Key;
					world.Value.SyntaxNode->LayoutAttributes.TryGetValue(L"InterfaceBlock", outputBlock->Name);
					if (outputBlock->Name.Contains(L":"))
					{
						CoreLib::Text::Parser parser(outputBlock->Name);
						auto blockName = parser.ReadWord();
						parser.Read(L":");
						auto indx = parser.ReadInt();
						outputBlock->Name = blockName;
						outputBlock->Attributes[L"Index"] = String(indx);
					}
					String strIdx;
					if (world.Value.SyntaxNode->LayoutAttributes.TryGetValue(L"InterfaceBlockIndex", strIdx))
						outputBlock->Attributes[L"Index"] = strIdx;
					if (world.Value.SyntaxNode->LayoutAttributes.ContainsKey(L"Packed"))
						outputBlock->Attributes[L"Packed"] = L"1";
					w->WorldOutput = outputBlock;
					compiledShader->InterfaceBlocks[outputBlock->Name] = outputBlock;
					w->Attributes = world.Value.SyntaxNode->LayoutAttributes;
					w->Shader = compiledShader.Ptr();
					w->ShaderName = shader->Name;
					w->WorldName = world.Key;
					w->IsAbstract = world.Value.IsAbstract;
					auto impOps = shader->Pipeline->GetImportOperatorsFromSourceWorld(world.Key);

					w->TargetMachine = world.Value.SyntaxNode->TargetMachine;
					CoreLib::Text::Parser parser(w->TargetMachine);
					try
					{
						if (!parser.IsEnd())
						{
							w->TargetMachine = parser.ReadWord();
							if (parser.LookAhead(L"("))
							{
								parser.Read(L"(");
								while (!parser.LookAhead(L")"))
								{
									auto param = parser.ReadWord();
									if (parser.LookAhead(L":"))
									{
										parser.Read(L":");
										auto value = parser.ReadWord();
										w->BackendParameters[param] = value;
									}
									else
										w->BackendParameters[param] = L"";
									if (parser.LookAhead(L";"))
									{
										parser.Read(L";");
									}
									else
										break;
								}
								parser.Read(L")");
							}
						}
					}
					catch (const CoreLib::Text::TextFormatException & ex)
					{
						result.GetErrorWriter()->Error(34031, L"invalid target machine syntax. \n" + ex.Message, world.Value.SyntaxNode->Position);
					}
					w->WorldDefPosition = world.Value.SyntaxNode->Position;
					compiledShader->Worlds[world.Key] = w;
				}

				Dictionary<String, List<ComponentDefinitionIR*>> worldComps;
				struct ThroughVar
				{
					bool Export = false;
					String InputName, OutputName;
					String InputWorldName;
					ImportOperatorDefSyntaxNode * ImportOperator;
					ComponentDefinitionIR * Component;
					int GetHashCode()
					{
						return PointerHash<1>().GetHashCode(Component);
					}
					bool operator == (const ThroughVar & other)
					{
						return Component == other.Component;
					}
				};
				Dictionary<String, EnumerableHashSet<ThroughVar>> worldThroughVars;
				for (auto & world : shader->Pipeline->Worlds)
				{
					List<ComponentDefinitionIR*> components;
					for (auto & compDef : shader->IR->Definitions)
						if (compDef->World == world.Key)
							components.Add(compDef.Ptr());

					auto & compiledWorld = compiledShader->Worlds[world.Key].GetValue();
					DependencySort(components, [](ComponentDefinitionIR * def)
					{
						return def->Dependency;
					});
					worldComps[world.Key] = components;
					worldThroughVars[world.Key] = EnumerableHashSet<ThroughVar>();

					if (world.Value.SyntaxNode->LayoutAttributes.ContainsKey(L"Pinned"))
					{
						for (auto & comp : components)
						{
							ComponentDefinition compDef;
							compDef.LayoutAttribs = comp->Implementation->SyntaxNode->LayoutAttributes;
							compDef.Name = comp->Component->UniqueName;
							compDef.Type = TranslateExpressionType(comp->Component->Type->DataType);
							compDef.OrderingStr = comp->Implementation->SyntaxNode->Position.FileName +
								String(comp->Implementation->SyntaxNode->Position.Line).PadLeft(L' ', 8) + compDef.Name;
							compiledWorld->WorldOutput->Entries.AddIfNotExists(compDef.Name, compDef);
						}
					}
				}
				for (auto & world : shader->Pipeline->Worlds)
				{
					auto compiledWorld = compiledShader->Worlds[world.Key].GetValue();
					for (auto & impOp : shader->Pipeline->SyntaxNode->ImportOperators)
					{
						if (impOp->DestWorld.Content == world.Key)
						{
							InputInterface input;
							input.Block = compiledShader->Worlds[impOp->SourceWorld.Content].GetValue()->WorldOutput;
							input.ImportOperator = *impOp;
							compiledWorld->WorldInputs[impOp->SourceWorld.Content] = input;
						}
					}
					auto & components = worldComps[world.Key].GetValue();
					
					for (auto & comp : components)
					{
						auto srcWorld = world.Key;
						EnumerableHashSet<String> outputWorlds;
						if (comp->Implementation->ExportWorlds.Contains(srcWorld))
							outputWorlds.Add(srcWorld);
						struct ImportTechnique
						{
							String SourceWorld;
							ImportOperatorDefSyntaxNode * ImportOperator;
						};
						EnumerableDictionary<String, ImportTechnique> inputSourceWorlds;
						auto useInWorld = [&](String userWorld)
						{
							if (userWorld == srcWorld)
								return;
							auto path = currentShader->Pipeline->FindImportOperatorChain(srcWorld, userWorld);
							if (path.Count() == 0)
								throw InvalidProgramException(L"no import exists, this should have been checked by semantics analyzer.");
							for (int i = 0; i < path[0].Nodes.Count(); i++)
							{
								auto & node = path[0].Nodes[i];
								if (node.TargetWorld != userWorld)
								{
									// should define output in node.TargetWorld
									outputWorlds.Add(node.TargetWorld);
								}
								if (node.TargetWorld != srcWorld)
								{
									// should define input in node.TargetWorld
									ImportTechnique tech;
									tech.SourceWorld = path[0].Nodes[i - 1].TargetWorld;
									tech.ImportOperator = path[0].Nodes[i].ImportOperator;
									inputSourceWorlds[node.TargetWorld] = tech;
								}
							}
						};
						for (auto user : comp->Users)
						{
							if (user->World != srcWorld)
							{
								useInWorld(user->World);
							}
						}

						ShaderComponentImplSymbol * compImpl = comp->Implementation;
						
						// define outputs in all involved worlds
						for (auto & outputWorld : outputWorlds)
						{
							auto & w = compiledShader->Worlds[outputWorld].GetValue();
							
							ComponentDefinition compDef;
							compDef.Name = comp->Component->UniqueName;
							compDef.OrderingStr = compImpl->SyntaxNode->Position.FileName +
								String(compImpl->SyntaxNode->Position.Line).PadLeft(L' ', 8) + compDef.Name;
							compDef.Type = TranslateExpressionType(comp->Component->Type->DataType);
							compDef.LayoutAttribs = compImpl->SyntaxNode->LayoutAttributes;
							w->WorldOutput->Entries.AddIfNotExists(compDef.Name, compDef);
							// if an input is also defined in this world (i.e. this is a through world), insert assignment
							ImportTechnique importTech;
							if (inputSourceWorlds.TryGetValue(outputWorld, importTech))
							{
								ThroughVar tvar;
								tvar.ImportOperator = importTech.ImportOperator;
								tvar.Export = true;
								tvar.InputName = comp->Component->UniqueName;
								tvar.Component = comp;
								tvar.OutputName = w->WorldOutput->Name + L"." + comp->Component->UniqueName;
								tvar.InputWorldName = importTech.SourceWorld;
								worldThroughVars[outputWorld].GetValue().Add(tvar);
							}
						}
						// define inputs
						for (auto & input : inputSourceWorlds)
						{
							ThroughVar tvar;
							tvar.ImportOperator = input.Value.ImportOperator;
							tvar.Export = false;
							tvar.InputName = comp->Component->UniqueName;
							tvar.Component = comp;
							tvar.InputWorldName = input.Value.SourceWorld;
							worldThroughVars[input.Key].GetValue().Add(tvar);
						}
					}
				}
				for (auto & world : shader->Pipeline->Worlds)
				{
					if (world.Value.IsAbstract)
						continue;
					NamingCounter = 0;

					auto & components = worldComps[world.Key].GetValue();
					auto compiledWorld = compiledShader->Worlds[world.Key].GetValue().Ptr();
					currentWorld = compiledWorld;
					codeWriter.PushNode();
					variables.PushScope();
					HashSet<String> localComponents;
					for (auto & comp : components)
						localComponents.Add(comp->Component->UniqueName);

					DependencySort(components, [](ComponentDefinitionIR * def)
					{
						return def->Dependency;
					});

					auto generateImportInstr = [&](ComponentDefinitionIR * comp, ComponentDefinitionIR * user)
					{
						ImportInstruction * instr = nullptr;
						if (!compiledWorld->ImportInstructions.TryGetValue(comp->Component->UniqueName, instr))
						{
							auto path = shader->Pipeline->FindImportOperatorChain(comp->World, world.Key);
							auto importOp = path[0].Nodes.Last().ImportOperator;
							auto sourceWorld = compiledShader->Worlds[path[0].Nodes[path[0].Nodes.Count() - 2].TargetWorld].GetValue().Ptr();
							instr = new ImportInstruction(importOp->Usings.Count(), comp->Component->UniqueName, importOp,
								sourceWorld, TranslateExpressionType(comp->Component->Type->DataType));
							for (int i = 0; i < importOp->Usings.Count(); i++)
							{
								// resolve import operator arguments
								ILOperand * val = nullptr;
								if (!variables.TryGetValue(importOp->Usings[i].Content, val))
								{
									ImportInstruction * impInstr = nullptr;
									compiledWorld->ImportInstructions.TryGetValue(importOp->Usings[i].Content, impInstr);
									val = impInstr;
								}
								String userCompName;
								if (user)
									userCompName = user->Component->Name;
								else
									userCompName = comp->Component->Name;
								if (!val)
									result.GetErrorWriter()->Error(50010, L"\'" + importOp->Usings[i].Content + L"\': implicit import operator argument is not accessible when attempting to import \'"
										+ comp->Component->Name + L"\' from world \'" + sourceWorld->WorldName + L"\' when compiling \'" + userCompName + L"\' for world \'" + compiledWorld->WorldName + L"\'.\nsee import operator declaration at " +
										importOp->Position.ToString() + L".", comp->Implementation->SyntaxNode->Position);
								else
									instr->Arguments[i] = val;
							}
							sourceWorld->WorldOutput->UserWorlds.Add(world.Key);
							instr->Name = L"_vout" + comp->Component->UniqueName;
							codeWriter.Insert(instr);
							compiledWorld->ImportInstructions[comp->Component->UniqueName] = instr;
							CompiledComponent ccomp;
							ccomp.CodeOperand = instr;
							ccomp.Attributes = comp->Implementation->SyntaxNode->LayoutAttributes;
							compiledWorld->LocalComponents[comp->Component->UniqueName] = ccomp;
						}
						return instr;
					};
					HashSet<String> thisWorldComponents;
					for (auto & comp : components)
					{
						thisWorldComponents.Add(comp->Component->UniqueName);
					}
					auto & throughVars = worldThroughVars[world.Key].GetValue();
					auto genInputVar = [&]()
					{
						for (auto & throughVar : throughVars)
						{
							bool shouldSkip = false;
							for (auto & depComp : throughVar.ImportOperator->Usings)
							{
								if (thisWorldComponents.Contains(depComp.Content))
								{
									shouldSkip = true;
									break;
								}
							}
							if (shouldSkip)
								continue;
							auto srcInstr = generateImportInstr(throughVar.Component, nullptr);
							if (throughVar.Export)
							{
								auto exp = new ExportInstruction(throughVar.Component->Component->UniqueName, compiledWorld->ExportOperator.Content, compiledWorld, srcInstr);
								codeWriter.Insert(exp);
								throughVars.Remove(throughVar);
							}
						}
					};
					genInputVar();
					for (auto & comp : components)
					{
						genInputVar();
						for (auto & dep : comp->Dependency)
						{
							if (dep->World != world.Key)
							{
								generateImportInstr(dep, comp);
							}
						}
						thisWorldComponents.Remove(comp->Component->UniqueName);
						VisitComponent(comp);
					}
					
					variables.PopScope();
					compiledWorld->Code = codeWriter.PopNode();
					EvalReferencedFunctionClosure(compiledWorld);
					currentWorld = nullptr;

					// fill in meta data
					WorldMetaData wdata;
					for (auto & comp : components)
						wdata.Components.Add(comp->Component->UniqueName);
					wdata.Name = compiledWorld->WorldName;
					wdata.TargetName = compiledWorld->TargetMachine;
					wdata.OutputBlock = compiledWorld->WorldOutput->Name;
					for (auto & inputBlock : compiledWorld->WorldInputs)
						wdata.InputBlocks.Add(inputBlock.Value.Block->Name);
					compiledWorld->Shader->MetaData.Worlds.Add(wdata.Name, wdata);
				}
				for (auto & block : compiledShader->InterfaceBlocks)
				{
					SortInterfaceBlock(block.Value.Ptr());
					InterfaceBlockMetaData blockMeta;
					blockMeta.Name = block.Value->Name;
					blockMeta.Attributes = block.Value->Attributes;
					int offset = 0;
					bool pack = block.Value->Attributes.ContainsKey(L"Packed");
					for (auto & entry : block.Value->Entries)
					{
						InterfaceBlockEntry ment;
						ment.Type = dynamic_cast<ILBasicType*>(entry.Value.Type.Ptr())->Type;
						ment.Name = entry.Key;
						ment.Attributes = entry.Value.LayoutAttribs;
						if (!pack)
							offset = RoundToAlignment(offset, AlignmentOfBaseType(ment.Type));
						ment.Offset = offset;
						entry.Value.Offset = offset;
						ment.Size = SizeofBaseType(ment.Type);
						offset += ment.Size;
						blockMeta.Entries.Add(ment);
					}
					block.Value->Size = offset;
					if (!pack && block.Value->Entries.Count() > 0)
						block.Value->Size = RoundToAlignment(offset, AlignmentOfBaseType(dynamic_cast<ILBasicType*>((*(block.Value->Entries.begin())).Value.Type.Ptr())->Type));
					blockMeta.Size = block.Value->Size;
					compiledShader->MetaData.InterfaceBlocks[blockMeta.Name] = blockMeta;
				}
				currentShader = nullptr;
			}
			void EvalReferencedFunctionClosure(CompiledWorld * world)
			{
				List<String> workList;
				for (auto & rfunc : world->ReferencedFunctions)
					workList.Add(rfunc);
				for (int i = 0; i < workList.Count(); i++)
				{
					auto rfunc = workList[i];
					RefPtr<FunctionSymbol> funcSym;
					if (symTable->Functions.TryGetValue(rfunc, funcSym))
					{
						for (auto & rrfunc : funcSym->ReferencedFunctions)
						{
							world->ReferencedFunctions.Add(rrfunc);
							workList.Add(rrfunc);
						}
					}
				}
			}
			virtual void VisitComponent(ComponentSyntaxNode *) override
			{
				throw NotImplementedException();
			}
			void VisitComponent(ComponentDefinitionIR * comp)
			{
				currentComponent = comp->Component;
				currentComponentImpl = comp->Implementation;
				String varName = L"_vcmp" + currentComponent->UniqueName;

				RefPtr<ILType> type = TranslateExpressionType(currentComponent->Type->DataType);
				auto allocVar = codeWriter.AllocVar(type, result.Program->ConstantPool->CreateConstant(1));
				allocVar->Name = varName;
				variables.Add(currentComponent->UniqueName, allocVar);
				CompiledComponent ccomp;
				ccomp.CodeOperand = allocVar;
				ccomp.Attributes = comp->Implementation->SyntaxNode->LayoutAttributes;
				currentWorld->LocalComponents[currentComponent->UniqueName] = ccomp;
				if (currentComponentImpl->SyntaxNode->Expression)
				{
					currentComponentImpl->SyntaxNode->Expression->Accept(this);
					Assign(currentComponentImpl->SyntaxNode->Type->ToExpressionType(), allocVar, exprStack.Last());
					if (currentWorld->WorldOutput->Entries.ContainsKey(currentComponent->UniqueName))
					{
						auto exp = new ExportInstruction(currentComponent->UniqueName, currentWorld->ExportOperator.Content, currentWorld,
							allocVar);
						codeWriter.Insert(exp);
					}
					exprStack.Clear();
				}
				else if (currentComponentImpl->SyntaxNode->BlockStatement)
				{
					currentComponentImpl->SyntaxNode->BlockStatement->Accept(this);
				}
				currentComponentImpl = nullptr;
				currentComponent = nullptr;
			}
			virtual void VisitFunction(FunctionSyntaxNode* function) override
			{
				if (function->IsExtern)
					return;
				RefPtr<CompiledFunction> func = new CompiledFunction();
				result.Program->Functions.Add(func);
				func->Name = function->InternalName;
				func->ReturnType = TranslateExpressionType(function->ReturnType->ToExpressionType());
				variables.PushScope();
				codeWriter.PushNode();
				int id = 0;
				for (auto &param : function->Parameters)
				{
					func->Parameters.Add(param->Name, TranslateExpressionType(param->Type->ToExpressionType()));
					auto op = FetchArg(param->Type->ToExpressionType(), ++id);
					op->Name = String(L"p_") + param->Name;
					variables.Add(param->Name, op);
				}
				function->Body->Accept(this);
				func->Code = codeWriter.PopNode();
				variables.PopScope();
			}
			virtual void VisitBlockStatement(BlockStatementSyntaxNode* stmt) override
			{
				variables.PushScope();
				for (auto & subStmt : stmt->Statements)
					subStmt->Accept(this);
				variables.PopScope();
			}
			virtual void VisitEmptyStatement(EmptyStatementSyntaxNode*) override {}
			virtual void VisitWhileStatement(WhileStatementSyntaxNode* stmt) override
			{
				RefPtr<WhileInstruction> instr = new WhileInstruction();
				variables.PushScope();
				codeWriter.PushNode();
				stmt->Predicate->Accept(this);
				codeWriter.Insert(new ReturnInstruction(PopStack()));
				instr->ConditionCode = codeWriter.PopNode();
				codeWriter.PushNode();
				stmt->Statement->Accept(this);
				instr->BodyCode = codeWriter.PopNode();
				codeWriter.Insert(instr.Release());
				variables.PopScope();
			}
			virtual void VisitDoWhileStatement(DoWhileStatementSyntaxNode* stmt) override
			{
				RefPtr<WhileInstruction> instr = new DoInstruction();
				variables.PushScope();
				codeWriter.PushNode();
				stmt->Predicate->Accept(this);
				codeWriter.Insert(new ReturnInstruction(PopStack()));
				instr->ConditionCode = codeWriter.PopNode();
				codeWriter.PushNode();
				stmt->Statement->Accept(this);
				instr->BodyCode = codeWriter.PopNode();
				codeWriter.Insert(instr.Release());
				variables.PopScope();
			}
			virtual void VisitForStatement(ForStatementSyntaxNode* stmt) override
			{
				RefPtr<ForInstruction> instr = new ForInstruction();
				variables.PushScope();
				if (stmt->TypeDef)
				{
					AllocVarInstruction * varOp = AllocVar(stmt->TypeDef->ToExpressionType());
					varOp->Name = L"v_" + stmt->IterationVariable.Content;
					variables.Add(stmt->IterationVariable.Content, varOp);
				}
				ILOperand * iterVar = nullptr;
				if (!variables.TryGetValue(stmt->IterationVariable.Content, iterVar))
					throw InvalidProgramException(L"Iteration variable not found in variables dictionary. This should have been checked by semantics analyzer.");
				stmt->InitialExpression->Accept(this);
				Assign(stmt->TypeDef->ToExpressionType(), iterVar, PopStack());

				codeWriter.PushNode();
				stmt->EndExpression->Accept(this);
				auto val = PopStack();
				codeWriter.Insert(new CmpleInstruction(new LoadInstruction(iterVar), val));
				instr->ConditionCode = codeWriter.PopNode();

				codeWriter.PushNode();
				ILOperand * stepVal = nullptr;
				if (stmt->StepExpression)
				{
					stmt->StepExpression->Accept(this);
					stepVal = PopStack();
				}
				else
				{
					if (iterVar->Type->IsFloat())
						stepVal = result.Program->ConstantPool->CreateConstant(1.0f);
					else
						stepVal = result.Program->ConstantPool->CreateConstant(1);
				}
				auto afterVal = new AddInstruction(new LoadInstruction(iterVar), stepVal);
				codeWriter.Insert(afterVal);
				Assign(stmt->TypeDef->ToExpressionType(), iterVar, afterVal);
				instr->SideEffectCode = codeWriter.PopNode();

				codeWriter.PushNode();
				stmt->Statement->Accept(this);
				instr->BodyCode = codeWriter.PopNode();
				codeWriter.Insert(instr.Release());
				variables.PopScope();
			}
			virtual void VisitIfStatement(IfStatementSyntaxNode* stmt) override
			{
				RefPtr<IfInstruction> instr = new IfInstruction();
				variables.PushScope();
				stmt->Predicate->Accept(this);
				instr->Operand = PopStack();
				codeWriter.PushNode();
				stmt->PositiveStatement->Accept(this);
				instr->TrueCode = codeWriter.PopNode();
				if (stmt->NegativeStatement)
				{
					codeWriter.PushNode();
					stmt->NegativeStatement->Accept(this);
					instr->FalseCode = codeWriter.PopNode();
				}
				codeWriter.Insert(instr.Release());
				variables.PopScope();
			}
			virtual void VisitReturnStatement(ReturnStatementSyntaxNode* stmt) override
			{
				if (currentComponentImpl != nullptr)
				{
					if (stmt->Expression)
					{
						stmt->Expression->Accept(this);
						ILOperand *op = nullptr;
						variables.TryGetValue(currentComponent->UniqueName, op);
						auto val = PopStack();
						codeWriter.Store(op, val);
						if (currentWorld->WorldOutput->Entries.ContainsKey(currentComponent->UniqueName))
						{
							auto exp = new ExportInstruction(currentComponent->UniqueName, currentWorld->ExportOperator.Content, currentWorld, op);
							codeWriter.Insert(exp);
						}
					}
				}
				else
				{
					if (stmt->Expression)
						stmt->Expression->Accept(this);
					codeWriter.Insert(new ReturnInstruction(PopStack()));
				}
			}
			virtual void VisitBreakStatement(BreakStatementSyntaxNode*) override
			{
				codeWriter.Insert(new BreakInstruction());
			}
			virtual void VisitContinueStatement(ContinueStatementSyntaxNode*) override
			{
				codeWriter.Insert(new ContinueInstruction());
			}
			virtual void VisitSelectExpression(SelectExpressionSyntaxNode * expr) override
			{
				expr->SelectorExpr->Accept(this);
				auto predOp = PopStack();
				expr->Expr0->Accept(this);
				auto v0 = PopStack();
				expr->Expr1->Accept(this);
				auto v1 = PopStack();
				codeWriter.Select(predOp, v0, v1);
			}
			ILOperand * EnsureBoolType(ILOperand * op, ExpressionType type)
			{
				if (type != ExpressionType::Bool)
				{
					auto cmpeq = new CmpneqInstruction();
					cmpeq->Operands[0] = op;
					cmpeq->Operands[1] = result.Program->ConstantPool->CreateConstant(0);
					cmpeq->Type = new ILBasicType(ILBaseType::Int);
					codeWriter.Insert(cmpeq);
					return cmpeq;
				}
				else
					return op;
			}
			
			virtual void VisitVarDeclrStatement(VarDeclrStatementSyntaxNode* stmt) override
			{
				for (auto & v : stmt->Variables)
				{
					AllocVarInstruction * varOp = AllocVar(stmt->Type->ToExpressionType());
					varOp->Name = L"v" + String(NamingCounter++) + L"_" + v->Name;
					variables.Add(v->Name, varOp);
					if (v->Expression)
					{
						v->Expression->Accept(this);
						Assign(stmt->Type->ToExpressionType(), varOp, PopStack());
					}
				}
			}
			virtual void VisitExpressionStatement(ExpressionStatementSyntaxNode* stmt) override
			{
				stmt->Expression->Accept(this);
				PopStack();
			}
			void Assign(const ExpressionType & /*type*/, ILOperand * left, ILOperand * right)
			{
				if (auto add = dynamic_cast<AddInstruction*>(left))
				{
					auto baseOp = add->Operands[0].Ptr();
					codeWriter.Store(add->Operands[0].Ptr(), codeWriter.Update(codeWriter.Load(baseOp), add->Operands[1].Ptr(), right));
					add->Erase();
				}
				else
					codeWriter.Store(left, right);
			}
			virtual void VisitBinaryExpression(BinaryExpressionSyntaxNode* expr) override
			{
				expr->RightExpression->Accept(this);
				auto right = PopStack();
				if (expr->Operator == Operator::Assign)
				{
					expr->LeftExpression->Access = ExpressionAccess::Write;
					expr->LeftExpression->Accept(this);
					auto left = PopStack();
					Assign(expr->LeftExpression->Type, left, right);
					PushStack(left);
				}
				else
				{
					expr->LeftExpression->Access = ExpressionAccess::Read;
					expr->LeftExpression->Accept(this);
					auto left = PopStack();
					BinaryInstruction * rs = 0;
					switch (expr->Operator)
					{
					case Operator::Add:
					case Operator::AddAssign:
						rs = new AddInstruction();
						break;
					case Operator::Sub:
					case Operator::SubAssign:
						rs = new SubInstruction();
						break;
					case Operator::Mul:
					case Operator::MulAssign:
						rs = new MulInstruction();
						break;
					case Operator::Mod:
					case Operator::ModAssign:
						rs = new ModInstruction();
						break;
					case Operator::Div:
					case Operator::DivAssign:
						rs = new DivInstruction();
						break;
					case Operator::And:
						rs = new AndInstruction();
						break;
					case Operator::Or:
						rs = new OrInstruction();
						break;
					case Operator::BitAnd:
						rs = new BitAndInstruction();
						break;
					case Operator::BitOr:
						rs = new BitOrInstruction();
						break;
					case Operator::BitXor:
						rs = new BitXorInstruction();
						break;
					case Operator::Lsh:
						rs = new ShlInstruction();
						break;
					case Operator::Rsh:
						rs = new ShrInstruction();
						break;
					case Operator::Eql:
						rs = new CmpeqlInstruction();
						break;
					case Operator::Neq:
						rs = new CmpneqInstruction();
						break;
					case Operator::Greater:
						rs = new CmpgtInstruction();
						break;
					case Operator::Geq:
						rs = new CmpgeInstruction();
						break;
					case Operator::Leq:
						rs = new CmpleInstruction();
						break;
					case Operator::Less:
						rs = new CmpltInstruction();
						break;
					default:
						throw NotImplementedException(L"Code gen not implemented for this operator.");
					}
					rs->Operands.SetSize(2);
					rs->Operands[0] = left;
					rs->Operands[1] = right;
					rs->Type = TranslateExpressionType(expr->Type);
					codeWriter.Insert(rs);
					switch (expr->Operator)
					{
					case Operator::AddAssign:
					case Operator::SubAssign:
					case Operator::MulAssign:
					case Operator::DivAssign:
					case Operator::ModAssign:
					{
						expr->LeftExpression->Access = ExpressionAccess::Write;
						expr->LeftExpression->Accept(this);
						auto target = PopStack();
						Assign(expr->Type, target, rs);
						break;
					}
					default:
						break;
					}
					PushStack(rs);
				}
			}
			virtual void VisitConstantExpression(ConstantExpressionSyntaxNode* expr) override
			{
				ILConstOperand * op;
				if (expr->ConstType == ConstantExpressionSyntaxNode::ConstantType::Float)
				{
					op = result.Program->ConstantPool->CreateConstant(expr->FloatValue);
				}
				else
				{
					op = result.Program->ConstantPool->CreateConstant(expr->IntValue);
				}
				PushStack(op);
			}
			void GenerateIndexExpression(ILOperand * base, ILOperand * idx, bool read)
			{
				if (read)
				{
					auto ldInstr = codeWriter.Retrieve(base, idx);
					ldInstr->Attribute = base->Attribute;
					PushStack(ldInstr);
				}
				else
				{
					PushStack(codeWriter.Add(base, idx));
				}
			}
			virtual void VisitIndexExpression(IndexExpressionSyntaxNode* expr) override
			{
				expr->BaseExpression->Access = expr->Access;
				expr->BaseExpression->Accept(this);
				auto base = PopStack();
				expr->IndexExpression->Access = ExpressionAccess::Read;
				expr->IndexExpression->Accept(this);
				auto idx = PopStack();
				GenerateIndexExpression(base, idx,
					expr->Access == ExpressionAccess::Read);
			}
			virtual void VisitMemberExpression(MemberExpressionSyntaxNode * expr) override
			{
				RefPtr<Object> refObj;
				if (expr->Tags.TryGetValue(L"ComponentReference", refObj))
				{
					if (auto refComp = dynamic_cast<ShaderComponentSymbol*>(refObj.Ptr()))
					{
						ILOperand * op;
						if (variables.TryGetValue(refComp->UniqueName, op))
							PushStack(op);
						else
							PushStack(currentWorld->ImportInstructions[refComp->UniqueName]());
					}
				}
				else
				{
					expr->BaseExpression->Access = expr->Access;
					expr->BaseExpression->Accept(this);
					auto base = PopStack();
					auto generateSingleMember = [&](wchar_t memberName)
					{
						int idx = 0;
						if (memberName == L'y' || memberName == L'g')
							idx = 1;
						else if (memberName == L'z' || memberName == L'b')
							idx = 2;
						else if (memberName == L'w' || memberName == L'a')
							idx = 3;

						GenerateIndexExpression(base, result.Program->ConstantPool->CreateConstant(idx),
							expr->Access == ExpressionAccess::Read);
					};
					if (expr->BaseExpression->Type.IsVectorType())
					{
						if (expr->MemberName.Length() == 1)
						{
							generateSingleMember(expr->MemberName[0]);
						}
						else
						{
							if (expr->Access != ExpressionAccess::Read)
								throw InvalidOperationException(L"temporary vector (vec.xyz) is read-only.");
							String funcName = BaseTypeToString(expr->Type.BaseType);
							auto rs = AllocVar(expr->Type);
							ILOperand* tmp = codeWriter.Load(rs);
							for (int i = 0; i < expr->MemberName.Length(); i++)
							{
								generateSingleMember(expr->MemberName[i]);
								tmp = codeWriter.Update(tmp, result.Program->ConstantPool->CreateConstant(i), PopStack());
							}
							codeWriter.Store(rs, tmp);
							PushStack(codeWriter.Load(rs));
						}
					}
					else
						throw NotImplementedException(L"member expression codegen");
				}
			}
			virtual void VisitInvokeExpression(InvokeExpressionSyntaxNode* expr) override
			{
				List<ILOperand*> args;
				if (currentWorld)
					currentWorld->ReferencedFunctions.Add(expr->FunctionExpr->Variable);
				for (auto arg : expr->Arguments)
				{
					arg->Accept(this);
					args.Add(PopStack());
				}
				auto instr = new CallInstruction(args.Count());
				instr->Function = expr->FunctionExpr->Variable;
				for (int i = 0; i < args.Count(); i++)
					instr->Arguments[i] = args[i];
				instr->Type = TranslateExpressionType(expr->Type);
				codeWriter.Insert(instr);
				PushStack(instr);
			}
			virtual void VisitTypeCastExpression(TypeCastExpressionSyntaxNode * expr) override
			{
				expr->Expression->Accept(this);
				auto base = PopStack();
				if (expr->Expression->Type == expr->Type)
				{
					PushStack(base);
				}
				else if (expr->Expression->Type == ExpressionType::Float &&
					expr->Type == ExpressionType::Int)
				{
					auto instr = new Float2IntInstruction(base);
					codeWriter.Insert(instr);
					PushStack(instr);
				}
				else if (expr->Expression->Type == ExpressionType::Int &&
					expr->Type == ExpressionType::Float)
				{
					auto instr = new Int2FloatInstruction(base);
					codeWriter.Insert(instr);
					PushStack(instr);
				}
				else
				{
					Error(40001, L"Invalid type cast: \"" + expr->Expression->Type.ToString() + L"\" to \"" +
						expr->Type.ToString() + L"\"", expr);
				}
			}
			virtual void VisitUnaryExpression(UnaryExpressionSyntaxNode* expr) override
			{
				if (expr->Operator == Operator::PostDec || expr->Operator == Operator::PostInc
					|| expr->Operator == Operator::PreDec || expr->Operator == Operator::PreInc)
				{
					expr->Expression->Access = ExpressionAccess::Read;
					expr->Expression->Accept(this);
					auto base = PopStack();
					BinaryInstruction * instr;
					if (expr->Operator == Operator::PostDec)
						instr = new SubInstruction();
					else
						instr = new AddInstruction();
					instr->Operands.SetSize(2);
					instr->Operands[0] = base;
					if (expr->Type == ExpressionType::Float)
						instr->Operands[1] = result.Program->ConstantPool->CreateConstant(1.0f);
					else
						instr->Operands[1] = result.Program->ConstantPool->CreateConstant(1);
					instr->Type = TranslateExpressionType(expr->Type);
					codeWriter.Insert(instr);

					expr->Expression->Access = ExpressionAccess::Write;
					expr->Expression->Accept(this);
					auto dest = PopStack();
					auto store = new StoreInstruction(dest, instr);
					codeWriter.Insert(store);
					PushStack(base);
				}
				else if (expr->Operator == Operator::PreDec || expr->Operator == Operator::PreInc)
				{
					expr->Expression->Access = ExpressionAccess::Read;
					expr->Expression->Accept(this);
					auto base = PopStack();
					BinaryInstruction * instr;
					if (expr->Operator == Operator::PostDec)
						instr = new SubInstruction();
					else
						instr = new AddInstruction();
					instr->Operands.SetSize(2);
					instr->Operands[0] = base;
					if (expr->Type == ExpressionType::Float)
						instr->Operands[1] = result.Program->ConstantPool->CreateConstant(1.0f);
					else
						instr->Operands[1] = result.Program->ConstantPool->CreateConstant(1);
					instr->Type = TranslateExpressionType(expr->Type);
					codeWriter.Insert(instr);

					expr->Expression->Access = ExpressionAccess::Write;
					expr->Expression->Accept(this);
					auto dest = PopStack();
					auto store = new StoreInstruction(dest, instr);
					codeWriter.Insert(store);
					PushStack(instr);
				}
				else
				{
					expr->Expression->Accept(this);
					auto base = PopStack();
					auto genUnaryInstr = [&](ILOperand * input)
					{
						UnaryInstruction * rs = 0;
						switch (expr->Operator)
						{
						case Operator::Not:
							input = EnsureBoolType(input, expr->Expression->Type);
							rs = new NotInstruction();
							break;
						case Operator::Neg:
							rs = new NegInstruction();
							break;
						case Operator::BitNot:
							rs = new BitNotInstruction();
							break;
						default:
							throw NotImplementedException(L"Code gen is not implemented for this operator.");
						}
						rs->Operand = input;
						rs->Type = input->Type;
						codeWriter.Insert(rs);
						return rs;
					};
					PushStack(genUnaryInstr(base));
				}
			}
			bool GenerateVarRef(String name, ExpressionType & type, ExpressionAccess access)
			{
				ILOperand * var = 0;
				String srcName = name;
				if (!variables.TryGetValue(srcName, var))
				{
					return false;
				}
				if (access == ExpressionAccess::Read)
				{
					auto instr = new LoadInstruction();
					instr->Name = L"local_" + name;
					instr->Operand = var;
					instr->Type = TranslateExpressionType(type);
					codeWriter.Insert(instr);
					instr->Attribute = var->Attribute;
					if (!Is<LeaInstruction>(var))
						throw L"error";
					PushStack(instr);
				}
				else
				{
					PushStack(var);
				}
				return true;
			}
			virtual void VisitVarExpression(VarExpressionSyntaxNode* expr) override
			{
				RefPtr<Object> refObj;
				if (expr->Tags.TryGetValue(L"ComponentReference", refObj))
				{
					if (auto refComp = dynamic_cast<ShaderComponentSymbol*>(refObj.Ptr()))
					{
						ILOperand * op;
						if (variables.TryGetValue(refComp->UniqueName, op))
							PushStack(op);
						else
							PushStack(currentWorld->ImportInstructions[refComp->UniqueName]());
					}
				}
				else if (!GenerateVarRef(expr->Variable, expr->Type, expr->Access))
				{
						throw InvalidProgramException(L"identifier is neither a variable nor a regnoized component.");
				}
			}
			virtual void VisitParameter(ParameterSyntaxNode*) override {}
			virtual void VisitType(TypeSyntaxNode*) override {}
			virtual void VisitDeclrVariable(Variable*) override {}
		private:
			CodeGenerator & operator = (const CodeGenerator & other) = delete;
		public:
			CodeGenerator(SymbolTable * symbols, ErrorWriter * pErr, CompileResult & _result)
				: ICodeGenerator(pErr), symTable(symbols), result(_result)
			{
				result.Program = new CompiledProgram();
				codeWriter.SetConstantPool(result.Program->ConstantPool.Ptr());
			}
		};

		ICodeGenerator * CreateCodeGenerator(SymbolTable * symbols, CompileResult & result)
		{
			return new CodeGenerator(symbols, result.GetErrorWriter(), result);
		}
	}
}
