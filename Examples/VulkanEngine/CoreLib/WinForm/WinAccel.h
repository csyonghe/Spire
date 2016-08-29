#ifndef GX_WINACCEL_H
#define GX_WINACCEL_H

#include "../Basic.h"
#include <windows.h>

namespace CoreLib
{
	namespace WinForm
	{
		using namespace CoreLib;
		using namespace CoreLib::Basic;

		class MenuItem;

		class Accelerator : public Object
		{
			friend class AccelTable;
		public:
			typedef unsigned char AccelAuxKey;
			static const unsigned char Ctrl = FCONTROL;
			static const unsigned char Shift = FSHIFT;
			static const unsigned char Alt = FALT;
		private:
			ACCEL accel;
		public:
			Accelerator(AccelAuxKey auxKey, Word mainKey);
			Accelerator();
		};

		class AccelTable : public Object
		{
		private:
			HACCEL handle;
			List<ACCEL> accels;
		public:
			AccelTable();
			~AccelTable();
		public:
			HACCEL GetHandle();
			void RegisterAccel(Accelerator acc, MenuItem * item);
			void UpdateAccel(Accelerator acc, MenuItem * item);
			void Update();
		};
	}
}

#endif