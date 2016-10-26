#ifndef GAME_ENGINE_CAMERA_H
#define GAME_ENGINE_CAMERA_H

#include "CoreLib/VectorMath.h"
#include "Actor.h"

namespace GameEngine
{
	class CameraActor : public Actor
	{
	private:
		float yaw = 0.0f, pitch = 0.0f, roll = 0.0f;
		float collisionRadius = 50.0f;
		VectorMath::Vec3 position;
	public:
		float ZNear = 40.0f, ZFar = 400000.0f;
		float FOV = 75.0f;
	protected:
		virtual bool ParseField(Level * level, CoreLib::Text::Parser & parser, bool & isInvalid) override;
	public:
		CameraActor()
		{
			position.SetZero();
		}
		VectorMath::Vec3 GetPosition()
		{
			return position;
		}
		void SetPosition(const VectorMath::Vec3 & value);
		float GetYaw() { return yaw; }
		float GetPitch() { return pitch; }
		float GetRoll() { return roll; }
		void SetYaw(float value);
		void SetPitch(float value);
		void SetRoll(float value);
		void SetOrientation(float pYaw, float pPitch, float pRoll);
		float GetCollisionRadius() { return collisionRadius; }
		void SetCollisionRadius(float value);
		virtual void Tick() override { }
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::Camera;
		}
		virtual CoreLib::String GetTypeName() override
		{
			return L"Camera";
		}
	};
}

#endif