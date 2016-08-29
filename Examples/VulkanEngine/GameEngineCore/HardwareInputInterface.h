#ifndef GAME_ENGINE_HARDWARE_INPUT_INTERFACE
#define GAME_ENGINE_HARDWARE_INPUT_INTERFACE

#include "CoreLib/Basic.h"
#include "Common.h"

namespace GameEngine
{
	namespace SpecialKeys
	{
		const wchar_t MouseLeftButton = 1;
		const wchar_t MouseRightButton = 2;
		const wchar_t MouseMiddleButton = 4;
		const wchar_t Control = 0x11;
		const wchar_t Space = 0x20;
		const wchar_t Escape = 0x1B;
		const wchar_t Enter = 0x0D;
		const wchar_t LeftArrow = 0x25;
		const wchar_t UpArrow = 0x26;
		const wchar_t RightArrow = 0x27;
		const wchar_t DownArrow = 0x28;
		const wchar_t Shift = 0x10;
	}

	class KeyStateQueryResult
	{
	public:
		bool IsDown;
		bool HasPressed;
	};

	class HardwareInputInterface : public CoreLib::Object
	{
	public:
		virtual KeyStateQueryResult QueryKeyState(wchar_t key) = 0;
		virtual void QueryCursorPosition(int & x, int & y) = 0;
		virtual void SetCursorPosition(int x, int y) = 0;
		virtual void SetCursorVisiblity(bool visible) = 0;
	};

	HardwareInputInterface * CreateHardwareInputInterface(WindowHandle window);
}

#endif