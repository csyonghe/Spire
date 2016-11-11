#include "CoreLib/LibIO.h"
#include "SpireLib.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace Spire::Compiler;

// Try to read an argument for a command-line option.
wchar_t const* tryReadCommandLineArgument(wchar_t const* option, wchar_t***ioCursor, wchar_t**end)
{
	wchar_t**& cursor = *ioCursor;
	if (cursor == end)
	{
		fprintf(stderr, "expected an argument for command-line option '%s'", String(option).ToMultiByteString());
		exit(1);
	}
	else
	{
		return *cursor++;
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
			if (arg[0] == L'-')
			{
				String argStr(arg);

				// The argument looks like an option, so try to parse it.
				if (argStr == L"-out")
					outputDir = tryReadCommandLineArgument(arg, &argCursor, argEnd);
				else if (argStr == L"-symbol")
					options.SymbolToCompile = tryReadCommandLineArgument(arg, &argCursor, argEnd);
				else if (argStr == L"-schedule")
					options.ScheduleFileName = tryReadCommandLineArgument(arg, &argCursor, argEnd);
				else if (argStr == L"-backend")
				{
					String name = tryReadCommandLineArgument(arg, &argCursor, argEnd);
					if (name == "glsl")
					{
						options.Target = CodeGenTarget::GLSL;
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
						fprintf(stderr, "unknown code generation target '%s'\n", name.ToMultiByteString());
					}
				}
				else if (argStr == L"-genchoice")
					options.Mode = CompilerMode::GenerateChoice;
				else if (argStr == L"--")
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
					fprintf(stderr, "unknown command-line option '%s'\n", argStr.ToMultiByteString());
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

		String fileName = inputPaths[0];

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
				printf("Cannot open schedule file '%s'.\n", options.ScheduleFileName.ToMultiByteString());
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
					f.SaveToFile(Path::Combine(outputDir, f.MetaData.ShaderName + L".cse"));
				}
				catch (Exception &)
				{
					result.GetErrorWriter()->Error(4, L"cannot write output file \'" + Path::Combine(outputDir, f.MetaData.ShaderName + L".cse") + L"\'.",
						CodePosition(0, 0, L""));
				}
			}
		}
		catch (Exception & e)
		{
			wprintf(L"internal compiler error: %s\n", e.Message.Buffer());
		}
		result.PrintError(true);
		if (result.Success)
			returnValue = 0;
	}
end:;
#ifdef _MSC_VER
	_CrtDumpMemoryLeaks();
#endif
	return returnValue;
}