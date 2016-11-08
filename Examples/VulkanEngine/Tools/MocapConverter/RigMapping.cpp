#include "RigMapping.h"
#include "CoreLib/Parser.h"
#include "CoreLib/LibIO.h"

namespace GameEngine
{
	namespace Tools
	{
		using namespace CoreLib;
		using namespace CoreLib::Text;
		RigMappingFile::RigMappingFile(CoreLib::String fileName)
		{
			Parser parser(CoreLib::IO::File::ReadAllText(fileName));
			while (!parser.IsEnd())
			{
				if (parser.LookAhead(L"mesh_rotate"))
				{
					parser.ReadToken();
					RootRotation.x = (float)parser.ReadDouble();
					parser.Read(L",");
					RootRotation.y = (float)parser.ReadDouble();
					parser.Read(L",");
					RootRotation.z = (float)parser.ReadDouble();
				}
				else if (parser.LookAhead(L"anim_scale"))
				{
					parser.ReadToken();
					TranslationScale = (float)parser.ReadDouble();
				}
				else if (parser.LookAhead(L"mapping"))
				{
					parser.ReadToken();
					while (!parser.IsEnd())
					{
						auto src = parser.ReadWord();
						parser.Read(L"=");
						auto dest = parser.ReadWord();
						Mapping[src] = dest;
					}
				}
				else
					throw TextFormatException(L"unknown field");
			}
		}
	}
}
