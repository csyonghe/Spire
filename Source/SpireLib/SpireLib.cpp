#include "SpireLib.h"
#include "../CoreLib/LibIO.h"
#include "../CoreLib/Parser.h"
#include "../SpireCore/StdInclude.h"
#include "include/Spire.h"
#include "../SpireCore/TypeTranslation.h"

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
		Spire::Compiler::CompileResult result;
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
		auto searchDirs = options.SearchDirectories;
		searchDirs.Add(Path::GetDirectoryName(fileName));
		searchDirs.Reverse();
		auto predefUnit = compiler->Parse(compileResult, SpireStdLib::GetCode(), L"stdlib");
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
						bool found = false;
						for (auto & dir : searchDirs)
						{
							String includeFile = Path::Combine(dir, inc.Content);
							if (File::Exists(includeFile))
							{
								if (processedUnits.Add(includeFile))
								{
									unitsToInclude.Add(includeFile);
								}
								found = true;
								break;
							}
						}
						if (!found)
						{
							compileResult.GetErrorWriter()->Error(2, L"cannot find file '" + inputFileName + L"'.", inc.Position);
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
					if ((i + 1) % 10)
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




	class Shader
	{
		friend class CompilationContext;
	private:
		bool isShader = false;
		String targetPipeline, shaderName;
		List<String> usings;
	public:
		Shader(String name, bool pIsShader)
		{
			shaderName = name;
			isShader = pIsShader;
		}
		void TargetPipeline(CoreLib::String pipelineName)
		{
			targetPipeline = pipelineName;
		}
		void UseModule(CoreLib::String moduleName)
		{
			usings.Add(moduleName);
		}
		String GetName() const
		{
			return shaderName;
		}
		String GetSource() const
		{
			StringBuilder codeBuilder;
			codeBuilder << L"shader " << shaderName;
			if (targetPipeline.Length())
				codeBuilder << L":" << targetPipeline;
			codeBuilder << L"\n{\n";
			for (auto & m : usings)
				codeBuilder << L"using " << m << L";\n";
			codeBuilder << L"\n}\n";
			return codeBuilder.ToString();
		}
	};

	class CompileResult
	{
	public:
		bool Success = false;
		CoreLib::List<CompileError> Errors, Warnings;
		CoreLib::EnumerableDictionary<String, CompiledShaderSource> Sources;
	};

	class ComponentMetaData
	{
	public:
		RefPtr<ILType> Type;
		String TypeName;
		String Register;
		String Name;
		int Offset = 0;
		int GetHashCode()
		{
			return Name.GetHashCode();
		}
		bool operator == (const ComponentMetaData & other)
		{
			return Name == other.Name;
		}
	};

	class ModuleMetaData
	{
	public:
		EnumerableDictionary<String, EnumerableHashSet<ComponentMetaData>> ComponentsByWorld;
		EnumerableHashSet<ComponentMetaData> Requirements;
	};
	
	class CompilationContext
	{
	private:
		bool useCache = false;
		CoreLib::String cacheDir;
		List<CompileUnit> moduleUnits;
		RefPtr<Spire::Compiler::CompilationContext> compileContext;
		HashSet<String> processedModuleUnits;
		RefPtr<ShaderCompiler> compiler;
		RefPtr<ProgramSyntaxNode> programToCompile;
		CompileResult compileResult;
		EnumerableDictionary<String, ModuleMetaData> modules;
	public:
		CompileOptions Options;

		CompilationContext(bool /*pUseCache*/, CoreLib::String /*pCacheDir*/)
		{
			compiler = CreateShaderCompiler();
			compileContext = new Spire::Compiler::CompilationContext();
			LoadModuleSource(SpireStdLib::GetCode(), L"stdlib");
		}

		~CompilationContext()
		{
			SpireStdLib::Finalize();
		}

		ModuleMetaData * FindModule(CoreLib::String moduleName)
		{
			return modules.TryGetValue(moduleName);
		}

		void UpdateModuleLibrary(List<CompileUnit> & units)
		{
			Spire::Compiler::CompileResult result;
			compiler->Compile(result, *compileContext, units, Options);
			compileResult.Errors = _Move(result.ErrorList);
			compileResult.Warnings = _Move(result.WarningList);
			compileResult.Success = result.ErrorList.Count() == 0;
			for (auto & shader : compileContext->Symbols.Shaders)
			{
				if (!modules.ContainsKey(shader.Key))
				{
					ModuleMetaData meta;
					for (auto & comp : shader.Value->Components)
					{
						ComponentMetaData compMeta;
						compMeta.Name = comp.Key;
						compMeta.Type = TranslateExpressionType(comp.Value->Type->DataType);
						compMeta.TypeName = compMeta.Type->ToString();
						for (auto & impl : comp.Value->Implementations)
						{
							impl->SyntaxNode->LayoutAttributes.TryGetValue(L"Binding", compMeta.Register);
							if (impl->SyntaxNode->IsParam)
							{
								meta.Requirements.Add(compMeta);
							}
							else
							{
								for (auto & world : impl->Worlds)
								{
									auto list = meta.ComponentsByWorld.TryGetValue(world);
									if (!list)
									{
										meta.ComponentsByWorld[world] = EnumerableHashSet<ComponentMetaData>();
										list = meta.ComponentsByWorld.TryGetValue(world);
									}
									if (!list->Contains(compMeta))
									{
										if (list->Count())
										{
											compMeta.Offset = list->Last().Offset + list->Last().Type->GetSize();
											compMeta.Offset = RoundToAlignment(compMeta.Offset, compMeta.Type->GetAlignment());
										}
										list->Add(compMeta);
									}
								}
							}
						}
					}
					modules.Add(shader.Key, _Move(meta));
				}
			}
		}

		void LoadModuleSource(CoreLib::String src, CoreLib::String fileName)
		{
			List<CompileUnit> units;
			LoadModuleSource(units, processedModuleUnits, compileResult, src, fileName);
			moduleUnits.AddRange(units);
			UpdateModuleLibrary(units);
		}

		void LoadModuleSource(List<CompileUnit> & units, HashSet<String> & processedUnits, CompileResult & cresult, CoreLib::String src, CoreLib::String fileName)
		{
			Spire::Compiler::CompileResult result;
			List<String> unitsToInclude;
			unitsToInclude.Add(fileName);
			processedUnits.Add(fileName);
			auto searchDirs = Options.SearchDirectories;
			searchDirs.Add(Path::GetDirectoryName(fileName));
			searchDirs.Reverse();
			for (int i = 0; i < unitsToInclude.Count(); i++)
			{
				auto inputFileName = unitsToInclude[i];
				try
				{
					String source = src;
					if (i > 0)
						source = File::ReadAllText(inputFileName);
					auto unit = compiler->Parse(result, source, inputFileName);
					units.Add(unit);
					if (unit.SyntaxNode)
					{
						for (auto inc : unit.SyntaxNode->Usings)
						{
							bool found = false;
							for (auto & dir : searchDirs)
							{
								String includeFile = Path::Combine(dir, inc.Content);
								if (File::Exists(includeFile))
								{
									if (processedUnits.Add(includeFile))
									{
										unitsToInclude.Add(includeFile);
									}
									found = true;
									break;
								}
							}
							if (!found)
							{
								result.GetErrorWriter()->Error(2, L"cannot find file '" + inputFileName + L"'.", inc.Position);
							}
						}
					}
				}
				catch (IOException)
				{
					result.GetErrorWriter()->Error(1, L"cannot open file '" + inputFileName + L"'.", CodePosition(0, 0, L""));
				}
			}
			cresult.Errors.AddRange(result.ErrorList);
			cresult.Warnings.AddRange(result.WarningList);
		}
		Shader * NewShader(CoreLib::String name)
		{
			return new Shader(name, true);
		}
		bool Compile(CompileResult & result, const Shader & shader)
		{
			return Compile(result, shader.GetSource(), shader.GetName());
		}
		bool Compile(CompileResult & result, CoreLib::String source, CoreLib::String fileName)
		{
			List<CompileUnit> userUnits;
			HashSet<String> processedUserUnits = processedModuleUnits;
			result = compileResult;
			result.Errors = compileResult.Errors;
			result.Warnings = compileResult.Warnings;
			if (result.Errors.Count() == 0)
			{
				LoadModuleSource(userUnits, processedUserUnits, result, source, fileName);
				if (result.Errors.Count() == 0)
				{
					Spire::Compiler::CompilationContext tmpCtx(*compileContext);
					Spire::Compiler::CompileResult cresult;
					compiler->Compile(cresult, tmpCtx, userUnits, Options);
					result.Sources = cresult.CompiledSource;
					result.Errors = _Move(cresult.ErrorList);
					result.Warnings = _Move(cresult.WarningList);
				}
			}
			result.Success = (result.Errors.Count() == 0);
			return result.Success;
		}
	};
}

using namespace SpireLib;

// implementation of C interface

#define CTX(x) reinterpret_cast<SpireLib::CompilationContext *>(x)
#define SHADER(x) reinterpret_cast<SpireLib::Shader*>(x)
#define RS(x) reinterpret_cast<SpireLib::CompileResult*>(x)
#define MODULE(x) reinterpret_cast<SpireLib::ModuleMetaData*>(x)

SpireCompilationContext * spCreateCompilationContext(const char * cacheDir)
{
	return reinterpret_cast<SpireCompilationContext *>(new SpireLib::CompilationContext((cacheDir?true:false), cacheDir));
}

void spSetCodeGenTarget(SpireCompilationContext * ctx, int target)
{
	CTX(ctx)->Options.Target = (CodeGenTarget)target;
}

void spAddSearchPath(SpireCompilationContext * ctx, const char * searchDir)
{
	CTX(ctx)->Options.SearchDirectories.Add(searchDir);
}

void spSetBackendParameter(SpireCompilationContext * ctx, const char * paramName, const char * value)
{
	CTX(ctx)->Options.BackendArguments[paramName] = value;
}

void spSetShaderToCompile(SpireCompilationContext * ctx, const char * shaderName)
{
	CTX(ctx)->Options.SymbolToCompile = shaderName;
}

void spDestroyCompilationContext(SpireCompilationContext * ctx)
{
	delete CTX(ctx);
}

void spLoadModuleLibrary(SpireCompilationContext * ctx, const char * fileName)
{
	CTX(ctx)->LoadModuleSource(File::ReadAllText(fileName), fileName);
}

void spLoadModuleLibraryFromSource(SpireCompilationContext * ctx, const char * source, const char * fileName)
{
	CTX(ctx)->LoadModuleSource(source, fileName);
}

SpireShader * spCreateShader(SpireCompilationContext * ctx, const char * name)
{
	return reinterpret_cast<SpireShader*>(CTX(ctx)->NewShader(name));
}

void spShaderAddModule(SpireShader * shader, const char * moduleName)
{
	SHADER(shader)->UseModule(moduleName);
}

void spShaderSetPipeline(SpireShader * shader, const char * pipelineName)
{
	SHADER(shader)->TargetPipeline(pipelineName);
}

SpireModule * spFindModule(SpireCompilationContext * ctx, const char * moduleName)
{
	return reinterpret_cast<SpireModule*>(CTX(ctx)->FindModule(moduleName));
}

int spModuleGetComponentsByWorld(SpireModule * module, const char * worldName, SpireComponentInfo * buffer, int bufferSize)
{
	auto moduleNode = MODULE(module);
	String worldNameStr = worldName;
	if (auto components = moduleNode->ComponentsByWorld.TryGetValue(worldNameStr))
	{
		if (!buffer)
			return components->Count();
		if (bufferSize < components->Count())
			return SPIRE_ERROR_INSUFFICIENT_BUFFER;
		int ptr = 0;
		for (auto & comp : *components)
		{
			buffer[ptr].Name = comp.Name.ToMultiByteString();
			buffer[ptr].TypeName = comp.TypeName.ToMultiByteString();
			buffer[ptr].Alignment = comp.Type->GetAlignment();
			buffer[ptr].Size = comp.Type->GetSize();
			buffer[ptr].Offset = comp.Offset;
			ptr++;
		}
		return ptr;
	}
	return 0;
}

int spModuleGetRequiredComponents(SpireModule * module, SpireComponentInfo * buffer, int bufferSize)
{
	auto moduleNode = MODULE(module);
	auto & components = moduleNode->Requirements;
	if (!buffer)
		return components.Count();
	if (bufferSize < components.Count())
		return SPIRE_ERROR_INSUFFICIENT_BUFFER;
	int ptr = 0;
	for (auto & comp : components)
	{
		buffer[ptr].Name = comp.Name.ToMultiByteString();
		buffer[ptr].TypeName = comp.TypeName.ToMultiByteString();
		buffer[ptr].Alignment = comp.Type->GetAlignment();
		buffer[ptr].Size = comp.Type->GetSize();
		buffer[ptr].Offset = comp.Offset;
		ptr++;
	}
	return ptr;
}

void spDestroyShader(SpireShader * shader)
{
	delete SHADER(shader);
}

SpireCompilationResult * spCompileShader(SpireCompilationContext * ctx, SpireShader * shader)
{
	SpireLib::CompileResult * rs = new SpireLib::CompileResult();
	CTX(ctx)->Compile(*rs, *SHADER(shader));
	return reinterpret_cast<SpireCompilationResult*>(rs);
}

SpireCompilationResult * spCompileShaderFromSource(SpireCompilationContext * ctx, const char * source, const char * fileName)
{
	SpireLib::CompileResult * rs = new SpireLib::CompileResult();
	CTX(ctx)->Compile(*rs, source, fileName);
	return reinterpret_cast<SpireCompilationResult*>(rs);
}

int spIsCompilationSucessful(SpireCompilationResult * result)
{
	return RS(result)->Success ? 1 : 0;
}

int spGetMessageCount(SpireCompilationResult * result, int messageType)
{
	return messageType == SPIRE_ERROR ? RS(result)->Errors.Count() : RS(result)->Warnings.Count();
}

int spGetMessageContent(SpireCompilationResult * result, int messageType, int index, SpireErrorMessage * pMsg)
{
	auto * list = (messageType == SPIRE_ERROR) ? &(RS(result)->Errors) : (messageType == SPIRE_WARNING) ? &(RS(result)->Warnings) : nullptr;
	if (list)
	{
		if (index >= 0 && index < list->Count())
		{
			auto & msg = (*list)[index];
			pMsg->Message = msg.Message.ToMultiByteString();
			pMsg->ErrorId = msg.ErrorID;
			pMsg->FileName = msg.Position.FileName.ToMultiByteString();
			pMsg->Line = msg.Position.Line;
			pMsg->Col = msg.Position.Col;
			return 1;
		}
	}
	return SPIRE_ERROR_INVALID_PARAMETER;
}

int ReturnStr(const char * content, char * buffer, int bufferSize)
{
	int len = (int)strlen(content);
	if (buffer)
	{
		if (bufferSize >= len + 1)
		{
			memcpy(buffer, content, len + 1);
			return len + 1;
		}
		else
			return SPIRE_ERROR_INSUFFICIENT_BUFFER;
	}
	else
		return len + 1;
}

int spGetCompiledShaderNames(SpireCompilationResult * result, char * buffer, int bufferSize)
{
	StringBuilder sb;
	auto rs = RS(result);
	bool first = true;
	for (auto x : rs->Sources)
	{
		if (!first)
			sb << L"\n";
		sb << x.Key;
		first = false;
	}
	auto str = sb.ProduceString();
	return ReturnStr(str.ToMultiByteString(), buffer, bufferSize);
}

int spGetCompiledShaderStageNames(SpireCompilationResult * result, const char * shaderName, char * buffer, int bufferSize)
{
	auto rs = RS(result);
	if (auto src = rs->Sources.TryGetValue(shaderName))
	{
		StringBuilder sb;
		bool first = true;
		for (auto x : src->Stages)
		{
			if (!first)
				sb << L"\n";
			sb << x.Key;
			first = false;
		}
		auto str = sb.ProduceString();
		return ReturnStr(str.ToMultiByteString(), buffer, bufferSize);
	}
	else
	{
		return SPIRE_ERROR_INVALID_PARAMETER;
	}
}

char * spGetShaderStageSource(SpireCompilationResult * result, const char * shaderName, const char * stage, int * length)
{
	auto rs = RS(result);
	if (auto src = rs->Sources.TryGetValue(shaderName))
	{
		if (auto state = src->Stages.TryGetValue(stage))
		{
			if (state->MainCode.Length())
			{
				*length = (int)strlen(state->MainCode.ToMultiByteString()) + 1;
				return state->MainCode.ToMultiByteString();
			}
			else
			{
				*length = state->BinaryCode.Count();
				return (char*)state->BinaryCode.Buffer();
			}
		}
	}
	return nullptr;
}

void spDestroyCompilationResult(SpireCompilationResult * result)
{
	delete RS(result);
}


