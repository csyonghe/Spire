#include "StaticMeshActor.h"
#include "Level.h"
#include "Engine.h"

namespace GameEngine
{
	bool StaticMeshActor::ParseField(Level * level, CoreLib::Text::Parser & parser)
	{
		if (Actor::ParseField(level, parser))
			return true;
		if (parser.LookAhead(L"mesh"))
		{
			parser.ReadToken();
			MeshName = parser.ReadStringLiteral();
			Mesh = level->LoadMesh(MeshName);
			if (!Mesh)
				Print(L"error: mesh '%s' not found.\n", MeshName.Buffer());
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
			}
			return true;
		}
		return false;
	}

}

