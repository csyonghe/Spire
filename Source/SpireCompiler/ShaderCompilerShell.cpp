#include "CoreLib/LibIO.h"
#include "SpireLib.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace Spire::Compiler;

int wmain(int argc, wchar_t* argv[])
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
			return -1;
		}
	}
	CompileResult result;
	auto files = SpireLib::CompileShaderSource(result, fileName, options);
	for (auto & f : files)
	{
		if (f.BinaryFileName.Length())
		{
			if (!f.ProduceBinary(outputDir))
				result.GetErrorWriter()->Error(3, L"cannot compile generated c++ source '" + f.BinarySourceName + L"'.",
					CodePosition(0, 0, L""));
		}
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
	result.PrintError(true);
	if (result.Success)
		return 0;
	return 1;
}

