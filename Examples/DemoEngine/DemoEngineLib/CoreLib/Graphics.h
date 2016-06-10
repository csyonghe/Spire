/***********************************************************************

CoreLib - The MIT License (MIT)
Copyright (c) 2016, Yong He

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.

***********************************************************************/

/***********************************************************************
WARNING: This is an automatically generated file.
***********************************************************************/
#include "Basic.h"
#include "WinForm.h"

/***********************************************************************
VIEWFRUSTUM.H
***********************************************************************/
#ifndef CORELIB_GRAPHICS_VIEWFRUSTUM_H
#define CORELIB_GRAPHICS_VIEWFRUSTUM_H

namespace CoreLib
{
	namespace Graphics
	{
		class ViewFrustum
		{
		public:
			VectorMath::Vec3 CamPos, CamDir, CamUp;
			float zMin, zMax;
			float Aspect, FOV;
			CoreLib::Array<VectorMath::Vec3, 8> GetVertices(float zNear, float zFar) const;
			VectorMath::Matrix4 GetViewTransform() const
			{
				VectorMath::Matrix4 rs;
				VectorMath::Vec3 right;
				VectorMath::Vec3::Cross(right, CamDir, CamUp);
				VectorMath::Vec3::Normalize(right, right);
				VectorMath::Matrix4 mat;
				mat.values[0] = right.x; mat.values[4] = right.y; mat.values[8] = right.z; mat.values[12] = 0.0f;
				mat.values[1] = CamUp.x; mat.values[5] = CamUp.y; mat.values[9] = CamUp.z; mat.values[13] = 0.0f;
				mat.values[2] = -CamDir.x; mat.values[6] = -CamDir.y; mat.values[10] = -CamDir.z; mat.values[14] = 0.0f;
				mat.values[3] = 0.0f; mat.values[7] = 0.0f; mat.values[11] = 0.0f; mat.values[15] = 1.0f;
				VectorMath::Matrix4 translate;
				VectorMath::Matrix4::Translation(translate, -CamPos.x, -CamPos.y, -CamPos.z);
				VectorMath::Matrix4::Multiply(rs, mat, translate);
				return rs;
			}
			VectorMath::Matrix4 GetProjectionTransform() const
			{
				VectorMath::Matrix4 rs;
				VectorMath::Matrix4::CreatePerspectiveMatrixFromViewAngle(rs,
					FOV,
					Aspect,
					zMin,
					zMax);
				return rs;
			}
			VectorMath::Matrix4 GetViewProjectionTransform() const
			{
				auto view = GetViewTransform();
				auto proj = GetProjectionTransform();
				return proj * view;
			}
		};
	}
}
#endif

/***********************************************************************
TEXTUREFILE.H
***********************************************************************/
#ifndef CORELIB_GRAPHICS_TEXTURE_FILE_H
#define CORELIB_GRAPHICS_TEXTURE_FILE_H


namespace CoreLib
{
	namespace Graphics
	{
		enum class TextureType : short
		{
			Texture2D
		};
		enum class TextureStorageFormat : short
		{
			R8, RG8, RGB8, RGBA8,
			R_F32, RG_F32, RGB_F32, RGBA_F32
		};
		class TextureFileHeader
		{
		public:
			TextureType Type;
			TextureStorageFormat Format;
			int Width, Height;
		};

		class TextureFile
		{
		private:
			CoreLib::Basic::List<unsigned char> buffer;
			TextureStorageFormat format;
			int width, height;
			void LoadFromStream(CoreLib::IO::Stream * stream);
		public:
			TextureFile()
			{
				width = height = 0;
				format = TextureStorageFormat::RGBA8;
			}
			TextureFile(CoreLib::Basic::String fileName);
			TextureFile(CoreLib::IO::Stream * stream);
			TextureStorageFormat GetFormat()
			{
				return format;
			}
			int GetWidth()
			{
				return width;
			}
			int GetHeight()
			{
				return height;
			}
			void SaveToFile(CoreLib::Basic::String fileName);
			void SaveToStream(CoreLib::IO::Stream * stream);
			void SetData(TextureStorageFormat format, int width, int height, CoreLib::Basic::ArrayView<unsigned char> data);
			CoreLib::Basic::ArrayView<unsigned char> GetData()
			{
				return buffer.GetArrayView();
			}
			CoreLib::Basic::List<float> GetPixels()
			{
				CoreLib::Basic::List<float> pixels;
				pixels.SetSize(width * height * 4);
				for (int i = 0; i < width*height; i++)
				{
					float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
					switch (format)
					{
					case TextureStorageFormat::R8:
						color[0] = buffer[i] / 255.0f;
						break;
					case TextureStorageFormat::RG8:
						color[0] = buffer[i*2] / 255.0f;
						color[1] = buffer[i*2 + 1] / 255.0f;
						break;
					case TextureStorageFormat::RGB8:
						color[0] = buffer[i * 3] / 255.0f;
						color[1] = buffer[i * 3 + 1] / 255.0f;
						color[2] = buffer[i * 3 + 2] / 255.0f;
						break;
					case TextureStorageFormat::RGBA8:
						color[0] = buffer[i * 4] / 255.0f;
						color[1] = buffer[i * 4 + 1] / 255.0f;
						color[2] = buffer[i * 4 + 2] / 255.0f;
						color[3] = buffer[i * 4 + 3] / 255.0f;
						break;
					case TextureStorageFormat::R_F32:
						color[0] = ((float*)buffer.Buffer())[i];
						break;
					case TextureStorageFormat::RG_F32:
						color[0] = ((float*)buffer.Buffer())[i*2];
						color[1] = ((float*)buffer.Buffer())[i*2 + 1];
						break;
					case TextureStorageFormat::RGB_F32:
						color[0] = ((float*)buffer.Buffer())[i * 3];
						color[1] = ((float*)buffer.Buffer())[i * 3 + 1];
						color[2] = ((float*)buffer.Buffer())[i * 3 + 2];
						break;
					case TextureStorageFormat::RGBA_F32:
						color[0] = ((float*)buffer.Buffer())[i * 4];
						color[1] = ((float*)buffer.Buffer())[i * 4 + 1];
						color[2] = ((float*)buffer.Buffer())[i * 4 + 2];
						color[3] = ((float*)buffer.Buffer())[i * 4 + 3];
						break;
					}
					pixels[i * 4] = color[0];
					pixels[i * 4 + 1] = color[1];
					pixels[i * 4 + 2] = color[2];
					pixels[i * 4 + 3] = color[3];

				}
				return pixels;
			}
		};
	}
}

#endif

/***********************************************************************
UISYSTEMINTERFACE.H
***********************************************************************/
#ifndef GX_UI_SYS_INTERFACE_H
#define GX_UI_SYS_INTERFACE_H


namespace GraphicsUI
{
	struct Rect
	{
		int x, y, h, w;
		Rect()
		{
			x = y = h = w = 0;
		}
		Rect(int ax, int ay, int aw, int ah)
		{
			x = ax; y = ay; h = ah; w = aw;
		}
		bool Intersects(const Rect & other)
		{
			if (x > other.x + other.w)
				return false;
			if (x + w < other.x)
				return false;
			if (y > other.y + other.h)
				return false;
			if (y + h < other.y)
				return false;
			return true;
		}
	};


	struct Color
	{
		unsigned char R, G, B, A;
		Color() { R = G = B = 0; A = 0; };
		Color(unsigned char AR, unsigned char AG, unsigned char AB, unsigned char AA) { R = AR; G = AG; B = AB; A = AA; };
		Color(unsigned char AR, unsigned char AG, unsigned char AB) { R = AR; G = AG; B = AB; A = 255; };
		Color(COLORREF c) { R = GetRValue(c); G = GetGValue(c); B = GetBValue(c); A = 255; };
	};

	class Pen
	{
	public:
		Color Color;
		float Width;
		int DashPattern = -1;
		Pen(const GraphicsUI::Color & c)
		{
			this->Color = c;
			Width = 1.0f;
		}
		Pen()
		{
			Color = GraphicsUI::Color(0, 0, 0);
			Width = 1.0f;
		}
	};

	class IImage : public CoreLib::Object
	{
	public:
		virtual void Draw(int x, int y) = 0;
		virtual int GetHeight() = 0;
		virtual int GetWidth() = 0;
	};

	class IBakedText
	{
	public:
		virtual int GetWidth() = 0;
		virtual int GetHeight() = 0;
		virtual ~IBakedText() {}
	};

	class IFont
	{
	public:
		virtual Rect MeasureString(const CoreLib::String & text, int width) = 0;
		virtual IBakedText * BakeString(const CoreLib::String & text, int width) = 0;
	};

	enum class DefaultFontType
	{
		Content, Title, Symbol
	};

	class ISystemInterface : public CoreLib::Object
	{
	public:
		virtual void SetRenderTransform(int dx, int dy) = 0;
		virtual void SetClipRect(const Rect & rect) = 0;
		virtual void DrawLine(const Pen & pen, float x0, float y0, float x1, float y1) = 0;
		virtual void FillRectangle(const Color & color, const Rect & rect) = 0;
		virtual void FillPolygon(const Color & color, CoreLib::ArrayView<VectorMath::Vec2> points) = 0;
		virtual void DrawRectangleShadow(const Color & color, float x, float y, float w, float h, float offsetX, float offsetY, float shadowSize) = 0;
		virtual void DrawBakedText(IBakedText * text, const Color & color, int x, int y) = 0;
		virtual void AdvanceTime(float seconds) = 0;
		virtual IFont * LoadDefaultFont(DefaultFontType dt = DefaultFontType::Content) = 0;
	};
}

#endif

/***********************************************************************
LIBUI.H
***********************************************************************/
#ifndef GX_UI_H
#define GX_UI_H


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


	struct FormStyle
	{
		Color TitleBarColors[4];
		Color TitleBarDeactiveColors[4];
		Color TitleBarFontColor;
		Color BackColor, BorderColor;
		bool ShowIcon;
		Color CtrlButtonBackColor;
		IFont * TitleFont = nullptr;
		int CtrlButtonBorderStyle;
		int TitleBarHeight;
		bool TopMost;
		bool Sizeable = true;
		FormStyle();
	};


	class ColorTable
	{
	public:
		Color ControlBackColor, ControlFontColor;
		Color ControlBorderColor;
		Color EditableAreaBackColor;
		Color ScrollBarBackColor;
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
		Color SelectionColor, SelectionForeColor, HighlightColor, HighlightForeColor, UnfocusedSelectionColor;
		FormStyle DefaultFormStyle;
		Color ShadowColor;
	};

	ColorTable CreateDefaultColorTable();
	ColorTable CreateDarkColorTable();

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
		static int SCROLLBAR_BUTTON_SIZE;
		static int SCROLLBAR_MIN_PAGESIZE;
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
		Control(Container * parent, bool addToParent);
		~Control();
	public:
		GraphicsUI::Cursor Cursor;
		CoreLib::String Name;
		int ID;
		CONTROLTYPE Type;
		Container *Parent;
		int  Left, Top;
		bool BackgroundShadow = false;
		int ShadowOffset = 8;
		float ShadowSize = 20.0;
		bool Enabled, Visible, Focused, TabStop = false, GenerateMouseHoverEvent = false;
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
		virtual void SetFocus();
		virtual bool ContainsFocus();
		virtual void LostFocus(Control * newFocus);
		virtual bool DoClosePopup();
	};

	class Line : public Control
	{
	public:
		Line(Container * owner);
		virtual void Draw(int absX, int absY);
	};

	class Container : public Control
	{
	protected:
		bool drawChildren = true;
		virtual Control * FindControlAtPosition(int x, int y);

	public:
		Container(Container * parent);
		Container(Container * parent, bool addToParent);
		~Container();
	public:
		CoreLib::List<CoreLib::RefPtr<Control>> Controls;
		int Margin;
		virtual void AddChild(Control *nControl);
		virtual void RemoveChild(Control *AControl);
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
		int HandleMessage(HWND hWnd, UINT message, WPARAM &wParam, LPARAM &lParam);
		void StringInputed(CoreLib::String AString);
	};

	class MouseMessageStack
	{
	public:
		Control * Ctrl;
		int X, Y;
	};

	class UIEntry: public Container
	{
	private:
		CoreLib::List<MouseMessageStack> controlStack;
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
		int HandleSystemMessage(HWND hWnd, UINT message, WPARAM &wParam, LPARAM &lParam);
		virtual void InternalBroadcastMessage(UI_MsgArgs *Args);
		virtual void SizeChanged();
		virtual void Draw(int absX,int absY);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyUp(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoKeyPress(unsigned short Key, SHIFTSTATE Shift);
		virtual bool DoMouseDown(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseUp(int X, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseMove(int X, int Y);
		virtual bool DoMouseWheel(int delta);
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
		Color SelectionColor,UnfocusedSelectionColor,SelectionForeColor,FocusRectColor,HighLightColor,HighLightForeColor;
		CoreLib::List<Control *> Selection;
		int SelectedIndex = -1;
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
		virtual bool DoMouseDown(int x, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseUp(int x, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseMove(int x, int Y);
		virtual bool DoMouseWheel(int delta) override;
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
		virtual void Posit(int left, int top, int width, int height) override;
		virtual void Draw(int absX, int absY) override;
		virtual void SizeChanged();
		virtual void HandleMessage(const UI_MsgArgs *Args);
		virtual bool DoMouseDown(int x, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseUp(int x, int Y, SHIFTSTATE Shift);
		virtual bool DoMouseMove(int x, int Y);
		virtual bool DoMouseWheel(int delta);
		virtual bool DoKeyDown(unsigned short Key, SHIFTSTATE Shift);
		virtual void SetFocus();
		virtual void LostFocus(Control * newFocus);
		virtual bool DoClosePopup();
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

	class VScrollPanel : public Container
	{
	private:
		ScrollBar * vscrollBar = nullptr;
		Container * content = nullptr;
		void ScrollBar_Changed(UI_Base * sender);
	public:
		VScrollPanel(Container * parent);
		CoreLib::List<CoreLib::RefPtr<Control>> & GetChildren()
		{
			return content->Controls;
		}
		virtual void SizeChanged() override;
		virtual void AddChild(Control *nControl) override;
		virtual void RemoveChild(Control *AControl) override;
		virtual bool DoMouseWheel(int delta) override;
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

/***********************************************************************
ASEFILE.H
***********************************************************************/
#ifndef CORE_LIB_GRAPHICS_ASE_H
#define CORE_LIB_GRAPHICS_ASE_H


namespace CoreLib
{
	namespace Graphics
	{
		class AseValue
		{
		public:
			CoreLib::Basic::String Str;
			VectorMath::Vec4 Val;
			AseValue() = default;
			AseValue(const CoreLib::Basic::String & str) { Str = str; }
			AseValue(VectorMath::Vec4 val) { Val = val; }
			AseValue(float val) { Val = VectorMath::Vec4::Create(val, 0.0f, 0.0f, 0.0f); }
		};
		class AseMaterial
		{
		public:
			CoreLib::Basic::String Name;
			CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, AseValue> Fields;
			CoreLib::Basic::List<CoreLib::Basic::RefPtr<AseMaterial>> SubMaterials;
		};
		class AseMeshFace
		{
		public:
			int Ids[3];
			int SmoothGroup;
			int MaterialId;
		};
		class AseMeshAttribFace
		{
		public:
			int Ids[3];
		};
		template<typename T>
		class AseMeshVertAttrib
		{
		public:
			CoreLib::Basic::List<T> Data;
			CoreLib::Basic::List<AseMeshAttribFace> Faces;
		};
		class AseMesh
		{
		public:
			CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, AseValue> Attributes;
			CoreLib::Basic::List<VectorMath::Vec3> Vertices;
			CoreLib::Basic::List<AseMeshFace> Faces;
			AseMeshVertAttrib<VectorMath::Vec3> Colors;
			AseMeshVertAttrib<VectorMath::Vec3> Normals;
			CoreLib::Basic::List<AseMeshVertAttrib<VectorMath::Vec3>> TexCoords;
			void RecomputeNormals();
			void ConstructPerVertexFaceList(Basic::List<int> & faceCountAtVert, Basic::List<int> & vertFaceList) const;
		};
		class AseGeomObject
		{
		public:
			CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, AseValue> Attributes;
			CoreLib::Basic::RefPtr<AseMesh> Mesh;
			int MaterialId;
		};
		class AseFile
		{
		public:
			CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, AseValue> Attributes;
			CoreLib::Basic::List<CoreLib::Basic::RefPtr<AseGeomObject>> GeomObjects;
			CoreLib::Basic::List<CoreLib::Basic::RefPtr<AseMaterial>> Materials;
		public:
			void Parse(const CoreLib::Basic::String & content, bool flipYZ);
			void LoadFromFile(const CoreLib::Basic::String & fileName, bool flipYZ);
		};
	}
}

#endif

/***********************************************************************
BBOX.H
***********************************************************************/
#ifndef CORE_LIB_GRAPHICS_BBOX_H
#define CORE_LIB_GRAPHICS_BBOX_H


namespace CoreLib
{
	namespace Graphics
	{
		using namespace VectorMath;

		class BBox
		{
		public:
			union
			{
				struct
				{
					float xMin, yMin, zMin, xMax, yMax, zMax;
#pragma warning(suppress : 4201)
				};
				struct
				{
					Vec3 Min, Max;
#pragma warning(suppress : 4201)
				};
			};
			inline int MaxDimension() const
			{
				float xsize = xMax-xMin;
				float ysize = yMax-yMin;
				float zsize = zMax-zMin;
				if (xsize>ysize)
				{
					if (xsize>zsize)
						return 0;
					else
						return 2;
				}
				else
				{
					if (ysize > zsize)
						return 1;
					else
						return 2;
				}
			}
			inline void Init()
			{
				xMin = yMin = zMin = FLT_MAX;
				xMax = yMax = zMax = -FLT_MAX;
			}
			inline void Union(const BBox & box)
			{
				xMin = Math::Min(box.xMin, xMin);
				yMin = Math::Min(box.yMin, yMin);
				zMin = Math::Min(box.zMin, zMin);

				xMax = Math::Max(box.xMax, xMax);
				yMax = Math::Max(box.yMax, yMax);
				zMax = Math::Max(box.zMax, zMax);
			}
			inline void Union(const Vec3 &v)
			{
				xMin = Math::Min(v.x, xMin);
				yMin = Math::Min(v.y, yMin);
				zMin = Math::Min(v.z, zMin);

				xMax = Math::Max(v.x, xMax);
				yMax = Math::Max(v.y, yMax);
				zMax = Math::Max(v.z, zMax);
			}
			inline BBox Intersection(const BBox & box)
			{
				BBox rs;
				rs.xMin = Math::Max(box.xMin, xMin);
				rs.yMin = Math::Max(box.yMin, yMin);
				rs.zMin = Math::Max(box.zMin, zMin);

				rs.xMax = Math::Min(box.xMax, xMax);
				rs.yMax = Math::Min(box.yMax, yMax);
				rs.zMax = Math::Min(box.zMax, zMax);
				return rs;
			}
			inline bool Contains(const Vec3 & v)
			{
				return v.x >= xMin && v.x <= xMax &&
					   v.y >= yMin && v.y <= yMax &&
					   v.z >= zMin && v.z <= zMax;
			}
			inline bool ContainsNoOverlap(const Vec3 & v)
			{
				return v.x >= xMin && v.x < xMax &&
					v.y >= yMin && v.y < yMax &&
					v.z >= zMin && v.z < zMax;
			}
			inline bool Intersects(const BBox & box)
			{
				return !(xMin>=box.xMax || yMin >= box.yMax || zMin >= box.zMax || 
					xMax <= box.xMin || yMax <= box.yMin || zMax <= box.zMin);
			}
			inline void GetCornerPoints(Vec3 cornerPoints[8]) const
			{
				cornerPoints[0] = Vec3::Create(xMin, yMin, zMin);
				cornerPoints[1] = Vec3::Create(xMax, yMin, zMin);
				cornerPoints[2] = Vec3::Create(xMin, yMax, zMin);
				cornerPoints[3] = Vec3::Create(xMax, yMax, zMin);
				cornerPoints[4] = Vec3::Create(xMin, yMin, zMax);
				cornerPoints[5] = Vec3::Create(xMax, yMin, zMax);
				cornerPoints[6] = Vec3::Create(xMin, yMax, zMax);
				cornerPoints[7] = Vec3::Create(xMax, yMax, zMax);
			}
			float Distance(Vec3 p);
		};

		inline bool RayBBoxIntersection(const BBox & bbox, const Vec3 & origin, const Vec3 & dir, float & tmin, float & tmax)
		{
			float tymin, tymax, tzmin, tzmax;
			Vec3 rdir = dir;
			rdir.x = 1.0f / dir.x;
			rdir.y = 1.0f / dir.y;
			rdir.z = 1.0f / dir.z;

			if (rdir.x >= 0)
			{
				tmin = (bbox.Min.x - origin.x) * rdir.x;
				tmax = (bbox.Max.x - origin.x) * rdir.x;
			}
			else
			{
				tmin = (bbox.Max.x - origin.x) * rdir.x;
				tmax = (bbox.Min.x - origin.x) * rdir.x;
			}
			if (rdir.y >= 0)
			{
				tymin = (bbox.Min.y - origin.y) * rdir.y;
				tymax = (bbox.Max.y - origin.y) * rdir.y;
			}
			else
			{
				tymin = (bbox.Max.y - origin.y) * rdir.y;
				tymax = (bbox.Min.y - origin.y) * rdir.y;
			}
			if (tmin - tymax > Epsilon || tymin - tmax > Epsilon)
				return false;
			if (tymin > tmin)
				tmin = tymin;
			if (tymax < tmax)
				tmax = tymax;
			if (rdir.z >= 0)
			{
				tzmin = (bbox.Min.z - origin.z) * rdir.z;
				tzmax = (bbox.Max.z - origin.z) * rdir.z;
			}
			else
			{
				tzmin = (bbox.Max.z - origin.z) * rdir.z;
				tzmax = (bbox.Min.z - origin.z) * rdir.z;
			}
			if (tmin - tzmax > Epsilon || tzmin - tmax > Epsilon)
				return false;
			if (tzmin > tmin)
				tmin = tzmin;
			if (tzmax < tmax)
				tmax = tzmax;
			return tmin <= tmax;
		}

		inline void TransformBBox(BBox & bboxOut, const Matrix4 & mat, const BBox & bboxIn)
		{
			Vec3 v, v_t;
			bboxOut.Init();
			for (int i = 0; i < 8; i++)
			{
				if (i & 1)
					v.x = bboxIn.xMax;
				else
					v.x = bboxIn.xMin;
				if (i & 2)
					v.y = bboxIn.yMax;
				else
					v.y = bboxIn.yMin;
				if (i & 4)
					v.z = bboxIn.zMax;
				else
					v.z = bboxIn.zMin;
				mat.Transform(v_t, v);
				bboxOut.Union(v_t);
			}
		}
	}
}

#endif

/***********************************************************************
BEZIERMESH.H
***********************************************************************/
#ifndef CORE_LIB_BEZIER_MESH_H
#define CORE_LIB_BEZIER_MESH_H


namespace CoreLib
{
	namespace Graphics
	{
		using namespace VectorMath;
		using namespace CoreLib::Basic;

		class BezierPatch
		{
		public:
			Vec3 ControlPoints[4][4];
			Vec3 TangentU[3][4];
			Vec3 TangentV[3][4];
			Vec2 TexCoords[4];
		};

		class BezierMesh
		{
		public:
			List<BezierPatch> Patches;
		};

		struct ObjModel;
		BezierMesh BezierMeshFromQuadObj(ObjModel & obj);
	}
}

#endif

/***********************************************************************
OBJMODEL.H
***********************************************************************/
#ifndef RAY_TRACE_PRO_OBJ_MODEL_H
#define RAY_TRACE_PRO_OBJ_MODEL_H


namespace CoreLib
{
	namespace Graphics
	{
		enum class PolygonType
		{
			Triangle, Quad
		};
		struct ObjFace
		{
			int VertexIds[4];
			int NormalIds[4];
			int TexCoordIds[4];
			unsigned int SmoothGroup;
			int MaterialId;
		};
		const int ObjMaterialVersion = 1;
		struct ObjMaterial
		{
			Basic::String Name;
			float SpecularRate;
			VectorMath::Vec3 Diffuse, Specular;
			bool IsOpaque;
			Basic::String DiffuseMap, BumpMap, AlphaMap;
			ObjMaterial()
			{
				IsOpaque = false; 
				Diffuse.SetZero();
				Specular.SetZero();
				SpecularRate = 0.0f;
			}
		};
		struct ObjModel
		{
			Basic::List<Basic::RefPtr<ObjMaterial>> Materials;
			Basic::List<VectorMath::Vec3> Vertices, Normals;
			Basic::List<VectorMath::Vec2> TexCoords;
			Basic::List<ObjFace> Faces;
			void ConstructPerVertexFaceList(Basic::List<int> & faceCountAtVert, Basic::List<int> & vertFaceList) const;
			void SaveToBinary(IO::BinaryWriter & writer);
			bool LoadFromBinary(IO::BinaryReader & reader);
		};
		bool LoadObj(ObjModel & mdl, const char * fileName, PolygonType polygonType = PolygonType::Triangle);
		void RecomputeNormals(ObjModel & mdl);
	}
}

#endif

/***********************************************************************
CAMERA.H
***********************************************************************/
#ifndef GX_GL_CAMERA_H
#define GX_GL_CAMERA_H
namespace CoreLib
{
	namespace Graphics
	{
		class Camera
		{
		public:
			Camera();
		public:
			float alpha,beta;
			VectorMath::Vec3 pos,up,dir;
			bool CanFly;
			void GoForward(float u);
			void MoveLeft(float u);
			void TurnLeft(float u);
			void TurnUp(float u);
			void GetTransform(VectorMath::Matrix4 & mat);
			void GetView(ViewFrustum & view);
			void Reset();
			void GetInverseRotationMatrix(float rot[9]);
		};

#ifdef _WIN32
		class CameraController
		{
		public:
			static void HandleCameraKeys(Camera & camera, VectorMath::Matrix4 & transform, float dtime, float minSpeed, float maxSpeed, bool flipYZ);
		};
#endif
	}
}
#endif

/***********************************************************************
GGX.H
***********************************************************************/
#ifndef CORELIB_GRAPHICS_GGX_H
#define CORELIB_GRAPHICS_GGX_H


namespace CoreLib
{
	namespace Graphics
	{
		CoreLib::Basic::List<VectorMath::Vec2> ComputeTextureFV(float maxRoughness, int size);
		CoreLib::Basic::List<float> ComputeTextureD(float maxRoughness, int size);
		TextureFile ComputeTextureFileFV(float maxRoughness, int size);
		TextureFile ComputeTextureFileD(float maxRoughness, int size);
	}
}

#endif
