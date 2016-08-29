#include "LibUI.h"
#include "../TextIO.h"
namespace GraphicsUI
{
	using namespace CoreLib;
	using namespace VectorMath;

	bool IsPunctuation(unsigned int ch)
	{
		if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
			return false;
		return true;
	}

	class TextLine
	{
		List<int> lineStarts;
	public:
		List<unsigned int> Chars;
		int DisplayLines = 1;
		TextLine()
		{
			lineStarts.Clear();
		}
		int GetSubLineIndex(int col)
		{
			if (DisplayLines == 1)
				return 0;
			int result = 0;
			while (result < lineStarts.Count() && lineStarts[result] <= col)
				result++;
			return result;
		}
		int GetSubLineStartCharIndex(int subLine)
		{
			if (subLine <= 0)
				return 0;
			else if (subLine <= lineStarts.Count())
				return lineStarts[subLine - 1];
			else if (lineStarts.Count())
				return lineStarts.Last();
			else
				return 0;
		}
		int GetSubLineEndCharIndex(int subLine)
		{
			if (lineStarts.Count() == 0)
				return Chars.Count();
			if (subLine < 0)
				subLine = 0;
			if (subLine < lineStarts.Count())
				return lineStarts[subLine];
			else
				return Chars.Count();
		}
		String ToString()
		{
			StringBuilder sb;
			for (auto ch : Chars)
				if (ch == L'\t')
					sb << L"    ";
				else
					sb << (wchar_t)ch;
			return sb.ProduceString();
		}
		String GetDisplayLine(int i, int tabSpaces, int charCount = -1)
		{
			StringBuilder sb;
			if (charCount == 0)
				return String();
			if (lineStarts.Count() == 0)
			{
				if (charCount == -1)
					charCount = Chars.Count();
				for (auto ch : Chars)
				{
					if (ch == L'\t')
					{
						for (int z = 0; z < tabSpaces; z++)
							sb << L" ";
					}
					else
						sb << (wchar_t)ch;
					charCount--;
					if (charCount == 0)
						break;
				}
			}
			else
			{
				int start = i == 0 ? 0 : lineStarts[i - 1];
				int end = i < lineStarts.Count() ? lineStarts[i] : Chars.Count();
				if (charCount == -1)
					charCount = end - start;
				for (int j = start; j < end; j++)
				{
					if (Chars[j] == L'\t')
					{
						for (int z = 0; z < tabSpaces; z++)
							sb << L" ";
					}
					else
						sb << (wchar_t)Chars[j];
					charCount--;
					if (charCount == 0)
						break;
				}
			}
			return sb.ProduceString();
		}

		void ClearWordWrap()
		{
			DisplayLines = 1;
			lineStarts.Clear();
		}
		template<typename StringMeasurer>
		void WordWrap(int width, StringMeasurer & measurer)
		{
			DisplayLines = 1;
			lineStarts.Clear();
			int ptr = 0;
			while (ptr < Chars.Count())
			{
				int lineStart = ptr;
				int space = width;
				int lastBreakable = lineStart;
				int i = ptr;
				while (i < Chars.Count())
				{
					int charWidth = measurer.GetCharWidth(Chars[i]);
					if (space > charWidth)
					{
						if (IsPunctuation(Chars[i]))
							lastBreakable = i + 1;
						space -= charWidth;
						i++;
					}
					else
					{
						DisplayLines++;
						break;
					}
					
				}
				if (i < Chars.Count())
				{
					if (lastBreakable > lineStart)
						ptr = lastBreakable;
					else if (i == ptr)
						ptr = i + 1;
					else
						ptr = i;
					lineStarts.Add(Math::Min(Chars.Count(), ptr));
				}
				else
					break;
			}
		}
	};
	class TextBuffer
	{
	public:
		List<TextLine> Lines;
		TextBuffer()
		{
			Lines.Add(TextLine());
		}
		String GetAllText()
		{
			StringBuilder sb;
			for (int i = 0; i < Lines.Count(); i++)
			{
				for (auto & ch : Lines[i].Chars)
					sb << (wchar_t)ch;
				if (i < Lines.Count() - 1)
					sb << L'\n';
			}
			return sb.ProduceString();
		}
		int GetDisplayLineCount()
		{
			int count = 0;
			for (auto & line : Lines)
				count += line.DisplayLines;
			return count;
		}
		CaretPos LogicalToPhysical(const CaretPos & logical)
		{
			CaretPos rs;
			rs.Line = 0;
			int line = Math::Clamp(logical.Line, 0, Lines.Count() - 1);
			for (int i = 0; i < line; i++)
				rs.Line += Lines[i].DisplayLines;
			int subLine = Lines[line].GetSubLineIndex(logical.Col);
			rs.Line += subLine;
			rs.Col = logical.Col - Lines[line].GetSubLineStartCharIndex(subLine);
			return rs;
		}
		CaretPos PhysicalToLogical(const CaretPos & physical, bool wordWrap, int & subLine)
		{
			CaretPos rs;
			int logicalLine, logicalCol;
			if (!wordWrap)
			{
				subLine = 0;
				logicalLine = physical.Line;
				logicalCol = 0;
				if (logicalLine < 0)
					logicalLine = logicalCol = 0;
				else if (logicalLine < Lines.Count())
					logicalCol = Math::Clamp(physical.Col, 0, Lines[logicalLine].Chars.Count());
				else
				{
					logicalLine = Lines.Count() - 1;
					logicalCol = Lines.Last().Chars.Count();
				}
			}
			else
			{
				logicalLine = 0;
				subLine = physical.Line;
				while (logicalLine < Lines.Count() && subLine >= Lines[logicalLine].DisplayLines)
				{
					subLine -= Lines[logicalLine].DisplayLines;
					logicalLine++;
				}
				if (logicalLine < Lines.Count())
					logicalCol = Math::Min(Lines[logicalLine].GetSubLineStartCharIndex(subLine) + physical.Col,
						Lines[logicalLine].GetSubLineEndCharIndex(subLine));
				else
				{
					logicalLine = Lines.Count() - 1;
					logicalCol = Lines.Last().Chars.Count();
				}
			}
			return CaretPos(logicalLine, logicalCol);
		}
		String GetDisplayLine(int i, bool wordWrap, int tabSpaces, int & logicalLine, int & logicalCol)
		{
			if (!wordWrap)
			{
				logicalLine = i;
				logicalCol = 0;
				if (i < Lines.Count())
					return Lines[i].ToString();
				else
					return String();
			}
			else
			{
				logicalLine = 0;
				int subLine = i;
				while (logicalLine < Lines.Count() && subLine >= Lines[logicalLine].DisplayLines)
				{
					subLine -= Lines[logicalLine].DisplayLines;
					logicalLine++;
				}
				if (logicalLine < Lines.Count())
				{
					logicalCol = Lines[logicalLine].GetSubLineStartCharIndex(subLine);
					return Lines[logicalLine].GetDisplayLine(subLine, tabSpaces);
				}
				else
				{
					logicalCol = 0;
					return String();
				}
			}
		}
		String GetLine(int i)
		{
			if (i < 0 || i >= Lines.Count())
				return String();
			StringBuilder sb;
			for (auto & ch : Lines[i].Chars)
				sb << (wchar_t)ch;
			return sb.ProduceString();
		}
		
		void SetText(const String & text)
		{
			Lines.Clear();
			Lines.Add(TextLine());
			int line = 0;
			bool ignoreN = false;
			for (auto & ch : text)
			{
				if (ignoreN && ch == L'\n')
				{
					ignoreN = false;
					continue;
				}
				if (ch == L'\r')
				{
					ignoreN = true;
					line++;
					Lines.Add(TextLine());
				}
				else
				{
					if (ch == L'\n')
					{
						line++;
						Lines.Add(TextLine());
					}
					else
						Lines[line].Chars.Add(ch);
					ignoreN = false;
				}
			}
		}
	};

	struct ScreenLine
	{
		Label * label;
		int LogicalLine;
		int LogicalCol;
	};

	enum class OperationName
	{
		None,
		InsertText,
		DeleteText,
		ReplaceText
	};

	class Operation
	{
	public:
		OperationName Name = OperationName::None;
		String Text, Text1;
		CaretPos SelEnd1; // for replace command
		CaretPos SelStart, SelEnd, Pos;
	};

	class OperationStack
	{
	public:
		List<Operation> operations;
		int undoPtr = 0;
		int firstPtr = 0;
		int endPtr = 0;
		bool locked = false;
		bool IsBufferEmpty()
		{
			return undoPtr != firstPtr;
		}
		Operation & GetLastOperation()
		{
			return operations[(operations.Count() + (endPtr - 1)) % operations.Count()];
		}
		CoreLib::Diagnostics::TimePoint lastOperationTime;
		OperationStack()
		{
			operations.SetSize(1024);
		}
		void SetSize(int size)
		{
			operations.SetSize(size);
			Clear();
		}
		void Lock()
		{
			locked = true;
		}
		void Unlock()
		{
			locked = false;
		}
		void PushOperation(const Operation & op)
		{
			if (locked)
				return;
			CoreLib::Diagnostics::TimePoint time = CoreLib::Diagnostics::PerformanceCounter::Start();
			auto & lastOp = GetLastOperation();
			if (op.Name == OperationName::InsertText && endPtr != firstPtr && lastOp.Name == OperationName::InsertText
				&& CoreLib::Diagnostics::PerformanceCounter::ToSeconds(time - lastOperationTime) < 0.5f &&
				op.Pos.Line == lastOp.Pos.Line &&
				op.Pos.Col == lastOp.Pos.Col + lastOp.Text.Length() &&
				op.SelEnd.Line == op.Pos.Line)
			{
				// merge insert commands
				lastOp.Text = lastOp.Text + op.Text;
				lastOp.SelEnd.Col = lastOp.Pos.Col + lastOp.Text.Length();
			}
			else
			{
				endPtr = undoPtr;
				operations[endPtr] = op;
				endPtr++;
				endPtr %= operations.Count();
				undoPtr = endPtr;
				if (endPtr == firstPtr)
				{
					firstPtr++;
					firstPtr %= operations.Count();
				}
			}
			lastOperationTime = time;
		}
		Operation GetNextRedo()
		{
			if (undoPtr != endPtr)
			{
				int oldUndoPtr = undoPtr;
				undoPtr++;
				undoPtr %= operations.Count();
				return operations[oldUndoPtr];
			}
			else
				return Operation();
		}
		Operation PopOperation()
		{
			if (undoPtr != firstPtr)
			{
				undoPtr--;
				undoPtr += operations.Count();
				undoPtr %= operations.Count();
				return operations[undoPtr];
			}
			else
				return Operation();
		}
		void Clear()
		{
			undoPtr = endPtr = firstPtr = 0;
		}
	};

	class MultiLineTextBoxImpl : public MultiLineTextBox
	{
	private:
		const int TabSpaces = 4;
		int desiredCol = 0;
		CaretPos caretPos, selStart, selEnd, wordSelStart;
		CaretPos physicalSelStart, physicalSelEnd;
		int caretDocumentPosX = 0;
		int caretDocumentPosY = 0;
		bool readOnly = false;
		bool caretPosChanged = true;
		bool selecting = false;
		bool wordSelecting = false;
		List<ScreenLine> screen;
		CoreLib::Diagnostics::TimePoint time;
		int lineHeight = 20;
		bool wordWrap = true;
		int maxLineWidth = 0;
		bool invalidateScreen = true;
		TextBuffer textBuffer;
		Container * content;
		ScrollBar * vScroll, *hScroll;
		Menu * contextMenu;
		Dictionary<unsigned int, int> textWidthCache;
		OperationStack operationStack;
		void SelectionChanged()
		{
			if (selStart < selEnd)
			{
				physicalSelStart = textBuffer.LogicalToPhysical(selStart);
				physicalSelEnd = textBuffer.LogicalToPhysical(selEnd);
			}
			else
			{
				physicalSelStart = textBuffer.LogicalToPhysical(selEnd);
				physicalSelEnd = textBuffer.LogicalToPhysical(selStart);
			}
			caretPosChanged = true;
			OnCaretPosChanged(this);
		}

		void UpdateCaretDocumentPos()
		{
			auto docPos = CaretPosToDocumentPos(caretPos);
			caretDocumentPosX = docPos.x;
			caretDocumentPosY = docPos.y;
		}
		Vec2i CaretPosToDocumentPos(const CaretPos & cpos)
		{
			int docPosX = 0;
			int docPosY = 0;
			if (cpos.Line >= 0 && cpos.Line < textBuffer.Lines.Count())
			{
				for (int i = 0; i < cpos.Line; i++)
				{
					docPosY += textBuffer.Lines[i].DisplayLines;
				}
				int subLine = textBuffer.Lines[cpos.Line].GetSubLineIndex(cpos.Col);
				docPosY += subLine;
				docPosY *= lineHeight;
				int start = textBuffer.Lines[cpos.Line].GetSubLineStartCharIndex(subLine);
				int end = Math::Min(cpos.Col, textBuffer.Lines[cpos.Line].Chars.Count());
				docPosX = font->MeasureString(textBuffer.Lines[cpos.Line].GetDisplayLine(subLine, TabSpaces, end - start)).w;
			}
			return Vec2i::Create(docPosX, docPosY);
		}
		CaretPos ScreenPosToCaretPos(int x, int y)
		{
			int docX = x + hScroll->GetPosition() - LeftIndent;
			int physicalLine = y / lineHeight + vScroll->GetPosition();
			if (physicalLine < 0)
				physicalLine = 0;
			int logicalLine = 0;
			int counter = physicalLine;

			while (logicalLine < textBuffer.Lines.Count() && counter >= textBuffer.Lines[logicalLine].DisplayLines)
			{
				counter -= textBuffer.Lines[logicalLine].DisplayLines;
				logicalLine++;
			}
			if (logicalLine >= textBuffer.Lines.Count())
			{
				return CaretPos(textBuffer.Lines.Count() - 1, textBuffer.Lines.Last().Chars.Count());
			}
			auto & line = textBuffer.Lines[logicalLine];
			int start = line.GetSubLineStartCharIndex(counter);
			int col = line.Chars.Count();
			auto str = line.GetDisplayLine(counter, TabSpaces);
			int cx = 0;
			for (int i = start; i < line.Chars.Count(); i++)
			{
				int chWidth = GetCharWidth(line.Chars[i]);
				int nx = font->MeasureString(line.GetDisplayLine(counter, TabSpaces, i - start + 1)).w;
				if (nx > docX)
				{
					if ((docX - cx) <= (chWidth >> 1))
						col = i;
					else
						col = i + 1;
					break;
				}
				cx = nx;
			}
			return CaretPos(logicalLine, col);
		}
		void CreateLineLabels(int lines)
		{
			screen.Clear();
			for (auto & lbl : content->GetChildren())
				lbl = nullptr;
			content->GetChildren().Clear();
			for (int i = 0; i < lines; i++)
			{
				auto lbl = new Label(content);
				ScreenLine sl;
				sl.label = lbl;
				sl.LogicalLine = 0;
				sl.LogicalCol = 0;
				lbl->Posit(LeftIndent, i * lineHeight, content->GetWidth(), lineHeight);
				lbl->Enabled = false;
				screen.Add(sl);
			}
		}
	public:
		int GetCharWidth(unsigned int ch)
		{
			int rs = 0;
			if (!textWidthCache.TryGetValue(ch, rs))
			{
				if (ch == L'\t')
					rs = font->MeasureString(L" ").w * TabSpaces;
				else
					rs = font->MeasureString(String((wchar_t)ch)).w;
				textWidthCache[ch] = rs;
			}
			return rs;
		}
	public:
		int LeftIndent = 4;
	private:
		void RefreshScreen()
		{
			for (int i = 0; i < screen.Count(); i++)
			{
				int logicalLine = 0, logicalCol = 0;
				auto lineTxt = textBuffer.GetDisplayLine(vScroll->GetPosition() + i, wordWrap, TabSpaces, logicalLine, logicalCol);
				int start = 0;
				int pos = hScroll->GetPosition();
				int totalWidth = 0;
				while (start < lineTxt.Length() && pos > 0)
				{
					int chWidth = GetCharWidth(lineTxt[start]);
					totalWidth += chWidth;
					pos -= chWidth;
					start++;
				}
				int offset = 0;
				if (start > 0 && pos < 0)
				{
					start--;
					offset = -(GetCharWidth(lineTxt[start]) + pos);
				}
				int txtWidth = 0;
				int end = start;
				while (end < lineTxt.Length() && txtWidth < Width)
				{
					int chWidth = GetCharWidth(lineTxt[end]);
					totalWidth += chWidth;
					txtWidth += chWidth;
					end++;
				}
				for (int j = end; j < Math::Min(end + 100, lineTxt.Length()); j++)
					totalWidth += GetCharWidth(lineTxt[j]);
				if (totalWidth > maxLineWidth)
				{
					maxLineWidth = totalWidth;
				}
				screen[i].LogicalLine = logicalLine;
				screen[i].LogicalCol = logicalCol;
				screen[i].label->SetFont(font);
				screen[i].label->SetText(lineTxt.SubString(start, end - start));
				screen[i].label->Left = LeftIndent + offset;
			}
			int newHmax = Math::Max(0, maxLineWidth - content->GetWidth() + LeftIndent);
			hScroll->SetValue(0, newHmax,
				Math::Clamp(hScroll->GetPosition(), 0, newHmax), content->GetWidth() - LeftIndent);
		}
		void VScroll_Changed(UI_Base *)
		{
			invalidateScreen = true;
		}
		void HScroll_Changed(UI_Base *)
		{
			invalidateScreen = true;
		}
		void UpdateWordWrap()
		{
			if (wordWrap)
			{
				for (auto & line : textBuffer.Lines)
					line.WordWrap(content->GetWidth() - LeftIndent, *this);
			}
			else
			{
				for (auto & line : textBuffer.Lines)
					line.ClearWordWrap();
			}
		}
		void UpdateScrollBars()
		{
			int lineCount;
			if (wordWrap)
				lineCount = textBuffer.GetDisplayLineCount();
			else
				lineCount = textBuffer.Lines.Count();
			vScroll->SetValue(0, lineCount - 1,
				Math::Clamp(vScroll->GetPosition(), 0, lineCount - 1), content->GetHeight() / lineHeight);
			vScroll->LargeChange = vScroll->GetPageSize();
		}
	public:
		MultiLineTextBoxImpl(Container * parent)
			: MultiLineTextBox(parent)
		{
			Type = CT_MULTILINETEXTBOX;
			content = new Container(this);
			content->BorderStyle = BS_NONE;
			vScroll = new ScrollBar(this);
			vScroll->SetOrientation(SO_VERTICAL);
			hScroll = new ScrollBar(this);
			hScroll->SetOrientation(SO_HORIZONTAL);
			vScroll->SetWidth(Global::SCROLLBAR_BUTTON_SIZE);
			hScroll->SetHeight(Global::SCROLLBAR_BUTTON_SIZE);
			vScroll->OnChanged.Bind(this, &MultiLineTextBoxImpl::VScroll_Changed);
			hScroll->OnChanged.Bind(this, &MultiLineTextBoxImpl::HScroll_Changed);

			content->AcceptsFocus = false;
			vScroll->AcceptsFocus = false;
			hScroll->AcceptsFocus = false;
			content->TabStop = false;
			vScroll->TabStop = false;
			hScroll->TabStop = false;
			content->Enabled = false;
			WantsTab = true;
			content->BackColor = Global::Colors.EditableAreaBackColor;
			AcceptsFocus = TabStop = true;
			SetWordWrap(true);
			ResetCaretTimer();
			Cursor = CursorType::IBeam;
			SetFont(parent->GetFont());
			BorderStyle = BS_FLAT_;

			contextMenu = new Menu(this);
			auto mnUndo = new MenuItem(contextMenu, L"&Undo", L"Ctrl+Z");
			mnUndo->OnClick.Bind([this](auto) {Undo(); });
			auto mnRedo = new MenuItem(contextMenu, L"&Redo", L"Ctrl+Y");
			mnRedo->OnClick.Bind([this](auto) {Redo(); });
			new MenuItem(contextMenu);
			auto mnCut = new MenuItem(contextMenu, L"C&ut", L"Ctrl+X");
			mnCut->OnClick.Bind([this](auto) {Cut(); });
			auto mnCopy = new MenuItem(contextMenu, L"&Copy", L"Ctrl+C");
			mnCopy->OnClick.Bind([this](auto) {Copy(); });
			auto mnPaste = new MenuItem(contextMenu, L"&Paste", L"Ctrl+V");
			mnPaste->OnClick.Bind([this](auto) {Paste(); });
			auto mnSelAll = new MenuItem(contextMenu, L"&Select All", L"Ctrl+A");
			mnSelAll->OnClick.Bind([this](auto) {SelectAll(); });
		}
		virtual void DoDpiChanged() override
		{
			textWidthCache.Clear();
			if (font)
			{
				int fontLineHeight = this->font->MeasureString(L"X").h;
				lineHeight = (int)(fontLineHeight * 1.1f);
				CreateLineLabels(Height / lineHeight + 1);
			}
			hScroll->SmallChange = lineHeight;
			hScroll->LargeChange = lineHeight * 50;
			invalidateScreen = true;
			Container::DoDpiChanged();
		}
		virtual void SetReadOnly(bool value) override
		{
			readOnly = value;
		}
		virtual bool GetReadOnly() override
		{
			return readOnly;
		}
		void ResetCaretTimer()
		{
			time = CoreLib::Diagnostics::PerformanceCounter::Start();
		}
		virtual CaretPos GetCaretPos() override
		{
			return caretPos;
		}
		virtual void SetCaretPos(const CaretPos & pCaretPos) override
		{
			this->caretPos = pCaretPos;
			caretPosChanged = true;
			OnCaretPosChanged(this);
			ScrollToCaret();
		}
		virtual void MoveCaretToEnd() override
		{
			SetCaretPos(CaretPos(textBuffer.Lines.Count() - 1, textBuffer.Lines.Last().Chars.Count()));
			selStart = selEnd = caretPos;
			SelectionChanged();
		}
		virtual void ScrollToCaret() override
		{
			auto physicalPos = textBuffer.LogicalToPhysical(caretPos);
			if (physicalPos.Line < vScroll->GetPosition())
				vScroll->SetPosition(physicalPos.Line);
			else if (physicalPos.Line >= vScroll->GetPosition() + vScroll->GetPageSize())
				vScroll->SetPosition(physicalPos.Line - vScroll->GetPageSize() + 1);
			auto docPos = CaretPosToDocumentPos(caretPos);
			if (docPos.x < hScroll->GetPosition())
				hScroll->SetPosition(Math::Max(0, docPos.x - 100));
			else if (docPos.x > hScroll->GetPosition() + content->GetWidth() - LeftIndent)
			{
				hScroll->SetMax(Math::Max(hScroll->GetMax(), docPos.x - content->GetWidth() + LeftIndent));
				hScroll->SetPosition(Math::Max(docPos.x - content->GetWidth() + LeftIndent, 0));
			}
		}
		CaretPos FindPrevSeparator(CaretPos pos)
		{
			while (pos.Col > 0)
			{
				if (IsPunctuation(textBuffer.Lines[pos.Line].Chars[pos.Col - 1]))
					break;
				pos.Col--;
			}
			return pos;
		}
		CaretPos FindNextSeparator(CaretPos pos)
		{
			while (pos.Col < textBuffer.Lines[pos.Line].Chars.Count())
			{
				if (IsPunctuation(textBuffer.Lines[pos.Line].Chars[pos.Col]))
					break;
				pos.Col++;
			}
			return pos;
		}
	public:
		void UpdateWordSelection()
		{
			selStart = wordSelStart;
			if (selEnd < selStart)
			{
				selEnd = FindPrevSeparator(selEnd);
				selStart = FindNextSeparator(selStart);
			}
			else
			{
				selStart = FindPrevSeparator(wordSelStart);
				selEnd = FindNextSeparator(selEnd);
				if (selStart == selEnd && selStart.Col > 0)
					selStart.Col--;
			}
		}
		virtual bool DoMouseMove(int x, int y) override
		{
			if (!Enabled || !Visible)
				return false;
			Container::DoMouseMove(x, y);
			
			if (selecting)
			{
				selEnd = ScreenPosToCaretPos(x, y);
				if (wordSelecting)
					UpdateWordSelection();
				SetCaretPos(selEnd);
				desiredCol = textBuffer.LogicalToPhysical(caretPos).Col;
				SelectionChanged();
			}
			return true;
		}

		virtual bool DoDblClick() override
		{
			selecting = wordSelecting = true;
			wordSelStart = selStart;
			UpdateWordSelection();
			SetCaretPos(selEnd);
			desiredCol = textBuffer.LogicalToPhysical(caretPos).Col;
			SelectionChanged();
			Global::MouseCaptureControl = this;
			return true;
		}

		virtual bool DoMouseDown(int x, int y, SHIFTSTATE shift) override
		{
			if (!Enabled || !Visible)
				return false;
			Container::DoMouseDown(x, y, shift);
			SetFocus();
			auto newCaretPos = ScreenPosToCaretPos(x, y);
			if (shift == SS_BUTTONLEFT)
			{
				caretPos = selStart = selEnd = newCaretPos;
				selecting = true;
				Global::MouseCaptureControl = this;
				SelectionChanged();
			}
			else
			{
				CaretPos sel0, sel1;
				if (selStart < selEnd)
				{
					sel0 = selStart;
					sel1 = selEnd;
				}
				else
				{
					sel0 = selEnd;
					sel1 = selStart;
				}
				if (newCaretPos < sel0 || sel1 < newCaretPos)
				{
					caretPos = selStart = selEnd = newCaretPos;
					SelectionChanged();
				}
			}
			desiredCol = textBuffer.LogicalToPhysical(caretPos).Col;
			caretPosChanged = true;
			OnCaretPosChanged(this);
			ResetCaretTimer();
			return true;
		}
		virtual bool DoMouseUp(int x, int y, SHIFTSTATE shift) override
		{
			if (!Enabled || !Visible)
				return false;
			Container::DoMouseUp(x, y, shift);
			selecting = false;
			wordSelecting = false;
			ReleaseMouse();
			if (shift == SS_BUTTONRIGHT)
				contextMenu->Popup(x, y);
			return true;
		}

		bool IsNavigationKey(unsigned short key)
		{
			return (key == Keys::Left || key == Keys::Right || key == Keys::Down || key == Keys::Up
				|| key == Keys::PageDown || key == Keys::PageUp || key == Keys::Home || key == Keys::End);
		}

		void IncreaseLineIndent(CaretPos start, CaretPos end)
		{
			if (end < start)
				Swap(start, end);
			int lineBegin = Math::Max(0, start.Line);
			int lineEnd = Math::Min(textBuffer.Lines.Count() - 1, end.Line);
			start.Col = 0;
			end.Col = textBuffer.Lines[end.Line].Chars.Count();
			String oldText = GetTextFromRange(start, end);
			for (int i = lineBegin; i <= lineEnd; i++)
			{
				textBuffer.Lines[i].Chars.Insert(0, L'\t');
				if (wordWrap)
					textBuffer.Lines[i].WordWrap(content->GetWidth() - LeftIndent, *this);
			}
			TextModified();
			selStart = CaretPos(lineBegin, 0);
			selEnd = CaretPos(lineEnd, textBuffer.Lines[lineEnd].Chars.Count());
			SelectionChanged();

			Operation op;
			op.Name = OperationName::ReplaceText;
			op.Pos = start;
			op.SelStart = start; 
			op.SelEnd = end;
			op.Text = oldText;
			end.Col = textBuffer.Lines[end.Line].Chars.Count();
			op.SelEnd1 = end;
			op.Text1 = GetTextFromRange(start, end);
			operationStack.PushOperation(op);
		}

		void DecreaseLineIndent(CaretPos start, CaretPos end)
		{
			if (end < start)
				Swap(start, end);
			int lineBegin = Math::Max(0, start.Line);
			int lineEnd = Math::Min(textBuffer.Lines.Count() - 1, end.Line);
			start.Col = 0;
			end.Col = textBuffer.Lines[end.Line].Chars.Count();
			String oldText = GetTextFromRange(start, end);

			for (int i = lineBegin; i <= lineEnd; i++)
			{
				if (textBuffer.Lines[i].Chars.Count())
				{
					if (textBuffer.Lines[i].Chars.First() == L'\t')
						textBuffer.Lines[i].Chars.RemoveAt(0);
					else
					{
						for (int j = 0; j < TabSpaces; j++)
						{
							if (textBuffer.Lines[i].Chars.First() == L' ')
								textBuffer.Lines[i].Chars.RemoveAt(0);
							else
								break;
						}
					}
					if (wordWrap)
						textBuffer.Lines[i].WordWrap(content->GetWidth() - LeftIndent, *this);
				}
			}
			TextModified();
			selStart = CaretPos(lineBegin, 0);
			selEnd = CaretPos(lineEnd, textBuffer.Lines[lineEnd].Chars.Count());
			SelectionChanged();

			Operation op;
			op.Name = OperationName::ReplaceText;
			op.Pos = start;
			op.SelStart = start;
			op.SelEnd = end;
			op.Text = oldText;
			end.Col = textBuffer.Lines[end.Line].Chars.Count();
			op.SelEnd1 = end;
			op.Text1 = GetTextFromRange(start, end);
			operationStack.PushOperation(op);
		}

		bool DoKeyPress(unsigned short key, SHIFTSTATE shift) override
		{
			if (!Enabled || !Visible)
				return false;
			Control::DoKeyPress(key, shift);
			if ((shift & SS_CONTROL) == 0)
			{
				if (readOnly)
					return true;
				if (key >= Keys::Space)
				{
					InsertText((wchar_t)key);
				}
				else if (key == Keys::Return)
				{
					InsertText((wchar_t)key);
					if (caretPos.Line > 0)
					{
						StringBuilder spacesStr;
						for (int i = 0; i < textBuffer.Lines[caretPos.Line - 1].Chars.Count(); i++)
						{
							auto ch = textBuffer.Lines[caretPos.Line - 1].Chars[i];
							if (ch == L'\t' || ch == L' ')
								spacesStr << (wchar_t)ch;
							else
								break;
						}
						if (spacesStr.Length())
							InsertText(spacesStr.ProduceString());
					}
				}
				else if (key == Keys::Tab)
				{
					if ((shift & SS_SHIFT) == 0)
					{
						if (selEnd.Line != selStart.Line)
							IncreaseLineIndent(selStart, selEnd);
						else
							InsertText(L'\t');
					}
					else
					{
						DecreaseLineIndent(selStart, selEnd);
					}
					
				}
			}
			return true;
		}

		virtual bool DoKeyDown(unsigned short key, SHIFTSTATE shift) override
		{
			if (!Visible || !Enabled)
				return false;
			Control::DoKeyDown(key, shift);
			auto nPos = caretPos;
			if (IsNavigationKey(key))
			{
				int currentSubLine = textBuffer.Lines[nPos.Line].GetSubLineIndex(nPos.Col);
				switch (key)
				{
				case Keys::Left:
					nPos.Col--;
					if (nPos.Col < 0)
					{
						nPos.Line--;
						if (nPos.Line < 0)
							nPos.Line = nPos.Col = 0;
						else
							nPos.Col = textBuffer.Lines[nPos.Line].Chars.Count();
					}
					desiredCol = textBuffer.LogicalToPhysical(CaretPos(nPos)).Col;
					break;
				case Keys::Right:
					nPos.Col++;
					if (nPos.Col > textBuffer.Lines[nPos.Line].Chars.Count())
					{
						nPos.Line++;
						if (nPos.Line >= textBuffer.Lines.Count())
						{
							nPos.Line = textBuffer.Lines.Count() - 1;
							nPos.Col = textBuffer.Lines.Last().Chars.Count();
						}
						else
							nPos.Col = 0;
					}
					desiredCol = textBuffer.LogicalToPhysical(CaretPos(nPos)).Col;
					break;
				case Keys::Up:
				{
					currentSubLine--;
					if (currentSubLine < 0)
					{
						nPos.Line--;
						if (nPos.Line < 0)
						{
							nPos.Line = 0;
							currentSubLine = 0;
						}
						else
							currentSubLine = textBuffer.Lines[nPos.Line].DisplayLines - 1;
					}
					int startCol = textBuffer.Lines[nPos.Line].GetSubLineStartCharIndex(currentSubLine);
					int endCol = textBuffer.Lines[nPos.Line].GetSubLineEndCharIndex(currentSubLine);
					nPos.Col = Math::Min(startCol + desiredCol, endCol);
					break;
				}
				case Keys::Down:
				{
					currentSubLine++;
					if (currentSubLine >= textBuffer.Lines[nPos.Line].DisplayLines)
					{
						nPos.Line++;
						if (nPos.Line >= textBuffer.Lines.Count())
						{
							nPos.Line = textBuffer.Lines.Count() - 1;
							currentSubLine = textBuffer.Lines.Last().DisplayLines - 1;
						}
						else
							currentSubLine = 0;
					}
					int startCol = textBuffer.Lines[nPos.Line].GetSubLineStartCharIndex(currentSubLine);
					int endCol = textBuffer.Lines[nPos.Line].GetSubLineEndCharIndex(currentSubLine);
					nPos.Col = Math::Min(startCol + desiredCol, endCol);
					break;
				}
				case Keys::Home:
				{
					int startCol = textBuffer.Lines[nPos.Line].GetSubLineStartCharIndex(currentSubLine);
					nPos.Col = startCol;
					desiredCol = 0;
					break;
				}
				case Keys::End:
				{
					int endCol = textBuffer.Lines[nPos.Line].GetSubLineEndCharIndex(currentSubLine);
					nPos.Col = endCol;
					desiredCol = endCol - textBuffer.Lines[nPos.Line].GetSubLineStartCharIndex(currentSubLine);
					break;
				}
				case Keys::PageDown:
				{
					auto physical = textBuffer.LogicalToPhysical(caretPos);
					physical.Line += vScroll->GetPageSize();
					physical.Col = desiredCol;
					int subLine;
					nPos = textBuffer.PhysicalToLogical(physical, wordWrap, subLine);
					break;
				}
				case Keys::PageUp:
				{
					auto physical = textBuffer.LogicalToPhysical(caretPos);
					physical.Line -= vScroll->GetPageSize();
					physical.Col = desiredCol;
					int subLine;
					nPos = textBuffer.PhysicalToLogical(physical, wordWrap, subLine);
					break;
				}
				}
				if (shift == SS_SHIFT)
				{
					selEnd = nPos;
					SetCaretPos(selEnd);
					SelectionChanged();
				}
				else
				{
					selStart = selEnd = nPos;
					SetCaretPos(nPos);
					caretPosChanged = true;
				}
			}
			else if (shift == SS_CONTROL)
			{
				if (key == 'A') // select all
					SelectAll();
				else if (key == 'C') // copy
					Copy();
				else if (key == 'X') // cut
					Cut();
				else if (key == 'V') // paste
					Paste();
				else if (key == 'Z') // undo
					Undo();
				else if (key == 'Y') // redo
					Redo();
			}
			else if (shift == SS_SHIFT)
			{
				if (key == Keys::Delete)
					Cut();
			}
			else
			{
				if (readOnly)
					return true;
				if (key == Keys::Delete)
				{
					DeleteAfterCaret();
					TextModified();
				}
				else if (key == Keys::Backspace)
				{
					DeleteBeforeCaret();
					TextModified();
				}
			}
			ResetCaretTimer();
			return true;
		}

		void Cut() override
		{
			if (readOnly)
				return;
			String text;
			if (selStart != selEnd)
			{
				text = GetSelectionText();
				DeleteSelection();
			}
			else
			{
				text = textBuffer.GetLine(caretPos.Line);
				auto start = caretPos;
				auto end = caretPos;
				start.Col = 0;
				end.Line++;	end.Col = 0;
				Delete(start, end);
			}
			GetEntry()->System->SetClipboardText(text);
			TextModified();
		}

		void Copy() override
		{
			String text = GetSelectionText();
			if (text.Length() == 0)
				text = textBuffer.GetLine(caretPos.Line);
			GetEntry()->System->SetClipboardText(text);
		}

		void Paste() override
		{
			if (readOnly)
				return;
			auto text = GetEntry()->System->GetClipboardText();
			InsertText(text);
		}

		virtual void SelectAll() override
		{
			selStart.Col = 0;
			selStart.Line = 0;
			selEnd.Line = textBuffer.Lines.Count() - 1;
			selEnd.Col = textBuffer.Lines.Last().Chars.Count();
			caretPos = selEnd;
			SelectionChanged();
			ScrollToCaret();
		}

		virtual void Undo() override
		{
			if (readOnly)
				return;
			auto op = operationStack.PopOperation();
			operationStack.Lock();
			switch (op.Name)
			{
			case OperationName::InsertText:
				Delete(op.SelStart, op.SelEnd);
				caretPos = selStart = selEnd = op.Pos;
				TextModified();
				break;
			case OperationName::DeleteText:
				selStart = selEnd = caretPos = op.SelStart;
				InsertText(op.Text);
				break;
			case OperationName::ReplaceText:
				selStart = op.SelStart;
				caretPos = selEnd = op.SelEnd1;
				InsertText(op.Text);
				break;
			case OperationName::None:
				break;
			}
			operationStack.Unlock();
		}

		virtual void Redo() override
		{
			if (readOnly)
				return;
			auto op = operationStack.GetNextRedo();
			operationStack.Lock();
			switch (op.Name)
			{
			case OperationName::DeleteText:
				Delete(op.SelStart, op.SelEnd);
				caretPos = selStart = selEnd = op.SelStart;
				TextModified();
				break;
			case OperationName::InsertText:
				selStart = selEnd = caretPos = op.Pos;
				InsertText(op.Text);
				break;
			case OperationName::ReplaceText:
				selStart = op.SelStart;
				selEnd = op.SelEnd;
				InsertText(op.Text1);
				break;
			case OperationName::None:
				break;
			}
			operationStack.Unlock();
		}

		virtual void SetWordWrap(bool pValue) override
		{
			wordWrap = pValue;
			UpdateWordWrap();
			hScroll->Visible = !pValue;
			SizeChanged();
			invalidateScreen = true;
		}

		virtual bool GetWordWrap() override
		{
			return wordWrap;
		}

		void Delete(CaretPos start, CaretPos end)
		{
			Operation op;
			op.SelStart = start;
			op.SelEnd = end;
			op.Pos = caretPos;
			op.Name = OperationName::DeleteText;
			op.Text = GetTextFromRange(start, end);
			operationStack.PushOperation(op);

			selecting = wordSelecting = false;
			auto & lineStart = textBuffer.Lines[start.Line];
			auto lineEnd = textBuffer.Lines[end.Line];
			if (start.Col < lineStart.Chars.Count())
				lineStart.Chars.SetSize(start.Col);
			else
				start.Col = lineStart.Chars.Count();

			if (end.Col < lineEnd.Chars.Count())
				lineStart.Chars.AddRange(lineEnd.Chars.Buffer() + end.Col, lineEnd.Chars.Count() - end.Col);
			if (end.Line > start.Line)
				textBuffer.Lines.RemoveRange(start.Line + 1, end.Line - start.Line);
			if (wordWrap)
				lineStart.WordWrap(content->GetWidth() - LeftIndent, *this);
		}

		void DeleteSelection()
		{
			if (selEnd < selStart)
				Swap(selStart, selEnd);
			selStart.Line = Math::Clamp(selStart.Line, 0, textBuffer.Lines.Count() - 1);
			selEnd.Line = Math::Clamp(selEnd.Line, 0, textBuffer.Lines.Count() - 1);
			Delete(selStart, selEnd);
			caretPos = selStart;
			selEnd = selStart;
		}

		void DeleteBeforeCaret()
		{
			if (selStart != selEnd)
				DeleteSelection();
			else
			{
				CaretPos prior = caretPos;
				prior.Col--;
				if (prior.Col < 0)
				{
					prior.Line--;
					if (prior.Line < 0)
						return;
					prior.Col = textBuffer.Lines[prior.Line].Chars.Count();
				}
				Delete(prior, caretPos);
				caretPos = prior;
			}
		}

		void DeleteAfterCaret()
		{
			if (selStart != selEnd)
				DeleteSelection();
			else
			{
				CaretPos after = caretPos;
				after.Col++;
				if (after.Col > textBuffer.Lines[after.Line].Chars.Count())
				{
					after.Line++;
					after.Col = 0;
					if (after.Line >= textBuffer.Lines.Count())
						return;
				}
				Delete(caretPos, after);
			}
		}

		void TextModified()
		{
			selStart = selEnd = caretPos;
			invalidateScreen = true;
			UpdateScrollBars();
			SelectionChanged();
			ScrollToCaret();
			auto physical = textBuffer.LogicalToPhysical(caretPos);
			desiredCol = physical.Col;
		}

		virtual void Delete() override
		{
			DeleteSelection();
			TextModified();
		}

		virtual void IncreaseIndent() override
		{
			if (readOnly)
				return;
			IncreaseLineIndent(selStart, selEnd);
		}

		virtual void DecreaseIndent() override
		{
			if (readOnly)
				return;
			DecreaseLineIndent(selStart, selEnd);
		}

		virtual void Select(CaretPos start, CaretPos end) override
		{
			caretPos = end;
			selStart = start;
			selEnd = end;
			SelectionChanged();
		}

		virtual void InsertText(const String & text) override
		{
			selecting = wordSelecting = false;
			if (selStart != selEnd)
				DeleteSelection();

			Operation op;
			op.Name = OperationName::InsertText;
			op.Text = text;
			op.Pos = caretPos;
			op.SelStart = caretPos;

			caretPos.Line = Math::Clamp(caretPos.Line, 0, textBuffer.Lines.Count() - 1);
			TextBuffer nText;
			nText.SetText(text);
			List<unsigned int> lastHalf;
			auto & line = textBuffer.Lines[caretPos.Line];
			caretPos.Col = Math::Clamp(caretPos.Col, 0, line.Chars.Count());
			for (int i = caretPos.Col; i < line.Chars.Count(); i++)
				lastHalf.Add(line.Chars[i]);
			nText.Lines.Last().Chars.AddRange(lastHalf);
			line.Chars.SetSize(caretPos.Col);
			line.Chars.AddRange(nText.Lines.First().Chars);
			if (wordWrap)
			{
				line.WordWrap(content->GetWidth() - LeftIndent, *this);
				for (int i = 1; i < nText.Lines.Count(); i++)
					nText.Lines[i].WordWrap(content->GetWidth() - LeftIndent, *this);
			}
			if (nText.Lines.Count() > 1)
			{
				textBuffer.Lines.InsertRange(caretPos.Line + 1, nText.Lines.Buffer() + 1, nText.Lines.Count() - 1);
				caretPos.Col = nText.Lines.Last().Chars.Count() - lastHalf.Count();
			}
			else
				caretPos.Col = line.Chars.Count() - lastHalf.Count();
			caretPos.Line += nText.Lines.Count() - 1;

			op.SelEnd = caretPos;
			operationStack.PushOperation(op);
			TextModified();
		}

		virtual String GetText() override
		{
			return textBuffer.GetAllText();
		}
		virtual VectorMath::Vec2i GetCaretScreenPos() override
		{
			int x = caretDocumentPosX + LeftIndent - hScroll->GetPosition();
			int y = caretDocumentPosY - vScroll->GetPosition() * lineHeight + lineHeight;
			int absX, absY;
			LocalPosToAbsolutePos(x, y, absX, absY);
			return Vec2i::Create(absX, absY);
		}
		virtual void SetText(const CoreLib::String & pText) override
		{
			operationStack.Clear();
			textBuffer.SetText(pText);
			caretPos = selStart = selEnd = CaretPos(0, 0);
			UpdateWordWrap();
			hScroll->SetValue(0, 0, 0, content->GetWidth() - LeftIndent);
			UpdateScrollBars();
			invalidateScreen = true;
		}
		virtual void SetFont(IFont * pFont) override
		{
			Control::SetFont(pFont);
			DoDpiChanged();
		}
		virtual bool DoMouseWheel(int delta) override
		{
			if (Visible && Enabled && vScroll->Visible)
			{
				vScroll->SetPosition(Math::Clamp(vScroll->GetPosition() + (delta > 0 ? -1 : 1) * 3, 0, vScroll->GetMax()));
				return true;
			}
			return false;
		}
		virtual void SizeChanged() override
		{
			int hScrollHeight = (hScroll->Visible ? hScroll->GetHeight() : 0);
			int vScrollWidth = (vScroll->Visible ? vScroll->GetWidth() : 0);
			content->Posit(0, 0, Width - vScrollWidth, Height - hScrollHeight);
			UpdateWordWrap();
			
			vScroll->Left = Width - vScroll->GetWidth();
			hScroll->Top = Height - hScroll->GetHeight();
			vScroll->SetHeight(Height - hScrollHeight);
			hScroll->SetWidth(Width - vScrollWidth + 2);
			UpdateScrollBars();
			CreateLineLabels(Height / lineHeight + 1);
			invalidateScreen = true;
			SelectionChanged();
		}
		virtual void SetScrollBars(bool vertical, bool horizontal) override
		{
			vScroll->Visible = vertical;
			hScroll->Visible = horizontal;
		}
		virtual String GetTextFromRange(CaretPos start, CaretPos end) override
		{
			StringBuilder sb;
			while (start < end)
			{
				while (start.Col >= textBuffer.Lines[start.Line].Chars.Count())
				{
					start.Line++;
					start.Col = 0;
					if (start.Line >= textBuffer.Lines.Count())
						break;
					sb << L"\n";
				}
				if (start.Line < textBuffer.Lines.Count())
					sb << (wchar_t)textBuffer.Lines[start.Line].Chars[start.Col];
				else
					break;
				start.Col++;
			}
			return sb.ProduceString();
		}

		virtual String GetSelectionText() override
		{
			CaretPos sel0, sel1;
			if (selStart < selEnd)
			{
				sel0 = selStart;
				sel1 = selEnd;
			}
			else
			{
				sel0 = selEnd;
				sel1 = selStart;
			}
			sel0.Line = Math::Clamp(sel0.Line, 0, textBuffer.Lines.Count() - 1);
			sel1.Line = Math::Clamp(sel1.Line, 0, textBuffer.Lines.Count() - 1);
			return GetTextFromRange(sel0, sel1);
		}
		virtual void ImeInputString(const String & txt) override
		{
			if (readOnly)
				return;
			InsertText(txt);
		}
		virtual void Draw(int absX, int absY) override
		{
			Control::Draw(absX, absY);
			if (invalidateScreen)
			{
				invalidateScreen = false;
				RefreshScreen();
			}
			if (caretPosChanged)
			{
				caretPosChanged = false;
				UpdateCaretDocumentPos();
			}
			if (vScroll->Visible && hScroll->Visible)
			{
				GetEntry()->DrawCommands.SolidBrushColor = Global::Colors.ScrollBarBackColor;
				GetEntry()->DrawCommands.FillRectangle(absX + Left + Width - vScroll->GetWidth() + 1, 
					absY + Top + Height - hScroll->GetHeight() - 1, absX + Left + Width - 1, absY + Top + Height - 1);
			}
			auto entry = GetEntry();
			auto & graphics = entry->DrawCommands;
			entry->ClipRects->AddRect(GraphicsUI::Rect(absX + Left + LeftIndent, absY + Top + 1, Width - 1, Height - 1));
			Container::DrawChildren(absX + Left, absY + Top);
			if (selStart != selEnd)
			{
				for (int i = 0; i < screen.Count(); i++)
				{
					int physLine = i + vScroll->GetPosition();
					int highlightStart = 0, highlightEnd = 0;
					if (physLine == physicalSelStart.Line)
					{
						highlightStart = physicalSelStart.Col;
						highlightEnd = (physicalSelEnd.Line == physLine ? physicalSelEnd.Col : -1);
					}
					else if (physLine == physicalSelEnd.Line)
					{
						highlightStart = 0;
						highlightEnd = physicalSelEnd.Col;
					}
					else if (physLine > physicalSelStart.Line && physLine < physicalSelEnd.Line)
					{
						highlightStart = 0;
						highlightEnd = -1;
					}
					else
						continue;
					highlightStart += screen[i].LogicalCol;
					int quadStart = LeftIndent - hScroll->GetPosition();
					auto & lineBuffer = textBuffer.Lines[screen[i].LogicalLine];
					auto & chars = lineBuffer.Chars;
					for (int j = screen[i].LogicalCol; j < highlightStart; j++)
					{
						int chId = j;
						if (chId < chars.Count())
							quadStart += GetCharWidth(chars[chId]);
					}
					if (highlightEnd == -1)
						highlightEnd = lineBuffer.GetSubLineEndCharIndex(lineBuffer.GetSubLineIndex(screen[i].LogicalCol));
					else
						highlightEnd += screen[i].LogicalCol;
					int quadEnd = quadStart;
					int ptr = highlightStart;
					while (ptr < highlightEnd && ptr < chars.Count())
					{
						quadEnd += GetCharWidth(chars[ptr]);
						ptr++;
					}
					graphics.SolidBrushColor = IsFocused() ? Global::Colors.SelectionColor : Global::Colors.UnfocusedSelectionColor;
					int ox = absX + Left;
					quadStart += ox;
					quadEnd += ox;
					int oy = absY + Top;
					entry->ClipRects->AddRect(Rect(ox + 1, oy + 1, content->GetWidth() - 1, content->GetHeight() - 1));
					entry->ClipRects->AddRect(Rect(quadStart, oy + i * lineHeight, quadEnd - quadStart, lineHeight));
					graphics.FillRectangle(quadStart, oy + i * lineHeight, quadEnd, oy + i * lineHeight + lineHeight);
					screen[i].label->FontColor = Global::Colors.SelectionForeColor;
					screen[i].label->Draw(ox, oy);
					screen[i].label->FontColor = Global::Colors.ControlFontColor;
					entry->ClipRects->PopRect();
					entry->ClipRects->PopRect();
				}
			}
			if (IsFocused())
			{
				// draw caret
				float timePassed = CoreLib::Diagnostics::PerformanceCounter::EndSeconds(time);
				int tick = int(timePassed / CURSOR_FREQUENCY);
				if (IsFocused() && ((tick & 1) == 0))
				{
					auto screenPos = GetCaretScreenPos();
					int absCursorPosX = screenPos.x;
					int absCursorPosY = screenPos.y;
					graphics.PenColor = Color(255 - BackColor.R, 255 - BackColor.G, 255 - BackColor.B, 255);
					graphics.DrawLine(absCursorPosX, absCursorPosY - lineHeight, absCursorPosX, absCursorPosY);
				}
			}
			entry->ClipRects->PopRect();
		}
		virtual void SetUndoStackSize(int size) override
		{
			operationStack.SetSize(size);
		}
		virtual int GetLineCount() override
		{
			return textBuffer.Lines.Count();
		}
		virtual CoreLib::String GetLine(int i) override
		{
			return textBuffer.GetLine(i);
		}
		virtual void DeleteLine(int i) override
		{
			textBuffer.Lines.RemoveAt(i);
			TextModified();
		}
	};

	MultiLineTextBox * CreateMultiLineTextBox(Container * parent)
	{
		return new MultiLineTextBoxImpl(parent);
	}
}