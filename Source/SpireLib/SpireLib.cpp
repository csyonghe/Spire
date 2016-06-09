#include "SpireLib.h"
#include "../CoreLib/LibIO.h"
#include "../CoreLib/Parser.h"
#include "../SpireCore/StdInclude.h"
#include "ImportOperator.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace CoreLib::Text;
using namespace Spire::Compiler;

namespace SpireLib
{
	void ReadSource(EnumerableDictionary<CoreLib::Basic::String, CompiledShaderSource> & sources, CoreLib::Text::Parser & parser, String src)
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
			CompiledShaderSource compiledSrc;
			compiledSrc.ParseFromGLSL(getShaderSource());
			sources[worldName] = compiledSrc;
		}
	}
	CompiledShaderSource ShaderLib::GetWorldSource(String world)
	{
		CompiledShaderSource rs;
		Sources.TryGetValue(world, rs);
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
		List<ImportOperatorHandler*> importHandlers;
		List<ExportOperatorHandler*> exportHandlers;
		CreateGLSLImportOperatorHandlers(importHandlers);
		CreateGLSLExportOperatorHandlers(exportHandlers);
		for (auto handler : exportHandlers)
			compiler->RegisterExportOperator(L"glsl", handler);
		for (auto handler : importHandlers)
			compiler->RegisterImportOperator(L"glsl", handler);
		try
		{
			if (compileResult.ErrorList.Count() == 0)
				compiler->Compile(compileResult, units, options);
			DestroyImportOperatorHanlders(importHandlers);
			DestroyExportOperatorHanlders(exportHandlers);
		}
		catch (...)
		{
			DestroyImportOperatorHanlders(importHandlers);
			DestroyExportOperatorHanlders(exportHandlers);
			throw;
		}
		if (compileResult.Success)
		{
			if (options.Mode == CompilerMode::ProduceShader)
			{
				EnumerableDictionary<String, ShaderLibFile> shaderLibs;
				for (auto file : compileResult.CompiledSource)
				{
					auto shaderName = Path::GetFileNameWithoutEXT(file.Key);
					ShaderLibFile * libFile = shaderLibs.TryGetValue(shaderName);
					if (!libFile)
					{
						shaderLibs.Add(shaderName, ShaderLibFile());
						libFile = shaderLibs.TryGetValue(shaderName);
						libFile->MetaData.ShaderName = shaderName;
					}
					libFile->Sources = file.Value;
				}
				for (auto & libFile : shaderLibs)
				{
					for (auto & shader : compileResult.Program->Shaders)
					{
						if (shader->MetaData.ShaderName == libFile.Key)
						{
							// fill in meta data
							libFile.Value.MetaData = shader->MetaData;
						}
					}
					resultFiles.Add(libFile.Value);
				}
			}
		}
		return resultFiles;
	}

	List<ShaderLibFile> CompileShaderSource(Spire::Compiler::CompileResult & compileResult,
		const CoreLib::String & src, Spire::Compiler::CompileOptions & options)
	{
		Spire::Compiler::NamingCounter = 0;
		RefPtr<ShaderCompiler> compiler = CreateShaderCompiler();
		List<CompileUnit> units;
		HashSet<String> processedUnits;
		List<String> unitsToInclude;
		unitsToInclude.Add(L"");
		processedUnits.Add(L"");
		auto predefUnit = compiler->Parse(compileResult, LibIncludeString, L"stdlib");
		for (int i = 0; i < unitsToInclude.Count(); i++)
		{
			auto inputFileName = unitsToInclude[i];
			try
			{
				String source = src;
				if (i > 0)
					source = File::ReadAllText(inputFileName);
				auto unit = compiler->Parse(compileResult, source, Path::GetFileName(inputFileName));
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
				compileResult.GetErrorWriter()->Error(1, L"cannot open file '" + Path::GetFileName(inputFileName) + L"'.", CodePosition(0, 0, L""));
			}
		}
		units.Add(predefUnit);
		return CompileUnits(compileResult, compiler.Ptr(), units, options);
	}

	List<ShaderLibFile> CompileShaderSourceFromFile(Spire::Compiler::CompileResult & compileResult, 
		CoreLib::Basic::String sourceFileName,
		Spire::Compiler::CompileOptions & options)
	{
		Spire::Compiler::NamingCounter = 0;
		RefPtr<ShaderCompiler> compiler = CreateShaderCompiler();
		List<CompileUnit> units;
		HashSet<String> processedUnits;
		List<String> unitsToInclude;
		unitsToInclude.Add(sourceFileName);
		processedUnits.Add(sourceFileName);
		auto predefUnit = compiler->Parse(compileResult, LibIncludeString, L"stdlib");
		for (int i = 0; i < unitsToInclude.Count(); i++)
		{
			auto inputFileName = unitsToInclude[i];
			try
			{
				String source = File::ReadAllText(inputFileName);
				auto unit = compiler->Parse(compileResult, source, Path::GetFileName(inputFileName));
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
				compileResult.GetErrorWriter()->Error(1, L"cannot open file '" + Path::GetFileName(inputFileName) + L"'.", CodePosition(0, 0, sourceFileName));
			}
		}
		units.Add(predefUnit);
		return CompileUnits(compileResult, compiler.Ptr(), units, options);
	}
	void ShaderLibFile::AddSource(CoreLib::Basic::String source, CoreLib::Text::Parser & parser)
	{
		ReadSource(Sources, parser, source);
	}

	CoreLib::String ShaderLibFile::ToString()
	{
		StringBuilder writer;
		writer << L"name " << MetaData.ShaderName << EndLine;
		for (auto & world : MetaData.Worlds)
		{
			writer << L"world " << world.Key << EndLine << L"{" << EndLine;
			writer << L"target " << world.Value.TargetName << EndLine;
			for (auto & blk : world.Value.InputBlocks)
			{
				writer << L"in " << blk << L";\n";
			}
			writer << L"out " << world.Value.OutputBlock << L";\n";
			for (auto & comp : world.Value.Components)
				writer << L"comp " << comp << L";\n";
			writer << L"}" << EndLine;
		}
		for (auto & ublock : MetaData.InterfaceBlocks)
		{
			writer << L"interface " << ublock.Key << L" size " << ublock.Value.Size << L"\n{\n";
			for (auto & entry : ublock.Value.Entries)
			{
				writer << ILBaseTypeToString(entry.Type) << L" " << entry.Name << L" : " << entry.Offset << L"," << entry.Size;
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
			writer << L"{" << EndLine;
			writer << src.Value.GetAllCodeGLSL() << EndLine;
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
		MetaData.Worlds.Clear();
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
			else if (fieldName == L"binary")
			{
			}
			else if (fieldName == L"world")
			{
				WorldMetaData world;
				world.Name = parser.ReadWord();
				parser.Read(L"{");
				while (!parser.LookAhead(L"}"))
				{
					auto readAttribs = [&](InterfaceMetaData & comp)
					{
						parser.Read(L"{");
						while (!parser.LookAhead(L"}"))
						{
							auto name = parser.ReadWord();
							parser.Read(L":");
							auto value = parser.ReadStringLiteral();
							comp.Attributes[name] = parser.UnescapeStringLiteral(value);
						}
						parser.Read(L"}");
					};
					auto subFieldName = parser.ReadWord();
					if (subFieldName == L"target")
						world.TargetName = parser.ReadWord();
					else if (subFieldName == L"in")
					{
						world.InputBlocks.Add(parser.ReadWord());
						parser.Read(L";");
					}
					else if (subFieldName == L"out")
					{
						world.OutputBlock = parser.ReadWord();
						parser.Read(L";");
					}
					else if (subFieldName == L"comp")
					{
						auto compName = parser.ReadWord();
						parser.Read(L";");
						world.Components.Add(compName);
					}
				}
				parser.Read(L"}");
				MetaData.Worlds[world.Name] = world;
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
					entry.Type = ILBaseTypeFromString(parser.ReadWord());
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