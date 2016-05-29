#ifndef CORELIB_GRAPHICS_VIEWFRUSTUM_H
#define CORELIB_GRAPHICS_VIEWFRUSTUM_H
#include "../VectorMath.h"
#include "../Basic.h"

namespace CoreLib
{
	namespace Graphics
	{
		class ViewFrustum
		{
		public:
			VectorMath::Vec3 CamPos, CamDir, CamUp;
			float zMin, zMax;
			float Aspect, FOV;
			CoreLib::Array<VectorMath::Vec3, 8> GetVertices(float zNear, float zFar) const;
			VectorMath::Matrix4 GetViewTransform() const
			{
				VectorMath::Matrix4 rs;
				VectorMath::Vec3 right;
				VectorMath::Vec3::Cross(right, CamDir, CamUp);
				VectorMath::Vec3::Normalize(right, right);
				VectorMath::Matrix4 mat;
				mat.values[0] = right.x; mat.values[4] = right.y; mat.values[8] = right.z; mat.values[12] = 0.0f;
				mat.values[1] = CamUp.x; mat.values[5] = CamUp.y; mat.values[9] = CamUp.z; mat.values[13] = 0.0f;
				mat.values[2] = -CamDir.x; mat.values[6] = -CamDir.y; mat.values[10] = -CamDir.z; mat.values[14] = 0.0f;
				mat.values[3] = 0.0f; mat.values[7] = 0.0f; mat.values[11] = 0.0f; mat.values[15] = 1.0f;
				VectorMath::Matrix4 translate;
				VectorMath::Matrix4::Translation(translate, -CamPos.x, -CamPos.y, -CamPos.z);
				VectorMath::Matrix4::Multiply(rs, mat, translate);
				return rs;
			}
			VectorMath::Matrix4 GetProjectionTransform() const
			{
				VectorMath::Matrix4 rs;
				VectorMath::Matrix4::CreatePerspectiveMatrixFromViewAngle(rs,
					FOV,
					Aspect,
					zMin,
					zMax);
				return rs;
			}
			VectorMath::Matrix4 GetViewProjectionTransform() const
			{
				auto view = GetViewTransform();
				auto proj = GetProjectionTransform();
				return proj * view;
			}
		};
	}
}
#endif