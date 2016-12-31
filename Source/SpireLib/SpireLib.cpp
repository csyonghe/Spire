#include "SpireLib.h"
#include "../CoreLib/LibIO.h"
#include "../CoreLib/Tokenizer.h"
#include "../SpireCore/StdInclude.h"
#include "../../Spire.h"
#include "../SpireCore/TypeLayout.h"
#include "../SpireCore/Preprocessor.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace CoreLib::Text;
using namespace Spire::Compiler;

struct SpireDiagnosticSink
{
	int errorCount;
	CoreLib::List<Spire::Compiler::Diagnostic> diagnostics;
};

struct SpireParameterSet
{
	ILModuleParameterSet * paramSet = nullptr;
	int bindingSlotCount = 0;
	List<SpireResourceBindingInfo> bindings;
};

class ComponentMetaData
{
public:
	RefPtr<ExpressionType> Type;
	String TypeName;
	String Name;
	int Offset = 0;
	int Alignment = 0;
	int Size = 0;
	int GetHashCode()
	{
		return Name.GetHashCode();
	}
	bool operator == (const ComponentMetaData & other)
	{
		return Name == other.Name;
	}
};

struct SpireModule
{
	String Name;
	List<ComponentMetaData> Parameters;
	List<ComponentMetaData> Requirements;
};

namespace SpireLib
{
	void ReadSource(EnumerableDictionary<String, StageSource> & sources, CoreLib::Text::TokenReader & parser, String src)
	{
		auto getShaderSource = [&]()
		{
			auto token = parser.ReadToken();
			int endPos = token.Position.Pos + 1;
			int brace = 0;
			while (endPos < src.Length() && !(src[endPos] == '}' && brace == 0))
			{
				if (src[endPos] == '{')
					brace++;
				else if (src[endPos] == '}')
					brace--;
				endPos++;
			}
			while (!parser.IsEnd() && parser.NextToken().Position.Pos != endPos)
				parser.ReadToken();
			parser.ReadToken();
			return src.SubString(token.Position.Pos + 1, endPos - token.Position.Pos - 1);
		};
		while (!parser.IsEnd() && !parser.LookAhead("}"))
		{
			auto worldName = parser.ReadWord();
			StageSource compiledSrc;
			if (parser.LookAhead("binary"))
			{
				parser.ReadToken();
				parser.Read("{");
				while (!parser.LookAhead("}") && !parser.IsEnd())
				{
					auto val = parser.ReadUInt();
					compiledSrc.BinaryCode.AddRange((unsigned char*)&val, sizeof(unsigned int));
					if (parser.LookAhead(","))
						parser.ReadToken();
				}
				parser.Read("}");
			}
			if (parser.LookAhead("text"))
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
		if (result.GetErrorCount() == 0)
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
		result.PrintDiagnostics();
		return false;
	}

	List<ShaderLibFile> CompileUnits(Spire::Compiler::CompileResult & compileResult,
		ShaderCompiler * compiler, List<CompileUnit> & units,
		Spire::Compiler::CompileOptions & options)
	{
		List<ShaderLibFile> resultFiles;
		compiler->Compile(compileResult, units, options);
		if (compileResult.GetErrorCount() == 0)
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
		struct IncludeHandlerImpl : IncludeHandler
		{
			List<String> searchDirs;

			virtual bool TryToFindIncludeFile(
				CoreLib::String const& pathToInclude,
				CoreLib::String const& pathIncludedFrom,
				CoreLib::String* outFoundPath,
				CoreLib::String* outFoundSource) override
			{
				String path = Path::Combine(Path::GetDirectoryName(pathIncludedFrom), pathToInclude);
				if (File::Exists(path))
				{
					*outFoundPath = path;
					*outFoundSource = File::ReadAllText(path);
					return true;
				}

				for (auto & dir : searchDirs)
				{
					path = Path::Combine(dir, pathToInclude);
					if (File::Exists(path))
					{
						*outFoundPath = path;
						*outFoundSource = File::ReadAllText(path);
						return true;
					}
				}
				return false;
			}

		};

		IncludeHandlerImpl includeHandler;
		includeHandler.searchDirs = options.SearchDirectories;

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
		auto predefUnit = compiler->Parse(compileResult, SpireStdLib::GetCode(), "stdlib", &includeHandler, options.PreprocessorDefinitions);
		for (int i = 0; i < unitsToInclude.Count(); i++)
		{
			auto inputFileName = unitsToInclude[i];
			try
			{
				String source = src;
				if (i > 0)
					source = File::ReadAllText(inputFileName);
				auto unit = compiler->Parse(compileResult, source, inputFileName, &includeHandler, options.PreprocessorDefinitions);
				units.Add(unit);
				if (unit.SyntaxNode)
				{
					for (auto inc : unit.SyntaxNode->GetUsings())
					{
						bool found = false;
						for (auto & dir : searchDirs)
						{
							String includeFile = Path::Combine(dir, inc->fileName.Content);
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
							compileResult.GetErrorWriter()->diagnose(inc->fileName.Position, Diagnostics::cannotFindFile, inc->fileName);
						}
					}
				}
			}
			catch (IOException)
			{
				compileResult.GetErrorWriter()->diagnose(CodePosition(0, 0, 0, ""), Diagnostics::cannotOpenFile, inputFileName);
			}
		}
		units.Add(predefUnit);
		if (compileResult.GetErrorCount() == 0)
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
			compileResult.GetErrorWriter()->diagnose(CodePosition(0, 0, 0, ""), Diagnostics::cannotOpenFile, Path::GetFileName(sourceFileName));
		}
		return List<ShaderLibFile>();
	}
	void ShaderLibFile::AddSource(CoreLib::Basic::String source, CoreLib::Text::TokenReader & parser)
	{
		ReadSource(Sources, parser, source);
	}

	CoreLib::String ShaderLibFile::ToString()
	{
		StringBuilder writer;
		writer << "name " << MetaData.ShaderName << EndLine;
		for (auto & ublock : MetaData.ParameterSets)
		{
			writer << "paramset \"" << ublock.Key << "\" size " << ublock.Value->BufferSize 
				<< " binding " << ublock.Value->DescriptorSetId << "\n{\n";
			for (auto & entry : ublock.Value->Parameters)
			{
				writer << entry.Value->Name << "(\"" << entry.Key << "\") : ";
				entry.Value->Type->Serialize(writer);
				writer << " at ";
				if (entry.Value->BindingPoints.Count())
				{
					writer << "binding(";
					for (auto binding : entry.Value->BindingPoints)
						writer << binding << " ";
					writer << ")";
				}
				else
				{
					writer << "buffer(" << entry.Value->BufferOffset << ", "
						<< (int)GetTypeSize(entry.Value->Type.Ptr(), LayoutRule::Std140) << ")";
				}
				writer << ";\n";
			}
			writer << "}\n";
		}
		writer << "source" << EndLine << "{" << EndLine;
		for (auto & src : Sources)
		{
			writer << src.Key << EndLine;
			if (src.Value.BinaryCode.Count())
			{
				writer << "binary" << EndLine << "{" << EndLine;
				auto binaryBuffer = (unsigned int*)src.Value.BinaryCode.Buffer();
				for (int i = 0; i < src.Value.BinaryCode.Count() / 4; i++)
				{
					writer << String((long long)binaryBuffer[i]) << ",";
					if ((i + 1) % 10)
						writer << EndLine;
				}
				writer << EndLine << "}" << EndLine;
			}
			writer << "text" << EndLine << "{" << EndLine;
			writer << src.Value.MainCode << EndLine;

			writer << "}" << EndLine;
		}
		writer << "}" << EndLine;
		StringBuilder formatSB;
		IndentString(formatSB, writer.ProduceString());
		return formatSB.ProduceString();
	}

	void ShaderLibFile::Clear()
	{
		Sources.Clear();
		MetaData.ParameterSets.Clear();
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
		CoreLib::Text::TokenReader parser(src);
		while (!parser.IsEnd())
		{
			auto fieldName = parser.ReadWord();
			if (fieldName == "name")
			{
				MetaData.ShaderName = parser.ReadWord();
			}
			else if (fieldName == "source")
			{
				parser.Read("{");
				ReadSource(Sources, parser, src);
				parser.Read("}");
			}
			else if (fieldName == "paramset")
			{
				RefPtr<ILModuleParameterSet> paramSet = new ILModuleParameterSet();
				paramSet->BindingName = parser.ReadStringLiteral();
				if (parser.LookAhead("size"))
				{
					parser.ReadToken();
					paramSet->BufferSize = parser.ReadInt();
				}
				if (parser.LookAhead("binding"))
				{
					parser.ReadToken();
					paramSet->DescriptorSetId = parser.ReadInt();
				}
				parser.Read("{");
				while (!parser.LookAhead("}"))
				{
					RefPtr<ILModuleParameterInstance> inst = new ILModuleParameterInstance();
					inst->Name = parser.ReadWord();
					parser.Read("(");
					auto key = parser.ReadStringLiteral();
					parser.Read(")");
					inst->Type = ILType::Deserialize(parser);
					parser.Read("at");
					if (parser.LookAhead("binding"))
					{
						parser.ReadToken();
						parser.Read("(");
						while (!parser.LookAhead(")"))
							inst->BindingPoints.Add(parser.ReadInt());
						parser.Read(")");
					}
					else
					{
						parser.Read("buffer");
						parser.Read("(");
						inst->BufferOffset = parser.ReadInt();
						parser.Read(")");
					}
					paramSet->Parameters.Add(key, inst);
				}
				parser.Read("}");
				MetaData.ParameterSets.Add(paramSet->BindingName, paramSet);
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
		String shaderName;
		String src;
	public:
		Shader(String name, String source)
		{
			shaderName = name;
			src = source;
		}
		String GetName() const
		{
			return shaderName;
		}
		String GetSource() const
		{
			return src;
		}
	};

	class CompileResult
	{
	public:
		CoreLib::EnumerableDictionary<String, CompiledShaderSource> Sources;
		CoreLib::EnumerableDictionary<String, List<SpireParameterSet>> ParamSets;

	};

	class CompilationContext
	{
	private:
		bool useCache = false;
		CoreLib::String cacheDir;
		struct State
		{
			List<CompileUnit> moduleUnits;
			HashSet<String> processedModuleUnits;
			EnumerableDictionary<String, SpireModule> modules;
			int errorCount = 0;
		};
		List<State> states;
		List<RefPtr<Spire::Compiler::CompilationContext>> compileContext;
		RefPtr<ShaderCompiler> compiler;

		struct IncludeHandlerImpl : IncludeHandler
		{
			List<String> searchDirs;

			virtual bool TryToFindIncludeFile(
				CoreLib::String const& pathToInclude,
				CoreLib::String const& pathIncludedFrom,
				CoreLib::String* outFoundPath,
				CoreLib::String* outFoundSource) override
			{
				String path = Path::Combine(Path::GetDirectoryName(pathIncludedFrom), pathToInclude);
				if (File::Exists(path))
				{
					*outFoundPath = path;
					*outFoundSource = File::ReadAllText(path);
					return true;
				}

				for (auto & dir : searchDirs)
				{
					path = Path::Combine(dir, pathToInclude);
					if (File::Exists(path))
					{
						*outFoundPath = path;
						*outFoundSource = File::ReadAllText(path);
						return true;
					}
				}
				return false;
			}
		};
		IncludeHandlerImpl includeHandler;

	public:
		CompileOptions Options;

		CompilationContext(bool /*pUseCache*/, CoreLib::String /*pCacheDir*/)
		{
			compiler = CreateShaderCompiler();
			compileContext.Add(new Spire::Compiler::CompilationContext());
			states.Add(State());
			LoadModuleSource(SpireStdLib::GetCode(), "stdlib", NULL);
		}

		~CompilationContext()
		{
			SpireStdLib::Finalize();
		}

		SpireModule * FindModule(CoreLib::String moduleName)
		{
			return states.Last().modules.TryGetValue(moduleName);
		}

		void UpdateModuleLibrary(List<CompileUnit> & units, SpireDiagnosticSink * sink)
		{
			Spire::Compiler::CompileResult result;
			compiler->Compile(result, *compileContext.Last(), units, Options);
			for (auto & shader : compileContext.Last()->Symbols.Shaders)
			{
				if (!states.Last().modules.ContainsKey(shader.Key))
				{
					SpireModule meta;
					meta.Name = shader.Key;
					int offset = 0;
					for (auto & comp : shader.Value->Components)
					{
						if (comp.Value->Implementations.Count() != 1)
							continue;
						auto impl = comp.Value->Implementations.First();
						if (!impl->SyntaxNode->IsRequire() && !impl->SyntaxNode->IsParam())
							continue;
						ComponentMetaData compMeta;
						compMeta.Name = comp.Key;
                        compMeta.Type = comp.Value->Type->DataType;
						compMeta.TypeName = compMeta.Type->ToString();
						if (compMeta.Type->GetBindableResourceType() == BindableResourceType::NonBindable)
						{
							compMeta.Alignment = (int)GetTypeAlignment(compMeta.Type.Ptr(), LayoutRule::Std140);
							compMeta.Size = (int)GetTypeSize(compMeta.Type.Ptr(), LayoutRule::Std140);
							offset = RoundToAlignment(offset, compMeta.Alignment);
							compMeta.Offset = offset;
							offset += compMeta.Size;
						}
						if (impl->SyntaxNode->IsRequire())
							meta.Requirements.Add(compMeta);
						else
							meta.Parameters.Add(compMeta);
					}
					states.Last().modules.Add(shader.Key, _Move(meta));
				}
			}
			if (sink)
			{
				sink->diagnostics.AddRange(result.sink.diagnostics);
				sink->errorCount += result.GetErrorCount();
			}
		}

		int LoadModuleSource(CoreLib::String src, CoreLib::String fileName, SpireDiagnosticSink* sink)
		{
			List<CompileUnit> units;
			int errCount = LoadModuleUnits(units, src, fileName, sink);
			states.Last().moduleUnits.AddRange(units);
			UpdateModuleLibrary(units, sink);
			return errCount;
		}

		int LoadModuleUnits(List<CompileUnit> & units, CoreLib::String src, CoreLib::String fileName, SpireDiagnosticSink* sink)
		{
			auto & processedUnits = states.Last().processedModuleUnits;

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
					auto unit = compiler->Parse(result, source, inputFileName, &includeHandler, Options.PreprocessorDefinitions);
					units.Add(unit);
					if (unit.SyntaxNode)
					{
						for (auto inc : unit.SyntaxNode->GetUsings())
						{
							bool found = false;
							for (auto & dir : searchDirs)
							{
								String includeFile = Path::Combine(dir, inc->fileName.Content);
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
								result.GetErrorWriter()->diagnose(inc->fileName.Position, Diagnostics::cannotFindFile, inc->fileName);
							}
						}
					}
				}
				catch (IOException)
				{
					result.GetErrorWriter()->diagnose(CodePosition(0, 0, 0, ""), Diagnostics::cannotOpenFile, inputFileName);
				}
			}
			if (sink)
			{
				sink->diagnostics.AddRange(result.sink.diagnostics);
				sink->errorCount += result.GetErrorCount();
			}
			return result.GetErrorCount();
		}
		Shader * NewShaderFromSource(const char * source, const char * fileName)
		{
			Spire::Compiler::CompileResult result;
			auto unit = compiler->Parse(result, source, fileName, nullptr, Dictionary<String, String>());
			auto list = unit.SyntaxNode->GetMembersOfType<TemplateShaderSyntaxNode>();
			if (list.Count())
				return new Shader((*list.begin())->Name.Content, String(source));
			return nullptr;
		}
		Shader * NewShaderFromFile(const char * fileName)
		{
			try
			{
				return NewShaderFromSource(File::ReadAllText(fileName).Buffer(), fileName);
			}
			catch (Exception)
			{
				return nullptr;
			}
		}
		void PushContext()
		{
			states.Add(states.Last());
			compileContext.Add(new Spire::Compiler::CompilationContext(*compileContext.Last()));
		}
		void PopContext()
		{
			compileContext.Last() = nullptr;
			compileContext.SetSize(compileContext.Count() - 1);
			states.Last() = State();
			states.SetSize(states.Count() - 1);
		}
		bool Compile(CompileResult & result, const Shader & shader, ArrayView<SpireModule*> modulesArgs, const char * additionalSource, SpireDiagnosticSink* sink)
		{
			Options.SymbolToCompile = shader.GetName();
			Options.TemplateShaderArguments.Clear();
			for (auto module : modulesArgs)
				Options.TemplateShaderArguments.Add(module->Name);
			return Compile(result, additionalSource + shader.GetSource(), shader.GetName(), sink);
		}
		bool Compile(CompileResult & result, CoreLib::String source, CoreLib::String fileName, SpireDiagnosticSink* sink)
		{
			if (states.Last().errorCount != 0)
				return false;
			PushContext();
			List<CompileUnit> units;
			states.Last().errorCount += LoadModuleUnits(units, source, fileName, sink);
			if (states.Last().errorCount != 0)
			{
				PopContext();
				return false;
			}
		
			Spire::Compiler::CompileResult cresult;
			compiler->Compile(cresult, *compileContext.Last(), units, Options);
			result.Sources = cresult.CompiledSource;
			states.Last().errorCount += cresult.GetErrorCount();
			if (sink)
			{
				sink->diagnostics.AddRange(cresult.sink.diagnostics);
				sink->errorCount += cresult.GetErrorCount();
			}
			if (states.Last().errorCount == 0)
			{
				for (auto shader : result.Sources)
				{
					List<SpireParameterSet> paramSets;
					for (auto & pset : shader.Value.MetaData.ParameterSets)
					{
						SpireParameterSet set;
						set.paramSet = pset.Value.Ptr();
						for (auto & item : pset.Value->Parameters)
						{
							auto resType = item.Value->Type->GetBindableResourceType();
							if (resType != BindableResourceType::NonBindable)
							{
								SpireResourceBindingInfo info;
								info.Type = (SpireBindableResourceType)resType;
								info.NumLegacyBindingPoints = item.Value->BindingPoints.Count();
								info.LegacyBindingPoints = item.Value->BindingPoints.Buffer();
								info.Name = item.Value->Name.Buffer();
								set.bindings.Add(info);
							}
						}
						paramSets.Add(_Move(set));
					}
					result.ParamSets[shader.Key] = _Move(paramSets);
				}
			}
			bool succ = states.Last().errorCount == 0;
			PopContext();
			return succ;
		}
	};
}

using namespace SpireLib;

// implementation of C interface

#define CTX(x) reinterpret_cast<SpireLib::CompilationContext *>(x)
#define SHADER(x) reinterpret_cast<SpireLib::Shader*>(x)
#define RS(x) reinterpret_cast<SpireLib::CompileResult*>(x)

SpireCompilationContext * spCreateCompilationContext(const char * cacheDir)
{
	return reinterpret_cast<SpireCompilationContext *>(new SpireLib::CompilationContext((cacheDir ? true : false), cacheDir));
}

void spSetCodeGenTarget(SpireCompilationContext * ctx, int target)
{
	CTX(ctx)->Options.Target = (CodeGenTarget)target;
}

void spAddSearchPath(SpireCompilationContext * ctx, const char * searchDir)
{
	CTX(ctx)->Options.SearchDirectories.Add(searchDir);
}

void spAddPreprocessorDefine(SpireCompilationContext * ctx, const char * key, const char * value)
{
	CTX(ctx)->Options.PreprocessorDefinitions[key] = value;
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

// `SpireDiagnosticSink` implementation

SpireDiagnosticSink* spCreateDiagnosticSink(SpireCompilationContext * /*ctx*/)
{
	SpireDiagnosticSink* sink = new SpireDiagnosticSink();
	sink->errorCount = 0;
	return sink;
}

void spClearDiagnosticSink(SpireDiagnosticSink* sink)
{
	if (!sink) return;

	sink->errorCount = 0;
	sink->diagnostics.Clear();
}

void spDestroyDiagnosticSink(SpireDiagnosticSink* sink)
{
	delete sink;
}

//

void spLoadModuleLibrary(SpireCompilationContext * ctx, const char * fileName, SpireDiagnosticSink* sink)
{
	CTX(ctx)->LoadModuleSource(File::ReadAllText(fileName), fileName, sink);
}

void spLoadModuleLibraryFromSource(SpireCompilationContext * ctx, const char * source, const char * fileName, SpireDiagnosticSink* sink)
{
	CTX(ctx)->LoadModuleSource(source, fileName, sink);
}

void spPushContext(SpireCompilationContext * ctx)
{
	CTX(ctx)->PushContext();
}

void spPopContext(SpireCompilationContext * ctx)
{
	CTX(ctx)->PopContext();
}

SpireShader* spCreateShaderFromSource(SpireCompilationContext * ctx, const char * source)
{
	return reinterpret_cast<SpireShader*>(CTX(ctx)->NewShaderFromSource(source, ""));
}

SpireShader* spCreateShaderFromFile(SpireCompilationContext * ctx, const char * fileName)
{
	return reinterpret_cast<SpireShader*>(CTX(ctx)->NewShaderFromFile(fileName));
}

const char* spShaderGetName(SpireShader * shader)
{
	return SHADER(shader)->GetName().Buffer();
}

SpireModule * spFindModule(SpireCompilationContext * ctx, const char * moduleName)
{
	return CTX(ctx)->FindModule(moduleName);
}

const char * spGetModuleName(SpireModule * module)
{
	if (!module) return nullptr;
	auto moduleNode = module;
	return moduleNode->Name.Buffer();
}

int spModuleGetParameterCount(SpireModule * module)
{
	auto moduleNode = module;
	return moduleNode->Parameters.Count();
}
int spModuleGetParameterBufferSize(SpireModule * module)
{
	auto moduleNode = module;
	int size = 0;
	for (auto & param : moduleNode->Parameters)
		size = Math::Max(size, param.Size + param.Offset);
	return size;
}

int spModuleGetParameter(SpireModule * module, int index, SpireComponentInfo * result)
{
	auto moduleNode = module;
	auto & param = moduleNode->Parameters[index];
	result->TypeName = param.TypeName.Buffer();
	result->Size = param.Size;
	result->Offset = param.Offset;
	result->Alignment = param.Alignment;
	result->Name = param.Name.Buffer();
	result->BindableResourceType = (int)param.Type->GetBindableResourceType();
	return 0;
}

int spModuleGetRequiredComponents(SpireModule * module, SpireComponentInfo * buffer, int bufferSize)
{
	auto moduleNode = module;
	auto & components = moduleNode->Requirements;
	if (!buffer)
		return components.Count();
	if (bufferSize < components.Count())
		return SPIRE_ERROR_INSUFFICIENT_BUFFER;
	int ptr = 0;
	for (auto & comp : components)
	{
		buffer[ptr].Name = comp.Name.Buffer();
		buffer[ptr].TypeName = comp.TypeName.Buffer();
		buffer[ptr].Alignment = (int) GetTypeAlignment(comp.Type.Ptr());
		buffer[ptr].Size = (int) GetTypeSize(comp.Type.Ptr());
		buffer[ptr].Offset = comp.Offset;
		ptr++;
	}
	return ptr;
}

void spDestroyShader(SpireShader * shader)
{
	delete SHADER(shader);
}

SpireCompilationResult * spCompileShader(SpireCompilationContext * ctx, SpireShader * shader, 
	SpireModule** args,
	int argCount,
	const char * additionalSource,
	SpireDiagnosticSink* sink)
{
	SpireLib::CompileResult * rs = new SpireLib::CompileResult();
	CTX(ctx)->Compile(*rs, *SHADER(shader), ArrayView<SpireModule*>(args, argCount), additionalSource, sink);
	return reinterpret_cast<SpireCompilationResult*>(rs);
}

SpireCompilationResult * spCompileShaderFromSource(SpireCompilationContext * ctx, const char * source, const char * fileName, SpireDiagnosticSink* sink)
{
	SpireLib::CompileResult * rs = new SpireLib::CompileResult();
	CTX(ctx)->Compile(*rs, source, fileName, sink);
	return reinterpret_cast<SpireCompilationResult*>(rs);
}

int spDiagnosticSinkHasAnyErrors(SpireDiagnosticSink* sink)
{
	if (!sink) return false;
	return sink->errorCount != 0;
}

int spGetDiagnosticCount(SpireDiagnosticSink* sink)
{
	return sink->diagnostics.Count();
}

int spGetDiagnosticByIndex(SpireDiagnosticSink* sink, int index, SpireDiagnostic * outDiagnostic)
{
	if (!sink)          return SPIRE_ERROR_INVALID_PARAMETER;
	if (!outDiagnostic) return SPIRE_ERROR_INVALID_PARAMETER;
	if (index < 0)      return SPIRE_ERROR_INVALID_PARAMETER;

	auto & diagnostics = sink->diagnostics;
	if (index >= diagnostics.Count())
		return SPIRE_ERROR_INVALID_PARAMETER;

	auto & msg = diagnostics[index];
	outDiagnostic->Message = msg.Message.Buffer();
	outDiagnostic->ErrorId = msg.ErrorID;
	outDiagnostic->FileName = msg.Position.FileName.Buffer();
	outDiagnostic->Line = msg.Position.Line;
	outDiagnostic->Col = msg.Position.Col;
	// Note: we rely here on the `SpireSeverity` and `Spire::Compiler::Severity`
	// enums having the same members. Realistically, we should probably just
	// use the external enum internally too.
	outDiagnostic->severity = (SpireSeverity)msg.severity;
	return 1;
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

int spGetDiagnosticOutput(SpireDiagnosticSink* sink, char * buffer, int bufferSize)
{
	StringBuilder sb;
	for (auto & x : sink->diagnostics)
	{
		sb << x.Position.ToString() << ": " << Spire::Compiler::getSeverityName(x.severity);
		if (x.ErrorID >= 0)
		{
			sb << " " << x.ErrorID;
		}
		sb << ": " << x.Message << "\n";
	}
	auto str = sb.ProduceString();
	return ReturnStr(str.Buffer(), buffer, bufferSize);
}

int spGetCompiledShaderNames(SpireCompilationResult * result, char * buffer, int bufferSize)
{
	StringBuilder sb;
	auto rs = RS(result);
	bool first = true;
	for (auto x : rs->Sources)
	{
		if (!first)
			sb << "\n";
		sb << x.Key;
		first = false;
	}
	auto str = sb.ProduceString();
	return ReturnStr(str.Buffer(), buffer, bufferSize);
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
				sb << "\n";
			sb << x.Key;
			first = false;
		}
		auto str = sb.ProduceString();
		return ReturnStr(str.Buffer(), buffer, bufferSize);
	}
	else
	{
		return SPIRE_ERROR_INVALID_PARAMETER;
	}
}

const char * spGetShaderStageSource(SpireCompilationResult * result, const char * shaderName, const char * stage, int * length)
{
	auto rs = RS(result);
	CompiledShaderSource * src = nullptr;
	if (shaderName == nullptr)
	{
		if (rs->Sources.Count())
			src = &rs->Sources.First().Value;
	}
	else
	{
		src = rs->Sources.TryGetValue(shaderName);
	}
	if (src)
	{
		if (auto state = src->Stages.TryGetValue(stage))
		{
			if (state->MainCode.Length())
			{
				if (length)
					*length = state->MainCode.Length() + 1;
				return state->MainCode.Buffer();
			}
			else
			{
				if (length)
					*length = state->BinaryCode.Count();
				return (const char*)state->BinaryCode.Buffer();
			}
		}
	}
	return nullptr;
}

int spGetShaderParameterSetCount(SpireCompilationResult * result, const char * shaderName)
{
	auto rs = RS(result);
	CompiledShaderSource * src = nullptr;
	if (shaderName == nullptr)
	{
		if (rs->Sources.Count())
			src = &rs->Sources.First().Value;
	}
	else
	{
		src = rs->Sources.TryGetValue(shaderName);
	}
	if (src)
	{
		return src->MetaData.ParameterSets.Count();
	}
	return 0;
}
SpireParameterSet * spGetShaderParameterSet(SpireCompilationResult * result, const char * shaderName, int index)
{
	auto rs = RS(result);
	List<SpireParameterSet> * sets = nullptr;
	if (shaderName == nullptr)
	{
		if (rs->ParamSets.Count())
			sets = &rs->ParamSets.First().Value;
	}
	else
	{
		sets = rs->ParamSets.TryGetValue(shaderName);
	}
	if (sets)
	{
		return &(*sets)[index];
	}
	return nullptr;
}
const char * spParameterSetGetBindingName(SpireParameterSet * set)
{
	return set->paramSet->BindingName.Buffer();
}
int spParameterSetGetBindingIndex(SpireParameterSet * set)
{
	return set->paramSet->DescriptorSetId;
}
int spParameterSetGetBindingSlotCount(SpireParameterSet * set)
{
	return set->bindings.Count();
}
SpireResourceBindingInfo * spParameterSetGetBindingSlot(SpireParameterSet * set, int index)
{
	return &set->bindings[index];
}
void spDestroyCompilationResult(SpireCompilationResult * result)
{
	delete RS(result);
}
