#include "CoreLib/LibIO.h"
#include "SpireLib.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace Spire::Compiler;

// Try to read an argument for a command-line option.
wchar_t const* tryReadCommandLineArgumentRaw(wchar_t const* option, wchar_t***ioCursor, wchar_t**end)
{
    wchar_t**& cursor = *ioCursor;
    if (cursor == end)
    {
        fprintf(stderr, "expected an argument for command-line option '%S'", option);
        exit(1);
    }
    else
    {
        return *cursor++;
    }
}

String tryReadCommandLineArgument(wchar_t const* option, wchar_t***ioCursor, wchar_t**end)
{
    return String::FromWString(tryReadCommandLineArgumentRaw(option, ioCursor, end));
}

Profile TranslateProfileName(char const* name)
{
#define PROFILE(TAG, NAME, STAGE, VERSION)	if(strcmp(name, #NAME) == 0) return Profile::TAG;
#define PROFILE_ALIAS(TAG, NAME)			if(strcmp(name, #NAME) == 0) return Profile::TAG;
#include "SpireCore/ProfileDefs.h"

    return Profile::Unknown;
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
                else if (argStr == "-backend" || argStr == "-target")
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
                    else if (name == "dxbc")
                    {
                        options.Target = CodeGenTarget::DXBytecode;
                    }
                    else if (name == "dxbc-assembly")
                    {
                        options.Target = CodeGenTarget::DXBytecodeAssembly;
                    }
                    else
                    {
                        fprintf(stderr, "unknown code generation target '%S'\n", name.ToWString());
                    }
                }
                // A "profile" specifies both a specific target stage and a general level
                // of capability required by the program.
                else if (argStr == "-profile")
                {
                    String name = tryReadCommandLineArgument(arg, &argCursor, argEnd);

                    Profile profile = TranslateProfileName(name.begin());
                    if( profile.raw == Profile::Unknown )
                    {
                        fprintf(stderr, "unknown profile '%s'\n", name);
                    }
                    else
                    {
                        options.profile = profile;
                    }
                }
                else if (argStr == "-entry")
                {
                    String name = tryReadCommandLineArgument(arg, &argCursor, argEnd);

                    EntryPointOption entry;
                    entry.name = name;

                    // TODO(tfoley): Allow user to fold a specification of a profile into the entry-point name,
                    // for the case where they might be compiling multiple entry points in one invocation...
                    //
                    // For now, just use the last profile set on the command-line to specify this

                    entry.profile = options.profile;

                    options.entryPoints.Add(entry);
                }
                else if (argStr == "-stage")
                {
                    String name = tryReadCommandLineArgument(arg, &argCursor, argEnd);
                    StageTarget stage = StageTarget::Unknown;
                    if (name == "vertex") { stage = StageTarget::VertexShader; }
                    else if (name == "fragment") { stage = StageTarget::FragmentShader; }
                    else if (name == "hull") { stage = StageTarget::HullShader; }
                    else if (name == "domain") { stage = StageTarget::DomainShader; }
                    else if (name == "compute") { stage = StageTarget::ComputeShader; }
                    else
                    {
                        fprintf(stderr, "unknown stage '%S'\n", name.ToWString());
                    }
                    options.stage = stage;
                }
                else if (argStr == "-pass-through")
                {
                    String name = tryReadCommandLineArgument(arg, &argCursor, argEnd);
                    PassThroughMode passThrough = PassThroughMode::None;
                    if (name == "fxc") { passThrough = PassThroughMode::HLSL; }
                    else
                    {
                        fprintf(stderr, "unknown pass-through target '%S'\n", name.ToWString());
                    }
                    options.passThrough = passThrough;
                }
                else if (argStr == "-genchoice")
                    options.Mode = CompilerMode::GenerateChoice;
                else if (argStr[1] == 'D')
                {
                    // The value to be defined might be part of the same option, as in:
                    //     -DFOO
                    // or it might come separately, as in:
                    //     -D FOO
                    wchar_t const* defineStr = arg + 2;
                    if (defineStr[0] == 0)
                    {
                        // Need to read another argument from the command line
                        defineStr = tryReadCommandLineArgumentRaw(arg, &argCursor, argEnd);
                    }
                    // The string that sets up the define can have an `=` between
                    // the name to be defined and its value, so we search for one.
                    wchar_t const* eqPos = nullptr;
                    for(wchar_t const* dd = defineStr; *dd; ++dd)
                    {
                        if (*dd == '=')
                        {
                            eqPos = dd;
                            break;
                        }
                    }

                    // Now set the preprocessor define
                    //
                    if (eqPos)
                    {
                        // If we found an `=`, we split the string...
                        options.PreprocessorDefinitions[String::FromWString(defineStr, eqPos)] = String::FromWString(eqPos+1);
                    }
                    else
                    {
                        // If there was no `=`, then just #define it to an empty string
                        options.PreprocessorDefinitions[String::FromWString(defineStr)] = String();
                    }
                }
                else if (argStr[1] == 'I')
                {
                    // The value to be defined might be part of the same option, as in:
                    //     -IFOO
                    // or it might come separately, as in:
                    //     -I FOO
                    // (see handling of `-D` above)
                    wchar_t const* includeDirStr = arg + 2;
                    if (includeDirStr[0] == 0)
                    {
                        // Need to read another argument from the command line
                        includeDirStr = tryReadCommandLineArgumentRaw(arg, &argCursor, argEnd);
                    }

                    options.SearchDirectories.Add(String::FromWString(includeDirStr));
                }
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

        // For any entry points that were given without an explicit profile, we can now apply
        // the profile that was given to them.
        if( options.entryPoints.Count() != 0 )
        {
            if( options.profile.raw == Profile::Unknown )
            {
                fprintf(stderr, "error: no profile specified; use the '-profile <profile name>' option");
                exit(1);
            }

            for( auto& e : options.entryPoints )
            {
                if( e.profile.raw == Profile::Unknown )
                {
                    e.profile = options.profile;
                }
            }
        }

        //

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