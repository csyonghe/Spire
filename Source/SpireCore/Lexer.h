#ifndef RASTER_RENDERER_LEXER_H
#define RASTER_RENDERER_LEXER_H

#include "../CoreLib/Basic.h"
#include "CompileError.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;
		
		class Lexer
		{
		public:
			List<Token> Parse(const String & fileName, const String & str, DiagnosticSink * sink);
		};
	}
}

#endif