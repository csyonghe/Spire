#include "Engine.h"
#include "Level.h"
#include "CoreLib/LibIO.h"
#include "CoreLib/Parser.h"

namespace GameEngine
{
	using namespace CoreLib;
	using namespace CoreLib::IO;

	Level::Level(const CoreLib::String & fileName)
	{
		Text::Parser parser(File::ReadAllText(fileName));
		while (!parser.IsEnd())
		{
			auto actor = Engine::Instance()->ParseActor(this, parser);
			if (!actor)
				throw InvalidOperationException(String(L"invalid object type: ") + parser.NextToken().Str);
			if (actor->GetEngineType() == EngineActorType::StaticMesh)
				StaticActors.Add(actor.As<StaticMeshActor>());
			else
				GeneralActors.Add(actor);
			actorRegistry[actor->Name] = actor.Ptr();
			if (actor->GetEngineType() == EngineActorType::Camera)
				CurrentCamera = actor.As<CameraActor>();
		}
	}
	Mesh * Level::LoadMesh(const CoreLib::String & fileName)
	{
		RefPtr<Mesh> result = nullptr;
		if (!Meshes.TryGetValue(fileName, result))
		{
			auto actualName = Engine::Instance()->FindFile(fileName, ResourceType::Mesh);
			result = new Mesh();
			result->LoadFromFile(actualName);
			Meshes[fileName] = result;
		}
		return result.Ptr();
	}
	Skeleton * Level::LoadSkeleton(const CoreLib::String & fileName)
	{
		RefPtr<Skeleton> result = nullptr;
		if (!Skeletons.TryGetValue(fileName, result))
		{
			auto actualName = Engine::Instance()->FindFile(fileName, ResourceType::Mesh);
			result = new Skeleton();
			result->LoadFromFile(actualName);
			Skeletons[fileName] = result;
		}
		return result.Ptr();
	}
	Material * Level::LoadMaterial(const CoreLib::String & fileName)
	{
		RefPtr<Material> result = nullptr;
		if (!Materials.TryGetValue(fileName, result))
		{
			auto actualName = Engine::Instance()->FindFile(fileName, ResourceType::Material);
			result = new Material();
			result->LoadFromFile(actualName);
			Materials[fileName] = result;
		}
		return result.Ptr();
	}
	Material * Level::CreateNewMaterial()
	{
		Material* mat = new Material();
		Materials[String(L"$materialInstance") + String(Materials.Count())] = mat;
		return mat;
	}
	SkeletalAnimation * Level::LoadSkeletalAnimation(const CoreLib::String & fileName)
	{
		RefPtr<SkeletalAnimation> result = nullptr;
		if (!Animations.TryGetValue(fileName, result))
		{
			auto actualName = Engine::Instance()->FindFile(fileName, ResourceType::Mesh);
			result = new SkeletalAnimation();
			result->LoadFromFile(actualName);
			Animations[fileName] = result;
		}
		return result.Ptr();
	}
	Actor * Level::FindActor(const CoreLib::String & name)
	{
		Actor * result = nullptr;
		actorRegistry.TryGetValue(name, result);
		return result;
	}
}
