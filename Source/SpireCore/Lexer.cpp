#include "Lexer.h"
#include "../CoreLib/Tokenizer.h"

namespace Spire
{
	namespace Compiler
	{
		List<Token> Lexer::Parse(const String & fileName, const String & str, List<CompileError> & errorList)
		{
			return CoreLib::Text::TokenizeText(fileName, str, [&](CoreLib::Text::TokenizeErrorType errType, CoreLib::Text::CodePosition pos)
			{
				auto curChar = str[pos.Pos];
				switch (errType)
				{
				case CoreLib::Text::TokenizeErrorType::InvalidCharacter:
					errorList.Add(CompileError(L"Illegal character '\\x" + String((int)curChar, 16) + L"'", 10000, pos));
					break;
				case CoreLib::Text::TokenizeErrorType::InvalidEscapeSequence:
					errorList.Add(CompileError(L"Illegal character literial.", 10001, pos));
					break;
				default:
					break;
				}
			});
		}
	}
}