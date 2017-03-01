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

                    RefPtr<ILProgram> program = new ILProgram();
                    RefPtr<SyntaxVisitor> codeGen = CreateILCodeGenerator(result.GetErrorWriter(), program.Ptr(), const_cast<CompileOptions*>(&options));
                    codeGen->VisitProgram(programSyntaxNode.Ptr());
                    
                    if (result.GetErrorCount() > 0)
                        return;

                    RefPtr<CodeGenBackend> backend = nullptr;
                    switch (options.Target)
                    {
                    case CodeGenTarget::HLSL:
                        backend = CreateHLSLCodeGen();
                        break;
                    case CodeGenTarget::GLSL:
                        backend = CreateGLSLCodeGen();
                        break;
                    case CodeGenTarget::GLSL_Vulkan:
                        backend = CreateGLSL_VulkanCodeGen();
                        break;
                    default:
                        result.sink.diagnose(CodePosition(), Diagnostics::unimplemented, "specified backend");
                        return;
                    }

                    CompiledShaderSource compiledSource = backend->GenerateShader(options, program.Ptr(), &result.sink);
                    compiledSource.MetaData.ShaderName = options.outputName;
                    StageSource stageSrc;
                    stageSrc.MainCode = program->ToString();
                    compiledSource.Stages["il"] = stageSrc;
                    result.CompiledSource["code"] = compiledSource;

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
