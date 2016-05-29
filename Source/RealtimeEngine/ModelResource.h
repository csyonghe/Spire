#ifndef REALTIME_ENGINE_MODEL_RESOURCE_H
#define REALTIME_ENGINE_MODEL_RESOURCE_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Graphics/ObjModel.h"
#include "CoreLib/Graphics/AseFile.h"
#include "CoreLib/LibIO.h"
#include "Mesh.h"

using namespace CoreLib::Basic;
using namespace VectorMath;
using namespace CoreLib::Graphics;

namespace RealtimeEngine
{
	class ModelResource
	{
	public:
		static List<Mesh> LoadObj(const ObjModel & mdl);
		static void ConvertObjToMeshes(String fileName);

		static void ConvertAseToMeshes(String fileName);
	};

	Vec3 UnpackUnitVector8(unsigned int value);
}
#endif