#ifndef RASTER_RENDERER_LEXER_H
#define RASTER_RENDERER_LEXER_H

#include "CoreLib/Basic.h"
#include "CompileError.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;

		enum class TokenType
		{
			// illegal
			Unkown,
			// identifier
			Identifier,
			KeywordReturn, KeywordBreak, KeywordContinue,
			KeywordIf, KeywordElse, KeywordFor, KeywordWhile, KeywordDo,
			// constant
			IntLiterial, DoubleLiterial, StringLiterial, CharLiterial,
			// operators
			Semicolon, Comma, Dot, LBrace, RBrace, LBracket, RBracket, LParent, RParent,
			OpAssign, OpAdd, OpSub, OpMul, OpDiv, OpMod, OpNot, OpBitNot, OpLsh, OpRsh, 
			OpEql, OpNeq, OpGreater, OpLess, OpGeq, OpLeq,
			OpAnd, OpOr, OpBitXor, OpBitAnd, OpBitOr,
			OpInc, OpDec, OpAddAssign, OpSubAssign, OpMulAssign, OpDivAssign, OpModAssign,
			
			QuestionMark, Colon, RightArrow, At,
		};

		String TokenTypeToString(TokenType type);

		class Token
		{
		public:
			TokenType Type = TokenType::Unkown;
			String Content;
			CodePosition Position;
			Token() = default;
			Token(TokenType type, const String & content, int line, int col, String fileName)
			{
				Type = type;
				Content = content;
				Position = CodePosition(line, col, fileName);
			}
		};

		class Lexer
		{
		public:
			List<Token> Parse(const String & fileName, const String & str, List<CompileError> & errorList);
		};
	}
}

#endif