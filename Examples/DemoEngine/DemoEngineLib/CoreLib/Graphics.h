/***********************************************************************

CoreLib - The MIT License (MIT)
Copyright (c) 2016, Yong He

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.

***********************************************************************/

/***********************************************************************
WARNING: This is an automatically generated file.
***********************************************************************/
#include "Basic.h"

/***********************************************************************
VIEWFRUSTUM.H
***********************************************************************/
#ifndef CORELIB_GRAPHICS_VIEWFRUSTUM_H
#define CORELIB_GRAPHICS_VIEWFRUSTUM_H

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

/***********************************************************************
TEXTUREFILE.H
***********************************************************************/
#ifndef CORELIB_GRAPHICS_TEXTURE_FILE_H
#define CORELIB_GRAPHICS_TEXTURE_FILE_H


namespace CoreLib
{
	namespace Graphics
	{
		enum class TextureType : short
		{
			Texture2D
		};
		enum class TextureStorageFormat : short
		{
			R8, RG8, RGB8, RGBA8,
			R_F32, RG_F32, RGB_F32, RGBA_F32
		};
		class TextureFileHeader
		{
		public:
			TextureType Type;
			TextureStorageFormat Format;
			int Width, Height;
		};

		class TextureFile
		{
		private:
			CoreLib::Basic::List<unsigned char> buffer;
			TextureStorageFormat format;
			int width, height;
			void LoadFromStream(CoreLib::IO::Stream * stream);
		public:
			TextureFile()
			{
				width = height = 0;
				format = TextureStorageFormat::RGBA8;
			}
			TextureFile(CoreLib::Basic::String fileName);
			TextureFile(CoreLib::IO::Stream * stream);
			TextureStorageFormat GetFormat()
			{
				return format;
			}
			int GetWidth()
			{
				return width;
			}
			int GetHeight()
			{
				return height;
			}
			void SaveToFile(CoreLib::Basic::String fileName);
			void SaveToStream(CoreLib::IO::Stream * stream);
			void SetData(TextureStorageFormat format, int width, int height, CoreLib::Basic::ArrayView<unsigned char> data);
			CoreLib::Basic::ArrayView<unsigned char> GetData()
			{
				return buffer.GetArrayView();
			}
			CoreLib::Basic::List<float> GetPixels()
			{
				CoreLib::Basic::List<float> pixels;
				pixels.SetSize(width * height * 4);
				for (int i = 0; i < width*height; i++)
				{
					float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
					switch (format)
					{
					case TextureStorageFormat::R8:
						color[0] = buffer[i] / 255.0f;
						break;
					case TextureStorageFormat::RG8:
						color[0] = buffer[i*2] / 255.0f;
						color[1] = buffer[i*2 + 1] / 255.0f;
						break;
					case TextureStorageFormat::RGB8:
						color[0] = buffer[i * 3] / 255.0f;
						color[1] = buffer[i * 3 + 1] / 255.0f;
						color[2] = buffer[i * 3 + 2] / 255.0f;
						break;
					case TextureStorageFormat::RGBA8:
						color[0] = buffer[i * 4] / 255.0f;
						color[1] = buffer[i * 4 + 1] / 255.0f;
						color[2] = buffer[i * 4 + 2] / 255.0f;
						color[3] = buffer[i * 4 + 3] / 255.0f;
						break;
					case TextureStorageFormat::R_F32:
						color[0] = ((float*)buffer.Buffer())[i];
						break;
					case TextureStorageFormat::RG_F32:
						color[0] = ((float*)buffer.Buffer())[i*2];
						color[1] = ((float*)buffer.Buffer())[i*2 + 1];
						break;
					case TextureStorageFormat::RGB_F32:
						color[0] = ((float*)buffer.Buffer())[i * 3];
						color[1] = ((float*)buffer.Buffer())[i * 3 + 1];
						color[2] = ((float*)buffer.Buffer())[i * 3 + 2];
						break;
					case TextureStorageFormat::RGBA_F32:
						color[0] = ((float*)buffer.Buffer())[i * 4];
						color[1] = ((float*)buffer.Buffer())[i * 4 + 1];
						color[2] = ((float*)buffer.Buffer())[i * 4 + 2];
						color[3] = ((float*)buffer.Buffer())[i * 4 + 3];
						break;
					}
					pixels[i * 4] = color[0];
					pixels[i * 4 + 1] = color[1];
					pixels[i * 4 + 2] = color[2];
					pixels[i * 4 + 3] = color[3];

				}
				return pixels;
			}
		};
	}
}

#endif

/***********************************************************************
ASEFILE.H
***********************************************************************/
#ifndef CORE_LIB_GRAPHICS_ASE_H
#define CORE_LIB_GRAPHICS_ASE_H


namespace CoreLib
{
	namespace Graphics
	{
		class AseValue
		{
		public:
			CoreLib::Basic::String Str;
			VectorMath::Vec4 Val;
			AseValue() = default;
			AseValue(const CoreLib::Basic::String & str) { Str = str; }
			AseValue(VectorMath::Vec4 val) { Val = val; }
			AseValue(float val) { Val = VectorMath::Vec4::Create(val, 0.0f, 0.0f, 0.0f); }
		};
		class AseMaterial
		{
		public:
			CoreLib::Basic::String Name;
			CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, AseValue> Fields;
			CoreLib::Basic::List<CoreLib::Basic::RefPtr<AseMaterial>> SubMaterials;
		};
		class AseMeshFace
		{
		public:
			int Ids[3];
			int SmoothGroup;
			int MaterialId;
		};
		class AseMeshAttribFace
		{
		public:
			int Ids[3];
		};
		template<typename T>
		class AseMeshVertAttrib
		{
		public:
			CoreLib::Basic::List<T> Data;
			CoreLib::Basic::List<AseMeshAttribFace> Faces;
		};
		class AseMesh
		{
		public:
			CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, AseValue> Attributes;
			CoreLib::Basic::List<VectorMath::Vec3> Vertices;
			CoreLib::Basic::List<AseMeshFace> Faces;
			AseMeshVertAttrib<VectorMath::Vec3> Colors;
			AseMeshVertAttrib<VectorMath::Vec3> Normals;
			CoreLib::Basic::List<AseMeshVertAttrib<VectorMath::Vec3>> TexCoords;
			void RecomputeNormals();
			void ConstructPerVertexFaceList(Basic::List<int> & faceCountAtVert, Basic::List<int> & vertFaceList) const;
		};
		class AseGeomObject
		{
		public:
			CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, AseValue> Attributes;
			CoreLib::Basic::RefPtr<AseMesh> Mesh;
			int MaterialId;
		};
		class AseFile
		{
		public:
			CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, AseValue> Attributes;
			CoreLib::Basic::List<CoreLib::Basic::RefPtr<AseGeomObject>> GeomObjects;
			CoreLib::Basic::List<CoreLib::Basic::RefPtr<AseMaterial>> Materials;
		public:
			void Parse(const CoreLib::Basic::String & content, bool flipYZ);
			void LoadFromFile(const CoreLib::Basic::String & fileName, bool flipYZ);
		};
	}
}

#endif

/***********************************************************************
BBOX.H
***********************************************************************/
#ifndef CORE_LIB_GRAPHICS_BBOX_H
#define CORE_LIB_GRAPHICS_BBOX_H


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

/***********************************************************************
BEZIERMESH.H
***********************************************************************/
#ifndef CORE_LIB_BEZIER_MESH_H
#define CORE_LIB_BEZIER_MESH_H


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

/***********************************************************************
OBJMODEL.H
***********************************************************************/
#ifndef RAY_TRACE_PRO_OBJ_MODEL_H
#define RAY_TRACE_PRO_OBJ_MODEL_H


namespace CoreLib
{
	namespace Graphics
	{
		enum class PolygonType
		{
			Triangle, Quad
		};
		struct ObjFace
		{
			int VertexIds[4];
			int NormalIds[4];
			int TexCoordIds[4];
			unsigned int SmoothGroup;
			int MaterialId;
		};
		const int ObjMaterialVersion = 1;
		struct ObjMaterial
		{
			Basic::String Name;
			float SpecularRate;
			VectorMath::Vec3 Diffuse, Specular;
			bool IsOpaque;
			Basic::String DiffuseMap, BumpMap, AlphaMap;
			ObjMaterial()
			{
				IsOpaque = false; 
				Diffuse.SetZero();
				Specular.SetZero();
				SpecularRate = 0.0f;
			}
		};
		struct ObjModel
		{
			Basic::List<Basic::RefPtr<ObjMaterial>> Materials;
			Basic::List<VectorMath::Vec3> Vertices, Normals;
			Basic::List<VectorMath::Vec2> TexCoords;
			Basic::List<ObjFace> Faces;
			void ConstructPerVertexFaceList(Basic::List<int> & faceCountAtVert, Basic::List<int> & vertFaceList) const;
			void SaveToBinary(IO::BinaryWriter & writer);
			bool LoadFromBinary(IO::BinaryReader & reader);
		};
		bool LoadObj(ObjModel & mdl, const char * fileName, PolygonType polygonType = PolygonType::Triangle);
		void RecomputeNormals(ObjModel & mdl);
	}
}

#endif

/***********************************************************************
CAMERA.H
***********************************************************************/
#ifndef GX_GL_CAMERA_H
#define GX_GL_CAMERA_H
namespace CoreLib
{
	namespace Graphics
	{
		class Camera
		{
		public:
			Camera();
		public:
			float alpha,beta;
			VectorMath::Vec3 pos,up,dir;
			bool CanFly;
			void GoForward(float u);
			void MoveLeft(float u);
			void TurnLeft(float u);
			void TurnUp(float u);
			void GetTransform(VectorMath::Matrix4 & mat);
			void GetView(ViewFrustum & view);
			void Reset();
			void GetInverseRotationMatrix(float rot[9]);
		};

#ifdef _WIN32
		class CameraController
		{
		public:
			static void HandleCameraKeys(Camera & camera, VectorMath::Matrix4 & transform, float dtime, float minSpeed, float maxSpeed, bool flipYZ);
		};
#endif
	}
}
#endif

/***********************************************************************
GGX.H
***********************************************************************/
#ifndef CORELIB_GRAPHICS_GGX_H
#define CORELIB_GRAPHICS_GGX_H


namespace CoreLib
{
	namespace Graphics
	{
		CoreLib::Basic::List<VectorMath::Vec2> ComputeTextureFV(float maxRoughness, int size);
		CoreLib::Basic::List<float> ComputeTextureD(float maxRoughness, int size);
		TextureFile ComputeTextureFileFV(float maxRoughness, int size);
		TextureFile ComputeTextureFileD(float maxRoughness, int size);
	}
}

#endif
