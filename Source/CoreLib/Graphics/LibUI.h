#ifndef GX_UI_H
#define GX_UI_H

#include "../VectorMath.h"
#include "../Basic.h"
#include "../Events.h"
#include "../WinForm/WinTimer.h"
#include "UISystemInterface.h"

namespace GraphicsUI
{
	const int STR_NAME_LENGTH = 32;
	const float CURSOR_FREQUENCY = 0.5f;

	// enum types defination

	const int COLOR_LIGHTEN = 100;
	// Dash dot pattern in binary : 1010101010101010
	const int DASH_DOT_PATTERN = 43690;
	const int DASH_DOT_FACTOR = 1;

	// Border Styles
	const int BS_NONE = 0;
	const int BS_RAISED = 1;
	const int BS_LOWERED = 2;
	const int BS_FLAT_ = 3;

	// Shift States
	const int SS_SHIFT = 1;
	const int SS_CONTROL = 2;
	const int SS_ALT = 4;
	const int SS_CONTROL_AND_SHIFT = 3;
	const int SS_CONTROL_AND_ALT = 6;
	const int SS_BUTTONLEFT = 8;
	const int SS_BUTTONMIDDLE = 16;
	const int SS_BUTTONRIGHT = 32;

	//Control Types
	//Bit 32: Determines whether the control is a container.
	const int CT_CONTROL = 0;
	const int CT_CONTAINER = 1048576;
	const int CT_ENTRY = 1048577;
	const int CT_FORM = 1048578;
	const int CT_LABEL = 1;
	const int CT_BUTTON = 2;
	const int CT_CHECKBOX = 4;
	const int CT_RADIOBOX = 8;
	const int CT_TEXTBOX = 16;
	const int CT_IMETEXTBOX = 17;
	const int CT_SCROLLBAR = 32;
	const int CT_LISTBOX = 64;
	const int CT_PROGRESSBAR = 128;
	const int CT_MENU = 256;
	const int CT_MENU_ITEM = 512;
	const int CT_TOOL_BUTTON = 513;


	typedef int CONTROLTYPE;
	typedef int BORDERSTYLE;
	typedef unsigned char SHIFTSTATE;

	//Clip Rect Stack
	const int MAX_CLIPRECT_STACK_SIZE = 32;

	//Combo box
	const int COMBOBOX_LIST_SIZE = 6;

	//ProgressBar Style
	const int PROGRESSBAR_STYLE_SMOOTH = 1;
	const int PROGRESSBAR_STYLE_NORMAL = 2;

	//Scrollbar Orientation
	const int SO_VERTICAL = 0;
	const int SO_HORIZONTAL = 1;

	const int SCROLLBAR_BUTTON_SIZE = 17;
	const int SCROLLBAR_MIN_PAGESIZE = 8;

	//Message Type defination
	const int MSG_UI_NOTIFY = 0;
	const int MSG_UI_CLICK = 1;
	const int MSG_UI_DBLCLICK = 2;
	const int MSG_UI_MOUSEDOWN = 3;
	const int MSG_UI_MOUSEUP = 4;
	const int MSG_UI_MOUSEMOVE = 5;
	const int MSG_UI_MOUSEENTER = 6;
	const int MSG_UI_MOUSELEAVE = 7;
	const int MSG_UI_MOUSEHOVER = 19;
	const int MSG_UI_KEYDOWN = 8;
	const int MSG_UI_KEYUP = 9;
	const int MSG_UI_KEYPRESS = 10;
	const int MSG_UI_CHANGED = 11;
	const int MSG_UI_RESIZE = 12;

	// this message(TopLayer Draw) is sent by Entry to notify controls to do some drawing above any other controls if necessary.
	const int MSG_UI_TOPLAYER_DRAW = 13;
	const int MSG_UI_MOUSEWHEEL = 14;
	// Form Messages
	const int MSG_UI_FORM_ACTIVATE = 15;
	const int MSG_UI_FORM_DEACTIVATE = 16;
	const int MSG_UI_FORM_SHOW = 17;
	const int MSG_UI_FORM_HIDE = 18;

	class UI_Base;

	struct UI_MsgArgs
	{
		UI_Base *Sender;
		CoreLib::String Tags;
		int Type;
		void *Data;
		UI_MsgArgs()
		{
			Sender = NULL;
			Data = NULL;
			Type = -1;
		};
	};


	struct UIMouseEventArgs
	{
		int X, Y;
		int Delta;
		SHIFTSTATE Shift;
	};

	struct UIKeyEventArgs
	{
		unsigned short Key;
		SHIFTSTATE Shift;
	};

	typedef CoreLib::Event<UI_Base *> NotifyEvent;
	typedef CoreLib::Event<UI_Base *, UIMouseEventArgs &> MouseEvent;
	typedef CoreLib::Event<UI_Base *, UIKeyEventArgs &> KeyEvent;



	class Graphics
	{
	public:
		static int DashPattern;
		static Color PenColor, SolidBrushColor;
		static Color GradiantBrushColor1, GradiantBrushColor2;
		static Color GradiantBrushColor3, GradiantBrushColor4;
		static void DrawArc(ISystemInterface * sys, int x, int y, int rad, float theta, float theta2);
		static void DrawRectangle(ISystemInterface * sys, int x1, int y1, int x2, int y2);
		static void FillRectangle(ISystemInterface * sys, int x1, int y1, int x2, int y2);
		static void DrawRoundRect(ISystemInterface * sys, int x1, int y1, int x2, int y2, int rad);
		static void FillRoundRect(ISystemInterface * sys, int x1, int y1, int x2, int y2, int rad);
		static void DrawLine(ISystemInterface * sys, int x1, int y1, int x2, int y2);
	};

	class ColorTable
	{
	public:
		Color ControlBackColor;
		Color ControlBorderColor;
		Color ToolButtonBorderHighLight;
		Color ToolButtonBorderSelected;
		Color ToolButtonBackColor1;
		Color ToolButtonBackColor2;
		Color ToolButtonBackColorHighlight1;
		Color ToolButtonBackColorHighlight2;
		Color ToolButtonBackColorPressed1;
		Color ToolButtonBackColorPressed2;
		Color ToolButtonBackColorChecked1;
		Color ToolButtonBackColorChecked2;
		Color ToolButtonSeperatorColor;
		Color ButtonBackColorChecked;
		Color StatusStripBackColor1;
		Color StatusStripBackColor2;
		Color StatusStripBackColor3;
		Color StatusStripBackColor4;
		Color MenuItemForeColor;
		Color MenuItemHighlightForeColor;
		Color MenuItemDisabledForeColor;
		Color MemuIconBackColor;
		Color MenuBackColor;
		Color MenuBorderColor;
		Color MenuSeperatorColor;
		Color TabPageBorderColor;
		Color TabPageItemBackColor1, TabPageItemBackColor2;
		Color TabPageItemSelectedBackColor1, TabPageItemSelectedBackColor2;
		Color TabPageItemHighlightBackColor1, TabPageItemHighlightBackColor2;
	};

	ColorTable CreateDefaultColorTable();

	class ClipRectStack
	{
	protected:
		ISystemInterface * system;
		Rect Buffer[MAX_CLIPRECT_STACK_SIZE];
	public:
		ClipRectStack(ISystemInterface * pSystem);
		~ClipRectStack();
	public:
		int StackSize;
		int WindowWidth, WindowHeight;
		void PushRect(Rect nRect);
		Rect PopRect();
		Rect GetTop();
		void AddRect(Rect nRect);//this function will calculate the intersection with the top of the stack and push it into the stack. It will automatically call the PushRect function.
		void Clear();
	};

	class UIEntry;
	class Container;
	class Label;
	class Control;
	class Global
	{
	public:
		static int EventGUID;
		static int HoverTimeThreshold;
		static Control * PointedComponent;
		static Control * MouseCaptureControl;
		static int CursorPosX;
		static int CursorPosY;
		static ColorTable ColorTable;
	};

	class UI_Base : public CoreLib::Object
	{
	public:
		virtual void HandleMessage(const UI_MsgArgs *Args);
	};

	enum Cursor
	{
		Arrow, Cross, IBeam, Wait, SizeNS, SizeWE, SizeNESW, SizeNWSE, SizeAll
	};

	class Control : public UI_Base
	{
		friend Container;
		friend UIEntry;
	protected:
		int EventID; // Used by Containers to avoid message resending;
	protected:
		CoreLib::WinForm::Timer tmrHover;
		IFont * font = nullptr;
		Rect clientRect;
		bool LastInClient;
		bool TopMost;
		int Height, Width;
		bool IsPointInClient(int X, int Y);
		virtual Control * FindControlAtPosition(int x, int y);
		void HoverTimerTick(Object * sender, CoreLib::WinForm::EventArgs e);
		void LocalPosToAbsolutePos(int x, int y, int & ax, int & ay);
	public:
		Control(Container * parent);
		~Control();
	public:
		GraphicsUI::Cursor Cursor;
		CoreLib::String Name;
		int ID;
		CONTROLTYPE Type;
		Container *Parent;
		int  Left, Top;
		bool BackgroundShadow;
		bool Enabled, Visible, Focused, TabStop = false, GenerateMouseHoverEvent = false;
		Color BackColor, FontColor, BorderColor;
		BORDERSTYLE BorderStyle;
		int AbsolutePosX; int AbsolutePosY;
		void SetName(CoreLib::String pName);
		void Posit(int pLeft, int pTop, int pWidth, int pHeight);
		void SetHeight(int val);
		void SetWidth(int val);
		int GetHeight();
		int GetWidth();
		Rect ClientRect();
		virtual void SizeChanged();
		virtual void Draw(int absX, int absY);
		virtual void SetFont(IFont * AFont);
		virtual void KillFocus();
		IFont * GetFont() { return font; }
		bool IsChildOf(Container * ctrl);
		void ReleaseMouse();
	public:
		virtual UIEntry * GetEntry();
	public:
		enum _DockStyle
		{
			dsNone, dsTop, dsBottom, dsLeft, dsRight, dsFill
		};
	public:
		_DockStyle DockStyle;
		NotifyEvent OnClick;
		NotifyEvent OnDblClick;
		NotifyEvent OnChanged;
		NotifyEvent OnResize;
		NotifyEvent OnMouseEnter;
		NotifyEvent OnMouseLeave;
		NotifyEvent OnMouseHover;
		NotifyEvent OnLostFocus;
		MouseEvent OnMouseDown;
		MouseEvent OnMouseMove;
		MouseEvent OnMouseUp;
		MouseEvent OnMouseWheel;
		KeyEvent   OnKeyDown;
		KeyEvent   OnKeyUp;
		KeyEvent   OnKeyPress;
		//Message
		void BroadcastMessage(const UI_MsgArgs *Args);
		//Event Reactions
		virtual bool DoMouseMove(int X, int Y);
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseEnter();
		virtual bool DoMouseLeave();
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyUp(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyPress(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoMouseHover();
		virtual bool DoClick();
		virtual bool DoDblClick();
		virtual void SetFocus();
		virtual bool ContainsFocus();
		virtual void LostFocus(Control * newFocus);
		virtual bool DoClosePopup();
	};

	class Container : public Control
	{
	protected:
		bool drawChildren = true;
		virtual Control * FindControlAtPosition(int x, int y);
	public:
		Container(Container * parent);
		~Container();
	public:
		CoreLib::List<CoreLib::RefPtr<Control>> Controls;
		CoreLib::List<Control *> TabList;
		int Margin;
		void AddChild(Control *nControl);
		void RemoveChild(Control *AControl);
		virtual bool ContainsFocus();
		virtual void ArrangeControls(Rect initalClientRect);
		virtual void SetAlpha(unsigned char Alpha);
		virtual void InternalBroadcastMessage(UI_MsgArgs *Args);
		virtual bool DoDblClick();
		virtual bool DoMouseLeave();
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyUp(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyPress(unsigned short Key, SHIFTSTATE Shift);
		virtual void Draw(int absX, int absY);
		virtual void DrawChildren(int absX, int absY);
		virtual void SizeChanged();
		virtual bool DoClosePopup();
		virtual void KillFocus();
	};

	struct FormStyle
	{
		Color TitleBarColors[4];
		Color TitleBarDeactiveColors[4];
		Color TitleBarFontColor;
		bool ShowIcon;
		Color CtrlButtonBackColor;
		IFont * TitleFont = nullptr;
		int CtrlButtonBorderStyle;
		int TitleBarHeight;
		bool TopMost;
		bool Sizeable = true;
		FormStyle();
	};

	class Label;
	class Button;

	enum class ResizeMode
	{
		None, Left = 1, Right = 2, Top = 4, Bottom = 8, TopLeft = 5, TopRight = 6, BottomLeft = 9, BottomRight = 10
	};

	class Form : public Container
	{
	protected:
		bool DownInTitleBar;
		bool DownInButton;
		int DownPosX,DownPosY;
		Control *btnClose;
		Label *lblClose;
		Label *lblTitle;
		CoreLib::String Text; 
		FormStyle formStyle;
		ResizeMode resizeMode = ResizeMode::None;
		ResizeMode GetResizeHandleType(int x, int y);
		void FormStyleChanged();
	public:
		Form(UIEntry * parent);
		~Form();
	public:
		NotifyEvent OnResize;
		NotifyEvent OnShow;
		NotifyEvent OnClose;

		bool ButtonClose;
		bool Activated;
			
		void SetText(CoreLib::String AText);
		CoreLib::String GetText();
		int GetClientHeight();
		int GetClientWidth();
		FormStyle GetFormStyle()
		{
			return formStyle;
		}
		void SetFormStyle(const FormStyle & FormStyle);

		virtual bool DoMouseMove(int X, int Y);
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
		virtual void Draw(int absX,int absY);
		virtual void SizeChanged();
		virtual void SetAlpha(unsigned char Alpha);
	};

	class Label: public Container
	{
	protected:
		CoreLib::String FCaption;
		bool FChanged;
		CoreLib::RefPtr<IBakedText> text = nullptr;
	public:
		int TextWidth = 0;
		int TextHeight = 0;
		Label(Container * parent);
		~Label();
	public:
		Color ShadowColor;
		bool DropShadow;
		bool AutoSize = true, MultiLine = false;
		virtual void SetText(const CoreLib::String & text);
		virtual void SetFont(IFont * pFont) override;
		void UpdateText();
		CoreLib::String GetText();
		virtual void Draw(int absX, int absY);
		virtual void SizeChanged();
	};

	class Button: public Label
	{
	protected:
		bool IsMouseDown;
	public:
		Button(Container * parent);
	public:
		bool Checked;
		Color FocusRectColor;
		virtual void Draw(int absX, int absY);
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyUp(unsigned short Key, SHIFTSTATE Shift);
			
	};

	class CheckBox : public Label
	{
	public:
		CheckBox(Container * parent);
	public:
		bool Checked;
		Color FocusRectColor;
		virtual void SetText(const CoreLib::String & text) override;
		virtual void Draw(int absX, int absY);
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
	};

	class RadioBox : public CheckBox
	{
	public:
		RadioBox(Container * parent);
	public:
		bool GetValue();
		void SetValue(bool AValue);
		virtual void Draw(int absX, int absY);
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
	};

	class CustomTextBox : public Control
	{
	protected:
		long long Time,Freq;
		CoreLib::String FText;
		IFont * font = nullptr;
		CoreLib::RefPtr<IBakedText> text;
		int SelOrigin;
		bool Changed, KeyDown,SelectMode;
		int LabelOffset;
		int HitTest(int posX);
		void CursorPosChanged();
	public:
		CustomTextBox(Container * parent);
	public:
		bool Locked;
		int TextBorderX,TextBorderY;
		int CursorPos = 0,SelStart = 0, SelLength = 0;
		int AbsCursorPosX,AbsCursorPosY;
		Color SelectionColor,SelectedTextColor;
		const CoreLib::String GetText();
		void TextChanged();
		void SetText(const CoreLib::String & AText);
		void SetFont(IFont * AFont);
		void CopyToClipBoard();
		void PasteFromClipBoard();
		void DeleteSelectionText();
		void SelectAll();
 		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseMove(int X, int Y);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyUp(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyPress(unsigned short Key, SHIFTSTATE Shift);
		bool DoInput(const CoreLib::String & AInput);
		virtual void Draw(int absX, int absY);
	};

	class TextBox : public CustomTextBox
	{
	public:
		TextBox(Container * parent)
			: CustomTextBox(parent)
		{}
		virtual bool DoKeyPress(unsigned short Key, SHIFTSTATE Shift);
	};

	class IMETextBox : public TextBox
	{
	public:
		IMETextBox(Container * parent) : TextBox(parent) {Type=CT_IMETEXTBOX;};
	};

	class IMEWindow : public Container
	{
	protected:
		CoreLib::List<CoreLib::String> CandidateList;
		Label * lblIMEName, *lblCompStr, *lblCompReadStr;
		CoreLib::List<Label *> lblCandList;
	public:
		IMEWindow(Container * parent);
		~IMEWindow();
	public:
		Control *Panel;
		int CursorPos,CandidateCount;
		int BorderWeight;
		int WindowWidth, WindowHeight;
		CoreLib::String strComp,strIME,strCompRead;
		bool ShowCandidate;
		void ChangeInputMethod(CoreLib::String AInput);
		void ChangeCompositionString(CoreLib::String AString);
		void ChangeCompositionReadString(CoreLib::String AString);
		void SetCandidateListItem(int Index, const wchar_t *Data);
		void SetCandidateCount(int Count);
		virtual void Draw(int absX, int absY);
	};

	class IMEHandler : public CoreLib::Object
	{
	public:
		IMEHandler(UIEntry * entry);
		~IMEHandler();
	public:
		bool Enabled;
		CustomTextBox *TextBox;
		IMEWindow *IMEWindow;
		int HandleMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		void StringInputed(CoreLib::String AString);
	};

	class UIEntry: public Container
	{
	private:
		CoreLib::List<Control*> controlStack;
	protected:
		SHIFTSTATE GetCurrentShiftState();
		void TranslateMouseMessage(UIMouseEventArgs &Data,WPARAM wParam, LPARAM lParam);
		void DeactivateAllForms();
	public:
		UIEntry(int WndWidth, int WndHeight, ISystemInterface * pSystem);
		ISystemInterface * System = nullptr;
	public:
		Label * CheckmarkLabel = nullptr;
		CoreLib::RefPtr<ClipRectStack> ClipRects;
		Control *FocusedControl;
		CoreLib::RefPtr<IMEHandler> FIMEHandler;
		Form *ActiveForm = nullptr;
		CoreLib::List<Form*> Forms;
		void DrawUI();
		void RemoveForm(Form *Form);
		void ShowWindow(Form *Form);
		void CloseWindow(Form *Form);
		int HandleSystemMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		virtual void InternalBroadcastMessage(UI_MsgArgs *Args);
		virtual void SizeChanged();
		virtual void Draw(int absX,int absY);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyUp(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyPress(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseMove(int X, int Y);
		virtual bool DoMouseWeel(int delta);
		virtual void HandleMessage(const UI_MsgArgs *Args);
		void MoveFocusBackward();
		void MoveFocusForward();
		int GetLineHeight()
		{
			return CheckmarkLabel->TextHeight;
		}
		void SetFocusedControl(Control *Target);

		virtual UIEntry * GetEntry() override
		{
			return this;
		}
		virtual Control * FindControlAtPosition(int x, int y) override;
	};

	class ScrollBar: public Container
	{
	protected:
		int tmrOrientation;
		CoreLib::WinForm::Timer tmrTick;
		bool DownInSlider;
		int OriPos;
		int DownPosX,DownPosY;
		int Orientation; // SO_VERTICAL or SO_HORIZONTAL
		int Position;
		int Max,Min;
		int PageSize;
		bool PointInSlider(int X, int Y);
		bool PointInFreeSpace(int X, int Y);
		void BtnIncMouseDown(UI_Base * sender, UIMouseEventArgs & e);
		void BtnDecMouseDown(UI_Base * sender, UIMouseEventArgs & e);
		void BtnIncMouseUp(UI_Base * sender, UIMouseEventArgs & e);
		void BtnDecMouseUp(UI_Base * sender, UIMouseEventArgs & e);
		void tmrTick_Tick(Object * sender, CoreLib::WinForm::EventArgs e);
		virtual Control* FindControlAtPosition(int x, int y) override
		{
			return Control::FindControlAtPosition(x, y);
		}
	public:
		ScrollBar(Container * parent);
		~ScrollBar();
	public:
		int SmallChange,LargeChange;
		Button *btnInc, *btnDec;
		Control *Slider;
		int GetOrientation();
		void SetOrientation(int NewOri);
		void SetValue(int AMin, int AMax, int APos, int pageSize);
		void SetMin(int AMin);
		void SetMax(int AMax);
		void SetPosition(int APos);
		int GetMin();
		int GetMax();
		int GetPosition();
		virtual void Draw(int absX, int absY);
		virtual bool DoMouseMove(int X, int Y);
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseUp(int X,int Y, SHIFTSTATE Shift);
		virtual void HandleMessage(const UI_MsgArgs *Args);
		virtual void SizeChanged();
	};

	class ListBox : public Container
	{
	protected:
		int HighLightID;
		int ItemHeight;
		bool Selecting;
		bool HotTrack;
		int SelOriX,SelOriY;
		int BorderWidth;
		bool DownInItem;
		bool selectionHasChanged = false;
		ScrollBar *ScrollBar;
		void ListChanged();
		void SelectionChanged();
		virtual Control* FindControlAtPosition(int x, int y) override
		{
			return Control::FindControlAtPosition(x, y);
		}
	public:
		ListBox(Container * parent);
	public:
		CoreLib::List<Control*> Items;
		bool HideSelection, MultiSelect;
		Color SelectionColor,UnfocusedSelectionColor,SelectionForeColor,FocusRectColor,HighLightColor,HighLightForeColor;
		CoreLib::List<Control *> Selection;
		int SelectedIndex;
		int Margin;
		int HitTest(int X, int Y);
		Control *GetSelectedItem();
		int GetItemHeight();
		Label *GetTextItem(int Index);
		CheckBox *GetCheckBoxItem(int Index);
		Control *GetItem(int Index);
		int AddTextItem(CoreLib::String Text);
		int AddCheckBoxItem(CoreLib::String Text);
		int AddControlItem(Control *Item);
		void Delete(int Index);
		void Delete(Control *Item);
		void Clear();
		bool ItemInSelection(Control *Item);
	public:
		virtual void Draw(int absX, int absY);
		virtual void SizeChanged();
		virtual void HandleMessage(const UI_MsgArgs *Args);
		virtual bool DoMouseDown(int x, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseUp(int x, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseMove(int x, int Y);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
	};

	class ComboBox : public ListBox
	{
	private:
		int lH,lW,lL,lT;
	protected:	
		int ListLeft,ListTop,ListHeight,ListWidth;
		int ButtonSize;
		bool ShowList;
		bool PosInList(int X, int Y);
		void ChangeSelectedItem(int id);
		void ToggleList(bool sl);
		void BeginListBoxFunctions();
		void EndListBoxFunctions();
	public:
		ComboBox(Container * parent);
	public:
		TextBox *TextBox;
		Button *btnDrop;
		int ButtonWidth;
		void SetText(const CoreLib::String & pText)
		{
			TextBox->SetText(pText);
		}
		CoreLib::String GetText()
		{
			return TextBox->GetText();
		}
		virtual void Draw(int absX, int absY);
		virtual void SizeChanged();
		virtual void HandleMessage(const UI_MsgArgs *Args);
		virtual bool DoMouseDown(int x, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseUp(int x, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseMove(int x, int Y);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
		virtual void SetFocus();
		virtual void LostFocus(Control * newFocus);
		virtual bool DoClosePopup();
	};

	class ProgressBar : public Control
	{
	protected:
		int Max,Position;
	public:
		Color ProgressBarColors[4];
		int Style;
		ProgressBar(Container * parent);
		~ProgressBar();
		void SetMax(int AMax);
		void SetPosition(int APos);
		int GetMax();
		int GetPosition();
		virtual void Draw(int absX, int absY);
	};

	class MenuItem;

	class Menu : public Container
	{
		friend MenuItem;
	private:
		int ItemHeight = 24;
		static const int Margin = 2;
	public:
		enum MenuStyle
		{
			msPopup, msMainMenu
		};
	private:
		CoreLib::List<MenuItem*> Items;
		MenuStyle style;
	protected:
		MenuItem * parentItem = nullptr;
		Menu * curSubMenu = nullptr;
		void PositMenuItems();
		void ItemSelected(MenuItem * item); // Called by MenuItem
		void PopupSubMenu(Menu * subMenu, int x, int y); // Called by MenuItem
		void CloseSubMenu();
		int GetSelectedItemID();
	public:
		NotifyEvent OnPopup;
		NotifyEvent OnMenuClosed;
	public:
		void AddItem(MenuItem * item);
		void RemoveItem(MenuItem * item);
		int Count();
		MenuItem * GetItem(int id);
		void DrawPopup();
		void DrawMenuBar(int absX, int absY);
		void Draw(int absX, int absY);
		void Popup(int x, int y);
		void CloseMenu();
		bool DoClosePopup();
		Menu(Container * parent, MenuStyle mstyle = msPopup);
		virtual bool DoMouseHover();
		virtual bool DoMouseMove(int X, int Y);
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
		virtual void HandleMessage(const UI_MsgArgs * Args);
		virtual void SetFocus();
	};

	class MenuItem : public Container
	{
		friend Menu;
	protected:
		wchar_t accKey;
		bool isButton;
		void ItemSelected(MenuItem * item); // Called by SubMenu
		virtual Control* FindControlAtPosition(int x, int y) override
		{
			return Control::FindControlAtPosition(x, y);
		}
	private:
		bool cursorInClient;
		static const int Margin = 8;
		bool isSeperator;
		Label *lblText = nullptr, *lblShortcut = nullptr;
		void Init();
	public:
		bool IsSeperator();
		bool Selected;
		bool Checked;
		CoreLib::String GetText();
		void SetText(const CoreLib::String & str);
		CoreLib::String GetShortcutText();
		void SetShortcutText(const CoreLib::String & str);
		Menu * SubMenu = nullptr;
		void AddItem(MenuItem * item);
		void RemoveItem(MenuItem * item);
		Menu * GetSubMenu();
		MenuItem * GetItem(int id);
		wchar_t GetAccessKey();
		int Count();
		int MeasureWidth(bool isButton = false);
		MenuItem::MenuItem(Menu * parent);
		MenuItem::MenuItem(MenuItem * parent);
		MenuItem(Menu* menu, const CoreLib::String & text);
		MenuItem(MenuItem* menu, const CoreLib::String & text);

		MenuItem(Menu* menu, const CoreLib::String & text, const CoreLib::String & shortcutText);
		MenuItem(MenuItem* menu, const CoreLib::String & text, const CoreLib::String & shortcutText);

		void DrawMenuItem(int width, int height);
		void DrawMenuButton(int width, int height);
		virtual bool DoMouseEnter();
		virtual bool DoMouseLeave();
		virtual bool DoMouseHover();
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoClick();
		virtual void HandleMessage(const UI_MsgArgs * Args);
	};
	
	class ImageDisplay : public Container
	{
	private:
		CoreLib::RefPtr<IImage> image;
	public:
		void SetImage(IImage * img);
		IImage * GetImage();
		void Draw(int absX, int absY);
		ImageDisplay(Container * parent);
	};

	class ToolStrip;

	class ToolButton : public Container
	{
		friend ToolStrip;
	protected:
		virtual Control* FindControlAtPosition(int x, int y) override
		{
			return Control::FindControlAtPosition(x, y);
		}
	public:
		enum _ButtonStyle
		{
			bsNormal, bsDropDown, bsSeperator
		};
	private:
		static const int Margin = 3;
		static const int DropDownButtonWidth = 12;
	private:
		CoreLib::RefPtr<IImage> image, imageDisabled;
		CoreLib::String text;
		Label * lblText = nullptr;
		ToolButton * bindButton;
		int mousePosX;
		void Init();
	public:
		ToolButton(ToolStrip * parent);
		ToolButton(ToolStrip * parent, const CoreLib::String & _text, _ButtonStyle bs, IImage * img);
	public:
		bool Selected;
		bool Checked;
		bool Pressed;
		bool ShowText;
		_ButtonStyle ButtonStyle;
		CoreLib::String GetText();
		void BindButton(ToolButton * btn);
		void SetText(const CoreLib::String & _text);
		void Draw(int absX, int absY);
		int MeasureWidth();
		int MeasureHeight();
		void SetImage(IImage * img);
		bool DoMouseEnter();
		bool DoMouseLeave();
		bool DoMouseMove(int X, int Y);
		bool DoMouseDown(int X, int Y, SHIFTSTATE shift);
		bool DoMouseUp(int X, int Y, SHIFTSTATE shift);
	};

	class ToolStrip : public Container
	{
	public:
		enum ToolStripOrientation
		{
			Horizontal, Vertical
		};
	private:
		static const int LeftMargin = 4;
		static const int TopMargin = 2;
			
		CoreLib::List<ToolButton*> buttons;
	protected:
		void PositButtons();
		virtual void SizeChanged();
	public:
		bool FullLineFill;
		bool ShowText;
		bool MultiLine;
		ToolStripOrientation Orientation;
		ToolStrip(Container * parent);
		ToolButton * AddButton(const CoreLib::String & text, IImage * bmp);
		void AddSeperator();
		ToolButton * GetButton(int id);
		int Count();
		void Draw(int absX, int absY);
		bool DoMouseMove(int X, int Y);
		bool DoMouseDown(int X, int Y, SHIFTSTATE shift);
		bool DoMouseUp(int X, int Y, SHIFTSTATE shift);
		bool DoMouseLeave();
	};

	class StatusStrip;

	class StatusPanel : public Container
	{
	public:
		enum _FillMode
		{
			Fill, Fixed, AutoSize
		};
	private:
		Label* text;
		void Init();
	public:
		_FillMode FillMode;
		StatusPanel(StatusStrip * parent);
		StatusPanel(StatusStrip * parent, const CoreLib::String & text, int width, _FillMode fm);
		void SetText(const CoreLib::String & text);
		CoreLib::String GetText();
		int MeasureWidth();
		void Draw(int absX, int absY);
	};

	class StatusStrip : public Container
	{
		friend class StatusPanel;
	private:
		CoreLib::List<StatusPanel*> panels;
		void PositItems();
	protected:
		void AddItem(StatusPanel * pannel);
	public:
		int LeftMargin;
		int TopMargin;
		StatusStrip(Container * parent);
		int Count();
		StatusPanel * GetItem(int id);
		bool DoMouseMove(int X, int Y);
		bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		bool DoMouseUp(int X, int Y, SHIFTSTATE Shift);
		void Draw(int absX, int absY);
	};

	class TabPage;

	class TabControl : public Container
	{
	private:
		int highlightItem;
		int headerHeight;
		int MeasureHeight();
		void SetClient();
		friend TabPage;
	protected:
		CoreLib::List<TabPage*> pages;
		void AddItem(TabPage * page);
	public:
		enum _TabStyle
		{
			tsText, tsImage, tsTextImage
		};
		enum _TabPosition
		{
			tpTop, tpBottom
		};
	public:
		bool CanClose, CanMove;
		_TabStyle TabStyle;
		_TabPosition TabPosition;
		int SelectedIndex;
		void RemoveItem(TabPage * page);
		TabPage * GetItem(int id);
		TabPage * GetSelectedItem();
		bool DoMouseMove(int X, int Y);
		bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		bool DoMouseUp(int X, int Y, SHIFTSTATE Shift);
		void Draw(int absX, int absY);
		void SizeChanged();
		void SwitchPage(int id);
		int HitTest(int X, int Y);
		TabControl(Container * parent);
	};

	class TabPage : public Container
	{
	private:
		Label* text;
		CoreLib::RefPtr<IImage> image;
		static const int LeftMargin = 4;
		static const int TopMargin = 6;
	public:
		void SetText(const CoreLib::String & text);
		CoreLib::String GetText();
		void SetImage(IImage * bitmap);
		int MeasureWidth(TabControl::_TabStyle style);
		int MeasureHeight(TabControl::_TabStyle style);
		void DrawHeader(int x, int y, int h, TabControl::_TabStyle style);
		TabPage(TabControl * parent);
	};

	class UpDown : public Container
	{
	protected:
		virtual Control* FindControlAtPosition(int x, int y) override
		{
			return Control::FindControlAtPosition(x, y);
		}
	private:
		Button * btnUp, *btnDown;
		TextBox * text;
		int state;
		int ldY;
		float inc;
		CoreLib::WinForm::Timer tmrHover;
		void tmrHoverTick(Object * sender, CoreLib::WinForm::EventArgs e);
	public:
		UpDown(Container * parent, TextBox * txtBox, float _min, float _max, float minInc, float maxInc);
		int Digits;			
		float MinIncrement;
		float MaxIncrement;
		float Min, Max;
		void Draw(int absX, int absY);
		bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		bool DoMouseMove(int X, int Y);
		bool DoMouseUp(int X, int Y, SHIFTSTATE Shift);
	};
}
#endif