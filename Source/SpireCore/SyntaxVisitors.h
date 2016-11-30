#ifndef RASTER_RENDERER_SYNTAX_PRINTER_H
#define RASTER_RENDERER_SYNTAX_PRINTER_H

#include "Diagnostics.h"
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
			ICodeGenerator(DiagnosticSink * perr)
				: SyntaxVisitor(perr)
			{}
			virtual void ProcessFunction(FunctionSyntaxNode * func) = 0;
			virtual void ProcessShader(ShaderIR * shader) = 0;
			virtual void ProcessStruct(StructSyntaxNode * st) = 0;
		};

		SyntaxVisitor * CreateSemanticsVisitor(SymbolTable * symbols, DiagnosticSink * err);
		ICodeGenerator * CreateCodeGenerator(SymbolTable * symbols, CompileResult & result);
	}
}

#endif