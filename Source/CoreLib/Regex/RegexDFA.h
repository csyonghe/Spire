#ifndef REGEX_DFA_H
#define REGEX_DFA_H

#include "RegexNFA.h"

namespace CoreLib
{
	namespace Text
	{
		using namespace CoreLib::Basic;

		typedef List<Word> RegexCharTable;
	
		class CharTableGenerator : public Object
		{
		private:
			List<String> sets;
			RegexCharTable * table;
			int AddSet(String set);
		public:
			List<RegexCharSet::RegexCharRange> elements;
			CharTableGenerator(RegexCharTable * _table);
			int Generate(List<RegexCharSet *> & charSets);
		};

		class DFA_Table_Tag
		{
		public:
			bool IsFinal = false;
			List<int> TerminalIdentifiers; // sorted
			DFA_Table_Tag();
		};

		class DFA_Table : public Object
		{
		public:
			int StateCount;
			int AlphabetSize;
			int ** DFA;
			List<RefPtr<DFA_Table_Tag>> Tags;
			int StartState;
			RefPtr<RegexCharTable> CharTable;
			DFA_Table();
			~DFA_Table();
		};

		class DFA_Node : public Object
		{
		public:
			int ID = -1;
			bool IsFinal = false;
			List<int> TerminalIdentifiers; // sorted
			List<NFA_Node*> Nodes;  // sorted
			List<DFA_Node *> Translations;
			DFA_Node(int elements);
			bool operator == (const DFA_Node & node);
		};

		class DFA_Graph : public Object
		{
		private:
			List<RegexCharSet::RegexCharRange> CharElements;
			RefPtr<RegexCharTable> table;
			DFA_Node * startNode;
			List<RefPtr<DFA_Node>> nodes;
			void CombineCharElements(NFA_Node * node, List<Word> & elem);
		public:
			void Generate(NFA_Graph * nfa);
			String Interpret();
			void ToDfaTable(DFA_Table * dfa);
		};
	}
}
#endif