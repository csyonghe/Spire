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

/***********************************************************************
WINACCEL.H
***********************************************************************/
#ifndef GX_WINACCEL_H
#define GX_WINACCEL_H

#include <windows.h>

namespace CoreLib
{
	namespace WinForm
	{
		using namespace CoreLib;
		using namespace CoreLib::Basic;

		class MenuItem;

		class Accelerator : public Object
		{
			friend class AccelTable;
		public:
			typedef unsigned char AccelAuxKey;
			static const unsigned char Ctrl = FCONTROL;
			static const unsigned char Shift = FSHIFT;
			static const unsigned char Alt = FALT;
		private:
			ACCEL accel;
		public:
			Accelerator(AccelAuxKey auxKey, Word mainKey);
			Accelerator();
		};

		class AccelTable : public Object
		{
		private:
			HACCEL handle;
			List<ACCEL> accels;
		public:
			AccelTable();
			~AccelTable();
		public:
			HACCEL GetHandle();
			void RegisterAccel(Accelerator acc, MenuItem * item);
			void UpdateAccel(Accelerator acc, MenuItem * item);
			void Update();
		};
	}
}

#endif

/***********************************************************************
WINMESSAGE.H
***********************************************************************/
#ifndef GX_WINMESSAGE_H
#define GX_WINMESSAGE_H
#define   ULONG_PTR   void*
#pragma warning(push)
#pragma warning(disable:4458)
#pragma comment(lib, "gdiplus.lib")
#include <Unknwn.h>
#include <gdiplus.h>
#pragma warning(pop)

namespace CoreLib
{
	namespace WinForm
	{
		using namespace CoreLib::Basic;
		const int gxMsgKeyBroadcast = WM_APP + 1;

		using namespace Gdiplus;
		class WinMessage
		{
		public:
			HWND hWnd;
			UINT message;
			WPARAM wParam;
			LPARAM lParam;

			WinMessage(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		};

		class WinNotification
		{
		public:
			HWND hWnd;
			UINT code;
		};

		class EventArgs : Object
		{
		public:

		};

		enum MouseButton
		{
			mbNone, mbLeft, mbRight, mbMiddle
		};

		class MouseEventArgs : public EventArgs
		{
		public:
			MouseButton Button;
			int X,Y;
			int Delta;
			bool BaseControl, Shift, LButton, MButton, RButton;
			bool *Processed;
			MouseEventArgs();
			MouseEventArgs(MouseButton btn, int _x, int _y, int _delta,  unsigned int buttons, bool * processed);
		};

		class KeyEventArgs : public EventArgs
		{
		public:
			wchar_t Key;
			int KeyCode;
			int * KeyCodeRef;
			bool BaseControl, Shift, Alt;
			KeyEventArgs();
			KeyEventArgs(wchar_t key, int * keycode, bool ctrl, bool shift, bool alt);
		};

		class WindowCloseEventArgs : public EventArgs
		{
		public:
			bool Cancel;
		};

		class ResizingEventArgs : public EventArgs
		{
		public:
			int Width;
			int Height;
			int Left;
			int Top;
		};

		class Rect : public Object
		{
		public:
			int Left;
			int Top;
			int Right;
			int Bottom;
		};

		class PaintEventArgs : public EventArgs
		{
		public:
			RefPtr<Gdiplus::Graphics> Graphics;
			Rect UpdateRect;
		};

		class UserMessageArgs : public EventArgs
		{
		public:
			unsigned int Message;
			LPARAM LParam;
			WPARAM WParam;
		};

		typedef Event<Object *, UserMessageArgs> UserEvent;
		typedef Event<Object *, EventArgs> NotifyEvent;
		typedef Event<Object *, MouseEventArgs> MouseEvent;
		typedef Event<Object *, KeyEventArgs> KeyEvent;
		typedef Event<Object *, WindowCloseEventArgs &> WindowCloseEvent;
		typedef Event<Object *, ResizingEventArgs &> ResizingEvent;
		typedef Event<Object *, PaintEventArgs> PaintEvent;

	}
}

#endif

/***********************************************************************
DEBUG.H
***********************************************************************/
#ifndef GX_WIN_DEBUG_H
#define GX_WIN_DEBUG_H


namespace CoreLib
{
	namespace Diagnostics
	{
		using namespace CoreLib::Basic;
		class Debug
		{
		public:
			static void Write(const String & text)
			{
				if (IsDebuggerPresent() != 0)
				{
					OutputDebugStringW(text.Buffer());
				}
			}
			static void WriteLine(const String & text)
			{
				if (IsDebuggerPresent() != 0)
				{
					OutputDebugStringW(text.Buffer());
					OutputDebugStringW(L"\n");
				}
			}
		};

		class DebugWriter
		{
		public:
			DebugWriter & operator << (const String & text)
			{
				Debug::Write(text);
				return *this;
			}
		};
	}
}

#endif

/***********************************************************************
WINAPP.H
***********************************************************************/
#ifndef GX_WIN_APP_H
#define GX_WIN_APP_H


namespace CoreLib
{
	namespace WinForm
	{
		using namespace CoreLib::Text;

		class Component;
		class BaseForm;
		class AccelTable;

		class Application : public Object
		{
		private:
			static DWORD uiThreadId;
			static int globalUID;
			static HINSTANCE instance, prevInstance;
			static HWND mainFormHandle;
			static BaseForm * mainForm;
			static String *cmdLine;
			static Dictionary<HWND, Component* > *components;
			static Dictionary<UINT_PTR, Object *> *objMap;
			static int cmdShow;
			static void * gdiToken;
			static AccelTable * accelTable;
			static LOGFONT SysFont;
			static bool terminate;
			static NotifyEvent * onMainLoop;
			static EnumerableHashSet<BaseForm*> forms;
			static void RegisterControlClass();
			static void RegisterFormClass();
			static void RegisterGLFormClass();
			static void ProcessMessage(MSG & msg);
		public:
			static const wchar_t * ControlClassName;
			static const wchar_t * FormClassName;
			static const wchar_t * GLFormClassName;
			static void Init();
			static void Init(HINSTANCE hIns, HINSTANCE hPrevIns, LPTSTR cmdLine, int show = 0);
			static void Dispose();
			static void RegisterComponent(Component * Ctrl);
			static void UnRegisterComponent(Component *  Ctrl);
			static Component * GetComponent(const HWND handle); 
			static HINSTANCE GetHandle();
			static void SetMainLoopEventHandler(NotifyEvent * mainLoop);
			static void Run(const BaseForm * MainForm, bool NonBlocking = false);
			static void Terminate();
			static HWND GetMainFormHandle();
			static BaseForm * GetMainForm();
			static void DoEvents();
			static int RegisterHandle(Object * obj);
			static void UnRegisterHandle(UINT_PTR _handle);
			static Object * GetObject(UINT_PTR _handle);
			static String GetCommandLine();
			static String GetExePath();
			static void SetAccel(AccelTable * acc);
			static void RegisterObjectHandle(UINT_PTR handle, Object * obj);
			static Object * GetObjectFromHandle(UINT_PTR handle);
			static LOGFONT GetSysFont();
			static int GenerateGUID();
			static bool IsUIThread();
			static void RegisterForm(BaseForm * form);
			static void UnregisterForm(BaseForm * form);

		};

		class CommandLineParser : public Object
		{
		private:
			LexStream stream;
		public:
			CommandLineParser(const String & cmdLine);
			String GetFileName();
			bool OptionExists(const String & opt);
			String GetOptionValue(const String & opt);
			String GetToken(int id);
			int GetTokenCount();
		};

		class SystemMetrics
		{
		public:
			static int GetScreenWidth();
			static int GetScreenHeight();
		};

		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	}
}

#endif

/***********************************************************************
WINCTRLS.H
***********************************************************************/
#ifndef GX_WIN_CTRLS_H
#define GX_WIN_CTRLS_H
#pragma warning(push)
#pragma warning(disable:4458)
#include <objidl.h>
#pragma warning(pop)

using namespace CoreLib;
using namespace CoreLib::Basic;
namespace CoreLib
{
	namespace WinForm
	{
		using namespace Gdiplus;

		class WinMessage;
		class BaseForm;

		class Component;

		class Action : public Object
		{
		public:
			virtual void Apply(Component * comp) = 0;
		};

		class Component : public Object
		{
			friend LRESULT CALLBACK SubClassWndProc(HWND hWnd,UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass,
				DWORD_PTR dwRefData);
			friend LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		protected:
			List<RefPtr<Component>> children;
			HWND handle;
			WNDPROC prevProc;
		protected:
			BaseForm * GetOwnerForm();
			virtual int ProcessMessage(WinMessage & msg);
			virtual int ProcessNotification(WinNotification note);
			void BroadcastMessage(WinMessage & msg);
			void SubClass();
			void UnSubClass();
		public:
			Component();
			~Component()
			{
				Destroy();
			}
			String Name;
			HWND GetHandle() const;
			void SetParent(const Component * Parent);
			Component * GetParent();
			UserEvent OnUserEvent;
			virtual void Destroy();
			virtual void AddChild(Component * comp);
			virtual void RemoveChild(Component * comp);
			virtual void ClearChildren()
			{
				for (auto & child : children)
					child = nullptr;
				children.Clear();
			}
			int GetChildrenCount();
			Component * GetChildren(int id);
			void ApplyActionToComponentAndChildren(Action * act);
		};

		class BaseControl : public Component 
		{
		protected:
			RefPtr<Font> font;
			HFONT nativeFont;
			bool ownerDraw;
			bool mouseDown;
			bool captureClick;
			void SetStyle(int StyleType, unsigned int Style, bool val);
			bool GetStyle(int StyleType, unsigned int Style);
			int ProcessMessage(WinMessage & msg);
			int ProcessNotification(WinNotification note);
			void InitFont();
			virtual void _Click();
			virtual void _Focused();
			virtual void _LostFocus();
			virtual void _OnResize();
		public:
			BaseControl();
			virtual void Destroy();
			int GetClientWidth();
			int GetClientHeight();
			int GetWidth();
			int GetHeight();
			void EnableRedraw();
			void DisableRedraw();
			void Refresh();
			void Invalidate();
			void BringToTop();
			void SetWidth(int w);
			void SetHeight(int h);
			void SetPosition(int x, int y, int w, int h);
			int GetLeft();
			int GetTop();
			void SetLeft(int l);
			void SetTop(int t);
			bool GetEnabled();
			void SetEnabled(bool val);
			bool GetVisible();
			void SetVisible(bool val);
			bool Focused();
			void SetFocus();
			void SetFont(Font * font);
			void SetTabStop(bool ts);
			bool GetTabStop();
		public:
			NotifyEvent OnClick;
			NotifyEvent OnDblClick;
			NotifyEvent OnFocus;
			NotifyEvent OnLostFocus;
			NotifyEvent OnMouseEnter;
			NotifyEvent OnMouseLeave;
			MouseEvent OnMouseDown;
			MouseEvent OnMouseUp;
			MouseEvent OnMouseMove;
			MouseEvent OnMouseWheel;
			KeyEvent OnKeyPressed;
			KeyEvent OnKeyUp;
			KeyEvent OnKeyDown;
			ResizingEvent OnResizing;
			NotifyEvent OnResized;
			PaintEvent OnPaint;
		};

		enum class ScrollBars
		{
			None = -1, Both = SB_BOTH, Horizontal = SB_HORZ, Vertical = SB_VERT
		};

		class Control : public BaseControl
		{
		protected:
			ScrollBars scrollBars = ScrollBars::None;
			virtual void CreateWnd(Component * parent);
			int ProcessNotification(WinNotification note);

			void Init(Component * parent);
			Control();

		public:
			Control(Component * parent);
			~Control();
			void Destroy();
		};

		class ScrollPanel : public Control
		{
		private:
			int lastScrollY = 0;
			int lastScrollX = 0;
			int maxYScroll = 0, maxXScroll = 0;
			RefPtr<Control> hiddenCtrl;
		protected:
			virtual int ProcessMessage(WinMessage & msg) override;
			virtual void _OnResize() override;
			ScrollPanel();
			void VScroll(int delta);
			void HScroll(int delta);
		public:
			~ScrollPanel()
			{
				hiddenCtrl = nullptr;
			}
			ScrollPanel(Component * parent);
			virtual void AddChild(Component * comp) override
			{
				Control::AddChild(comp);
				UpdateScrollBar();
			}
			virtual void RemoveChild(Component * comp) override
			{
				Control::RemoveChild(comp);
				UpdateScrollBar();
			}
			virtual void ClearChildren() override
			{
				Control::ClearChildren();
				VScroll(lastScrollY);
				HScroll(lastScrollX);
				UpdateScrollBar();
			}
			int GetVerticalScrollPosition();
			void SetVerticalScrollPosition(int val);
			void SetScrollBar(ScrollBars scroll);
			void UpdateScrollBar();
		};

	}
}

#endif

/***********************************************************************
WINMENU.H
***********************************************************************/
#ifndef GX_WINMENU_H
#define GX_WINMENU_H


namespace CoreLib
{
	namespace WinForm
	{
		class CustomMenu;
		class PopupMenu;
		class Form;

		class MenuItem : public Object
		{
		public:
			enum MenuItemType
			{
				mitCommand, mitDropDown, mitSeperator
			};
		private:
			String text;
			String shortcutText;
			MenuItemType type;
			RefPtr<PopupMenu> popup;
			CustomMenu * parent;
			int ident;
			void UpdateText();
			void Init(int pos);
			void SetBitmap();
			void SetState(bool _enabled, bool _checked);
			void GetState(bool &_enabled, bool &_checked);
		public:
			NotifyEvent OnClick;
			NotifyEvent OnPopup;
			PopupMenu * GetSubMenus();
			void SetText(const String & _text, const String & _shortcut);
			void SetText(const String & _text);
			void SetShortcutText(const String & _shortcut);
			void SetType(MenuItemType ntype);
			void SetEnabled(bool _enabled);
			void SetChecked(bool _checked);
			bool GetEnabled();
			bool GetChecked();

			String GetText();
			String GetCompleteText();
			String GetShortcutText();
			void UpdateState();
			void Clicked();
			int GetIdentifier();
		public:
			MenuItem(CustomMenu * menu, int pos = -1);
			MenuItem(MenuItem * menu, int pos = -1);
			~MenuItem();
		};

		class CustomMenu : public Component
		{
			friend class MenuItem;
		protected:
			List<RefPtr<MenuItem>> items;
			int ident;
			MenuItem * _owner;
		public:
			void AddItem(MenuItem * item);
			void InsertItem(MenuItem * item, int pos);
			void RemoveItem(MenuItem * item);
			void Clear();
			HMENU GetHandle();
			int GetCount();
			MenuItem * GetItem(int id);
			MenuItem * operator [](int id);
			CustomMenu();
			void OnPopup();
			virtual ~CustomMenu();
		};

		class PopupMenu : public CustomMenu
		{
		private:
			Form * ownerForm;
		public:
			PopupMenu(Form * _owner = 0);
			void Popup(int x, int y);
		
		};

		class MainMenu : public CustomMenu
		{
		public:
			MainMenu();
		};
	}
}
#endif

/***********************************************************************
WINFORM.H
***********************************************************************/
#ifndef GX_WINFORM_H
#define GX_WINFORM_H


namespace CoreLib
{
	namespace WinForm
	{
		class Button;

		enum FormBorder
		{
			fbNone,
			fbSingle,
			fbSizeable,
			fbFixedDialog,
			fbToolWindow,
			fbFixedToolWindow
		};

		class BaseForm : public BaseControl 
		{
		public:
			enum DialogResult
			{
				OK, Cancel, Yes, No, Retry, Ignore
			};
		protected:
			DialogResult dlgResult;
			bool closed, firstShown;
			bool wantTab = false, wantChars = false;
			BaseForm * dlgOwner;
			Button * defButton, * cancelButton;
			int ProcessMessage(WinMessage & msg);
			virtual void Create()=0;
		private:
			MainMenu * menu;
			RefPtr<AccelTable> accelTable;
		public:
			BaseForm();
			virtual ~BaseForm();
		public:
			void		Show();
			void		BringToFront();
			DialogResult ShowModal(BaseForm * owner);
			void		Close();
			void		SetDialogResult(DialogResult result);
			void		SetText(String text);
			String	GetText();
			void		SetBorder(FormBorder border);
			FormBorder	GetBorder();
			bool		GetShowInTaskBar();
			void		SetShowInTaskBar(bool val);
			bool		GetControlBox();
			void		SetControlBox(bool val);
			bool		GetTopMost();
			void		SetTopMost(bool val);
			bool		GetMinimizeBox();
			void		SetMinimizeBox(bool val);
			bool		GetMaximizeBox();
			void		SetMaximizeBox(bool val);
			void		SetClientWidth(int w);
			void		SetClientHeight(int h);
			int			GetClientWidth();
			int			GetClientHeight();
			void		SetWantTab(bool val);
			bool		GetWantTab();
			void		SetWantChars(bool val);
			bool		GetWantChars();
			void		SetMainMenu(MainMenu * _menu);
			void		SetDefaultButton(Button * btn);
			Button *	GetDefaultButton();
			void		SetCancelButton(Button * btn);
			Button *	GetCancelButton();
			int			MessageBox(const String & msg, const String & title, unsigned int style);
			void		Invoke(const Event<> & func);
			void		InvokeAsync(const Event<> & func);
			template <typename Func>
			void Invoke(const Func & f)
			{
				Invoke(Event<>(f));
			}
		public:
			NotifyEvent OnLoad;
			NotifyEvent OnShow;
			WindowCloseEvent OnClose;
			void RegisterAccel(Accelerator acc, MenuItem * item);
			void UpdateAccel(Accelerator acc, MenuItem * item);
			void UpdateAccel();
		};

		class Form : public BaseForm
		{
		protected:
			virtual void Create();
		public:
			Form();
			~Form();
		};
	}
}
#endif

/***********************************************************************
WINTIMER.H
***********************************************************************/
#ifndef GX_WIN_TIMER_H
#define GX_WIN_TIMER_H

namespace CoreLib
{
	namespace WinForm
	{
		using namespace CoreLib::Basic;

		class Timer : public Object
		{
		private:
			UINT_PTR timerHandle;
		public:
			int Interval;
			NotifyEvent OnTick;
			void StartTimer();
			void StopTimer();
			Timer();
			~Timer();
		};
	}
}

#endif

/***********************************************************************
WINBUTTONS.H
***********************************************************************/
#ifndef GX_WINBUTTONS_H
#define GX_WINBUTTONS_H


namespace CoreLib
{
	namespace WinForm
	{
		class ClearDefaultButtonAction : public Action
		{
		public:
			void Apply(Component * comp);
		};

		class CustomButton : public Control
		{		
		protected:
			virtual void CreateWnd(Component * parent);
			virtual int ProcessNotification(WinNotification note);
			CustomButton();
		public:
			CustomButton(Component * parent);
			void SetText(const String & text);
		
		};

		class Button : public CustomButton
		{
			friend class BaseForm;
			friend class ClearDefaultButtonAction;
		protected:
			int ProcessNotification(WinNotification note);
			int ProcessMessage(WinMessage & msg);
		public:
			Button(Component * parent);
			void SetDefault(bool def);

		};

		class CheckBox : public CustomButton
		{
		protected:
			virtual void CreateWnd(Component * parent);
			virtual int ProcessNotification(WinNotification note);
			CheckBox();
		public:
			CheckBox(Component * parent);
			void SetChecked(bool chk);
			bool GetChecked();
		};

		class RadioBox : public CheckBox
		{
		protected:
			virtual void CreateWnd(Component * parent);
			virtual int ProcessNotification(WinNotification note);
		public:
			RadioBox(Component * parent);
		};

		class GroupBox : public CustomButton
		{
		protected:
			virtual void CreateWnd(Component * parent);
		public:
			GroupBox(Component * parent);
		};
	}
}
#endif

/***********************************************************************
WINCOMMONDLG.H
***********************************************************************/
#ifndef GX_WIN_COMMON_DLG_H
#define GX_WIN_COMMON_DLG_H


namespace CoreLib
{
	namespace WinForm
	{
		class FileDialog : public Object
		{
		private:
			static const int FilterBufferSize = 512;
			static const int FileBufferSize = 25600;
		private:
			Component * owner;
			String initDir;
		private:
			wchar_t fileBuf[FileBufferSize];
			wchar_t filterBuf[FilterBufferSize];
			OPENFILENAME fn;
			void PrepareDialog();
			void PostDialogShow();
		public:
			String Filter;
			String DefaultEXT;
			String FileName;
			List<String> FileNames;
			bool MultiSelect;
			bool CreatePrompt;
			bool FileMustExist;
			bool HideReadOnly;
			bool OverwritePrompt;
			bool PathMustExist;
			bool ShowOpen();
			bool ShowSave();
			FileDialog(const Component * _owner);
			~FileDialog();
		};

		class ColorDialog : public Object
		{
		private:
			Component * owner;
			COLORREF cr[16];
			CHOOSECOLOR cs;
		public:
			COLORREF Color;
			bool FullOpen;
			bool PreventFullOpen;
			bool ShowColor();
			ColorDialog(Component * _owner);	
		};
	}
}

#endif

/***********************************************************************
WINLISTBOX.H
***********************************************************************/
#ifndef GX_WINLISTBOX_H
#define GX_WINLISTBOX_H


namespace CoreLib
{
	namespace WinForm
	{
		class ListBox : public Control
		{
		protected:
			virtual void CreateWnd(Component * parent) override;
			virtual int ProcessNotification(WinNotification note) override;
			ListBox();
			void _SelChange();
		public:
			NotifyEvent SelectionChanged;
			ListBox(Component * parent);
			void Clear();
			int Count();
			void AddItem(const String & text);
			void InsertItem(int id, const String & text);
			void RemoveItem(int id);
			String GetItem(int id);
			int GetSelectionIndex();
			void SetSelectionIndex(int idx);
		};

		enum class ComboBoxStyle
		{
			Simple = CBS_SIMPLE, DropDown = CBS_DROPDOWN, DropDownList = CBS_DROPDOWNLIST
		};

		class ComboBox : public Control
		{
		protected:
			Component * parent;
			virtual void CreateWnd(Component * pParent) override;
			virtual int ProcessNotification(WinNotification note) override;
			ComboBox();
			void _SelChange();
			void _TextChange();

		public:
			NotifyEvent SelectionChanged;
			NotifyEvent TextChanged;

			ComboBox(Component * parent);
			void Clear();
			int Count();
			void SetStyle(ComboBoxStyle style);
			void AddItem(const String & text);
			void InsertItem(int id, const String & text);
			void RemoveItem(int id);
			String GetItem(int id);
			int GetSelectionIndex();
			void SetSelectionIndex(int idx);
		};
	}
}
#endif

/***********************************************************************
WINTEXTBOX.H
***********************************************************************/
#ifndef GX_WINTEXTBOX_H
#define GX_WINTEXTBOX_H


namespace CoreLib
{
	namespace WinForm
	{
		class TextBox : public Control
		{		
		protected:
			virtual void CreateWnd(Component * parent) override;
			virtual int ProcessMessage(WinMessage & msg) override;

			virtual int ProcessNotification(WinNotification note) override;
			TextBox();
		public:
			NotifyEvent OnChanged;
			TextBox(Component * parent);
			void SetText(const String & text);
			String GetText();
			void SetReadOnly(bool val);
			bool GetReadOnly();
			void SetMultiLine(bool val);
			bool GetMultiLine();
			void SetPassword(bool val);
			bool GetPassword();
			void SetAcceptNumber(bool val);
			bool GetAcceptNumber();
		};

		class Label : public Control
		{		
		protected:
			virtual void CreateWnd(Component * parent);
			Label();
		public:
			Label(Component * parent);
			void SetText(const String & text);
			String GetText();
		};
	}
}

#endif
