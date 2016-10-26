#include "SkeletalMeshActor.h"
#include "Engine.h"

namespace GameEngine
{
	bool SkeletalMeshActor::ParseField(Level * level, CoreLib::Text::Parser & parser, bool &isInvalid)
	{
		if (Actor::ParseField(level, parser, isInvalid))
			return true;
		if (parser.LookAhead(L"mesh"))
		{
			parser.ReadToken();
			MeshName = parser.ReadStringLiteral();
			Mesh = level->LoadMesh(MeshName);
			if (!Mesh)
				isInvalid = false;
			return true;
		}
		if (parser.LookAhead(L"material"))
		{
			if (parser.NextToken(1).Str == L"{")
			{
				MaterialInstance = level->CreateNewMaterial();
				MaterialInstance->Parse(parser);
			}
			else
			{
				parser.ReadToken();
				auto materialName = parser.ReadStringLiteral();
				MaterialInstance = level->LoadMaterial(materialName);
				if (!MaterialInstance)
					isInvalid = true;
			}
			return true;
		}
		if (parser.LookAhead(L"Skeleton"))
		{
			parser.ReadToken();
			SkeletonName = parser.ReadStringLiteral();
			Skeleton = level->LoadSkeleton(SkeletonName);
			if (!Skeleton)
				isInvalid = true;
			return true;
		}
		if (parser.LookAhead(L"SimpleAnimation"))
		{
			parser.ReadToken();
			SimpleAnimationName = parser.ReadStringLiteral();
			SimpleAnimation = level->LoadSkeletalAnimation(SimpleAnimationName);
			if (!SimpleAnimation)
				isInvalid = true;
			return true;
		}
		return false;
	}
	void GameEngine::SkeletalMeshActor::Tick()
	{
		auto time = Engine::Instance()->GetCurrentTime();
		if (Animation)
			Animation->GetPose(nextPose, time);
	}
	void SkeletalMeshActor::OnLoad()
	{
		if (SimpleAnimation)
			Animation = new SimpleAnimationSynthesizer(Skeleton, SimpleAnimation);

		Tick();
	}
}