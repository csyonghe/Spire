#include "ViewFrustum.h"
using namespace CoreLib::Basic;
using namespace VectorMath;

namespace CoreLib
{
	namespace Graphics
	{
		Array<VectorMath::Vec3, 8> ViewFrustum::GetVertices(float zNear, float zFar) const
		{
			Array<VectorMath::Vec3, 8> result;
			result.SetSize(result.GetCapacity());
			auto right = Vec3::Cross(CamDir, CamUp).Normalize();
			auto up = CamUp;
			auto nearCenter = CamPos + CamDir * zNear;
			auto farCenter = CamPos + CamDir * zFar;
			auto tanFOV = tan(FOV / 180.0f * (Math::Pi * 0.5f));
			auto nearUpScale = tanFOV * zNear;
			auto farUpScale = tanFOV * zFar;
			auto nearRightScale = nearUpScale * Aspect;
			auto farRightScale = farUpScale * Aspect;
			result[0] = nearCenter - right * nearRightScale + up * nearUpScale;
			result[1] = nearCenter + right * nearRightScale + up * nearUpScale;
			result[2] = nearCenter + right * nearRightScale - up * nearUpScale;
			result[3] = nearCenter - right * nearRightScale - up * nearUpScale;
			result[4] = farCenter - right * farRightScale + up * farUpScale;
			result[5] = farCenter + right * farRightScale + up * farUpScale;
			result[6] = farCenter + right * farRightScale - up * farUpScale;
			result[7] = farCenter - right * farRightScale - up * farUpScale;
			return result;
		}
	}
}
