#ifndef BAKERSL_SHADER_CLOSURE_H
#define BAKERSL_SHADER_CLOSURE_H
#include "SymbolTable.h"

namespace Spire
{
	namespace Compiler
	{
		RefPtr<ShaderClosure> CreateShaderClosure(ErrorWriter * err, SymbolTable * symTable, ShaderSymbol * shader);
		void FlattenShaderClosure(ErrorWriter * err, ShaderClosure * shader);
		void InsertImplicitImportOperators(ShaderIR * shader);
	}
}

#endif