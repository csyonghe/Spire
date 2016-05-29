#include "Regex.h"

namespace CoreLib
{
namespace Text
{
	RegexMatcher::RegexMatcher(DFA_Table * table)
		:dfa(table)
	{
	}

	int RegexMatcher::Match(const String & str, int startPos)
	{
		int state = dfa->StartState;
		if (state == -1)
			return -1;
		for (int i=startPos; i<str.Length(); i++)
		{
			Word charClass = (*dfa->CharTable)[str[i]];
			if (charClass == 0xFFFF)
				return -1;
			int nextState = dfa->DFA[state][charClass];
			if (nextState == -1)
			{
				if (dfa->Tags[state]->IsFinal)
					return i-startPos;
				else
					return -1;
			}
			else
				state = nextState;
		}
		if (dfa->Tags[state]->IsFinal)
			return str.Length()-startPos;
		else
			return -1;
	}

	DFA_Table * PureRegex::GetDFA()
	{
		return dfaTable.operator->();
	}

	PureRegex::PureRegex(const String & regex)
	{
		RegexParser p;
		RefPtr<RegexNode> tree = p.Parse(regex);
		if (tree)
		{
			NFA_Graph nfa;
			nfa.GenerateFromRegexTree(tree.operator ->());
			DFA_Graph dfa;
			dfa.Generate(&nfa);
			dfaTable = new DFA_Table();
			dfa.ToDfaTable(dfaTable.operator->());
		}
		else
		{
			IllegalRegexException ex;
			if (p.Errors.Count())
				ex.Message = p.Errors[0].Text;
			throw ex;
		}
	}

	bool PureRegex::IsMatch(const String & str)
	{
		RegexMatcher matcher(dfaTable.operator->());
		return (matcher.Match(str, 0)==str.Length());
	}

	PureRegex::RegexMatchResult PureRegex::Search(const String & str, int startPos)
	{
		RegexMatcher matcher(dfaTable.operator ->());
		for (int i=startPos; i<str.Length(); i++)
		{
			int len = matcher.Match(str, i);
			if (len >= 0)
			{
				RegexMatchResult rs;
				rs.Start = i;
				rs.Length = len;
				return rs;
			}
		}
		RegexMatchResult rs;
		rs.Start = 0;
		rs.Length = -1;
		return rs;
	}
}
}