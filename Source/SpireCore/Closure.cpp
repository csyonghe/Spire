#include "Closure.h"
#include "StringObject.h"
#include "Naming.h"

namespace Spire
{
	namespace Compiler
	{
		void CheckComponentRedefinition(DiagnosticSink * err, ShaderClosure * parent, ShaderClosure * child)
		{
			for (auto & comp : child->Components)
			{
				RefPtr<ShaderComponentSymbol> ccomp;
				RefPtr<ShaderClosure> su;
				if ((comp.Value->Implementations.First()->SyntaxNode->IsPublic() ||
					comp.Value->Implementations.First()->SyntaxNode->IsOutput()))
				{
                    if (parent->Components.TryGetValue(comp.Key, ccomp))
                    {
						err->diagnose(comp.Value->Implementations.First()->SyntaxNode, Diagnostics::nameAlreadyDefinedInCurrentScope, comp.Key);
                        err->diagnose(ccomp->Implementations.First()->SyntaxNode, Diagnostics::seePreviousDefinition);
                    }
                    else if (parent->SubClosures.TryGetValue(comp.Key, su))
                    {
						err->diagnose(comp.Value->Implementations.First()->SyntaxNode->Position, Diagnostics::nameAlreadyDefinedInCurrentScope, comp.Key);
                        err->diagnose(su->UsingPosition, Diagnostics::seePreviousDefinition);
                    }
				}
			}
			for (auto & c : child->SubClosures)
			{
				if (c.Value->IsInPlace)
				{
					RefPtr<ShaderComponentSymbol> ccomp;
					RefPtr<ShaderClosure> su;
                    if (parent->Components.TryGetValue(c.Key, ccomp))
                    {
                        err->diagnose(c.Value->UsingPosition, Diagnostics::nameAlreadyDefinedInCurrentScope, c.Key);
                        err->diagnose(ccomp->Implementations.First()->SyntaxNode, Diagnostics::seePreviousDefinition);
                    }
                    else if (parent->SubClosures.TryGetValue(c.Key, su))
                    {
                        err->diagnose(c.Value->UsingPosition, Diagnostics::nameAlreadyDefinedInCurrentScope, c.Key);
                        err->diagnose(su->UsingPosition, Diagnostics::seePreviousDefinition);
                    }
					for (auto & sc : c.Value->SubClosures)
						if (sc.Value->IsInPlace)
							CheckComponentRedefinition(err, parent, sc.Value.Ptr());
				}
			}
		}
		RefPtr<ShaderClosure> CreateShaderClosure(DiagnosticSink * err, SymbolTable * symTable, ShaderSymbol * shader, CodePosition usingPos, 
			ShaderClosure * rootShader,
			const EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>>& pRefMap)
		{
			RefPtr<ShaderClosure> rs = new ShaderClosure();
			if (rootShader == nullptr)
			{
				rootShader = rs.Ptr();
				rootShader->Pipeline = shader->ParentPipeline;
			}
			rs->ModuleSyntaxNode = shader->SyntaxNode;
			rs->Name = shader->SyntaxNode->Name.Content;
			rs->RefMap = pRefMap;
			if (shader->ParentPipeline && rootShader->Pipeline)
			{
				if (shader->ParentPipeline->IsChildOf(rootShader->Pipeline))
					rootShader->Pipeline = shader->ParentPipeline;
				else if (!rootShader->Pipeline->IsChildOf(shader->ParentPipeline))
				{
                    err->diagnose(shader->SyntaxNode->Position, Diagnostics::pipelineOfModuleIncompatibleWithPipelineOfShader,
                        shader->ParentPipeline->SyntaxNode->Name,
                        shader->SyntaxNode->Name.Content,
                        rootShader->Pipeline->SyntaxNode->Name.Content,
                        rootShader->Name);
                    err->diagnose(shader->SyntaxNode->Position, Diagnostics::seeDefinitionOfShader, shader->SyntaxNode->Name);
				}
			}
			
			rs->Pipeline = rootShader->Pipeline;
			rs->UsingPosition = usingPos;
			rs->Position = shader->SyntaxNode->Position;
			for (auto & mbr : shader->SyntaxNode->Members)
			{
				if (auto import = dynamic_cast<ImportSyntaxNode*>(mbr.Ptr()))
				{
					// create component for each argument
					EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>> refMap;
					for (auto & arg : import->Arguments)
					{
						RefPtr<ShaderComponentSymbol> ccomp = new ShaderComponentSymbol();
						auto compName = "arg" + String(rs->Components.Count()) + "_" + 
							(import->ObjectName.Content.Length()==0?import->ShaderName.Content:import->ObjectName.Content) + arg->ArgumentName.Content;
						auto impl = new ShaderComponentImplSymbol();
						auto compSyntax = new ComponentSyntaxNode();
						compSyntax->Position = arg->Expression->Position;
						compSyntax->Name.Content = compName;
						CloneContext cloneCtx;
						compSyntax->Expression = arg->Expression->Clone(cloneCtx);
						compSyntax->TypeNode = new BasicTypeSyntaxNode();
						compSyntax->TypeNode->Position = compSyntax->Position;
						impl->SyntaxNode = compSyntax;
						ccomp->Name = compName;
						ccomp->Type = new Type();
						ccomp->Type->DataType = arg->Expression->Type;
						ccomp->Implementations.Add(impl);
						rs->Components[compName] = ccomp;
						refMap[arg->ArgumentName.Content] = ccomp;
					}
					RefPtr<ShaderSymbol> shaderSym;
					if (symTable->Shaders.TryGetValue(import->ShaderName.Content, shaderSym))
					{
						// fill in automatic arguments
						for (auto & param : shaderSym->Components)
						{
							if (param.Value->IsRequire() && !refMap.ContainsKey(param.Key))
							{
								auto arg = rs->FindComponent(param.Key);
								if (arg && arg->Type->DataType->Equals(param.Value->Type->DataType.Ptr()))
								{
									refMap[param.Key] = arg;
								}
							}
						}
						auto refClosure = CreateShaderClosure(err, symTable, shaderSym.Ptr(), import->Position, rootShader, refMap);
						refClosure->IsPublic = import->IsPublic();
						refClosure->Parent = rs.Ptr();
						Token bindingVal;
						if (import->FindSimpleAttribute("Binding", bindingVal))
						{
							refClosure->BindingIndex = StringToInt(bindingVal.Content);
						}
						if (import->IsInplace)
						{
							refClosure->IsInPlace = true;
							CheckComponentRedefinition(err, rs.Ptr(), refClosure.Ptr());
							rs->SubClosures["annonymousObj" + String(UniqueIdGenerator::Next())] = refClosure;
						}
						else
						{
							rs->SubClosures[import->ObjectName.Content] = refClosure;
						}
					}
				}
				else if (auto compt = dynamic_cast<ComponentSyntaxNode*>(mbr.Ptr()))
				{
					RefPtr<ShaderComponentSymbol> comp;
					if (shader->Components.TryGetValue(compt->Name.Content, comp) &&
						!rs->Components.ContainsKey(compt->Name.Content))
					{
						RefPtr<ShaderComponentSymbol> ccomp = new ShaderComponentSymbol(*comp);
						rs->Components.Add(comp->Name, ccomp);
					}
				}
			}
			// check for unassigned arguments
			for (auto & comp : shader->Components)
			{
				if (comp.Value->Implementations.First()->SyntaxNode->IsRequire() &&
					!pRefMap.ContainsKey(comp.Key))
				{
                    err->diagnose(rs->UsingPosition, Diagnostics::parameterOfModuleIsUnassigned, comp.Key, shader->SyntaxNode->Name);
					// try to provide more info on why it is unassigned
					auto arg = rootShader->FindComponent(comp.Key, true, false);
                    if (!arg)
                        err->diagnose(rootShader, Diagnostics::implicitParameterMatchingFailedBecauseShaderDoesNotDefineComponent,
                            rootShader->Name,
                            comp.Key);
					else
					{
						if (comp.Value->Type->DataType->Equals(arg->Type->DataType.Ptr()))
						{
                            err->diagnose(rs->UsingPosition, Diagnostics::implicitParameterMatchingFailedBecauseNameNotAccessible, shader->SyntaxNode->Name);
						}
						else
						{
                            err->diagnose(rs->UsingPosition, Diagnostics::implicitParameterMatchingFailedBecauseTypeMismatch, comp.Value->Type->DataType);
						}
                        err->diagnose(comp.Value->Implementations.First()->SyntaxNode, Diagnostics::seeRequirementDeclaration);
                        err->diagnose(arg->Implementations.First()->SyntaxNode, Diagnostics::seePotentialDefinitionOfComponent, comp.Key);
					}
				}
			}
			return rs;
		}

		RefPtr<ShaderClosure> CreateShaderClosure(DiagnosticSink * err, SymbolTable * symTable, ShaderSymbol * shader)
		{
			return CreateShaderClosure(err, symTable, shader, shader->SyntaxNode->Position, nullptr, EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>>());
		}

		class ReplaceReferenceVisitor : public SyntaxVisitor
		{
		private:
			ShaderClosure * shaderClosure = nullptr;
			ShaderComponentSymbol * currentComponent = nullptr;
			ImportExpressionSyntaxNode * currentImport = nullptr;
			void ReplaceReference(RefPtr<StringObject> refComp)
			{
				String targetComp;
				if ((*replacements).TryGetValue(refComp->Content, targetComp))
				{
					auto oldComp = shaderClosure->AllComponents[refComp->Content]().Symbol;
					auto newComp = shaderClosure->AllComponents[targetComp]().Symbol;
					if (auto * importOps = currentComponent->DependentComponents.TryGetValue(newComp))
						importOps->Add(currentImport);
					else
					{
						EnumerableHashSet<RefPtr<ImportExpressionSyntaxNode>> op;
						op.Add(currentImport);
						currentComponent->DependentComponents.Add(newComp, op);
					}
					currentComponent->DependentComponents.Remove(oldComp);
					if (auto * importOps = currentImpl->DependentComponents.TryGetValue(newComp))
						importOps->Add(currentImport);
					else
					{
						EnumerableHashSet<RefPtr<ImportExpressionSyntaxNode>> op;
						op.Add(currentImport);
						currentImpl->DependentComponents.Add(newComp, op);
					}
					currentImpl->DependentComponents.Remove(oldComp);
					currentImpl->ComponentReferencePositions[newComp] = currentImpl->ComponentReferencePositions[oldComp]();
					refComp->Content = newComp->UniqueName;
				}
			}
		public:
			ShaderComponentImplSymbol * currentImpl = nullptr;
			EnumerableDictionary<String, String> * replacements;
			ReplaceReferenceVisitor(ShaderClosure * closure, ShaderComponentSymbol * comp, EnumerableDictionary<String, String> &pReplacements)
				: SyntaxVisitor(nullptr), shaderClosure(closure), currentComponent(comp), replacements(&pReplacements)
			{}

			RefPtr<ExpressionSyntaxNode> VisitImportExpression(ImportExpressionSyntaxNode * import) override
			{
				currentImport = import;
				import->Component->Accept(this);
				if (import->Component->Tags.ContainsKey("ComponentReference"))
				{
					import->ComponentUniqueName = import->Component->Tags["ComponentReference"]().As<StringObject>()->Content;
				}
				currentImport = nullptr;
				for (auto & arg : import->Arguments)
					arg->Accept(this);
				return import;
			}

			RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode * var) override
			{
				RefPtr<Object> compRef;
				if (var->Tags.TryGetValue("ComponentReference", compRef))
				{
					ReplaceReference(compRef.As<StringObject>());
				}
				return var;
			}

			RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * member) override
			{
				member->BaseExpression->Accept(this);
				RefPtr<Object> compRef;
				if (member->Tags.TryGetValue("ComponentReference", compRef))
				{
					ReplaceReference(compRef.As<StringObject>());
				}
				return member;
			}
		};

		class ResolveDependencyVisitor : public SyntaxVisitor
		{
		private:
			ShaderClosure * shaderClosure = nullptr, *rootShader = nullptr;
			ShaderComponentSymbol * currentComponent = nullptr;
			ImportExpressionSyntaxNode * currentImport = nullptr;
			void AddReference(ShaderComponentSymbol * referee, ImportExpressionSyntaxNode * importOp, CodePosition pos)
			{
				ComponentInstance referedCompInst;
				if (rootShader->AllComponents.TryGetValue(referee->UniqueName, referedCompInst))
					referee = referedCompInst.Symbol;
				if (auto * importOps = currentComponent->DependentComponents.TryGetValue(referee))
					importOps->Add(importOp);
				else
				{
					EnumerableHashSet<RefPtr<ImportExpressionSyntaxNode>> op;
					op.Add(importOp);
					currentComponent->DependentComponents.Add(referee, op);
				}

				if (auto * importOps = currentImpl->DependentComponents.TryGetValue(referee))
					importOps->Add(importOp);
				else
				{
					EnumerableHashSet<RefPtr<ImportExpressionSyntaxNode>> op;
					op.Add(importOp);
					currentImpl->DependentComponents.Add(referee, op);
				}
				currentImpl->ComponentReferencePositions[referee] = pos;
			}
		public:
			ShaderComponentImplSymbol * currentImpl = nullptr;

			ResolveDependencyVisitor(DiagnosticSink * err, ShaderClosure * pRootShader, ShaderClosure * closure, ShaderComponentSymbol * comp)
				: SyntaxVisitor(err), shaderClosure(closure), rootShader(pRootShader), currentComponent(comp)
			{}

			RefPtr<ExpressionSyntaxNode> VisitImportExpression(ImportExpressionSyntaxNode * import) override
			{
				currentImport = import;
				import->Component->Accept(this);
				if (!import->Component->Tags.ContainsKey("ComponentReference"))
				{
					getSink()->diagnose(import->Component.Ptr(), Diagnostics::firstArgumentToImportNotComponent);
				}
				else
				{
					import->ComponentUniqueName = import->Component->Tags["ComponentReference"]().As<StringObject>()->Content;
				}
				currentImport = nullptr;
				for (auto & arg : import->Arguments)
					arg->Accept(this);
				import->ImportOperatorDef->Accept(this);
				return import;
			}

			RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode * var) override
			{
                if (auto decl = var->Scope->LookUp(var->Variable))
                {
                    // TODO(tfoley): wire up the variable to the declaration
                    return var;
                }
                // Otherwise look in other places...
				if (var->Type->AsBasicType() && var->Type->AsBasicType()->Component)
				{
					if (auto comp = shaderClosure->FindComponent(var->Type->AsBasicType()->Component->Name))
					{
						if (comp->Implementations.First()->SyntaxNode->IsRequire())
							shaderClosure->RefMap.TryGetValue(comp->Name, comp);
						var->Tags["ComponentReference"] = new StringObject(comp->UniqueName);
						AddReference(comp.Ptr(), currentImport, var->Position);
					}
					else
						throw InvalidProgramException("cannot resolve reference.");
				}
				if (auto comp = shaderClosure->FindComponent(var->Variable))
				{
					if (comp->Implementations.First()->SyntaxNode->IsRequire())
						shaderClosure->RefMap.TryGetValue(var->Variable, comp);
					var->Tags["ComponentReference"] = new StringObject(comp->UniqueName);

					AddReference(comp.Ptr(), currentImport, var->Position);
				}
				else if (auto closure = shaderClosure->FindClosure(var->Variable))
				{
					ShaderSymbol * originalShader = nullptr;
					if (var->Type->AsBasicType())
						originalShader = var->Type->AsBasicType()->Shader;
					var->Type = new BasicExpressionType(originalShader, closure.Ptr());
				}
				else if (!(var->Type->AsBasicType() && var->Type->AsBasicType()->BaseType == BaseType::Function))
					throw InvalidProgramException("cannot resolve reference.");
				return var;
			}

			RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * member) override
			{
				member->BaseExpression->Accept(this);
				if (member->BaseExpression->Type->AsBasicType() && member->BaseExpression->Type->AsBasicType()->ShaderClosure)
				{
					if (auto comp = member->BaseExpression->Type->AsBasicType()->ShaderClosure->FindComponent(member->MemberName))
					{
						member->Tags["ComponentReference"] = new StringObject(comp->UniqueName);
						AddReference(comp.Ptr(), currentImport, member->Position);
					}
					else if (auto shader = member->BaseExpression->Type->AsBasicType()->ShaderClosure->FindClosure(member->MemberName))
					{
						ShaderSymbol * originalShader = nullptr;
						if (member->Type->AsBasicType())
							originalShader = member->Type->AsBasicType()->Shader;
						member->Type = new BasicExpressionType(originalShader, shader.Ptr());
					}
				}
				else if (member->Type->AsBasicType() && member->Type->AsBasicType()->Component)
				{
					if (auto comp = shaderClosure->FindComponent(member->Type->AsBasicType()->Component->Name))
					{
						member->Tags["ComponentReference"] = new StringObject(comp->UniqueName);
						AddReference(comp.Ptr(), currentImport, member->Position);
					}
					else
						throw InvalidProgramException("cannot resolve reference.");
				}
				return member;
			}
		};

		void ResolveReference(DiagnosticSink * err, ShaderClosure * rootShader, ShaderClosure* shader)
		{
			for (auto & comp : shader->Components)
			{
				ResolveDependencyVisitor depVisitor(err, rootShader, shader, comp.Value.Ptr());
				for (auto & impl : comp.Value->Implementations)
				{
					depVisitor.currentImpl = impl.Ptr();
					impl->SyntaxNode->Accept(&depVisitor);
				}
			}
			for (auto & subClosure : shader->SubClosures)
				ResolveReference(err, rootShader, subClosure.Value.Ptr());
		}

		void ReplaceRefMapReference(ShaderClosure * root, ShaderClosure * shader, EnumerableDictionary<String, String> & replacements)
		{
			for (auto & map : shader->RefMap)
			{
				String newName = map.Value->UniqueName;
				while (replacements.TryGetValue(newName, newName))
				{
				}
				if (newName != map.Value->UniqueName)
					map.Value = root->AllComponents[newName]().Symbol;
			}
			for (auto & subclosure : shader->SubClosures)
				ReplaceRefMapReference(root, subclosure.Value.Ptr(), replacements);
		}


		void ReplaceReference(ShaderClosure * shader, EnumerableDictionary<String, String> & replacements)
		{
			ReplaceRefMapReference(shader, shader, replacements);
			for (auto & comp : shader->AllComponents)
			{
				ReplaceReferenceVisitor replaceVisitor(shader, comp.Value.Symbol, replacements);
				for (auto & impl : comp.Value.Symbol->Implementations)
				{
					replaceVisitor.currentImpl = impl.Ptr();
					impl->SyntaxNode->Accept(&replaceVisitor);
				}
			}
		}

		String GetUniqueCodeName(String name)
		{
			StringBuilder sb;
			for (auto ch : name)
			{
				if (ch == '.')
					sb << "_";
				else
					sb << ch;
			}
			return EscapeDoubleUnderscore(sb.ProduceString());
		}

		bool IsInAbstractWorld(PipelineSymbol * pipeline, ShaderComponentSymbol* comp)
		{
			return comp->Implementations.First()->Worlds.Count() && !comp->Implementations.First()->SyntaxNode->IsRequire() &&
				pipeline->IsAbstractWorld(comp->Implementations.First()->Worlds.First());
		}

		void AssignUniqueNames(ShaderClosure * shader, String namePrefix, String publicNamePrefix)
		{
			for (auto & comp : shader->Components)
			{
				if (IsInAbstractWorld(shader->Pipeline, comp.Value.Ptr()))
				{
					comp.Value->UniqueKey = comp.Value->UniqueName = comp.Value->Name;
				}
				else
				{
					String uniqueChoiceName;
					if (comp.Value->Implementations.First()->SyntaxNode->IsPublic())
						uniqueChoiceName = publicNamePrefix + comp.Key;
					else
						uniqueChoiceName = namePrefix + comp.Key;
					comp.Value->ChoiceNames.Add(uniqueChoiceName);
					comp.Value->UniqueKey = uniqueChoiceName;
					comp.Value->UniqueName = GetUniqueCodeName(uniqueChoiceName);
				}
			}
			for (auto & subClosure : shader->SubClosures)
			{
				if (subClosure.Value->IsInPlace)
					AssignUniqueNames(subClosure.Value.Ptr(), namePrefix + subClosure.Value->Name + ".", publicNamePrefix);
				else
					AssignUniqueNames(subClosure.Value.Ptr(), namePrefix + subClosure.Key + ".", publicNamePrefix + subClosure.Key + ".");
			}
		}

		bool IsConsistentGlobalComponentDefinition(ShaderComponentSymbol * comp0, ShaderComponentSymbol * comp1)
		{
			if (comp0->Type->DataType != comp1->Type->DataType)
				return false;
			if (comp0->Implementations.First()->Worlds.Count() != comp1->Implementations.First()->Worlds.Count())
				return false;
			for (auto w : comp0->Implementations.First()->Worlds)
				if (!comp1->Implementations.First()->Worlds.Contains(w))
					return false;
			return true;
		}

		void GatherComponents(DiagnosticSink * err, ShaderClosure * closure, ShaderClosure * subClosure)
		{
			for (auto & comp : subClosure->Components)
			{
				ShaderComponentSymbol* existingComp = nullptr;
				if (comp.Value->IsRequire())
					continue;
				ComponentInstance existingCompInst;
				if (closure->AllComponents.TryGetValue(comp.Value->UniqueName, existingCompInst))
				{
					existingComp = existingCompInst.Symbol;
					if (IsInAbstractWorld(closure->Pipeline, comp.Value.Ptr()) &&
						IsInAbstractWorld(closure->Pipeline, existingComp))
					{
						if (!IsConsistentGlobalComponentDefinition(comp.Value.Ptr(), existingComp))
						{
                            err->diagnose(comp.Value->Implementations.First()->SyntaxNode, Diagnostics::globalComponentConflictWithPreviousDeclaration, existingComp->Name);
                            err->diagnose(existingComp->Implementations.First()->SyntaxNode, Diagnostics::seePreviousDefinition);
						}
						else
						{
    						// silently ignore consistently defined global components (components in abstract worlds)
							err->diagnose(comp.Value->Implementations.First()->SyntaxNode, Diagnostics::componentIsAlreadyDefinedUseRequire, existingComp->Name, closure->Name);
                            err->diagnose(existingComp->Implementations.First()->SyntaxNode, Diagnostics::seePreviousDefinition);
						}
					}
					else if (comp.Value->Implementations.First()->SyntaxNode->Parameters.Count() == 0)
					{
						err->diagnose(comp.Value->Implementations.First()->SyntaxNode, Diagnostics::componentAlreadyDefinedWhenCompiling, comp.Value->UniqueKey, closure->Name);
						auto currentClosure = subClosure;
						while (currentClosure != nullptr && currentClosure != closure)
						{
                            err->diagnose(currentClosure->UsingPosition, Diagnostics::seeInclusionOf, currentClosure->Name);
							currentClosure = currentClosure->Parent;
						}
					}
				}
				closure->AllComponents[comp.Value->UniqueName] = ComponentInstance(subClosure, comp.Value.Ptr());
			}
			for (auto & sc : subClosure->SubClosures)
				GatherComponents(err, closure, sc.Value.Ptr());
		}

		bool IsWorldFeasible(SymbolTable * symTable, PipelineSymbol * pipeline, ShaderComponentImplSymbol * impl, String world, ShaderComponentSymbol*& unaccessibleComp)
		{
			// shader parameter (uniform values) are available to all worlds
			if (impl->SyntaxNode->IsParam())
				return true;
			bool isWFeasible = true;
			for (auto & dcomp : impl->DependentComponents)
			{
				if (dcomp.Value.Contains(nullptr))
				{
					bool reachable = false;
					for (auto & dw : dcomp.Key->Type->FeasibleWorlds)
					{
						if (symTable->IsWorldImplicitlyReachable(pipeline, dw, world, dcomp.Key->Type->DataType))
						{
							reachable = true;
							break;
						}
					}
					if (!reachable)
					{
						unaccessibleComp = dcomp.Key;
						isWFeasible = false;
						break;
					}
				}
			}
			return isWFeasible;
		}

		void SolveWorldConstraints(DiagnosticSink * err, SymbolTable * symTable, ShaderClosure * shader)
		{
			EnumerableHashSet<String> allWorlds;
			for (auto w : shader->Pipeline->Worlds)
				if (!shader->Pipeline->IsAbstractWorld(w.Key))
					allWorlds.Add(w.Key);
			auto depOrder = shader->GetDependencyOrder();
			for (auto & comp : depOrder)
			{
				comp->Type->FeasibleWorlds.Clear();
				for (auto & impl : comp->Implementations)
				{
					auto autoWorld = allWorlds;
					for (auto & w : impl->Worlds)
					{
						ShaderComponentSymbol* unaccessibleComp = nullptr;
						if (!IsWorldFeasible(symTable, shader->Pipeline, impl.Ptr(), w, unaccessibleComp))
						{
                            err->diagnose(impl->ComponentReferencePositions[unaccessibleComp](), Diagnostics::componentCantBeComputedAtWorldBecauseDependentNotAvailable,
                                comp->Name,
                                w,
                                unaccessibleComp->Name);
                            err->diagnose(unaccessibleComp->Implementations.First()->SyntaxNode, Diagnostics::seeDefinitionOf, unaccessibleComp->Name);
						}
						// if a world is explicitly stated in any of the component defintions, remove it from autoWorld
						autoWorld.Remove(w);
					}
				}
				for (auto & impl : comp->Implementations)
				{
					if (impl->Worlds.Count() == 0) // if this component definition is not qualified with a world
					{
						EnumerableHashSet<String> deducedWorlds = allWorlds;
						EnumerableHashSet<String> feasibleWorlds;
						// the auto-deduced world for this definition is all feasible worlds in autoWorld.
						for (auto & w : deducedWorlds)
						{
							ShaderComponentSymbol* unaccessibleComp = nullptr;
							bool isWFeasible = IsWorldFeasible(symTable, shader->Pipeline, impl.Ptr(), w, unaccessibleComp);
							if (isWFeasible)
								feasibleWorlds.Add(w);
						}
						impl->Worlds = feasibleWorlds;
					}
					for (auto & w : impl->Worlds)
						comp->Type->FeasibleWorlds.Add(w);
				}
			}
			for (auto & comp : depOrder)
			{
				comp->Type->ConstrainedWorlds = comp->Type->FeasibleWorlds;
			}
			auto useInWorld = [&](String comp, String world)
			{
				// comp is used in world, restrict comp.ContainedWorlds to guarantee
				// all candidate definitions can reach world
				RefPtr<ShaderComponentSymbol> compSym;
				if (shader->Components.TryGetValue(comp, compSym))
				{
					EnumerableHashSet<String> newWorlds;
					for (auto & w : compSym->Type->ConstrainedWorlds)
						if (symTable->IsWorldReachable(shader->Pipeline, w, world, compSym->Type->DataType))
							newWorlds.Add(w);
					compSym->Type->ConstrainedWorlds = _Move(newWorlds);
				}
			};
			for (auto impOp : shader->Pipeline->SyntaxNode->GetImportOperators())
			{
				for (auto comp : impOp->Usings)
				{
					useInWorld(comp, impOp->DestWorld.Content);
				}
			}
		}

		bool CheckCircularReference(DiagnosticSink * err, ShaderClosure * shader)
		{
			bool rs = false;
			for (auto & comp : shader->AllComponents)
			{
				for (auto & impl : comp.Value.Symbol->Implementations)
				{
					// check circular references
					HashSet<ShaderComponentSymbol*> set;
					List<ShaderComponentSymbol*> referredComponents;
					referredComponents.Add(comp.Value.Symbol);
					for (int i = 0; i < referredComponents.Count(); i++)
					{
						auto xcomp = referredComponents[i];
						for (auto & xcompImpl : xcomp->Implementations)
						{
							for (auto & rcomp : xcompImpl->DependentComponents)
							{
								if (set.Add(rcomp.Key))
								{
									referredComponents.Add(rcomp.Key);
								}
								if (rcomp.Key == comp.Value.Symbol)
								{
									err->diagnose(impl->SyntaxNode->Position, Diagnostics::circularReferenceNotAllowed, rcomp.Key->Name);
									rs = true;
								}
							}
						}
					}
				}
			}
			return rs;
		}

		void PropagateArgumentConstraints(ShaderComponentSymbol * requirement, ShaderComponentSymbol * arg)
		{
			for (auto w : requirement->Implementations.First()->ExportWorlds)
			{
				for (auto impl : arg->Implementations)
				{
					if (impl->Worlds.Contains(w))
						impl->ExportWorlds.Add(w);
				}
			}
			for (auto w : requirement->Implementations.First()->SrcPinnedWorlds)
			{
				for (auto impl : arg->Implementations)
				{
					if (impl->Worlds.Contains(w))
						impl->SrcPinnedWorlds.Add(w);
				}
			}
		}

		void VerifyAndPropagateArgumentConstraints(DiagnosticSink * err, SymbolTable * symTable, ShaderClosure * shader)
		{
			for (auto & map : shader->RefMap)
			{
				auto & arg = map.Value;
				RefPtr<ShaderComponentSymbol> requirement;
				if (shader->Components.TryGetValue(map.Key, requirement) && requirement->IsRequire())
				{
					if (requirement->Implementations.First()->SyntaxNode->Rate)
					{
						for (auto w : requirement->Implementations.First()->Worlds)
						{
							if (!symTable->IsWorldImplicitlyReachable(shader->Pipeline, arg->Type->FeasibleWorlds, w, requirement->Type->DataType))
							{
                                err->diagnose(arg->Implementations.First()->SyntaxNode->Position, Diagnostics::argumentNotAvilableInWorld,
                                    arg->Name,
                                    w,
                                    shader->Name);
                                err->diagnose(requirement->Implementations.First()->SyntaxNode->Position, Diagnostics::seeRequirementDeclaration);
							}
						}
						PropagateArgumentConstraints(requirement.Ptr(), arg.Ptr());
					}
				}
			}
			for (auto & subClosure : shader->SubClosures)
				VerifyAndPropagateArgumentConstraints(err, symTable, subClosure.Value.Ptr());
		}

		void AddPipelineComponents(ShaderClosure * shader)
		{
			for (auto & comp : shader->Pipeline->Components)
			{
				if (!comp.Value->IsRequire())
					shader->Components.AddIfNotExists(comp.Key, new ShaderComponentSymbol(*comp.Value));
			}
		}

		void GatherArgumentMappings(EnumerableDictionary<String, String> & result, ShaderClosure* shader)
		{
			for (auto & map : shader->RefMap)
			{
				result[shader->Components[map.Key]()->UniqueName] = map.Value->UniqueName;
			}
			for (auto & subShader : shader->SubClosures)
				GatherArgumentMappings(result, subShader.Value.Ptr());
		}

		void RemoveTrivialComponents(ShaderClosure * shader)
		{
			// remove trivial components, e.g. if A = B, replace all references to A with B.
			// this is not just an optimization, it is also critical for CodeGen because 
			// code gen does not support components that returns another function component or sampler2D etc.
			// i.e. function/sampler2D components must be referenced directly.
			EnumerableDictionary<String, String> compSub;
			for (auto & comp : shader->AllComponents)
			{
				// if this component is required by pipeline (e.g. gl_Position), do not attempt to remove it
				if (shader->Pipeline->Components.ContainsKey(comp.Key))
					continue;

				if (comp.Value.Symbol->Implementations.Count() == 1 &&
					comp.Value.Symbol->Implementations.First()->SyntaxNode->Expression &&
					!comp.Value.Symbol->Implementations.First()->SyntaxNode->IsOutput())
				{
					RefPtr<Object> compRef;
					if (comp.Value.Symbol->Implementations.First()->SyntaxNode->Expression->Tags.TryGetValue("ComponentReference", compRef))
					{
						compSub[comp.Key] = compRef.As<StringObject>()->Content;
					}
				}
			}
			// gather argument mappings
			EnumerableDictionary<String, String> arguments;
			GatherArgumentMappings(arguments, shader);
			EnumerableDictionary<String, String> replacements;
			for (auto & replace : compSub)
			{
				// search transitively for replaceDest;
				String replaceDest = replace.Key;
				while (compSub.ContainsKey(replaceDest))
				{
					replaceDest = compSub[replaceDest]();
					arguments.TryGetValue(replaceDest, replaceDest);
				}
				if (replace.Key != replaceDest)
					replacements[replace.Key] = replaceDest;
			}
			ReplaceReference(shader, replacements);
			for (auto & r : replacements)
				shader->AllComponents.Remove(r.Key);
		}

		void PropagatePipelineRequirements(DiagnosticSink * err, ShaderClosure * shader)
		{
			for (auto & req : shader->Pipeline->Components)
			{
				if (req.Value->IsRequire())
				{
					ComponentInstance comp;
					
					StringBuilder errMsg;
					if (shader->AllComponents.TryGetValue(req.Key, comp))
					{
						if (!comp.Symbol->Type->DataType->Equals(req.Value->Type->DataType.Ptr()))
						{
                            err->diagnose(comp.Symbol->Implementations.First()->SyntaxNode, Diagnostics::componentTypeNotWhatPipelineRequires,
                                req.Key,
								comp.Symbol->Type->DataType,
                                shader->Pipeline->SyntaxNode->Name.Content,
                                req.Value->Type->DataType);
                            err->diagnose(req.Value->Implementations.First()->SyntaxNode, Diagnostics::seePipelineRequirementDefinition);
						}
					}
					else
					{
                        err->diagnose(shader->Position, Diagnostics::shaderDoesNotDefineComponentAsRequiredByPipeline,
                            shader->Name,
                            req.Key,
                            shader->Pipeline->SyntaxNode->Name);
                        err->diagnose(req.Value->Implementations.First()->SyntaxNode, Diagnostics::seePipelineRequirementDefinition);
					}
				}
			}
		}

		void diagnoseModuleUsingStack(DiagnosticSink* sink, ShaderClosure * shader)
		{
			if (shader->Parent)
			{
                sink->diagnose(shader, Diagnostics::seeModuleBeingUsedIn, shader->Name, shader->Parent->Name);
				diagnoseModuleUsingStack(sink, shader->Parent);
			}
			else
			{
                sink->diagnose(shader, Diagnostics::noteShaderIsTargetingPipeine, shader->Name, shader->Pipeline->SyntaxNode->Name);
                sink->diagnose(shader->Pipeline->SyntaxNode, Diagnostics::alsoSeePipelineDefinition);
			}
		}
	
		void CheckPipelineShaderConsistency(DiagnosticSink * err, ShaderClosure * shader)
		{
			for (auto & comp : shader->Components)
			{
				for (auto & impl : comp.Value->Implementations)
				{
					bool inAbstractWorld = false;
					if (impl->SyntaxNode->Rate)
					{
						auto & userSpecifiedWorlds = impl->SyntaxNode->Rate->Worlds;
						for (auto & world : userSpecifiedWorlds)
						{
							{
                                if (!shader->Pipeline->WorldDependency.ContainsKey(world.World.Content))
                                {
									err->diagnose(world.World.Position, Diagnostics::worldIsNotDefinedInPipeline, world.World, shader->Pipeline->SyntaxNode->Name);
    								diagnoseModuleUsingStack(err, shader);
                                }
							}
							WorldSyntaxNode* worldDecl;
							if (shader->Pipeline->Worlds.TryGetValue(world.World.Content, worldDecl))
							{
								if (worldDecl->IsAbstract())
								{
									inAbstractWorld = true;
									if (userSpecifiedWorlds.Count() > 1)
									{
										err->diagnose(world.World.Position, Diagnostics::abstractWorldCannotAppearWithOthers);
										diagnoseModuleUsingStack(err, shader);
									}
								}
							}
						}
					}
					if (!inAbstractWorld && !impl->SyntaxNode->IsRequire() && !impl->SyntaxNode->IsInput() && !impl->SyntaxNode->IsParam()
						&& !impl->SyntaxNode->Expression && !impl->SyntaxNode->BlockStatement)
					{
						err->diagnose(impl->SyntaxNode->Position, Diagnostics::nonAbstractComponentMustHaveImplementation);
					}

					bool isDefinedInAbstractWorld = false, isDefinedInNonAbstractWorld = false;
					if (impl->SyntaxNode->Rate)
					{
						for (auto & w : impl->SyntaxNode->Rate->Worlds)
						{
							auto world = shader->Pipeline->Worlds.TryGetValue(w.World.Content);
							if (world)
							{
								if ((*world)->IsAbstract())
									isDefinedInAbstractWorld = true;
								else
									isDefinedInNonAbstractWorld = true;
							}
						}
					}
					else
						isDefinedInNonAbstractWorld = true;
					if (impl->SyntaxNode->Expression || impl->SyntaxNode->BlockStatement)
					{
						if (isDefinedInAbstractWorld)
							err->diagnose(impl->SyntaxNode->Position, Diagnostics::componentInInputWorldCantHaveCode, impl->SyntaxNode->Name);
					}
				}
			}
			for (auto & subShader : shader->SubClosures)
				CheckPipelineShaderConsistency(err, subShader.Value.Ptr());
		}

		void FlattenShaderClosure(DiagnosticSink * err, SymbolTable * symTable, ShaderClosure * shader)
		{
			// add input(extern) components from pipeline
			AddPipelineComponents(shader);
			CheckPipelineShaderConsistency(err, shader);
			// assign choice names
			AssignUniqueNames(shader, "", "");
			// traverse closures to get component list
			GatherComponents(err, shader, shader);
			PropagatePipelineRequirements(err, shader);
			ResolveReference(err, shader, shader);
			// propagate world constraints
			if (CheckCircularReference(err, shader))
				return;
			if (err->GetErrorCount())
				return;
			RemoveTrivialComponents(shader);
			SolveWorldConstraints(err, symTable, shader);
			// check pipeline constraints
			for (auto & requirement : shader->Pipeline->Components)
			{
				if (!requirement.Value->IsRequire())
					continue;
				auto comp = shader->FindComponent(requirement.Key);
				if (!comp)
				{
                    err->diagnose(shader->Position, Diagnostics::shaderDoesProvideRequirement,
                        shader->Name,
                        requirement.Key,
                        shader->Pipeline->SyntaxNode->Name.Content);
                    err->diagnose(requirement.Value->Implementations.First()->SyntaxNode, Diagnostics::seeRequirementDeclaration);
				}
				else
				{
					for (auto & impl : requirement.Value->Implementations)
					{
						for (auto w : impl->Worlds)
						{
							if (!symTable->IsWorldImplicitlyReachable(shader->Pipeline, comp->Type->FeasibleWorlds, w, requirement.Value->Type->DataType))
							{
                                err->diagnose(comp->Implementations.First()->SyntaxNode, Diagnostics::componentNotAvilableInWorld,
                                    comp->Name,
                                    w,
                                    shader->Pipeline->SyntaxNode->Name);
                                err->diagnose(requirement.Value->Implementations.First()->SyntaxNode, Diagnostics::seeRequirementDeclaration);
							}
						}
					}
					PropagateArgumentConstraints(requirement.Value.Ptr(), comp.Ptr());
				}
			}
			// check argument constraints
			VerifyAndPropagateArgumentConstraints(err, symTable, shader);
		}
	}
}