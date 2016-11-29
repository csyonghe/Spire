#ifndef BAKERSL_SHADER_CLOSURE_H
#define BAKERSL_SHADER_CLOSURE_H
#include "SymbolTable.h"

namespace Spire
{
	namespace Compiler
	{
		RefPtr<ShaderClosure> CreateShaderClosure(DiagnosticSink * sink, SymbolTable * symTable, ShaderSymbol * shader);
		void FlattenShaderClosure(DiagnosticSink * sink, SymbolTable * symTable, ShaderClosure * shader);
		void InsertImplicitImportOperators(DiagnosticSink * sink, ShaderIR * shader);
	}
}

#endif