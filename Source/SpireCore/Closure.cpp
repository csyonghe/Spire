#include "Closure.h"

namespace Spire
{
	namespace Compiler
	{
		void CheckComponentRedefinition(ErrorWriter * err, ShaderClosure * parent, ShaderClosure * child)
		{
			for (auto & comp : child->Components)
			{
				RefPtr<ShaderComponentSymbol> ccomp;
				RefPtr<ShaderClosure> su;
				if ((comp.Value->Implementations.First()->SyntaxNode->IsPublic ||
					comp.Value->Implementations.First()->SyntaxNode->IsOutput))
				{
					if (parent->Components.TryGetValue(comp.Key, ccomp))
						err->Error(33022, L"\'" + comp.Key + L"\' is already defined in current scope.\nsee previous definition at " + ccomp->Implementations.First()->SyntaxNode->Position.ToString(),
							comp.Value->Implementations.First()->SyntaxNode->Position);
					else if (parent->SubClosures.TryGetValue(comp.Key, su))
						err->Error(33022, L"\'" + comp.Key + L"\' is already defined in current scope.\nsee previous definition at " + su->UsingPosition.ToString(),
							comp.Value->Implementations.First()->SyntaxNode->Position);
				}
			}
			for (auto & c : child->SubClosures)
			{
				if (c.Value->IsInPlace)
				{
					RefPtr<ShaderComponentSymbol> ccomp;
					RefPtr<ShaderClosure> su;
					if (parent->Components.TryGetValue(c.Key, ccomp))
						err->Error(33022, L"\'" + c.Key + L"\' is already defined in current scope.\nsee previous definition at " + ccomp->Implementations.First()->SyntaxNode->Position.ToString(),
							c.Value->UsingPosition);
					else if (parent->SubClosures.TryGetValue(c.Key, su))
						err->Error(33022, L"\'" + c.Key + L"\' is already defined in current scope.\nsee previous definition at " + su->UsingPosition.ToString(),
							c.Value->UsingPosition);
					for (auto & sc : c.Value->SubClosures)
						if (sc.Value->IsInPlace)
							CheckComponentRedefinition(err, parent, sc.Value.Ptr());
				}
			}
		}
		RefPtr<ShaderClosure> CreateShaderClosure(ErrorWriter * err, SymbolTable * symTable, ShaderSymbol * shader, CodePosition usingPos, const Dictionary<String, RefPtr<ShaderComponentSymbol>>& pRefMap)
		{
			RefPtr<ShaderClosure> rs = new ShaderClosure();
			rs->Name = shader->SyntaxNode->Name.Content;
			rs->RefMap = pRefMap;
			rs->Pipeline = shader->Pipeline;
			rs->UsingPosition = usingPos;
			rs->Position = shader->SyntaxNode->Position;
			for (auto & mbr : shader->SyntaxNode->Members)
			{
				if (auto import = dynamic_cast<ImportSyntaxNode*>(mbr.Ptr()))
				{
					// create component for each argument
					Dictionary<String, RefPtr<ShaderComponentSymbol>> refMap;
					for (auto & arg : import->Arguments)
					{
						RefPtr<ShaderComponentSymbol> ccomp = new ShaderComponentSymbol();
						auto compName = L"arg" + String(rs->Components.Count()) + L"_" + 
							(import->ObjectName.Content.Length()==0?import->ShaderName.Content:import->ObjectName.Content) + arg->ArgumentName.Content;
						auto impl = new ShaderComponentImplSymbol();
						auto compSyntax = new ComponentSyntaxNode();
						compSyntax->Position = arg->Expression->Position;
						compSyntax->Name.Content = compName;
						CloneContext cloneCtx;
						compSyntax->Expression = arg->Expression->Clone(cloneCtx);
						compSyntax->Type = TypeSyntaxNode::FromExpressionType(arg->Expression->Type);
						compSyntax->Type->Position = compSyntax->Position;
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
							if (param.Value->IsParam() && !refMap.ContainsKey(param.Key))
							{
								auto arg = rs->FindComponent(param.Key);
								if (arg && arg->Type->DataType == param.Value->Type->DataType)
								{
									refMap[param.Key] = arg;
								}
							}
						}
						auto refClosure = CreateShaderClosure(err, symTable, shaderSym.Ptr(), import->Position, refMap);
						refClosure->IsPublic = import->IsPublic;
						refClosure->Parent = rs.Ptr();
						if (import->IsInplace)
						{
							refClosure->IsInPlace = true;
							CheckComponentRedefinition(err, rs.Ptr(), refClosure.Ptr());
							rs->SubClosures[L"annonymousObj" + String(GUID::Next())] = refClosure;
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
				if (comp.Value->Implementations.First()->SyntaxNode->IsParam &&
					!pRefMap.ContainsKey(comp.Key))
				{
					StringBuilder errMsg;
					errMsg << L"argument '" + comp.Key + L"' is unassigned.";
					// try to provide more info on why it is unassigned
					if (auto arg = rs->FindComponent(comp.Key, true))
						errMsg << L" automatic argument filling failed because the component of the same name is not accessible from '" << shader->SyntaxNode->Name.Content << L"'.";
					else
						errMsg << L" automatic argument filling failed because shader '" << shader->SyntaxNode->Name.Content << L"' does not define component '" + comp.Key + L"'.";
					err->Error(33023,errMsg.ProduceString(), rs->UsingPosition);
				}
			}
			return rs;
		}

		RefPtr<ShaderClosure> CreateShaderClosure(ErrorWriter * err, SymbolTable * symTable, ShaderSymbol * shader)
		{
			return CreateShaderClosure(err, symTable, shader, shader->SyntaxNode->Position, Dictionary<String, RefPtr<ShaderComponentSymbol>>());
		}


		class ResolveDependencyVisitor : public SyntaxVisitor
		{
		private:
			ShaderClosure * shaderClosure = nullptr;
			ShaderComponentSymbol * currentComponent = nullptr;
			void AddReference(ShaderComponentSymbol * referee, CodePosition pos)
			{
				referee->UserComponents.Add(currentComponent);
				currentComponent->DependentComponents.Add(referee);
				currentImpl->DependentComponents.Add(referee);
				currentImpl->ComponentReferencePositions[referee] = pos;
			}
		public:
			ShaderComponentImplSymbol * currentImpl = nullptr;
			ResolveDependencyVisitor(ErrorWriter * err, ShaderClosure * closure, ShaderComponentSymbol * comp)
				: SyntaxVisitor(err), shaderClosure(closure), currentComponent(comp)
			{}

			void VisitVarExpression(VarExpressionSyntaxNode * var) override
			{
				VariableEntry varEntry;
				if (!var->Scope->FindVariable(var->Variable, varEntry))
				{
					if (auto comp = shaderClosure->FindComponent(var->Variable))
					{
						if (comp->Implementations.First()->SyntaxNode->IsParam)
							shaderClosure->RefMap.TryGetValue(var->Variable, comp);
						var->Tags[L"ComponentReference"] = comp;
						AddReference(comp.Ptr(), var->Position);
					}
					else if (auto closure = shaderClosure->FindClosure(var->Variable))
					{
						var->Type.ShaderClosure = closure.Ptr();
					}
				}
			}

			void VisitMemberExpression(MemberExpressionSyntaxNode * member) override
			{
				member->BaseExpression->Accept(this);
				if (member->BaseExpression->Type.ShaderClosure)
				{
					if (auto comp = member->BaseExpression->Type.ShaderClosure->FindComponent(member->MemberName))
					{
						member->Tags[L"ComponentReference"] = comp;
						AddReference(comp.Ptr(), member->Position);
					}
					else if (auto shader = member->BaseExpression->Type.ShaderClosure->FindClosure(member->MemberName))
						member->Type.ShaderClosure = shader.Ptr();
				}
			}
		};

		void ResolveReference(ErrorWriter * err, ShaderClosure* shader)
		{
			for (auto & comp : shader->Components)
			{
				ResolveDependencyVisitor depVisitor(err, shader, comp.Value.Ptr());
				for (auto & impl : comp.Value->Implementations)
				{
					depVisitor.currentImpl = impl.Ptr();
					impl->SyntaxNode->Accept(&depVisitor);
				}
			}
			for (auto & subClosure : shader->SubClosures)
				ResolveReference(err, subClosure.Value.Ptr());
		}

		String GetUniqueCodeName(String name)
		{
			StringBuilder sb;
			for (auto ch : name)
			{
				if (ch == L'.')
					sb << L"I_I";
				else
					sb << ch;
			}
			return sb.ProduceString();
		}

		bool IsInAbstractWorld(PipelineSymbol * pipeline, ShaderComponentSymbol* comp)
		{
			return comp->Implementations.First()->Worlds.Count() &&
				pipeline->IsAbstractWorld(comp->Implementations.First()->Worlds.First());
		}

		void AssignUniqueNames(ShaderClosure * shader, String namePrefix, String publicNamePrefix)
		{
			for (auto & comp : shader->Components)
			{
				if (IsInAbstractWorld(shader->Pipeline, comp.Value.Ptr()))
				{
					comp.Value->UniqueName = comp.Value->Name;
				}
				else
				{
					String uniqueChoiceName;
					if (comp.Value->Implementations.First()->SyntaxNode->IsPublic)
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
					AssignUniqueNames(subClosure.Value.Ptr(), namePrefix + subClosure.Value->Name + L".", publicNamePrefix);
				else
					AssignUniqueNames(subClosure.Value.Ptr(), namePrefix + subClosure.Key + L".", publicNamePrefix + subClosure.Key + L".");
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

		void GatherComponents(ErrorWriter * err, ShaderClosure * closure, ShaderClosure * subClosure)
		{
			for (auto & comp : subClosure->Components)
			{
				ShaderComponentSymbol* existingComp = nullptr;
				if (closure->AllComponents.TryGetValue(comp.Value->UniqueName, existingComp))
				{
					if (IsInAbstractWorld(closure->Pipeline, comp.Value.Ptr()) &&
						IsInAbstractWorld(closure->Pipeline, existingComp))
					{
						// silently ignore consistently defined global components (components in abstract worlds)
						if (!IsConsistentGlobalComponentDefinition(comp.Value.Ptr(), existingComp))
						{
							err->Error(34025, L"'" + existingComp->Name + L"': global component conflicts with previous declaration.\nsee previous declaration at " + existingComp->Implementations.First()->SyntaxNode->Position.ToString(),
								comp.Value->Implementations.First()->SyntaxNode->Position);
						}
						else
						{
							for (auto & user : existingComp->UserComponents)
							{
								user->DependentComponents.Remove(existingComp);
								user->DependentComponents.Add(comp.Value.Ptr());
								for (auto & impl : user->Implementations)
									if (impl->DependentComponents.Contains(existingComp))
									{
										impl->DependentComponents.Remove(existingComp);
										impl->DependentComponents.Add(comp.Value.Ptr());
									}
							}
							err->Warning(34026, L"'" + existingComp->Name + L"': component is already defined when compiling shader '" + closure->Name + L"'. use 'require' to declare it as a parameter. \nsee previous declaration at " + existingComp->Implementations.First()->SyntaxNode->Position.ToString(),
								comp.Value->Implementations.First()->SyntaxNode->Position);
						}
					}
					else
					{
						StringBuilder errBuilder;
						errBuilder << L"component named '" << comp.Value->UniqueKey << L"\' is already defined when compiling '" << closure->Name << L"'.";
						auto currentClosure = subClosure;
						while (currentClosure != nullptr && currentClosure != closure)
						{
							errBuilder << L"\nsee inclusion of '" << currentClosure->Name << L"' at " << currentClosure->UsingPosition.ToString() << L".";
							currentClosure = currentClosure->Parent;
						}
						err->Error(34024, errBuilder.ProduceString(), comp.Value->Implementations.First()->SyntaxNode->Position);
					}
				}
				closure->AllComponents[comp.Value->UniqueName] = comp.Value.Ptr();
			}
			for (auto & sc : subClosure->SubClosures)
				GatherComponents(err, closure, sc.Value.Ptr());
		}

		bool IsWorldFeasible(PipelineSymbol * pipeline, ShaderComponentImplSymbol * impl, String world, ShaderComponentSymbol*& unaccessibleComp)
		{
			bool isWFeasible = true;
			for (auto & dcomp : impl->DependentComponents)
			{
				bool reachable = false;
				for (auto & dw : dcomp->Type->FeasibleWorlds)
				{
					if (pipeline->IsWorldReachable(dw, world))
					{
						reachable = true;
						break;
					}
				}
				if (!reachable)
				{
					unaccessibleComp = dcomp;
					isWFeasible = false;
					break;
				}
			}
			return isWFeasible;
		}

		void SolveWorldConstraints(ErrorWriter * err, ShaderClosure * shader)
		{
			EnumerableHashSet<String> allWorlds;
			for (auto w : shader->Pipeline->Worlds)
				if (!shader->Pipeline->IsAbstractWorld(w.Key))
					allWorlds.Add(w.Key);
			auto depOrder = shader->GetDependencyOrder();
			for (auto & comp : depOrder)
			{
				Dictionary<String, EnumerableHashSet<String>> autoWorlds;
				comp->Type->FeasibleWorlds.Clear();
				for (auto & impl : comp->Implementations)
				{
					if (!autoWorlds.ContainsKey(impl->AlternateName))
						autoWorlds[impl->AlternateName] = allWorlds;
					auto & autoWorld = autoWorlds[impl->AlternateName]();
					for (auto & w : impl->Worlds)
					{
						ShaderComponentSymbol* unaccessibleComp = nullptr;
						if (!IsWorldFeasible(shader->Pipeline, impl.Ptr(), w, unaccessibleComp))
						{
							err->Error(33100, L"'" + comp->Name + L"' cannot be computed at '" + w + L"' because the dependent component '" + unaccessibleComp->Name + L"' is not accessible.\nsee definition of '"
								+ unaccessibleComp->Name + L"' at " + unaccessibleComp->Implementations.First()->SyntaxNode->Position.ToString(),
								impl->ComponentReferencePositions[unaccessibleComp]());
						}
						autoWorld.Remove(w);
					}
				}
				for (auto & impl : comp->Implementations)
				{
					if (impl->Worlds.Count() == 0)
					{
						EnumerableHashSet<String> deducedWorlds = autoWorlds[impl->AlternateName]();
						EnumerableHashSet<String> feasibleWorlds;
						for (auto & w : deducedWorlds)
						{
							ShaderComponentSymbol* unaccessibleComp = nullptr;
							bool isWFeasible = IsWorldFeasible(shader->Pipeline, impl.Ptr(), w, unaccessibleComp);
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
						if (shader->Pipeline->IsWorldReachable(w, world))
							newWorlds.Add(w);
					compSym->Type->ConstrainedWorlds = _Move(newWorlds);
				}
			};
			for (auto impOp : shader->Pipeline->SyntaxNode->ImportOperators)
			{
				for (auto comp : impOp->Usings)
				{
					useInWorld(comp.Content, impOp->DestWorld.Content);
				}
			}
			for (auto & userWorld : shader->Pipeline->Worlds)
			{
				for (auto comp : userWorld.Value.SyntaxNode->Usings)
				{
					useInWorld(comp.Content, userWorld.Key);
				}
			}
		}

		bool CheckCircularReference(ErrorWriter * err, ShaderClosure * shader)
		{
			bool rs = false;
			for (auto & comp : shader->Components)
			{
				for (auto & impl : comp.Value->Implementations)
				{
					// check circular references
					HashSet<ShaderComponentSymbol*> set;
					List<ShaderComponentSymbol*> referredComponents;
					referredComponents.Add(comp.Value.Ptr());
					for (int i = 0; i < referredComponents.Count(); i++)
					{
						auto xcomp = referredComponents[i];
						for (auto & xcompImpl : xcomp->Implementations)
						{
							for (auto & rcomp : xcompImpl->DependentComponents)
							{
								if (set.Add(rcomp))
								{
									referredComponents.Add(rcomp);
								}
								if (rcomp == comp.Value.Ptr())
								{
									err->Error(32013, L"circular reference is not allowed.", impl->SyntaxNode->Position);
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

		void VerifyAndPropagateArgumentConstraints(ErrorWriter * err, ShaderClosure * shader)
		{
			for (auto & map : shader->RefMap)
			{
				auto & arg = map.Value;
				RefPtr<ShaderComponentSymbol> requirement;
				if (shader->Components.TryGetValue(map.Key, requirement) && requirement->IsParam())
				{
					if (requirement->Implementations.First()->SyntaxNode->Rate)
					{
						for (auto w : requirement->Implementations.First()->Worlds)
						{
							if (!shader->Pipeline->IsWorldReachable(arg->Type->FeasibleWorlds, w))
							{
								err->Error(32015, L"argument '" + arg->Name + L"' is not available in world '" + w + L"' as required by '" + shader->Name
									+ L"'.\nsee requirement declaration at " +
									requirement->Implementations.First()->SyntaxNode->Position.ToString(), arg->Implementations.First()->SyntaxNode->Position);
							}
						}
						PropagateArgumentConstraints(requirement.Ptr(), arg.Ptr());
					}
				}
			}
			for (auto & subClosure : shader->SubClosures)
				VerifyAndPropagateArgumentConstraints(err, subClosure.Value.Ptr());
		}

		void FlattenShaderClosure(ErrorWriter * err, ShaderClosure * shader)
		{
			ResolveReference(err, shader);
			// assign choice names
			AssignUniqueNames(shader, L"", L"");
			// traverse closures to get component list
			GatherComponents(err, shader, shader);
			// propagate world constraints
			if (CheckCircularReference(err, shader))
				return;
			SolveWorldConstraints(err, shader);
			// check pipeline constraints
			for (auto & requirement : shader->Pipeline->Components)
			{
				auto comp = shader->FindComponent(requirement.Key);
				if (!comp)
				{
					err->Error(32014, L"shader '" + shader->Name + L"' does not provide '" + requirement.Key + L"' as required by '" + shader->Pipeline->SyntaxNode->Name.Content
						+ L"'.\nsee requirement declaration at " +
						requirement.Value->Implementations.First()->SyntaxNode->Position.ToString(), shader->Position);
				}
				else
				{
					for (auto & impl : requirement.Value->Implementations)
					{
						for (auto w : impl->Worlds)
						{
							if (!shader->Pipeline->IsWorldReachable(comp->Type->FeasibleWorlds, w))
							{
								err->Error(32015, L"component '" + comp->Name + L"' is not available in world '" + w + L"' as required by '" + shader->Pipeline->SyntaxNode->Name.Content
									+ L"'.\nsee requirement declaration at " +
									requirement.Value->Implementations.First()->SyntaxNode->Position.ToString(), comp->Implementations.First()->SyntaxNode->Position);
							}
						}
					}
					PropagateArgumentConstraints(requirement.Value.Ptr(), comp.Ptr());
				}
			}
			// check argument constraints
			VerifyAndPropagateArgumentConstraints(err, shader);
		}
	}
}