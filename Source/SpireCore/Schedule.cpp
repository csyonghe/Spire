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
			TokenList tokens;
            TokenReader reader;
            String fileName;
			Token ReadToken(const char * string)
			{
				if (reader.PeekTokenType() == TokenType::EndOfFile)
				{
					sink->diagnose(reader.PeekLoc(), Diagnostics::tokenNameExpectedButEOF, string);
					throw 0;
				}
				else if (reader.PeekToken().Content != string)
				{
					sink->diagnose(reader.PeekLoc(), Diagnostics::tokenNameExpected, string);
					throw 20001;
				}
				return reader.AdvanceToken();
			}

			Token ReadToken(CoreLib::Text::TokenType type)
			{
				if (reader.PeekTokenType() == TokenType::EndOfFile)
				{
					sink->diagnose(reader.PeekLoc(), Diagnostics::tokenTypeExpectedButEOF, type);
					throw 0;
				}
				else if (reader.PeekTokenType() != type)
				{
					sink->diagnose(reader.PeekLoc(), Diagnostics::tokenTypeExpected, type);
					throw 20001;
				}
				return reader.AdvanceToken();
			}

			bool LookAheadToken(const char * string)
			{
				if (reader.PeekTokenType() == TokenType::EndOfFile)
				{
                    // TODO(tfoley): this error condition seems wrong
                    // it shouldn't be an error to see EOF as out *lookahead*
					sink->diagnose(reader.PeekLoc(), Diagnostics::tokenNameExpectedButEOF);
					return false;
				}
				else
				{
					if (reader.PeekToken().Content == string)
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
				try
				{
					while (reader.PeekTokenType() != TokenType::EndOfFile)
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

							while (reader.PeekTokenType() != TokenType::EndOfFile)
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
							while (reader.PeekTokenType() != TokenType::EndOfFile)
							{
								auto token = ReadToken(TokenType::StringLiterial);
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