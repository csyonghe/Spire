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
			virtual void ProcessStruct(StructSyntaxNode * st) = 0;
		};

		SyntaxVisitor * CreateSemanticsVisitor(SymbolTable * symbols, ErrorWriter * err);
		ICodeGenerator * CreateCodeGenerator(SymbolTable * symbols, CompileResult & result);
		RefPtr<ILType> TranslateExpressionType(const ExpressionType & type);
	}
}

#endif