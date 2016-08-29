#include "CameraControl.h"
#include <Windows.h>

namespace DemoEngine
{
	void CameraControl::HandleKeys(float dtime)
	{
		static Vec3 speed = Vec3::Create(0.0f, 0.0f, 0.0f);
		Vec3 force;
		Vec3 left;
		force.SetZero();
		left.x = cos(Cam.alpha);	left.y = 0;	left.z = -sin(Cam.alpha);
		if (GetAsyncKeyState(L'W') < 0)
		{
			force += Cam.dir;
		}
		else if (GetAsyncKeyState(L'S') < 0 && GetAsyncKeyState(VK_CONTROL) == 0)
		{
			force -= Cam.dir;
		}
		if (GetAsyncKeyState(L'A') < 0)
		{
			force += left;
		}
		else if (GetAsyncKeyState(L'D') < 0)
		{
			force -= left;
		}
		float forceLen = force.Length();
		if (forceLen > 0.0f)
		{
			force *= 1.0f / forceLen;
		}
		float accelLen = Acceleration * dtime;
		float spdLen = speed.Length2();
		if (spdLen < accelLen * accelLen * 16.0f)
		{
			speed = Vec3::Create(0.0f, 0.0f, 0.0f);
		}
		else if (spdLen>0.0f)
		{
			Vec3 spdDir;
			Vec3::Normalize(spdDir, speed);
			speed -= spdDir * accelLen * 4.0f;
		}

		speed += force * accelLen * 5.0f;
		spdLen = speed.Length2();

		if (spdLen > MaxSpeed*MaxSpeed)
		{
			Vec3::Normalize(speed, speed);
			speed *= MaxSpeed;
		}

		if (GetAsyncKeyState(VK_SHIFT) < 0)
			dtime *= 0.1f;

		float lturn = 0.0f;
		float uturn = 0.0f;
		if (GetAsyncKeyState(VK_LEFT) < 0)
		{
			lturn = TurnSpeed * dtime;
		}
		if (GetAsyncKeyState(VK_RIGHT) < 0)
		{
			lturn = -TurnSpeed * dtime;
		}

		if (GetAsyncKeyState(VK_UP) < 0)
		{
			uturn = TurnSpeed * dtime;
		}
		if (GetAsyncKeyState(VK_DOWN) < 0)
		{
			uturn = -TurnSpeed * dtime;
		}

		Cam.pos += speed*dtime;
		Cam.TurnLeft(lturn);
		Cam.TurnUp(uturn);

	}
}