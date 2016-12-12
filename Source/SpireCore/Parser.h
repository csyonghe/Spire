#ifndef RASTER_RENDERER_PARSER_H
#define RASTER_RENDERER_PARSER_H

#include "Lexer.h"
#include "Syntax.h"

namespace Spire
{
	namespace Compiler
	{
        RefPtr<ProgramSyntaxNode> ParseProgram(
            TokenSpan const&    tokens,
            DiagnosticSink*     sink,
            String const&       fileName);
	}
}

#endif