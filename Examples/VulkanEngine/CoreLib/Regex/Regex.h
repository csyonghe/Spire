#ifndef GX_REGEX_H
#define GX_REGEX_H

#include "RegexDFA.h"

namespace CoreLib
{
	namespace Text
	{
		class IllegalRegexException : public Exception
		{
		};

		class RegexMatcher : public Object
		{
		private:
			DFA_Table * dfa;
		public:
			RegexMatcher(DFA_Table * table);
			int Match(const String & str, int startPos = 0);
		};

		class PureRegex : public Object
		{
		private:
			RefPtr<DFA_Table> dfaTable;
		public:
			struct RegexMatchResult
			{
				int Start;
				int Length;
			};
			PureRegex(const String & regex);
			bool IsMatch(const String & str); // Match Whole Word
			RegexMatchResult Search(const String & str, int startPos = 0);
			DFA_Table * GetDFA();
		};
	}
}

#endif