#ifndef CORE_LIB_THREADING_H
#define CORE_LIB_THREADING_H
#include <atomic>
#include <thread>
#include <mutex>
#include <xmmintrin.h>
#include "Basic.h"
#include "Events.h"

#ifndef _WIN32
#define __stdcall
#endif

namespace CoreLib
{
	namespace Threading
	{
		class SpinLock
		{
		private:
			std::atomic_flag lck;
		public:
			SpinLock()
			{
				lck.clear();
			}
			inline bool TryLock()
			{
				return !lck.test_and_set(std::memory_order_acquire);
			}
			inline void Lock()
			{
				while (lck.test_and_set(std::memory_order_acquire))
				{
				}
			}
			inline void Unlock()
			{
				lck.clear(std::memory_order_release);
			}
			SpinLock & operator = (const SpinLock & /*other*/)
			{
				lck.clear();
				return *this;
			}
		};

		class ParallelSystemInfo
		{
		public:
			static int GetProcessorCount();
		};

		typedef CoreLib::Basic::Event<> ThreadProc;
		typedef CoreLib::Basic::Event<CoreLib::Basic::Object *> ThreadParameterizedProc;
		class Thread;

		class ThreadParam
		{
		public:
			Thread * thread;
			CoreLib::Basic::Object * threadParam;
		};

		enum class ThreadPriority
		{
			Normal,
			AboveNormal,
			Highest,
			Critical,
			BelowNormal,
			Lowest,
			Idle
		};
		unsigned int __stdcall ThreadProcedure(const ThreadParam& param);
		class Thread : public CoreLib::Basic::Object
		{
			friend unsigned int __stdcall ThreadProcedure(const ThreadParam& param);
		private:
			 ThreadParam internalParam;
		public:
			
		private:
			std::thread threadHandle;
			CoreLib::Basic::RefPtr<ThreadProc> threadProc;
			CoreLib::Basic::RefPtr<ThreadParameterizedProc> paramedThreadProc;
		public:
			Thread()
			{
				internalParam.threadParam = nullptr;
				internalParam.thread = this;
			}
			Thread(ThreadProc * p)
				: Thread()
			{
				Start(p);
			}
			Thread(ThreadParameterizedProc * p, CoreLib::Basic::Object * param)
				: Thread()
			{
				Start(p, param);
			}
			void Start(ThreadProc * p)
			{
				threadProc = p;
				threadHandle = std::thread(ThreadProcedure, internalParam);
			}
			void Start(ThreadParameterizedProc * p, CoreLib::Basic::Object * param)
			{
				paramedThreadProc = p;
				internalParam.thread = this;
				internalParam.threadParam = param;
				threadHandle = std::thread(ThreadProcedure, internalParam);
			}
			void Join()
			{
				if (threadHandle.joinable())
					threadHandle.join();
			}
			void Detach()
			{
				if (threadHandle.joinable())
					threadHandle.detach();
			}
			std::thread::id GetHandle()
			{
				return threadHandle.get_id();
			}
		};

		class Mutex : public CoreLib::Basic::Object
		{
		private:
			std::mutex handle;
		public:
			void Lock()
			{
				handle.lock();
			}
			bool TryLock()
			{
				return handle.try_lock();
			}
			void Unlock()
			{
				return handle.unlock();
			}
		};
	}
}

#endif
