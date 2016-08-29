#include "Camera.h"
#ifdef _WIN32
#include <Windows.h>
#endif
using namespace VectorMath;

namespace CoreLib
{
	namespace Graphics
	{
		Camera::Camera()
		{
			Reset();
			CanFly = true;
		}

		void Camera::GetInverseRotationMatrix(float mat[9])
		{
			Vec3 left;
			Vec3::Cross(left, dir, up);
			Vec3::Normalize(left, left);
			mat[0] = left.x; mat[1] = up.x; mat[2] = -dir.x;
			mat[3] = left.y; mat[4] = up.y; mat[5] = -dir.y;
			mat[6] = left.z; mat[7] = up.z; mat[8] = -dir.z;
		}

		void Camera::Reset()
		{
			alpha = (float)PI;
			beta = 0.0f;
			pos = Vec3::Create(0.0f,0.0f,0.0f);
			up = Vec3::Create(0.0f,1.0f,0.0f);
			dir = Vec3::Create(0.0f,0.0f,-1.0f);
		}

		void Camera::GoForward(float u)
		{
			Vec3 vp;
			if (CanFly) 
			{
				pos += dir*u;
			}
			else
			{
				vp.x = sin(alpha);
				vp.z = cos(alpha);
				pos += vp*u;
			}
		}

		void Camera::MoveLeft(float u)
		{
			Vec3 l, vp;
			vp.x = sin(alpha);
			vp.z = cos(alpha);
			l.x=vp.z;	l.y=0;	l.z=-vp.x;
			pos += l*u;
		}

		void Camera::TurnLeft(float u)
		{
			alpha += u;
		}

		void Camera::TurnUp(float u)
		{
			beta += u;
			if (beta > (float)PI/2)
				beta=(float)PI/2;
			if (beta < (float)-PI/2)
				beta=-(float)PI/2;
		}

		void Camera::GetTransform(Matrix4 & rs)
		{
			dir = Vec3::Create((float)sin(alpha)*cos(beta),
					   (float)sin(beta),
					   (float)cos(alpha)*cos(beta));
			up = Vec3::Create((float)sin(alpha)*cos(PI/2+beta),
					  (float)sin(PI/2+beta),
					  (float)cos(alpha)*(float)cos(PI/2+beta));
			ViewFrustum view;
			GetView(view);
			rs = view.GetViewTransform();
		}

		void Camera::GetView(ViewFrustum & view)
		{
			dir = Vec3::Create((float)sin(alpha)*cos(beta),
				(float)sin(beta),
				(float)cos(alpha)*cos(beta));
			up = Vec3::Create((float)sin(alpha)*cos(PI / 2 + beta),
				(float)sin(PI / 2 + beta),
				(float)cos(alpha)*(float)cos(PI / 2 + beta));
			view.CamPos = pos;
			view.CamDir = dir;
			view.CamUp = up;
		}

#ifdef _WIN32
		void CameraController::HandleCameraKeys(Camera & camera, Matrix4 & transform, float dtime, float /*minSpeed*/, float maxSpeed, bool flipYZ)
		{
			const float CameraMaxSpeed = maxSpeed;
			const float CameraAcceleration = CameraMaxSpeed;
			const float CameraTurnAngle = 3.14159f/4.0f;

			static Vec3 speed = Vec3::Create(0.0f, 0.0f, 0.0f);
			Vec3 force;
			Vec3 left;
			force.SetZero();
			left.x=cos(camera.alpha);	left.y=0;	left.z=-sin(camera.alpha);
			if (GetAsyncKeyState(L'W') != 0)
			{
				force += camera.dir;
			}
			else if (GetAsyncKeyState(L'S') != 0 && GetAsyncKeyState(VK_CONTROL) == 0)
			{
				force -= camera.dir;
			}
			if (GetAsyncKeyState(L'A') != 0)
			{
				force += left;
			}
			else if (GetAsyncKeyState(L'D') != 0)
			{
				force -= left;
			}
			float forceLen = force.Length();
			if (forceLen > 0.0f)
			{
				force *= 1.0f/forceLen;
			}
			float accelLen = CameraAcceleration * dtime;
			float spdLen = speed.Length2();
			if (spdLen < accelLen * accelLen * 16.0f)
			{
				speed = Vec3::Create(0.0f,0.0f,0.0f);
			}
			else if (spdLen>0.0f)
			{
				Vec3 spdDir;
				Vec3::Normalize(spdDir, speed);
				speed -= spdDir * accelLen * 4.0f;
			}
				
			speed += force * accelLen * 5.0f;
			spdLen = speed.Length2();

			if (spdLen > CameraMaxSpeed*CameraMaxSpeed)
			{
				Vec3::Normalize(speed, speed);
				speed *= CameraMaxSpeed;
			}

			if (GetAsyncKeyState(VK_SHIFT))
				dtime *= 0.1f;

			float lturn = 0.0f;
			float uturn = 0.0f;
			if (GetAsyncKeyState(VK_LEFT))
			{
				lturn = CameraTurnAngle * dtime;
			}
			if (GetAsyncKeyState(VK_RIGHT))
			{
				lturn = -CameraTurnAngle * dtime;
			}

			if (GetAsyncKeyState(VK_UP))
			{
				uturn = CameraTurnAngle * dtime;
			}
			if (GetAsyncKeyState(VK_DOWN))
			{
				uturn = -CameraTurnAngle * dtime;
			}
			camera.pos += speed*dtime;
			camera.TurnLeft(lturn);
			camera.TurnUp(uturn);
			camera.GetTransform(transform);
			if (flipYZ)
			{
				Matrix4 flip;
				Matrix4::CreateIdentityMatrix(flip);
				flip.m[1][1] = 0.0f;
				flip.m[1][2] = -1.0f;
				flip.m[2][1] = 1.0f;
				flip.m[2][2] = 0.0f;
				Matrix4 flipped;
				Matrix4::Multiply(flipped, transform, flip);
				transform = flipped;
			}
		}
#endif
	}
}