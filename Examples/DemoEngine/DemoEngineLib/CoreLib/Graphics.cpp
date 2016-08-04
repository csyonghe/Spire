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
#include "Graphics.h"

/***********************************************************************
ASEFILE.CPP
***********************************************************************/

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace CoreLib::Text;
using namespace VectorMath;

namespace CoreLib
{
	namespace Graphics
	{
		void SkipBlock(Parser & parser)
		{
			while (!parser.IsEnd() && !parser.LookAhead(L"}"))
			{
				auto word = parser.ReadToken();
				if (word.Str == L"{")
					SkipBlock(parser);
			}
			parser.Read(L"}");
		}

		void SkipField(Parser & parser)
		{
			while (!parser.IsEnd() && !parser.LookAhead("*") && !parser.LookAhead(L"}"))
			{
				auto word = parser.ReadToken();
				if (word.Str == L"{")
					SkipBlock(parser);
			}
		}

		void ParseAttributes(EnumerableDictionary<String, AseValue> & attribs, Parser & parser)
		{
			parser.Read(L"*");
			auto fieldName = parser.ReadWord();
			if (parser.NextToken().TypeID == TokenType_StringLiteral)
			{
				attribs[fieldName] = AseValue(parser.ReadStringLiteral());
			}
			else
			{
				Vec4 val;
				val.SetZero();
				int ptr = 0;
				while (!parser.LookAhead(L"*") && !parser.LookAhead(L"}"))
				{
					if (ptr < 4)
					{
						if (parser.NextToken().TypeID == TokenType_Float || parser.NextToken().TypeID == TokenType_Int)
						{
							val[ptr] = (float)parser.ReadDouble();
							ptr++;
						}
						else if (parser.LookAhead(L"{"))
						{
							parser.ReadToken();
							SkipBlock(parser);
						}
						else
							SkipField(parser);
					}
					else
						throw InvalidOperationException(L"Invalid file format: attribute contains more than 4 values.");
				}
				attribs[fieldName] = AseValue(val);
			}
		}

		void ParseAttributeBlock(EnumerableDictionary<String, AseValue> & attribs, Parser & parser)
		{
			parser.Read(L"{");
			while (!parser.LookAhead("}"))
			{
				if (parser.LookAhead(L"*"))
					ParseAttributes(attribs, parser);
				else
					break;
			}
			parser.Read(L"}");
		}

		RefPtr<AseMaterial> ParseSubMaterial(Parser & parser)
		{
			RefPtr<AseMaterial> result = new AseMaterial();
			ParseAttributeBlock(result->Fields, parser);
			AseValue val;
			val.Str = L"noname";
			result->Fields.TryGetValue(L"MATERIAL_NAME", val);
			result->Name = val.Str;
			return result;
		}

		RefPtr<AseMaterial> ParseMaterial(Parser & parser)
		{
			RefPtr<AseMaterial> result = new AseMaterial();
			parser.Read(L"{");
			while (!parser.LookAhead(L"}"))
			{
				parser.Read(L"*");
				auto fieldName = parser.ReadWord();
				if (fieldName == L"SUBMATERIAL")
				{
					parser.ReadInt(); // id
					result->SubMaterials.Add(ParseSubMaterial(parser));
				}
				else if (fieldName == L"MATERIAL_NAME")
				{
					result->Name = parser.ReadStringLiteral();
				}
				else
				{
					parser.Back(2);
					ParseAttributes(result->Fields, parser);
				}
			}
			parser.Read(L"}");
			return result;
		}


		void ParseVertexList(List<Vec3> & verts, Parser & parser)
		{
			parser.Read(L"{");
			while (!parser.LookAhead(L"}"))
			{
				parser.Read(L"*");
				parser.ReadToken(); // name
				parser.ReadToken(); // id
				Vec3 c;
				c.x = (float)parser.ReadDouble();
				c.y = (float)parser.ReadDouble();
				c.z = (float)parser.ReadDouble();
				verts.Add(c);
			}
			parser.Read(L"}");
		}

		void ParseFaceList(List<AseMeshFace> & faces, Parser & parser)
		{
			parser.Read(L"{");
			while (!parser.LookAhead(L"}"))
			{
				AseMeshFace f;
				parser.Read(L"*");
				parser.Read(L"MESH_FACE"); // name
				parser.ReadToken(); // id
				parser.Read(L":");
				parser.Read(L"A"); parser.Read(L":");
				f.Ids[0] = parser.ReadInt();
				parser.Read(L"B"); parser.Read(L":");
				f.Ids[1] = parser.ReadInt();
				parser.Read(L"C"); parser.Read(L":");
				f.Ids[2] = parser.ReadInt();
				while (!parser.LookAhead(L"*") && !parser.LookAhead(L"}"))
					parser.ReadToken();
				while (parser.LookAhead(L"*"))
				{
					parser.Read(L"*");
					auto word = parser.ReadWord();
					if (word == L"MESH_SMOOTHING")
					{
						try
						{
							// comma separated int list
							f.SmoothGroup = 0;
							while (parser.NextToken().TypeID == TokenType_Int)
							{
								f.SmoothGroup |= (1 << parser.ReadInt());
								if (parser.LookAhead(L","))
									parser.ReadToken();
								else
									break;
							}
						}
						catch (Exception)
						{
							f.SmoothGroup = 0;
							parser.Back(1);
						}
					}
					else if (word == L"MESH_MTLID")
						f.MaterialId = parser.ReadInt() - 1;
					else if (word == L"MESH_FACE")
					{
						parser.Back(2);
						break;
					}
				}
				faces.Add(f);
			}
			parser.Read(L"}");
		}

		void ParseAttribFaceList(List<AseMeshAttribFace> & faces, Parser & parser)
		{
			parser.Read(L"{");
			while (!parser.LookAhead(L"}"))
			{
				AseMeshAttribFace f;
				parser.Read(L"*");
				parser.ReadToken(); // name
				parser.ReadToken(); // id
				f.Ids[0] = parser.ReadInt();
				f.Ids[1] = parser.ReadInt();
				f.Ids[2] = parser.ReadInt();
				faces.Add(f);
			}
			parser.Read(L"}");
		}

		RefPtr<AseMesh> ParseMesh(Parser & parser)
		{
			RefPtr<AseMesh> result = new AseMesh();
			parser.Read(L"{");
			while (!parser.LookAhead(L"}"))
			{
				parser.Read(L"*");
				auto fieldName = parser.ReadWord();
				if (fieldName == L"MESH_NUMVERTEX")
				{
					int count = parser.ReadInt();
					result->Vertices.Reserve(count);
				}
				else if (fieldName == L"MESH_NUMFACES")
				{
					int count = parser.ReadInt();
					result->Faces.Reserve(count);
				}
				else if (fieldName == L"MESH_VERTEX_LIST")
				{
					ParseVertexList(result->Vertices, parser);
				}
				else if (fieldName == L"MESH_FACE_LIST")
				{
					ParseFaceList(result->Faces, parser);
				}
				else if (fieldName == L"MESH_NUMCVERTEX")
				{
					result->Colors.Data.Reserve(parser.ReadInt());
				}
				else if (fieldName == L"MESH_NUMCVFACES")
				{
					result->Colors.Faces.Reserve(parser.ReadInt());
				}
				else if (fieldName == L"MESH_CVERTLIST")
				{
					ParseVertexList(result->Colors.Data, parser);
				}
				else if (fieldName == L"MESH_CFACELIST")
				{
					ParseAttribFaceList(result->Colors.Faces, parser);
				}
				else if (fieldName == L"MESH_TVERTLIST")
				{
					result->TexCoords.SetSize(Math::Max(result->TexCoords.Count(), 1));
					ParseVertexList(result->TexCoords[0].Data, parser);
				}
				else if (fieldName == L"MESH_TFACELIST")
				{
					result->TexCoords.SetSize(Math::Max(result->TexCoords.Count(), 1));
					result->TexCoords[0].Faces.Reserve(result->Faces.Count());
					ParseAttribFaceList(result->TexCoords[0].Faces, parser);
				}
				else if (fieldName == L"MESH_NUMTVFACES")
				{
					result->TexCoords.SetSize(Math::Max(result->TexCoords.Count(), 1));
					result->TexCoords[0].Faces.Reserve(parser.ReadInt());
				}
				else if (fieldName == L"MESH_MAPPINGCHANNEL")
				{
					int channelId = parser.ReadInt();
					result->TexCoords.SetSize(Math::Max(channelId, result->TexCoords.Count()));
					auto & texCoords = result->TexCoords[channelId - 1];
					parser.Read(L"{");
					while (!parser.LookAhead(L"}"))
					{
						parser.Read(L"*");
						auto word = parser.ReadWord();
						if (word == L"MESH_NUMTVERTEX")
						{
							texCoords.Data.Reserve(parser.ReadInt());
						}
						else if (word == L"MESH_NUMTVFACES")
						{
							texCoords.Faces.Reserve(parser.ReadInt());
						}
						else if (word == L"MESH_TVERTLIST")
						{
							ParseVertexList(texCoords.Data, parser);
						}
						else if (word == L"MESH_TFACELIST")
						{
							ParseAttribFaceList(texCoords.Faces, parser);
						}
						else
							SkipField(parser);
					}
					parser.Read(L"}");
				}
				else
				{
					SkipField(parser);
				}
			}
			parser.Read(L"}");
			return result;
		}

		RefPtr<AseGeomObject> ParseGeomObject(Parser & parser)
		{
			RefPtr<AseGeomObject> result = new AseGeomObject();
			parser.Read(L"{");
			while (!parser.LookAhead(L"}"))
			{
				parser.Read(L"*");
				auto fieldName = parser.ReadWord();
				if (fieldName == L"NODE_NAME")
				{
					result->Attributes[fieldName] = AseValue(parser.ReadStringLiteral());
				}
				else if (fieldName == L"NODE_TM")
				{
					ParseAttributeBlock(result->Attributes, parser);
				}
				else if (fieldName == L"MESH")
				{
					result->Mesh = ParseMesh(parser);
				}
				else if (fieldName == L"MATERIAL_REF")
				{
					result->MaterialId = parser.ReadInt();
				}
				else
					SkipField(parser);
			}
			parser.Read(L"}");
			return result;
		}

		void AseFile::Parse(const String & content, bool flipYZ)
		{
			Parser parser(content);
			while (!parser.IsEnd())
			{
				parser.Read(L"*");
				auto fieldName = parser.ReadToken().Str;
				if (fieldName == L"MATERIAL_LIST")
				{
					parser.Read(L"{");
					while (!parser.LookAhead(L"}"))
					{
						parser.Read(L"*");
						auto subField = parser.ReadWord();
						if (subField == L"MATERIAL_COUNT")
						{
							int count = parser.ReadInt();
							Materials.SetSize(count);
						}
						else if (subField == L"MATERIAL")
						{
							int id = parser.ReadInt();
							if (id >= Materials.Count())
								Materials.SetSize(id + 1);
							Materials[id] = ParseMaterial(parser);
						}
						else
							SkipField(parser);
					}
					parser.Read(L"}");
				}
				else if (fieldName == L"GEOMOBJECT")
				{
					GeomObjects.Add(ParseGeomObject(parser));
				}
				else
					SkipField(parser);
			}
			if (flipYZ)
			{
				for (auto & obj : GeomObjects)
				{
					for (auto & vert : obj->Mesh->Vertices)
					{
						auto tmp = vert.z;
						vert.z = -vert.y;
						vert.y = tmp;
					}
				}
			}
		}

		void AseFile::LoadFromFile(const String & fileName, bool flipYZ)
		{
			Parse(File::ReadAllText(fileName), flipYZ);
		}

		void AseMesh::ConstructPerVertexFaceList(Basic::List<int> & faceCountAtVert, Basic::List<int> & vertFaceList) const
		{
			faceCountAtVert.SetSize(Vertices.Count());
			for (int i = 0; i < faceCountAtVert.Count(); i++)
				faceCountAtVert[i] = 0;
			for (int i = 0; i<Faces.Count(); i++)
			{
				faceCountAtVert[Faces[i].Ids[0]]++;
				faceCountAtVert[Faces[i].Ids[1]]++;
				faceCountAtVert[Faces[i].Ids[2]]++;
			}
			int scan = 0;
			for (int i = 0; i<faceCountAtVert.Count(); i++)
			{
				int s = faceCountAtVert[i];
				faceCountAtVert[i] = scan;
				scan += s;
			}
			vertFaceList.SetSize(scan);
			for (int i = 0; i<Faces.Count(); i++)
			{
				vertFaceList[faceCountAtVert[Faces[i].Ids[0]]++] = i;
				vertFaceList[faceCountAtVert[Faces[i].Ids[1]]++] = i;
				vertFaceList[faceCountAtVert[Faces[i].Ids[2]]++] = i;
			}
		}

		void AseMesh::RecomputeNormals()
		{
			Normals.Data.Clear();
			Normals.Faces.SetSize(Faces.Count());
			Dictionary<Vec3, int> normalMap;
			List<Vec3> faceNormals;
			faceNormals.SetSize(Faces.Count());
			for (int i = 0; i<Faces.Count(); i++)
			{
				Vec3 v1 = Vertices[Faces[i].Ids[0]];
				Vec3 v2 = Vertices[Faces[i].Ids[1]];
				Vec3 v3 = Vertices[Faces[i].Ids[2]];
				Vec3 ab, ac;
				Vec3::Subtract(ab, v2, v1);
				Vec3::Subtract(ac, v3, v1);
				Vec3 n;
				Vec3::Cross(n, ab, ac);
				float len = n.Length();
				if (len > 1e-6f)
					Vec3::Scale(n, n, 1.0f / len);
				else
					n = Vec3::Create(1.0f, 0.0f, 0.0f);
				if (n.Length() > 1.2f || n.Length() < 0.5f)
					n = Vec3::Create(1.0f, 0.0f, 0.0f);
				faceNormals[i] = n;
			}
			List<int> vertShare;
			List<int> vertFaces;
			ConstructPerVertexFaceList(vertShare, vertFaces);
			int start = 0;
			for (int i = 0; i<Faces.Count(); i++)
			{
				auto & face = Faces[i];
				auto & nface = Normals.Faces[i];
				for (int j = 0; j < 3; j++)
				{
					int vid = face.Ids[j];
					if (vid == -1)
						continue;
					if (vid == 0)
						start = 0;
					else
						start = vertShare[vid - 1];
					int count = 0;
					Vec3 n;
					n.SetZero();
					for (int k = start; k < vertShare[vid]; k++)
					{
						int fid = vertFaces[k];
						if (Faces[fid].SmoothGroup & face.SmoothGroup)
						{
							Vec3::Add(n, faceNormals[fid], n);
							count++;
						}
					}
					if (count == 0)
						n = faceNormals[i];
					else
					{
						Vec3::Scale(n, n, 1.0f / count);
						if (n.Length() < 0.5f)
						{
							n = faceNormals[i];
						}
						else
							n = n.Normalize();
					}
					if (!normalMap.TryGetValue(n, nface.Ids[j]))
					{
						Normals.Data.Add(n);
						nface.Ids[j] = Normals.Data.Count() - 1;
						normalMap[n] = Normals.Data.Count()-1;
					}
				}
			}
		}
	}
}

/***********************************************************************
BBOX.CPP
***********************************************************************/

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

/***********************************************************************
BEZIERMESH.CPP
***********************************************************************/

namespace CoreLib
{
	namespace Graphics
	{
		class MeshPatch
		{
		public:
			//int VertCount;
			// All ptrs are indices into VertIds array
			int VertPtr;
			int RingVertCount;
			bool IsBoundary[4];
			int NeighborPtr[4];
			// FanSize: number of ring vertices between boundaries
			// NeighborPtr[i] + NeighborFanSize[i] may be less than NeighborPtr[i+1]
			// for boundaries verts. The rest space is reserved for backward-fan.
			int NeighborFanSize[4]; 
			int TexCoordId[4];
		};

		class InvalidMeshTopologyException : public Exception
		{
		public:
			InvalidMeshTopologyException(String message)
				: Exception(message)
			{}
		};

		class PatchedMesh
		{
		public:
			List<MeshPatch> Patches;
			List<int> VertIds;
			List<int> Valances;
			List<Vec3> Vertices;
			List<Vec2> TexCoords;
		private:
			static bool IsAdjacentFace(ObjFace & face0, ObjFace & face1)
			{
				int sharedVerts = 0;
				for (int i = 0; i<4; i++)
					for (int j = 0; j<4; j++)
						if (face0.VertexIds[i] == face1.VertexIds[j])
						{
							sharedVerts ++;
							if (sharedVerts == 2)
								return true;
						}
				return false;
			}
		public:
			struct RingVertex
			{
				int Id;
				float Angle;
			};
			struct Edge
			{
				int V0, V1;
				inline bool operator == (const Edge & other) const
				{
					return V0 == other.V0 && V1 == other.V1;
				}
				inline int GetHashCode() const
				{
					return (V0<<16) + V1;
				}
			};
			struct EdgeFaces
			{
				int Faces[2];
				char VId0[2], VId1[2];
			};
			static void ComputeTopology(ObjModel &mdl, Dictionary<Edge, EdgeFaces> &edgeFaces, List<int> &valences)
			{
				valences.SetSize(mdl.Vertices.Count());
				for (int i = 0; i<valences.Count(); i++)
					valences[i] = 0;
				for (int i = 0; i<mdl.Faces.Count(); i++)
				{
					for (int j = 0; j<4; j++)
					{
						int ev0 = j;
						int ev1 = (j+1)&3;
						Edge e;
						e.V0 = mdl.Faces[i].VertexIds[ev0];
						e.V1 = mdl.Faces[i].VertexIds[ev1];	
						if (e.V0 == -1)
							throw InvalidMeshTopologyException(L"The input model is not a quad mesh.");
						if (e.V0 > e.V1)
						{
							Swap(e.V0, e.V1);
							Swap(ev0, ev1);
						}
						EdgeFaces ef;
						if (edgeFaces.TryGetValue(e, ef))
						{
							if (ef.Faces[1] == -1)
							{
								ef.Faces[1] = i;
								ef.VId0[1] = (char)ev0;
								ef.VId1[1] = (char)ev1;
							}
							else
								throw InvalidMeshTopologyException(L"Invalid mesh topology: an edge is shared by more than two faces.");
						}
						else
						{
							ef.Faces[0] = i; ef.Faces[1] = -1;
							ef.VId0[0] = (char)ev0; ef.VId0[1] = -1;
							ef.VId1[0] = (char)ev1; ef.VId1[1] = -1;
							valences[e.V0]++;
							valences[e.V1]++;
						}
						edgeFaces[e] = ef;
					}
				}
			}
			template<typename Func>
			static bool TraverseFaceCCW(ObjModel &mdl, Dictionary<Edge, EdgeFaces> &edgeFaces, int fid, int fvid, Func f)
			{
				ObjFace *curFace = &mdl.Faces[fid];
				int curVid = fvid;
				int curVid1 = (curVid+3)&3;
				int curFid = fid;
				int vid = curFace->VertexIds[fvid];
#pragma warning (suppress:4127)
				while (true)
				{
					Edge e;
					e.V0 = curFace->VertexIds[curVid];
					e.V1 = curFace->VertexIds[curVid1];
					int edgeV1 = e.V1;
					if (e.V0 > e.V1)
					{
						Swap(e.V0, e.V1);
					}
					EdgeFaces ef;
					if (!edgeFaces.TryGetValue(e, ef))
						return true;
					if (ef.Faces[0] == curFid)
						curFid = ef.Faces[1];
					else
						curFid = ef.Faces[0];
					if (curFid == -1)
						return true;
					curFace = &mdl.Faces[curFid];
					for (int i = 0; i<4; i++)
						if (curFace->VertexIds[i] == vid)
						{
							curVid = i;
							curVid1 = (curVid+1)&3;
							if (curFace->VertexIds[curVid1] == edgeV1)
								curVid1 = (curVid+3)&3;
							break;
						}
					if (curFid == fid)
						return false;
					f(curFid, curVid, curVid1);
				}
			}
			static bool IsBoundaryVertex(ObjModel &mdl, Dictionary<Edge, EdgeFaces> &edgeFaces, int fid, int fvid)
			{
				return TraverseFaceCCW(mdl, edgeFaces, fid, fvid, [](int /*a*/, int /*b*/, int /*c*/){});
			}
			static int GetOppositeVertexId(ObjFace & face, int v0, int v1)
			{
				int id = 0;
				for (int l = 0; l<4; l++)
					if (face.VertexIds[l] == v0)
					{
						id = l;
						break;
					}
				if (face.VertexIds[(id+1)&3] == v1)
					return face.VertexIds[(id+3)&3];
				else
					return face.VertexIds[(id+1)&3];
			}
			static PatchedMesh FromObj(ObjModel &mdl)
			{
				PatchedMesh mesh;
				List<int> vertFaceListOffset, facesAtVert;
				Dictionary<Edge, EdgeFaces> edgeFaces;
				List<Vec3> vertexNormals;
				List<Vec3> faceNormals;
				faceNormals.SetSize(mdl.Faces.Count());
				for (int i = 0; i<mdl.Faces.Count(); i++)
				{
					auto & face = mdl.Faces[i];
					Vec3 faceNormal;
					Vec3::Cross(faceNormal, mdl.Vertices[face.VertexIds[1]]-mdl.Vertices[face.VertexIds[0]],
						mdl.Vertices[face.VertexIds[2]]-mdl.Vertices[face.VertexIds[0]]);
					faceNormals[i] = faceNormal.Normalize();
				}
				ComputeTopology(mdl, edgeFaces, mesh.Valances);
				mdl.ConstructPerVertexFaceList(vertFaceListOffset, facesAtVert);
				vertexNormals.SetSize(mdl.Vertices.Count());

				for (int i = 0; i<mdl.Vertices.Count(); i++)
				{
					Vec3 vnormal = Vec3::Create(0.0f, 0.0f, 0.0f);
					int start = i==0?0:vertFaceListOffset[i-1];
					for (int j = start; j<vertFaceListOffset[i]; j++)
					{
						vnormal += faceNormals[facesAtVert[j]];
					}
					vnormal *= 1.0f/(vertFaceListOffset[i]-start);
					vertexNormals[i] = vnormal.Normalize();
				}
				HashSet<int> vertSet;
				List<RingVertex> ringPoints;

				mesh.Vertices = mdl.Vertices;
				for (int i = 0; i < mdl.Faces.Count(); i++)
				{
					auto face = mdl.Faces[i];
					MeshPatch patch;
					patch.VertPtr = mesh.VertIds.Count();
					int adjacentFaces[4];
					for (int j = 0; j<4; j++)
					{
						mesh.VertIds.Add(face.VertexIds[j]);
						int j1 = (j+3)&3;
						Edge e;
						e.V0 = face.VertexIds[j];
						e.V1 = face.VertexIds[j1];
						if (e.V0 > e.V1) Swap(e.V0, e.V1);
						EdgeFaces ef;
						ef.Faces[0] = ef.Faces[1] = -1;
						edgeFaces.TryGetValue(e, ef);
						if (ef.Faces[0] == i)
							adjacentFaces[j] = ef.Faces[1];
						else
							adjacentFaces[j] = ef.Faces[0];
					}
					patch.RingVertCount = 0;
					for (int j = 0; j < 4; j++)
					{
						auto _AppendRingVertex = [&](int vertId)
						{
							RingVertex rv;
							rv.Id = vertId;
							ringPoints.Add(rv);
						};
						ringPoints.Clear();
						vertSet.Clear();
						patch.IsBoundary[j] = TraverseFaceCCW(mdl, edgeFaces, i, j, [&](int faceId, int ev0, int /*ev1*/)
						{
							if (faceId == adjacentFaces[j])
							{
								_AppendRingVertex(GetOppositeVertexId(mdl.Faces[faceId], face.VertexIds[j], face.VertexIds[(j+3)&3]));
								return;
							}
							else
							{
								if (faceId == i) return;
								if (faceId == adjacentFaces[0]) return;
								if (faceId == adjacentFaces[1]) return;
								if (faceId == adjacentFaces[2]) return;
								if (faceId == adjacentFaces[3]) return;
							}
							_AppendRingVertex(mdl.Faces[faceId].VertexIds[(ev0+2)&3]);
							_AppendRingVertex(mdl.Faces[faceId].VertexIds[(ev0+3)&3]);
						});
						bool outFanAppended = false;
						if (patch.IsBoundary[j])
						{
							// for boundary vertices, we cannot traverse through topology graph
							// however sort all vertices ccw regarding to vertex normal should 
							// give us correct result for most cases.
							ringPoints.Clear();
							vertSet.Clear();
							Vec3 p0 = mesh.Vertices[face.VertexIds[j]];
							Vec3 u = mesh.Vertices[face.VertexIds[(j+3)&3]] - p0;
							Vec3 v;
							Vec3 vertNormal = vertexNormals[face.VertexIds[j]];
							Vec3::Cross(v, vertNormal, u);
							v = v.Normalize();
							Vec3::Cross(u, v, vertNormal);
							int vertId = face.VertexIds[j];
							int faceStart = (vertId == 0)?0:vertFaceListOffset[vertId-1];
							int faceEnd = vertFaceListOffset[vertId];
							auto _AppendRingVertex = [&](int vertId)
							{
								if (!vertSet.Contains(vertId))
								{
									vertSet.Add(vertId);
									Vec3 p = mesh.Vertices[vertId];
									Vec3 vp = p - p0;
									float ax = Vec3::Dot(vp, u);
									float ay = Vec3::Dot(vp, v);
									RingVertex rv;
									rv.Id = vertId;
									rv.Angle = atan2(ay, ax);
									if (rv.Angle < 0)
										rv.Angle += PI * 2.0f;
									ringPoints.Add(rv);
								}
							};
							for (int k = faceStart; k<faceEnd; k++)
							{
								int nfaceId = facesAtVert[k];
								auto nface = mdl.Faces[nfaceId];
								if (nfaceId == adjacentFaces[j])
								{
									_AppendRingVertex(GetOppositeVertexId(nface, face.VertexIds[j], face.VertexIds[(j+3)&3]));
									continue;
								}
								else
								{
									if (nfaceId == i) continue;
									if (nfaceId == adjacentFaces[0] ||
										nfaceId == adjacentFaces[1] ||
										nfaceId == adjacentFaces[2] ||
										nfaceId == adjacentFaces[3]) 
									{
										outFanAppended = true;
										_AppendRingVertex(GetOppositeVertexId(nface, face.VertexIds[j], face.VertexIds[(j+1)&3]));
										continue;
									}
								}
								for (int l = 0; l<4; l++)
								{
									if (nface.VertexIds[l] != vertId)
									{
										_AppendRingVertex(nface.VertexIds[l]);
									}
								}
							}
							std::sort(ringPoints.Buffer(), ringPoints.Buffer()+ringPoints.Count(), [](const RingVertex & p0, const RingVertex & p1)
							{
								return p0.Angle < p1.Angle;
							});
						}
						patch.TexCoordId[j] = face.TexCoordIds[j];
						patch.NeighborPtr[j] = mesh.VertIds.Count();
						for (int k = 0; k<ringPoints.Count(); k++)
							mesh.VertIds.Add(ringPoints[k].Id);
						patch.RingVertCount += ringPoints.Count();
						patch.NeighborFanSize[j] = ringPoints.Count();
						if (outFanAppended)
							patch.NeighborFanSize[j]--;
					}
					mesh.Patches.Add(patch);
				}
				return mesh;
			}
		};

		class TangentWeights
		{
		private:
			static bool initialized;
			static const int MaxPrecomputedValence = 15;
			static float alphaWeights[MaxPrecomputedValence+1][MaxPrecomputedValence+1];
			static float betaWeights[MaxPrecomputedValence+1][MaxPrecomputedValence+1];
		public:
			static void Initialize()
			{
				if (initialized)
					return;
				for (int n=1; n<=MaxPrecomputedValence; n++)
				{
					float oneOverN = 1.0f / n;
					for (int i=0; i<n; i++)
					{
						float cosPiOverN = cos( Math::Pi * oneOverN );
						float oneOverDenom = 1.0f / ((float)n * sqrt(4.0f + pow(cosPiOverN, 2.0f)));

						alphaWeights[n][i] = oneOverN + cosPiOverN * oneOverDenom;
						alphaWeights[n][i] *= cos( (2.0f * Math::Pi * (float)i) * oneOverN);

						betaWeights[n][i] = oneOverDenom * cos( (2.0f * Math::Pi * (float)i + Math::Pi) * oneOverN);
					}
				}
				initialized = true;
			}

			static inline float Alpha(int n, int i)
			{
				if (n < MaxPrecomputedValence)
					return alphaWeights[n][i];
				else
				{
					float oneOverN = 1.0f / n;
					float cosPiOverN = cos(Math::Pi * oneOverN);
					float rs = oneOverN + cosPiOverN / ((float)n * sqrt(4.0f + pow(cosPiOverN, 2.0f)));
					return rs * cos( (2.0f * Math::Pi * (float)i) * oneOverN);
				}
			}

			static inline float Beta(int n, int i)
			{
				if (n < MaxPrecomputedValence)
					return betaWeights[n][i];
				else
				{
					float oneOverN = 1.0f / n;
					float cosPiOverN = cos( Math::Pi * oneOverN );
					float oneOverDenom = 1.0f / ((float)n * sqrt(4.0f + pow(cosPiOverN, 2.0f)));
					return oneOverDenom * cos( (2.0f * Math::Pi * (float)i + Math::Pi) * oneOverN);
				}
			}
		};
		bool TangentWeights::initialized = false;
		float TangentWeights::alphaWeights[MaxPrecomputedValence+1][MaxPrecomputedValence+1];
		float TangentWeights::betaWeights[MaxPrecomputedValence+1][MaxPrecomputedValence+1];

		void GenerateIteriorPoints(Vec3 points[], PatchedMesh & mesh, MeshPatch & patch)
		{
			for (int i=0; i<4; i++)
			{
				int iA, iB, iC, iD;
				iA = ((i+3)&3) + patch.VertPtr;
				iB = ((i+2)&3) + patch.VertPtr;
				iC = i + patch.VertPtr;
				iD = ((i+1)&3) + patch.VertPtr;
				int valence = mesh.Valances[mesh.VertIds[patch.VertPtr + i]];
				if (patch.IsBoundary[i])
				{
					if (valence == 2)
					{
						// Case 1: Boundary vertex contained in only 1 quad. The
						// interior point mask is:
						//
						// 2 --- 1
						// |     |
						// 4 --- 2
						float scale = 1.f / 9.f;
						points[i] =   mesh.Vertices[mesh.VertIds[iC]] * 4.0f;
						points[i] += (mesh.Vertices[mesh.VertIds[iA]] + mesh.Vertices[mesh.VertIds[iD]]) * 2.0f;
						points[i] +=  mesh.Vertices[mesh.VertIds[iB]];
						points[i] *=  scale;
					}
					else
					{
						// Case 2: Boundary vertex contained in K quads.  The interior
						// point mask is:
						// 2 --- 1
						// |     |
						// 2K -- 2
						int k = valence - 1;
						float scale = 1.0f / (float)(2 * k + 5);
						points[i] =   mesh.Vertices[mesh.VertIds[iC]] * (float)(k*2);
						points[i] += (mesh.Vertices[mesh.VertIds[iA]] + mesh.Vertices[mesh.VertIds[iD]]) * 2.0f;
						points[i] +=  mesh.Vertices[mesh.VertIds[iB]];
						points[i] *=  scale;
					}
				}
				else
				{
					// Case 3: Non-boundary vertex of valence N. The interior point
					// mask is:
					//
					// 2 --- 1
					// |     |
					// N --- 2
					float scale = 1.0f / (float)(valence + 5);
					points[i] = mesh.Vertices[mesh.VertIds[iC]] * (float)valence;
					points[i] += (mesh.Vertices[mesh.VertIds[iA]] + mesh.Vertices[mesh.VertIds[iD]]) * 2.0f;
					points[i] += mesh.Vertices[mesh.VertIds[iB]];
					points[i] *= scale;
				}
			}
		}

		void GenerateEdgePoints(Vec3 points[2][4], PatchedMesh & mesh, MeshPatch & patch)
		{
			for (int i=0; i<4; i++)
			{
				if (patch.IsBoundary[i] && patch.IsBoundary[(i+1)&3])
				{
					// Case 1: Boundary edge.  The edge points are just a weighted
					// combination of the two mesh vertices forming the edge

					int iA = patch.VertPtr + i;
					int iB = patch.VertPtr + ((i+1)&3);
					float scale = 1.0f / 3.0f;

					points[0][i]  = mesh.Vertices[mesh.VertIds[iA]] * 2.0f;
					points[0][i] += mesh.Vertices[mesh.VertIds[iB]];
					points[0][i] *= scale;

					points[1][i]  = mesh.Vertices[mesh.VertIds[iA]];
					points[1][i] += mesh.Vertices[mesh.VertIds[iB]] * 2.0f;
					points[1][i] *= scale;
				}
				else
				{
					int iA, iB, iC, iD;
					iA = (i+3)&3;
					iB = (i+2)&3;
					iC = i;
					iD = (i+1)&3;
					int iE = (patch.NeighborPtr[iD] == patch.VertPtr+4) ? patch.NeighborPtr[0]+patch.RingVertCount-1 : patch.NeighborPtr[iD]-1;
					int iF = patch.NeighborPtr[iD];

					iA += patch.VertPtr;
					iB += patch.VertPtr;
					iC += patch.VertPtr;
					iD += patch.VertPtr;
					int valence = mesh.Valances[mesh.VertIds[iC]];
					float bigWeight = (float)(2 * valence);
					float scale = 1.0f / (bigWeight + 10.0f);

					points[0][i] =   mesh.Vertices[mesh.VertIds[iC]] * bigWeight;
					points[0][i] +=  mesh.Vertices[mesh.VertIds[iD]] * 4.0f;
					points[0][i] += (mesh.Vertices[mesh.VertIds[iA]] + mesh.Vertices[mesh.VertIds[iE]]) * 2.0f;
					points[0][i] +=  mesh.Vertices[mesh.VertIds[iB]] + mesh.Vertices[mesh.VertIds[iF]];
					points[0][i] *=  scale;

					valence = mesh.Valances[mesh.VertIds[iD]];
					bigWeight = (float)(2 * valence);
					scale = 1.0f / (bigWeight + 10.0f);

					points[1][i] =   mesh.Vertices[mesh.VertIds[iC]] * 4.0f;
					points[1][i] +=  mesh.Vertices[mesh.VertIds[iD]] * bigWeight;
					points[1][i] += (mesh.Vertices[mesh.VertIds[iB]] + mesh.Vertices[mesh.VertIds[iF]]) * 2.0f;
					points[1][i] +=  mesh.Vertices[mesh.VertIds[iA]] + mesh.Vertices[mesh.VertIds[iE]];
					points[1][i] *=  scale;
				}
			}
		}

		void GenerateCornerPoints(Vec3 points[], PatchedMesh & mesh, MeshPatch & patch)
		{
			for (int i=0; i<4; i++)
			{
				int valence = mesh.Valances[mesh.VertIds[patch.VertPtr + i]];
				if (patch.IsBoundary[i])
				{
					if (valence == 2)
					{
						// Case 1: The vertex is on the boundary and only belongs to 1
						// face. The corner bezier patch control point is the vertex.
						points[i] = mesh.Vertices[mesh.VertIds[patch.VertPtr + i]];
					}
					else
					{
						// Case 2: The vertex is on the boundary and belongs to
						// multiple faces.  The current vertex is the corner
						// corresponding to iB.  iA and iC are neighboring vertices
						// along the boundary.  Note that they may or may not be part
						// of the current face.  The mask is:
						//
						// iA -- iB -- iC
						//
						// 1 --- 4 --- 1

						int  fanEndOffset = (i == 3) ? patch.VertPtr+4+patch.RingVertCount : patch.NeighborPtr[i+1];
						bool hasFan1 = (patch.NeighborFanSize[i] > 0);
						bool hasFan2 = (patch.NeighborPtr[i] + patch.NeighborFanSize[i] < fanEndOffset);
						int iA = (hasFan1) ? (patch.NeighborPtr[i] + patch.NeighborFanSize[i] - 1) : patch.VertPtr + ((i+3)&3);
						int iB = patch.VertPtr + i;
						int iC = (hasFan2) ? (patch.NeighborPtr[i] + patch.NeighborFanSize[i]) : patch.VertPtr + ((i+1)&3);
						float scale = 1.f / 6.f;
						points[i] =  mesh.Vertices[mesh.VertIds[iB]] * 4.0f;
						points[i] += mesh.Vertices[mesh.VertIds[iA]];
						points[i] += mesh.Vertices[mesh.VertIds[iC]];
						points[i] *= scale;
					}
				}
				else
				{
					int iA, iB, iC, iD;
					iA = i;
					iB = (i+1)&3;
					iC = (i+2)&3;
					iD = (i+3)&3;
					int iE = (patch.NeighborPtr[i] == patch.VertPtr+4) ? patch.NeighborPtr[0]+patch.RingVertCount-1 : patch.NeighborPtr[i]-1;
					int iF = patch.NeighborPtr[iB];
					iA += patch.VertPtr;
					iB += patch.VertPtr;
					iC += patch.VertPtr;
					iD += patch.VertPtr;
					
					int base = patch.NeighborPtr[i];
					int extraRingVerts = 2 * (valence - 3) + 1;

					points[i] =   mesh.Vertices[mesh.VertIds[iA]] * (float)(valence * valence);
					points[i] += (mesh.Vertices[mesh.VertIds[iB]] + mesh.Vertices[mesh.VertIds[iD]]) * 4.0f;
					points[i] +=  mesh.Vertices[mesh.VertIds[iC]] + mesh.Vertices[mesh.VertIds[iE]] + mesh.Vertices[mesh.VertIds[iF]];

					for (int j=0; j<extraRingVerts; j++)
					{
						float weight = (j&1) ? 1.0f : 4.0f;
						points[i] += mesh.Vertices[mesh.VertIds[base+j]] * weight;
					}

					float scale = 1.0f / (float)( valence*valence + (valence*5) );
					points[i] *= scale;
				}
			}
		}

		void FixCorners(PatchedMesh & mesh, MeshPatch & patch, Vec3 ducp[3][4], Vec3 dvcp[3][4])
		{
			// maps corner [0-3] to indices into ducp and dvcp
			const int duIdx[4][2] = { {0,0}, {2,0}, {2,3}, {0,3} };
			const int dvIdx[4][2] = { {0,0}, {0,3}, {2,3}, {2,0} };

			for (int i=0; i<4; i++)
			{
				Vec3 du = Vec3::Create(0, 0, 0);
				Vec3 dv = Vec3::Create(0, 0, 0);
				int duStart, dvStart;

				int valence = mesh.Valances[mesh.VertIds[patch.VertPtr + i]];
				int shift = valence - 1;

				if (i&1)
				{
					du += mesh.Vertices[mesh.VertIds[patch.VertPtr + ((i+3)&3)]] * TangentWeights::Alpha(valence, 0);
					du += mesh.Vertices[mesh.VertIds[patch.VertPtr + ((i+1)&3)]] * TangentWeights::Alpha(valence, shift);
					du += mesh.Vertices[mesh.VertIds[patch.VertPtr + ((i+2)&3)]] * TangentWeights::Beta(valence, shift);

					dv += mesh.Vertices[mesh.VertIds[patch.VertPtr + ((i+1)&3)]] * TangentWeights::Alpha(valence, 0);
					dv += mesh.Vertices[mesh.VertIds[patch.VertPtr + ((i+2)&3)]] * TangentWeights::Beta(valence, 0);
					dv += mesh.Vertices[mesh.VertIds[patch.VertPtr + ((i+3)&3)]] * TangentWeights::Alpha(valence, 1);

					duStart = 0;
					dvStart = 1;
				}
				else
				{
					du += mesh.Vertices[mesh.VertIds[patch.VertPtr + ((i+1)&3)]] * TangentWeights::Alpha(valence, 0);
					du += mesh.Vertices[mesh.VertIds[patch.VertPtr + ((i+2)&3)]] * TangentWeights::Beta(valence, 0);
					du += mesh.Vertices[mesh.VertIds[patch.VertPtr + ((i+3)&3)]] * TangentWeights::Alpha(valence, 1);

					dv += mesh.Vertices[mesh.VertIds[patch.VertPtr + ((i+3)&3)]] * TangentWeights::Alpha(valence, 0);
					dv += mesh.Vertices[mesh.VertIds[patch.VertPtr + ((i+1)&3)]] * TangentWeights::Alpha(valence, shift);
					dv += mesh.Vertices[mesh.VertIds[patch.VertPtr + ((i+2)&3)]] * TangentWeights::Beta(valence, shift);

					duStart = 1;
					dvStart = 0;
				}

				int ringVert;
				int extraRingFaces = valence - 3;

				ringVert = patch.NeighborPtr[(i+3)&3] + patch.NeighborFanSize[(i+3)&3] - 1;
				du += mesh.Vertices[mesh.VertIds[ringVert]] * TangentWeights::Beta(valence, duStart);
				dv += mesh.Vertices[mesh.VertIds[ringVert]] * TangentWeights::Beta(valence, dvStart);

				ringVert = patch.NeighborPtr[(i+1)&3];
				du += mesh.Vertices[mesh.VertIds[ringVert]] * TangentWeights::Beta(valence, duStart + extraRingFaces + 1);
				dv += mesh.Vertices[mesh.VertIds[ringVert]] * TangentWeights::Beta(valence, dvStart + extraRingFaces + 1);

				ringVert = patch.NeighborPtr[i];
				du += mesh.Vertices[mesh.VertIds[ringVert]] * TangentWeights::Alpha(valence, duStart + 1);
				dv += mesh.Vertices[mesh.VertIds[ringVert]] * TangentWeights::Alpha(valence, dvStart + 1);

				for (int j=0; j<extraRingFaces; j++)
				{
					ringVert++;
					du += mesh.Vertices[mesh.VertIds[ringVert]] * TangentWeights::Beta(valence, duStart + j + 1);
					dv += mesh.Vertices[mesh.VertIds[ringVert]] * TangentWeights::Beta(valence, dvStart + j + 1);

					ringVert++;
					du += mesh.Vertices[mesh.VertIds[ringVert]] * TangentWeights::Alpha(valence, duStart + j + 2);
					dv += mesh.Vertices[mesh.VertIds[ringVert]] * TangentWeights::Alpha(valence, dvStart + j + 2);
				}

				ducp[duIdx[i][0]][duIdx[i][1]] = du;
				dvcp[dvIdx[i][0]][dvIdx[i][1]] = dv;
			}

			ducp[2][0] *= -1.f;
			ducp[2][3] *= -1.f;
			dvcp[2][0] *= -1.f;
			dvcp[2][3] *= -1.f;
		}

		void FixEdges(PatchedMesh & mesh, MeshPatch & patch, Vec3 ducp[3][4], Vec3 dvcp[3][4], Vec3 cp[4][4])
		{
			Vec3 v;

			float c0, c1;
			float twoPi = 2.0f * Math::Pi;

			// Assume the patch has (u,v)=(0,0) at bottom left and (1,0) at
			// bottom right, and sides labeled like this:
			// ---t---
			// l     r
			// ---b---

			// du across left edge -----------------------------------------------
			c0 = cos(twoPi / (float)(mesh.Valances[mesh.VertIds[patch.VertPtr + 3]]));
			c1 = cos(twoPi / (float)(mesh.Valances[mesh.VertIds[patch.VertPtr]]));
			v = ((dvcp[1][0] * 2.0f * c1) - (dvcp[0][0] * c0) ) * (1.0f / 3.0f);
			v += (cp[1][1] - cp[1][0]) * 3.0f;
			ducp[0][1] = v;

			v =  ( (dvcp[2][0] * c1) - (dvcp[1][0] * 2.0f * c0) ) * (1.0f / 3.0f);
			v += (cp[2][1] - cp[2][0]) * 3.0f;
			ducp[0][2] = v;

			// du across right edge -------------------------------------------------
			c0 = cos(twoPi / (float)(mesh.Valances[mesh.VertIds[patch.VertPtr + 1]]));
			c1 = cos(twoPi / (float)(mesh.Valances[mesh.VertIds[patch.VertPtr + 2]]));

			v = ( (dvcp[1][3] * 2.0f * c0) - (dvcp[0][3] * c1) ) * (1.0f / 3.0f);
			v += (cp[1][2] - cp[1][3]) * 3.0f;
			ducp[2][1] = v * -1.0f;

			v =  ( (dvcp[2][3] * c0) - (dvcp[1][3] * 2.0f * c1) ) * (1.0f / 3.0f);
			v += (cp[2][2] - cp[2][3]) * 3.f;
			ducp[2][2] = v * -1.f;

			// dv across bottom edge -----------------------------------------------
			c0 = cos( twoPi / (float)(mesh.Valances[mesh.VertIds[patch.VertPtr + 1]]) );
			c1 = cos( twoPi / (float)(mesh.Valances[mesh.VertIds[patch.VertPtr]]) );
			v = ( (ducp[1][0] * 2.f * c1) - (ducp[0][0] * c0) ) * (1.0f / 3.0f);
			v += (cp[1][1] - cp[0][1]) * 3.0f;
			dvcp[0][1] = v;

			v =  ( (ducp[2][0] * c1) - (ducp[1][0] * 2.0f * c0) ) * (1.0f / 3.0f);
			v += (cp[1][2] - cp[0][2]) * 3.0f;
			dvcp[0][2] = v;

			// dv across top edge --------------------------------------------------
			c0 = cos( twoPi / (float)(mesh.Valances[mesh.VertIds[patch.VertPtr + 3]]) );
			c1 = cos( twoPi / (float)(mesh.Valances[mesh.VertIds[patch.VertPtr + 2]]) );
			v = ( (ducp[1][3] * 2.0f * c0) - (ducp[0][3] * c1) ) * (1.0f / 3.0f);
			v += (cp[2][1] - cp[3][1]) * 3.0f;
			dvcp[2][1] = v * -1.0f;

			v =  ( (ducp[2][3] * c0) - (ducp[1][3] * 2.0f * c1) ) * (1.0f / 3.0f);
			v += (cp[2][2] - cp[3][2]) * 3.0f;
			dvcp[2][2] = v * -1.0f;
		}

		BezierMesh BezierMeshFromQuadObj(ObjModel &obj)
		{
			TangentWeights::Initialize();
			PatchedMesh patchMesh = PatchedMesh::FromObj(obj);
			BezierMesh mesh;
			for (int i = 0; i<patchMesh.Patches.Count(); i++)
			{
				auto iPatch = patchMesh.Patches[i];
				BezierPatch oPatch;
				Vec3 points[2][4];

				GenerateIteriorPoints(points[0], patchMesh, iPatch);
				oPatch.ControlPoints[1][1] = points[0][0];
				oPatch.ControlPoints[1][2] = points[0][1];
				oPatch.ControlPoints[2][2] = points[0][2];
				oPatch.ControlPoints[2][1] = points[0][3];

				GenerateEdgePoints(points, patchMesh, iPatch);
				oPatch.ControlPoints[0][1] = points[0][0];
				oPatch.ControlPoints[0][2] = points[1][0];
				oPatch.ControlPoints[1][3] = points[0][1];
				oPatch.ControlPoints[2][3] = points[1][1];
				oPatch.ControlPoints[3][2] = points[0][2];
				oPatch.ControlPoints[3][1] = points[1][2];
				oPatch.ControlPoints[2][0] = points[0][3];
				oPatch.ControlPoints[1][0] = points[1][3];

				GenerateCornerPoints(points[0], patchMesh, iPatch);
				oPatch.ControlPoints[0][0] = points[0][0];
				oPatch.ControlPoints[0][3] = points[0][1];
				oPatch.ControlPoints[3][3] = points[0][2];
				oPatch.ControlPoints[3][0] = points[0][3];

				for (int k = 0; k < 4; k++)
				{
					for (int j=0; j<3; j++)
					{
						oPatch.TangentU[j][k] = (oPatch.ControlPoints[k][j+1] - oPatch.ControlPoints[k][j]) * 3.0f;
						oPatch.TangentV[j][k] = (oPatch.ControlPoints[j+1][k] - oPatch.ControlPoints[j][k]) * 3.0f;
					}
				}

				if (!*(int *)(iPatch.IsBoundary))
				{
					FixCorners(patchMesh, iPatch, oPatch.TangentU, oPatch.TangentV);
					FixEdges(patchMesh, iPatch, oPatch.TangentU, oPatch.TangentV, oPatch.ControlPoints);
				}
				for (int k = 0; k < 4; k++)
				{
					if (iPatch.TexCoordId[k] != -1)
						oPatch.TexCoords[k] = obj.TexCoords[iPatch.TexCoordId[k]];
				}
				mesh.Patches.Add(oPatch);
			}
			return mesh;
		}
	}
}

/***********************************************************************
CAMERA.CPP
***********************************************************************/
#ifdef _WIN32
#include <Windows.h>
#endif
using namespace VectorMath;

namespace CoreLib
{
	namespace Graphics
	{
		Camera::Camera()
		{
			Reset();
			CanFly = true;
		}

		void Camera::GetInverseRotationMatrix(float mat[9])
		{
			Vec3 left;
			Vec3::Cross(left, dir, up);
			Vec3::Normalize(left, left);
			mat[0] = left.x; mat[1] = up.x; mat[2] = -dir.x;
			mat[3] = left.y; mat[4] = up.y; mat[5] = -dir.y;
			mat[6] = left.z; mat[7] = up.z; mat[8] = -dir.z;
		}

		void Camera::Reset()
		{
			alpha = (float)PI;
			beta = 0.0f;
			pos = Vec3::Create(0.0f,0.0f,0.0f);
			up = Vec3::Create(0.0f,1.0f,0.0f);
			dir = Vec3::Create(0.0f,0.0f,-1.0f);
		}

		void Camera::GoForward(float u)
		{
			Vec3 vp;
			if (CanFly) 
			{
				pos += dir*u;
			}
			else
			{
				vp.x = sin(alpha);
				vp.z = cos(alpha);
				pos += vp*u;
			}
		}

		void Camera::MoveLeft(float u)
		{
			Vec3 l, vp;
			vp.x = sin(alpha);
			vp.z = cos(alpha);
			l.x=vp.z;	l.y=0;	l.z=-vp.x;
			pos += l*u;
		}

		void Camera::TurnLeft(float u)
		{
			alpha += u;
		}

		void Camera::TurnUp(float u)
		{
			beta += u;
			if (beta > (float)PI/2)
				beta=(float)PI/2;
			if (beta < (float)-PI/2)
				beta=-(float)PI/2;
		}

		void Camera::GetTransform(Matrix4 & rs)
		{
			dir = Vec3::Create((float)sin(alpha)*cos(beta),
					   (float)sin(beta),
					   (float)cos(alpha)*cos(beta));
			up = Vec3::Create((float)sin(alpha)*cos(PI/2+beta),
					  (float)sin(PI/2+beta),
					  (float)cos(alpha)*(float)cos(PI/2+beta));
			ViewFrustum view;
			GetView(view);
			rs = view.GetViewTransform();
		}

		void Camera::GetView(ViewFrustum & view)
		{
			dir = Vec3::Create((float)sin(alpha)*cos(beta),
				(float)sin(beta),
				(float)cos(alpha)*cos(beta));
			up = Vec3::Create((float)sin(alpha)*cos(PI / 2 + beta),
				(float)sin(PI / 2 + beta),
				(float)cos(alpha)*(float)cos(PI / 2 + beta));
			view.CamPos = pos;
			view.CamDir = dir;
			view.CamUp = up;
		}

#ifdef _WIN32
		void CameraController::HandleCameraKeys(Camera & camera, Matrix4 & transform, float dtime, float /*minSpeed*/, float maxSpeed, bool flipYZ)
		{
			const float CameraMaxSpeed = maxSpeed;
			const float CameraAcceleration = CameraMaxSpeed;
			const float CameraTurnAngle = 3.14159f/4.0f;

			static Vec3 speed = Vec3::Create(0.0f, 0.0f, 0.0f);
			Vec3 force;
			Vec3 left;
			force.SetZero();
			left.x=cos(camera.alpha);	left.y=0;	left.z=-sin(camera.alpha);
			if (GetAsyncKeyState(L'W') != 0)
			{
				force += camera.dir;
			}
			else if (GetAsyncKeyState(L'S') != 0 && GetAsyncKeyState(VK_CONTROL) == 0)
			{
				force -= camera.dir;
			}
			if (GetAsyncKeyState(L'A') != 0)
			{
				force += left;
			}
			else if (GetAsyncKeyState(L'D') != 0)
			{
				force -= left;
			}
			float forceLen = force.Length();
			if (forceLen > 0.0f)
			{
				force *= 1.0f/forceLen;
			}
			float accelLen = CameraAcceleration * dtime;
			float spdLen = speed.Length2();
			if (spdLen < accelLen * accelLen * 16.0f)
			{
				speed = Vec3::Create(0.0f,0.0f,0.0f);
			}
			else if (spdLen>0.0f)
			{
				Vec3 spdDir;
				Vec3::Normalize(spdDir, speed);
				speed -= spdDir * accelLen * 4.0f;
			}
				
			speed += force * accelLen * 5.0f;
			spdLen = speed.Length2();

			if (spdLen > CameraMaxSpeed*CameraMaxSpeed)
			{
				Vec3::Normalize(speed, speed);
				speed *= CameraMaxSpeed;
			}

			if (GetAsyncKeyState(VK_SHIFT))
				dtime *= 0.1f;

			float lturn = 0.0f;
			float uturn = 0.0f;
			if (GetAsyncKeyState(VK_LEFT))
			{
				lturn = CameraTurnAngle * dtime;
			}
			if (GetAsyncKeyState(VK_RIGHT))
			{
				lturn = -CameraTurnAngle * dtime;
			}

			if (GetAsyncKeyState(VK_UP))
			{
				uturn = CameraTurnAngle * dtime;
			}
			if (GetAsyncKeyState(VK_DOWN))
			{
				uturn = -CameraTurnAngle * dtime;
			}
			camera.pos += speed*dtime;
			camera.TurnLeft(lturn);
			camera.TurnUp(uturn);
			camera.GetTransform(transform);
			if (flipYZ)
			{
				Matrix4 flip;
				Matrix4::CreateIdentityMatrix(flip);
				flip.m[1][1] = 0.0f;
				flip.m[1][2] = -1.0f;
				flip.m[2][1] = 1.0f;
				flip.m[2][2] = 0.0f;
				Matrix4 flipped;
				Matrix4::Multiply(flipped, transform, flip);
				transform = flipped;
			}
		}
#endif
	}
}

/***********************************************************************
GGX.CPP
***********************************************************************/
using namespace CoreLib::Basic;
using namespace VectorMath;

namespace CoreLib
{
	namespace Graphics
	{
		List<Vec2> ComputeTextureFV(float maxRoughness, int size)
		{
			List<Vec2> result;
			result.SetSize(size * size);
			for (int j = 0; j < size; j++)
			{
				float roughness = maxRoughness * ((float)j + 0.5f) / (float)(size);
				float alpha = roughness*roughness;
				for (int i = 0; i < size; i++)
				{
					float dotLH = ((float)i + 0.5f) / (float)(size);
					if (dotLH < 0.1f) dotLH = 0.1f;
					// F
					float F_a, F_b;
					float tmp = 1.0f - dotLH;
					float dotLH5 = (tmp*tmp) * (tmp*tmp) *tmp;
					F_a = 1.0f;
					F_b = dotLH5;

					// V
					float vis;
					float k = alpha / 2.0f;
					float k2 = k*k;
					float invK2 = 1.0f - k2;
					vis = 1.0f/(dotLH*dotLH*invK2 + k2);
					Vec2 fv;
					fv.x = F_a*vis;
					fv.y = F_b*vis;
					if (Math::IsNaN(fv.x))
						fv.x = 10000.0f;
					if (Math::IsNaN(fv.y))
						fv.y = 10000.0f;
					result[j*size + i] = fv;
				}
			}
			return result;
		}

		List<float> ComputeTextureD(float maxRoughness, int size)
		{
			List<float> result;
			result.SetSize(size * size);
			for (int j = 0; j < size; j++)
			{
				float roughness = maxRoughness * ((float)j + 0.5f) / (float)(size);
				float alpha = roughness*roughness;
				float alphaSqr = alpha*alpha;
				for (int i = 0; i < size; i++)
				{
					float dotNH = sqrt(sqrt(((float)i + 0.5f)/ (float)(size)));
				
					float pi = 3.14159f;
					float denom = dotNH * dotNH *(alphaSqr - 1.0f) + 1.0f;

					float D = alphaSqr / (pi * denom * denom);
					if (Math::IsNaN(D))
						D = 10000.0f;
					result[j*size + i] = D;
				}
			}
			return result;
		}
		TextureFile ComputeTextureFileFV(float maxRoughness, int size)
		{
			auto data = ComputeTextureFV(maxRoughness, size);
			TextureFile file;
			List<unsigned char> udata;
			udata.SetSize(size * size * 2);
			for (int i = 0; i < data.Count(); i++)
			{
				udata[i * 2] = (unsigned char)Math::Clamp((int)(data[i].x * 255.0f), 0, 255);
				udata[i * 2 + 1] = (unsigned char)Math::Clamp((int)(data[i].y * 255.0f), 0, 255);
			}
			file.SetData(TextureStorageFormat::RG_F32, size, size, udata.GetArrayView());
			return file;
		}
		TextureFile ComputeTextureFileD(float maxRoughness, int size)
		{
			auto data = ComputeTextureD(maxRoughness, size);
			List<unsigned char> udata;
			udata.SetSize(data.Count() * sizeof(float));
			memcpy(udata.Buffer(), data.Buffer(), udata.Count());
			TextureFile file;
			file.SetData(TextureStorageFormat::R_F32, size, size, udata.GetArrayView());
			return file;
		}
	}
}

/***********************************************************************
OBJMODEL.CPP
***********************************************************************/
#include <map>
using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace VectorMath;

namespace CoreLib
{
	namespace Graphics
	{
		struct FaceVertex
		{
			int vid, nid, tid;
		};

		struct Vec3_Less
		{
			bool operator()(const Vec3 & v1, const Vec3 & v2) const
			{
				if (v1.x < v2.x)
					return true;
				else if (v1.x > v2.x)
					return false;
				else
				{
					if (v1.y < v2.y)
						return true;
					else if (v1.y > v2.y)
						return false;
					else
						return v1.z < v2.z;
				}
			}
		};

#ifdef _MSC_VER
#define SafeScan fscanf_s
#define SafeSScan sscanf_s
#else
#define SafeScan fscanf
#define SafeSScan sscanf
#endif

		void LoadObjMaterialLib(ObjModel & mdl, const String & filename, Dictionary<String, int> & matLookup);
		String RemoveLineBreakAndQuote(String name)
		{
			int pos = 0;
			while (pos<name.Length() && (name[pos] == L' ' || name[pos] == L'\t'))
				pos++;
			if (pos<name.Length() && name[pos] == L'\"')
				pos++;
			int end = name.Length()-1;
			while (end >= 0 && (name[end] == L' ' || name[end] == L'\t' || name[end] == L'\n'))
			{
				end--;
			}
			if (end >= 0 && name[end] == L'\"')
				end--;
			return name.SubString(pos, end-pos+1);
		}
		bool LoadObj(ObjModel & mdl, const char * fileName, PolygonType polygonType)
		{
			FILE * f = 0;
			fopen_s(&f, fileName, "rt");
			if (f == 0)
				return false;
			bool retVal = true;
			String matFileName;
			const int bufferSize = 4096;
			char buf[bufferSize];
			int smoothGroup = 0;
			int matId = -1;
			List<FaceVertex> vertices;
			Dictionary<String, int> matLookup;
#define checkResult(succ) if (!(succ)){retVal = false; break;}
			while (!feof(f))
			{
#ifdef _MSC_VER
				checkResult(fscanf_s(f, "%s", buf, bufferSize));
#else
				checkResult(fscanf(f, "%s", buf));
#endif

				if (_stricmp(buf, "v") == 0)
				{
					Vec3 v;
					checkResult(SafeScan(f, "%f %f %f", &v.x, &v.y, &v.z));
					mdl.Vertices.Add(v);
				}
				else if (_stricmp(buf, "vn") == 0)
				{
					Vec3 v;
					checkResult(SafeScan(f, "%f %f %f", &v.x, &v.y, &v.z));
					mdl.Normals.Add(v);
				}
				else if (_stricmp(buf, "vt") == 0)
				{
					Vec2 v;
					checkResult(SafeScan(f, "%f %f", &v.x, &v.y));
					//v.y = -v.y;
					mdl.TexCoords.Add(v);
				}
				else if (_stricmp(buf, "f") == 0)
				{
					checkResult(fgets(buf, bufferSize, f));
					int len = (int)strlen(buf);
					buf[len] = '\0';
					int startPos = 0;
					vertices.Clear();
					for (int i = 0; i<=len; i++)
					{
						if (buf[i] != ' ' && buf[i] != '\t' && buf[i] != '\n' && buf[i] != '\0')
						{
							continue;
						}
						else if (i == startPos)
						{
							startPos++;
							continue;
						}
						char str[50];
						memset(str, 0, 50);
						memcpy(str, buf+startPos, i-startPos);
						if (strstr(str, "//"))
						{
							int vid, nid;
							SafeSScan(str, "%d//%d", &vid, &nid);
							FaceVertex vtx;
							if (vid < 0)
								vtx.vid = mdl.Vertices.Count() + vid;
							else
								vtx.vid = vid - 1;
							if (nid < 0)
								vtx.nid = mdl.Normals.Count() + nid;
							else
								vtx.nid = nid - 1;
							vtx.tid = -1;
							vertices.Add(vtx);
						}
						else
						{
							int slashCount = 0;
							for (int j = 0; j<i-startPos; j++)
							{
								if (str[j] == '/') slashCount ++;
							}
							if (slashCount == 0)
							{
								FaceVertex vtx;
								vtx.nid = vtx.tid = -1;
								int vid;
								SafeSScan(str, "%d", &vid);
								if (vid < 0)
									vtx.vid = mdl.Vertices.Count() + vid;
								else
									vtx.vid = vid - 1;
								vertices.Add(vtx);
							}
							else if (slashCount == 3)
							{
								FaceVertex vtx;
								vtx.nid = -1;
								SafeSScan(str, "%d/%d", &vtx.vid, &vtx.tid);
								if (vtx.vid < 0)
									vtx.vid += mdl.Vertices.Count();
								else
									vtx.vid --;
								if (vtx.tid < 0)
									vtx.tid += mdl.TexCoords.Count();
								else
									vtx.tid --;
								vertices.Add(vtx);
							}
							else
							{
								FaceVertex vtx;
								SafeSScan(str, "%d/%d/%d", &vtx.vid, &vtx.tid, &vtx.nid);
								if (vtx.vid < 0)
									vtx.vid += mdl.Vertices.Count();
								else
									vtx.vid--;
								if (vtx.tid < 0)
									vtx.tid += mdl.TexCoords.Count();
								else
									vtx.tid--;
								if (vtx.nid < 0)
									vtx.nid += mdl.Normals.Count();
								else
									vtx.nid --;
								vertices.Add(vtx);
							}
						}
						startPos = i+1;
					}
					// simple triangulation
					if (vertices.Count() == 4 && polygonType == PolygonType::Quad)
					{
						ObjFace face;
						for (int k = 0; k<4; k++)
						{
							face.VertexIds[k] = vertices[k].vid;
							face.NormalIds[k] = vertices[k].nid;
							face.TexCoordIds[k] = vertices[k].tid;
						}
						face.SmoothGroup = smoothGroup;
						face.MaterialId = matId;
						mdl.Faces.Add(face);
					}
					else
					{
						for (int i = 2; i<vertices.Count(); i++)
						{
							ObjFace face;
							face.VertexIds[0] = vertices[0].vid;
							face.VertexIds[1] = vertices[i-1].vid;
							face.VertexIds[2] = vertices[i].vid;
							face.VertexIds[3] = -1;
							face.NormalIds[0] = vertices[0].nid;
							face.NormalIds[1] = vertices[i-1].nid;
							face.NormalIds[2] = vertices[i].nid;
							face.NormalIds[3] = -1;
							face.TexCoordIds[0] = vertices[0].tid;
							face.TexCoordIds[1] = vertices[i-1].tid;
							face.TexCoordIds[2] = vertices[i].tid;
							face.TexCoordIds[3] = -1;
							face.SmoothGroup = smoothGroup;
							face.MaterialId = matId;
							mdl.Faces.Add(face);
						}
					}
				}
				else if (_stricmp(buf, "usemtl") == 0)
				{
					String mtlName;
					checkResult(fgets(buf, 199, f));
					mtlName = buf;
					mtlName = RemoveLineBreakAndQuote(mtlName);
					matLookup.TryGetValue(mtlName, matId);
				}
				else if (_stricmp(buf, "s") == 0)
				{
#ifdef _MSC_VER
					checkResult(fscanf_s(f, "%s", buf, 200));
#else
					checkResult(fscanf(f, "%s", buf));
#endif
					if (buf[0] >= '0' && buf[0] <= '9')
					{
						checkResult(SafeSScan(buf, "%d", &smoothGroup));
					}
					else
						smoothGroup = 0;
				}
				else if (_stricmp(buf, "mtllib") == 0)
				{
					checkResult(fgets(buf, 199, f));
					matFileName = buf;
					matFileName = RemoveLineBreakAndQuote(matFileName);
					String path = Path::GetDirectoryName(fileName);
					LoadObjMaterialLib(mdl, Path::Combine(path, matFileName), matLookup);
				}
				else
				{
					while (!feof(f) && fgetc(f) != '\n');
				}
			}

			fclose(f);
			return retVal;
		}

		void LoadObjMaterialLib(ObjModel & mdl, const String & filename, Dictionary<String, int> & matLookup)
		{
			FILE * f = 0;
			fopen_s(&f, filename.ToMultiByteString(), "rt");
			if (!f)
			{
				printf("Error loading obj material library \'%s\'", filename.ToMultiByteString());
				return;
			}
			char buf[200];
			ObjMaterial* curMat = 0;
			int succ = 0;
			while (!feof(f))
			{
#ifdef _MSC_VER
				succ = fscanf_s(f, "%s", buf, 200);
#else
				succ = fscanf(f, "%s", buf);
#endif
				if (!succ)
					break;
				if (_stricmp(buf, "newmtl") == 0)
				{
					curMat = new ObjMaterial();
					mdl.Materials.Add(curMat);
					char *succ2 = fgets(buf, 199, f);
					if (!succ2)
						break;
					String matName = buf;
					matName = RemoveLineBreakAndQuote(matName);
					matLookup[matName] = mdl.Materials.Count() - 1;
					curMat->Name = matName;
				}
				else if (_stricmp(buf, "kd") == 0)
				{
					Vec3 v;
					succ = SafeScan(f, "%f %f %f", &v.x, &v.y, &v.z);
					if (!succ)
						break;
					if (curMat)
						curMat->Diffuse = v;
				}
				else if (_stricmp(buf, "ks") == 0)
				{
					Vec3 v;
					succ = SafeScan(f, "%f %f %f", &v.x, &v.y, &v.z);
					if (!succ)
						break;
					if (curMat)
						curMat->Specular = v;
				}
				else if (_stricmp(buf, "ns") == 0)
				{
					float s;
					succ = SafeScan(f, "%f", &s);
					if (!succ)
						break;
					if (curMat)
						curMat->SpecularRate = s;
				}
				else if (_stricmp(buf, "map_kd") == 0)
				{
					char * succ2 = fgets(buf, 199, f);
					if (!succ2)
						break;
					String name = buf;
					name = RemoveLineBreakAndQuote(name);
					if (curMat)
						curMat->DiffuseMap = name;
				}
				else if (_stricmp(buf, "map_bump") == 0)
				{
					char *succ2 = fgets(buf, 199, f);
					if (!succ2)
						break;
					String name = buf;
					name = RemoveLineBreakAndQuote(name);
					if (curMat)
						curMat->BumpMap = name;
				}
				else if (_stricmp(buf, "map_d") == 0)
				{
					fgets(buf, 199, f);
					String name = buf;
					name = RemoveLineBreakAndQuote(name);
					if (curMat)
						curMat->AlphaMap = name;
				}
				else if (_stricmp(buf, "opaque") == 0)
				{
					curMat->IsOpaque = true;
				}
				else
				{
					while (!feof(f) && fgetc(f) != '\n');
				}
			}
			fclose(f);
			for (auto & mtl : mdl.Materials)
			{
				if (mtl->DiffuseMap == L"" && mtl->AlphaMap == L"")
					mtl->IsOpaque = true;
			}
		}

		void ObjModel::SaveToBinary(IO::BinaryWriter & writer)
		{
			writer.Write(Vertices.Count());
			writer.Write(Vertices.Buffer(), Vertices.Count());

			writer.Write(Normals.Count());
			writer.Write(Normals.Buffer(), Normals.Count());

			writer.Write(TexCoords.Count());
			writer.Write(TexCoords.Buffer(), TexCoords.Count());

			writer.Write(Faces.Count());
			writer.Write(Faces.Buffer(), Faces.Count());

			writer.Write(ObjMaterialVersion); // version
			writer.Write(Materials.Count());
			for (int i = 0; i<Materials.Count(); i++)
			{
				writer.Write(Materials[i]->Name);
				writer.Write(Materials[i]->Diffuse);
				writer.Write(Materials[i]->Specular);
				writer.Write(Materials[i]->SpecularRate);
				writer.Write(Materials[i]->BumpMap);
				writer.Write(Materials[i]->AlphaMap);
				writer.Write(Materials[i]->DiffuseMap);
			}
		}

		bool ObjModel::LoadFromBinary(IO::BinaryReader & reader)
		{
			Vertices.SetSize(reader.ReadInt32());
			reader.Read(Vertices.Buffer(), Vertices.Count());

			Normals.SetSize(reader.ReadInt32());
			reader.Read(Normals.Buffer(), Normals.Count());

			TexCoords.SetSize(reader.ReadInt32());
			reader.Read(TexCoords.Buffer(), TexCoords.Count());

			Faces.SetSize(reader.ReadInt32());
			reader.Read(Faces.Buffer(), Faces.Count());

			int ver = reader.ReadInt32();
			if (ver != ObjMaterialVersion)
				return false;
			int materialCount = reader.ReadInt32();
			for (int i = 0; i<materialCount; i++)
			{
				RefPtr<ObjMaterial> mat = new ObjMaterial();
				Materials.Add(mat);
				mat->Name = reader.ReadString();
				reader.Read(&mat->Diffuse, 1);
				reader.Read(&mat->Specular, 1);
				reader.Read(&mat->SpecularRate, 1);
				mat->BumpMap = reader.ReadString();
				mat->AlphaMap = reader.ReadString();
				mat->DiffuseMap = reader.ReadString();
			}
			return true;
		}

		void ObjModel::ConstructPerVertexFaceList(Basic::List<int> & faceCountAtVert, Basic::List<int> & vertFaceList) const
		{
			faceCountAtVert.SetSize(Vertices.Count());
			memset(faceCountAtVert.Buffer(), 0, faceCountAtVert.Count()*sizeof(int));
			for (int i = 0; i<Faces.Count(); i++)
			{
				faceCountAtVert[Faces[i].VertexIds[0]]++;
				faceCountAtVert[Faces[i].VertexIds[1]]++;
				faceCountAtVert[Faces[i].VertexIds[2]]++;
				if (Faces[i].VertexIds[3] != -1)
					faceCountAtVert[Faces[i].VertexIds[3]]++;
			}
			int scan = 0;
			for (int i = 0; i<faceCountAtVert.Count(); i++)
			{
				int s = faceCountAtVert[i];
				faceCountAtVert[i] = scan;
				scan += s;
			}
			vertFaceList.SetSize(scan);
			for (int i = 0; i<Faces.Count(); i++)
			{
				vertFaceList[faceCountAtVert[Faces[i].VertexIds[0]]++] = i;
				vertFaceList[faceCountAtVert[Faces[i].VertexIds[1]]++] = i;
				vertFaceList[faceCountAtVert[Faces[i].VertexIds[2]]++] = i;
				if (Faces[i].VertexIds[3] != -1)
					vertFaceList[faceCountAtVert[Faces[i].VertexIds[3]]++] = i;
			}
		}

		void RecomputeNormals(ObjModel & mdl)
		{
			mdl.Normals.Clear();
			std::map<Vec3, int, Vec3_Less> normalMap;
			//Dictionary<Vec3, int> normalMap;
			List<Vec3> faceNormals;
			faceNormals.SetSize(mdl.Faces.Count());
			for (int i = 0; i<mdl.Faces.Count(); i++)
			{
				Vec3 v1 = mdl.Vertices[mdl.Faces[i].VertexIds[0]];
				Vec3 v2 = mdl.Vertices[mdl.Faces[i].VertexIds[1]];
				Vec3 v3 = mdl.Vertices[mdl.Faces[i].VertexIds[2]];
				Vec3 ab, ac;
				Vec3::Subtract(ab, v2, v1);
				Vec3::Subtract(ac, v3, v1);
				Vec3 n;
				Vec3::Cross(n, ab, ac);
				float len = n.Length();
				if (len > 1e-6f)
					Vec3::Scale(n, n, 1.0f/len);
				else
					n = Vec3::Create(1.0f, 0.0f, 0.0f);
				if (n.Length() > 1.2f || n.Length() < 0.5f)
					n = Vec3::Create(1.0f, 0.0f, 0.0f);
				faceNormals[i] = n;
			}
			List<int> vertShare;
			List<int> vertFaces;
			mdl.ConstructPerVertexFaceList(vertShare, vertFaces);
			int start = 0;
			for (int i = 0; i<mdl.Faces.Count(); i++)
			{
				ObjFace & face = mdl.Faces[i];
				for (int j = 0; j < 4; j++)
				{
					int vid = face.VertexIds[j];
					if (vid == -1)
						continue;
					if (vid == 0)
						start = 0;
					else
						start = vertShare[vid-1];
					int count = 0;
					Vec3 n;
					n.SetZero();
					for (int k = start; k < vertShare[vid]; k++)
					{
						int fid = vertFaces[k];
						if (mdl.Faces[fid].SmoothGroup & face.SmoothGroup)
						{
							Vec3::Add(n, faceNormals[fid], n);
							count ++;
						}
					}
					if (count == 0)
						n = faceNormals[i];
					else
					{
						Vec3::Scale(n, n, 1.0f / count);
						if (n.Length() < 0.5f)
						{
							n = faceNormals[i];
						}
						else
							n = n.Normalize();
					}
					/*if (!normalMap.TryGetValue(n, face.NormalIds[j]))
					{
						mdl.Normals.Add(n);
						face.NormalIds[j] = mdl.Normals.Count()-1;
						normalMap[n] = mdl.Normals.Count()-1;
					}*/
					std::map<Vec3, int, Vec3_Less>::iterator iter = normalMap.find(n);
					if (iter != normalMap.end())
					{
						face.NormalIds[j] = iter->second;
					}
					else
					{
						mdl.Normals.Add(n);
						face.NormalIds[j] = mdl.Normals.Count()-1;
						normalMap[n] = mdl.Normals.Count()-1;
					}
				}
			}
		}
	}
}

/***********************************************************************
TEXTUREFILE.CPP
***********************************************************************/

namespace CoreLib
{
	namespace Graphics
	{
		using namespace CoreLib::Basic;
		using namespace CoreLib::IO;

		TextureFile::TextureFile(String fileName)
		{
			FileStream stream(fileName);
			LoadFromStream(&stream);
		}
		TextureFile::TextureFile(Stream * stream)
		{
			LoadFromStream(stream);
		}
		int GetPixelSize(TextureStorageFormat format)
		{
			switch (format)
			{
			case TextureStorageFormat::R8:
				return 1;
			case TextureStorageFormat::RG8:
				return 2;
			case TextureStorageFormat::RGB8:
				return 3;
			case TextureStorageFormat::RGBA8:
				return 4;
			case TextureStorageFormat::R_F32:
				return 4;
			case TextureStorageFormat::RG_F32:
				return 8;
			case TextureStorageFormat::RGB_F32:
				return 12;
			case TextureStorageFormat::RGBA_F32:
				return 16;
			default:
				return 0;
			}
		}
		void TextureFile::LoadFromStream(Stream * stream)
		{
			BinaryReader reader(stream);
			TextureFileHeader header;
			int headerSize = reader.ReadInt32();
			reader.Read((unsigned char*)&header, headerSize);
			if (header.Type == TextureType::Texture2D)
			{
				width = header.Width;
				height = header.Height;
				format = header.Format;
				int pixelSize = GetPixelSize(format);
				buffer.SetSize(pixelSize * width * height);
				reader.Read(buffer.Buffer(), buffer.Count());
			}
			reader.ReleaseStream();
		}
		void TextureFile::SetData(TextureStorageFormat storageFormat, int w, int h, CoreLib::Basic::ArrayView<unsigned char> data)
		{
			int pixelSize = GetPixelSize(storageFormat);
			if (data.Count() != pixelSize * w * h)
				throw InvalidOperationException(L"Data size does not match texture format.");
			buffer.SetSize(data.Count());
			memcpy(buffer.Buffer(), data.Buffer(), data.Count());
			this->format = storageFormat;
			this->width = w;
			this->height = h;
		}
		void TextureFile::SaveToStream(Stream * stream)
		{
			BinaryWriter writer(stream);
			int headerSize = sizeof(TextureFileHeader);
			writer.Write(headerSize);
			TextureFileHeader header;
			header.Format = format;
			header.Width = width;
			header.Height = height;
			header.Type = TextureType::Texture2D;
			writer.Write(header);
			writer.Write(buffer.Buffer(), buffer.Count());
			writer.ReleaseStream();
		}
		void TextureFile::SaveToFile(String fileName)
		{
			FileStream stream(fileName, FileMode::Create);
			SaveToStream(&stream);
		}
	}
}

/***********************************************************************
VIEWFRUSTUM.CPP
***********************************************************************/
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
