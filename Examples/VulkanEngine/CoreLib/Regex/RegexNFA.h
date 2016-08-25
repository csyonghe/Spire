#ifndef REGEX_NFA_H
#define REGEX_NFA_H

#include "RegexTree.h"

namespace CoreLib
{
	namespace Text
	{
		using namespace CoreLib::Basic;

		class NFA_Node;

		class NFA_Translation : public Object
		{
		public:
			RefPtr<RegexCharSet> CharSet;
			NFA_Node * NodeSrc, * NodeDest;
			NFA_Translation();
			NFA_Translation(NFA_Node * src, NFA_Node * dest, RefPtr<RegexCharSet> charSet);
			NFA_Translation(NFA_Node * src, NFA_Node * dest);
		};

		class NFA_Node : public Object
		{
		private:
			static int HandleCount;
		public:
			int ID = -1;
			bool Flag = 0;
			bool IsFinal = false;
			int TerminalIdentifier;
			List<NFA_Translation *> Translations;
			List<NFA_Translation *> PrevTranslations;
			void RemoveTranslation(NFA_Translation * trans);
			void RemovePrevTranslation(NFA_Translation * trans);
			NFA_Node();
		};

		class NFA_Graph : public RegexNodeVisitor
		{
			friend class DFA_Graph;
		private:
			NFA_Node * start = nullptr, * end = nullptr;
			struct NFA_StatePair
			{
				NFA_Node * start = nullptr, * end = nullptr;
			};
			List<NFA_StatePair> stateStack;
			NFA_StatePair PopState();
			void PushState(NFA_StatePair s);
		private:
			List<RefPtr<NFA_Node>> nodes;
			List<RefPtr<NFA_Translation>> translations;
			void ClearNodes();
			void ClearNodeFlags();
			void GetValidStates(List<NFA_Node *> & states);
			void GetEpsilonClosure(NFA_Node * node, List<NFA_Node *> & states);
			void EliminateEpsilon();
		public:
			NFA_Node * CreateNode();
			NFA_Translation * CreateTranslation();
			String Interpret();
			void GenerateFromRegexTree(RegexNode * tree, bool elimEpsilon = true);
			void PostGenerationProcess();
			void CombineNFA(NFA_Graph * graph);
			NFA_Node * GetStartNode();
			void SetStartNode(NFA_Node * node);
			void SetTerminalIdentifier(int id);
			virtual void VisitCharSetNode(RegexCharSetNode * node);
			virtual void VisitRepeatNode(RegexRepeatNode * node);
			virtual void VisitConnectionNode(RegexConnectionNode * node);
			virtual void VisitSelectionNode(RegexSelectionNode * node);
		};
	}
}


#endif