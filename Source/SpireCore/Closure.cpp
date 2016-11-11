#include "Closure.h"
#include "StringObject.h"
#include "Naming.h"

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
		RefPtr<ShaderClosure> CreateShaderClosure(ErrorWriter * err, SymbolTable * symTable, ShaderSymbol * shader, CodePosition usingPos, 
			ShaderClosure * rootShader,
			const EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>>& pRefMap)
		{
			RefPtr<ShaderClosure> rs = new ShaderClosure();
			if (rootShader == nullptr)
			{
				rootShader = rs.Ptr();
				rootShader->Pipeline = shader->Pipeline;
			}
			rs->Name = shader->SyntaxNode->Name.Content;
			rs->RefMap = pRefMap;
			if (shader->Pipeline && rootShader->Pipeline)
			{
				if (shader->Pipeline->IsChildOf(rootShader->Pipeline))
					rootShader->Pipeline = shader->Pipeline;
				else
				{
					StringBuilder sb;
					sb << L"pipeline '" << shader->Pipeline->SyntaxNode->Name.Content << L"' targeted by module '" <<
						shader->SyntaxNode->Name.Content << L"' is incompatible with pipeline '" << rootShader->Pipeline->SyntaxNode->Name.Content << L"' targeted by shader '" << rootShader->Name << L"'.\nsee definition of shader '" << shader->SyntaxNode->Name.Content << L"' at " << shader->SyntaxNode->Position.ToString();
					err->Error(33041, sb.ProduceString(), shader->SyntaxNode->Position);
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
						auto compName = L"arg" + String(rs->Components.Count()) + L"_" + 
							(import->ObjectName.Content.Length()==0?import->ShaderName.Content:import->ObjectName.Content) + arg->ArgumentName.Content;
						auto impl = new ShaderComponentImplSymbol();
						auto compSyntax = new ComponentSyntaxNode();
						compSyntax->Position = arg->Expression->Position;
						compSyntax->Name.Content = compName;
						CloneContext cloneCtx;
						compSyntax->Expression = arg->Expression->Clone(cloneCtx);
						compSyntax->TypeNode = TypeSyntaxNode::FromExpressionType(arg->Expression->Type.Ptr());
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
							if (param.Value->IsParam() && !refMap.ContainsKey(param.Key))
							{
								auto arg = rs->FindComponent(param.Key);
								if (arg && arg->Type->DataType->Equals(param.Value->Type->DataType.Ptr()))
								{
									refMap[param.Key] = arg;
								}
							}
						}
						auto refClosure = CreateShaderClosure(err, symTable, shaderSym.Ptr(), import->Position, rootShader, refMap);
						refClosure->IsPublic = import->IsPublic;
						refClosure->Parent = rs.Ptr();
						if (import->IsInplace)
						{
							refClosure->IsInPlace = true;
							CheckComponentRedefinition(err, rs.Ptr(), refClosure.Ptr());
							rs->SubClosures[L"annonymousObj" + String(UniqueIdGenerator::Next())] = refClosure;
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
					errMsg << L"parameter '" << comp.Key << L"' of module '" << shader->SyntaxNode->Name.Content << L"' is unassigned.";
					// try to provide more info on why it is unassigned
					auto arg = rootShader->FindComponent(comp.Key, true, false);
					if (!arg)
						errMsg << L" implicit parameter matching failed because shader '" << rootShader->Name << L"' does not define component '" + comp.Key + L"'.";
					else
					{
						if (comp.Value->Type->DataType->Equals(arg->Type->DataType.Ptr()))
						{
							errMsg << L" implicit parameter matching failed because the component of the same name is not accessible from '" << shader->SyntaxNode->Name.Content << L"'.\ncheck if you have declared necessary requirements and properly used the 'public' qualifier.";
						}
						else
						{
							errMsg << L"implicit parameter matching failed because the component of the same name does not match parameter type '"
								<< comp.Value->Type->DataType->ToString() << L"'.";
						}
						errMsg << L"\nsee requirement declaration at " << comp.Value->Implementations.First()->SyntaxNode->Position.ToString() << L".";
						errMsg << L"\nsee potential definition of component '" << comp.Key << L"' at " << arg->Implementations.First()->SyntaxNode->Position.ToString()
							<< L".\n";
					}
					err->Error(33023,errMsg.ProduceString(), rs->UsingPosition);
				}
			}
			return rs;
		}

		RefPtr<ShaderClosure> CreateShaderClosure(ErrorWriter * err, SymbolTable * symTable, ShaderSymbol * shader)
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
				if (replacements.TryGetValue(refComp->Content, targetComp))
				{
					auto oldComp = shaderClosure->AllComponents[refComp->Content]();
					auto newComp = shaderClosure->AllComponents[targetComp]();
					newComp->UserComponents.Add(currentComponent);
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
			EnumerableDictionary<String, String> & replacements;
			ReplaceReferenceVisitor(ShaderClosure * closure, ShaderComponentSymbol * comp, EnumerableDictionary<String, String> &pReplacements)
				: SyntaxVisitor(nullptr), shaderClosure(closure), currentComponent(comp), replacements(pReplacements)
			{}

			RefPtr<ExpressionSyntaxNode> VisitImportExpression(ImportExpressionSyntaxNode * import) override
			{
				currentImport = import;
				import->Component->Accept(this);
				if (import->Component->Tags.ContainsKey(L"ComponentReference"))
				{
					import->ComponentUniqueName = import->Component->Tags[L"ComponentReference"]().As<StringObject>()->Content;
				}
				currentImport = nullptr;
				for (auto & arg : import->Arguments)
					arg->Accept(this);
				return import;
			}

			RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode * var) override
			{
				RefPtr<Object> compRef;
				if (var->Tags.TryGetValue(L"ComponentReference", compRef))
				{
					ReplaceReference(compRef.As<StringObject>());
				}
				return var;
			}

			RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * member) override
			{
				member->BaseExpression->Accept(this);
				RefPtr<Object> compRef;
				if (member->Tags.TryGetValue(L"ComponentReference", compRef))
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
				rootShader->AllComponents.TryGetValue(referee->UniqueName, referee);
				referee->UserComponents.Add(currentComponent);
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

			ResolveDependencyVisitor(ErrorWriter * err, ShaderClosure * pRootShader, ShaderClosure * closure, ShaderComponentSymbol * comp)
				: SyntaxVisitor(err), shaderClosure(closure), rootShader(pRootShader), currentComponent(comp)
			{}

			RefPtr<ExpressionSyntaxNode> VisitImportExpression(ImportExpressionSyntaxNode * import) override
			{
				currentImport = import;
				import->Component->Accept(this);
				if (!import->Component->Tags.ContainsKey(L"ComponentReference"))
				{
					Error(32047, L"first argument of an import operator call does not resolve to a component.", import->Component.Ptr());
				}
				else
				{
					import->ComponentUniqueName = import->Component->Tags[L"ComponentReference"]().As<StringObject>()->Content;
				}
				currentImport = nullptr;
				for (auto & arg : import->Arguments)
					arg->Accept(this);
				return import;
			}

			RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode * var) override
			{
				VariableEntry varEntry;
				if (!var->Scope->FindVariable(var->Variable, varEntry))
				{
					if (var->Type->AsBasicType() && var->Type->AsBasicType()->Component)
					{
						if (auto comp = shaderClosure->FindComponent(var->Type->AsBasicType()->Component->Name))
						{
							if (comp->Implementations.First()->SyntaxNode->IsParam)
								shaderClosure->RefMap.TryGetValue(comp->Name, comp);
							var->Tags[L"ComponentReference"] = new StringObject(comp->UniqueName);
							AddReference(comp.Ptr(), currentImport, var->Position);
						}
						else
							throw InvalidProgramException(L"cannot resolve reference.");
					}
					if (auto comp = shaderClosure->FindComponent(var->Variable))
					{
						if (comp->Implementations.First()->SyntaxNode->IsParam)
							shaderClosure->RefMap.TryGetValue(var->Variable, comp);
						var->Tags[L"ComponentReference"] = new StringObject(comp->UniqueName);

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
						throw InvalidProgramException(L"cannot resolve reference.");
				}
				return var;
			}

			RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * member) override
			{
				member->BaseExpression->Accept(this);
				if (member->BaseExpression->Type->AsBasicType() && member->BaseExpression->Type->AsBasicType()->ShaderClosure)
				{
					if (auto comp = member->BaseExpression->Type->AsBasicType()->ShaderClosure->FindComponent(member->MemberName))
					{
						member->Tags[L"ComponentReference"] = new StringObject(comp->UniqueName);
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
						member->Tags[L"ComponentReference"] = new StringObject(comp->UniqueName);
						AddReference(comp.Ptr(), currentImport, member->Position);
					}
					else
						throw InvalidProgramException(L"cannot resolve reference.");
				}
				return member;
			}
		};

		void ResolveReference(ErrorWriter * err, ShaderClosure * rootShader, ShaderClosure* shader)
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

		void ReplaceReference(ShaderClosure * shader, EnumerableDictionary<String, String> & replacements)
		{
			for (auto & comp : shader->AllComponents)
			{
				ReplaceReferenceVisitor replaceVisitor(shader, comp.Value, replacements);
				for (auto & impl : comp.Value->Implementations)
				{
					replaceVisitor.currentImpl = impl.Ptr();
					impl->SyntaxNode->Accept(&replaceVisitor);
				}
			}
			for (auto & rep : replacements)
			{
				shader->AllComponents[rep.Key]()->UserComponents.Clear();
			}
		}

		String GetUniqueCodeName(String name)
		{
			StringBuilder sb;
			for (auto ch : name)
			{
				if (ch == L'.')
					sb << L"_";
				else
					sb << ch;
			}
			return EscapeDoubleUnderscore(sb.ProduceString());
		}

		bool IsInAbstractWorld(PipelineSymbol * pipeline, ShaderComponentSymbol* comp)
		{
			return comp->Implementations.First()->Worlds.Count() && !comp->Implementations.First()->SyntaxNode->IsParam &&
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
				if (comp.Value->IsParam())
					continue;
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
							err->Warning(34026, L"'" + existingComp->Name + L"': component is already defined when compiling shader '" + closure->Name + L"'. use 'require' to declare it as a parameter. \nsee previous declaration at " + existingComp->Implementations.First()->SyntaxNode->Position.ToString(),
								comp.Value->Implementations.First()->SyntaxNode->Position);
						}
					}
					else if (comp.Value->Implementations.First()->SyntaxNode->Parameters.Count() == 0)
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

		bool IsWorldFeasible(SymbolTable * symTable, PipelineSymbol * pipeline, ShaderComponentImplSymbol * impl, String world, ShaderComponentSymbol*& unaccessibleComp)
		{
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

		void SolveWorldConstraints(ErrorWriter * err, SymbolTable * symTable, ShaderClosure * shader)
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
						if (!IsWorldFeasible(symTable, shader->Pipeline, impl.Ptr(), w, unaccessibleComp))
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
			for (auto impOp : shader->Pipeline->SyntaxNode->ImportOperators)
			{
				for (auto comp : impOp->Usings)
				{
					useInWorld(comp, impOp->DestWorld.Content);
				}
			}
		}

		bool CheckCircularReference(ErrorWriter * err, ShaderClosure * shader)
		{
			bool rs = false;
			for (auto & comp : shader->AllComponents)
			{
				for (auto & impl : comp.Value->Implementations)
				{
					// check circular references
					HashSet<ShaderComponentSymbol*> set;
					List<ShaderComponentSymbol*> referredComponents;
					referredComponents.Add(comp.Value);
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
								if (rcomp.Key == comp.Value)
								{
									err->Error(32013, L"'" + rcomp.Key->Name + L"': circular reference is not allowed.", impl->SyntaxNode->Position);
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

		void VerifyAndPropagateArgumentConstraints(ErrorWriter * err, SymbolTable * symTable, ShaderClosure * shader)
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
							if (!symTable->IsWorldImplicitlyReachable(shader->Pipeline, arg->Type->FeasibleWorlds, w, requirement->Type->DataType))
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
				VerifyAndPropagateArgumentConstraints(err, symTable, subClosure.Value.Ptr());
		}

		void AddPipelineComponents(ShaderClosure * shader)
		{
			for (auto & comp : shader->Pipeline->Components)
			{
				if (!comp.Value->IsParam())
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
				if (comp.Value->Implementations.Count() == 1 &&
					comp.Value->Implementations.First()->SyntaxNode->Expression &&
					!comp.Value->Implementations.First()->SyntaxNode->IsOutput)
				{
					RefPtr<Object> compRef;
					if (comp.Value->Implementations.First()->SyntaxNode->Expression->Tags.TryGetValue(L"ComponentReference", compRef))
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

		void PropagatePipelineRequirements(ErrorWriter * err, ShaderClosure * shader)
		{
			for (auto & req : shader->Pipeline->Components)
			{
				if (req.Value->IsParam())
				{
					ShaderComponentSymbol * comp;
					StringBuilder errMsg;
					if (shader->AllComponents.TryGetValue(req.Key, comp))
					{
						if (!comp->Type->DataType->Equals(req.Value->Type->DataType.Ptr()))
						{
							errMsg << L"component '" << req.Key << L"' has type '" << comp->Type->DataType->ToString() << L"', but pipeline '"
								<< shader->Pipeline->SyntaxNode->Name.Content << L"' requires it to be '" << req.Value->Type->DataType->ToString() 
								<< L"'.\nsee pipeline requirement definition at " << req.Value->Implementations.First()->SyntaxNode->Position.ToString();
							err->Error(32051, errMsg.ProduceString(), comp->Implementations.First()->SyntaxNode->Position);
						}
					}
					else
					{
						errMsg << L"shader '" << shader->Name << L"' does not define '" << req.Key << L"' as required by pipeline '"
							<< shader->Pipeline->SyntaxNode->Name.Content << L"''.\nsee pipeline requirement definition at "
							<< req.Value->Implementations.First()->SyntaxNode->Position.ToString();
						err->Error(32052, errMsg.ProduceString(), shader->Position);
					}
				}
			}
		}

		void PrintModuleUsingStack(StringBuilder & sb, ShaderClosure * shader)
		{
			if (shader->Parent)
			{
				sb << L"see module '" + shader->Name << L"' being used in '" + shader->Parent->Name << L"' at " << shader->Position.ToString() << L"\n";
				PrintModuleUsingStack(sb, shader->Parent);
			}
			else
			{
				sb << L"shader '" << shader->Name << L"' is targeting pipeline '" << shader->Pipeline->SyntaxNode->Name.Content << L"' at " << shader->Position.ToString() << L"\nalso see pipeline definition at " << shader->Pipeline->SyntaxNode->Position.ToString();
			}
		}
	
		void CheckPipelineShaderConsistency(ErrorWriter * err, ShaderClosure * shader)
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
								StringBuilder sb;
								sb << L"\'" << world.World.Content << L"' is not a defined world in '" <<
									shader->Pipeline->SyntaxNode->Name.Content << L"'.\n";
								PrintModuleUsingStack(sb, shader);
								if (!shader->Pipeline->WorldDependency.ContainsKey(world.World.Content))
									err->Error(33012, sb.ProduceString(), world.World.Position);
							}
							WorldSymbol worldSym;
							if (shader->Pipeline->Worlds.TryGetValue(world.World.Content, worldSym))
							{
								if (worldSym.IsAbstract)
								{
									inAbstractWorld = true;
									if (userSpecifiedWorlds.Count() > 1)
									{
										StringBuilder sb;
										sb << L"abstract world cannot appear with other worlds.\n";
										PrintModuleUsingStack(sb, shader);
										err->Error(33013, sb.ProduceString(),
											world.World.Position);
										PrintModuleUsingStack(sb, shader);
									}
								}
							}
						}
					}
					if (!inAbstractWorld && !impl->SyntaxNode->IsParam && !impl->SyntaxNode->IsInput
						&& !impl->SyntaxNode->Expression && !impl->SyntaxNode->BlockStatement)
					{
						err->Error(33014, L"non-abstract component must have an implementation.",
							impl->SyntaxNode->Position);
					}

					bool isDefinedInAbstractWorld = false, isDefinedInNonAbstractWorld = false;
					if (impl->SyntaxNode->Rate)
					{
						for (auto & w : impl->SyntaxNode->Rate->Worlds)
						{
							auto world = shader->Pipeline->Worlds.TryGetValue(w.World.Content);
							if (world)
							{
								if (world->IsAbstract)
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
							err->Error(33039, L"'" + impl->SyntaxNode->Name.Content + L"': no code allowed for component defined in input world.", impl->SyntaxNode->Position);
					}
				}
			}
			for (auto & subShader : shader->SubClosures)
				CheckPipelineShaderConsistency(err, subShader.Value.Ptr());
		}

		void FlattenShaderClosure(ErrorWriter * err, SymbolTable * symTable, ShaderClosure * shader)
		{
			// add input(extern) components from pipeline
			AddPipelineComponents(shader);
			CheckPipelineShaderConsistency(err, shader);
			// assign choice names
			AssignUniqueNames(shader, L"", L"");
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
				if (!requirement.Value->IsParam())
					continue;
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
							if (!symTable->IsWorldImplicitlyReachable(shader->Pipeline, comp->Type->FeasibleWorlds, w, requirement.Value->Type->DataType))
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
			VerifyAndPropagateArgumentConstraints(err, symTable, shader);
		}
	}
}