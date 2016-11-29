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
					sink->Error(10000, "Illegal character '\\x" + String((unsigned char)curChar, 16) + "'", pos);
					break;
				case CoreLib::Text::TokenizeErrorType::InvalidEscapeSequence:
					sink->Error(10001, "Illegal character literial.", pos);
					break;
				default:
					break;
				}
			});
		}
	}
}