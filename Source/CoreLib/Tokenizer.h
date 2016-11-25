#ifndef CORELIB_TEXT_PARSER_H
#define CORELIB_TEXT_PARSER_H

#include "Basic.h"

namespace CoreLib
{
	namespace Text
	{
		class TextFormatException : public Exception
		{
		public:
			TextFormatException(String message)
				: Exception(message)
			{}
		};

		class CodePosition
		{
		public:
			int Line = -1, Col = -1, Pos = -1;
			String FileName;
			String ToString()
			{
				StringBuilder sb(100);
				sb << FileName;
				if (Line != -1)
					sb << L"(" << Line << L")";
				return sb.ProduceString();
			}
			CodePosition() = default;
			CodePosition(int line, int col, int pos, String fileName)
			{
				Line = line;
				Col = col;
				Pos = pos;
				this->FileName = fileName;
			}
			bool operator < (const CodePosition & pos) const
			{
				return FileName < pos.FileName || (FileName == pos.FileName && Line < pos.Line) ||
					(FileName == pos.FileName && Line == pos.Line && Col < pos.Col);
			}
			bool operator == (const CodePosition & pos) const
			{
				return FileName == pos.FileName && Line == pos.Line && Col == pos.Col;
			}
		};

		enum class TokenType
		{
			// illegal
			Unknown,
			// identifier
			Identifier,
			// constant
			IntLiterial, DoubleLiterial, StringLiterial, CharLiterial,
			// operators
			Semicolon, Comma, Dot, LBrace, RBrace, LBracket, RBracket, LParent, RParent,
			OpAssign, OpAdd, OpSub, OpMul, OpDiv, OpMod, OpNot, OpBitNot, OpLsh, OpRsh,
			OpEql, OpNeq, OpGreater, OpLess, OpGeq, OpLeq,
			OpAnd, OpOr, OpBitXor, OpBitAnd, OpBitOr,
			OpInc, OpDec, OpAddAssign, OpSubAssign, OpMulAssign, OpDivAssign, OpModAssign,
			OpShlAssign, OpShrAssign, OpOrAssign, OpAndAssign, OpXorAssign,

			QuestionMark, Colon, RightArrow, At,
		};

		String TokenTypeToString(TokenType type);

		class Token
		{
		public:
			TokenType Type = TokenType::Unknown;
			String Content;
			CodePosition Position;
			Token() = default;
			Token(TokenType type, const String & content, int line, int col, int pos, String fileName)
			{
				Type = type;
				Content = content;
				Position = CodePosition(line, col, pos, fileName);
			}
		};

		enum class TokenizeErrorType
		{
			InvalidCharacter, InvalidEscapeSequence
		};

		List<Token> TokenizeText(const String & fileName, const String & text, Procedure<TokenizeErrorType, CodePosition> errorHandler);
		List<Token> TokenizeText(const String & fileName, const String & text);
		List<Token> TokenizeText(const String & text);
		
		String EscapeStringLiteral(String str);
		String UnescapeStringLiteral(String str);

		class TokenReader
		{
		private:
			bool legal;
			List<Token> tokens;
			int tokenPtr;
		public:
			TokenReader(Basic::String text);
			int ReadInt()
			{
				auto token = ReadToken();
				bool neg = false;
				if (token.Content == L'-')
				{
					neg = true;
					token = ReadToken();
				}
				if (token.Type == TokenType::IntLiterial)
				{
					if (neg)
						return -StringToInt(token.Content);
					else
						return StringToInt(token.Content);
				}
				throw TextFormatException(L"Text parsing error: int expected.");
			}
			unsigned int ReadUInt()
			{
				auto token = ReadToken();
				if (token.Type == TokenType::IntLiterial)
				{
					return StringToUInt(token.Content);
				}
				throw TextFormatException(L"Text parsing error: int expected.");
			}
			double ReadDouble()
			{
				auto token = ReadToken();
				bool neg = false;
				if (token.Content == L'-')
				{
					neg = true;
					token = ReadToken();
				}
				if (token.Type == TokenType::DoubleLiterial || token.Type == TokenType::IntLiterial)
				{
					if (neg)
						return -StringToDouble(token.Content);
					else
						return StringToDouble(token.Content);
				}
				throw TextFormatException(L"Text parsing error: floating point value expected.");
			}
			String ReadWord()
			{
				auto token = ReadToken();
				if (token.Type == TokenType::Identifier)
				{
					return token.Content;
				}
				throw TextFormatException(L"Text parsing error: identifier expected.");
			}
			String Read(const wchar_t * expectedStr)
			{
				auto token = ReadToken();
				if (token.Content == expectedStr)
				{
					return token.Content;
				}
				throw TextFormatException(L"Text parsing error: \'" + String(expectedStr) + L"\' expected.");
			}
			String Read(String expectedStr)
			{
				auto token = ReadToken();
				if (token.Content == expectedStr)
				{
					return token.Content;
				}
				throw TextFormatException(L"Text parsing error: \'" + expectedStr + L"\' expected.");
			}
			
			String ReadStringLiteral()
			{
				auto token = ReadToken();
				if (token.Type == TokenType::StringLiterial)
				{
					return UnescapeStringLiteral(token.Content.SubString(1, token.Content.Length()-2));
				}
				throw TextFormatException(L"Text parsing error: string literal expected.");
			}
			void Back(int count)
			{
				tokenPtr -= count;
			}
			Token ReadToken()
			{
				if (tokenPtr < tokens.Count())
				{
					auto &rs = tokens[tokenPtr];
					tokenPtr++;
					return rs;
				}
				throw TextFormatException(L"Unexpected ending.");
			}
			Token NextToken()
			{
				if (tokenPtr < tokens.Count())
					return tokens[tokenPtr];
				else
				{
					Token rs;
					rs.Type = TokenType::Unknown;
					return rs;
				}
			}
			bool LookAhead(String token)
			{
				if (tokenPtr < tokens.Count())
				{
					auto next = NextToken();
					return next.Content == token;
				}
				else
				{
					return false;
				}
			}
			bool IsEnd()
			{
				return tokenPtr == tokens.Count();
			}
		public:
			bool IsLegalText()
			{
				return legal;
			}
		};

		List<String> Split(String str, wchar_t c);
	}
}

#endif