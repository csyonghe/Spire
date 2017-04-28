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

struct SpireUniformFieldImpl
{
	int offset;
	int size;
	String name;
	String type;
};

struct SpireParameterSet
{
	ILModuleParameterSet * paramSet = nullptr;
	int bindingSlotCount = 0;
	int uniformBufferLegacyBindingPoint = -1;
	List<SpireResourceBindingInfo> bindings;
	List<SpireParameterSet> subsets;
	List<SpireUniformFieldImpl> uniforms;
};


class ComponentMetaData
{
public:
	RefPtr<ExpressionType> Type;
	String TypeName;
	String Name;
	bool IsSpecialize = false;
	List<int> Values;
	int Offset = 0;
	int Alignment = 0;
	int Size = 0;
	BindableResourceType bindableType;
	int typeSpecificBindingIndex = -1;
	int typeAgnosticBindingIndex = -1;
	int GetHashCode()
	{
		return Name.GetHashCode();
	}
	bool operator == (const ComponentMetaData & other)
	{
		return Name == other.Name;
	}
};

struct CompilerState;

struct SpireModule
{
	String Name;
	int Id = 0;
	int UniformBufferSize = 0;
	int UniformBufferOffset = 0;
	SpireBindingIndex BindingIndex;
	List<ComponentMetaData> Parameters;
	EnumerableDictionary<String, ComponentMetaData> ParameterMap;
	List<ComponentMetaData> Requirements;
	Dictionary<String, String> Attribs;
	List<RefPtr<SpireModule>> SubModules;
	CompilerState * State = nullptr;
	static int IdAllocator;
};

int SpireModule::IdAllocator = 0;

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
				if (entry.Value->BufferOffset == -1)
				{
					writer << "binding(";
					for (auto binding : entry.Value->BindingPoints)
						writer << binding << " ";
					writer << ")";
				}
				else
				{
                    // TODO(tfoley): Should use the correct layout rule here, and not assume `Std140`
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

	
}

using namespace SpireLib;


struct ShaderParameter
{
	String TypeName;
	String Name;
	int BindingId;
};

class Shader
{
	friend class ::CompilationContext;
private:
	String shaderName;
	String src;
public:
	int Id;
	List<ShaderParameter> Parameters;
	RefPtr<Decl> Syntax;
	Shader(String name, String source)
	{
		static int idAllocator = 0;
		Id = idAllocator++;
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

struct CompilerState : public RefObject
{
	List<CompileUnit> moduleUnits;
	HashSet<String> processedModuleUnits;
	EnumerableDictionary<String, RefPtr<SpireModule>> modules;
	EnumerableDictionary<String, RefPtr<Shader>> shaders;
	RefPtr<Spire::Compiler::CompilationContext> context;
	RefPtr<CompilerState> Parent;
	// the version of this state
	int Version = 0;
	// the version of parent states cached in this state
	int CachedParentVersion = 0;
	void Update() // update this state to include latest version of parent
	{
		if (Parent)
		{
			Parent->Update();
			if (Parent->Version != CachedParentVersion)
			{
				context->MergeWith(Parent->context.Ptr());
				CachedParentVersion = Parent->Version;
				Version++;
			}
		}
	}

	int errorCount = 0;
	CompilerState()
	{
		context = new Spire::Compiler::CompilationContext();
	}
	CompilerState(RefPtr<CompilerState> parent)
	{
		this->Parent = parent;

		// Do the ugly thing to copy parent's symbol table to child
		if (parent)
			context = new Spire::Compiler::CompilationContext(*parent->context);
	}
	~CompilerState()
	{
		printf("break");
	}
};

class CompilationContext;

struct SpireCompilationEnvironment
{
	::CompilationContext * context;
	RefPtr<::CompilerState> state;
};

class CompilationContext
{
public:
	bool useCache = false;
	CoreLib::String cacheDir;

	List<RefPtr<::CompilerState>> states;
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
		states.Add(new ::CompilerState());
		LoadModuleSource(states.First().Ptr(), SpireStdLib::GetCode(), "stdlib", NULL);
	}

	~CompilationContext()
	{
		SpireStdLib::Finalize();
	}

	SpireModule * FindModule(CoreLib::String moduleName)
	{
		auto ptr = states.Last()->modules.TryGetValue(moduleName);
		if (ptr)
			return ptr->Ptr();
		else
			return nullptr;
	}

	StringBuilder moduleKeyBuilder;
	SpireModule * SpecializeModule(SpireModule * module, int * params, int numParams, SpireDiagnosticSink * sink)
	{
		moduleKeyBuilder.Clear();
		moduleKeyBuilder.Append(module->Name);
		for (auto & param : module->Parameters)
		{
			if (param.IsSpecialize)
			{
				int id = -1;
				for (int i = 0; i < numParams; i++)
				{
					moduleKeyBuilder.Append(params[id]);
					moduleKeyBuilder.Append('_');
				}
			}
		}
		if (auto smodule = module->State->modules.TryGetValue(moduleKeyBuilder.Buffer()))
			return smodule->Ptr();
		RefPtr<ShaderSymbol> originalModule;
		module->State->context->Symbols.Shaders.TryGetValue(module->Name, originalModule);
		CompileUnit unit;
		unit.SyntaxNode = new ProgramSyntaxNode();
		CloneContext cloneCtx;
		auto newModule = originalModule->SyntaxNode->Clone(cloneCtx);
		newModule->Name.Content = moduleKeyBuilder.ToString();
		int id = 0;
		for (auto & member : newModule->Members)
		{
			if (auto param = member.As<ComponentSyntaxNode>())
			{
				if (auto specialize = param->FindSpecializeModifier())
				{
					if (id >= numParams)
					{
						return nullptr;
					}
					auto newParam = param->Clone(cloneCtx);
					newParam->modifiers.first = nullptr;
					newParam->modifiers.flags = ModifierFlag::Public;
					param->BlockStatement = nullptr;
					auto expr = new ConstantExpressionSyntaxNode();
					if (param->Type->Equals(ExpressionType::Bool))
						expr->ConstType = ConstantExpressionSyntaxNode::ConstantType::Bool;
					else
						expr->ConstType = ConstantExpressionSyntaxNode::ConstantType::Int;
					expr->IntValue = params[id];
					newParam->Expression = expr;
					newModule->Members.Add(newParam);
					param->Name.Content = param->Name.Content + "placeholder";
					id++;
				}
			}
		}
		unit.SyntaxNode->Members.Add(newModule);
		List<CompileUnit> units;
		units.Add(unit);
		UpdateModuleLibrary(module->State, units, sink);
		return FindModule(newModule->Name.Content);
	}

	LayoutRule GetUniformBufferLayoutRule()
	{
		if (this->Options.Target == CodeGenTarget::HLSL)
			return LayoutRule::HLSL;
		else
			return LayoutRule::Std140;
	}

	RefPtr<SpireModule> CreateModule(CompilerState * state, Spire::Compiler::ShaderSymbol * shader, LayoutInfo & parentParamStruct, SpireBindingIndex & bindingIndex)
	{
		RefPtr<SpireModule> newModule = new SpireModule();
		newModule->State = state;
		auto & meta = *newModule;
		meta.Id = SpireModule::IdAllocator++;
		meta.Name = shader->SyntaxNode->Name.Content;
		meta.BindingIndex = bindingIndex;
		int offsets[2] = { 0, 0 };
		for (auto attrib : shader->SyntaxNode->GetModifiersOfType<Spire::Compiler::SimpleAttribute>())
			meta.Attribs[attrib->Key] = attrib->Value.Content;
		auto layout = GetLayoutRulesImpl(GetUniformBufferLayoutRule());
		LayoutInfo requireStruct = layout->BeginStructLayout();
		bool firstCompEncountered = false;
		for (auto & comp : shader->Components)
		{
			if (comp.Value->Implementations.Count() != 1)
				continue;
			auto impl = comp.Value->Implementations.First();
			if (!impl->SyntaxNode->IsRequire() && !impl->SyntaxNode->IsParam())
				continue;
			if (comp.Value->Implementations.First()->SyntaxNode->IsComponentFunction())
				continue;
			auto & structInfo = impl->SyntaxNode->IsParam() ? parentParamStruct : requireStruct;
			ComponentMetaData compMeta;
			compMeta.Name = comp.Key;
			compMeta.Type = comp.Value->Type->DataType;
			compMeta.TypeName = compMeta.Type->ToString();
			if (auto specialize = impl->SyntaxNode->FindSpecializeModifier())
			{
				for (auto val : specialize->Values)
				{
					compMeta.Values.Add(dynamic_cast<ConstantExpressionSyntaxNode*>(val.Ptr())->IntValue);
				}
				compMeta.IsSpecialize = true;
			}
			auto bindableType = compMeta.Type->GetBindableResourceType();
			if (compMeta.Type->GetBindableResourceType() == BindableResourceType::NonBindable)
			{
				if (!firstCompEncountered)
				{
                    // TODO(tfoley): This logic seems very wrong.
                    // We should be handling things by adding a field to the `parentParamStruct`
                    // (which should handle whatever layout rules it wants to use)
                    // rather than doing this ad hoc logic to special-case HLSL...

					firstCompEncountered = true;
					if (GetUniformBufferLayoutRule() == LayoutRule::HLSL)
						parentParamStruct.size = (size_t)RoundToAlignment((int)parentParamStruct.size, 16);
					meta.UniformBufferOffset = (int)parentParamStruct.size;
				}
				compMeta.Alignment = (int)GetTypeAlignment(compMeta.Type.Ptr(), GetUniformBufferLayoutRule());
				compMeta.Size = (int)GetTypeSize(compMeta.Type.Ptr(), GetUniformBufferLayoutRule());
				auto fieldInfo = GetLayout(compMeta.Type.Ptr(), layout);
				compMeta.Offset = (int)layout->AddStructField(&structInfo, fieldInfo);
			}
			else
			{
				switch (bindableType)
				{
				case BindableResourceType::Texture:
					compMeta.typeSpecificBindingIndex = bindingIndex.texture;
					bindingIndex.texture++;
					break;
				case BindableResourceType::Sampler:
					compMeta.typeSpecificBindingIndex = bindingIndex.sampler;
					bindingIndex.sampler++;
					break;
				case BindableResourceType::Buffer:
					compMeta.typeSpecificBindingIndex = bindingIndex.uniformBuffer;
					bindingIndex.uniformBuffer++;
					break; 
				case BindableResourceType::StorageBuffer:
					compMeta.typeSpecificBindingIndex = bindingIndex.storageBuffer;
					bindingIndex.storageBuffer++;
					break;
				}
				compMeta.typeAgnosticBindingIndex = bindingIndex.general;
				bindingIndex.general++;
			}
			if (impl->SyntaxNode->IsRequire())
				meta.Requirements.Add(compMeta);
			else
			{
				meta.Parameters.Add(compMeta);
				meta.ParameterMap[compMeta.Name] = compMeta;
			}
		}
		layout->EndStructLayout(&requireStruct);
		
		for (auto sub : shader->SyntaxNode->GetMembersOfType<ImportSyntaxNode>())
		{
			newModule->SubModules.Add(CreateModule(state, state->context->Symbols.Shaders[sub->ShaderName.Content]().Ptr(), parentParamStruct, bindingIndex));
		}
		meta.UniformBufferSize = (int)(parentParamStruct.size - meta.UniformBufferOffset);
		return newModule;
	}

	void UpdateModuleLibrary(CompilerState * state, List<CompileUnit> & units, SpireDiagnosticSink * sink)
	{
		Spire::Compiler::CompileResult result;
		compiler->Compile(result, *state->context, units, Options);
		state->Version++;
		for (auto & shader : state->context->Symbols.Shaders)
		{
			if (!state->modules.ContainsKey(shader.Key))
			{
				SpireBindingIndex bindingIndex;
				auto layout = GetLayoutRulesImpl(GetUniformBufferLayoutRule());
				LayoutInfo paramStruct = layout->BeginStructLayout();
				RefPtr<SpireModule> newModule = CreateModule(state, shader.Value.Ptr(), paramStruct, bindingIndex);
				newModule->BindingIndex;
				layout->EndStructLayout(&paramStruct);
				newModule->UniformBufferSize = (int)paramStruct.size;
				state->modules.Add(shader.Key, newModule);
			}
		}
		for (auto & unit : units)
		{
			for (auto & shader : unit.SyntaxNode->GetMembersOfType<TemplateShaderSyntaxNode>())
			{
				RefPtr<Shader> rs = new Shader(shader->Name.Content, "");
				int i = 0;
				rs->Syntax = shader;
				for (auto & param : shader->Parameters)
				{
					ShaderParameter p;
					p.BindingId = i;
					p.Name = param->ModuleName.Content;
					p.TypeName = param->InterfaceName.Content;
					rs->Parameters.Add(p);
					i++;
				}
				state->shaders[shader->Name.Content] = rs;
			}
			for (auto & shader : unit.SyntaxNode->GetMembersOfType<ShaderSyntaxNode>())
			{
				if (shader->IsModule)
					continue;
				RefPtr<Shader> rs = new Shader(shader->Name.Content, "");
				rs->Syntax = shader;
				HashSet<int> usedIds;
				for (auto & imp : unit.SyntaxNode->GetMembersOfType<ImportSyntaxNode>())
				{
					ShaderParameter param;
					param.TypeName = imp->ShaderName.Content;
					param.Name = imp->ObjectName.Content;
					param.BindingId = -1;
					String binding;
					if (imp->FindSimpleAttribute("Binding", binding))
					{
						param.BindingId = StringToInt(binding);
						usedIds.Add(param.BindingId);
					}
					rs->Parameters.Add(param);
				}
				int idAlloc = 0;
				for (auto & param : rs->Parameters)
				{
					if (param.BindingId == -1)
					{
						while (usedIds.Contains(idAlloc))
							idAlloc++;
						param.BindingId = idAlloc;
						idAlloc++;
					}
				}
				state->shaders[shader->Name.Content] = rs;
			}
		}
		if (sink)
		{
			sink->diagnostics.AddRange(result.sink.diagnostics);
			sink->errorCount += result.GetErrorCount();
		}
	}

	int LoadModuleSource(CompilerState * state, CoreLib::String src, CoreLib::String fileName, SpireDiagnosticSink* sink)
	{
		List<CompileUnit> units;
		int errCount = LoadModuleUnits(state, units, src, fileName, sink);
		state->moduleUnits.AddRange(units);
		UpdateModuleLibrary(state, units, sink);
		return errCount;
	}

	int LoadModuleUnits(CompilerState * state, List<CompileUnit> & units, CoreLib::String src, CoreLib::String fileName, SpireDiagnosticSink* sink)
	{
		auto & processedUnits = state->processedModuleUnits;
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
	Shader * FindShader(const char * name)
	{
		RefPtr<Shader> rs;
		if (states.Last()->shaders.TryGetValue(name, rs))
			return rs.Ptr();
		return nullptr;
	}
	Shader * GetShader(int index)
	{
		int i = 0;
		for (auto & shader : states.Last()->shaders)
		{
			if (i == index)
				return shader.Value.Ptr();
			i++;
		}
		return nullptr;
	}
	int GetShaderCount()
	{
		if (states.Count())
			return states.Last()->shaders.Count();
		return 0;
	}
	Shader * NewShaderFromSource(CompilerState * state, const char * source, const char * fileName, SpireDiagnosticSink * sink)
	{
		int shaderCount = GetShaderCount();
		LoadModuleSource(state, source, fileName, sink);
		int newShaderCount = GetShaderCount();
		if (newShaderCount > shaderCount)
			return GetShader(shaderCount);
		return nullptr;
	}
	Shader * NewShaderFromFile(CompilerState * state, const char * fileName, SpireDiagnosticSink * sink)
	{
		try
		{
			return NewShaderFromSource(state, File::ReadAllText(fileName).Buffer(), fileName, sink);
		}
		catch (Exception)
		{
			return nullptr;
		}
	}
	void PushContext()
	{
		states.Add(new CompilerState(states.Last()));
	}
	void PopContext()
	{
		states.Last() = nullptr;
		states.SetSize(states.Count() - 1);
	}
	bool Compile(::CompileResult & result, RefPtr<CompilerState> currentState, const Shader & shader, ArrayView<SpireModule*> modulesArgs, const char * additionalSource, SpireDiagnosticSink* sink)
	{
		Options.SymbolToCompile = shader.GetName();
		Options.TemplateShaderArguments.Clear();
		for (auto module : modulesArgs)
			Options.TemplateShaderArguments.Add(module->Name);
		return Compile(result, currentState, shader.Syntax, additionalSource, shader.GetName(), sink);
	}
	SpireParameterSet GetParameterSet(ILModuleParameterSet * module)
	{
		SpireParameterSet set;
		set.paramSet = module;
		set.uniformBufferLegacyBindingPoint = module->UniformBufferLegacyBindingPoint;
		for (auto & item : module->Parameters)
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
			else
			{
				SpireUniformFieldImpl ufield;
				ufield.name = item.Key.Buffer();
				ufield.offset = item.Value->BufferOffset;
				ufield.size = item.Value->Size;
				StringBuilder sb;
				item.Value->Type->Serialize(sb);
				ufield.type = sb.ProduceString();
				set.uniforms.Add(ufield);
			}
		}
		for (auto & submodule : module->SubModules)
			set.subsets.Add(GetParameterSet(submodule.Ptr()));
		return set;
	}
	bool Compile(::CompileResult & result, RefPtr<CompilerState> currentState, RefPtr<Decl> entryPoint, CoreLib::String source, CoreLib::String fileName, SpireDiagnosticSink* sink)
	{
		if (currentState->errorCount != 0)
			return false;
		currentState->Update();
		List<CompileUnit> units;
		currentState->errorCount += LoadModuleUnits(currentState.Ptr(), units, source, fileName, sink);
		if (currentState->errorCount != 0)
		{
			return false;
		}
		if (entryPoint)
		{
			CompileUnit newUnit;
			newUnit.SyntaxNode = new ProgramSyntaxNode();
			newUnit.SyntaxNode->Members.Add(entryPoint);
			units.Add(newUnit);
		}
		
		Spire::Compiler::CompileResult cresult;
		compiler->Compile(cresult, *(currentState->context), units, Options);
		result.Sources = cresult.CompiledSource;
		currentState->errorCount += cresult.GetErrorCount();
		if (sink)
		{
			sink->diagnostics.AddRange(cresult.sink.diagnostics);
			sink->errorCount += cresult.GetErrorCount();
		}
		if (currentState->errorCount == 0)
		{
			for (auto shader : result.Sources)
			{
				List<SpireParameterSet> paramSets;
				for (auto & pset : shader.Value.MetaData.ParameterSets)
				{
					if (!pset.Value->IsTopLevel)
						continue;
					paramSets.Add(GetParameterSet(pset.Value.Ptr()));
				}
				result.ParamSets[shader.Key] = _Move(paramSets);
			}
		}
		bool succ = currentState->errorCount == 0;
		return succ;
	}
};

// implementation of C interface

#define CTX(x) reinterpret_cast<::CompilationContext *>(x)
#define SHADER(x) reinterpret_cast<::Shader*>(x)
#define RS(x) reinterpret_cast<::CompileResult*>(x)

SpireCompilationContext * spCreateCompilationContext(const char * cacheDir)
{
	return reinterpret_cast<SpireCompilationContext *>(new ::CompilationContext((cacheDir ? true : false), cacheDir));
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
	CTX(ctx)->LoadModuleSource(CTX(ctx)->states.Last().Ptr(), File::ReadAllText(fileName), fileName, sink);
}

void spEnvLoadModuleLibrary(SpireCompilationEnvironment * env, const char * fileName, SpireDiagnosticSink * sink)
{
	env->context->LoadModuleSource(env->state.Ptr(), File::ReadAllText(fileName), fileName, sink);
}

void spLoadModuleLibraryFromSource(SpireCompilationContext * ctx, const char * source, const char * fileName, SpireDiagnosticSink* sink)
{
	CTX(ctx)->LoadModuleSource(CTX(ctx)->states.Last().Ptr(), source, fileName, sink);
}

void spEnvLoadModuleLibraryFromSource(SpireCompilationEnvironment * env, const char * source, const char * fileName, SpireDiagnosticSink * sink)
{
	env->context->LoadModuleSource(env->state.Ptr(), source, fileName, sink);
}

void spPushContext(SpireCompilationContext * ctx)
{
	CTX(ctx)->PushContext();
}

void spPopContext(SpireCompilationContext * ctx)
{
	CTX(ctx)->PopContext();
}

SpireCompilationEnvironment * spGetCurrentEnvironment(SpireCompilationContext * ctx)
{
	auto rs = new SpireCompilationEnvironment();
	rs->context = CTX(ctx);
	rs->state = CTX(ctx)->states.Last();
	return rs;
}

SpireCompilationEnvironment * spCreateEnvironment(SpireCompilationContext * ctx, SpireCompilationEnvironment * forkOrigin)
{
	auto rs = new SpireCompilationEnvironment();
	rs->context = CTX(ctx);
	if (forkOrigin)
		rs->state = new CompilerState(*forkOrigin->state);
	else
		rs->state = new CompilerState();
	return rs;
}

void spReleaseEnvironment(SpireCompilationEnvironment * env)
{
	delete env;
}

SpireShader* spCreateShaderFromSource(SpireCompilationContext * ctx, const char * source, SpireDiagnosticSink * sink)
{
	return reinterpret_cast<SpireShader*>(CTX(ctx)->NewShaderFromSource(CTX(ctx)->states.Last().Ptr(), source, "", sink));
}

SpireShader * spEnvCreateShaderFromSource(SpireCompilationEnvironment * env, const char * source, SpireDiagnosticSink * sink)
{
	return reinterpret_cast<SpireShader*>(env->context->NewShaderFromSource(env->state.Ptr(), source, "", sink));
}

SpireShader * spFindShader(SpireCompilationContext * ctx, const char * name)
{
	return reinterpret_cast<SpireShader*>(CTX(ctx)->FindShader(name));
}

SpireShader * spEnvFindShader(SpireCompilationEnvironment * env, const char * name)
{
	RefPtr<Shader> rs;
	if (env->state->shaders.TryGetValue(name, rs))
		return reinterpret_cast<SpireShader*>(rs.Ptr());
	return nullptr;
}

int spGetShaderCount(SpireCompilationContext * ctx)
{
	return CTX(ctx)->GetShaderCount();
}

int spEnvGetShaderCount(SpireCompilationEnvironment * env)
{
	return env->state->shaders.Count();
}

SpireShader * spGetShader(SpireCompilationContext * ctx, int index)
{
	return reinterpret_cast<SpireShader*>(CTX(ctx)->GetShader(index));
}

SpireShader * spEnvGetShader(SpireCompilationEnvironment * env, int index)
{
	int i = 0;
	for (auto & shader : env->state->shaders)
	{
		if (i == index)
			return reinterpret_cast<SpireShader*>(shader.Value.Ptr());
		i++;
	}
	return nullptr;
}

SpireShader* spCreateShaderFromFile(SpireCompilationContext * ctx, const char * fileName, SpireDiagnosticSink * sink)
{
	return reinterpret_cast<SpireShader*>(CTX(ctx)->NewShaderFromFile(CTX(ctx)->states.Last().Ptr(), fileName, sink));
}

SpireShader * spEnvCreateShaderFromFile(SpireCompilationEnvironment * env, const char * fileName, SpireDiagnosticSink * sink)
{
	return reinterpret_cast<SpireShader*>(env->context->NewShaderFromFile(env->state.Ptr(), fileName, sink));

}

unsigned int spShaderGetId(SpireShader * shader)
{
	return SHADER(shader)->Id;
}

const char* spShaderGetName(SpireShader * shader)
{
	return SHADER(shader)->GetName().Buffer();
}

const char * spShaderGetParameterType(SpireShader * shader, int i)
{
	if (shader && i >= 0 && i < SHADER(shader)->Parameters.Count())
		return SHADER(shader)->Parameters[i].TypeName.Buffer();
	return nullptr;
}

const char * spShaderGetParameterName(SpireShader * shader, int i)
{
	if (shader && i >= 0 && i < SHADER(shader)->Parameters.Count())
		return SHADER(shader)->Parameters[i].Name.Buffer();
	return nullptr;
}

int spShaderGetParameterBinding(SpireShader * shader, int i)
{
	if (shader && i >= 0 && i < SHADER(shader)->Parameters.Count())
		return SHADER(shader)->Parameters[i].BindingId;
	return -1;
}

int spShaderGetParameterCount(SpireShader * shader)
{
	return SHADER(shader)->Parameters.Count();
}

SpireModule * spFindModule(SpireCompilationContext * ctx, const char * moduleName)
{
	return CTX(ctx)->FindModule(moduleName);
}

SpireModule * spEnvFindModule(SpireCompilationEnvironment * env, const char * moduleName)
{
	auto ptr = env->state->modules.TryGetValue(moduleName);
	if (ptr)
		return ptr->Ptr();
	else
		return nullptr;
}

unsigned int spGetModuleUID(SpireModule * module)
{
	return module->Id;
}

const char * spGetModuleName(SpireModule * module)
{
	if (!module) return nullptr;
	auto moduleNode = module;
	return moduleNode->Name.Buffer();
}

SpireModule * spSpecializeModule(SpireCompilationContext * ctx, SpireModule * module, int * paramValues, int numParams, SpireDiagnosticSink * sink)
{
	return CTX(ctx)->SpecializeModule(module, paramValues, numParams, sink);
}

int spModuleGetParameterCount(SpireModule * module)
{
	auto moduleNode = module;
	return moduleNode->Parameters.Count();
}
int spModuleGetParameterBufferSize(SpireModule * module)
{
	return module->UniformBufferSize;
}
int spModuleHasAttrib(SpireModule * module, const char * name)
{
	return module->Attribs.ContainsKey(name);
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
	result->Specialize = param.IsSpecialize;
	return 1;
}

int spModuleGetParameterByName(SpireModule * module, const char * name, SpireComponentInfo * result)
{
	ComponentMetaData param;
	if (module->ParameterMap.TryGetValue(name, param))
	{
		result->TypeName = param.TypeName.Buffer();
		result->Size = param.Size;
		result->Offset = param.Offset;
		result->Alignment = param.Alignment;
		result->Name = param.Name.Buffer();
		result->BindableResourceType = (int)param.Type->GetBindableResourceType();
		result->Specialize = param.IsSpecialize;
		return 1;
	}
	return 0;
}

int spModuleGetSubModuleCount(SpireModule * module)
{
	return module->SubModules.Count();
}

SpireModule * spModuleGetSubModule(SpireModule * module, int index)
{
	return module->SubModules[index].Ptr();
}

int spModuleGetBufferOffset(SpireModule * module)
{
	return module->UniformBufferOffset;
}

int spModuleGetBindingOffset(SpireModule * module, SpireBindingIndex * pIndexOut)
{
	*pIndexOut = module->BindingIndex;
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
		buffer[ptr].Alignment = (int)GetTypeAlignment(comp.Type.Ptr());
		buffer[ptr].Size = (int)GetTypeSize(comp.Type.Ptr());
		buffer[ptr].Offset = comp.Offset;
		ptr++;
	}
	return ptr;
}

SpireCompilationResult * spCompileShader(SpireCompilationContext * ctx, SpireShader * shader,
	SpireModule** args,
	int argCount,
	const char * additionalSource,
	SpireDiagnosticSink* sink)
{
	::CompileResult * rs = new ::CompileResult();
	CTX(ctx)->PushContext();
	CTX(ctx)->Compile(*rs, CTX(ctx)->states.Last(), *SHADER(shader), ArrayView<SpireModule*>(args, argCount), additionalSource, sink);
	CTX(ctx)->PopContext();
	return reinterpret_cast<SpireCompilationResult*>(rs);
}

SPIRE_API SpireCompilationResult * spEnvCompileShader(SpireCompilationEnvironment * env, SpireShader * shader, SpireModule ** args, int argCount, const char * additionalSource, SpireDiagnosticSink * sink)
{
	::CompileResult * rs = new ::CompileResult();
	env->context->Compile(*rs, env->state, *SHADER(shader), ArrayView<SpireModule*>(args, argCount), additionalSource, sink);
	return reinterpret_cast<SpireCompilationResult*>(rs);
}

SpireCompilationResult * spCompileShaderFromSource(SpireCompilationContext * ctx, const char * source, const char * fileName, SpireDiagnosticSink* sink)
{
	::CompileResult * rs = new ::CompileResult();
	CTX(ctx)->PushContext();
	CTX(ctx)->Compile(*rs, CTX(ctx)->states.Last(), nullptr, source, fileName, sink);
	CTX(ctx)->PopContext();
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
	List<SpireParameterSet> * list = nullptr;
	if (shaderName == nullptr)
	{
		if (rs->ParamSets.Count())
			list = &rs->ParamSets.First().Value;
	}
	else
	{
		list = rs->ParamSets.TryGetValue(shaderName);
	}
	if (list)
	{
		return list->Count();
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
int spParameterSetGetBufferSize(SpireParameterSet * set)
{
	return set->paramSet->BufferSize;
}
int spParameterSetGetBufferOffset(SpireParameterSet * set)
{
	return set->paramSet->UniformBufferOffset;
}
int spParameterSetGetStartBindingIndex(SpireParameterSet * set, SpireBindingIndex * pIndexOut)
{
	pIndexOut->texture = set->paramSet->TextureBindingStartIndex;
	pIndexOut->sampler = set->paramSet->SamplerBindingStartIndex;
	pIndexOut->storageBuffer = set->paramSet->StorageBufferBindingStartIndex;
	pIndexOut->uniformBuffer = set->paramSet->UniformBindingStartIndex;
	return 0;
}
int spParameterSetGetUniformField(SpireParameterSet * set, int index, SpireUniformField * pUniformLayout)
{
	pUniformLayout->name = set->uniforms[index].name.Buffer();
	pUniformLayout->type = set->uniforms[index].type.Buffer();
	pUniformLayout->offset = set->uniforms[index].offset;
	pUniformLayout->size = set->uniforms[index].size;
	return 0;
}
int spParameterSetGetUniformFieldCount(SpireParameterSet * set)
{
	return set->uniforms.Count();
}
int spParameterSetGetSubSetCount(SpireParameterSet * set)
{
	return set->subsets.Count();
}
SpireParameterSet * spParameterSetGetSubSet(SpireParameterSet * set, int index)
{
	return &set->subsets[index];
}
const char * spParameterSetGetBindingName(SpireParameterSet * set)
{
	return set->paramSet->BindingName.Buffer();
}
int spParameterSetGetBindingIndex(SpireParameterSet * set)
{
	return set->paramSet->DescriptorSetId;
}
int spParameterSetGetUniformBufferLegacyBindingPoint(SpireParameterSet * set)
{
	return set->paramSet->UniformBufferLegacyBindingPoint;
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
