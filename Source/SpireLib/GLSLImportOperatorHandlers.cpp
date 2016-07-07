#include "ImportOperator.h"
using namespace Spire::Compiler;
using namespace CoreLib::Basic;

class GLSLImportOperatorHandler : public ImportOperatorHandler
{
public:
	virtual void GenerateSetInput(StringBuilder &, ComponentDefinition *, const ImportOperatorContext &) override
	{}
	virtual void GeneratePreamble(StringBuilder &, InterfaceBlock *, const ImportOperatorContext &) override
	{}
	virtual void GenerateEpilogue(StringBuilder &, InterfaceBlock *, const ImportOperatorContext &) override
	{}
};

class StandardGLSLImportOperatorHandler : public GLSLImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"standardImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext &) override
	{
		if (block->Entries.Count() == 0)
			return;
		sb << L"in " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			if (ent.Value.Type->IsIntegral())
				sb << L"flat ";
			sb << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & /*sb*/, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		auto block = ctx.SourceWorld->WorldOutput;
		instr->Name = L"blk" + block->Name + L"." + instr->ComponentName;		
	}
};

class VertexGLSLImportOperatorHandler : public GLSLImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"vertexImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & /*ctx*/) override
	{
		int location = 0;
		for (auto & ent : block->Entries)
		{
			sb << L"layout(location = " << location << L") ";
			sb << L"in " << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
			location++;
		}
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder &/*sb*/, ImportInstruction * instr, const ImportOperatorContext & /*ctx*/) override
	{
		instr->Name = instr->ComponentName;
	}
};

void GenerateBufferInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block)
{
	String strIdx;
	if (block->Attributes.TryGetValue(L"Index", strIdx))
		sb << L"layout(location = " << strIdx << L") ";
	sb << L"uniform float * " << block->Name << L";" << EndLine;
}

class BufferGLSLImportOperatorHandler : public GLSLImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"bufferImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext &) override
	{
		GenerateBufferInterfaceDefinition(sb, block);
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		auto block = ctx.SourceWorld->WorldOutput;
		sb << instr->Type->ToString() << L" " << instr->Name << L";\n";
		if (instr->Type->IsTexture())
		{
			sb << instr->Name << L" = *(" << instr->Type->ToString() << L"*)(" << block->Name <<
				L" + " << String(block->Entries[instr->ComponentName].GetValue().Offset / 4) << L" + gl_GlobalInvocationID.x * "
				<< String(block->Size / 4) << L");" << EndLine;
		}
		else
		{
			int vecSize = instr->Type->GetVectorSize();
			for (int i = 0; i < vecSize; i++)
			{
				sb << instr->Name;
				if (vecSize > 1)
				{
					if (vecSize == 9)
						sb << L"[" << i/3 << L"][" << i%3 << L"]";
					else if (vecSize == 16)
						sb << L"[" << i / 4 << L"][" << i % 4 << L"]";
					else
						sb << L"[" << i << L"]";
				}
				sb << L" = *";
				if (instr->Type->IsIntVector() || instr->Type->IsInt())
					sb << L"(int*)";
				sb<< L"(" << block->Name <<
					L" + " << String(block->Entries[instr->ComponentName].GetValue().Offset / 4 + i) << L" + gl_GlobalInvocationID.x * "
					<< String(block->Size / 4) << L");" << EndLine;
			}
		}
	}

	virtual void GeneratePreamble(StringBuilder & sb, InterfaceBlock * /*block*/, const ImportOperatorContext & /*context*/) override
	{
		sb << L"if (gl_GlobalInvocationID.x >= sys_thread_count) return;" << EndLine;
	}
	virtual void GenerateEpilogue(StringBuilder &, InterfaceBlock *, const ImportOperatorContext &) override
	{}
};

class BufferGLSLExportOperatorHandler : public ExportOperatorHandler
{
	virtual String GetName() override
	{
		return L"bufferExport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block) override
	{
		GenerateBufferInterfaceDefinition(sb, block);
		sb << L"uniform uint sys_thread_count;" << EndLine;
		sb << L"layout(local_size_x = 256) in;" << EndLine;
	}
	virtual void GenerateExport(StringBuilder & sb, InterfaceBlock * block, CompiledWorld *, String componentName, String valueVar) override
	{
		auto & comp = block->Entries[componentName].GetValue();
		if (comp.Type->IsTexture() || comp.Type->GetVectorSize() > 4)
		{
			throw NotImplementedException(L"exporting sampler2D or matrices is not supported.");
		}
		else
		{
			for (int i = 0; i < comp.Type->GetVectorSize(); i++)
			{
				sb << L"*";
				if (comp.Type->IsIntVector() || comp.Type->IsInt())
					sb << L"(int*)";

				sb << L"(" << block->Name <<
					L" + " << String(block->Entries[componentName].GetValue().Offset / 4 + i) << L" + gl_GlobalInvocationID.x * "
					<< String(block->Size / 4) << L") = " << valueVar;
				if (comp.Type->GetVectorSize() > 1)
					sb << L"[" << i << L"]";
				sb << L";" << EndLine;
			}
		}
	}
	virtual void GeneratePreamble(StringBuilder & sb, InterfaceBlock *) override
	{
		sb << L"if (gl_GlobalInvocationID.x >= sys_thread_count) return;" << EndLine;
	}
	virtual void GenerateEpilogue(StringBuilder &, InterfaceBlock *) override
	{}
};

class UniformGLSLImportOperatorHandler : public GLSLImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"uniformImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & ctx) override
	{
		if (block->Entries.Count() == 0)
			return;
		bool useBindlessTexture = ctx.BackendArguments.ContainsKey(L"bindless_texture");
		int activeEntryCount = 0;
		for (auto & ent : block->Entries)
		{
			if (useBindlessTexture || !ent.Value.Type->IsTexture())
				activeEntryCount++;
		}
		String bufferType = L"uniform";
		if (block->Attributes.ContainsKey(L"ShaderStorageBlock"))
			bufferType = L"buffer";

		if (activeEntryCount)
		{
			sb << L"layout(std140";
			String strIndex;
			if (block->Attributes.TryGetValue(L"Index", strIndex))
				sb << L", binding = " << strIndex;
			if (ctx.BackendArguments.ContainsKey(L"command_list"))
				sb << L", commandBindableNV";
			sb << L") ";
			sb << bufferType;
			sb << L" " << block->Name << L"\n{\n";
			for (auto & ent : block->Entries)
			{
				if (!useBindlessTexture && ent.Value.Type->IsTexture())
					continue;
				sb << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
			}
			sb << L"} blk" << block->Name << L";\n";
		}
		if (!useBindlessTexture)
		{
			int bindPoint = 0;
			String bindingStart;
			if (ctx.BackendArguments.TryGetValue(L"TextureBindingStart", bindingStart))
				bindPoint = StringToInt(bindingStart);
			for (auto & ent : block->Entries)
			{
				if (ent.Value.Type->IsTexture())
				{
					sb << L"layout(binding = " << bindPoint << L") uniform " << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
					bindPoint++;
				}
			}
		}
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & /*sb*/, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		auto block = ctx.SourceWorld->WorldOutput;
		if (ctx.BackendArguments.ContainsKey(L"bindless_texture") || !instr->Type->IsTexture())
			instr->Name = L"blk" + block->Name + L"." + instr->ComponentName;
		else
			instr->Name = instr->ComponentName;
	}
};


class TextureGLSLImportOperatorHandler : public GLSLImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"textureImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & ctx) override
	{
		if (block->Entries.Count() == 0)
			return;
		sb << L"layout(std140";
		String strIndex;
		if (block->Attributes.TryGetValue(L"Index", strIndex))
			sb << L", binding = " << strIndex;
		if (ctx.BackendArguments.ContainsKey(L"command_list"))
			sb << L", commandBindableNV";
		sb << L") ";
		sb << L"uniform " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			sb << L"sampler2D " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & context) override
	{
		if (instr->Arguments.Count() != 1)
		{
			context.Result.GetErrorWriter()->Error(50001, L"missing import operator argument.", instr->ImportOperator->Position);
			return;
		}
		sb << instr->Type->ToString() << L" " << instr->Name << L" = " << instr->Type->ToString() << "(texture(blk" << context.SourceWorld->WorldOutput->Name
			<< L"."<< instr->ComponentName << L", " << instr->Arguments[0]->Name << L")";
		int vecSize = instr->Type->GetVectorSize();
		if (vecSize <= 1)
			sb << L".x";
		else if (vecSize == 2)
			sb << L".xy";
		else if (vecSize == 3)
			sb << L".xyz";
		CompiledComponent ccomp;
		if (context.SourceWorld->LocalComponents.TryGetValue(instr->ComponentName, ccomp))
		{
			if (ccomp.Attributes.ContainsKey(L"Normal"))
			{
				sb << L" * 2.0 - 1.0";
			}
		}
		sb << L");\n";
	}
};

class StandardGLSLExportOperatorHandler : public ExportOperatorHandler
{
	virtual String GetName() override
	{
		return L"standardExport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block) override
	{
		if (block->Entries.Count() == 0)
			return;
		sb << L"out " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			if (ent.Value.Type->IsIntegral())
				sb << L"flat ";
			sb << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateExport(StringBuilder & sb, InterfaceBlock * block, CompiledWorld *, String componentName, String valueVar) override
	{
		sb << L"blk" << block->Name << L"." << componentName << L" = " << valueVar << L";\n";
	}
	virtual void GeneratePreamble(StringBuilder &, InterfaceBlock *) override
	{}
	virtual void GenerateEpilogue(StringBuilder &, InterfaceBlock *) override
	{}
};

class FragmentGLSLExportOperatorHandler : public ExportOperatorHandler
{
	virtual String GetName() override
	{
		return L"fragmentExport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block) override
	{
		if (block->Entries.Count() == 0)
			return;
		int idx = 0;
		for (auto & ent : block->Entries)
		{
			if (!ent.Value.LayoutAttribs.ContainsKey(L"DepthOutput"))
			{
				sb << L"layout(location = " << idx << L") out " << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
				idx++;
			}
		}
	}
	virtual void GenerateExport(StringBuilder & sb, InterfaceBlock * /*block*/, CompiledWorld * world, String componentName, String valueVar) override
	{
		CompiledComponent ccomp;
		bool isNormal = false;
		bool isDepthOutput = false;
		if (world->LocalComponents.TryGetValue(componentName, ccomp))
		{
			if (ccomp.Attributes.ContainsKey(L"Normal"))
				isNormal = true;
			if (ccomp.Attributes.ContainsKey(L"DepthOutput"))
				isDepthOutput = true;
		}
		if (isDepthOutput)
			sb << L"gl_FragDepth";
		else
			sb << componentName;
		sb << L" = ";
		if (isNormal)
			sb << valueVar << L" * 0.5 + 0.5";
		else
			sb << valueVar;
		sb << L";\n";
	}	
	virtual void GeneratePreamble(StringBuilder &, InterfaceBlock *) override
	{}
	virtual void GenerateEpilogue(StringBuilder &, InterfaceBlock *) override
	{}
};


void CreateGLSLImportOperatorHandlers(CoreLib::Basic::List<Spire::Compiler::ImportOperatorHandler *> & handlers)
{
	handlers.Add(new StandardGLSLImportOperatorHandler());
	handlers.Add(new UniformGLSLImportOperatorHandler());
	handlers.Add(new TextureGLSLImportOperatorHandler());
	handlers.Add(new VertexGLSLImportOperatorHandler());
	handlers.Add(new BufferGLSLImportOperatorHandler());
}

void CreateGLSLExportOperatorHandlers(CoreLib::Basic::List<Spire::Compiler::ExportOperatorHandler *> & handlers)
{
	handlers.Add(new StandardGLSLExportOperatorHandler());
	handlers.Add(new FragmentGLSLExportOperatorHandler());
	handlers.Add(new BufferGLSLExportOperatorHandler());
}

