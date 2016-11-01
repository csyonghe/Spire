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
			if (parser.LookAhead(L"rotate"))
			{
				parser.ReadToken();
				RootRotation.x = (float)parser.ReadDouble();
				parser.Read(L",");
				RootRotation.y = (float)parser.ReadDouble();
				parser.Read(L",");
				RootRotation.z = (float)parser.ReadDouble();
			}
			if (parser.LookAhead(L"mapping"))
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
		}

	}
}
