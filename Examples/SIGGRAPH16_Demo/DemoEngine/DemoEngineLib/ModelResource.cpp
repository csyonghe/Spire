#include "ModelResource.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;

namespace DemoEngine
{
	class ObjMeshFace
	{
	public:
		int MaterialId;
		int VertIds[3];
	};

	class ObjMeshVertex
	{
	public:
		Vec3 Position;
		Vec3 Normal;
		Vec2 TexCoords[8];
		unsigned int Color;
		Vec3 Tangent, BiTangent;
	};

	unsigned int PackColor(Vec3 v)
	{
		union Converter
		{
			struct
			{
				unsigned char x, y, z, w;
			} fields;
			unsigned int value;
		} c;
		c.fields.x = (unsigned char)Math::Clamp((v.x) * 255.0f, 0.0f, 255.0f);
		c.fields.y = (unsigned char)Math::Clamp((v.y) * 255.0f, 0.0f, 255.0f);
		c.fields.z = (unsigned char)Math::Clamp((v.z) * 255.0f, 0.0f, 255.0f);
		c.fields.w = 255;
		return c.value;
	}

	unsigned int PackTexCoord(Vec2 v)
	{
		union Converter
		{
			struct
			{
				unsigned short x, y;
			} fields;
			unsigned int value;
		} c;
		c.fields.x = FloatToHalf(v.x);
		c.fields.y = FloatToHalf(v.y);
		return c.value;
	}

	unsigned int PackUnitVector8(Vec3 v, int k)
	{
		union Converter
		{
			struct
			{
				unsigned int x : 10, y : 10, z : 10, w : 2;
			} fields;
			unsigned int value;
		} c;
		c.fields.x = Math::Clamp((int)((v.x + 1.0f) * 0.5f * 1023.0f), 0, 1023);
		c.fields.y = Math::Clamp((int)((v.y + 1.0f) * 0.5f * 1023.0f), 0, 1023);
		c.fields.z = Math::Clamp((int)((v.z + 1.0f) * 0.5f * 1023.0f), 0, 1023);
		c.fields.w = k;
		return c.value;
	}

	Vec3 UnpackUnitVector8(unsigned int value)
	{
		union Converter
		{
			struct
			{
				unsigned int x : 10, y : 10, z : 10, w : 2;
			} fields;
			unsigned int value;
		} c;
		c.value = value;
		Vec3 rs;
		rs.x = (c.fields.x / 1023.0f) * 2.0f - 1.0f;
		rs.y = (c.fields.y / 1023.0f) * 2.0f - 1.0f;
		rs.z = (c.fields.z / 1023.0f) * 2.0f - 1.0f;
		return rs;
	}

	class IndexVertex
	{
	public:
		int PositionId;
		int NormalId;
		Array<int,8> TexCoordId;
		int TangentId, BiTangentId;
		IndexVertex()
		{}
		IndexVertex(int posId, int normId, int texCoordId, int tangentId = -1, int bitangentId = -1)
		{
			TexCoordId.SetSize(1);
			for (int i = 0; i < TexCoordId.Count(); i++)
				TexCoordId[i] = -1;
			PositionId = posId;
			NormalId = normId;
			TexCoordId[0] = texCoordId;
			TangentId = tangentId;
			BiTangentId = bitangentId;
		}
		int GetHashCode()
		{
			return ((PositionId << 16) + (NormalId << 6) + TexCoordId[0]) ^ (TangentId << 16) ^ BiTangentId;
		}
		bool operator == (const IndexVertex & other)
		{
			bool rs = PositionId == other.PositionId && NormalId == other.NormalId && TangentId == other.TangentId && BiTangentId == other.BiTangentId;
			if (!rs)
				return false;
			if (TexCoordId.Count() != other.TexCoordId.Count())
				return false;
			for (int i = 0; i < TexCoordId.Count(); i++)
				if (TexCoordId[i] != other.TexCoordId[i])
					return false;
			return true;
		}
	};

	class FaceVertexIndex
	{
	public:
		int Indices[4];
	};

	class CompiledMesh
	{
	public:
		List<ObjMeshFace> Faces;
		List<ObjMeshVertex> Vertices;
		int TexCoordChannels;
		bool ContainsColor;
	public:
		void SetMesh(const ObjModel & obj, int matId)
		{
			Faces.Clear();
			Vertices.Clear();
			Faces.Reserve(obj.Faces.Count());
			Vertices.Reserve(obj.Vertices.Count());
			for (auto & face : obj.Faces)
			{
				if (face.MaterialId != matId)
					continue;
				ObjMeshFace rsFace;
				rsFace.MaterialId = face.MaterialId;
				for (int i = 0; i < 4; i++)
				{
					if (face.VertexIds[i] == -1)
						break;
					IndexVertex idx(face.VertexIds[i], face.NormalIds[i], face.TexCoordIds[i]);
					rsFace.VertIds[i] = Vertices.Count();
					ObjMeshVertex vert;
					vert.Position = obj.Vertices[face.VertexIds[i]];
					vert.Normal = obj.Normals[face.NormalIds[i]];
					if (face.TexCoordIds[i] != -1)
						vert.TexCoords[0] = obj.TexCoords[face.TexCoordIds[i]];
					Vertices.Add(vert);
				}
				Faces.Add(rsFace);
			}
		}
		void SetObjMesh(const ObjModel & obj, List<Vec3> & tangents, List<Vec3> & bitangents,
			List<FaceVertexIndex> & tangentIndices, List<FaceVertexIndex> & bitangentIndices, int matId)
		{
			Faces.Clear();
			Vertices.Clear();
			Dictionary<IndexVertex, int> vertMap;
			Faces.Reserve(obj.Faces.Count());
			Vertices.Reserve(obj.Vertices.Count());
			int fid = 0;
			TexCoordChannels = 1;
			ContainsColor = false;
			for (auto & face : obj.Faces)
			{
				if (face.MaterialId != matId)
				{
					fid++;
					continue;
				}
				ObjMeshFace rsFace;
				rsFace.MaterialId = face.MaterialId;
				for (int i = 0; i < 4; i++)
				{
					if (face.VertexIds[i] == -1)
						break;

					rsFace.VertIds[i] = Vertices.Count();
					ObjMeshVertex vert;
					vert.Position = obj.Vertices[face.VertexIds[i]];
					vert.Normal = obj.Normals[face.NormalIds[i]];
					vert.Tangent = tangents[tangentIndices[fid].Indices[i]];
					vert.BiTangent = bitangents[bitangentIndices[fid].Indices[i]];
					Vec3 recoveredNormal;
					Vec3 tangent = vert.Tangent;
					Vec3 bitangent = vert.BiTangent;
					Vec3::Cross(recoveredNormal, bitangent, tangent);
					if (Vec3::Dot(recoveredNormal, obj.Normals[face.NormalIds[i]]) < 0.0f)
						vert.Tangent = tangent;
					if (face.TexCoordIds[i] != -1)
						vert.TexCoords[0] = obj.TexCoords[face.TexCoordIds[i]];
					Vertices.Add(vert);
				}
				Faces.Add(rsFace);
				fid++;
			}
		}

		void SetAseMesh(const AseMesh & obj, List<Vec3> & tangents, List<Vec3> & bitangents,
			List<FaceVertexIndex> & tangentIndices, List<FaceVertexIndex> & bitangentIndices, int matId)
		{
			Faces.Clear();
			Vertices.Clear();
			Faces.Reserve(obj.Faces.Count());
			Vertices.Reserve(obj.Vertices.Count());
			int fid = 0;
			TexCoordChannels = obj.TexCoords.Count();
			ContainsColor = obj.Colors.Data.Count() != 0;
			for (int faceId = 0; faceId < obj.Faces.Count(); faceId++)
			{
				auto & face = obj.Faces[faceId];
				auto & nface = obj.Normals.Faces[faceId];
				if (face.MaterialId != matId)
				{
					fid++;
					continue;
				}
				ObjMeshFace rsFace;
				rsFace.MaterialId = face.MaterialId;
				for (int i = 0; i < 3; i++)
				{
					rsFace.VertIds[i] = Vertices.Count();
					ObjMeshVertex vert;
					vert.Position = obj.Vertices[face.Ids[i]];
					auto normal = obj.Normals.Data[nface.Ids[i]];
					vert.Normal = normal;
					vert.Tangent = tangents[tangentIndices[fid].Indices[i]];
					vert.BiTangent = bitangents[bitangentIndices[fid].Indices[i]];
					Vec3 recoveredNormal;
					Vec3 tangent = vert.Tangent;
					Vec3 bitangent = vert.BiTangent;
					Vec3::Cross(recoveredNormal, bitangent, tangent);
					if (Vec3::Dot(recoveredNormal, normal) < 0.0f)
						vert.Tangent = tangent;
					for (int j = 0; j < obj.TexCoords.Count(); j++)
					{
						auto tex = obj.TexCoords[j].Data[obj.TexCoords[j].Faces[faceId].Ids[i]];
						Vec2 tex2;
						tex2.x = tex.x;
						tex2.y = tex.y;
						vert.TexCoords[j] = tex2;
					}
					if (ContainsColor)
						vert.Color = PackColor(obj.Colors.Data[obj.Colors.Faces[faceId].Ids[i]]);
					Vertices.Add(vert);
				}
				Faces.Add(rsFace);
				fid++;
			}
		}

		void GenerateRenderableMesh(Mesh * mesh)
		{
			mesh->Attributes.Add(VertexAttribute(L"vert_position", VertexDataType::Float, 3, false));
			mesh->Attributes.Add(VertexAttribute(L"vert_normal", VertexDataType::Float, 3, true));
			mesh->Attributes.Add(VertexAttribute(L"vert_tangent", VertexDataType::Float, 3, true));
			int texChannels = 1; // = TexCoordChannels;
			for (int i = 0; i < texChannels; i++)
				mesh->Attributes.Add(VertexAttribute(String(L"vert_texCoord") + String(i), VertexDataType::Float, 2, false));
			if (ContainsColor)
				mesh->Attributes.Add(VertexAttribute(L"vert_color", VertexDataType::UChar, 4, true));

			mesh->Bounds.Init();
			for (auto & vert : Vertices)
			{
				mesh->VertexData.AddRange((unsigned char*)&vert.Position, 12);
				mesh->VertexData.AddRange((unsigned char*)&vert.Normal, 12);
				mesh->VertexData.AddRange((unsigned char*)&vert.Tangent, 12);
				for (int i = 0; i < texChannels; i++)
					mesh->VertexData.AddRange((unsigned char*)&vert.TexCoords[i], 8);
				if (ContainsColor)
					mesh->VertexData.AddRange((unsigned char*)&vert.Color, 4);
				mesh->Bounds.Union(vert.Position);
			}
		}
	};

	void GenerateMeshFromObj(ArrayView<Mesh*> meshes, const ObjModel & mdl)
	{
		CompiledMesh compiledMesh;
		List<Vec3> tangentVerts, bitangentVerts;
		List<Vec3> packedTangentVerts, packedBitangentVerts;
		List<FaceVertexIndex> faceTangentIndices, faceBiTangentIndices;
		faceTangentIndices.SetSize(mdl.Faces.Count());
		faceBiTangentIndices.SetSize(mdl.Faces.Count());
		tangentVerts.SetSize(mdl.Faces.Count() * 4);
		bitangentVerts.SetSize(tangentVerts.Count());
		List<int> faceCountAtVert;
		List<int> vertFaceList;
		mdl.ConstructPerVertexFaceList(faceCountAtVert, vertFaceList);
		for (int i = 0; i < tangentVerts.Count(); i++)
		{
			tangentVerts[i].SetZero();
			bitangentVerts[i].SetZero();
		}
		for (int i = 0; i < mdl.Faces.Count(); i++)
		{
			Vec3 v1 = mdl.Vertices[mdl.Faces[i].VertexIds[0]];
			Vec3 v2 = mdl.Vertices[mdl.Faces[i].VertexIds[1]];
			Vec3 v3 = mdl.Vertices[mdl.Faces[i].VertexIds[2]];
			Vec2 tex1, tex2, tex3;
			tex1.SetZero(); tex2.SetZero(); tex3.SetZero();
			if (mdl.Faces[i].TexCoordIds[0] != -1)
				tex1 = mdl.TexCoords[mdl.Faces[i].TexCoordIds[0]];
			if (mdl.Faces[i].TexCoordIds[1] != -1)
				tex2 = mdl.TexCoords[mdl.Faces[i].TexCoordIds[1]];
			if (mdl.Faces[i].TexCoordIds[2] != -1)
				tex3 = mdl.TexCoords[mdl.Faces[i].TexCoordIds[2]];
			Vec3 ab = v2 - v1;
			Vec3 ac = v3 - v1;
			Vec3 normal;
			Vec3::Cross(normal, ab, ac);
			Vec3::Normalize(normal, normal);
			Vec3 faceTangent, faceBitangent;
			GetOrthoVec(faceTangent, normal);
			Vec3::Cross(faceBitangent, faceTangent, normal);
			Vec3::Normalize(faceTangent, faceTangent);
			Vec3::Normalize(faceBitangent, faceBitangent);
			ObjFace & face = mdl.Faces[i];
			for (int j = 0; j < 4; j++)
			{
				int vid = face.VertexIds[j];
				if (vid == -1)
					continue;

				Vec3 tangent, bitangent;
				auto vNormal = mdl.Normals[face.NormalIds[j]];

				float x1 = v2.x - v1.x;
				float x2 = v3.x - v1.x;
				float y1 = v2.y - v1.y;
				float y2 = v3.y - v1.y;
				float z1 = v2.z - v1.z;
				float z2 = v3.z - v1.z;

				float s1 = tex2.x - tex1.x;
				float s2 = tex3.x - tex1.x;
				float t1 = tex2.y - tex1.y;
				float t2 = tex3.y - tex1.y;
				float denom = (s1 * t2 - s2 * t1);
				if (denom != 0.0f)
				{
					float r = 1.0f / denom;
					tangent = Vec3::Create((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
					bitangent = Vec3::Create((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);
				}
				else
				{
					tangent = faceTangent;
					bitangent = faceBitangent;
				}
				tangentVerts[i * 4 + j] = tangent;
				bitangentVerts[i * 4 + j] = bitangent;
			}
		}
		for (int i = 0; i < mdl.Faces.Count(); i++)
		{
			ObjFace & face = mdl.Faces[i];
			for (int j = 0; j < 4; j++)
			{
				int vid = face.VertexIds[j];
				if (vid == -1)
					continue;
				auto vNormal = mdl.Normals[face.NormalIds[j]];
				Vec3 tangent;
				tangent.SetZero();
				Vec3 bitangent;
				bitangent.SetZero();
				int start = 0;
				if (vid > 0)
					start = faceCountAtVert[vid - 1];
				int end = faceCountAtVert[vid];
				for (int v = start; v < end; v++)
				{
					int fid = vertFaceList[v];
					if (mdl.Faces[fid].SmoothGroup & face.SmoothGroup)
					{
						for (int k = 0; k < 4; k++)
							if (mdl.Faces[fid].VertexIds[k] == vid)
							{
								tangent += tangentVerts[fid * 4 + k];
								bitangent += bitangentVerts[fid * 4 + k];
								break;
							}
					}
				}
				Vec3::Cross(bitangent, tangent, vNormal);
				if (bitangent.Length2() < 1e-3f)
				{
					GetOrthoVec(bitangent, vNormal);
				}
				Vec3::Cross(tangent, vNormal, bitangent);
				tangent = tangent.Normalize();
				bitangent = bitangent.Normalize();
				
				packedTangentVerts.Add(tangent);
				faceTangentIndices[i].Indices[j] = packedTangentVerts.Count() - 1;
				
				packedBitangentVerts.Add(bitangent);
				faceBiTangentIndices[i].Indices[j] = packedBitangentVerts.Count() - 1;
				
			}
		}
		for (int i = 0; i < meshes.Count(); i++)
		{
			compiledMesh.SetObjMesh(mdl, packedTangentVerts, packedBitangentVerts, faceTangentIndices, faceBiTangentIndices, i);
			compiledMesh.GenerateRenderableMesh(meshes[i]);
		}
	}

	List<Mesh> ModelResource::LoadObj(const ObjModel & mdl)
	{
		List<Mesh> rs;
		rs.SetSize(mdl.Materials.Count());
		List<Mesh*> meshes;
		meshes.SetSize(rs.Count());
		for (int i = 0; i < rs.Count(); i++)
		{
			meshes[i] = &rs[i];
		}
		GenerateMeshFromObj(meshes.GetArrayView(), mdl);
		return rs;
	}

	void ModelResource::ConvertObjToMeshes(String fileName)
	{
		ObjModel mdl;
		CoreLib::Graphics::LoadObj(mdl, fileName.ToMultiByteString());
		RecomputeNormals(mdl);
		auto meshes = ModelResource::LoadObj(mdl);
		auto objName = Path::GetFileNameWithoutEXT(fileName);
		auto basePath = Path::GetDirectoryName(fileName);
		auto includeFile = Path::Combine(basePath, objName + L"_common.drawable");
		auto shaderName = Path::Combine(basePath, objName + L".shader");
		if (!File::Exists(includeFile))
			File::WriteAllText(includeFile, L"");
		if (!File::Exists(shaderName))
			File::WriteAllText(shaderName, L"");
		for (int i = 0; i < meshes.Count(); i++)
		{
			if (meshes[i].VertexData.Count())
			{
				auto name = mdl.Materials[i]->Name;
				if (name.Length() == 0)
					name = String(i);
				auto componentName = objName + L"_" + name;
				auto componentFileName = componentName + L".vtx";
				meshes[i].SaveToFile(Path::Combine(basePath, componentFileName));
				auto drawableFileName = componentName + L".drawable";
				StringBuilder sb;
				sb << L"model\n{\n\tfile \"" << componentFileName << L"\"\n}\nshader \"" << objName << L".shader\"\nusing \"" << objName << L"_common.drawable" << L"\"\n";
				if (mdl.Materials[i]->DiffuseMap.Length())
					sb << L"uniform texture2D texAlbedo = \"" << mdl.Materials[i]->DiffuseMap << L"\";\n";
				if (mdl.Materials[i]->BumpMap.Length())
					sb << L"uniform texture2D texNormal = \"" << mdl.Materials[i]->BumpMap << L"\";\n";
				auto drawableFile = Path::Combine(basePath, drawableFileName);
				if (!File::Exists(drawableFile))
					File::WriteAllText(drawableFile, sb.ProduceString());
			}
		}
	}

	List<Mesh> LoadAseMesh(AseMesh & mdl)
	{
		List<Mesh> result;
		Dictionary<int, int> materials;
		for (auto &f : mdl.Faces)
			materials.AddIfNotExists(f.MaterialId, materials.Count());
		result.SetSize(materials.Count());
		mdl.RecomputeNormals();

		CompiledMesh compiledMesh;
		List<Vec3> tangentVerts, bitangentVerts;
		List<Vec3> packedTangentVerts, packedBitangentVerts;
		List<FaceVertexIndex> faceTangentIndices, faceBiTangentIndices;
		faceTangentIndices.SetSize(mdl.Faces.Count());
		faceBiTangentIndices.SetSize(mdl.Faces.Count());
		tangentVerts.SetSize(mdl.Faces.Count() * 4);
		bitangentVerts.SetSize(tangentVerts.Count());
		List<int> faceCountAtVert;
		List<int> vertFaceList;
		mdl.ConstructPerVertexFaceList(faceCountAtVert, vertFaceList);
		for (int i = 0; i < tangentVerts.Count(); i++)
		{
			tangentVerts[i].SetZero();
			bitangentVerts[i].SetZero();
		}
		for (int i = 0; i < mdl.Faces.Count(); i++)
		{
			Vec3 v1 = mdl.Vertices[mdl.Faces[i].Ids[0]];
			Vec3 v2 = mdl.Vertices[mdl.Faces[i].Ids[1]];
			Vec3 v3 = mdl.Vertices[mdl.Faces[i].Ids[2]];
			Vec3 tex1, tex2, tex3;
			tex1.SetZero(); tex2.SetZero(); tex3.SetZero();
			if (mdl.TexCoords.Count() && mdl.TexCoords[0].Faces[i].Ids[0] != -1)
				tex1 = mdl.TexCoords[0].Data[mdl.TexCoords[0].Faces[i].Ids[0]];
			if (mdl.TexCoords.Count() && mdl.TexCoords[0].Faces[i].Ids[1] != -1)
				tex2 = mdl.TexCoords[0].Data[mdl.TexCoords[0].Faces[i].Ids[1]];
			if (mdl.TexCoords.Count() && mdl.TexCoords[0].Faces[i].Ids[2] != -1)
				tex3 = mdl.TexCoords[0].Data[mdl.TexCoords[0].Faces[i].Ids[2]];
			Vec3 ab = v2 - v1;
			Vec3 ac = v3 - v1;
			Vec3 normal;
			Vec3::Cross(normal, ab, ac);
			Vec3::Normalize(normal, normal);
			Vec3 faceTangent, faceBitangent;
			GetOrthoVec(faceTangent, normal);
			Vec3::Cross(faceBitangent, faceTangent, normal);
			Vec3::Normalize(faceTangent, faceTangent);
			Vec3::Normalize(faceBitangent, faceBitangent);
			auto & nface = mdl.Normals.Faces[i];
			for (int j = 0; j < 3; j++)
			{
				Vec3 tangent, bitangent;
				auto vNormal = mdl.Normals.Data[nface.Ids[j]];

				float x1 = v2.x - v1.x;
				float x2 = v3.x - v1.x;
				float y1 = v2.y - v1.y;
				float y2 = v3.y - v1.y;
				float z1 = v2.z - v1.z;
				float z2 = v3.z - v1.z;

				float s1 = tex2.x - tex1.x;
				float s2 = tex3.x - tex1.x;
				float t1 = tex2.y - tex1.y;
				float t2 = tex3.y - tex1.y;
				float denom = (s1 * t2 - s2 * t1);
				if (denom != 0.0f)
				{
					float r = 1.0f / denom;
					tangent = Vec3::Create((t2 * x1 - t1 * x2) * r, (t2 * y1 - t1 * y2) * r, (t2 * z1 - t1 * z2) * r);
					bitangent = Vec3::Create((s1 * x2 - s2 * x1) * r, (s1 * y2 - s2 * y1) * r, (s1 * z2 - s2 * z1) * r);
				}
				else
				{
					tangent = faceTangent;
					bitangent = faceBitangent;
				}
				tangentVerts[i * 4 + j] = tangent;
				bitangentVerts[i * 4 + j] = bitangent;
			}
		}
		for (int i = 0; i < mdl.Faces.Count(); i++)
		{
			auto & face = mdl.Faces[i];
			auto & nface = mdl.Normals.Faces[i];
			for (int j = 0; j < 3; j++)
			{
				int vid = face.Ids[j];
				auto vNormal = mdl.Normals.Data[nface.Ids[j]];
				Vec3 tangent;
				tangent.SetZero();
				Vec3 bitangent;
				bitangent.SetZero();
				int start = 0;
				if (vid > 0)
					start = faceCountAtVert[vid - 1];
				int end = faceCountAtVert[vid];
				for (int v = start; v < end; v++)
				{
					int fid = vertFaceList[v];
					if (mdl.Faces[fid].SmoothGroup & face.SmoothGroup)
					{
						for (int k = 0; k < 3; k++)
							if (mdl.Faces[fid].Ids[k] == vid)
							{
								tangent += tangentVerts[fid * 4 + k];
								bitangent += bitangentVerts[fid * 4 + k];
								break;
							}
					}
				}
				Vec3::Cross(bitangent, tangent, vNormal);
				if (bitangent.Length2() < 1e-3f)
				{
					GetOrthoVec(bitangent, vNormal);
				}
				Vec3::Cross(tangent, vNormal, bitangent);
				tangent = tangent.Normalize();
				bitangent = bitangent.Normalize();
				packedTangentVerts.Add(tangent);
				faceTangentIndices[i].Indices[j] = packedTangentVerts.Count() - 1;
				packedBitangentVerts.Add(bitangent);
				faceBiTangentIndices[i].Indices[j] = packedBitangentVerts.Count() - 1;
				
			}
		}
		for (int i = 0; i < result.Count(); i++)
		{
			compiledMesh.SetAseMesh(mdl, packedTangentVerts, packedBitangentVerts, faceTangentIndices, faceBiTangentIndices, i);
			compiledMesh.GenerateRenderableMesh(&result[i]);
		}
		return result;
	}

	void ModelResource::ConvertAseToMeshes(String fileName)
	{
		AseFile mdl;
		mdl.LoadFromFile(fileName, true);
		auto objName = Path::GetFileNameWithoutEXT(fileName);
		auto basePath = Path::GetDirectoryName(fileName);
		for (auto & obj : mdl.GeomObjects)
		{
			List<Mesh> meshes = LoadAseMesh(*obj->Mesh.Ptr());
			auto includeFile = Path::Combine(basePath, objName + L"_common.drawable");
			auto shaderName = Path::Combine(basePath, objName + L".shader");
			if (!File::Exists(includeFile))
				File::WriteAllText(includeFile, L"");
			if (!File::Exists(shaderName))
				File::WriteAllText(shaderName, L"");
			for (int i = 0; i < meshes.Count(); i++)
			{
				if (meshes[i].VertexData.Count())
				{
					String name;
					name = String(i);
					auto componentName = objName + L"_" + name;
					auto componentFileName = componentName + L".vtx";
					meshes[i].SaveToFile(Path::Combine(basePath, componentFileName));
					auto drawableFileName = componentName + L".drawable";
					StringBuilder sb;
					sb << L"model\n{\n\tfile \"" << componentFileName << L"\"\n}\nshader \"" << objName << L".shader\"\nusing \"" << objName << L"_common.drawable" << L"\"\n";
					auto drawableFile = Path::Combine(basePath, drawableFileName);
					if (!File::Exists(drawableFile))
						File::WriteAllText(drawableFile, sb.ProduceString());
				}
			}
		}
	}

}