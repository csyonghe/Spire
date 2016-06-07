#ifndef GX_REGEX_PARSER_H
#define GX_REGEX_PARSER_H

#include "../Basic.h"

namespace CoreLib
{
	namespace Text
	{
		using namespace CoreLib::Basic;

		class RegexCharSetNode;
		class RegexRepeatNode;
		class RegexConnectionNode;
		class RegexSelectionNode;

		class RegexNodeVisitor : public Object
		{
		public:
			virtual void VisitCharSetNode(RegexCharSetNode * node);
			virtual void VisitRepeatNode(RegexRepeatNode * node);
			virtual void VisitConnectionNode(RegexConnectionNode * node);
			virtual void VisitSelectionNode(RegexSelectionNode * node);
		};

		class RegexNode : public Object
		{
		public:
			virtual String Reinterpret() = 0;
			virtual void Accept(RegexNodeVisitor * visitor) = 0;
		};

		class RegexCharSet : public Object
		{
		private:
			List<RegexCharSet *> OriSet;
			void CopyCtor(const RegexCharSet & set);
		public:
			bool Neg;
			struct RegexCharRange
			{
				wchar_t Begin,End;
				bool operator == (const RegexCharRange & r);
			};
			List<RegexCharRange> Ranges;
			List<unsigned short> Elements; 
		
		public:
			RegexCharSet()
			{
				Neg = false;
			}
			RegexCharSet(const RegexCharSet & set);
			String Reinterpret();
			void Normalize();
			void Sort();
			void AddRange(RegexCharRange r);
			void SubtractRange(RegexCharRange r);
			bool Contains(RegexCharRange r);
			bool operator ==(const RegexCharSet & set);
			RegexCharSet & operator = (const RegexCharSet & set);
			static void InsertElement(List<RefPtr<RegexCharSet>> &L, RefPtr<RegexCharSet> & elem);
			static void RangeMinus(RegexCharRange r1, RegexCharRange r2, RegexCharSet & rs);
			static void CharSetMinus(RegexCharSet & s1, RegexCharSet & s2);
			static void RangeIntersection(RegexCharRange r1, RegexCharRange r2, RegexCharSet &rs);
			static void CalcCharElementFromPair(RegexCharSet * c1, RegexCharSet * c2, RegexCharSet & AmB, RegexCharSet & BmA, RegexCharSet & AnB);
			static void CalcCharElements(List<RegexCharSet *> & sets, List<RegexCharRange> & elements);
		};

		class RegexCharSetNode : public RegexNode
		{
		public:
			RefPtr<RegexCharSet> CharSet;
		public:
			String Reinterpret();
			virtual void Accept(RegexNodeVisitor * visitor);
			RegexCharSetNode();
		};

		class RegexRepeatNode : public RegexNode
		{
		public:
			enum _RepeatType
			{
				rtOptional, rtArbitary, rtMoreThanOnce, rtSpecified
			};
			_RepeatType RepeatType = rtOptional;
			int MinRepeat = -1, MaxRepeat = -1;
			RefPtr<RegexNode> Child;
		public:
			String Reinterpret();
			virtual void Accept(RegexNodeVisitor * visitor);
		};

		class RegexConnectionNode : public RegexNode
		{
		public:
			RefPtr<RegexNode> LeftChild, RightChild;
		public:
			String Reinterpret();
			virtual void Accept(RegexNodeVisitor * visitor);
		};

		class RegexSelectionNode : public RegexNode
		{
		public:
			RefPtr<RegexNode> LeftChild, RightChild;
		public:
			String Reinterpret();
			virtual void Accept(RegexNodeVisitor * visitor);
		};

		class RegexParser : public Object
		{
		private:
			String src;
			int ptr;
			RegexNode * ParseSelectionNode();
			RegexNode * ParseConnectionNode();
			RegexNode * ParseRepeatNode();
			RegexCharSet * ParseCharSet();
			wchar_t ReadNextCharInCharSet();
			int ParseInteger();
			bool IsOperator();
		public:
			struct SyntaxError
			{
				int Position;
				String Text;
			};
			List<SyntaxError> Errors;
			RefPtr<RegexNode> Parse(const String & regex); 
		};
	}
}

#endif