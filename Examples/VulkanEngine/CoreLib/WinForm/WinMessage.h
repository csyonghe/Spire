#ifndef GX_WINMESSAGE_H
#define GX_WINMESSAGE_H
#include <Windows.h>
#include "../Basic.h"
#define   ULONG_PTR   void*
#pragma warning(push)
#pragma warning(disable:4458)
#pragma comment(lib, "gdiplus.lib")
#include <Unknwn.h>
#include <gdiplus.h>
#pragma warning(pop)
#include "../Events.h"

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