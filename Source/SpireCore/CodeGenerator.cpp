#include "SyntaxVisitors.h"
#include "ScopeDictionary.h"
#include "CodeWriter.h"
#include "StringObject.h"
#include "Naming.h"
#include "TypeTranslation.h"
#include "../CoreLib/Tokenizer.h"
#include <assert.h>

namespace Spire
{
	namespace Compiler
	{
		const int MaxBindingValue = 128;

		template<typename Func>
		class ImportNodeVisitor : public SyntaxVisitor
		{
		public:
			const Func & func;
			ImportNodeVisitor(const Func & f)
				: SyntaxVisitor(nullptr), func(f)
			{}
			virtual RefPtr<ExpressionSyntaxNode> VisitImportExpression(ImportExpressionSyntaxNode * expr) override
			{
				func(expr);
				return expr;
			}
		};

		template<typename Func>
		void EnumerateImportExpressions(SyntaxNode * node, const Func & f)
		{
			ImportNodeVisitor<Func> visitor(f);
			node->Accept(&visitor);
		}

		class CodeGenerator : public ICodeGenerator
		{
		private:
			SymbolTable * symTable;
			ILWorld * currentWorld = nullptr;
			ComponentDefinitionIR * currentComponent = nullptr;
			ILOperand * returnRegister = nullptr;
			ImportExpressionSyntaxNode * currentImport = nullptr;
			ShaderIR * currentShader = nullptr;
			CompileResult & result;
			List<ILOperand*> exprStack;
			CodeWriter codeWriter;
			ScopeDictionary<String, ILOperand*> variables;
			Dictionary<String, RefPtr<ILType>> genericTypeMappings;

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
			AllocVarInstruction * AllocVar(ExpressionType * etype)
			{
				AllocVarInstruction * varOp = 0;
				RefPtr<ILType> type = TranslateExpressionType(etype, &genericTypeMappings);
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
			FetchArgInstruction * FetchArg(ExpressionType * etype, int argId)
			{
				auto type = TranslateExpressionType(etype, &genericTypeMappings);
				auto arrType = dynamic_cast<ILArrayType*>(type.Ptr());
				FetchArgInstruction * varOp = 0;
				if (arrType)
				{
					auto baseType = arrType->BaseType.Release();
					varOp = codeWriter.FetchArg(baseType, argId);
				}
				else
				{
					varOp = codeWriter.FetchArg(type, argId);
				}
				return varOp;
			}
			void TranslateStages(ILShader * compiledShader, PipelineSyntaxNode * pipeline)
			{
				for (auto & stage : pipeline->Stages)
				{
					RefPtr<ILStage> ilStage = new ILStage();
					ilStage->Position = stage->Position;
					ilStage->Name = stage->Name.Content;
					ilStage->StageType = stage->StageType.Content;
					for (auto & attrib : stage->Attributes)
					{
						StageAttribute sattrib;
						sattrib.Name = attrib.Key;
						sattrib.Position = attrib.Value.Position;
						sattrib.Value = attrib.Value.Content;
						ilStage->Attributes[attrib.Key] = sattrib;
					}
					compiledShader->Stages[stage->Name.Content] = ilStage;
				}
			}
			String GetComponentFunctionName(ComponentSyntaxNode * comp)
			{
				StringBuilder nameSb;
				nameSb << comp->ParentModuleName.Content << "." << comp->Name.Content;
				StringBuilder finalNameSb;
				for (auto ch : nameSb.ProduceString())
				{
					if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
						finalNameSb << ch;
					else
						finalNameSb << '_';
				}
				return EscapeDoubleUnderscore(finalNameSb.ProduceString());
			}
		public:
			virtual RefPtr<StructSyntaxNode> VisitStruct(StructSyntaxNode * st) override
			{
				result.Program->Structs.Add(symTable->Structs[st->Name.Content]()->Type);
				return st;
			}
			virtual void ProcessFunction(FunctionSyntaxNode * func) override
			{
				VisitFunction(func);
			}
			virtual void ProcessStruct(StructSyntaxNode * st) override
			{
				VisitStruct(st);
			}

			virtual void ProcessShader(ShaderIR * shader) override
			{
				currentShader = shader;
				auto pipeline = shader->Shader->Pipeline;
				RefPtr<ILShader> compiledShader = new ILShader();
				compiledShader->Name = shader->Shader->Name;
				compiledShader->Position = shader->Shader->Position;
				TranslateStages(compiledShader.Ptr(), pipeline->SyntaxNode);
				result.Program->Shaders.Add(compiledShader);

				genericTypeMappings.Clear();

				
				// pass 1: iterating all worlds
				// create ILWorld and ILRecordType objects for all worlds

				for (auto & world : pipeline->Worlds)
				{
					auto w = new ILWorld();
					auto recordType = new ILRecordType();
					recordType->TypeName = world.Key;
					genericTypeMappings[world.Key] = recordType;
					w->Name = world.Key;
					w->OutputType = recordType;
					w->Attributes = world.Value.SyntaxNode->LayoutAttributes;
					w->Shader = compiledShader.Ptr();
					w->IsAbstract = world.Value.IsAbstract;
					auto impOps = pipeline->GetImportOperatorsFromSourceWorld(world.Key);
					w->Position = world.Value.SyntaxNode->Position;
					compiledShader->Worlds[world.Key] = w;
				}

				// pass 2: iterating all worlds:
				// 1) Gather list of components for each world, and store it in worldComps dictionary.
				// 2) For each abstract world, add its components to record type

				Dictionary<String, List<ComponentDefinitionIR*>> worldComps;
				
				for (auto & world : pipeline->Worlds)
				{
					// gather list of components
					List<ComponentDefinitionIR*> components;
					for (auto & compDef : shader->Definitions)
						if (compDef->World == world.Key)
							components.Add(compDef.Ptr());

					// for abstract world, fill in record type now
					if (world.Value.IsAbstract)
					{
						auto compiledWorld = compiledShader->Worlds[world.Key]();
						for (auto & comp : components)
						{
							ILObjectDefinition compDef;
							compDef.Attributes = comp->SyntaxNode->LayoutAttributes;
							compDef.Name = comp->UniqueName;
							compDef.Type = TranslateExpressionType(comp->Type.Ptr(), &genericTypeMappings);
							compDef.Position = comp->SyntaxNode->Position;
							compDef.Binding = -1;
							
							compiledWorld->OutputType->Members.AddIfNotExists(compDef.Name, compDef);
						}
					}
					// put the list in worldComps
					worldComps[world.Key] = components;
				}

				// allocate binding slots for shader resources (textures, buffers, samplers etc.)

				Dictionary<int, ILObjectDefinition*> usedTextureBindings, usedBufferBindings, usedSamplerBindings, usedStorageBufferBindings;
				int textureBindingAllcator = 0, samplerBindingAllocator = 0, storageBufferBindingAllocator = 0, bufferBindingAllocator = 0;
				// first pass: process components with user defined binding slots
				for (auto & world : pipeline->Worlds)
				{
					if (world.Value.IsAbstract)
					{
						auto compiledWorld = compiledShader->Worlds[world.Key]();
						for (auto & compDefPair : compiledWorld->OutputType->Members)
						{
							auto & compDef = compDefPair.Value;
							auto bindableResType = compDef.Type->GetBindableResourceType();
							if (bindableResType != BindableResourceType::NonBindable)
							{
								Dictionary<int, ILObjectDefinition*> * bindingRegistry = nullptr;
								switch (bindableResType)
								{
								case BindableResourceType::Texture:
									bindingRegistry = &usedTextureBindings;
									break;
								case BindableResourceType::Sampler:
									bindingRegistry = &usedSamplerBindings;
									break;
								case BindableResourceType::Buffer:
									bindingRegistry = &usedBufferBindings;
									break;
								case BindableResourceType::StorageBuffer:
									bindingRegistry = &usedStorageBufferBindings;
									break;
								}

								String bindingValStr;
								if (compDef.Attributes.TryGetValue("Binding", bindingValStr))
								{
									int bindingVal = StringToInt(bindingValStr);

									ILObjectDefinition * otherComp = nullptr;
									if (bindingRegistry->TryGetValue(bindingVal, otherComp))
									{
										getSink()->diagnose(compDef.Position, Diagnostics::bindingAlreadyOccupied, bindingVal, otherComp->Name);
										getSink()->diagnose(otherComp->Position, Diagnostics::seeDefinitionOf, otherComp->Name);
									}
									if (bindingVal < 0 || bindingVal >= MaxBindingValue)
									{
										getSink()->diagnose(compDef.Position, Diagnostics::invalidBindingValue, bindingVal);
									}
									(*bindingRegistry)[bindingVal] = &compDef;
									compDef.Binding = bindingVal;
								}
							}
						}
					}
				}
				// second pass: assign bindings slots for rest of resource components
				for (auto & world : pipeline->Worlds)
				{
					if (world.Value.IsAbstract)
					{
						auto compiledWorld = compiledShader->Worlds[world.Key]();
						for (auto & compDefPair : compiledWorld->OutputType->Members)
						{
							auto & compDef = compDefPair.Value;
							auto bindableResType = compDef.Type->GetBindableResourceType();
							if (bindableResType != BindableResourceType::NonBindable)
							{
								Dictionary<int, ILObjectDefinition*> * bindingRegistry = nullptr;
								if (compDef.Binding != -1)
									continue;
								int * bindingAllocator = nullptr;
								switch (bindableResType)
								{
								case BindableResourceType::Texture:
									bindingRegistry = &usedTextureBindings;
									bindingAllocator = &textureBindingAllcator;
									break;
								case BindableResourceType::Sampler:
									bindingRegistry = &usedSamplerBindings;
									bindingAllocator = &samplerBindingAllocator;
									break;
								case BindableResourceType::Buffer:
									bindingRegistry = &usedBufferBindings;
									bindingAllocator = &bufferBindingAllocator;
									break;
								case BindableResourceType::StorageBuffer:
									bindingRegistry = &usedStorageBufferBindings;
									bindingAllocator = &storageBufferBindingAllocator;
									break;
								}
								while (bindingRegistry->ContainsKey(*bindingAllocator))
								{
									(*bindingAllocator)++;
								}
								compDef.Binding = *bindingAllocator;
								(*bindingAllocator)++;
								int maxBinding = GetMaxResourceBindings(bindableResType);
								if (compDef.Binding > maxBinding)
								{
									getSink()->diagnose(compDef.Position, Diagnostics::bindingExceedsLimit, compDef.Binding, compDef.Name);
								}
							}
						}
					}
				}

				// now we need to deal with import operators
				// create world input declarations base on input components
				for (auto & world : compiledShader->Worlds)
				{
					auto &components = worldComps[world.Key]();
					for (auto & comp : components)
					{
						if (comp->SyntaxNode->IsInput)
						{
							ILObjectDefinition def;
							def.Name = comp->UniqueName;
							def.Type = TranslateExpressionType(comp->Type.Ptr(), &genericTypeMappings);
							def.Position = comp->SyntaxNode->Position;
							def.Attributes = comp->SyntaxNode->LayoutAttributes;
							world.Value->Inputs.Add(def);
						}
					}
				}

				// fill in record types
				for (auto & comps : worldComps)
				{
					for (auto & comp : comps.Value)
					{
						// for each import operator call "import[w0->w1](x)", add x to w0's record type
						EnumerateImportExpressions(comp->SyntaxNode.Ptr(), [&](ImportExpressionSyntaxNode * importExpr)
						{
							auto recType = genericTypeMappings[importExpr->ImportOperatorDef->SourceWorld.Content]().As<ILRecordType>();
							ILObjectDefinition entryDef;
							entryDef.Attributes = comp->SyntaxNode->LayoutAttributes;
							entryDef.Name = importExpr->ComponentUniqueName;
							entryDef.Type = TranslateExpressionType(importExpr->Type.Ptr(), &genericTypeMappings);
							entryDef.Position = importExpr->Position;
							recType->Members.AddIfNotExists(importExpr->ComponentUniqueName, entryDef);
						});
						// if comp is output, add comp to its world's record type
						if (comp->SyntaxNode->IsOutput)
						{
							auto recType = genericTypeMappings[comp->World]().As<ILRecordType>();
							ILObjectDefinition entryDef;
							entryDef.Attributes = comp->SyntaxNode->LayoutAttributes;
							entryDef.Name = comp->UniqueName;
							entryDef.Type = TranslateExpressionType(comp->Type.Ptr(), &genericTypeMappings);
							entryDef.Position = comp->SyntaxNode->Position;
							recType->Members.AddIfNotExists(comp->UniqueName, entryDef);
						}
					}
				}

				// sort components by dependency
				for (auto & world : compiledShader->Worlds)
				{
					auto &components = worldComps[world.Key]();
					DependencySort(components, [](ComponentDefinitionIR * def)
					{
						return def->Dependency;
					});
				}
			
				// generate component functions
				for (auto & comp : shader->Definitions)
				{
					currentComponent = comp.Ptr();
					if (comp->SyntaxNode->Parameters.Count())
					{
						auto funcName = GetComponentFunctionName(comp->SyntaxNode.Ptr());
						if (result.Program->Functions.ContainsKey(funcName))
							continue;
						RefPtr<ILFunction> func = new ILFunction();
						RefPtr<FunctionSymbol> funcSym = new FunctionSymbol();
						func->Name = funcName;
						func->ReturnType = TranslateExpressionType(comp->Type, &genericTypeMappings);
						symTable->Functions[funcName] = funcSym;
						result.Program->Functions[funcName] = func;
						for (auto dep : comp->GetComponentFunctionDependencyClosure())
						{
							if (dep->SyntaxNode->Parameters.Count())
							{
								funcSym->ReferencedFunctions.Add(GetComponentFunctionName(dep->SyntaxNode.Ptr()));
							}
						}
						int id = 0;
						Dictionary<String, ILOperand*> refComponents;
						variables.PushScope();
						codeWriter.PushNode();
						for (auto & dep : comp->GetComponentFunctionDependencyClosure())
						{
							if (dep->SyntaxNode->Parameters.Count() == 0)
							{
								auto paramType = TranslateExpressionType(dep->Type, &genericTypeMappings);
								String paramName = EscapeDoubleUnderscore("p" + String(id) + "_" + dep->OriginalName); 
								func->Parameters.Add(paramName, ILParameter(paramType));
								auto argInstr = codeWriter.FetchArg(paramType, id + 1);
								argInstr->Name = paramName;
								variables.Add(dep->UniqueName, argInstr);
								id++;
							}
						}
						for (auto & param : comp->SyntaxNode->Parameters)
						{
							auto paramType = TranslateExpressionType(param->Type, &genericTypeMappings);
							String paramName = EscapeDoubleUnderscore("p" + String(id) + "_" + param->Name);
							func->Parameters.Add(paramName, ILParameter(paramType, param->Qualifier));
							auto argInstr = codeWriter.FetchArg(paramType, id + 1);
							argInstr->Name = paramName;
							variables.Add(param->Name, argInstr);
							id++;
						}
						if (comp->SyntaxNode->Expression)
						{
							comp->SyntaxNode->Expression->Accept(this);
							codeWriter.Insert(new ReturnInstruction(PopStack()));
						}
						else
						{
							comp->SyntaxNode->BlockStatement->Accept(this);
						}
						variables.PopScope();
						func->Code = codeWriter.PopNode();
					}
					currentComponent = nullptr;
				}
				
				for (auto & world : pipeline->Worlds)
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
						localComponents.Add(comp->UniqueName);

					DependencySort(components, [](ComponentDefinitionIR * def)
					{
						return def->Dependency;
					});

					for (auto & comp : components)
					{
						if (comp->SyntaxNode->Parameters.Count() == 0)
							VisitComponent(comp);
					}
					
					variables.PopScope();
					compiledWorld->Code = codeWriter.PopNode();
					EvalReferencedFunctionClosure(compiledWorld);
					currentWorld = nullptr;
				}
				currentShader = nullptr;
			}

			void EvalReferencedFunctionClosure(ILWorld * world)
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
			virtual RefPtr<ComponentSyntaxNode> VisitComponent(ComponentSyntaxNode *) override
			{
				throw NotImplementedException();
			}
			void VisitComponent(ComponentDefinitionIR * comp)
			{
				currentComponent = comp;
				String varName = EscapeDoubleUnderscore(currentComponent->OriginalName);
				RefPtr<ILType> type = TranslateExpressionType(currentComponent->Type, &genericTypeMappings);

				if (comp->SyntaxNode->IsInput)
				{
					auto loadInput = new LoadInputInstruction(type.Ptr(), comp->UniqueName);
					codeWriter.Insert(loadInput);
					variables.Add(currentComponent->UniqueName, loadInput);
					return;
				}

				ILOperand * componentVar = nullptr;
				
				if (currentComponent->SyntaxNode->Expression)
				{
					currentComponent->SyntaxNode->Expression->Accept(this);
					componentVar = exprStack.Last();
					if (currentWorld->OutputType->Members.ContainsKey(currentComponent->UniqueName))
					{
						auto exp = new ExportInstruction(currentComponent->UniqueName, currentWorld, componentVar);
						codeWriter.Insert(exp);
					}
					exprStack.Clear();
				}
				else if (currentComponent->SyntaxNode->BlockStatement)
				{
					returnRegister = nullptr;
					currentComponent->SyntaxNode->BlockStatement->Accept(this);
					componentVar = returnRegister;
				}

				/*if (!currentComponent->Type->IsTexture() && !currentComponent->Type->IsArray())
				{
					auto vartype = TranslateExpressionType(currentComponent->Type.Ptr(), &recordTypes);
					auto var = codeWriter.AllocVar(vartype, result.Program->ConstantPool->CreateConstant(1));
					var->Name = varName;
					codeWriter.Store(var, componentVar);
					componentVar = var;
				}
				else*/
					componentVar->Name = varName;
				currentWorld->Components[currentComponent->UniqueName] = componentVar;
				variables.Add(currentComponent->UniqueName, componentVar);
				currentComponent = nullptr;
			}
			virtual RefPtr<FunctionSyntaxNode> VisitFunction(FunctionSyntaxNode* function) override
			{
				if (function->IsExtern)
					return function;
				RefPtr<ILFunction> func = new ILFunction();
				result.Program->Functions.Add(function->InternalName, func);
				func->Name = function->InternalName;
				func->ReturnType = TranslateExpressionType(function->ReturnType);
				variables.PushScope();
				codeWriter.PushNode();
				int id = 0;
				for (auto &param : function->Parameters)
				{
					func->Parameters.Add(param->Name, ILParameter(TranslateExpressionType(param->Type), param->Qualifier));
					auto op = FetchArg(param->Type.Ptr(), ++id);
					op->Name = EscapeDoubleUnderscore(String("p_") + param->Name);
					variables.Add(param->Name, op);
				}
				function->Body->Accept(this);
				func->Code = codeWriter.PopNode();
				variables.PopScope();
				return function;
			}
			virtual RefPtr<StatementSyntaxNode> VisitBlockStatement(BlockStatementSyntaxNode* stmt) override
			{
				variables.PushScope();
				for (auto & subStmt : stmt->Statements)
					subStmt->Accept(this);
				variables.PopScope();
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitWhileStatement(WhileStatementSyntaxNode* stmt) override
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
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitDoWhileStatement(DoWhileStatementSyntaxNode* stmt) override
			{
				RefPtr<DoInstruction> instr = new DoInstruction();
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
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitForStatement(ForStatementSyntaxNode* stmt) override
			{
				RefPtr<ForInstruction> instr = new ForInstruction();
				variables.PushScope();
				if (stmt->TypeDef)
				{
					AllocVarInstruction * varOp = AllocVar(stmt->IterationVariableType.Ptr());
					varOp->Name = EscapeDoubleUnderscore(stmt->IterationVariable.Content);
					variables.Add(stmt->IterationVariable.Content, varOp);
				}
				ILOperand * iterVar = nullptr;
				if (stmt->IterationVariable.Content.Length() && !variables.TryGetValue(stmt->IterationVariable.Content, iterVar))
					throw InvalidProgramException("Iteration variable not found in variables dictionary. This should have been checked by semantics analyzer.");
				if (stmt->InitialExpression)
				{
					codeWriter.PushNode();
					stmt->InitialExpression->Accept(this);
					PopStack();
					instr->InitialCode = codeWriter.PopNode();
				}

				if (stmt->PredicateExpression)
				{
					codeWriter.PushNode();
					stmt->PredicateExpression->Accept(this);
					PopStack();
					instr->ConditionCode = codeWriter.PopNode();
				}
			
				if (stmt->SideEffectExpression)
				{
					codeWriter.PushNode();
					stmt->SideEffectExpression->Accept(this);
					PopStack();
					instr->SideEffectCode = codeWriter.PopNode();
				}

				codeWriter.PushNode();
				stmt->Statement->Accept(this);
				instr->BodyCode = codeWriter.PopNode();
				codeWriter.Insert(instr.Release());
				variables.PopScope();
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitIfStatement(IfStatementSyntaxNode* stmt) override
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
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitReturnStatement(ReturnStatementSyntaxNode* stmt) override
			{
				returnRegister = nullptr;
				if (currentWorld != nullptr && currentComponent != nullptr && !currentImport)
				{
					if (stmt->Expression)
					{
						stmt->Expression->Accept(this);
						returnRegister = PopStack();
						if (currentComponent->SyntaxNode->Parameters.Count() == 0)
						{
							if (currentWorld->OutputType->Members.ContainsKey(currentComponent->UniqueName))
							{
								auto exp = new ExportInstruction(currentComponent->UniqueName, currentWorld, returnRegister);
								codeWriter.Insert(exp);
							}
						}
						else
						{
							codeWriter.Insert(new ReturnInstruction(returnRegister));
						}
					}
				}
				else
				{
					if (stmt->Expression)
					{
						stmt->Expression->Accept(this);
						returnRegister = PopStack();
					}
					codeWriter.Insert(new ReturnInstruction(returnRegister));
				}
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitBreakStatement(BreakStatementSyntaxNode* stmt) override
			{
				codeWriter.Insert(new BreakInstruction());
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitContinueStatement(ContinueStatementSyntaxNode* stmt) override
			{
				codeWriter.Insert(new ContinueInstruction());
				return stmt;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitSelectExpression(SelectExpressionSyntaxNode * expr) override
			{
				expr->SelectorExpr->Accept(this);
				auto predOp = PopStack();
				expr->Expr0->Accept(this);
				auto v0 = PopStack();
				expr->Expr1->Accept(this);
				auto v1 = PopStack();
				PushStack(codeWriter.Select(predOp, v0, v1));
				return expr;
			}
			ILOperand * EnsureBoolType(ILOperand * op, RefPtr<ExpressionType> type)
			{
				if (!type->Equals(ExpressionType::Bool.Ptr()))
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
			virtual RefPtr<StatementSyntaxNode> VisitDiscardStatement(DiscardStatementSyntaxNode * stmt) override
			{
				codeWriter.Discard();
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitVarDeclrStatement(VarDeclrStatementSyntaxNode* stmt) override
			{
				for (auto & v : stmt->Variables)
				{
					AllocVarInstruction * varOp = AllocVar(stmt->Type.Ptr());
					varOp->Name = EscapeDoubleUnderscore(v->Name);
					variables.Add(v->Name, varOp);
					if (v->Expression)
					{
						v->Expression->Accept(this);
						Assign(varOp, PopStack());
					}
				}
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitExpressionStatement(ExpressionStatementSyntaxNode* stmt) override
			{
				stmt->Expression->Accept(this);
				PopStack();
				return stmt;
			}
			void Assign(ILOperand * left, ILOperand * right)
			{
				if (auto add = dynamic_cast<AddInstruction*>(left))
				{
					auto baseOp = add->Operands[0].Ptr();
					codeWriter.Update(baseOp, add->Operands[1].Ptr(), right);
					add->Erase();
				}
				else if (auto swizzle = dynamic_cast<SwizzleInstruction*>(left))
				{
					auto baseOp = swizzle->Operand.Ptr();
					int index = 0;
					for (int i = 0; i < swizzle->SwizzleString.Length(); i++)
					{
						switch (swizzle->SwizzleString[i])
						{
						case 'r':
						case 'x':
							index = 0;
							break;
						case 'g':
						case 'y':
							index = 1;
							break;
						case 'b':
						case 'z':
							index = 2;
							break;
						case 'a':
						case 'w':
							index = 3;
							break;
						}
						codeWriter.Update(baseOp, result.Program->ConstantPool->CreateConstant(index),
							codeWriter.Retrieve(right, result.Program->ConstantPool->CreateConstant(i)));
					}
					swizzle->Erase();
				}
				else
					codeWriter.Store(left, right);
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitBinaryExpression(BinaryExpressionSyntaxNode* expr) override
			{
				expr->RightExpression->Accept(this);
				auto right = PopStack();
				if (expr->Operator == Operator::Assign)
				{
					expr->LeftExpression->Access = ExpressionAccess::Write;
					expr->LeftExpression->Accept(this);
					auto left = PopStack();
					Assign(left, right);
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
					case Operator::AndAssign:
						rs = new BitAndInstruction();
						break;
					case Operator::BitOr:
					case Operator::OrAssign:
						rs = new BitOrInstruction();
						break;
					case Operator::BitXor:
					case Operator::XorAssign:
						rs = new BitXorInstruction();
						break;
					case Operator::Lsh:
					case Operator::LshAssign:
						rs = new ShlInstruction();
						break;
					case Operator::Rsh:
					case Operator::RshAssign:
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
						throw NotImplementedException("Code gen not implemented for this operator.");
					}
					rs->Operands.SetSize(2);
					rs->Operands[0] = left;
					rs->Operands[1] = right;
					rs->Type = TranslateExpressionType(expr->Type, &genericTypeMappings);
					codeWriter.Insert(rs);
					switch (expr->Operator)
					{
					case Operator::AddAssign:
					case Operator::SubAssign:
					case Operator::MulAssign:
					case Operator::DivAssign:
					case Operator::ModAssign:
					case Operator::LshAssign:
					case Operator::RshAssign:
					case Operator::AndAssign:
					case Operator::OrAssign:
					case Operator::XorAssign:
					{
						expr->LeftExpression->Access = ExpressionAccess::Write;
						expr->LeftExpression->Accept(this);
						auto target = PopStack();
						Assign(target, rs);
						break;
					}
					default:
						break;
					}
					PushStack(rs);
				}
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitProject(ProjectExpressionSyntaxNode * project) override
			{
				project->BaseExpression->Accept(this);
				auto rs = PopStack();
				auto proj = new ProjectInstruction();
				proj->ComponentName = currentImport->ComponentUniqueName;
				proj->Operand = rs;
				codeWriter.Insert(proj);
				PushStack(proj);
				return project;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitConstantExpression(ConstantExpressionSyntaxNode* expr) override
			{
				ILConstOperand * op;
				if (expr->ConstType == ConstantExpressionSyntaxNode::ConstantType::Float)
				{
					op = result.Program->ConstantPool->CreateConstant(expr->FloatValue);
				}
				else if (expr->ConstType == ConstantExpressionSyntaxNode::ConstantType::Bool)
				{
					op = result.Program->ConstantPool->CreateConstant(expr->IntValue != 0);
				}
				else
				{
					op = result.Program->ConstantPool->CreateConstant(expr->IntValue);
				}
				PushStack(op);
				return expr;
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
			virtual RefPtr<ExpressionSyntaxNode> VisitImportExpression(ImportExpressionSyntaxNode * expr) override
			{
				variables.PushScope();
				List<ILOperand*> arguments;
				for (int i = 0; i < expr->Arguments.Count(); i++)
				{
					expr->Arguments[i]->Accept(this);
					auto argOp = PopStack();
					arguments.Add(argOp);
					variables.Add(expr->ImportOperatorDef->Parameters[i]->Name, argOp);
				}
				currentImport = expr;
				auto oldTypeMapping = genericTypeMappings.TryGetValue(expr->ImportOperatorDef->TypeName.Content);
				auto componentType = TranslateExpressionType(expr->Type, &genericTypeMappings);
				genericTypeMappings[expr->ImportOperatorDef->TypeName.Content] = componentType;
				codeWriter.PushNode();
				expr->ImportOperatorDef->Body->Accept(this);
				currentImport = nullptr;
				auto impInstr = new ImportInstruction(expr->Arguments.Count());
				for (int i = 0; i < expr->Arguments.Count(); i++)
					impInstr->Arguments[i] = arguments[i];
				impInstr->ImportOperator = codeWriter.PopNode();
				variables.PopScope();
				if (oldTypeMapping)
					genericTypeMappings[expr->ImportOperatorDef->TypeName.Content] = *oldTypeMapping;
				else
					genericTypeMappings.Remove(expr->ImportOperatorDef->TypeName.Content);
				impInstr->ComponentName = expr->ComponentUniqueName;
				impInstr->Type = TranslateExpressionType(expr->Type, &genericTypeMappings);
				codeWriter.Insert(impInstr);
				PushStack(impInstr);
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitIndexExpression(IndexExpressionSyntaxNode* expr) override
			{
				expr->BaseExpression->Access = expr->Access;
				expr->BaseExpression->Accept(this);
				auto base = PopStack();
				expr->IndexExpression->Access = ExpressionAccess::Read;
				expr->IndexExpression->Accept(this);
				auto idx = PopStack();
				GenerateIndexExpression(base, idx,
					expr->Access == ExpressionAccess::Read);
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * expr) override
			{
				RefPtr<Object> refObj;
				if (expr->Tags.TryGetValue("ComponentReference", refObj))
				{
					if (auto refComp = refObj.As<StringObject>())
					{
						ILOperand * op;
						if (variables.TryGetValue(refComp->Content, op))
							PushStack(op);
						else
							throw InvalidProgramException("referencing undefined component/variable. probable cause: unchecked circular reference.");
					}
				}
				else
				{
					expr->BaseExpression->Access = expr->Access;
					expr->BaseExpression->Accept(this);
					auto base = PopStack();
					auto generateSingleMember = [&](char memberName)
					{
						int idx = 0;
						if (memberName == 'y' || memberName == 'g')
							idx = 1;
						else if (memberName == 'z' || memberName == 'b')
							idx = 2;
						else if (memberName == 'w' || memberName == 'a')
							idx = 3;

						GenerateIndexExpression(base, result.Program->ConstantPool->CreateConstant(idx),
							expr->Access == ExpressionAccess::Read);
					};
					if (expr->BaseExpression->Type->IsVectorType())
					{
						if (expr->MemberName.Length() == 1)
						{
							generateSingleMember(expr->MemberName[0]);
						}
						else
						{
							auto rs = new SwizzleInstruction();
							rs->Type = TranslateExpressionType(expr->Type.Ptr(), &genericTypeMappings);
							rs->SwizzleString = expr->MemberName;
							rs->Operand = base;
							codeWriter.Insert(rs);
							PushStack(rs);
						}
					}
					else if (expr->BaseExpression->Type->IsStruct())
					{
						int id = expr->BaseExpression->Type->AsBasicType()->Struct->SyntaxNode->FindField(expr->MemberName);
						GenerateIndexExpression(base, result.Program->ConstantPool->CreateConstant(id),
							expr->Access == ExpressionAccess::Read);
					}
					else
						throw NotImplementedException("member expression codegen");
				}
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitInvokeExpression(InvokeExpressionSyntaxNode* expr) override
			{
				List<ILOperand*> args;
				String funcName;
				bool hasSideEffect = false;
				if (auto basicType = expr->FunctionExpr->Type->AsBasicType())
				{
					if (basicType->Func)
					{
						funcName = basicType->Func->SyntaxNode->IsExtern ? basicType->Func->SyntaxNode->Name : basicType->Func->SyntaxNode->InternalName;
						for (auto & param : basicType->Func->SyntaxNode->Parameters)
						{
							if (param->Qualifier == ParameterQualifier::Out || param->Qualifier == ParameterQualifier::InOut)
							{
								hasSideEffect = true;
								break;
							}
						}
					}
					else if (basicType->Component)
					{
						auto funcCompName = expr->FunctionExpr->Tags["ComponentReference"]().As<StringObject>()->Content;
						auto funcComp = *(currentShader->DefinitionsByComponent[funcCompName]().TryGetValue(currentComponent->World));
						funcName = GetComponentFunctionName(funcComp->SyntaxNode.Ptr());
						for (auto & param : funcComp->SyntaxNode->Parameters)
						{
							if (param->Qualifier == ParameterQualifier::Out || param->Qualifier == ParameterQualifier::InOut)
							{
								hasSideEffect = true;
								break;
							}
						}
						// push additional arguments
						for (auto & dep : funcComp->GetComponentFunctionDependencyClosure())
						{
							if (dep->SyntaxNode->Parameters.Count() == 0)
							{
								ILOperand * op = nullptr;
								if (variables.TryGetValue(dep->UniqueName, op))
									args.Add(op);
								else
									throw InvalidProgramException("cannot resolve reference for implicit component function argument.");
							}
						}
					}
				}
				if (currentWorld)
				{
					currentWorld->ReferencedFunctions.Add(funcName);
				}
				for (auto arg : expr->Arguments)
				{
					arg->Accept(this);
					args.Add(PopStack());
				}
				auto instr = new CallInstruction(args.Count());
				instr->SideEffect = hasSideEffect;
				instr->Function = funcName;
				for (int i = 0; i < args.Count(); i++)
					instr->Arguments[i] = args[i];
				instr->Type = TranslateExpressionType(expr->Type, &genericTypeMappings);
				codeWriter.Insert(instr);
				PushStack(instr);
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitTypeCastExpression(TypeCastExpressionSyntaxNode * expr) override
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
					getSink()->diagnose(expr, Diagnostics::invalidTypeCast, expr->Expression->Type, expr->Type);
				}
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitUnaryExpression(UnaryExpressionSyntaxNode* expr) override
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
					instr->Type = TranslateExpressionType(expr->Type, &genericTypeMappings);
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
					instr->Type = TranslateExpressionType(expr->Type, &genericTypeMappings);
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
							throw NotImplementedException("Code gen is not implemented for this operator.");
						}
						rs->Operand = input;
						rs->Type = input->Type;
						codeWriter.Insert(rs);
						return rs;
					};
					PushStack(genUnaryInstr(base));
				}
				return expr;
			}
			bool GenerateVarRef(String name, ExpressionAccess access)
			{
				ILOperand * var = 0;
				String srcName = name;
				if (!variables.TryGetValue(srcName, var))
				{
					return false;
				}
				if (access == ExpressionAccess::Read)
				{
					PushStack(var);
				}
				else
				{
					PushStack(var);
				}
				return true;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode* expr) override
			{
				RefPtr<Object> refObj;
				if (expr->Tags.TryGetValue("ComponentReference", refObj))
				{
					if (auto refComp = refObj.As<StringObject>())
					{
						ILOperand * op;
						if (variables.TryGetValue(refComp->Content, op))
							PushStack(op);
						else
							throw InvalidProgramException(String("referencing undefined component/variable '") + refComp->Content + "'. probable cause: unchecked circular reference.");
					}
				}
				else if (!GenerateVarRef(expr->Variable, expr->Access))
				{
					throw InvalidProgramException("identifier is neither a variable nor a recognized component.");
				}
				return expr;
			}
		private:
			CodeGenerator & operator = (const CodeGenerator & other) = delete;
		public:
			CodeGenerator(SymbolTable * symbols, DiagnosticSink * pErr, CompileResult & _result)
				: ICodeGenerator(pErr), symTable(symbols), result(_result)
			{
				result.Program = new ILProgram();
				codeWriter.SetConstantPool(result.Program->ConstantPool.Ptr());
			}
		};

		ICodeGenerator * CreateCodeGenerator(SymbolTable * symbols, CompileResult & result)
		{
			return new CodeGenerator(symbols, result.GetErrorWriter(), result);
		}
	}
}
