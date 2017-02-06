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
			ProduceLibrary,
			ProduceShader,
			GenerateChoice
		};

		enum class Language
		{
			Unknown,
#define LANGUAGE(TAG, NAME) TAG,
#include "ProfileDefs.h"
		};

		enum class ProfileFamily
		{
			Unknown,
#define PROFILE_FAMILY(TAG) TAG,
#include "ProfileDefs.h"
		};

		enum class ProfileVersion
		{
			Unknown,
#define PROFILE_VERSION(TAG, FAMILY) TAG,
#include "ProfileDefs.h"
		};

		enum class Stage
		{
			Unknown,
#define PROFILE_STAGE(TAG, NAME) TAG,
#include "ProfileDefs.h"
		};

		struct Profile
		{
			typedef uint32_t RawVal;
			enum : RawVal
			{
			Unknown,

#define PROFILE(TAG, NAME, STAGE, VERSION) TAG = (uint32_t(Stage::STAGE) << 16) | uint32_t(ProfileVersion::VERSION),
#include "ProfileDefs.h"
			};

			Profile() {}
			Profile(RawVal raw)
				: raw(raw)
			{}

			Stage GetStage() const { return Stage((uint32_t(raw) >> 16) & 0xFFFF); }
			ProfileVersion GetVersion() const { return ProfileVersion(uint32_t(raw) & 0xFFFF); }

			RawVal raw = Unknown;
		};

		enum class StageTarget
		{
			Unknown,
			VertexShader,
			HullShader,
			DomainShader,
			GeometryShader,
			FragmentShader,
			ComputeShader,
		};

		enum class CodeGenTarget
		{
			Unknown,

			GLSL, GLSL_Vulkan, GLSL_Vulkan_OneDesc, HLSL, SPIRV,

			DXBytecode,
			DXBytecodeAssembly,
		};

		// Describes an entry point that we've been requested to compile
		struct EntryPointOption
		{
			String name;
			Profile profile;
		};

		class CompileOptions
		{
		public:
			CompilerMode Mode = CompilerMode::ProduceShader;
			CodeGenTarget Target = CodeGenTarget::Unknown;
			StageTarget stage = StageTarget::Unknown;
			EnumerableDictionary<String, String> BackendArguments;
			String ScheduleSource, ScheduleFileName;
			String SymbolToCompile;
			List<String> TemplateShaderArguments;
			List<String> SearchDirectories;
            Dictionary<String, String> PreprocessorDefinitions;

			// All entry points we've been asked to compile
			List<EntryPointOption> entryPoints;

			// the code generation profile we've been asked to use
			Profile profile;
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
			virtual CompileUnit Parse(
				CompileResult & result,
				String source,
				String fileName,
				IncludeHandler* includeHandler,
				Dictionary<String,String> const& preprocessorDefinitions,
				CompileUnit predefUnit) = 0;
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