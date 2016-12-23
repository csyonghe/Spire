#include "VariantIR.h"
#include "Closure.h"
#include "StringObject.h"
#include "GetDependencyVisitor.h"

namespace Spire
{
	namespace Compiler
	{
		void ShaderIR::EliminateDeadCode()
		{
			// mark entry points
			auto MarkUsing = [&](String compName, String userWorld)
			{
				if (auto defs = DefinitionsByComponent.TryGetValue(compName))
				{
					if (auto def = defs->TryGetValue(userWorld))
						(*def)->IsEntryPoint = true;
					else
					{
						for (auto & world : Shader->Pipeline->WorldDependency[userWorld]())
						{
							if (auto def2 = defs->TryGetValue(world))
							{
								(*def2)->IsEntryPoint = true;
								break;
							}
						}
					}
				}
			};
			for (auto & impOp : Shader->Pipeline->SyntaxNode->GetImportOperators())
				for (auto & ref : impOp->Usings)
					MarkUsing(ref, impOp->DestWorld.Content);
			for (auto & req : Shader->Pipeline->Components)
				if (req.Value->IsRequire())
				{
					for (auto & impl : req.Value->Implementations)
					{
						if (impl->Worlds.Count())
						{
							for (auto & world : impl->Worlds)
								MarkUsing(req.Key, world);
						}
						else
						{
							for (auto & world : Shader->Pipeline->Worlds)
								MarkUsing(req.Key, world.Key);
						}
					}
				}
			for (auto & def : Definitions)
				if (def->SyntaxNode->HasSimpleAttribute("FragDepth"))
					def->IsEntryPoint = true;
			List<ComponentDefinitionIR*> workList;
			HashSet<ComponentDefinitionIR*> referencedDefs;
			for (auto & def : Definitions)
			{
				if (def->IsEntryPoint)
				{
					if (referencedDefs.Add(def.Ptr()))
						workList.Add(def.Ptr());
				}
			}
			for (int i = 0; i < workList.Count(); i++)
			{
				auto def = workList[i];
				for (auto & dep : def->Dependency)
				{
					if (referencedDefs.Add(dep))
						workList.Add(dep);
				}
			}
			List<RefPtr<ComponentDefinitionIR>> newDefinitions;
			for (auto & def : Definitions)
			{
				if (referencedDefs.Contains(def.Ptr()))
				{
					newDefinitions.Add(def);
					EnumerableHashSet<ComponentDefinitionIR*> newSet;
					for (auto & comp : def->Users)
						if (referencedDefs.Contains(comp))
						{
							newSet.Add(comp);
						}
					def->Users = newSet;
					newSet.Clear();
					for (auto & comp : def->Dependency)
						if (referencedDefs.Contains(comp))
						{
							newSet.Add(comp);
						}
					def->Dependency = newSet;
				}
			}
			Definitions = _Move(newDefinitions);
			for (auto & kv : DefinitionsByComponent)
			{
				for (auto & def : kv.Value)
					if (!referencedDefs.Contains(def.Value))
						kv.Value.Remove(def.Key);
			}
		}

		class ReferenceWorkItem
		{
		public:
			ComponentDependency Dependency;
			String SourceWorld;
			int GetHashCode()
			{
				return Dependency.GetHashCode();
			}
			bool operator == (const ReferenceWorkItem & other)
			{
				return Dependency == other.Dependency && SourceWorld == other.SourceWorld;
			}
		};

		void ShaderIR::ResolveComponentReference()
		{
			// build bidirectional dependency map of component definitions
			for (auto & comp : Definitions)
			{
				comp->Dependency.Clear();
				comp->Users.Clear();
			}
			for (auto & comp : Definitions)
			{
				List<ReferenceWorkItem> workList;
				for (auto & dep : GetDependentComponents(comp->SyntaxNode.Ptr()))
				{
					ReferenceWorkItem item;
					item.Dependency = dep;
					item.SourceWorld = dep.ImportOperator ? dep.ImportOperator->SourceWorld.Content : comp->World;
					workList.Add(item);
				}
				HashSet<ReferenceWorkItem> proceseedDefCompss;
				for (int i = 0; i < workList.Count(); i++)
				{
					auto dep = workList[i];
					if (!proceseedDefCompss.Add(dep))
						continue;
					auto & depDefs = DefinitionsByComponent[dep.Dependency.ReferencedComponent]();
					// select the best overload according to import operator ordering,
					// prefer user-pinned definitions (as provided in the choice file)
					List<String> depWorlds;
					depWorlds.Add(dep.SourceWorld);
					for (auto & w : Shader->Pipeline->WorldDependency[dep.SourceWorld]())
						depWorlds.Add(w);
					depWorlds.Add("<uniform>");
					for (int pass = 0; pass < 2; pass++)
					{
						// in the first pass, examine the pinned definitions only
						// in the second pass, examine all the rest definitions
						for (auto & depWorld : depWorlds)
						{
							auto refComp = Shader->AllComponents[dep.Dependency.ReferencedComponent]();
							bool isPinned = refComp.Symbol->Type->PinnedWorlds.Contains(depWorld);
							if ((pass == 0 && !isPinned) || (pass == 1 && isPinned)) continue;
							ComponentDefinitionIR * depDef;
							if (depDefs.TryGetValue(depWorld, depDef))
							{
								comp->Dependency.Add(depDef);
								depDef->Users.Add(comp.Ptr());
								// add additional dependencies due to import operators
								auto processImportOperatorUsings = [&](ImportOperatorDefSyntaxNode * importOp)
								{
									for (auto & importUsing : importOp->Usings)
									{
										ComponentInstance refComp;
										if (!Shader->AllComponents.TryGetValue(importUsing, refComp))
											throw InvalidProgramException("import operator dependency not exists.");
										ReferenceWorkItem workItem;
										workItem.Dependency = ComponentDependency(refComp.Symbol->UniqueName, nullptr);
										workItem.SourceWorld = importOp->SourceWorld.Content;
										workList.Add(workItem);
									}
								};
								if (dep.Dependency.ImportOperator)
								{
									processImportOperatorUsings(dep.Dependency.ImportOperator);
								}
								if (depWorld != dep.SourceWorld && depWorld != "<uniform>")
								{
									auto importPath = SymbolTable->FindImplicitImportOperatorChain(Shader->Pipeline, depWorld, dep.SourceWorld, refComp.Symbol->Type->DataType);
									if (importPath.Count() == 0)
										continue;
									processImportOperatorUsings(importPath.First().Nodes.Last().ImportOperator);
								}
								goto selectionEnd; // first preferred overload is found, terminate searching
							}
						}
					}
				selectionEnd:;
				}
			}
		}
		List<String> ShaderIR::GetComponentDependencyOrder()
		{
			List<String> result, workList;
			HashSet<String> set;
			for (auto & comp : DefinitionsByComponent)
			{
				bool emptyDependency = true;
				for (auto & def : comp.Value)
					if (def.Value->Dependency.Count())
					{
						emptyDependency = false;
						break;
					}
				if (emptyDependency)
				{
					workList.Add(comp.Key);
				}
			}
			for (int i = 0; i < workList.Count(); i++)
			{
				auto comp = workList[i];
				if (!set.Contains(comp))
				{
					bool insertable = true;
					for (auto & def : DefinitionsByComponent[comp]())
					{
						for (auto & dep : def.Value->Dependency)
							if (!set.Contains(dep->UniqueName))
							{
								insertable = false;
								goto breakLoc;
							}
					}
				breakLoc:;
					if (insertable)
					{
						if (set.Add(comp))
						{
							result.Add(comp);
							for (auto & def : DefinitionsByComponent[comp]())
								for (auto & user : def.Value->Users)
									workList.Add(user->UniqueName);
						}
					}
				}
			}
			return result;
		}
		EnumerableHashSet<ComponentDefinitionIR*>& ComponentDefinitionIR::GetComponentFunctionDependencyClosure()
		{
			if (dependencyClosure.Count() || Dependency.Count() == 0)
				return dependencyClosure;
			for (auto & dep : Dependency)
			{
				dependencyClosure.Add(dep);
				if (dep->SyntaxNode->IsComponentFunction())
				{
					for (auto & x : dep->GetComponentFunctionDependencyClosure())
						dependencyClosure.Add(x);
				}
			}
			return dependencyClosure;
		}
}
}