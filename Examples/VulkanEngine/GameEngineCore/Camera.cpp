#include "Camera.h"

namespace GameEngine
{
	using namespace VectorMath;

	void UpdatePosition(Matrix4 & rs, const Vec3 & pos)
	{
		rs.values[12] = -(rs.values[0] * pos.x + rs.values[4] * pos.y + rs.values[8] * pos.z);
		rs.values[13] = -(rs.values[1] * pos.x + rs.values[5] * pos.y + rs.values[9] * pos.z);
		rs.values[14] = -(rs.values[2] * pos.x + rs.values[6] * pos.y + rs.values[10] * pos.z);
	}

	void TransformFromCamera(Matrix4 & rs, float yaw, float pitch, float roll, const Vec3 & pos)
	{
		Matrix4 m0, m1, m2, m3;
		Matrix4::RotationY(m0, yaw);
		Matrix4::RotationX(m1, pitch);
		Matrix4::RotationZ(m2, roll);
		Matrix4::Multiply(m3, m2, m1);
		Matrix4::Multiply(rs, m3, m0);

		UpdatePosition(rs, pos);
		rs.values[15] = 1.0f;
	}

	void CameraActor::SetPosition(const VectorMath::Vec3 & value)
	{
		position = value;
		UpdatePosition(LocalTransform, value);
	}

	void CameraActor::SetYaw(float value)
	{
		yaw = value;
		TransformFromCamera(LocalTransform, yaw, pitch, roll, position);
	}
	void CameraActor::SetPitch(float value)
	{
		pitch = value;
		TransformFromCamera(LocalTransform, yaw, pitch, roll, position);
	}
	void CameraActor::SetRoll(float value)
	{
		roll = value;
		TransformFromCamera(LocalTransform, yaw, pitch, roll, position);
	}
	void CameraActor::SetOrientation(float pYaw, float pPitch, float pRoll)
	{
		yaw = pYaw;
		pitch = pPitch;
		roll = pRoll;
		TransformFromCamera(LocalTransform, yaw, pitch, roll, position);
	}
	void CameraActor::SetCollisionRadius(float value)
	{
		collisionRadius = value;
	}
	bool CameraActor::ParseField(Level * level, CoreLib::Text::Parser & parser, bool & isInvalid)
	{
		if (Actor::ParseField(level, parser, isInvalid))
			return true;
		if (parser.LookAhead(L"position"))
		{
			parser.ReadToken();
			position = ParseVec3(parser);
			TransformFromCamera(LocalTransform, yaw, pitch, roll, position);
			return true;
		}
		if (parser.LookAhead(L"orientation"))
		{
			parser.ReadToken();
			auto orientation = ParseVec3(parser);
			yaw = orientation.x;
			pitch = orientation.y;
			roll = orientation.z;
			TransformFromCamera(LocalTransform, yaw, pitch, roll, position);
			return true;
		}
		if (parser.LookAhead(L"znear"))
		{
			parser.ReadToken();
			ZNear = (float)parser.ReadDouble();
			return true;
		}
		if (parser.LookAhead(L"zfar"))
		{
			parser.ReadToken();
			ZFar = (float)parser.ReadDouble();
			return true;
		}
		if (parser.LookAhead(L"fov"))
		{
			parser.ReadToken();
			FOV = (float)parser.ReadDouble();
			return true;
		}
		if (parser.LookAhead(L"radius"))
		{
			parser.ReadToken();
			collisionRadius = (float)parser.ReadDouble();
			return true;
		}
		return false;
	}
}
