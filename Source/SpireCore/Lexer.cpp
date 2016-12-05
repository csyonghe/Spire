#include "Lexer.h"
#include "../CoreLib/Tokenizer.h"

namespace Spire
{
	namespace Compiler
	{
		List<Token> Lexer::Parse(const String & fileName, const String & str, DiagnosticSink * sink)
		{
			return CoreLib::Text::TokenizeText(fileName, str, [&](CoreLib::Text::TokenizeErrorType errType, CoreLib::Text::CodePosition pos)
			{
				auto curChar = str[pos.Pos];
				switch (errType)
				{
				case CoreLib::Text::TokenizeErrorType::InvalidCharacter:
					sink->diagnose(pos, Diagnostics::illegalCharacter, String((unsigned char)curChar, 16));
					break;
				case CoreLib::Text::TokenizeErrorType::InvalidEscapeSequence:
					sink->diagnose(pos, Diagnostics::illegalCharacterLiteral);
					break;
				default:
					break;
				}
			});
		}
	}
}