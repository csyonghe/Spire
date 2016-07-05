#include "SymbolTable.h"
#include "ShaderCompiler.h"

namespace Spire
{
	namespace Compiler
	{
		bool SymbolTable::SortShaders()
		{
			HashSet<ShaderSymbol*> shaderSet;
			ShaderDependenceOrder.Clear();
			List<ShaderSymbol *> nextShaders, currentShaders;
			// sort shaders in dependency order
			for (auto & shader : Shaders)
			{
				if (shader.Value->DependentShaders.Count() == 0)
				{
					ShaderDependenceOrder.Add(shader.Value.Ptr());
					shaderSet.Add(shader.Value.Ptr());
				}
				else
					currentShaders.Add(shader.Value.Ptr());
			}
			while (currentShaders.Count())
			{
				nextShaders.Clear();
				for (auto & shader : currentShaders)
				{
					bool pass = true;
					for (auto & dshader : shader->DependentShaders)
						if (!shaderSet.Contains(dshader))
						{
							pass = false;
							break;
						}
					if (pass)
					{
						ShaderDependenceOrder.Add(shader);
						shaderSet.Add(shader);
					}
					else
						nextShaders.Add(shader);
				}
				currentShaders.SwapWith(nextShaders);
			}
			return (ShaderDependenceOrder.Count() == Shaders.Count());
		}
		void SymbolTable::EvalFunctionReferenceClosure()
		{
			for (auto & func : Functions)
			{
				List<String> funcList;
				EnumerableHashSet<String> funcSet;
				for (auto & ref : func.Value->ReferencedFunctions)
				{
					funcList.Add(ref);
					funcSet.Add(ref);
				}
				for (int i = 0; i < funcList.Count(); i++)
				{
					RefPtr<FunctionSymbol> funcSym;
					if (Functions.TryGetValue(funcList[i], funcSym))
					{
						for (auto rfunc : funcSym->ReferencedFunctions)
						{
							if (funcSet.Add(rfunc))
								funcList.Add(rfunc);
						}
					}
				}
				func.Value->ReferencedFunctions = _Move(funcSet);
			}
		}

		bool PipelineSymbol::IsAbstractWorld(String world)
		{
			WorldSymbol ws;
			if (Worlds.TryGetValue(world, ws))
				return ws.IsAbstract;
			return false;
		}

		bool PipelineSymbol::IsWorldReachable(String src, String targetWorld)
		{
			if (src == targetWorld)
				return true;
			if (ReachableWorlds.ContainsKey(src))
				if (ReachableWorlds[src]().Contains(targetWorld))
					return true;
			return false;
		}

		bool PipelineSymbol::IsWorldDirectlyReachable(String src, String targetWorld)
		{
			if (src == targetWorld)
				return true;
			for (auto & op : SyntaxNode->ImportOperators)
				if (op->SourceWorld.Content == src && op->DestWorld.Content == targetWorld)
					return true;
			return false;
		}

		List<String>& PipelineSymbol::GetWorldTopologyOrder()
		{
			if (WorldTopologyOrder.Count() != 0)
				return WorldTopologyOrder;
			List<String> rs;
			HashSet<String> rsSet;
			bool changed = true;
			while (changed)
			{
				changed = false;
				for (auto & w : WorldDependency)
				{
					if (!rsSet.Contains(w.Key))
					{
						bool canAdd = true;
						for (auto & dw : w.Value)
							if (!rsSet.Contains(dw))
							{
								canAdd = false;
								break;
							}
						if (canAdd)
						{
							rsSet.Add(w.Key);
							rs.Add(w.Key);
							changed = true;
						}
					}
				}
			}
			WorldTopologyOrder = _Move(rs);
			return WorldTopologyOrder;
		}
		
		bool PipelineSymbol::IsWorldReachable(EnumerableHashSet<String>& src, String targetWorld)
		{
			for (auto srcW : src)
			{
				if (srcW == targetWorld)
					return true;
				if (ReachableWorlds.ContainsKey(srcW))
					if (ReachableWorlds[srcW]().Contains(targetWorld))
						return true;
			}
			return false;
		}
		
		List<ImportPath> PipelineSymbol::FindImportOperatorChain(String worldSrc, String worldDest)
		{
			List<ImportPath> resultPathes;
			if (worldSrc == worldDest)
				return resultPathes;
			List<ImportPath> pathes, pathes2;
			pathes.Add(ImportPath());
			pathes[0].Nodes.Add(ImportPath::Node(worldSrc, nullptr));
			while (pathes.Count())
			{
				pathes2.Clear();
				for (auto & p : pathes)
				{
					String world0 = p.Nodes.Last().TargetWorld;
					for (auto op : SyntaxNode->ImportOperators)
					{
						if (op->SourceWorld.Content == world0)
						{
							ImportPath np = p;
							np.Nodes.Add(ImportPath::Node(op->DestWorld.Content, op.Ptr()));
							if (op->DestWorld.Content == worldDest)
								resultPathes.Add(np);
							else
								pathes2.Add(np);
						}
					}
				}
				pathes.SwapWith(pathes2);
			}
			return resultPathes;
		}
		List<ImportOperatorDefSyntaxNode*> PipelineSymbol::GetImportOperatorsFromSourceWorld(String worldSrc)
		{
			List<ImportOperatorDefSyntaxNode*> rs;
			for (auto & op : this->SyntaxNode->ImportOperators)
			{
				if (op->SourceWorld.Content == worldSrc)
					rs.Add(op.Ptr());
			}
			return rs;
		}
		List<ShaderComponentSymbol*> ShaderSymbol::GetComponentDependencyOrder()
		{
			List<ShaderComponentSymbol*> components;

			for (auto & comp : Components)
			{
				components.Add(comp.Value.Ptr());
			}
			SortComponents(components);
			return components;
		}
		void ShaderSymbol::SortComponents(List<ShaderComponentSymbol*>& comps)
		{
			comps.Sort([&](ShaderComponentSymbol*c0, ShaderComponentSymbol*c1)
			{
				return c0->Implementations.First()->SyntaxNode->Position < c1->Implementations.First()->SyntaxNode->Position;
			});
			HashSet<ShaderComponentSymbol*> allSymbols, addedSymbols;
			for (auto & comp : comps)
				allSymbols.Add(comp);
			List<ShaderComponentSymbol*> sorted;
			bool changed = true;
			while (changed)
			{
				changed = false;
				for (auto & comp : comps)
				{
					if (!addedSymbols.Contains(comp))
					{
						bool isFirst = true;
						for (auto & impl : comp->Implementations)
							for (auto & dep : impl->DependentComponents)
								if (allSymbols.Contains(dep) && !addedSymbols.Contains(dep))
								{
									isFirst = false;
									goto loopEnd;
								}
					loopEnd:;
						if (isFirst)
						{
							addedSymbols.Add(comp);
							sorted.Add(comp);
							changed = true;
						}
					}
				}
			}
			comps = _Move(sorted);
		}

		ShaderSymbol::ComponentReference ShaderSymbol::ResolveComponentReference(String compName, bool topLevel)
		{
			ComponentReference result;
			result.IsAccessible = true;
			RefPtr<ShaderComponentSymbol> refComp, privateRefComp;
			if (Components.TryGetValue(compName, refComp))
			{
				result.Component = refComp.Ptr();
				return result;
			}
			for (auto & shaderUsing : ShaderUsings)
			{
				if (shaderUsing.Shader->Components.TryGetValue(compName, refComp))
				{
					if (refComp->Implementations.First()->SyntaxNode->IsPublic)
					{
						result.Component = refComp.Ptr();
						result.IsAccessible = true;
						return result;
					}
					else
					{
						result.Component = refComp.Ptr();
						result.IsAccessible = false;
					}
				}
				else if (shaderUsing.IsPublic || topLevel)
				{
					auto rresult = shaderUsing.Shader->ResolveComponentReference(compName, false);
					if (rresult.IsAccessible)
						return rresult;
					else
						result = rresult;
				}
			}
			result.IsAccessible = false;
			return result;
		}

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
			for (auto & impOp : Shader->Pipeline->SyntaxNode->ImportOperators)
				for (auto & ref : impOp->Usings)
					MarkUsing(ref.Content, impOp->DestWorld.Content);
			for (auto & w : Shader->Pipeline->SyntaxNode->Worlds)
				for (auto & ref : w->Usings)
					MarkUsing(ref.Content, w->Name.Content);
			for (auto & comp : Definitions)
				if (comp->Implementation->ExportWorlds.Contains(comp->World) ||
					(Shader->Pipeline->IsAbstractWorld(comp->World) &&
					(comp->Implementation->SyntaxNode->LayoutAttributes.ContainsKey(L"Pinned") || Shader->Pipeline->Worlds[comp->World]().SyntaxNode->LayoutAttributes.ContainsKey(L"Pinned"))))
				{
					comp->IsEntryPoint = true;
				}

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
				List<ShaderComponentSymbol *> workList;
				for (auto & dep : comp->Implementation->DependentComponents)
					workList.Add(dep);
				HashSet<ShaderComponentSymbol*> proceseedDefCompss;
				for (int i = 0; i < workList.Count(); i++)
				{
					auto dep = workList[i];
					if (!proceseedDefCompss.Add(dep))
						continue;
					auto & depDefs = DefinitionsByComponent[dep->UniqueName]();
					// select the best overload according to import operator ordering,
					// prefer user-pinned definitions (as provided in the choice file)
					List<String> depWorlds;
					depWorlds.Add(comp->World);
					for (auto & w : Shader->Pipeline->WorldDependency[comp->World]())
						depWorlds.Add(w);
					for (int pass = 0; pass < 2; pass++)
					{
						// in the first pass, examine the pinned definitions only
						// in the second pass, examine all the rest definitions
						for (auto & depWorld : depWorlds)
						{
							bool isPinned = false;
							for (auto existingDef : dep->Type->PinnedWorlds)
								if (existingDef.StartsWith(depWorld))
								{
									isPinned = true;
									break;
								}
							if ((pass == 0 && !isPinned) || (pass == 1 && isPinned)) continue;
							ComponentDefinitionIR * depDef;
							if (depDefs.TryGetValue(depWorld, depDef))
							{
								comp->Dependency.Add(depDef);
								depDef->Users.Add(comp.Ptr());
								// add additional dependencies due to import operators
								if (depWorld != comp->World)
								{
									auto importPath = Shader->Pipeline->FindImportOperatorChain(depWorld, comp->World);
									if (importPath.Count() == 0)
										throw InvalidProgramException(L"no import path found.");
									auto & usings = importPath.First().Nodes.Last().ImportOperator->Usings;
									for (auto & importUsing : usings)
									{
										ShaderComponentSymbol* refComp;
										if (!Shader->AllComponents.TryGetValue(importUsing.Content, refComp))
											throw InvalidProgramException(L"import operator dependency not exists.");
										workList.Add(refComp);
									}
								}
								goto selectionEnd; // first preferred overload is found, terminate searching
							}
						}
					}
					selectionEnd:;
				}
			}
		}
		List<ShaderComponentSymbol*> ShaderIR::GetComponentDependencyOrder()
		{
			List<ShaderComponentSymbol*> result, workList;
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
					workList.Add(Shader->AllComponents[comp.Key]());
				}
			}
			for (int i = 0; i < workList.Count(); i++)
			{
				auto comp = workList[i];
				if (!set.Contains(comp->UniqueName))
				{
					bool insertable = true;
					for (auto & def : DefinitionsByComponent[comp->UniqueName]())
					{
						for (auto & dep : def.Value->Dependency)
							if (!set.Contains(dep->Component->UniqueName))
							{
								insertable = false;
								goto breakLoc;
							}
					}
				breakLoc:;
					if (insertable)
					{
						if (set.Add(comp->UniqueName))
						{
							result.Add(comp);
							for (auto & def : DefinitionsByComponent[comp->UniqueName]())
								for (auto & user : def.Value->Users)
									workList.Add(user->Component);
						}
					}
				}
			}
			return result;
		}
		bool SymbolTable::CheckComponentImplementationConsistency(ErrorWriter * err, ShaderComponentSymbol * comp, ShaderComponentImplSymbol * impl)
		{
			bool rs = true;
			if (impl->SyntaxNode->Rate)
			{
				for (auto & cimpl : comp->Implementations)
				{
					for (auto & w : cimpl->Worlds)
						if (impl->Worlds.Contains(w) && impl->AlternateName == cimpl->AlternateName)
						{
							err->Error(33020, L"\'" + comp->Name + L"\' is already defined at '" + w + L"\'.", impl->SyntaxNode->Position);
							rs = false;
						}
				}
			}
			else
			{
				for (auto & cimpl : comp->Implementations)
				{
					if (cimpl->Worlds.Count() == 0 && impl->Worlds.Count() == 0 && impl->AlternateName == cimpl->AlternateName)
					{
						err->Error(33020, L"\'" + comp->Name + L"\' is already defined.", impl->SyntaxNode->Position);
						rs = false;
					}
				}
			}
			for (auto & cimpl : comp->Implementations)
			{
				if (impl->SyntaxNode->IsOutput != cimpl->SyntaxNode->IsOutput)
				{
					err->Error(33021, L"\'" + comp->Name + L"\': inconsistent signature.\nsee previous definition at " + cimpl->SyntaxNode->Position.ToString(), impl->SyntaxNode->Position);
					rs = false;
					break;
				}
				if (impl->SyntaxNode->IsParam != cimpl->SyntaxNode->IsParam)
				{
					err->Error(33021, L"\'" + comp->Name + L"\': inconsistent signature.\nsee previous definition at " + cimpl->SyntaxNode->Position.ToString(), impl->SyntaxNode->Position);
					rs = false;
					break;
				}
				if (impl->SyntaxNode->IsPublic != cimpl->SyntaxNode->IsPublic)
				{
					err->Error(33021, L"\'" + comp->Name + L"\': inconsistent signature.\nsee previous definition at " + cimpl->SyntaxNode->Position.ToString(), impl->SyntaxNode->Position);
					rs = false;
					break;
				}
				if (impl->SyntaxNode->Type->ToExpressionType(this) != cimpl->SyntaxNode->Type->ToExpressionType(this))
				{
					err->Error(33021, L"\'" + comp->Name + L"\': inconsistent signature.\nsee previous definition at " + cimpl->SyntaxNode->Position.ToString(), impl->SyntaxNode->Position);
					rs = false;
					break;
				}
			}
			if (impl->SyntaxNode->IsParam && comp->Implementations.Count() != 0)
			{
				err->Error(33022, L"\'" + comp->Name + L"\': parameter name conflicts with existing definition.", impl->SyntaxNode->Position);
				rs = false;
			}
			return rs;
		}

		int GUID::currentGUID = 0;
		void GUID::Clear()
		{
			currentGUID = 0;
		}
		int GUID::Next()
		{
			return currentGUID++;
		}
		RefPtr<ShaderComponentSymbol> ShaderClosure::FindComponent(String name, bool findInPrivate)
		{
			RefPtr<ShaderComponentSymbol> rs;
			if (RefMap.TryGetValue(name, rs))
				return rs;
			if (Components.TryGetValue(name, rs))
				return rs;
			for (auto & subClosure : SubClosures)
			{
				if (subClosure.Value->IsInPlace)
				{
					rs = subClosure.Value->FindComponent(name);
					if (rs && (findInPrivate || rs->Implementations.First()->SyntaxNode->IsPublic))
						return rs;
					else
						rs = nullptr;
				}
			}
			return rs;
		}
		RefPtr<ShaderClosure> ShaderClosure::FindClosure(String name)
		{
			RefPtr<ShaderClosure> rs;
			if (SubClosures.TryGetValue(name, rs))
				return rs;
			for (auto & subClosure : SubClosures)
			{
				if (subClosure.Value->IsInPlace)
				{
					rs = subClosure.Value->FindClosure(name);
					if (rs && rs->IsPublic)
						return rs;
					else
						rs = nullptr;
				}
			}
			return rs;
		}
		List<ShaderComponentSymbol*> ShaderClosure::GetDependencyOrder()
		{
			List<ShaderComponentSymbol*> comps;
			for (auto & comp : AllComponents)
				comps.Add(comp.Value);
			comps.Sort([&](ShaderComponentSymbol*c0, ShaderComponentSymbol*c1)
			{
				return c0->Implementations.First()->SyntaxNode->Position < c1->Implementations.First()->SyntaxNode->Position;
			});
			HashSet<ShaderComponentSymbol*> allSymbols, addedSymbols;
			for (auto & comp : comps)
				allSymbols.Add(comp);
			List<ShaderComponentSymbol*> sorted;
			bool changed = true;
			while (changed)
			{
				changed = false;
				for (auto & comp : comps)
				{
					if (!addedSymbols.Contains(comp))
					{
						bool isFirst = true;
						for (auto & impl : comp->Implementations)
							for (auto & dep : impl->DependentComponents)
								if (allSymbols.Contains(dep) && !addedSymbols.Contains(dep))
								{
									isFirst = false;
									goto loopEnd;
								}
					loopEnd:;
						if (isFirst)
						{
							addedSymbols.Add(comp);
							sorted.Add(comp);
							changed = true;
						}
					}
				}
			}
			return sorted;
		}
	}
}
