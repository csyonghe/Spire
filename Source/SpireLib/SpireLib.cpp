#include "SpireLib.h"
#include "../CoreLib/LibIO.h"
#include "../CoreLib/Parser.h"
#include "../SpireCore/StdInclude.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace CoreLib::Text;
using namespace Spire::Compiler;

namespace SpireLib
{
	void ReadSource(EnumerableDictionary<String, StageSource> & sources, CoreLib::Text::Parser & parser, String src)
	{
		auto getShaderSource = [&]()
		{
			auto token = parser.ReadToken();
			int endPos = token.Position + 1;
			int brace = 0;
			while (endPos < src.Length() && !(src[endPos] == L'}' && brace == 0))
			{
				if (src[endPos] == L'{')
					brace++;
				else if (src[endPos] == L'}')
					brace--;
				endPos++;
			}
			while (!parser.IsEnd() && parser.NextToken().Position != endPos)
				parser.ReadToken();
			parser.ReadToken();
			return src.SubString(token.Position + 1, endPos - token.Position - 1);
		};
		while (!parser.IsEnd() && !parser.LookAhead(L"}"))
		{
			auto worldName = parser.ReadWord();
			StageSource compiledSrc;
			if (parser.LookAhead(L"binary"))
			{
				parser.ReadToken();
				parser.Read(L"{");
				while (!parser.LookAhead(L"}") && !parser.IsEnd())
				{
					auto val = parser.ReadUInt();
					compiledSrc.BinaryCode.AddRange((unsigned char*)&val, sizeof(unsigned int));
					if (parser.LookAhead(L","))
						parser.ReadToken();
				}
				parser.Read(L"}");
			}
			if (parser.LookAhead(L"text"))
			{
				parser.ReadToken();
				compiledSrc.MainCode = getShaderSource();
			}
			sources[worldName] = compiledSrc;
		}
	}
	StageSource ShaderLib::GetStageSource(String stage)
	{
		StageSource rs;
		Sources.TryGetValue(stage, rs);
		return rs;
	}
	ShaderLib::ShaderLib(CoreLib::Basic::String fileName)
	{
		Reload(fileName);
	}
	void ShaderLib::Reload(CoreLib::Basic::String fileName)
	{
		Load(fileName);
	}
	bool ShaderLib::CompileFrom(String symbolName, String sourceFileName, String schedule)
	{
		CompileResult result;
		CompileOptions options;
		options.ScheduleSource = schedule;
		options.SymbolToCompile = symbolName;
		options.Mode = CompilerMode::ProduceShader;
		auto shaderLibs = CompileShaderSourceFromFile(result, sourceFileName, options);
		if (result.Success)
		{
			for (auto & lib : shaderLibs)
			{
				if (lib.MetaData.ShaderName == symbolName)
				{
					FromString(shaderLibs[0].ToString());
					return true;
				}
			}
		}
		result.PrintError(true);
		return false;
	}

	List<ShaderLibFile> CompileUnits(Spire::Compiler::CompileResult & compileResult,
		ShaderCompiler * compiler, List<CompileUnit> & units,
		Spire::Compiler::CompileOptions & options)
	{
		List<ShaderLibFile> resultFiles;
		compiler->Compile(compileResult, units, options);
		if (compileResult.Success)
		{
			if (options.Mode == CompilerMode::ProduceShader)
			{
				EnumerableDictionary<String, ShaderLibFile> shaderLibs;
				for (auto file : compileResult.CompiledSource)
				{
					ShaderLibFile libFile;
					libFile.MetaData = file.Value.MetaData;
					libFile.Sources = file.Value.Stages;
					resultFiles.Add(libFile);
				}
			}
		}
		return resultFiles;
	}

	List<ShaderLibFile> CompileShaderSource(Spire::Compiler::CompileResult & compileResult,
		const CoreLib::String & src, const CoreLib::String & fileName, Spire::Compiler::CompileOptions & options)
	{
		Spire::Compiler::NamingCounter = 0;
		RefPtr<ShaderCompiler> compiler = CreateShaderCompiler();
		List<CompileUnit> units;
		HashSet<String> processedUnits;
		List<String> unitsToInclude;
		unitsToInclude.Add(fileName);
		processedUnits.Add(fileName);
		auto predefUnit = compiler->Parse(compileResult, LibIncludeString, L"stdlib");
		for (int i = 0; i < unitsToInclude.Count(); i++)
		{
			auto inputFileName = unitsToInclude[i];
			try
			{
				String source = src;
				if (i > 0)
					source = File::ReadAllText(inputFileName);
				auto unit = compiler->Parse(compileResult, source, inputFileName);
				units.Add(unit);
				if (unit.SyntaxNode)
				{
					for (auto inc : unit.SyntaxNode->Usings)
					{
						String includeFile = Path::Combine(Path::GetDirectoryName(inputFileName), inc.Content);
						if (processedUnits.Add(includeFile))
						{
							unitsToInclude.Add(includeFile);
						}
					}
				}
			}
			catch (IOException)
			{
				compileResult.GetErrorWriter()->Error(1, L"cannot open file '" + inputFileName + L"'.", CodePosition(0, 0, L""));
			}
		}
		units.Add(predefUnit);
		if (compileResult.ErrorList.Count() == 0)
			return CompileUnits(compileResult, compiler.Ptr(), units, options);
		else
			return List<ShaderLibFile>();
	}

	List<ShaderLibFile> CompileShaderSourceFromFile(Spire::Compiler::CompileResult & compileResult, 
		const CoreLib::Basic::String & sourceFileName,
		Spire::Compiler::CompileOptions & options)
	{
		try
		{
			return CompileShaderSource(compileResult, File::ReadAllText(sourceFileName), sourceFileName, options);
		}
		catch (IOException)
		{
			compileResult.GetErrorWriter()->Error(1, L"cannot open file '" + Path::GetFileName(sourceFileName) + L"'.", CodePosition(0, 0, L""));
		}
		return List<ShaderLibFile>();
	}
	void ShaderLibFile::AddSource(CoreLib::Basic::String source, CoreLib::Text::Parser & parser)
	{
		ReadSource(Sources, parser, source);
	}

	CoreLib::String ShaderLibFile::ToString()
	{
		StringBuilder writer;
		writer << L"name " << MetaData.ShaderName << EndLine;
		for (auto & stage : MetaData.Stages)
		{
			writer << L"stage " << stage.Key << EndLine << L"{" << EndLine;
			writer << L"target " << stage.Value.TargetName << EndLine;
			for (auto & blk : stage.Value.InputBlocks)
			{
				writer << L"in " << blk << L";\n";
			}
			writer << L"out " << stage.Value.OutputBlock << L";\n";
			for (auto & comp : stage.Value.Components)
				writer << L"comp " << comp << L";\n";
			writer << L"}" << EndLine;
		}
		for (auto & ublock : MetaData.InterfaceBlocks)
		{
			writer << L"interface " << ublock.Key << L" size " << ublock.Value.Size << L"\n{\n";
			for (auto & entry : ublock.Value.Entries)
			{
				writer << entry.Type->ToString() << L" " << entry.Name << L" : " << entry.Offset << L"," << entry.Size;
				if (entry.Attributes.Count())
				{
					writer << L"\n{\n";
					for (auto & attrib : entry.Attributes)
					{
						writer << attrib.Key << L" : " << CoreLib::Text::Parser::EscapeStringLiteral(attrib.Value) << L";\n";
					}
					writer << L"}";
				}
				writer << L";\n";
			}
			writer << L"}\n";
		}
		writer << L"source" << EndLine << L"{" << EndLine;
		for (auto & src : Sources)
		{
			writer << src.Key << EndLine;
			if (src.Value.BinaryCode.Count())
			{
				writer << L"binary" << EndLine << L"{" << EndLine;
				auto binaryBuffer = (unsigned int*)src.Value.BinaryCode.Buffer();
				for (int i = 0; i < src.Value.BinaryCode.Count() / 4; i++)
				{
					writer << String((long long)binaryBuffer[i]) << L",";
					if ((i+1) % 10)
						writer << EndLine;
				}
				writer << EndLine << L"}" << EndLine;
			}
			writer << L"text" << EndLine << L"{" << EndLine;
			writer << src.Value.MainCode << EndLine;

			writer << L"}" << EndLine;
		}
		writer << L"}" << EndLine;
		StringBuilder formatSB;
		IndentString(formatSB, writer.ProduceString());
		return formatSB.ProduceString();
	}
	
	void ShaderLibFile::Clear()
	{
		Sources.Clear();
		MetaData.Stages.Clear();
		Sources.Clear();
	}

	void ShaderLibFile::SaveToFile(CoreLib::Basic::String fileName)
	{
		StreamWriter fwriter(fileName);
		fwriter.Write(ToString());
	}

	void ShaderLibFile::FromString(const String & src)
	{
		Clear();
		CoreLib::Text::Parser parser(src);
		while (!parser.IsEnd())
		{
			auto fieldName = parser.ReadWord();
			if (fieldName == L"name")
			{
				MetaData.ShaderName = parser.ReadWord();
			}
			else if (fieldName == L"source")
			{
				parser.Read(L"{");
				ReadSource(Sources, parser, src);
				parser.Read(L"}");
			}
			
			else if (fieldName == L"stage")
			{
				StageMetaData stage;
				stage.Name = parser.ReadWord();
				parser.Read(L"{");
				while (!parser.LookAhead(L"}"))
				{
					auto subFieldName = parser.ReadWord();
					if (subFieldName == L"target")
						stage.TargetName = parser.ReadWord();
					else if (subFieldName == L"in")
					{
						stage.InputBlocks.Add(parser.ReadWord());
						parser.Read(L";");
					}
					else if (subFieldName == L"out")
					{
						stage.OutputBlock = parser.ReadWord();
						parser.Read(L";");
					}
					else if (subFieldName == L"comp")
					{
						auto compName = parser.ReadWord();
						parser.Read(L";");
						stage.Components.Add(compName);
					}
				}
				parser.Read(L"}");
				MetaData.Stages[stage.Name] = stage;
			}
			else if (fieldName == L"interface")
			{
				InterfaceBlockMetaData block;
				if (!parser.LookAhead(L"{") && !parser.LookAhead(L"size"))
					block.Name = parser.ReadWord();
				if (parser.LookAhead(L"size"))
				{
					parser.ReadWord();
					block.Size = parser.ReadInt();
				}
				parser.Read(L"{");
				while (!parser.LookAhead(L"}") && !parser.IsEnd())
				{
					InterfaceBlockEntry entry;
					entry.Type = TypeFromString(parser);
					entry.Name = parser.ReadWord();
					parser.Read(L":");
					entry.Offset = parser.ReadInt();
					parser.Read(L",");
					entry.Size = parser.ReadInt();
					if (parser.LookAhead(L"{"))
					{
						parser.Read(L"{");
						while (!parser.LookAhead(L"}") && !parser.IsEnd())
						{
							auto attribName = parser.ReadWord();
							parser.Read(L":");
							auto attribValue = parser.ReadStringLiteral();
							parser.Read(L";");
							entry.Attributes[attribName] = attribValue;
						}
						parser.Read(L"}");
					}
					parser.Read(L";");
					block.Entries.Add(entry);
				}
				parser.Read(L"}");
				MetaData.InterfaceBlocks[block.Name] = block;
			}
		}
	}

	void ShaderLibFile::Load(String fileName)
	{
		String src = File::ReadAllText(fileName);
		FromString(src);
	}
}