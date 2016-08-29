#ifndef CORELIB_PERFORMANCE_COUNTER_H
#define CORELIB_PERFORMANCE_COUNTER_H

#include "Common.h"
#include <chrono>

namespace CoreLib
{
	namespace Diagnostics
	{
		typedef std::chrono::high_resolution_clock::time_point TimePoint;
		typedef std::chrono::high_resolution_clock::duration Duration;
		class PerformanceCounter
		{
		public:
			static TimePoint Start();
			static Duration End(TimePoint counter);
			static float EndSeconds(TimePoint counter);
			static double ToSeconds(Duration duration);
		};
	}
}

#endif