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
			RefPtr<ExpressionSyntaxNode> TranslateTypeNodeImpl(const RefPtr<ExpressionSyntaxNode> & node)
			{
				if (!node) return nullptr;
				auto expr = node->Accept(this).As<ExpressionSyntaxNode>();
				expr = ExpectATypeRepr(expr);
				return expr;
			}
			RefPtr<ExpressionType> ExtractTypeFromTypeRepr(const RefPtr<ExpressionSyntaxNode>& typeRepr)
			{
				if (!typeRepr) return nullptr;
				if (auto typeType = typeRepr->Type->As<TypeExpressionType>())
				{
					return typeType->type;
				}
				return ExpressionType::Error;
			}
			RefPtr<ExpressionType> TranslateTypeNode(const RefPtr<ExpressionSyntaxNode> & node)
			{
				if (!node) return nullptr;
				auto typeRepr = TranslateTypeNodeImpl(node);
				return ExtractTypeFromTypeRepr(typeRepr);
			}
			TypeExp TranslateTypeNode(TypeExp const& typeExp)
			{
				auto typeRepr = TranslateTypeNodeImpl(typeExp.exp);

				TypeExp result;
				result.exp = typeRepr;
				result.type = ExtractTypeFromTypeRepr(typeRepr);
				return result;
			}

			RefPtr<ExpressionSyntaxNode> ConstructDeclRefExpr(
				Decl*							decl,
				RefPtr<ExpressionSyntaxNode>	baseExpr,
				RefPtr<ExpressionSyntaxNode>	originalExpr)
			{
				if (baseExpr)
				{
					auto expr = new MemberExpressionSyntaxNode();
					expr->Position = originalExpr->Position;
					expr->BaseExpression = baseExpr;
					expr->MemberName = decl->Name.Content;
					expr->Type = GetTypeForDeclRef(decl);
					return expr;
				}
				else
				{
					auto expr = new VarExpressionSyntaxNode();
					expr->Position = originalExpr->Position;
					expr->Variable = decl->Name.Content;
					expr->Type = GetTypeForDeclRef(decl);
					return expr;
				}
			}

			RefPtr<ExpressionSyntaxNode> ResolveOverloadedExpr(RefPtr<OverloadedExpr> overloadedExpr, LookupMask mask)
			{
				auto lookupResult = overloadedExpr->lookupResult;
				assert(lookupResult.isValid() && lookupResult.isOverloaded());

				// filter the lookup to only consider declarations of the right flavor
				lookupResult.mask = LookupMask(uint8_t(lookupResult.mask) & uint8_t(mask));
				
				// and re-run lookup to see what we found
				lookupResult = DoLookup(lookupResult.decl->Name.Content, lookupResult);

				if (!lookupResult.isValid())
				{
					// If we didn't find any symbols after filtering, then just
					// use the original and report errors that way
					return overloadedExpr;
				}

				if (lookupResult.isOverloaded())
				{
					// We had an ambiguity anyway, so report it.
					getSink()->diagnose(overloadedExpr, Diagnostics::unimplemented, "ambiguous reference");

					// TODO(tfoley): should we construct a new ErrorExpr here?
					overloadedExpr->Type = ExpressionType::Error;
					return overloadedExpr;
				}

				// otherwise, we had a single decl and it was valid, hooray!
				return ConstructDeclRefExpr(lookupResult.decl, overloadedExpr->base, overloadedExpr);
			}

			RefPtr<ExpressionSyntaxNode> ExpectATypeRepr(RefPtr<ExpressionSyntaxNode> expr)
			{
				if (auto overloadedExpr = expr.As<OverloadedExpr>())
				{
					expr = ResolveOverloadedExpr(overloadedExpr, LookupMask::Type);
				}

				if (auto typeType = expr->Type.type->As<TypeExpressionType>())
				{
					return expr;
				}
				else if (expr->Type.type->Equals(ExpressionType::Error))
				{
					return expr;
				}

				getSink()->diagnose(expr, Diagnostics::unimplemented, "expected a type");
				// TODO: construct some kind of `ErrorExpr`?
				return expr;
			}

			RefPtr<ExpressionType> ExpectAType(RefPtr<ExpressionSyntaxNode> expr)
			{
				auto typeRepr = ExpectATypeRepr(expr);
				if (auto typeType = typeRepr->Type->As<TypeExpressionType>())
				{
					return typeType->type;
				}
				return ExpressionType::Error;
			}

			RefPtr<ExpressionType> ExtractGenericArgType(RefPtr<ExpressionSyntaxNode> exp)
			{
				return ExpectAType(exp);
			}

			int ExtractGenericArgInteger(RefPtr<ExpressionSyntaxNode> exp)
			{
				return CheckIntegerConstantExpression(exp.Ptr());
			}

			// Construct a type reprsenting the instantiation of
			// the given generic declaration for the given arguments.
			// The arguments should already be checked against
			// the declaration.
			RefPtr<ExpressionType> InstantiateGenericType(
				GenericDecl*								genericDecl,
				List<RefPtr<ExpressionSyntaxNode>> const&	args)
			{
				if (auto magicType = genericDecl->inner->FindModifier<MagicTypeModifier>())
				{
					if (magicType->name == "Vector")
					{
						auto vecType = new VectorExpressionType(
							ExtractGenericArgType(args[0]),
							ExtractGenericArgInteger(args[1]));
						vecType->decl = genericDecl->inner.Ptr();
						return vecType;
					}
					else if (magicType->name == "Matrix")
					{
						auto matType = new MatrixExpressionType(
							ExtractGenericArgType(args[0]),
							ExtractGenericArgInteger(args[1]),
							ExtractGenericArgInteger(args[2]));
						matType->decl = genericDecl->inner.Ptr();
						return matType;
					}
					else if (magicType->name == "Texture")
					{
						auto textureType = new TextureType(
							magicType->tag,
							ExtractGenericArgType(args[0]));
						textureType->decl = genericDecl->inner.Ptr();
						return textureType;
					}
					else
					{
						throw "unimplemented";
					}
				}

				// catch-all for cases that we don't handle
				throw "unimplemented";
			}

			// Make sure a declaration has been checked, so we can refer to it.
			// Note that this may lead to us recursively invoking checking,
			// so this may not be the best way to handle things.
			void EnsureDecl(RefPtr<Decl> decl, DeclCheckState state = DeclCheckState::CheckedHeader)
			{
				if (decl->IsChecked(state)) return;
				if (decl->checkState == DeclCheckState::CheckingHeader)
				{
					// We tried to reference the same declaration while checking it!
					throw "circularity";
				}

				if (DeclCheckState::CheckingHeader > decl->checkState)
				{
					decl->SetCheckState(DeclCheckState::CheckingHeader);
				}

				// TODO: not all of the `Visit` cases are ready to
				// handle this being called on-the-fly
				decl->Accept(this);

				decl->SetCheckState(DeclCheckState::Checked);
			}

			void EnusreAllDeclsRec(RefPtr<Decl> decl)
			{
				EnsureDecl(decl, DeclCheckState::Checked);
				if (auto containerDecl = decl.As<ContainerDecl>())
				{
					for (auto m : containerDecl->Members)
					{
						EnusreAllDeclsRec(m);
					}
				}
			}

			// Take an existing type (expression) and coerce it to one
			// that can be used for declaraing a variable/parameter/etc.
			TypeExp CoerceToProperType(TypeExp const& typeExp)
			{
				TypeExp result = typeExp;
				ExpressionType* type = result.type.Ptr();
				if (auto basicType = type->As<BasicExpressionType>())
				{
					// TODO: `void` shouldn't be a basic type, to make this easier to avoid
					if (basicType->BaseType == BaseType::Void)
					{
						// TODO(tfoley): pick the right diagnostic message
						getSink()->diagnose(result.exp.Ptr(), Diagnostics::parameterCannotBeVoid);
						result.type = ExpressionType::Error;
						return result;
					}
					else
					{
						return result;
					}
				}
				else if (auto genericDeclRefType = type->As<GenericDeclRefType>())
				{
					// We are using a reference to a generic declaration as a concrete
					// type. This means we should substitute in any default parameter values
					// if they are available.
					//
					// TODO(tfoley): A more expressive type system would substitute in
					// "fresh" variables and then solve for their values...
					//

					auto genericDecl = genericDeclRefType->decl;
					EnsureDecl(genericDecl);
					List<RefPtr<ExpressionSyntaxNode>> args;
					for (RefPtr<Decl> member : genericDecl->Members)
					{
						if (auto typeParam = member.As<GenericTypeParamDecl>())
						{
							if (!typeParam->initType.exp)
							{
								getSink()->diagnose(result.exp.Ptr(), Diagnostics::unimplemented, "can't fill in default for generic type parameter");
								result.type = ExpressionType::Error;
								return result;
							}

							// TODO: this is one place where syntax should get cloned!
							args.Add(typeParam->initType.exp);
						}
						else if (auto valParam = member.As<GenericValueParamDecl>())
						{
							if (!valParam->Expr)
							{
								getSink()->diagnose(result.exp.Ptr(), Diagnostics::unimplemented, "can't fill in default for generic type parameter");
								result.type = ExpressionType::Error;
								return result;
							}

							// TODO: this is one place where syntax should get cloned!
							args.Add(valParam->Expr);
						}
						else
						{
							// ignore non-parameter members
						}
					}

					result.type = InstantiateGenericType(genericDecl, args);
					return result;
				}
				else
				{
					// default case: we expect this to be a proper type
					return result;
				}
			}

			RefPtr<ExpressionSyntaxNode> CheckTerm(RefPtr<ExpressionSyntaxNode> term)
			{
				return term->Accept(this).As<ExpressionSyntaxNode>();
			}

			RefPtr<ExpressionSyntaxNode> VisitGenericType(GenericTypeSyntaxNode * typeNode) override
			{
				auto& base = typeNode->base;
				base = CheckTerm(base);
				auto& args = typeNode->Args;
				for (auto& arg : typeNode->Args)
				{
					arg = CheckTerm(arg);
				}

#if TIMREMOVED
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
#endif
				{
					auto baseDeclRef = base.As<DeclRefExpr>();

					if (!baseDeclRef)
					{
						getSink()->diagnose(typeNode, Diagnostics::unimplemented, "unexpected base term in generic app");
						typeResult = ExpressionType::Error;
						return typeNode;
					}
					auto decl = baseDeclRef->decl;

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
						auto type = InstantiateGenericType(genericDecl, args);
						typeResult = type;
						typeNode->Type = new TypeExpressionType(type);
						return typeNode;
					}
					else
					{
						// TODO: correct diagnostic here!
						getSink()->diagnose(typeNode, Diagnostics::unimplemented, "unexpected base term in generic app");
						typeResult = ExpressionType::Error;
						return typeNode;
					}
				}
			}
		public:
			RefPtr<ImportOperatorDefSyntaxNode> VisitImportOperatorDef(ImportOperatorDefSyntaxNode* op) override
			{
				currentImportOperator = op;
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
				return op;
			}

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
					comp->SetCheckState(DeclCheckState::Checked);
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
					EnsureDecl(op);
				}
				currentPipeline = nullptr;
				return pipeline;
			}

			virtual CoreLib::RefPtr<InterfaceSyntaxNode> VisitInterface(InterfaceSyntaxNode * interfaceNode) override
			{
				for (auto & comp : interfaceNode->GetComponents())
				{
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
					EnsureDecl(comp);
				}
				this->currentShader = nullptr;
			}

			bool MatchType_GenericType(String typeName, ExpressionType * valueType)
			{
				if (auto genericType = valueType->As<ImportOperatorGenericParamType>())
					return genericType->GenericTypeVar == typeName;
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
				if (comp->IsChecked(DeclCheckState::Checked)) return comp;

				// HACK(tfoley): Don't want to check these inside a template shader
				if (!currentShader)
					return comp;

				comp->SetCheckState(DeclCheckState::CheckingHeader);

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

				comp->SetCheckState(DeclCheckState::CheckedHeader);

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

				comp->SetCheckState(DeclCheckState::Checked);

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

			virtual RefPtr<GenericDecl> VisitGenericDecl(GenericDecl* genericDecl) override
			{
				// check the parameters
				for (auto m : genericDecl->Members)
				{
					if (auto typeParam = m.As<GenericTypeParamDecl>())
					{
						typeParam->initType = TranslateTypeNode(typeParam->initType);
					}
					else if (auto valParam = m.As<GenericValueParamDecl>())
					{
						// TODO: some real checking here...
						valParam->Type = TranslateTypeNode(valParam->Type);
					}
				}

				// check the nested declaration
				// TODO: this needs to be done in an appropriate environment...
				genericDecl->inner->Accept(this);
				return genericDecl;
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
					VisitStruct(s.Ptr());
				}

				// HACK(tfoley): Visiting all generic declarations here,
				// because otherwise they won't get visited.
				for (auto & g : program->GetMembersOfType<GenericDecl>())
				{
					VisitGenericDecl(g.Ptr());
				}

				for (auto & func : program->GetFunctions())
				{
					if (!func->IsChecked(DeclCheckState::Checked))
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
					EnsureDecl(func);
				}
				for (auto & pipeline : program->GetPipelines())
				{
					VisitPipeline(pipeline.Ptr());
				}
				for (auto & interfaceNode : program->GetInterfaces())
				{
					EnsureDecl(interfaceNode);
				}
				// build initial symbol table for shaders
				for (auto & shader : program->GetShaders())
				{
					if (!shader->IsChecked(DeclCheckState::Checked))
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
					if (!shader->IsChecked(DeclCheckState::CheckedHeader))
					{
						VisitShaderPass1(shader.Ptr());
						shader->SetCheckState(DeclCheckState::CheckedHeader);
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
						shader->SyntaxNode->SetCheckState(DeclCheckState::Checked);
						shader->SemanticallyChecked = true;
					}
				}

				// Force everything to be fully checked, just in case
				// Note that we don't just call this on the program,
				// because we'd end up recursing into this very code path...
				for (auto d : programNode->Members)
				{
					EnusreAllDeclsRec(d);
				}

				return programNode;
			}

			virtual RefPtr<StructSyntaxNode> VisitStruct(StructSyntaxNode * structNode) override
			{
				if (structNode->IsChecked(DeclCheckState::Checked))
					return structNode;
				structNode->SetCheckState(DeclCheckState::Checked);

				for (auto field : structNode->GetFields())
				{
					field->Type = CoerceToProperType(TranslateTypeNode(field->Type));
					field->SetCheckState(DeclCheckState::Checked);
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
				if (functionNode->IsChecked(DeclCheckState::Checked))
					return functionNode;

				VisitFunctionDeclaration(functionNode);
				functionNode->SetCheckState(DeclCheckState::Checked);

				if (!functionNode->IsExtern())
				{
					// TIM:HACK: members functions might not have an `InternalName`
					if (auto found = symbolTable->Functions.TryGetValue(functionNode->InternalName))
					{
						currentFunc = found->Ptr();
					}
					this->function = functionNode;
					if (functionNode->Body)
					{
						functionNode->Body->Accept(this);
					}
					this->function = NULL;
					currentFunc = nullptr;
				}
				return functionNode;
			}

			void VisitFunctionDeclaration(FunctionSyntaxNode *functionNode)
			{
				if (functionNode->IsChecked(DeclCheckState::CheckedHeader)) return;
				functionNode->SetCheckState(DeclCheckState::CheckingHeader);

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
					para->Type = CoerceToProperType(TranslateTypeNode(para->Type));
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
				functionNode->SetCheckState(DeclCheckState::CheckedHeader);
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
				if (auto baseShaderType = memberExpr->BaseExpression->Type->As<ShaderType>())
				{
					auto func = ResolveFunctionComponent(baseShaderType->Shader, memberExpr->MemberName, arguments);
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
							importExpr->Type = arguments[0]->Type;
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

				auto rsType = new ImportOperatorGenericParamType(currentImportOperator->TypeName.Content);
				project->Type = rsType;
				return project;
			}

			// Coerce an expression to a specific  type that it is expected to have in context
			RefPtr<ExpressionSyntaxNode> CoerceExprToType(
				RefPtr<ExpressionSyntaxNode>	expr,
				RefPtr<ExpressionType>			type)
			{
				if (MatchType_ValueReceiver(type.Ptr(), expr->Type.Ptr()))
				{
					// TODO: insert the conversion operation here...
					return expr;
				}
				else
				{
					getSink()->diagnose(expr, Diagnostics::typeMismatch, expr->Type, type);
					return expr;
				}
			}

			// Resolve a call to a function, represented here
			// by a symbol with a `FuncType` type.
			RefPtr<ExpressionSyntaxNode> ResolveFunctionApp(
				RefPtr<FuncType>			funcType,
				InvokeExpressionSyntaxNode*	appExpr)
			{
				// TODO(tfoley): Actual checking logic needs to go here...

				auto& args = appExpr->Arguments;
				List<RefPtr<ParameterSyntaxNode>> params;
				RefPtr<ExpressionType> resultType;
				if (auto funcDecl = funcType->decl)
				{
					EnsureDecl(funcDecl);

					params = funcDecl->GetParameters().ToArray();
					resultType = funcDecl->ReturnType;
				}
				else if (auto funcSym = funcType->Func)
				{
					auto funcDecl = funcSym->SyntaxNode;
					EnsureDecl(funcDecl);

					params = funcDecl->GetParameters().ToArray();
					resultType = funcDecl->ReturnType;
				}
				else if (auto componentFuncSym = funcType->Component)
				{
					auto componentFuncDecl = componentFuncSym->Implementations.First()->SyntaxNode;
					params = componentFuncDecl->GetParameters().ToArray();
					resultType = componentFuncDecl->Type;
				}

				auto argCount = args.Count();
				auto paramCount = params.Count();
				if (argCount != paramCount)
				{
					getSink()->diagnose(appExpr, Diagnostics::unimplemented, "wrong number of arguments for call");
					appExpr->Type = ExpressionType::Error;
					return appExpr;
				}

				for (int ii = 0; ii < argCount; ++ii)
				{
					auto arg = args[ii];
					auto param = params[ii];

					arg = CoerceExprToType(arg, param->Type);

					args[ii] = arg;
				}

				assert(resultType);
				appExpr->Type = resultType;
				return appExpr;
			}

			// Resolve a constructor call, formed by apply a type to arguments
			RefPtr<ExpressionSyntaxNode> ResolveConstructorApp(
				RefPtr<ExpressionType>		type,
				InvokeExpressionSyntaxNode*	appExpr)
			{
				// TODO(tfoley): Actual checking logic needs to go here...

				appExpr->Type = type;
				return appExpr;
			}


			//

			virtual void VisitExtensionDecl(ExtensionDecl* decl) override
			{
				decl->targetType = TranslateTypeNode(decl->targetType);

				// TODO: need to check that the target type names a declaration...

				if (auto targetDeclRefType = decl->targetType->As<DeclRefType>())
				{
					// Attach our extension to that type as a candidate...
					if (auto aggTypeDecl = dynamic_cast<AggTypeDecl*>(targetDeclRefType->decl))
					{
						decl->nextCandidateExtension = aggTypeDecl->candidateExtensions;
						aggTypeDecl->candidateExtensions = decl;
					}
					else
					{
						getSink()->diagnose(decl->targetType.exp, Diagnostics::unimplemented, "expected a nominal type here");
					}
				}
				else if (decl->targetType->Equals(ExpressionType::Error))
				{
					// there was an error, so ignore
				}
				else
				{
					getSink()->diagnose(decl->targetType.exp, Diagnostics::unimplemented, "expected a nominal type here");
				}

				// now check the members of the extension
				for (auto m : decl->Members)
				{
					EnsureDecl(m);
				}
			}

			virtual void VisitConstructorDecl(ConstructorDecl* decl) override
			{
				if (decl->IsChecked(DeclCheckState::Checked)) return;
				decl->SetCheckState(DeclCheckState::CheckingHeader);

				for (auto& paramDecl : decl->GetParameters())
				{
					paramDecl->Type = CoerceToProperType(TranslateTypeNode(paramDecl->Type));
				}
				decl->SetCheckState(DeclCheckState::CheckedHeader);

				// TODO(tfoley): check body
			}

			//

			struct OverloadCandidate
			{
				enum class Flavor
				{
					Func,
					ComponentFunc,
				};
				Flavor flavor;

				enum class Status
				{
					Unchecked,
					ArityChecked,
					TypeChecked,
					Appicable,
				};
				Status status = Status::Unchecked;

				union
				{
					FunctionDeclBase*		func;
					ComponentSyntaxNode*	componentFunc;
				};

				// The type of the result expression if this candidate is selected
				RefPtr<ExpressionType>	resultType;
			};



			// State related to overload resolution for a call
			// to an overloaded symbol
			struct OverloadResolveContext
			{
				enum class Mode
				{
					// We are just checking if a candidate works or not
					JustTrying,

					// We want to actually update the AST for a chosen candidate
					ForReal,
				};

				RefPtr<InvokeExpressionSyntaxNode> appExpr;

				OverloadCandidate bestCandidateStorage;
				OverloadCandidate*	bestCandidate = nullptr;
				bool ambiguous = false;
				Mode mode = Mode::JustTrying;
			};

			bool TryCheckOverloadCandidateArity(
				OverloadResolveContext&		context,
				OverloadCandidate const&	candidate)
			{
				int argCount = context.appExpr->Arguments.Count();
				int paramCount = 0;
				switch (candidate.flavor)
				{
				case OverloadCandidate::Flavor::Func:
					paramCount = candidate.func->GetParameters().Count();
					break;

				case OverloadCandidate::Flavor::ComponentFunc:
					paramCount = candidate.componentFunc->GetParameters().Count();
					break;
				}
				if (argCount != paramCount && context.mode != OverloadResolveContext::Mode::JustTrying)
				{
					getSink()->diagnose(context.appExpr, Diagnostics::unimplemented, "parameter and argument count mismatch");
				}

				return argCount == paramCount;
			}

			bool TryCheckOverloadCandidateTypes(
				OverloadResolveContext&		context,
				OverloadCandidate const&	candidate)
			{
				auto& args = context.appExpr->Arguments;
				int argCount = args.Count();

				List<RefPtr<ParameterSyntaxNode>> params;
				switch (candidate.flavor)
				{
				case OverloadCandidate::Flavor::Func:
					params = candidate.func->GetParameters().ToArray();
					break;

				case OverloadCandidate::Flavor::ComponentFunc:
					params = candidate.componentFunc->GetParameters().ToArray();
					break;
				}
				int paramCount = params.Count();
				assert(argCount == paramCount);

				bool success = true;
				for (int ii = 0; ii < argCount; ++ii)
				{
					auto arg = args[ii];
					auto param = params[ii];

					if (!MatchType_ValueReceiver(param->Type.Ptr(), arg->Type.Ptr()))
					{
						if (context.mode == OverloadResolveContext::Mode::JustTrying)
							return false;
						else
						{
							getSink()->diagnose(arg, Diagnostics::typeMismatch, arg->Type, param->Type);
							success = false;
						}
					}
				}
				return success;
			}

			bool TryCheckOverloadCandidateDirections(
				OverloadResolveContext&		context,
				OverloadCandidate const&	candidate)
			{
				// TODO(tfoley): check `in` and `out` markers, as needed.
				return true;
			}

			// Try to check an overload candidate, but bail out
			// if any step fails
			void TryCheckOverloadCandidate(
				OverloadResolveContext&		context,
				OverloadCandidate&			candidate)
			{
				if (!TryCheckOverloadCandidateArity(context, candidate))
					return;

				candidate.status = OverloadCandidate::Status::ArityChecked;
				if (!TryCheckOverloadCandidateTypes(context, candidate))
					return;

				candidate.status = OverloadCandidate::Status::TypeChecked;
				if (!TryCheckOverloadCandidateDirections(context, candidate))
					return;

				candidate.status = OverloadCandidate::Status::Appicable;
			}

			// Take an overload candidate that previously got through
			// `TryCheckOverloadCandidate` above, and try to finish
			// up the work and turn it into a real expression.
			//
			// If the candidate isn't actually applicable, this is
			// where we'd start reporting the issue(s).
			RefPtr<ExpressionSyntaxNode> CompleteOverloadCandidate(
				OverloadResolveContext&		context,
				OverloadCandidate&			candidate)
			{
				context.mode = OverloadResolveContext::Mode::ForReal;
				context.appExpr->Type = ExpressionType::Error;

				if (!TryCheckOverloadCandidateArity(context, candidate))
					return context.appExpr;

				if (!TryCheckOverloadCandidateTypes(context, candidate))
					return context.appExpr;

				if (!TryCheckOverloadCandidateDirections(context, candidate))
					return context.appExpr;

				context.appExpr->Type = candidate.resultType;
				return context.appExpr;
			}

			void AddOverloadCandidate(
				OverloadResolveContext& context,
				OverloadCandidate&		candidate)
			{
				// Try the candidate out, to see if it is applicable at all.
				TryCheckOverloadCandidate(context, candidate);

				if (!context.bestCandidate || candidate.status > context.bestCandidate->status)
				{
					// This candidate is the best one so far, so lets remember it
					context.bestCandidateStorage = candidate;
					context.bestCandidate = &context.bestCandidateStorage;
					context.ambiguous = false;
				}
				else if (candidate.status == OverloadCandidate::Status::Appicable)
				{
					// This is the case where we might have to consider more functions...
					throw "haven't implemented the case where we track multiple applicable overloads";
				}
				else if (candidate.status == context.bestCandidate->status)
				{
					// This candidate is just as applicable as the last, but
					// neither is actually usable.
					context.ambiguous = true;
				}
				else
				{
					// This candidate wasn't good enough to keep considering
				}
			}

			void AddFuncOverloadCandidate(
				RefPtr<FunctionDeclBase>	funcDecl,
				OverloadResolveContext&		context)
			{
				EnsureDecl(funcDecl);

				OverloadCandidate candidate;
				candidate.flavor = OverloadCandidate::Flavor::Func;
				candidate.func = funcDecl.Ptr();
				candidate.resultType = funcDecl->ReturnType;

				AddOverloadCandidate(context, candidate);
			}

			void AddComponentFuncOverloadCandidate(
				RefPtr<ShaderComponentSymbol>	componentFuncSym,
				OverloadResolveContext&			context)
			{
				auto componentFuncDecl = componentFuncSym->Implementations.First()->SyntaxNode.Ptr();

				OverloadCandidate candidate;
				candidate.flavor = OverloadCandidate::Flavor::ComponentFunc;
				candidate.componentFunc = componentFuncDecl;
				candidate.resultType = componentFuncDecl->Type;

				AddOverloadCandidate(context, candidate);
			}

			void AddFuncOverloadCandidate(
				RefPtr<FuncType>		funcType,
				OverloadResolveContext&	context)
			{
				if (funcType->decl)
				{
					AddFuncOverloadCandidate(funcType->decl, context);
				}
				else if (funcType->Func)
				{
					AddFuncOverloadCandidate(funcType->Func->SyntaxNode, context);
				}
				else if (funcType->Component)
				{
					AddComponentFuncOverloadCandidate(funcType->Component, context);
				}
			}

			void AddCtorOverloadCandidate(
				RefPtr<ExpressionType>	type,
				ConstructorDecl*		ctorDecl,
				OverloadResolveContext&	context)
			{
				OverloadCandidate candidate;
				candidate.flavor = OverloadCandidate::Flavor::Func;
				candidate.func = ctorDecl;
				candidate.resultType = type;

				AddOverloadCandidate(context, candidate);
			}

			// If the given declaration has generic parameters, then
			// return the corresponding `GenericDecl` that holds the
			// parameters, etc.
			GenericDecl* GetOuterGeneric(Decl* decl)
			{
				auto parentDecl = decl->ParentDecl;
				if (!parentDecl) return nullptr;
				auto parentGeneric = dynamic_cast<GenericDecl*>(parentDecl);
				return parentGeneric;
			}

			bool TryUnifyTypes(
				RefPtr<ExpressionType> fst,
				RefPtr<ExpressionType> snd)
			{
				if (fst->Equals(snd)) return true;

				if (auto fstDeclRefType = fst->As<DeclRefType>())
				{
					if (auto sndDeclRefType = snd->As<DeclRefType>())
					{
						auto fstDecl = fstDeclRefType->decl;
						auto sndDecl = sndDeclRefType->decl;

						// can't be unified if they refer to differnt declarations.
						if (fstDecl != sndDecl) return false;

						// next need to unify the "path" by which
						// they were referenced...
						//
						// TODO(tfoley): actually implement this
						return true;
					}
				}

				throw "unimplemented";
			}

			// Is the candidate extension declaration actually applicable to the given type
			bool IsExtensionApplicableToType(
				ExtensionDecl*			extDecl,
				RefPtr<ExpressionType>	type)
			{
				if (auto extGenericDecl = GetOuterGeneric(extDecl))
				{
					// we need to check whether we can unify the type on the extension
					// with the type in question.
					// This has to be done with a constraint solver.

					return TryUnifyTypes(extDecl->targetType, type);

					// TODO(tfoley): figure out how to check things here!!!
					throw "unimplemented";
					return true;
				}
				else
				{
					// The easy case is when the extension isn't
					// generic:
					return type->Equals(extDecl->targetType);
				}
			}

			void AddAggTypeOverloadCandidates(
				RefPtr<ExpressionType>	type,
				RefPtr<AggTypeDecl>		aggTypeDecl,
				OverloadResolveContext&	context)
			{
				for (auto ctor : aggTypeDecl->GetMembersOfType<ConstructorDecl>())
				{
					// now work through this candidate...
					AddCtorOverloadCandidate(type, ctor.Ptr(), context);
				}

				// Now walk through any extensions we can find for this type
				for (auto ext = aggTypeDecl->candidateExtensions; ext; ext = ext->nextCandidateExtension)
				{
					if (!IsExtensionApplicableToType(ext, type))
						continue;

					for (auto ctor : ext->GetMembersOfType<ConstructorDecl>())
					{
						// now work through this candidate...
						AddCtorOverloadCandidate(type, ctor.Ptr(), context);
					}
				}
			}

			void AddTypeOverloadCandidates(
				RefPtr<ExpressionType>	type,
				OverloadResolveContext&	context)
			{
				if (auto declRefType = type->As<DeclRefType>())
				{
					if (auto aggTypeDecl = dynamic_cast<AggTypeDecl*>(declRefType->decl))
					{
						AddAggTypeOverloadCandidates(type, aggTypeDecl, context);
					}
				}
			}

			void AddDeclOverloadCandidates(
				RefPtr<Decl>			decl,
				OverloadResolveContext&	context)
			{
				if (auto funcDecl = decl.As<FunctionDeclBase>())
				{
					AddFuncOverloadCandidate(funcDecl, context);
				}
				else if (auto aggTypeDecl = decl.As<AggTypeDecl>())
				{
					auto type = CreateDeclRefType(aggTypeDecl.Ptr());
					AddAggTypeOverloadCandidates(type, aggTypeDecl, context);
				}
				else
				{
					// TODO(tfoley): any other cases needed here?
				}
			}

			LookupResult GetNext(LookupResult const& inResult)
			{
				if (!inResult.isValid()) return inResult;

				LookupResult result = inResult;
				result.index++;
				return DoLookup(result.decl->Name.Content, result);
			}

			void AddOverloadCandidates(
				RefPtr<ExpressionSyntaxNode>	funcExpr,
				OverloadResolveContext&			context)
			{
				auto funcExprType = funcExpr->Type;
				if (auto funcType = funcExprType->As<FuncType>())
				{
					AddFuncOverloadCandidate(funcType, context);
				}
				else if (auto typeType = funcExprType->As<TypeExpressionType>())
				{
					// The expression named a type, so we have a constructor call
					// on our hands.
					AddTypeOverloadCandidates(typeType->type, context);
				}
				else if (auto overloadedExpr = funcExpr.As<OverloadedExpr>())
				{
					auto lookupResult = overloadedExpr->lookupResult;
					while (lookupResult.isValid())
					{
						AddDeclOverloadCandidates(lookupResult.decl, context);

						lookupResult = GetNext(lookupResult);
					}
				}
			}



			RefPtr<ExpressionSyntaxNode> ResolveInvoke(InvokeExpressionSyntaxNode * expr)
			{
				// Look at the base expression for the call, and figure out how
				// to invoke it.
				auto funcExpr = expr->FunctionExpr;
				auto funcExprType = funcExpr->Type;
				if (funcExprType->Equals(ExpressionType::Error))
				{
					expr->Type = ExpressionType::Error;
					return expr;
				}

				OverloadResolveContext context;
				context.appExpr = expr;
				AddOverloadCandidates(funcExpr, context);

				if (!context.bestCandidate)
				{
					// Nothing at all was found that we could even consider invoking
					getSink()->diagnose(expr->FunctionExpr, Diagnostics::expectedFunction);
					expr->Type = ExpressionType::Error;
					return expr;
				}

				if (!context.ambiguous)
				{
					// There was one best candidate, even if it might not have been
					// applicable in the end.
					// We will report errors for this one candidate, then, to give
					// the user the most help we can.
					return CompleteOverloadCandidate(context, *context.bestCandidate);
				}

				// Otherwise things were ambiguous, and we need to report it.
				if (context.bestCandidate->status == OverloadCandidate::Status::Appicable)
				{
					// There were multiple applicable candidates, so we need to
					// report them.
					getSink()->diagnose(expr, Diagnostics::unimplemented, "ambiguous overloaded call");
					expr->Type = ExpressionType::Error;
					return expr;
				}
				else
				{
					// There were multple equally-good candidates, but none actually usable.
					getSink()->diagnose(expr, Diagnostics::unimplemented, "no applicable overload found");
					expr->Type = ExpressionType::Error;
					return expr;
				}
			}

			RefPtr<ExpressionSyntaxNode> CheckExpr(RefPtr<ExpressionSyntaxNode> expr)
			{
				return expr->Accept(this).As<ExpressionSyntaxNode>();
			}

			virtual RefPtr<ExpressionSyntaxNode> VisitInvokeExpression(InvokeExpressionSyntaxNode *expr) override
			{
				// check the base expression first
				expr->FunctionExpr = CheckExpr(expr->FunctionExpr);

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

			// TODO: need to figure out how to unify this with the logic
			// in the generic case...
			static DeclRefType* CreateDeclRefType(Decl* decl)
			{
				if (auto builtinMod = decl->FindModifier<BuiltinTypeModifier>())
				{
					auto type = new BasicExpressionType(builtinMod->tag);
					type->decl = decl;
					return type;
				}
				else if (auto magicMod = decl->FindModifier<MagicTypeModifier>())
				{
					if (magicMod->name == "SamplerState")
					{
						auto type = new SamplerStateType();
						type->decl = decl;
						type->flavor = SamplerStateType::Flavor(magicMod->tag);
						return type;
					}
					else
					{
						throw "unimplemented";
					}
				}
				else
				{
					return new DeclRefType(decl);
				}
			}

			bool DeclPassesLookupMask(Decl* decl, LookupMask mask)
			{
				// type declarations
				if(auto aggTypeDecl = dynamic_cast<AggTypeDecl*>(decl))
				{
					return int(mask) & int(LookupMask::Type);
				}
				else if(auto simpleTypeDecl = dynamic_cast<SimpleTypeDecl*>(decl))
				{
					return int(mask) & int(LookupMask::Type);
				}
				// function declarations
				else if(auto funcDecl = dynamic_cast<FunctionDeclBase*>(decl))
				{
					return int(mask) & int(LookupMask::Function);
				}
				// component declarations have kind of odd rules
				else if(auto componentDecl = dynamic_cast<ComponentSyntaxNode*>(decl))
				{
					if (componentDecl->IsComponentFunction())
					{
						return int(mask) & int(LookupMask::Function);
					}
					else
					{
						return int(mask) & int(LookupMask::Value);
					}
				}

				// default behavior is to assume a value declaration
				// (no overloading allowed)

				return int(mask) & int(LookupMask::Value);
			}

			LookupResult DoLookup(String const& name, LookupResult inResult)
			{
				LookupResult result;
				result.mask = inResult.mask;

				ContainerDecl* scope = inResult.scope;
				int index = inResult.index;
				while (scope)
				{
					auto memberCount = scope->Members.Count();

					for (;index < memberCount; index++)
					{
						auto member = scope->Members[index].Ptr();
						if (member->Name.Content != name)
							continue;

						// TODO: filter based on our mask
						if (!DeclPassesLookupMask(member, result.mask))
							continue;

						// if we had previously found a result
						if (result.isValid())
						{
							// we now have a potentially overloaded result,
							// and can return with this knowledge
							result.flags |= LookupResult::Flags::Overloaded;
							return result;
						}
						else
						{
							// this is the first result!
							result.decl = member;
							result.scope = scope;
							result.index = index;

							// TODO: need to establish a mask for subsequent
							// lookup...
						}
					}

					// we reached the end of a scope, so if we found
					// anything inside that scope, we should return now
					// rather than continue to search parent scopes
					if (result.isValid())
					{
						return result;
					}

					// Otherwise, we proceed to the next scope up.
					scope = scope->ParentDecl;
					index = 0;
				}

				// If we run out of scopes, then we are done.
				return result;
			}

			LookupResult LookUp(String const& name, ContainerDecl* scope)
			{
				LookupResult result;
				result.scope = scope;
				return DoLookup(name, result);
			}

			virtual RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode *expr) override
			{
				ShaderUsing shaderObj;
				expr->Type = ExpressionType::Error;

				auto lookupResult = LookUp(expr->Variable, expr->scope);
				if (lookupResult.isValid())
				{
					// Found at least one declaration, but did we find many?
					if (lookupResult.isOverloaded())
					{
						auto overloadedExpr = new OverloadedExpr();
						overloadedExpr->Type = ExpressionType::Overloaded;
						overloadedExpr->lookupResult = lookupResult;
						return overloadedExpr;
					}
					else
					{
						// Only a single decl, that's good
						auto decl = lookupResult.decl;
						expr->decl = decl;
						expr->Type = GetTypeForDeclRef(decl);
						return expr;
					}
				}

				// Ad hoc lookup rules for cases where the scope-based lookup currently doesn't apply.
				if (currentShader && currentShader->ShaderObjects.TryGetValue(expr->Variable, shaderObj))
				{
					auto basicType = new ShaderType(shaderObj.Shader, nullptr);
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

			// Get the type to use when referencing a declaration
			QualType GetTypeForDeclRef(Decl* decl)
			{
				// We need to insert an appropriate type for the expression, based on
				// what we found.
				if (auto varDecl = dynamic_cast<VarDeclBase*>(decl))
				{
					QualType qualType;
					qualType.type = varDecl->Type;
					qualType.IsLeftValue = true; // TODO(tfoley): allow explicit `const` or `let` variables
					return qualType;
				}
				else if (auto compDecl = dynamic_cast<ComponentSyntaxNode*>(decl))
				{
					if (compDecl->IsComponentFunction())
					{
						// TODO: need to implement this case
					}
					else
					{
						return compDecl->Type.type;
					}
				}
				else if (auto typeAliasDecl = dynamic_cast<TypeDefDecl*>(decl))
				{
					auto type = new NamedExpressionType(typeAliasDecl);
					typeResult = type;
					return new TypeExpressionType(type);
				}
				else if (auto aggTypeDecl = dynamic_cast<AggTypeDecl*>(decl))
				{
					auto type = CreateDeclRefType(aggTypeDecl);
					typeResult = type;
					return new TypeExpressionType(type);
				}
				else if (auto simpleTypeDecl = dynamic_cast<SimpleTypeDecl*>(decl))
				{
					auto type = CreateDeclRefType(simpleTypeDecl);
					typeResult = type;
					return new TypeExpressionType(type);
				}
				else if (auto genericDecl = dynamic_cast<GenericDecl*>(decl))
				{
					auto type = new GenericDeclRefType(genericDecl);
					typeResult = type;
					return new TypeExpressionType(type);
				}
				else if (auto funcDecl = dynamic_cast<FunctionDeclBase*>(decl))
				{
					auto type = new FuncType();
					type->decl = funcDecl;
					return type;
				}

				getSink()->diagnose(decl, Diagnostics::unimplemented, "cannot form reference to this kind of declaration");
				return ExpressionType::Error;
			}

			virtual RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * expr) override
			{
				expr->BaseExpression = expr->BaseExpression->Accept(this).As<ExpressionSyntaxNode>();
				auto & baseType = expr->BaseExpression->Type;
				if (auto declRefType = baseType->AsDeclRefType())
				{
					if (auto aggTypeDecl = dynamic_cast<AggTypeDecl*>(declRefType->decl))
					{
						// Checking of the type must be complete before we can reference its members safely
						EnsureDecl(aggTypeDecl, DeclCheckState::Checked);

						// TODO(tfoley): It is unfortunate that the lookup strategy
						// here isn't unified with the ordinary `Scope` case.
						// In particular, if we add support for "transparent" declarations,
						// etc. here then we would need to add them in ordinary lookup
						// as well.

						Decl* memberDecl = nullptr; // The first declaration we found, if any
						Decl* secondDecl = nullptr; // Another declaration with the same name, if any
						for (auto m : aggTypeDecl->Members)
						{
							if (m->Name.Content != expr->MemberName)
								continue;

							if (!memberDecl)
							{
								memberDecl = m.Ptr();
							}
							else
							{
								secondDecl = m.Ptr();
								break;
							}
						}

						// If we didn't find any member, then we signal an error
						if (!memberDecl)
						{
							expr->Type = ExpressionType::Error;
							getSink()->diagnose(expr, Diagnostics::noMemberOfNameInType, expr->MemberName, baseType);
							return expr;
						}

						// If we found only a single member, then we are fine
						if (!secondDecl)
						{
							expr->Type = GetTypeForDeclRef(memberDecl);

							// When referencing a member variable, the result is an l-value
							// if and only if the base expression was.
							if (auto memberVarDecl = dynamic_cast<VarDeclBase*>(memberDecl))
							{
								expr->Type.IsLeftValue = expr->BaseExpression->Type.IsLeftValue;
							}
							return expr;
						}

						// We found multiple members with the same name, and need
						// to resolve the embiguity at some point...
						expr->Type = ExpressionType::Error;
						getSink()->diagnose(expr, Diagnostics::unimplemented, "ambiguous member reference");
						return expr;

#if 0

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
#endif
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
				else if (auto baseShaderType = baseType->As<ShaderType>())
				{
					ShaderUsing shaderObj;
					auto refComp = baseShaderType->Shader->ResolveComponentReference(expr->MemberName);
					if (refComp.IsAccessible)
						expr->Type = refComp.Component->Type->DataType;
					else if (baseShaderType->Shader->ShaderObjects.TryGetValue(expr->MemberName, shaderObj))
					{
						if (shaderObj.IsPublic)
						{
							auto shaderType = new ShaderType(shaderObj.Shader, nullptr);
							expr->Type = shaderType;
						}
						else
							expr->Type = ExpressionType::Error;
					}
					else
						expr->Type = ExpressionType::Error;
				}
				// All remaining cases assume we have a `BasicType`
				else if (!baseType->AsBasicType())
					expr->Type = ExpressionType::Error;
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