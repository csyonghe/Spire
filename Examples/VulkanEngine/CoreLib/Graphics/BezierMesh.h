#ifndef CORE_LIB_BEZIER_MESH_H
#define CORE_LIB_BEZIER_MESH_H

#include "../VectorMath.h"
#include "../Basic.h"

namespace CoreLib
{
	namespace Graphics
	{
		using namespace VectorMath;
		using namespace CoreLib::Basic;

		class BezierPatch
		{
		public:
			Vec3 ControlPoints[4][4];
			Vec3 TangentU[3][4];
			Vec3 TangentV[3][4];
			Vec2 TexCoords[4];
		};

		class BezierMesh
		{
		public:
			List<BezierPatch> Patches;
		};

		struct ObjModel;
		BezierMesh BezierMeshFromQuadObj(ObjModel & obj);
	}
}

#endif