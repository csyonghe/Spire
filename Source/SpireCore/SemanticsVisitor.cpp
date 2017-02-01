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
				DeclRef							declRef,
				RefPtr<ExpressionSyntaxNode>	baseExpr,
				RefPtr<ExpressionSyntaxNode>	originalExpr)
			{
				if (baseExpr)
				{
					auto expr = new MemberExpressionSyntaxNode();
					expr->Position = originalExpr->Position;
					expr->BaseExpression = baseExpr;
					expr->MemberName = declRef.GetName();
					expr->Type = GetTypeForDeclRef(declRef);
					expr->declRef = declRef;
					return expr;
				}
				else
				{
					auto expr = new VarExpressionSyntaxNode();
					expr->Position = originalExpr->Position;
					expr->Variable = declRef.GetName();
					expr->Type = GetTypeForDeclRef(declRef);
					expr->declRef = declRef;
					return expr;
				}
			}

			LookupResult RefineLookup(LookupResult const& inResult, LookupMask mask)
			{
				if (!inResult.isValid()) return inResult;
				if (!inResult.isOverloaded()) return inResult;

				LookupResult result;
				for (auto item : inResult.items)
				{
					if (!DeclPassesLookupMask(item.declRef.GetDecl(), mask))
						continue;

					AddToLookupResult(result, item);
				}
				return result;
			}

			RefPtr<ExpressionSyntaxNode> ConstructDerefExpr(
				RefPtr<ExpressionSyntaxNode> base,
				RefPtr<ExpressionSyntaxNode> originalExpr)
			{
				auto ptrLikeType = base->Type->As<PointerLikeType>();
				assert(ptrLikeType);

				auto derefExpr = new DerefExpr();
				derefExpr->Position = originalExpr->Position;
				derefExpr->base = base;
				derefExpr->Type = ptrLikeType->elementType;

				// TODO(tfoley): handle l-value status here

				return derefExpr;
			}

			RefPtr<ExpressionSyntaxNode> ConstructLookupResultExpr(
				LookupResultItem const&			item,
				RefPtr<ExpressionSyntaxNode>	baseExpr,
				RefPtr<ExpressionSyntaxNode>	originalExpr)
			{
				// If we collected any breadcrumbs, then these represent
				// additional segments of the lookup path that we need
				// to expand here.
				auto bb = baseExpr;
				for (auto breadcrumb = item.breadcrumbs; breadcrumb; breadcrumb = breadcrumb->next)
				{
					switch (breadcrumb->kind)
					{
					case LookupResultItem::Breadcrumb::Kind::Member:
						bb = ConstructDeclRefExpr(breadcrumb->declRef, bb, originalExpr);
						break;
					case LookupResultItem::Breadcrumb::Kind::Deref:
						bb = ConstructDerefExpr(bb, originalExpr);
						break;
					default:
						SPIRE_UNREACHABLE("all cases handle");
					}
				}

				return ConstructDeclRefExpr(item.declRef, bb, originalExpr);
			}

			RefPtr<ExpressionSyntaxNode> ResolveOverloadedExpr(RefPtr<OverloadedExpr> overloadedExpr, LookupMask mask)
			{
				auto lookupResult = overloadedExpr->lookupResult2;
				assert(lookupResult.isValid() && lookupResult.isOverloaded());

				// Take the lookup result we had, and refine it based on what is expected in context.
				lookupResult = RefineLookup(lookupResult, mask);

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
				return ConstructLookupResultExpr(lookupResult.item, overloadedExpr->base, overloadedExpr);
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

			RefPtr<Val> ExtractGenericArgVal(RefPtr<ExpressionSyntaxNode> exp)
			{
				if (auto overloadedExpr = exp.As<OverloadedExpr>())
				{
					// assume that if it is overloaded, we want a type
					exp = ResolveOverloadedExpr(overloadedExpr, LookupMask::Type);
				}

				if (auto typeType = exp->Type->As<TypeExpressionType>())
				{
					return typeType->type;
				}
				else if (exp->Type->Equals(ExpressionType::Error))
				{
					return exp->Type.type;
				}
				else
				{
					int val = ExtractGenericArgInteger(exp);
					return new IntVal(val);
				}
			}

			// Construct a type reprsenting the instantiation of
			// the given generic declaration for the given arguments.
			// The arguments should already be checked against
			// the declaration.
			RefPtr<ExpressionType> InstantiateGenericType(
				GenericDeclRef								genericDeclRef,
				List<RefPtr<ExpressionSyntaxNode>> const&	args)
			{
				RefPtr<Substitutions> subst = new Substitutions();
				subst->genericDecl = genericDeclRef.GetDecl();
				subst->outer = genericDeclRef.substitutions;

				for (auto argExpr : args)
				{
					subst->args.Add(ExtractGenericArgVal(argExpr));
				}

				DeclRef innerDeclRef;
				innerDeclRef.decl = genericDeclRef.GetInner();
				innerDeclRef.substitutions = subst;

				return DeclRefType::Create(innerDeclRef);
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

			// A "proper" type is one that can be used as the type of an expression.
			// Put simply, it can be a concrete type like `int`, or a generic
			// type that is applied to arguments, like `Texture2D<float4>`.
			// The type `void` is also a proper type, since we can have expressions
			// that return a `void` result (e.g., many function calls).
			//
			// A "non-proper" type is any type that can't actually have values.
			// A simple example of this in C++ is `std::vector` - you can't have
			// a value of this type.
			//
			// Part of what this function does is give errors if somebody tries
			// to use a non-proper type as the type of a variable (or anything
			// else that needs a proper type).
			//
			// The other thing it handles is the fact that HLSL lets you use
			// the name of a non-proper type, and then have the compiler fill
			// in the default values for its type arguments (e.g., a variable
			// given type `Texture2D` will actually have type `Texture2D<float4>`).
			TypeExp CoerceToProperType(TypeExp const& typeExp)
			{
				TypeExp result = typeExp;
				ExpressionType* type = result.type.Ptr();
				if (auto genericDeclRefType = type->As<GenericDeclRefType>())
				{
					// We are using a reference to a generic declaration as a concrete
					// type. This means we should substitute in any default parameter values
					// if they are available.
					//
					// TODO(tfoley): A more expressive type system would substitute in
					// "fresh" variables and then solve for their values...
					//

					auto genericDeclRef = genericDeclRefType->GetDeclRef();
					EnsureDecl(genericDeclRef.decl);
					List<RefPtr<ExpressionSyntaxNode>> args;
					for (RefPtr<Decl> member : genericDeclRef.GetDecl()->Members)
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

					result.type = InstantiateGenericType(genericDeclRef, args);
					return result;
				}
				else
				{
					// default case: we expect this to be a proper type
					return result;
				}
			}

			// Check a type, and coerce it to be proper
			TypeExp CheckProperType(TypeExp typeExp)
			{
				return CoerceToProperType(TranslateTypeNode(typeExp));
			}

			// For our purposes, a "usable" type is one that can be
			// used to declare a function parameter, variable, etc.
			// These turn out to be all the proper types except
			// `void`.
			//
			// TODO(tfoley): consider just allowing `void` as a
			// simple example of a "unit" type, and get rid of
			// this check.
			TypeExp CoerceToUsableType(TypeExp const& typeExp)
			{
				TypeExp result = CoerceToProperType(typeExp);
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
				}
				return result;
			}

			// Check a type, and coerce it to be usable
			TypeExp CheckUsableType(TypeExp typeExp)
			{
				return CoerceToUsableType(TranslateTypeNode(typeExp));
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
					auto baseDeclRefExpr = base.As<DeclRefExpr>();

					if (!baseDeclRefExpr)
					{
						getSink()->diagnose(typeNode, Diagnostics::unimplemented, "unexpected base term in generic app");
						typeResult = ExpressionType::Error;
						return typeNode;
					}
					auto declRef = baseDeclRefExpr->declRef;

					if (auto genericDeclRef = declRef.As<GenericDeclRef>())
					{
						int argCount = typeNode->Args.Count();
						int argIndex = 0;
						for (RefPtr<Decl> member : genericDeclRef.GetDecl()->Members)
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
						auto type = InstantiateGenericType(genericDeclRef, args);
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
					para->Type = CheckUsableType(para->Type);
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
					comp->Type = CheckProperType(comp->Type);
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
						param->Type = CheckUsableType(param->Type);
						if (param->Expr)
							getSink()->diagnose(param->Expr->Position, Diagnostics::defaultParamNotAllowedInInterface, param->Name);
					}
					comp->Type = CheckProperType(comp->Type);
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
						comp->Type = CheckProperType(comp->Type);
						if (comp->IsRequire())
						{
							shaderSymbol->IsAbstract = true;
							if (!shaderSymbol->SyntaxNode->IsModule)
							{
								getSink()->diagnose(shaderSymbol->SyntaxNode, Diagnostics::parametersOnlyAllowedInModules);
							}
						}
						for (auto & param : comp->GetParameters())
							param->Type = CheckUsableType(param->Type);
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

			typedef unsigned int ConversionCost;
			enum : ConversionCost
			{
				kConversionCost_None = 0,
				kConversionCost_Promotion = 10,
				kConversionCost_Conversion = 20,

				kConversionCost_ScalarToVector = 1,
			};

			enum BaseTypeConversionKind : uint8_t
			{
				kBaseTypeConversionKind_Signed,
				kBaseTypeConversionKind_Unsigned,
				kBaseTypeConversionKind_Float,
				kBaseTypeConversionKind_Error,
			};

			enum BaseTypeConversionRank : uint8_t
			{
				kBaseTypeConversionRank_Bool,
				kBaseTypeConversionRank_Int8,
				kBaseTypeConversionRank_Int16,
				kBaseTypeConversionRank_Int32,
				kBaseTypeConversionRank_IntPtr,
				kBaseTypeConversionRank_Int64,
				kBaseTypeConversionRank_Error,
			};

			struct BaseTypeConversionInfo
			{
				BaseTypeConversionKind	kind;
				BaseTypeConversionRank	rank;
			};
			static BaseTypeConversionInfo GetBaseTypeConversionInfo(BaseType baseType)
			{
				switch (baseType)
				{
				#define CASE(TAG, KIND, RANK) \
					case BaseType::TAG: { BaseTypeConversionInfo info = {kBaseTypeConversionKind_##KIND, kBaseTypeConversionRank_##RANK}; return info; } break

					CASE(Bool, Unsigned, Bool);
					CASE(Int, Signed, Int32);
					CASE(UInt, Unsigned, Int32);
					CASE(Float, Float, Int32);
					CASE(Void, Error, Error);
					CASE(Error, Error, Error);

				#undef CASE

				default:
					break;
				}
				SPIRE_UNREACHABLE("all cases handled");
			}

			// Central engine for implementing implicit coercion logic
			bool TryCoerceImpl(
				RefPtr<ExpressionType>			toType,		// the target type for conversion
				RefPtr<ExpressionSyntaxNode>*	outToExpr,	// (optional) a place to stuff the target expression
				RefPtr<ExpressionType>			fromType,	// the source type for the conversion
				RefPtr<ExpressionSyntaxNode>	fromExpr,	// the source expression
				ConversionCost*					outCost)	// (optional) a place to stuff the conversion cost
			{
				// Easy case: the types are equal
				if (toType->Equals(fromType))
				{
					if (outToExpr)
						*outToExpr = fromExpr;
					if (outCost)
						*outCost = kConversionCost_None;
					return true;
				}

				// TODO(tfoley): catch error types here, and route appropriately

				// Handle the special case of constraint variables here.
				//
				// Basically, if any type constraint variables are involved, we
				// just record the fact that they have been constrained to be
				// equal, and then act as if the coercion succeeded.
				// We'll check the actual result later on, when we solve for
				// the constraint variables.
				if (toType->As<ConstraintVarType>() ||fromType->As<ConstraintVarType>() )
				{
					RegisterCoercionConstraint(toType, fromType);
					return true;
				}

				//

				if (auto toBasicType = toType->AsBasicType())
				{
					if (auto fromBasicType = fromType->AsBasicType())
					{
						// Conversions between base types are always allowed,
						// and the only question is what the cost will be.

						auto toInfo = GetBaseTypeConversionInfo(toBasicType->BaseType);
						auto fromInfo = GetBaseTypeConversionInfo(fromBasicType->BaseType);

						if (outToExpr)
							*outToExpr = CreateImplicitCastExpr(toType, fromExpr);

						if (outCost)
						{
							if (toInfo.kind == fromInfo.kind && toInfo.rank > fromInfo.rank)
							{
								*outCost = kConversionCost_Promotion;
							}
							else
							{
								*outCost = kConversionCost_Conversion;
							}
						}

						return true;
					}
				}

				if (auto toVectorType = toType->AsVectorType())
				{
					if (auto fromVectorType = fromType->AsVectorType())
					{
						// Conversion between vector types.

						// If element counts don't match, then bail:
						if (toVectorType->elementCount != fromVectorType->elementCount)
							return false;

						// Otherwise, if we can convert the element types, we are golden
						ConversionCost elementCost;
						if (CanCoerce(toVectorType->elementType, fromVectorType->elementType, &elementCost))
						{
							if (outToExpr)
								*outToExpr = CreateImplicitCastExpr(toType, fromExpr);
							if (outCost)
								*outCost = elementCost;
							return true;
						}
					}
					else if (auto fromScalarType = fromType->AsBasicType())
					{
						// Conversion from scalar to vector.
						// Should allow as long as we can coerce the scalar to our element type.
						ConversionCost elementCost;
						if (CanCoerce(toVectorType->elementType, fromScalarType, &elementCost))
						{
							if (outToExpr)
								*outToExpr = CreateImplicitCastExpr(toType, fromExpr);
							if (outCost)
								*outCost = elementCost + kConversionCost_ScalarToVector;
							return true;
						}
					}
				}

				// TODO: more cases!

				return false;
			}

			// Check whether a type coercion is possible
			bool CanCoerce(
				RefPtr<ExpressionType>			toType,			// the target type for conversion
				RefPtr<ExpressionType>			fromType,		// the source type for the conversion
				ConversionCost*					outCost = 0)	// (optional) a place to stuff the conversion cost
			{
				return TryCoerceImpl(
					toType,
					nullptr,
					fromType,
					nullptr,
					outCost);
			}

			// Perform type coercion, and emit errors if it isn't possible
			RefPtr<ExpressionSyntaxNode> Coerce(
				RefPtr<ExpressionType>			toType,
				RefPtr<ExpressionSyntaxNode>	fromExpr)
			{
				RefPtr<ExpressionSyntaxNode> expr;
				if (!TryCoerceImpl(
					toType,
					&expr,
					fromExpr->Type.Ptr(),
					fromExpr.Ptr(),
					nullptr))
				{
					getSink()->diagnose(fromExpr->Position, Diagnostics::typeMismatch, toType, fromExpr->Type);
				}
				return expr;
			}

			RefPtr<ExpressionSyntaxNode> CreateImplicitCastExpr(
				RefPtr<ExpressionType>			toType,
				RefPtr<ExpressionSyntaxNode>	fromExpr)
			{
				auto castExpr = new TypeCastExpressionSyntaxNode();
				castExpr->Position = fromExpr->Position;
				castExpr->TargetType.type = toType;
				castExpr->Type = toType;
				castExpr->Expression = fromExpr;
				return castExpr;
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
					comp->Expression = Coerce(compSym->Type->DataType, comp->Expression);
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
						typeParam->initType = CheckProperType(typeParam->initType);
					}
					else if (auto valParam = m.As<GenericValueParamDecl>())
					{
						// TODO: some real checking here...
						valParam->Type = CheckUsableType(valParam->Type);
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
					field->Type = CheckUsableType(field->Type);
					field->SetCheckState(DeclCheckState::Checked);
				}
				return structNode;
			}

			virtual RefPtr<TypeDefDecl> VisitTypeDefDecl(TypeDefDecl* decl) override
			{
				if (decl->IsChecked(DeclCheckState::Checked)) return decl;

				decl->SetCheckState(DeclCheckState::CheckingHeader);
				decl->Type = CheckProperType(decl->Type);
				decl->SetCheckState(DeclCheckState::Checked);
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
				auto returnType = CheckProperType(functionNode->ReturnType);
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
					para->Type = CheckUsableType(para->Type);
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
						if (function)
						{
							stmt->Expression = Coerce(function->ReturnType, stmt->Expression);
						}
						else if (currentComp)
						{
							stmt->Expression = Coerce(currentComp->Type->DataType, stmt->Expression);
						}
						else if (currentImportOperator)
						{
							// TODO(tfoley): fix this up to use new infrastructure
							if (!MatchType_GenericType(currentImportOperator->TypeName.Content, stmt->Expression->Type.Ptr()))
								getSink()->diagnose(stmt, Diagnostics::importOperatorReturnTypeMismatch, stmt->Expression->Type, currentImportOperator->TypeName);
						}
						else
						{
							// TODO(tfoley): this case currently gets triggered for member functions,
							// which aren't being checked consistently (because of the whole symbol
							// table idea getting in the way).

//							getSink()->diagnose(stmt, Diagnostics::unimplemented, "case for return stmt");
						}
					}
				}
				return stmt;
			}

			virtual RefPtr<Variable> VisitDeclrVariable(Variable* varDecl)
			{
				TypeExp typeExp = CheckUsableType(varDecl->Type);
				if (typeExp.type->GetBindableResourceType() != BindableResourceType::NonBindable)
				{
					// We don't want to allow bindable resource types as local variables (at least for now).
					auto parentDecl = varDecl->ParentDecl;
					if (auto parentScopeDecl = dynamic_cast<ScopeDecl*>(parentDecl))
					{
						getSink()->diagnose(varDecl->Type, Diagnostics::invalidTypeForLocalVariable);
					}
				}
				else if (auto declRefType = typeExp.type->AsDeclRefType())
				{
					if (auto worldDeclRef = declRefType->declRef.As<WorldDeclRef>())
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
					varDecl->Expr = Coerce(varDecl->Type, varDecl->Expr);
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

					// TODO(tfoley): Need to actual insert coercion here...
					if(CanCoerce(leftType, expr->Type))
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

					auto elementType = CoerceToUsableType(TypeExp(expr->BaseExpression, baseTypeType->type));
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

					bool isValid = false;
#if TIMREMOVED
					// TODO(tfoley): need to handle the indexing logic for these types...
					bool isValid = baseExprType->AsGenericType() &&
						(baseExprType->AsGenericType()->GenericTypeName == "StructuredBuffer" ||
							baseExprType->AsGenericType()->GenericTypeName == "RWStructuredBuffer" ||
							baseExprType->AsGenericType()->GenericTypeName == "PackedBuffer");
#else
					isValid = isValid || baseExprType->As<ArrayLikeType>();
#endif
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
				else if (auto bufferType = expr->BaseExpression->Type->As<ArrayLikeType>())
				{
					expr->Type = bufferType->elementType;
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
#if TIMREMOVED
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
#else
				throw "dead code";
#endif
			}

			bool IsValidWorldTypeForProjection(
				ProjectExpressionSyntaxNode*	projection,
				String const&					expectedWorld)
			{
				auto baseType = projection->BaseExpression->Type;
				auto declRefType = baseType->AsDeclRefType();
				if (!declRefType)
					return false;

				auto worldDeclRef = declRefType->declRef.As<WorldDeclRef>();
				if (!worldDeclRef)
					return false;

				// TODO(tfoley): Doing this is a string-based check is wrong...
				return worldDeclRef.GetName() == expectedWorld;
			}

			RefPtr<ExpressionSyntaxNode> VisitProject(ProjectExpressionSyntaxNode * project) override
			{
				if (currentImportOperator == nullptr)
				{
					getSink()->diagnose(project, Diagnostics::projectionOutsideImportOperator);
					return project;
				}
				project->BaseExpression = CheckExpr(project->BaseExpression);

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
				// TODO(tfoley): clean this up so there is only one version...
				return Coerce(type, expr);
			}

			// Resolve a call to a function, represented here
			// by a symbol with a `FuncType` type.
			RefPtr<ExpressionSyntaxNode> ResolveFunctionApp(
				RefPtr<FuncType>			funcType,
				InvokeExpressionSyntaxNode*	appExpr)
			{
				// TODO(tfoley): Actual checking logic needs to go here...
#if 0
				auto& args = appExpr->Arguments;
				List<RefPtr<ParameterSyntaxNode>> params;
				RefPtr<ExpressionType> resultType;
				if (auto funcDeclRef = funcType->declRef)
				{
					EnsureDecl(funcDeclRef.GetDecl());

					params = funcDeclRef->GetParameters().ToArray();
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
#else
				throw "unimplemented";
#endif
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
				if (decl->IsChecked(DeclCheckState::Checked)) return;

				decl->SetCheckState(DeclCheckState::CheckingHeader);
				decl->targetType = CheckProperType(decl->targetType);

				// TODO: need to check that the target type names a declaration...

				if (auto targetDeclRefType = decl->targetType->As<DeclRefType>())
				{
					// Attach our extension to that type as a candidate...
					if (auto aggTypeDeclRef = targetDeclRefType->declRef.As<AggTypeDeclRef>())
					{
						auto aggTypeDecl = aggTypeDeclRef.GetDecl();
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

				decl->SetCheckState(DeclCheckState::CheckedHeader);

				// now check the members of the extension
				for (auto m : decl->Members)
				{
					EnsureDecl(m);
				}

				decl->SetCheckState(DeclCheckState::Checked);
			}

			virtual void VisitConstructorDecl(ConstructorDecl* decl) override
			{
				if (decl->IsChecked(DeclCheckState::Checked)) return;
				decl->SetCheckState(DeclCheckState::CheckingHeader);

				for (auto& paramDecl : decl->GetParameters())
				{
					paramDecl->Type = CheckUsableType(paramDecl->Type);
				}
				decl->SetCheckState(DeclCheckState::CheckedHeader);

				// TODO(tfoley): check body

				decl->SetCheckState(DeclCheckState::Checked);
			}

			//

			struct Constraint
			{
				Decl*		decl; // the declaration of the thing being constraints
				RefPtr<Val>	val; // the value to which we are constraining it
				bool satisfied = false; // Has this constraint been met?
			};

			// A collection of constraints that will need to be satisified (solved)
			// in order for checking to suceed.
			struct ConstraintSystem
			{
				List<Constraint> constraints;
			};


			// The current constraint system we are solving
			ConstraintSystem* currentConstraintSystem = nullptr;

			void PushConstraintSystem(ConstraintSystem* system)
			{
				assert(!currentConstraintSystem);
				currentConstraintSystem = system;
			}

			void PopConstraintSystem(ConstraintSystem* system)
			{
				assert(currentConstraintSystem == system);
				currentConstraintSystem = nullptr;
			}

			// Try to solve a system of generic constraints.
			// The `system` argument provides the constraints.
			// The `varSubst` argument provides the list of constraint
			// variables that were created for the system.
			//
			// Returns a new substitution representing the values that
			// we solved for along the way.
			RefPtr<Substitutions> TrySolveConstraintSystem(
				ConstraintSystem*		system,
				RefPtr<Substitutions>	varSubst)
			{
				// For now the "solver" is going to be ridiculously simplistic.

				// We will loop over each constraint variable, and then
				// try to solve the constraints on just that variable.
				List<RefPtr<Val>> solvedArgs;
				for (auto varArg : varSubst->args)
				{
					if (auto typeVar = dynamic_cast<ConstraintVarType*>(varArg.Ptr()))
					{
						RefPtr<ExpressionType> type = nullptr;
						for (auto& c : system->constraints)
						{
							if (c.decl != typeVar->declRef.GetDecl())
								continue;

							auto cType = c.val.As<ExpressionType>();
							assert(cType.Ptr());

							if (!type)
							{
								type = cType;
							}
							else
							{
								if (!type->Equals(cType))
								{
									// failure!
									return nullptr;
								}
							}

							c.satisfied = true;
						}

						if (!type)
						{
							// failure!
							return nullptr;
						}
						solvedArgs.Add(type);
					}
					else if (auto valueVar = dynamic_cast<ConstraintVarInt*>(varArg.Ptr()))
					{
						// TODO(tfoley): maybe support more than integers some day?
						RefPtr<IntVal> val = nullptr;
						for (auto& c : system->constraints)
						{
							if (c.decl != valueVar->declRef.GetDecl())
								continue;

							auto cVal = c.val.As<IntVal>();
							assert(cVal.Ptr());

							if (!val)
							{
								val = cVal;
							}
							else
							{
								if (val->value != cVal->value)
								{
									// failure!
									return nullptr;
								}
							}

							c.satisfied = true;
						}

						if (!val)
						{
							// failure!
							return nullptr;
						}
						solvedArgs.Add(val);
					}
					else
					{
						// ignore anything that isn't a generic parameter
					}
				}

				// Make sure we haven't constructed any spurious constraints
				// that we aren't able to satisfy:
				for (auto c : system->constraints)
				{
					if (!c.satisfied)
					{
						return nullptr;
					}
				}

				RefPtr<Substitutions> newSubst = new Substitutions();
				newSubst->genericDecl = varSubst->genericDecl;
				newSubst->outer = varSubst->outer;
				newSubst->args = solvedArgs;
				return newSubst;
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

				// Reference to the declaration being applied
				LookupResultItem item;

				// The type of the result expression if this candidate is selected
				RefPtr<ExpressionType>	resultType;

				// A system for tracking constraints introduced on generic parameters
				ConstraintSystem constraintSystem;

				// How much conversion cost should be considered for this overload,
				// when ranking candidates.
				ConversionCost conversionCostSum = kConversionCost_None;
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
				RefPtr<ExpressionSyntaxNode> baseExpr;

				// Are we still trying out candidates, or are we
				// checking the chosen one for real?
				Mode mode = Mode::JustTrying;

				// We store one candidate directly, so that we don't
				// need to do dynamic allocation on the list every time
				OverloadCandidate bestCandidateStorage;
				OverloadCandidate*	bestCandidate = nullptr;

				// Full list of all candidates being considered, in the ambiguous case
				List<OverloadCandidate> bestCandidates;
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
					paramCount = candidate.item.declRef.As<FuncDeclBaseRef>().GetParameters().Count();
					break;

				case OverloadCandidate::Flavor::ComponentFunc:
					paramCount = candidate.item.declRef.As<ComponentDeclRef>().GetParameters().Count();
					break;
				}
				if (argCount != paramCount && context.mode != OverloadResolveContext::Mode::JustTrying)
				{
					getSink()->diagnose(context.appExpr, Diagnostics::unimplemented, "parameter and argument count mismatch");
				}

				return argCount == paramCount;
			}

			bool TryCheckOverloadCandidateTypes(
				OverloadResolveContext&	context,
				OverloadCandidate&		candidate)
			{
				auto& args = context.appExpr->Arguments;
				int argCount = args.Count();

				List<ParamDeclRef> params;
				switch (candidate.flavor)
				{
				case OverloadCandidate::Flavor::Func:
					params = candidate.item.declRef.As<FuncDeclBaseRef>().GetParameters().ToArray();
					break;

				case OverloadCandidate::Flavor::ComponentFunc:
					params = candidate.item.declRef.As<ComponentDeclRef>().GetParameters().ToArray();
					break;
				}
				int paramCount = params.Count();
				assert(argCount == paramCount);

				for (int ii = 0; ii < argCount; ++ii)
				{
					auto& arg = args[ii];
					auto param = params[ii];

					if (context.mode == OverloadResolveContext::Mode::JustTrying)
					{
						ConversionCost cost = kConversionCost_None;
						if (!CanCoerce(param.GetType(), arg->Type, &cost))
						{
							return false;
						}
						candidate.conversionCostSum += cost;
					}
					else
					{
						arg = Coerce(param.GetType(), arg);
					}
				}
				return true;
			}

			bool TryCheckOverloadCandidateDirections(
				OverloadResolveContext&		context,
				OverloadCandidate const&	candidate)
			{
				// TODO(tfoley): check `in` and `out` markers, as needed.
				return true;
			}

			// Try to solve for the values of any generic parameters,
			// in the case where we are applying a generic with implicit parameters
			bool TrySolveOverloadGenericConstraints(
				OverloadResolveContext&		context,
				OverloadCandidate&			candidate)
			{
				// If the declaration isn't generic, then there is nothing to solve
				auto declRef = candidate.item.declRef;
				if (!declRef.GetParent().As<GenericDeclRef>())
					return true;

				// Note(tfoley): We aren't currently tracking whether generic arguments
				// were provided explicitly or implicitly, so we just guess here:
				// if there were any constraints generated, then it must be implicit.
				//
				// TODO: do this right.

				if (!candidate.constraintSystem.constraints.Count())
					return true;

				auto subst = candidate.item.declRef.substitutions;

				auto newSubst = TrySolveConstraintSystem(&candidate.constraintSystem, subst);

				if (!newSubst)
					return false;

				// We apply the actual substitutions here
				candidate.item.declRef.substitutions = newSubst;
				return true;
			}

			// Try to check an overload candidate, but bail out
			// if any step fails
			void TryCheckOverloadCandidate(
				OverloadResolveContext&		context,
				OverloadCandidate&			candidate)
			{
				PushConstraintSystem(&candidate.constraintSystem);

				if (!TryCheckOverloadCandidateArity(context, candidate))
					goto done;

				candidate.status = OverloadCandidate::Status::ArityChecked;
				if (!TryCheckOverloadCandidateTypes(context, candidate))
					goto done;

				if (!TrySolveOverloadGenericConstraints(context, candidate))
					goto done;


				candidate.status = OverloadCandidate::Status::TypeChecked;
				if (!TryCheckOverloadCandidateDirections(context, candidate))
					goto done;

				candidate.status = OverloadCandidate::Status::Appicable;

			done:
				PopConstraintSystem(&candidate.constraintSystem);
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

				PushConstraintSystem(&candidate.constraintSystem);

				if (!TryCheckOverloadCandidateArity(context, candidate))
					goto done;

				if (!TryCheckOverloadCandidateTypes(context, candidate))
					goto done;

				if (!TryCheckOverloadCandidateDirections(context, candidate))
					goto done;

				context.appExpr->FunctionExpr = ConstructLookupResultExpr(
					candidate.item, context.baseExpr, context.appExpr->FunctionExpr);
				context.appExpr->Type = candidate.resultType;

			done:
				PopConstraintSystem(&candidate.constraintSystem);
				return context.appExpr;
			}

			// Implement a comparison operation between overload candidates,
			// so that the better candidate compares as less-than the other
			int CompareOverloadCandidates(
				OverloadCandidate*	left,
				OverloadCandidate*	right)
			{
				// If one candidate got further along in validation, pick it
				if (left->status != right->status)
					return int(right->status) - int(left->status);

				// If one candidate is 
				if (left->conversionCostSum != right->conversionCostSum)
					return left->conversionCostSum - right->conversionCostSum;


				return 0;
			}

			void AddOverloadCandidate(
				OverloadResolveContext& context,
				OverloadCandidate&		candidate)
			{
				// Try the candidate out, to see if it is applicable at all.
				TryCheckOverloadCandidate(context, candidate);

				// Filter our existing candidates, to remove any that are worse than our new one

				bool keepThisCandidate = true; // should this candidate be kept?

				int oldCandidateCount = context.bestCandidates.Count();
				if (context.bestCandidates.Count() != 0)
				{
					// We have multiple candidates right now, so filter them.
					bool anyFiltered = false;
					// Note that we are querying the list length on every iteration,
					// because we might remove things.
					for (int cc = 0; cc < context.bestCandidates.Count(); ++cc)
					{
						int cmp = CompareOverloadCandidates(&candidate, &context.bestCandidates[cc]);
						if (cmp < 0)
						{
							// our new candidate is better!

							// remove it from the list (by swapping in a later one)
							context.bestCandidates.FastRemoveAt(cc);
							// and then reduce our index so that we re-visit the same index
							--cc;

							anyFiltered = true;
						}
						else if(cmp > 0)
						{
							// our candidate is worse!
							keepThisCandidate = false;
						}
					}
					// It should not be possible that we removed some existing candidate *and*
					// chose not to keep this candidate (otherwise the better-ness relation
					// isn't transitive). Therefore we confirm that we either chose to keep
					// this candidate (in which case filtering is okay), or we didn't filter
					// anything.
					assert(keepThisCandidate || !anyFiltered);
				}
				else if(context.bestCandidate)
				{
					// There's only one candidate so far
					int cmp = CompareOverloadCandidates(&candidate, context.bestCandidate);
					if(cmp < 0)
					{
						// our new candidate is better!
						context.bestCandidate = nullptr;
					}
					else if (cmp > 0)
					{
						// our candidate is worse!
						keepThisCandidate = false;
					}
				}

				// If our candidate isn't good enough, then drop it
				if (!keepThisCandidate)
					return;

				// Otherwise we want to keep the candidate
				if (context.bestCandidates.Count() > 0)
				{
					// There were already multiple candidates, and we are adding one more
					context.bestCandidates.Add(candidate);
				}
				else if (context.bestCandidate)
				{
					// There was a unique best candidate, but now we are ambiguous
					context.bestCandidates.Add(*context.bestCandidate);
					context.bestCandidates.Add(candidate);
					context.bestCandidate = nullptr;
				}
				else
				{
					// This is the only candidate worthe keeping track of right now
					context.bestCandidateStorage = candidate;
					context.bestCandidate = &context.bestCandidateStorage;
				}
			}

			void AddFuncOverloadCandidate(
				LookupResultItem			item,
				FuncDeclBaseRef				funcDeclRef,
				OverloadResolveContext&		context)
			{
				EnsureDecl(funcDeclRef.GetDecl());

				OverloadCandidate candidate;
				candidate.flavor = OverloadCandidate::Flavor::Func;
				candidate.item = item;
				candidate.resultType = funcDeclRef.GetResultType();

				AddOverloadCandidate(context, candidate);
			}

			void AddComponentFuncOverloadCandidate(
				RefPtr<ShaderComponentSymbol>	componentFuncSym,
				OverloadResolveContext&			context)
			{
#if 0
				auto componentFuncDecl = componentFuncSym->Implementations.First()->SyntaxNode.Ptr();

				OverloadCandidate candidate;
				candidate.flavor = OverloadCandidate::Flavor::ComponentFunc;
				candidate.componentFunc = componentFuncDecl;
				candidate.resultType = componentFuncDecl->Type;

				AddOverloadCandidate(context, candidate);
#else
				throw "unimplemented";
#endif
			}

			void AddFuncOverloadCandidate(
				RefPtr<FuncType>		funcType,
				OverloadResolveContext&	context)
			{
#if 0
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
#else
				throw "unimplemented";
#endif
			}

			void AddCtorOverloadCandidate(
				LookupResultItem		typeItem,
				RefPtr<ExpressionType>	type,
				ConstructorDeclRef		ctorDeclRef,
				OverloadResolveContext&	context)
			{
				EnsureDecl(ctorDeclRef.GetDecl());

				// `typeItem` refers to the type being constructed (the thing
				// that was applied as a function) so we need to construct
				// a `LookupResultItem` that refers to the constructor instead

				LookupResultItem ctorItem;
				ctorItem.declRef = ctorDeclRef;
				ctorItem.breadcrumbs = new LookupResultItem::Breadcrumb(LookupResultItem::Breadcrumb::Kind::Member, typeItem.declRef, typeItem.breadcrumbs);

				OverloadCandidate candidate;
				candidate.flavor = OverloadCandidate::Flavor::Func;
				candidate.item = ctorItem;
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


			// Record the fact that a type coercion occured, so we can solve
			// for it in our constraint system.
			//
			// Note: for right now we only store type equality constraints,
			// so we can't solve for a type variable that needs to be implicitly
			// convertible from two different types.
			void RegisterCoercionConstraint(
				RefPtr<ExpressionType>			toType,
				RefPtr<ExpressionType>			fromType)
			{
				assert(this->currentConstraintSystem);

				if (auto toConstraintVar = toType->As<ConstraintVarType>())
				{
					Constraint constraint;

					// TODO(tfoley): should these be `DeclRef`s?
					constraint.decl = toConstraintVar->declRef.GetDecl(); 
					constraint.val = fromType;
					currentConstraintSystem->constraints.Add(constraint);
				}

				if (auto fromConstraintVar = fromType->As<ConstraintVarType>())
				{
					// Note: mirrors the above case
					Constraint constraint;
					constraint.decl = fromConstraintVar->declRef.GetDecl(); 
					constraint.val = toType;
					currentConstraintSystem->constraints.Add(constraint);
				}
			}

			// Try to find a unification for two values
			bool TryUnifyVals(
				ConstraintSystem&	constraints,
				RefPtr<Val>			fst,
				RefPtr<Val>			snd)
			{
				// if both values are types, then unify types
				if (auto fstType = fst.As<ExpressionType>())
				{
					if (auto sndType = snd.As<ExpressionType>())
					{
						return TryUnifyTypes(constraints, fstType, sndType);
					}
				}

				// if both values are integers, then compare them
				if (auto fstIntVal = fst.As<IntVal>())
				{
					if (auto sndIntVal = snd.As<IntVal>())
					{
						return fstIntVal->value == sndIntVal->value;
					}
				}

				throw "unimplemented";

				// default: fail
				return false;
			}

			bool TryUnifySubstitutions(
				ConstraintSystem&		constraints,
				RefPtr<Substitutions>	fst,
				RefPtr<Substitutions>	snd)
			{
				// They must both be NULL or non-NULL
				if (!fst || !snd)
					return fst == snd;

				// They must be specializing the same generic
				if (fst->genericDecl != snd->genericDecl)
					return false;

				// Their arguments must unify
				assert(fst->args.Count() == snd->args.Count());
				int argCount = fst->args.Count();
				for (int aa = 0; aa < argCount; ++aa)
				{
					if (!TryUnifyVals(constraints, fst->args[aa], snd->args[aa]))
						return false;
				}

				// Their "base" specializations must unify
				if (!TryUnifySubstitutions(constraints, fst->outer, snd->outer))
					return false;

				return true;
			}

			bool TryUnifyTypeParam(
				ConstraintSystem&				constraints,
				RefPtr<GenericTypeParamDecl>	typeParamDecl,
				RefPtr<ExpressionType>			type)
			{
				// We want to constrain the given type parameter
				// to equal the given type.
				Constraint constraint;
				constraint.decl = typeParamDecl.Ptr();
				constraint.val = type;

				constraints.constraints.Add(constraint);

				return true;
			}

			bool TryUnifyTypes(
				ConstraintSystem&	constraints,
				RefPtr<ExpressionType> fst,
				RefPtr<ExpressionType> snd)
			{
				if (fst->Equals(snd)) return true;

				if (auto fstDeclRefType = fst->As<DeclRefType>())
				{
					auto fstDeclRef = fstDeclRefType->declRef;

					if (auto typeParamDecl = dynamic_cast<GenericTypeParamDecl*>(fstDeclRef.GetDecl()))
						return TryUnifyTypeParam(constraints, typeParamDecl, snd);

					if (auto sndDeclRefType = snd->As<DeclRefType>())
					{
						auto sndDeclRef = sndDeclRefType->declRef;

						// can't be unified if they refer to differnt declarations.
						if (fstDeclRef.GetDecl() != sndDeclRef.GetDecl()) return false;

						// next we need to unify the substitutions applied
						// to each decalration reference.
						if (!TryUnifySubstitutions(
							constraints,
							fstDeclRef.substitutions,
							sndDeclRef.substitutions))
						{
							return false;
						}

						return true;
					}
				}

				if (auto sndDeclRefType = snd->As<DeclRefType>())
				{
					auto sndDeclRef = sndDeclRefType->declRef;

					if (auto typeParamDecl = dynamic_cast<GenericTypeParamDecl*>(sndDeclRef.GetDecl()))
						return TryUnifyTypeParam(constraints, typeParamDecl, fst);
				}

				throw "unimplemented";
				return false;
			}

			// Is the candidate extension declaration actually applicable to the given type
			ExtensionDeclRef ApplyExtensionToType(
				ExtensionDecl*			extDecl,
				RefPtr<ExpressionType>	type)
			{
				if (auto extGenericDecl = GetOuterGeneric(extDecl))
				{
#if 0
					// we need to check whether we can unify the type on the extension
					// with the type in question.
					// This has to be done with a constraint solver.

					// Construct type-level constraint variables for all the
					// generic parameters
					List<RefPtr<Val>> constraintArgs;
					for (auto m : extGenericDecl->Members)
					{
						if (auto typeParam = m.As<GenericTypeParamDecl>())
						{
							auto type = new ConstraintVarType();
							type->decl = typeParam;
							constraintArgs.Add(type);
						}
						else if (auto valParam = m.As<GenericValueParamDecl>())
						{
							auto val = new ConstraintVarIntVal();
							val->decl = valParam;
							constraintArgs.Add(val);
						}
						else
						{
							// ignore anything that isn't a generic parameter
						}
					}

					// Consruct a reference to the extension with our constraint variables
					// as the 
					RefPtr<Substitutions> constraintSubst = new Substitutions();
					constraintSubst->genericDecl = extGenericDecl;
					constraintSubst->args = constraintArgs;
					ExtensionDeclRef extDeclRef = DeclRef(extDecl, constraintSubst).As<ExtensionDeclRef>();
#endif
					ConstraintSystem constraints;

					if (!TryUnifyTypes(constraints, extDecl->targetType, type))
						return DeclRef().As<ExtensionDeclRef>();

					// We expect to find a bunch of constraints that we'll
					// need to solve. For now the "solver" is going to be
					// ridiculously simplistic.

					// We will loop over the generic parameters, and for
					// each we will try to find a way to satisfy all
					// the constraints for that parameter
					List<RefPtr<Val>> args;
					for (auto m : extGenericDecl->Members)
					{
						if (auto typeParam = m.As<GenericTypeParamDecl>())
						{
							RefPtr<ExpressionType> type = nullptr;
							for (auto& c : constraints.constraints)
							{
								if (c.decl != typeParam.Ptr())
									continue;

								auto cType = c.val.As<ExpressionType>();
								assert(cType.Ptr());

								if (!type)
								{
									type = cType;
								}
								else
								{
									if (!type->Equals(cType))
									{
										// failure!
										return DeclRef().As<ExtensionDeclRef>();
									}
								}

								c.satisfied = true;
							}

							if (!type)
							{
								// failure!
								return DeclRef().As<ExtensionDeclRef>();
							}
							args.Add(type);
						}
						else if (auto valParam = m.As<GenericValueParamDecl>())
						{
							// TODO(tfoley): maybe support more than integers some day?
							RefPtr<IntVal> val = nullptr;
							for (auto& c : constraints.constraints)
							{
								if (c.decl != valParam.Ptr())
									continue;

								auto cVal = c.val.As<IntVal>();
								assert(cVal.Ptr());

								if (!val)
								{
									val = cVal;
								}
								else
								{
									if (val->value != cVal->value)
									{
										// failure!
										return DeclRef().As<ExtensionDeclRef>();
									}
								}

								c.satisfied = true;
							}

							if (!val)
							{
								// failure!
								return DeclRef().As<ExtensionDeclRef>();
							}
							args.Add(val);
						}
						else
						{
							// ignore anything that isn't a generic parameter
						}
					}

					// Make sure we haven't constructed any spurious constraints
					// that we aren't able to satisfy:
					for (auto c : constraints.constraints)
					{
						if (!c.satisfied)
						{
							return DeclRef().As<ExtensionDeclRef>();
						}
					}

					// Consruct a reference to the extension with our constraint variables
					// as the 
					RefPtr<Substitutions> constraintSubst = new Substitutions();
					constraintSubst->genericDecl = extGenericDecl;
					constraintSubst->args = args;
					ExtensionDeclRef extDeclRef = DeclRef(extDecl, constraintSubst).As<ExtensionDeclRef>();

					// We expect/require that the result of unification is such that
					// the target types are now equal
					assert(extDeclRef.GetTargetType()->Equals(type));

					return extDeclRef;
				}
				else
				{
					// The easy case is when the extension isn't generic:
					// either it applies to the type or not.
					if (!type->Equals(extDecl->targetType))
						return DeclRef().As<ExtensionDeclRef>();
					return DeclRef(extDecl, nullptr).As<ExtensionDeclRef>();
				}
			}

			// Given a generic declaration, create constraint-solver variables
			// for its parameters, and then substitute those into the inner declaration.
			DeclRef CreateGenericConstraintSolverVariables(GenericDeclRef genericDeclRef)
			{
				RefPtr<Substitutions> subst = new Substitutions();
				subst->genericDecl = genericDeclRef.GetDecl();
				subst->outer = genericDeclRef.substitutions;

				for (auto m : genericDeclRef.GetMembers())
				{
					if (auto genericTypeParam = m.As<GenericTypeParamDeclRef>())
					{
						auto typeVar = new ConstraintVarType(genericTypeParam);
						subst->args.Add(typeVar);
					}
					else if (auto genericValParam = m.As<GenericValueParamDeclRef>())
					{
						// TODO(tfoley): support things other than integers?
						auto valVar = new ConstraintVarInt(genericValParam);
						subst->args.Add(valVar);
					}
				}

				return DeclRef(genericDeclRef.GetInner(), subst);
			}



			void AddAggTypeOverloadCandidates(
				LookupResultItem		typeItem,
				RefPtr<ExpressionType>	type,
				AggTypeDeclRef			aggTypeDeclRef,
				OverloadResolveContext&	context)
			{
				for (auto ctorDeclRef : aggTypeDeclRef.GetMembersOfType<ConstructorDeclRef>())
				{
					// now work through this candidate...
					AddCtorOverloadCandidate(typeItem, type, ctorDeclRef, context);
				}

				// Now walk through any extensions we can find for this types
				for (auto ext = aggTypeDeclRef.GetCandidateExtensions(); ext; ext = ext->nextCandidateExtension)
				{
					auto extDeclRef = ApplyExtensionToType(ext, type);
					if (!extDeclRef)
						continue;

					for (auto ctorDeclRef : extDeclRef.GetMembersOfType<ConstructorDeclRef>())
					{
						// TODO(tfoley): `typeItem` here should really reference the extension...

						// now work through this candidate...
						AddCtorOverloadCandidate(typeItem, type, ctorDeclRef, context);
					}

					// Also check for generic constructors
					for (auto genericDeclRef : extDeclRef.GetMembersOfType<GenericDeclRef>())
					{
						if (auto ctorDecl = genericDeclRef.GetDecl()->inner.As<ConstructorDecl>())
						{
							DeclRef innerRef = CreateGenericConstraintSolverVariables(genericDeclRef);
							ConstructorDeclRef innerCtorRef = innerRef.As<ConstructorDeclRef>();

							AddCtorOverloadCandidate(typeItem, type, innerCtorRef, context);

							// TODO(tfoley): need a way to do the solving step for the constraint system
						}
					}
				}
			}

			void AddTypeOverloadCandidates(
				RefPtr<ExpressionType>	type,
				OverloadResolveContext&	context)
			{
				if (auto declRefType = type->As<DeclRefType>())
				{
					if (auto aggTypeDeclRef = declRefType->declRef.As<AggTypeDeclRef>())
					{
						AddAggTypeOverloadCandidates(LookupResultItem(aggTypeDeclRef), type, aggTypeDeclRef, context);
					}
				}
			}

			void AddDeclRefOverloadCandidates(
				LookupResultItem		item,
				OverloadResolveContext&	context)
			{
				if (auto funcDeclRef = item.declRef.As<FuncDeclBaseRef>())
				{
					AddFuncOverloadCandidate(item, funcDeclRef, context);
				}
				else if (auto aggTypeDeclRef = item.declRef.As<AggTypeDeclRef>())
				{
					auto type = DeclRefType::Create(aggTypeDeclRef);
					AddAggTypeOverloadCandidates(item, type, aggTypeDeclRef, context);
				}
				else
				{
					// TODO(tfoley): any other cases needed here?
				}
			}

			void AddOverloadCandidates(
				RefPtr<ExpressionSyntaxNode>	funcExpr,
				OverloadResolveContext&			context)
			{
				auto funcExprType = funcExpr->Type;
				if (auto typeType = funcExprType->As<TypeExpressionType>())
				{
					// The expression named a type, so we have a constructor call
					// on our hands.
					AddTypeOverloadCandidates(typeType->type, context);
				}
				else if (auto funcDeclRefExpr = funcExpr.As<DeclRefExpr>())
				{
					// The expression referenced a function declaration
					AddDeclRefOverloadCandidates(LookupResultItem(funcDeclRefExpr->declRef), context);
				}
				else if (auto funcType = funcExprType->As<FuncType>())
				{
					// TODO(tfoley): deprecate this path...
					AddFuncOverloadCandidate(funcType, context);
				}
				else if (auto overloadedExpr = funcExpr.As<OverloadedExpr>())
				{
					auto lookupResult = overloadedExpr->lookupResult2;
					assert(lookupResult.isOverloaded());
					for(auto item : lookupResult.items)
					{
						AddDeclRefOverloadCandidates(item, context);
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
				if (auto funcMemberExpr = funcExpr.As<MemberExpressionSyntaxNode>())
				{
					context.baseExpr = funcMemberExpr->BaseExpression;
				}
				else if(auto funcOverloadExpr = funcExpr.As<OverloadedExpr>())
				{
					context.baseExpr = funcOverloadExpr->base;
				}
				AddOverloadCandidates(funcExpr, context);

				if (context.bestCandidates.Count() > 0)
				{
					// Things were ambiguous.
					if (context.bestCandidates[0].status != OverloadCandidate::Status::Appicable)
					{
						// There were multple equally-good candidates, but none actually usable.
						// We will construct a diagnostic message to help out.

						String funcName;
						if (auto baseVar = funcExpr.As<VarExpressionSyntaxNode>())
							funcName = baseVar->Variable;
						else if(auto baseMemberRef = funcExpr.As<MemberExpressionSyntaxNode>())
							funcName = baseMemberRef->MemberName;

						StringBuilder argsListBuilder;
						argsListBuilder << "(";
						bool first = true;
						for (auto a : expr->Arguments)
						{
							if (!first) argsListBuilder << ", ";
							argsListBuilder << a->Type->ToString();
							first = false;
						}
						argsListBuilder << ")";
						String argsList = argsListBuilder.ProduceString();

						if (funcName.Length() != 0)
						{
							getSink()->diagnose(expr, Diagnostics::noApplicableOverloadForNameWithArgs, funcName, argsList);
						}
						else
						{
							getSink()->diagnose(expr, Diagnostics::noApplicableWithArgs, funcName, argsList);
						}

						// TODO: iterate over the candidates under consideration and print them?
						expr->Type = ExpressionType::Error;
						return expr;
					}
					else
					{
						// There were multiple applicable candidates, so we need to report them.
						getSink()->diagnose(expr, Diagnostics::unimplemented, "ambiguous overloaded call");
						expr->Type = ExpressionType::Error;
						return expr;
					}
				}
				else if (context.bestCandidate)
				{
					// There was one best candidate, even if it might not have been
					// applicable in the end.
					// We will report errors for this one candidate, then, to give
					// the user the most help we can.
					return CompleteOverloadCandidate(context, *context.bestCandidate);
				}
				else
				{
					// Nothing at all was found that we could even consider invoking
					getSink()->diagnose(expr->FunctionExpr, Diagnostics::expectedFunction);
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

				bool anyError = false;
				for (auto & arg : expr->Arguments)
				{
					arg = arg->Accept(this).As<ExpressionSyntaxNode>();
					if (arg->Type->Equals(ExpressionType::Error))
						anyError = true;
				}
				if (anyError)
				{
					expr->Type = ExpressionType::Error;
					return expr;
				}

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
					return (int(mask) & int(LookupMask::Function)) != 0;
				}
				// component declarations have kind of odd rules
				else if(auto componentDecl = dynamic_cast<ComponentSyntaxNode*>(decl))
				{
					if (componentDecl->IsComponentFunction())
					{
						return (int(mask) & int(LookupMask::Function)) != 0;
					}
					else
					{
						return (int(mask) & int(LookupMask::Value)) != 0;
					}
				}

				// default behavior is to assume a value declaration
				// (no overloading allowed)

				return (int(mask) & int(LookupMask::Value)) != 0;
			}

			void BuildMemberDictionary(ContainerDecl* decl)
			{
				decl->transparentMembers.Clear();

				for (auto m : decl->Members)
				{
					auto name = m->Name.Content;

					// Add any transparent members to a separate list for lookup
					if (m->HasModifier(ModifierFlag::Transparent))
					{
						TransparentMemberInfo info;
						info.decl = m.Ptr();
						decl->transparentMembers.Add(info);
					}

					// Ignore members with an empty name
					if (name.Length() == 0)
						continue;

					m->nextInContainerWithSameName = nullptr;

					Decl* next = nullptr;
					if (decl->memberDictionary.TryGetValue(name, next))
						m->nextInContainerWithSameName = next;

					decl->memberDictionary[name] = m.Ptr();

				}
				decl->memberDictionaryIsValid = true;
			}

			void AddToLookupResult(
				LookupResult&		result,
				LookupResultItem	item)
			{
				if (!result.isValid())
				{
					// If we hadn't found a hit before, we have one now
					result.item = item;
				}
				else if (!result.isOverloaded())
				{
					// We are about to make this overloaded
					result.items.Add(result.item);
					result.items.Add(item);
				}
				else
				{
					// The result was already overloaded, so we pile on
					result.items.Add(item);
				}
			}

			// Helper for constructing breadcrumb trails during lookup, without
			// any heap allocation
			struct BreadcrumbInfo
			{
				LookupResultItem::Breadcrumb::Kind kind;
				DeclRef declRef;
				BreadcrumbInfo* prev = nullptr;
			};

			LookupResultItem CreateLookupResultItem(
				DeclRef declRef,
				BreadcrumbInfo* breadcrumbInfos)
			{
				LookupResultItem item;
				item.declRef = declRef;

				// breadcrumbs were constructed "backwards" on the stack, so we
				// reverse them here by building a linked list the other way
				RefPtr<LookupResultItem::Breadcrumb> breadcrumbs;
				for (auto bb = breadcrumbInfos; bb; bb = bb->prev)
				{
					breadcrumbs = new LookupResultItem::Breadcrumb(
						bb->kind,
						bb->declRef,
						breadcrumbs);
				}
				item.breadcrumbs = breadcrumbs;
				return item;
			}

			// Look for members of the given name in the given container for declarations
			void DoLocalLookupImpl(
				String const&		name,
				ContainerDeclRef	containerDeclRef,
				LookupResult&		result,
				BreadcrumbInfo*		inBreadcrumbs)
			{
				ContainerDecl* containerDecl = containerDeclRef.GetDecl();

				// Ensure that the lookup dictionary in the container is up to date
				if (!containerDecl->memberDictionaryIsValid)
				{
					BuildMemberDictionary(containerDecl);
				}

				// Look up the declarations with the chosen name in the container.
				Decl* firstDecl = nullptr;
				containerDecl->memberDictionary.TryGetValue(name, firstDecl);

				// Now iterate over those declarations (if any) and see if
				// we find any that meet our filtering criteria.
				// For example, we might be filtering so that we only consider
				// type declarations.
				for (auto m = firstDecl; m; m = m->nextInContainerWithSameName)
				{
					if (!DeclPassesLookupMask(m, result.mask))
						continue;

					// The declaration passed the test, so add it!
					AddToLookupResult(result, CreateLookupResultItem(DeclRef(m, containerDeclRef.substitutions), inBreadcrumbs));
				}


				// TODO(tfoley): should we look up in the transparent decls
				// if we already has a hit in the current container?

				for(auto transparentInfo : containerDecl->transparentMembers)
				{
					// The reference to the transparent member should use whatever
					// substitutions we used in referring to its outer container
					DeclRef transparentMemberDeclRef(transparentInfo.decl, containerDeclRef.substitutions);

					// We need to leave a breadcrumb so that we know that the result
					// of lookup involves a member lookup step here

					BreadcrumbInfo memberRefBreadcrumb;
					memberRefBreadcrumb.kind = LookupResultItem::Breadcrumb::Kind::Member;
					memberRefBreadcrumb.declRef = transparentMemberDeclRef;
					memberRefBreadcrumb.prev = inBreadcrumbs;

					DoMemberLookupImpl(name, transparentMemberDeclRef, result, &memberRefBreadcrumb);
				}

				// TODO(tfoley): need to consider lookup via extension here?
			}

			void DoMemberLookupImpl(
				String const&			name,
				RefPtr<ExpressionType>	baseType,
				LookupResult&			ioResult,
				BreadcrumbInfo*			breadcrumbs)
			{
				// If the type was pointer-like, then dereference it
				// automatically here.
				if (auto pointerLikeType = baseType->As<PointerLikeType>())
				{
					// Need to leave a breadcrumb to indicate that we
					// did an implicit dereference here
					BreadcrumbInfo derefBreacrumb;
					derefBreacrumb.kind = LookupResultItem::Breadcrumb::Kind::Deref;
					derefBreacrumb.prev = breadcrumbs;

					// Recursively perform lookup on the result of deref
					return DoMemberLookupImpl(name, pointerLikeType->elementType, ioResult, &derefBreacrumb);
				}

				// Default case: no dereference needed

				if (auto baseDeclRefType = baseType->As<DeclRefType>())
				{
					if (auto baseAggTypeDeclRef = baseDeclRefType->declRef.As<AggTypeDeclRef>())
					{
						DoLocalLookupImpl(name, baseAggTypeDeclRef, ioResult, breadcrumbs);
					}
				}

				// TODO(tfoley): any other cases to handle here?
			}

			void DoMemberLookupImpl(
				String const&	name,
				DeclRef			baseDeclRef,
				LookupResult&	ioResult,
				BreadcrumbInfo*	breadcrumbs)
			{
				auto baseType = GetTypeForDeclRef(baseDeclRef);
				return DoMemberLookupImpl(name, baseType, ioResult, breadcrumbs);
			}

			void DoLookupImpl(String const& name, LookupResult& result)
			{
				ContainerDecl* scope = result.scope;
				ContainerDecl* endScope = result.endScope;
				for (;scope != endScope; scope = scope->ParentDecl)
				{
					ContainerDeclRef scopeRef = DeclRef(scope, nullptr).As<ContainerDeclRef>();
					DoLocalLookupImpl(name, scopeRef, result, nullptr);

					if (result.isValid())
					{
						// If we've found a result in this scope, then there
						// is no reason to look further up (for now).
						return;
					}

#if 0


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
#endif
				}

				// If we run out of scopes, then we are done.
			}

			LookupResult DoLookup(String const& name, LookupResult inResult)
			{
				LookupResult result;
				result.mask = inResult.mask;
				result.endScope = inResult.endScope;
				DoLookupImpl(name, result);
				return result;
			}

			LookupResult LookUp(String const& name, ContainerDecl* scope)
			{
				LookupResult result;
				result.scope = scope;
				DoLookupImpl(name, result);
				return result;
			}

			// perform lookup within the context of a particular container declaration,
			// and do *not* look further up the chain
			LookupResult LookUpLocal(String const& name, ContainerDecl* scope)
			{
				LookupResult result;
				result.scope = scope;
				result.endScope = scope->ParentDecl;
				return DoLookup(name, result);

			}

			LookupResult LookUpLocal(String const& name, ContainerDeclRef containerDeclRef)
			{
				LookupResult result;
				DoLocalLookupImpl(name, containerDeclRef, result, nullptr);
				return result;
			}

			virtual RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode *expr) override
			{
				// If we've already resolved this expression, don't try again.
				if (expr->declRef)
					return expr;

				ShaderUsing shaderObj;
				expr->Type = ExpressionType::Error;

				auto lookupResult = LookUp(expr->Variable, expr->scope);
				if (lookupResult.isValid())
				{
					// Found at least one declaration, but did we find many?
					if (lookupResult.isOverloaded())
					{
						auto overloadedExpr = new OverloadedExpr();
						overloadedExpr->Position = expr->Position;
						overloadedExpr->Type = ExpressionType::Overloaded;
						overloadedExpr->lookupResult2 = lookupResult;
						return overloadedExpr;
					}
					else
					{
						// Only a single decl, that's good
						return ConstructLookupResultExpr(lookupResult.item, nullptr, expr);
#if 0
						auto declRef = lookupResult.declRef;
						expr->declRef = declRef;
						expr->Type = GetTypeForDeclRef(declRef);
						return expr;
#endif
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

#if TIMREMOVED
				// These are geneic types that magically "decay" when the value is referenced, and that is super gross
				if (expr->Type->IsGenericType("Uniform") || expr->Type->IsGenericType("Patch") || expr->Type->IsGenericType("StorageBuffer"))
					expr->Type = expr->Type->AsGenericType()->BaseType;
#endif

				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitTypeCastExpression(TypeCastExpressionSyntaxNode * expr) override
			{
				expr->Expression = expr->Expression->Accept(this).As<ExpressionSyntaxNode>();
				auto targetType = CheckProperType(expr->TargetType);

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
			QualType GetTypeForDeclRef(DeclRef declRef)
			{
				// We need to insert an appropriate type for the expression, based on
				// what we found.
				if (auto varDeclRef = declRef.As<VarDeclBaseRef>())
				{
					QualType qualType;
					qualType.type = varDeclRef.GetType();
					qualType.IsLeftValue = true; // TODO(tfoley): allow explicit `const` or `let` variables
					return qualType;
				}
				else if(auto compDeclRef = declRef.As<ComponentDeclRef>())
				{
					if (compDeclRef.GetDecl()->IsComponentFunction())
					{
						// TODO: need to implement this case
						throw "unimplemented";
					}
					else
					{
						return compDeclRef.GetType();
					}
				}
				else if (auto typeAliasDeclRef = declRef.As<TypeDefDeclRef>())
				{
					EnsureDecl(typeAliasDeclRef.GetDecl());
					auto type = new NamedExpressionType(typeAliasDeclRef);
					typeResult = type;
					return new TypeExpressionType(type);
				}
				else if (auto aggTypeDeclRef = declRef.As<AggTypeDeclRef>())
				{
					auto type = DeclRefType::Create(aggTypeDeclRef);
					typeResult = type;
					return new TypeExpressionType(type);
				}
				else if (auto simpleTypeDeclRef = declRef.As<SimpleTypeDeclRef>())
				{
					auto type = DeclRefType::Create(simpleTypeDeclRef);
					typeResult = type;
					return new TypeExpressionType(type);
				}
				else if (auto genericDeclRef = declRef.As<GenericDeclRef>())
				{
					auto type = new GenericDeclRefType(genericDeclRef);
					typeResult = type;
					return new TypeExpressionType(type);
				}
				else if (auto funcDeclRef = declRef.As<FuncDeclBaseRef>())
				{
					auto type = new FuncType();
					type->declRef = funcDeclRef;
					return type;
				}

				getSink()->diagnose(declRef, Diagnostics::unimplemented, "cannot form reference to this kind of declaration");
				return ExpressionType::Error;
			}

			RefPtr<ExpressionSyntaxNode> MaybeDereference(RefPtr<ExpressionSyntaxNode> inExpr)
			{
				RefPtr<ExpressionSyntaxNode> expr = inExpr;
				for (;;)
				{
					auto& type = expr->Type;
					if (auto pointerLikeType = type->As<PointerLikeType>())
					{
						type = pointerLikeType->elementType;

						auto derefExpr = new DerefExpr();
						derefExpr->base = expr;
						derefExpr->Type = pointerLikeType->elementType;

						// TODO(tfoley): deal with l-value-ness here

						expr = derefExpr;
						continue;
					}

					// Default case: just use the expression as-is
					return expr;
				}
			}

			virtual RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * expr) override
			{
				expr->BaseExpression = CheckExpr(expr->BaseExpression);

				expr->BaseExpression = MaybeDereference(expr->BaseExpression);

				auto & baseType = expr->BaseExpression->Type;

				// Note: Checking for vector types before declaration-reference types,
				// because vectors are also declaration reference types...
				if (auto baseVecType = baseType->AsVectorType())
				{
					RefPtr<SwizzleExpr> swizExpr = new SwizzleExpr();
					swizExpr->Position = expr->Position;
					swizExpr->base = expr->BaseExpression;

					int limitElement = baseVecType->elementCount;

					int elementIndices[4];
					int elementCount = 0;

					bool elementUsed[4] = { false, false, false, false };
					bool anyDuplicates = false;
					bool anyError = false;

					for (int i = 0; i < expr->MemberName.Length(); i++)
					{
						auto ch = expr->MemberName[i];
						int elementIndex = -1;
						switch (ch)
						{
						case 'x': case 'r': elementIndex = 0; break;
						case 'y': case 'g': elementIndex = 1; break;
						case 'z': case 'b': elementIndex = 2; break;
						case 'w': case 'a': elementIndex = 3; break;
						default:
							// An invalid character in the swizzle is an error
							getSink()->diagnose(swizExpr, Diagnostics::unimplemented, "invalid component name for swizzle");
							anyError = true;
							continue;
						}

						// TODO(tfoley): GLSL requires that all component names
						// come from the same "family"...

						// Make sure the index is in range for the source type
						if (elementIndex >= limitElement)
						{
							getSink()->diagnose(swizExpr, Diagnostics::unimplemented, "swizzle component out of range for type");
							anyError = true;
							continue;
						}

						// Check if we've seen this index before
						for (int ee = 0; ee < elementCount; ee++)
						{
							if (elementIndices[ee] == elementIndex)
								anyDuplicates = true;
						}

						// add to our list...
						elementIndices[elementCount++] = elementIndex;
					}

					for (int ee = 0; ee < elementCount; ++ee)
					{
						swizExpr->elementIndices[ee] = elementIndices[ee];
					}
					swizExpr->elementCount = elementCount;

					if (anyError)
					{
						swizExpr->Type = ExpressionType::Error;
					}
					else if (elementCount == 1)
					{
						// single-component swizzle produces a scalar
						//
						// Note(tfoley): the official HLSL rules seem to be that it produces
						// a one-component vector, which is then implicitly convertible to
						// a scalar, but that seems like it just adds complexity.
						swizExpr->Type = baseVecType->elementType;
					}
					else
					{
						// TODO(tfoley): would be nice to "re-sugar" type
						// here if the input type had a sugared name...
						swizExpr->Type = new VectorExpressionType(
							baseVecType->elementType,
							elementCount);
					}

					// A swizzle can be used as an l-value as long as there
					// were no duplicates in the list of components
					swizExpr->Type.IsLeftValue = !anyDuplicates;

					return swizExpr;
				}
				else if (auto declRefType = baseType->AsDeclRefType())
				{
					if (auto aggTypeDeclRef = declRefType->declRef.As<AggTypeDeclRef>())
					{
						// Checking of the type must be complete before we can reference its members safely
						EnsureDecl(aggTypeDeclRef.GetDecl(), DeclCheckState::Checked);


						LookupResult lookupResult = LookUpLocal(expr->MemberName, aggTypeDeclRef);
						if (!lookupResult.isValid())
						{
							goto fail;
						}

						if (lookupResult.isOverloaded())
						{
							auto overloadedExpr = new OverloadedExpr();
							overloadedExpr->Position = expr->Position;
							overloadedExpr->Type = ExpressionType::Overloaded;
							overloadedExpr->base = expr->BaseExpression;
							overloadedExpr->lookupResult2 = lookupResult;
							return overloadedExpr;
						}

						// default case: we have found something
						return ConstructLookupResultExpr(lookupResult.item, expr->BaseExpression, expr);
#if 0
						DeclRef memberDeclRef(lookupResult.decl, aggTypeDeclRef.substitutions);
						return ConstructDeclRefExpr(memberDeclRef, expr->BaseExpression, expr);
#endif

#if 0


						// TODO(tfoley): It is unfortunate that the lookup strategy
						// here isn't unified with the ordinary `Scope` case.
						// In particular, if we add support for "transparent" declarations,
						// etc. here then we would need to add them in ordinary lookup
						// as well.

						Decl* memberDecl = nullptr; // The first declaration we found, if any
						Decl* secondDecl = nullptr; // Another declaration with the same name, if any
						for (auto m : aggTypeDeclRef.GetMembers())
						{
							if (m.GetName() != expr->MemberName)
								continue;

							if (!memberDecl)
							{
								memberDecl = m.GetDecl();
							}
							else
							{
								secondDecl = m.GetDecl();
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
							// TODO: need to
							DeclRef memberDeclRef(memberDecl, aggTypeDeclRef.substitutions);

							expr->declRef = memberDeclRef;
							expr->Type = GetTypeForDeclRef(memberDeclRef);

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

#endif

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
				fail:
					getSink()->diagnose(expr, Diagnostics::noMemberOfNameInType, expr->MemberName, baseType);
					expr->Type = ExpressionType::Error;
					return expr;
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