#ifndef CORE_LIB_GRAPHICS_BBOX_H
#define CORE_LIB_GRAPHICS_BBOX_H

#include "../VectorMath.h"
#include "../LibMath.h"

namespace CoreLib
{
	namespace Graphics
	{
		using namespace VectorMath;

		class BBox
		{
		public:
			union
			{
				struct
				{
					float xMin, yMin, zMin, xMax, yMax, zMax;
#pragma warning(suppress : 4201)
				};
				struct
				{
					Vec3 Min, Max;
#pragma warning(suppress : 4201)
				};
			};
			inline int MaxDimension() const
			{
				float xsize = xMax-xMin;
				float ysize = yMax-yMin;
				float zsize = zMax-zMin;
				if (xsize>ysize)
				{
					if (xsize>zsize)
						return 0;
					else
						return 2;
				}
				else
				{
					if (ysize > zsize)
						return 1;
					else
						return 2;
				}
			}
			inline void Init()
			{
				xMin = yMin = zMin = FLT_MAX;
				xMax = yMax = zMax = -FLT_MAX;
			}
			inline void Union(const BBox & box)
			{
				xMin = Math::Min(box.xMin, xMin);
				yMin = Math::Min(box.yMin, yMin);
				zMin = Math::Min(box.zMin, zMin);

				xMax = Math::Max(box.xMax, xMax);
				yMax = Math::Max(box.yMax, yMax);
				zMax = Math::Max(box.zMax, zMax);
			}
			inline void Union(const Vec3 &v)
			{
				xMin = Math::Min(v.x, xMin);
				yMin = Math::Min(v.y, yMin);
				zMin = Math::Min(v.z, zMin);

				xMax = Math::Max(v.x, xMax);
				yMax = Math::Max(v.y, yMax);
				zMax = Math::Max(v.z, zMax);
			}
			inline BBox Intersection(const BBox & box)
			{
				BBox rs;
				rs.xMin = Math::Max(box.xMin, xMin);
				rs.yMin = Math::Max(box.yMin, yMin);
				rs.zMin = Math::Max(box.zMin, zMin);

				rs.xMax = Math::Min(box.xMax, xMax);
				rs.yMax = Math::Min(box.yMax, yMax);
				rs.zMax = Math::Min(box.zMax, zMax);
				return rs;
			}
			inline bool Contains(const Vec3 & v)
			{
				return v.x >= xMin && v.x <= xMax &&
					   v.y >= yMin && v.y <= yMax &&
					   v.z >= zMin && v.z <= zMax;
			}
			inline bool ContainsNoOverlap(const Vec3 & v)
			{
				return v.x >= xMin && v.x < xMax &&
					v.y >= yMin && v.y < yMax &&
					v.z >= zMin && v.z < zMax;
			}
			inline bool Intersects(const BBox & box)
			{
				return !(xMin>=box.xMax || yMin >= box.yMax || zMin >= box.zMax || 
					xMax <= box.xMin || yMax <= box.yMin || zMax <= box.zMin);
			}
			inline void GetCornerPoints(Vec3 cornerPoints[8]) const
			{
				cornerPoints[0] = Vec3::Create(xMin, yMin, zMin);
				cornerPoints[1] = Vec3::Create(xMax, yMin, zMin);
				cornerPoints[2] = Vec3::Create(xMin, yMax, zMin);
				cornerPoints[3] = Vec3::Create(xMax, yMax, zMin);
				cornerPoints[4] = Vec3::Create(xMin, yMin, zMax);
				cornerPoints[5] = Vec3::Create(xMax, yMin, zMax);
				cornerPoints[6] = Vec3::Create(xMin, yMax, zMax);
				cornerPoints[7] = Vec3::Create(xMax, yMax, zMax);
			}
			float Distance(Vec3 p);
		};

		inline bool RayBBoxIntersection(const BBox & bbox, const Vec3 & origin, const Vec3 & dir, float & tmin, float & tmax)
		{
			float tymin, tymax, tzmin, tzmax;
			Vec3 rdir = dir;
			rdir.x = 1.0f / dir.x;
			rdir.y = 1.0f / dir.y;
			rdir.z = 1.0f / dir.z;

			if (rdir.x >= 0)
			{
				tmin = (bbox.Min.x - origin.x) * rdir.x;
				tmax = (bbox.Max.x - origin.x) * rdir.x;
			}
			else
			{
				tmin = (bbox.Max.x - origin.x) * rdir.x;
				tmax = (bbox.Min.x - origin.x) * rdir.x;
			}
			if (rdir.y >= 0)
			{
				tymin = (bbox.Min.y - origin.y) * rdir.y;
				tymax = (bbox.Max.y - origin.y) * rdir.y;
			}
			else
			{
				tymin = (bbox.Max.y - origin.y) * rdir.y;
				tymax = (bbox.Min.y - origin.y) * rdir.y;
			}
			if (tmin - tymax > Epsilon || tymin - tmax > Epsilon)
				return false;
			if (tymin > tmin)
				tmin = tymin;
			if (tymax < tmax)
				tmax = tymax;
			if (rdir.z >= 0)
			{
				tzmin = (bbox.Min.z - origin.z) * rdir.z;
				tzmax = (bbox.Max.z - origin.z) * rdir.z;
			}
			else
			{
				tzmin = (bbox.Max.z - origin.z) * rdir.z;
				tzmax = (bbox.Min.z - origin.z) * rdir.z;
			}
			if (tmin - tzmax > Epsilon || tzmin - tmax > Epsilon)
				return false;
			if (tzmin > tmin)
				tmin = tzmin;
			if (tzmax < tmax)
				tmax = tzmax;
			return tmin <= tmax;
		}

		inline void TransformBBox(BBox & bboxOut, const Matrix4 & mat, const BBox & bboxIn)
		{
			Vec3 v, v_t;
			bboxOut.Init();
			for (int i = 0; i < 8; i++)
			{
				if (i & 1)
					v.x = bboxIn.xMax;
				else
					v.x = bboxIn.xMin;
				if (i & 2)
					v.y = bboxIn.yMax;
				else
					v.y = bboxIn.yMin;
				if (i & 4)
					v.z = bboxIn.zMax;
				else
					v.z = bboxIn.zMin;
				mat.Transform(v_t, v);
				bboxOut.Union(v_t);
			}
		}
	}
}

#endif