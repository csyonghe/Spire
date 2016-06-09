// Compiler.cpp : Defines the entry point for the console application.
//
#include "../CoreLib/Basic.h"
#include "../CoreLib/LibIO.h"
#include "ShaderCompiler.h"
#include "Lexer.h"
#include "Parser.h"
#include "SyntaxVisitors.h"
#include "StdInclude.h"
#include "Schedule.h"
#include "CodeGenBackend.h"
#include "../CoreLib/Parser.h"
#include "Closure.h"

#ifdef CreateDirectory
#undef CreateDirectory
#endif

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace Spire::Compiler;

namespace Spire
{
	namespace Compiler
	{
		class ShaderCompilerImpl : public ShaderCompiler
		{
		private:
			Dictionary<String, RefPtr<CodeGenBackend>> backends;
			Dictionary<String, Dictionary<String, ImportOperatorHandler *>> opHandlers;
			Dictionary<String, Dictionary<String, ExportOperatorHandler *>> exportHandlers;

			void ResolveAttributes(SymbolTable * symTable)
			{
				for (auto & shader : symTable->ShaderDependenceOrder)
				{
					auto comps = shader->GetComponentDependencyOrder();
					for (auto & comp : comps)
					{
						for (auto & impl : comp->Implementations)
							for (auto & attrib : impl->SyntaxNode->LayoutAttributes)
							{
								try
								{
									if (attrib.Value.StartsWith(L"%"))
									{
										CoreLib::Text::Parser parser(attrib.Value.SubString(1, attrib.Value.Length() - 1));
										auto compName = parser.ReadWord();
										parser.Read(L".");
										auto compAttrib = parser.ReadWord();
										RefPtr<ShaderComponentSymbol> compSym;
										if (shader->Components.TryGetValue(compName, compSym))
										{
											for (auto & timpl : compSym->Implementations)
											{
												String attribValue;
												if (timpl->SyntaxNode->LayoutAttributes.TryGetValue(compAttrib, attribValue))
													attrib.Value = attribValue;
											}
										}
									}
								}
								catch (Exception)
								{
								}
							}
					}
				}
			}

			/* Generate a shader variant by applying mechanic choice rules and the choice file.
			   The choice file provides "preferred" definitions, as represented in ShaderComponentSymbol::Type::PinnedWorlds
		       The process resolves the component references by picking a pinned definition if one is available, or a definition
			   with the preferred import path as defined by import operator ordering.
			   After all references are resolved, all unreferenced definitions (dead code) are eliminated, 
			   resulting a shader variant ready for code generation.
			*/
			RefPtr<ShaderIR> GenerateShaderVariantIR(CompileResult & cresult, ShaderClosure * shader, Schedule & schedule)
			{
				RefPtr<ShaderIR> result = new ShaderIR();
				result->Shader = shader;
				// mark pinned worlds
				for (auto & comp : shader->Components)
				{
					for (auto & impl : comp.Value->Implementations)
					{
						for (auto & w : impl->Worlds)
						{
							if (impl->SrcPinnedWorlds.Contains(w) || impl->SyntaxNode->IsInline || impl->ExportWorlds.Contains(w))
							{
								comp.Value->Type->PinnedWorlds.Add(w);
							}
						}
					}
				}
				// apply choices
				Dictionary<String, ShaderComponentSymbol*> choiceComps;
				for (auto & comp : shader->AllComponents)
				{
					for (auto & choiceName : comp.Value->ChoiceNames)
						choiceComps[choiceName] = comp.Value;
				}
				HashSet<ShaderComponentImplSymbol*> pinnedImpl;
				for (auto & choice : schedule.Choices)
				{
					ShaderComponentSymbol * comp = nullptr;
					if (choiceComps.TryGetValue(choice.Key, comp))
					{
						comp->Type->PinnedWorlds.Clear();
						for (auto & selectedDef : choice.Value)
						{
							if (comp->Type->ConstrainedWorlds.Contains(selectedDef->WorldName))
							{
								comp->Type->PinnedWorlds.Add(selectedDef->WorldName);
								// find specified impl
								for (auto & impl : comp->Implementations)
								{
									if (impl->AlternateName == selectedDef->AlternateName && impl->Worlds.Contains(selectedDef->WorldName))
										pinnedImpl.Add(impl.Ptr());
								}
							}
							else
							{
								cresult.GetErrorWriter()->Warning(33101, L"'" + selectedDef->WorldName + L"' is not a valid choice for '" + choice.Key
									+ L"'.", selectedDef.Ptr()->Position);
							}
						}
					}
				}
				for (auto & attribs : schedule.AddtionalAttributes)
				{
					ShaderComponentSymbol * comp = nullptr;
					if (choiceComps.TryGetValue(attribs.Key, comp))
					{
						// apply attributes
						for (auto & impl : comp->Implementations)
						{
							for (auto & attrib : attribs.Value)
								impl->SyntaxNode->LayoutAttributes[attrib.Key] = attrib.Value;
						}
					}
				}
				// generate definitions
				for (auto & comp : shader->AllComponents)
				{
					EnumerableDictionary<String, ComponentDefinitionIR*> defs;
					for (auto & impl : comp.Value->Implementations)
					{
						for (auto & w : impl->Worlds)
						{
							RefPtr<ComponentDefinitionIR> def = new ComponentDefinitionIR();
							def->Component = comp.Value;
							def->Implementation = impl.Ptr();
							def->World = w;
							result->Definitions.Add(def);
							bool existingDefIsPinned = false;
							if (auto existingDef = defs.TryGetValue(w))
								existingDefIsPinned = pinnedImpl.Contains((*existingDef)->Implementation);
							if (!existingDefIsPinned)
								defs[w] = def.Ptr();
						}
					}
					result->DefinitionsByComponent[comp.Key] = defs;
				}
				bool changed = true;
				while (changed)
				{
					changed = false;
					result->ResolveComponentReference();
					result->EliminateDeadCode();
					// check circular references
					for (auto & def : result->Definitions)
					{
						if (def->Dependency.Contains(def.Ptr()))
						{
							cresult.GetErrorWriter()->Error(33102, L"component definition \'" + def->Component->Name + L"\' involves circular reference.",
								def->Implementation->SyntaxNode->Position);
							return nullptr;
						}
					}
					/*
					// eliminate redundant (downstream) definitions, one at a time
					auto comps = result->GetComponentDependencyOrder();
					for (int i = comps.Count() - 1; i >= 0; i--)
					{
						auto comp = comps[i];
						auto & defs = result->DefinitionsByComponent[comp->UniqueName]();
						EnumerableHashSet<ComponentDefinitionIR*> removedDefs;
						for (auto & def : defs)
							if (!def.Value->IsEntryPoint && !comp->Type->PinnedWorlds.Contains(def.Value->World))
							{
								for (auto & otherDef : defs)
								{
									if (otherDef.Value != def.Value && !removedDefs.Contains(otherDef.Value)
										&& shader->Pipeline->IsWorldReachable(otherDef.Value->World, def.Value->World))
									{
										removedDefs.Add(def.Value);
										break;
									}
								}
							}
						if (removedDefs.Count())
						{
							result->RemoveDefinitions([&](ComponentDefinitionIR* def) {return removedDefs.Contains(def); });
							changed = true;
						}
					}
					*/
				}
				return result;
			}
		public:
			virtual CompileUnit Parse(CompileResult & result, String source, String fileName) override
			{
				result.Success = false;
				Lexer lexer;
				auto tokens = lexer.Parse(fileName, source, result.ErrorList);
				Parser parser(tokens, result.ErrorList, fileName);
				CompileUnit rs;
				rs.SyntaxNode = parser.Parse();
				return rs;
			}
			virtual void Compile(CompileResult & result, List<CompileUnit> & units, const CompileOptions & options) override
			{
				result.Success = false;
				RefPtr<ProgramSyntaxNode> programSyntaxNode = new ProgramSyntaxNode();
				for (auto & unit : units)
				{
					programSyntaxNode->Include(unit.SyntaxNode.Ptr());
				}

				SymbolTable symTable;
				RefPtr<SyntaxVisitor> visitor = CreateSemanticsVisitor(&symTable, result.GetErrorWriter());
				try
				{
					programSyntaxNode->Accept(visitor.Ptr());
					visitor = nullptr;
					if (result.ErrorList.Count() > 0)
						return;
					symTable.EvalFunctionReferenceClosure();
					if (result.ErrorList.Count() > 0)
						return;
					List<RefPtr<ShaderClosure>> shaderClosures;

					for (auto & shader : symTable.ShaderDependenceOrder)
					{
						if (shader->IsAbstract)
							continue;
						auto shaderClosure = CreateShaderClosure(result.GetErrorWriter(), &symTable, shader);
						FlattenShaderClosure(result.GetErrorWriter(), shaderClosure.Ptr());
						shaderClosures.Add(shaderClosure);
					}
					
					ResolveAttributes(&symTable);

					if (result.ErrorList.Count() > 0)
						return;

					Schedule schedule;
					if (options.ScheduleSource != L"")
					{
						schedule = Schedule::Parse(options.ScheduleSource, options.ScheduleFileName, result.ErrorList);
					}
					for (auto shader : shaderClosures)
					{
						// generate shader variant from schedule file, and also apply mechanic deduction rules
						shader->IR = GenerateShaderVariantIR(result, shader.Ptr(), schedule);
					}
					if (options.Mode == CompilerMode::ProduceShader)
					{
						if (result.ErrorList.Count() > 0)
							return;
						// generate IL code
						RefPtr<ICodeGenerator> codeGen = CreateCodeGenerator(this, &symTable, result);
						for (auto & func : programSyntaxNode->Functions)
							codeGen->ProcessFunction(func.Ptr());
						for (auto & shader : shaderClosures)
							codeGen->ProcessShader(shader.Ptr());
						if (result.ErrorList.Count() > 0)
							return;
						// emit target code
						EnumerableHashSet<String> symbolsToGen;
						for (auto & shader : units[0].SyntaxNode->Shaders)
							if (!shader->IsModule)
								symbolsToGen.Add(shader->Name.Content);
						for (auto & func : units[0].SyntaxNode->Functions)
							symbolsToGen.Add(func->Name);
						auto IsSymbolToGen = [&](String & shaderName)
						{
							if (symbolsToGen.Contains(shaderName))
								return true;
							for (auto & symbol : symbolsToGen)
								if (shaderName.StartsWith(symbol))
									return true;
							return false;
						};
						for (auto & shader : result.Program->Shaders)
						{
							if (options.SymbolToCompile.Length() == 0 && IsSymbolToGen(shader->MetaData.ShaderName)
								|| options.SymbolToCompile == shader->MetaData.ShaderName)
							{
								StringBuilder glslBuilder;
								Dictionary<String, String> targetCode;
								result.CompiledSource[shader->MetaData.ShaderName + L".glsl"] = EnumerableDictionary<String, CompiledShaderSource>();
								auto & worldSources = result.CompiledSource[shader->MetaData.ShaderName + L".glsl"]();
								for (auto & world : shader->Worlds)
								{
									if (world.Value->IsAbstract)
										continue;
									RefPtr<CodeGenBackend> backend;
									if (!backends.TryGetValue(world.Value->TargetMachine, backend))
									{
										result.GetErrorWriter()->Error(40000, L"backend '" + world.Value->TargetMachine + L"' is not supported.",
											world.Value->WorldDefPosition);
									}
									else
									{
										backend->SetParameters(world.Value->BackendParameters);
										Dictionary<String, ImportOperatorHandler*> importHandlers;
										Dictionary<String, ExportOperatorHandler*> beExportHandlers;

										opHandlers.TryGetValue(world.Value->TargetMachine, importHandlers);
										exportHandlers.TryGetValue(world.Value->TargetMachine, beExportHandlers);

										worldSources[world.Key] = backend->GenerateShaderWorld(result, &symTable, world.Value.Ptr(), importHandlers, beExportHandlers);
									}
								}
							}
						}
						result.Success = result.ErrorList.Count() == 0;
					}
					else if (options.Mode == CompilerMode::GenerateChoice)
					{
						for (auto shader : shaderClosures)
						{
							if (options.SymbolToCompile.Length() == 0 || shader->Name == options.SymbolToCompile)
							{
								auto &worldOrder = shader->Pipeline->GetWorldTopologyOrder();
								for (auto & comp : shader->AllComponents)
								{
									ShaderChoice choice;
									if (comp.Value->ChoiceNames.Count() == 0)
										continue;
									if (comp.Value->IsParam())
										continue;
									choice.ChoiceName = comp.Value->ChoiceNames.First();
									for (auto & impl : comp.Value->Implementations)
									{
										for (auto w : impl->Worlds)
											if (comp.Value->Type->ConstrainedWorlds.Contains(w))
												choice.Options.Add(ShaderChoiceValue(w, impl->AlternateName));
									}
									if (auto defs = shader->IR->DefinitionsByComponent.TryGetValue(comp.Key))
									{
										int latestWorldOrder = -1;
										for (auto & def : *defs)
										{
											int order = worldOrder.IndexOf(def.Key);
											if (latestWorldOrder < order)
											{
												choice.DefaultValue = def.Key;
												latestWorldOrder = order;
											}
										}
									}
									result.Choices.Add(choice);
								}
							}
						}
					}
					else
					{
						result.GetErrorWriter()->Error(2, L"unsupported compiler mode.", CodePosition());
						return;
					}
					result.Success = true;
				}
				catch (int)
				{
				}
				catch (...)
				{
					throw;
				}
				return;
			}

			virtual void RegisterImportOperator(String backendName, ImportOperatorHandler * handler) override
			{
				if (!opHandlers.ContainsKey(backendName))
					opHandlers[backendName] = Dictionary<String, ImportOperatorHandler*>();
				opHandlers[backendName]().Add(handler->GetName(), handler);
			}

			virtual void RegisterExportOperator(String backendName, ExportOperatorHandler * handler) override
			{
				if (!exportHandlers.ContainsKey(backendName))
					exportHandlers[backendName] = Dictionary<String, ExportOperatorHandler*>();
				exportHandlers[backendName]().Add(handler->GetName(), handler);
			}

			ShaderCompilerImpl()
			{
				backends.Add(L"glsl", CreateGLSLCodeGen());
			}
		};

		ShaderCompiler * CreateShaderCompiler()
		{
			return new ShaderCompilerImpl();
		}

	}
}
