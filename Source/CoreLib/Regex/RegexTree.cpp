#include "RegexTree.h"
#include "../LibMath.h"

namespace CoreLib
{
namespace Text
{
	void RegexNodeVisitor::VisitCharSetNode(RegexCharSetNode * )
	{
	}

	void RegexNodeVisitor::VisitRepeatNode(RegexRepeatNode * )
	{

	}

	void RegexNodeVisitor::VisitSelectionNode(RegexSelectionNode * )
	{
	}

	void RegexNodeVisitor::VisitConnectionNode(RegexConnectionNode * )
	{

	}

	void RegexCharSetNode::Accept(RegexNodeVisitor * visitor)
	{
		visitor->VisitCharSetNode(this);
	}

	void RegexSelectionNode::Accept(RegexNodeVisitor * visitor)
	{
		visitor->VisitSelectionNode(this);
	}

	void RegexConnectionNode::Accept(RegexNodeVisitor * visitor)
	{
		visitor->VisitConnectionNode(this);
	}

	void RegexRepeatNode::Accept(RegexNodeVisitor *visitor)
	{
		visitor->VisitRepeatNode(this);
	}

	String RegexConnectionNode::Reinterpret()
	{
		return LeftChild->Reinterpret() + RightChild->Reinterpret();
	}

	String RegexSelectionNode::Reinterpret()
	{
		return LeftChild->Reinterpret() + L"|" + RightChild->Reinterpret();
	}

	String RegexRepeatNode::Reinterpret()
	{
		wchar_t t;
		if (RepeatType == RegexRepeatNode::rtArbitary)
			t = L'*';
		else if (RepeatType == rtOptional)
			t = L'?';
		else
			t = L'+';
		return String(L"(") + Child->Reinterpret() + L")" + t;
	}

	String RegexCharSet::Reinterpret()
	{
		if (Ranges.Count()== 1 && Ranges[0].Begin == Ranges[0].End &&
			!Neg)
		{
			return (Ranges[0].Begin>=28 && Ranges[0].Begin <127)? String((wchar_t)Ranges[0].Begin):
				String(L"<") + String((int)Ranges[0].Begin) + String(L">");
		}
		else
		{
			StringBuilder rs;
			rs.Append(L"[");
			if (Neg)
				rs.Append(L'^');
			for (int i=0; i<Ranges.Count(); i++)
			{
				if (Ranges[i].Begin == Ranges[i].End)
					rs.Append(Ranges[i].Begin);
				else
				{
					rs.Append(Ranges[i].Begin>=28 && Ranges[i].Begin<128?Ranges[i].Begin:
						String(L"<") + String((int)Ranges[i].Begin) + L">");
					rs.Append(L'-');
					rs.Append(Ranges[i].End>=28 && Ranges[i].End<128?Ranges[i].End:
						String(L"<") + String((int)Ranges[i].End)+ L">");
				}
			}
			rs.Append(L']');
			return rs.ProduceString();
		}
	}

	String RegexCharSetNode::Reinterpret()
	{
		return CharSet->Reinterpret();
	}

	void RegexCharSet::Sort()
	{
		for (int i=0; i<Ranges.Count()-1; i++)
		{
			for (int j=i+1; j<Ranges.Count(); j++)
			{
				RegexCharRange ri,rj;
				ri = Ranges[i];
				rj = Ranges[j];
				if (Ranges[i].Begin > Ranges[j].Begin)
				{
					RegexCharRange range = Ranges[i];
					Ranges[i] = Ranges[j];
					Ranges[j] = range;
				}
			}
		}
	}

	void RegexCharSet::Normalize()
	{
		for (int i=0; i<Ranges.Count()-1; i++)
		{
			for (int j=i+1; j<Ranges.Count(); j++)
			{
				if ((Ranges[i].Begin >= Ranges[j].Begin && Ranges[i].Begin <= Ranges[j].End) ||
					(Ranges[j].Begin >= Ranges[i].Begin && Ranges[j].Begin <= Ranges[i].End) )
				{
					Ranges[i].Begin = Math::Min(Ranges[i].Begin, Ranges[j].Begin);
					Ranges[i].End = Math::Max(Ranges[i].End, Ranges[j].End);
					Ranges.RemoveAt(j);
					j--;
				}
			}
		}
		Sort();
		if (Neg)
		{
			List<RegexCharRange> nranges;
			nranges.AddRange(Ranges);
			Ranges.Clear();
			RegexCharRange range;
			range.Begin = 1;
			for (int i=0; i<nranges.Count(); i++)
			{
				range.End = nranges[i].Begin-1;
				Ranges.Add(range);
				range.Begin = nranges[i].End+1;
			}
			range.End = 65530;
			Ranges.Add(range);
			Neg = false;
		}
	}

	bool RegexCharSet::Contains(RegexCharRange r)
	{
		for (int i=0; i<Ranges.Count(); i++)
		{
			if (r.Begin >= Ranges[i].Begin && r.End <= Ranges[i].End)
				return true;
		}
		return false;
	}

	void RegexCharSet::RangeIntersection(RegexCharRange r1, RegexCharRange r2, RegexCharSet & rs)
	{
		RegexCharRange r;
		r.Begin = Math::Max(r1.Begin,r2.Begin);
		r.End = Math::Min(r1.End, r2.End);
		if (r.Begin <= r.End)
			rs.Ranges.Add(r);
	}
	
	void RegexCharSet::RangeMinus(RegexCharRange r1, RegexCharRange r2, RegexCharSet & rs)
	{
		if (r2.Begin <= r1.Begin && r2.End>= r1.Begin && r2.End <= r1.End)
		{
			RegexCharRange r;
			r.Begin = ((int)r2.End + 1)>0xFFFF?0xFFFF:r2.End+1;
			r.End = r1.End;
			if (r.Begin <= r.End && !(r.Begin == r.End && r.Begin == 65530))
				rs.Ranges.Add(r);
		}
		else if (r2.Begin >= r1.Begin && r2.Begin <= r1.End && r2.End >= r1.End)
		{
			RegexCharRange r;
			r.Begin = r1.Begin;
			r.End = r2.Begin == 1? 1: r2.Begin - 1;
			if (r.Begin <= r.End && !(r.Begin == r.End == 1))
				rs.Ranges.Add(r);
		}
		else if (r2.Begin >= r1.Begin && r2.End <= r1.End)
		{
			RegexCharRange r;
			r.Begin = r1.Begin;
			r.End = r2.Begin == 1? 1: r2.Begin - 1;
			if (r.Begin <= r.End && !(r.Begin == r.End && r.Begin  == 1))
				rs.Ranges.Add(r);
			r.Begin = r2.End == 0xFFFF? r2.End : r2.End + 1;
			r.End = r1.End;
			if (r.Begin <= r.End && !(r.Begin == r.End && r.Begin  == 65530))
				rs.Ranges.Add(r);
		}
		else if (r2.End<r1.Begin || r1.End < r2.Begin)
		{
			rs.Ranges.Add(r1);
		}
	}

	void RegexCharSet::CharSetMinus(RegexCharSet & s1, RegexCharSet & s2)
	{
		RegexCharSet s;
		for (int i=0; i<s1.Ranges.Count(); i++)
		{
			for (int j=0; j<s2.Ranges.Count(); j++)
			{
				if (i>=s1.Ranges.Count() || i<0)
					return;
				s.Ranges.Clear();
				RangeMinus(s1.Ranges[i], s2.Ranges[j], s);
				if (s.Ranges.Count() == 1)
					s1.Ranges[i] = s.Ranges[0];
				else if (s.Ranges.Count() == 2)
				{
					s1.Ranges[i] = s.Ranges[0];
					s1.Ranges.Add(s.Ranges[1]);
				}
				else
				{
					s1.Ranges.RemoveAt(i);
					i--;
				}
			}
		}
	}

	RegexCharSet & RegexCharSet::operator = (const RegexCharSet & set)
	{
		CopyCtor(set);
		return *this;
	}

	bool RegexCharSet::RegexCharRange::operator == (const RegexCharRange & r)
	{
		return r.Begin == Begin && r.End == End;
	}

	void RegexCharSet::AddRange(RegexCharRange newR)
	{
		//RegexCharSet set;
		//set.Ranges.Add(r);
		//for (int i=0; i<Ranges.Count(); i++)
		//{
		//	if (Ranges[i].Begin < r.Begin && Ranges[i].End > r.Begin)
		//	{
		//		RegexCharRange nrange;
		//		nrange.Begin = r.Begin;
		//		nrange.End = Ranges[i].End;
		//		Ranges[i].End = r.Begin == 1? 1:r.Begin-1;
		//		if (!Ranges.Contains(nrange))
		//			Ranges.Add(nrange);
		//	}
		//	if (r.End > Ranges[i].Begin && r.End < Ranges[i].End)
		//	{
		//		RegexCharRange nrange;
		//		nrange.Begin = r.End == 0xFFFF ? 0xFFFF : r.End+1;
		//		nrange.End = Ranges[i].End;
		//		Ranges[i].End = r.End;
		//		if (!Ranges.Contains(nrange))
		//			Ranges.Add(nrange);
		//	}
		//	if (r.Begin == Ranges[i].Begin && r.End == Ranges[i].End)
		//		return;
		//}
		//for (int i=0; i<Ranges.Count(); i++)
		//	set.SubtractRange(Ranges[i]);
		//for (int i=0; i<set.Ranges.Count(); i++)
		//{
		//	for (int j=0; j<Ranges.Count(); j++)
		//		if (Ranges[j].Begin == set.Ranges[i].Begin ||
		//			Ranges[j].Begin == set.Ranges[i].End ||
		//			Ranges[j].End == set.Ranges[i].End||
		//			Ranges[j].End == set.Ranges[i].Begin)
		//		{
		//			RegexCharRange sr = set.Ranges[i];
		//			RegexCharRange r = Ranges[j];
		//			throw 0;
		//		}
		//	if (!Ranges.Contains(set.Ranges[i]))
		//		Ranges.Add(set.Ranges[i]);
		//}
		//Normalize();
		if (newR.Begin > newR.End)
			return;
		Sort();
		int rangeCount = Ranges.Count();
		for (int i=0; i<rangeCount; i++)
		{
			RegexCharRange & oriR = Ranges[i];
			if (newR.Begin > oriR.Begin)
			{
				if (newR.Begin > oriR.End)
				{

				}
				else if (newR.End > oriR.End)
				{
					RegexCharRange nRange;
					nRange.Begin = newR.Begin;
					nRange.End = oriR.End;
					wchar_t newR_begin = newR.Begin;
					newR.Begin = oriR.End + 1;
					oriR.End = newR_begin-1;
					Ranges.Add(nRange);
				}
				else if (newR.End == oriR.End)
				{
					oriR.End = newR.Begin - 1;
					Ranges.Add(newR);
					return;
				}
				else if (newR.End < oriR.End)
				{
					RegexCharRange nRange;
					nRange.Begin = newR.End + 1;
					nRange.End = oriR.End;
					oriR.End = newR.Begin - 1;
					Ranges.Add(newR);
					Ranges.Add(nRange);
					return;
				}
			}
			else if (newR.Begin == oriR.Begin)
			{
				if (newR.End > oriR.End)
				{
					newR.Begin = oriR.End + 1;
				}
				else if (newR.End == oriR.End)
				{
					return;
				}
				else
				{
					wchar_t oriR_end = oriR.End;
					oriR.End = newR.End;
					newR.End = oriR_end;
					newR.Begin = oriR.End + 1;
					Ranges.Add(newR);
					return;
				}
			}
			else if (newR.Begin < oriR.Begin)
			{
				if (newR.End > oriR.End)
				{
					RegexCharRange nRange;
					nRange.Begin = newR.Begin;
					nRange.End = oriR.Begin-1;
					Ranges.Add(nRange);
					newR.Begin = oriR.Begin;
					i--;
				}
				else if (newR.End == oriR.End)
				{
					RegexCharRange nRange;
					nRange.Begin = newR.Begin;
					nRange.End = oriR.Begin-1;
					Ranges.Add(nRange);
					return;
				}
				else if (newR.End < oriR.End && newR.End >= oriR.Begin)
				{
					RegexCharRange nRange;
					nRange.Begin = newR.Begin;
					nRange.End = oriR.Begin-1;
					Ranges.Add(nRange);
					nRange.Begin = newR.End+1;
					nRange.End = oriR.End;
					Ranges.Add(nRange);
					oriR.End = newR.End;
					return;
				}
				else
					break;
			}
		}
		Ranges.Add(newR);
		
	}

	void RegexCharSet::SubtractRange(RegexCharRange r)
	{
		
		int rc = Ranges.Count();
		for (int i=0; i<rc; i++)
		{
			RegexCharSet rs;
			RangeMinus(Ranges[i], r, rs);
			if (rs.Ranges.Count() == 1)
				Ranges[i] = rs.Ranges[0];
			else if (rs.Ranges.Count() == 0)
			{
				Ranges.RemoveAt(i);
				i--;
				rc--;
			}
			else
			{
				Ranges[i] = rs.Ranges[0];
				Ranges.Add(rs.Ranges[1]);
			}
		}
		Normalize();
	}

	void RegexCharSet::CalcCharElementFromPair(RegexCharSet * c1, RegexCharSet * c2, RegexCharSet & AmB, RegexCharSet & BmA, RegexCharSet & AnB)
	{
		AmB = *c1;
		BmA = *c2;
		CharSetMinus(AmB, *c2);
		CharSetMinus(BmA, *c1);
		for (int i=0; i<c1->Ranges.Count(); i++)
		{
			if (c2->Ranges.Count())
			{
				for (int j=0; j<c2->Ranges.Count(); j++)
				{
					RangeIntersection(c1->Ranges[i], c2->Ranges[j], AnB);
				}
			}
		}
		AmB.Normalize();
		BmA.Normalize();
		AnB.Normalize();
	}

	bool RegexCharSet::operator ==(const RegexCharSet & set)
	{
		if (Ranges.Count() != set.Ranges.Count())
			return false;
		for (int i=0; i<Ranges.Count(); i++)
		{
			if (Ranges[i].Begin != set.Ranges[i].Begin ||
				Ranges[i].End != set.Ranges[i].End)
				return false;
		}
		return true;
	}

	void RegexCharSet::InsertElement(List<RefPtr<RegexCharSet>> &L, RefPtr<RegexCharSet> & elem)
	{
		bool find = false;
		for (int i=0; i<L.Count(); i++)
		{
			if ((*L[i]) == *elem)
			{
				for (int k=0; k<elem->OriSet.Count(); k++)
				{
					if (!L[i]->OriSet.Contains(elem->OriSet[k]))
						L[i]->OriSet.Add(elem->OriSet[k]);
				}
				find = true;
				break;
			}
		}
		if (!find)
			L.Add(elem);
	}

	void RegexCharSet::CalcCharElements(List<RegexCharSet *> &sets, List<RegexCharRange> & elements)
	{
		RegexCharSet set;
		for (int i=0; i<sets.Count(); i++)
			for (int j=0; j<sets[i]->Ranges.Count(); j++)
				set.AddRange(sets[i]->Ranges[j]);
		for (int j=0; j<set.Ranges.Count(); j++)
		{
			for (int i=0; i<sets.Count(); i++)
			{
				if (sets[i]->Contains(set.Ranges[j]))
					sets[i]->Elements.Add((unsigned short)j);
			}
			elements.Add(set.Ranges[j]);
		}
		/*
		List<RefPtr<RegexCharSet>> L;
		if (!sets.Count())
			return;
		int lastSetCount = sets.Count();
		for (int i=0; i<sets.Count(); i++)
			sets[i]->OriSet.Add(sets[i]);
		L.Add(new RegexCharSet(*(sets[0])));
		for (int i=1; i<sets.Count(); i++)
		{
			RefPtr<RegexCharSet> bma = new RegexCharSet(*sets[i]);
			bma->OriSet = sets[i]->OriSet;
			for (int j=L.Count()-1; j>=0; j--)
			{
				RefPtr<RegexCharSet> bma2 = new RegexCharSet();
				RefPtr<RegexCharSet> amb = new RegexCharSet();
				RefPtr<RegexCharSet> anb = new RegexCharSet();
				CalcCharElementFromPair(L[j].operator ->(), sets[i], *amb, *bma2, *anb);
				CharSetMinus(*bma, *L[j]);
				L[j]->Normalize();
				amb->OriSet = L[j]->OriSet;
				anb->OriSet = amb->OriSet;
				for (int k=0; k<bma->OriSet.Count(); k++)
				{
					if (!anb->OriSet.Contains(bma->OriSet[k]))
						anb->OriSet.Add(bma->OriSet[k]);
				}
				if (amb->Ranges.Count())
				{
					L[j] = amb;
				}
				else
				{
					L[j] = 0;
					L.RemoveAt(j);
				}
				if (anb->Ranges.Count())
				{
					InsertElement(L,anb);
				}
			}
			if (bma->Ranges.Count())
			{
				InsertElement(L,bma);
			}

		}
		for (int i=0; i<L.Count(); i++)
		{
			for (int j=0; j<L[i]->OriSet.Count(); j++)
			{
				L[i]->OriSet[j]->Elements.Add(i);
			}
			elements.Add(L[i].Release());
		}
		for (int i=lastSetCount; i<sets.Count(); i++)
			RemoveAt sets[i];
		sets.SetSize(lastSetCount);

		*/
	}

	void RegexCharSet::CopyCtor(const RegexCharSet & set)
	{
		Ranges = set.Ranges;
		Elements = set.Elements;
		Neg = set.Neg;
		OriSet = set.OriSet;
	}

	RegexCharSet::RegexCharSet(const RegexCharSet & set)
	{
		CopyCtor(set);
	}


}
}