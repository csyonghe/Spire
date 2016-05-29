#ifndef REALTIME_ENGINE_CAMERA_CONTROL_H
#define REALTIME_ENGINE_CAMERA_CONTROL_H

#include "CoreLib/Graphics/Camera.h"
#include "CoreLib/VectorMath.h"

namespace RealtimeEngine
{
	using namespace VectorMath;

	class CameraControl
	{
	public:
		CoreLib::Graphics::Camera Cam;
		float MaxSpeed;
		float Acceleration;
		float TurnSpeed;
		void Reset()
		{
			Cam.Reset();
			MaxSpeed = 10.0f;
			Acceleration = 50.0f;
			TurnSpeed = 3.14159f / 4.0f;
		}
		Matrix4 GetViewMatrix()
		{
			Matrix4 rs;
			Cam.GetTransform(rs);
			return rs;
		}
		void HandleKeys(float dtime);
	};
}

#endif