#ifndef RASTER_SHADER_COMPILER_H
#define RASTER_SHADER_COMPILER_H

#include "../CoreLib/Basic.h"
#include "Diagnostics.h"
#include "CompiledProgram.h"
#include "Syntax.h"
#include "CodeGenBackend.h"

namespace Spire
{
	namespace Compiler
	{
		class ILConstOperand;
        struct IncludeHandler;

		enum class CompilerMode
		{
			ProduceShader,
			GenerateChoice
		};

		enum class CodeGenTarget
		{
			GLSL, HLSL, SPIRV
		};

		class CompileOptions
		{
		public:
			CompilerMode Mode = CompilerMode::ProduceShader;
			CodeGenTarget Target = CodeGenTarget::GLSL;
			EnumerableDictionary<String, String> BackendArguments;
			String ScheduleSource, ScheduleFileName;
			String SymbolToCompile;
			List<String> SearchDirectories;
		};

		class CompileUnit
		{
		public:
			RefPtr<ProgramSyntaxNode> SyntaxNode;
		};

		class CompilationContext
		{
		public:
			SymbolTable Symbols;
			EnumerableDictionary<String, RefPtr<ShaderClosure>> ShaderClosures;
			RefPtr<ILProgram> Program;
		};

		class ShaderCompiler : public CoreLib::Basic::Object
		{
		public:
			virtual CompileUnit Parse(CompileResult & result, String source, String fileName, IncludeHandler* includeHandler) = 0;
			virtual void Compile(CompileResult & result, CompilationContext & context, List<CompileUnit> & units, const CompileOptions & options) = 0;
			void Compile(CompileResult & result, List<CompileUnit> & units, const CompileOptions & options)
			{
				CompilationContext context;
				Compile(result, context, units, options);
			}
		};

		ShaderCompiler * CreateShaderCompiler();
	}
}

#endif