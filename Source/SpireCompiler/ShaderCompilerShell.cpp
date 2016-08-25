#include "CoreLib/LibIO.h"
#include "SpireLib.h"
#include <fstream>

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
	auto files = SpireLib::CompileShaderSourceFromFile(result, fileName, options);

	//__DEBUG__
	//IL output
	{
		if (result.ErrorList.Count() != 0) {
			printf("Error");
			result.PrintError(true);
		}
		//print IL 
		auto compiledProgram = result.Program.Ptr();
		StringBuilder sb;

		//function part
		sb << L"function" << EndLine;
		sb << L"{" << EndLine;
		for (auto &pfunc : compiledProgram->Functions) {
			sb << pfunc->ReturnType->ToString() << L" " << pfunc->Name << L"(";
			bool first = true;
			for (auto &name2param : pfunc->Parameters) {
				if (!first)
					sb << L", ";
				sb << name2param.Value->ToString() << L" " << name2param.Key;
				first = false;
			}
			sb << L")" << EndLine;
			sb << L"{" << EndLine;
			pfunc->Code->NameAllInstructions();
			sb << pfunc->Code->ToString() << EndLine;
			sb << L"}" << EndLine;
		}
		sb << L"}" << EndLine;

		//struct part
		//TODO

		//shader part
		for (auto &pshader : compiledProgram->Shaders) {
			sb << L"Shader " << pshader->MetaData.ShaderName << EndLine;
			sb << L"{" << EndLine;
			for (auto &pworld : pshader->Worlds) {
				sb << L"World " << pworld.Key << EndLine;
				sb << L"{" << EndLine;
				pworld.Value.Ptr()->Code->NameAllInstructions();
				sb << pworld.Value.Ptr()->Code->ToString() << EndLine;
				sb << L"}" << EndLine;
			}
			sb << L"}" << EndLine;
		}

		StringBuilder sb_indent;
		IndentString(sb_indent, sb.ToString());
		StreamWriter sw(String(L"IL.out"));
		sw.Write(sb_indent.ToString());
	}

	//__DEBUG__
	//output shaders in seperate files
	{
		for (auto &f : files)
		{
			String prefix = Path::Combine(outputDir, f.MetaData.ShaderName);
			for (auto & src : f.Sources) {
				StreamWriter fwriter(prefix + "." + src.Key);
				if (src.Value.OutputDeclarations != L"spirv")
					fwriter << src.Value.GetAllCodeGLSL() << EndLine;
				else
					fwriter << src.Value.MainCode << EndLine;
				fwriter.Close();
			}
		}
	}

	//__DEBUG__
	//output SPIRV shaders in binary form
	{
		for (auto &f : result.CompiledSource)
		{
			for (auto &w : f.Value)
			{
				std::ofstream bs(String(w.Key + L".spv").ToMultiByteString(), std::ios::out | std::ios::binary);
				for (auto &b : w.Value.BinaryCode)
				{
					bs.write(reinterpret_cast<const char*>(&b),1);
				}
				bs.close();
			}
		}
	}

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
	result.PrintError(true);
	if (result.Success)
		return 0;
	return 1;
}

