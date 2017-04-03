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
				if (!func.Value->IsReferencedFunctionsTransitiveClosureEvaluated)
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
							if (funcSym->IsReferencedFunctionsTransitiveClosureEvaluated)
							{
								for (auto rfunc : funcSym->ReferencedFunctions)
									funcSet.Add(rfunc);
							}
							else
							{
								for (auto rfunc : funcSym->ReferencedFunctions)
								{
									if (funcSet.Add(rfunc))
										funcList.Add(rfunc);
								}
							}
						}
					}
					func.Value->ReferencedFunctions = _Move(funcSet);
					func.Value->IsReferencedFunctionsTransitiveClosureEvaluated = true;
				}
			}
		}

		List<ImportPath>& PipelineSymbol::GetPaths(String srcWorld, String destWorld)
		{
			if (auto first = pathCache.TryGetValue(srcWorld))
			{
				if (auto second = first->TryGetValue(destWorld))
					return *second;
			}
			else
			{
				pathCache[srcWorld] = EnumerableDictionary<String, List<ImportPath>>();
			}
			auto path = FindPaths(srcWorld, destWorld);
			auto & dict = pathCache[srcWorld]();
			dict[destWorld] = _Move(path);
			return dict[destWorld]();
		}

		List<ImportPath> PipelineSymbol::FindPaths(String worldSrc, String worldDest)
		{
			List<ImportPath> resultPaths;
			if (worldSrc == worldDest)
				return resultPaths;
			List<ImportPath> paths, paths2;
			paths.Add(ImportPath());
			paths[0].Nodes.Add(ImportPath::Node(worldSrc, nullptr));
			while (paths.Count())
			{
				paths2.Clear();
				for (auto & p : paths)
				{
					String world0 = p.Nodes.Last().TargetWorld;
					for (auto op : SyntaxNode->GetImportOperators())
					{
						if (op->SourceWorld.Content == world0)
						{
							ImportPath np = p;
							if (op->GetParameters().Count() != 0)
								np.IsImplicitPath = false;
							for (auto &req : op->Requirements)
								np.TypeRequirements.Add(req.Ptr());
							np.Nodes.Add(ImportPath::Node(op->DestWorld.Content, op.Ptr()));
							if (op->DestWorld.Content == worldDest)
								resultPaths.Add(np);
							else
								paths2.Add(np);
						}
					}
				}
				paths.SwapWith(paths2);
			}
			return resultPaths;
		}

		bool PipelineSymbol::IsAbstractWorld(String world)
		{
			WorldSyntaxNode* worldDecl;
			if (Worlds.TryGetValue(world, worldDecl))
				return worldDecl->IsAbstract();
			return false;
		}

		bool PipelineSymbol::IsChildOf(PipelineSymbol * parentPipeline)
		{
			if (this == parentPipeline)
				return true;
			else if (ParentPipeline)
				return ParentPipeline->IsChildOf(parentPipeline);
			else
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
		
		List<ImportOperatorDefSyntaxNode*> PipelineSymbol::GetImportOperatorsFromSourceWorld(String worldSrc)
		{
			List<ImportOperatorDefSyntaxNode*> rs;
			auto dict = ImportOperatorsByPath.TryGetValue(worldSrc);
			if (dict)
			{
				for (auto & op : *dict)
				{
					for (auto & x : op.Value)
						rs.Add(x.Ptr());
				}
			}
			return rs;
		}
		void PipelineSymbol::AddImportOperator(RefPtr<ImportOperatorDefSyntaxNode> op)
		{
			auto list = ImportOperators.TryGetValue(op->Name.Content);
			if (!list)
			{
				ImportOperators[op->Name.Content] = List<RefPtr<ImportOperatorDefSyntaxNode>>();
				list = ImportOperators.TryGetValue(op->Name.Content);
			}
			list->Add(op);

			auto first = ImportOperatorsByPath.TryGetValue(op->SourceWorld.Content);
			if (!first)
			{
				ImportOperatorsByPath[op->SourceWorld.Content] = EnumerableDictionary<String, List<RefPtr<ImportOperatorDefSyntaxNode>>>();
				first = ImportOperatorsByPath.TryGetValue(op->SourceWorld.Content);
			}

			auto second = first->TryGetValue(op->DestWorld.Content);
			if (!second)
			{
				(*first)[op->DestWorld.Content] = List<RefPtr<ImportOperatorDefSyntaxNode>>();
				second = first->TryGetValue(op->DestWorld.Content);
			}
			second->Add(op);
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
					if (refComp->Implementations.First()->SyntaxNode->IsPublic())
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
			if (ParentPipeline && ParentPipeline->Components.TryGetValue(compName, refComp))
			{
				if (!refComp->IsRequire())
				{
					result.Component = refComp.Ptr();

                    // HACK(tfoley): I'm not sure if this is a valid bug fix or not.
                    result.IsAccessible = true;

					return result;
				}
			}
			result.IsAccessible = false;
			return result;
		}

		bool SymbolTable::CheckComponentImplementationConsistency(DiagnosticSink * err, ShaderComponentSymbol * comp, ShaderComponentImplSymbol * impl)
		{
			bool rs = true;
			if (impl->SyntaxNode->Rate)
			{
				for (auto & cimpl : comp->Implementations)
				{
					for (auto & w : cimpl->Worlds)
						if (impl->Worlds.Contains(w))
						{
                            err->diagnose(impl->SyntaxNode->Position, Diagnostics::componentIsAlreadyDefinedInThatWorld, comp->Name, w);
							rs = false;
						}
				}
			}
			else
			{
				for (auto & cimpl : comp->Implementations)
				{
					if (cimpl->Worlds.Count() == 0 && impl->Worlds.Count() == 0)
					{
                        err->diagnose(impl->SyntaxNode->Position, Diagnostics::componentIsAlreadyDefined, comp->Name);
						rs = false;
					}
				}
			}
			for (auto & cimpl : comp->Implementations)
			{
				if (impl->SyntaxNode->IsOutput() != cimpl->SyntaxNode->IsOutput())
				{
                    err->diagnose(impl->SyntaxNode->Position,
                        Diagnostics::inconsistentSignatureForComponent,
                        comp->Name);
                    err->diagnose(cimpl->SyntaxNode->Position,
                        Diagnostics::seePreviousDefinition);
					rs = false;
					break;
				}
				if (impl->SyntaxNode->IsRequire() != cimpl->SyntaxNode->IsRequire())
				{
                    err->diagnose(impl->SyntaxNode->Position,
                        Diagnostics::inconsistentSignatureForComponent,
                        comp->Name);
                    err->diagnose(cimpl->SyntaxNode->Position,
                        Diagnostics::seePreviousDefinition);
					rs = false;
					break;
				}
				if (impl->SyntaxNode->IsPublic() != cimpl->SyntaxNode->IsPublic())
				{
                    err->diagnose(impl->SyntaxNode->Position,
                        Diagnostics::inconsistentSignatureForComponent,
                        comp->Name);
                    err->diagnose(cimpl->SyntaxNode->Position,
                        Diagnostics::seePreviousDefinition);
					rs = false;
					break;
				}
				if (!impl->SyntaxNode->Type->Equals(cimpl->SyntaxNode->Type.Ptr()))
				{
                    err->diagnose(impl->SyntaxNode->Position,
                        Diagnostics::inconsistentSignatureForComponent,
                        comp->Name);
                    err->diagnose(cimpl->SyntaxNode->Position,
                        Diagnostics::seePreviousDefinition);
					rs = false;
					break;
				}
			}
			if (impl->SyntaxNode->IsRequire() && comp->Implementations.Count() != 0)
			{
                err->diagnose(impl->SyntaxNode->Position,
                    Diagnostics::parameterNameConflictsWithExistingDefinition, comp->Name);
				rs = false;
			}
			return rs;
		}

		String PrintType(RefPtr<ExpressionType> type, String recordReplaceStr)
		{
			if (auto basic = type->AsBasicType())
			{
				if (basic->BaseType == BaseType::Generic)
					return recordReplaceStr;
				else
					return basic->ToString();
			}
			else if (auto arr = type.As<ArrayExpressionType>())
			{
				if (arr->ArrayLength > 0)
					return PrintType(arr->BaseType, recordReplaceStr) + "[" + arr->ArrayLength + "]";
				else
					return PrintType(arr->BaseType, recordReplaceStr) + "[]";
			}
			else if (auto gen = type.As<GenericExpressionType>())
			{
				return gen->GenericTypeName + "<" + PrintType(gen->BaseType, recordReplaceStr) + ">";
			}
			return "";
		}

		bool SymbolTable::CheckTypeRequirement(const ImportPath & p, RefPtr<ExpressionType> type)
		{
			for (auto & req : p.TypeRequirements)
			{
				auto typeStr = type->ToString();
				auto retType = PrintType(req->ReturnType, typeStr);
				StringBuilder sbInternalName;
				sbInternalName << req->Name.Content;
				for (auto & op : req->GetParameters())
				{
					sbInternalName << "@" << PrintType(op->Type, typeStr);
				}
				auto funcName = sbInternalName.ProduceString();
				auto func = Functions.TryGetValue(funcName);
				if (!func)
					return false;
				if ((*func)->SyntaxNode->ReturnType->ToString() != retType)
					return false;
			}
			return true;
		}

		bool SymbolTable::IsWorldReachable(PipelineSymbol * pipe, String src, String targetWorld, RefPtr<ExpressionType> type)
		{
			if (src == targetWorld)
				return true;
			return From(pipe->GetPaths(src, targetWorld)).Any([&](const ImportPath & p)
			{
				return CheckTypeRequirement(p, type);
			});
		}

		bool SymbolTable::IsWorldImplicitlyReachable(PipelineSymbol * pipe, String src, String targetWorld, RefPtr<ExpressionType> type)
		{
			if (src == targetWorld)
				return true;
			return From(pipe->GetPaths(src, targetWorld)).Any([&](const ImportPath & p)
			{
				return p.IsImplicitPath && CheckTypeRequirement(p, type);
			});
		}

		bool SymbolTable::IsWorldImplicitlyReachable(PipelineSymbol * pipe, EnumerableHashSet<String>& src, String targetWorld, RefPtr<ExpressionType> type)
		{
			for (auto srcW : src)
			{
				if (IsWorldImplicitlyReachable(pipe, srcW, targetWorld, type))
					return true;
			}
			return false;
		}

		bool SymbolTable::IsWorldReachable(PipelineSymbol * pipe, EnumerableHashSet<String>& src, String targetWorld, RefPtr<ExpressionType> type)
		{
			for (auto srcW : src)
			{
				if (IsWorldReachable(pipe, srcW, targetWorld, type))
					return true;
			}
			return false;
		}

		List<ImportPath> SymbolTable::FindImplicitImportOperatorChain(PipelineSymbol * pipe, String worldSrc, String worldDest, RefPtr<ExpressionType> type)
		{
			return From(pipe->GetPaths(worldSrc, worldDest)).Where([&](const ImportPath & p)
			{
				if (p.IsImplicitPath)
					return CheckTypeRequirement(p, type);
				return false;
			}).ToList();
		}

        Decl* SymbolTable::LookUp(String const& name)
        {
            Decl* decl = nullptr;
            if (globalDecls.TryGetValue(name, decl))
                return decl;
            return nullptr;
        }

		void SymbolTable::MergeWith(SymbolTable & symTable)
		{
			for (auto & f : symTable.FunctionOverloads)
				FunctionOverloads[f.Key] = f.Value;
			for (auto & f : symTable.Functions)
				Functions[f.Key] = f.Value;
			for (auto & f : symTable.Shaders)
				Shaders[f.Key] = f.Value;
			for (auto & f : symTable.Pipelines)
				Pipelines[f.Key] = f.Value;
			for (auto & f : symTable.globalDecls)
				globalDecls[f.Key] = f.Value;
			SortShaders();
		}

		int UniqueIdGenerator::currentGUID = 0;
		void UniqueIdGenerator::Clear()
		{
			currentGUID = 0;
		}
		int UniqueIdGenerator::Next()
		{
			return currentGUID++;
		}
		RefPtr<ShaderComponentSymbol> ShaderClosure::FindComponent(String name, bool findInPrivate, bool includeParams)
		{
			RefPtr<ShaderComponentSymbol> rs;
			if (RefMap.TryGetValue(name, rs))
			{
				if (includeParams || !rs->IsRequire())
					return rs;
				else
					return nullptr;
			}
			if (Components.TryGetValue(name, rs))
			{
				if (includeParams || !rs->IsRequire())
					return rs;
				else
					return nullptr;
			}
			for (auto & subClosure : SubClosures)
			{
				if (subClosure.Value->IsInPlace)
				{
					rs = subClosure.Value->FindComponent(name, findInPrivate, includeParams);
					if (rs && (findInPrivate || rs->Implementations.First()->SyntaxNode->IsPublic()))
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
				comps.Add(comp.Value.Symbol);
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
