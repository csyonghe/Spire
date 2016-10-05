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

		bool PipelineSymbol::IsWorldImplicitlyReachable(String src, String targetWorld)
		{
			if (src == targetWorld)
				return true;
			if (ImplicitlyReachableWorlds.ContainsKey(src))
				if (ImplicitlyReachableWorlds[src]().Contains(targetWorld))
					return true;
			return false;
		}

		bool PipelineSymbol::IsWorldImplicitlyReachable(EnumerableHashSet<String>& src, String targetWorld)
		{
			for (auto srcW : src)
			{
				if (srcW == targetWorld)
					return true;
				if (ImplicitlyReachableWorlds.ContainsKey(srcW))
					if (ImplicitlyReachableWorlds[srcW]().Contains(targetWorld))
						return true;
			}
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
		
		List<ImportPath> PipelineSymbol::FindImplicitImportOperatorChain(String worldSrc, String worldDest)
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
						if (op->SourceWorld.Content == world0 && op->Parameters.Count() == 0)
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
								if (allSymbols.Contains(dep.Key) && !addedSymbols.Contains(dep.Key))
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
			if (Pipeline->Components.TryGetValue(compName, refComp))
			{
				if (!refComp->IsParam())
				{
					result.Component = refComp.Ptr();
					return result;
				}
			}
			result.IsAccessible = false;
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
				if (!impl->SyntaxNode->Type->Equals(cimpl->SyntaxNode->Type.Ptr()))
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
			ShaderClosure * root = this;
			while (root->Parent != nullptr)
				root = root->Parent;
			if (root != this)
			{
				// find global components in root (pipeline-defined components)
				if (root->Components.TryGetValue(name, rs))
					return rs;
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
								if (allSymbols.Contains(dep.Key) && !addedSymbols.Contains(dep.Key))
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
