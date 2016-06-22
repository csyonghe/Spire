#include "RegexNFA.h"
#include "../Basic.h"

namespace CoreLib
{
namespace Text
{
	int NFA_Node::HandleCount = 0;

	NFA_Translation::NFA_Translation(NFA_Node * src, NFA_Node * dest, RefPtr<RegexCharSet> charSet)
		: CharSet(charSet), NodeSrc(src), NodeDest(dest)
	{}

	NFA_Translation::NFA_Translation()
	{
		NodeSrc = NodeDest = 0;
	}

	NFA_Translation::NFA_Translation(NFA_Node * src, NFA_Node * dest)
		: NodeSrc(src), NodeDest(dest)
	{
	}

	NFA_Node::NFA_Node()
		: Flag(false), IsFinal(false), TerminalIdentifier(0)
	{
		HandleCount ++;
		ID = HandleCount;
	}

	void NFA_Node::RemoveTranslation(NFA_Translation * trans)
	{
		int fid = Translations.IndexOf(trans);
		if (fid != -1)
			Translations.RemoveAt(fid);
	}

	void NFA_Node::RemovePrevTranslation(NFA_Translation * trans)
	{
		int fid = PrevTranslations.IndexOf(trans);
		if (fid != -1)
			PrevTranslations.RemoveAt(fid);
	}

	NFA_Node * NFA_Graph::CreateNode()
	{
		NFA_Node * nNode = new NFA_Node();
		nodes.Add(nNode);
		return nNode;
	}

	NFA_Translation * NFA_Graph::CreateTranslation()
	{
		NFA_Translation * trans = new NFA_Translation();
		translations.Add(trans);
		return trans;
	}

	void NFA_Graph::ClearNodes()
	{
		for (int i=0; i<nodes.Count(); i++)
			nodes[i] = 0;
		for (int i=0; i<translations.Count(); i++)
			translations[i] = 0;
		nodes.Clear();
		translations.Clear();
	}

	void NFA_Graph::GenerateFromRegexTree(RegexNode * tree, bool elimEpsilon)
	{
		NFA_StatePair s;
		tree->Accept(this);
		s = PopState();
		start = s.start;
		end = s.end;
		end->IsFinal = true;

		if (elimEpsilon)
		{
			PostGenerationProcess();
		}

	}

	void NFA_Graph::PostGenerationProcess()
	{
		EliminateEpsilon();
		for (int i=0; i<translations.Count(); i++)
		{
			if (translations[i]->CharSet)
				translations[i]->CharSet->Normalize();
			else
			{
				translations[i] = 0;
				translations.RemoveAt(i);
				i--;
			}
		}
	}

	NFA_Node * NFA_Graph::GetStartNode()
	{
		return start;
	}

	void NFA_Graph::PushState(NFA_StatePair s)
	{
		stateStack.Add(s);
	}

	NFA_Graph::NFA_StatePair NFA_Graph::PopState()
	{
		NFA_StatePair s = stateStack.Last();
		stateStack.RemoveAt(stateStack.Count()-1);
		return s;
	}

	void NFA_Graph::VisitCharSetNode(RegexCharSetNode * node)
	{
		NFA_StatePair s;
		s.start = CreateNode();
		s.end = CreateNode();
		NFA_Translation * trans = CreateTranslation();
		trans->CharSet = node->CharSet;
		trans->NodeSrc = s.start;
		trans->NodeDest = s.end;
		s.start->Translations.Add(trans);
		s.end->PrevTranslations.Add(trans);
		PushState(s);
	}

	void NFA_Graph::VisitRepeatNode(RegexRepeatNode * node)
	{
		NFA_StatePair sr;
		sr.start = sr.end = nullptr;
		node->Child->Accept(this);
		NFA_StatePair s = PopState();
		if (node->RepeatType == RegexRepeatNode::rtArbitary)
		{
			sr.start = CreateNode();
			sr.end = CreateNode();

			NFA_Translation * trans = CreateTranslation();
			trans->NodeSrc = sr.start;
			trans->NodeDest = sr.end;
			sr.start->Translations.Add(trans);
			sr.end->PrevTranslations.Add(trans);
			
			NFA_Translation * trans1 = CreateTranslation();
			trans1->NodeSrc = sr.end;
			trans1->NodeDest = s.start;
			sr.end->Translations.Add(trans1);
			s.start->PrevTranslations.Add(trans1);

			NFA_Translation * trans2 = CreateTranslation();
			trans2->NodeSrc = s.end;
			trans2->NodeDest = sr.end;
			s.end->Translations.Add(trans2);
			sr.end->PrevTranslations.Add(trans2);
		}
		else if (node->RepeatType == RegexRepeatNode::rtOptional)
		{
			sr = s;

			NFA_Translation * trans = CreateTranslation();
			trans->NodeSrc = sr.start;
			trans->NodeDest = sr.end;
			sr.start->Translations.Add(trans);
			sr.end->PrevTranslations.Add(trans);
		}
		else if (node->RepeatType == RegexRepeatNode::rtMoreThanOnce)
		{
			sr = s;

			NFA_Translation * trans = CreateTranslation();
			trans->NodeSrc = sr.end;
			trans->NodeDest = sr.start;
			sr.start->PrevTranslations.Add(trans);
			sr.end->Translations.Add(trans);
		}
		else if (node->RepeatType == RegexRepeatNode::rtSpecified)
		{
			if (node->MinRepeat == 0)
			{
				if (node->MaxRepeat > 0)
				{
					for (int i=1; i<node->MaxRepeat; i++)
					{
						node->Child->Accept(this);
						NFA_StatePair s1 = PopState();
						NFA_Translation * trans = CreateTranslation();
						trans->NodeDest = s1.start;
						trans->NodeSrc = s.end;
						trans->NodeDest->PrevTranslations.Add(trans);
						trans->NodeSrc->Translations.Add(trans);

						trans = CreateTranslation();
						trans->NodeDest = s1.start;
						trans->NodeSrc = s.start;
						trans->NodeDest->PrevTranslations.Add(trans);
						trans->NodeSrc->Translations.Add(trans);

						s.end = s1.end;
					}
					NFA_Translation * trans = CreateTranslation();
					trans->NodeDest = s.end;
					trans->NodeSrc = s.start;
					trans->NodeDest->PrevTranslations.Add(trans);
					trans->NodeSrc->Translations.Add(trans);
					sr = s;
				}
				else if (node->MaxRepeat == 0)
				{
					sr.start = CreateNode();
					sr.end = CreateNode();
					NFA_Translation * trans = CreateTranslation();
					trans->NodeDest = sr.end;
					trans->NodeSrc = sr.start;
					trans->NodeDest->PrevTranslations.Add(trans);
					trans->NodeSrc->Translations.Add(trans);
				}
				else
				{
					// Arbitary repeat
					sr.start = CreateNode();
					sr.end = CreateNode();

					NFA_Translation * trans = CreateTranslation();
					trans->NodeSrc = sr.start;
					trans->NodeDest = sr.end;
					sr.start->Translations.Add(trans);
					sr.end->PrevTranslations.Add(trans);
					
					NFA_Translation * trans1 = CreateTranslation();
					trans1->NodeSrc = sr.end;
					trans1->NodeDest = s.start;
					sr.end->Translations.Add(trans1);
					s.start->PrevTranslations.Add(trans1);

					NFA_Translation * trans2 = CreateTranslation();
					trans2->NodeSrc = s.end;
					trans2->NodeDest = sr.end;
					s.end->Translations.Add(trans2);
					sr.end->PrevTranslations.Add(trans2);
				}
			}
			else
			{
				NFA_Node * lastBegin = s.start;
				for (int i=1; i<node->MinRepeat; i++)
				{
					node->Child->Accept(this);
					NFA_StatePair s1 = PopState();
					NFA_Translation * trans = CreateTranslation();
					trans->NodeDest = s1.start;
					trans->NodeSrc = s.end;
					trans->NodeDest->PrevTranslations.Add(trans);
					trans->NodeSrc->Translations.Add(trans);
					s.end = s1.end;
					lastBegin = s1.start;
				}
				if (node->MaxRepeat == -1)
				{
					NFA_Translation * trans = CreateTranslation();
					trans->NodeDest = lastBegin;
					trans->NodeSrc = s.end;
					trans->NodeDest->PrevTranslations.Add(trans);
					trans->NodeSrc->Translations.Add(trans);
				}
				else if (node->MaxRepeat > node->MinRepeat)
				{
					lastBegin = s.end;
					for (int i=node->MinRepeat; i<node->MaxRepeat; i++)
					{
						node->Child->Accept(this);
						NFA_StatePair s1 = PopState();
						NFA_Translation * trans = CreateTranslation();
						trans->NodeDest = s1.start;
						trans->NodeSrc = s.end;
						trans->NodeDest->PrevTranslations.Add(trans);
						trans->NodeSrc->Translations.Add(trans);

						trans = CreateTranslation();
						trans->NodeDest = s1.start;
						trans->NodeSrc = lastBegin;
						trans->NodeDest->PrevTranslations.Add(trans);
						trans->NodeSrc->Translations.Add(trans);

						s.end = s1.end;
					}

					NFA_Translation * trans = CreateTranslation();
					trans->NodeDest = s.end;
					trans->NodeSrc = lastBegin;
					trans->NodeDest->PrevTranslations.Add(trans);
					trans->NodeSrc->Translations.Add(trans);
				}

				sr = s;
			}
			
		}
		PushState(sr);
	}

	void NFA_Graph::VisitSelectionNode(RegexSelectionNode * node)
	{
		NFA_StatePair s, s1, sr;
		sr.start = CreateNode();
		sr.end = CreateNode();
		s.start = sr.start;
		s.end = sr.end;
		s1.start = sr.start;
		s1.end = sr.end;
		if (node->LeftChild)
		{
			node->LeftChild->Accept(this);
			s = PopState();
		}
		if (node->RightChild)
		{
			node->RightChild->Accept(this);
			s1 = PopState();
		}
		
		NFA_Translation * trans;
		trans = CreateTranslation();
		trans->NodeSrc = sr.start;
		trans->NodeDest = s.start;
		sr.start->Translations.Add(trans);
		s.start->PrevTranslations.Add(trans);

		trans = CreateTranslation();
		trans->NodeSrc = sr.start;
		trans->NodeDest = s1.start;
		sr.start->Translations.Add(trans);
		s1.start->PrevTranslations.Add(trans);

		trans = CreateTranslation();
		trans->NodeSrc = s.end;
		trans->NodeDest = sr.end;
		s.end->Translations.Add(trans);
		sr.end->PrevTranslations.Add(trans);

		trans = CreateTranslation();
		trans->NodeSrc = s1.end;
		trans->NodeDest = sr.end;
		s1.end->Translations.Add(trans);
		sr.end->PrevTranslations.Add(trans);

		PushState(sr);
	}

	void NFA_Graph::VisitConnectionNode(RegexConnectionNode * node)
	{
		NFA_StatePair s, s1;
		node->LeftChild->Accept(this);
		s = PopState();
		node->RightChild->Accept(this);
		s1 = PopState();
		NFA_Translation * trans = CreateTranslation();
		trans->NodeDest = s1.start;
		trans->NodeSrc = s.end;
		s.end->Translations.Add(trans);
		s1.start->PrevTranslations.Add(trans);
		s.end = s1.end;
		PushState(s);
		
	}

	void NFA_Graph::ClearNodeFlags()
	{
		for (int i=0; i<nodes.Count(); i++)
			nodes[i]->Flag = false;
	}

	void NFA_Graph::GetValidStates(List<NFA_Node *> & states)
	{
		RefPtr<List<NFA_Node *>> list1 = new List<NFA_Node *>();
		RefPtr<List<NFA_Node *>> list2 = new List<NFA_Node *>();
		list1->Add(start);
		states.Add(start);
		ClearNodeFlags();
		while (list1->Count())
		{
			list2->Clear();
			for (int i=0; i<list1->Count(); i++)
			{
				bool isValid = false;
				NFA_Node * curNode = (*list1)[i];
				curNode->Flag = true;
				for (int j=0; j<curNode->PrevTranslations.Count(); j++)
				{
					if (curNode->PrevTranslations[j]->CharSet)
					{
						isValid = true;
						break;
					}
					
				}
				if (isValid)
					states.Add(curNode);
				for (int j=0; j<curNode->Translations.Count(); j++)
				{
					if (!curNode->Translations[j]->NodeDest->Flag)
					{
						list2->Add(curNode->Translations[j]->NodeDest);
					}
				}
			}
			RefPtr<List<NFA_Node *>> tmp = list1;
			list1 = list2;
			list2 = tmp;
		}
	}

	void NFA_Graph::GetEpsilonClosure(NFA_Node * node, List<NFA_Node *> & states)
	{
		RefPtr<List<NFA_Node *>> list1 = new List<NFA_Node *>();
		RefPtr<List<NFA_Node *>> list2 = new List<NFA_Node *>();
		list1->Add(node);
		ClearNodeFlags();
		while (list1->Count())
		{
			list2->Clear();
			for (int m=0; m<list1->Count(); m++)
			{
				NFA_Node * curNode = (*list1)[m];
				for (int i=0; i<curNode->Translations.Count(); i++)
				{
					
					if (!curNode->Translations[i]->CharSet)
					{
						if (!curNode->Translations[i]->NodeDest->Flag)
						{
							states.Add(curNode->Translations[i]->NodeDest);
							list2->Add(curNode->Translations[i]->NodeDest);
							curNode->Translations[i]->NodeDest->Flag = true;
						}
					}
				}
			}
			RefPtr<List<NFA_Node *>> tmp = list1;
			list1 = list2;
			list2 = tmp;
		}
	}

	void NFA_Graph::EliminateEpsilon()
	{
		List<NFA_Node *> validStates;
		GetValidStates(validStates);
		for (int i=0; i<validStates.Count(); i++)
		{
			NFA_Node * curState = validStates[i];
			List<NFA_Node *> closure;
			GetEpsilonClosure(curState, closure);
			// Add translations from epsilon closures
			for (int j=0; j<closure.Count(); j++)
			{
				NFA_Node * curNode = closure[j];
				for (int k=0; k<curNode->Translations.Count(); k++)
				{
					if (curNode->Translations[k]->CharSet)
					{
						// Generate a translation from curState to curNode->Dest[k]
						NFA_Translation * trans = CreateTranslation();
						trans->CharSet = curNode->Translations[k]->CharSet;
						trans->NodeSrc = curState;
						trans->NodeDest = curNode->Translations[k]->NodeDest;
						curState->Translations.Add(trans);
						trans->NodeDest->PrevTranslations.Add(trans);
					}
				}
				if (curNode == end)
				{
					curState->IsFinal = true;
					curState->TerminalIdentifier = end->TerminalIdentifier;
				}
			}
		}
		// Remove epsilon-translations and invalid states
		ClearNodeFlags();
		for (int i=0; i<validStates.Count(); i++)
		{
			validStates[i]->Flag = true;
		}
		for (int i=0; i<nodes.Count(); i++)
		{
			if (!nodes[i]->Flag)
			{
				// Remove invalid state
				for (int j=0; j<nodes[i]->PrevTranslations.Count(); j++)
				{
					NFA_Translation * trans = nodes[i]->PrevTranslations[j];
					trans->NodeSrc->RemoveTranslation(trans);
					int fid = translations.IndexOf(trans);
					if (fid != -1)
					{
						translations[fid] = 0;
						translations.RemoveAt(fid);
					}
				}
				for (int j=0; j<nodes[i]->Translations.Count(); j++)
				{
					NFA_Translation * trans = nodes[i]->Translations[j];
					trans->NodeDest->RemovePrevTranslation(trans);
					int fid = translations.IndexOf(trans);
					if (fid != -1)
					{
						translations[fid] = 0;
						translations.RemoveAt(fid);
					}
				}
			}
		}

		for (int i=0; i<validStates.Count(); i++)
		{
			for (int j=0; j<validStates[i]->Translations.Count(); j++)
			{
				NFA_Translation * trans = validStates[i]->Translations[j];
				if (!trans->CharSet)
				{
					validStates[i]->RemoveTranslation(trans);
					trans->NodeDest->RemovePrevTranslation(trans);
					int fid = translations.IndexOf(trans);
					if (fid != -1)
					{
						translations[fid] = 0;
						translations.RemoveAt(fid);
					}
				}
			}
		}

		int ptr = 0;
		while (ptr < nodes.Count())
		{
			if (!nodes[ptr]->Flag)
			{
				nodes[ptr] = 0;
				nodes.RemoveAt(ptr);
			}
			else
				ptr ++;
		}
	}

	String NFA_Graph::Interpret()
	{
		StringBuilder sb(4096);
		for (int i=0; i<nodes.Count(); i++)
		{
			sb.Append(L"State: ");
			if (nodes[i]->IsFinal)
				sb.Append(L"[");
			if (nodes[i] == start)
				sb.Append(L"*");
			sb.Append(String(nodes[i]->ID));
			if (nodes[i]->IsFinal)
				sb.Append(L"]");
			sb.Append(L'\n');
			for (int j=0; j<nodes[i]->Translations.Count(); j++)
			{
				sb.Append(L"\t");
				if (nodes[i]->Translations[j]->CharSet)
					sb.Append(nodes[i]->Translations[j]->CharSet->Reinterpret());
				else
					sb.Append(L"<epsilon>");
				sb.Append(L":");
				sb.Append(String(nodes[i]->Translations[j]->NodeDest->ID));
				sb.Append(L"\n");
			}
		}
		return sb.ProduceString();
		
	}

	void NFA_Graph::SetStartNode(NFA_Node *node)
	{
		start = node;
	}

	void NFA_Graph::CombineNFA(NFA_Graph * graph)
	{
		for (int i=0; i<graph->nodes.Count(); i++)
		{
			nodes.Add(graph->nodes[i]);
		}
		for (int i=0; i<graph->translations.Count(); i++)
		{
			translations.Add(graph->translations[i]);
		}

	}

	void NFA_Graph::SetTerminalIdentifier(int id)
	{
		for (int i=0; i<nodes.Count(); i++)
		{
			if (nodes[i]->IsFinal)
			{
				nodes[i]->TerminalIdentifier = id;
			}
		}
	}
}
}