// Compiler.cpp : Defines the entry point for the console application.
//
#include "../CoreLib/Basic.h"
#include "../CoreLib/LibIO.h"
#include "ShaderCompiler.h"
#include "Lexer.h"
#include "Parser.h"
#include "Preprocessor.h"
#include "SyntaxVisitors.h"
#include "StdInclude.h"
#include "Schedule.h"
#include "CodeGenBackend.h"
#include "../CoreLib/Tokenizer.h"
#include "Closure.h"
#include "VariantIR.h"
#include "Naming.h"

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
							for (auto attrib : impl->SyntaxNode->GetLayoutAttributes())
							{
								try
								{
									if (attrib->GetValue().StartsWith("%"))
									{
										CoreLib::Text::TokenReader parser(attrib->GetValue().SubString(1, attrib->GetValue().Length() - 1));
										auto compName = parser.ReadWord();
										parser.Read(".");
										auto compAttrib = parser.ReadWord();
										RefPtr<ShaderComponentSymbol> compSym;
										if (shader->Components.TryGetValue(compName, compSym))
										{
											for (auto & timpl : compSym->Implementations)
											{
												Token attribValue;
												if (timpl->SyntaxNode->FindSimpleAttribute(compAttrib, attribValue))
													attrib->Value = attribValue;
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
			RefPtr<ShaderIR> GenerateShaderVariantIR(CompileResult & cresult, ShaderClosure * shader, Schedule & schedule, SymbolTable * symbolTable)
			{
				RefPtr<ShaderIR> result = new ShaderIR();
				result->Shader = shader;
				result->SymbolTable = symbolTable;
				// mark pinned worlds
				for (auto & comp : shader->Components)
				{
					for (auto & impl : comp.Value->Implementations)
					{
						for (auto & w : impl->Worlds)
						{
							if (impl->SrcPinnedWorlds.Contains(w) || impl->SyntaxNode->IsInline() || impl->ExportWorlds.Contains(w) || impl->SyntaxNode->IsInput())
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
					for (auto & choiceName : comp.Value.Symbol->ChoiceNames)
						choiceComps[choiceName] = comp.Value.Symbol;
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
									if (impl->Worlds.Contains(selectedDef->WorldName))
										pinnedImpl.Add(impl.Ptr());
								}
							}
							else
							{
                                cresult.GetErrorWriter()->diagnose(selectedDef.Ptr()->Position, Diagnostics::worldIsNotAValidChoiceForKey, selectedDef->WorldName, choice.Key);
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
                            {
                                auto modifier = new SimpleAttribute();
                                modifier->Key = attrib.Key;
                                modifier->Value.Content = attrib.Value;

                                modifier->next = impl->SyntaxNode->modifiers.first;
                                impl->SyntaxNode->modifiers.first = modifier;
                            }
						}
					}
				}
				// generate definitions
				Dictionary<ShaderClosure*, ModuleInstanceIR*> moduleInstanceMap;
				auto createModuleInstance = [&](ShaderClosure * closure)
				{
					ModuleInstanceIR * inst;
					if (moduleInstanceMap.TryGetValue(closure, inst))
						return inst;
					List<String> namePath;
					
					auto parent = closure;
					while (parent)
					{
						if (parent->Name.Length())
							namePath.Add(parent->Name);
						else
							namePath.Add(parent->ModuleSyntaxNode->Name.Content);
						parent = parent->Parent;
					}
					StringBuilder sbBindingName;
					for (int i = namePath.Count() - 2; i >= 0; i--)
					{
						sbBindingName << namePath[i];
						if (i > 0)
							sbBindingName << ".";
					}
					// if this is the root module, its binding name is module name.
					if (namePath.Count() == 1)
						sbBindingName << namePath[0];
					inst = new ModuleInstanceIR();
					inst->SyntaxNode = closure->ModuleSyntaxNode;
					inst->BindingIndex = closure->BindingIndex;
					inst->UsingPosition = closure->UsingPosition;
					result->ModuleInstances.Add(inst);
					inst->BindingName = sbBindingName.ProduceString();
					moduleInstanceMap[closure] = inst;
					return inst;
				};
				for (auto & comp : shader->AllComponents)
				{
					EnumerableDictionary<String, ComponentDefinitionIR*> defs;
					Dictionary<String, ShaderComponentImplSymbol*> impls;
					for (auto & impl : comp.Value.Symbol->Implementations)
					{
						auto createComponentDef = [&](const String & w)
						{
							RefPtr<ComponentDefinitionIR> def = new ComponentDefinitionIR();
							def->OriginalName = comp.Value.Symbol->Name;
							def->UniqueKey = comp.Value.Symbol->UniqueKey;
							def->UniqueName = comp.Value.Symbol->UniqueName;
							def->Type = comp.Value.Symbol->Type->DataType;
							def->IsEntryPoint = (impl->ExportWorlds.Contains(w) || impl->SyntaxNode->IsParam() ||
								(shader->Pipeline->IsAbstractWorld(w) &&
								(impl->SyntaxNode->HasSimpleAttribute("Pinned") || shader->Pipeline->Worlds[w]()->HasSimpleAttribute("Pinned"))));
							CloneContext cloneCtx;
							def->SyntaxNode = impl->SyntaxNode->Clone(cloneCtx);
							def->World = w;
							def->ModuleInstance = createModuleInstance(comp.Value.Closure);
							return def;
						};
						// parameter component will only have one defintion that is shared by all worlds
						if (impl->SyntaxNode->IsParam())
						{
							auto def = createComponentDef("<uniform>");
							result->Definitions.Add(def);
							defs["<uniform>"] = def.Ptr();
						}
						else
						{
							for (auto & w : impl->Worlds)
							{
								auto def = createComponentDef(w);
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
                            cresult.GetErrorWriter()->diagnose(def->SyntaxNode->Position, Diagnostics::componentDefinitionCircularity, def->OriginalName);
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
			ShaderSyntaxNode * InstantiateShaderTemplate(DiagnosticSink* sink, SymbolTable* symTable, TemplateShaderSyntaxNode* ts, const List<String>& args)
			{
				if (ts->Parameters.Count() > args.Count())
				{
					sink->diagnose(ts->Position, Diagnostics::insufficientTemplateShaderArguments, ts->Name);
					return nullptr;
				}
				if (ts->Parameters.Count() < args.Count())
				{
					sink->diagnose(ts->Position, Diagnostics::tooManyTemplateShaderArguments, ts->Name);
					return nullptr;
				}
				// check semantics
				bool hasErrors = false;
				for (int i = 0; i < args.Count(); i++)
				{
					auto name = args[i];
					if (auto module = symTable->Shaders.TryGetValue(name))
					{
						if (ts->Parameters[i]->InterfaceName.Content.Length())
						{
							if ((*module)->SyntaxNode->Name.Content != ts->Parameters[i]->InterfaceName.Content &&
								(*module)->SyntaxNode->InterfaceNames.FindFirst([&](const Token & t) { return t.Content == ts->Parameters[i]->InterfaceName.Content; }) == -1)
							{
								hasErrors = true;
								sink->diagnose(ts->Parameters[i]->Position, Diagnostics::templateShaderArgumentDidNotImplementRequiredInterface, name, ts->Parameters[i]->ModuleName, ts->Parameters[i]->InterfaceName);
								sink->diagnose((*module)->SyntaxNode->Position, Diagnostics::seeDefinitionOf, (*module)->SyntaxNode->Name);
							}
						}
					}
					else
					{
						hasErrors = true;
						sink->diagnose(ts->Parameters[i]->Position, Diagnostics::templateShaderArgumentIsNotDefined, args[i], ts->Parameters[i]->ModuleName.Content);
					}
				}
				if (hasErrors)
					return nullptr;
				ShaderSyntaxNode * result = new ShaderSyntaxNode();
				result->Name = ts->Name;
				StringBuilder nameBuilder;
				nameBuilder << ts->Name.Content << "<";
				for (int i = 0; i < args.Count(); i++)
				{
					nameBuilder << args[i];
					if (i < args.Count()-1)
						nameBuilder << ",";
				}
				nameBuilder << ">";
				result->Name.Content = nameBuilder.ProduceString();
				result->ParentPipelineName = ts->ParentPipelineName;
				for (auto & member : ts->Members)
				{
					if (auto import = member.As<ImportSyntaxNode>())
					{
						int index = ts->Parameters.FindFirst([&](RefPtr<TemplateShaderParameterSyntaxNode> p) { return p->ModuleName.Content == import->ShaderName.Content; });
						if (index != -1)
						{
							CloneContext cloneCtx;
							auto newImport = import->Clone(cloneCtx);
							auto attribModifier = new SimpleAttribute();
							attribModifier->Key = "Binding";
							attribModifier->Value.Content = String(index);
							newImport->modifiers.first = attribModifier;
							newImport->Scope->Parent = result->Scope;
							newImport->ShaderName.Content = args[index];
							result->Members.Add(newImport);
							continue;
						}
					}
					result->Members.Add(member);
				}
				result->IsModule = false;
				return result;
			}
		public:
			virtual CompileUnit Parse(CompileResult & result, String source, String fileName, IncludeHandler* includeHandler, Dictionary<String,String> const& preprocesorDefinitions) override
			{
                auto tokens = PreprocessSource(source, fileName, result.GetErrorWriter(), includeHandler, preprocesorDefinitions);
				CompileUnit rs;
                rs.SyntaxNode = ParseProgram(tokens, result.GetErrorWriter(), fileName);
				return rs;
			}
			virtual void Compile(CompileResult & result, CompilationContext & context, List<CompileUnit> & units, const CompileOptions & options) override
			{
				RefPtr<ProgramSyntaxNode> programSyntaxNode = new ProgramSyntaxNode();
				for (auto & unit : units)
				{
					programSyntaxNode->Include(unit.SyntaxNode.Ptr());
				}

				SymbolTable & symTable = context.Symbols;
				auto & shaderClosures = context.ShaderClosures;
				
				RefPtr<SyntaxVisitor> visitor = CreateSemanticsVisitor(&symTable, result.GetErrorWriter());
				try
				{
					programSyntaxNode->Accept(visitor.Ptr());
					if (result.GetErrorCount() > 0)
						return;
					// if user specified a template shader symbol, instantiate the template now
					String symbolToCompile = options.SymbolToCompile;
					if (symbolToCompile.Length())
					{
						auto templateShaders = programSyntaxNode->GetMembersOfType<TemplateShaderSyntaxNode>();
						for (auto & ts : templateShaders)
							if (ts->Name.Content == symbolToCompile)
							{
								auto shader = InstantiateShaderTemplate(result.GetErrorWriter(), &symTable, ts.Ptr(), options.TemplateShaderArguments);
								if (shader)
								{
									programSyntaxNode->Members.Add(shader);
									symbolToCompile = shader->Name.Content;
									programSyntaxNode->Accept(visitor.Ptr());
								}
								break;
							}
					}
					visitor = nullptr;
					symTable.EvalFunctionReferenceClosure();
					if (result.GetErrorCount() > 0)
						return;

					for (auto & shader : symTable.ShaderDependenceOrder)
					{
						if (shader->IsAbstract)
							continue;
						if (!shaderClosures.ContainsKey(shader->SyntaxNode->Name.Content))
						{
							auto shaderClosure = CreateShaderClosure(result.GetErrorWriter(), &symTable, shader);
							FlattenShaderClosure(result.GetErrorWriter(), &symTable, shaderClosure.Ptr());
							shaderClosures.Add(shader->SyntaxNode->Name.Content, shaderClosure);
						}
					}
					
					ResolveAttributes(&symTable);

					if (result.GetErrorCount() > 0)
						return;
					CodeGenBackend * backend = nullptr;
					switch(options.Target)
					{
					case CodeGenTarget::SPIRV:
						backend = backends["spirv"]().Ptr();
						break;
					case CodeGenTarget::GLSL:
						backend = backends["glsl"]().Ptr();
						break;
					case CodeGenTarget::GLSL_Vulkan:
						backend = backends["glsl_vk"]().Ptr();
						break;
					case CodeGenTarget::GLSL_Vulkan_OneDesc:
						backend = backends["glsl_vk_onedesc"]().Ptr();
						break;
					case CodeGenTarget::HLSL:
						backend = backends["hlsl"]().Ptr();
						break;
					default:
						// TODO: emit an appropriate diagnostic
						return;
					}

					Schedule schedule;
					if (options.ScheduleSource != "")
					{
						schedule = Schedule::Parse(options.ScheduleSource, options.ScheduleFileName, result.GetErrorWriter());
					}
					for (auto shader : shaderClosures)
					{
						// generate shader variant from schedule file, and also apply mechanic deduction rules
						if (!shader.Value->IR)
							shader.Value->IR = GenerateShaderVariantIR(result, shader.Value.Ptr(), schedule, &symTable);
					}
					if (options.Mode == CompilerMode::ProduceShader)
					{
						if (result.GetErrorWriter()->GetErrorCount() > 0)
							return;
						// generate IL code
						
						RefPtr<ICodeGenerator> codeGen = CreateCodeGenerator(&symTable, result);
						if (context.Program)
						{
							result.Program->Functions = context.Program->Functions;
							result.Program->Shaders = context.Program->Shaders;
							result.Program->Structs = context.Program->Structs;
							result.Program->ConstantPool = context.Program->ConstantPool;
						}
						for (auto & s : programSyntaxNode->GetStructs())
							codeGen->ProcessStruct(s.Ptr());

						for (auto & func : programSyntaxNode->GetFunctions())
							codeGen->ProcessFunction(func.Ptr());
						for (auto & shader : shaderClosures)
						{
							InsertImplicitImportOperators(result.GetErrorWriter(), shader.Value->IR.Ptr());
						}
						if (result.GetErrorCount() > 0)
							return;
						for (auto & shader : shaderClosures)
						{
							codeGen->ProcessShader(shader.Value->IR.Ptr());
						}
						if (result.GetErrorCount() > 0)
							return;
						// emit target code
						EnumerableHashSet<String> symbolsToGen;
						for (auto & unit : units)
						{
							for (auto & shader : unit.SyntaxNode->GetShaders())
								if (!shader->IsModule)
									symbolsToGen.Add(shader->Name.Content);
							for (auto & func : unit.SyntaxNode->GetFunctions())
								symbolsToGen.Add(func->Name.Content);
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
						for (auto & shader : result.Program->Shaders)
						{
							if ((symbolToCompile.Length() == 0 && IsSymbolToGen(shader->Name))
								|| EscapeCodeName(symbolToCompile) == shader->Name)
							{
								StringBuilder glslBuilder;
								Dictionary<String, String> targetCode;
								result.CompiledSource[shader->Name] = backend->GenerateShader(result, &symTable, shader.Ptr(), result.GetErrorWriter());
							}
						}
					}
					else if (options.Mode == CompilerMode::GenerateChoice)
					{
						for (auto shader : shaderClosures)
						{
							if (options.SymbolToCompile.Length() == 0 || shader.Value->Name == options.SymbolToCompile)
							{
								auto &worldOrder = shader.Value->Pipeline->GetWorldTopologyOrder();
								for (auto & comp : shader.Value->AllComponents)
								{
									ShaderChoice choice;
									if (comp.Value.Symbol->ChoiceNames.Count() == 0)
										continue;
									if (comp.Value.Symbol->IsRequire())
										continue;
									choice.ChoiceName = comp.Value.Symbol->ChoiceNames.First();
									for (auto & impl : comp.Value.Symbol->Implementations)
									{
										for (auto w : impl->Worlds)
											if (comp.Value.Symbol->Type->ConstrainedWorlds.Contains(w))
												choice.Options.Add(ShaderChoiceValue(w));
									}
									if (auto defs = shader.Value->IR->DefinitionsByComponent.TryGetValue(comp.Key))
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
                        result.GetErrorWriter()->diagnose(CodePosition(), Diagnostics::unsupportedCompilerMode);
						return;
					}
					context.Program = result.Program;
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
				backends.Add("glsl", CreateGLSLCodeGen());
				backends.Add("hlsl", CreateHLSLCodeGen());
				backends.Add("spirv", CreateSpirVCodeGen());
				backends.Add("glsl_vk", CreateGLSL_VulkanCodeGen());
				backends.Add("glsl_vk_onedesc", CreateGLSL_VulkanOneDescCodeGen());
			}

			~ShaderCompilerImpl()
			{
				compilerInstances--;
				if (compilerInstances == 0)
				{
					BasicExpressionType::Finalize();
					SpireStdLib::Finalize();
				}
			}
		};

		ShaderCompiler * CreateShaderCompiler()
		{
			return new ShaderCompilerImpl();
		}

	}
}
