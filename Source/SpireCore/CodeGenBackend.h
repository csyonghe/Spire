#ifndef CODE_GEN_BACKEND_H
#define CODE_GEN_BACKEND_H

#include "../CoreLib/Basic.h"
#include "CompiledProgram.h"
#include "SymbolTable.h"

namespace Spire
{
	namespace Compiler
	{		
		class CodeGenBackend : public CoreLib::Basic::Object
		{
		public:
			virtual CompiledShaderSource GenerateShader(CompileResult & result, SymbolTable * symbols, ILShader * shader, DiagnosticSink * err) = 0;
		};

		CodeGenBackend * CreateGLSLCodeGen();
		CodeGenBackend * CreateGLSL_VulkanCodeGen();
		CodeGenBackend * CreateHLSLCodeGen();
		CodeGenBackend * CreateSpirVCodeGen();
	}
}

#endif