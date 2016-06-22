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
						f.MaterialId = parser.ReadInt();
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
LIBUI.CPP
***********************************************************************/

namespace GraphicsUI
{
	using namespace CoreLib;
	using namespace VectorMath;

	GraphicsUI::ColorTable Global::Colors = CreateDefaultColorTable();
	int Global::HoverTimeThreshold = 200;
	int Global::EventGUID = 0;
	int Global::CursorPosX = 0;
	int Global::CursorPosY = 0;
	int Global::DeviceLineHeight = 18;
	int Global::SCROLLBAR_BUTTON_SIZE = 17;
	int Global::SCROLLBAR_MIN_PAGESIZE = 8;

	Control * Global::PointedComponent = nullptr;
	Control * Global::MouseCaptureControl = nullptr;

	Control * lastFocusedCtrl = 0;

	//Message Type defination
	const int MSG_UI_CLICK = 1;
	const int MSG_UI_DBLCLICK = 2;
	const int MSG_UI_MOUSEDOWN = 3;
	const int MSG_UI_MOUSEUP = 4;
	const int MSG_UI_MOUSEMOVE = 5;
	const int MSG_UI_MOUSEENTER = 6;
	const int MSG_UI_MOUSELEAVE = 7;
	const int MSG_UI_MOUSEHOVER = 19;
	const int MSG_UI_KEYDOWN = 8;
	const int MSG_UI_KEYUP = 9;
	const int MSG_UI_KEYPRESS = 10;
	const int MSG_UI_CHANGED = 11;
	const int MSG_UI_RESIZE = 12;

	// this message(TopLayer Draw) is sent by Entry to notify controls to do some drawing above any other controls if necessary.
	const int MSG_UI_TOPLAYER_DRAW = 13;
	const int MSG_UI_MOUSEWHEEL = 14;
	// Form Messages
	const int MSG_UI_FORM_ACTIVATE = 15;
	const int MSG_UI_FORM_DEACTIVATE = 16;

	Control * FindNextFocus(Control * ctrl);

	void Graphics::DrawArc(int x, int y, int rad, float theta, float theta2)
	{
		float lastX = x + rad*cos(theta);
		float lastY = y - rad*sin(theta);
		int segs = rad;
		float deltaPhi = (theta2-theta)/segs;
		theta += deltaPhi;
		for (int i=1; i < segs + 1; i++)
		{	
			float nx = x + rad*cos(theta);
			float ny = y - rad*sin(theta);
			DrawLine(lastX, lastY, nx, ny);
			theta += deltaPhi;
			lastX = nx;
			lastY = ny;
		}
	}

	void Graphics::FillEllipse(float x1, float y1, float x2, float y2)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::Ellipse;
		cmd.SolidColorParams.color = SolidBrushColor;
		cmd.x0 = x1 + dx; cmd.y0 = y1 + dy;
		cmd.x1 = x2 + dx; cmd.y1 = y2 + dy;
		commandBuffer.Add(cmd);
	}

	void Graphics::FillTriangle(int x0, int y0, int x1, int y1, int x2, int y2)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::Triangle;
		cmd.x0 = (float)x0 + dx;
		cmd.x1 = (float)x1 + dx;
		cmd.y0 = (float)y0 + dy;
		cmd.y1 = (float)y1 + dy;
		cmd.TriangleParams.x2 = (float)x2 + dx;
		cmd.TriangleParams.y2 = (float)y2 + dy;
		cmd.TriangleParams.color = SolidBrushColor;
		commandBuffer.Add(cmd);
	}

	void Graphics::DrawRectangle(int x1, int y1, int x2, int y2)
	{
		DrawLine((float)x1 + 0.5f, (float)y1 + 0.5f, (float)x2, (float)y1 + 0.5f);
		DrawLine((float)x1 + 0.5f, (float)y1 + 1.5f, (float)x1 + 0.5f, (float)y2);
		DrawLine((float)x2 + 0.5f, (float)y1 + 0.5f, (float)x2 + 0.5f, (float)y2);
		DrawLine((float)x2 + 0.5f, (float)y2 + 0.5f, (float)x1 + 0.5f, (float)y2 + 0.5f);
	}

	void Graphics::FillRectangle(int x1, int y1, int x2, int y2)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::SolidQuad;
		cmd.SolidColorParams.color = SolidBrushColor;
		cmd.x0 = (float)x1 + dx; cmd.y0 = (float)y1 + dy;
		cmd.x1 = (float)x2 + dx; cmd.y1 = (float)y2 + dy;
		commandBuffer.Add(cmd);
	}

	void Graphics::DrawLine(float x1, float y1, float x2, float y2)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::Line;
		cmd.x0 = x1 + dx; cmd.y0 = y1 + dy;
		cmd.x1 = x2 + dx; cmd.y1 = y2 + dy;
		cmd.SolidColorParams.color = PenColor;
		commandBuffer.Add(cmd);
	}

	void Graphics::SetClipRect(int x, int y, int w, int h)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::ClipQuad;
		cmd.x0 = (float)x + dx;
		cmd.x1 = (float)x + w + dx;
		cmd.y0 = (float)y + dy;
		cmd.y1 = (float)y + h + dy;
		commandBuffer.Add(cmd);
	}

	void Graphics::DrawShadowRect(Color shadowColor, int x0, int y0, int w, int h, int offsetX, int offsetY, float size)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::ShadowQuad;
		cmd.ShadowParams.x = (short)(x0 + dx);
		cmd.ShadowParams.y = (short)(y0 + dy);
		cmd.ShadowParams.w = (short)w;
		cmd.ShadowParams.h = (short)h;
		cmd.ShadowParams.offsetX = (unsigned char)offsetX;
		cmd.ShadowParams.offsetY = (unsigned char)offsetY;
		cmd.ShadowParams.color = shadowColor;
		cmd.ShadowParams.shadowSize = (unsigned char)size;
		float shadowSize = size * 1.5f;
		cmd.x0 = x0 + dx + offsetX - shadowSize; cmd.y0 = y0 + dy + offsetY - shadowSize;
		cmd.x1 = cmd.x0 + w + shadowSize * 2.0f; cmd.y1 = cmd.y0 + h + shadowSize * 2.0f;
		commandBuffer.Add(cmd);
	}

	void Graphics::DrawTextQuad(IBakedText * txt, int x, int y)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::TextQuad;
		cmd.x0 = (float)x + dx;
		cmd.y0 = (float)y + dy;
		cmd.x1 = cmd.x0 + (float)txt->GetWidth();
		cmd.y1 = cmd.y0 + (float)txt->GetHeight();
		cmd.TextParams.color = SolidBrushColor;
		cmd.TextParams.text = txt;
		commandBuffer.Add(cmd);
	}

	void Graphics::DrawImage(IImage * img, int x, int y)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::TextQuad;
		cmd.x0 = (float)x + dx;
		cmd.y0 = (float)y + dy;
		cmd.x1 = cmd.x0 + (float)img->GetWidth();
		cmd.y1 = cmd.y0 + (float)img->GetHeight();
		cmd.TextureParams.image = img;
		commandBuffer.Add(cmd);
	}

	ColorTable CreateDarkColorTable()
	{
		ColorTable tbl;

		tbl.ShadowColor = Color(0, 0, 0, 255);
		tbl.ControlBackColor = Color(0, 0, 0, 0);
		tbl.ControlBorderColor = Color(140, 140, 140, 255);
		tbl.ControlFontColor = Color(255, 255, 255, 255);
		tbl.EditableAreaBackColor = Color(50, 50, 50, 170);

		tbl.MemuIconBackColor = Color(127, 127, 127, 255);
		tbl.MenuBackColor = Color(80, 80, 80, 255);
		tbl.MenuBorderColor = Color(127, 127, 127, 255);
		tbl.MenuSeperatorColor = Color(130, 130, 130, 255);
		tbl.MenuItemForeColor = Color(255, 255, 255, 255);
		tbl.MenuItemDisabledForeColor = Color(180, 180, 180, 255);
		tbl.MenuItemHighlightForeColor = tbl.MenuItemForeColor;
	
		tbl.TabPageBorderColor = tbl.ControlBorderColor;
		tbl.TabPageItemSelectedBackColor1 = Color(140, 140, 140, 255);
		tbl.TabPageItemSelectedBackColor2 = tbl.TabPageItemSelectedBackColor1;

		tbl.TabPageItemHighlightBackColor1 = Color(70, 70, 70, 255);
		tbl.TabPageItemHighlightBackColor2 = tbl.TabPageItemHighlightBackColor1;

		tbl.TabPageItemBackColor1 = tbl.ControlBackColor;
		tbl.TabPageItemBackColor2 = tbl.TabPageBorderColor;

		tbl.ButtonBackColorChecked = Color(40, 40, 40, 255);

		tbl.DefaultFormStyle.ShowIcon = true;
		tbl.DefaultFormStyle.CtrlButtonBorderStyle = BS_RAISED;
		tbl.DefaultFormStyle.TitleBarColors[0] = Color(85, 85, 85, 255);
		tbl.DefaultFormStyle.TitleBarColors[1] = tbl.DefaultFormStyle.TitleBarColors[0];
		tbl.DefaultFormStyle.TitleBarColors[2] = Color(166, 202, 240, 255);
		tbl.DefaultFormStyle.TitleBarColors[3] = tbl.DefaultFormStyle.TitleBarColors[2];
		tbl.DefaultFormStyle.TitleBarDeactiveColors[0] = Color(128, 128, 128, 255);
		tbl.DefaultFormStyle.TitleBarDeactiveColors[1] = tbl.DefaultFormStyle.TitleBarDeactiveColors[0];
		tbl.DefaultFormStyle.TitleBarDeactiveColors[2] = Color(192, 192, 192, 255);
		tbl.DefaultFormStyle.TitleBarDeactiveColors[3] = tbl.DefaultFormStyle.TitleBarDeactiveColors[2];

		tbl.DefaultFormStyle.TitleBarFontColor = Color(255, 255, 255, 255);
		tbl.DefaultFormStyle.TopMost = false;
		tbl.DefaultFormStyle.BackColor = Color(0, 0, 0, 180);
		tbl.DefaultFormStyle.BorderColor = tbl.ControlBorderColor;

		tbl.SelectionColor = Color(224, 135, 0, 255);
		tbl.UnfocusedSelectionColor = Color(100, 100, 100, 127);
		tbl.HighlightColor = Color(100, 100, 100, 127);
		tbl.HighlightForeColor = Color(255, 255, 255, 255);
		tbl.SelectionForeColor = Color(255, 255, 255, 255);
		tbl.FocusRectColor = Color(120, 120, 120, 220);

		tbl.ToolButtonBackColor1 = tbl.ControlBackColor;
		tbl.ToolButtonBackColor2 = Color(55, 55, 55, 220);
		tbl.ToolButtonBackColorHighlight1 = tbl.SelectionColor;
		tbl.ToolButtonBackColorHighlight2 = tbl.SelectionColor;
		tbl.ToolButtonBackColorPressed1 = Color(184, 75, 0, 255);
		tbl.ToolButtonBackColorPressed2 = Color(184, 75, 0, 255);
		tbl.ToolButtonBorderHighLight = Color(254, 193, 92, 0);
		tbl.ToolButtonBorderSelected = Color(254, 193, 92, 0);
		tbl.ToolButtonSeperatorColor = Color(130, 130, 130, 255);
		tbl.ToolButtonBackColorChecked1 = Color(204, 105, 0, 255);
		tbl.ToolButtonBackColorChecked2 = tbl.ToolButtonBackColorChecked1;
		tbl.StatusStripBackColor1 = tbl.StatusStripBackColor2 = tbl.ToolButtonBackColor2;
		tbl.StatusStripBackColor3 = tbl.StatusStripBackColor4 = tbl.ToolButtonBackColor2;

		tbl.ScrollBarBackColor = tbl.EditableAreaBackColor;
		tbl.ScrollBarBackColor.R += 30;
		tbl.ScrollBarBackColor.G += 30;
		tbl.ScrollBarBackColor.B += 30;

		tbl.ScrollBarForeColor = Color(180, 180, 180, 255);
		tbl.ScrollBarHighlightColor = Color(140, 140, 140, 255);
		tbl.ScrollBarPressedColor = Color(100, 100, 100, 255);
		tbl.ScrollBarSliderColor = Color(110, 110, 110, 255);
		return tbl;
	}

	int emToPixel(float em)
	{
		return (int)(em * Global::DeviceLineHeight);
	}

	ColorTable CreateDefaultColorTable()
	{
		ColorTable tbl;
		tbl.ShadowColor = Color(0, 0, 0, 120);
		tbl.ControlBackColor = Color(235,238,241,255);
		tbl.ControlBorderColor = Color(160, 160, 160, 255);
		tbl.ControlFontColor = Color(0, 0, 0, 255);
		tbl.EditableAreaBackColor = Color(255, 255, 255, 255);
		tbl.ScrollBarBackColor = Color(255, 255, 255, 127);
		tbl.MemuIconBackColor = Color(232,232,225,255);
		tbl.MenuBackColor = Color(242,242,238,255);
		tbl.MenuBorderColor = Color(150,150,150,255);
		tbl.MenuSeperatorColor = Color(180,180,180,255);
		tbl.MenuItemForeColor = Color(0,0,0,255);
		tbl.MenuItemDisabledForeColor = Color(180,180,180,255);
		tbl.MenuItemHighlightForeColor = tbl.MenuItemForeColor;
		tbl.ToolButtonBackColor1 = tbl.ControlBackColor;
		tbl.ToolButtonBackColor2 = Color(215,226,228,255);
		tbl.ToolButtonBackColorHighlight1 = Color(255,250,210,255);
		tbl.ToolButtonBackColorHighlight2 = Color(253,236,168,255);
		tbl.ToolButtonBackColorPressed1 = Color(249,217,132,255);
		tbl.ToolButtonBackColorPressed2 = Color(252,236,194,255);
		tbl.ToolButtonBorderHighLight = Color(254,193,92,255);
		tbl.ToolButtonBorderSelected = Color(254,193,92,255);
		tbl.ToolButtonSeperatorColor = Color(170,170,160,255);
		tbl.ToolButtonBackColorChecked1 = Color(253, 247, 182, 255);
		tbl.ToolButtonBackColorChecked2 = tbl.ToolButtonBackColorChecked1;
		tbl.StatusStripBackColor1 = tbl.StatusStripBackColor2 = tbl.ToolButtonBackColor2;
		tbl.StatusStripBackColor3 = tbl.StatusStripBackColor4 = tbl.ToolButtonBackColor2;

		tbl.TabPageBorderColor = Color(127, 127, 127, 255);
		tbl.TabPageItemSelectedBackColor1 = Color(210, 227, 255, 255);
		tbl.TabPageItemSelectedBackColor2 = tbl.ControlBackColor;

		tbl.TabPageItemHighlightBackColor1 = Color(220, 244, 255, 255);
		tbl.TabPageItemHighlightBackColor2 = Color(220, 244, 255, 255);

		tbl.TabPageItemBackColor1 = tbl.ControlBackColor;
		tbl.TabPageItemBackColor2 = tbl.TabPageBorderColor;

		tbl.ButtonBackColorChecked = Color(254,216,152,255);

		tbl.SelectionColor = Color(10, 36, 106, 255);
		tbl.HighlightColor = Color(200, 200, 200, 255);
		tbl.HighlightForeColor = Color(0, 0, 0, 255);
		tbl.SelectionForeColor = Color(255, 255, 255, 255);
		tbl.UnfocusedSelectionColor = Color(200, 200, 200, 255);
		tbl.DefaultFormStyle.ShowIcon = true;
		tbl.DefaultFormStyle.CtrlButtonBorderStyle = BS_RAISED;
		tbl.DefaultFormStyle.TitleBarColors[0] = Color(10, 36, 106, 255);
		tbl.DefaultFormStyle.TitleBarColors[1] = tbl.DefaultFormStyle.TitleBarColors[0];
		tbl.DefaultFormStyle.TitleBarColors[2] = Color(166, 202, 240, 255);
		tbl.DefaultFormStyle.TitleBarColors[3] = tbl.DefaultFormStyle.TitleBarColors[2];
		tbl.DefaultFormStyle.TitleBarDeactiveColors[0] = Color(128, 128, 128, 255);
		tbl.DefaultFormStyle.TitleBarDeactiveColors[1] = tbl.DefaultFormStyle.TitleBarDeactiveColors[0];
		tbl.DefaultFormStyle.TitleBarDeactiveColors[2] = Color(192, 192, 192, 255);
		tbl.DefaultFormStyle.TitleBarDeactiveColors[3] = tbl.DefaultFormStyle.TitleBarDeactiveColors[2];

		tbl.DefaultFormStyle.TitleBarFontColor = Color(255, 255, 255, 255);
		tbl.DefaultFormStyle.TopMost = false;
		tbl.DefaultFormStyle.BackColor = tbl.ControlBackColor;
		tbl.DefaultFormStyle.BorderColor = tbl.ControlBorderColor;

		tbl.UnfocusedSelectionColor = Color(127, 127, 127, 255);
		tbl.FocusRectColor = Color(120, 120, 120, 220);

		tbl.ScrollBarBackColor = tbl.EditableAreaBackColor;
		tbl.ScrollBarBackColor.R -= 15;
		tbl.ScrollBarBackColor.G -= 15;
		tbl.ScrollBarBackColor.B -= 15;

		tbl.ScrollBarForeColor = Color(80, 80, 80, 255);
		tbl.ScrollBarSliderColor = Color(tbl.ScrollBarBackColor.R - 30, tbl.ScrollBarBackColor.R - 30, tbl.ScrollBarBackColor.R - 30, 255);
		tbl.ScrollBarHighlightColor = Color(tbl.ScrollBarSliderColor.R - 60, tbl.ScrollBarSliderColor.G - 60, tbl.ScrollBarSliderColor.B - 60, 255);
		tbl.ScrollBarPressedColor = Color(tbl.ScrollBarHighlightColor.R - 20, tbl.ScrollBarHighlightColor.G - 20, tbl.ScrollBarHighlightColor.B - 20, 255);

		return tbl;
	}

	int ClampInt(int val, int min, int max)
	{
		if (val < min)
			return min;
		else if (val > max)
			return max;
		else
			return val;
	}

	ClipRectStack::ClipRectStack(Graphics * g)
	{
		StackSize = 0;
		graphics = g;
	}

	void ClipRectStack::PushRect(Rect nRect)
	{
		Buffer[StackSize] = nRect;
		StackSize ++;
		graphics->SetClipRect(nRect.x, nRect.y, nRect.w, nRect.h);
	}

	Rect ClipRectStack::PopRect()
	{
		if (StackSize)
			StackSize--;
		if (StackSize)
		{
			auto r = Buffer[StackSize - 1];
			graphics->SetClipRect(r.x, r.y, r.w, r.h);
			return Buffer[StackSize-1];
		}
		else
		{
			auto rect = Rect(0,0,WindowWidth,WindowHeight);
			graphics->SetClipRect(rect.x, rect.y, rect.w, rect.h);
			return rect;
		}
	}

	Rect ClipRectStack::GetTop()
	{
		return Buffer[StackSize-1];
	}

	void ClipRectStack::Clear()
	{
		StackSize = 0;
	}

	void ClipRectStack::AddRect(Rect nRect)
	{
		Rect cRect;
		if (StackSize)
		{
			int nx1,nx2,ny1,ny2;
			nx1 = nRect.x + nRect.w;
			nx2 = Buffer[StackSize-1].x + Buffer[StackSize-1].w;
			ny1 = nRect.y + nRect.h;
			ny2 = Buffer[StackSize-1].y + Buffer[StackSize-1].h;
			cRect.x = Math::Max(nRect.x,Buffer[StackSize-1].x);
			cRect.y = Math::Max(nRect.y,Buffer[StackSize-1].y);
			cRect.w = Math::Min(nx1,nx2)-cRect.x;
			cRect.h = Math::Min(ny1,ny2)-cRect.y;
		}
		else
		{
			cRect = nRect;
		}
		PushRect(cRect);
	}

	void UI_Base::HandleMessage(const UI_MsgArgs *)
	{
	}

	Control::Control(Container * parent, bool addToParent)
	{
		ID = 0;
		EventID = -1;
		Cursor = CursorType::Arrow;
		Width = Height = Left = Top = 0;
		Name = "unnamed";
		Enabled = true;
		Visible = true;
		TopMost = false;
		LastInClient = false;
		BackgroundShadow = false;
		FontColor = Color(0, 0, 0, 255);
		Parent = parent;
		if (parent)
		{
			font = parent->GetFont();
			if (addToParent)
				parent->AddChild(this);
		}
		TabStop = false;
		BorderStyle = BS_RAISED;
		Type = CT_CONTROL;
		AbsolutePosX = AbsolutePosY = 0;
		BackColor = Global::Colors.ControlBackColor;
		BorderColor = Global::Colors.ControlBorderColor;
		DockStyle = dsNone;
	}

	Control::Control(Container * parent)
		: Control(parent, true)
	{
	}

	Control::~Control()
	{
		auto entry = GetEntry();
		if (Global::PointedComponent == this)
			Global::PointedComponent = Parent;
		if (Global::MouseCaptureControl == this)
			Global::MouseCaptureControl = nullptr;
		if (entry && entry->FocusedControl == this)
			entry->FocusedControl = nullptr;
	}

	bool Control::DoClosePopup()
	{
		return false;
	}

	VectorMath::Vec2i Control::GetRelativePos(Container * parent)
	{
		VectorMath::Vec2i result;
		result.x = Left;
		result.y = Top;
		auto current = this;
		auto curParent = this->Parent;
		while (curParent != parent)
		{
			result.x += curParent->Left;
			result.y += curParent->Top;
			if (current->DockStyle == dsFill || current->DockStyle == dsNone)
			{
				result.x += curParent->ClientRect().x;
				result.y += curParent->ClientRect().y;
			}
			current = curParent;
			curParent = curParent->Parent;
		}
		return result;
	}

	void Control::LocalPosToAbsolutePos(int x, int y, int & ax, int & ay)
	{
		auto relPos = GetRelativePos(nullptr);
		ax = relPos.x + x;
		ay = relPos.y + y;
	}

	bool Control::IsFocused()
	{
		auto focus = GetEntry()->FocusedControl;
		while (focus)
		{
			if (focus == this)
				return true;
			focus = focus->Parent;
		}
		return false;
	}

	void Control::BroadcastMessage(const UI_MsgArgs *Args)
	{
		switch (Args->Type)
		{
		case MSG_UI_CLICK:
			OnClick.Invoke(this);
			return;
		case MSG_UI_DBLCLICK:
			OnDblClick.Invoke(this);
			return;
		case MSG_UI_CHANGED:
			OnChanged.Invoke(this);
			return;
		case MSG_UI_RESIZE:
			OnResize.Invoke(this);
			return;
		case MSG_UI_MOUSEENTER:
			OnMouseEnter.Invoke(this);
			return;
		case MSG_UI_MOUSELEAVE:
			OnMouseLeave.Invoke(this);
			return;
		case MSG_UI_MOUSEMOVE:
			OnMouseMove.Invoke(this, *((UIMouseEventArgs*)Args->Data));
			return;
		case MSG_UI_MOUSEDOWN:
			OnMouseDown.Invoke(this, *((UIMouseEventArgs*)Args->Data));
			return;
		case MSG_UI_MOUSEUP:
			OnMouseUp.Invoke(this, *((UIMouseEventArgs*)Args->Data));
			return;
		case MSG_UI_MOUSEWHEEL:
			OnMouseWheel.Invoke(this,*((UIMouseEventArgs*)Args->Data));
			return;
		case MSG_UI_MOUSEHOVER:
			OnMouseHover.Invoke(this);
			return;
		case MSG_UI_KEYDOWN:
			OnKeyDown.Invoke(this, *((UIKeyEventArgs*)Args->Data));
			return;
		case MSG_UI_KEYUP:
			OnKeyUp.Invoke(this, *((UIKeyEventArgs*)Args->Data));
			return;
		case MSG_UI_KEYPRESS:
			OnKeyPress.Invoke(this, *((UIKeyEventArgs*)Args->Data));
			return;
		}
	}

	int Control::GetWidth()
	{
		return Width;
	}

	int Control::GetHeight()
	{
		return Height;
	}

	void Control::Posit(int ALeft, int ATop, int AWidth, int AHeight)
	{
		Left = ALeft;
		Top = ATop;
		Height = AHeight;
		Width = AWidth;
		SizeChanged();
	}
		
	Rect Control::ClientRect()
	{
		return clientRect;
	}

	bool Control::IsPointInClient(int X, int Y)
	{
		bool rs = (X>0 && Y>0 && X<Width && Y<Height);
		return rs;
	}

	Control * Control::FindControlAtPosition(int x, int y)
	{
		bool rs = (x>0 && y>0 && x<Width && y<Height);
		if (rs && Visible)
			return this;
		return nullptr;
	}

	bool Control::IsChildOf(Container * ctrl)
	{
		auto parent = Parent;
		while (parent && parent != ctrl)
			parent = parent->Parent;
		if (parent)
			return true;
		return false;
	}

	void Control::ReleaseMouse()
	{
		if (Global::MouseCaptureControl == this)
			Global::MouseCaptureControl = nullptr;
	}

	void Control::SetHeight(int val)
	{
		Height = val;
		SizeChanged();
	}

	void Control::SetWidth(int val)
	{
		Width = val;
		SizeChanged();
	}

	void Control::SizeChanged()
	{
		clientRect = Rect(0,0,Width,Height);
		UI_MsgArgs Arg;
		Arg.Sender = this;
		Arg.Type = MSG_UI_RESIZE;
		BroadcastMessage(&Arg);
		OnResize.Invoke(this);
	}

	bool Control::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		if (IsPointInClient(X,Y))
		{
			if (Shift == SS_BUTTONLEFT)
			{
				IsMouseDown = true;
				Global::MouseCaptureControl = this;
			}
			GetEntry()->System->SwitchCursor(Cursor);
			UI_MsgArgs Args;UIMouseEventArgs Data;
			Args.Sender = this;	Args.Type = MSG_UI_MOUSEDOWN;
			Data.Shift = Shift;	Data.X = X;	Data.Y = Y;
			Args.Data = &Data;
			BroadcastMessage(&Args);
			if (Parent)
				SetFocus();
		}
		return false;
	}

	bool Control::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
			
		if (IsPointInClient(X,Y))
		{
			if (IsMouseDown && (Shift & SS_BUTTONLEFT) && Visible)
				DoClick();
		}
		IsMouseDown = false;
		UI_MsgArgs Args;UIMouseEventArgs Data;
		Args.Sender = this;	Args.Type = MSG_UI_MOUSEUP;
		Data.Shift = Shift;	Data.X = X;	Data.Y = Y;
		Args.Data = &Data;
		BroadcastMessage(&Args);
		ReleaseMouse();
		return false;
	}

	bool Control::DoMouseMove(int X, int Y)
	{
		if (!Enabled || !Visible)
			return false;
		UI_MsgArgs Args;
		UIMouseEventArgs Data;
		Args.Sender = this;	
		Data.Shift = 0;	Data.X = X-Left;	Data.Y = Y-Top;
		Args.Data = &Data;
		GetEntry()->System->SwitchCursor(Cursor);
		Args.Type = MSG_UI_MOUSEMOVE;
		BroadcastMessage(&Args);
		return false;
	}

	bool Control::DoMouseEnter()
	{
		UI_MsgArgs Args;
		UIMouseEventArgs Data;
		Args.Sender = this;
		Data.Shift = 0;	Data.X = 0;	Data.Y = 0;
		Args.Data = &Data;
		Args.Type = MSG_UI_MOUSEENTER;
		BroadcastMessage(&Args);
		//GetEntry()->System->SwitchCursor(Cursor);
		return false;
	}

	bool Control::DoMouseLeave()
	{
		UI_MsgArgs Args;
		UIMouseEventArgs Data;
		Args.Sender = this;	
		Data.Shift = 0;	Data.X = 0;	Data.Y = 0;
		Args.Data = &Data;
		Args.Type = MSG_UI_MOUSELEAVE;
		BroadcastMessage(&Args);
		//GetEntry()->System->SwitchCursor(CursorType::Arrow);
		return false;
	}

	bool Control::DoMouseHover()
	{
		OnMouseHover.Invoke(this);
		return false;
	}

	bool Control::DoKeyDown(unsigned short Key, SHIFTSTATE Shift) 
	{
		if (!Enabled || !Visible)
			return false;
		UI_MsgArgs Args;UIKeyEventArgs Data;
		Args.Sender = this;	Args.Type = MSG_UI_KEYDOWN;
		Data.Key = Key;Data.Shift = Shift;
		Args.Data = &Data;
		BroadcastMessage(&Args);
		return false;
	}

	bool Control::DoKeyUp(unsigned short Key, SHIFTSTATE Shift) 
	{
		if (!Enabled || !Visible)
			return false;
		UI_MsgArgs Args;UIKeyEventArgs Data;
		Args.Sender = this;	Args.Type = MSG_UI_KEYUP;
		Data.Key = Key;Data.Shift = Shift;
		Args.Data = &Data;
		BroadcastMessage(&Args);
		return false;
	}

	bool Control::DoKeyPress(unsigned short Key, SHIFTSTATE Shift) 
	{
		if (!Enabled || !Visible)
			return false;
		UI_MsgArgs Args;UIKeyEventArgs Data;
		Args.Sender = this;	Args.Type = MSG_UI_KEYPRESS;
		Data.Key = Key;Data.Shift = Shift;
		Args.Data = &Data;
		BroadcastMessage(&Args);
		return false;
	}

	bool Control::DoClick() 
	{
		if (!Enabled || !Visible )
			return false;
		UI_MsgArgs Args;
		Args.Sender = this;
		Args.Type = MSG_UI_CLICK;
		BroadcastMessage(&Args);
		return false;
	}

	bool Control::DoDblClick()
	{
		if (!Enabled || !Visible)
			return false;
		UI_MsgArgs Args;
		Args.Sender = this;
		Args.Type = MSG_UI_DBLCLICK;
		BroadcastMessage(&Args);
		return false;
	}

	void Control::LostFocus(Control * /*newFocus*/)
	{
		OnLostFocus.Invoke(this);
	}

	void Control::SetName(String AName)
	{
		Name = AName;
	}

	void Control::Draw(int absX, int absY)
	{
		absX = absX + Left;
		absY = absY + Top;
		AbsolutePosX = absX;  AbsolutePosY = absY;
		auto &clipRects = GetEntry()->ClipRects;
		auto entry = GetEntry();
		if (BackgroundShadow)
		{
			// Draw background shadow
			Rect R = clipRects->PopRect();
			{
				Color shadowColor = Global::Colors.ShadowColor;
				shadowColor.A = ShadowOpacity;
				entry->DrawCommands.DrawShadowRect(shadowColor, absX, absY, Width, Height, ShadowOffset, ShadowOffset, ShadowSize);
			}
			clipRects->PushRect(R);
		}
		//Draw Background
		if (BackColor.A)
		{
			entry->DrawCommands.SolidBrushColor = BackColor;
			entry->DrawCommands.FillRectangle(absX + 1, absY + 1, absX + Width - 1, absY + Height - 1);
		}
		//Draw Border
		Color LightColor, DarkColor;
		LightColor.R = (unsigned char)ClampInt(BorderColor.R + COLOR_LIGHTEN,0,255);
		LightColor.G = (unsigned char)ClampInt(BorderColor.G + COLOR_LIGHTEN,0,255);
		LightColor.B = (unsigned char)ClampInt(BorderColor.B + COLOR_LIGHTEN,0,255);
		LightColor.A = BorderColor.A;
		DarkColor.R = (unsigned char)ClampInt(BorderColor.R - COLOR_LIGHTEN, 0, 255);
		DarkColor.G = (unsigned char)ClampInt(BorderColor.G - COLOR_LIGHTEN, 0, 255);
		DarkColor.B = (unsigned char)ClampInt(BorderColor.B - COLOR_LIGHTEN, 0, 255);
		DarkColor.A = BorderColor.A;
		if (BorderStyle == BS_RAISED)
		{
			entry->DrawCommands.PenColor = LightColor;
			entry->DrawCommands.DrawLine(absX, absY, absX + Width - 1, absY);
			entry->DrawCommands.DrawLine(absX, absY, absX, absY + Height - 1);

			entry->DrawCommands.PenColor = DarkColor;
			entry->DrawCommands.DrawLine(absX + Width - 1, absY, absX + Width - 1, absY + Height - 1);
			entry->DrawCommands.DrawLine(absX + Width - 1, absY + Height - 1, absX, absY + Height - 1);
		}
		else if (BorderStyle == BS_LOWERED)
		{
			entry->DrawCommands.PenColor = DarkColor;
			entry->DrawCommands.DrawLine(absX, absY, absX + Width - 1, absY);
			entry->DrawCommands.DrawLine(absX, absY, absX, absY + Height - 1);

			entry->DrawCommands.PenColor = LightColor;
			entry->DrawCommands.DrawLine(absX + Width - 1, absY, absX + Width - 1, absY + Height - 1);
			entry->DrawCommands.DrawLine(absX + Width - 1, absY + Height - 1, absX, absY + Height - 1);
		}
		else if (BorderStyle == BS_FLAT_)
		{
			entry->DrawCommands.PenColor = BorderColor;
			entry->DrawCommands.DrawRectangle(absX, absY, absX + Width - 1, absY + Height - 1);
		}
	}

	void Control::SetFont(IFont * AFont)
	{
		this->font = AFont;
	}

	void Control::KillFocus()
	{
		OnLostFocus(this);
		if (GetEntry()->FocusedControl == this)
			GetEntry()->FocusedControl = nullptr;
	}

	void Control::SetFocus()
	{			
		GetEntry()->SetFocusedControl(this);		
	}

	Label::Label(Container * parent)
		: Container(parent)
	{
		BorderStyle = BS_NONE;
		BackColor.A = 0;
		BackColor.R = 255;  BackColor.G =255; BackColor.B = 255;
		FontColor = Global::Colors.ControlFontColor;
		FChanged = true;
		Type = CT_LABEL;
		AutoSize = true;
		DropShadow = false;
		this->font = parent->GetFont();
	}

	Label::~Label()
	{
	}

	String Label::GetText()
	{
		return FCaption;
	}

	void Label::SetText(const String & pText)
	{
		bool diff = FCaption != pText;
		FChanged = FChanged || diff;
		if (FCaption != pText)
		{
			FCaption = pText;
			UpdateText();
		}
	}

	void Label::SetFont(IFont * pFont)
	{
		if (pFont != font)
		{
			Control::SetFont(pFont);
			FChanged = true;
		}
	}

	void Label::SizeChanged()
	{
	}

	void Label::DoDpiChanged()
	{
		UpdateText();
	}

	void Label::UpdateText()
	{
		if (text)
			text = nullptr;
		auto size = font->MeasureString(FCaption);
		TextWidth = size.w;
		TextHeight = size.h;
		FChanged = false;
		if (AutoSize)
		{
			SetWidth(TextWidth + Padding.Horizontal());
			SetHeight(TextHeight + Padding.Vertical());
		}
	}

	void Label::Draw(int absX, int absY)
	{
		Control::Draw(absX, absY);
		absX = absX + Left;
		auto entry = GetEntry();
		if (font == nullptr)
		{
			font = entry->System->LoadDefaultFont();
			FChanged = true;
			UpdateText();
		}
		if (FChanged || !text)
		{
			text = font->BakeString(FCaption);
			FChanged = false;
		}
		if (VertAlignment == VerticalAlignment::Top)
			absY = absY + Top + Padding.Top;
		else if (VertAlignment == VerticalAlignment::Center)
			absY = absY + Top + (Height - TextHeight) / 2;
		else
			absY = absY + Top + Height - Padding.Bottom - TextHeight;
		if (DropShadow)
		{
			entry->DrawCommands.SolidBrushColor = ShadowColor;
			entry->DrawCommands.DrawTextQuad(text.Ptr(), absX + 1, absY + 1);
		}
		entry->DrawCommands.SolidBrushColor = FontColor;
		entry->DrawCommands.DrawTextQuad(text.Ptr(), absX, absY);
	}

	Button::Button(Container * parent)
		: Label(parent)
	{
		IsMouseDown = false;
		TabStop = true;
		Type = CT_BUTTON;
		BorderStyle = BS_RAISED;
		BackColor = Global::Colors.ControlBackColor;
		FontColor = Global::Colors.ControlFontColor;
		Checked = false;
		Padding = GetEntry()->GetLineHeight() / 2;
		Padding.Top = Padding.Bottom = Padding.Left / 2;
	}

	Button::Button(Container * parent, const CoreLib::String & text)
		:Button(parent)
	{
		SetText(text);
	}

	void Button::Draw(int absX, int absY)
	{
		if (!Visible)
			return;
		int lastBorder = BorderStyle;
		Color backColor = BackColor;
		if (Checked)
		{
			BackColor = Global::Colors.ButtonBackColorChecked;
			BorderStyle = BS_LOWERED;
		}
		Control::Draw(absX,absY);
		BorderStyle = lastBorder;
		BackColor = backColor;
		absX = absX + Left;
		absY = absY + Top;
		auto entry = GetEntry();
		if (font == nullptr)
			font = entry->System->LoadDefaultFont();
		if (FChanged || !text)
		{
			text = font->BakeString(FCaption);
		}
		int tx,ty;
		tx = (Width - TextWidth)/2;
		ty = (Height - TextHeight)/2;
		if (BorderStyle == BS_LOWERED)
		{
			tx += 1;
			ty += 1;
		}
		auto & graphics = entry->DrawCommands;
		if (Enabled)
		{
			graphics.SolidBrushColor = FontColor;
			graphics.DrawTextQuad(text.Ptr(), absX+tx,absY+ty);
		}
		else
		{
			graphics.SolidBrushColor = Color(255, 255, 255, FontColor.A);
			graphics.DrawTextQuad(text.Ptr(), absX + tx + 1, absY + ty + 1);
			graphics.SolidBrushColor = Color((unsigned char)ClampInt(FontColor.R + COLOR_LIGHTEN, 0, 255),
				(unsigned char)ClampInt(FontColor.R + COLOR_LIGHTEN, 0, 255),
				(unsigned char)ClampInt(FontColor.R + COLOR_LIGHTEN, 0, 255),
				FontColor.A);
			graphics.DrawTextQuad(text.Ptr(), absX + tx, absY + ty);
		}
		
		// Draw Focus Rect
		if (IsFocused())
		{
			graphics.PenColor = Global::Colors.FocusRectColor;
			graphics.DrawRectangle(absX + 3, absY + 3, absX + Width - 3, absY + Height - 3);
		}
	}

	bool Button::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Label::DoMouseDown(X,Y,Shift); 
		if (!Enabled || !Visible)
			return false;
		if (Shift == SS_BUTTONLEFT)
		{
			BorderStyle = BS_LOWERED;
		}
		return true;
	}

	bool Button::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Label::DoMouseUp(X,Y,Shift);
		BorderStyle = BS_RAISED;
		return true;
	}

	bool Button::DoDblClick()
	{
		return DoMouseDown(1, 1, SS_BUTTONLEFT);
	}

	bool Button::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Label::DoKeyDown(Key,Shift);
		if (!Enabled || !Visible)
			return false;
		if (Key == 0x20) // VK_SPACE
		{
			IsMouseDown = true;
			BorderStyle = BS_LOWERED;
		}
		else if (Key == 0x0D) // VK_RETURN
		{
			Control::DoClick();
		}
		return false;
	}

	bool Button::DoKeyUp(unsigned short Key, SHIFTSTATE Shift)
	{
		Label::DoKeyUp(Key,Shift);
		if (!Enabled || !Visible)
			return false;
		if (Key == 0x20) // VK_SPACE
		{
			IsMouseDown = false;
			BorderStyle = BS_RAISED;
			Control::DoClick();
		}
		return false;
	}

	void Button::DoDpiChanged()
	{
		Padding = GetEntry()->GetLineHeight() / 2;
		Padding.Top = Padding.Bottom = Padding.Left / 2;
		Label::DoDpiChanged();
	}

	Control * Container::FindControlAtPosition(int x, int y)
	{
		if (Visible && IsPointInClient(x, y))
		{
			if (x <= Padding.Left || y <= Padding.Top || x >= Width - Padding.Right || y >= Height - Padding.Bottom)
				return this;
			for (int i = controls.Count() - 1; i >= 0; i--)
			{
				if (controls[i]->EventID != Global::EventGUID)
				{
					int dx = 0;
					int dy = 0;
					if (controls[i]->DockStyle == dsNone || controls[i]->DockStyle == dsFill)
					{
						dx = clientRect.x;
						dy = clientRect.y;
					}
					int nx = x - dx;
					int ny = y - dy;
					if (auto child = controls[i]->FindControlAtPosition(nx - controls[i]->Left, ny - controls[i]->Top))
						return child;
				}
			}
			return this;
		}
		return nullptr;
	}

	Container::Container(Container * parent, bool addToParent)
		: Control(parent, addToParent)
	{
		Type = CT_CONTAINER;
		TabStop = false;
		Padding = 0;
		BorderStyle = BS_NONE;
	}

	Container::Container(Container * parent)
		: Container(parent, true)
	{
	}

	Container::Container(Container * parent, ContainerLayoutType pLayout)
		: Container(parent, true)
	{
		layout = pLayout;
	}

	bool Container::DoClosePopup()
	{
		for (int i=0;i<controls.Count(); i++)
			controls[i]->DoClosePopup();
		return false;
	}

	void Container::KillFocus()
	{
		for (int i = 0; i<controls.Count(); i++)
		{
			controls[i]->KillFocus();
		}
		Control::KillFocus();
	}

	void Container::SetLayout(ContainerLayoutType pLayout)
	{
		layout = pLayout;
	}

	void Container::DoDpiChanged()
	{
		if (layout == ContainerLayoutType::None)
		{
			float dpiScale = GetEntry()->GetDpiScale();
			for (auto & child : controls)
			{
				child->Posit((int)(child->Left*dpiScale),
					(int)(child->Top * dpiScale),
					(int)(child->GetWidth()*dpiScale),
					(int)(child->GetHeight() * dpiScale));
			}
		}
		for (auto & child : controls)
			child->DoDpiChanged();
		
		SizeChanged();
	}

	void Container::AddChild(Control *nControl)
	{
		controls.Add(nControl);
		nControl->Parent = this;
	}

	void Container::RemoveChild(Control *AControl)
	{
		for (int i=0; i<controls.Count(); i++)
		{
			if (controls[i] == AControl)
			{
				controls[i] = nullptr;
				controls.RemoveAt(i);
				break;
			}
		}
	}

	void Container::DrawChildren(int absX, int absY)
	{
		auto entry = GetEntry();
		entry->ClipRects->AddRect(Rect(absX + Padding.Left, absY + Padding.Top, Width - Padding.Horizontal(), Height - Padding.Vertical()));
		for (int i = 0; i<controls.Count(); i++)
		{
			if (controls[i]->Visible)
			{
				Control *ctrl = controls[i].Ptr();
				if (ctrl->Visible)
				{
					int dx = 0;
					int dy = 0;
					if (ctrl->DockStyle == dsNone || ctrl->DockStyle == dsFill)
					{
						dx = clientRect.x;
						dy = clientRect.y;
					}
					entry->ClipRects->AddRect(Rect(ctrl->Left + absX + dx, ctrl->Top + absY + dy, ctrl->GetWidth(), ctrl->GetHeight()));
					auto clipRect = entry->ClipRects->GetTop();
					if (ctrl->Visible && clipRect.Intersects(Rect(absX + dx + ctrl->Left, absY + dy + ctrl->Top, ctrl->GetWidth(), ctrl->GetHeight())))
						ctrl->Draw(absX + dx, absY + dy);
					entry->ClipRects->PopRect();
				}
			}
		}
		entry->ClipRects->PopRect();
	}

	void Container::Draw(int absX, int absY)
	{
		Control::Draw(absX, absY);
		absX+=Left; absY+=Top;
		if (drawChildren)
			DrawChildren(absX, absY);
	}

	void Container::ArrangeControls(Rect initalClientRect)
	{
		clientRect = initalClientRect;
		clientRect.x = initalClientRect.x + Padding.Left;
		clientRect.y = initalClientRect.y + Padding.Top;
		clientRect.w -= Padding.Horizontal();
		clientRect.h -= Padding.Vertical();
		for (int i=0; i < controls.Count(); i++)
		{
			if (!controls[i]->Visible)
				continue;
			switch (controls[i]->DockStyle)
			{
			case dsTop:
				controls[i]->Posit(clientRect.x, clientRect.y, clientRect.w, controls[i]->GetHeight());
				clientRect.y += controls[i]->GetHeight();
				clientRect.h -= controls[i]->GetHeight();
				break;
			case dsBottom:
				controls[i]->Posit(clientRect.x, clientRect.y + clientRect.h - controls[i]->GetHeight(), clientRect.w,
					controls[i]->GetHeight());
				clientRect.h -= controls[i]->GetHeight();
				break;
			case dsLeft:
				controls[i]->Posit(clientRect.x, clientRect.y, controls[i]->GetWidth(), clientRect.h);
				clientRect.x += controls[i]->GetWidth();
				clientRect.w -= controls[i]->GetWidth();
				break;
			case dsRight:
				controls[i]->Posit(clientRect.x + clientRect.w - controls[i]->GetWidth(), clientRect.y,
					controls[i]->GetWidth(), clientRect.h);
				clientRect.w -= controls[i]->GetWidth();
				break;
			default:
				break;
			}
		}
		int layoutX = 0;
		int layoutY = 0;
		int maxHeight = 0;
		for (int i = 0; i < controls.Count(); i++)
		{
			if (controls[i]->DockStyle == dsFill)
			{
				controls[i]->Posit(0, 0, clientRect.w, clientRect.h);
			}
		}
		if (layout == ContainerLayoutType::Flow || layout == ContainerLayoutType::Stack)
		{
			for (int i = 0; i < controls.Count(); i++)
			{
				if (controls[i]->DockStyle == dsNone)
				{
					if (layout == ContainerLayoutType::Stack ||
						(layoutX > 0 && layoutX + controls[i]->GetWidth() + controls[i]->Margin.Left > clientRect.w)) // new line
					{
						layoutY += maxHeight;
						layoutX = 0;
						maxHeight = 0;
					}
					controls[i]->Left = layoutX + controls[i]->Margin.Left;
					controls[i]->Top = layoutY + controls[i]->Margin.Top;
					if (layout == ContainerLayoutType::Stack)
					{
						controls[i]->Posit(controls[i]->Left, controls[i]->Top, Width - Padding.Horizontal(), controls[i]->GetHeight());
					}
					layoutX += controls[i]->GetWidth() + controls[i]->Margin.Horizontal();
					maxHeight = Math::Max(maxHeight, controls[i]->GetHeight() + controls[i]->Margin.Vertical());
				}
			}
		}
		if (AutoWidth || AutoHeight)
		{
			int nWidth = 0;
			int nHeight = 0;
			for (int i = 0; i < controls.Count(); i++)
			{
				int cw = controls[i]->GetWidth() + controls[i]->Left + controls[i]->Margin.Right;
				int ch = controls[i]->GetHeight() + controls[i]->Top + controls[i]->Margin.Bottom;
				if (controls[i]->DockStyle == dsLeft || controls[i]->DockStyle == dsRight)
					ch -= clientRect.y;
				if (controls[i]->DockStyle == dsTop || controls[i]->DockStyle == dsBottom)
					cw -= clientRect.x;
				if (cw > nWidth)
					nWidth = cw;
				if (ch > nHeight)
					nHeight = ch;
			}
			nWidth += Padding.Horizontal();
			nHeight += Padding.Vertical();
			if (AutoWidth) Width = nWidth;
			if (AutoHeight) Height = nHeight;
		}
	}

	void Container::SizeChanged()
	{
		Control::SizeChanged();
		ArrangeControls(Rect(0, 0, Width, Height));
	}

	bool Container::DoMouseLeave()
	{
		Control::DoMouseLeave();
		if (!Enabled || !Visible)
			return false;
		return false;
	}

	bool Container::DoDblClick()
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoDblClick();
		return false;
	}
	
	bool Container::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoKeyDown(Key,Shift);
		return false;
	}

	bool Container::DoKeyUp(unsigned short Key, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoKeyUp(Key,Shift);
		return false;
	}

	bool Container::DoKeyPress(unsigned short Key, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoKeyPress(Key,Shift);
		return false;
	}

	void Container::DoFocusChange()
	{
		if (Parent)
			Parent->DoFocusChange();
	}

	void Container::InternalBroadcastMessage(UI_MsgArgs *Args)
	{
		this->HandleMessage(Args);
		for (int i = controls.Count() - 1; i>=0; i--)
		{
			Container * ctn = dynamic_cast<Container *>(controls[i].Ptr());
			if (ctn)
				ctn->InternalBroadcastMessage(Args);
			else
				controls[i]->HandleMessage(Args);
		}
			
	}

	Form::Form(UIEntry * parent)
		: Container(parent)
	{
		Type = CT_FORM;
		Activated = false;
		ButtonClose = true;
		DownInTitleBar = false;
		DownInButton = false;
		BackgroundShadow = true;
		ShadowOffset = 0;
		ShadowSize = 25.0f;
		DownPosX = DownPosY = 0;
		Text = L"Form";
		parent->Forms.Add(this);
		this->content = nullptr;
		btnClose = new Control(this);
		lblTitle = new Label(this);
		lblClose = new Label(this);
		wchar_t CloseSymbol[2] = {114,0}; 
		lblClose->SetText(CloseSymbol);
		lblClose->SetFont(GetEntry()->System->LoadDefaultFont(GraphicsUI::DefaultFontType::Symbol));
		btnClose->Visible = false;
		lblTitle->Visible = false;
		lblClose->Visible = false;
		btnClose->BorderStyle = BS_NONE;
		btnClose->BackColor.A = 0;
		formStyle = Global::Colors.DefaultFormStyle;
		formStyle.TitleFont = parent->GetEntry()->System->LoadDefaultFont(GraphicsUI::DefaultFontType::Title);
		content = new Container(this);
		content->DockStyle = dsFill;
		content->BackColor.A = 0;
		content->BorderStyle = BS_NONE;
		FormStyleChanged();
		SetText(Text);
		Padding = 5;
		Posit(20, 20, 200, 200);
	}

	void Form::SetText(String AText)
	{
		Text = AText;
		lblTitle->SetText(Text);
	}

	String Form::GetText()
	{
		return Text;
	}

	void Form::AddChild(Control * ctrl)
	{
		if (!content)
			Container::AddChild(ctrl);
		else
			content->AddChild(ctrl);
	}

	ContainerLayoutType Form::GetLayout()
	{
		return content->GetLayout();
	}

	void Form::SetLayout(ContainerLayoutType pLayout)
	{
		content->SetLayout(pLayout);
	}

	CoreLib::List<CoreLib::RefPtr<Control>>& Form::GetChildren()
	{
		return content->GetChildren();
	}

	Control * Form::FindControlAtPosition(int x, int y)
	{
		auto ctrl = Container::FindControlAtPosition(x, y);
		if (ctrl)
			return ctrl;
		else
		{
			int additionalMargin = GetEntry()->GetLineHeight() / 2;
			if (x <= -additionalMargin || x - Width >= additionalMargin ||
				y <= -additionalMargin || y - Height >= additionalMargin)
				return nullptr;
			else
				return this;
		}
	}

	int Form::GetClientHeight()
	{
		return Height - Padding.Vertical() - GetTitleBarHeight();
	}

	int Form::GetClientWidth()
	{
		return Width - Padding.Horizontal();
	}

	ResizeMode Form::GetResizeHandleType(int x, int y)
	{
		int handleSize = 4;
		ResizeMode rs = ResizeMode::None;
		if (formStyle.Sizeable)
		{
			if (x <= handleSize)
				rs = (ResizeMode)((int)rs | (int)ResizeMode::Left);
			if (x >= Width - handleSize)
				rs = (ResizeMode)((int)rs | (int)ResizeMode::Right);
			if (y <= handleSize)
				rs = (ResizeMode)((int)rs | (int)ResizeMode::Top);
			if (y >= Height - handleSize)
				rs = (ResizeMode)((int)rs | (int)ResizeMode::Bottom);
		}
		return rs;
	}

	void Form::FormStyleChanged()
	{
		int titleHeight = GetTitleBarHeight();
		lblTitle->SetFont(formStyle.TitleFont);
		lblTitle->FontColor = formStyle.TitleBarFontColor;
		lblClose->FontColor = formStyle.TitleBarFontColor;
		btnClose->Posit(0, Padding.Top, titleHeight - Padding.Right, titleHeight - Padding.Right);
		BackColor = formStyle.BackColor;
		BorderColor = formStyle.BorderColor;
		BorderStyle = BS_FLAT_;
		btnClose->BackColor = formStyle.CtrlButtonBackColor;
		SizeChanged();
	}

	int Form::GetTitleBarHeight()
	{
		return (int)(GetEntry()->GetLineHeight() * formStyle.emTitleBarHeight);
	}

	void Form::SizeChanged()
	{
		int titleHeight = GetTitleBarHeight();
		btnClose->Posit(Width- titleHeight, 3, titleHeight - 4, titleHeight - 4);
		lblClose->Posit(Width- titleHeight + 2, 3, titleHeight - 4, titleHeight - 4);
		Control::SizeChanged();
		OnResize.Invoke(this);
		ArrangeControls(Rect(1, 1 + titleHeight, Width - 2, Height - 2 - titleHeight));
		
	}

	void Form::Draw(int absX,int absY)
	{
		if (!Enabled ||!Visible)
			return;
		int ox=absX, oy=absY;
		absX+=Left; absY+=Top;
		drawChildren = false;
		if (Activated)
		{
			ShadowOpacity = 180;
			ShadowSize = 30.0f;
		}
		else
		{
			ShadowOpacity = 90;
			ShadowSize = 10.0f;
		}
		Container::Draw(ox,oy);
		auto entry = GetEntry();
		//Title bar
		Color *Color = Activated?formStyle.TitleBarColors :formStyle.TitleBarDeactiveColors; 
		auto & graphics = entry->DrawCommands;
		graphics.SolidBrushColor = Color[0];
		int titleHeight = GetTitleBarHeight();
		graphics.FillRectangle(absX + 1, absY + 1, absX + Width - 1, absY + 1 + titleHeight);
		entry->ClipRects->AddRect(Rect(absX,  absY, lblClose->Left - 24, titleHeight));
		lblTitle->Draw(absX+8,absY+1+(titleHeight - lblTitle->GetHeight())/2);
		entry->ClipRects->PopRect();
		//Draw close Button
		if (ButtonClose)
		{
			btnClose->Draw(absX,absY);
			lblClose->Draw(absX,absY);
		}

		//Draw Controls
		entry->ClipRects->AddRect(Rect(absX + Padding.Left, absY + Padding.Top + titleHeight, Width - Padding.Horizontal(), Height - Padding.Vertical() - titleHeight));
		DrawChildren(absX, absY);
		entry->ClipRects->PopRect();
	}

	void Form::SetFormStyle(const FormStyle &AFormStyle)
	{
		formStyle = AFormStyle;
		BackColor = formStyle.BackColor;
		BorderColor = formStyle.BorderColor;
		FormStyleChanged();
	}

	void Form::HandleMessage(const UI_MsgArgs * msg)
	{
		if (msg->Type == MSG_UI_FORM_ACTIVATE)
		{
			auto nxt = FindNextFocus(this);
			if (nxt && nxt->IsChildOf(this))
				nxt->SetFocus();
		}
	}

	bool Form::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled ||!Visible)
			return false;
		Container::DoMouseUp(X-1,Y-1,Shift);
		DownInTitleBar = false;
		resizeMode = ResizeMode::None;
		this->ReleaseMouse();
		if (DownInButton)
		{
			int titleHeight = GetTitleBarHeight();
			if (X > Width - titleHeight && X < Width && Y > 0 && Y < titleHeight + 1)
			{
				GetEntry()->CloseWindow(this);
			}
		}
		if (Left < 0) Left = 0;
		if (Top < 0) Top = 0;
		if (Left > Parent->GetWidth() - 50)
			Left = Parent->GetWidth() - 50;
		if (Top > Parent->GetHeight() - 50)
			Top = Parent->GetHeight() - 50;
		DownInButton = false;
		return true;
	}

	CursorType GetResizeCursor(ResizeMode rm)
	{
		switch (rm)
		{
		case ResizeMode::None:
			return CursorType::Arrow;
		case ResizeMode::Left:
		case ResizeMode::Right:
			return CursorType::SizeWE;
		case ResizeMode::Top:
		case ResizeMode::Bottom:
			return CursorType::SizeNS;
		case ResizeMode::TopLeft:
		case ResizeMode::BottomRight:
			return CursorType::SizeNWSE;
		default:
			return CursorType::SizeNESW;
		}
	}

	bool Form::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled ||!Visible)
			return false;
		Container::DoMouseDown(X-1,Y-1,Shift);
		DownInButton=false;
		DownPosX = X; DownPosY = Y;
		resizeMode = GetResizeHandleType(X, Y);
		if (resizeMode == ResizeMode::None)
		{
			int titleHeight = GetTitleBarHeight();
			if (X > 3 && X < Width - titleHeight && Y > 0 && Y < titleHeight + 1)
			{
				DownInTitleBar = true;
				Global::MouseCaptureControl = this;
			}
			else
			{
				DownInTitleBar = false;
				if (X > Width - titleHeight && X < Width - 2 && Y > 0 && Y < titleHeight + 1)
				{
					DownInButton = true;
					Global::MouseCaptureControl = this;
				}
			}
		}
		else
		{
			GetEntry()->System->SwitchCursor(GetResizeCursor(resizeMode));
			Global::MouseCaptureControl = this;
		}
		return true;
	}

	bool Form::DoMouseMove(int X, int Y)
	{
		const int MinWidth = 120;
		const int MinHeight = GetTitleBarHeight() * 2;

		if (!Enabled ||!Visible)
			return false;
		Container::DoMouseMove(X-1,Y-1);		
		if (resizeMode != ResizeMode::None)
		{
			GetEntry()->System->SwitchCursor(GetResizeCursor(resizeMode));

			if ((int)resizeMode & (int)ResizeMode::Left)
			{
				int dwidth = DownPosX - X;
				if (Width + dwidth < MinWidth)
					dwidth = MinWidth - Width;
				Left -= dwidth;
				Width += dwidth;
			}
			if ((int)resizeMode & (int)ResizeMode::Right)
			{
				int dwidth = X - DownPosX;
				if (Width + dwidth < MinWidth)
					dwidth = MinWidth - Width;
				else
					DownPosX = X;
				Width += dwidth;
			}
			if ((int)resizeMode & (int)ResizeMode::Top)
			{
				int dHeight = DownPosY - Y;
				if (Height + dHeight < MinHeight)
					dHeight = MinHeight - Height;
				Top -= dHeight;
				Height += dHeight;
			}
			if ((int)resizeMode & (int)ResizeMode::Bottom)
			{
				int dHeight = Y - DownPosY;
				if (Height + dHeight < MinHeight)
					dHeight = MinHeight - Height;
				else
					DownPosY = Y;
				Height += dHeight;
			}
			SizeChanged();
		}
		else
		{
			auto rm = GetResizeHandleType(X, Y);
			GetEntry()->System->SwitchCursor(GetResizeCursor(rm));

			if (DownInTitleBar)
			{
				int dx, dy;
				dx = X - DownPosX; dy = Y - DownPosY;
				Left += dx; Top += dy;
			}
		}
		return true;
	}

	bool Form::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		if (!Enabled ||!Visible)
			return false;
		Container::DoKeyDown(Key,Shift);
		return false;
	}

	UIEntry::UIEntry(int WndWidth, int WndHeight, ISystemInterface * pSystem)
		: Container(nullptr)
	{
		this->System = pSystem;
		this->font = pSystem->LoadDefaultFont();
		ClipRects = new ClipRectStack(&DrawCommands);
		Left = Top =0;
		Global::EventGUID = 0;
		Height = WndHeight;
		Width = WndWidth;
		BorderStyle = BS_NONE;
		Type = CT_ENTRY;
		FocusedControl = NULL;
		ClipRects->WindowHeight = WndHeight;
		ClipRects->WindowWidth = WndWidth;
		ActiveForm = 0;
		CheckmarkLabel = new Label(this);
		CheckmarkLabel->AutoSize = true;
		CheckmarkLabel->Visible = false;
		CheckmarkLabel->SetFont(pSystem->LoadDefaultFont(DefaultFontType::Symbol));
		CheckmarkLabel->SetText(L"a");
		
		ImeMessageHandler.Init(this);
		ImeMessageHandler.ImeWindow->Visible = false;
		ImeMessageHandler.ImeWindow->WindowWidth = WndWidth;
		ImeMessageHandler.ImeWindow->WindowHeight = WndHeight;
		DoDpiChanged();
	}

	void UIEntry::InternalBroadcastMessage(UI_MsgArgs *Args)
	{
		//Broadcast to the activated form only.
		if (ActiveForm)
		{
			ActiveForm->InternalBroadcastMessage(Args);
		}
		for (auto & ctrl : controls)
			if (dynamic_cast<Form*>(ctrl.Ptr()) == nullptr)
			{
				if (auto ctn = dynamic_cast<Container*>(ctrl.Ptr()))
					ctn->InternalBroadcastMessage(Args);
				else
					ctrl->HandleMessage(Args);
			}
	}

	void UIEntry::SizeChanged()
	{
		Container::SizeChanged();
		ImeMessageHandler.ImeWindow->WindowWidth = Width;
		ImeMessageHandler.ImeWindow->WindowHeight = Height;
		ClipRects->WindowHeight = Height;
		ClipRects->WindowWidth = Width;
		for (auto & form : Forms)
		{
			if (form->Left + form->GetWidth() > Width - 1)
				form->Left = Width - form->GetWidth() - 1;
			if (form->Top + form->GetHeight() > Height - 1)
				form->Top = Height - form->GetHeight() - 1;
			if (form->Left < 0)
				form->Left = 0;
			if (form->Top < 0)
				form->Top = 0;
		}
	}

	void UIEntry::RemoveForm(Form *Form)
	{
		for (int i=0; i<Forms.Count(); i++)
		{
			if (Forms[i] == Form)
			{
				Forms[i] = 0;
				Forms.RemoveAt(i);
				break;
			}
		}
		this->RemoveChild(Form);
	}

	List<DrawCommand> & UIEntry::DrawUI()
	{
		DrawCommands.ClearCommands();
		Draw(0,0);
		return DrawCommands.Buffer();
	}

	bool UIEntry::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		if (Key == 0x09)  // VK_TAB
		{
			if (Shift & SS_CONTROL)
			{
				if (Forms.Count())
				{
					auto window = Forms.First();
					//window->SetFocus();
					ShowWindow(window);
				}
			}
			else
			{
				if (FocusedControl && FocusedControl->WantsTab)
					goto noTabProcess;
				if (Shift == SS_SHIFT)
					MoveFocusBackward();
				else
					MoveFocusForward();
			}
			return true;
		}
		else if (Key == 0x12) // VK_MENU
		{
			Menu * menu = nullptr;
			if (ActiveForm && ActiveForm->MainMenu)
				menu = ActiveForm->MainMenu;
			else if (this->MainMenu)
				menu = this->MainMenu;
			if (menu)
			{
				menu->SetFocus();
				if (menu->Count())
					menu->GetItem(0)->Selected = true;
				return true;
			}
		}
	noTabProcess:;
		auto ctrl = FocusedControl;
		while (ctrl && ctrl != this)
		{
			if (ctrl->DoKeyDown(Key, Shift))
				return true;
			ctrl = ctrl->Parent;
		}
		Control::DoKeyDown(Key, Shift);

		return false;
	}

	bool UIEntry::DoKeyUp(unsigned short Key, SHIFTSTATE Shift)
	{
		auto ctrl = FocusedControl;
		while (ctrl && ctrl != this)
		{
			if (ctrl->DoKeyUp(Key, Shift))
				return true;
			ctrl = ctrl->Parent;
		}
		Control::DoKeyUp(Key, Shift);

		return false;
	}

	bool UIEntry::DoKeyPress(unsigned short Key, SHIFTSTATE Shift)
	{
		bool rs = false;
		auto ctrl = FocusedControl;
		while (ctrl && ctrl != this)
		{
			if (ctrl->DoKeyPress(Key, Shift))
				return true;
			ctrl = ctrl->Parent;
		}
		if (ImeMessageHandler.ImeWindow->Visible &&
			(Key == 9 || (Key >= 32 && Key <= 127)))
		{
			ImeMessageHandler.StringInputed(String((wchar_t)(Key)));
			return true;
		}
		Control::DoKeyPress(Key, Shift);

		return rs;
	}

	template<typename Func>
	void BroadcastMouseMessage(List<MouseMessageStack> & stack, int X, int Y, const Func & f)
	{ 
		// mouse messages send to pointed components only
		auto ctrlToBroadcast = Global::MouseCaptureControl ? Global::MouseCaptureControl : Global::PointedComponent;
		if (ctrlToBroadcast)
		{
			stack.Clear();
			while (ctrlToBroadcast)
			{
				MouseMessageStack item;
				item.Ctrl = ctrlToBroadcast;
				stack.Add(item);
				ctrlToBroadcast = ctrlToBroadcast->Parent;
			}
			auto parent = stack.Last().Ctrl;
			int cx = X;
			int cy = Y;
			
			for (int i = stack.Count() - 2; i >= 0; i--)
			{
				auto ctrl = stack[i].Ctrl;
				cx -= ctrl->Left;
				cy -= ctrl->Top;
				if (ctrl->DockStyle == Control::dsNone || ctrl->DockStyle == Control::dsFill)
				{
					cx -= parent->ClientRect().x;
					cy -= parent->ClientRect().y;
				}
				stack[i].X = cx;
				stack[i].Y = cy;
				parent = ctrl;
			}
			for (int i = 0; i < stack.Count() - 1; i++)
				if (f(stack[i].Ctrl, stack[i].X, stack[i].Y))
					break;
		}
	}
	bool UIEntry::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		// Detect new active Form.
		Form *nForm=0;
		Global::PointedComponent = FindControlAtPosition(X, Y);
		if (Global::MouseCaptureControl == nullptr)
		{
			DeactivateAllForms();
			for (int i = Forms.Count() - 1; i >= 0; i--)
			{
				Form *curForm = Forms[i];
				if (curForm->Visible && curForm->Enabled && (curForm == Global::PointedComponent || 
					(Global::PointedComponent && Global::PointedComponent->IsChildOf(curForm))))
				{
					ShowWindow(curForm);
					nForm = curForm;
					break;
				}
			}
			if (nForm == 0)
			{
				if (ActiveForm)
				{
					SetFocusedControl(nullptr);
				}
				ActiveForm = 0;
			}
		}
		Global::EventGUID ++;
		bool processed = false;
		BroadcastMouseMessage(controlStack, X, Y, [&](Control* ctrl, int x, int y)
		{
			bool rs = ctrl->DoMouseDown(x, y, Shift);
			processed = processed || rs;
			return rs;
		});
		if (!processed && !Global::MouseCaptureControl && Global::PointedComponent == this)
		{
			UIMouseEventArgs e;
			e.Delta = 0;
			e.Shift = Shift;
			e.X = X;
			e.Y = Y;
			Global::MouseCaptureControl = this;
			OnMouseDown.Invoke(this, e);
		}
		return false;
	}

	bool UIEntry::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Global::PointedComponent = FindControlAtPosition(X, Y);
		Global::EventGUID++;
		bool processed = false;
		BroadcastMouseMessage(controlStack, X, Y, [&](Control* ctrl, int x, int y)
		{
			bool rs = ctrl->DoMouseUp(x, y, Shift);
			processed = processed || rs;
			return rs;
		});
		if (Global::MouseCaptureControl == this || !processed)
		{
			UIMouseEventArgs e;
			e.Delta = 0;
			e.Shift = Shift;
			e.X = X;
			e.Y = Y;
			OnMouseUp.Invoke(this, e);
			ReleaseMouse();
		}
		return false;
	}

	bool UIEntry::DoMouseMove(int X, int Y)
	{
		auto pointedComp = FindControlAtPosition(X, Y);
		if (pointedComp != Global::PointedComponent)
		{
			if (!Global::MouseCaptureControl)
			{
				auto cur = Global::PointedComponent;
				while (cur && !pointedComp->IsChildOf((Container*)cur))
				{
					cur->DoMouseLeave();
					cur = cur->Parent;
				}
				auto cur2 = pointedComp;
				while (cur2 != cur)
				{
					cur2->DoMouseEnter();
					cur2 = cur2->Parent;
				}
				Global::PointedComponent = pointedComp;
			}
		}
		Global::CursorPosX = X;
		Global::CursorPosY = Y;
		Global::EventGUID++;
		bool processed = false;
		BroadcastMouseMessage(controlStack, X, Y, [&](Control* ctrl, int x, int y)
		{
			bool rs = ctrl->DoMouseMove(x, y);
			processed = processed || rs;
			return rs;
		});
		if (Global::MouseCaptureControl == this || !processed)
		{
			Control::DoMouseMove(X, Y);
			UIMouseEventArgs e;
			e.Delta = 0;
			e.Shift = 0;
			e.X = X;
			e.Y = Y;
			OnMouseMove.Invoke(this, e);
		}
		return processed;
	}

	bool GraphicsUI::UIEntry::DoMouseWheel(int delta)
	{
		auto ctrlToBroadcast = Global::MouseCaptureControl ? Global::MouseCaptureControl : Global::PointedComponent;
		while (ctrlToBroadcast && ctrlToBroadcast != this)
		{
			if (ctrlToBroadcast->DoMouseWheel(delta))
				return true;
			ctrlToBroadcast = ctrlToBroadcast->Parent;
		}
		UIMouseEventArgs e;
		e.Delta = delta;
		e.Shift = 0;
		e.X = Global::CursorPosX;
		e.Y = Global::CursorPosY;
		OnMouseWheel.Invoke(this, e);
		return false;
	}

	bool UIEntry::DoMouseHover()
	{
		auto ctrlToBroadcast = Global::MouseCaptureControl ? Global::MouseCaptureControl : Global::PointedComponent;
		while (ctrlToBroadcast && ctrlToBroadcast != this)
		{
			if (ctrlToBroadcast->DoMouseHover())
				return true;
			ctrlToBroadcast = ctrlToBroadcast->Parent;
		}
		OnMouseHover.Invoke(this);
		return false;
	}

	bool UIEntry::DoDblClick()
	{
		auto ctrlToBroadcast = Global::MouseCaptureControl ? Global::MouseCaptureControl : Global::PointedComponent;
		while (ctrlToBroadcast && ctrlToBroadcast != this)
		{
			if (ctrlToBroadcast->DoDblClick())
				return true;
			ctrlToBroadcast = ctrlToBroadcast->Parent;
		}
		OnDblClick.Invoke(this);
		return true;
	}

	bool UIEntry::DoTick()
	{
		for (auto & ctrl : tickEventSubscribers)
			ctrl->DoTick();
		return true;
	}

	void UIEntry::DeactivateAllForms()
	{
		for (int i=0; i<Forms.Count(); i++)
		{
			Forms[i]->Activated = false;
		}
	}

	void UIEntry::ShowWindow(Form * Form)
	{
		if (Form == ActiveForm)
		{
			Form->Activated = true;
			return;
		}
		GraphicsUI::Form* form;
		for (int i=0; i<Forms.Count(); i++)
		{
			if (Forms[i] == Form)
			{
				form = Forms[i];
				Forms.RemoveAt(i);
				break;
			}
		}
		Forms.Add(form);
		if (!Form->Visible)
			Form->OnShow.Invoke(Form);
		Form->Visible = true;
		DeactivateAllForms();
		if (ActiveForm != Form)
		{
			UI_MsgArgs Args;
			Args.Sender = this;
			Args.Type = MSG_UI_FORM_DEACTIVATE;
			if (ActiveForm)
				ActiveForm->HandleMessage(&Args);
			else
				HandleMessage(&Args);
			ActiveForm = Form;
			Args.Type = MSG_UI_FORM_ACTIVATE;
			Form->HandleMessage(&Args);
		}
		ActiveForm = Form;
		Form->Activated = true;
	}

	void UIEntry::CloseWindow(Form *Form)
	{
		if (Form->Visible)
			Form->OnClose.Invoke(Form);
		Form->Visible = false;
			
		FocusedControl = 0;
		ActiveForm = 0;
		for (int i=Forms.Count()-1; i>=0; i--)
		{
			GraphicsUI::Form *curForm;
			curForm = Forms[i];
			if (curForm->Visible)
			{
				ShowWindow(curForm);
				break;
			}
		};
	}

	void UIEntry::HandleMessage(const UI_MsgArgs *Args)
	{
		if (Args->Type == MSG_UI_FORM_DEACTIVATE)
		{
			KillFocus();
			SetFocusedControl(0);
		}
	}

	void UIEntry::DoDpiChanged()
	{
		int nLineHeight = font->MeasureString(L"M").h;
		if (lineHeight != 0)
			dpiScale = nLineHeight / (float)lineHeight;
		Global::DeviceLineHeight = lineHeight = nLineHeight;
		Global::SCROLLBAR_BUTTON_SIZE = (int)(GetLineHeight());
		CheckmarkLabel->DoDpiChanged();
		Container::DoDpiChanged();
	}

	Control * FindNextFocus(Control * ctrl)
	{
		if (auto ctn = dynamic_cast<Container*>(ctrl))
		{
			for (auto & child : ctn->GetChildren())
			{
				if (child->Enabled && child->Visible)
				{
					if (child->TabStop)
						return child.Ptr();
					else if (auto tmpRs = FindNextFocus(child.Ptr()))
						return tmpRs;
				}
			}
				
		}
		auto parent = ctrl->Parent;
		while (parent->GetChildren().Last() == ctrl)
		{
			ctrl = parent;
			parent = ctrl->Parent;
			if (!parent)
				break;
		}
		if (parent)
		{
			int idx = parent->GetChildren().IndexOf(ctrl);
			for (int i = idx + 1; i < parent->GetChildren().Count(); i++)
			{
				if (parent->GetChildren()[i]->Enabled && parent->GetChildren()[i]->Visible)
				{
					if (parent->GetChildren()[i]->TabStop)
						return parent->GetChildren()[i].Ptr();
					else
						return FindNextFocus(parent->GetChildren()[i].Ptr());
				}
			}
		}
		return nullptr;
	}

	Control * GetLastLeaf(Container * ctn)
	{
		if (ctn->GetChildren().Count() == 0)
			return ctn;
		for (int i = ctn->GetChildren().Count() - 1; i >= 0; i--)
		{
			auto ctrl = ctn->GetChildren()[i].Ptr();
			if (ctrl->Visible && ctrl->Enabled)
			{
				if ((ctrl->Type & CT_CONTAINER) != 0)
				{
					return GetLastLeaf(dynamic_cast<Container*>(ctrl));
				}
				else
					return ctrl;
			}
		}
		return ctn;
	}

	Control * FindPreviousFocus(Control * ctrl)
	{
		auto parent = ctrl->Parent;
		while (parent && parent->GetChildren().First() == ctrl)
		{
			ctrl = parent;
			parent = ctrl->Parent;
			if (!parent)
				break;
		}
		if (parent)
		{
			int idx = parent->GetChildren().IndexOf(ctrl);
			for (int i = idx - 1; i >= 0; i--)
			{
				if (parent->GetChildren()[i]->Enabled && parent->GetChildren()[i]->Visible)
				{
					if (parent->GetChildren()[i]->TabStop)
						return parent->GetChildren()[i].Ptr();
					else if (auto ctn = dynamic_cast<Container*>(parent->GetChildren()[i].Ptr()))
					{
						auto last = GetLastLeaf(ctn);
						if (last->Visible && last->Enabled && last->TabStop)
							return last;
						else
							return FindPreviousFocus(GetLastLeaf(ctn));
					}
				}
			}
			return FindPreviousFocus(parent);
		}
		return nullptr;
	}


	void UIEntry::MoveFocusBackward()
	{
		if (FocusedControl)
		{
			auto nxt = FindPreviousFocus(FocusedControl);
			if (!nxt)
			{
				nxt = GetLastLeaf(this);
				if (!nxt->TabStop || !nxt->Enabled || !nxt->Visible)
					nxt = FindPreviousFocus(nxt);
			}
			if (nxt && nxt != FocusedControl)
			{
				FocusedControl->LostFocus(nxt);
				FocusedControl->KillFocus();
				SetFocusedControl(nxt);
			}
		}
	}

	void UIEntry::MoveFocusForward()
	{
		if (FocusedControl)
		{
			auto nxt = FindNextFocus(FocusedControl);
			if (!nxt)
				nxt = FindNextFocus(this);
			if (nxt && nxt != FocusedControl)
			{
				FocusedControl->LostFocus(nxt);
				FocusedControl->KillFocus();
				SetFocusedControl(nxt);
			}
		}
	}

	VectorMath::Vec2i UIEntry::GetCaretScreenPos()
	{
		if (FocusedControl && (FocusedControl->Type & CT_IME_RECEIVER))
		{
			auto receiver = dynamic_cast<ImeCharReceiver*>(FocusedControl);
			if (receiver)
			{
				return receiver->GetCaretScreenPos();
			}
		}
		return VectorMath::Vec2i::Create(0, 0);
	}

	void UIEntry::Draw(int absX,int absY)
	{
		drawChildren = false;
		Container::Draw(absX,absY);
		//Draw Forms
		for (auto children : controls)
		{
			if (children->Visible && children->Type != CT_FORM)
			{
				int dx = 0;
				int dy = 0;
				if (children->DockStyle == dsNone || children->DockStyle == dsFill)
				{
					dx = clientRect.x;
					dy = clientRect.y;
				}
				ClipRects->AddRect(Rect(children->Left + absX + dx, children->Top + absY + dy, children->GetWidth() + 1, children->GetHeight() + 1));
				children->Draw(absX + dx, absY + dy);
				ClipRects->PopRect();
			}
		}
		for (int i=0; i<Forms.Count(); i++)
		{
			Forms[i]->Draw(absX + clientRect.x,absY + clientRect.y);
		}
		//Draw Top Layer Menus
		UI_MsgArgs Arg;
		Arg.Sender = this;
		Arg.Type = MSG_UI_TOPLAYER_DRAW;
		InternalBroadcastMessage(&Arg);

		//Draw IME 
		if (FocusedControl && (FocusedControl->Type & CT_IME_RECEIVER))
		{
			if (ImeMessageHandler.ImeWindow->Visible)
			{
				auto screenPos = ImeMessageHandler.TextBox->GetCaretScreenPos();
				ImeMessageHandler.ImeWindow->Draw(screenPos.x, screenPos.y);
			}
		}
			
	}

	void UIEntry::SetFocusedControl(Control *Target)
	{
		while (Target && !Target->AcceptsFocus)
			Target = Target->Parent;
		if (FocusedControl == Target)
			return;
		if (FocusedControl)
		{
			FocusedControl->LostFocus(Target);
			KillFocus();
		}
		if (!FocusedControl)
			KillFocus();
		auto parent = Target;
		bool formFound = false;
		while (parent)
		{
			if (parent->Type == CT_FORM)
			{
				this->ShowWindow((Form*)(parent));
				formFound = true;
				break;
			}
			parent = parent->Parent;
		}
	
		if (!formFound)
		{
			this->DeactivateAllForms();
			this->ActiveForm = nullptr;
		}
		FocusedControl = Target;
		if (Target && Target->Parent)
			Target->Parent->DoFocusChange();
		if (Target && (Target->Type & CT_IME_RECEIVER) != 0)
		{
			if (Target->Type == CT_IMETEXTBOX)
				ImeMessageHandler.TextBox = (TextBox*)(Target);
			else
				ImeMessageHandler.TextBox = (MultiLineTextBox*)(Target);
		}
		else
		{
			ImeMessageHandler.TextBox = nullptr;
		}
	}

	Control * UIEntry::FindControlAtPosition(int x, int y)
	{
		if (Visible)
		{
			auto checkCtrl = [&](Control * ctrl) -> Control*
			{
				int dx = 0;
				int dy = 0;
				if (ctrl->DockStyle == dsNone || ctrl->DockStyle == dsFill)
				{
					dx = clientRect.x;
					dy = clientRect.y;
				}
				int nx = x - dx;
				int ny = y - dy;
				if (auto child = ctrl->FindControlAtPosition(nx - ctrl->Left, ny - ctrl->Top))
					return child;
				return nullptr;
			};
			for (int i = Forms.Count() - 1; i >= 0; i--)
			{
				if (auto rs = checkCtrl(Forms[i]))
					return rs;
			}
			for (int i = controls.Count() - 1; i >= 0; i--)
			{
				if (auto rs = checkCtrl(controls[i].Ptr()))
					return rs;
			}
			return this;
		}
		return nullptr;
	}

	CheckBox::CheckBox(Container * parent)
		: Label(parent)
	{
		FontColor = Global::Colors.MenuItemForeColor;
		BackColor = Global::Colors.EditableAreaBackColor;
		TabStop = true;
		Type = CT_CHECKBOX;
		BorderStyle = BS_FLAT_;
		BackColor.A = 0;
		Checked = false;
	}

	CheckBox::CheckBox(Container * parent, const CoreLib::String & text, bool checked)
		: CheckBox(parent)
	{
		SetText(text);
		Checked = checked;
	}


	void CheckBox::ComputeAutoSize()
	{
		if (AutoSize)
		{
			this->Width = TextWidth + (int)(GetEntry()->CheckmarkLabel->TextWidth * 1.5f) + 2;
			this->Height = TextHeight + 1;
		}
	}

	void CheckBox::DoDpiChanged()
	{
		Label::DoDpiChanged();
		ComputeAutoSize();
	}

	void CheckBox::SetText(const CoreLib::String & pText)
	{
		Label::SetText(pText);
		ComputeAutoSize();
	}

	void CheckBox::Draw(int absX, int absY)
	{
		auto oldBorderStyle = BorderStyle;
		BorderStyle = BS_NONE;
		Control::Draw(absX, absY);
		BorderStyle = oldBorderStyle;
		absX = absX + Left;
		absY = absY + Top;
		auto entry = GetEntry();
		int checkBoxSize = GetEntry()->CheckmarkLabel->TextWidth;
		int checkBoxTop = (Height - checkBoxSize) >> 1;
		auto & graphics = entry->DrawCommands;
		graphics.SolidBrushColor = Global::Colors.EditableAreaBackColor;
		graphics.FillRectangle(absX + 1, absY + checkBoxTop + 1, absX + checkBoxSize, absY + checkBoxTop + checkBoxSize);
		//Draw Check Box
		Color lightColor, darkColor;
		if (BorderStyle == BS_LOWERED)
		{
			lightColor.R = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.R + COLOR_LIGHTEN, 0, 255);
			lightColor.G = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.G + COLOR_LIGHTEN, 0, 255);
			lightColor.B = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.B + COLOR_LIGHTEN, 0, 255);
			lightColor.A = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.A + COLOR_LIGHTEN, 0, 255);
			darkColor.R = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.R - COLOR_LIGHTEN, 0, 255);
			darkColor.G = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.G - COLOR_LIGHTEN, 0, 255);
			darkColor.B = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.B - COLOR_LIGHTEN, 0, 255);
			darkColor.A = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.A + COLOR_LIGHTEN, 0, 255);
		}
		else
		{
			lightColor = darkColor = Global::Colors.ControlBorderColor;
		}
		graphics.PenColor = darkColor;
		graphics.DrawLine(absX, absY + checkBoxTop, absX + checkBoxSize, absY + checkBoxTop);
		graphics.DrawLine(absX, absY + checkBoxTop + 1, absX, absY + checkBoxSize + checkBoxTop);
		graphics.PenColor = lightColor;
		graphics.DrawLine(absX + checkBoxSize, absY + checkBoxTop, absX + checkBoxSize, absY + checkBoxSize + checkBoxTop);
		graphics.DrawLine(absX + checkBoxSize - 1, absY + checkBoxSize + checkBoxTop, absX + 1, absY + checkBoxSize + checkBoxTop);
		// Draw check mark
		if (Checked)
		{
			auto checkMark = entry->CheckmarkLabel;
			checkMark->FontColor = FontColor;
			checkMark->Draw(absX + (checkBoxSize - checkMark->TextWidth) / 2 + 1,absY + checkBoxTop + (checkBoxSize - checkMark->TextHeight) / 2 - 1);
		}
		//Draw Caption
		int textStart = checkBoxSize + checkBoxSize / 4;
		BorderStyle = BS_NONE;
		Label::Draw(absX + textStart - Left, absY - Top);
		BorderStyle = oldBorderStyle;
		// Draw Focus Rect
		if (IsFocused())
		{
			graphics.PenColor = Global::Colors.FocusRectColor;
			graphics.DrawRectangle(absX + textStart, absY, absX + text->GetWidth() + textStart, absY + text->GetHeight());
		}
	}

	bool CheckBox::DoDblClick()
	{
		Control::DoDblClick();
		CheckBox::DoMouseDown(1, 1, 0);
		return true;
	}

	bool CheckBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Label::DoMouseDown(X,Y,Shift);
		if (!Enabled || !Visible)
			return false;
		Checked = !Checked;
		UI_MsgArgs Args;
		Args.Sender = this;
		Args.Type = MSG_UI_CHANGED;
		BroadcastMessage(&Args);
		return true;
	}

	bool CheckBox::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key,Shift);
		if (!Enabled || !Visible)
			return false;
		if (Key == 0x20 || Key == 0x0D) // VK_SPACE, VK_RETURN
		{
			Checked = !Checked; 
			UI_MsgArgs Args;
			Args.Sender = this;
			Args.Type = MSG_UI_CHANGED;
			BroadcastMessage(&Args);
		}
		return false;
	}

	RadioBox::RadioBox(Container * parent)
		: CheckBox(parent)
	{
		Type = CT_RADIOBOX;
	}

	bool RadioBox::GetValue()
	{
		return Checked;
	}

	void RadioBox::SetValue(bool AValue)
	{
		if (AValue)
		{
			if (Parent && (Parent->Type & CT_CONTAINER) != 0) // A type less then zero means the control is a container. 
			{
				for (int i=0; i<((Container * )Parent)->GetChildren().Count(); i++)
				{
					Control * curControl = ((Container * )Parent)->GetChildren()[i].Ptr();
					if (curControl->Type == CT_RADIOBOX)
					((RadioBox *)curControl)->Checked = false;
				}
				Checked = true;
			}
		}
	}

	void RadioBox::Draw(int absX, int absY)
	{
		auto oldBorderStyle = BorderStyle;
		BorderStyle = BS_NONE;
		Control::Draw(absX,absY);
		BorderStyle = oldBorderStyle;
		absX = absX + Left;
		absY = absY + Top;
		auto entry = GetEntry();
		int checkBoxSize = GetEntry()->CheckmarkLabel->TextWidth;
		int rad = checkBoxSize / 2 + 1;
		int dotX = absX + rad;
		int dotY = absY + (Height >> 1);
		auto & graphics = entry->DrawCommands;
		graphics.SolidBrushColor = Global::Colors.EditableAreaBackColor;
		graphics.FillEllipse((float)dotX - rad, (float)dotY - rad, (float)dotX + rad, (float)dotY + rad);

		if (BorderStyle == BS_LOWERED)
		{
			//Draw Check Box
			Color LightColor, DarkColor;
			LightColor.R = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.R + COLOR_LIGHTEN, 0, 255);
			LightColor.G = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.G + COLOR_LIGHTEN, 0, 255);
			LightColor.B = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.B + COLOR_LIGHTEN, 0, 255);
			LightColor.A = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.A + COLOR_LIGHTEN, 0, 255);
			DarkColor.R = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.R - COLOR_LIGHTEN, 0, 255);
			DarkColor.G = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.G - COLOR_LIGHTEN, 0, 255);
			DarkColor.B = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.B - COLOR_LIGHTEN, 0, 255);
			DarkColor.A = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.A + COLOR_LIGHTEN, 0, 255);
			graphics.PenColor = DarkColor;
			graphics.DrawArc(dotX, dotY, rad, Math::Pi / 4, Math::Pi * 5 / 4);
			graphics.PenColor = LightColor;
			graphics.DrawArc(dotX, dotY, rad, PI * 5 / 4, PI * 9 / 4);
		}
		else
		{
			graphics.PenColor = Global::Colors.ControlBorderColor;
			graphics.DrawArc(dotX, dotY, rad, 0.0f, Math::Pi * 2.0f);
		}
		float dotRad = rad * 0.5f;
		if (Checked)
		{
			// Draw dot
			graphics.SolidBrushColor = Global::Colors.ControlFontColor;
			graphics.FillEllipse((dotX + 0.5f - dotRad), (dotY + 0.5f - dotRad), (dotX + dotRad), (dotY + dotRad));
		}
		//Draw Caption
		int textStart = checkBoxSize + checkBoxSize / 4;
		BorderStyle = BS_NONE;
		Label::Draw(absX + textStart - Left, absY - Top);
		BorderStyle = oldBorderStyle;
		// Draw Focus Rect
		if (IsFocused())
		{
			graphics.PenColor = Global::Colors.FocusRectColor;
			graphics.DrawRectangle(absX + textStart, absY, absX + text->GetWidth() + textStart, absY + text->GetHeight());
		}
	}

	bool RadioBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X,Y,Shift);
		if (!Enabled || !Visible)
			return false;
		SetValue(true);
		return true;
	}

	bool RadioBox::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key,Shift);
		if (!Enabled || !Visible)
			return false;
		if (Key == 0x20 || Key == 0x0D) // VK_SPACE, VK_RETURN
		{
			SetValue(true);
		}
		return true;
	}

	CustomTextBox::CustomTextBox(Container * parent)
		: Container(parent)
	{
		Cursor = CursorType::IBeam;
		Type = CT_TEXTBOX;
		FText= "";
		font = parent->GetFont();
		SelectMode = false;
		TabStop = true;
		Locked = false; Changed = true;
		SelStart = SelLength = SelOrigin = 0;
		SelectionColor = Global::Colors.SelectionColor;
		SelectedTextColor = Global::Colors.SelectionForeColor;
		BorderStyle = BS_FLAT_;
		BackColor = Global::Colors.EditableAreaBackColor;
		FontColor = Global::Colors.ControlFontColor;
		TextBorderX =2; TextBorderY = 4;
		LabelOffset = TextBorderX;
		menu = new Menu(this, Menu::msPopup);
		auto mnCut = new MenuItem(menu, L"Cut", L"Ctrl+X");
		auto mnCopy = new MenuItem(menu, L"Copy", L"Ctrl+C");
		auto mnPaste = new MenuItem(menu, L"Paste", L"Ctrl+V");
		auto mnSelAll = new MenuItem(menu, L"Select All", L"Ctrl+A");
		mnCut->OnClick.Bind([this](auto) 
		{
			CopyToClipBoard();
			DeleteSelectionText();
		});
		mnCopy->OnClick.Bind([this](auto)
		{
			CopyToClipBoard();
		});
		mnPaste->OnClick.Bind([this](auto)
		{
			PasteFromClipBoard();
		});
		mnSelAll->OnClick.Bind([this](auto)
		{
			SelectAll();
		});
		time = CoreLib::Diagnostics::PerformanceCounter::Start();
		CursorPos = 0;
		KeyDown = false;
		DoDpiChanged();
	}

	const String CustomTextBox::GetText()
	{
		return FText;
	}

	void CustomTextBox::SetText(const String & AText)
	{
		FText = AText;
		Changed = true;
		CursorPos = FText.Length();
		SelLength = 0;
		OnChanged.Invoke(this);
	}

	void CustomTextBox::SetFont(IFont * AFont)
	{
		this->font = AFont;
		Changed = true;
	}

	void CustomTextBox::CursorPosChanged()
	{
		//Calculate Text offset.
		int txtWidth = font->MeasureString(FText).w;
		if (txtWidth <= Width-TextBorderX*2)
		{
			LabelOffset = TextBorderX;
		}
		else
		{
			String ls;
			ls = FText.SubString(0, CursorPos);
			int px = font->MeasureString(ls).w+LabelOffset;
			if (px>Width-TextBorderX)
			{
				int delta = px-(Width-TextBorderX);
				LabelOffset-=delta;
			}
			else if (px<TextBorderX && LabelOffset<2)
			{
				LabelOffset += 40;
				if (LabelOffset>2)
					LabelOffset = 2;
			}
		}
	}

	int CustomTextBox::HitTest(int posX)
	{
		String curText;
		posX -= LabelOffset;
		curText = "";
		for (int i =0;i<FText.Length();i++)
		{
			curText = curText + FText[i];
			int tw = font->MeasureString(curText).w;
			if (tw>posX)
			{
				int cw = font->MeasureString(FText[i]).w;
				cw /=2;
				if (tw-cw>posX)
				{
					return i;
				}
				else
				{
					return i+1;
				}
			}
		}
		return FText.Length();
	}


	String DeleteString(String src, int start, int len)
	{
		return src.SubString(0, start) + src.SubString(start + len, src.Length() - start - len);
	}


	bool CustomTextBox::DoInput(const String & AInput)
	{
		if (AInput == L"\t")
			return false;
		if (Locked)
			return true;
		String nStr;
		nStr = AInput;
		if (SelLength !=0)
		{
			DeleteSelectionText();
		}
		if (CursorPos!=FText.Length())
			FText = FText.SubString(0, CursorPos) + nStr + FText.SubString(CursorPos, FText.Length() - CursorPos);
		else
			FText = FText + AInput;
		TextChanged();
		CursorPos += nStr.Length();
		SelStart = CursorPos;
		return true;
	}

	void CustomTextBox::CopyToClipBoard()
	{
		if( SelLength != 0)
		{
			GetEntry()->System->SetClipboardText(FText.SubString(SelStart, SelLength));
		}
	}
	void CustomTextBox::PasteFromClipBoard()
	{
		DeleteSelectionText();
		auto txt = GetEntry()->System->GetClipboardText();
		int fid = txt.IndexOf(L'\r');
		if (fid != -1)
			txt = txt.SubString(0, fid);
		fid = txt.IndexOf(L'\n');
		if (fid != -1)
			txt = txt.SubString(0, fid);
		DoInput(txt);
	}


	void CustomTextBox::DeleteSelectionText()
	{
		if (SelLength != 0 && !Locked)
		{
			if (SelStart + SelLength > FText.Length())
				SelLength = FText.Length() - SelStart;
			FText = DeleteString(FText, SelStart, SelLength);
			TextChanged();
			SelLength=0;
			CursorPos = SelStart;
		}
	}

	bool CustomTextBox::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key,Shift);
		if (Enabled && Visible)
		{
			KeyDown=true;
			if (Shift==SS_SHIFT)
			{
				int selEnd;
				selEnd = SelStart+SelLength;
				if (Key == 0x25) // VK_LEFT
				{
					if (CursorPos==0)
						return false;
					CursorPos --;
					if (CursorPos<SelStart)
					{
						SelStart = CursorPos;
						SelLength = selEnd-CursorPos;
					}
					else if (CursorPos>SelStart)
						SelLength = CursorPos-SelStart;
					else
					{
						SelStart=CursorPos;
						SelLength =0;
					}
					cursorPosChanged = true;
				}
				else if(Key == 0x27) // VK_RIGHT
				{
					if (CursorPos==FText.Length())
						return false;
					CursorPos ++;
					if (CursorPos<selEnd)
					{
						SelStart = CursorPos;
						SelLength = selEnd-CursorPos;
					}
					else if (CursorPos>selEnd)
						SelLength = CursorPos-SelStart;
					else
					{
						SelStart= CursorPos;
						SelLength = 0;
					}	
					cursorPosChanged = true;
				}
				return true;
			}
			else if (Shift == SS_CONTROL)
			{
				if (Key == L'C')
				{
					CopyToClipBoard();
				}
				else if (Key == L'V')
				{
					DeleteSelectionText();
					if (!Locked)
						PasteFromClipBoard();
				}
				else if (Key == L'X')
				{
					CopyToClipBoard();
					DeleteSelectionText();
				}
				else if (Key == L'A')
				{
					SelectAll();
				}
				return true;
			}
			else if (Shift == 0)
			{
				if (Key == 0x25) // VK_LEFT
				{
					if (SelLength == 0)
						CursorPos--;
					else
					{
						CursorPos = SelStart;
					}
					SelLength = 0;
					SelStart = CursorPos=ClampInt(CursorPos,0,FText.Length());
					cursorPosChanged = true;
					return true;
				}			
				else if (Key == 0x27) // VK_RIGHT
				{
					if (SelLength ==0)
						CursorPos++;
					else
						CursorPos = SelStart+SelLength;
					SelLength = 0;
					SelStart = CursorPos=ClampInt(CursorPos,0,FText.Length());
					cursorPosChanged = true;
					return true;
				}
				else if (Key == 0x2E && !Locked) // VK_DELETE
				{
					if (SelLength!=0)
					{
						FText = DeleteString(FText, SelStart, SelLength);
						TextChanged();
						SelLength=0;
						CursorPos = SelStart;
						cursorPosChanged = true;
					}
					else if (CursorPos<(int)FText.Length())
					{
						FText = DeleteString(FText, CursorPos, 1);
						TextChanged();
					}
					return true;
				}
				else if (Key == 0x08 && !Locked) // VK_BACK
				{
					if (SelLength != 0)
					{
						DeleteSelectionText();
						cursorPosChanged = true;
					}
					else if (CursorPos>0)
					{
						FText = DeleteString(FText, CursorPos-1, 1);
						CursorPos--;
						TextChanged();
					}
					return true;
				}
			}
		}
		return true;
	}

	bool CustomTextBox::DoKeyPress(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyPress(Key,Shift);
		if ((Shift & SS_CONTROL) == 0)
		{
			if (Key >= 32)
			{
				DoInput((wchar_t)Key);
				return true;
			}
		}
		return true;
	}

	void CustomTextBox::TextChanged()
	{
		cursorPosChanged = true;
		Changed = true;
		UI_MsgArgs Args;
		Args.Sender = this;
		Args.Type = MSG_UI_CHANGED;
		BroadcastMessage(&Args);
	}

	bool CustomTextBox::DoKeyUp(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyUp(Key,Shift);
		KeyDown = false;
		return true;
	}

	bool CustomTextBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X, Y, Shift);
		if (Enabled && Visible)
		{
			time = CoreLib::Diagnostics::PerformanceCounter::Start();
			SetFocus();
			if (Shift & SS_BUTTONLEFT)
			{
				SelLength = 0;
				SelStart = HitTest(X);
				CursorPos = SelStart;
				SelectMode = true;
				SelOrigin = CursorPos;
				cursorPosChanged = true;
				Global::MouseCaptureControl = this;
			}
			return true;
		}
		else
			SelectMode = false;
		return false;
	}

	bool CustomTextBox::DoMouseMove(int X , int Y)
	{
		Control::DoMouseMove(X,Y);
		if (Enabled && Visible)
		{
			if (SelectMode)
			{
				int cp = HitTest(X);
				if (cp < SelOrigin)
				{
					SelStart = cp;
					SelLength = SelOrigin - cp;
				}
				else if (cp >= SelOrigin)
				{
					SelStart = SelOrigin;
					SelLength = cp - SelOrigin;
				}
				CursorPos = cp;
				cursorPosChanged = true;
			}
			return true;
		}
		return false;
	}

	bool CustomTextBox::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X,Y,Shift);
		SelectMode = false;
		ReleaseMouse();
		if (Enabled && Visible)
		{
			if (Shift == SS_BUTTONRIGHT)
			{
				menu->Popup(X, Y);
			}
			return true;
		}
		return false;
	}

	void CustomTextBox::SelectAll()
	{
		SelStart = 0;
		SelLength = FText.Length();
	}

	void CustomTextBox::Posit(int pLeft, int pTop, int pWidth, int /*pHeight*/)
	{
		Control::Posit(pLeft, pTop, pWidth, Height);
	}

	void CustomTextBox::DoDpiChanged()
	{
		Changed = true;
		if (font)
			Height = (int)(font->MeasureString(L"M").h * 1.2f);
		Container::DoDpiChanged();
	}

	bool IsSeparatorChar(wchar_t ch)
	{
		bool isLetter = ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == L'_');
		return !isLetter;
	}

	bool CustomTextBox::DoDblClick()
	{
		if (CursorPos >= FText.Length())
			SelectAll();
		else
		{
			int begin = CursorPos;
			while (begin > 0 && !IsSeparatorChar(FText[begin - 1]))
				begin--;
			int end = CursorPos;
			while (end < FText.Length() && !IsSeparatorChar(FText[end]))
				end++;
			SelStart = begin;
			SelLength = end - begin;
			CursorPos = end;
			cursorPosChanged = true;
		}
		return true;
	}

	void CustomTextBox::Draw(int absX, int absY)
	{
		Control::Draw(absX,absY);
		absX+=Left; absY+=Top;	
		auto entry = GetEntry();
		if (font == nullptr)
		{
			font = entry->System->LoadDefaultFont();
			Changed = true;
		}
		if (cursorPosChanged)
		{
			cursorPosChanged = false;
			CursorPosChanged();
		}
		if (Changed)
		{
			text = font->BakeString(FText);
			Changed = false;
		}
		auto & graphics = entry->DrawCommands;
		//Draw Text
		Rect textRect;
		textRect.x = absX+TextBorderX; textRect.y = absY; textRect.w = Width-TextBorderX-TextBorderX; textRect.h = Height;
		entry->ClipRects->AddRect(textRect);
		graphics.SolidBrushColor = FontColor;
		graphics.DrawTextQuad(text.Ptr(), absX+LabelOffset,absY+TextBorderY);
		entry->ClipRects->PopRect();
		String ls;
		ls= FText.SubString(0, CursorPos);
		
		int spX=0,epX=0;
		//Draw Selection Rect
			
		if ((IsFocused() || menu->Visible) && SelLength!=0)
		{
			if (SelStart + SelLength > FText.Length())
				SelLength = FText.Length() - SelStart;
			ls = FText.SubString(0, SelStart);
			spX = font->MeasureString(ls).w;
			ls = FText.SubString(0, SelStart+SelLength);
			epX = font->MeasureString(ls).w;
			spX+=LabelOffset+absX; epX+=LabelOffset+absX;
			graphics.SolidBrushColor = SelectionColor;
			graphics.FillRectangle(spX, absY + TextBorderX, epX - 1, absY + Height - TextBorderX);
			entry->ClipRects->AddRect(Rect(spX, absY + TextBorderX, epX - 1 - spX, Height - TextBorderX));
			graphics.SolidBrushColor = Color(255, 255, 255, 255);
			graphics.DrawTextQuad(text.Ptr(), absX + LabelOffset, absY + TextBorderY);
			entry->ClipRects->PopRect();
		}
			
		//Draw Cursor
		float timePassed = CoreLib::Diagnostics::PerformanceCounter::EndSeconds(time);
		int tick = int(timePassed / CURSOR_FREQUENCY);
		if (IsFocused() && ((tick&1)==0 || KeyDown))
		{
			int csX = font->MeasureString(ls).w;
			csX += LabelOffset;
			AbsCursorPosX = absX+csX;
			AbsCursorPosY = absY+Height-TextBorderX;
			graphics.PenColor = Color(255 - BackColor.R, 255 - BackColor.G, 255 - BackColor.B, 255);
			graphics.DrawLine(AbsCursorPosX, absY + TextBorderX, AbsCursorPosX, AbsCursorPosY);
		}

	}

	void TextBox::ImeInputString(const String & txt)
	{
		DoInput(txt);
	}

	bool TextBox::DoKeyPress(unsigned short Key, SHIFTSTATE Shift)
	{
		CustomTextBox::DoKeyPress(Key,Shift);
		if (!IsFocused() || !Enabled ||!Visible)
			return false;
		return false;
	}

	VectorMath::Vec2i TextBox::GetCaretScreenPos()
	{
		return Vec2i::Create(AbsCursorPosX, AbsCursorPosY);
	}

	IMEWindow::IMEWindow(Container * parent)
		: Container(parent)
	{
		lblCompStr = new Label(this);
		Panel = new Control(this);
		Panel->BorderStyle = BS_FLAT_;
	}

	IMEWindow::~IMEWindow()
	{
	}

	void IMEWindow::ChangeCompositionString(String AString)
	{
		lblCompStr->SetText(AString);
		strComp = AString;
	}

	void IMEWindow::Draw(int absX, int absY)
	{
		int maxW=0;
		int height;
		int cpx,cpy;
		const int panelMargin = 4;
		absX += panelMargin;
		if (strComp.Length())
		{
			if (lblCompStr->TextWidth+absX > WindowWidth)
				cpx = WindowWidth-lblCompStr->TextWidth;
			else
				cpx=absX;
			if (lblCompStr->TextHeight+absY>WindowHeight)
				cpy = absY - 40;
			else
				cpy = absY;
			height = lblCompStr->TextHeight;
			maxW = lblCompStr->TextWidth;
			Panel->Left = cpx - panelMargin;
			Panel->Top = cpy - panelMargin;
			Panel->SetWidth(maxW + panelMargin * 2);
			Panel->SetHeight(height + panelMargin * 2);
			Panel->Draw(0,0);
			lblCompStr->Draw(cpx,cpy);		
		}
	}
	
	void IMEHandler::Init(UIEntry * entry)
	{
		TextBox = NULL;
		ImeWindow = new GraphicsUI::IMEWindow(entry);
	}

	bool IMEHandler::DoImeStart()
	{
		ImeWindow->ChangeCompositionString(String(L""));
		ImeWindow->Visible = true;
		return true;
	}

	bool IMEHandler::DoImeEnd()
	{
		ImeWindow->Visible = false;
		return true;
	}

	bool IMEHandler::DoImeCompositeString(const CoreLib::String & str)
	{
		ImeWindow->ChangeCompositionString(str);
		return false;
	}

	bool IMEHandler::DoImeResultString(const CoreLib::String & str)
	{
		StringInputed(str);
		return true;
	}

	void IMEHandler::StringInputed(String AString)
	{
		if (TextBox)
			TextBox->ImeInputString(AString);
	}

	ScrollBar::ScrollBar(Container * parent, bool addToParent)
		: Container(parent, addToParent)
	{
		Type = CT_SCROLLBAR;
		BorderStyle = BS_NONE;
		BackColor = Global::Colors.ScrollBarBackColor;
		btnInc = new Button(this);
		btnDec = new Button(this);
		Slider = new Control(this);
		btnInc->TabStop = false;
		btnDec->TabStop = false;
		btnInc->BackColor.A = 0;
		btnDec->BackColor.A = 0;
		btnInc->SetFont(GetEntry()->System->LoadDefaultFont(DefaultFontType::Symbol));
		btnDec->SetFont(GetEntry()->System->LoadDefaultFont(DefaultFontType::Symbol));
		Min = 0; Max = 100; Position = 0;
		btnInc->OnMouseDown.Bind(this, &ScrollBar::BtnIncMouseDown);
		btnDec->OnMouseDown.Bind(this, &ScrollBar::BtnDecMouseDown);
		btnInc->OnMouseUp.Bind(this, &ScrollBar::BtnIncMouseUp);
		btnDec->OnMouseUp.Bind(this, &ScrollBar::BtnDecMouseUp);
		btnInc->BorderStyle = BS_NONE;
		btnDec->BorderStyle = BS_NONE;
		btnInc->BorderColor.A = 0;
		btnDec->BorderColor.A = 0;
		Slider->BorderStyle = BS_NONE;
		Slider->BackColor = Global::Colors.ScrollBarSliderColor;
		btnInc->FontColor = btnDec->FontColor = Global::Colors.ScrollBarForeColor;
		Slider->OnMouseEnter.Bind([this](auto) {Slider->BackColor = Global::Colors.ScrollBarHighlightColor; });
		Slider->OnMouseLeave.Bind([this](auto) {Slider->BackColor = Global::Colors.ScrollBarForeColor; });

		SetOrientation(SO_HORIZONTAL);
		SetValue(0,100,0,20);
		SmallChange = 1;
		LargeChange = 10;
		DownInSlider = false;
		
	}

	ScrollBar::ScrollBar(Container * parent)
		: ScrollBar(parent, true)
	{
	}

	ScrollBar::~ScrollBar()
	{
	}

	void ScrollBar::Draw(int absX, int absY)
	{
		if (!Visible) return;
		Control::Draw(absX,absY);
		absX+=Left; absY+=Top;
		if (DownInSlider)
			Slider->BackColor = Global::Colors.ScrollBarPressedColor;
		else
		{
			if (highlightSlider)
				Slider->BackColor = Global::Colors.ScrollBarHighlightColor;
			else
				Slider->BackColor = Global::Colors.ScrollBarSliderColor;
		}
		btnInc->Draw(absX,absY);
		btnDec->Draw(absX,absY);
		if (Slider->Visible)
			Slider->Draw(absX, absY);
	}

	void ScrollBar::SetOrientation(int NewOri)
	{
		Orientation = NewOri;
		Position = Min;
		SetValue(Min,Max,Position, PageSize);
		if (NewOri == SO_HORIZONTAL)
		{
			Height = Global::SCROLLBAR_BUTTON_SIZE;
			btnInc->SetText(L"4"); 
			btnDec->SetText(L"3");
		}
		else
		{
			Width = Global::SCROLLBAR_BUTTON_SIZE;
			btnInc->SetText(L"6"); 
			btnDec->SetText(L"5");
		}
		SizeChanged();
	}

	void ScrollBar::SizeChanged()
	{
		Control::SizeChanged();
		if (Orientation == SO_HORIZONTAL)
		{
			btnDec->Posit(0,0,Global::SCROLLBAR_BUTTON_SIZE,Height);
			btnInc->Posit(Width- Global::SCROLLBAR_BUTTON_SIZE,0, Global::SCROLLBAR_BUTTON_SIZE,Height);
			Slider->Posit(Global::SCROLLBAR_BUTTON_SIZE,0,PageSize,Height);
		}
		else
		{
			btnDec->Posit(0,0,Width, Global::SCROLLBAR_BUTTON_SIZE);
			btnInc->Posit(0,Height- Global::SCROLLBAR_BUTTON_SIZE,Width, Global::SCROLLBAR_BUTTON_SIZE);
			Slider->Posit(0, Global::SCROLLBAR_BUTTON_SIZE,Width,PageSize);
		}
		SetValue(Min,Max,Position, PageSize);
	}

	void ScrollBar::DoDpiChanged()
	{
		Container::DoDpiChanged();
		SizeChanged();
	}

	int ScrollBar::GetOrientation()
	{
		return Orientation;
	}

	int ScrollBar::GetMax()
	{
		return Max;
	}

	int ScrollBar::GetMin()
	{
		return Min;
	}

	int ScrollBar::GetPosition()
	{
		return Position;
	}
	int ScrollBar::GetPageSize()
	{
		return PageSize;
	}
	void ScrollBar::SetMax(int AMax)
	{
		SetValue(Min,AMax,Position, PageSize);
	}

	void ScrollBar::SetMin(int AMin)
	{
		SetValue(AMin,Max,Position, PageSize);
	}

	void ScrollBar::SetPosition(int APos)
	{
		SetValue(Min,Max,APos, PageSize);
	}

	void ScrollBar::SetValue(int AMin, int AMax, int APos, int pageSize)
	{
		int FreeSlide = (Orientation==SO_HORIZONTAL)?Width-(Global::SCROLLBAR_BUTTON_SIZE+1)*2:
														Height-(Global::SCROLLBAR_BUTTON_SIZE+1)*2;
		if (AMin>=0 && AMax>AMin && APos>=AMin && APos<=AMax)
		{
			bool Changed = (AMin != Min || AMax !=Max || APos !=Position);
			Min = AMin;
			Max = AMax;
			Position = APos;
			if (Changed)
			{
				UI_MsgArgs Args;
				Args.Sender = this;
				Args.Type = MSG_UI_CHANGED;
				BroadcastMessage(&Args);
				OnChanged.Invoke(this);
			}
				
			PageSize = pageSize;
			float p = PageSize/(float)(PageSize+AMax-AMin);
			int slideSize = Math::Max((int)(p*FreeSlide), GetEntry()->GetLineHeight());
			int spos = (int)((FreeSlide - slideSize)*(APos/(float)(AMax-AMin))) + Global::SCROLLBAR_BUTTON_SIZE + 1;
			if (Orientation == SO_HORIZONTAL)
			{	
				Slider->Left = spos;
				Slider->SetWidth(slideSize);
			}
			else
			{
				Slider->Top = spos;
				Slider->SetHeight(slideSize);
			}
			Slider->Visible = true;
		}
		else
		{
			Slider->Visible = false;
			Min = Max = Position = 0;
		}
	}

	bool ScrollBar::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoMouseDown(X,Y,Shift);
		DownInSlider = false;
		DownPosX = X; DownPosY=Y;
		if (PointInSlider(X,Y))
		{
			DownInSlider = true;
			OriPos = Position;
			Global::MouseCaptureControl = this;
		}
		else if (PointInFreeSpace(X,Y))
		{
			int nPos = Position;
			if (Orientation == SO_HORIZONTAL)
			{
				if (X>Slider->Left)
					nPos += LargeChange;
				else
					nPos -= LargeChange;
			}
			else
			{
				if (Y>Slider->Top)
					nPos += LargeChange;
				else
					nPos -= LargeChange;
			}
			nPos = Math::Clamp(nPos, Min, Max);
			SetPosition(nPos);
		}
		auto hitTest = Container::FindControlAtPosition(X, Y);
		if (hitTest == btnDec || hitTest == btnInc)
			hitTest->DoMouseDown(X - hitTest->Left, Y - hitTest->Top, Shift);
		return true;
	}

	bool ScrollBar::DoMouseMove(int X, int Y)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoMouseMove(X, Y);
		int Delta, FreeSpace, Range, APos;
		if (DownInSlider)
		{
			Range = Max-Min;
			int slideSize = (Orientation == SO_HORIZONTAL?Slider->GetWidth():Slider->GetHeight());
			FreeSpace = (Orientation == SO_HORIZONTAL?Width:Height)-(Global::SCROLLBAR_BUTTON_SIZE+1)*2-slideSize;
			if (Orientation == SO_HORIZONTAL)
			{
				Delta = X-DownPosX;
			}
			else
			{
				Delta = Y-DownPosY;
			}
			APos = OriPos + (int)(Delta*(float)Range/(float)FreeSpace);
			APos = Math::Max(Min, APos);
			APos = Math::Min(Max, APos);
			SetPosition(APos);
		}
		auto hitTest = Container::FindControlAtPosition(X, Y);
		if (hitTest == btnDec || hitTest == btnInc)
			hitTest->DoMouseMove(X - hitTest->Left, Y - hitTest->Top);
		else if (hitTest == Slider)
			highlightSlider = true;
		return true;
	}

	bool ScrollBar::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoMouseUp(X,Y,Shift);

		DownPosX=DownPosY=0;
		DownInSlider = false;
		auto hitTest = Container::FindControlAtPosition(X, Y);
		if (hitTest == btnDec || hitTest == btnInc)
			hitTest->DoMouseUp(X - hitTest->Left, Y - hitTest->Top, Shift);
		ReleaseMouse();
		return true;
	}

	bool ScrollBar::DoMouseLeave()
	{
		highlightSlider = false;
		return false;
	}

	bool ScrollBar::DoMouseHover()
	{
		if (tmrOrientation != -1)
			GetEntry()->SubscribeTickEvent(this);
		return false;
	}

	bool ScrollBar::DoTick()
	{
		if (tmrOrientation == 0)
		{
			if (Position - SmallChange >= Min)
			{
				SetPosition(Position - SmallChange);
			}
			else
				SetPosition(Min);
		}
		else
		{
			if (Position + SmallChange <= Max)
			{
				SetPosition(Position + SmallChange);
			}
			else
				SetPosition(Max);
		}
		return true;
	}

	void ScrollBar::BtnDecMouseDown(UI_Base *, UIMouseEventArgs &)
	{
		if (Position-SmallChange>=Min)
		{
			SetPosition(Position-SmallChange);
			tmrOrientation = 0;
		}
	}

	void ScrollBar::BtnIncMouseDown(UI_Base *, UIMouseEventArgs &)
	{
		if (Position+SmallChange<=Max)
		{
			SetPosition(Position+SmallChange);
			tmrOrientation = 1;
		}
	}

	void ScrollBar::BtnDecMouseUp(UI_Base *, UIMouseEventArgs &)
	{
		tmrOrientation = -1;
		GetEntry()->UnSubscribeTickEvent(this);
	}

	void ScrollBar::BtnIncMouseUp(UI_Base *, UIMouseEventArgs &)
	{
		tmrOrientation = -1;
		GetEntry()->UnSubscribeTickEvent(this);
	}

	void ScrollBar::HandleMessage(const UI_MsgArgs *Args)
	{
		Control::HandleMessage(Args);
	}

	bool ScrollBar::PointInSlider(int X, int Y)
	{
		return (X>=Slider->Left && X<=Slider->Left+Slider->GetWidth() && Y>=Slider->Top && Y<=Slider->Top+Slider->GetHeight());
	}

	bool ScrollBar::PointInFreeSpace(int X, int Y)
	{
		if (PointInSlider(X,Y))
			return false;
		if (Orientation == SO_HORIZONTAL)
		{
			return (Y>0 && Y<Height && X>Global::SCROLLBAR_BUTTON_SIZE && X<Width- Global::SCROLLBAR_BUTTON_SIZE);
		}
		else
		{
			return (X>0 && X<Width && Y>Global::SCROLLBAR_BUTTON_SIZE && Y<Height- Global::SCROLLBAR_BUTTON_SIZE);
		}
	}


	ListBox::ListBox(Container * parent)
		: Container(parent)
	{
		Type = CT_LISTBOX;
		TabStop = true;
		BorderStyle = BS_FLAT_;
		BackColor = Global::Colors.EditableAreaBackColor;
		HideSelection = false;
		MultiSelect = false;
		Selecting = false;
		DownInItem = false;
		HotTrack = false;
		SelectedIndex= -1;
		Padding = 1;
		SelectionColor = Global::Colors.SelectionColor;
		HighLightColor = Global::Colors.HighlightColor;
		HighLightForeColor = Global::Colors.HighlightForeColor;
		SelectionForeColor = Global::Colors.SelectionForeColor;
		FontColor = Global::Colors.ControlFontColor;
		UnfocusedSelectionColor = Global::Colors.UnfocusedSelectionColor;
		HighLightColor = Global::Colors.HighlightColor;
		ScrollBar = new GraphicsUI::ScrollBar(this);
		ScrollBar->SetOrientation(SO_VERTICAL);
		ScrollBar->Visible = false;
		BorderWidth = 2;
		DoDpiChanged();
	}

	bool ListBox::ItemInSelection(Control *Item)
	{
		for (int i=0; i<Selection.Count(); i++)
		{
			if (Selection[i] == Item)
				return true;
		}
		return false;
	}

	void ListBox::Draw(int absX, int absY)
	{
		Control::Draw(absX,absY);
		if (!Visible) return;
		absX+=Left;absY+=Top;
		int ShowCount = Height / ItemHeight +1;
		int bdr = (ScrollBar->Visible?ScrollBar->GetWidth():0);
		auto entry = GetEntry();
		entry->ClipRects->AddRect(Rect(absX+BorderWidth, absY + BorderWidth, Width-BorderWidth*2 - bdr, Height-BorderWidth*2));
		bool focused = IsFocused();
		auto & graphics = entry->DrawCommands;
		for (int i=ScrollBar->GetPosition();i<=ScrollBar->GetPosition()+ShowCount && i<Items.Count();i++)
		{
			Control *CurItem = Items[i];
			if (i==HighLightID)
			{
				CurItem->BackColor = HighLightColor;
				CurItem->FontColor = HighLightForeColor;
			}
			else if (SelectedIndex ==i || ItemInSelection(CurItem))
			{

				CurItem->BackColor = HideSelection && !focused ? BackColor : (focused ? SelectionColor : UnfocusedSelectionColor);
				CurItem->FontColor = SelectionForeColor;
			}
				
			else
			{
				CurItem->BackColor = BackColor;
				CurItem->FontColor = FontColor;
			}
			CurItem->Posit(BorderWidth,BorderWidth+(i-ScrollBar->GetPosition())*ItemHeight,Width-BorderWidth*2-bdr, ItemHeight);
			graphics.SolidBrushColor = CurItem->BackColor;
			graphics.FillRectangle(absX + BorderWidth, absY + CurItem->Top, absX + Width - BorderWidth, absY + CurItem->Top + CurItem->GetHeight());
			CurItem->Draw(absX,absY);
		}
		if (focused && AcceptsFocus)
		{
			int FID =SelectedIndex;
			if (FID==-1) FID =0;
			bdr = BorderWidth*2;
			if (ScrollBar->Visible)	bdr += ScrollBar->GetWidth()+1;
			int RectX1 = BorderWidth+absX;
			int RectX2 = RectX1 + Width - bdr;
			int RectY1 = (FID-ScrollBar->GetPosition())*ItemHeight+absY+BorderWidth-1;
			int RectY2 = RectY1+ItemHeight+1;
			graphics.PenColor = Global::Colors.FocusRectColor;
			graphics.DrawRectangle(RectX1, RectY1, RectX2, RectY2);
		}
		entry->ClipRects->PopRect();
		ScrollBar->Draw(absX,absY);
	}

	void ListBox::SizeChanged()
	{
		ScrollBar->Posit(Width - Global::SCROLLBAR_BUTTON_SIZE - BorderWidth, BorderWidth, Global::SCROLLBAR_BUTTON_SIZE, Height - BorderWidth*2);
		ListChanged();
	}

	bool ListBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		lastSelIdx = SelectedIndex;
		Control::DoMouseDown(X,Y,Shift);
		int bdr=0,ShowCount=Height/ItemHeight;
		if (!Enabled || !Visible)
			return false;
		Selecting = false;
		DownInItem = false;
		auto hitTest = Container::FindControlAtPosition(X, Y);
		for (int i=ScrollBar->GetPosition();i<=ScrollBar->GetPosition()+ShowCount && i<Items.Count();i++)
		{
			Control *CurItem = Items[i];
			if (hitTest == CurItem || (hitTest && hitTest->IsChildOf((Container*)CurItem)))
				CurItem->DoMouseDown(X-CurItem->Left, Y-CurItem->Top, Shift);
		}
		if (ScrollBar->Visible)
		{
			if (hitTest == ScrollBar)
				ScrollBar->DoMouseDown(X-hitTest->Left, Y-hitTest->Top, Shift);
			bdr = ScrollBar->GetWidth();
		}
		if (X < Width-bdr)
		{
			DownInItem = true;
			auto newSelIdx = HitTest(X,Y);
			SelectedIndex = newSelIdx;
			if (MultiSelect)
			{
				Selecting = true;
				SelOriX = X;
				SelOriY = Y+ScrollBar->GetPosition()*ItemHeight+BorderWidth;
			}
		}
		if (hitTest != ScrollBar)
			Global::MouseCaptureControl = this;
		return true;
	}

	bool ListBox::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key,Shift);
		if (!Enabled || !Visible)
			return false;
		int ShowCount=Height/ItemHeight;
		for (int i = ScrollBar->GetPosition();i <= ScrollBar->GetPosition()+ShowCount && i < Items.Count();i++)
		{
			Control *CurItem =Items[i];
			CurItem->DoKeyDown(Key,Shift);
		}
		if (Items.Count())
		{
			if (Key == 0x28) // VK_DOWN
			{
				SelectedIndex = ClampInt(SelectedIndex+1,0,Items.Count()-1);
				SelectionChanged();

			}
			else if (Key == 0x26) // VK_UP
			{
				SelectedIndex = ClampInt(SelectedIndex-1,0,Items.Count()-1);
				SelectionChanged();
					
			}
			int sy =(SelectedIndex-ScrollBar->GetPosition())*ItemHeight+BorderWidth-1;
			if (sy<=5)
			{
				ScrollBar->SetPosition(ClampInt(SelectedIndex,0,ScrollBar->GetMax()));
			}
			else if (sy>Height-ItemHeight-5)
			{
				ScrollBar->SetPosition(ClampInt(SelectedIndex-Height / ItemHeight +1,0,ScrollBar->GetMax()));
			}
		}
		return false;
	}

	bool ListBox::DoMouseLeave()
	{
		this->ScrollBar->DoMouseLeave();
		return false;
	}

	void ListBox::DoDpiChanged()
	{
		ItemHeight = 18;
		if (font)
			ItemHeight = (int)(font->MeasureString(L"M").h * 1.1f);
		Container::DoDpiChanged();
	}

	bool ListBox::DoMouseMove(int X, int Y)
	{
		Control::DoMouseMove(X,Y);
		if (!Enabled || !Visible)
			return false;
		auto hitTest = Container::FindControlAtPosition(X, Y);
		int bdr = ScrollBar->Visible?ScrollBar->GetWidth():0;
		if (ScrollBar->Visible && hitTest == ScrollBar)
			ScrollBar->DoMouseMove(X - hitTest->Left, Y - hitTest->Top);
		else
			ScrollBar->DoMouseLeave();
		int ShowCount=Height/ItemHeight;
		for (int i = ScrollBar->GetPosition(); i <= ScrollBar->GetPosition() + ShowCount && i<Items.Count(); i++)
		{
			Control *CurItem = Items[i];
			if (hitTest == CurItem || (hitTest && hitTest->IsChildOf((Container*)CurItem)))
				CurItem->DoMouseMove(X - CurItem->Left, Y - CurItem->Top);
		}
		if (Selecting)
		{
			Selection.Clear();
			int cX,cY;
			cX = X;
			cY = Y - BorderWidth + ScrollBar->GetPosition()*ItemHeight;
			if (SelOriY>cY)
			{
				int tmpY = cY;
				cY=SelOriY;SelOriY=tmpY;
			}
			int idBegin = SelOriY / ItemHeight;
			int idEnd = cY / ItemHeight;
			if (idBegin<Items.Count())
			{
				if (idEnd>=Items.Count()) idEnd = Items.Count()-1;
				SelectedIndex=idEnd;
				for (int i=idBegin;i<=idEnd;i++)
				{
					Selection.Add(Items[i]);
				}
				auto newSelIdx = idEnd;
				SelectedIndex = newSelIdx;
			}
		}
		else if (DownInItem)
		{
			auto newSelIdx = HitTest(X, Y);
			SelectedIndex = newSelIdx;
		}
		if (DownInItem && ScrollBar->Visible)
		{
			if (Y>=Height)
			{
				if (ScrollBar->GetPosition()<ScrollBar->GetMax())
					ScrollBar->SetPosition(ScrollBar->GetPosition()+1);
			}
			else if (Y<0)
			{
				if (ScrollBar->GetPosition()>ScrollBar->GetMin())
					ScrollBar->SetPosition(ScrollBar->GetPosition()-1);
			}
		}
		if (HotTrack && X>0 && X<Width - bdr &&Y>0 && Y<Height)
		{
			HighLightID = HitTest(X,Y);
		}
		else
		{
			HighLightID = -1;
		}
		return true;
	}

	bool ListBox::DoMouseWheel(int delta)
	{
		if (Visible && Enabled && ScrollBar->Visible)
		{
			ScrollBar->SetPosition(Math::Clamp(ScrollBar->GetPosition() + (delta > 0 ? -1 : 1) * 3, 0, ScrollBar->GetMax()));
			return true;
		}
		return false;
	}

	bool ListBox::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X,Y,Shift);
		if (!Enabled || !Visible)
			return false;
		int ShowCount=Height/ItemHeight;
		auto hitTest = Container::FindControlAtPosition(X, Y);
		for (int i = ScrollBar->GetPosition(); i <= ScrollBar->GetPosition() + ShowCount && i<Items.Count(); i++)
		{
			Control *CurItem = Items[i];
			if (hitTest == CurItem || (hitTest && hitTest->IsChildOf((Container*)CurItem)))
				CurItem->DoMouseUp(X - CurItem->Left, Y - CurItem->Top, Shift);
		}
		DownInItem = false;
		Selecting = false;
		if (ScrollBar->Visible && hitTest == ScrollBar)
			ScrollBar->DoMouseUp(X - hitTest->Left, Y - hitTest->Top,Shift);
		if (lastSelIdx != SelectedIndex || (Items.Count() && Items[0]->Type == CT_CHECKBOX))
		{
			SelectionChanged();
		}
		ReleaseMouse();
		return true;
	}

	void ListBox::SelectionChanged()
	{
		UI_MsgArgs a;
		a.Sender = this;
		a.Type = MSG_UI_CHANGED;
		BroadcastMessage(&a);
	}

	int ListBox::AddCheckBoxItem(String Text)
	{
		CheckBox *chkBox = new CheckBox(this);
		chkBox->SetFont(font);
		chkBox->SetText(Text);
		chkBox->SetHeight(chkBox->TextHeight);
		chkBox->BackColor = Color(255,255,255,0);
		return AddControlItem(chkBox);
	}

	int ListBox::AddControlItem(Control *Item)
	{
		Items.Add(Item);
		Item->BackColor = Color(255,255,255,0);
		if (Item->GetHeight()>ItemHeight)
			ItemHeight = Item->GetHeight();
		Item->AcceptsFocus = false;
		Item->TabStop = false;
		ListChanged();
		return Items.Count()-1;
	}

	int ListBox::AddTextItem(String Text)
	{
		Label *lbl = new Label(this);
		lbl->SetFont(font);
		lbl->SetText(Text);
		lbl->SetHeight(lbl->TextHeight);
		lbl->BackColor = Color(255,255,255,0);
		return AddControlItem(lbl);
	}
	
	void ListBox::Delete(Control *Item)
	{
		for (int i=0; i<Items.Count(); i++)
		{
			if (Items[i] == Item)
			{
				Items[i] = 0;
				Items.RemoveAt(i);
				ListChanged();
				break;
			}
		}
		this->RemoveChild(Item);
	}

	void ListBox::Delete(int Index)
	{
		this->RemoveChild(Items[Index]);
		Items[Index] = 0;
		Items.RemoveAt(Index);
		ListChanged();
	}

	void ListBox::Clear()
	{
		for (auto item : Items)
			RemoveChild(item);
		Items.Clear();
	}

	void ListBox::ListChanged()
	{
		int PageSize; //当前ListBox的大小能同时显示的项目个数
		PageSize = Height / ItemHeight;
		if (PageSize<1) PageSize=1;
		if (PageSize>=Items.Count())
		{
			//无需显示滚动条
			ScrollBar->Visible = false;
			ScrollBar->SetValue(0, 1, 0, 1);
		}
		else
		{
			//需要显示滚动条
			ScrollBar->Visible = true;
			ScrollBar->SetValue(0,Items.Count()-PageSize,(SelectedIndex==-1)?0:ClampInt(SelectedIndex,0,Items.Count()-PageSize), PageSize);
		}
	}

	CheckBox* ListBox::GetCheckBoxItem(int Index)
	{
		return (CheckBox *)(Items[Index]);
	}

	Label* ListBox::GetTextItem(int Index)
	{
		return (Label *)(Items[Index]);
	}

	Control* ListBox::GetItem(int Index)
	{
		return (Control *)(Items[Index]);
	}

	int ListBox::GetItemHeight()
	{
		return ItemHeight;
	}

	int ListBox::HitTest(int , int Y)
	{
		int rs = Y/ItemHeight + ScrollBar->GetPosition();
		if (rs>=Items.Count())
			rs = -1;
		return rs;
	}

	Control* ListBox::GetSelectedItem()
	{
		if (SelectedIndex!=-1)
			return (Control *)(Items[SelectedIndex]);
		else
			return NULL;
	}

	ComboBox::ComboBox(Container * parent)
		: ListBox(parent)
	{
		btnDrop = new Button(this);
		btnDrop->AcceptsFocus = false;
		btnDrop->TabStop = false;
		btnDrop->SetFont(GetEntry()->System->LoadDefaultFont(DefaultFontType::Symbol));
		btnDrop->SetText(L"6");
		btnDrop->BorderColor.A = 0;
		TextBox = new GraphicsUI::TextBox(this);
		BorderStyle = BS_FLAT_;
		TextBox->BorderStyle = BS_NONE;
		TextBox->BackColor.A = 0;
		TextBox->AcceptsFocus = false;
		TextBox->TabStop = false;
		ShowList = false;
		HotTrack = true;
		HighLightColor = SelectionColor;
		HighLightForeColor = SelectionForeColor;
		SelectionColor = BackColor;
		SelectionForeColor = FontColor;
		UnfocusedSelectionColor = BackColor;
		BorderWidth = 1;
		DoDpiChanged();
	}

	bool ComboBox::DoClosePopup()
	{
		ToggleList(false);
		return false;
	}


	void ComboBox::SetSelectedIndex(int id)
	{
		ChangeSelectedItem(id);
	}

	void ComboBox::DoDpiChanged()
	{
		ListBox::DoDpiChanged();
		Posit(Left, Top, Width, Global::SCROLLBAR_BUTTON_SIZE + BorderWidth * 4);
	}

	void ComboBox::Posit(int x, int y, int w, int)
	{
		ListBox::Posit(x, y, w, Global::SCROLLBAR_BUTTON_SIZE + BorderWidth * 4);
	}

	void ComboBox::SizeChanged()
	{
		TextBox->Posit(BorderWidth, 0, Width - Global::SCROLLBAR_BUTTON_SIZE - BorderWidth * 2, Height);
		btnDrop->Posit(Width - Global::SCROLLBAR_BUTTON_SIZE - BorderWidth, BorderWidth, Global::SCROLLBAR_BUTTON_SIZE, Height - BorderWidth * 2);
	}
	void ComboBox::Draw(int absX, int absY)
	{
		Control::Draw(absX, absY);
		absX += Left; absY += Top;
		if (!Visible)
			return;
		TextBox->Draw(absX, absY);
		btnDrop->Checked = ShowList;
		btnDrop->Draw(absX, absY);
		if (IsFocused())
		{
			auto & graphics = GetEntry()->DrawCommands;
			graphics.PenColor = Global::Colors.FocusRectColor;
			graphics.DrawRectangle(absX + 3, absY + 3, absX + btnDrop->Left - 2, absY + Height - 3);
		}
	}

	void ComboBox::ToggleList(bool sl)
	{
		auto entry = GetEntry();
		ShowList = sl;
		ListLeft = 0;
		ListTop = Height+1;
		ListHeight = ItemHeight * ClampInt(Items.Count(),1,COMBOBOX_LIST_SIZE);
		ListWidth = Width;
		if (AbsolutePosY + ListTop + ListHeight > entry->GetHeight())
		{
			ListTop -= Height + ListHeight;
		}
		int vlH,vlW,vlL,vlT;
		vlH = Height; vlW = Width;
		vlL = Left; vlT = Top;
		Left = 0; Top = 0;
		Height = ListHeight; Width = ListWidth; Left = ListLeft; Top = ListTop;
		ListBox::SizeChanged();
		Height = vlH; Width = vlW; Left = vlL; Top = vlT;
		if (ShowList)
		{
			Global::MouseCaptureControl = this;
		}
	}

	bool ComboBox::PosInList(int X, int Y)
	{
		if (ShowList)
		{
			return (X >=ListLeft && X <ListLeft+ListWidth && Y>=ListTop && Y<=ListTop+ListHeight);
		}
		else
			return false;
	}

	void ComboBox::ChangeSelectedItem(int id)
	{
		if (id != -1)
		{
			if (Items[id]->Type != CT_CHECKBOX)
				TextBox->SetText(((Label *)Items[id])->GetText());
		}
		else
			TextBox->SetText(L"");
		SelectedIndex = id;
	}

	void ComboBox::SetFocus()
	{
		Control::SetFocus();
	}

	void ComboBox::BeginListBoxFunctions()
	{
		lH = Height; lW = Width;
		lL = Left; lT = Top;
		Left = 0; Top = 0;
		ListBox::Posit(ListLeft,ListTop,ListWidth,ListHeight);
		Rect R;
		R.x = ListLeft+AbsolutePosX;
		R.y = ListTop + AbsolutePosY;
		R.w = ListWidth+1;
		R.h = ListHeight+2;
		GetEntry()->ClipRects->AddRect(R);
		btnDrop->Visible = false;
	}

	void ComboBox::EndListBoxFunctions()
	{
		GetEntry()->ClipRects->PopRect();
		ComboBox::Posit(lL,lT,lW,lH);
		btnDrop->Visible = true;
	}

	bool ComboBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X,Y,Shift);
		if (!Visible || !Enabled)
			return false;
		lastSelIdx = SelectedIndex;
		if (IsPointInClient(X, Y))
		{
			ToggleList(!ShowList);
			Global::MouseCaptureControl = this;
		}
		else
		{
			if (PosInList(X,Y))
			{
				BeginListBoxFunctions();
				ListBox::DoMouseDown(X - ListLeft, Y - ListTop, Shift);
				EndListBoxFunctions();
				Global::MouseCaptureControl = this;
			}
			else
			{
				ToggleList(false);
				ReleaseMouse();
			}
		}
		return true;
	}

	bool ComboBox::DoMouseMove(int X, int Y)
	{
		Control::DoMouseMove(X,Y);
		if (!Visible || !Enabled)
			return false;
		if (ShowList)
		{
			BeginListBoxFunctions();
			ListBox::DoMouseMove(X - ListLeft, Y - ListTop);
			EndListBoxFunctions();
		}
		return true;
	}

	bool ComboBox::DoMouseWheel(int delta)
	{
		if (Visible && Enabled)
		{
			if (ShowList)
				return ListBox::DoMouseWheel(delta);
			else
			{
				int nselId = SelectedIndex;
				if (delta > 0)
					nselId--;
				else
					nselId++;
				nselId = Math::Clamp(nselId, 0, Items.Count() - 1);
				if (nselId != SelectedIndex)
				{
					ChangeSelectedItem(nselId);
					OnChanged.Invoke(this);
				}
				return true;
			}
		}
		return false;
	}

	bool ComboBox::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X,Y,Shift);
		if (!Visible || !Enabled)
			return false;
		if (ShowList)
		{
			BeginListBoxFunctions();
			bool PosInItem;
			int bdr = ScrollBar->Visible ? ScrollBar->GetWidth() : 0;
			PosInItem = X<ListLeft + Width - bdr && X>ListLeft && Y > ListTop && Y < ListTop + ListHeight;
			if (PosInItem)
			{
				ToggleList(false);
				ChangeSelectedItem(SelectedIndex);
				ListBox::DoMouseUp(X - ListLeft, Y - ListTop, Shift);
				ReleaseMouse();
			}
			else
			{
				ListBox::DoMouseUp(X - ListLeft, Y - ListTop, Shift);
				Global::MouseCaptureControl = this;
			}
			EndListBoxFunctions();
		}
		else
			ReleaseMouse();
		return true;
	}

	bool ComboBox::DoKeyDown(unsigned short Key, SHIFTSTATE shift)
	{
		if (!Visible || !Enabled)
			return false;
		bool AltDown = (shift != 0);
		if (!AltDown && (Key == 0x26 || Key == 0x28))
		{
			if (Key == 0x26) // VK_UP
			{
				HighLightID = ClampInt(HighLightID - 1, 0, Items.Count() - 1);
			}
			else if (Key == 0x28) // VK_DOWN
			{
				HighLightID = ClampInt(HighLightID + 1, 0, Items.Count() - 1);
			}
			if (!ShowList)
			{
				if (HighLightID != SelectedIndex)
				{
					ChangeSelectedItem(HighLightID);
					SelectionChanged();
				}
			}
			else
			{
				int sy = (HighLightID - ScrollBar->GetPosition())*ItemHeight + BorderWidth - 1;
				if (sy < 0)
				{
					ScrollBar->SetPosition(ClampInt(HighLightID, 0, ScrollBar->GetMax()));
				}
				else if (sy > ListHeight - ItemHeight - 1)
				{
					ScrollBar->SetPosition(ClampInt(HighLightID - ListHeight / ItemHeight + 1, 0, ScrollBar->GetMax()));
				}
			}
		}
		if ((Key == 0x20 || Key == 0x0D)) // VK_SPACE VK_RETURN
		{
			if (ShowList)
			{
				if (HighLightID != SelectedIndex)
				{
					ChangeSelectedItem(HighLightID);
					SelectionChanged();
				}
			}
			ToggleList(!ShowList);
			return true;
		}
		else if (Key == 0x1B) // VK_ESCAPE
		{
			ToggleList(false);
			return true;
		}
		if (Key == 0x26 || Key == 0x28)
			return true;
		return false;
	}

	void ComboBox::HandleMessage(const UI_MsgArgs *Args)
	{
		if (ShowList)
			ListBox::HandleMessage(Args);
		if (Args->Type == MSG_UI_TOPLAYER_DRAW)
		{
			if (Visible && ShowList)
			{
				BeginListBoxFunctions();
				int lstB = BorderStyle;
				Color lstBC= BorderColor;
				BorderColor = Global::Colors.ControlFontColor;
				BorderStyle = BS_FLAT_;
				auto oldShadow = BackgroundShadow;
				BackgroundShadow = true;
				ListBox::Draw(AbsolutePosX,AbsolutePosY);
				BackgroundShadow = oldShadow;
				BorderStyle = lstB;
				BorderColor = lstBC;
				EndListBoxFunctions();
			}
		}
		if (Args->Type == MSG_UI_MOUSEWHEEL)
		{
			if (!ShowList && IsFocused())
			{
				if ((*(UIMouseEventArgs *)Args->Data).Delta<0)
				{
					SelectedIndex=ClampInt(SelectedIndex+1,0,Items.Count()-1);
					ChangeSelectedItem(SelectedIndex);
				}
				else
				{
					SelectedIndex=ClampInt(SelectedIndex-1,0,Items.Count()-1);
					ChangeSelectedItem(SelectedIndex);
				}
			}
		}
	}

	void ComboBox::LostFocus(Control * newFocus)
	{
		Control::LostFocus(newFocus);
		while (newFocus && newFocus != this)
			newFocus = newFocus->Parent;
		if (!newFocus)
			ToggleList(false);
	}

	ProgressBar::ProgressBar(Container * parent)
		: Control(parent)
	{
		BorderStyle = BS_LOWERED;
		Type = CT_PROGRESSBAR;
		Style = PROGRESSBAR_STYLE_NORMAL;
		Max = 100;
		Position = 0;
	}

	ProgressBar::~ProgressBar()
	{
			
	}

	void ProgressBar::SetMax(int AMax)
	{
		Max = AMax;
		if (Position>Max)
			Position = Max;
	}

	void ProgressBar::SetPosition(int APos)
	{
		Position = ClampInt(APos,0,Max);
	}

	int ProgressBar::GetMax()
	{
		return Max;
	}

	int ProgressBar::GetPosition()
	{
		return Position;
	}

	void ProgressBar::Draw(int absX, int absY)
	{
		Control::Draw(absX,absY);
		absX+=Left; absY+=Top;
		int PH,PW;
		PH = Height - 4;
		auto entry = GetEntry();
		auto & graphics = entry->DrawCommands;
		if (Style == 2) //Block Style
		{
			entry->ClipRects->AddRect(Rect(absX+2,absY+2,Width-6,Height-4));
			PW = int(PH *0.65);
			int bc = (int)(Position/(float)Max *ceil((Width - 2)/(float)PW));
			for (int i=0;i<bc;i++)
			{
				int cx = i*PW+3+absX;
				int cy = 2+absY;
				graphics.SolidBrushColor = Global::Colors.SelectionColor;
				graphics.FillRectangle(cx, cy, cx + PW - 2, cy + PH);
			}
			entry->ClipRects->PopRect();
		}
		else
		{
			int cx = absX+3, cy= absY+2;
			PW = (Width -4)*Position/Max;
			graphics.SolidBrushColor = Global::Colors.SelectionColor;
			graphics.FillRectangle(cx, cy, cx + PW, cy + PH);
		}
	}
		
	Menu::Menu(Container * parent, MenuStyle s)
		: Container(parent), style(s)
	{
		Type = CT_MENU;
		TabStop = s == msMainMenu;
		TopMost = true;
		Padding = 0;
		Height = Padding.Vertical();
		Width = Padding.Horizontal();
		BorderStyle = BS_NONE;
		BorderColor = Global::Colors.MenuBorderColor;
		BackColor = Global::Colors.MenuBackColor;
		curSubMenu = 0;
		parentItem = 0;
		if (style == msPopup)
			Visible = false;
		if (style == msPopup)
			BackgroundShadow = true;
		else
		{
			DockStyle = dsTop;
			BackColor = Global::Colors.ToolButtonBackColor1;
		}
		if (s == msMainMenu)
		{
			if (parent->Type == CT_ENTRY)
				((UIEntry*)parent)->MainMenu = this;
			else if (parent->Type == CT_FORM)
				((Form*)parent)->MainMenu = this;
		}
		else
			Padding = 2;
	}

	void Menu::SetFocus()
	{
		if (style == msMainMenu)
			lastFocusedCtrl = GetEntry()->FocusedControl;
		Container::SetFocus();	
	}

	void Menu::DoDpiChanged()
	{
		Container::DoDpiChanged();
		PositMenuItems();
	}

	void Menu::PopupSubMenu(Menu * subMenu, int x, int y)
	{
		if (!subMenu->Visible || subMenu != curSubMenu)
		{
			if (curSubMenu)
				CloseSubMenu();
			subMenu->Popup(x, y);
			curSubMenu = subMenu;
		}
	}

	void Menu::CloseSubMenu()
	{
		if (curSubMenu)
		{
			curSubMenu->CloseSubMenu();
			curSubMenu->CloseMenu();
			curSubMenu = 0;
			ReleaseMouse();
			if (this->style != msMainMenu)
				Global::MouseCaptureControl = this;
		}
	}

	bool Menu::DoClosePopup()
	{
		CloseSubMenu();
		if (style == msPopup)
			CloseMenu();
		return false;
	}

	void Menu::AddItem(MenuItem * item)
	{
		Items.Add(item);
		item->Parent = this;
		if (controls.IndexOf(item) == -1)
			controls.Add(item);
		PositMenuItems();
	}

	void Menu::RemoveItem(MenuItem * item)
	{
		int fid = -1;
		fid = Items.IndexOf(item);
		if (fid != -1)
		{
			Items[fid] = 0;
			Items.RemoveAt(fid);
		}
		fid = controls.IndexOf(item);
		if (fid != -1)
		{
			controls.RemoveAt(fid);
		}
		PositMenuItems();
	}

	void Menu::PositMenuItems()
	{
		if (style == msPopup)
		{
			int cHeight = Padding.Top;
			Width = 0;
			ItemHeight = (int)(GetEntry()->GetLineHeight() * 1.5f);
			for (int i=0; i<Items.Count(); i++)
			{
				if (!Items[i]->Visible)
					continue;
				int nWidth = Items[i]->MeasureWidth() + ItemHeight;
				if (nWidth + Padding.Horizontal() > Width)
					Width = nWidth + Padding.Horizontal();
				if (Items[i]->IsSeperator())
					Items[i]->SetHeight(ItemHeight>>2);
				else
					Items[i]->SetHeight(ItemHeight);
					
				Items[i]->Left = Padding.Left;
				Items[i]->Top = cHeight;

				cHeight += Items[i]->GetHeight();
			}
			Height = cHeight + Padding.Bottom;
			for (int i=0; i<Items.Count(); i++)
			{
				Items[i]->SetWidth(Width - Padding.Horizontal());
			}
		}
		else
		{
			Height = (int)(GetEntry()->GetLineHeight() * 1.25f);
			Width = 0;
			for (int i=0; i<Items.Count(); i++)
			{
				Items[i]->isButton = true;
				if (Items[i]->Visible && !Items[i]->IsSeperator())
				{
					Items[i]->Top = 0;
					Items[i]->SetWidth(Items[i]->MeasureWidth(true));
					Items[i]->SetHeight(Height);
					Items[i]->Left = Width;
					Width += Items[i]->GetWidth();
				}
				else
					Items[i]->Visible = false;
			}
		}
	}

	int Menu::Count()
	{
		return Items.Count();
	}

	MenuItem * Menu::GetItem(int id)
	{
		if (id < Items.Count())
		{
			return Items[id];
		}
		else
		{
			throw CoreLib::IndexOutofRangeException();
		}
	}

	void Menu::ItemSelected(MenuItem * item)
	{
		if (parentItem)
			parentItem->ItemSelected(item);
		if (style == msPopup)
			CloseMenu();
		else
		{
			for (int i=0; i<Items.Count(); i++)
				Items[i]->Selected = false;
		}
		if (style == msMainMenu)
		{
			if (lastFocusedCtrl)
				lastFocusedCtrl->SetFocus();
		}
	}

	void Menu::DrawPopup()
	{
		auto entry = GetEntry();
		int absX, absY;
		LocalPosToAbsolutePos(0, 0, absX, absY);
		Control::Draw(absX - Left, absY - Top);
		auto & graphics = entry->DrawCommands;
		graphics.SetRenderTransform(absX, absY);
		for (auto & item : Items)
			ItemHeight = Math::Max(ItemHeight, item->GetHeight());
		graphics.SolidBrushColor = Global::Colors.MemuIconBackColor;
		graphics.FillRectangle(Padding.Left, Padding.Top, ItemHeight + Padding.Left, Height - Padding.Bottom);
		graphics.PenColor = Global::Colors.MenuBorderColor;
		graphics.DrawRectangle(0, 0, Width - 1, Height - 1);
		graphics.DrawLine(ItemHeight+Padding.Left, Padding.Top, ItemHeight + Padding.Left, Height - Padding.Bottom - 1);
		int cposY = 0;
		for (int i =0; i<Items.Count(); i++)
		{
			int itemHeight = Items[i]->GetHeight();
			graphics.SetRenderTransform(absX + Padding.Left, absY + Padding.Top + cposY);
			Items[i]->DrawMenuItem(Width - Padding.Horizontal(), ItemHeight);
			cposY += itemHeight;
		}
		graphics.SetRenderTransform(0, 0);
	}


	void Menu::Popup(int x, int y)
	{
		if (!Visible)
		{
			auto entry = GetEntry();
			if (!parentItem)
				lastFocusedCtrl = entry->FocusedControl;
			OnPopup.Invoke(this);
			PositMenuItems();
			for (int i=0; i<Items.Count(); i++)
				Items[i]->Selected = false;
			Left = x;
			Top = y;
			int ax, ay;
			LocalPosToAbsolutePos(0, 0, ax, ay);
			if (ax + Width > entry->GetWidth())
				Left -= Width;
			if (ay + Height > entry->GetHeight())
				Top -= Height;
			Visible = true;
			SetFocus();
			Global::MouseCaptureControl = this;
		}
	}

	void Menu::CloseMenu()
	{
		if (Visible)
		{
			Visible = false;
			OnMenuClosed.Invoke(this);
			if ((!parentItem || parentItem->isButton) && lastFocusedCtrl)
				lastFocusedCtrl->SetFocus();
			if (parentItem && parentItem->isButton)
				parentItem->Selected = false;
			if (parentItem)
			{
				Control * parent = ((MenuItem*)parentItem)->Parent;
				if (parent)
					((Menu*)parent)->curSubMenu = 0;
			}
			enableMouseHover = false;
			curSubMenu = nullptr;
			ReleaseMouse();
			if (Global::MouseCaptureControl && Global::MouseCaptureControl->IsChildOf(this))
				Global::MouseCaptureControl = nullptr;
		}
	}

	void Menu::DrawMenuBar(int absX, int absY)
	{
		Control::Draw(absX, absY);
		auto entry = GetEntry();
		int ox = absX + Left + Padding.Left;
		int oy = absY + Top + Padding.Top;
		int cposY = 0;
		auto & graphics = entry->DrawCommands;
		for (int i = 0; i < Items.Count(); i++)
		{
			graphics.SetRenderTransform(ox + Items[i]->Left, oy + Items[i]->Top);
			int itemHeight = Items[i]->IsSeperator()?3:ItemHeight;
			Items[i]->DrawMenuButton(Items[i]->GetWidth(), Items[i]->GetHeight());
			cposY += itemHeight;
		}
		graphics.SetRenderTransform(0, 0);
	}

	void Menu::Draw(int absX, int absY)
	{
		if (style == msMainMenu)
			DrawMenuBar(absX, absY);
	}

	bool Menu::DoMouseHover()
	{
		if (!enableMouseHover)
			return false;
		enableMouseHover = false;
		for (auto & item : Items)
		{
			if (item->Selected)
			{
				item->DoMouseHover();
			}
		}
		return false;
	}


	bool Menu::DoMouseMove(int X, int Y)
	{
		Container::DoMouseMove(X,Y);
		if (!Visible || ! Enabled)
			return false;
		for (auto & item : Items)
		{
			if (X >= item->Left && X < item->Left + item->Width &&
				Y >= item->Top && Y < item->Top + item->Height)
			{
				item->Selected = true;
			}
			else
				item->Selected = false;
		}
		if (IsPointInClient(X, Y))
		{
			enableMouseHover = true;
			if (parentItem)
				parentItem->Selected = true;
		}
		else
		{
			enableMouseHover = false;
			if (!curSubMenu)
			{
				for (int i = 0; i < Items.Count(); i++)
					Items[i]->Selected = false;
			}
		}
		if (curSubMenu)
		{
			for (auto & item : Items)
			{
				if (item->Selected)
				{
					if (item->SubMenu && item->SubMenu && item->SubMenu->Count())
					{
						if (style == Menu::msMainMenu)
						{
							CloseSubMenu();
							PopupSubMenu(item->SubMenu, -item->Padding.Left, Height - item->Padding.Vertical());
						}
					}
				}
			}
		}
		if (!Parent || (Parent->Type != CT_MENU && Parent->Type != CT_MENU_ITEM))
			return true;
		return false;
	}
	bool Menu::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Container::DoMouseDown(X, Y, Shift);
		if (!IsPointInClient(X, Y))
		{
			if (style == msPopup)
				CloseMenu();
			else
			{
				for (int i=0; i<Items.Count(); i++)
					Items[i]->Selected = false;
			}
		}
		else
		{
			for (auto & item : Items)
				if (X >= item->Left && X < item->Left + item->Width &&
					Y >= item->Top && Y <= item->Top + item->Height)
					item->DoMouseDown(X - item->Left, Y - item->Top, Shift);
			return true;
		}
		
		return false;
	}
	bool Menu::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Container::DoMouseUp(X, Y, Shift);
		return true;
	}

	int Menu::GetSelectedItemID()
	{
		for (int i=0; i<Items.Count(); i++)
			if (Items[i]->Selected && Items[i]->Enabled && Items[i]->Visible && !Items[i]->IsSeperator())
				return i;
		return -1;
	}

	bool Menu::DoKeyDown(unsigned short Key, SHIFTSTATE /*Shift*/)
	{
		if (!Enabled || !Visible)
			return false;
		if ((Key >= L'A' && Key <= L'Z') || (Key >= L'0' && Key <= L'9'))
		{
			for (int i=0; i<Items.Count(); i++)
				Items[i]->Selected = false;
			for (int i=0; i<Items.Count(); i++)
				if (Items[i]->GetAccessKey() == Key)
				{
					Items[i]->Hit();
					Items[i]->Selected = true;
					return true;
				}
			return false;
		}
		int id = GetSelectedItemID();

		if (Key == 0x20 || Key == 0x0D) // VK_SPACE || VK_RETURN
		{
			if (id >= 0 && Items[id]->Selected && Items[id]->Enabled && !Items[id]->IsSeperator())
			{
				Items[id]->Hit();
			}
			return true;
		}
		if (style == msPopup)
		{
			Menu * parentMainMenu = nullptr;
			if (parentItem && parentItem->Parent && parentItem->Parent->Type == CT_MENU &&
				((Menu*)parentItem->Parent)->style == msMainMenu)
				parentMainMenu = ((Menu*)parentItem->Parent);
			if (Key == 0x26 || Key == 0x28) // VK_UP VK_DOWN
			{
				for (int i=0; i<Items.Count(); i++)
					Items[i]->Selected = false;
					
				if (Key == 0x28)
				{
					int nxt = id + 1;
					int tc = Items.Count();
					nxt %= Items.Count();
					while (nxt != id && tc)
					{
						if (!Items[nxt]->IsSeperator() && Items[nxt]->Visible && Items[nxt]->Enabled)
						{
							Items[nxt]->Selected = true;
							break;
						}
						nxt ++;
						nxt %= Items.Count();
						tc--;
					}
					if (nxt == id)
						Items[id]->Selected = true;
				}
				else if (Key == 0x26)
				{
					int nxt = id - 1;
					int tc = Items.Count();
					if (nxt < 0)
						nxt += Items.Count();
					nxt %= Items.Count();
					while (nxt != id && tc)
					{
						if (!Items[nxt]->IsSeperator() && Items[nxt]->Visible && Items[nxt]->Enabled)
						{
							Items[nxt]->Selected = true;
							break;
						}
						nxt --;
						if (nxt < 0)
							nxt += Items.Count();
						nxt %= Items.Count();
						tc--;
					}
					if (nxt == id && id != -1)
						Items[id]->Selected = true;
				}
				return true;
			}
			if (Key == 0x27)  // VK_RIGHT
			{
				if (id != -1 && Items[id]->SubMenu && Items[id]->SubMenu->Count())
				{
					PopupSubMenu(Items[id]->SubMenu, Items[id]->Width - 2, 0);
					for (int i=0; i<Items[id]->SubMenu->Count(); i++)
					{
						MenuItem * item = Items[id]->SubMenu->GetItem(i);
						if (!item->IsSeperator() && item->Enabled && item->Visible)
						{
							item->Selected = true;
							break;
						}
					}
				}
				else if (parentMainMenu && parentMainMenu->Items.Count())
				{
					int pid = -1;
					for (int i = 0; i < parentMainMenu->Items.Count(); i++)
						if (parentMainMenu->Items[i]->Selected)
						{
							pid = i;
							break;
						}
					int npId = (pid + 1) % parentMainMenu->Items.Count();
					int trials = 0;
					while (trials < parentMainMenu->Items.Count() && (parentMainMenu->Items[npId]->IsSeperator() || !parentMainMenu->Items[npId]->Enabled
						|| !parentMainMenu->Items[npId]->Visible))
						npId = (npId + 1) % parentMainMenu->Items.Count();
					for (auto item : parentMainMenu->Items)
						item->Selected = false;
					parentMainMenu->Items[npId]->Selected = true;

					parentMainMenu->Items[npId]->Hit();
				}
				return true;
			}
			else if (Key == 0x25)
			{
				if (parentMainMenu && parentMainMenu->Items.Count())
				{
					int pid = -1;
					for (int i = 0; i < parentMainMenu->Items.Count(); i++)
						if (parentMainMenu->Items[i]->Selected)
						{
							pid = i;
							break;
						}
					int npId = (pid - 1 + parentMainMenu->Items.Count()) % parentMainMenu->Items.Count();
					int trials = 0;
					while (trials < parentMainMenu->Items.Count() && (parentMainMenu->Items[npId]->IsSeperator() || !parentMainMenu->Items[npId]->Enabled
						|| !parentMainMenu->Items[npId]->Visible))
						npId = (npId - 1 + parentMainMenu->Items.Count()) % parentMainMenu->Items.Count();
					for (auto item : parentMainMenu->Items)
						item->Selected = false;
					parentMainMenu->Items[npId]->Selected = true;
					parentMainMenu->Items[npId]->Hit();
				}
				else if (parentItem && parentItem->Parent)
				{
					((Menu *)parentItem->Parent)->CloseSubMenu();
				}
				return true;
			}
			else if (Key == 0x1B) // VK_LEFT || VK_ESCAPE
			{
				if (parentItem && parentItem->Parent)
				{
					((Menu *)parentItem->Parent)->CloseSubMenu();
				}
				CloseMenu();
				if (parentMainMenu)
				{
					parentMainMenu->SetFocus();
					int pid = parentMainMenu->Items.IndexOf(parentItem);
					if (pid != -1)
						parentMainMenu->Items[pid]->Selected = true;
				}
				return true;
			}
		}
		else
		{
			if (Key == 0x25 || Key == 0x27) // VK_LEFT VK_RIGHT
			{
				for (int i=0; i<Items.Count(); i++)
					Items[i]->Selected = false;
					
				if (Key == 0x27)
				{
					int nxt = id + 1;
					int tc = Items.Count();
					nxt %= Items.Count();
					while (nxt != id && tc)
					{
						if (!Items[nxt]->IsSeperator() && Items[nxt]->Visible && Items[nxt]->Enabled)
						{
							Items[nxt]->Selected = true;
							break;
						}
						nxt ++;
						nxt %= Items.Count();
						tc--;
					}
					if (nxt == id)
						Items[id]->Selected = true;
				}
				else if (Key == 0x25)
				{
					int nxt = id - 1;
					int tc = Items.Count();
					if (nxt < 0)
						nxt += Items.Count();
					nxt %= Items.Count();
					while (nxt != id && tc)
					{
						if (!Items[nxt]->IsSeperator() && Items[nxt]->Visible && Items[nxt]->Enabled)
						{
							Items[nxt]->Selected = true;
							break;
						}
						nxt --;
						if (nxt < 0)
							nxt += Items.Count();
						nxt %= Items.Count();
						tc--;
					}
					if (nxt == id && id != -1)
						Items[id]->Selected = true;
				}
				return true;
			}
			else if (Key == 0x28) // VK_DOWN
 			{
				if (id != -1)
					Items[id]->Hit();
				if (curSubMenu)
				{
					for (int i=0; i<curSubMenu->Count(); i++)
					{
						MenuItem * item = curSubMenu->GetItem(i);
						if (!item->IsSeperator() && item->Enabled && item->Visible)
						{
							item->Selected = true;
							break;
						}
					}
				}
				return true;
			}
			else if (Key == 0x1B) // VK_ESCAPE
			{
				CloseSubMenu();
				for (auto item : Items)
					item->Selected = false;
				if (lastFocusedCtrl)
					lastFocusedCtrl->SetFocus();
				return true;
			}
		}

		return false;
	}

	void Menu::HandleMessage(const UI_MsgArgs * Args)
	{
		if (Args->Type == MSG_UI_TOPLAYER_DRAW)
		{
			if (Visible)
				if (style == msPopup)
					DrawPopup();
		}
	}

	MenuItem::MenuItem(Menu * parent)
		: Container(parent), accKey(0)
	{
		TabStop = false;
		isSeperator = true;
		parent->AddItem(this);
	}

	MenuItem::MenuItem(MenuItem * parent)
		: Container(parent->GetSubMenu()), accKey(0)
	{
		TabStop = false;
		isSeperator = true;
		parent->AddItem(this);
	}

	MenuItem::MenuItem(Menu * parent, const String & text, const String & shortcutText)
		: Container(parent)
	{
		TabStop = false;
		isSeperator = false;
		Init();
		SetText(text);
		lblShortcut->SetText(shortcutText);
		parent->AddItem(this);

	}

	MenuItem::MenuItem(MenuItem * parent, const String & text, const String & shortcutText)
		: Container(parent->GetSubMenu())
	{
		TabStop = false;
		isSeperator = false;
		Init();
		SetText(text);
		parent->AddItem(this);
		lblShortcut->SetText(shortcutText);
	}

	MenuItem::MenuItem(Menu * parent, const String & text)
		: MenuItem(parent, text, L"")
	{
	}

	MenuItem::MenuItem(MenuItem * parent, const String & text)
		: MenuItem(parent, text, L"")
	{
	}

	void MenuItem::SetText(const String & text)
	{
		StringBuilder unescape;
		accKey = 0;
		accKeyId = -1;
		for (int i = 0; i < text.Length(); i++)
		{
			if (text[i] != L'&')
				unescape << text[i];
			else if (i < text.Length() - 1)
			{
				if (text[i + 1] != L'&')
				{
					accKey = text[i + 1];
					accKeyId = unescape.Length();
				}
			}
		}
		if (accKey >= 97 && accKey <= 122)
			accKey = accKey + (-97 + 65);
		lblText->SetText(unescape.ProduceString());
	}

	String MenuItem::GetText()
	{
		return lblText->GetText();
	}

	void MenuItem::SetShortcutText(const String & text)
	{
		lblShortcut->SetText(text);
	}

	String MenuItem::GetShortcutText()
	{
		return lblShortcut->GetText();
	}

	void MenuItem::Init()
	{
		Type = CT_MENU_ITEM; 
		Selected = false;
		Checked = false;
		isButton = false;
		accKey = 0;
		DoDpiChanged();
		if (!isSeperator)
		{
			lblText = new Label(this);
			lblShortcut = new Label(this);
			lblText->AutoSize = true;
			lblShortcut->AutoSize = true;
		}
	}

	bool MenuItem::IsSeperator()
	{
		return isSeperator;
	}

	wchar_t MenuItem::GetAccessKey()
	{
		return accKey;
	}

	int MenuItem::MeasureWidth(bool pIsButton)
	{
		if (!pIsButton)
		{
			if (isSeperator)
			{
				return 20;
			}
			else
			{
				lblText->SetHeight(lblText->TextHeight);
				lblShortcut->SetHeight(lblText->TextHeight);
				int rm = 0;
				if (SubMenu && SubMenu->Count())
					rm = 8;
				return lblText->TextWidth + 16 +
					lblShortcut->TextWidth + separatorHeading + Padding.Horizontal() + rm;
			}
		}
		else
		{
			return lblText->TextWidth + separatorHeading + Padding.Horizontal();
		}
	}

	bool MenuItem::DoMouseEnter()
	{
		cursorInClient = true;
		Control::DoMouseEnter();
		Menu * mn = (Menu*)Parent;
		if (mn)
		{
			for (int i=0; i<mn->Count(); i++)
				mn->GetItem(i)->Selected = false;
		}
		if (Enabled && Visible && !isSeperator)
			Selected = true;
		return false;
	}

	bool MenuItem::DoMouseLeave()
	{
		cursorInClient = false;
		Control::DoMouseLeave();
		Menu * mn = (Menu*)Parent;
		if (SubMenu && mn && mn->curSubMenu == SubMenu)
			Selected = true;
		else
			Selected = false;
		return false;
	}

	bool MenuItem::DoMouseHover()
	{
		if (isButton)
			return false;
		if (Enabled && SubMenu && SubMenu->Count() != 0)
		{
			if (Parent)
			{
				((Menu *)Parent)->PopupSubMenu(SubMenu, Width - Padding.Left, -Padding.Top);
			}
		}
		else
		{
			if (Parent)
			{
				((Menu *)Parent)->CloseSubMenu();
			}
		}
		return false;
	}

	bool MenuItem::DoClick()
	{	
		if (!isSeperator && Enabled && Visible && Parent)
		{
			if (!SubMenu || SubMenu->Count() == 0)
				((Menu*)Parent)->ItemSelected(this);
		}
		Control::DoClick();
		return false;
	}

	void MenuItem::AddItem(MenuItem * item)
	{
		GetSubMenu();
		SubMenu->AddItem(item);
	}

	void MenuItem::RemoveItem(MenuItem * item)
	{
		if (SubMenu)
			SubMenu->RemoveItem(item);
	}

	Menu * MenuItem::GetSubMenu()
	{
		if (!SubMenu)
		{
			SubMenu = new Menu(this);
			SubMenu->parentItem = this;
		}
		return SubMenu;
	}

	int MenuItem::Count()
	{
		if (SubMenu)
			return SubMenu->Count();
		else
			return 0;
	}
	
	void MenuItem::ItemSelected(MenuItem * item)
	{
		if (Parent)
		{
			((Menu *)Parent)->ItemSelected(item);
		}
	}


	void MenuItem::DrawMenuButton(int width, int height)
	{
		if (!isSeperator && Visible)
		{
			auto entry = GetEntry();
			if (Selected || (SubMenu && SubMenu->Visible))
			{
				auto & graphics = entry->DrawCommands;
				if (SubMenu && SubMenu->Visible)
				{
					graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorPressed1;
					graphics.FillRectangle(0,0,width, height);
				}
				else
				{
					graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorHighlight1;
					graphics.FillRectangle(0,0,width, height);
				}
				graphics.PenColor = Global::Colors.ToolButtonBorderHighLight;
				graphics.DrawRectangle(0,0,width,height);
				lblText->FontColor = Global::Colors.MenuItemHighlightForeColor;
			}
			else
			{
				if (Enabled)
					lblText->FontColor = Global::Colors.MenuItemForeColor;
				else
					lblText->FontColor = Global::Colors.MenuItemDisabledForeColor;
			}
			lblText->Draw((width-lblText->GetWidth())/2, 
				(height-entry->GetLineHeight())/2);
			
		}
	}

	void MenuItem::DrawMenuItem(int width, int itemHeight)
	{
		auto entry = GetEntry();
		auto & graphics = entry->DrawCommands;
		if (isSeperator)
		{
			graphics.PenColor = Global::Colors.MenuItemDisabledForeColor;
			graphics.DrawLine(itemHeight + separatorHeading, Height >> 1, width, Height >> 1);
		}
		else
		{
			if (Selected || (SubMenu && SubMenu->Visible))
			{
				if (SubMenu && SubMenu->Visible)
					graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorPressed1;
				else
					graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorHighlight1;
				graphics.FillRectangle(0, 0, width, itemHeight);
			}
			int top = (itemHeight - lblText->GetHeight())/2;
			if (!Enabled)
			{
				lblText->FontColor = Global::Colors.MenuItemDisabledForeColor;
				lblShortcut->FontColor = Global::Colors.MenuItemDisabledForeColor;
			}
			else
			{
				if (Selected)
				{
					lblText->FontColor = Global::Colors.MenuItemHighlightForeColor;
					lblShortcut->FontColor = Global::Colors.MenuItemHighlightForeColor;
				}
				else
				{
					lblText->FontColor = Global::Colors.MenuItemForeColor;
					lblShortcut->FontColor = Global::Colors.MenuItemForeColor;
				}
			}
			lblText->Draw(itemHeight + separatorHeading, top);
			lblShortcut->Draw(width - Padding.Right - lblShortcut->GetWidth(), top);
			if (SubMenu && SubMenu->Count())
			{
				int size = GetEntry()->GetLineHeight() >> 1;
				int x1 = width - Padding.Right;
				int y1 = itemHeight / 2 - size / 2;
				graphics.SolidBrushColor = lblText->FontColor;
				graphics.FillTriangle(x1, y1, x1 + size / 2, itemHeight / 2, x1, y1 + size);
			}
			if (Checked)
			{
				// Draw Checkmark
				if (Selected)
					graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorPressed1;
				else
					graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorHighlight1;
				const int IconMargin = 2;
				graphics.FillRectangle(0, 0, itemHeight, itemHeight);
				if (!Selected)
				{
					graphics.PenColor = Global::Colors.ToolButtonBorderHighLight;
					graphics.DrawRectangle(IconMargin, IconMargin, Height - IconMargin, Height-IconMargin);
				}
				entry->CheckmarkLabel->FontColor = lblText->FontColor;
				entry->CheckmarkLabel->Draw((itemHeight - entry->CheckmarkLabel->GetHeight())/2 + 2,
					(itemHeight - entry->CheckmarkLabel->GetHeight())/2);
			}
			
		}
	}

	void MenuItem::HandleMessage(const UI_MsgArgs * Args)
	{
		Control::HandleMessage(Args);
	}

	void MenuItem::DoDpiChanged()
	{
		Container::DoDpiChanged();
		Padding.Left = Padding.Right = GetEntry()->GetLineHeight() / 2;
	}

	void MenuItem::Hit()
	{
		Menu * mn = (Menu*)Parent;

		if (Parent && SubMenu && SubMenu->Count())
		{
			if (isButton)
				mn->PopupSubMenu(SubMenu, -Padding.Left, Height - Padding.Vertical());
			else
				mn->PopupSubMenu(SubMenu, Width - Padding.Left, -Padding.Top);
		}
		else
		{
			while (mn)
			{
				if (mn->style == Menu::msPopup)
					mn->CloseMenu();
				else
					break;
				if (mn->Parent)
					mn = dynamic_cast<Menu*>(mn->Parent->Parent);
			}
			OnClick(this);
		}
	}

	bool MenuItem::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X,Y, Shift);
		if (IsPointInClient(X, Y))
		{
			Hit();
			return true;
		}
		return false;
	}
	bool MenuItem::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X, Y, Shift);

		return false;
	}
	bool MenuItem::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key, Shift);
		if (SubMenu)
			SubMenu->DoKeyDown(Key,Shift);
		return false;
	}

	MenuItem * MenuItem::GetItem(int id)
	{
		if (SubMenu && SubMenu->Count() > id)
			return SubMenu->GetItem(id);
		else
			return 0;
	}

	ImageDisplay::ImageDisplay(Container * parent)
		: Container(parent)
	{
		BorderStyle = BS_LOWERED;
	}

	void ImageDisplay::SetImage(IImage *img)
	{
		image = img;
	}

	IImage * ImageDisplay::GetImage()
	{
		return image.operator->();
	}

	void ImageDisplay::Draw(int absX, int absY)
	{
		Control::Draw(absX, absY);
		absX += Left;
		absY += Top;
		if (image)
		{
			auto entry = GetEntry();
			entry->ClipRects->AddRect(Rect(absX, absY, Width-2, Height-2));
			GetEntry()->DrawCommands.DrawImage(image.Ptr(), absX, absY);
			entry->ClipRects->PopRect();
		}
	}

	ToolButton::ToolButton(ToolStrip * parent)
		: Container(parent)
	{
		ButtonStyle = bsNormal;
		Init();
	}

	ToolButton::ToolButton(ToolStrip * parent, const String & _text, _ButtonStyle bs, IImage * bmp)
		: Container(parent)
	{
		ButtonStyle = bs;
		Init();
		SetText(_text);
		SetImage(bmp);
	}

	void ToolButton::Init()
	{
		Type = CT_TOOL_BUTTON;
		Selected = false;
		Checked = false;
		Pressed = false;
		ShowText = false;
		lblText = new Label(this);
		bindButton = 0;
		Padding = imageLabelPadding = GetEntry()->GetLineHeight() / 4;
	}

	void ToolButton::SetImage(IImage * bmp)
	{
		if (bmp)
		{
			imageDisabled = image = bmp;
		}
	}


	String ToolButton::GetText()
	{
		return text;
	}

	void ToolButton::SetText(const String & _text)
	{
		text = _text;
		lblText->SetText(_text);
	}

	void ToolButton::BindButton(ToolButton * btn)
	{
		bindButton = btn;
		btn->bindButton = this;
	}

	bool ToolButton::DoMouseEnter()
	{
		Control::DoMouseEnter();
		if (Enabled && Visible)
			Selected = true;
		if (bindButton && bindButton->Enabled && bindButton->Visible)
			bindButton->Selected = true;
		return false;
	}

	bool ToolButton::DoMouseLeave()
	{
		Control::DoMouseLeave();
		Selected = false;
		if (bindButton)
			bindButton->Selected = false;
		return false;
	}

	int ToolButton::MeasureWidth()
	{
		int imgSize = image?image->GetWidth():0;
		int textWidth = imageLabelPadding + lblText->GetWidth();
		if (ButtonStyle == bsNormal)
			return imgSize + Padding.Horizontal() + (ShowText ? textWidth:0);
		else if (ButtonStyle == bsDropDown)
			return DropDownButtonWidth;
		else
			return Padding.Horizontal();
	}

	int ToolButton::MeasureHeight()
	{
		int imgSize = image?image->GetHeight():0;
		if (lblText->GetHeight() > imgSize)
			imgSize = lblText->GetHeight();
		return imgSize + Padding.Vertical();
	}

	bool ToolButton::DoMouseDown(int X, int Y, SHIFTSTATE shift)
	{
		Control::DoMouseDown(X, Y, shift);
		if (Enabled && Visible)
			Pressed = true;
		return false;
	}

	bool ToolButton::DoMouseUp(int X, int Y, SHIFTSTATE shift)
	{
		Control::DoMouseUp(X,Y,shift);
		Pressed = false;
		return false;
	}

	bool ToolButton::DoMouseMove(int X, int Y)
	{
		Control::DoMouseMove(X,Y);
		if (Enabled && Visible && IsPointInClient(X-Left, Y-Top))
		{
			Pressed = true;
			return true;
		}
		else
			Pressed = false;
		return false;
	}

	void ToolButton::Draw(int absX, int absY)
	{
		if (!Visible)
			return;
		absX += Left;
		absY += Top;
		auto entry = GetEntry();
		auto & graphics = entry->DrawCommands;
		if (ButtonStyle == bsSeperator)
		{
			graphics.PenColor = Global::Colors.ToolButtonSeperatorColor;
			graphics.DrawLine(absX+1,absY+1, absX+1, absY+Height-2);
			return;
		}
		bool drawbkg = true;
		if (Selected || Global::PointedComponent == this || Global::PointedComponent->IsChildOf(this))
		{
			if (Checked || Pressed)
			{
				graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorPressed1;
			}
			else
			{
				graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorHighlight1;
			}

		}
		else
		{
			if (Checked)
			{
				graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorChecked1;
			}
			else
			{
				drawbkg = false;
			}
		}
		if (drawbkg)
			graphics.FillRectangle(absX,absY,absX+Width-1,absY+Height-1);
		if (Selected || Checked)
		{
			graphics.PenColor = Global::Colors.ToolButtonBorderHighLight;
			graphics.DrawRectangle(absX,absY,absX+Width-1,absY+Height-1);
		}
		if (ButtonStyle == bsNormal)
		{
			int imgX=absX, imgY=absY;
			if (!ShowText)
			{
				if (image)
				{
					imgX += (Width-image->GetWidth())/2;
					imgY += (Height-image->GetHeight())/2;
				}
			}
			else
			{
				if (image)
				{
					imgX += imageLabelPadding;
					imgY += (Height-image->GetHeight())/2;
				}
			}
			if (Enabled)
			{
				if (image)
				{
					graphics.DrawImage(image.Ptr(), imgX, imgY);
				}
			}
			else
			{
				if (imageDisabled)
				{
					graphics.DrawImage(imageDisabled.Ptr(), imgX, imgY);
				}
			}
			if (ShowText)
			{
				int imgw = (image?image->GetWidth():0);
				lblText->Draw(imgX + imgw + imageLabelPadding, absY + (Height-lblText->GetHeight())/2);
			}

		}
		else
		{
			Color color;
			if (Enabled)
				color = Color(0,0,0,255);
			else
				color = Global::Colors.ToolButtonSeperatorColor;
			Array<Vec2, 3> polygon;
			graphics.SolidBrushColor = color;
			graphics.FillTriangle(absX + 3, absY + 10, absX + 7, absY + 10, absX + 5, absY + 12);
		}
				
	}

	ToolStrip::ToolStrip(Container * parent)
		: Container(parent)
	{
		DockStyle = dsTop;
		MultiLine = false;
		FullLineFill = true;
		ShowText = false;
		SetOrientation(Horizontal);
	}

	ToolButton * ToolStrip::AddButton(const String &text, IImage *bmp)
	{
		ToolButton * btn = new ToolButton(this, text, ToolButton::bsNormal, bmp);
		buttons.Add(btn);
		btn->Parent = this;
		PositButtons();
		return btn;
	}

	void ToolStrip::AddSeperator()
	{
		ToolButton * btn = new ToolButton(this, L"", ToolButton::bsSeperator, 0);
		buttons.Add(btn);
		btn->Parent = this;
		PositButtons();
	}

	bool ToolStrip::DoMouseLeave()
	{
		Control::DoMouseLeave();
		for (int i=0; i<buttons.Count(); i++)
			buttons[i]->Selected = false;
		return false;
	}

	void ToolStrip::SetOrientation(ToolStripOrientation ori)
	{
		orientation = ori;
		Padding = 0;
		if (orientation == Horizontal)
		{
			Padding.Left = GetEntry()->GetLineHeight() / 2;
			Padding.Top = Padding.Bottom = Padding.Left / 2;
		}
		else
		{
			Padding.Top = GetEntry()->GetLineHeight() / 2;
			Padding.Left = Padding.Right = Padding.Top / 2;
		}
	}

	ToolButton * ToolStrip::GetButton(int id)
	{
		return buttons[id];
	}

	int ToolStrip::Count()
	{
		return buttons.Count();
	}

	void ToolStrip::SizeChanged()
	{
		Control::SizeChanged();
		PositButtons();
	}

	void ToolStrip::PositButtons()
	{
		int left = Padding.Left;
		if (orientation == Horizontal)
		{
			if (!MultiLine)
			{
				int maxW = 0, maxH = 0;
				for (int i=0; i<buttons.Count(); i++)
				{
					buttons[i]->ShowText = ShowText;
					if (!buttons[i]->Visible)
						continue;
					int w = buttons[i]->MeasureWidth();
					int h = buttons[i]->MeasureHeight();
					if (w>maxW)
						maxW = w;
					if (h>maxH)
						maxH = h;
				}
				for (int i=0; i<buttons.Count(); i++)
				{
					if (!buttons[i]->Visible)
						continue;
					buttons[i]->Posit(left, 0, buttons[i]->MeasureWidth(), maxH);
					left += buttons[i]->GetWidth();
				}
				Width = left + Padding.Right;
				Height = maxH + Padding.Vertical();
			}
		}
		else
		{
			int maxW = 0, maxH = 0;
			int top = Padding.Top;
			for (int i=0; i<buttons.Count(); i++)
			{
				buttons[i]->ShowText = ShowText;
				if (!buttons[i]->Visible)
					continue;
				int w = buttons[i]->MeasureWidth();
				int h = buttons[i]->MeasureHeight();
				if (w>maxW)
					maxW = w;
				if (h>maxH)
					maxH = h;
			}
			for (int i=0; i<buttons.Count(); i++)
			{
				if (!buttons[i]->Visible)
					continue;
				int w = (FullLineFill?Width:maxW);
				buttons[i]->Posit(0, top, w, buttons[i]->MeasureHeight());
				top += buttons[i]->GetHeight();
			}
			Height = top + Padding.Top;
		}
	}

	void ToolStrip::Draw(int absX, int absY)
	{
		auto & graphics = GetEntry()->DrawCommands;
		graphics.SolidBrushColor = Global::Colors.ToolButtonBackColor1;
		graphics.FillRectangle(absX+Left,absY+Top,absX+Left+Width-1,absY+Top+Height-1);
		for (int i=0; i<buttons.Count(); i++)
			buttons[i]->Draw(absX+Left, absY+Top);
	}

	bool ToolStrip::DoMouseDown(int X, int Y, GraphicsUI::SHIFTSTATE shift)
	{
		Control::DoMouseDown(X,Y,shift);
		if (!Visible || !Enabled)
			return false;
		for (int i=0; i<buttons.Count(); i++)
			buttons[i]->DoMouseDown(X-Left, Y-Top, shift);
		return false;
	}

	bool ToolStrip::DoMouseUp(int X, int Y, GraphicsUI::SHIFTSTATE shift)
	{
		Control::DoMouseUp(X,Y,shift);
		if (!Visible || !Enabled)
			return false;
		for (int i=0; i<buttons.Count(); i++)
			buttons[i]->DoMouseUp(X-Left, Y-Top, shift);
		return false;
	}

	bool ToolStrip::DoMouseMove(int X, int Y)
	{
		Control::DoMouseMove(X,Y);
		if (!Visible || !Enabled)
			return false;
		if (!IsPointInClient(X-Left, Y-Top))
			return false;
		for (int i=0; i<buttons.Count(); i++)
		{
			int nx = X-Left;
			int ny = Y-Top;
			bool inside = (nx>buttons[i]->Left && nx<buttons[i]->Left+buttons[i]->GetWidth() &&
				ny>buttons[i]->Top && ny<buttons[i]->Top+buttons[i]->GetHeight());
			if (!buttons[i]->LastInClient && inside)
				buttons[i]->DoMouseEnter();
			if (buttons[i]->LastInClient && !inside)
				buttons[i]->DoMouseLeave();
			buttons[i]->LastInClient = inside;
			buttons[i]->DoMouseMove(X-Left, Y-Top);
		}
		return false;
	}

	StatusPanel::StatusPanel(StatusStrip * parent)
		: Container(parent)
	{
		Init();
		parent->AddItem(this);
	}

	StatusPanel::StatusPanel(StatusStrip * parent, const String & _text, int width, _FillMode fm)
		: Container(parent)
	{
		Init();
		FillMode = fm;
		SetText(_text);
		Width = width;
		Height = (int)(GetEntry()->GetLineHeight() * 1.2f);
		parent->AddItem(this);
	}

	void StatusPanel::Init()
	{
		BackColor = Color(0,0,0,0);
		BorderStyle = BS_NONE;
		FillMode = Fixed;
		Width = 50;
		text = new Label(this);
	}

	void StatusPanel::SetText(const String & _text)
	{
		text->SetText(_text);
	}

	String StatusPanel::GetText()
	{
		return text->GetText();
	}

	int StatusPanel::MeasureWidth()
	{
		if (FillMode == Fixed)
			return Width;
		else if (FillMode == AutoSize)
			return text->GetWidth();
		else
			return -1;
	}

	void StatusPanel::Draw(int absX, int absY)
	{
		Control::Draw(absX, absY);
		auto entry = GetEntry();
		entry->ClipRects->AddRect(Rect(absX + Left, absY + Top, Width - text->TextHeight, Height));
		text->Draw(absX+Left, absY+Top + ((Height - text->TextHeight) >> 1));
		entry->ClipRects->PopRect();
	}
		
	StatusStrip::StatusStrip(Container * parent)
		: Container(parent)
	{
		DockStyle = dsBottom;
		DoDpiChanged();
	}

	void StatusStrip::AddItem(GraphicsUI::StatusPanel *panel)
	{
		panels.Add(panel);
		Height = Math::Max(Height, panel->GetHeight());
	}

	int StatusStrip::Count()
	{
		return panels.Count();
	}

	StatusPanel * StatusStrip::GetItem(int id)
	{
		return panels[id];
	}

	void StatusStrip::PositItems()
	{
		int fc = 0;
		int w = Width - Padding.Horizontal();
		for (int i=0; i<panels.Count(); i++)
		{
			int cw = panels[i]->MeasureWidth();
			if (cw!=-1)
				w -= cw;
			else
				fc ++;
		}
		if (fc == 0)
			fc = 1;
		int fw = w/fc;
		int h = Height - Padding.Vertical();
		int left = 0;
		for (int i=0; i<panels.Count(); i++)
		{
			int cw = panels[i]->MeasureWidth();
			if (cw != -1)
			{
				panels[i]->Posit(left, 0, cw, h);
				left += cw;
			}
			else
			{
				panels[i]->Posit(left, 0, fw, h);
				left += fw;
			}
		}
	}

	void StatusStrip::Draw(int absX, int absY)
	{
		absX += Left;
		absY += Top;
		PositItems();
		auto & graphics = GetEntry()->DrawCommands;
		graphics.SolidBrushColor = Global::Colors.StatusStripBackColor1;
		graphics.FillRectangle(absX, absY, absX+Width, absY+Height);
		for (int i=0; i<panels.Count(); i++)
		{
			panels[i]->Draw(absX, absY);
		}
	}

	bool StatusStrip::DoMouseMove(int X, int Y)
	{
		Control::DoMouseMove(X,Y);
		if (!Enabled || !Visible)
			return false;
		for (int i=0; i<panels.Count(); i++)
		{
			panels[i]->DoMouseMove(X-Left, Y-Top);
		}
		return false;
	}

	bool StatusStrip::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X,Y,Shift);
		if (!Enabled || !Visible)
			return false;
		for (int i=0; i<panels.Count(); i++)
		{
			panels[i]->DoMouseUp(X-Left, Y-Top, Shift);
		}
		return false;
	}

	void StatusStrip::DoDpiChanged()
	{
		Container::DoDpiChanged();
		Padding.Top = Padding.Bottom = 0;
		Padding.Left = Padding.Right = GetEntry()->GetLineHeight() / 2;
		Height = (int)(GetEntry()->GetLineHeight() * 1.2f);
		PositItems();
	}

	bool StatusStrip::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X,Y,Shift);
		if (!Enabled || !Visible)
			return false;
		for (int i=0; i<panels.Count(); i++)
		{
			panels[i]->DoMouseDown(X-Left, Y-Top, Shift);
		}
		return false;
	}

	TabPage::TabPage(TabControl * parent)
		: Container(parent)
	{
		text = new Label(this);
		text->Visible = false;
		BorderStyle = BS_NONE;
		parent->AddItem(this);
		imageTextPadding = GetEntry()->GetLineHeight() / 2;
		Padding = imageTextPadding;
	}

	TabPage::TabPage(TabControl * parent, CoreLib::String text)
		: TabPage(parent)
	{
		SetText(text);
	}

	void TabPage::SetText(const String &_text)
	{
		text->SetText(_text);
	}

	String TabPage::GetText()
	{
		return text->GetText();
	}

	void TabPage::SetImage(IImage *bitmap)
	{
		image = bitmap;
	}

	int TabPage::MeasureWidth(TabControl::_TabStyle style)
	{
		switch (style)
		{
		case TabControl::tsImage:
			return (image?image->GetWidth():0);
		case TabControl::tsText:
			return text->GetWidth();
		case TabControl::tsTextImage:
			return text->GetWidth() + (image?image->GetWidth() + imageTextPadding : 0);
		default:
			return 0;
		}
	}

	int TabPage::MeasureHeight(TabControl::_TabStyle style)
	{
		switch (style)
		{
		case TabControl::tsImage:
			return (image?image->GetHeight():0);
		case TabControl::tsText:
			return text->GetHeight() + Padding.Vertical();
		case TabControl::tsTextImage:
			return Math::Max(text->GetHeight(), (image?image->GetHeight():0));
		default:
			return 0;
		}
	}

	TabControl::TabControl(Container * parent)
		: Container(parent)
	{
		highlightItem = -1;
		SelectedIndex = -1;
		CanClose = false;
		CanMove = false;
		TabStyle = tsTextImage;
		TabPosition = tpTop;
		DoDpiChanged();
	}

	void TabControl::SetClient()
	{
		if (TabPosition == tpTop)
		{
			clientRect.x = 0;
			clientRect.y = headerHeight;
			clientRect.w = Width;
			clientRect.h = Height - headerHeight;
		}
		else
		{
			clientRect.x = 0;
			clientRect.y = 0;
			clientRect.w = Width;
			clientRect.h = Height - headerHeight;
		}
		for (int i=0; i<pages.Count(); i++)
		{
			pages[i]->Posit(0, 0, clientRect.w, clientRect.h);
		}
	}

	void TabControl::AddItem(TabPage *page)
	{
		page->Parent = this;
		page->Visible = false;
		pages.Add(page);
		headerHeight = MeasureHeight();
		SetClient();
		if (SelectedIndex == -1)
			SwitchPage(0);
	}

	void TabControl::RemoveItem(TabPage * page)
	{
		int fid = pages.IndexOf(page);
		if (fid != -1)
		{
			pages[fid] = 0;
			pages.RemoveAt(fid);
		}
		RemoveChild(page);
		if (SelectedIndex == fid)
			SwitchPage(SelectedIndex-1);
		headerHeight = MeasureHeight();
		SetClient();
	}

	void TabControl::SwitchPage(int id)
	{
		for (int i=0; i<pages.Count(); i++)
			pages[i]->Visible = false;
		pages[id]->Visible = true;
		SelectedIndex = id;
	}

	TabPage * TabControl::GetItem(int id)
	{
		return pages[id];
	}

	TabPage * TabControl::GetSelectedItem()
	{
		if (SelectedIndex != -1)
			return pages[SelectedIndex];
		else
			return 0;
	}

	void TabPage::DrawHeader(int x, int y, int h, const MarginValues & headerPadding, TabControl::_TabStyle style)
	{
		switch (style)
		{
		case TabControl::tsTextImage:
			{
				int cw = x + headerPadding.Left;
				if (image)
				{
					GetEntry()->DrawCommands.DrawImage(image.Ptr(), cw, y + headerPadding.Top);
					cw += image->GetWidth() + imageTextPadding;
				}
				text->Draw(cw, y + (h-text->GetHeight())/2);
			}
			break;
		case TabControl::tsText:
			{
				text->Draw(x + Padding.Left, y + Padding.Top);
			}
			break;
		case TabControl::tsImage:
			{
				if (image)
					GetEntry()->DrawCommands.DrawImage(image.Ptr(), x + headerPadding.Left, y + headerPadding.Top);
			}
			break;
		}
	}

	int TabControl::MeasureHeight()
	{
		int h = 0;
		for (int i=0; i<pages.Count(); i++)
		{
			int ch = pages[i]->MeasureHeight(TabStyle);
			if (ch>h)
				h = ch;
		}
		return h + HeaderPadding.Vertical();
	}

	int TabControl::HitTest(int X, int Y)
	{
		bool inHeader = false;
		if (TabPosition == tpTop)
		{
			inHeader = (Y < headerHeight && Y > 0);
		}
		else
			inHeader = (Y > Height-headerHeight && Y<Height);
		if (inHeader)
		{
			int cw = 0;
			for (int i=0; i<pages.Count(); i++)
			{
				int pw = pages[i]->MeasureWidth(TabStyle) + HeaderPadding.Horizontal();
				if (X>cw && X<=cw+pw)
				{
					return i;
				}
				cw += pw;
			}
			return -1;
		}
		else
			return -1;
	}

	void TabControl::SizeChanged()
	{
		Control::SizeChanged();
		SetClient();
	}

	bool TabControl::DoMouseMove(int X, int Y)
	{
		Container::DoMouseMove(X,Y);
		if (!Visible || !Enabled)
			return false;
		highlightItem = HitTest(X,Y);
		return false;
	}

	bool TabControl::DoMouseDown(int X, int Y, GraphicsUI::SHIFTSTATE Shift)
	{
		Container::DoMouseDown(X,Y,Shift);
		if (!Visible || !Enabled)
			return false;
		int citem = HitTest(X,Y);
		if (citem != -1)
			SwitchPage(citem);
		return false;
	}

	bool TabControl::DoMouseUp(int X, int Y, GraphicsUI::SHIFTSTATE Shift)
	{
		Container::DoMouseUp(X,Y,Shift);
		if (!Visible || !Enabled)
			return false;
		return false;
	}

	void TabControl::DoDpiChanged()
	{
		Container::DoDpiChanged();
		HeaderPadding.Left = HeaderPadding.Right = GetEntry()->GetLineHeight() / 2;
		HeaderPadding.Top = HeaderPadding.Bottom = HeaderPadding.Left / 4;
		headerHeight = MeasureHeight() + HeaderPadding.Vertical();
	}

	void TabControl::Draw(int absX, int absY)
	{
		SetClient();
		absX += Left;
		absY += Top;
		if (!Visible)
			return;
		int maxWidth = Width-16;
		if (CanClose)
			Width -= 16;
		int cw = 0;
		TabPage * page = GetSelectedItem();
		auto entry = GetEntry();
		if (page)
		{
			entry->ClipRects->AddRect(Rect(absX+clientRect.x, absY+clientRect.y, absX+clientRect.x+clientRect.w, absY+clientRect.y+clientRect.h));
			page->Draw(absX+clientRect.x, absY+clientRect.y);
			entry->ClipRects->PopRect();
		}
		auto & graphics = entry->DrawCommands;
		graphics.SetRenderTransform(absX, absY);
		int h0 = Height-headerHeight-1;
		for (int i=0; i<pages.Count(); i++)
		{
			int pw = pages[i]->MeasureWidth(TabStyle) + HeaderPadding.Horizontal();
			if (cw + pw > maxWidth)
				break;
			if (SelectedIndex != i && highlightItem != i)
			{
				graphics.SolidBrushColor = Global::Colors.TabPageItemBackColor1;
			}
			else if (SelectedIndex == i)
			{
				graphics.SolidBrushColor = Global::Colors.TabPageItemSelectedBackColor1;
			}
			else
			{
				graphics.SolidBrushColor = Global::Colors.TabPageItemHighlightBackColor1;
			}
			graphics.PenColor = Global::Colors.TabPageBorderColor;
			if (TabPosition == tpTop)
			{
				graphics.FillRectangle(cw, 0, cw+pw, headerHeight);
				graphics.DrawLine(cw,0,cw+pw,0);
				graphics.DrawLine(cw,0, cw, headerHeight - 2);
				graphics.DrawLine(cw+pw,0, cw+pw, headerHeight - 2);
				if (SelectedIndex != i)
				{
					graphics.DrawLine(cw, headerHeight - 1, cw+pw, headerHeight - 1);
				}
				pages[i]->DrawHeader(cw, 0, headerHeight, HeaderPadding, TabStyle);
			}
			else
			{
				graphics.FillRectangle(cw, h0+headerHeight, cw+pw, h0);
				graphics.DrawLine(cw,h0, cw, h0+headerHeight);
				graphics.DrawLine(cw+pw,h0, cw+pw, h0+headerHeight);
				graphics.DrawLine(cw, h0+headerHeight, cw+pw, h0+headerHeight);
				if (SelectedIndex != i)
				{
					graphics.DrawLine(cw,h0,cw+pw,h0);
				}
				pages[i]->DrawHeader(cw, h0, headerHeight, HeaderPadding, TabStyle);
			}
				
			cw += pw;
		}
			
		if (TabPosition == tpTop)
		{
			graphics.DrawLine(cw,headerHeight, Width, headerHeight);
			graphics.DrawLine(0,headerHeight,0,Height-1);
			graphics.DrawLine(Width-1,headerHeight,Width-1,Height-1);
			graphics.DrawLine(0,Height-1,Width,Height-1);
		}
		else
		{
			graphics.DrawLine(cw,h0, Width, h0);
			graphics.DrawLine(0,0,0,Height-headerHeight);
			graphics.DrawLine(Width-1,0,Width-1,Height-1-headerHeight);
			graphics.DrawLine(0,0,Width,0);
		}
		graphics.SetRenderTransform(0, 0);
	}

	class DeviceNotReadyException
	{};

	UpDown::UpDown(Container * parent, GraphicsUI::TextBox *txtBox, float _min, float _max, float minInc, float maxInc)
		: Container(parent)
	{
		Digits = 3;
		state = 0;
		text = txtBox;
		Min = _min;
		Max = _max;
		MinIncrement = minInc;
		MaxIncrement = maxInc;
		Left = text->Left + text->GetWidth();
		Height = text->GetHeight();
		Top = text->Top;
		Width = 16;
		btnUp = new Button(this);
		btnUp->SetHeight(Height/2);
		btnDown = new Button(this);
		btnDown->SetHeight(Height/2);
		btnUp->SetWidth(Width);
		btnDown->SetWidth(Width);
		auto symFont = GetEntry()->System->LoadDefaultFont(DefaultFontType::Symbol);
		btnUp->SetFont(symFont);
		btnDown->SetFont(symFont);
		btnUp->SetText(L"5");
		btnDown->SetText(L"6");
	}

	UpDown::~UpDown()
	{
		GetEntry()->UnSubscribeTickEvent(this);
	}

	void UpDown::Draw(int absX, int absY)
	{
		btnUp->BorderStyle = btnDown->BorderStyle = BS_RAISED;
		if (state == 1)
		{
			btnUp->BorderStyle = BS_LOWERED;
		}
		else if (state == 2)
		{
			btnDown->BorderStyle = BS_LOWERED;
		}
		absX += Left;
		absY += Top;
		btnUp->Draw(absX,absY);
		btnDown->Draw(absX,absY+btnUp->GetHeight());
	}

	bool UpDown::DoTick()
	{
		float val = (float)StringToDouble(text->GetText());
		if (state == 1)
			val += inc;
		else
			val -= inc;
		val = Math::Max(Min, val);
		val = Math::Min(Max, val);
		text->SetText(String(val, (L"%." + String(Digits) + L"f").Buffer()));
		return true;
	}

	bool UpDown::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoMouseDown(X,Y, Shift);
		ldY = Y;
		if (Y-Top<Height/2)
			state = 1;
		else
			state = 2;
		inc = MinIncrement;
		Global::MouseCaptureControl = this;
		return false;
	}

	bool UpDown::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X,Y,Shift);
		state = 0;
		GetEntry()->UnSubscribeTickEvent(this);
		ReleaseMouse();
		return false;
	}

	bool UpDown::DoMouseHover()
	{
		if (state != 0)
		{
			GetEntry()->SubscribeTickEvent(this);
		}
		return true;
	}

	bool UpDown::DoMouseMove(int /*X*/, int Y)
	{
		if (state)
		{
			int dY = Y-ldY;
			float s = fabs(dY/100.0f);
			inc = MinIncrement * (1.0f-s) + MaxIncrement * s;
		}
		return false;
	}
	UIEntry * Control::GetEntry()
	{
		if (entryCache == nullptr)
		{
			Control * parent = Parent;
			if (parent)
				entryCache = parent->GetEntry();
		}
		return entryCache;
	}
	void VScrollPanel::ScrollBar_Changed(UI_Base * /*sender*/)
	{
		content->Top = -vscrollBar->GetPosition();
	}

	VScrollPanel::VScrollPanel(Container * parent)
		: Container(parent)
	{
		vscrollBar = new ScrollBar(this, false);
		content = new Container(this, false);
		Container::AddChild(vscrollBar);
		Container::AddChild(content);
		vscrollBar->SetOrientation(SO_VERTICAL);
		vscrollBar->Posit(0, 0, Global::SCROLLBAR_BUTTON_SIZE, 50);
		vscrollBar->DockStyle = dsRight;
		vscrollBar->SmallChange = 30;
		vscrollBar->OnChanged.Bind(this, &VScrollPanel::ScrollBar_Changed);
		content->AutoHeight = true;
		BorderStyle = content->BorderStyle = BS_NONE;
		BackColor.A = content->BackColor.A = 0;
	}
	void VScrollPanel::SizeChanged()
	{
		content->SizeChanged();
		//for (auto & ctrl : content->GetChildren())
		//	maxY = Math::Max(ctrl->Top + ctrl->GetHeight(), maxY);
		int maxY = content->GetHeight();
		auto entry = GetEntry();
		vscrollBar->LargeChange = Math::Max(Height - 30, 10);
		if (maxY > Height)
		{
			maxY += entry->GetLineHeight() * 3;
			if (!vscrollBar->Visible)
			{
				vscrollBar->Visible = true;
				SizeChanged();
				return;
			}
			int vmax = maxY - Height;
			vscrollBar->SetValue(0, vmax, Math::Clamp(vscrollBar->GetPosition(), 0, vmax), Height);
			vscrollBar->Visible = true;
		}
		else
		{
			vscrollBar->SetPosition(0);
			vscrollBar->Visible = false;
		}
		vscrollBar->Posit(0, 0, Global::SCROLLBAR_BUTTON_SIZE, Height - 2);
		content->Posit(0, -vscrollBar->GetPosition(), vscrollBar->Visible ? Width - vscrollBar->GetWidth() : Width, maxY);
		Container::SizeChanged();
	}
	void VScrollPanel::AddChild(Control * ctrl)
	{
		content->AddChild(ctrl);
		SizeChanged();
	}
	void VScrollPanel::RemoveChild(Control * ctrl)
	{
		content->RemoveChild(ctrl);
		SizeChanged();
	}
	bool VScrollPanel::DoMouseWheel(int delta)
	{
		if (vscrollBar->Visible)
		{
			int nPos = vscrollBar->GetPosition() + (delta < 0 ? 1 : -1) * GetEntry()->GetLineHeight() * 3;
			nPos = Math::Clamp(nPos, vscrollBar->GetMin(), vscrollBar->GetMax());
			vscrollBar->SetPosition(nPos);
			return true;
		}
		return false;
	}
	void VScrollPanel::DoFocusChange()
	{
		Container::DoFocusChange();
		auto focusedCtrl = GetEntry()->FocusedControl;
		if (focusedCtrl && focusedCtrl->IsChildOf(content))
		{
			auto pos = focusedCtrl->GetRelativePos(content);
			if (pos.y - vscrollBar->GetPosition() < 0)
			{
				vscrollBar->SetPosition(Math::Clamp(pos.y, vscrollBar->GetMin(), vscrollBar->GetMax()));
			}
			else if (pos.y - vscrollBar->GetPosition() + focusedCtrl->GetHeight() > Height)
			{
				vscrollBar->SetPosition(Math::Clamp(pos.y - Height + focusedCtrl->GetHeight(), vscrollBar->GetMin(), vscrollBar->GetMax()));
			}
		}
	}
	ContainerLayoutType VScrollPanel::GetLayout()
	{
		return content->GetLayout();
	}
	void VScrollPanel::SetLayout(ContainerLayoutType pLayout)
	{
		content->SetLayout(pLayout);
	}
	void VScrollPanel::ClearChildren()
	{
		for (auto & child : content->GetChildren())
			child = nullptr;
		content->GetChildren().Clear();
	}
	int VScrollPanel::GetClientWidth()
	{
		return content->GetWidth();
	}
	int VScrollPanel::GetClientHeight()
	{
		return content->GetHeight();
	}
	Line::Line(Container * owner)
		: Control(owner)
	{
	}
	void Line::Draw(int absX, int absY)
	{
		auto & graphics = GetEntry()->DrawCommands;
		graphics.PenColor = BorderColor;
		graphics.DrawLine(absX + Left, absY + Top, absX + Left + Width - 1, absY + Top + Height - 1);
	}

	CommandForm::CommandForm(UIEntry * parent)
		:Form(parent)
	{
		this->SetText(L"Command Prompt");
		txtCmd = new TextBox(this);
		txtCmd->SetHeight((int)(GetEntry()->GetLineHeight() * 1.2f));
		txtCmd->DockStyle = dsBottom;
		textBox = CreateMultiLineTextBox(this);
		textBox->DockStyle = dsFill;
		textBox->BorderStyle = BS_NONE;
		textBox->TabStop = false;
		textBox->SetReadOnly(true);
		txtCmd->OnKeyDown.Bind([=](UI_Base *, UIKeyEventArgs & e)
		{
			if (e.Key == Keys::Return)
			{
				auto cmdText = txtCmd->GetText();
				if (cmdText.Length())
				{
					commandHistories.Add(cmdText);
					cmdPtr = commandHistories.Count();
					txtCmd->SetText(L"");
					Write(L"> " + cmdText + L"\n");
					OnCommand(cmdText);

					auto pos = textBox->GetCaretPos();
					if (pos.Col > 0)
						textBox->InsertText(L"\n");
				}
			}
			else if (e.Key == Keys::Up)
			{
				cmdPtr--;
				if (cmdPtr < 0)
					cmdPtr = 0;
				if (cmdPtr < commandHistories.Count())
				{
					txtCmd->SetText(commandHistories[cmdPtr]);
				}
			}
			else if (e.Key == Keys::Down)
			{
				cmdPtr++;
				if (cmdPtr >= commandHistories.Count())
					cmdPtr = commandHistories.Count();
				if (cmdPtr < commandHistories.Count())
					txtCmd->SetText(commandHistories[cmdPtr]);
				else
					txtCmd->SetText(L"");
			}
		});
		this->Posit(10, 10, 500, 400);
	}
	void CommandForm::Write(const CoreLib::String & text)
	{
		textBox->MoveCaretToEnd();
		textBox->InsertText(text);
		while (textBox->GetLineCount() > 2048)
			textBox->DeleteLine(0);
	}
	bool CommandForm::DoMouseUp(int x, int y, SHIFTSTATE shift)
	{
		Form::DoMouseUp(x, y, shift);
		if (this->Visible)
			txtCmd->SetFocus();
		return true;
	}
	void UICommandLineWriter::Write(const String & text)
	{
		cmdForm->Write(text);
	}
}

/***********************************************************************
LIBUI_MULTITEXTBOX.CPP
***********************************************************************/
namespace GraphicsUI
{
	using namespace CoreLib;
	using namespace VectorMath;

	bool IsPunctuation(unsigned int ch)
	{
		if ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z'))
			return false;
		return true;
	}

	class TextLine
	{
		List<int> lineStarts;
	public:
		List<unsigned int> Chars;
		int DisplayLines = 1;
		TextLine()
		{
			lineStarts.Clear();
		}
		int GetSubLineIndex(int col)
		{
			if (DisplayLines == 1)
				return 0;
			int result = 0;
			while (result < lineStarts.Count() && lineStarts[result] <= col)
				result++;
			return result;
		}
		int GetSubLineStartCharIndex(int subLine)
		{
			if (subLine <= 0)
				return 0;
			else if (subLine <= lineStarts.Count())
				return lineStarts[subLine - 1];
			else if (lineStarts.Count())
				return lineStarts.Last();
			else
				return 0;
		}
		int GetSubLineEndCharIndex(int subLine)
		{
			if (lineStarts.Count() == 0)
				return Chars.Count();
			if (subLine < 0)
				subLine = 0;
			if (subLine < lineStarts.Count())
				return lineStarts[subLine];
			else
				return Chars.Count();
		}
		String ToString()
		{
			StringBuilder sb;
			for (auto ch : Chars)
				if (ch == L'\t')
					sb << L"    ";
				else
					sb << (wchar_t)ch;
			return sb.ProduceString();
		}
		String GetDisplayLine(int i, int tabSpaces, int charCount = -1)
		{
			StringBuilder sb;
			if (charCount == 0)
				return String();
			if (lineStarts.Count() == 0)
			{
				if (charCount == -1)
					charCount = Chars.Count();
				for (auto ch : Chars)
				{
					if (ch == L'\t')
					{
						for (int z = 0; z < tabSpaces; z++)
							sb << L" ";
					}
					else
						sb << (wchar_t)ch;
					charCount--;
					if (charCount == 0)
						break;
				}
			}
			else
			{
				int start = i == 0 ? 0 : lineStarts[i - 1];
				int end = i < lineStarts.Count() ? lineStarts[i] : Chars.Count();
				if (charCount == -1)
					charCount = end - start;
				for (int j = start; j < end; j++)
				{
					if (Chars[j] == L'\t')
					{
						for (int z = 0; z < tabSpaces; z++)
							sb << L" ";
					}
					else
						sb << (wchar_t)Chars[j];
					charCount--;
					if (charCount == 0)
						break;
				}
			}
			return sb.ProduceString();
		}

		void ClearWordWrap()
		{
			DisplayLines = 1;
			lineStarts.Clear();
		}
		template<typename StringMeasurer>
		void WordWrap(int width, StringMeasurer & measurer)
		{
			DisplayLines = 1;
			lineStarts.Clear();
			int ptr = 0;
			while (ptr < Chars.Count())
			{
				int lineStart = ptr;
				int space = width;
				int lastBreakable = lineStart;
				int i = ptr;
				while (i < Chars.Count())
				{
					int charWidth = measurer.GetCharWidth(Chars[i]);
					if (space > charWidth)
					{
						if (IsPunctuation(Chars[i]))
							lastBreakable = i + 1;
						space -= charWidth;
						i++;
					}
					else
					{
						DisplayLines++;
						break;
					}
					
				}
				if (i < Chars.Count())
				{
					if (lastBreakable > lineStart)
						ptr = lastBreakable;
					else if (i == ptr)
						ptr = i + 1;
					else
						ptr = i;
					lineStarts.Add(Math::Min(Chars.Count(), ptr));
				}
				else
					break;
			}
		}
	};
	class TextBuffer
	{
	public:
		List<TextLine> Lines;
		TextBuffer()
		{
			Lines.Add(TextLine());
		}
		String GetAllText()
		{
			StringBuilder sb;
			for (int i = 0; i < Lines.Count(); i++)
			{
				for (auto & ch : Lines[i].Chars)
					sb << (wchar_t)ch;
				if (i < Lines.Count() - 1)
					sb << L'\n';
			}
			return sb.ProduceString();
		}
		int GetDisplayLineCount()
		{
			int count = 0;
			for (auto & line : Lines)
				count += line.DisplayLines;
			return count;
		}
		CaretPos LogicalToPhysical(const CaretPos & logical)
		{
			CaretPos rs;
			rs.Line = 0;
			int line = Math::Clamp(logical.Line, 0, Lines.Count() - 1);
			for (int i = 0; i < line; i++)
				rs.Line += Lines[i].DisplayLines;
			int subLine = Lines[line].GetSubLineIndex(logical.Col);
			rs.Line += subLine;
			rs.Col = logical.Col - Lines[line].GetSubLineStartCharIndex(subLine);
			return rs;
		}
		CaretPos PhysicalToLogical(const CaretPos & physical, bool wordWrap, int & subLine)
		{
			CaretPos rs;
			int logicalLine, logicalCol;
			if (!wordWrap)
			{
				subLine = 0;
				logicalLine = physical.Line;
				logicalCol = 0;
				if (logicalLine < 0)
					logicalLine = logicalCol = 0;
				else if (logicalLine < Lines.Count())
					logicalCol = Math::Clamp(physical.Col, 0, Lines[logicalLine].Chars.Count());
				else
				{
					logicalLine = Lines.Count() - 1;
					logicalCol = Lines.Last().Chars.Count();
				}
			}
			else
			{
				logicalLine = 0;
				subLine = physical.Line;
				while (logicalLine < Lines.Count() && subLine >= Lines[logicalLine].DisplayLines)
				{
					subLine -= Lines[logicalLine].DisplayLines;
					logicalLine++;
				}
				if (logicalLine < Lines.Count())
					logicalCol = Math::Min(Lines[logicalLine].GetSubLineStartCharIndex(subLine) + physical.Col,
						Lines[logicalLine].GetSubLineEndCharIndex(subLine));
				else
				{
					logicalLine = Lines.Count() - 1;
					logicalCol = Lines.Last().Chars.Count();
				}
			}
			return CaretPos(logicalLine, logicalCol);
		}
		String GetDisplayLine(int i, bool wordWrap, int tabSpaces, int & logicalLine, int & logicalCol)
		{
			if (!wordWrap)
			{
				logicalLine = i;
				logicalCol = 0;
				if (i < Lines.Count())
					return Lines[i].ToString();
				else
					return String();
			}
			else
			{
				logicalLine = 0;
				int subLine = i;
				while (logicalLine < Lines.Count() && subLine >= Lines[logicalLine].DisplayLines)
				{
					subLine -= Lines[logicalLine].DisplayLines;
					logicalLine++;
				}
				if (logicalLine < Lines.Count())
				{
					logicalCol = Lines[logicalLine].GetSubLineStartCharIndex(subLine);
					return Lines[logicalLine].GetDisplayLine(subLine, tabSpaces);
				}
				else
				{
					logicalCol = 0;
					return String();
				}
			}
		}
		String GetLine(int i)
		{
			if (i < 0 || i >= Lines.Count())
				return String();
			StringBuilder sb;
			for (auto & ch : Lines[i].Chars)
				sb << (wchar_t)ch;
			return sb.ProduceString();
		}
		
		void SetText(const String & text)
		{
			Lines.Clear();
			Lines.Add(TextLine());
			int line = 0;
			bool ignoreN = false;
			for (auto & ch : text)
			{
				if (ignoreN && ch == L'\n')
				{
					ignoreN = false;
					continue;
				}
				if (ch == L'\r')
				{
					ignoreN = true;
					line++;
					Lines.Add(TextLine());
				}
				else
				{
					if (ch == L'\n')
					{
						line++;
						Lines.Add(TextLine());
					}
					else
						Lines[line].Chars.Add(ch);
					ignoreN = false;
				}
			}
		}
	};

	struct ScreenLine
	{
		Label * label;
		int LogicalLine;
		int LogicalCol;
	};

	enum class OperationName
	{
		None,
		InsertText,
		DeleteText,
		ReplaceText
	};

	class Operation
	{
	public:
		OperationName Name = OperationName::None;
		String Text, Text1;
		CaretPos SelEnd1; // for replace command
		CaretPos SelStart, SelEnd, Pos;
	};

	class OperationStack
	{
	public:
		List<Operation> operations;
		int undoPtr = 0;
		int firstPtr = 0;
		int endPtr = 0;
		bool locked = false;
		bool IsBufferEmpty()
		{
			return undoPtr != firstPtr;
		}
		Operation & GetLastOperation()
		{
			return operations[(operations.Count() + (endPtr - 1)) % operations.Count()];
		}
		CoreLib::Diagnostics::TimePoint lastOperationTime;
		OperationStack()
		{
			operations.SetSize(1024);
		}
		void SetSize(int size)
		{
			operations.SetSize(size);
			Clear();
		}
		void Lock()
		{
			locked = true;
		}
		void Unlock()
		{
			locked = false;
		}
		void PushOperation(const Operation & op)
		{
			if (locked)
				return;
			CoreLib::Diagnostics::TimePoint time = CoreLib::Diagnostics::PerformanceCounter::Start();
			auto & lastOp = GetLastOperation();
			if (op.Name == OperationName::InsertText && endPtr != firstPtr && lastOp.Name == OperationName::InsertText
				&& CoreLib::Diagnostics::PerformanceCounter::ToSeconds(time - lastOperationTime) < 0.5f &&
				op.Pos.Line == lastOp.Pos.Line &&
				op.Pos.Col == lastOp.Pos.Col + lastOp.Text.Length() &&
				op.SelEnd.Line == op.Pos.Line)
			{
				// merge insert commands
				lastOp.Text = lastOp.Text + op.Text;
				lastOp.SelEnd.Col = lastOp.Pos.Col + lastOp.Text.Length();
			}
			else
			{
				endPtr = undoPtr;
				operations[endPtr] = op;
				endPtr++;
				endPtr %= operations.Count();
				undoPtr = endPtr;
				if (endPtr == firstPtr)
				{
					firstPtr++;
					firstPtr %= operations.Count();
				}
			}
			lastOperationTime = time;
		}
		Operation GetNextRedo()
		{
			if (undoPtr != endPtr)
			{
				int oldUndoPtr = undoPtr;
				undoPtr++;
				undoPtr %= operations.Count();
				return operations[oldUndoPtr];
			}
			else
				return Operation();
		}
		Operation PopOperation()
		{
			if (undoPtr != firstPtr)
			{
				undoPtr--;
				undoPtr += operations.Count();
				undoPtr %= operations.Count();
				return operations[undoPtr];
			}
			else
				return Operation();
		}
		void Clear()
		{
			undoPtr = endPtr = firstPtr = 0;
		}
	};

	class MultiLineTextBoxImpl : public MultiLineTextBox
	{
	private:
		const int TabSpaces = 4;
		int desiredCol = 0;
		CaretPos caretPos, selStart, selEnd, wordSelStart;
		CaretPos physicalSelStart, physicalSelEnd;
		int caretDocumentPosX = 0;
		int caretDocumentPosY = 0;
		bool readOnly = false;
		bool caretPosChanged = true;
		bool selecting = false;
		bool wordSelecting = false;
		List<ScreenLine> screen;
		CoreLib::Diagnostics::TimePoint time;
		int lineHeight = 20;
		bool wordWrap = true;
		int maxLineWidth = 0;
		bool invalidateScreen = true;
		TextBuffer textBuffer;
		Container * content;
		ScrollBar * vScroll, *hScroll;
		Menu * contextMenu;
		Dictionary<unsigned int, int> textWidthCache;
		OperationStack operationStack;
		void SelectionChanged()
		{
			if (selStart < selEnd)
			{
				physicalSelStart = textBuffer.LogicalToPhysical(selStart);
				physicalSelEnd = textBuffer.LogicalToPhysical(selEnd);
			}
			else
			{
				physicalSelStart = textBuffer.LogicalToPhysical(selEnd);
				physicalSelEnd = textBuffer.LogicalToPhysical(selStart);
			}
			caretPosChanged = true;
			OnCaretPosChanged(this);
		}

		void UpdateCaretDocumentPos()
		{
			auto docPos = CaretPosToDocumentPos(caretPos);
			caretDocumentPosX = docPos.x;
			caretDocumentPosY = docPos.y;
		}
		Vec2i CaretPosToDocumentPos(const CaretPos & cpos)
		{
			int docPosX = 0;
			int docPosY = 0;
			if (cpos.Line >= 0 && cpos.Line < textBuffer.Lines.Count())
			{
				for (int i = 0; i < cpos.Line; i++)
				{
					docPosY += textBuffer.Lines[i].DisplayLines;
				}
				int subLine = textBuffer.Lines[cpos.Line].GetSubLineIndex(cpos.Col);
				docPosY += subLine;
				docPosY *= lineHeight;
				int start = textBuffer.Lines[cpos.Line].GetSubLineStartCharIndex(subLine);
				int end = Math::Min(cpos.Col, textBuffer.Lines[cpos.Line].Chars.Count());
				docPosX = font->MeasureString(textBuffer.Lines[cpos.Line].GetDisplayLine(subLine, TabSpaces, end - start)).w;
			}
			return Vec2i::Create(docPosX, docPosY);
		}
		CaretPos ScreenPosToCaretPos(int x, int y)
		{
			int docX = x + hScroll->GetPosition() - LeftIndent;
			int physicalLine = y / lineHeight + vScroll->GetPosition();
			if (physicalLine < 0)
				physicalLine = 0;
			int logicalLine = 0;
			int counter = physicalLine;

			while (logicalLine < textBuffer.Lines.Count() && counter >= textBuffer.Lines[logicalLine].DisplayLines)
			{
				counter -= textBuffer.Lines[logicalLine].DisplayLines;
				logicalLine++;
			}
			if (logicalLine >= textBuffer.Lines.Count())
			{
				return CaretPos(textBuffer.Lines.Count() - 1, textBuffer.Lines.Last().Chars.Count());
			}
			auto & line = textBuffer.Lines[logicalLine];
			int start = line.GetSubLineStartCharIndex(counter);
			int col = line.Chars.Count();
			auto str = line.GetDisplayLine(counter, TabSpaces);
			int cx = 0;
			for (int i = start; i < line.Chars.Count(); i++)
			{
				int chWidth = GetCharWidth(line.Chars[i]);
				int nx = font->MeasureString(line.GetDisplayLine(counter, TabSpaces, i - start + 1)).w;
				if (nx > docX)
				{
					if ((docX - cx) <= (chWidth >> 1))
						col = i;
					else
						col = i + 1;
					break;
				}
				cx = nx;
			}
			return CaretPos(logicalLine, col);
		}
		void CreateLineLabels(int lines)
		{
			screen.Clear();
			for (auto & lbl : content->GetChildren())
				lbl = nullptr;
			content->GetChildren().Clear();
			for (int i = 0; i < lines; i++)
			{
				auto lbl = new Label(content);
				ScreenLine sl;
				sl.label = lbl;
				sl.LogicalLine = 0;
				sl.LogicalCol = 0;
				lbl->Posit(LeftIndent, i * lineHeight, content->GetWidth(), lineHeight);
				lbl->Enabled = false;
				screen.Add(sl);
			}
		}
	public:
		int GetCharWidth(unsigned int ch)
		{
			int rs = 0;
			if (!textWidthCache.TryGetValue(ch, rs))
			{
				if (ch == L'\t')
					rs = font->MeasureString(L" ").w * TabSpaces;
				else
					rs = font->MeasureString(String((wchar_t)ch)).w;
				textWidthCache[ch] = rs;
			}
			return rs;
		}
	public:
		int LeftIndent = 4;
	private:
		void RefreshScreen()
		{
			for (int i = 0; i < screen.Count(); i++)
			{
				int logicalLine = 0, logicalCol = 0;
				auto lineTxt = textBuffer.GetDisplayLine(vScroll->GetPosition() + i, wordWrap, TabSpaces, logicalLine, logicalCol);
				int start = 0;
				int pos = hScroll->GetPosition();
				int totalWidth = 0;
				while (start < lineTxt.Length() && pos > 0)
				{
					int chWidth = GetCharWidth(lineTxt[start]);
					totalWidth += chWidth;
					pos -= chWidth;
					start++;
				}
				int offset = 0;
				if (start > 0 && pos < 0)
				{
					start--;
					offset = -(GetCharWidth(lineTxt[start]) + pos);
				}
				int txtWidth = 0;
				int end = start;
				while (end < lineTxt.Length() && txtWidth < Width)
				{
					int chWidth = GetCharWidth(lineTxt[end]);
					totalWidth += chWidth;
					txtWidth += chWidth;
					end++;
				}
				for (int j = end; j < Math::Min(end + 100, lineTxt.Length()); j++)
					totalWidth += GetCharWidth(lineTxt[j]);
				if (totalWidth > maxLineWidth)
				{
					maxLineWidth = totalWidth;
				}
				screen[i].LogicalLine = logicalLine;
				screen[i].LogicalCol = logicalCol;
				screen[i].label->SetFont(font);
				screen[i].label->SetText(lineTxt.SubString(start, end - start));
				screen[i].label->Left = LeftIndent + offset;
			}
			int newHmax = Math::Max(0, maxLineWidth - content->GetWidth() + LeftIndent);
			hScroll->SetValue(0, newHmax,
				Math::Clamp(hScroll->GetPosition(), 0, newHmax), content->GetWidth() - LeftIndent);
		}
		void VScroll_Changed(UI_Base *)
		{
			invalidateScreen = true;
		}
		void HScroll_Changed(UI_Base *)
		{
			invalidateScreen = true;
		}
		void UpdateWordWrap()
		{
			if (wordWrap)
			{
				for (auto & line : textBuffer.Lines)
					line.WordWrap(content->GetWidth() - LeftIndent, *this);
			}
			else
			{
				for (auto & line : textBuffer.Lines)
					line.ClearWordWrap();
			}
		}
		void UpdateScrollBars()
		{
			int lineCount;
			if (wordWrap)
				lineCount = textBuffer.GetDisplayLineCount();
			else
				lineCount = textBuffer.Lines.Count();
			vScroll->SetValue(0, lineCount - 1,
				Math::Clamp(vScroll->GetPosition(), 0, lineCount - 1), content->GetHeight() / lineHeight);
			vScroll->LargeChange = vScroll->GetPageSize();
		}
	public:
		MultiLineTextBoxImpl(Container * parent)
			: MultiLineTextBox(parent)
		{
			Type = CT_MULTILINETEXTBOX;
			content = new Container(this);
			content->BorderStyle = BS_NONE;
			vScroll = new ScrollBar(this);
			vScroll->SetOrientation(SO_VERTICAL);
			hScroll = new ScrollBar(this);
			hScroll->SetOrientation(SO_HORIZONTAL);
			vScroll->SetWidth(Global::SCROLLBAR_BUTTON_SIZE);
			hScroll->SetHeight(Global::SCROLLBAR_BUTTON_SIZE);
			vScroll->OnChanged.Bind(this, &MultiLineTextBoxImpl::VScroll_Changed);
			hScroll->OnChanged.Bind(this, &MultiLineTextBoxImpl::HScroll_Changed);

			content->AcceptsFocus = false;
			vScroll->AcceptsFocus = false;
			hScroll->AcceptsFocus = false;
			content->TabStop = false;
			vScroll->TabStop = false;
			hScroll->TabStop = false;
			content->Enabled = false;
			WantsTab = true;
			content->BackColor = Global::Colors.EditableAreaBackColor;
			AcceptsFocus = TabStop = true;
			SetWordWrap(true);
			ResetCaretTimer();
			Cursor = CursorType::IBeam;
			SetFont(parent->GetFont());
			BorderStyle = BS_FLAT_;

			contextMenu = new Menu(this);
			auto mnUndo = new MenuItem(contextMenu, L"&Undo", L"Ctrl+Z");
			mnUndo->OnClick.Bind([this](auto) {Undo(); });
			auto mnRedo = new MenuItem(contextMenu, L"&Redo", L"Ctrl+Y");
			mnRedo->OnClick.Bind([this](auto) {Redo(); });
			new MenuItem(contextMenu);
			auto mnCut = new MenuItem(contextMenu, L"C&ut", L"Ctrl+X");
			mnCut->OnClick.Bind([this](auto) {Cut(); });
			auto mnCopy = new MenuItem(contextMenu, L"&Copy", L"Ctrl+C");
			mnCopy->OnClick.Bind([this](auto) {Copy(); });
			auto mnPaste = new MenuItem(contextMenu, L"&Paste", L"Ctrl+V");
			mnPaste->OnClick.Bind([this](auto) {Paste(); });
			auto mnSelAll = new MenuItem(contextMenu, L"&Select All", L"Ctrl+A");
			mnSelAll->OnClick.Bind([this](auto) {SelectAll(); });
		}
		virtual void DoDpiChanged() override
		{
			textWidthCache.Clear();
			if (font)
			{
				int fontLineHeight = this->font->MeasureString(L"X").h;
				lineHeight = (int)(fontLineHeight * 1.1f);
				CreateLineLabels(Height / lineHeight + 1);
			}
			hScroll->SmallChange = lineHeight;
			hScroll->LargeChange = lineHeight * 50;
			invalidateScreen = true;
			Container::DoDpiChanged();
		}
		virtual void SetReadOnly(bool value) override
		{
			readOnly = value;
		}
		virtual bool GetReadOnly() override
		{
			return readOnly;
		}
		void ResetCaretTimer()
		{
			time = CoreLib::Diagnostics::PerformanceCounter::Start();
		}
		virtual CaretPos GetCaretPos() override
		{
			return caretPos;
		}
		virtual void SetCaretPos(const CaretPos & pCaretPos) override
		{
			this->caretPos = pCaretPos;
			caretPosChanged = true;
			OnCaretPosChanged(this);
			ScrollToCaret();
		}
		virtual void MoveCaretToEnd() override
		{
			SetCaretPos(CaretPos(textBuffer.Lines.Count() - 1, textBuffer.Lines.Last().Chars.Count()));
			selStart = selEnd = caretPos;
			SelectionChanged();
		}
		virtual void ScrollToCaret() override
		{
			auto physicalPos = textBuffer.LogicalToPhysical(caretPos);
			if (physicalPos.Line < vScroll->GetPosition())
				vScroll->SetPosition(physicalPos.Line);
			else if (physicalPos.Line >= vScroll->GetPosition() + vScroll->GetPageSize())
				vScroll->SetPosition(physicalPos.Line - vScroll->GetPageSize() + 1);
			auto docPos = CaretPosToDocumentPos(caretPos);
			if (docPos.x < hScroll->GetPosition())
				hScroll->SetPosition(Math::Max(0, docPos.x - 100));
			else if (docPos.x > hScroll->GetPosition() + content->GetWidth() - LeftIndent)
			{
				hScroll->SetMax(Math::Max(hScroll->GetMax(), docPos.x - content->GetWidth() + LeftIndent));
				hScroll->SetPosition(Math::Max(docPos.x - content->GetWidth() + LeftIndent, 0));
			}
		}
		CaretPos FindPrevSeparator(CaretPos pos)
		{
			while (pos.Col > 0)
			{
				if (IsPunctuation(textBuffer.Lines[pos.Line].Chars[pos.Col - 1]))
					break;
				pos.Col--;
			}
			return pos;
		}
		CaretPos FindNextSeparator(CaretPos pos)
		{
			while (pos.Col < textBuffer.Lines[pos.Line].Chars.Count())
			{
				if (IsPunctuation(textBuffer.Lines[pos.Line].Chars[pos.Col]))
					break;
				pos.Col++;
			}
			return pos;
		}
	public:
		void UpdateWordSelection()
		{
			selStart = wordSelStart;
			if (selEnd < selStart)
			{
				selEnd = FindPrevSeparator(selEnd);
				selStart = FindNextSeparator(selStart);
			}
			else
			{
				selStart = FindPrevSeparator(wordSelStart);
				selEnd = FindNextSeparator(selEnd);
				if (selStart == selEnd && selStart.Col > 0)
					selStart.Col--;
			}
		}
		virtual bool DoMouseMove(int x, int y) override
		{
			if (!Enabled || !Visible)
				return false;
			Container::DoMouseMove(x, y);
			
			if (selecting)
			{
				selEnd = ScreenPosToCaretPos(x, y);
				if (wordSelecting)
					UpdateWordSelection();
				SetCaretPos(selEnd);
				desiredCol = textBuffer.LogicalToPhysical(caretPos).Col;
				SelectionChanged();
			}
			return true;
		}

		virtual bool DoDblClick() override
		{
			selecting = wordSelecting = true;
			wordSelStart = selStart;
			UpdateWordSelection();
			SetCaretPos(selEnd);
			desiredCol = textBuffer.LogicalToPhysical(caretPos).Col;
			SelectionChanged();
			Global::MouseCaptureControl = this;
			return true;
		}

		virtual bool DoMouseDown(int x, int y, SHIFTSTATE shift) override
		{
			if (!Enabled || !Visible)
				return false;
			Container::DoMouseDown(x, y, shift);
			SetFocus();
			auto newCaretPos = ScreenPosToCaretPos(x, y);
			if (shift == SS_BUTTONLEFT)
			{
				caretPos = selStart = selEnd = newCaretPos;
				selecting = true;
				Global::MouseCaptureControl = this;
				SelectionChanged();
			}
			else
			{
				CaretPos sel0, sel1;
				if (selStart < selEnd)
				{
					sel0 = selStart;
					sel1 = selEnd;
				}
				else
				{
					sel0 = selEnd;
					sel1 = selStart;
				}
				if (newCaretPos < sel0 || sel1 < newCaretPos)
				{
					caretPos = selStart = selEnd = newCaretPos;
					SelectionChanged();
				}
			}
			desiredCol = textBuffer.LogicalToPhysical(caretPos).Col;
			caretPosChanged = true;
			OnCaretPosChanged(this);
			ResetCaretTimer();
			return true;
		}
		virtual bool DoMouseUp(int x, int y, SHIFTSTATE shift) override
		{
			if (!Enabled || !Visible)
				return false;
			Container::DoMouseUp(x, y, shift);
			selecting = false;
			wordSelecting = false;
			ReleaseMouse();
			if (shift == SS_BUTTONRIGHT)
				contextMenu->Popup(x, y);
			return true;
		}

		bool IsNavigationKey(unsigned short key)
		{
			return (key == Keys::Left || key == Keys::Right || key == Keys::Down || key == Keys::Up
				|| key == Keys::PageDown || key == Keys::PageUp || key == Keys::Home || key == Keys::End);
		}

		void IncreaseLineIndent(CaretPos start, CaretPos end)
		{
			if (end < start)
				Swap(start, end);
			int lineBegin = Math::Max(0, start.Line);
			int lineEnd = Math::Min(textBuffer.Lines.Count() - 1, end.Line);
			start.Col = 0;
			end.Col = textBuffer.Lines[end.Line].Chars.Count();
			String oldText = GetTextFromRange(start, end);
			for (int i = lineBegin; i <= lineEnd; i++)
			{
				textBuffer.Lines[i].Chars.Insert(0, L'\t');
				if (wordWrap)
					textBuffer.Lines[i].WordWrap(content->GetWidth() - LeftIndent, *this);
			}
			TextModified();
			selStart = CaretPos(lineBegin, 0);
			selEnd = CaretPos(lineEnd, textBuffer.Lines[lineEnd].Chars.Count());
			SelectionChanged();

			Operation op;
			op.Name = OperationName::ReplaceText;
			op.Pos = start;
			op.SelStart = start; 
			op.SelEnd = end;
			op.Text = oldText;
			end.Col = textBuffer.Lines[end.Line].Chars.Count();
			op.SelEnd1 = end;
			op.Text1 = GetTextFromRange(start, end);
			operationStack.PushOperation(op);
		}

		void DecreaseLineIndent(CaretPos start, CaretPos end)
		{
			if (end < start)
				Swap(start, end);
			int lineBegin = Math::Max(0, start.Line);
			int lineEnd = Math::Min(textBuffer.Lines.Count() - 1, end.Line);
			start.Col = 0;
			end.Col = textBuffer.Lines[end.Line].Chars.Count();
			String oldText = GetTextFromRange(start, end);

			for (int i = lineBegin; i <= lineEnd; i++)
			{
				if (textBuffer.Lines[i].Chars.Count())
				{
					if (textBuffer.Lines[i].Chars.First() == L'\t')
						textBuffer.Lines[i].Chars.RemoveAt(0);
					else
					{
						for (int j = 0; j < TabSpaces; j++)
						{
							if (textBuffer.Lines[i].Chars.First() == L' ')
								textBuffer.Lines[i].Chars.RemoveAt(0);
							else
								break;
						}
					}
					if (wordWrap)
						textBuffer.Lines[i].WordWrap(content->GetWidth() - LeftIndent, *this);
				}
			}
			TextModified();
			selStart = CaretPos(lineBegin, 0);
			selEnd = CaretPos(lineEnd, textBuffer.Lines[lineEnd].Chars.Count());
			SelectionChanged();

			Operation op;
			op.Name = OperationName::ReplaceText;
			op.Pos = start;
			op.SelStart = start;
			op.SelEnd = end;
			op.Text = oldText;
			end.Col = textBuffer.Lines[end.Line].Chars.Count();
			op.SelEnd1 = end;
			op.Text1 = GetTextFromRange(start, end);
			operationStack.PushOperation(op);
		}

		bool DoKeyPress(unsigned short key, SHIFTSTATE shift) override
		{
			if (!Enabled || !Visible)
				return false;
			Control::DoKeyPress(key, shift);
			if ((shift & SS_CONTROL) == 0)
			{
				if (readOnly)
					return true;
				if (key >= Keys::Space)
				{
					InsertText((wchar_t)key);
				}
				else if (key == Keys::Return)
				{
					InsertText((wchar_t)key);
					if (caretPos.Line > 0)
					{
						StringBuilder spacesStr;
						for (int i = 0; i < textBuffer.Lines[caretPos.Line - 1].Chars.Count(); i++)
						{
							auto ch = textBuffer.Lines[caretPos.Line - 1].Chars[i];
							if (ch == L'\t' || ch == L' ')
								spacesStr << (wchar_t)ch;
							else
								break;
						}
						if (spacesStr.Length())
							InsertText(spacesStr.ProduceString());
					}
				}
				else if (key == Keys::Tab)
				{
					if ((shift & SS_SHIFT) == 0)
					{
						if (selEnd.Line != selStart.Line)
							IncreaseLineIndent(selStart, selEnd);
						else
							InsertText(L'\t');
					}
					else
					{
						DecreaseLineIndent(selStart, selEnd);
					}
					
				}
			}
			return true;
		}

		virtual bool DoKeyDown(unsigned short key, SHIFTSTATE shift) override
		{
			if (!Visible || !Enabled)
				return false;
			Control::DoKeyDown(key, shift);
			auto nPos = caretPos;
			if (IsNavigationKey(key))
			{
				int currentSubLine = textBuffer.Lines[nPos.Line].GetSubLineIndex(nPos.Col);
				switch (key)
				{
				case Keys::Left:
					nPos.Col--;
					if (nPos.Col < 0)
					{
						nPos.Line--;
						if (nPos.Line < 0)
							nPos.Line = nPos.Col = 0;
						else
							nPos.Col = textBuffer.Lines[nPos.Line].Chars.Count();
					}
					desiredCol = textBuffer.LogicalToPhysical(CaretPos(nPos)).Col;
					break;
				case Keys::Right:
					nPos.Col++;
					if (nPos.Col > textBuffer.Lines[nPos.Line].Chars.Count())
					{
						nPos.Line++;
						if (nPos.Line >= textBuffer.Lines.Count())
						{
							nPos.Line = textBuffer.Lines.Count() - 1;
							nPos.Col = textBuffer.Lines.Last().Chars.Count();
						}
						else
							nPos.Col = 0;
					}
					desiredCol = textBuffer.LogicalToPhysical(CaretPos(nPos)).Col;
					break;
				case Keys::Up:
				{
					currentSubLine--;
					if (currentSubLine < 0)
					{
						nPos.Line--;
						if (nPos.Line < 0)
						{
							nPos.Line = 0;
							currentSubLine = 0;
						}
						else
							currentSubLine = textBuffer.Lines[nPos.Line].DisplayLines - 1;
					}
					int startCol = textBuffer.Lines[nPos.Line].GetSubLineStartCharIndex(currentSubLine);
					int endCol = textBuffer.Lines[nPos.Line].GetSubLineEndCharIndex(currentSubLine);
					nPos.Col = Math::Min(startCol + desiredCol, endCol);
					break;
				}
				case Keys::Down:
				{
					currentSubLine++;
					if (currentSubLine >= textBuffer.Lines[nPos.Line].DisplayLines)
					{
						nPos.Line++;
						if (nPos.Line >= textBuffer.Lines.Count())
						{
							nPos.Line = textBuffer.Lines.Count() - 1;
							currentSubLine = textBuffer.Lines.Last().DisplayLines - 1;
						}
						else
							currentSubLine = 0;
					}
					int startCol = textBuffer.Lines[nPos.Line].GetSubLineStartCharIndex(currentSubLine);
					int endCol = textBuffer.Lines[nPos.Line].GetSubLineEndCharIndex(currentSubLine);
					nPos.Col = Math::Min(startCol + desiredCol, endCol);
					break;
				}
				case Keys::Home:
				{
					int startCol = textBuffer.Lines[nPos.Line].GetSubLineStartCharIndex(currentSubLine);
					nPos.Col = startCol;
					desiredCol = 0;
					break;
				}
				case Keys::End:
				{
					int endCol = textBuffer.Lines[nPos.Line].GetSubLineEndCharIndex(currentSubLine);
					nPos.Col = endCol;
					desiredCol = endCol - textBuffer.Lines[nPos.Line].GetSubLineStartCharIndex(currentSubLine);
					break;
				}
				case Keys::PageDown:
				{
					auto physical = textBuffer.LogicalToPhysical(caretPos);
					physical.Line += vScroll->GetPageSize();
					physical.Col = desiredCol;
					int subLine;
					nPos = textBuffer.PhysicalToLogical(physical, wordWrap, subLine);
					break;
				}
				case Keys::PageUp:
				{
					auto physical = textBuffer.LogicalToPhysical(caretPos);
					physical.Line -= vScroll->GetPageSize();
					physical.Col = desiredCol;
					int subLine;
					nPos = textBuffer.PhysicalToLogical(physical, wordWrap, subLine);
					break;
				}
				}
				if (shift == SS_SHIFT)
				{
					selEnd = nPos;
					SetCaretPos(selEnd);
					SelectionChanged();
				}
				else
				{
					selStart = selEnd = nPos;
					SetCaretPos(nPos);
					caretPosChanged = true;
				}
			}
			else if (shift == SS_CONTROL)
			{
				if (key == 'A') // select all
					SelectAll();
				else if (key == 'C') // copy
					Copy();
				else if (key == 'X') // cut
					Cut();
				else if (key == 'V') // paste
					Paste();
				else if (key == 'Z') // undo
					Undo();
				else if (key == 'Y') // redo
					Redo();
			}
			else if (shift == SS_SHIFT)
			{
				if (key == Keys::Delete)
					Cut();
			}
			else
			{
				if (readOnly)
					return true;
				if (key == Keys::Delete)
				{
					DeleteAfterCaret();
					TextModified();
				}
				else if (key == Keys::Backspace)
				{
					DeleteBeforeCaret();
					TextModified();
				}
			}
			ResetCaretTimer();
			return true;
		}

		void Cut() override
		{
			if (readOnly)
				return;
			String text;
			if (selStart != selEnd)
			{
				text = GetSelectionText();
				DeleteSelection();
			}
			else
			{
				text = textBuffer.GetLine(caretPos.Line);
				auto start = caretPos;
				auto end = caretPos;
				start.Col = 0;
				end.Line++;	end.Col = 0;
				Delete(start, end);
			}
			GetEntry()->System->SetClipboardText(text);
			TextModified();
		}

		void Copy() override
		{
			String text = GetSelectionText();
			if (text.Length() == 0)
				text = textBuffer.GetLine(caretPos.Line);
			GetEntry()->System->SetClipboardText(text);
		}

		void Paste() override
		{
			if (readOnly)
				return;
			auto text = GetEntry()->System->GetClipboardText();
			InsertText(text);
		}

		virtual void SelectAll() override
		{
			selStart.Col = 0;
			selStart.Line = 0;
			selEnd.Line = textBuffer.Lines.Count() - 1;
			selEnd.Col = textBuffer.Lines.Last().Chars.Count();
			caretPos = selEnd;
			SelectionChanged();
			ScrollToCaret();
		}

		virtual void Undo() override
		{
			if (readOnly)
				return;
			auto op = operationStack.PopOperation();
			operationStack.Lock();
			switch (op.Name)
			{
			case OperationName::InsertText:
				Delete(op.SelStart, op.SelEnd);
				caretPos = selStart = selEnd = op.Pos;
				TextModified();
				break;
			case OperationName::DeleteText:
				selStart = selEnd = caretPos = op.Pos;
				InsertText(op.Text);
				break;
			case OperationName::ReplaceText:
				selStart = op.SelStart;
				selEnd = op.SelEnd1;
				InsertText(op.Text);
				break;
			case OperationName::None:
				break;
			}
			operationStack.Unlock();
		}

		virtual void Redo() override
		{
			if (readOnly)
				return;
			auto op = operationStack.GetNextRedo();
			operationStack.Lock();
			switch (op.Name)
			{
			case OperationName::DeleteText:
				Delete(op.SelStart, op.SelEnd);
				caretPos = selStart = selEnd = op.Pos;
				TextModified();
				break;
			case OperationName::InsertText:
				selStart = selEnd = caretPos = op.Pos;
				InsertText(op.Text);
				break;
			case OperationName::ReplaceText:
				selStart = op.SelStart;
				selEnd = op.SelEnd;
				InsertText(op.Text1);
				break;
			case OperationName::None:
				break;
			}
			operationStack.Unlock();
		}

		virtual void SetWordWrap(bool pValue) override
		{
			wordWrap = pValue;
			UpdateWordWrap();
			hScroll->Visible = !pValue;
			SizeChanged();
			invalidateScreen = true;
		}

		virtual bool GetWordWrap() override
		{
			return wordWrap;
		}

		void Delete(CaretPos start, CaretPos end)
		{
			Operation op;
			op.SelStart = start;
			op.SelEnd = end;
			op.Pos = caretPos;
			op.Name = OperationName::DeleteText;
			op.Text = GetTextFromRange(start, end);
			operationStack.PushOperation(op);

			selecting = wordSelecting = false;
			auto & lineStart = textBuffer.Lines[start.Line];
			auto lineEnd = textBuffer.Lines[end.Line];
			if (start.Col < lineStart.Chars.Count())
				lineStart.Chars.SetSize(start.Col);
			else
				start.Col = lineStart.Chars.Count();

			if (end.Col < lineEnd.Chars.Count())
				lineStart.Chars.AddRange(lineEnd.Chars.Buffer() + end.Col, lineEnd.Chars.Count() - end.Col);
			if (end.Line > start.Line)
				textBuffer.Lines.RemoveRange(start.Line + 1, end.Line - start.Line);
			if (wordWrap)
				lineStart.WordWrap(content->GetWidth() - LeftIndent, *this);
		}

		void DeleteSelection()
		{
			if (selEnd < selStart)
				Swap(selStart, selEnd);
			selStart.Line = Math::Clamp(selStart.Line, 0, textBuffer.Lines.Count() - 1);
			selEnd.Line = Math::Clamp(selEnd.Line, 0, textBuffer.Lines.Count() - 1);
			Delete(selStart, selEnd);
			caretPos = selStart;
			selEnd = selStart;
		}

		void DeleteBeforeCaret()
		{
			if (selStart != selEnd)
				DeleteSelection();
			else
			{
				CaretPos prior = caretPos;
				prior.Col--;
				if (prior.Col < 0)
				{
					prior.Line--;
					if (prior.Line < 0)
						return;
					prior.Col = textBuffer.Lines[prior.Line].Chars.Count();
				}
				Delete(prior, caretPos);
				caretPos = prior;
			}
		}

		void DeleteAfterCaret()
		{
			if (selStart != selEnd)
				DeleteSelection();
			else
			{
				CaretPos after = caretPos;
				after.Col++;
				if (after.Col > textBuffer.Lines[after.Line].Chars.Count())
				{
					after.Line++;
					after.Col = 0;
					if (after.Line >= textBuffer.Lines.Count())
						return;
				}
				Delete(caretPos, after);
			}
		}

		void TextModified()
		{
			selStart = selEnd = caretPos;
			invalidateScreen = true;
			UpdateScrollBars();
			SelectionChanged();
			ScrollToCaret();
			auto physical = textBuffer.LogicalToPhysical(caretPos);
			desiredCol = physical.Col;
		}

		virtual void Delete() override
		{
			DeleteSelection();
			TextModified();
		}

		virtual void IncreaseIndent() override
		{
			if (readOnly)
				return;
			IncreaseLineIndent(selStart, selEnd);
		}

		virtual void DecreaseIndent() override
		{
			if (readOnly)
				return;
			DecreaseLineIndent(selStart, selEnd);
		}

		virtual void Select(CaretPos start, CaretPos end) override
		{
			caretPos = end;
			selStart = start;
			selEnd = end;
			SelectionChanged();
		}

		virtual void InsertText(const String & text) override
		{
			selecting = wordSelecting = false;
			if (selStart != selEnd)
				DeleteSelection();

			Operation op;
			op.Name = OperationName::InsertText;
			op.Text = text;
			op.Pos = caretPos;
			op.SelStart = caretPos;

			caretPos.Line = Math::Clamp(caretPos.Line, 0, textBuffer.Lines.Count() - 1);
			TextBuffer nText;
			nText.SetText(text);
			List<unsigned int> lastHalf;
			auto & line = textBuffer.Lines[caretPos.Line];
			caretPos.Col = Math::Clamp(caretPos.Col, 0, line.Chars.Count());
			for (int i = caretPos.Col; i < line.Chars.Count(); i++)
				lastHalf.Add(line.Chars[i]);
			nText.Lines.Last().Chars.AddRange(lastHalf);
			line.Chars.SetSize(caretPos.Col);
			line.Chars.AddRange(nText.Lines.First().Chars);
			if (wordWrap)
			{
				line.WordWrap(content->GetWidth() - LeftIndent, *this);
				for (int i = 1; i < nText.Lines.Count(); i++)
					nText.Lines[i].WordWrap(content->GetWidth() - LeftIndent, *this);
			}
			if (nText.Lines.Count() > 1)
			{
				textBuffer.Lines.InsertRange(caretPos.Line + 1, nText.Lines.Buffer() + 1, nText.Lines.Count() - 1);
				caretPos.Col = nText.Lines.Last().Chars.Count() - lastHalf.Count();
			}
			else
				caretPos.Col = line.Chars.Count() - lastHalf.Count();
			caretPos.Line += nText.Lines.Count() - 1;

			op.SelEnd = caretPos;
			operationStack.PushOperation(op);
			TextModified();
		}

		virtual String GetText() override
		{
			return textBuffer.GetAllText();
		}
		virtual VectorMath::Vec2i GetCaretScreenPos() override
		{
			int x = caretDocumentPosX + LeftIndent - hScroll->GetPosition();
			int y = caretDocumentPosY - vScroll->GetPosition() * lineHeight + lineHeight;
			int absX, absY;
			LocalPosToAbsolutePos(x, y, absX, absY);
			return Vec2i::Create(absX, absY);
		}
		virtual void SetText(const CoreLib::String & pText) override
		{
			operationStack.Clear();
			textBuffer.SetText(pText);
			caretPos = selStart = selEnd = CaretPos(0, 0);
			UpdateWordWrap();
			hScroll->SetValue(0, 0, 0, content->GetWidth() - LeftIndent);
			UpdateScrollBars();
			invalidateScreen = true;
		}
		virtual void SetFont(IFont * pFont) override
		{
			Control::SetFont(pFont);
			DoDpiChanged();
		}
		virtual bool DoMouseWheel(int delta) override
		{
			if (Visible && Enabled && vScroll->Visible)
			{
				vScroll->SetPosition(Math::Clamp(vScroll->GetPosition() + (delta > 0 ? -1 : 1) * 3, 0, vScroll->GetMax()));
				return true;
			}
			return false;
		}
		virtual void SizeChanged() override
		{
			int hScrollHeight = (hScroll->Visible ? hScroll->GetHeight() : 0);
			int vScrollWidth = (vScroll->Visible ? vScroll->GetWidth() : 0);
			content->Posit(0, 0, Width - vScrollWidth, Height - hScrollHeight);
			UpdateWordWrap();
			
			vScroll->Left = Width - vScroll->GetWidth();
			hScroll->Top = Height - hScroll->GetHeight();
			vScroll->SetHeight(Height - hScrollHeight);
			hScroll->SetWidth(Width - vScrollWidth + 2);
			UpdateScrollBars();
			CreateLineLabels(Height / lineHeight + 1);
			invalidateScreen = true;
			SelectionChanged();
		}
		virtual void SetScrollBars(bool vertical, bool horizontal) override
		{
			vScroll->Visible = vertical;
			hScroll->Visible = horizontal;
		}
		virtual String GetTextFromRange(CaretPos start, CaretPos end) override
		{
			StringBuilder sb;
			while (start < end)
			{
				while (start.Col >= textBuffer.Lines[start.Line].Chars.Count())
				{
					start.Line++;
					start.Col = 0;
					if (start.Line >= textBuffer.Lines.Count())
						break;
					sb << L"\n";
				}
				if (start.Line < textBuffer.Lines.Count())
					sb << (wchar_t)textBuffer.Lines[start.Line].Chars[start.Col];
				else
					break;
				start.Col++;
			}
			return sb.ProduceString();
		}

		virtual String GetSelectionText() override
		{
			CaretPos sel0, sel1;
			if (selStart < selEnd)
			{
				sel0 = selStart;
				sel1 = selEnd;
			}
			else
			{
				sel0 = selEnd;
				sel1 = selStart;
			}
			sel0.Line = Math::Clamp(sel0.Line, 0, textBuffer.Lines.Count() - 1);
			sel1.Line = Math::Clamp(sel1.Line, 0, textBuffer.Lines.Count() - 1);
			return GetTextFromRange(sel0, sel1);
		}
		virtual void ImeInputString(const String & txt) override
		{
			if (readOnly)
				return;
			InsertText(txt);
		}
		virtual void Draw(int absX, int absY) override
		{
			Control::Draw(absX, absY);
			if (invalidateScreen)
			{
				invalidateScreen = false;
				RefreshScreen();
			}
			if (caretPosChanged)
			{
				caretPosChanged = false;
				UpdateCaretDocumentPos();
			}
			if (vScroll->Visible && hScroll->Visible)
			{
				GetEntry()->DrawCommands.SolidBrushColor = Global::Colors.ScrollBarBackColor;
				GetEntry()->DrawCommands.FillRectangle(absX + Left + Width - vScroll->GetWidth() + 1, 
					absY + Top + Height - hScroll->GetHeight() - 1, absX + Left + Width - 1, absY + Top + Height - 1);
			}
			auto entry = GetEntry();
			auto & graphics = entry->DrawCommands;
			entry->ClipRects->AddRect(GraphicsUI::Rect(absX + Left + LeftIndent, absY + Top + 1, Width - 1, Height - 1));
			Container::DrawChildren(absX + Left, absY + Top);
			if (selStart != selEnd)
			{
				for (int i = 0; i < screen.Count(); i++)
				{
					int physLine = i + vScroll->GetPosition();
					int highlightStart = 0, highlightEnd = 0;
					if (physLine == physicalSelStart.Line)
					{
						highlightStart = physicalSelStart.Col;
						highlightEnd = (physicalSelEnd.Line == physLine ? physicalSelEnd.Col : -1);
					}
					else if (physLine == physicalSelEnd.Line)
					{
						highlightStart = 0;
						highlightEnd = physicalSelEnd.Col;
					}
					else if (physLine > physicalSelStart.Line && physLine < physicalSelEnd.Line)
					{
						highlightStart = 0;
						highlightEnd = -1;
					}
					else
						continue;
					highlightStart += screen[i].LogicalCol;
					int quadStart = LeftIndent - hScroll->GetPosition();
					auto & lineBuffer = textBuffer.Lines[screen[i].LogicalLine];
					auto & chars = lineBuffer.Chars;
					for (int j = screen[i].LogicalCol; j < highlightStart; j++)
					{
						int chId = j;
						if (chId < chars.Count())
							quadStart += GetCharWidth(chars[chId]);
					}
					if (highlightEnd == -1)
						highlightEnd = lineBuffer.GetSubLineEndCharIndex(lineBuffer.GetSubLineIndex(screen[i].LogicalCol));
					else
						highlightEnd += screen[i].LogicalCol;
					int quadEnd = quadStart;
					int ptr = highlightStart;
					while (ptr < highlightEnd && ptr < chars.Count())
					{
						quadEnd += GetCharWidth(chars[ptr]);
						ptr++;
					}
					graphics.SolidBrushColor = IsFocused() ? Global::Colors.SelectionColor : Global::Colors.UnfocusedSelectionColor;
					int ox = absX + Left;
					quadStart += ox;
					quadEnd += ox;
					int oy = absY + Top;
					entry->ClipRects->AddRect(Rect(ox + 1, oy + 1, content->GetWidth() - 1, content->GetHeight() - 1));
					entry->ClipRects->AddRect(Rect(quadStart, oy + i * lineHeight, quadEnd - quadStart, lineHeight));
					graphics.FillRectangle(quadStart, oy + i * lineHeight, quadEnd, oy + i * lineHeight + lineHeight);
					screen[i].label->FontColor = Global::Colors.SelectionForeColor;
					screen[i].label->Draw(ox, oy);
					screen[i].label->FontColor = Global::Colors.ControlFontColor;
					entry->ClipRects->PopRect();
					entry->ClipRects->PopRect();
				}
			}
			if (IsFocused())
			{
				// draw caret
				float timePassed = CoreLib::Diagnostics::PerformanceCounter::EndSeconds(time);
				int tick = int(timePassed / CURSOR_FREQUENCY);
				if (IsFocused() && ((tick & 1) == 0))
				{
					auto screenPos = GetCaretScreenPos();
					int absCursorPosX = screenPos.x;
					int absCursorPosY = screenPos.y;
					graphics.PenColor = Color(255 - BackColor.R, 255 - BackColor.G, 255 - BackColor.B, 255);
					graphics.DrawLine(absCursorPosX, absCursorPosY - lineHeight, absCursorPosX, absCursorPosY);
				}
			}
			entry->ClipRects->PopRect();
		}
		virtual void SetUndoStackSize(int size) override
		{
			operationStack.SetSize(size);
		}
		virtual int GetLineCount() override
		{
			return textBuffer.Lines.Count();
		}
		virtual CoreLib::String GetLine(int i) override
		{
			return textBuffer.GetLine(i);
		}
		virtual void DeleteLine(int i) override
		{
			textBuffer.Lines.RemoveAt(i);
			TextModified();
		}
	};

	MultiLineTextBox * CreateMultiLineTextBox(Container * parent)
	{
		return new MultiLineTextBoxImpl(parent);
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
