# Integrating Spire in Your Engine
If you have read the [Understanding Spire](https://github.com/csyonghe/Spire/tree/master/Docs/tutorial1) tutorial and want to integrate Spire into your engine, 
follow this guide to learn the compiler interface.
## As Stand-alone Compiler
The simplest way to use Spire is to use it as a stand-alone Compiler. Build "Source/Spire.sln" and use "SpireCompiler.exe". The command line format is:
```
> SpireCompiler.exe filename [-out output_filename] [-symbol shader_to_compile] [-schedule schedule_file] [-genchoice]
```
`filename` specifies the source filename.

Optionally, you can use:

`-out`: specifies the output filename. <br/>
`-symbol`: instructs the compiler to only generate code for the specified shader.<br/>
`-schedule`: instructs the compiler to apply the specified schedule file. A schedule file contains rate placement or algorithmic choice decisions for the given shader.<br/>
`-genchoice`: instructs the compiler to generate a choice file, which contains all the optimization options (including rate placement or algorithimic choices) exposed by the input shader.

SpireCompiler emits the compiled shader in parsable text format. The output file contains GPU shaders for each pipeline defined world, as well as 
metadata that describes the binding interface and components being computed at each world.

## As Library
To integrate Spire more closely to your engine or to extend spire compiler, you can use it as a library. 
The Spire compiler library is platform-independent and self-contained. The library is distributed as four C++ source files (yes, that is all you need!)
 with no additional dependencies. 
To integrate Spire into your engine, simply grab "Spire.h" "Spire.cpp" "Basic.h" "Basic.cpp" from "LibraryRelease" directory and place them into your project. 
To use Spire, include "Spire.h" in your source file.

###Compiling Spire Shaders
To invoke the compiler, call:
```c++
CoreLib::List<SpireLib::ShaderLibFile> CompileShaderSourceFromFile(Spire::Compiler::CompileResult & result,
	CoreLib::String sourceFileName,
	Spire::Compiler::CompileOptions &options);
```
Compiles shader from given filename. Each compiled shader correspond to one `SpireLib::ShaderLibFile` object in the returned the list.
Alternatively,
```c++
CoreLib::List<SpireLib::ShaderLibFile> CompileShaderSource(Spire::Compiler::CompileResult & result,
	const CoreLib::String &source,
	Spire::Compiler::CompileOptions &options);
```
compiles shader from given source string. `sourceFileName` argument can be any identifier.
You can then access the compiled shader code for each world using the returned `ShaderLibFile` objects. Each `ShaderLibFile` object correspond to
the compiled shader code for a `shader` in Spire source code. The following is the definition of `ShaderLibFile`.
For example, 
```c++
Spire::Compiler::CompileResult result;
Spire::Compiler::CompileOptions options;
auto compiledShaders = CompileShaderSourceFromFile(result, "Docs/tutorial1/Demo1.shader", options);
```
compiles [Docs/tutorial1/Demo1.shader](https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/Demo1.shader). 
`compiledShaders` will contain `ShaderLibFile` object, representing the compiled code of `Demo1Shader`.

To retrieve compiled GLSL source from the compiled shader, use `ShaderLibFile::Sources` member:
```c++
CoreLib::String vsSrc = compiledShaders[0].Sources["vs"];
```
`ShaderLibFile::MetaData` contains the metadata about the compiled shader, such a list of components and interface block definitions 
for each world.

###Enumerating Shader choices
To enumerate possible placement and algorithmic choices exposed in a given Spire shader, call one of `CompileShaderSourceFromFile` or
`CompileShaderSource` with `CompilerOptions::Mode` set to `Spire::Compiler::CompileMode::GenerateChoice`, as following:
```c++
CompileOptions options;
CompileResult result;
options.Mode = Spire::Compiler::CompilerMode::GenerateChoice;
SpireLib::CompileShaderSourceFromFile(result, "Docs/tutorial1/Demo1.shader", options);

// get all choices in this List:
CoreLib::List<ShaderChoice> choices = result.Choices;
```
The compiler function will return the set of choices in `result.Choices` as a `List` of `ShaderChoice`.
The definition of `ShaderChoice` is as follows:
```c++
class ShaderChoice
{
public:
	CoreLib::String ChoiceName;   // the name (identifier) of this choice, used in schedule file.
	CoreLib::String DefaultValue; // the default choice value
	CoreLib::List<ShaderChoiceValue> Options; // valid options for this choice.
};

class ShaderChoiceValue
{
public:
	CoreLib::String WorldName;       // the world name of this choice
	CoreLib::String AlternateName;   // the variant name of this choice
};
```
For example, the `ShaderChoice` object for `specular` component in [Docs/tutorial1/Demo2.shader](https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/Demo2.shader)
will be:
```c++
ShaderChoice
{
	ChoiceName = "specular"
	DefaultValue = "fs:ggx"
	Options = []
	{
		ShaderChoiceValue
		{
			WorldName = "vs"
			AlternateName = "ggx"
		},
		ShaderChoiceValue
		{
			WorldName = "fs"
			AlternateName = "ggx"
		},
		ShaderChoiceValue
		{
			WorldName = "vs"
			AlternateName = "phong"
		},
		ShaderChoiceValue
		{
			WorldName = "fs"
			AlternateName = "phong"
		},
		...
	}
}
```

###Compile Shader with Specific Choices

You can compile a shader using a specified schedule string that contains a set of choice values. To do so, call one of `CompileShaderSourceFromFile` or
`CompileShaderSource` with `CompilerOptions::ScheduleSource` set a string encoding all choices, as in the following example code:
```c++
CompileOptions options;
CompileResult result;
CoreLib::String scheduleSrc = GenerateChoice(...);
options.ScheduleSource = schedulSrc;
SpireLib::CompileShaderSourceFromFile(result, "Docs/tutorial1/Demo1.shader", options);
```
An important step here is to encode the choice values in a schedule string. The schedule string has the following syntax:
```
ChoiceName1 = ChoiceValue0[, ChoiceValue1, ..., ChoiceValueN];
ChoiceName2 = ChoiceValue0[, ChoiceValue1, ..., ChoiceValueN];
...
```
`ChoiceName` is the name (identifier) of the choice, and `ChoiceValue`s specify the component overloads you wish to include in the final shader.
For example, if you want the `ggx` variant of `specular` component to be computed at both `vs` and `fs` world, specify the following in the schedule string:
```
specular = vs:ggx, fs:ggx;
```

