#ifndef GX_WIN_CTRLS_H
#define GX_WIN_CTRLS_H
#pragma warning(push)
#pragma warning(disable:4458)
#include <Windows.h>
#include <Unknwn.h>
#include <objidl.h>
#include <gdiplus.h>
#pragma warning(pop)
#include "../Basic.h"
#include "WinMessage.h"

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