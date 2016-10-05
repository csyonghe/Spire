#include "IL.h"
#include "../CoreLib/Parser.h"

namespace Spire
{
	namespace Compiler
	{
		RefPtr<KeyHoleNode> ParseInternal(CoreLib::Text::Parser & parser)
		{
			RefPtr<KeyHoleNode> result = new KeyHoleNode();
			result->NodeType = parser.ReadWord();
			if (parser.LookAhead(L"<"))
			{
				parser.ReadToken();
				result->CaptureId = parser.ReadInt();
				parser.ReadToken();
			}
			if (parser.LookAhead(L"("))
			{
				while (!parser.LookAhead(L")"))
				{
					result->Children.Add(ParseInternal(parser));
					if (parser.LookAhead(L","))
						parser.ReadToken();
					else
					{
						break;
					}
				}
				parser.Read(L")");
			}
			return result;
		}

		RefPtr<KeyHoleNode> KeyHoleNode::Parse(String format)
		{
			CoreLib::Text::Parser parser(format);
			return ParseInternal(parser);
		}

		bool KeyHoleNode::Match(List<ILOperand*> & matchResult, ILOperand * instr)
		{
			bool matches = false;
			if (NodeType == L"store")
				matches = dynamic_cast<StoreInstruction*>(instr) != nullptr;
			else if (NodeType == L"op")
				matches = true;
			else if (NodeType == L"load")
				matches = dynamic_cast<LoadInstruction*>(instr) != nullptr;
			else if (NodeType == L"add")
				matches = dynamic_cast<AddInstruction*>(instr) != nullptr;
			else if (NodeType == L"mul")
				matches = dynamic_cast<MulInstruction*>(instr) != nullptr;
			else if (NodeType == L"sub")
				matches = dynamic_cast<SubInstruction*>(instr) != nullptr;
			else if (NodeType == L"call")
				matches = dynamic_cast<CallInstruction*>(instr) != nullptr;
			else if (NodeType == L"switch")
				matches = dynamic_cast<SwitchInstruction*>(instr) != nullptr;
			if (matches)
			{
				if (Children.Count() > 0)
				{
					ILInstruction * cinstr = dynamic_cast<ILInstruction*>(instr);
					if (cinstr != nullptr)
					{
						int opId = 0;
						for (auto & op : *cinstr)
						{
							if (opId >= Children.Count())
							{
								matches = false;
								break;
							}
							matches = matches && Children[opId]->Match(matchResult, &op);
							opId++;
						}
						if (opId != Children.Count())
							matches = false;
					}
					else
						matches = false;
				}
			}
			if (matches && CaptureId != -1)
			{
				matchResult.SetSize(Math::Max(matchResult.Count(), CaptureId + 1));
				matchResult[CaptureId] = instr;
			}
			return matches;
		}
	}
}