#include "WinAccel.h"
#include "WinMenu.h"
namespace CoreLib
{
	namespace WinForm
	{
		Accelerator::Accelerator()
		{
			memset(&accel, 0, sizeof(ACCEL));
		}

		Accelerator::Accelerator(AccelAuxKey auxKey, Word mainKey)
		{
			accel.fVirt = auxKey +  FVIRTKEY;
			accel.key = mainKey;
		}

		AccelTable::AccelTable()
		{
			handle = 0;
		}

		AccelTable::~AccelTable()
		{
			if (handle)
				DestroyAcceleratorTable(handle);
		}

		HACCEL AccelTable::GetHandle()
		{
			return handle;
		}

		
		void AccelTable::UpdateAccel(Accelerator acc, MenuItem * item)
		{
			int fid = -1;
			for (int i=0; i<accels.Count(); i++)
			{
				if (accels[i].fVirt == acc.accel.fVirt && accels[i].key == acc.accel.key)
				{
					fid = i;
					break;
				}
			}
			acc.accel.cmd = (WORD)item->GetIdentifier();
			if (fid != -1)
			{
				accels[fid] = acc.accel;
			}
			else
			{
				accels.Add(acc.accel);
			}
		}

		void AccelTable::RegisterAccel(Accelerator acc, MenuItem * item)
		{
			bool find = false;
			for (int i=0; i<accels.Count(); i++)
			{
				if (accels[i].fVirt == acc.accel.fVirt && accels[i].key == acc.accel.key)
				{
					find = true;
					break;
				}
			}
			if (find)
			{
				throw L"Accelerator confliction.";
			}
			acc.accel.cmd = (WORD)item->GetIdentifier();
			accels.Add(acc.accel);
		}

		void AccelTable::Update()
		{
			if (handle)
				DestroyAcceleratorTable(handle);
			handle = CreateAcceleratorTable(accels.Buffer(), accels.Count());
		}
	}
}