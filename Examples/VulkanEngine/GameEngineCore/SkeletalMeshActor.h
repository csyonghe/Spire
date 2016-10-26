#ifndef GAME_ENGINE_SKELETAL_ANIMATED_ACTOR_H
#define GAME_ENGINE_SKELETAL_ANIMATED_ACTOR_H

#include "Actor.h"

namespace GameEngine
{
	class SkeletalMeshActor : public Actor
	{
	private:
		Pose nextPose;
		float startTime = 0.0f;
	protected:
		virtual bool ParseField(Level * level, CoreLib::Text::Parser & parser, bool &isInvalid) override;
	public:
		CoreLib::RefPtr<AnimationSynthesizer> Animation;
		Mesh * Mesh = nullptr;
		Skeleton * Skeleton = nullptr;
		SkeletalAnimation * SimpleAnimation = nullptr;
		CoreLib::String MeshName, SkeletonName, SimpleAnimationName;
		Material * MaterialInstance = nullptr;
		CoreLib::RefPtr<SkeletalMeshRenderContext> RenderContext;
		virtual void Tick() override;
		Pose & GetCurrentPose()
		{
			return nextPose;
		}
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::SkeletalMesh;
		}
		virtual CoreLib::String GetTypeName() override
		{
			return L"SkeletalMesh";
		}
		virtual void OnLoad() override;
	};
}

#endif