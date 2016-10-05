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
#include "VariantIR.h"

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
		int compilerInstances = 0;

		class ShaderCompilerImpl : public ShaderCompiler
		{
		private:
			Dictionary<String, RefPtr<CodeGenBackend>> backends;

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
							if (impl->SrcPinnedWorlds.Contains(w) || impl->SyntaxNode->IsInline || impl->ExportWorlds.Contains(w) || impl->SyntaxNode->IsInput)
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
					Dictionary<String, ShaderComponentImplSymbol*> impls;
					for (auto & impl : comp.Value->Implementations)
					{
						for (auto & w : impl->Worlds)
						{
							RefPtr<ComponentDefinitionIR> def = new ComponentDefinitionIR();
							def->OriginalName = comp.Value->Name;
							def->UniqueKey = comp.Value->UniqueKey;
							def->UniqueName = comp.Value->UniqueName;
							def->Type = comp.Value->Type->DataType;
							def->IsEntryPoint = (impl->ExportWorlds.Contains(w) ||
								(shader->Pipeline->IsAbstractWorld(w) &&
								(impl->SyntaxNode->LayoutAttributes.ContainsKey(L"Pinned") || shader->Pipeline->Worlds[w]().SyntaxNode->LayoutAttributes.ContainsKey(L"Pinned"))));
							CloneContext cloneCtx;
							def->SyntaxNode = impl->SyntaxNode->Clone(cloneCtx);
							def->World = w;
							result->Definitions.Add(def);
							bool existingDefIsPinned = false;
							if (defs.ContainsKey(w))
								existingDefIsPinned = pinnedImpl.Contains(impls[w]());
							if (!existingDefIsPinned)
							{
								defs[w] = def.Ptr();
								impls[w] = impl.Ptr();
							}
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
							cresult.GetErrorWriter()->Error(33102, L"component definition \'" + def->OriginalName + L"\' involves circular reference.",
								def->SyntaxNode->Position);
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
					CodeGenBackend * backend = nullptr;
					if (options.Target == CodeGenTarget::SPIRV)
						backend = backends[L"spirv"]().Ptr();
					else
						backend = backends[L"glsl"]().Ptr();

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
						RefPtr<ICodeGenerator> codeGen = CreateCodeGenerator(&symTable, result);
						for (auto & s : programSyntaxNode->Structs)
							codeGen->ProcessStruct(s.Ptr());
						for (auto & func : programSyntaxNode->Functions)
							codeGen->ProcessFunction(func.Ptr());
						for (auto & shader : shaderClosures)
						{
							InsertImplicitImportOperators(shader->IR.Ptr());
						}
						if (result.ErrorList.Count() > 0)
							return;
						for (auto & shader : shaderClosures)
						{
							codeGen->ProcessShader(shader->IR.Ptr());
						}
						if (result.ErrorList.Count() > 0)
							return;
						// emit target code
						EnumerableHashSet<String> symbolsToGen;
						for (auto & unit : units)
						{
							for (auto & shader : unit.SyntaxNode->Shaders)
								if (!shader->IsModule)
									symbolsToGen.Add(shader->Name.Content);
							for (auto & func : unit.SyntaxNode->Functions)
								symbolsToGen.Add(func->Name);
						}
						auto IsSymbolToGen = [&](String & shaderName)
						{
							if (symbolsToGen.Contains(shaderName))
								return true;
							for (auto & symbol : symbolsToGen)
								if (shaderName.StartsWith(symbol))
									return true;
							return false;
						};
						ErrorWriter errWriter(result.ErrorList, result.WarningList);
						for (auto & shader : result.Program->Shaders)
						{
							if ((options.SymbolToCompile.Length() == 0 && IsSymbolToGen(shader->Name))
								|| options.SymbolToCompile == shader->Name)
							{
								StringBuilder glslBuilder;
								Dictionary<String, String> targetCode;
								result.CompiledSource[shader->Name] = backend->GenerateShader(result, &symTable, shader.Ptr(), &errWriter);
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

			ShaderCompilerImpl()
			{
				if (compilerInstances == 0)
				{
					BasicExpressionType::Init();
				}
				compilerInstances++;
				backends.Add(L"glsl", CreateGLSLCodeGen());
				//backends.Add(L"spirv", CreateSpirVCodeGen());
			}

			~ShaderCompilerImpl()
			{
				compilerInstances--;
				if (compilerInstances == 0)
				{
					BasicExpressionType::Finalize();
				}
			}
		};

		ShaderCompiler * CreateShaderCompiler()
		{
			return new ShaderCompilerImpl();
		}

	}
}
