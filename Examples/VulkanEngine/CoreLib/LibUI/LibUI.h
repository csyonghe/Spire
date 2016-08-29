#ifndef GX_UI_H
#define GX_UI_H

#include "../VectorMath.h"
#include "../Basic.h"
#include "../Events.h"
#include "../PerformanceCounter.h"
#include "UISystemInterface.h"
#include "../LibIO.h"

namespace GraphicsUI
{
	const int STR_NAME_LENGTH = 32;
	const float CURSOR_FREQUENCY = 0.5f;

	// enum types defination

	const int COLOR_LIGHTEN = 40;

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
	const int CT_IME_RECEIVER = (1 << 30);
	const int CT_IMETEXTBOX = CT_TEXTBOX | CT_IME_RECEIVER;
	const int CT_MULTILINETEXTBOX = 1024 | CT_IME_RECEIVER;
	const int CT_SCROLLBAR = 32;
	const int CT_LISTBOX = 64;
	const int CT_PROGRESSBAR = 128;
	const int CT_MENU = 256;
	const int CT_MENU_ITEM = 512;
	const int CT_TOOL_BUTTON = 513;

	namespace Keys
	{
		const int Left = 0x25;
		const int Up = 0x26; 
		const int Down = 0x28;
		const int Right = 0x27;
		const int Escape = 0x1B;
		const int Return = 0x0D;
		const int Space = 0x20;
		const int Shift = 0x10;
		const int Ctrl = 0x11;
		const int Alt = 0x12;
		const int Backspace = 0x08;
		const int Delete = 0x2E;
		const int Home = 0x24;
		const int End = 0x23;
		const int PageUp = 0x21;
		const int PageDown = 0x22;
		const int Insert = 0x2D;
		const int Tab = 0x09;
	}

	struct MarginValues
	{
		int Left = 0, Top = 0, Right = 0, Bottom = 0;
		MarginValues & operator = (const MarginValues & val) = default;
		MarginValues & operator = (int val)
		{
			Left = Top = Right = Bottom = val;
			return *this;
		}
		int Horizontal()
		{
			return Left + Right;
		}
		int Vertical()
		{
			return Top + Bottom;
		}
	};

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
	private:
		int dx = 0, dy = 0;
		CoreLib::List<DrawCommand> commandBuffer;
	public:
		Color PenColor, SolidBrushColor;
		void SetRenderTransform(int x, int y)
		{
			dx = x;
			dy = y;
		}
		void SetClipRect(int x, int y, int w, int h);
		void DrawShadowRect(Color color, int x0, int y0, int w, int h, int offsetX, int offsetY, float size);
		void DrawTextQuad(IBakedText * text, int x, int y);
		void DrawImage(IImage * image, int x, int y);
		void DrawArc(int x, int y, int rad, float theta, float theta2);
		void DrawRectangle(int x1, int y1, int x2, int y2);
		void FillRectangle(int x1, int y1, int x2, int y2);
		void FillEllipse(float x1, float y1, float x2, float y2);
		void FillTriangle(int x0, int y0, int x1, int y1, int x2, int y2);
		void DrawLine(float x1, float y1, float x2, float y2);
		inline void DrawLine(int x1, int y1, int x2, int y2)
		{
			DrawLine((float)x1 + 0.5f, (float)y1 + 0.5f, (float)x2 + 0.5f, (float)y2 + 0.5f);
		}
		void ClearCommands()
		{
			commandBuffer.Clear();
		}
		CoreLib::List<DrawCommand> & Buffer()
		{
			return commandBuffer;
		}
	};


	struct FormStyle
	{
		Color TitleBarColors[4];
		Color TitleBarDeactiveColors[4];
		Color TitleBarFontColor;
		Color BackColor, BorderColor;
		bool ShowIcon = false;
		Color CtrlButtonBackColor;
		IFont * TitleFont = nullptr;
		int CtrlButtonBorderStyle;
		float emTitleBarHeight = 1.2f;
		bool TopMost = false;
		bool Sizeable = true;
	};


	class ColorTable
	{
	public:
		Color ControlBackColor, ControlFontColor;
		Color ControlBorderColor;
		Color EditableAreaBackColor;
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
		Color SelectionColor, SelectionForeColor, HighlightColor, HighlightForeColor, UnfocusedSelectionColor, FocusRectColor;
		Color ScrollBarForeColor, ScrollBarBackColor, ScrollBarHighlightColor, ScrollBarPressedColor, ScrollBarSliderColor;
		FormStyle DefaultFormStyle;
		Color ShadowColor;
	};

	ColorTable CreateDefaultColorTable();
	ColorTable CreateDarkColorTable();

	class ClipRectStack
	{
	protected:
		Graphics * graphics;
		Rect Buffer[MAX_CLIPRECT_STACK_SIZE];
	public:
		ClipRectStack(Graphics * pGraphics);
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
		static Control * MouseDownControl;
		static Control * MouseCaptureControl;
		static int CursorPosX;
		static int CursorPosY;
		static int DeviceLineHeight;
		static ColorTable Colors;
		static int SCROLLBAR_BUTTON_SIZE;
		static int SCROLLBAR_MIN_PAGESIZE;
	};

	int emToPixel(float em);

	class UI_Base : public CoreLib::Object
	{
	public:
		virtual void HandleMessage(const UI_MsgArgs *Args);
	};

	class Control : public UI_Base
	{
		friend Container;
		friend UIEntry;
	protected:
		int EventID; // Used by Containers to avoid message resending;
	protected:
		UIEntry * entryCache = nullptr;
		IFont * font = nullptr;
		Rect clientRect;
		bool LastInClient;
		bool TopMost;
		int Height, Width;
		bool IsPointInClient(int X, int Y);
		virtual Control * FindControlAtPosition(int x, int y);
		void LocalPosToAbsolutePos(int x, int y, int & ax, int & ay);
	public:
		Control(Container * parent);
		Control(Container * parent, bool addToParent);
		~Control();
	public:
		bool WantsTab = false;
		bool AcceptsFocus = true;
		MarginValues Margin;
		GraphicsUI::CursorType Cursor;
		CoreLib::String Name;
		int ID;
		CONTROLTYPE Type;
		Container *Parent;
		int  Left, Top;
		bool BackgroundShadow = false;
		int ShadowOffset = 2;
		float ShadowSize = 6.0;
		unsigned char ShadowOpacity = 170;
		bool Enabled, Visible;
		bool TabStop = false;
		Color BackColor, FontColor, BorderColor;
		BORDERSTYLE BorderStyle;
		int AbsolutePosX; int AbsolutePosY;
		void SetName(CoreLib::String pName);
		virtual void Posit(int pLeft, int pTop, int pWidth, int pHeight);
		void SetHeight(int val);
		void SetWidth(int val);
		int GetHeight();
		int GetWidth();
		Rect ClientRect();
		virtual void SizeChanged();
		bool IsFocused();
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
		virtual bool DoMouseWheel(int /*delta*/) { return false; }
		virtual bool DoMouseEnter();
		virtual bool DoMouseLeave();
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyUp(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyPress(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoMouseHover();
		virtual bool DoClick();
		virtual bool DoDblClick();
		virtual bool DoTick() { return false; };
		virtual void SetFocus();
		virtual void LostFocus(Control * newFocus);
		virtual bool DoClosePopup();
		virtual void DoDpiChanged() {}
		VectorMath::Vec2i GetRelativePos(Container * parent);
	};

	class ImeCharReceiver
	{
	public:
		virtual void ImeInputString(const CoreLib::String & /*txt*/) = 0;
		virtual VectorMath::Vec2i GetCaretScreenPos() = 0;
	};

	class Line : public Control
	{
	public:
		Line(Container * owner);
		virtual void Draw(int absX, int absY);
	};

	enum class ContainerLayoutType
	{
		None, Flow, Stack
	};

	class Container : public Control
	{
	protected:
		bool drawChildren = true;
		ContainerLayoutType layout = ContainerLayoutType::None;
		virtual Control * FindControlAtPosition(int x, int y);
		CoreLib::List<CoreLib::RefPtr<Control>> controls;
	public:
		Container(Container * parent);
		Container(Container * parent, ContainerLayoutType pLayout);
		Container(Container * parent, bool addToParent);
	public:
		bool AutoHeight = false, AutoWidth = false;
		MarginValues Padding;
		virtual CoreLib::List<CoreLib::RefPtr<Control>> & GetChildren() { return controls; }
		virtual void AddChild(Control *nControl);
		virtual void RemoveChild(Control *AControl);
		virtual void ArrangeControls(Rect initalClientRect);
		virtual void InternalBroadcastMessage(UI_MsgArgs *Args);
		virtual bool DoDblClick() override;
		virtual bool DoMouseLeave() override;
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift)override;
		virtual bool DoKeyUp(unsigned short Key, SHIFTSTATE Shift)override;
		virtual bool DoKeyPress(unsigned short Key, SHIFTSTATE Shift)override;
		virtual void DoFocusChange();
		virtual void Draw(int absX, int absY) override;
		virtual void DrawChildren(int absX, int absY);
		virtual void SizeChanged() override;
		virtual bool DoClosePopup() override;
		virtual void KillFocus() override;
		virtual ContainerLayoutType GetLayout() { return layout; }
		virtual void SetLayout(ContainerLayoutType layout);
		virtual void DoDpiChanged() override;
	};

	class Label;
	class Button;

	enum class ResizeMode
	{
		None, Left = 1, Right = 2, Top = 4, Bottom = 8, TopLeft = 5, TopRight = 6, BottomLeft = 9, BottomRight = 10
	};
	
	class Menu;

	class Form : public Container
	{
	protected:
		bool DownInTitleBar;
		bool DownInButton;
		int DownPosX,DownPosY;
		Control *btnClose;
		Label *lblClose;
		Label *lblTitle;
		Container * content;
		CoreLib::String Text; 
		FormStyle formStyle;
		ResizeMode resizeMode = ResizeMode::None;
		ResizeMode GetResizeHandleType(int x, int y);
		void FormStyleChanged();
		int GetTitleBarHeight();
	public:
		Form(UIEntry * parent);
	public:
		Menu * MainMenu = nullptr;
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
		virtual void HandleMessage(const UI_MsgArgs * msg) override;
		virtual bool DoMouseMove(int X, int Y) override;
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift) override;
		virtual void Draw(int absX,int absY) override;
		virtual void AddChild(Control*) override;
		virtual void SizeChanged() override;
		virtual ContainerLayoutType GetLayout() override;
		virtual void SetLayout(ContainerLayoutType layout) override;
		virtual CoreLib::List<CoreLib::RefPtr<Control>> & GetChildren() override;
		virtual Control * FindControlAtPosition(int x, int y) override;
	};

	enum class VerticalAlignment
	{
		Top, Center, Bottom
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
		VerticalAlignment VertAlignment = VerticalAlignment::Top;
		Label(Container * parent);
		~Label();
	public:
		Color ShadowColor;
		bool DropShadow;
		bool AutoSize = true;
		virtual void SetText(const CoreLib::String & text);
		virtual void SetFont(IFont * pFont) override;
		void UpdateText();
		CoreLib::String GetText();
		virtual void Draw(int absX, int absY) override;
		virtual void SizeChanged() override;
		virtual void DoDpiChanged() override;
	};

	class Button: public Label
	{
	private:
		bool IsMouseDown = false;
	public:
		Button(Container * parent);
		Button(Container * parent, const CoreLib::String & text);
	public:
		bool Checked;
		virtual void Draw(int absX, int absY) override;
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseLeave() override;
		virtual bool DoDblClick() override;
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift) override;
		virtual bool DoKeyUp(unsigned short Key, SHIFTSTATE Shift) override;
		virtual void DoDpiChanged() override;
	};

	class CheckBox : public Label
	{
	private:
		void ComputeAutoSize();
	public:
		CheckBox(Container * parent);
		CheckBox(Container * parent, const CoreLib::String & text, bool checked = false);
	public:
		bool Checked;
		virtual void DoDpiChanged() override;
		virtual void SetText(const CoreLib::String & text) override;
		virtual void Draw(int absX, int absY) override;
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoDblClick() override;
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift) override;
	};

	class RadioBox : public CheckBox
	{
	public:
		RadioBox(Container * parent);
	public:
		bool GetValue();
		void SetValue(bool AValue);
		virtual void Draw(int absX, int absY) override;
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift) override;
	};

	class Menu;

	class CustomTextBox : public Container
	{
	protected:
		CoreLib::Diagnostics::TimePoint time;
		CoreLib::String FText;
		IFont * font = nullptr;
		Menu * menu = nullptr;
		CoreLib::RefPtr<IBakedText> text;
		int SelOrigin;
		bool Changed, cursorPosChanged = false, KeyDown,SelectMode;
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
		void SetFont(IFont * AFont) override;
		void CopyToClipBoard();
		void PasteFromClipBoard();
		void DeleteSelectionText();
		void SelectAll();
		virtual void Posit(int pLeft, int pTop, int pWidth, int pHeight) override;
		virtual void DoDpiChanged() override;
		virtual bool DoDblClick() override;
 		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseMove(int X, int Y) override;
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift) override;
		virtual bool DoKeyUp(unsigned short Key, SHIFTSTATE Shift) override;
		virtual bool DoKeyPress(unsigned short Key, SHIFTSTATE Shift) override;
		bool DoInput(const CoreLib::String & AInput);
		virtual void Draw(int absX, int absY) override;
	};

	class TextBox : public CustomTextBox, public ImeCharReceiver
	{
	public:
		TextBox(Container * parent)
			: CustomTextBox(parent)
		{
			Type = CT_IMETEXTBOX;
		}
		virtual void ImeInputString(const CoreLib::String & txt) override;
		virtual bool DoKeyPress(unsigned short Key, SHIFTSTATE Shift) override;
		virtual VectorMath::Vec2i GetCaretScreenPos() override;
	};

	class IMETextBox : public TextBox
	{
	public:
		IMETextBox(Container * parent) : TextBox(parent) {Type=CT_IMETEXTBOX;};
	};

	class IMEWindow : public Container
	{
	protected:
		Label * lblCompStr;
	public:
		IMEWindow(Container * parent);
		~IMEWindow();
	public:
		Control *Panel;
		int WindowWidth, WindowHeight;
		CoreLib::String strComp;
		void ChangeCompositionString(CoreLib::String AString);
		virtual void Draw(int absX, int absY) override;
	};

	class IMEHandler : public CoreLib::Object
	{
	public:
		void Init(UIEntry * entry);
	public:
		ImeCharReceiver * TextBox = nullptr;
		IMEWindow * ImeWindow = nullptr;
		bool DoImeStart();
		bool DoImeEnd();
		bool DoImeCompositeString(const CoreLib::String & str);
		bool DoImeResultString(const CoreLib::String & str);
		void StringInputed(CoreLib::String AString);
	};

	class MouseMessageStack
	{
	public:
		Control * Ctrl;
		int X, Y;
	};

	class UIEntry : public Container
	{
	private:
		CoreLib::List<MouseMessageStack> controlStack;
		CoreLib::EnumerableHashSet<Control*> tickEventSubscribers;
		int lineHeight = 0;
		float dpiScale = 1.0f;
	protected:
		void DeactivateAllForms();
	public:
		UIEntry(int WndWidth, int WndHeight, ISystemInterface * pSystem);
		ISystemInterface * System = nullptr;
	public:
		bool KeyInputConsumed = false, MouseInputConsumed = false;
		Menu * MainMenu = nullptr;
		Label * CheckmarkLabel = nullptr;
		CoreLib::RefPtr<ClipRectStack> ClipRects;
		Control *FocusedControl;
		IMEHandler ImeMessageHandler;
		Form *ActiveForm = nullptr;
		Graphics DrawCommands;
		CoreLib::List<Form*> Forms;
		CoreLib::List<DrawCommand> & DrawUI();
		void RemoveForm(Form *Form);
		void ShowWindow(Form *Form);
		void CloseWindow(Form *Form);
		void SubscribeTickEvent(Control * ctrl)
		{
			tickEventSubscribers.Add(ctrl);
		}
		void UnSubscribeTickEvent(Control * ctrl)
		{
			tickEventSubscribers.Remove(ctrl);
		}
		virtual void InternalBroadcastMessage(UI_MsgArgs *Args) override;
		virtual void SizeChanged() override;
		virtual void Draw(int absX,int absY) override;
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift) override;
		virtual bool DoKeyUp(unsigned short Key, SHIFTSTATE Shift) override;
		virtual bool DoKeyPress(unsigned short Key, SHIFTSTATE Shift) override;
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseMove(int X, int Y) override;
		virtual bool DoMouseWheel(int delta) override;
		virtual bool DoMouseHover() override;
		virtual bool DoDblClick() override;
		virtual void DoDpiChanged() override;
		virtual bool DoTick() override;
		virtual void HandleMessage(const UI_MsgArgs *Args) override;
		void MoveFocusBackward();
		void MoveFocusForward();
		VectorMath::Vec2i GetCaretScreenPos();
		int GetLineHeight()
		{
			return lineHeight;
		}
		void SetFocusedControl(Control *Target);

		virtual UIEntry * GetEntry() override
		{
			return this;
		}
		virtual Control * FindControlAtPosition(int x, int y) override;
		float GetDpiScale()
		{
			return dpiScale;
		}
	};

	class ScrollBar: public Container
	{
	protected:
		int tmrOrientation = -1;
		bool DownInSlider;
		bool highlightSlider = false;
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
		virtual Control* FindControlAtPosition(int x, int y) override
		{
			return Control::FindControlAtPosition(x, y);
		}
	public:
		ScrollBar(Container * parent);
		ScrollBar(Container * parent, bool addToParent);
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
		int GetPageSize();
		virtual bool DoTick() override;
		virtual void Draw(int absX, int absY) override;
		virtual bool DoMouseMove(int X, int Y) override;
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseUp(int X,int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseLeave() override;
		virtual bool DoMouseHover() override;
		virtual void HandleMessage(const UI_MsgArgs *Args) override;
		virtual void SizeChanged() override;
		virtual void DoDpiChanged() override;
	};

	class ListBox : public Container
	{
	protected:
		int HighLightID = -1;
		int ItemHeight;
		bool Selecting = false;
		bool HotTrack;
		int SelOriX,SelOriY;
		int BorderWidth;
		bool DownInItem;
		int lastSelIdx = -1;
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
		Color SelectionColor,UnfocusedSelectionColor,SelectionForeColor,HighLightColor,HighLightForeColor;
		CoreLib::List<Control *> Selection;
		int SelectedIndex = -1;
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
		virtual void Draw(int absX, int absY) override;
		virtual void SizeChanged() override;
		virtual bool DoMouseDown(int x, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseUp(int x, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseMove(int x, int Y) override;
		virtual bool DoMouseWheel(int delta) override;
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift) override;
		virtual bool DoMouseLeave() override;
		virtual void DoDpiChanged() override;
	};

	class ComboBox : public ListBox
	{
	private:
		int lH,lW,lL,lT;
	protected:	
		int ListLeft,ListTop,ListHeight,ListWidth;
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
		virtual void Posit(int left, int top, int width, int height) override;
		virtual void Draw(int absX, int absY) override;
		virtual void SizeChanged() override;
		virtual void DoDpiChanged() override;
		virtual void HandleMessage(const UI_MsgArgs *Args) override;
		virtual bool DoMouseDown(int x, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseUp(int x, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseMove(int x, int Y) override;
		virtual bool DoMouseWheel(int delta) override;
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift) override;
		virtual void SetFocus() override;
		virtual void LostFocus(Control * newFocus) override;
		virtual bool DoClosePopup() override;
		void SetSelectedIndex(int id);
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
	public:
		enum MenuStyle
		{
			msPopup, msMainMenu
		};
	private:
		CoreLib::List<MenuItem*> Items;
		MenuStyle style;
		bool enableMouseHover = false; // handle mouse hover only when true
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
		void Draw(int absX, int absY) override;
		void Popup(int x, int y);
		void CloseMenu();
		virtual bool DoClosePopup() override;
		Menu(Container * parent, MenuStyle mstyle = msPopup);
		virtual bool DoMouseHover() override;
		virtual bool DoMouseMove(int X, int Y) override;
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift) override;
		virtual void HandleMessage(const UI_MsgArgs * Args) override;
		virtual void SetFocus() override;
		virtual void DoDpiChanged() override;
	};

	class MenuItem : public Container
	{
		friend Menu;
	protected:
		wchar_t accKey;
		int accKeyId = -1;
		bool isButton;
		void ItemSelected(MenuItem * item); // Called by SubMenu
		virtual Control* FindControlAtPosition(int x, int y) override
		{
			return Control::FindControlAtPosition(x, y);
		}
	private:
		bool cursorInClient;
		static const int separatorHeading = 8;
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
		MenuItem(Menu * parent);
		MenuItem(MenuItem * parent);
		MenuItem(Menu* menu, const CoreLib::String & text);
		MenuItem(MenuItem* menu, const CoreLib::String & text);

		MenuItem(Menu* menu, const CoreLib::String & text, const CoreLib::String & shortcutText);
		MenuItem(MenuItem* menu, const CoreLib::String & text, const CoreLib::String & shortcutText);
		void Hit();
		void DrawMenuItem(int width, int height);
		void DrawMenuButton(int width, int height);
		virtual bool DoMouseEnter() override;
		virtual bool DoMouseLeave() override;
		virtual bool DoMouseHover() override;
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift) override;
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift) override;
		virtual bool DoClick() override;
		virtual void HandleMessage(const UI_MsgArgs * Args) override;
		virtual void DoDpiChanged() override;
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

	class VScrollPanel : public Container
	{
	private:
		ScrollBar * vscrollBar = nullptr;
		Container * content = nullptr;
		void ScrollBar_Changed(UI_Base * sender);
	public:
		VScrollPanel(Container * parent);
		virtual CoreLib::List<CoreLib::RefPtr<Control>> & GetChildren() override
		{
			return content->GetChildren();
		}
		virtual void SizeChanged() override;
		virtual void AddChild(Control *nControl) override;
		virtual void RemoveChild(Control *AControl) override;
		virtual bool DoMouseWheel(int delta) override;
		virtual void DoFocusChange() override;
		virtual ContainerLayoutType GetLayout() override;
		virtual void SetLayout(ContainerLayoutType layout) override;
		void ClearChildren();
		int GetClientWidth();
		int GetClientHeight();
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
		static const int DropDownButtonWidth = 12;
	private:
		CoreLib::RefPtr<IImage> image, imageDisabled;
		CoreLib::String text;
		Label * lblText = nullptr;
		ToolButton * bindButton;
		int imageLabelPadding = 4;
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
		void Draw(int absX, int absY) override;
		int MeasureWidth();
		int MeasureHeight();
		void SetImage(IImage * img);
		bool DoMouseEnter() override;
		bool DoMouseLeave() override;
		bool DoMouseMove(int X, int Y) override;
		bool DoMouseDown(int X, int Y, SHIFTSTATE shift) override;
		bool DoMouseUp(int X, int Y, SHIFTSTATE shift) override;
	};

	class ToolStrip : public Container
	{
	public:
		enum ToolStripOrientation
		{
			Horizontal, Vertical
		};
	private:
		CoreLib::List<ToolButton*> buttons;
		ToolStripOrientation orientation;
	protected:
		void PositButtons();
		virtual void SizeChanged() override;
	public:
		bool FullLineFill;
		bool ShowText;
		bool MultiLine;
		ToolStrip(Container * parent);
		ToolButton * AddButton(const CoreLib::String & text, IImage * bmp);
		void AddSeperator();
		ToolStripOrientation GetOrientation()
		{
			return orientation;
		}
		void SetOrientation(ToolStripOrientation ori);
		ToolButton * GetButton(int id);
		int Count();
		void Draw(int absX, int absY) override;
		bool DoMouseMove(int X, int Y) override;
		bool DoMouseDown(int X, int Y, SHIFTSTATE shift) override;
		bool DoMouseUp(int X, int Y, SHIFTSTATE shift) override;
		bool DoMouseLeave() override;
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
		StatusStrip(Container * parent);
		int Count();
		StatusPanel * GetItem(int id);
		bool DoMouseMove(int X, int Y) override;
		bool DoMouseDown(int X, int Y, SHIFTSTATE Shift) override;
		bool DoMouseUp(int X, int Y, SHIFTSTATE Shift) override;
		virtual void DoDpiChanged() override;
		void Draw(int absX, int absY) override;
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
		MarginValues HeaderPadding;

		bool CanClose, CanMove;
		_TabStyle TabStyle;
		_TabPosition TabPosition;
		int SelectedIndex;
		void RemoveItem(TabPage * page);
		TabPage * GetItem(int id);
		TabPage * GetSelectedItem();
		bool DoMouseMove(int X, int Y) override;
		bool DoMouseDown(int X, int Y, SHIFTSTATE Shift) override;
		bool DoMouseUp(int X, int Y, SHIFTSTATE Shift) override;
		virtual void DoDpiChanged() override;
		void Draw(int absX, int absY) override;
		void SizeChanged() override;
		void SwitchPage(int id);
		int HitTest(int X, int Y);
		TabControl(Container * parent);
	};

	class TabPage : public Container
	{
	private:
		Label* text;
		CoreLib::RefPtr<IImage> image;
		int imageTextPadding = 4;
	public:
		void SetText(const CoreLib::String & text);
		CoreLib::String GetText();
		void SetImage(IImage * bitmap);
		int MeasureWidth(TabControl::_TabStyle style);
		int MeasureHeight(TabControl::_TabStyle style);
		void DrawHeader(int x, int y, int h, const MarginValues & headerPadding, TabControl::_TabStyle style);
		TabPage(TabControl * parent);
		TabPage(TabControl * parent, CoreLib::String text);
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
	public:
		UpDown(Container * parent, TextBox * txtBox, float _min, float _max, float minInc, float maxInc);
		~UpDown();
		int Digits;			
		float MinIncrement;
		float MaxIncrement;
		float Min, Max;
		void Draw(int absX, int absY) override;
		bool DoTick() override;
		bool DoMouseDown(int X, int Y, SHIFTSTATE Shift) override;
		bool DoMouseMove(int X, int Y) override;
		bool DoMouseUp(int X, int Y, SHIFTSTATE Shift) override;
		bool DoMouseHover() override;
	};

	struct CaretPos
	{
		int Line = 0, Col = 0;
		CaretPos() = default;
		CaretPos(int line, int col)
		{
			Line = line;
			Col = col;
		}
		bool operator < (const CaretPos & pos)
		{
			return Line < pos.Line || (Line == pos.Line && Col < pos.Col);
		}
		bool operator <=(const CaretPos & pos)
		{
			return Line < pos.Line || (Line == pos.Line && Col <= pos.Col);
		}
		bool operator ==(const CaretPos & pos)
		{
			return Line == pos.Line && Col == pos.Col;
		}
		bool operator !=(const CaretPos & pos)
		{
			return Line != pos.Line || Col != pos.Col;
		}
	};

	class MultiLineTextBox : public Container, public ImeCharReceiver
	{
	public:
		MultiLineTextBox(Container * parent)
			: Container(parent)
		{}
		NotifyEvent OnCaretPosChanged;
		virtual CoreLib::String GetSelectionText() = 0;
		virtual void SetCaretPos(const CaretPos & pCaretPos) = 0;
		virtual void ScrollToCaret() = 0;
		virtual CaretPos GetCaretPos() = 0;
		virtual CoreLib::String GetText() = 0;
		virtual void SetText(const CoreLib::String & pText) = 0;
		virtual void SetWordWrap(bool pValue) = 0;
		virtual bool GetWordWrap() = 0;
		virtual void SetUndoStackSize(int size) = 0;
		virtual CoreLib::String GetTextFromRange(CaretPos start, CaretPos end) = 0;
		virtual void SetScrollBars(bool vertical, bool horizontal) = 0;
		virtual void InsertText(const CoreLib::String & text) = 0;
		virtual void Delete() = 0;
		virtual void Redo() = 0;
		virtual void Undo() = 0;
		virtual void Copy() = 0;
		virtual void Paste() = 0;
		virtual void Cut() = 0;
		virtual void IncreaseIndent() = 0;
		virtual void DecreaseIndent() = 0;
		virtual void MoveCaretToEnd() = 0;
		virtual void SelectAll() = 0;
		virtual void SetReadOnly(bool value) = 0;
		virtual bool GetReadOnly() = 0;
		virtual void Select(CaretPos start, CaretPos end) = 0;
		virtual int GetLineCount() = 0;
		virtual CoreLib::String GetLine(int i) = 0;
		virtual void DeleteLine(int i) = 0;
	};
	MultiLineTextBox * CreateMultiLineTextBox(Container * parent);

	class CommandForm : public Form
	{
	private:
		MultiLineTextBox * textBox;
		TextBox * txtCmd;
		CoreLib::List<CoreLib::String> commandHistories;
		int cmdPtr = 0;
	public:
		CommandForm(UIEntry * parent);
		void Write(const CoreLib::String & text);
		CoreLib::Event<CoreLib::String> OnCommand;
		virtual bool DoMouseUp(int x, int y, SHIFTSTATE shift) override;
	};

	class UICommandLineWriter : public CoreLib::IO::CommandLineWriter
	{
	private:
		CommandForm * cmdForm = nullptr;
	public:
		UICommandLineWriter(CommandForm * form)
		{
			cmdForm = form;
		}
		CoreLib::Event<const CoreLib::String &> OnWriteText;
		virtual void Write(const CoreLib::String & text) override;
	};
}
#endif