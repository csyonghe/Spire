#ifndef RASTER_SHADER_COMPILER_H
#define RASTER_SHADER_COMPILER_H

#include "../CoreLib/Basic.h"
#include "CompileError.h"
#include "CompiledProgram.h"
#include "Syntax.h"
#include "CodeGenBackend.h"

namespace Spire
{
	namespace Compiler
	{
		class ILConstOperand;

		enum class CompilerMode
		{
			ProduceShader,
			GenerateChoice
		};

		enum class CodeGenTarget
		{
			GLSL, SPIRV
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

		class ShaderCompiler : public CoreLib::Basic::Object
		{
		public:
			virtual CompileUnit Parse(CompileResult & result, String source, String fileName) = 0;
			virtual void Compile(CompileResult & result, List<CompileUnit> & units, const CompileOptions & options) = 0;
		};

		ShaderCompiler * CreateShaderCompiler();
	}
}

#endif