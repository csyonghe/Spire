# Spire
Spire is a shading language and compiler framework that facilitates rapid exploration of shader optimization choices (such as frequency reduction and algorithmic approximation) afforded by modern real-time graphics engines.

Paper: http://graphics.cs.cmu.edu/projects/spire/

[Tutorial: Understanding Spire](https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1)

## Getting Started
To experience the benefit of using Spire as your engine's shading language, we have build a rendering engine that supports offline  texture prebaking, object space shading, screen space multi-resolution shading and shadow map generation. The engine uses the Spire compiler to generate GPU compute, vertex and fragment shaders for all these rendering stages. To see it working, compile the demo engine from `"Examples/DemoEngine/DemoEngine.sln"` and run the `SceneViewer` project. In `SceneViewer`, select "File"->"Open", and open "Examples/Scenes/couch/couch.world". This loads up a couch scene. The couch is rendered using [Examples/Scenes/couch/couch.shader](https://github.com/csyonghe/Spire/blob/master/Examples/Scenes/couch/couch.shader).

![](https://github.com/csyonghe/Spire/blob/master/Docs/sceneViewer.jpg)

The Choice Control window allows you to dynamically recompile the modified shader and explore different rate placement choices in real-time.

Currently the demo engine runs only on Windows with an NVIDIA Kepler or later GPU. AMD and Intel GPUs are not supported. 
However, the compiler is platform independent and is compatible with both msvc and g++ 5.0 (with `-fpermissive -std=c++14 -msse2` flag). Spire currently supports generating GLSL compute, vertex and fragment shaders. 

## Understanding Spire
<a href="https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1">Read this tutorial</a> to learn the basics of Spire.

## Using Spire
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
