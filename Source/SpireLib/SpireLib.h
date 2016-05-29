#ifndef LIB_BAKER_SL_H
#define LIB_BAKER_SL_H

#include "CoreLib/Basic.h"
#include "CoreLib/Parser.h"
#include "../SpireCore/ShaderCompiler.h"

namespace SpireLib
{
	class ShaderLibFile : public CoreLib::Basic::Object
	{
	public:
		CoreLib::Basic::String BinaryFileName, BinarySourceName, BinarySource;
		CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, Spire::Compiler::CompiledShaderSource> Sources; // indexed by world
		Spire::Compiler::ShaderMetaData MetaData;
		void AddSource(CoreLib::Basic::String source, CoreLib::Text::Parser & parser);
		void SaveToFile(CoreLib::Basic::String fileName);
		ShaderLibFile() = default;
		void Clear();
		void Load(CoreLib::Basic::String fileName);
		bool ProduceBinary(CoreLib::Basic::String outputDir);
	};
	
	CoreLib::Basic::List<ShaderLibFile> CompileShaderSource(Spire::Compiler::CompileResult & result,
		CoreLib::Basic::String sourceFileName,
		Spire::Compiler::CompileOptions &options);

	class ShaderLib : public ShaderLibFile
	{
	public:
		Spire::Compiler::CompiledShaderSource GetWorldSource(CoreLib::Basic::String world);
		ShaderLib() = default;
		ShaderLib(CoreLib::Basic::String fileName);
		void Reload(CoreLib::Basic::String fileName);
		bool CompileFrom(CoreLib::Basic::String symbolName, CoreLib::Basic::String sourceFileName, CoreLib::Basic::String schedule, CoreLib::Basic::String outputDir);
	};
}

#endif