#include "RegexDFA.h"
#include "../Basic.h"

namespace CoreLib
{
namespace Text
{
	CharTableGenerator::CharTableGenerator(RegexCharTable * _table)
		: table(_table)
	{
		table->SetSize(65536);
		memset(table->Buffer(),0,sizeof(Word)*table->Count());
	}

	DFA_Table_Tag::DFA_Table_Tag()
	{
		IsFinal = false;
	}

	int CharTableGenerator::AddSet(String set)
	{
		int fid = sets.IndexOf(set);
		if (fid != -1)
			return fid;
		else
		{
			sets.Add(set);
			return sets.Count()-1;
		}
	}

	int CharTableGenerator::Generate(List<RegexCharSet *> & charSets)
	{
		/*List<RegexCharSet *> cs;
		cs.SetCapacity(charSets.Count());
		String str;
		str.Alloc(1024);
		for (int i=1; i<65536; i++)
		{
			str = L"";
			cs.Clear();
			for (int j=0; j<charSets.Count(); j++)
			{
				if (charSets[j]->Contains(i))
				{
					str += (wchar_t)(j+1);
					cs.Add(charSets[j]);
				}
			}
			int lastCount = sets.Count();
			if (str.Length())
			{
				int id = AddSet(str);
				if (id == lastCount)
				{
					for (int j=0; j<cs.Count(); j++)
						cs[j]->Elements.Add(id);
				}
				(*table)[i] = id;
			}
			else
				(*table)[i] = 0xFFFF;
		}
		return sets.Count();*/
		
		RegexCharSet::CalcCharElements(charSets, elements);
		for (int i=0; i<table->Count(); i++)
			(*table)[i] = 0xFFFF;
		Word* buf = table->Buffer();
		for (int i=0; i<elements.Count(); i++)
		{
			for (int k=elements[i].Begin; k<=elements[i].End; k++)	
			{
#ifdef _DEBUG
				if ((*table)[k] != 0xFFFF)
				{
					throw L"Illegal subset generation."; // This indicates a bug.
				}
#endif
				buf[k] = (Word)i;
			}
		}
		return elements.Count();
	}

	DFA_Node::DFA_Node(int elements)
	{
		Translations.SetSize(elements);
		for (int i=0; i<elements; i++)
			Translations[i] = 0;
	}

	void DFA_Graph::CombineCharElements(NFA_Node * node, List<Word> & elem)
	{
		for (int i=0; i<node->Translations.Count(); i++)
		{
			for (int j=0; j<node->Translations[i]->CharSet->Elements.Count(); j++)
			{
				if (elem.IndexOf(node->Translations[i]->CharSet->Elements[j]) == -1)
					elem.Add(node->Translations[i]->CharSet->Elements[j]);
			}
		}
	}

	void DFA_Graph::Generate(NFA_Graph * nfa)
	{
		table = new RegexCharTable();
		List<RegexCharSet * > charSets;
		for (int i=0; i<nfa->translations.Count(); i++)
		{
			if (nfa->translations[i]->CharSet && nfa->translations[i]->CharSet->Ranges.Count())
				charSets.Add(nfa->translations[i]->CharSet.operator->());
		}
		CharTableGenerator gen(table.operator ->());
		int elements = gen.Generate(charSets);
		CharElements = gen.elements;
		List<DFA_Node *> L,D;
		startNode = new DFA_Node(elements);
		startNode->ID = 0;
		startNode->Nodes.Add(nfa->start);
		L.Add(startNode);
		nodes.Add(startNode);
		List<Word> charElem;
		do
		{
			DFA_Node * node = L.Last();
			L.RemoveAt(L.Count()-1);
			charElem.Clear();
			node->IsFinal = false;
			for (int i=0; i<node->Nodes.Count(); i++)
			{
				CombineCharElements(node->Nodes[i], charElem);
				if (node->Nodes[i]->IsFinal)
					node->IsFinal = true;
			}
			for (int i=0; i<charElem.Count(); i++)
			{
				DFA_Node * n = new DFA_Node(0);
				for (int j=0; j<node->Nodes.Count(); j++)
				{
					for (int k=0; k<node->Nodes[j]->Translations.Count(); k++)
					{
						NFA_Translation * trans = node->Nodes[j]->Translations[k];
						if (trans->CharSet->Elements.Contains(charElem[i]))
						{
							if (!n->Nodes.Contains(node->Nodes[j]->Translations[k]->NodeDest))
								n->Nodes.Add(node->Nodes[j]->Translations[k]->NodeDest);
						}
					}
				}
				int fid = -1;
				for (int j=0; j<nodes.Count(); j++)
				{
					if ((*nodes[j]) == *n)
					{
						fid = j;
						break;
					}
				}
				if (fid == -1)
				{
					n->Translations.SetSize(elements);
					for (int m=0; m<elements; m++)
						n->Translations[m] = 0;
					n->ID = nodes.Count();
					L.Add(n);
					nodes.Add(n);
					fid = nodes.Count()-1;
				}
				else
					delete n;
				n = nodes[fid].operator ->();
				node->Translations[charElem[i]] = n;
			}
		}
		while (L.Count());

		// Set Terminal Identifiers
		HashSet<int> terminalIdentifiers;
		for (int i=0; i<nodes.Count(); i++)
		{
			terminalIdentifiers.Clear();
			for (int j=0; j<nodes[i]->Nodes.Count(); j++)
			{
				if (nodes[i]->Nodes[j]->IsFinal && 
					!terminalIdentifiers.Contains(nodes[i]->Nodes[j]->TerminalIdentifier))
				{
					nodes[i]->IsFinal = true;
					terminalIdentifiers.Add(nodes[i]->Nodes[j]->TerminalIdentifier);
					nodes[i]->TerminalIdentifiers.Add(nodes[i]->Nodes[j]->TerminalIdentifier);
				}
			}
			nodes[i]->TerminalIdentifiers.Sort();
		}
	}

	bool DFA_Node::operator == (const DFA_Node & node)
	{
		if (Nodes.Count() != node.Nodes.Count())
			return false;
		for (int i=0; i<node.Nodes.Count(); i++)
		{
			if (node.Nodes[i] != Nodes[i])
				return false;
		}
		return true;
	}

	String DFA_Graph::Interpret()
	{
		StringBuilder sb(4096000);
		for (int i=0; i<nodes.Count(); i++)
		{
			if (nodes[i]->IsFinal)
				sb.Append(L'#');
			else if (nodes[i] == startNode)
				sb.Append(L'*');
			sb.Append(String(nodes[i]->ID));
			sb.Append(L'(');
			for (int j=0; j<nodes[i]->Nodes.Count(); j++)
			{
				sb.Append(String(nodes[i]->Nodes[j]->ID));
				sb.Append(L" ");
			}
			sb.Append(L")\n");
			for (int j=0; j<nodes[i]->Translations.Count(); j++)
			{
				if (nodes[i]->Translations[j])
				{
					sb.Append(L"\tOn ");
					sb.Append(String(j));
					sb.Append(L": ");
					sb.Append(String(nodes[i]->Translations[j]->ID));
					sb.Append(L'\n');
				}
			}
		}

		sb.Append(L"\n\n==================\n");
		sb.Append(L"Char Set Table:\n");
		for (int i=0; i<CharElements.Count(); i++)
		{
			sb.Append(L"Class ");
			sb.Append(String(i));
			sb.Append(L": ");
			RegexCharSet s;
			s.Ranges.Add(CharElements[i]);
			sb.Append(s.Reinterpret());
			sb.Append(L"\n");
		}
		return sb.ProduceString();
	}

	void DFA_Graph::ToDfaTable(DFA_Table * dfa)
	{
		dfa->CharTable = table;
		dfa->DFA = new int*[nodes.Count()];
		dfa->Tags.SetSize(nodes.Count());
		for (int i=0; i<nodes.Count(); i++)
			dfa->Tags[i] = new DFA_Table_Tag();
		dfa->StateCount = nodes.Count();
		dfa->AlphabetSize = CharElements.Count();
		for (int i=0; i<nodes.Count(); i++)
		{
			dfa->DFA[i] = new int[table->Count()];
			for (int j=0; j<nodes[i]->Translations.Count(); j++)
			{
				if (nodes[i]->Translations[j])
					dfa->DFA[i][j] = nodes[i]->Translations[j]->ID;
				else
					dfa->DFA[i][j] = -1;
			}
			if (nodes[i] == startNode)
				dfa->StartState = i;
			if (nodes[i]->IsFinal)
			{
				dfa->Tags[i]->IsFinal = true;
				dfa->Tags[i]->TerminalIdentifiers = nodes[i]->TerminalIdentifiers;
			}
		}
	}

	DFA_Table::DFA_Table()
	{
		DFA = 0;
		StateCount = 0;
		AlphabetSize = 0;
		StartState = -1;
	}
	
	DFA_Table::~DFA_Table()
	{
		if (DFA)
		{
			for (int i=0; i<StateCount; i++)
				delete [] DFA[i];
			delete [] DFA;
		}
	}
}
}