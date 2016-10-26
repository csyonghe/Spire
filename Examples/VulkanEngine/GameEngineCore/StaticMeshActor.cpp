#include "StaticMeshActor.h"
#include "Level.h"
#include "Engine.h"

namespace GameEngine
{
	bool StaticMeshActor::ParseField(Level * level, CoreLib::Text::Parser & parser, bool & isInvalid)
	{
		if (Actor::ParseField(level, parser, isInvalid))
			return true;
		if (parser.LookAhead(L"mesh"))
		{
			parser.ReadToken();
			MeshName = parser.ReadStringLiteral();
			Mesh = level->LoadMesh(MeshName);
			if (!Mesh)
			{
				isInvalid = true;
			}
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
		return false;
	}

}

