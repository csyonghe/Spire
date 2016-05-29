#ifndef CODE_GEN_BACKEND_H
#define CODE_GEN_BACKEND_H

#include "CoreLib/Basic.h"
#include "CompiledProgram.h"
#include "SymbolTable.h"

namespace Spire
{
	namespace Compiler
	{
		class ImportOperatorContext
		{
		public:
			EnumerableDictionary<String, String> & Arguments;
			EnumerableDictionary<String, String> & BackendArguments;
			CompiledWorld * SourceWorld, * DestWorld;
			CompileResult & Result;
			ImportOperatorContext(EnumerableDictionary<String, String> & args,
				EnumerableDictionary<String, String> & backendArgs,
				CompiledWorld * destWorld,
				CompileResult & result, CompiledWorld * srcWorld)
				: Arguments(args), BackendArguments(backendArgs), DestWorld(destWorld),
				Result(result), SourceWorld(srcWorld)
			{}
		};
		class ImportOperatorHandler
		{
		public:
			virtual String GetName() = 0;
			virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & context) = 0;
			virtual void GeneratePreamble(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & context) = 0;
			virtual void GenerateEpilogue(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & context) = 0;
			virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & context) = 0;
			virtual void GenerateSetInput(StringBuilder & sb, ComponentDefinition * gvar, const ImportOperatorContext & context) = 0;
		};

		class ExportOperatorHandler
		{
		public:
			virtual String GetName() = 0;
			virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block) = 0;
			virtual void GeneratePreamble(StringBuilder & sb, InterfaceBlock * block) = 0;
			virtual void GenerateEpilogue(StringBuilder & sb, InterfaceBlock * block) = 0;
			virtual void GenerateExport(StringBuilder & sb, InterfaceBlock * block, CompiledWorld * world, String compName, String valueVar) = 0;
		};

		class CodeGenBackend : public CoreLib::Basic::Object
		{
		public:
			virtual CompiledShaderSource GenerateShaderWorld(CompileResult & result, SymbolTable * symbols, CompiledWorld * shader,
				Dictionary<String, ImportOperatorHandler *> & opHandlers,
				Dictionary<String, ExportOperatorHandler *> & exportHandlers) = 0;
			virtual void SetParameters(EnumerableDictionary<String, String> & arguments) = 0;
		};

		CodeGenBackend * CreateGLSLCodeGen();
	}
}

#endif