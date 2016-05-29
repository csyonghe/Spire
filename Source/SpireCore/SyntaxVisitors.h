#ifndef RASTER_RENDERER_SYNTAX_PRINTER_H
#define RASTER_RENDERER_SYNTAX_PRINTER_H

#include "CompileError.h"
#include "Syntax.h"
#include "IL.h"
#include "SymbolTable.h"

namespace Spire
{
	namespace Compiler
	{
		class ShaderCompiler;
		class ShaderLinkInfo;
		class ShaderSymbol;

		class ICodeGenerator : public SyntaxVisitor
		{
		public:
			ICodeGenerator(ErrorWriter * perr)
				: SyntaxVisitor(perr)
			{}
			virtual void ProcessFunction(FunctionSyntaxNode * func) = 0;
			virtual void ProcessShader(ShaderClosure * shader) = 0;
		};

		SyntaxVisitor * CreateComponentDependencyVisitor(SymbolTable * symbols, ShaderSymbol * currentShader, 
			ShaderComponentSymbol * compSym,
			ErrorWriter * err,
			EnumerableHashSet<ShaderComponentSymbol *> & _dependentComponents,
			Dictionary<ShaderComponentSymbol*, SyntaxNode*> & _referenceNodes);
		SyntaxVisitor * CreateSemanticsVisitor(SymbolTable * symbols, ErrorWriter * err);
		ICodeGenerator * CreateCodeGenerator(ShaderCompiler * compiler, SymbolTable * symbols, CompileResult & result);
	}
}

#endif