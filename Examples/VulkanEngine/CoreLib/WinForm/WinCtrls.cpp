#include "WinCtrls.h"
#include "WinMessage.h"
#include <windowsx.h>
#include "WinApp.h"
#include <commctrl.h>
#include "WinForm.h"
#include <stdio.h>
namespace CoreLib
{
	namespace WinForm
	{
		LRESULT CALLBACK SubClassWndProc(HWND hWnd,UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/,
			DWORD_PTR /*dwRefData*/)
		{
			int rs = -1;
			Component * comp = 0;
			if (message == WM_COMMAND)
			{
				if (lParam != 0)
				{
					comp = Application::GetComponent((HWND)lParam);
					WinNotification note;
					note.code = HIWORD(wParam);
					note.hWnd = (HWND)lParam;
					if (comp)
						rs = comp->ProcessNotification(note)?0:1;
				}
			}
			else if (message == WM_NOTIFY)
			{
				WinNotification note;
				note.code = ((LPNMHDR)lParam)->code;
				note.hWnd = ((LPNMHDR)lParam)->hwndFrom;
				comp = Application::GetComponent(note.hWnd);
				if (comp)
					rs = comp->ProcessNotification(note)?0:1;
			}
			WinMessage msg(hWnd, message, wParam, lParam);
			comp = Application::GetComponent(hWnd);
			if (comp)
				rs = comp->ProcessMessage(msg);
			// Call Original WndProc
			if (rs != -1)
				return rs;
			//if (msg.message == WM_CHAR && msg.wParam == 0)
			//	return true;
			return DefSubclassProc(hWnd, msg.message, msg.wParam, msg.lParam);
		}

		Component::Component()
		{
			handle = 0;
		}

		BaseForm* Component::GetOwnerForm()
		{
			HWND chandle = handle;
			while (chandle)
			{
				Component * comp = Application::GetComponent(chandle);
				if (dynamic_cast<BaseForm*>(comp))
					return (BaseForm*)comp;
				else
					chandle = ::GetParent(chandle);
			}
			return 0;
		}

		void Component::BroadcastMessage(WinMessage & msg)
		{
			ProcessMessage(msg);
			for (int i=0; i<children.Count(); i++)
				children[i]->BroadcastMessage(msg);
		}

		void Component::AddChild(Component * comp)
		{
			children.Add(comp);
			comp->SetParent(this);
		}

		void Component::RemoveChild(Component * comp)
		{
			int id = 0;
			for (auto & child : children)
			{
				if (child.Ptr() == comp)
				{
					child = nullptr;
					break;
				}
				id++;
			}
			if (id < children.Count())
				children.RemoveAt(id);
		}

		void Component::SubClass()
		{
			SetWindowSubclass(handle, SubClassWndProc, 0, (DWORD_PTR)this);
		}

		void Component::UnSubClass()
		{
			RemoveWindowSubclass(handle, SubClassWndProc, 0);
		}

		void Component::Destroy()
		{

		}

		int Component::ProcessNotification(WinNotification /*note*/)
		{
			return -1;
		}

		int Component::ProcessMessage(WinMessage & msg)
		{
			UserMessageArgs args;
			if (msg.message >= WM_USER)
			{
				args.Message = msg.message;
				args.LParam = msg.lParam;
				args.WParam = msg.wParam;
				OnUserEvent.Invoke(this, args);
				return 0;
			}
			return -1;
		}

		HWND Component::GetHandle() const
		{
			return handle;
		}

		void Component::ApplyActionToComponentAndChildren(Action * act)
		{
			act->Apply(this);
			for (int i=0; i<children.Count(); i++)
			{
				children[i]->ApplyActionToComponentAndChildren(act);
			}
		}

		void Component::SetParent(const Component * Parent)
		{
			if (::GetParent(handle) != Parent->GetHandle())
				::SetParent(handle, Parent->GetHandle());
		}

		Component * Component::GetParent()
		{
			HWND p = ::GetParent(handle);
			if (p)
			{
				return Application::GetComponent(p);
			}
			else
				return 0;
		}

		int Component::GetChildrenCount()
		{
			return children.Count();
		}

		Component * Component::GetChildren(int id)
		{
			return children[id].operator->();
		}

		int BaseControl::ProcessMessage(WinMessage & msg)
		{
			int rs = -1; 
			switch (msg.message)
			{
			case WM_LBUTTONUP:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					OnMouseUp.Invoke(this, MouseEventArgs(mbLeft, mx, my,0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
					RECT rect;
					GetWindowRect(handle, &rect);
					if (captureClick && mouseDown && mx >= 0 && mx <= rect.right-rect.left && my >= 0 && my <= rect.bottom - rect.top)
					{
						OnClick.Invoke(this, EventArgs());
					}
					mouseDown = false;
				}
				break;
			case WM_MBUTTONUP:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					OnMouseUp.Invoke(this, MouseEventArgs(mbMiddle, mx, my,0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
					RECT rect;
					GetWindowRect(handle, &rect);
					if (captureClick && mouseDown && mx >= 0 && mx <= rect.right-rect.left && my >= 0 && my <= rect.bottom - rect.top)
					{
						OnClick.Invoke(this, EventArgs());
					}
					mouseDown = false;
				}
				break;
			case WM_RBUTTONUP:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					OnMouseUp.Invoke(this, MouseEventArgs(mbRight, mx, my,0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
					RECT rect;
					GetWindowRect(handle, &rect);
					if (captureClick && mouseDown && mx >= 0 && mx <= rect.right-rect.left && my >= 0 && my <= rect.bottom - rect.top)
					{
						OnClick.Invoke(this, EventArgs());
					}
					mouseDown = false;
				}
				break;
			case WM_LBUTTONDOWN:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					mouseDown = true;
					OnMouseDown.Invoke(this, MouseEventArgs(mbLeft, mx, my, 0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
				}
				break;
			case WM_MBUTTONDOWN:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					mouseDown = true;
					OnMouseDown.Invoke(this, MouseEventArgs(mbMiddle, mx, my, 0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
				}
				break;
			case WM_RBUTTONDOWN:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					mouseDown = true;
					OnMouseDown.Invoke(this, MouseEventArgs(mbRight, mx, my, 0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
				}
				break;
			case WM_MOUSEMOVE:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					OnMouseMove.Invoke(this, MouseEventArgs(mbNone, mx, my, 0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
				}
				break;
			case WM_MOUSEWHEEL:
				{
					int delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
					bool processed = false;
					OnMouseWheel.Invoke(this, MouseEventArgs(mbNone, 0,0, delta, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
					if (processed)
						rs = 0;
				}
				break;
			case WM_CHAR:
				{ 
					OnKeyPressed.Invoke(this, KeyEventArgs((wchar_t)(msg.wParam), (int*)&msg.wParam, false, false, false));
					if (msg.wParam == 0)
						rs = 0;
				}
				break;
			case WM_KEYDOWN:
				{
					OnKeyDown.Invoke(this, KeyEventArgs(0, (int*)&msg.wParam, false, false, false));
					BaseControl * ctrl = dynamic_cast<BaseControl *>(Application::GetComponent(msg.hWnd));
					if (ctrl)
					{
						WinMessage nmsg(msg.hWnd, gxMsgKeyBroadcast, msg.wParam, msg.lParam);
						BaseForm * f = ctrl->GetOwnerForm();
						if (f)
							f->BroadcastMessage(nmsg);
						else
							ctrl->BroadcastMessage(nmsg);
					}
				}
				break;
			case WM_KEYUP:
				{
					OnKeyUp.Invoke(this, KeyEventArgs(0, (int*)&msg.wParam, false, false, false));
				}
				break;
			case WM_SETFOCUS:
				{
					OnFocus.Invoke(this, EventArgs());
				}
				break;
			case WM_KILLFOCUS:
				{
					OnLostFocus.Invoke(this, EventArgs());
				}
				break;
			case WM_MOUSEHOVER:
				{
					OnMouseEnter.Invoke(this, EventArgs());
				}
				break;
			case WM_MOUSELEAVE:
				{
					OnMouseLeave.Invoke(this, EventArgs());
				}
				break;
			case WM_SIZING:
				{
					ResizingEventArgs args;
					RECT * rect = (RECT *)msg.lParam;
					args.Width = rect->right - rect->left;
					args.Height = rect->bottom - rect->top;
					args.Left = rect->left;
					args.Top = rect->top;
					OnResizing.Invoke(this, args);
					rect->left = args.Left;
					rect->top = args.Top;
					rect->bottom = args.Top + args.Height;
					rect->right = args.Left + args.Width;
				}
				break;
			case WM_SIZE:
				{
					_OnResize();
					OnResized.Invoke(this, EventArgs());
				}
				break;
			case WM_PAINT:
				{
					if (ownerDraw)
					{
						PaintEventArgs args;
						RECT rect;
						int NeedDraw = GetUpdateRect(handle, &rect, true);
						if (NeedDraw)
						{
							PAINTSTRUCT s;
							args.Graphics = new Graphics(BeginPaint(handle, &s));
							args.UpdateRect.Left = rect.left;
							args.UpdateRect.Right = rect.right;
							args.UpdateRect.Bottom = rect.bottom;
							args.UpdateRect.Top = rect.top;
							OnPaint.Invoke(this, args);
							args.Graphics = 0;
							EndPaint(handle, &s);
							ReleaseDC(handle,s.hdc);
						}
						rs = 0;
					}
				}
				break;
			default:
				break;
			}
			if (rs != -1)
				return rs;
			return Component::ProcessMessage(msg);
		}

		int BaseControl::ProcessNotification(WinNotification note)
		{
			switch (note.code)
			{
			case NM_SETFOCUS:
				_Focused();
				break;
			case NM_KILLFOCUS:
				_LostFocus();
				break;
			}
			return -1;
		}

		void BaseControl::_Focused()
		{
			OnFocus.Invoke(this, EventArgs());
		}

		void BaseControl::_LostFocus()
		{
			OnLostFocus.Invoke(this, EventArgs());
		}

		void BaseControl::_OnResize()
		{
		}

		void BaseControl::Destroy()
		{
			if (nativeFont)
				DeleteObject(nativeFont);
			DestroyWindow(handle);
		}

		void BaseControl::SetWidth(int w)
		{
			RECT r;
			GetWindowRect(handle, &r);
			POINT point, point1;
			point.x = r.left;
			point.y = r.top;
			point1.x = r.right;
			point1.y = r.bottom;
			ScreenToClient(::GetParent(handle), &point);
			ScreenToClient(::GetParent(handle), &point1);

			MoveWindow(handle, point.x, point.y, w, point1.y-point.y, true);
		}

		void BaseControl::SetHeight(int h)
		{
			RECT r;
			GetWindowRect(handle, &r);
			POINT point, point1;
			point.x = r.left;
			point.y = r.top;
			point1.x = r.right;
			point1.y = r.bottom;
			ScreenToClient(::GetParent(handle), &point);
			ScreenToClient(::GetParent(handle), &point1);

			MoveWindow(handle, point.x, point.y, point1.x-point.x, h, true);
		}

		void BaseControl::SetLeft(int l)
		{
			RECT r;
			GetWindowRect(handle, &r);
			POINT point, point1;
			point.x = r.left;
			point.y = r.top;
			point1.x = r.right;
			point1.y = r.bottom;
			ScreenToClient(::GetParent(handle), &point);
			ScreenToClient(::GetParent(handle), &point1);

			MoveWindow(handle, l, point.y, point1.x - point.x, point1.y - point.y, true);
		}

		void BaseControl::SetTop(int t)
		{
			RECT r;
			GetWindowRect(handle, &r);
			POINT point, point1;
			point.x = r.left;
			point.y = r.top;
			point1.x = r.right;
			point1.y = r.bottom;
			ScreenToClient(::GetParent(handle), &point);
			ScreenToClient(::GetParent(handle), &point1);

			MoveWindow(handle, point.x, t, point1.x - point.x, point1.y - point.y, true);
		}

		void BaseControl::EnableRedraw()
		{
			SendMessage(handle, WM_SETREDRAW, TRUE, 0);
		}
		void BaseControl::DisableRedraw()
		{
			SendMessage(handle, WM_SETREDRAW, FALSE, 0);
		}
		void BaseControl::Refresh()
		{
			RECT rect;
			GetClientRect(handle, &rect);
			RedrawWindow(handle, &rect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		}
		void BaseControl::Invalidate()
		{
			RECT rect;
			GetClientRect(handle, &rect);
			InvalidateRect(handle, &rect, TRUE);
		}
		void BaseControl::BringToTop()
		{
			BringWindowToTop(handle);
		}
		int BaseControl::GetClientWidth()
		{
			RECT r;
			GetClientRect(handle, &r);
			return r.right - r.left;
		}

		int BaseControl::GetClientHeight()
		{
			RECT r;
			GetClientRect(handle, &r);
			return r.bottom - r.top;
		}

		int BaseControl::GetWidth()
		{
			RECT r;
			GetWindowRect(handle, &r);
			return r.right-r.left;
		}

		int BaseControl::GetHeight()
		{
			RECT r;
			GetWindowRect(handle, &r);
			return r.bottom-r.top;
		}

		int BaseControl::GetLeft()
		{
			RECT r;
			GetWindowRect(handle, &r);
			return r.left;
		}

		int BaseControl::GetTop()
		{
			RECT r;
			GetWindowRect(handle, &r);
			return r.top;
		}

		void BaseControl::SetStyle(int StyleType, unsigned int Style, bool val)
		{
			auto style = GetWindowLongPtr(handle, StyleType);
			if (val)
				style = style | Style;
			else
				style = style & (~(LONG_PTR)Style);
			SetWindowLongPtr(handle, StyleType, style);
		}

		bool BaseControl::GetStyle(int StyleType, unsigned int Style)
		{
			auto style = GetWindowLongPtr(handle, StyleType);
			return ((style & Style) == Style);
		}

		bool BaseControl::GetEnabled()
		{
			return IsWindowEnabled(handle)==TRUE; 
		}
		void BaseControl::SetEnabled(bool val)
		{
			EnableWindow(handle, val);
		}
		bool BaseControl::GetVisible()
		{
			return IsWindowVisible(handle)==TRUE;
		}
		void BaseControl::SetVisible(bool val)
		{
			ShowWindow(handle, val?SW_SHOW:SW_HIDE);
		}
		bool BaseControl::Focused()
		{
			return (GetFocus() == handle);
		}
		void BaseControl::SetFocus()
		{
			::SetFocus(handle);
		}
		void BaseControl::SetFont(Font * f)
		{
			font = f;
			LOGFONT fnt;
			Graphics g(handle);
			f->GetLogFontW(&g, &fnt);
			if (nativeFont)
				DeleteObject(nativeFont);
			nativeFont = CreateFontIndirect(&fnt);
			SendMessage(handle,WM_SETFONT,(WPARAM)nativeFont,TRUE);
		}

		void BaseControl::SetPosition(int x, int y, int w, int h)
		{
			MoveWindow(handle, x,y,w,h, TRUE);
		}

		BaseControl::BaseControl()
			: ownerDraw(false), captureClick(true)
		{
			nativeFont = 0;
		}


		void BaseControl::SetTabStop(bool ts)
		{
			SetStyle(GWL_STYLE, WS_TABSTOP, ts);
		}

		bool BaseControl::GetTabStop()
		{
			return GetStyle(GWL_STYLE, WS_TABSTOP);
		}

		void BaseControl::InitFont()
		{
			auto sysFont = Application::GetSysFont();
			Font * f = new Font(Graphics(handle).GetHDC(), &sysFont);
			SetFont(f);
		}

		void BaseControl::_Click()
		{
			OnClick.Invoke(this, EventArgs());
		}

		void Control::CreateWnd(Component * parent)
		{
			handle = CreateWindowEx(WS_EX_CONTROLPARENT, Application::ControlClassName, 0, WS_CHILD | WS_GROUP | WS_TABSTOP,
				0, 0, 50, 50, parent->GetHandle(), NULL, Application::GetHandle(), NULL);
			ownerDraw = true;
			if (!handle)
			{
				throw "Failed to create window.";
			}
		}

		void Control::Init(Component * parent)
		{
			CreateWnd(parent);
			InitFont();
			Application::RegisterComponent(this);
			SubClass();
			ShowWindow(handle, SW_SHOW); 
			UpdateWindow(handle); 
		}

		int Control::ProcessNotification(WinNotification note)
		{
			return BaseControl::ProcessNotification(note);
		}

		void ScrollPanel::VScroll(int delta)
		{
			RECT crect;
			GetClientRect(handle, &crect);
			if (lastScrollY - delta < 0)
				delta = lastScrollY;
			if (lastScrollY - delta > maxYScroll)
				delta = lastScrollY - maxYScroll;
			if (delta != 0)
			{
				ScrollWindow(handle, 0, delta, NULL, &crect);
				lastScrollY = lastScrollY - delta;
				SCROLLINFO sinfo;
				sinfo.cbSize = sizeof(SCROLLINFO);
				sinfo.fMask = SIF_POS;
				sinfo.nPos = sinfo.nTrackPos = lastScrollY;
				SetScrollInfo(handle, SB_VERT, &sinfo, true);
			}
		}

		void ScrollPanel::HScroll(int delta)
		{
			RECT crect;
			GetClientRect(handle, &crect);
			if (lastScrollX - delta < 0)
				delta = lastScrollX;
			if (lastScrollX - delta > maxXScroll)
				delta = lastScrollX - maxXScroll;
			if (delta != 0)
			{
				ScrollWindow(handle, delta, 0, NULL, &crect);
				lastScrollX = lastScrollX - delta;
				SCROLLINFO sinfo;
				sinfo.cbSize = sizeof(SCROLLINFO);
				sinfo.fMask = SIF_POS;
				sinfo.nPos = sinfo.nTrackPos = lastScrollX;
				SetScrollInfo(handle, SB_HORZ, &sinfo, true);
			}
		}

		int ScrollPanel::ProcessMessage(WinMessage & msg)
		{
			int rs = -1;
			switch (msg.message)
			{
			case WM_VSCROLL:
			{
				RECT crect;
				GetClientRect(handle, &crect);
				int delta = 0;
				if (LOWORD(msg.wParam) == SB_THUMBTRACK || LOWORD(msg.wParam) == SB_THUMBPOSITION)
				{
					delta = lastScrollY-HIWORD(msg.wParam);
				}
				else if (LOWORD(msg.wParam) == SB_LINEDOWN)
				{
					delta = -30;
				}
				else if (LOWORD(msg.wParam) == SB_LINEUP)
				{
					delta = 30;
				}
				else if (LOWORD(msg.wParam) == SB_PAGEDOWN)
					delta = -(crect.bottom - crect.top);
				else if (LOWORD(msg.wParam) == SB_PAGEUP)
					delta = (crect.bottom - crect.top);
				VScroll(delta);
				rs = 0;
				break;
			}
			case WM_HSCROLL:
			{
				RECT crect;
				GetClientRect(handle, &crect);
				int delta = 0;
				if (LOWORD(msg.wParam) == SB_THUMBTRACK || LOWORD(msg.wParam) == SB_THUMBPOSITION)
				{
					delta = lastScrollX - HIWORD(msg.wParam);
				}
				else if (LOWORD(msg.wParam) == SB_LINELEFT)
				{
					delta = -30;
				}
				else if (LOWORD(msg.wParam) == SB_LINERIGHT)
				{
					delta = 30;
				}
				else if (LOWORD(msg.wParam) == SB_PAGELEFT)
					delta = -(crect.right - crect.left);
				else if (LOWORD(msg.wParam) == SB_PAGERIGHT)
					delta = (crect.right - crect.left);
				HScroll(delta);
				break;
			}
			case WM_MOUSEWHEEL:
			{
				auto zDelta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
				UINT lines;
				SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
				if (zDelta < 0)
					SetVerticalScrollPosition(GetVerticalScrollPosition() + lines * 25);
				else
					SetVerticalScrollPosition(GetVerticalScrollPosition() - lines * 25);
				break;
			}
			}
			if (rs == -1)
				return BaseControl::ProcessMessage(msg);
			else
				return rs;
		}

		void ScrollPanel::SetVerticalScrollPosition(int val)
		{
			SCROLLINFO sinfo;
			sinfo.cbSize = sizeof(SCROLLINFO);
			sinfo.fMask = SIF_POS | SIF_RANGE;
			GetScrollInfo(handle, SB_VERT, &sinfo);
			sinfo.nPos = Math::Clamp(val, sinfo.nMin, sinfo.nMax);
			VScroll(lastScrollY - sinfo.nPos);
		}

		int ScrollPanel::GetVerticalScrollPosition()
		{
			SCROLLINFO sinfo;
			sinfo.cbSize = sizeof(SCROLLINFO);
			sinfo.fMask = SIF_POS | SIF_RANGE;
			GetScrollInfo(handle, SB_VERT, &sinfo);
			return sinfo.nPos;
		}

		void ScrollPanel::_OnResize()
		{
			int bboxMaxY = -(1<<30);
			int bboxMaxX = -(1<<30);
			int bboxMinY = hiddenCtrl ? hiddenCtrl->GetTop() : 0;
			int bboxMinX = hiddenCtrl ? hiddenCtrl->GetLeft() : 0;
			for (auto & child : children)
			{
				if (auto ctrl = dynamic_cast<Control*>(child.Ptr()))
				{
					bboxMaxY = Math::Max(bboxMaxY, ctrl->GetTop() + ctrl->GetHeight());
					bboxMaxX = Math::Max(bboxMaxX, ctrl->GetLeft() + ctrl->GetWidth());
				}
			}

			RECT crect;
			GetClientRect(handle, &crect);
			if (scrollBars == ScrollBars::Both || scrollBars == ScrollBars::Vertical)
			{
				SCROLLINFO sinfo;
				sinfo.cbSize = sizeof(SCROLLINFO);
				sinfo.fMask = SIF_PAGE | SIF_RANGE;
				sinfo.nMin = 0;
				int clientHeight = crect.bottom - crect.top;
				maxYScroll = Math::Max(0, bboxMaxY - bboxMinY);
				sinfo.nMax = maxYScroll;
				sinfo.nPage = clientHeight;
				SetScrollInfo(handle, SB_VERT, &sinfo, true);
			}

			if (scrollBars == ScrollBars::Both || scrollBars == ScrollBars::Horizontal)
			{
				SCROLLINFO sinfo;
				sinfo.cbSize = sizeof(SCROLLINFO);
				sinfo.fMask = SIF_PAGE | SIF_RANGE;
				sinfo.nMin = 0;
				int clientWidth = crect.right - crect.left;
				maxXScroll = Math::Max(0, bboxMaxX - bboxMinX);
				sinfo.nMax = maxXScroll;
				sinfo.nPage = clientWidth;
				SetScrollInfo(handle, SB_HORZ, &sinfo, true);
			}
		}

		ScrollPanel::ScrollPanel()
		{
		}

		ScrollPanel::ScrollPanel(Component * parent)
		{
			Init(parent);
			hiddenCtrl = new Control(this);
			hiddenCtrl->SetPosition(0, 0, 0, 0);
		}

		void Control::Destroy()
		{
			UnSubClass();
			Application::UnRegisterComponent(this);
			BaseControl::Destroy();
		}

		void ScrollPanel::SetScrollBar(ScrollBars scroll)
		{
			scrollBars = scroll;
			if (scroll == ScrollBars::None)
				ShowScrollBar(handle, (int)ScrollBars::Both, false);
			else
				ShowScrollBar(handle, (int)scroll, true);
		}

		void ScrollPanel::UpdateScrollBar()
		{
			_OnResize();
		}

		Control::Control(Component * parent)
		{
			Init(parent);
		}

		Control::Control()
		{
		}

		Control::~Control()
		{
			Destroy();
		}
	}
}