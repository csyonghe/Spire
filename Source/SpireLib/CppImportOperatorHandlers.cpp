#include "ImportOperator.h"
using namespace Spire::Compiler;
using namespace CoreLib::Basic;

class CppImportOperatorHandler : public ImportOperatorHandler
{
public:
	virtual void GenerateSetInput(StringBuilder &, ComponentDefinition *, const ImportOperatorContext &) override
	{}
	virtual void GeneratePreamble(StringBuilder &, InterfaceBlock *, const ImportOperatorContext &) override
	{}
	virtual void GenerateEpilogue(StringBuilder &, InterfaceBlock *, const ImportOperatorContext &) override
	{}
};

class StandardCppImportOperatorHandler : public CppImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"standardImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext &) override
	{
		sb << L"in " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			sb << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		auto block = ctx.SourceWorld->WorldOutput;
		instr->Name = L"blk" + block->Name + L"." + instr->ComponentName;
	}
};

class VertexCppImportOperatorHandler : public CppImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"vertexImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & ctx) override
	{
		int location = 0;
		for (auto & ent : block->Entries)
		{
			sb << L"layout(location = " << location << L") ";
			sb << L"in " << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
			location++;
		}
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder &sb, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		instr->Name = instr->ComponentName;
	}
};

class UniformCppImportOperatorHandler : public CppImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"uniformImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & ctx) override
	{
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
			sb << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		auto block = ctx.SourceWorld->WorldOutput;
		instr->Name = L"blk" + block->Name + L"." + instr->ComponentName;
	}
};


class TextureCppImportOperatorHandler : public CppImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"textureImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & ctx) override
	{
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
			<< L"." << instr->ComponentName << L", " << instr->Arguments[0]->Name << L")";
		int vecSize = instr->Type->GetVectorSize();
		if (vecSize <= 1)
			sb << L".x";
		else if (vecSize == 2)
			sb << L".xy";
		else if (vecSize == 3)
			sb << L".xyz";
		sb << L");\n";
	}
};

class StandardExportOperatorHandler : public ExportOperatorHandler
{
	virtual String GetName() override
	{
		return L"standardExport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block) override
	{
		sb << L"out " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			sb << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateExport(StringBuilder & sb, InterfaceBlock * block, CompiledWorld *, String componentName, String valueVar) override
	{
		sb << L"blk" << block->Name << L"." << componentName << L" = " << valueVar << L";\n";
	}
};


void CreateCppImportOperatorHandlers(CoreLib::Basic::List<Spire::Compiler::ImportOperatorHandler *> & handlers)
{
	handlers.Add(new StandardCppImportOperatorHandler());
	handlers.Add(new UniformCppImportOperatorHandler());
	handlers.Add(new TextureCppImportOperatorHandler());
}
