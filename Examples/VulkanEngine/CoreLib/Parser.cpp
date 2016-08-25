#include "Parser.h"

using namespace CoreLib::Basic;

namespace CoreLib
{
	namespace Text
	{
		RefPtr<MetaLexer> Parser::metaLexer;
		MetaLexer * Parser::GetTextLexer()
		{
			if (!metaLexer)
			{
				metaLexer = new MetaLexer();
				metaLexer->SetLexProfile(
					L"#WhiteSpace = {\\s+}\n"\
					L"#SingleLineComment = {//[^\\n]*\\n}\n"\
					L"#MultiLineComment = {/\\*([^*]|\\*[^/])*\\*/}\n"\
					L"Identifier = {[a-zA-Z_]\\w*}\n"\
					L"IntConstant = {\\d+}\n"\
					L"FloatConstant = {\\d*.\\d+|\\d+(.\\d+)?(e(-)?\\d+)?}\n"\
					L"StringConstant = {\"([^\\\\\"]|\\\\\\.)*\"}\n"\
					L"CharConstant = {'[^\\n\\r]*'}\n"\
					L"LParent = {\\(}\n"\
					L"RParent = {\\)}\n"\
					L"LBrace = {{}\n"\
					L"RBrace = {}}\n"\
					L"LBracket = {\\[}\n"\
					L"RBracket = {\\]}\n"\
					L"Dot = {.}\n"\
					L"Semicolon = {;}\n"\
					L"Comma = {,}\n"\
					L"Colon = {:}\n"\
					L"OpAdd = {\\+}\n"\
					L"OpSub = {-}\n"\
					L"OpDiv = {/}\n"\
					L"OpMul = {\\*}\n"\
					L"OpMod = {%}\n"\
					L"OpExp = {^}\n"\
					L"OpGreater = {>}\n"\
					L"OpLess = {<}\n"\
					L"OpEqual = {==}\n"\
					L"OpGEqual = {>=}\n"\
					L"OpLEqual = {<=}\n"\
					L"OpNEqual = {!=}\n"\
					L"OpAnd = {&}\n"\
					L"OpOr = {\\|}\n"\
					L"OpNot = {!}\n"\
					L"OpAssign = {=}\n"\
					L"OpDollar = {$}\n"
					);
			}
			return metaLexer.Ptr();
		}
		void Parser::DisposeTextLexer()
		{
			metaLexer = nullptr;
		}
		Basic::List<Basic::String> Parser::SplitString(Basic::String str, wchar_t ch)
		{
			List<String> result;
			StringBuilder currentBuilder;
			for (int i = 0; i < str.Length(); i++)
			{
				if (str[i] == ch)
				{
					result.Add(currentBuilder.ToString());
					currentBuilder.Clear();
				}
				else
					currentBuilder.Append(str[i]);
			}
			result.Add(currentBuilder.ToString());
			return result;
		}
		Parser::Parser(String text)
		{
			this->text = text;
			
			stream = GetTextLexer()->Parse(text);
			for (auto token : stream)
			{
				if (token.TypeID != -1)
					tokens.Add(token);
			}
			tokenPtr = 0;
		}


		List<String> Split(String text, wchar_t c)
		{
			List<String> result;
			StringBuilder sb;
			for (int i = 0; i < text.Length(); i++)
			{
				if (text[i] == c)
				{
					auto str = sb.ToString();
					if (str.Length() != 0)
						result.Add(str);
					sb.Clear();
				}
				else
					sb << text[i];
			}
			auto lastStr = sb.ToString();
			if (lastStr.Length())
				result.Add(lastStr);
			return result;
		}
		CommandLineParser::CommandLineParser(const String & cmdLine)
		{
			String profile =
				L"String = {\"[^\"]*\"}\n"\
				L"Value = {\\d+(.\\d*)?}\n"\
				L"Token = {[^ \\t\\n\"\']+}\n"\
				L"#WhiteSpace = {\\s+}\n";
			MetaLexer lexer(profile);
			lexer.Parse(cmdLine, stream);
		}

		String CommandLineParser::GetFileName()
		{
			if (stream.Count() && stream.FirstNode()->Value.TypeID != 1)
				return stream.FirstNode()->Value.Str;
			else
				return L"";
		}

		bool CommandLineParser::OptionExists(const String & opt)
		{
			for (auto * node = stream.FirstNode(); node; node = node->GetNext())
			{
				if (node->Value.Str.Equals(opt, false))
				{
					return true;
				}
			}
			return false;
		}

		String CommandLineParser::GetOptionValue(const String & opt)
		{
			for (auto * node = stream.FirstNode(); node; node = node->GetNext())
			{
				if (node->Value.Str.Equals(opt, false))
				{
					node = node->GetNext();
					if (node)
						return node->Value.Str;
					else
						return L"";
				}
			}
			return L"";
		}

		String CommandLineParser::GetToken(int id)
		{
			auto node = stream.FirstNode();
			for (int i = 0; i<id; i++)
			{
				node = node->GetNext();
			}
			return node->Value.Str;
		}

		int CommandLineParser::GetTokenCount()
		{
			return stream.Count();
		}
	}
}