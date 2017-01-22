#include "SyntaxVisitors.h"

#include <assert.h>

namespace Spire
{
	namespace Compiler
	{
		bool IsNumeric(BaseType t)
		{
			return t == BaseType::Int || t == BaseType::Float || t == BaseType::UInt;
		}

		String GetFullComponentName(ComponentSyntaxNode * comp)
		{
			StringBuilder sb;
			sb << comp->Name.Content;
			for (auto & param : comp->GetParameters())
			{
				sb << "@" << param->Type.type->ToString();
			}
			return sb.ProduceString();
		}

		String TranslateHLSLTypeNames(String name)
		{
			if (name == "float2" || name == "half2")
				return "vec2";
			else if (name == "float3" || name == "half3")
				return "vec3";
			else if (name == "float4" || name == "half4")
				return "vec4";
			else if (name == "half")
				return "float";
			else if (name == "int2")
				return "ivec2";
			else if (name == "int3")
				return "ivec3";
			else if (name == "int4")
				return "ivec4";
			else if (name == "uint2")
				return "uvec2";
			else if (name == "uint3")
				return "uvec3";
			else if (name == "uint4")
				return "uvec4";
			else if (name == "float3x3" || name == "half3x3")
				return "mat3";
			else if (name == "float4x4" || name == "half4x4")
				return "mat4";
			else
				return name;
		}

		class SemanticsVisitor : public SyntaxVisitor
		{
			ProgramSyntaxNode * program = nullptr;
			FunctionSyntaxNode * function = nullptr;
			FunctionSymbol * currentFunc = nullptr;
			ShaderSymbol * currentShader = nullptr;
			PipelineSymbol * currentPipeline = nullptr;
			ImportOperatorDefSyntaxNode * currentImportOperator = nullptr;
			ShaderComponentSymbol * currentComp = nullptr;
			ComponentSyntaxNode * currentCompNode = nullptr;
			List<SyntaxNode *> loops;
			SymbolTable * symbolTable;
		public:
			SemanticsVisitor(SymbolTable * symbols, DiagnosticSink * pErr)
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
		public:
			// Translate Types
			RefPtr<ExpressionType> typeResult;
			RefPtr<ExpressionType> TranslateTypeNode(const RefPtr<ExpressionSyntaxNode> & node)
			{
				typeResult = ExpressionType::Error;
				node->Accept(this);
				assert(typeResult);
				return typeResult;
			}
			TypeExp TranslateTypeNode(TypeExp const& typeExp)
			{
				TypeExp result;
				result.exp = typeExp.exp;
				result.type = TranslateTypeNode(typeExp.exp);
				return result;
			}

			RefPtr<ExpressionType> ExtractGenericArgType(RefPtr<ExpressionSyntaxNode> exp)
			{
				if (auto typeType = exp->Type.type->As<TypeExpressionType>())
				{
					return typeType->type;
				}
				else if (exp->Type.type->Equals(ExpressionType::Error))
				{
					return exp->Type;
				}
				else
				{
					throw "unimplemented";
				}
			}

			int ExtractGenericArgInteger(RefPtr<ExpressionSyntaxNode> exp)
			{
				return CheckIntegerConstantExpression(exp.Ptr());
			}

			RefPtr<ExpressionSyntaxNode> VisitGenericType(GenericTypeSyntaxNode * typeNode) override
			{
				auto& args = typeNode->Args;
				for (auto& arg : typeNode->Args)
				{
					arg = arg->Accept(this).As<ExpressionSyntaxNode>();
				}

				// Certain generic types have baked-in support here
				if (typeNode->GenericTypeName == "PackedBuffer" ||
					typeNode->GenericTypeName == "StructuredBuffer" ||
					typeNode->GenericTypeName == "RWStructuredBuffer" ||
					typeNode->GenericTypeName == "Uniform" ||
					typeNode->GenericTypeName == "Patch" ||
					typeNode->GenericTypeName == "PackedBuffer")
				{
					RefPtr<GenericExpressionType> rs = new GenericExpressionType();
					rs->BaseType = typeResult;
					rs->GenericTypeName = typeNode->GenericTypeName;
					typeResult = rs;
					return typeNode;
				}
				else
				{
					auto decl = typeNode->Scope->LookUp(typeNode->GenericTypeName);
					if (!decl)
					{
						getSink()->diagnose(typeNode, Diagnostics::undefinedIdentifier, typeNode->GenericTypeName);
						typeResult = ExpressionType::Error;
						return typeNode;
					}

					if (auto genericDecl = dynamic_cast<GenericDecl*>(decl))
					{
						int argCount = typeNode->Args.Count();
						int argIndex = 0;
						for (RefPtr<Decl> member : genericDecl->Members)
						{
							if (auto typeParam = member.As<GenericTypeParamDecl>())
							{
								if (argIndex == argCount)
								{
									// Too few arguments!

								}

								// TODO: checking!
							}
							else if (auto valParam = member.As<GenericValueParamDecl>())
							{
								// TODO: checking
							}
							else
							{

							}
						}
						if (argIndex != argCount)
						{
							// Too many arguments!
						}

						// Now instantiate the declaration given those arguments
						if (auto magicTypeDecl = genericDecl->inner.As<MagicTypeDecl>())
						{
							if (magicTypeDecl->tag == "Vector")
							{
								auto vecType = new VectorExpressionType(
									ExtractGenericArgType(args[0]),
									ExtractGenericArgInteger(args[1]));
								typeResult = vecType;
								typeNode->Type = new TypeExpressionType(vecType);
								return typeNode;
							}
							else if (magicTypeDecl->tag == "Matrix")
							{
								auto vecType = new MatrixExpressionType(
									ExtractGenericArgType(args[0]),
									ExtractGenericArgInteger(args[1]),
									ExtractGenericArgInteger(args[2]));
								typeResult = vecType;
								typeNode->Type = new TypeExpressionType(vecType);
								return typeNode;
							}
						}

						// catch-all for cases that we don't handle
						throw "unimplemented";
					}
					else
					{
						// TODO: correct diagnostic here!
						getSink()->diagnose(typeNode, Diagnostics::undefinedIdentifier, typeNode->GenericTypeName);
						typeResult = ExpressionType::Error;
						return typeNode;
					}
				}
			}
		public:
			RefPtr<PipelineSyntaxNode> VisitPipeline(PipelineSyntaxNode * pipeline) override
			{
				RefPtr<PipelineSymbol> psymbol = new PipelineSymbol();
				psymbol->SyntaxNode = pipeline;
				if (pipeline->ParentPipelineName.Content.Length())
				{
					RefPtr<PipelineSymbol> parentPipeline;
					if (symbolTable->Pipelines.TryGetValue(pipeline->ParentPipelineName.Content, parentPipeline))
					{
						psymbol->ParentPipeline = parentPipeline.Ptr();
					}
					else
					{
						getSink()->diagnose(pipeline->ParentPipelineName, Diagnostics::undefinedPipelineName, pipeline->ParentPipelineName.Content);
					}
				}
				currentPipeline = psymbol.Ptr();
				symbolTable->Pipelines.Add(pipeline->Name.Content, psymbol);
				for (auto world : pipeline->GetWorlds())
				{
					if (!psymbol->Worlds.ContainsKey(world->Name.Content))
					{
						psymbol->Worlds.Add(world->Name.Content, world.Ptr());
						psymbol->WorldDependency.Add(world->Name.Content, EnumerableHashSet<String>());
					}
					else
					{
						getSink()->diagnose(world.Ptr(), Diagnostics::worldNameAlreadyDefined, world->Name.Content);
					}
				}
				for (auto comp : pipeline->GetAbstractComponents())
				{
					comp->Type = TranslateTypeNode(comp->Type);
					if (comp->IsRequire() || comp->IsInput() || (comp->Rate && comp->Rate->Worlds.Count() == 1
						&& psymbol->IsAbstractWorld(comp->Rate->Worlds.First().World.Content)))
						AddNewComponentSymbol(psymbol->Components, psymbol->FunctionComponents, comp);
					else
					{
						getSink()->diagnose(comp.Ptr(), Diagnostics::cannotDefineComponentsInAPipeline);
					}
				}
				for (auto & op : pipeline->GetImportOperators())
				{
					psymbol->AddImportOperator(op);
				}
				// add initial world dependency edges
				for (auto op : pipeline->GetImportOperators())
				{
					if (!psymbol->WorldDependency.ContainsKey(op->DestWorld.Content))
						getSink()->diagnose(op->DestWorld, Diagnostics::undefinedWorldName, op->DestWorld.Content);
					else
					{
						if (psymbol->Worlds[op->DestWorld.Content].GetValue()->IsAbstract())
							getSink()->diagnose(op->DestWorld, Diagnostics::abstractWorldAsTargetOfImport);
						else if (!psymbol->WorldDependency.ContainsKey(op->SourceWorld.Content))
							getSink()->diagnose(op->SourceWorld, Diagnostics::undefinedWorldName2, op->SourceWorld.Content);
						else
						{
							if (IsWorldDependent(psymbol.Ptr(), op->SourceWorld.Content, op->DestWorld.Content))
							{
								getSink()->diagnose(op->Name, Diagnostics::importOperatorCircularity, op->Name.Content, op->SourceWorld.Content, op->DestWorld.Content);
							}
							else
							{
								psymbol->WorldDependency[op->DestWorld.Content].GetValue().Add(op->SourceWorld.Content);
							}
						}
					}

				}
				// propagate world dependency graph
				bool changed = true;
				while (changed)
				{
					changed = false;
					for (auto world : pipeline->GetWorlds())
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

				for (auto & op : pipeline->GetImportOperators())
				{
					currentImportOperator = op.Ptr();
					HashSet<String> paraNames;
					for (auto & para : op->GetParameters())
					{
						if (paraNames.Contains(para->Name.Content))
							getSink()->diagnose(para.Ptr(), Diagnostics::parameterAlreadyDefined, para->Name);
						else
							paraNames.Add(para->Name.Content);
						para->Type = TranslateTypeNode(para->Type);
						if (para->Type.Equals(ExpressionType::Void.Ptr()))
						{
							getSink()->diagnose(para.Ptr(), Diagnostics::parameterCannotBeVoid);
						}
					}
					auto oldSymFuncs = symbolTable->Functions;
					auto oldSymFuncOverloads = symbolTable->FunctionOverloads;
					for (auto req : op->Requirements)
					{
						VisitFunctionDeclaration(req.Ptr());
					}
					op->Body->Accept(this);
					symbolTable->Functions = oldSymFuncs;
					symbolTable->FunctionOverloads = oldSymFuncOverloads;
					currentImportOperator = nullptr;
				}
				currentPipeline = nullptr;
				return pipeline;
			}

			virtual CoreLib::RefPtr<InterfaceSyntaxNode> VisitInterface(InterfaceSyntaxNode * interfaceNode) override
			{
				for (auto & comp : interfaceNode->GetComponents())
				{
					interfaceNode->Scope->decls.AddIfNotExists(comp->Name.Content, comp.Ptr());
					for (auto & param : comp->GetParameters())
					{
						param->Type = TranslateTypeNode(param->Type);
						if (param->Expr)
							getSink()->diagnose(param->Expr->Position, Diagnostics::defaultParamNotAllowedInInterface, param->Name);
					}
					comp->Type = TranslateTypeNode(comp->Type);
					if (comp->Expression)
						comp->Expression->Accept(this);
					if (comp->BlockStatement)
						comp->BlockStatement->Accept(this);
				}
				return interfaceNode;
			}

			virtual RefPtr<ImportSyntaxNode> VisitImport(ImportSyntaxNode * import) override
			{
				RefPtr<ShaderSymbol> refShader;
				symbolTable->Shaders.TryGetValue(import->ShaderName.Content, refShader);
				if (refShader)
				{
					// type check
					List<ShaderComponentSymbol*> paramList;
					for (auto & comp : refShader->Components)
						if (comp.Value->IsRequire())
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
								getSink()->diagnose(arg->Expression.Ptr(), Diagnostics::positionArgumentAfterNamed);
								break;
							}
							if (position >= paramList.Count())
							{
								getSink()->diagnose(arg->Expression.Ptr(), Diagnostics::tooManyArguments);
								break;
							}
							arg->ArgumentName.Content = paramList[position]->Name;
							arg->ArgumentName.Position = arg->Position;
						}
						position++;
						RefPtr<ShaderComponentSymbol> refComp, argComp;
						if (auto funcs = refShader->FunctionComponents.TryGetValue(arg->ArgumentName.Content))
							if (funcs->First()->IsRequire())
								refComp = funcs->First();
						if (!refComp)
							refShader->Components.TryGetValue(arg->ArgumentName.Content, refComp);

						if (refComp)
						{
							if (refComp->Implementations.First()->SyntaxNode->IsComponentFunction()) // this is a function parameter
							{
								arg->ArgumentName.Content = refComp->Name;
								// construct an invocation node to resolve overloaded component function
								RefPtr<InvokeExpressionSyntaxNode> tempInvoke = new InvokeExpressionSyntaxNode();
								tempInvoke->Position = arg->Position;
								tempInvoke->Scope = arg->Scope;
								tempInvoke->FunctionExpr = arg->Expression;
								for (auto & param : refComp->Implementations.First()->SyntaxNode->GetParameters())
								{
									RefPtr<VarExpressionSyntaxNode> tempArg = new VarExpressionSyntaxNode();
									tempArg->Type = param->Type;
									tempInvoke->Arguments.Add(tempArg);
								}
								auto resolvedExpr = ResolveInvoke(tempInvoke.Ptr());
								if (auto resolveInvoke = resolvedExpr.As<InvokeExpressionSyntaxNode>())
								{
									if (auto funcType = resolveInvoke->FunctionExpr->Type->As<FuncType>())
									{
										if (auto funcComponent = funcType->Component)
										{
											// modify function name to resolved name
											if (auto memberExpr = arg->Expression.As<MemberExpressionSyntaxNode>())
												memberExpr->MemberName = funcComponent->Name;
											else if (auto varExpr = arg->Expression.As<VarExpressionSyntaxNode>())
												varExpr->Variable = funcComponent->Name;
										}
										else
											getSink()->diagnose(arg.Ptr(), Diagnostics::ordinaryFunctionAsModuleArgument);

									}
								}
								else
									getSink()->diagnose(arg.Ptr(), Diagnostics::invalidValueForArgument, arg->ArgumentName.Content);
							}
							else
							{
								arg->Accept(this);
								if (!refComp->Type->DataType->Equals(arg->Expression->Type.Ptr()))
								{
									getSink()->diagnose(arg->Expression.Ptr(), Diagnostics::argumentTypeDoesNotMatchParameterType, arg->Expression->Type, refComp->Type->DataType);
								}
								if (!refComp->IsRequire())
									getSink()->diagnose(arg->ArgumentName, Diagnostics::nameIsNotAParameterOfCallee, arg->ArgumentName.Content, import->ShaderName.Content);
							}
						}
						else
							getSink()->diagnose(arg->ArgumentName, Diagnostics::nameIsNotAParameterOfCallee, arg->ArgumentName.Content, import->ShaderName.Content);
					}
				}
				return import;
			}

			class ShaderImportVisitor : public SyntaxVisitor
			{
			private:
				SymbolTable * symbolTable = nullptr;
				ShaderSymbol * currentShader = nullptr;
				ShaderComponentSymbol * currentComp = nullptr;
			public:
				ShaderImportVisitor(DiagnosticSink * writer, SymbolTable * symTable)
					: SyntaxVisitor(writer), symbolTable(symTable)
				{}
				virtual RefPtr<ShaderSyntaxNode> VisitShader(ShaderSyntaxNode * shader) override
				{
					currentShader = symbolTable->Shaders[shader->Name.Content].GetValue().Ptr();
					SyntaxVisitor::VisitShader(shader);
					currentShader = nullptr;
					return shader;
				}
				virtual RefPtr<ComponentSyntaxNode> VisitComponent(ComponentSyntaxNode * comp) override
				{
					RefPtr<ShaderComponentSymbol> compSym;

					currentShader->Components.TryGetValue(comp->Name.Content, compSym);
					currentComp = compSym.Ptr();
					SyntaxVisitor::VisitComponent(comp);
					if (comp->Expression || comp->BlockStatement)
					{
						if (comp->IsRequire())
							getSink()->diagnose(comp, Diagnostics::requireWithComputation);
						if (comp->IsParam())
							getSink()->diagnose(comp, Diagnostics::paramWithComputation);
					}
					if (compSym->Type->DataType->GetBindableResourceType() != BindableResourceType::NonBindable && !comp->IsParam()
						&& !comp->IsRequire() && !dynamic_cast<InterfaceSyntaxNode*>(comp->ParentDecl))
						getSink()->diagnose(comp, Diagnostics::resourceTypeMustBeParamOrRequire, comp->Name);
					if (compSym->Type->DataType->GetBindableResourceType() != BindableResourceType::NonBindable &&
						(comp->Expression || comp->BlockStatement))
						getSink()->diagnose(comp, Diagnostics::cannotDefineComputationOnResourceType, comp->Name);

					if (comp->HasSimpleAttribute("FragDepth"))
					{
						if (!comp->IsOutput())
							getSink()->diagnose(comp, Diagnostics::fragDepthAttributeCanOnlyApplyToOutput);
						if (!comp->Type.Equals(ExpressionType::Float))
							getSink()->diagnose(comp, Diagnostics::fragDepthAttributeCanOnlyApplyToFloatComponent);
					}
					currentComp = nullptr;
					return comp;
				}
				virtual RefPtr<ImportSyntaxNode> VisitImport(ImportSyntaxNode * import) override
				{
					RefPtr<ShaderSymbol> refShader;
					symbolTable->Shaders.TryGetValue(import->ShaderName.Content, refShader);
					if (!refShader)
						getSink()->diagnose(import->ShaderName, Diagnostics::undefinedIdentifier, import->ShaderName.Content);
					currentShader->DependentShaders.Add(refShader.Ptr());
					if (!currentComp)
					{
						ShaderUsing su;
						su.Shader = refShader.Ptr();
						su.IsPublic = import->IsPublic();
						if (import->IsInplace)
						{
							currentShader->ShaderUsings.Add(su);
						}
						else
						{
							if (currentShader->ShaderObjects.ContainsKey(import->ObjectName.Content) ||
								currentShader->Components.ContainsKey(import->ObjectName.Content))
							{
								getSink()->diagnose(import->ShaderName, Diagnostics::nameAlreadyDefined, import->ShaderName);
							}
							currentShader->ShaderObjects[import->ObjectName.Content] = su;
						}
					}
					if (currentComp)
						getSink()->diagnose(import->ShaderName, Diagnostics::usingInComponentDefinition);
					return import;
				}
			};

			void CheckShaderInterfaceRequirements(ShaderSymbol * shaderSym)
			{
				for (auto interfaceName : shaderSym->SyntaxNode->InterfaceNames)
				{
					auto interfaceNode = dynamic_cast<InterfaceSyntaxNode*>(symbolTable->LookUp(interfaceName.Content));
					if (!interfaceNode)
					{
						getSink()->diagnose(interfaceName.Position, Diagnostics::undefinedIdentifier, interfaceName);
						continue;
					}
					for (auto comp : interfaceNode->GetComponents())
					{
						auto compRef = shaderSym->ResolveComponentReference(GetFullComponentName(comp.Ptr()));
						if (compRef.IsAccessible)
						{
							if (!compRef.Component->Implementations.First()->SyntaxNode->IsPublic())
							{
								getSink()->diagnose(compRef.Component->Implementations.First()->SyntaxNode->Position, Diagnostics::interfaceImplMustBePublic, comp->Name.Content, interfaceName);
								getSink()->diagnose(comp->Position, Diagnostics::seeInterfaceDefinitionOf, comp->Name);
							}
							if (!compRef.Component->Type->DataType->Equals(comp->Type.Ptr()))
							{
								getSink()->diagnose(compRef.Component->Implementations.First()->SyntaxNode->Position, Diagnostics::componentTypeDoesNotMatchInterface, comp->Name, interfaceName);
								getSink()->diagnose(comp->Position, Diagnostics::seeInterfaceDefinitionOf, comp->Name);
							}
						}
						else
						{
							if (!comp->Expression && !comp->BlockStatement) // interface does not define default impl, and shader does not provide impl
							{
								getSink()->diagnose(shaderSym->SyntaxNode->Position, Diagnostics::shaderDidNotDefineComponent, shaderSym->SyntaxNode->Name, comp->Name.Content, interfaceName);
								getSink()->diagnose(comp->Position, Diagnostics::seeInterfaceDefinitionOf, comp->Name);
								if (compRef.Component)
									getSink()->diagnose(compRef.Component->Implementations.First()->SyntaxNode->Position, Diagnostics::doYouForgetToMakeComponentAccessible, comp->Name,
										shaderSym->SyntaxNode->Name);
							}
							else // if interface provides default impl, add it to shader
							{
								CloneContext ctx;
								auto newComp = comp->Clone(ctx);
								shaderSym->SyntaxNode->Members.Add(newComp);
								newComp->modifiers.flags |= Public;
								AddNewComponentSymbol(shaderSym->Components, shaderSym->FunctionComponents, newComp);
							}
						}
					}
				}
			}

			// pass 1: fill components in shader symbol table
			void VisitShaderPass1(ShaderSyntaxNode * shader)
			{
				HashSet<String> inheritanceSet;
				auto curShader = shader;
				inheritanceSet.Add(curShader->Name.Content);
				auto & shaderSymbol = symbolTable->Shaders[curShader->Name.Content].GetValue();
				this->currentShader = shaderSymbol.Ptr();

				if (shader->ParentPipelineName.Content.Length() == 0) // implicit pipeline
				{
					if (program->GetPipelines().Count() == 1)
					{
						shader->ParentPipelineName = shader->Name; // get line and col from shader name
						shader->ParentPipelineName.Content = program->GetPipelines().First()->Name.Content;
					}
					else if (!shader->IsModule)
					{
						// current compilation context has more than one pipeline defined,
						// in which case we do not allow implicit pipeline specification
						getSink()->diagnose(curShader->Name, Diagnostics::explicitPipelineSpecificationRequiredForShader, shader->Name.Content);
					}
				}

				auto pipelineName = shader->ParentPipelineName.Content;
				if (pipelineName.Length())
				{
					auto pipeline = symbolTable->Pipelines.TryGetValue(pipelineName);
					if (pipeline)
						shaderSymbol->ParentPipeline = pipeline->Ptr();
					else
					{
						getSink()->diagnose(shader->ParentPipelineName, Diagnostics::undefinedPipelineName, pipelineName);
						throw 0;
					}
				}

				if (shader->IsModule)
					shaderSymbol->IsAbstract = true;
				// add components to symbol table
				for (auto & mbr : shader->Members)
				{
					if (auto comp = dynamic_cast<ComponentSyntaxNode*>(mbr.Ptr()))
					{
						comp->Type = TranslateTypeNode(comp->Type);
						if (comp->IsRequire())
						{
							shaderSymbol->IsAbstract = true;
							if (!shaderSymbol->SyntaxNode->IsModule)
							{
								getSink()->diagnose(shaderSymbol->SyntaxNode, Diagnostics::parametersOnlyAllowedInModules);
							}
						}
						for (auto & param : comp->GetParameters())
							param->Type = TranslateTypeNode(param->Type);
						AddNewComponentSymbol(shaderSymbol->Components, shaderSymbol->FunctionComponents, comp);
					}
				}
				// add shader objects to symbol table
				ShaderImportVisitor importVisitor(sink, symbolTable);
				shader->Accept(&importVisitor);

				this->currentShader = nullptr;
			}
			// pass 2: type checking component definitions
			void VisitShaderPass2(ShaderSyntaxNode * shaderNode)
			{
				RefPtr<ShaderSymbol> shaderSym;
				if (!symbolTable->Shaders.TryGetValue(shaderNode->Name.Content, shaderSym))
					return;
				CheckShaderInterfaceRequirements(shaderSym.Ptr());
				this->currentShader = shaderSym.Ptr();
				for (auto & comp : shaderNode->Members)
				{
					comp->Accept(this);
				}
				this->currentShader = nullptr;
			}

			bool MatchType_GenericType(String typeName, ExpressionType * valueType)
			{
				if (auto basicType = valueType->AsBasicType())
					return basicType->GenericTypeVar == typeName;
				return false;
			}

			bool MatchType_ValueReceiver(ExpressionType * receiverType, ExpressionType * valueType)
			{
				if (receiverType->Equals(valueType))
					return true;
				if (receiverType->IsIntegral() && valueType->Equals(ExpressionType::Int.Ptr()))
					return true;
				if (receiverType->Equals(ExpressionType::Float.Ptr()) && valueType->IsIntegral())
					return true;
				if (receiverType->IsVectorType() && valueType->IsVectorType())
				{
					auto recieverVecType = receiverType->AsVectorType();
					auto valueVecType = valueType->AsVectorType();
					if (GetVectorBaseType(recieverVecType) == BaseType::Float &&
						GetVectorSize(recieverVecType) == GetVectorSize(valueVecType))
						return true;
					if (GetVectorBaseType(recieverVecType) == BaseType::UInt &&
						GetVectorBaseType(valueVecType) == BaseType::Int &&
						GetVectorSize(recieverVecType) == GetVectorSize(valueVecType))
						return true;
				}
				return false;
			}
			virtual RefPtr<ComponentSyntaxNode> VisitComponent(ComponentSyntaxNode * comp) override
			{
				this->currentCompNode = comp;
				RefPtr<ShaderComponentSymbol> compSym;
				currentShader->Components.TryGetValue(comp->Name.Content, compSym);
				this->currentComp = compSym.Ptr();
				if (auto specialize = comp->FindSpecializeModifier())
				{
					if (!comp->IsParam())
						getSink()->diagnose(comp->Position, Diagnostics::specializeCanOnlyBeUsedOnParam);
					if (!compSym->Type->DataType->Equals(ExpressionType::Int) && !compSym->Type->DataType->Equals(ExpressionType::Bool)
						&& !compSym->Type->DataType->Equals(ExpressionType::UInt))
						getSink()->diagnose(comp->Position, Diagnostics::specializedParameterMustBeInt);
					for (auto & val : specialize->Values)
					{
						if (!dynamic_cast<ConstantExpressionSyntaxNode*>(val.Ptr()))
						{
							getSink()->diagnose(val->Position, Diagnostics::specializationValuesMustBeConstantLiterial);
						}
						val->Accept(this);
						if (!val->Type->Equals(compSym->Type->DataType))
							getSink()->diagnose(val->Position, Diagnostics::typeMismatch, val->Type, currentComp->Type);
					}
				}
				for (auto & param : comp->GetParameters())
				{
					param->Accept(this);
				}
				if (comp->Expression)
				{
					comp->Expression = comp->Expression->Accept(this).As<ExpressionSyntaxNode>();
					if (!MatchType_ValueReceiver(compSym->Type->DataType.Ptr(), comp->Expression->Type.Ptr()) &&
						!comp->Expression->Type->Equals(ExpressionType::Error.Ptr()))
						getSink()->diagnose(comp->Name, Diagnostics::typeMismatch, comp->Expression->Type, currentComp->Type);
				}
				if (comp->BlockStatement)
					comp->BlockStatement->Accept(this);

				this->currentComp = nullptr;
				this->currentCompNode = nullptr;
				return comp;
			}
			virtual RefPtr<StatementSyntaxNode> VisitImportStatement(ImportStatementSyntaxNode * importStmt) override
			{
				importStmt->Import->Accept(this);
				return importStmt;
			}
			void AddNewComponentSymbol(EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>> & components,
				EnumerableDictionary<String, List<RefPtr<ShaderComponentSymbol>>> & funcComponents,
				RefPtr<ComponentSyntaxNode> comp)
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
				if (compImpl->SyntaxNode->IsOutput())
				{
					if (compImpl->SyntaxNode->Rate)
					{
						for (auto & w : compImpl->SyntaxNode->Rate->Worlds)
							compImpl->ExportWorlds.Add(w.World.Content);
					}
					else
					{
						getSink()->diagnose(compImpl->SyntaxNode.Ptr(), Diagnostics::componentMarkedExportMustHaveWorld, compImpl->SyntaxNode->Name);
					}
					if (compImpl->SyntaxNode->IsComponentFunction())
						getSink()->diagnose(compImpl->SyntaxNode->Name, Diagnostics::componetMarkedExportCannotHaveParameters, compImpl->SyntaxNode->Name);
				}
				auto compName = GetFullComponentName(comp.Ptr());
				if (!components.TryGetValue(compName, compSym))
				{
					compSym = new ShaderComponentSymbol();
					compSym->Type = new Type();
					compSym->Name = compName;
					compSym->Type->DataType = comp->Type;
					components.Add(compName, compSym);
				}
				else
				{
					if (comp->IsRequire())
						getSink()->diagnose(compImpl->SyntaxNode.Ptr(), Diagnostics::requirementsClashWithPreviousDef, compImpl->SyntaxNode->Name.Content);
					else
					{
						if (!compSym->Type->DataType->Equals(comp->Type.Ptr()))
						{
							getSink()->diagnose(comp->Name, Diagnostics::componentOverloadTypeMismatch, comp->Name.Content);
							getSink()->diagnose(compSym->Implementations.First()->SyntaxNode, Diagnostics::seePreviousDefinition);
						}
					}
					if (compImpl->SyntaxNode->IsComponentFunction())
					{
						getSink()->diagnose(compImpl->SyntaxNode.Ptr(), Diagnostics::functionRedefinition, compImpl->SyntaxNode->Name.Content);
						getSink()->diagnose(compSym->Implementations.Last()->SyntaxNode, Diagnostics::seePreviousDefinition);
					}
					symbolTable->CheckComponentImplementationConsistency(sink, compSym.Ptr(), compImpl.Ptr());
				}
				if (compImpl->SyntaxNode->IsComponentFunction())
				{
					auto list = funcComponents.TryGetValue(comp->Name.Content);
					if (!list)
					{
						funcComponents[comp->Name.Content] = List<RefPtr<ShaderComponentSymbol>>();
						list = funcComponents.TryGetValue(comp->Name.Content);
					}
					comp->Name.Content = compName;
					list->Add(compSym);
				}
				compSym->Implementations.Add(compImpl);
			}
			virtual RefPtr<ProgramSyntaxNode> VisitProgram(ProgramSyntaxNode * programNode) override
			{
				HashSet<String> funcNames;
				this->program = programNode;
				this->function = nullptr;
				for (auto & s : program->Members)
				{
					symbolTable->globalDecls.AddIfNotExists(s->Name.Content, s.Ptr());
				}
				for (auto & s : program->GetTypeDefs())
					VisitTypeDefDecl(s.Ptr());
				for (auto & s : program->GetStructs())
				{
					if (!s->SemanticallyChecked)
					{
						VisitStruct(s.Ptr());
						s->SemanticallyChecked = true;
					}
				}
				for (auto & func : program->GetFunctions())
				{
					if (!func->SemanticallyChecked)
					{
						VisitFunctionDeclaration(func.Ptr());
						if (funcNames.Contains(func->InternalName))
						{
							StringBuilder argList;
							argList << "(";
							bool first = true;
							for (auto & param : func->GetParameters())
							{
								if (!first)
									argList << ", ";
								argList << param->Type.type->ToString();
								first = false;
							}
							argList << ")";
							getSink()->diagnose(func, Diagnostics::functionRedefinitionWithArgList, func->Name, argList.ProduceString());
						}
						else
							funcNames.Add(func->InternalName);
					}
				}
				for (auto & func : program->GetFunctions())
				{
					if (!func->SemanticallyChecked)
					{
						func->Accept(this);
						func->SemanticallyChecked = true;
					}
				}
				for (auto & pipeline : program->GetPipelines())
				{
					VisitPipeline(pipeline.Ptr());
				}
				for (auto & interfaceNode : program->GetInterfaces())
				{
					if (!interfaceNode->SemanticallyChecked)
					{
						VisitInterface(interfaceNode.Ptr());
						interfaceNode->SemanticallyChecked = true;
					}
				}
				// build initial symbol table for shaders
				for (auto & shader : program->GetShaders())
				{
					if (!shader->SemanticallyChecked)
					{
						RefPtr<ShaderSymbol> shaderSym = new ShaderSymbol();
						shaderSym->SyntaxNode = shader.Ptr();
						if (symbolTable->Shaders.ContainsKey(shader->Name.Content))
						{
							getSink()->diagnose(shader->Name, Diagnostics::shaderAlreadyDefined, shader->Name);
						}
						symbolTable->Shaders[shader->Name.Content] = shaderSym;
					}
				}
				for (auto & shader : program->GetShaders())
				{
					if (!shader->SemanticallyChecked)
					{
						VisitShaderPass1(shader.Ptr());
						shader->SemanticallyChecked = true;
					}
				}
				if (sink->GetErrorCount() != 0)
					return programNode;
				// shader dependency is discovered in pass 1, we can now sort the shaders
				if (!symbolTable->SortShaders())
				{
					HashSet<ShaderSymbol*> sortedShaders;
					for (auto & shader : symbolTable->ShaderDependenceOrder)
						sortedShaders.Add(shader);
					for (auto & shader : symbolTable->Shaders)
						if (!sortedShaders.Contains(shader.Value.Ptr()))
						{
							getSink()->diagnose(shader.Value->SyntaxNode->Name, Diagnostics::shaderCircularity, shader.Key);
						}
				}

				for (auto & shader : symbolTable->ShaderDependenceOrder)
				{
					if (!shader->SemanticallyChecked)
					{
						VisitShaderPass2(shader->SyntaxNode);
						shader->SemanticallyChecked = true;
					}
				}

				return programNode;
			}

			virtual RefPtr<StructSyntaxNode> VisitStruct(StructSyntaxNode * structNode) override
			{
				for (auto field : structNode->GetFields())
				{
					field->Type = TranslateTypeNode(field->Type);
				}
				return structNode;
			}

			virtual RefPtr<TypeDefDecl> VisitTypeDefDecl(TypeDefDecl* decl) override
			{
				decl->Type = TranslateTypeNode(decl->Type);
				return decl;
			}

			virtual RefPtr<FunctionSyntaxNode> VisitFunction(FunctionSyntaxNode *functionNode) override
			{
				if (!functionNode->IsExtern())
				{
					currentFunc = symbolTable->Functions.TryGetValue(functionNode->InternalName)->Ptr();
					this->function = functionNode;
					functionNode->Body->Accept(this);
					this->function = NULL;
					currentFunc = nullptr;
				}
				return functionNode;
			}

			void VisitFunctionDeclaration(FunctionSyntaxNode *functionNode)
			{
				this->function = functionNode;
				auto returnType = TranslateTypeNode(functionNode->ReturnType);
				functionNode->ReturnType = returnType;
				StringBuilder internalName;
				internalName << functionNode->Name.Content;
				HashSet<String> paraNames;
				for (auto & para : functionNode->GetParameters())
				{
					if (paraNames.Contains(para->Name.Content))
						getSink()->diagnose(para, Diagnostics::parameterAlreadyDefined, para->Name);
					else
						paraNames.Add(para->Name.Content);
					para->Type = TranslateTypeNode(para->Type);
					if (para->Type.Equals(ExpressionType::Void.Ptr()))
						getSink()->diagnose(para, Diagnostics::parameterCannotBeVoid);
					internalName << "@" << para->Type.type->ToString();
				}
				functionNode->InternalName = internalName.ProduceString();
				RefPtr<FunctionSymbol> symbol = new FunctionSymbol();
				symbol->SyntaxNode = functionNode;
				symbolTable->Functions[functionNode->InternalName] = symbol;
				auto overloadList = symbolTable->FunctionOverloads.TryGetValue(functionNode->Name.Content);
				if (!overloadList)
				{
					symbolTable->FunctionOverloads[functionNode->Name.Content] = List<RefPtr<FunctionSymbol>>();
					overloadList = symbolTable->FunctionOverloads.TryGetValue(functionNode->Name.Content);
				}
				overloadList->Add(symbol);
				this->function = NULL;
			}

			virtual RefPtr<StatementSyntaxNode> VisitBlockStatement(BlockStatementSyntaxNode *stmt) override
			{
				for (auto & node : stmt->Statements)
				{
					node->Accept(this);
				}
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitBreakStatement(BreakStatementSyntaxNode *stmt) override
			{
				if (!loops.Count())
					getSink()->diagnose(stmt, Diagnostics::breakOutsideLoop);
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitContinueStatement(ContinueStatementSyntaxNode *stmt) override
			{
				if (!loops.Count())
					getSink()->diagnose(stmt, Diagnostics::continueOutsideLoop);
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitDoWhileStatement(DoWhileStatementSyntaxNode *stmt) override
			{
				loops.Add(stmt);
				if (stmt->Predicate != NULL)
					stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
				if (!stmt->Predicate->Type->Equals(ExpressionType::Error.Ptr()) &&
					!stmt->Predicate->Type->Equals(ExpressionType::Int.Ptr()) &&
					!stmt->Predicate->Type->Equals(ExpressionType::Bool.Ptr()))
				{
					getSink()->diagnose(stmt, Diagnostics::whilePredicateTypeError);
				}
				stmt->Statement->Accept(this);

				loops.RemoveAt(loops.Count() - 1);
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitForStatement(ForStatementSyntaxNode *stmt) override
			{
				loops.Add(stmt);
				if (stmt->InitialStatement)
				{
					stmt->InitialStatement = stmt->InitialStatement->Accept(this).As<StatementSyntaxNode>();
				}
				if (stmt->PredicateExpression)
				{
					stmt->PredicateExpression = stmt->PredicateExpression->Accept(this).As<ExpressionSyntaxNode>();
					if (!stmt->PredicateExpression->Type->Equals(ExpressionType::Bool.Ptr()) &&
						!stmt->PredicateExpression->Type->Equals(ExpressionType::Int.Ptr()) &&
						!stmt->PredicateExpression->Type->Equals(ExpressionType::UInt.Ptr()))
					{
						getSink()->diagnose(stmt->PredicateExpression.Ptr(), Diagnostics::forPredicateTypeError);
					}
				}
				if (stmt->SideEffectExpression)
				{
					stmt->SideEffectExpression = stmt->SideEffectExpression->Accept(this).As<ExpressionSyntaxNode>();
				}
				stmt->Statement->Accept(this);

				loops.RemoveAt(loops.Count() - 1);
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitIfStatement(IfStatementSyntaxNode *stmt) override
			{
				if (stmt->Predicate != NULL)
					stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
				if (!stmt->Predicate->Type->Equals(ExpressionType::Error.Ptr())
					&& (!stmt->Predicate->Type->Equals(ExpressionType::Int.Ptr()) &&
						!stmt->Predicate->Type->Equals(ExpressionType::Bool.Ptr())))
					getSink()->diagnose(stmt, Diagnostics::ifPredicateTypeError);

				if (stmt->PositiveStatement != NULL)
					stmt->PositiveStatement->Accept(this);

				if (stmt->NegativeStatement != NULL)
					stmt->NegativeStatement->Accept(this);
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitReturnStatement(ReturnStatementSyntaxNode *stmt) override
			{
				if (currentCompNode && currentCompNode->BlockStatement->Statements.Count() &&
					stmt != currentCompNode->BlockStatement->Statements.Last().Ptr())
				{
					getSink()->diagnose(stmt, Diagnostics::returnInComponentMustComeLast);
				}
				if (!stmt->Expression)
				{
					if (function && !function->ReturnType.Equals(ExpressionType::Void.Ptr()))
						getSink()->diagnose(stmt, Diagnostics::returnNeedsExpression);
				}
				else
				{
					stmt->Expression = stmt->Expression->Accept(this).As<ExpressionSyntaxNode>();
					if (!stmt->Expression->Type->Equals(ExpressionType::Error.Ptr()))
					{
						if (function && !MatchType_ValueReceiver(function->ReturnType.Ptr(), stmt->Expression->Type.Ptr()))
							getSink()->diagnose(stmt, Diagnostics::functionReturnTypeMismatch, stmt->Expression->Type, function->ReturnType);
						if (currentComp && !MatchType_ValueReceiver(currentComp->Type->DataType.Ptr(), stmt->Expression->Type.Ptr()))
						{
							getSink()->diagnose(stmt, Diagnostics::componentReturnTypeMismatch, stmt->Expression->Type, currentComp->Type->DataType);
						}
						if (currentImportOperator && !MatchType_GenericType(currentImportOperator->TypeName.Content, stmt->Expression->Type.Ptr()))
							getSink()->diagnose(stmt, Diagnostics::importOperatorReturnTypeMismatch, stmt->Expression->Type, currentImportOperator->TypeName);
					}
				}
				return stmt;
			}

			virtual RefPtr<Variable> VisitDeclrVariable(Variable* varDecl)
			{
				TypeExp typeExp = TranslateTypeNode(varDecl->Type);
				if (typeExp.type->IsTextureOrSampler() || typeExp.type->AsGenericType())
				{
					getSink()->diagnose(varDecl->Type, Diagnostics::invalidTypeForLocalVariable);
				}
				else if (auto declRefType = typeExp.type->AsDeclRefType())
				{
					if (auto worldDecl = dynamic_cast<WorldSyntaxNode*>(declRefType->decl))
					{
						// The type references a world, and we don't want to allow that here.
						// TODO(tfoley): there is no clear reason why this shouldn't be allowed semantically.
						getSink()->diagnose(varDecl->Type, Diagnostics::recordTypeVariableInImportOperator);
					}
				}
				varDecl->Type = typeExp;
				if (varDecl->Type.Equals(ExpressionType::Void.Ptr()))
					getSink()->diagnose(varDecl, Diagnostics::invalidTypeVoid);
				if (varDecl->Type.type->IsArray() && varDecl->Type.type->AsArrayType()->ArrayLength <= 0)
					getSink()->diagnose(varDecl, Diagnostics::invalidArraySize);
				if (varDecl->Expr != NULL)
				{
					varDecl->Expr = varDecl->Expr->Accept(this).As<ExpressionSyntaxNode>();
					if (!MatchType_ValueReceiver(varDecl->Type.Ptr(), varDecl->Expr->Type.Ptr())
						&& !varDecl->Expr->Type->Equals(ExpressionType::Error.Ptr()))
					{
						getSink()->diagnose(varDecl, Diagnostics::typeMismatch, varDecl->Expr->Type, varDecl->Type);
					}
				}
				return varDecl;
			}

			virtual RefPtr<StatementSyntaxNode> VisitWhileStatement(WhileStatementSyntaxNode *stmt) override
			{
				loops.Add(stmt);
				stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
				if (!stmt->Predicate->Type->Equals(ExpressionType::Error.Ptr()) &&
					!stmt->Predicate->Type->Equals(ExpressionType::Int.Ptr()) &&
					!stmt->Predicate->Type->Equals(ExpressionType::Bool.Ptr()))
					getSink()->diagnose(stmt, Diagnostics::whilePredicateTypeError2);

				stmt->Statement->Accept(this);
				loops.RemoveAt(loops.Count() - 1);
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitExpressionStatement(ExpressionStatementSyntaxNode *stmt) override
			{
				stmt->Expression = stmt->Expression->Accept(this).As<ExpressionSyntaxNode>();
				return stmt;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitBinaryExpression(BinaryExpressionSyntaxNode *expr) override
			{
				expr->LeftExpression = expr->LeftExpression->Accept(this).As<ExpressionSyntaxNode>();
				expr->RightExpression = expr->RightExpression->Accept(this).As<ExpressionSyntaxNode>();
				auto & leftType = expr->LeftExpression->Type;
				auto & rightType = expr->RightExpression->Type;
				RefPtr<ExpressionType> matchedType;
				auto checkAssign = [&]()
				{
					if (!leftType.IsLeftValue &&
						!leftType->Equals(ExpressionType::Error.Ptr()))
						getSink()->diagnose(expr->LeftExpression.Ptr(), Diagnostics::assignNonLValue);
					if (expr->Operator == Operator::AndAssign ||
						expr->Operator == Operator::OrAssign ||
						expr->Operator == Operator::XorAssign ||
						expr->Operator == Operator::LshAssign ||
						expr->Operator == Operator::RshAssign)
					{
						if (!(leftType->IsIntegral() && rightType->IsIntegral()))
						{
							getSink()->diagnose(expr, Diagnostics::bitOperationNonIntegral);
						}
					}
					expr->LeftExpression->Access = ExpressionAccess::Write;
					if (MatchType_ValueReceiver(leftType.Ptr(), expr->Type.Ptr()))
						expr->Type = leftType;
					else
						expr->Type = ExpressionType::Error;
				};
				if (expr->Operator == Operator::Assign)
				{
					expr->Type = rightType;
					checkAssign();
				}
				else
				{
					List<RefPtr<ExpressionType>> argTypes;
					argTypes.Add(leftType);
					argTypes.Add(rightType);
					List<RefPtr<FunctionSymbol>> * operatorOverloads = symbolTable->FunctionOverloads.TryGetValue(GetOperatorFunctionName(expr->Operator));
					auto overload = FindFunctionOverload(*operatorOverloads, [](RefPtr<FunctionSymbol> f)
					{
						return f->SyntaxNode->GetParameters();
					}, argTypes);
					if (!overload)
					{
						expr->Type = ExpressionType::Error;
						if (!leftType->Equals(ExpressionType::Error.Ptr()) && !rightType->Equals(ExpressionType::Error.Ptr()))
							getSink()->diagnose(expr, Diagnostics::noOverloadFoundForBinOperatorOnTypes, OperatorToString(expr->Operator), leftType, rightType);
					}
					else
					{
						expr->Type = overload->SyntaxNode->ReturnType;
					}
					if (expr->Operator > Operator::Assign)
						checkAssign();
				}
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitConstantExpression(ConstantExpressionSyntaxNode *expr) override
			{
				switch (expr->ConstType)
				{
				case ConstantExpressionSyntaxNode::ConstantType::Int:
					expr->Type = ExpressionType::Int;
					break;
				case ConstantExpressionSyntaxNode::ConstantType::Bool:
					expr->Type = ExpressionType::Bool;
					break;
				case ConstantExpressionSyntaxNode::ConstantType::Float:
					expr->Type = ExpressionType::Float;
					break;
				default:
					expr->Type = ExpressionType::Error;
					throw "Invalid constant type.";
					break;
				}
				return expr;
			}

			// Check that an expression resolves to an integer constant, and get its value
			int CheckIntegerConstantExpression(ExpressionSyntaxNode* exp, int defaultValue = 0)
			{
				if (!exp->Type.type->Equals(ExpressionType::Int))
				{
					getSink()->diagnose(exp, Diagnostics::expectedIntegerConstantWrongType, exp->Type);
					return defaultValue;
				}

				// TODO(tfoley): more serious constant folding here
				if (auto constExp = dynamic_cast<ConstantExpressionSyntaxNode*>(exp))
				{
					return constExp->IntValue;
				}

				getSink()->diagnose(exp, Diagnostics::expectedIntegerConstantNotConstant);
				return defaultValue;
			}

			virtual RefPtr<ExpressionSyntaxNode> VisitIndexExpression(IndexExpressionSyntaxNode *expr) override
			{
				expr->BaseExpression = expr->BaseExpression->Accept(this).As<ExpressionSyntaxNode>();
				if (expr->IndexExpression)
				{
					expr->IndexExpression = expr->IndexExpression->Accept(this).As<ExpressionSyntaxNode>();
				}
				if (expr->BaseExpression->Type->Equals(ExpressionType::Error.Ptr()))
					expr->Type = ExpressionType::Error;
				else if (auto baseTypeType = expr->BaseExpression->Type.type.As<TypeExpressionType>())
				{
					// We are trying to "index" into a type, so we have an expression like `float[2]`
					// which should be interpreted as resolving to an array type.

					int elementCount = 0;
					if (expr->IndexExpression)
					{
						elementCount = CheckIntegerConstantExpression(expr->IndexExpression.Ptr());
					}

					auto elementType = baseTypeType->type;
					auto arrayType = new ArrayExpressionType();
					arrayType->BaseType = elementType;
					arrayType->ArrayLength = elementCount;

					typeResult = arrayType;
					expr->Type = new TypeExpressionType(arrayType);
					return expr;
				}
				else
				{
					auto & baseExprType = expr->BaseExpression->Type;
					bool isValid = baseExprType->AsGenericType() &&
						(baseExprType->AsGenericType()->GenericTypeName == "StructuredBuffer" ||
							baseExprType->AsGenericType()->GenericTypeName == "RWStructuredBuffer" ||
							baseExprType->AsGenericType()->GenericTypeName == "PackedBuffer");
					isValid = isValid || (baseExprType->AsBasicType()); /*TODO(tfoley): figure this out: */ // && GetVectorSize(baseExprType->AsBasicType()->BaseType) != 0);
					isValid = isValid || baseExprType->AsArrayType();
					if (!isValid)
					{
						getSink()->diagnose(expr, Diagnostics::subscriptNonArray);
						expr->Type = ExpressionType::Error;
					}
					if (!expr->IndexExpression->Type->Equals(ExpressionType::Int.Ptr()) &&
						!expr->IndexExpression->Type->Equals(ExpressionType::UInt.Ptr()))
					{
						getSink()->diagnose(expr, Diagnostics::subscriptIndexNonInteger);
						expr->Type = ExpressionType::Error;
					}
				}
				if (expr->BaseExpression->Type->IsArray())
				{
					expr->Type = expr->BaseExpression->Type->AsArrayType()->BaseType;
				}
				else if (auto genType = expr->BaseExpression->Type->AsGenericType())
				{
					expr->Type = genType->BaseType;
				}
				else if (auto vecType = expr->BaseExpression->Type->AsVectorType())
				{
					expr->Type = vecType->elementType;
				}
				else if (auto matType = expr->BaseExpression->Type->AsMatrixType())
				{
					expr->Type = matType->rowType;
				}
				else
				{
					// TODO(tfoley): need an error case here...
				}

				// Result of an index expression is an l-value iff base is.
				expr->Type.IsLeftValue = expr->BaseExpression->Type.IsLeftValue;
				return expr;
			}
			bool MatchArguments(FunctionSyntaxNode * functionNode, List <RefPtr<ExpressionSyntaxNode>> &args)
			{
				if (functionNode->GetParameters().Count() != args.Count())
					return false;
				int i = 0;
				for (auto param : functionNode->GetParameters())
				{
					if (!param->Type.Equals(args[i]->Type.Ptr()))
						return false;
					i++;
				}
				return true;
			}

			template<typename GetParamFunc, typename PFuncT>
			PFuncT FindFunctionOverload(const List<PFuncT> & funcs, const GetParamFunc & getParam, const List<RefPtr<ExpressionType>> & arguments)
			{
				int bestMatchConversions = 1 << 30;
				PFuncT func = nullptr;
				for (auto & f : funcs)
				{
					auto params = getParam(f);
					if (params.Count() == arguments.Count())
					{
						int conversions = 0;
						bool match = true;
						int i = 0;
						for (auto param : params)
						{
							auto argType = arguments[i];
							auto paramType = param->Type;
							if (argType->Equals(paramType.Ptr()))
							{
							}
							else if (MatchType_ValueReceiver(paramType.Ptr(), argType.Ptr()))
							{
								conversions++;
							}
							else
							{
								match = false;
								break;
							}
							i++;
						}
						if (match && conversions < bestMatchConversions)
						{
							func = f;
							bestMatchConversions = conversions;
						}
					}
				}
				return func;
			}

			ShaderComponentSymbol * ResolveFunctionComponent(ShaderSymbol * shader, String name, const List<RefPtr<ExpressionType>> & args, bool topLevel = true)
			{
				auto list = shader->FunctionComponents.TryGetValue(name);
				if (list)
				{
					auto func = FindFunctionOverload(*list, [](RefPtr<ShaderComponentSymbol> & comp)
					{
						return comp->Implementations.First()->SyntaxNode->GetParameters();
					}, args);
					if (func)
					{
						return func.Ptr();
					}
				}
				for (auto & module : shader->ShaderUsings)
				{
					if (module.IsPublic || topLevel)
					{
						auto func = ResolveFunctionComponent(module.Shader, name, args, false);
						if (func)
							return func;
					}
				}
				return nullptr;
			}

			ShaderComponentSymbol * ResolveFunctionComponent(ShaderSymbol * shader, String name, const List<RefPtr<ExpressionSyntaxNode>> & args, bool topLevel = true)
			{
				return ResolveFunctionComponent(shader, name, From(args).Select([](RefPtr<ExpressionSyntaxNode> x) {return x->Type.type; }).ToList(), topLevel);
			}

			RefPtr<ExpressionSyntaxNode> ResolveFunctionOverload(InvokeExpressionSyntaxNode * invoke, MemberExpressionSyntaxNode* memberExpr, List<RefPtr<ExpressionSyntaxNode>> & arguments)
			{
				// TODO(tfoley): Figure out why we even need to do this here...
				DiagnosticSink::State savedState = sink->saveState();
				memberExpr->BaseExpression->Accept(this);
				sink->restoreState(savedState);
				if (memberExpr->BaseExpression->Type->IsShader())
				{
					auto basicType = memberExpr->BaseExpression->Type->AsBasicType();
					auto func = ResolveFunctionComponent(basicType->Shader, memberExpr->MemberName, arguments);
					if (func)
					{
						auto funcType = new FuncType();
						funcType->Component = func;
						memberExpr->Type = funcType;
						memberExpr->MemberName = func->Name;
						invoke->Type = func->Implementations.First()->SyntaxNode->Type;
						return invoke;
					}
				}
				else
				{
					invoke->Arguments.Insert(0, memberExpr->BaseExpression);
					auto funcExpr = new VarExpressionSyntaxNode();
					funcExpr->Scope = invoke->Scope;
					funcExpr->Position = invoke->Position;
					funcExpr->Variable = memberExpr->MemberName;
					invoke->FunctionExpr = funcExpr;
					return ResolveFunctionOverload(invoke, funcExpr, invoke->Arguments);
				}
				return invoke;
			}

			RefPtr<ExpressionSyntaxNode> ResolveFunctionOverload(InvokeExpressionSyntaxNode * invoke, VarExpressionSyntaxNode* varExpr, List<RefPtr<ExpressionSyntaxNode>> & arguments)
			{
				if (currentShader)
				{
					auto func = ResolveFunctionComponent(currentShader, varExpr->Variable, arguments);
					if (func)
					{
						auto funcType = new FuncType();
						funcType->Component = func;
						varExpr->Type = funcType;
						invoke->Type = func->Implementations.First()->SyntaxNode->Type;
						return invoke;
					}
				}
				// check if this is an import operator call
				if (currentShader && currentCompNode && arguments.Count() > 0 && currentShader->ParentPipeline)
				{
					if (auto impOpList = currentShader->ParentPipeline->ImportOperators.TryGetValue(varExpr->Variable))
					{
						// component with explicit import operator call must be qualified with explicit rate
						if (!currentCompNode->Rate)
						{
							getSink()->diagnose(varExpr, Diagnostics::importOperatorCalledFromAutoPlacedComponent, currentCompNode->Name);
							invoke->Type = ExpressionType::Error;
							return invoke;
						}
						// for now we do not support calling import operator from a multi-world component definition
						if (currentCompNode->Rate->Worlds.Count() > 1)
							getSink()->diagnose(varExpr, Diagnostics::importOperatorCalledFromMultiWorldComponent);
						auto validOverloads = From(*impOpList).Where([&](RefPtr<ImportOperatorDefSyntaxNode> imp) { return imp->DestWorld.Content == currentCompNode->Rate->Worlds.First().World.Content; }).ToList();
						auto func = FindFunctionOverload(validOverloads, [](RefPtr<ImportOperatorDefSyntaxNode> imp)
						{
							return imp->GetParameters();
						}, From(arguments).Skip(1).Select([](RefPtr<ExpressionSyntaxNode> x) {return x->Type.type; }).ToList());
						if (func)
						{
							RefPtr<ImportExpressionSyntaxNode> importExpr = new ImportExpressionSyntaxNode();
							importExpr->Position = varExpr->Position;
							importExpr->Component = arguments[0];
							CloneContext cloneCtx;
							importExpr->ImportOperatorDef = func->Clone(cloneCtx);
							importExpr->ImportOperatorDef->Scope->Parent = varExpr->Scope->Parent;
							importExpr->Type = arguments[0]->Type;
							importExpr->Scope = varExpr->Scope;
							importExpr->Access = ExpressionAccess::Read;
							for (int i = 1; i < arguments.Count(); i++)
								importExpr->Arguments.Add(arguments[i]);
							return importExpr;
						}
						else
						{
							StringBuilder argList;
							for (int i = 1; i < arguments.Count(); i++)
							{
								argList << arguments[i]->Type->ToString();
								if (i != arguments.Count() - 1)
									argList << ", ";
							}
							getSink()->diagnose(varExpr, Diagnostics::noApplicableImportOperator,
								varExpr->Variable,
								currentShader->ParentPipeline->SyntaxNode->Name,
								currentCompNode->Rate->Worlds.First().World,
								argList.ProduceString());
							invoke->Type = ExpressionType::Error;
						}
						return invoke;
					}
				}
				// this is not an import operator call, resolve as function call
				bool found = false;
				bool functionNameFound = false;
				RefPtr<FunctionSymbol> func;
				varExpr->Variable = TranslateHLSLTypeNames(varExpr->Variable);

				if (varExpr->Variable == "texture" && arguments.Count() > 0 &&
					arguments[0]->Type->IsGenericType("Texture"))
				{
					if (arguments.Count() != 2)
					{
						invoke->Type = ExpressionType::Error;
						found = false;
					}
					else
					{
						if (auto genType = arguments[0]->Type->AsGenericType())
						{
							invoke->Type = genType->BaseType;
							if (!arguments[1]->Type->Equals(ExpressionType::Float2.Ptr()))
							{
								found = false;
								invoke->Type = ExpressionType::Error;
							}
						}
						else
						{
							invoke->Type = ExpressionType::Error;
							found = false;
						}
					}
					auto funcType = new FuncType();
					funcType->Func = symbolTable->FunctionOverloads["texture"]().First().Ptr();
					varExpr->Type = funcType;
				}
				else
				{
					// find function overload with implicit argument type conversions
					auto namePrefix = varExpr->Variable + "@";
					List<RefPtr<FunctionSymbol>> * functionOverloads = symbolTable->FunctionOverloads.TryGetValue(varExpr->Variable);
					if (functionOverloads)
					{
						func = FindFunctionOverload(*functionOverloads, [](RefPtr<FunctionSymbol> f)
						{
							return f->SyntaxNode->GetParameters();
						}, From(arguments).Select([](RefPtr<ExpressionSyntaxNode> x) {return x->Type.type; }).ToList());
						functionNameFound = true;
					}
				}
				if (func)
				{
					if (!func->SyntaxNode->IsExtern())
					{
						varExpr->Variable = func->SyntaxNode->InternalName;
						if (currentFunc)
							currentFunc->ReferencedFunctions.Add(func->SyntaxNode->InternalName);
					}
					invoke->Type = func->SyntaxNode->ReturnType;
					auto funcType = new FuncType();
					funcType->Func = func.Ptr();
					varExpr->Type = funcType;
					found = true;
				}
				if (!found)
				{
					invoke->Type = ExpressionType::Error;
					StringBuilder argList;
					for (int i = 0; i < arguments.Count(); i++)
					{
						argList << arguments[i]->Type->ToString();
						if (i != arguments.Count() - 1)
							argList << ", ";
					}
					if (functionNameFound)
						getSink()->diagnose(varExpr, Diagnostics::noApplicationFunction, varExpr->Variable, argList.ProduceString());
					else
						getSink()->diagnose(varExpr, Diagnostics::undefinedIdentifier2, varExpr->Variable);
				}
				return invoke;
			}

			bool IsValidWorldTypeForProjection(
				ProjectExpressionSyntaxNode*	projection,
				String const&					expectedWorld)
			{
				auto baseType = projection->BaseExpression->Type;
				auto declRefType = baseType->AsDeclRefType();
				if (!declRefType)
					return false;

				auto worldDecl = dynamic_cast<WorldSyntaxNode*>(declRefType->decl);
				if (!worldDecl)
					return false;

				// TODO(tfoley): Doing this is a string-based check is wrong...
				return worldDecl->Name.Content == expectedWorld;
			}

			RefPtr<ExpressionSyntaxNode> VisitProject(ProjectExpressionSyntaxNode * project) override
			{
				if (currentImportOperator == nullptr)
				{
					getSink()->diagnose(project, Diagnostics::projectionOutsideImportOperator);
					return project;
				}
				project->BaseExpression->Accept(this);

				// Check that the type we are projecting from matches the expected world type.
				if (!IsValidWorldTypeForProjection(project, currentImportOperator->SourceWorld.Content))
				{
					getSink()->diagnose(project, Diagnostics::projectTypeMismatch, currentImportOperator->SourceWorld);
				}

				auto rsType = new BasicExpressionType(BaseType::Generic);
				project->Type = rsType;
				rsType->GenericTypeVar = currentImportOperator->TypeName.Content;
				return project;
			}

			RefPtr<ExpressionSyntaxNode> ResolveInvoke(InvokeExpressionSyntaxNode * expr)
			{
				if (auto varExpr = expr->FunctionExpr.As<VarExpressionSyntaxNode>())
				{
					return ResolveFunctionOverload(expr, varExpr.Ptr(), expr->Arguments);
				}
				else if (auto memberExpr = expr->FunctionExpr.As<MemberExpressionSyntaxNode>())
				{
					return ResolveFunctionOverload(expr, memberExpr.Ptr(), expr->Arguments);
				}
				else
				{
					getSink()->diagnose(expr->FunctionExpr, Diagnostics::expectedFunction);
					expr->Type = ExpressionType::Error;
				}
				return expr;
			}

			virtual RefPtr<ExpressionSyntaxNode> VisitInvokeExpression(InvokeExpressionSyntaxNode *expr) override
			{
				for (auto & arg : expr->Arguments)
					arg = arg->Accept(this).As<ExpressionSyntaxNode>();

				auto rs = ResolveInvoke(expr);
				if (auto invoke = dynamic_cast<InvokeExpressionSyntaxNode*>(rs.Ptr()))
				{
					// if this is still an invoke expression, test arguments passed to inout/out parameter are LValues
					if(auto funcType = invoke->FunctionExpr->Type->As<FuncType>())
					{
						List<RefPtr<ParameterSyntaxNode>> paramsStorage;
						List<RefPtr<ParameterSyntaxNode>> * params = nullptr;
						if (auto funcSym = funcType->Func)
						{
							paramsStorage = funcSym->SyntaxNode->GetParameters().ToArray();
							params = &paramsStorage;
						}
						else if (auto componentFuncSym = funcType->Component)
						{
							paramsStorage = componentFuncSym->Implementations.First()->SyntaxNode->GetParameters().ToArray();
							params = &paramsStorage;
						}
						if (params)
						{
							for (int i = 0; i < (*params).Count(); i++)
							{
								if ((*params)[i]->HasModifier(ModifierFlag::Out))
								{
									if (i < expr->Arguments.Count() && expr->Arguments[i]->Type->AsBasicType() &&
										!expr->Arguments[i]->Type.IsLeftValue)
									{
										getSink()->diagnose(expr->Arguments[i], Diagnostics::argumentExpectedLValue, (*params)[i]->Name);
									}
								}
							}
						}
					}
				}
				return rs;
			}

			String OperatorToString(Operator op)
			{
				switch (op)
				{
				case Spire::Compiler::Operator::Neg:
					return "-";
				case Spire::Compiler::Operator::Not:
					return "!";
				case Spire::Compiler::Operator::PreInc:
					return "++";
				case Spire::Compiler::Operator::PreDec:
					return "--";
				case Spire::Compiler::Operator::PostInc:
					return "++";
				case Spire::Compiler::Operator::PostDec:
					return "--";
				case Spire::Compiler::Operator::Mul:
				case Spire::Compiler::Operator::MulAssign:
					return "*";
				case Spire::Compiler::Operator::Div:
				case Spire::Compiler::Operator::DivAssign:
					return "/";
				case Spire::Compiler::Operator::Mod:
				case Spire::Compiler::Operator::ModAssign:
					return "%";
				case Spire::Compiler::Operator::Add:
				case Spire::Compiler::Operator::AddAssign:
					return "+";
				case Spire::Compiler::Operator::Sub:
				case Spire::Compiler::Operator::SubAssign:
					return "-";
				case Spire::Compiler::Operator::Lsh:
				case Spire::Compiler::Operator::LshAssign:
					return "<<";
				case Spire::Compiler::Operator::Rsh:
				case Spire::Compiler::Operator::RshAssign:
					return ">>";
				case Spire::Compiler::Operator::Eql:
					return "==";
				case Spire::Compiler::Operator::Neq:
					return "!=";
				case Spire::Compiler::Operator::Greater:
					return ">";
				case Spire::Compiler::Operator::Less:
					return "<";
				case Spire::Compiler::Operator::Geq:
					return ">=";
				case Spire::Compiler::Operator::Leq:
					return "<=";
				case Spire::Compiler::Operator::BitAnd:
				case Spire::Compiler::Operator::AndAssign:
					return "&";
				case Spire::Compiler::Operator::BitXor:
				case Spire::Compiler::Operator::XorAssign:
					return "^";
				case Spire::Compiler::Operator::BitOr:
				case Spire::Compiler::Operator::OrAssign:
					return "|";
				case Spire::Compiler::Operator::And:
					return "&&";
				case Spire::Compiler::Operator::Or:
					return "||";
				case Spire::Compiler::Operator::Assign:
					return "=";
				default:
					return "ERROR";
				}
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitUnaryExpression(UnaryExpressionSyntaxNode *expr) override
			{
				expr->Expression = expr->Expression->Accept(this).As<ExpressionSyntaxNode>();
				List<RefPtr<ExpressionType>> argTypes;
				argTypes.Add(expr->Expression->Type);
				List<RefPtr<FunctionSymbol>> * operatorOverloads = symbolTable->FunctionOverloads.TryGetValue(GetOperatorFunctionName(expr->Operator));
				auto overload = FindFunctionOverload(*operatorOverloads, [](RefPtr<FunctionSymbol> f)
				{
					return f->SyntaxNode->GetParameters();
				}, argTypes);
				if (!overload)
				{
					expr->Type = ExpressionType::Error;
					if (!expr->Expression->Type->Equals(ExpressionType::Error.Ptr()))
						getSink()->diagnose(expr, Diagnostics::noApplicationUnaryOperator, OperatorToString(expr->Operator), expr->Expression->Type);
				}
				else
				{
					expr->Type = overload->SyntaxNode->ReturnType;
				}
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode *expr) override
			{
				ShaderUsing shaderObj;
				expr->Type = ExpressionType::Error;
				auto decl = expr->Scope->LookUp(expr->Variable);
				if (decl)
				{
					// Found declaration through ordinary scope lookup rules
					expr->decl = decl;

					// We need to insert an appropriate type for the expression, based on
					// what we found.
					if (auto varDecl = dynamic_cast<VarDeclBase*>(decl))
					{
						expr->Type = varDecl->Type;

						// A variable reference is an l-value as long as the variable is mutable.
						// Currently the only immutable variable declarations are components.
						expr->Type.IsLeftValue = !(dynamic_cast<ComponentSyntaxNode*>(varDecl));
					}
					else if (auto compDecl = dynamic_cast<ComponentSyntaxNode*>(decl))
					{
						// TODO(tfoley): this is not correct in the case where we have a
						// component *function*.
						expr->Type = compDecl->Type;
					}
					else if (auto typeDecl = dynamic_cast<BuiltinTypeDecl*>(decl))
					{
						// TODO(tfoley): stash the resulting type somewhere on the decl, for convenience
						RefPtr<BasicExpressionType> type = new BasicExpressionType(typeDecl->tag);
						typeResult = type;
						expr->Type = new TypeExpressionType(type);
					}
					else if (auto typeAliasDecl = dynamic_cast<TypeDefDecl*>(decl))
					{
						auto type = new NamedExpressionType(typeAliasDecl);
						typeResult = type;
						expr->Type = new TypeExpressionType(type);
					}
					else if (auto aggTypeDecl = dynamic_cast<AggTypeDecl*>(decl))
					{
						auto type = new DeclRefType(aggTypeDecl);
						typeResult = type;
						expr->Type = new TypeExpressionType(type);
					}
					else if (auto simpleTypeDecl = dynamic_cast<SimpleTypeDecl*>(decl))
					{
						auto type = new DeclRefType(simpleTypeDecl);
						typeResult = type;
						expr->Type = new TypeExpressionType(type);
					}
					else
					{
						getSink()->diagnose(expr, Diagnostics::unimplemented, "declaration reference case");
					}
					return expr;
				}

				// Ad hoc lookup rules for cases where the scope-based lookup currently doesn't apply.
				if (currentShader && currentShader->ShaderObjects.TryGetValue(expr->Variable, shaderObj))
				{
					auto basicType = new BasicExpressionType(BaseType::Shader);
					basicType->Shader = shaderObj.Shader;
					expr->Type = basicType;
				}
				else if (currentPipeline && currentImportOperator)
				{
					RefPtr<ShaderComponentSymbol> comp;
					if (currentPipeline->Components.TryGetValue(expr->Variable, comp))
					{
						currentImportOperator->Usings.Add(comp->Name);
						expr->Type = comp->Type->DataType;
					}
					else
						getSink()->diagnose(expr, Diagnostics::undefinedIdentifier2, expr->Variable);
				}
				else if (currentShader)
				{
					auto compRef = currentShader->ResolveComponentReference(expr->Variable);
					if (compRef.IsAccessible)
					{
						expr->Type = compRef.Component->Type->DataType;
					}
					else if (compRef.Component)
					{
						getSink()->diagnose(expr, Diagnostics::componentNotAccessibleFromShader, expr->Variable, currentShader->SyntaxNode->Name);
					}
					else
						getSink()->diagnose(expr, Diagnostics::undefinedIdentifier2, expr->Variable);
				}
				else if (auto compDecl = dynamic_cast<ComponentSyntaxNode*>(decl)) // interface decl
				{
					expr->Type = compDecl->Type;
				}
				else
					getSink()->diagnose(expr, Diagnostics::undefinedIdentifier2, expr->Variable);

				if (expr->Type->IsGenericType("Uniform") || expr->Type->IsGenericType("Patch") || expr->Type->IsGenericType("StorageBuffer"))
					expr->Type = expr->Type->AsGenericType()->BaseType;

				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitTypeCastExpression(TypeCastExpressionSyntaxNode * expr) override
			{
				expr->Expression = expr->Expression->Accept(this).As<ExpressionSyntaxNode>();
				auto targetType = TranslateTypeNode(expr->TargetType);

				// The way to perform casting depends on the types involved
				if (expr->Expression->Type->Equals(ExpressionType::Error.Ptr()))
				{
					// If the expression being casted has an error type, then just silently succeed
					expr->Type = targetType;
					return expr;
				}
				else if (auto targetArithType = targetType->AsArithmeticType())
				{
					if (auto exprArithType = expr->Expression->Type->AsArithmeticType())
					{
						// Both source and destination types are arithmetic, so we might
						// have a valid cast
						auto targetScalarType = targetArithType->GetScalarType();
						auto exprScalarType = exprArithType->GetScalarType();

						if (!IsNumeric(exprScalarType->BaseType)) goto fail;
						if (!IsNumeric(targetScalarType->BaseType)) goto fail;

						// TODO(tfoley): this checking is incomplete here, and could
						// lead to downstream compilation failures
						expr->Type = targetType;
						return expr;
					}
				}

			fail:
				// Default: in no other case succeds, then the cast failed and we emit a diagnostic.
				getSink()->diagnose(expr, Diagnostics::invalidTypeCast, expr->Expression->Type, targetType->ToString());
				expr->Type = ExpressionType::Error;
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitSelectExpression(SelectExpressionSyntaxNode * expr) override
			{
				expr->SelectorExpr = expr->SelectorExpr->Accept(this).As<ExpressionSyntaxNode>();
				if (!expr->SelectorExpr->Type->Equals(ExpressionType::Int.Ptr()) && !expr->SelectorExpr->Type->Equals(ExpressionType::Bool.Ptr())
					&& !expr->SelectorExpr->Type->Equals(ExpressionType::Error.Ptr()))
				{
					expr->Type = ExpressionType::Error;
					getSink()->diagnose(expr, Diagnostics::selectPrdicateTypeMismatch);
				}
				expr->Expr0 = expr->Expr0->Accept(this).As<ExpressionSyntaxNode>();
				expr->Expr1 = expr->Expr1->Accept(this).As<ExpressionSyntaxNode>();
				if (!expr->Expr0->Type->Equals(expr->Expr1->Type.Ptr()))
				{
					getSink()->diagnose(expr, Diagnostics::selectValuesTypeMismatch);
				}
				expr->Type = expr->Expr0->Type;
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * expr) override
			{
				expr->BaseExpression = expr->BaseExpression->Accept(this).As<ExpressionSyntaxNode>();
				auto & baseType = expr->BaseExpression->Type;
				if (auto declRefType = baseType->AsDeclRefType())
				{
					if (auto structDecl = dynamic_cast<StructSyntaxNode*>(declRefType->decl))
					{
						StructField* field = structDecl->FindField(expr->MemberName);
						if (!field)
						{
							expr->Type = ExpressionType::Error;
							getSink()->diagnose(expr, Diagnostics::noMemberOfNameInType, expr->MemberName, baseType);
						}
						else
							expr->Type = field->Type;

						// A reference to a struct member is an l-value if the reference to the struct
						// value was also an l-value.
						expr->Type.IsLeftValue = expr->BaseExpression->Type.IsLeftValue;
						return expr;
					}

					// catch-all
					expr->Type = ExpressionType::Error;
				}
				else if (auto baseVecType = baseType->AsVectorType())
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
							case 'x':
							case 'r':
								children.Add(0);
								break;
							case 'y':
							case 'g':
								children.Add(1);
								break;
							case 'z':
							case 'b':
								children.Add(2);
								break;
							case 'w':
							case 'a':
								children.Add(3);
								break;
							default:
								error = true;
								expr->Type = ExpressionType::Error;
								break;
							}
						}
						int vecLen = GetVectorSize(baseVecType);
						for (auto m : children)
						{
							if (m >= vecLen)
							{
								error = true;
								expr->Type = ExpressionType::Error;
								break;
							}
						}
						if (!error)
						{
							expr->Type = new VectorExpressionType(
								baseVecType->elementType,
								children.Count());
						}

						// compute whether result of swizzle is an l-value
						//
						// Note(tfoley): The logic here seems to compute
						// whether the swizzle ever re-orders components,
						// but it should actually be checking if there are
						// any duplicated components.
						{
							bool isLValue = true;
							if (children.Count() > vecLen || children.Count() == 0)
								isLValue = false;
							int curMax = children[0];
							for (int i = 0; i < children.Count(); i++)
								if (children[i] < curMax)
								{
									isLValue = false;
									curMax = children[i];
								}
							expr->Type.IsLeftValue = isLValue;
						}
					}
				}
				// All remaining cases assume we have a `BasicType`
				else if (!baseType->AsBasicType())
					expr->Type = ExpressionType::Error;
				else if (baseType->AsBasicType()->BaseType == BaseType::Shader)
				{
					ShaderUsing shaderObj;
					auto refComp = baseType->AsBasicType()->Shader->ResolveComponentReference(expr->MemberName);
					if (refComp.IsAccessible)
						expr->Type = refComp.Component->Type->DataType;
					else if (baseType->AsBasicType()->Shader->ShaderObjects.TryGetValue(expr->MemberName, shaderObj))
					{
						if (shaderObj.IsPublic)
						{
							auto shaderType = new BasicExpressionType(BaseType::Shader);
							shaderType->Shader = shaderObj.Shader;
							expr->Type = shaderType;
						}
						else
							expr->Type = ExpressionType::Error;
					}
					else
						expr->Type = ExpressionType::Error;
				}
				else
					expr->Type = ExpressionType::Error;
				if (!baseType->Equals(ExpressionType::Error.Ptr()) &&
					expr->Type->Equals(ExpressionType::Error.Ptr()))
				{
					getSink()->diagnose(expr, Diagnostics::typeHasNoPublicMemberOfName, baseType, expr->MemberName);
				}
				return expr;
			}
			SemanticsVisitor & operator = (const SemanticsVisitor &) = delete;
		};

		SyntaxVisitor * CreateSemanticsVisitor(SymbolTable * symbols, DiagnosticSink * err)
		{
			return new SemanticsVisitor(symbols, err);
		}

	}
}