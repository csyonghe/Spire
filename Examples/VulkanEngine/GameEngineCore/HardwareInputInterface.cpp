#include "HardwareInputInterface.h"
#include "CoreLib/Basic.h"
#include <Windows.h>

namespace GameEngine
{
	using namespace CoreLib::Basic;

	class WindowsHardwareInputInterface : public HardwareInputInterface
	{
	private:
		List<bool> lastKeyState;
		HWND hwnd;
	public:
		WindowsHardwareInputInterface(WindowHandle window)
		{
			hwnd = (HWND)window;
			lastKeyState.SetSize(255);
			for (int i = 0; i < lastKeyState.Count(); i++)
				lastKeyState[i] = 0;
		}
		virtual KeyStateQueryResult QueryKeyState(wchar_t key) override
		{
			KeyStateQueryResult rs;
			rs.IsDown = (GetAsyncKeyState(key) != 0);
			rs.HasPressed = rs.IsDown && !lastKeyState[key];
			lastKeyState[key] = rs.IsDown;
			return rs;
		}

		virtual void QueryCursorPosition(int & x, int & y) override
		{
			POINT point;
			GetCursorPos(&point);
			ScreenToClient(hwnd, &point);
			x = point.x;
			y = point.y;
		}

		virtual void SetCursorPosition(int x, int y) override
		{
			POINT point;
			point.x = x;
			point.y = y;
			ClientToScreen(hwnd, &point);
			SetCursorPos(point.x, point.y);
		}

		virtual void SetCursorVisiblity(bool visible) override
		{
			ShowCursor(visible);
		}

	};

	HardwareInputInterface * CreateHardwareInputInterface(WindowHandle window)
	{
		return new WindowsHardwareInputInterface(window);
	}
}