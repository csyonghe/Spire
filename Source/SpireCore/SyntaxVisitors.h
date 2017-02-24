#ifndef RASTER_RENDERER_SYNTAX_PRINTER_H
#define RASTER_RENDERER_SYNTAX_PRINTER_H

#include "Diagnostics.h"
#include "Syntax.h"
#include "IL.h"
#include "CompiledProgram.h"

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
            virtual void ProcessStruct(StructSyntaxNode * st) = 0;
            virtual void ProcessGlobalVar(VarDeclBase * var) = 0;
        };

        SyntaxVisitor * CreateSemanticsVisitor(DiagnosticSink * err);
        ICodeGenerator * CreateCodeGenerator(CompileResult & result);
		class CompileOptions;
        SyntaxVisitor * CreateILCodeGenerator(DiagnosticSink * err, ILProgram * program, CompileOptions * options);
    }
}

#endif