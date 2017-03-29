#ifndef CODE_GEN_BACKEND_H
#define CODE_GEN_BACKEND_H

#include "../CoreLib/Basic.h"
#include "CompiledProgram.h"
#include "SymbolTable.h"
#include "TypeLayout.h"

namespace Spire
{
	namespace Compiler
	{		
		class CodeGenBackend : public CoreLib::Basic::Object
		{
		public:
			virtual CompiledShaderSource GenerateShader(CompileResult & result, SymbolTable * symbols, ILShader * shader, DiagnosticSink * err) = 0;
            virtual LayoutRule GetDefaultLayoutRule() = 0;
		};

		CodeGenBackend * CreateGLSLCodeGen();
		CodeGenBackend * CreateGLSL_VulkanCodeGen();
		CodeGenBackend * CreateGLSL_VulkanOneDescCodeGen();
		CodeGenBackend * CreateHLSLCodeGen();
		CodeGenBackend * CreateSpirVCodeGen();
	}
}

#endif