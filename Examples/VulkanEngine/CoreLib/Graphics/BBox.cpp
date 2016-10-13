#include "BBox.h"

namespace CoreLib
{
	namespace Graphics
	{
		float CoreLib::Graphics::BBox::Distance(VectorMath::Vec3 p)
		{
			if (Contains(p))
				return 0.0f;
			VectorMath::Vec3 center = (Min + Max)*0.5f;
			VectorMath::Vec3 dir = center - p;
			dir = dir.Normalize();
			float tmin, tmax;
			if (RayBBoxIntersection(*this, p, dir, tmin, tmax))
				return tmin;
			return 1e30f;
		}
	}
}