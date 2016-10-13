#ifndef GAME_ENGINE_LEVEL_H
#define GAME_ENGINE_LEVEL_H

#include "CoreLib/Basic.h"
#include "Mesh.h"
#include "Skeleton.h"
#include "StaticMeshActor.h"
#include "Camera.h"
#include "Material.h"

namespace GameEngine
{
	class Level : public CoreLib::Object
	{
	public:
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Material>> Materials;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Mesh>> Meshes;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<Skeleton>> Skeletons;
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::RefPtr<SkeletalAnimation>> Animations;
		CoreLib::List<CoreLib::RefPtr<StaticMeshActor>> StaticActors;
		CoreLib::List<CoreLib::RefPtr<Actor>> GeneralActors;
		CoreLib::EnumerableDictionary<CoreLib::String, Actor *> actorRegistry;
		CoreLib::RefPtr<CameraActor> CurrentCamera;
		
		Level(const CoreLib::String & fileName);
		Mesh * LoadMesh(const CoreLib::String & fileName);
		Skeleton * LoadSkeleton(const CoreLib::String & fileName);
		Material * LoadMaterial(const CoreLib::String & fileName);
		Material * CreateNewMaterial();
		SkeletalAnimation * LoadSkeletalAnimation(const CoreLib::String & fileName);
		Actor * FindActor(const CoreLib::String & name);
	};
}

#endif