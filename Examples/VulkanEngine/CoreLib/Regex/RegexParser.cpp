#include "RegexTree.h"

namespace CoreLib
{
namespace Text
{
	RegexCharSetNode::RegexCharSetNode()
	{
		CharSet = new RegexCharSet();
	}

	RefPtr<RegexNode> RegexParser::Parse(const String &regex)
	{
		src = regex;
		ptr = 0;
		try
		{
			return ParseSelectionNode();
		}
		catch (...)
		{
			return 0;
		}
	}

	RegexNode * RegexParser::ParseSelectionNode()
	{
		if (ptr >= src.Length())
			return 0;
		RefPtr<RegexNode> left = ParseConnectionNode();
		while (ptr < src.Length() && src[ptr] == L'|')
		{
			ptr ++;
			RefPtr<RegexNode> right = ParseConnectionNode();
			RegexSelectionNode * rs = new RegexSelectionNode();
			rs->LeftChild = left;
			rs->RightChild = right;
			left = rs;
		}
		return left.Release();
	}

	RegexNode * RegexParser::ParseConnectionNode()
	{
		if (ptr >= src.Length())
		{
			return 0;
		}
		RefPtr<RegexNode> left = ParseRepeatNode();
		while (ptr < src.Length() && src[ptr] != L'|' && src[ptr] != L')')
		{
			RefPtr<RegexNode> right = ParseRepeatNode();
			if (right)
			{
				RegexConnectionNode * reg = new RegexConnectionNode();
				reg->LeftChild = left;
				reg->RightChild = right;
				left = reg;
			}
			else
				break;
		}
		return left.Release();
	}

	RegexNode * RegexParser::ParseRepeatNode()
	{
		if (ptr >= src.Length() || src[ptr] == L')' || src[ptr] == L'|')
			return 0;
		
		RefPtr<RegexNode> content;
		if (src[ptr] == L'(')
		{
			RefPtr<RegexNode> reg;
			ptr ++;
			reg = ParseSelectionNode();
			if (src[ptr] != L')')
			{
				SyntaxError err;
				err.Position = ptr;
				err.Text = L"\')\' expected.";
				Errors.Add(err);
				throw 0;
			}
			ptr ++;
			content = reg.Release();
		}
		else
		{
			RefPtr<RegexCharSetNode> reg;
			if (src[ptr] == L'[')
			{
				reg = new RegexCharSetNode();
				ptr ++;
				reg->CharSet = ParseCharSet();
				
				if (src[ptr] != L']')
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = L"\']\' expected.";
					Errors.Add(err);
					throw 0;
				}
				ptr ++;
			}
			else if (src[ptr] == L'\\')
			{
				ptr ++;
				reg = new RegexCharSetNode();
				reg->CharSet = new RegexCharSet();
				switch (src[ptr])
				{
				case L'.':
					{
						reg->CharSet->Neg = true;
						break;
					}
				case L'w':
				case L'W':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'a';
						range.End = L'z';
						reg->CharSet->Ranges.Add(range);
						range.Begin = L'A';
						range.End = L'Z';
						reg->CharSet->Ranges.Add(range);
						range.Begin = L'_';
						range.End = L'_';
						reg->CharSet->Ranges.Add(range);
						range.Begin = L'0';
						range.End = L'9';
						reg->CharSet->Ranges.Add(range);
						if (src[ptr] == L'W')
							reg->CharSet->Neg = true;
						break;
					}
				case L's':
				case 'S':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L' ';
						range.End = L' ';
						reg->CharSet->Ranges.Add(range);
						range.Begin = L'\t';
						range.End = L'\t';
						reg->CharSet->Ranges.Add(range);
						range.Begin = L'\r';
						range.End = L'\r';
						reg->CharSet->Ranges.Add(range);
						range.Begin = L'\n';
						range.End = L'\n';
						reg->CharSet->Ranges.Add(range);
						if (src[ptr] == L'S')
							reg->CharSet->Neg = true;
						break;
					}
				case L'd':
				case L'D':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'0';
						range.End = L'9';
						reg->CharSet->Ranges.Add(range);
						if (src[ptr] == L'D')
							reg->CharSet->Neg = true;
						break;
					}
				case L'n':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'\n';
						range.End = L'\n';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case L't':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'\t';
						range.End = L'\t';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case L'r':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'\r';
						range.End = L'\r';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case L'v':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'\v';
						range.End = L'\v';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case L'f':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'\f';
						range.End = L'\f';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case L'*':
				case L'|':
				case L'(':
				case L')':
				case L'?':
				case L'+':
				case L'[':
				case L']':
				case L'\\':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = src[ptr];
						range.End = range.Begin;
						reg->CharSet->Ranges.Add(range);
						break;
					}
				default:
					{
						SyntaxError err;
						err.Position = ptr;
						err.Text = String(L"Illegal escape sequence \'\\") + src[ptr] + L"\'";
						Errors.Add(err);
						throw 0;
					}
				}
				ptr ++;
			}
			else if (!IsOperator())
			{
				RegexCharSet::RegexCharRange range;
				reg = new RegexCharSetNode();
				reg->CharSet->Neg = false;
				range.Begin = src[ptr];
				range.End = range.Begin;
				ptr ++;
				reg->CharSet->Ranges.Add(range);
			}
			else
			{
				SyntaxError err;
				err.Position = ptr;
				err.Text = String(L"Unexpected \'") + src[ptr] + L'\'';
				Errors.Add(err);
				throw 0;
			}
			content = reg.Release();
		}
		if (ptr < src.Length())
		{
			if (src[ptr] == L'*')
			{
				RefPtr<RegexRepeatNode> node = new RegexRepeatNode();
				node->Child = content;
				node->RepeatType = RegexRepeatNode::rtArbitary;
				ptr ++;
				return node.Release();
			}
			else if (src[ptr] == L'?')
			{
				RefPtr<RegexRepeatNode> node = new RegexRepeatNode();
				node->Child = content;
				node->RepeatType = RegexRepeatNode::rtOptional;
				ptr ++;
				return node.Release();
			}
			else if (src[ptr] == L'+')
			{
				RefPtr<RegexRepeatNode> node = new RegexRepeatNode();
				node->Child = content;
				node->RepeatType = RegexRepeatNode::rtMoreThanOnce;
				ptr ++;
				return node.Release();
			}
			else if (src[ptr] == L'{')
			{
				ptr++;
				RefPtr<RegexRepeatNode> node = new RegexRepeatNode();
				node->Child = content;
				node->RepeatType = RegexRepeatNode::rtSpecified;
				node->MinRepeat = ParseInteger();
				if (src[ptr] == L',')
				{
					ptr ++;
					node->MaxRepeat = ParseInteger();
				}
				else
					node->MaxRepeat = node->MinRepeat;
				if (src[ptr] == L'}')
					ptr++;
				else
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = L"\'}\' expected.";
					Errors.Add(err);
					throw 0;
				}
				if (node->MinRepeat < 0)
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = L"Minimun repeat cannot be less than 0.";
					Errors.Add(err);
					throw 0;
				}
				if (node->MaxRepeat != -1 && node->MaxRepeat < node->MinRepeat)
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = L"Max repeat cannot be less than min repeat.";
					Errors.Add(err);
					throw 0;
				}
				return node.Release();
			}
		}
		return content.Release();
	}

	bool IsDigit(wchar_t ch)
	{
		return ch>=L'0'&& ch <=L'9';
	}

	int RegexParser::ParseInteger()
	{
		StringBuilder number;
		while (IsDigit(src[ptr]))
		{
			number.Append(src[ptr]);
			ptr ++;
		}
		if (number.Length() == 0)
			return -1;
		else
			return StringToInt(number.ProduceString());
	}

	bool RegexParser::IsOperator()
	{
		return (src[ptr] == L'|' || src[ptr] == L'*' || src[ptr] == L'(' || src[ptr] == L')'
				|| src[ptr] == L'?' || src[ptr] == L'+');
	}

	wchar_t RegexParser::ReadNextCharInCharSet()
	{
		if (ptr < src.Length() && src[ptr] != L']')
		{
			if (src[ptr] == L'\\')
			{
				ptr ++;
				if (ptr >= src.Length())
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = String(L"Unexpected end of char-set when looking for escape sequence.");
					Errors.Add(err);
					throw 0;
				}
				wchar_t rs = 0;
				if (src[ptr] == L'\\')
					rs = L'\\';
				else if (src[ptr] == L'^')
					rs = L'^';
				else if (src[ptr] == L'-')
					rs = L'-';
				else if (src[ptr] == L']')
					rs = L']';
				else if (src[ptr] == L'n')
					rs = L'\n';
				else if (src[ptr] == L't')
					rs = L'\t';
				else if (src[ptr] == L'r')
					rs = L'\r';
				else if (src[ptr] == L'v')
					rs = L'\v';
				else if (src[ptr] == L'f')
					rs = L'\f';
				else
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = String(L"Illegal escape sequence inside charset definition \'\\") + src[ptr] + L"\'";
					Errors.Add(err);
					throw 0;
				}
				ptr ++;
				return rs;
			}
			else
				return src[ptr++];
		}
		else
		{
			SyntaxError err;
			err.Position = ptr;
			err.Text = String(L"Unexpected end of char-set.");
			Errors.Add(err);
			throw 0;
		}
	}

	RegexCharSet * RegexParser::ParseCharSet()
	{
		RefPtr<RegexCharSet> rs = new RegexCharSet();
		if (src[ptr] == L'^')
		{
			rs->Neg = true;
			ptr ++;
		}
		else
			rs->Neg = false;
		RegexCharSet::RegexCharRange range;
		while (ptr < src.Length() && src[ptr] != L']')
		{
			range.Begin = ReadNextCharInCharSet();
			//ptr ++;
			
			if (ptr >= src.Length())
			{
				break;
			}
			if (src[ptr] == L'-')
			{
				ptr ++;
				range.End = ReadNextCharInCharSet();	
			}
			else
			{
				range.End = range.Begin;
			}
			rs->Ranges.Add(range);
		
		}
		if (ptr >=src.Length() || src[ptr] != L']')
		{
			SyntaxError err;
			err.Position = ptr;
			err.Text = String(L"Unexpected end of char-set.");
			Errors.Add(err);
			throw 0;
		}
		return rs.Release();
	}
}
}