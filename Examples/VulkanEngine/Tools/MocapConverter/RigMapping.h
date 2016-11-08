#ifndef RIG_MAPPING_H
#define RIG_MAPPING_H

#include "CoreLib/Basic.h"
#include "VectorMath.h"

namespace GameEngine
{
	namespace Tools
	{
		class RigMappingFile
		{
		public:
			CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::String> Mapping;
			VectorMath::Vec3 RootRotation;
			float TranslationScale = 1.0f;
			RigMappingFile()
			{
				RootRotation.SetZero();
			}
			RigMappingFile(CoreLib::String fileName);
		};
	}
}

#endif