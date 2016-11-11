#include "Lexer.h"
#include <stdio.h>

namespace Spire
{
	namespace Compiler
	{
		enum class State
		{
			Start, Identifier, Operator, Int, Fixed, Double, Char, String, MultiComment, SingleComment
		};

		bool IsLetter(wchar_t ch)
		{
			return ((ch >= L'a' && ch <= L'z') ||
				(ch >= L'A' && ch <= L'Z') || ch == L'_' || ch == L'#');
		}

		bool IsDigit(wchar_t ch)
		{
			return ch >= L'0' && ch <= L'9';
		}

		bool IsPunctuation(wchar_t ch)
		{
			return  ch == L'+' || ch == L'-' || ch == L'*' || ch == L'/' || ch == L'%' ||
					ch == L'!' || ch == L'^' || ch == L'&' || ch == L'(' || ch == L')' ||
					ch == L'=' || ch == L'{' || ch == L'}' || ch == L'[' || ch == L']' ||
					ch == L'|' || ch == L';' || ch == L',' || ch == L'.' || ch == L'<' ||
					ch == L'>' || ch == L'~' || ch == L'@' || ch == L':' || ch == L'?';
		}

		TokenType GetKeywordTokenType(const String & str)
		{
			if (str == L"return")
				return TokenType::KeywordReturn;
			else if (str == L"break")
				return TokenType::KeywordBreak;
			else if (str == L"continue")
				return TokenType::KeywordContinue;
			else if (str == L"if")
				return TokenType::KeywordIf;
			else if (str == L"else")
				return TokenType::KeywordElse;
			else if (str == L"for")
				return TokenType::KeywordFor;
			else if (str == L"while")
				return TokenType::KeywordWhile;
			else if (str == L"do")
				return TokenType::KeywordDo;
			else
				return TokenType::Identifier;
		}

		void ParseOperators(const String & str, List<Token> & tokens, int line, int col, String fileName)
		{
			int pos = 0;
			while (pos < str.Length())
			{
				wchar_t curChar = str[pos];
				wchar_t nextChar = (pos < str.Length()-1)? str[pos + 1] : L'\0';
				wchar_t nextNextChar = (pos < str.Length() - 2) ? str[pos + 2] : L'\0';
				auto InsertToken = [&](TokenType type, const String & ct)
				{
					tokens.Add(Token(type, ct, line, col + pos, fileName));
				};
				switch(curChar)
				{
				case L'+':
					if (nextChar == L'+')
					{
						InsertToken(TokenType::OpInc, L"++");
						pos += 2;
					}
					else if (nextChar == L'=')
					{
						InsertToken(TokenType::OpAddAssign, L"+=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpAdd, L"+");
						pos++;
					}
					break;
				case L'-':
					if (nextChar == L'-')
					{
						InsertToken(TokenType::OpDec, L"--");
						pos += 2;
					}
					else if (nextChar == L'=')
					{
						InsertToken(TokenType::OpSubAssign, L"-=");
						pos += 2;
					}
					else if (nextChar == L'>')
					{
						InsertToken(TokenType::RightArrow, L"->");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpSub, L"-");
						pos++;
					}
					break;
				case L'*':
					if (nextChar == L'=')
					{
						InsertToken(TokenType::OpMulAssign, L"*=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpMul, L"*");
						pos++;
					}
					break;
				case L'/':
					if (nextChar == L'=')
					{
						InsertToken(TokenType::OpDivAssign, L"/=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpDiv, L"/");
						pos++;
					}
					break;
				case L'%':
					if (nextChar == L'=')
					{
						InsertToken(TokenType::OpModAssign, L"%=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpMod, L"%");
						pos++;
					}
					break;
				case L'|':
					if (nextChar == L'|')
					{
						InsertToken(TokenType::OpOr, L"||");
						pos += 2;
					}
					else if (nextChar == L'=')
					{
						InsertToken(TokenType::OpOrAssign, L"|=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpBitOr, L"|");
						pos++;
					}
					break;
				case L'&':
					if (nextChar == L'&')
					{
						InsertToken(TokenType::OpAnd, L"&&");
						pos += 2;
					}
					else if (nextChar == L'=')
					{
						InsertToken(TokenType::OpAndAssign, L"&=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpBitAnd, L"&");
						pos++;
					}
					break;
				case L'^':
					if (nextChar == L'=')
					{
						InsertToken(TokenType::OpXorAssign, L"^=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpBitXor, L"^");
						pos++;
					}
					break;
				case L'>':
					if (nextChar == L'>')
					{
						if (nextNextChar == L'=')
						{
							InsertToken(TokenType::OpShrAssign, L">>=");
							pos += 3;
						}
						else
						{
							InsertToken(TokenType::OpRsh, L">>");
							pos += 2;
						}
					}
					else if (nextChar == L'=')
					{
						InsertToken(TokenType::OpGeq, L">=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpGreater, L">");
						pos++;
					}
					break;
				case L'<':
					if (nextChar == L'<')
					{
						if (nextNextChar == L'=')
						{
							InsertToken(TokenType::OpShlAssign, L"<<=");
							pos += 3;
						}
						else
						{
							InsertToken(TokenType::OpLsh, L"<<");
							pos += 2;
						}
					}
					else if (nextChar == L'=')
					{
						InsertToken(TokenType::OpLeq, L"<=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpLess, L"<");
						pos++;
					}
					break;
				case L'=':
					if (nextChar == L'=')
					{
						InsertToken(TokenType::OpEql, L"==");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpAssign, L"=");
						pos++;
					}
					break;
				case L'!':
					if (nextChar == L'=')
					{
						InsertToken(TokenType::OpNeq, L"!=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpNot, L"!");
						pos++;
					}
					break;
				case L'?':
					InsertToken(TokenType::QuestionMark, L"?");
					pos++;
					break;
				case L'@':
					InsertToken(TokenType::At, L"@");
					pos++;
					break;
				case L':':
					InsertToken(TokenType::Colon, L":");
					pos++;
					break;
				case L'~':
					InsertToken(TokenType::OpBitNot, L"~");
					pos++;
					break;
				case L';':
					InsertToken(TokenType::Semicolon, L";");
					pos++;
					break;
				case L',':
					InsertToken(TokenType::Comma, L","); 
					pos++;
					break;
				case L'.':
					InsertToken(TokenType::Dot, L".");
					pos++;
					break;
				case L'{':
					InsertToken(TokenType::LBrace, L"{"); 
					pos++;
					break;
				case L'}':
					InsertToken(TokenType::RBrace, L"}"); 
					pos++;
					break;
				case L'[':
					InsertToken(TokenType::LBracket, L"["); 
					pos++;
					break;
				case L']':
					InsertToken(TokenType::RBracket, L"]"); 
					pos++;
					break;
				case L'(':
					InsertToken(TokenType::LParent, L"("); 
					pos++;
					break;
				case L')':
					InsertToken(TokenType::RParent, L")"); 
					pos++;
					break;
				}
			}
		}

		enum class LexDerivative
		{
			None, Line, File
		};

		List<Token> Lexer::Parse(const String & fileName, const String & str, List<CompileError> & errorList)
		{
			int lastPos = 0, pos = 0;
			int line = 1, col = 0;
			String file = fileName;
			State state = State::Start;
			StringBuilder tokenBuilder;
			int tokenLine, tokenCol;
			List<Token> tokenList;
			LexDerivative derivative = LexDerivative::None;
			auto InsertToken = [&](TokenType type)
			{
				derivative = LexDerivative::None;
				tokenList.Add(Token(type, tokenBuilder.ToString(), tokenLine, tokenCol, file));
				tokenBuilder.Clear();
			};
			auto ProcessTransferChar = [&](wchar_t nextChar)
			{
				switch(nextChar)
				{
				case L'\\':
				case L'\"':
				case L'\'':
					tokenBuilder.Append(nextChar);
					break;
				case L't':
					tokenBuilder.Append('\t');
					break;
				case L's':
					tokenBuilder.Append(' ');
					break;
				case L'n':
					tokenBuilder.Append('\n');
					break;
				case L'r':
					tokenBuilder.Append('\r');
					break;
				case L'b':
					tokenBuilder.Append('\b');
					break;
				}
			};
			while (pos <= str.Length())
			{
				wchar_t curChar = (pos < str.Length()?str[pos]:L' ');
				wchar_t nextChar = (pos < str.Length()-1)? str[pos + 1] : L'\0';
				if (lastPos != pos)
				{
					if (curChar == L'\n')
					{
						line++;
						col = 0;
					}
					else
						col++;
					lastPos = pos;
				}

				switch (state)
				{
				case State::Start:
					if (IsLetter(curChar))
					{
						state = State::Identifier;
						tokenLine = line;
						tokenCol = col;
					}
					else if (IsDigit(curChar))
					{
						state = State::Int;
						tokenLine = line;
						tokenCol = col;
					}
					else if (curChar == L'\'')
					{
						state = State::Char;
						pos++;
						tokenLine = line;
						tokenCol = col;
					}
					else if (curChar == L'"')
					{
						state = State::String;
						pos++;
						tokenLine = line;
						tokenCol = col;
					}
					else if (curChar == L' ' || curChar == L'\t' || curChar == L'\r' || curChar == L'\n' || curChar == 160) // 160:non-break space
						pos++;
					else if (curChar == L'/' && nextChar == L'/')
					{
						state = State::SingleComment;
						pos += 2;
					}
					else if (curChar == L'/' && nextChar == L'*')
					{
						pos += 2;
						state = State::MultiComment;
					}
					else if (IsPunctuation(curChar))
					{
						state = State::Operator;
						tokenLine = line;
						tokenCol = col;
					}
					else
					{
						errorList.Add(CompileError(L"Illegal character '" + String(curChar) + L"'", 10000, CodePosition(line, col, file)));
						pos++;
					}
					break;
				case State::Identifier:
					if (IsLetter(curChar) || IsDigit(curChar))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else
					{
						auto tokenStr = tokenBuilder.ToString();
						if (tokenStr == L"#line_reset#")
						{
							line = 0;
							col = 0;
							tokenBuilder.Clear();
						}
						else if (tokenStr == L"#line")
						{
							derivative = LexDerivative::Line;
							tokenBuilder.Clear();
						}
						else if (tokenStr == L"#file")
						{
							derivative = LexDerivative::File;
							tokenBuilder.Clear();
							line = 0;
							col = 0;
						}
						else
							InsertToken(GetKeywordTokenType(tokenStr));
						state = State::Start;
					}
					break;
				case State::Operator:
					if (IsPunctuation(curChar) && !((curChar == L'/' && nextChar == L'/') || (curChar == L'/' && nextChar == L'*')))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else
					{
						//do token analyze
						ParseOperators(tokenBuilder.ToString(), tokenList, tokenLine, tokenCol, file);
						tokenBuilder.Clear();
						state = State::Start;
					}
					break;
				case State::Int:
					if (IsDigit(curChar))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else if (curChar == L'.')
					{
						state = State::Fixed;
						tokenBuilder.Append(curChar);
						pos++;
					}
					else if (curChar == L'e' || curChar == L'E')
					{
						state = State::Double;
						tokenBuilder.Append(curChar);
						if (nextChar == L'-' || nextChar == L'+')
						{
							tokenBuilder.Append(nextChar);
							pos++;
						}
						pos++;
					}
					else
					{
						if (derivative == LexDerivative::Line)
						{
							derivative = LexDerivative::None;
							line = StringToInt(tokenBuilder.ToString()) - 1;
							col = 0;
							tokenBuilder.Clear();
						}
						else
						{
							InsertToken(TokenType::IntLiterial);
						}
						state = State::Start;
					}
					break;
				case State::Fixed:
					if (IsDigit(curChar))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else if (curChar == L'e' || curChar == L'E')
					{
						state = State::Double;
						tokenBuilder.Append(curChar);
						if (nextChar == L'-' || nextChar == L'+')
						{
							tokenBuilder.Append(nextChar);
							pos++;
						}
						pos++;
					}
					else
					{
						if (curChar == L'f')
							pos++;
						InsertToken(TokenType::DoubleLiterial);
						state = State::Start;
					}
					break;
				case State::Double:
					if (IsDigit(curChar))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else
					{
						if (curChar == L'f')
							pos++;
						InsertToken(TokenType::DoubleLiterial);
						state = State::Start;
					}
					break;
				case State::String:
					if (curChar != L'"')
					{
						if (curChar == L'\\')
						{
							ProcessTransferChar(nextChar);
							pos++;
						}
						else
							tokenBuilder.Append(curChar);
					}
					else
					{
						if (derivative == LexDerivative::File)
						{
							derivative = LexDerivative::None;
							file = tokenBuilder.ToString();
							tokenBuilder.Clear();
						}
						else
						{
							InsertToken(TokenType::StringLiterial);
						}
						state = State::Start;
					}
					pos++;
					break;
				case State::Char:
					if (curChar != L'\'')
					{
						if (curChar == L'\\')
						{
							ProcessTransferChar(nextChar);
							pos++;
						}
						else
							tokenBuilder.Append(curChar);
					}
					else
					{
						if (tokenBuilder.Length() > 1)
							errorList.Add(CompileError(L"Illegal character literial.", 10001, CodePosition(line, col-tokenBuilder.Length(), file)));
						InsertToken(TokenType::CharLiterial);
						state = State::Start;
					}
					pos++;
					break;
				case State::SingleComment:
					if (curChar == L'\n')
						state = State::Start;
					pos++;
					break;
				case State::MultiComment:
					if (curChar == L'*' && nextChar == '/')
					{
						state = State::Start;
						pos += 2;
					}
					else
						pos++;
					break;
				}
			}
			return tokenList;
		}

		String TokenTypeToString(TokenType type)
		{
			switch (type)
			{
			case TokenType::Unkown:
				return L"UnknownToken";
			case TokenType::Identifier:
				return L"Identifier";

			case TokenType::KeywordReturn:
				return L"\"return\"";
			case TokenType::KeywordBreak:
				return L"\"break\"";
			case TokenType::KeywordContinue:
				return L"\"continue\"";
			case TokenType::KeywordIf:
				return L"\"if\"";
			case TokenType::KeywordElse:
				return L"\"else\"";
			case TokenType::KeywordFor:
				return L"\"for\"";
			case TokenType::KeywordWhile:
				return L"\"while\"";
			case TokenType::KeywordDo:
				return L"\"do\"";
			case TokenType::IntLiterial:
				return L"Int Literial";
			case TokenType::DoubleLiterial:
				return L"Double Literial";
			case TokenType::StringLiterial:
				return L"String Literial";
			case TokenType::CharLiterial:
				return L"CharLiterial";
			case TokenType::QuestionMark:
				return L"'?'";
			case TokenType::Colon:
				return L"':'";
			case TokenType::Semicolon:
				return L"';'";
			case TokenType::Comma:
				return L"','";
			case TokenType::LBrace:
				return L"'{'";
			case TokenType::RBrace:
				return L"'}'";
			case TokenType::LBracket:
				return L"'['";
			case TokenType::RBracket:
				return L"']'";
			case TokenType::LParent:
				return L"'('";
			case TokenType::RParent:
				return L"')'";
			case TokenType::At:
				return L"'@'";
			case TokenType::OpAssign:
				return L"'='";
			case TokenType::OpAdd:
				return L"'+'";
			case TokenType::OpSub:
				return L"'-'";
			case TokenType::OpMul:
				return L"'*'";
			case TokenType::OpDiv:
				return L"'/'";
			case TokenType::OpMod:
				return L"'%'";
			case TokenType::OpNot:
				return L"'!'";
			case TokenType::OpLsh:
				return L"'<<'";
			case TokenType::OpRsh:
				return L"'>>'";
			case TokenType::OpAddAssign:
				return L"'+='";
			case TokenType::OpSubAssign:
				return L"'-='";
			case TokenType::OpMulAssign:
				return L"'*='";
			case TokenType::OpDivAssign:
				return L"'/='";
			case TokenType::OpModAssign:
				return L"'%='";
			case TokenType::OpEql:
				return L"'=='";
			case TokenType::OpNeq:
				return L"'!='";
			case TokenType::OpGreater:
				return L"'>'";
			case TokenType::OpLess:
				return L"'<'";
			case TokenType::OpGeq:
				return L"'>='";
			case TokenType::OpLeq:
				return L"'<='";
			case TokenType::OpAnd:
				return L"'&&'";
			case TokenType::OpOr:
				return L"'||'";
			case TokenType::OpBitXor:
				return L"'^'";
			case TokenType::OpBitAnd:
				return L"'&'";
			case TokenType::OpBitOr:
				return L"'|'";
			case TokenType::OpInc:
				return L"'++'";
			case TokenType::OpDec:
				return L"'--'";
			default:
				return L"";
			}
		}
		
	}
}