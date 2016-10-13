#include "WinMessage.h"
namespace CoreLib
{
	namespace WinForm
	{
		WinMessage::WinMessage(HWND _hWnd, UINT _message, WPARAM _wParam, LPARAM _lParam)
		{
			hWnd = _hWnd;
			message = _message;
			wParam = _wParam;
			lParam = _lParam;
		}

		MouseEventArgs::MouseEventArgs()
			:Button(mbNone), X(0), Y(0), Delta(0)
		{
		}
	
		MouseEventArgs::MouseEventArgs(WinForm::MouseButton btn, int _x, int _y, int _delta, unsigned int buttons, bool * processed)
			:Button(btn), X(_x), Y(_y), Delta(_delta)
		{
			BaseControl = (buttons & MK_CONTROL) == MK_CONTROL;
			Shift = (buttons & MK_SHIFT) == MK_SHIFT;
			LButton = mbLeft?true:false;
			RButton = mbRight?true:false;
			MButton = mbMiddle?true:false;
			Processed = processed;
		}

		KeyEventArgs::KeyEventArgs(wchar_t key, int * keycode, bool ctrl, bool shift, bool alt)
			:Key(key), KeyCode(*keycode), KeyCodeRef(keycode), BaseControl(ctrl), Shift(shift), Alt(alt)
		{
		}
	}
}