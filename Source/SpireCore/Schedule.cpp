#include "Schedule.h"
#include "Lexer.h"
using namespace CoreLib::Basic;

namespace Spire
{
	namespace Compiler
	{
		class ScheduleParser
		{
		private:
            DiagnosticSink * sink;
			List<Token> tokens;
			int pos;
			String fileName;
			Token & ReadToken(const char * string)
			{
				if (pos >= tokens.Count())
				{
					sink->diagnose(CodePosition(0, 0, 0, fileName), Diagnostics::tokenNameExpectedButEOF, string);
					throw 0;
				}
				else if (tokens[pos].Content != string)
				{
					sink->diagnose(tokens[pos].Position, Diagnostics::tokenNameExpected, string);
					throw 20001;
				}
				return tokens[pos++];
			}

			Token & ReadToken(CoreLib::Text::TokenType type)
			{
				if (pos >= tokens.Count())
				{
					sink->diagnose(CodePosition(0, 0, 0, fileName), Diagnostics::tokenTypeExpectedButEOF, type);
					throw 0;
				}
				else if (tokens[pos].Type != type)
				{
					sink->diagnose(tokens[pos].Position, Diagnostics::tokenTypeExpected, type);
					throw 20001;
				}
				return tokens[pos++];
			}

			bool LookAheadToken(const char * string)
			{
				if (pos >= tokens.Count())
				{
					sink->diagnose(CodePosition(0, 0, 0, fileName), Diagnostics::tokenNameExpectedButEOF);
					return false;
				}
				else
				{
					if (tokens[pos].Content == string)
						return true;
					else
						return false;
				}
			}
		public:
			ScheduleParser(DiagnosticSink * sink)
				: sink(sink)
			{}
			Schedule Parse(String source, String _fileName)
			{
				this->fileName = _fileName;
				Schedule schedule;
				Lexer lex;
				tokens = lex.Parse(fileName, source, sink);
				pos = 0;
				try
				{
					while (pos < tokens.Count())
					{
						if (LookAheadToken("attrib"))
						{
							EnumerableDictionary<String, String> additionalAttributes;
							ReadToken("attrib");
							String choiceName = ReadToken(TokenType::Identifier).Content;
							while (LookAheadToken("."))
							{
								choiceName = choiceName + ".";
								ReadToken(TokenType::Dot);
								choiceName = choiceName + ReadToken(TokenType::Identifier).Content;
							}
							ReadToken(TokenType::OpAssign);

							while (pos < tokens.Count())
							{
								auto name = ReadToken(TokenType::Identifier).Content;
								String value;
								if (LookAheadToken(":"))
								{
									ReadToken(":");
									value = ReadToken(TokenType::StringLiterial).Content;
								}
								additionalAttributes[name] = value;
								if (LookAheadToken(","))
									ReadToken(TokenType::Comma);
								else
									break;
							}
							schedule.AddtionalAttributes[choiceName] = additionalAttributes;
						}
						else
						{
							String choiceName = ReadToken(TokenType::Identifier).Content;
							while (LookAheadToken("."))
							{
								choiceName = choiceName + ".";
								ReadToken(TokenType::Dot);
								choiceName = choiceName + ReadToken(TokenType::Identifier).Content;
							}
							ReadToken(TokenType::OpAssign);
							List<RefPtr<ChoiceValueSyntaxNode>> worlds;
							while (pos < tokens.Count())
							{
								auto & token = ReadToken(TokenType::StringLiterial);
								RefPtr<ChoiceValueSyntaxNode> choiceValue = new ChoiceValueSyntaxNode();
								choiceValue->Position = token.Position;
								int splitterPos = token.Content.IndexOf(':');
								if (splitterPos != -1)
								{
									choiceValue->WorldName = token.Content.SubString(0, splitterPos);
									choiceValue->AlternateName = token.Content.SubString(splitterPos + 1, token.Content.Length() - splitterPos - 1);
								}
								else
								{
									choiceValue->WorldName = token.Content;
								}
								worlds.Add(choiceValue);
								if (LookAheadToken(","))
									ReadToken(TokenType::Comma);
								else
									break;
							}
							schedule.Choices[choiceName] = worlds;
						}
						ReadToken(TokenType::Semicolon);
					}
				}
				catch (...)
				{
				}
				return schedule;
			}
		};
	
		Schedule Schedule::Parse(String source, String fileName, DiagnosticSink * sink)
		{
			return ScheduleParser(sink).Parse(source, fileName);
		}
	}
}