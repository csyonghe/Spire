#include "CoreLib/LibIO.h"
#include "SpireLib.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace Spire::Compiler;

int wmain(int argc, wchar_t* argv[])
{
	int returnValue = -1;
	{
		String fileName = argv[1];
		String outputDir = Path::GetDirectoryName(fileName);
		CompileOptions options;
		for (int i = 2; i < argc; i++)
		{
			if (i < argc - 1)
			{
				if (String(argv[i]) == L"-out")
					outputDir = argv[i + 1];
				else if (String(argv[i]) == L"-symbol")
					options.SymbolToCompile = argv[i + 1];
				else if (String(argv[i]) == L"-schedule")
					options.ScheduleFileName = argv[i + 1];
				else if (String(argv[i]) == L"-backend")
					options.Target = (String(argv[i + 1]) == L"glsl") ? CodeGenTarget::GLSL : CodeGenTarget::SPIRV;
			}
			if (String(argv[i]) == L"-genchoice")
				options.Mode = CompilerMode::GenerateChoice;
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

