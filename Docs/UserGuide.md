# Using Spire
If you have read the [Understanding Spire](https://github.com/csyonghe/Spire/tree/master/Docs/tutorial1) tutorial and want to try integrating Spire into your engine, follow this guide to learn the compiler interface.
### As Library
The Spire compiler is distributed directly as four C++ source files with no external dependencies. To integrate the compiler into your engine, simply grab "Spire.h" "Spire.cpp" "Basic.h" "Basic.cpp" from "LibraryRelease" directory and place them into your project.
To invoke the compiler, call:
```c++
CoreLib::List<SpireLib::ShaderLibFile> CompileShaderSource(Spire::Compiler::CompileResult & result,
	CoreLib::String sourceFileName,
	Spire::Compiler::CompileOptions &options);
```
Compiles shader from given filename. Each compiled shader correspond to one `SpireLib::ShaderLibFile` object in the returned the list.
Alternatively,
```c++
CoreLib::List<SpireLib::ShaderLibFile> CompileShaderSource(Spire::Compiler::CompileResult & result,
	const CoreLib::String &source, const CoreLib::String &sourceFileName,
	Spire::Compiler::CompileOptions &options);
```
compiles shader from given source string. `sourceFileName` argument can be any identifier.
You can then access the compiled shader code for each world using the returned `ShaderLibFile` objects. The following is the definition of `ShaderLibFile`.
```c++
namespace SpireLib
{
	class ShaderLibFile : public CoreLib::Basic::Object
	{
	public:
		// compiled sources for each world
		CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, Spire::Compiler::CompiledShaderSource> Sources; 
		Spire::Compiler::ShaderMetaData MetaData;
		void AddSource(CoreLib::Basic::String source, CoreLib::Text::Parser & parser);
		void FromString(const CoreLib::String & str);
		CoreLib::String ToString();
		void SaveToFile(CoreLib::Basic::String fileName);
		ShaderLibFile() = default;
		void Clear();
		void Load(CoreLib::Basic::String fileName);
	};
}
```
An example of using Spire Compiler:
```c++
#include "Spire.h"

int main(int argc, char** args)
{
	if (argc > 1)
	{
		Spire::Compiler::CompileOptions options;
		Spire::Compiler::CompileResult result;
		auto compiledShaders = SpireLib::CompileShaderSource(result, args[1], options);
		if (compiledShaders.Count())
			compiledShaders.First().SaveToFile("d:\\test.cse");
	}
    return 0;
}
```
### As Stand-alone Compiler
Build "Source/Spire.sln" and use "SpireCompiler.exe". The command line format is:
```
SpireCompiler filename [-out output_filename] [-symbol shader_to_compile] [-schedule schedule_file] [-genchoice]
```
`filename` specifies the source filename.

Optionally, you can use:

`-out`: specifies the output filename.

`-symbol`: instructs the compiler to only generate code for the specified shader.

`-schedule`: instructs the compiler to apply the specified schedule file. A schedule file contains rate placement or algorithmic choice decisions for the given shader.

`-genchoice`: instructs the compiler to generate a choice file, which contains all the optimization options (including rate placement or algorithimic choices) exposed by the input shader.
