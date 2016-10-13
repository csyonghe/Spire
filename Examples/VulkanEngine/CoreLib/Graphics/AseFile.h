#ifndef CORE_LIB_GRAPHICS_ASE_H
#define CORE_LIB_GRAPHICS_ASE_H

#include "../VectorMath.h"
#include "../LibMath.h"
#include "../Basic.h"

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