#ifndef CORELIB_TEXT_PARSER_H
#define CORELIB_TEXT_PARSER_H

#include "Basic.h"
#include "Regex/MetaLexer.h"

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

		const int TokenType_Identifier = 3;
		const int TokenType_Int = 4;
		const int TokenType_Float = 5;
		const int TokenType_StringLiteral = 6;
		const int TokenType_CharLiteral = 7;

		class Parser
		{
		private:
			static RefPtr<MetaLexer> metaLexer;
			LazyLexStream stream;
			bool legal;
			String text;
			List<LazyLexToken> tokens;
			int tokenPtr;
			LexToken MakeToken(LazyLexToken ltk)
			{
				LexToken tk;
				tk.Position = ltk.Position;
				tk.TypeID = ltk.TypeID;
				tk.Str = text.SubString(ltk.Position, ltk.Length);
				return tk;
			}
		public:
			static MetaLexer * GetTextLexer();
			static void DisposeTextLexer();
			static Basic::List<Basic::String> SplitString(Basic::String str, wchar_t ch);
			Parser(Basic::String text);
			int ReadInt()
			{
				auto token = ReadToken();
				bool neg = false;
				if (token.Str == L'-')
				{
					neg = true;
					token = ReadToken();
				}
				if (token.TypeID == TokenType_Int)
				{
					if (neg)
						return -StringToInt(token.Str);
					else
						return StringToInt(token.Str);
				}
				throw TextFormatException(L"Text parsing error: int expected.");
			}
			unsigned int ReadUInt()
			{
				auto token = ReadToken();
				if (token.TypeID == TokenType_Int)
				{
					return StringToUInt(token.Str);
				}
				throw TextFormatException(L"Text parsing error: int expected.");
			}
			double ReadDouble()
			{
				auto token = ReadToken();
				bool neg = false;
				if (token.Str == L'-')
				{
					neg = true;
					token = ReadToken();
				}
				if (token.TypeID == TokenType_Float || token.TypeID == TokenType_Int)
				{
					if (neg)
						return -StringToDouble(token.Str);
					else
						return StringToDouble(token.Str);
				}
				throw TextFormatException(L"Text parsing error: floating point value expected.");
			}
			String ReadWord()
			{
				auto token = ReadToken();
				if (token.TypeID == TokenType_Identifier)
				{
					return token.Str;
				}
				throw TextFormatException(L"Text parsing error: identifier expected.");
			}
			String Read(const wchar_t * expectedStr)
			{
				auto token = ReadToken();
				if (token.Str == expectedStr)
				{
					return token.Str;
				}
				throw TextFormatException(L"Text parsing error: \'" + String(expectedStr) + L"\' expected.");
			}
			String Read(String expectedStr)
			{
				auto token = ReadToken();
				if (token.Str == expectedStr)
				{
					return token.Str;
				}
				throw TextFormatException(L"Text parsing error: \'" + expectedStr + L"\' expected.");
			}
			static String EscapeStringLiteral(String str)
			{
				StringBuilder sb;
				sb << L"\"";
				for (int i = 0; i < str.Length(); i++)
				{
					switch (str[i])
					{
					case L' ':
						sb << L"\\s";
						break;
					case L'\n':
						sb << L"\\n";
						break;
					case L'\r':
						sb << L"\\r";
						break;
					case L'\t':
						sb << L"\\t";
						break;
					case L'\v':
						sb << L"\\v";
						break;
					case L'\'':
						sb << L"\\\'";
						break;
					case L'\"':
						sb << L"\\\"";
						break;
					case L'\\':
						sb << L"\\\\";
						break;
					default:
						sb << str[i];
						break;
					}
				}
				sb << L"\"";
				return sb.ProduceString();
			}
			static String UnescapeStringLiteral(String str)
			{
				StringBuilder sb;
				int start = str.StartsWith(L"\"") ? 1 : 0;
				for (int i = start; i < str.Length() - start; i++)
				{
					if (str[i] == L'\\' && i < str.Length() - 1)
					{
						switch (str[i + 1])
						{
						case L's':
							sb << L" ";
							break;
						case L't':
							sb << L'\t';
							break;
						case L'n':
							sb << L'\n';
							break;
						case L'r':
							sb << L'\r';
							break;
						case L'v':
							sb << L'\v';
							break;
						case L'\'':
							sb << L'\'';
							break;
						case L'\"':
							sb << L"\"";
							break;
						case L'\\':
							sb << L"\\";
							break;
						default:
							i = i - 1;
							sb << str[i];
						}
						i++;
					}
					else
						sb << str[i];
				}
				return sb.ProduceString();
			}
			String ReadStringLiteral()
			{
				auto token = ReadToken();
				if (token.TypeID == TokenType_StringLiteral)
				{
					return UnescapeStringLiteral(token.Str.SubString(1, token.Str.Length()-2));
				}
				throw TextFormatException(L"Text parsing error: string literal expected.");
			}
			void Back(int count)
			{
				tokenPtr -= count;
			}
			LexToken ReadToken()
			{
				if (tokenPtr < tokens.Count())
				{
					LexToken rs = MakeToken(tokens[tokenPtr]);
					tokenPtr++;
					return rs;
				}
				throw TextFormatException(L"Unexpected ending.");
			}
			LexToken NextToken(int offset = 0)
			{
				if (tokenPtr + offset < tokens.Count())
					return MakeToken(tokens[tokenPtr + offset]);
				else
				{
					LexToken rs;
					rs.TypeID = -1;
					rs.Position = -1;
					return rs;
				}
			}
			bool LookAhead(String token)
			{
				if (tokenPtr < tokens.Count())
				{
					auto next = NextToken();
					return next.Str == token;
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

		class CommandLineParser : public Object
		{
		private:
			LexStream stream;
		public:
			CommandLineParser(const String & cmdLine);
			String GetFileName();
			bool OptionExists(const String & opt);
			String GetOptionValue(const String & opt);
			String GetToken(int id);
			int GetTokenCount();
		};
	}
}

#endif