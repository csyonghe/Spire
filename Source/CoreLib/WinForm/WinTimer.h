#ifndef GX_WIN_TIMER_H
#define GX_WIN_TIMER_H

#include "WinMessage.h"
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