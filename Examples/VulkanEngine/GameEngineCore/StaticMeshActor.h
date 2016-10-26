#ifndef GAME_ENGINE_STATIC_ACTOR_H
#define GAME_ENGINE_STATIC_ACTOR_H

#include "Actor.h"

namespace GameEngine
{
	class Level;

	class StaticMeshActor : public Actor
	{
	protected:
		virtual bool ParseField(Level * level, CoreLib::Text::Parser & parser, bool & isInvalid) override;
	public:
		CoreLib::String MeshName;
		Mesh * Mesh = nullptr;
		Material * MaterialInstance;
		CoreLib::RefPtr<StaticRenderContext> RenderContext;
		virtual void Tick() override { }
		virtual EngineActorType GetEngineType() override
		{
			return EngineActorType::StaticMesh;
		}
		virtual CoreLib::String GetTypeName() override
		{
			return L"StaticMesh";
		}
	};
}

#endif