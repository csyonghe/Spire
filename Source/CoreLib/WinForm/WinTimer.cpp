#include "WinTimer.h"
#include "WinApp.h"
namespace CoreLib
{
	namespace WinForm
	{
		void CALLBACK TimerFunc(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR idEvent, DWORD /*dwTime*/)
		{
			Timer * timer = ((Timer *)Application::GetObjectFromHandle((int)idEvent));
			if (timer)
				timer->OnTick.Invoke(timer, EventArgs());
		}

		Timer::Timer()
		{
			Interval = 100;
			timerHandle = (UINT_PTR)this;
		}

		Timer::~Timer()
		{
			KillTimer(0, timerHandle);
			Application::UnRegisterHandle(timerHandle);
		}

		void Timer::StartTimer()
		{
			timerHandle = SetTimer(0, timerHandle, Interval, TimerFunc);
			Application::RegisterObjectHandle(timerHandle, this);
		}
	
		void Timer::StopTimer()
		{
		
			KillTimer(0, timerHandle);
		}
	}
}