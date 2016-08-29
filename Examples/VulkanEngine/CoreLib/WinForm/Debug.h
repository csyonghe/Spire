#ifndef GX_WIN_DEBUG_H
#define GX_WIN_DEBUG_H

#include "../Basic.h"
#include <Windows.h>

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