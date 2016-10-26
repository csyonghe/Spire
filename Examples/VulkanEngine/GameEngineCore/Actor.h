#ifndef GAME_ENGINE_ACTOR_H
#define GAME_ENGINE_ACTOR_H

#include "CoreLib/Basic.h"
#include "CoreLib/Parser.h"
#include "Mesh.h"
#include "AnimationSynthesizer.h"
#include "Material.h"
#include "CoreLib/Graphics/BBox.h"
#include "RenderContext.h"

namespace GameEngine
{
	enum class EngineActorType
	{
		StaticMesh, SkeletalMesh, Light, BoundingVolume, Camera, UserController
	};
	class Actor : public CoreLib::Object
	{
	protected:
		VectorMath::Vec3 ParseVec3(CoreLib::Text::Parser & parser);
		VectorMath::Vec4 ParseVec4(CoreLib::Text::Parser & parser);
		VectorMath::Matrix4 ParseMatrix4(CoreLib::Text::Parser & parser);
		void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec3 & v);
		void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Vec4 & v);
		void Serialize(CoreLib::StringBuilder & sb, const VectorMath::Matrix4 & v);

		virtual bool ParseField(Level * level, CoreLib::Text::Parser & parser, bool &isInvalid);
		virtual void SerializeFields(CoreLib::StringBuilder & sb);
	public:
		CoreLib::String Name;
		CoreLib::Graphics::BBox Bounds;
		VectorMath::Matrix4 LocalTransform;
		CoreLib::List<CoreLib::RefPtr<Actor>> SubComponents;
		virtual void Tick() { }
		virtual EngineActorType GetEngineType() = 0;
		virtual void OnLoad() {};
		virtual void OnUnload() {};
		virtual void Parse(Level * level, CoreLib::Text::Parser & parser, bool & isInvalid);
		virtual void SerializeToText(CoreLib::StringBuilder & sb);
		virtual CoreLib::String GetTypeName() { return L"Actor"; }
		Actor()
		{
			VectorMath::Matrix4::CreateIdentityMatrix(LocalTransform);
		}
		virtual ~Actor() override;
	};
}

#endif