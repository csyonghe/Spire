#include "CoreLib/LibIO.h"
#include "SpireLib.h"
#include "D3DCompiler.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace Spire::Compiler;

// Try to read an argument for a command-line option.
String tryReadCommandLineArgument(wchar_t const* option, wchar_t***ioCursor, wchar_t**end)
{
	wchar_t**& cursor = *ioCursor;
	if (cursor == end)
	{
		fprintf(stderr, "expected an argument for command-line option '%S'", option);
		exit(1);
	}
	else
	{
		return String::FromWString(*cursor++);
	}
}

int wmain(int argc, wchar_t* argv[])
{
	int returnValue = -1;
	{
		// We need to parse any command-line arguments.
		String outputDir;
		CompileOptions options;

		// As we parse the command line, we will rewrite the
		// entries in `argv` to collect any "ordinary" arguments.
		wchar_t const** inputPaths = (wchar_t const**)&argv[1];
		wchar_t const** inputPathCursor = inputPaths;

		wchar_t** argCursor = &argv[1];
		wchar_t** argEnd = &argv[argc];
		while (argCursor != argEnd)
		{
			wchar_t const* arg = *argCursor++;
			if (arg[0] == '-')
			{
				String argStr = String::FromWString(arg);

				// The argument looks like an option, so try to parse it.
				if (argStr == "-out")
					outputDir = tryReadCommandLineArgument(arg, &argCursor, argEnd);
				else if (argStr == "-symbo")
					options.SymbolToCompile = tryReadCommandLineArgument(arg, &argCursor, argEnd);
				else if (argStr == "-schedule")
					options.ScheduleFileName = tryReadCommandLineArgument(arg, &argCursor, argEnd);
				else if (argStr == "-backend")
				{
					String name = tryReadCommandLineArgument(arg, &argCursor, argEnd);
					if (name == "glsl")
					{
						options.Target = CodeGenTarget::GLSL;
					}
					else if (name == "glsl_vk")
					{
						options.Target = CodeGenTarget::GLSL_Vulkan;
					}
					else if (name == "glsl_vk_onedesc")
					{
						options.Target = CodeGenTarget::GLSL_Vulkan_OneDesc;
					}
					else if (name == "hlsl")
					{
						options.Target = CodeGenTarget::HLSL;
					}
					else if (name == "spriv")
					{
						options.Target = CodeGenTarget::SPIRV;
					}
					else
					{
						fprintf(stderr, "unknown code generation target '%S'\n", name.ToWString());
					}
				}
				else if (argStr == "-genchoice")
					options.Mode = CompilerMode::GenerateChoice;
				else if (argStr == "--")
				{
					// The `--` option causes us to stop trying to parse options,
					// and treat the rest of the command line as input file names:
					while (argCursor != argEnd)
					{
						*inputPathCursor++ = *argCursor++;
					}
					break;
				}
				else
				{
					fprintf(stderr, "unknown command-line option '%S'\n", argStr.ToWString());
					// TODO: print a usage message
					exit(1);
				}
			}
			else
			{
				*inputPathCursor++ = arg;
			}
		}

		int inputPathCount = (int)(inputPathCursor - inputPaths);
		if (inputPathCount == 0)
		{
			fprintf(stderr, "error: no input file specified\n");
			exit(1);
		}
		else if (inputPathCount > 1)
		{
			fprintf(stderr, "error: multiple input files specified\n");
			exit(1);
		}

		String fileName = String::FromWString(inputPaths[0]);

		// Output directory defaults to the path of the input file
		if (outputDir.Length() == 0)
		{
			outputDir = Path::GetDirectoryName(fileName);
		}

		auto sourceDir = Path::GetDirectoryName(fileName);
		String schedule;
		if (options.ScheduleFileName.Length())
		{
			try
			{
				schedule = File::ReadAllText(options.ScheduleFileName);
				options.ScheduleSource = schedule;
			}
			catch (IOException)
			{
				printf("Cannot open schedule file '%S'.\n", options.ScheduleFileName.ToWString());
				goto end;
			}
		}
		CompileResult result;
		try
		{
			auto files = SpireLib::CompileShaderSourceFromFile(result, fileName, options);
			for (auto & f : files)
			{
				
				try
				{
					f.SaveToFile(Path::Combine(outputDir, f.MetaData.ShaderName + ".cse"));
				}
				catch (Exception &)
				{
					result.GetErrorWriter()->diagnose(CodePosition(0, 0, 0, ""), Diagnostics::cannotWriteOutputFile, Path::Combine(outputDir, f.MetaData.ShaderName + ".cse"));
				}
			}

			if (options.Target == CodeGenTarget::HLSL)
			{
				// verify shader using D3DCompileShaderFromFile
				RefPtr<D3DCompiler> d3dCompiler = LoadD3DCompiler();
				if (d3dCompiler)
				{
					for (auto & f : files)
					{
						for (auto & stage : f.Sources)
						{
							String errMsg;
							d3dCompiler->Compile(stage.Value.MainCode, stage.Key, errMsg);
							if (errMsg.Length())
							{
								auto dumpFileName = f.MetaData.ShaderName + "." + stage.Key + ".hlsl";
								result.GetErrorWriter()->diagnose(CodePosition(0, 0, 0, dumpFileName), Diagnostics::d3dCompileInfo, errMsg);
								File::WriteAllText(Path::Combine(Path::GetDirectoryName(fileName), dumpFileName), stage.Value.MainCode);
							}
						}
					}
				}
				else
				{
					printf("failed to load d3d compiler for verification.\n");
				}
			}
		}
		catch (Exception & e)
		{
			printf("internal compiler error: %S\n", e.Message.ToWString());
		}
		result.PrintDiagnostics();
		if (result.GetErrorCount() == 0)
			returnValue = 0;
		
	}
end:;
#ifdef _MSC_VER
	_CrtDumpMemoryLeaks();
#endif
	return returnValue;
}