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
#include "CodeGenBackend.h"
#include "../CoreLib/Tokenizer.h"
#include "Naming.h"

#include "Emit.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#undef WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#include <d3dcompiler.h>
#endif

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
        public:
            virtual CompileUnit Parse(CompileResult & result, String source, String fileName, IncludeHandler* includeHandler, Dictionary<String,String> const& preprocesorDefinitions,
                CompileUnit predefUnit) override
            {
                auto tokens = PreprocessSource(source, fileName, result.GetErrorWriter(), includeHandler, preprocesorDefinitions);
                CompileUnit rs;
                rs.SyntaxNode = ParseProgram(tokens, result.GetErrorWriter(), fileName, predefUnit.SyntaxNode.Ptr());
                return rs;
            }

            // Actual context for compilation... :(
            struct ExtraContext
            {
                CompileOptions const* options = nullptr;

                CompileResult* compileResult = nullptr;

                RefPtr<ProgramSyntaxNode> programSyntax;

                String sourceText;
                String sourcePath;

                CompileOptions const& getOptions() { return *options; }
            };


            String EmitHLSL(ExtraContext& context)
            {
                if (context.getOptions().passThrough != PassThroughMode::None)
                {
                    return context.sourceText;
                }
                else
                {
                    // TODO(tfoley): probably need a way to customize the emit logic...
                    return EmitProgram(context.programSyntax.Ptr());
                }
            }

            char const* GetHLSLProfileName(Profile profile)
            {
                switch(profile.raw)
                {
                #define PROFILE(TAG, NAME, STAGE, VERSION) case Profile::TAG: return #NAME;
                #include "ProfileDefs.h"

                default:
                    // TODO: emit an error here!
                    return "unknown";
                }
            }

#ifdef _WIN32
            void* GetD3DCompilerDLL()
            {
                // TODO(tfoley): let user specify version of d3dcompiler DLL to use.
                static HMODULE d3dCompiler =  LoadLibraryA("d3dcompiler_47");
                // TODO(tfoley): handle case where we can't find it gracefully
                assert(d3dCompiler);
                return d3dCompiler;
            }

            List<uint8_t> EmitDXBytecodeForEntryPoint(
                ExtraContext&				context,
                EntryPointOption const&		entryPoint)
            {
                static pD3DCompile D3DCompile_ = nullptr;
                if (!D3DCompile_)
                {
                    HMODULE d3dCompiler = (HMODULE)GetD3DCompilerDLL();
                    assert(d3dCompiler);

                    D3DCompile_ = (pD3DCompile)GetProcAddress(d3dCompiler, "D3DCompile");
                    assert(D3DCompile_);
                }

                String hlslCode = EmitHLSL(context);

                ID3DBlob* codeBlob;
                ID3DBlob* diagnosticsBlob;
                HRESULT hr = D3DCompile_(
                    hlslCode.begin(),
                    hlslCode.Length(),
                    context.sourcePath.begin(),
                    nullptr,
                    nullptr,
                    entryPoint.name.begin(),
                    GetHLSLProfileName(entryPoint.profile),
                    0,
                    0,
                    &codeBlob,
                    &diagnosticsBlob);
                List<uint8_t> data;
                if (codeBlob)
                {
                    data.AddRange((uint8_t const*)codeBlob->GetBufferPointer(), (int)codeBlob->GetBufferSize());
                    codeBlob->Release();
                }
                if (diagnosticsBlob)
                {
                    // TODO(tfoley): need a better policy for how we translate diagnostics
                    // back into the Spire world (although we should always try to generate
                    // HLSL that doesn't produce any diagnostics...)
                    String diagnostics = (char const*) diagnosticsBlob->GetBufferPointer();
                    fprintf(stderr, "%s", diagnostics.begin());
                    diagnosticsBlob->Release();
                }
                if (FAILED(hr))
                {
                    // TODO(tfoley): What to do on failure?
                }
                return data;
            }

            List<uint8_t> EmitDXBytecode(
                ExtraContext&				context)
            {
                if(context.getOptions().entryPoints.Count() != 1)
                {
                    if(context.getOptions().entryPoints.Count() == 0)
                    {
                        // TODO(tfoley): need to write diagnostics into this whole thing...
                        fprintf(stderr, "no entry point specified\n");
                    }
                    else
                    {
                        fprintf(stderr, "multiple entry points specified\n");
                    }
                    return List<uint8_t>();
                }

                return EmitDXBytecodeForEntryPoint(context, context.getOptions().entryPoints[0]);
            }

            String EmitDXBytecodeAssemblyForEntryPoint(
                ExtraContext&				context,
                EntryPointOption const&		entryPoint)
            {
                static pD3DDisassemble D3DDisassemble_ = nullptr;
                if (!D3DDisassemble_)
                {
                    HMODULE d3dCompiler = (HMODULE)GetD3DCompilerDLL();
                    assert(d3dCompiler);

                    D3DDisassemble_ = (pD3DDisassemble)GetProcAddress(d3dCompiler, "D3DDisassemble");
                    assert(D3DDisassemble_);
                }

                List<uint8_t> dxbc = EmitDXBytecodeForEntryPoint(context, entryPoint);
                if (!dxbc.Count())
                {
                    return "";
                }

                ID3DBlob* codeBlob;
                HRESULT hr = D3DDisassemble_(
                    &dxbc[0],
                    dxbc.Count(),
                    0,
                    nullptr,
                    &codeBlob);

                String result;
                if (codeBlob)
                {
                    result = String((char const*) codeBlob->GetBufferPointer());
                    codeBlob->Release();
                }
                if (FAILED(hr))
                {
                    // TODO(tfoley): need to figure out what to diagnose here...
                }
                return result;
            }


            String EmitDXBytecodeAssembly(
                ExtraContext&				context)
            {
                if(context.getOptions().entryPoints.Count() == 0)
                {
                    // TODO(tfoley): need to write diagnostics into this whole thing...
                    fprintf(stderr, "no entry point specified\n");
                    return "";
                }

                StringBuilder sb;
                for (auto entryPoint : context.getOptions().entryPoints)
                {
                    sb << EmitDXBytecodeAssemblyForEntryPoint(context, entryPoint);
                }
                return sb.ProduceString();
            }
#endif

            void DoNewEmitLogic(ExtraContext& context)
            {
                switch (context.getOptions().Target)
                {
                case CodeGenTarget::HLSL:
                    {
                        String hlslProgram = EmitHLSL(context);

                        if (context.compileResult)
                        {
                            StageSource stageSource;
                            stageSource.MainCode = hlslProgram;
                            CompiledShaderSource compiled;
                            compiled.Stages[""] = stageSource;
                            context.compileResult->CompiledSource[""] = compiled;
                        }
                        else
                        {
                            fprintf(stdout, "%s", hlslProgram.begin());
                        }
                        return;
                    }
                    break;

                case CodeGenTarget::DXBytecodeAssembly:
                    {
                        String hlslProgram = EmitDXBytecodeAssembly(context);

                        // HACK(tfoley): just print it out since that is what people probably expect.
                        // TODO: need a way to control where output gets routed across all possible targets.
                        fprintf(stdout, "%s", hlslProgram.begin());
                        return;
                    }
                    break;

                default:
                    throw "unimplemented";
                    return;
                }
            }


            virtual void Compile(CompileResult & result, CompilationContext & /*context*/, List<CompileUnit> & units, const CompileOptions & options) override
            {
                RefPtr<ProgramSyntaxNode> programSyntaxNode = new ProgramSyntaxNode();
                for (auto & unit : units)
                {
                    programSyntaxNode->Include(unit.SyntaxNode.Ptr());
                }

                
                RefPtr<SyntaxVisitor> visitor = CreateSemanticsVisitor(result.GetErrorWriter());
                try
                {
                    programSyntaxNode->Accept(visitor.Ptr());
                    if (result.GetErrorCount() > 0)
                        return;

#if 1
                    RefPtr<ILProgram> program = new ILProgram();
                    RefPtr<SyntaxVisitor> codeGen = CreateILCodeGenerator(result.GetErrorWriter(), program.Ptr(), const_cast<CompileOptions*>(&options));
                    codeGen->VisitProgram(programSyntaxNode.Ptr());
                    
                    if (result.GetErrorCount() > 0)
                        return;
                    CompiledShaderSource src;
                    StageSource stageSrc;
                    src.MetaData.ShaderName = options.outputName;
                    stageSrc.MainCode = program->ToString();
                    src.Stages["all"] = stageSrc;
                    result.CompiledSource["il"] = src;

                    // HACK(tfoley): for right now I just want to pretty-print an AST
                    // into another language, so the whole compiler back-end is just
                    // getting in the way.
                    //
                    // I'm going to bypass it for now and see what I can do:

                    ExtraContext extra;
                    extra.options = &options;
                    extra.programSyntax = programSyntaxNode;
                    extra.sourcePath = "spire"; // don't have this any more!
                    extra.sourcePath = "";
                    extra.compileResult = &result;

                    DoNewEmitLogic(extra);
                    return;

#else


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

                        // Generate symbols for any global variables
                        for (auto & globalVar : programSyntaxNode->GetMembersOfType<VarDeclBase>())
                            codeGen->ProcessGlobalVar(globalVar.Ptr());

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
#endif
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

            virtual void PassThrough(
                CompileResult &			/*result*/,
                String const&			sourceText,
                String const&			sourcePath,
                const CompileOptions &	options) override
            {
                ExtraContext extra;
                extra.options = &options;
                extra.sourcePath = sourcePath;
                extra.sourceText = sourceText;

                DoNewEmitLogic(extra);
                return;

            }

        };

        ShaderCompiler * CreateShaderCompiler()
        {
            return new ShaderCompilerImpl();
        }

    }
}
