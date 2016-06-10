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


#pragma comment(lib,"imm32.lib")

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam)	((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam)	((int)(short)HIWORD(lParam))
#endif

namespace GraphicsUI
{
	using namespace CoreLib;
	using namespace VectorMath;

	GraphicsUI::ColorTable Global::ColorTable = CreateDefaultColorTable();
	int Global::HoverTimeThreshold = 300;
	int Global::EventGUID = 0;
	int Global::CursorPosX = 0;
	int Global::CursorPosY = 0;
	int Global::SCROLLBAR_BUTTON_SIZE = 17;
	int Global::SCROLLBAR_MIN_PAGESIZE = 8;

	Control * Global::PointedComponent = nullptr;
	Control * Global::MouseCaptureControl = nullptr;

	int Graphics::DashPattern = -1;

	Color Graphics::PenColor, Graphics::SolidBrushColor;
	Color Graphics::GradiantBrushColor1, Graphics::GradiantBrushColor2;
	Color Graphics::GradiantBrushColor3, Graphics::GradiantBrushColor4;

	Control * lastFocusedCtrl = 0;

	void SwitchCursor(Cursor c)
	{
		LPTSTR cursorName;
		switch (c)
		{
		case Arrow:
			cursorName = IDC_ARROW;
			break;
		case IBeam:
			cursorName = IDC_IBEAM;
			break;
		case Cross:
			cursorName = IDC_CROSS;
			break;
		case Wait:
			cursorName = IDC_WAIT;
			break;
		case SizeAll:
			cursorName = IDC_SIZEALL;
			break;
		case SizeNS:
			cursorName = IDC_SIZENS;
			break;
		case SizeWE:
			cursorName = IDC_SIZEWE;
			break;
		case SizeNWSE:
			cursorName = IDC_SIZENWSE;
			break;
		case SizeNESW:
			cursorName = IDC_SIZENESW;
			break;
		default:
			cursorName = IDC_ARROW;
		}
		SetCursor(LoadCursor(0, cursorName));
	}

	void Graphics::DrawArc(ISystemInterface * sys, int x, int y, int rad, float theta, float theta2)
	{
		float lastX = x + rad*cos(theta);
		float lastY = y - rad*sin(theta);
		float deltaPhi = (theta2-theta)/rad;
		theta += deltaPhi;
		for (int i=1; i<rad+1; i++)
		{	
			float nx = x + rad*cos(theta);
			float ny = y - rad*sin(theta);
			sys->DrawLine(Pen(Graphics::PenColor), lastX, lastY, nx, ny);
			theta += deltaPhi;
			lastX = nx;
			lastY = ny;
		}
	}

	void Graphics::DrawRectangle(ISystemInterface * sys, int x1, int y1, int x2, int y2)
	{
		auto pen = Pen(Graphics::PenColor);
		sys->DrawLine(pen, (float)x1, (float)y1, (float)x2, (float)y1);
		sys->DrawLine(pen, (float)x2, (float)y1, (float)x2, (float)y2);
		sys->DrawLine(pen, (float)x2, (float)y2, (float)x1, (float)y2);
		sys->DrawLine(pen, (float)x1, (float)y2, (float)x1, (float)y1);
	}

	void Graphics::FillRectangle(ISystemInterface * sys, int x1, int y1, int x2, int y2)
	{
		sys->FillRectangle(SolidBrushColor, Rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1));
	}

	void Graphics::DrawRoundRect(ISystemInterface * sys, int x1, int y1, int x2, int y2, int rad)
	{
		auto pen = Pen(Graphics::PenColor);

		sys->DrawLine(pen, (float)x1 + rad, (float)y1, (float)x2 - rad, (float)y1);
		sys->DrawLine(pen, (float)x1 + rad, (float)y2, (float)x2 - rad, (float)y2);

		sys->DrawLine(pen, (float)x2, (float)y2 - rad, (float)x2, (float)y1 + rad);
		sys->DrawLine(pen, (float)x1, (float)y2 - rad, (float)x1, (float)y1 + rad);


		DrawArc(sys, x1+rad, y1+rad, rad, Math::Pi/2, Math::Pi);
		DrawArc(sys, x2-rad, y1+rad, rad, 0, Math::Pi /2);
		DrawArc(sys, x1+rad, y2-rad, rad, Math::Pi, Math::Pi *3/2);
		DrawArc(sys, x2-rad, y2-rad, rad, Math::Pi *3/2, Math::Pi *2);
	}

	void Graphics::FillRoundRect(ISystemInterface * sys, int x1, int y1, int x2, int y2, int rad)
	{
		Array<Vec2, 128> polygon;
		int edges = Math::Clamp(rad+3, 3, 30);
		float deltaPhi = Math::Pi/2/ edges;
		float theta = 0.0f;
		for (int i=0; i < edges +1; i++)
		{
			polygon.Add(Vec2::Create(x2-rad+rad*cos(theta), y1+rad-rad*sin(theta)));
			theta += deltaPhi;
		}
		theta = Math::Pi/2;
		for (int i = 0; i < edges + 1; i++)
		{
			polygon.Add(Vec2::Create(x1+rad+ rad*cos(theta), y1+rad- rad*sin(theta)));
			theta += deltaPhi;
		}
		theta = Math::Pi;
		for (int i = 0; i < edges + 1; i++)
		{
			polygon.Add(Vec2::Create(x1+rad+ rad*cos(theta), y2-rad - rad*sin(theta)));
			theta += deltaPhi;
		}
		theta = Math::Pi * 3 / 2;
		for (int i = 0; i < edges + 1; i++)
		{
			polygon.Add(Vec2::Create(x2-rad + rad*cos(theta), y2-rad-rad*sin(theta)));
			theta += deltaPhi;
		}
		sys->FillPolygon(SolidBrushColor, polygon.GetArrayView());
	}

	void Graphics::DrawLine(ISystemInterface * sys, int x1, int y1, int x2, int y2)
	{
		auto pen = Pen(Graphics::PenColor);
		sys->DrawLine(pen, (float)x1, (float)y1, (float)x2, (float)y2);
	}

	ColorTable CreateDarkColorTable()
	{
		ColorTable tbl;

		tbl.ShadowColor = Color(0, 0, 0, 200);
		tbl.ControlBackColor = Color(0, 0, 0, 0);
		tbl.ControlBorderColor = Color(220, 220, 220, 160);
		tbl.ControlFontColor = Color(255, 255, 255, 255);
		tbl.EditableAreaBackColor = Color(60, 60, 60, 140);
		tbl.ScrollBarBackColor = Color(70, 70, 70, 200);

		tbl.MemuIconBackColor = Color(127, 127, 127, 255);
		tbl.MenuBackColor = Color(80, 80, 80, 255);
		tbl.MenuBorderColor = Color(127, 127, 127, 255);
		tbl.MenuSeperatorColor = Color(130, 130, 130, 255);
		tbl.MenuItemForeColor = Color(255, 255, 255, 255);
		tbl.MenuItemDisabledForeColor = Color(180, 180, 180, 255);
		tbl.MenuItemHighlightForeColor = tbl.MenuItemForeColor;
		tbl.ToolButtonBackColor1 = tbl.ControlBackColor;
		tbl.ToolButtonBackColor2 = Color(215, 226, 228, 255);
		tbl.ToolButtonBackColorHighlight1 = Color(255, 250, 210, 255);
		tbl.ToolButtonBackColorHighlight2 = Color(253, 236, 168, 255);
		tbl.ToolButtonBackColorPressed1 = Color(249, 217, 132, 255);
		tbl.ToolButtonBackColorPressed2 = Color(252, 236, 194, 255);
		tbl.ToolButtonBorderHighLight = Color(254, 193, 92, 255);
		tbl.ToolButtonBorderSelected = Color(254, 193, 92, 255);
		tbl.ToolButtonSeperatorColor = Color(170, 170, 160, 255);
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

		tbl.SelectionColor = Color(244, 165, 0, 255);
		tbl.UnfocusedSelectionColor = Color(100, 100, 100, 127);
		tbl.HighlightColor = Color(100, 100, 100, 127);
		tbl.HighlightForeColor = Color(255, 255, 255, 255);
		tbl.SelectionForeColor = Color(255, 255, 255, 255);
		return tbl;
	}

	ColorTable CreateDefaultColorTable()
	{
		ColorTable tbl;
		tbl.ShadowColor = Color(0, 0, 0, 120);
		tbl.ControlBackColor = Color(235,238,241,255);
		tbl.ControlBorderColor = Color(211,232,254,255);
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

		tbl.TabPageBorderColor = Color(181, 201,241, 255);
		tbl.TabPageItemSelectedBackColor1 = Color(210,227,255, 255);
		tbl.TabPageItemSelectedBackColor2 = tbl.ControlBackColor;

		tbl.TabPageItemHighlightBackColor1 = Color(220,244,255, 255);
		tbl.TabPageItemHighlightBackColor2 = Color(220,244,255, 255);

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

	ClipRectStack::ClipRectStack(ISystemInterface * pSystem)
	{
		StackSize = 0;
		system = pSystem;
	}

	ClipRectStack::~ClipRectStack()
	{

	}

	void ClipRectStack::PushRect(Rect nRect)
	{
		Buffer[StackSize] = nRect;
		StackSize ++;
		system->SetClipRect(nRect);
	}

	Rect ClipRectStack::PopRect()
	{
		if (StackSize)
			StackSize--;
		if (StackSize)
		{
			system->SetClipRect(Buffer[StackSize - 1]);
			return Buffer[StackSize-1];
		}
		else
		{
			auto rect = Rect(0,0,WindowWidth,WindowHeight);
			system->SetClipRect(rect);
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
			cRect.x= max(nRect.x,Buffer[StackSize-1].x);
			cRect.y= max(nRect.y,Buffer[StackSize-1].y);
			cRect.w = min(nx1,nx2)-cRect.x;
			cRect.h = min(ny1,ny2)-cRect.y;
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
		Cursor = Arrow;
		Width = Height = Left = Top = 0;
		Name = "unnamed";
		Enabled = true;
		Visible = true;
		TopMost = false;
		Focused = false;
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
		GenerateMouseHoverEvent = false;
		BorderStyle = BS_RAISED;
		Type = CT_CONTROL;
		AbsolutePosX = AbsolutePosY = 0;
		BackColor = Global::ColorTable.ControlBackColor;
		BorderColor = Global::ColorTable.ControlBorderColor;
		tmrHover.Interval = Global::HoverTimeThreshold;
		tmrHover.OnTick.Bind(this, &Control::HoverTimerTick);
		DockStyle = dsNone;
	}

	Control::Control(Container * parent)
		: Control(parent, true)
	{
	}

	Control::~Control()
	{
		auto entry = GetEntry();
		if (entry && entry->FocusedControl == this)
			entry->FocusedControl = nullptr;
	}

	bool Control::DoClosePopup()
	{
		return false;
	}

	void Control::HoverTimerTick(Object *, CoreLib::WinForm::EventArgs e)
	{
		DoMouseHover();	
	}

	void Control::LocalPosToAbsolutePos(int x, int y, int & ax, int & ay)
	{
		ax = x + Left;
		ay = y + Top;
		auto parent = Parent;
		auto current = this;
		while (parent)
		{
			ax += parent->Left;
			ay += parent->Top;
			if (current->DockStyle == dsFill || current->DockStyle == dsNone)
			{
				ax += parent->ClientRect().x;
				ay += parent->ClientRect().y;
			}
			current = parent;
			parent = parent->Parent;
		}
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
			{
				OnMouseWheel.Invoke(this,*((UIMouseEventArgs*)Args->Data));
				return;
			}
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
			SwitchCursor(Cursor);
			UI_MsgArgs Args;UIMouseEventArgs Data;
			Args.Sender = this;	Args.Type = MSG_UI_MOUSEDOWN;
			Data.Shift = Shift;	Data.X = X;	Data.Y = Y;
			Args.Data = &Data;
			BroadcastMessage(&Args);
				
			if (TabStop && Parent)
				SetFocus();
		}
		tmrHover.StopTimer();
		return false;
	}

	bool Control::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
			
		if (IsPointInClient(X,Y))
		{
				
			if (Shift & SS_BUTTONLEFT)
				DoClick();
		}
		UI_MsgArgs Args;UIMouseEventArgs Data;
		Args.Sender = this;	Args.Type = MSG_UI_MOUSEUP;
		Data.Shift = Shift;	Data.X = X;	Data.Y = Y;
		Args.Data = &Data;
		BroadcastMessage(&Args);
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
		SwitchCursor(Cursor);
		Args.Type = MSG_UI_MOUSEMOVE;
		if (GenerateMouseHoverEvent)
			tmrHover.StartTimer();
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
		SwitchCursor(Arrow);
		if (GenerateMouseHoverEvent)
			tmrHover.StopTimer();
		return false;
	}

	bool Control::DoMouseHover()
	{
		OnMouseHover.Invoke(this);
		tmrHover.StopTimer();
		return false;
	}

	bool Control::DoKeyDown(unsigned short Key, SHIFTSTATE Shift) 
	{
		if (!Enabled || !Visible || !ContainsFocus())
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
		if (!Enabled || !Visible || !ContainsFocus())
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
		if (!Enabled || !Visible || !ContainsFocus())
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
		Focused = false;
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
				entry->System->DrawRectangleShadow(Global::ColorTable.ShadowColor, (float)absX, (float)absY, (float)Width, (float)Height, (float)ShadowOffset, (float)ShadowOffset, ShadowSize);
			}
			clipRects->PushRect(R);
		}
		//Draw Background
		auto sys = GetEntry()->System;
		if (BackColor.A)
		{
			Graphics::SolidBrushColor = BackColor;
			Graphics::FillRectangle(sys, absX, absY, absX + Width - 1, absY + Height - 1);
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
			Graphics::PenColor = LightColor;
			Graphics::DrawLine(sys, absX, absY, absX + Width - 1, absY);
			Graphics::DrawLine(sys, absX, absY, absX, absY + Height - 1);

			Graphics::PenColor = DarkColor;
			Graphics::DrawLine(sys, absX + Width - 1, absY, absX + Width - 1, absY + Height - 1);
			Graphics::DrawLine(sys, absX + Width - 1, absY + Height - 1, absX, absY + Height - 1);
		}
		else if (BorderStyle == BS_LOWERED)
		{
			Graphics::PenColor = DarkColor;
			Graphics::DrawLine(sys, absX, absY, absX + Width - 1, absY);
			Graphics::DrawLine(sys, absX, absY, absX, absY + Height - 1);

			Graphics::PenColor = LightColor;
			Graphics::DrawLine(sys, absX + Width - 1, absY, absX + Width - 1, absY + Height - 1);
			Graphics::DrawLine(sys, absX + Width - 1, absY + Height - 1, absX, absY + Height - 1);
		}
		else if (BorderStyle == BS_FLAT_)
		{
			Graphics::PenColor = BorderColor;
			Graphics::DrawRectangle(sys, absX, absY, absX + Width - 1, absY + Height - 1);
		}
	}

	void Control::SetFont(IFont * AFont)
	{
		this->font = AFont;
	}

	void Control::KillFocus()
	{
		if (this->Focused)
			OnLostFocus(this);
		if (GetEntry()->FocusedControl == this)
			GetEntry()->FocusedControl = nullptr;
		this->Focused = false;
	}

	void Control::SetFocus()
	{			
		if (TabStop)
		{
			GetEntry()->SetFocusedControl(this);		
		}
	}

	bool Control::ContainsFocus()
	{
		return Focused;
	}

	Label::Label(Container * parent)
		: Container(parent)
	{
		BorderStyle = BS_NONE;
		BackColor.A = 0;
		BackColor.R = 255;  BackColor.G =255; BackColor.B = 255;
		FontColor = Global::ColorTable.ControlFontColor;
		FChanged = true;
		Type = CT_LABEL;
		AutoSize = true;
		MultiLine = false;
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
		FCaption = pText;
		FChanged = true;
		UpdateText();

	}

	void Label::SetFont(IFont * pFont)
	{
		Control::SetFont(pFont);
		FChanged = true;
	}

	void Label::SizeChanged()
	{
	}

	void Label::UpdateText()
	{
		if (text)
			text = nullptr;
		if (MultiLine)
			text = font->BakeString(FCaption, Width);
		else
			text = font->BakeString(FCaption, 0);
		TextWidth = text->GetWidth();
		TextHeight = text->GetHeight();
		FChanged = false;
		if (AutoSize)
		{
			auto rect = font->MeasureString(FCaption, MultiLine ? Width : 0);
			SetWidth(rect.w);
			SetHeight(rect.h);
		}
	}

	void Label::Draw(int absX, int absY)
	{
		Control::Draw(absX, absY);
		absX = absX + Left;
		absY = absY + Top;
		auto entry = GetEntry();
		if (font == nullptr)
		{
			font = entry->System->LoadDefaultFont();
			FChanged = true;
		}
		if (FChanged || !text)
			UpdateText();
		if (DropShadow)
		{
			entry->System->DrawBakedText(text.Ptr(), ShadowColor, absX + 1, absY + 1);
		}
		entry->System->DrawBakedText(text.Ptr(), FontColor, absX, absY);
	}

	Button::Button(Container * parent)
		: Label(parent)
	{
		IsMouseDown = false;
		TabStop = true;
		Type = CT_BUTTON;
		BorderStyle = BS_RAISED;
		FocusRectColor.B = FocusRectColor.G = FocusRectColor.R = 0;
		FocusRectColor.A = 255;
		BackColor = Global::ColorTable.ControlBackColor;
		FontColor = Global::ColorTable.ControlFontColor;
		Checked = false;
		AutoSize = false;
	}

	void Button::Draw(int absX, int absY)
	{
		if (!Visible)
			return;
		int lastBorder = BorderStyle;
		Color backColor = BackColor;
		if (Checked)
		{
			BackColor = Global::ColorTable.ButtonBackColorChecked;
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
			UpdateText();
		int tx,ty;
		tx = (Width - text->GetWidth())/2;
		ty = (Height - text->GetHeight())/2;
		if (BorderStyle == BS_LOWERED)
		{
			tx += 1;
			ty += 1;
		}
			
		if (Enabled)
		{
			entry->System->DrawBakedText(text.Ptr(), FontColor, absX+tx,absY+ty);
		}
		else
		{
				
			entry->System->DrawBakedText(text.Ptr(), Color(255,255,255,FontColor.A), absX + tx + 1, absY + ty + 1);
			entry->System->DrawBakedText(text.Ptr(), Color((unsigned char)ClampInt(FontColor.R + COLOR_LIGHTEN, 0, 255),
				(unsigned char)ClampInt(FontColor.R + COLOR_LIGHTEN, 0, 255),
				(unsigned char)ClampInt(FontColor.R + COLOR_LIGHTEN, 0, 255),
				FontColor.A), absX + tx, absY + ty);
		}
		
		// Draw Focus Rect
		if (Focused)
		{
			Graphics::DashPattern = DASH_DOT_PATTERN;
			Graphics::PenColor = FocusRectColor;
			Graphics::DrawRectangle(entry->System, absX + 3, absY + 3, absX + Width - 3, absY + Height - 3);
			Graphics::DashPattern = -1;
		}
	}

	bool Button::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Label::DoMouseDown(X,Y,Shift); 
		if (!Enabled || !Visible)
			return false;
		if (Shift==SS_BUTTONLEFT)
		{
			IsMouseDown = true;
			BorderStyle = BS_LOWERED;
		}
		Global::MouseCaptureControl = this;
		return true;
	}

	bool Button::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Label::DoMouseUp(X,Y,Shift);
		IsMouseDown = false;
		BorderStyle = BS_RAISED;
		ReleaseMouse();
		return true;
	}

	bool Button::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Label::DoKeyDown(Key,Shift);
		if (!Focused || !Enabled || !Visible)
			return false;
		if (Key == VK_SPACE)
		{
			IsMouseDown = true;
			BorderStyle = BS_LOWERED;
		}
		else if (Key == VK_RETURN)
		{
			Control::DoClick();
		}
		return false;
	}

	bool Button::DoKeyUp(unsigned short Key, SHIFTSTATE Shift)
	{
		Label::DoKeyUp(Key,Shift);
		if (!Focused || !Enabled || !Visible)
			return false;
		if (Key == VK_SPACE)
		{
			IsMouseDown = false;
			BorderStyle = BS_RAISED;
			Control::DoClick();
		}
		return false;
	}

	Control * Container::FindControlAtPosition(int x, int y)
	{
		if (Visible && IsPointInClient(x, y))
		{
			if (x <= Margin || y <= Margin || x >= Width - Margin || y >= Height - Margin)
				return this;
			for (int i = Controls.Count() - 1; i >= 0; i--)
			{
				if (Controls[i]->EventID != Global::EventGUID)
				{
					int dx = 0;
					int dy = 0;
					if (Controls[i]->DockStyle == dsNone || Controls[i]->DockStyle == dsFill)
					{
						dx = clientRect.x;
						dy = clientRect.y;
					}
					int nx = x - dx;
					int ny = y - dy;
					if (auto child = Controls[i]->FindControlAtPosition(nx - Controls[i]->Left, ny - Controls[i]->Top))
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
		Margin = 0;
	}

	Container::Container(Container * parent)
		: Container(parent, true)
	{
	}

	Container::~Container()
	{
			
	}

	bool Container::DoClosePopup()
	{
		for (int i=0;i<Controls.Count(); i++)
			Controls[i]->DoClosePopup();
		return false;
	}

	void Container::KillFocus()
	{
		for (int i = 0; i<Controls.Count(); i++)
		{
			Controls[i]->KillFocus();
		}
		Control::KillFocus();
	}

	void Container::SetAlpha(unsigned char Alpha)
	{
		BackColor.A = Alpha;
		for (int i=0; i<Controls.Count(); i++)
		{
			Controls[i]->BackColor.A = Alpha;
		}
	}

	void Container::AddChild(Control *nControl)
	{
		Controls.Add(nControl);
		nControl->Parent = this;
	}

	void Container::RemoveChild(Control *AControl)
	{
		for (int i=0; i<Controls.Count(); i++)
		{
			if (Controls[i] == AControl)
			{
				Controls[i] = nullptr;
				Controls.RemoveAt(i);
				break;
			}
		}
	}

	void Container::DrawChildren(int absX, int absY)
	{
		auto entry = GetEntry();
		entry->ClipRects->AddRect(Rect(absX, absY, Width, Height));
		for (int i = 0; i<Controls.Count(); i++)
		{
			if (Controls[i]->Visible)
			{
				Control *ctrl = Controls[i].operator->();
				if (ctrl->Visible)
				{
					int dx = 0;
					int dy = 0;
					if (ctrl->DockStyle == dsNone || ctrl->DockStyle == dsFill)
					{
						dx = clientRect.x;
						dy = clientRect.y;
					}
					entry->ClipRects->AddRect(Rect(ctrl->Left + absX + dx, ctrl->Top + absY + dy, ctrl->GetWidth() + 1, ctrl->GetHeight() + 1));
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
		clientRect.x = initalClientRect.x + Margin;
		clientRect.y = initalClientRect.y + Margin;
		clientRect.w -= Margin * 2;
		clientRect.h -= Margin * 2;
		for (int i=0; i<Controls.Count(); i++)
		{
			if (!Controls[i]->Visible)
				continue;
			switch (Controls[i]->DockStyle)
			{
			case dsTop:
				Controls[i]->Posit(clientRect.x, clientRect.y, clientRect.w, Controls[i]->GetHeight());
				clientRect.y += Controls[i]->GetHeight();
				clientRect.h -= Controls[i]->GetHeight();
				break;
			case dsBottom:
				Controls[i]->Posit(clientRect.x, clientRect.y+clientRect.h-Controls[i]->GetHeight(), clientRect.w,
					Controls[i]->GetHeight());
				clientRect.h -= Controls[i]->GetHeight();
				break;
			case dsLeft:
				Controls[i]->Posit(clientRect.x, clientRect.y, Controls[i]->GetWidth(), clientRect.h);
				clientRect.x += Controls[i]->GetWidth();
				clientRect.w -= Controls[i]->GetWidth();
				break;
			case dsRight:
				Controls[i]->Posit(clientRect.x+clientRect.w-Controls[i]->GetWidth(), clientRect.y,
					Controls[i]->GetWidth(), clientRect.h);
				clientRect.w -= Controls[i]->GetWidth();
				break;
			}
		}
		for (int i=0; i<Controls.Count(); i++)
		{
			if (Controls[i]->DockStyle == dsFill)
			{
				Controls[i]->Posit(0,0, clientRect.w, clientRect.h);
			}
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
		for (int i=0; i<Controls.Count(); i++)
			Controls[i]->DoDblClick();
		return false;
	}

	bool Container::ContainsFocus()
	{
		if (Focused)
			return true;
		for (int i=0; i<Controls.Count(); i++)
		{
			if (Controls[i]->ContainsFocus())
				return true;
		}
		return false;
	}

	bool Container::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoKeyDown(Key,Shift);
		for (int i=Controls.Count()-1; i>=0; i--)
		{
			Controls[i]->DoKeyDown(Key,Shift);
		}
		return false;
	}

	bool Container::DoKeyUp(unsigned short Key, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoKeyUp(Key,Shift);
		for (int i=Controls.Count()-1; i>=0; i--)
		{
			Controls[i]->DoKeyUp(Key,Shift);
		}
		return false;
	}

	bool Container::DoKeyPress(unsigned short Key, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoKeyPress(Key,Shift);
		for (int i=Controls.Count()-1; i>=0; i--)
		{
			Controls[i]->DoKeyPress(Key,Shift);
		}
		return false;
	}

	void Container::InternalBroadcastMessage(UI_MsgArgs *Args)
	{
		this->HandleMessage(Args);
		for (int i=Controls.Count()-1; i>=0; i--)
		{
			Container * ctn = dynamic_cast<Container *>(Controls[i].operator->());
			if (ctn)
				ctn->InternalBroadcastMessage(Args);
			else
				Controls[i]->HandleMessage(Args);
		}
			
	}

	FormStyle::FormStyle()
	{

	}

	Form::Form(UIEntry * parent)
		: Container(parent)
	{
		Type = CT_FORM;
		Activated = false;
		ButtonClose = true;
		DownInTitleBar = false;
		DownInButton = false;
		BackgroundShadow = false;
		ShadowOffset = 0;
		ShadowSize = 25.0f;
		DownPosX = DownPosY = 0;
		Text = L"Form";
		parent->Forms.Add(this);
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
		formStyle = Global::ColorTable.DefaultFormStyle;
		formStyle.TitleFont = parent->GetEntry()->System->LoadDefaultFont(GraphicsUI::DefaultFontType::Title);
		formStyle.TitleBarHeight = (int)(parent->GetEntry()->GetLineHeight() * 1.2f);
	
		Left = Top = 20;
		Height = Width = 200;
		Margin = 5;
		FormStyleChanged();
		SetText(Text);
	}

	Form::~Form()
	{
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

	void Form::SetAlpha(unsigned char Alpha)
	{
		Container::SetAlpha(Alpha);
		formStyle.TitleBarColors[0].A = Alpha;
		formStyle.TitleBarColors[1].A = Alpha;
		formStyle.TitleBarColors[2].A = Alpha;
		formStyle.TitleBarColors[3].A = Alpha;

		formStyle.TitleBarDeactiveColors[0].A = Alpha;
		formStyle.TitleBarDeactiveColors[1].A = Alpha;
		formStyle.TitleBarDeactiveColors[2].A = Alpha;
		formStyle.TitleBarDeactiveColors[3].A = Alpha;
	}

	int Form::GetClientHeight()
	{
		return Height-Margin * 2-formStyle.TitleBarHeight;
	}

	int Form::GetClientWidth()
	{
		return Width - Margin * 2;
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
		lblTitle->SetFont(formStyle.TitleFont);
		lblTitle->FontColor = formStyle.TitleBarFontColor;
		lblClose->FontColor = formStyle.TitleBarFontColor;
		btnClose->Posit(0,0,formStyle.TitleBarHeight-4,formStyle.TitleBarHeight-4);
		BackColor = formStyle.BackColor;
		BorderColor = formStyle.BorderColor;
		BorderStyle = BS_FLAT_;
		btnClose->BackColor = formStyle.CtrlButtonBackColor;
		SizeChanged();
	}

	void Form::SizeChanged()
	{
		btnClose->Posit(Width-formStyle.TitleBarHeight,3,formStyle.TitleBarHeight-4,formStyle.TitleBarHeight-4);
		lblClose->Posit(Width-formStyle.TitleBarHeight+2,3,formStyle.TitleBarHeight-4,formStyle.TitleBarHeight-4);
		Control::SizeChanged();
		OnResize.Invoke(this);
		ArrangeControls(Rect(1, 1 + formStyle.TitleBarHeight, Width - 2, Height - 2 - formStyle.TitleBarHeight));
		
	}

	void Form::Draw(int absX,int absY)
	{
		if (!Enabled ||!Visible)
			return;
		int ox=absX, oy=absY;
		absX+=Left; absY+=Top;
		drawChildren = false;
	
		BackgroundShadow = Activated;
		Container::Draw(ox,oy);
		auto entry = GetEntry();
		//Title bar
		Color *Color = Activated?formStyle.TitleBarColors :formStyle.TitleBarDeactiveColors; 
		Graphics::SolidBrushColor = Color[0];
		Graphics::FillRectangle(entry->System, absX + 1, absY + 1, absX + Width - 2, absY + 1 + formStyle.TitleBarHeight);
		entry->ClipRects->AddRect(Rect(absX,  absY, lblClose->Left - 24, formStyle.TitleBarHeight));
		lblTitle->Draw(absX+8,absY+1+(formStyle.TitleBarHeight-lblTitle->GetHeight())/2);
		entry->ClipRects->PopRect();
		//Draw close Button
		if (ButtonClose)
		{
			btnClose->Draw(absX,absY);
			lblClose->Draw(absX,absY);
		}

		//Draw Controls
		entry->ClipRects->AddRect(Rect(absX + Margin, absY + Margin + formStyle.TitleBarHeight, Width - Margin * 2, Height - Margin * 2 - formStyle.TitleBarHeight));
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
			if (X>Width-formStyle.TitleBarHeight && X<Width && Y>0 && Y<formStyle.TitleBarHeight+1)
			{
				GetEntry()->CloseWindow(this);
			}
		}
		if (Left < 0) Left = 0;
		if (Top < 0) Top = 0;
		DownInButton = false;
		return false;
	}

	void SetResizeCursor(ResizeMode rm)
	{
		switch (rm)
		{
		case ResizeMode::None:
			SetCursor(LoadCursor(0, IDC_ARROW));
			break;
		case ResizeMode::Left:
		case ResizeMode::Right:
			SetCursor(LoadCursor(0, IDC_SIZEWE));
			break;
		case ResizeMode::Top:
		case ResizeMode::Bottom:
			SetCursor(LoadCursor(0, IDC_SIZENS));
			break;
		case ResizeMode::TopLeft:
		case ResizeMode::BottomRight:
			SetCursor(LoadCursor(0, IDC_SIZENWSE));
			break;
		default:
			SetCursor(LoadCursor(0, IDC_SIZENESW));
			break;
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
			if (X > 3 && X < Width - formStyle.TitleBarHeight && Y > 0 && Y < formStyle.TitleBarHeight + 1)
			{
				DownInTitleBar = true;
				Global::MouseCaptureControl = this;
			}
			else
			{
				DownInTitleBar = false;
				if (X > Width - formStyle.TitleBarHeight && X < Width - 2 && Y > 0 && Y < formStyle.TitleBarHeight + 1)
				{
					DownInButton = true;
					Global::MouseCaptureControl = this;
				}
			}
		}
		else
		{
			SetResizeCursor(resizeMode);
			Global::MouseCaptureControl = this;
		}
		return false;
	}

	bool Form::DoMouseMove(int X, int Y)
	{
		const int MinWidth = 120;
		const int MinHeight = formStyle.TitleBarHeight * 2;

		if (!Enabled ||!Visible)
			return false;
		Container::DoMouseMove(X-1,Y-1);		
		if (resizeMode != ResizeMode::None)
		{
			SetResizeCursor(resizeMode);
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
			return true;
		}
		else
		{
			auto rm = GetResizeHandleType(X, Y);
			SetResizeCursor(rm);
			if (DownInTitleBar)
			{
				int dx, dy;
				dx = X - DownPosX; dy = Y - DownPosY;
				Left += dx; Top += dy;
				return true;
			}
		}
		return false;
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
		ClipRects = new ClipRectStack(pSystem);
		Left = Top =0;
		Global::EventGUID = 0;
		Height = WndHeight;
		Width = WndWidth;
		BorderStyle = BS_NONE;
		Type = CT_ENTRY;
		FIMEHandler = new IMEHandler(this);
		FIMEHandler->IMEWindow->WindowWidth = WndWidth;
		FIMEHandler->IMEWindow->WindowHeight = WndHeight;
		FocusedControl = NULL;
		ClipRects->WindowHeight = WndHeight;
		ClipRects->WindowWidth = WndWidth;
		ActiveForm = 0;
		CheckmarkLabel = new Label(this);
		CheckmarkLabel->AutoSize = true;
		CheckmarkLabel->Visible = false;
		CheckmarkLabel->SetFont(pSystem->LoadDefaultFont(DefaultFontType::Symbol));
		CheckmarkLabel->SetText(L"a");
		FIMEHandler->IMEWindow->Visible = false;
		Global::SCROLLBAR_BUTTON_SIZE = (int)(GetLineHeight());
	}

	SHIFTSTATE UIEntry::GetCurrentShiftState()
	{
		SHIFTSTATE Shift = 0;
		if (GetAsyncKeyState(VK_SHIFT))
			Shift = Shift | SS_SHIFT;
		if (GetAsyncKeyState(VK_CONTROL))
			Shift = Shift | SS_CONTROL;
		if (GetAsyncKeyState(VK_MENU ))
			Shift = Shift | SS_ALT;
		return Shift;
	}

	void UIEntry::InternalBroadcastMessage(UI_MsgArgs *Args)
	{
		//Broadcast to the activated form only.
		if (ActiveForm)
		{
			ActiveForm->InternalBroadcastMessage(Args);
		}
		for (auto & ctrl : Controls)
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
		FIMEHandler->IMEWindow->WindowWidth = Width;
		FIMEHandler->IMEWindow->WindowHeight = Height;
		ClipRects->WindowHeight = Height;
		ClipRects->WindowWidth = Width;
	}

	void UIEntry::TranslateMouseMessage(UIMouseEventArgs &Data, WPARAM wParam, LPARAM lParam)
	{
		Data.Shift = GetCurrentShiftState();
		bool L,M,R, S, C;
		L = (wParam&MK_LBUTTON)!=0;
		M = (wParam&MK_MBUTTON)!=0;
		R = (wParam&MK_RBUTTON)!=0;
		S = (wParam & MK_SHIFT)!=0;
		C = (wParam & MK_CONTROL)!=0;
		Data.Delta = GET_WHEEL_DELTA_WPARAM(wParam);
		if (L)
		{
			Data.Shift = Data.Shift | SS_BUTTONLEFT;
		} else{
			if (M){
				Data.Shift = Data.Shift | SS_BUTTONMIDDLE;
			} 
			else{
				if (R) {
					Data.Shift = Data.Shift | SS_BUTTONRIGHT;
				}
			}
		}
		if (S)
			Data.Shift = Data.Shift | SS_SHIFT;
		if (C)
			Data.Shift = Data.Shift | SS_CONTROL;
		Data.X = GET_X_LPARAM(lParam);
		Data.Y = GET_Y_LPARAM(lParam);
	}

	int UIEntry::HandleSystemMessage(HWND hWnd, UINT message, WPARAM &wParam, LPARAM &lParam)
	{
		int rs = -1;
		unsigned short Key;
		UIMouseEventArgs Data;
			
		switch (message) 
		{
		case WM_CHAR:
			{
			Key = (unsigned short)(DWORD)wParam;
			DoKeyPress(Key,GetCurrentShiftState());
			break;
			}
		case WM_KEYUP:
			{
			Key = (unsigned short)(DWORD)wParam;
			DoKeyUp(Key,GetCurrentShiftState());
			break;
			}
		case WM_KEYDOWN:
			{
			Key = (unsigned short)(DWORD)wParam;
			DoKeyDown(Key,GetCurrentShiftState());
			break;
			}
		case WM_SYSKEYDOWN:
			{
				Key = (unsigned short)(DWORD)wParam;
				if ((lParam&(1<<29)))
				{
					DoKeyDown(Key, SS_ALT);
				}
				else
					DoKeyDown(Key,0);
				break;
			}
		case WM_MOUSEMOVE:
			{
			TranslateMouseMessage(Data,wParam,lParam);
			DoMouseMove(Data.X,Data.Y);
			break;
			}
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			{
			TranslateMouseMessage(Data,wParam,lParam);
			DoMouseDown(Data.X,Data.Y,Data.Shift);
			SetCapture(hWnd);
			break;
			}
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_LBUTTONUP:
			{
			ReleaseCapture();
			TranslateMouseMessage(Data,wParam,lParam);
			if (message == WM_RBUTTONUP)
				Data.Shift = Data.Shift | SS_BUTTONRIGHT;
			else if (message == WM_LBUTTONUP)
				Data.Shift = Data.Shift | SS_BUTTONLEFT;
			else if (message == WM_MBUTTONUP)
				Data.Shift = Data.Shift | SS_BUTTONMIDDLE;
			DoMouseUp(Data.X,Data.Y,Data.Shift);
			break;
			}
		case WM_LBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
			{
				DoDblClick();
			}
			break;
		case WM_MOUSEWHEEL:
			{
				UI_MsgArgs a;
				a.Sender = this;
				a.Type = MSG_UI_MOUSEWHEEL;
				UIMouseEventArgs e;
				TranslateMouseMessage(e, wParam, lParam);
				a.Data = &e;
				//InternalBroadcastMessage(&a);
				DoMouseWheel(e.Delta);
			}
			break;
		case WM_SIZE:
			{
				RECT rect;
				GetClientRect(hWnd, &rect);
				SetWidth(rect.right-rect.left);
				SetHeight(rect.bottom-rect.top);
			}
			break;
		case WM_PAINT:
			{
				//Draw(0,0);
			}
			break;
		case WM_ERASEBKGND:
			{
			}
			break;
		case WM_NCMBUTTONDOWN:
		case WM_NCRBUTTONDOWN:
		case WM_NCLBUTTONDOWN:
			{
				DoClosePopup();
			}
			break;
		default:
			break;
		}
		if (rs == -1)
			rs = FIMEHandler->HandleMessage(hWnd,message,wParam,lParam);
		return rs;
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

	void UIEntry::DrawUI()
	{
		Draw(0,0);
	}

	bool UIEntry::DoKeyDown(unsigned short Key,SHIFTSTATE Shift)
	{
		if (Key == VK_TAB)
		{
			if (Shift & SS_CONTROL)
			{
				if (Forms.Count())
					ShowWindow(Forms.First());
			}
			else
			{
				if (Shift == SS_SHIFT)
					MoveFocusBackward();
				else
					MoveFocusForward();
			}
		}
		//Key events are broadcasted to the active form only.
		if (ActiveForm)
		{
			ActiveForm->DoKeyDown(Key,Shift);
		}
		else
			Container::DoKeyDown(Key,Shift);
		return false;
	}

	bool UIEntry::DoKeyUp(unsigned short Key, SHIFTSTATE Shift)
	{
		//Key events are broadcasted to the active form only.
		if (ActiveForm)
		{
			ActiveForm->DoKeyUp(Key,Shift);
		}
		else
			Container::DoKeyUp(Key,Shift);
		return false;
	}

	bool UIEntry::DoKeyPress(unsigned short Key, SHIFTSTATE Shift)
	{
		//Key events are broadcasted to the active form only.
		if (ActiveForm)
		{
			ActiveForm->DoKeyPress(Key,Shift);
		}
		else
			Container::DoKeyPress(Key,Shift);
		return false;
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
			int cx = X - clientRect.x;
			int cy = Y - clientRect.y;
			for (int i = Forms.Count() - 1; i >= 0; i--)
			{
				Form *curForm = Forms[i];
				if (curForm->Visible && curForm->Enabled && cx >= curForm->Left && cx <= curForm->Left + curForm->GetWidth() &&
					cy >= curForm->Top && cy <= curForm->Top + curForm->GetHeight())
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
		if (Global::MouseCaptureControl == this)
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
		if (Global::MouseCaptureControl == this)
		{
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
		/*UI_MsgArgs a;
		a.Sender = this;
		a.Type = MSG_UI_MOUSEWHEEL;
		UIMouseEventArgs e;
		e.Delta = delta;
		a.Data = &e;
		InternalBroadcastMessage(&a);
		return false;*/
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
		DeactivateAllForms();
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

	Control * FindNextFocus(Control * ctrl)
	{
		if (auto ctn = dynamic_cast<Container*>(ctrl))
		{
			for (auto & child : ctn->Controls)
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
		while (parent->Controls.Last() == ctrl)
		{
			ctrl = parent;
			parent = ctrl->Parent;
			if (!parent)
				break;
		}
		if (parent)
		{
			int idx = parent->Controls.IndexOf(ctrl);
			for (int i = idx + 1; i < parent->Controls.Count(); i++)
			{
				if (parent->Controls[i]->Enabled && parent->Controls[i]->Visible)
				{
					if (parent->Controls[i]->TabStop)
						return parent->Controls[i].Ptr();
					else
						return FindNextFocus(parent->Controls[i].Ptr());
				}
			}
		}
		return nullptr;
	}

	Control * GetLastLeaf(Container * ctn)
	{
		if (ctn->Controls.Count() == 0)
			return ctn;
		for (int i = ctn->Controls.Count() - 1; i >= 0; i--)
		{
			auto ctrl = ctn->Controls[i].Ptr();
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
		while (parent && parent->Controls.First() == ctrl)
		{
			ctrl = parent;
			parent = ctrl->Parent;
			if (!parent)
				break;
		}
		if (parent)
		{
			int idx = parent->Controls.IndexOf(ctrl);
			for (int i = idx - 1; i >= 0; i--)
			{
				if (parent->Controls[i]->Enabled && parent->Controls[i]->Visible)
				{
					if (parent->Controls[i]->TabStop)
						return parent->Controls[i].Ptr();
					else if (auto ctn = dynamic_cast<Container*>(parent->Controls[i].Ptr()))
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

	void UIEntry::Draw(int absX,int absY)
	{
		drawChildren = false;
		Container::Draw(absX,absY);
		//Draw Forms
		for (auto children : Controls)
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
		if (FocusedControl && (FocusedControl->Type & CT_TEXTBOX)==CT_TEXTBOX)
		{
			if (FIMEHandler->Enabled)
			{
				FIMEHandler->IMEWindow->Draw(((CustomTextBox *)FocusedControl)->AbsCursorPosX,((CustomTextBox *)FocusedControl)->AbsCursorPosY);
			}
		}
			
	}

	void UIEntry::SetFocusedControl(Control *Target)
	{
		if (FocusedControl &&FocusedControl != Target)
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
				this->ShowWindow(dynamic_cast<Form*>(parent));
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
		if (Target)
			Target->Focused = true;
		FocusedControl = Target;
		if (Target && (Target->Type & CT_TEXTBOX) == CT_TEXTBOX)
		{
			FIMEHandler->TextBox = (CustomTextBox *)Target;
		}
		else
		{
			FIMEHandler->TextBox = NULL;
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
			for (int i = Controls.Count() - 1; i >= 0; i--)
			{
				if (auto rs = checkCtrl(Controls[i].Ptr()))
					return rs;
			}
			return this;
		}
		return nullptr;
	}

	CheckBox::CheckBox(Container * parent)
		: Label(parent)
	{
		FontColor = Global::ColorTable.MenuItemForeColor;
		FocusRectColor.R = 0; FocusRectColor.G = 0; FocusRectColor.G = 0; FocusRectColor.A = 100;
		BackColor = Global::ColorTable.ControlBackColor;
		TabStop = true;
		Type = CT_CHECKBOX;
		BorderStyle = BS_NONE;
		BackColor.A = 0;
		Checked = false;
	}


	void CheckBox::SetText(const CoreLib::String & pText)
	{
		Label::SetText(pText);
		if (AutoSize)
			this->Width = TextWidth + (int)(GetEntry()->CheckmarkLabel->TextWidth * 1.5f) + 1;
	}

	void CheckBox::Draw(int absX, int absY)
	{
		Control::Draw(absX,absY);
		absX = absX + Left;
		absY = absY + Top;
		//Draw Check Box
		Color LightColor, DarkColor;
		LightColor.R = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.R + COLOR_LIGHTEN,0,255);
		LightColor.G = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.G + COLOR_LIGHTEN,0,255);
		LightColor.B = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.B + COLOR_LIGHTEN,0,255);
		LightColor.A = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.A+COLOR_LIGHTEN, 0, 255);
		DarkColor.R = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.R - COLOR_LIGHTEN, 0, 255);
		DarkColor.G = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.G - COLOR_LIGHTEN, 0, 255);
		DarkColor.B = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.B - COLOR_LIGHTEN, 0, 255);
		DarkColor.A = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.A + COLOR_LIGHTEN, 0, 255);
		auto entry = GetEntry();
		int checkBoxSize = GetEntry()->CheckmarkLabel->TextWidth;
		int checkBoxTop = (Height - checkBoxSize) >> 1;
		Graphics::PenColor = DarkColor;
		Graphics::DrawLine(entry->System, absX, absY + checkBoxTop, absX + checkBoxSize, absY + checkBoxTop);
		Graphics::DrawLine(entry->System, absX, absY + checkBoxTop, absX, absY + checkBoxSize + checkBoxTop);
		Graphics::PenColor = LightColor;
		Graphics::DrawLine(entry->System, absX + checkBoxSize, absY + checkBoxTop, absX + checkBoxSize, absY + checkBoxSize + checkBoxTop);
		Graphics::DrawLine(entry->System, absX + checkBoxSize, absY + checkBoxSize + checkBoxTop, absX, absY + checkBoxSize + checkBoxTop);
		// Draw check mark
		if (Checked)
		{
			auto checkMark = entry->CheckmarkLabel;
			checkMark->FontColor = FontColor;
			checkMark->Draw(absX,absY);
		}
		//Draw Caption
		int textStart = checkBoxSize + checkBoxSize / 4;
		Label::Draw(absX+ textStart -Left, absY-Top);
		// Draw Focus Rect
		if (Focused)
		{
			Graphics::DashPattern = DASH_DOT_PATTERN;
			Graphics::PenColor = FocusRectColor;
			Graphics::DrawRectangle(entry->System, absX + textStart, absY, absX + text->GetWidth() + textStart, absY + text->GetHeight());
			Graphics::DashPattern = -1;
		}
	}

	bool CheckBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X,Y,Shift);
		if (!Enabled || !Visible)
			return false;
		Checked = !Checked;
		UI_MsgArgs Args;
		Args.Sender = this;
		Args.Type = MSG_UI_CHANGED;
		BroadcastMessage(&Args);
		return false;
	}

	bool CheckBox::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key,Shift);
		if (!Focused || !Enabled || !Visible)
			return false;
		if (Key == VK_RETURN || Key == VK_SPACE)
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
				for (int i=0; i<((Container * )Parent)->Controls.Count(); i++)
				{
					Control * curControl = ((Container * )Parent)->Controls[i].operator->();
					if (curControl->Type == CT_RADIOBOX)
					((RadioBox *)curControl)->Checked = false;
				}
				Checked = true;
			}
		}
	}

	void RadioBox::Draw(int absX, int absY)
	{
		Control::Draw(absX,absY);
		absX = absX + Left;
		absY = absY + Top;
		//Draw Check Box
		Color LightColor, DarkColor;
		LightColor.R = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.R + COLOR_LIGHTEN, 0, 255);
		LightColor.G = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.G + COLOR_LIGHTEN, 0, 255);
		LightColor.B = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.B + COLOR_LIGHTEN, 0, 255);
		LightColor.A = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.A + COLOR_LIGHTEN, 0, 255);
		DarkColor.R = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.R - COLOR_LIGHTEN, 0, 255);
		DarkColor.G = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.G - COLOR_LIGHTEN, 0, 255);
		DarkColor.B = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.B - COLOR_LIGHTEN, 0, 255);
		DarkColor.A = (unsigned char)ClampInt(Global::ColorTable.ControlBorderColor.A + COLOR_LIGHTEN, 0, 255);
		auto entry = GetEntry();
		int checkBoxSize = GetEntry()->GetLineHeight();
		int rad = checkBoxSize / 2;
		int dotX = absX + rad;
		int dotY = absY + (Height >> 1);
		Graphics::PenColor = DarkColor;
		Graphics::DrawArc(entry->System, dotX, dotY, rad, Math::Pi / 4, Math::Pi * 5 / 4);
		Graphics::PenColor = LightColor;
		Graphics::DrawArc(entry->System, dotX, dotY, rad, PI * 5 / 4, PI * 9 / 4);

		// Draw dot
		if (Checked)
		{
			Array<Vec2, 24> circlePoints;
			int edges = 20;
			float dTheta = Math::Pi * 2.0f / edges;
			float theta = 0.0f;
			float dotRad = rad * 0.5f;
			for (int i = 0; i < edges; i++)
			{
				circlePoints.Add(Vec2::Create(dotX + dotRad * cos(theta), dotY - dotRad * sin(theta)));
				theta += dTheta;
			}
			entry->System->FillPolygon(FontColor, circlePoints.GetArrayView());
		}
		//Draw Caption
		int textStart = checkBoxSize + checkBoxSize / 4;
		Label::Draw(absX + textStart - Left, absY - Top);
		// Draw Focus Rect
		if (Focused)
		{
			Graphics::DashPattern = DASH_DOT_PATTERN;
			Graphics::PenColor = FocusRectColor;
			Graphics::DrawRectangle(entry->System, absX + textStart, absY, absX + text->GetWidth() + textStart, absY + text->GetHeight());
			Graphics::DashPattern = -1;
		}
	}

	bool RadioBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X,Y,Shift);
		if (!Enabled || !Visible)
			return false;
		SetValue(true);
		return false;
	}

	bool RadioBox::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key,Shift);
		if (!Focused || !Enabled || !Visible)
			return false;
		if (Key == VK_RETURN || Key == VK_SPACE)
		{
			SetValue(true);
		}
		return false;
	}

	CustomTextBox::CustomTextBox(Container * parent)
		: Control(parent)
	{
		Cursor = IBeam;
		Type = CT_TEXTBOX;
		FText= "";
		font = parent->GetFont();
		SelectMode = false;
		TabStop = true;
		Locked = false; Changed = true;
		SelStart = SelLength = SelOrigin = 0;
		SelectionColor = Global::ColorTable.SelectionColor;
		SelectedTextColor = Global::ColorTable.SelectionForeColor;
		BorderStyle = BS_LOWERED;
		BackColor = Global::ColorTable.EditableAreaBackColor;
		FontColor = Global::ColorTable.ControlFontColor;
		TextBorderX =2; TextBorderY = 4;
		LabelOffset = TextBorderX;
		QueryPerformanceFrequency((LARGE_INTEGER *)&Freq);
		QueryPerformanceCounter((LARGE_INTEGER *)&Time);
		CursorPos = 0;
		KeyDown = false;
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
		int txtWidth = font->MeasureString(FText, 0).w;
		if (txtWidth <= Width-TextBorderX*2)
		{
			LabelOffset = TextBorderX;
		}
		else
		{
			String ls;
			ls = FText.SubString(0, CursorPos);
			int px = font->MeasureString(ls, 0).w+LabelOffset;
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

	bool CustomTextBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X,Y,Shift);
		if (Enabled && Visible && (Shift | SS_BUTTONLEFT))
		{
			SetFocus();
			SelLength = 0;
			SelStart = HitTest(X);
			CursorPos = SelStart;
			SelectMode = true;
			SelOrigin = CursorPos;
			CursorPosChanged();
			Global::MouseCaptureControl = this;
			return true;
		}
		else
			SelectMode=false;
		return false;
	}

	int CustomTextBox::HitTest(int posX)
	{
		String curText;
		posX -= LabelOffset;
		curText = "";
		for (int i =0;i<FText.Length();i++)
		{
			curText = curText + FText[i];
			int tw = font->MeasureString(curText, 0).w;
			if (tw>posX)
			{
				int cw = font->MeasureString(FText[i], 0).w;
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
		CursorPosChanged();
		return false;
	}

	void CustomTextBox::CopyToClipBoard()
	{
		if( SelLength!=0 && OpenClipboard( NULL ) )
		{
			EmptyClipboard();

			HGLOBAL hBlock = GlobalAlloc( GMEM_MOVEABLE, sizeof(WCHAR) * ( FText.Length() + 1 ) );
			if( hBlock )
			{
				WCHAR *pwszText = (WCHAR*)GlobalLock( hBlock );
				if( pwszText )
				{
					CopyMemory( pwszText, FText.Buffer() + SelStart, SelLength* sizeof(WCHAR) );
					pwszText[SelLength] = L'\0';  // Terminate it
					GlobalUnlock( hBlock );
				}
				SetClipboardData( CF_UNICODETEXT, hBlock );
			}
			CloseClipboard();
			if( hBlock )
				GlobalFree( hBlock );
		}
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
			CursorPosChanged();
		}
	}

	void CustomTextBox::PasteFromClipBoard()
	{
		DeleteSelectionText();

		if( OpenClipboard( NULL ) )
		{
			HANDLE handle = GetClipboardData( CF_UNICODETEXT );
			if( handle )
			{
				// Convert the ANSI string to Unicode, then
				// insert to our buffer.
				WCHAR *pwszText = (WCHAR*)GlobalLock( handle );
				if( pwszText )
				{
					// Copy all characters up to null.
					String txt = pwszText;
					wchar_t rtn[2]={13,0};
					int fid = txt.IndexOf(String(rtn));
					if (fid!=-1)
						txt = txt.SubString(0, fid);
					DoInput(txt);
					GlobalUnlock( handle );
				}
			}
			CloseClipboard();
		}
	}

	bool CustomTextBox::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key,Shift);
		if (Enabled && Visible && Focused)
		{
			KeyDown=true;
			if (Shift==SS_SHIFT)
			{
				int selEnd;
				selEnd = SelStart+SelLength;
				if (Key==VK_LEFT)
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
					CursorPosChanged();
				}
				else if(Key==VK_RIGHT)
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
					CursorPosChanged();
				}	
			}
			else
			{
				if (Key==VK_LEFT)
				{
					if (SelLength == 0)
						CursorPos--;
					else
					{
						CursorPos = SelStart;
					}
					SelLength = 0;
					SelStart = CursorPos=ClampInt(CursorPos,0,FText.Length());
					CursorPosChanged();
				}			
				else if (Key==VK_RIGHT)
				{
					if (SelLength ==0)
						CursorPos++;
					else
						CursorPos = SelStart+SelLength;
					SelLength = 0;
					SelStart = CursorPos=ClampInt(CursorPos,0,FText.Length());
					CursorPosChanged();
				}
				else if (Key == VK_DELETE && !Locked)
				{
					if (SelLength!=0)
					{
						FText = DeleteString(FText, SelStart, SelLength);
						TextChanged();
						SelLength=0;
						CursorPos = SelStart;
					}
					else if (CursorPos<(int)FText.Length())
					{
						FText = DeleteString(FText, CursorPos, 1);
						TextChanged();
					}
					CursorPosChanged();
				}
				else if (Key == VK_BACK &&!Locked)
				{
					if (SelLength !=0)
						DeleteSelectionText();
					else if (CursorPos>0)
					{
						FText = DeleteString(FText, CursorPos-1, 1);
						CursorPos--;
						TextChanged();
						CursorPosChanged();
					}
						
				}
			}
		}
		return false;
	}

	bool CustomTextBox::DoKeyPress(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyPress(Key,Shift);
		if (!Focused)
			return false;
		if (GetAsyncKeyState('C') && Shift == SS_CONTROL)
		{
			CopyToClipBoard();
		}
		else if (GetAsyncKeyState('V')  && Shift == SS_CONTROL)
		{
			DeleteSelectionText();
			if (!Locked)
				PasteFromClipBoard();
		}
		else if (GetAsyncKeyState('X')  && Shift == SS_CONTROL)
		{
			CopyToClipBoard();
			DeleteSelectionText();
		}
		else if (GetAsyncKeyState(L'A') && Shift == SS_CONTROL)
		{
			SelectAll();
		}
		return false;
	}

	void CustomTextBox::TextChanged()
	{
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
				CursorPosChanged();
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
			return true;
		return false;
	}

	void CustomTextBox::SelectAll()
	{
		SelStart = 0;
		SelLength = FText.Length();
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
		if (Changed)
		{
			text = font->BakeString(FText, 0);
			Changed = false;
		}
		//Draw Text
		Rect textRect;
		textRect.x = absX+TextBorderX; textRect.y = absY; textRect.w = Width-TextBorderX-TextBorderX; textRect.h = Height;
		entry->ClipRects->AddRect(textRect);
		entry->System->DrawBakedText(text.Ptr(), FontColor, absX+LabelOffset,absY+TextBorderY);
		entry->ClipRects->PopRect();
		String ls;
		ls= FText.SubString(0, CursorPos);
		int csX = font->MeasureString(ls, 0).w;
		int spX=0,epX=0;
		csX+=LabelOffset;
		//Draw Selection Rect
			
		if (Focused && SelLength!=0)
		{
			if (SelStart + SelLength > FText.Length())
				SelLength = FText.Length() - SelStart;
			ls = FText.SubString(0, SelStart);
			spX = font->MeasureString(ls, 0).w;
			ls = FText.SubString(0, SelStart+SelLength);
			epX = font->MeasureString(ls, 0).w;
			spX+=LabelOffset+absX; epX+=LabelOffset+absX;
			Graphics::SolidBrushColor = SelectionColor;
			Graphics::FillRectangle(entry->System, spX, absY + TextBorderX, epX - 1, absY + Height - TextBorderX);
			entry->ClipRects->AddRect(Rect(spX, absY + TextBorderX, epX - 1 - spX, Height - TextBorderX));
			entry->System->DrawBakedText(text.Ptr(), Color(255, 255, 255, 255), absX + LabelOffset, absY + TextBorderY);
			entry->ClipRects->PopRect();
		}
			
		//Draw Cursor
		long long CurTime;
		float TimePassed;
		QueryPerformanceCounter((LARGE_INTEGER *)&CurTime);
		TimePassed = (CurTime-Time)/(float)Freq;
		int tick = int(TimePassed/CURSOR_FREQUENCY);
		if (Focused && (tick%2 || KeyDown))
		{
			AbsCursorPosX = absX+csX;
			AbsCursorPosY = absY+Height-TextBorderX;
			Graphics::PenColor = Color(255 - BackColor.R, 255 - BackColor.G, 255 - BackColor.B, 255);
			Graphics::DrawLine(entry->System, AbsCursorPosX, absY + TextBorderX, AbsCursorPosX, AbsCursorPosY);				
		}

	}

	bool TextBox::DoKeyPress(unsigned short Key, SHIFTSTATE Shift)
	{
		CustomTextBox::DoKeyPress(Key,Shift);
		if (!Focused || !Enabled ||!Visible)
			return false;
		return false;
	}

	IMEWindow::IMEWindow(Container * parent)
		: Container(parent)
	{
		CursorPos = 0;
		CandidateCount = 0;
		BorderWeight = 5;
		ShowCandidate = false;
		lblCompStr = new Label(this);
		lblIMEName = new Label(this);
		lblCompReadStr = new Label(this);
		Panel = new Control(this);
		lblIMEName->FontColor = Color(0,0,255,255);
		Panel->BorderStyle = BS_RAISED;
		Panel->BackColor.A = 200;
	}

	IMEWindow::~IMEWindow()
	{
	}

	void IMEWindow::SetCandidateListItem(int Index, const wchar_t* Data)
	{
		String id;
		CandidateList[Index] = Data;
		id = String(Index+1);
		CandidateList[Index] = id + L":" + Data;
		if (!lblCandList[Index])
			lblCandList[Index] = new Label(this->Parent);
		lblCandList[Index]->SetText(CandidateList[Index]);
	}

	void IMEWindow::ChangeCompositionReadString(String AString)
	{
		strCompRead = AString;
		lblCompReadStr->SetText(AString);
	}

	void IMEWindow::ChangeCompositionString(String AString)
	{
		lblCompStr->SetText(AString);
		strComp = AString;
	}

	void IMEWindow::ChangeInputMethod(String AInput)
	{
		lblIMEName->SetText(AInput);
		strIME = AInput;
	}

	void IMEWindow::Draw(int absX, int absY)
	{
		int maxW=0;
		int height;
		int cpx,cpy;
		absX+=BorderWeight;
		if (strComp!="")
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
			Panel->Left = cpx-BorderWeight;
			Panel->Top = cpy-BorderWeight;
			Panel->SetWidth(maxW+BorderWeight*2);
			Panel->SetHeight(height + BorderWeight *2);
			Panel->Draw(0,0);
			lblCompStr->Draw(cpx,cpy);		
			
			if (ShowCandidate)
			{
				height = (CandidateCount+1) *20;
				if (height+absY+25>WindowHeight)
					cpy = absY-20-height;
				else
					cpy = absY+25;
				height -=25;
				for (int i =0;i<CandidateCount;i++)
				{
					if (lblCandList[i]->TextWidth > maxW)
						maxW = lblCandList[i]->TextWidth;
				}
				if (lblIMEName->TextWidth > maxW)
					maxW = lblIMEName->TextWidth;
				if (maxW+absX>WindowWidth)
					cpx = WindowWidth-maxW;
				else
					cpx = absX;
				Panel->Left = cpx-BorderWeight;
				Panel->Top = cpy-BorderWeight;
				Panel->SetWidth( maxW+BorderWeight*2);
				Panel->SetHeight( height + BorderWeight *2);
				Panel->Draw(0,0);
				for (int i =0;i<CandidateCount;i++)
				{
					int posY = cpy+20*i;
					lblCandList[i]->Draw(cpx,posY);
				}
				Panel->Left = cpx-BorderWeight;
				Panel->Top += Panel->GetHeight();
				Panel->SetWidth(maxW+BorderWeight*2);
				Panel->SetHeight(lblIMEName->TextHeight + BorderWeight*2);
				Panel->Draw(0,0);
				lblIMEName->Draw(cpx,Panel->Top+BorderWeight);
			}
		}
	}

	void IMEWindow::SetCandidateCount(int Count)
	{
		for (auto & obj : lblCandList)
		{
			this->Parent->RemoveChild(obj);
		}
		lblCandList.Clear();
		lblCandList.SetSize(Count);
		for (int i = 0; i < lblCandList.Count(); i++)
			lblCandList[i] = nullptr;
		CandidateList.SetSize(Count);
		CandidateCount = Count;
	}


	IMEHandler::IMEHandler(UIEntry * entry)
	{
		TextBox = NULL;
		IMEWindow = new GraphicsUI::IMEWindow(entry);
		Enabled = false;
	}

	IMEHandler::~IMEHandler()
	{
	}

	void IMEHandler::StringInputed(String AString)
	{
		if (TextBox)
			TextBox->DoInput(AString);
	}

	int IMEHandler::HandleMessage(HWND hWnd, UINT message, WPARAM &wParam, LPARAM &lParam)
	{
		int rs = -1;
		switch (message)
		{
		case WM_CHAR:
			{
				if (wParam == 9 || (wParam >= 32 && wParam <= 127))
					StringInputed(String((wchar_t)(wParam)));
			}
			break;
		case WM_IME_SETCONTEXT:
			lParam = 0;
			break;
		case WM_INPUTLANGCHANGE://改变输入法
			{
				HKL hKL=GetKeyboardLayout(0);
				if(ImmIsIME(hKL))//判断是否使用输入法
				{
					Enabled = true;
					wchar_t InputName[50]={0};
					HIMC hIMC=ImmGetContext(hWnd);
					ImmEscape(hKL,hIMC,IME_ESC_IME_NAME,InputName);
					ImmReleaseContext(hWnd,hIMC);
					IMEWindow->ChangeInputMethod(String(InputName));
				}
				else
				{
					Enabled = false;
					IMEWindow->ChangeInputMethod(String(L""));
				}
			}
			rs = 0;
			break;
		case WM_IME_NOTIFY://选字列表修改
			{
				switch(wParam)
				{
					bool OpenCandidate;
					int Count;
				case IMN_OPENCANDIDATE://打开选字列表
					OpenCandidate=1;
					IMEWindow->ShowCandidate = true;
					break;
				case IMN_CHANGECANDIDATE://修改选字列表
					OpenCandidate=0;
					{
						HIMC hIMC=ImmGetContext(hWnd);
						unsigned int ListSize=ImmGetCandidateList(hIMC,0,NULL,0);
						if(ListSize)
						{
							//开辟缓冲区存放选字列表信息
							unsigned char* Buffer=new unsigned char[ListSize];
							CANDIDATELIST* List=(CANDIDATELIST*)Buffer;
							ImmGetCandidateList(hIMC,0,List,ListSize);
								
							if(List->dwCount<List->dwSelection)
							{
								Count=0;
							}
							else
							{
								Count=min(List->dwCount-List->dwSelection,List->dwPageSize);
								IMEWindow->SetCandidateCount(Count);
								for(unsigned int Index=0;((int)Index<Count);Index++)
								{
									//获得列表信息并计算数量
									IMEWindow->SetCandidateListItem(Index,(TCHAR *)(Buffer+List->dwOffset[List->dwSelection+Index]));
								}
							}
							delete[] Buffer;
						}
						ImmReleaseContext(hWnd,hIMC);
					}
					if(OpenCandidate)
					{
						IMEWindow->ChangeCompositionString(String(L""));
					}
					break;
				case IMN_CLOSECANDIDATE://关闭选字列表
					IMEWindow->ShowCandidate = false;
					break;
				}
			}
			break;
		case WM_IME_COMPOSITION:
			{
				HIMC hIMC=ImmGetContext(hWnd);
				if(lParam&GCS_COMPSTR)//获得输入栏的文字
				{
					TCHAR EditString[201];
					unsigned int StrSize=ImmGetCompositionString(hIMC,GCS_COMPSTR,EditString,sizeof(EditString)-sizeof(char));
					EditString[StrSize/sizeof(TCHAR)]=0;
					IMEWindow->ChangeCompositionString(String(EditString));
				}
				if(lParam&GCS_COMPREADSTR)//获得输入栏的文字
				{
					TCHAR EditString[201];
					unsigned int StrSize=ImmGetCompositionString(hIMC,GCS_COMPREADSTR,EditString,sizeof(EditString)-sizeof(TCHAR));
					EditString[StrSize/sizeof(TCHAR)]=0;
					IMEWindow->ChangeCompositionReadString(String(EditString));
				}
				if(lParam&GCS_CURSORPOS)
				{
					int CurPos;
					CurPos=ImmGetCompositionString(hIMC,GCS_CURSORPOS,NULL,0);
					IMEWindow->CursorPos = (CurPos*sizeof(TCHAR));
				}
				if(lParam&GCS_RESULTSTR)
				{
					TCHAR ResultStr[201];
					unsigned int StrSize=ImmGetCompositionString(hIMC,GCS_RESULTSTR,ResultStr,sizeof(ResultStr)-sizeof(TCHAR));
					ResultStr[StrSize/sizeof(TCHAR)]=0;
					StringInputed(String(ResultStr));
				}
				ImmReleaseContext(hWnd,hIMC);
				rs = 0;
			}
			break;
		case WM_IME_STARTCOMPOSITION://把WM_IME_STARTCOMPOSITION视为已处理以便消除Windows自己打开的输入框
			IMEWindow->ChangeCompositionString(String(L""));
			rs = 1;
			break;
		case WM_IME_ENDCOMPOSITION:
			IMEWindow->ChangeCompositionString(String(L""));
			IMEWindow->ShowCandidate = false;
			rs = 0;
			break;
		}
		return rs;
	}

	ScrollBar::ScrollBar(Container * parent, bool addToParent)
		: Container(parent, addToParent)
	{
		Type = CT_SCROLLBAR;
		BorderStyle = BS_NONE;
		BackColor = Global::ColorTable.ScrollBarBackColor;
		btnInc = new Button(this);
		btnDec = new Button(this);
		Slider = new Control(this);
		btnInc->TabStop = false;
		btnDec->TabStop = false;
		btnInc->SetFont(GetEntry()->System->LoadDefaultFont(DefaultFontType::Symbol));
		btnDec->SetFont(GetEntry()->System->LoadDefaultFont(DefaultFontType::Symbol));
		Min = 0; Max = 100; Position = 0;
		btnInc->OnMouseDown.Bind(this, &ScrollBar::BtnIncMouseDown);
		btnDec->OnMouseDown.Bind(this, &ScrollBar::BtnDecMouseDown);
		btnInc->OnMouseUp.Bind(this, &ScrollBar::BtnIncMouseUp);
		btnDec->OnMouseUp.Bind(this, &ScrollBar::BtnDecMouseUp);
		SetOrientation(SO_HORIZONTAL);
		SetValue(0,100,0,20);
		SmallChange = 1;
		LargeChange = 10;
		DownInSlider = false;
		tmrTick.OnTick.Bind(this, &ScrollBar::tmrTick_Tick);
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
		btnInc->Draw(absX,absY);
		btnDec->Draw(absX,absY);
		Slider->Draw(absX,absY);
	}

	void ScrollBar::SetOrientation(int NewOri)
	{
		Orientation = NewOri;
		Position = Min;
		SetValue(Min,Max,Position, PageSize);
		if (NewOri == SO_HORIZONTAL)
		{
			btnInc->SetText(L"4"); 
			btnDec->SetText(L"3");
		}
		else
		{
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
				
		}
		else
		{
			throw ("Invalid ScrollBar value.");
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
		int Delta,FreeSpace,Range,APos;
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
			APos = max(Min,APos);
			APos = min(Max,APos);
			SetPosition(APos);
		}
		auto hitTest = Container::FindControlAtPosition(X, Y);
		if (hitTest == btnDec || hitTest == btnInc)
			hitTest->DoMouseMove(X - hitTest->Left, Y - hitTest->Top);
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

	void ScrollBar::tmrTick_Tick(Object *, CoreLib::WinForm::EventArgs e)
	{
		if (tmrOrientation == 0)
		{
			if (Position-SmallChange>=Min)
			{
				SetPosition(Position-SmallChange);
			}
			else
				SetPosition(Min);
		}
		else
		{
			if (Position+SmallChange<=Max)
			{
				SetPosition(Position+SmallChange);
			}
			else
				SetPosition(Max);
		}
		tmrTick.Interval = 80;
		tmrTick.StartTimer();
	}

	void ScrollBar::BtnDecMouseDown(UI_Base *, UIMouseEventArgs &)
	{
		if (Position-SmallChange>=Min)
		{
			SetPosition(Position-SmallChange);
			tmrOrientation = 0;
			tmrTick.Interval = 500;
			tmrTick.StartTimer();
		}
	}

	void ScrollBar::BtnIncMouseDown(UI_Base *, UIMouseEventArgs &)
	{
		if (Position+SmallChange<=Max)
		{
			SetPosition(Position+SmallChange);
			tmrOrientation = 1;
			tmrTick.Interval = 500;
			tmrTick.StartTimer();
		}
	}

	void ScrollBar::BtnDecMouseUp(UI_Base *, UIMouseEventArgs &)
	{
		tmrTick.StopTimer();
	}

	void ScrollBar::BtnIncMouseUp(UI_Base *, UIMouseEventArgs &)
	{
		tmrTick.StopTimer();
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
		BorderStyle = BS_LOWERED;
		BackColor = Global::ColorTable.EditableAreaBackColor;
		HideSelection = false;
		MultiSelect = false;
		Selecting = false;
		DownInItem = false;
		HotTrack = false;
		SelectedIndex= -1;
		ItemHeight = 18;
		Margin = 2;
		SelectionColor = Global::ColorTable.SelectionColor;
		HighLightColor = Global::ColorTable.HighlightColor;
		HighLightForeColor = Global::ColorTable.HighlightForeColor;
		SelectionForeColor = Global::ColorTable.SelectionForeColor;
		FontColor = Global::ColorTable.ControlFontColor;
		FocusRectColor = Color(0,0,0,255);
		UnfocusedSelectionColor = Global::ColorTable.UnfocusedSelectionColor;
		HighLightColor = Global::ColorTable.HighlightColor;
		ScrollBar = new GraphicsUI::ScrollBar(this);
		ScrollBar->SetOrientation(SO_VERTICAL);
		ScrollBar->Visible = false;
		BorderWidth = 2;
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
		entry->ClipRects->AddRect(Rect(absX,absY,Width-BorderWidth*2-(ScrollBar->Visible?ScrollBar->GetWidth():0),Height-BorderWidth*2));
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
				CurItem->BackColor = HideSelection&&!Focused?BackColor:(Focused?SelectionColor:UnfocusedSelectionColor);
				CurItem->FontColor = SelectionForeColor;
			}
				
			else
			{
				CurItem->BackColor = BackColor;
				CurItem->FontColor = FontColor;
			}
			CurItem->Posit(BorderWidth+Margin,BorderWidth+(i-ScrollBar->GetPosition())*ItemHeight,Width-BorderWidth*2-1-bdr-Margin*2,ItemHeight);
			Graphics::SolidBrushColor = CurItem->BackColor;
			Graphics::FillRectangle(entry->System, absX + BorderWidth, absY + CurItem->Top, absX + Width - BorderWidth, absY + CurItem->Top + CurItem->GetHeight());
			CurItem->Draw(absX,absY);
		}
		entry->ClipRects->PopRect();
		if (Focused)
		{
			int FID =SelectedIndex;
			if (FID==-1) FID =0;
			bdr = BorderWidth*2;
			if (ScrollBar->Visible)	bdr += ScrollBar->GetWidth()+1;
			int RectX1 = BorderWidth+absX;
			int RectX2 = RectX1 + Width - bdr;
			int RectY1 = (FID-ScrollBar->GetPosition())*ItemHeight+absY+BorderWidth-1;
			int RectY2 = RectY1+ItemHeight+1;
			Graphics::DashPattern = DASH_DOT_PATTERN;
			Graphics::PenColor = FocusRectColor;
			Graphics::DrawRectangle(entry->System, RectX1, RectY1, RectX2, RectY2);
			Graphics::DashPattern = -1;
		}
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
			if (hitTest == CurItem || hitTest && hitTest->IsChildOf((Container*)CurItem))
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
		return false;
	}

	bool ListBox::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key,Shift);
		if (!Enabled || !Visible || !Focused)
			return false;
		int ShowCount=Height/ItemHeight;
		for (int i = ScrollBar->GetPosition();i <= ScrollBar->GetPosition()+ShowCount && i < Items.Count();i++)
		{
			Control *CurItem =Items[i];
			CurItem->DoKeyDown(Key,Shift);
		}
		if (Items.Count())
		{
			if (Key == VK_DOWN)
			{
				SelectedIndex = ClampInt(SelectedIndex+1,0,Items.Count()-1);
				SelectionChanged();

			}
			else if (Key==VK_UP)
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

	bool ListBox::DoMouseMove(int X, int Y)
	{
		Control::DoMouseMove(X,Y);
		if (!Enabled || !Visible)
			return false;
		auto hitTest = Container::FindControlAtPosition(X, Y);
		int bdr = ScrollBar->Visible?ScrollBar->GetWidth():0;
		if (ScrollBar->Visible && hitTest == ScrollBar)
			ScrollBar->DoMouseMove(X - hitTest->Left, Y - hitTest->Top);
		int ShowCount=Height/ItemHeight;
		for (int i = ScrollBar->GetPosition(); i <= ScrollBar->GetPosition() + ShowCount && i<Items.Count(); i++)
		{
			Control *CurItem = Items[i];
			if (hitTest == CurItem || hitTest && hitTest->IsChildOf((Container*)CurItem))
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
		if (DownInItem)
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
		if (Visible && Enabled)
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
			if (hitTest == CurItem || hitTest && hitTest->IsChildOf((Container*)CurItem))
				CurItem->DoMouseUp(X - CurItem->Left, Y - CurItem->Top, Shift);
		}
		DownInItem = false;
		Selecting = false;
		if (ScrollBar->Visible && hitTest == ScrollBar)
			ScrollBar->DoMouseUp(X - hitTest->Left, Y - hitTest->Top,Shift);
		if (lastSelIdx != SelectedIndex || Items.Count() && Items[0]->Type == CT_CHECKBOX)
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
			ScrollBar->SetValue(0, 2, 0, 1);
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
		btnDrop->TabStop = false;
		btnDrop->SetFont(GetEntry()->System->LoadDefaultFont(DefaultFontType::Symbol));
		btnDrop->SetText(L"6");
		TextBox = new GraphicsUI::TextBox(this);
		BorderStyle = BS_LOWERED;
		TextBox->BorderStyle = BS_NONE;
		ShowList = false;
		HotTrack = true;
		HighLightColor = SelectionColor;
		HighLightForeColor = SelectionForeColor;
		FocusRectColor = Color(255,255,255,0);
		SelectionColor = BackColor;
		SelectionForeColor = FontColor;
		UnfocusedSelectionColor = BackColor;
		ButtonSize = Global::SCROLLBAR_BUTTON_SIZE;
		BorderWidth = 1;

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

	void ComboBox::SizeChanged()
	{
		TextBox->Posit(BorderWidth,BorderWidth,Width-ButtonSize-BorderWidth*2,Height);
		btnDrop->Posit(Width-ButtonSize-BorderWidth,BorderWidth,ButtonSize,ButtonSize);
	}

	void ComboBox::Posit(int left, int top, int width, int height)
	{
		height = Global::SCROLLBAR_BUTTON_SIZE + BorderWidth * 4;
		Control::Posit(left, top, width, height);
	}

	void ComboBox::Draw(int absX, int absY)
	{
		Control::Draw(absX,absY);
		absX+=Left; absY+=Top;
		if (!Visible)
			return;
		TextBox->Focused = (Focused && !ShowList);
		TextBox->Draw(absX,absY);
		btnDrop->Checked = ShowList;
		btnDrop->Draw(absX,absY);
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
		TextBox->SetFocus();
		SelectedIndex = id;
		//TextBox->SelectAll();
		//OnChanged.Invoke(this);
	}

	void ComboBox::SetFocus()
	{
		Control::SetFocus();
		//TextBox->SelectAll();
			
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

	bool ComboBox::DoKeyDown(unsigned short Key, SHIFTSTATE)
	{
		if (!Visible || !Enabled)
			return false;
		bool AltDown = GetAsyncKeyState(VK_SHIFT)!=0;
		if (Focused && !AltDown)
		{
			if (Key == VK_UP)
			{
				SelectedIndex=ClampInt(SelectedIndex-1,0,Items.Count()-1);
				ChangeSelectedItem(SelectedIndex);
				return false;
			}
			else if (Key == VK_DOWN)
			{
				SelectedIndex=ClampInt(SelectedIndex+1,0,Items.Count()-1);
				ChangeSelectedItem(SelectedIndex);
				return false;
			}
		}
		if (Focused && Key == VK_DOWN && AltDown)
		{
			ToggleList(ShowList=true);
		}
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
				BorderColor = Global::ColorTable.ControlFontColor;
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
			if (!ShowList && Focused)
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
		ProgressBarColors[0]=Color(60,86,156,255);
		ProgressBarColors[1]=Color(10,36,106,255);
		ProgressBarColors[2]=Color(10,36,106,255);
		ProgressBarColors[3]=Color(60,86,156,255);
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
		if (Style == 2) //Block Style
		{
			entry->ClipRects->AddRect(Rect(absX+2,absY+2,Width-6,Height-4));
			PW = int(PH *0.65);
			int bc = (int)(Position/(float)Max *ceil((Width - 2)/(float)PW));
			for (int i=0;i<bc;i++)
			{
				int cx = i*PW+3+absX;
				int cy = 2+absY;
				Graphics::SolidBrushColor = ProgressBarColors[0];
				Graphics::FillRectangle(entry->System, cx, cy, cx + PW - 2, cy + PH);
			}
			entry->ClipRects->PopRect();
		}
		else
		{
			int cx = absX+3, cy= absY+2;
			PW = (Width -4)*Position/Max;
			Graphics::SolidBrushColor = ProgressBarColors[0];
			Graphics::FillRectangle(entry->System, cx, cy, cx + PW, cy + PH);
		}
	}
		
	Menu::Menu(Container * parent, MenuStyle s)
		: Container(parent), style(s)
	{
		Type = CT_MENU;
		TabStop = true;
		TopMost = true;
		Height = (Margin*2);
		Width = (Margin*2);
		BorderStyle = BS_NONE;
		BorderColor = Global::ColorTable.MenuBorderColor;
		BackColor = Global::ColorTable.MenuBackColor;
		curSubMenu = 0;
		parentItem = 0;
		if (style == msPopup)
			Visible = false;
		if (style == msPopup)
			BackgroundShadow = true;
		else
		{
			DockStyle = dsTop;
			BackColor = Global::ColorTable.ToolButtonBackColor1;
		}
	}

	void Menu::SetFocus()
	{
		if (style == msMainMenu)
			lastFocusedCtrl = GetEntry()->FocusedControl;
		Container::SetFocus();	
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
		if (Controls.IndexOf(item) == -1)
			Controls.Add(item);
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
		fid = Controls.IndexOf(item);
		if (fid != -1)
		{
			Controls.RemoveAt(fid);
		}
		PositMenuItems();
	}

	void Menu::PositMenuItems()
	{
		if (style == msPopup)
		{
			int cHeight = 0;
			Width = 0;
			ItemHeight = (int)(GetEntry()->GetLineHeight() * 1.5f);
			for (int i=0; i<Items.Count(); i++)
			{
				if (!Items[i]->Visible)
					continue;
				int nWidth = Items[i]->MeasureWidth()+ItemHeight;
				if (nWidth > Width)
					Width = nWidth;
				if (Items[i]->IsSeperator())
					Items[i]->SetHeight(ItemHeight>>2);
				else
					Items[i]->SetHeight(ItemHeight);
					
				Items[i]->Left = 0;
				Items[i]->Top = cHeight;

				cHeight += Items[i]->GetHeight();
			}
			Height = cHeight + Margin*2;
			for (int i=0; i<Items.Count(); i++)
			{
				Items[i]->SetWidth(Width - Margin*2);
			}
		}
		else
		{
			Height = (int)(GetEntry()->GetLineHeight() * 1.1f);
			Width = Margin;
			for (int i=0; i<Items.Count(); i++)
			{
				Items[i]->isButton = true;
				if (Items[i]->Visible && !Items[i]->IsSeperator())
				{
					Items[i]->Top = Margin;
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
		entry->System->SetRenderTransform(absX, absY);
		for (auto & item : Items)
			ItemHeight = Math::Max(ItemHeight, item->GetHeight());
		Graphics::SolidBrushColor = Global::ColorTable.MemuIconBackColor;
		Graphics::FillRectangle(entry->System, 0,Margin, ItemHeight, Height-Margin);
		Graphics::PenColor = Global::ColorTable.MenuBorderColor;
		Graphics::DrawRectangle(entry->System, 0,0,Width,Height);
		Graphics::DrawLine(entry->System, ItemHeight+Margin, Margin, ItemHeight+Margin, Height-Margin);
		int cposY = 0;

		for (int i =0; i<Items.Count(); i++)
		{
			int itemHeight = Items[i]->GetHeight();
			entry->System->SetRenderTransform(absX + Margin, absY + Margin + cposY);
			Items[i]->DrawMenuItem(Width, ItemHeight);
			cposY += itemHeight;
		}
		entry->System->SetRenderTransform(0, 0);
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
			tmrHover.StopTimer();
			curSubMenu = nullptr;
			ReleaseMouse();
		}
	}

	void Menu::DrawMenuBar(int absX, int absY)
	{
		Control::Draw(absX, absY);
		auto entry = GetEntry();
		int ox = absX + Left;
		int oy = absY + Top;
		int cposY = 0;
		for (int i = 0; i < Items.Count(); i++)
		{
			entry->System->SetRenderTransform(ox + Items[i]->Left, oy + Items[i]->Top);
			int itemHeight = Items[i]->IsSeperator()?3:ItemHeight;
			Items[i]->DrawMenuButton(Items[i]->GetWidth(), Items[i]->GetHeight());
			cposY += itemHeight;
		}
		entry->System->SetRenderTransform(0, 0);
	}

	void Menu::Draw(int absX, int absY)
	{
		if (style == msMainMenu)
			DrawMenuBar(absX, absY);
	}

	bool Menu::DoMouseHover()
	{
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
			if (X >= item->Left && X <= item->Left + item->Width &&
				Y >= item->Top && Y < item->Top + item->Height)
			{
				item->Selected = true;
			}
			else
				item->Selected = false;
		}
		if (IsPointInClient(X, Y))
		{
			if (parentItem)
				parentItem->Selected = true;
			tmrHover.StartTimer();
		}
		else
		{
			tmrHover.StopTimer();
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
							PopupSubMenu(item->SubMenu, 0, Height + 2);
						}
					}
				}
			}
		}
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
				if (X >= item->Left && X <= item->Left + item->Width &&
					Y >= item->Top && Y <= item->Top + item->Height)
					item->DoMouseDown(X - item->Left, Y - item->Top, Shift);
			
		}
		return false;
	}
	bool Menu::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Container::DoMouseUp(X, Y, Shift);
		return false;
	}

	int Menu::GetSelectedItemID()
	{
		for (int i=0; i<Items.Count(); i++)
			if (Items[i]->Selected && Items[i]->Enabled && Items[i]->Visible && !Items[i]->IsSeperator())
				return i;
		return -1;
	}

	bool Menu::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		bool IsLastLevel = !(curSubMenu && curSubMenu->Visible);
		if (style == msMainMenu && Shift == SS_ALT)
		{
			if (Key == VK_MENU)
			{
				SetFocus();
				if (Items.Count())
					Items[0]->Selected = true;
				return false;
			}
			for (int i=0; i<Items.Count(); i++)
				Items[i]->Selected = false;
			for (int i=0; i<Items.Count(); i++)
				if (Items[i]->GetAccessKey() == Key)
				{
					Items[i]->DoClick();
					Items[i]->Selected = true;
					return false;
				}
			return false;
		}
		if (IsLastLevel && Enabled && Visible)
		{
			int id = GetSelectedItemID();
			if (style == msPopup)
			{
				if (Key == VK_UP || Key == VK_DOWN)
				{
					for (int i=0; i<Items.Count(); i++)
						Items[i]->Selected = false;
					
					if (Key == VK_DOWN)
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
					else if (Key == VK_UP)
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
					Container::DoKeyDown(Key, Shift);
				}
				if (Key == VK_RIGHT)
				{
					Container::DoKeyDown(Key, Shift);
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
				}
				else if (Key == VK_LEFT || Key == VK_ESCAPE)
				{
					if (parentItem && parentItem->Parent)
					{
						((Menu *)parentItem->Parent)->CloseSubMenu();
						Container::DoKeyDown(Key, Shift);
					}
					else if (Key == VK_ESCAPE)
					{
						CloseMenu();
						Container::DoKeyDown(Key, Shift);
					}
				}
				else if (Key == VK_RETURN)
				{
					Container::DoKeyDown(Key, Shift);
					if (id != -1)
						Items[id]->DoClick();
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
				}
				else
				{
					for (int i=0; i<Items.Count(); i++)
						if (Items[i]->GetAccessKey() == Key)
							Items[i]->DoClick();
				}
			}
			else
			{
				if (!Focused)
					return false;
				if (Key == VK_LEFT || Key == VK_RIGHT)
				{
					for (int i=0; i<Items.Count(); i++)
						Items[i]->Selected = false;
					
					if (Key == VK_RIGHT)
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
					else if (Key == VK_LEFT)
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
					Container::DoKeyDown(Key, Shift);
				}
				else if (Key == VK_RETURN || Key == VK_DOWN)
				{
					Container::DoKeyDown(Key, Shift);
					if (id != -1)
						Items[id]->DoClick();
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
				}
			}
		}
		else
			Container::DoKeyDown(Key, Shift);
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
		TabStop = true;
		isSeperator = true;
		parent->AddItem(this);
	}

	MenuItem::MenuItem(MenuItem * parent)
		: Container(parent->GetSubMenu()), accKey(0)
	{
		TabStop = true;
		isSeperator = true;
		parent->AddItem(this);
	}

	MenuItem::MenuItem(Menu * parent, const String & text, const String & shortcutText)
		: Container(parent)
	{
		TabStop = true;
		isSeperator = false;
		Init();
		SetText(text);
		lblShortcut->SetText(shortcutText);
		parent->AddItem(this);

	}

	MenuItem::MenuItem(MenuItem * parent, const String & text, const String & shortcutText)
		: Container(parent->GetSubMenu())
	{
		TabStop = true;
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
		lblText->SetText(text);
		int fid = text.IndexOf(L"&");
		if (fid != -1 && fid < text.Length())
			accKey = text[fid+1];
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
					lblShortcut->TextWidth + 8 + Margin*2 + rm;
					
			}
		}
		else
		{
			return lblText->TextWidth + Margin*2;
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
				((Menu *)Parent)->PopupSubMenu(SubMenu, Width - 2, 0);
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
			else
				DoMouseHover();

			/*if (Parent && SubMenu && SubMenu->Count())
			{
				Menu * mn = (Menu*)Parent;
				mn->PopupSubMenu(SubMenu.operator ->(), Left,Top+Height+2);
			}*/
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
				if (SubMenu && SubMenu->Visible)
				{
					Graphics::SolidBrushColor = Global::ColorTable.ToolButtonBackColorPressed1;
					Graphics::FillRectangle(entry->System, 0,0,width-1, height-1);
				}
				else
				{
					Graphics::SolidBrushColor = Global::ColorTable.ToolButtonBackColorHighlight1;
					Graphics::FillRectangle(entry->System, 0,0,width-1, height-1);
				}
				Graphics::PenColor = Global::ColorTable.ToolButtonBorderHighLight;
				Graphics::DrawRectangle(entry->System, 0,0,width-1,height-1);
				lblText->FontColor = Global::ColorTable.MenuItemHighlightForeColor;
			}
			else
			{
				if (Enabled)
					lblText->FontColor = Global::ColorTable.MenuItemForeColor;
				else
					lblText->FontColor = Global::ColorTable.MenuItemDisabledForeColor;
			}
			lblText->Draw((width-lblText->GetWidth())/2, 
				(height-lblText->GetHeight())/2);
			
		}
	}

	void MenuItem::DrawMenuItem(int width, int height)
	{
		auto entry = GetEntry();

		if (isSeperator)
		{
			Graphics::PenColor = Global::ColorTable.MenuItemDisabledForeColor;
			Graphics::DrawLine(entry->System, height + Margin, Height >> 1, width-2, Height >> 1);
		}
		else
		{
			if (Selected)
			{
				Graphics::SolidBrushColor = Global::ColorTable.ToolButtonBackColorHighlight1;
				Graphics::FillRoundRect(entry->System, Left, 0, Left+Width, Height, 5);
				//Graphics::DrawRoundRect(entry->System, Left, 0, Left+Width, Height, 5);
			}
			int top = (height - lblText->GetHeight()+2)/2;
			if (!Enabled)
			{
				lblText->FontColor = Global::ColorTable.MenuItemDisabledForeColor;
				lblShortcut->FontColor = Global::ColorTable.MenuItemDisabledForeColor;
			}
			else
			{
				if (Selected)
				{
					lblText->FontColor = Global::ColorTable.MenuItemHighlightForeColor;
					lblShortcut->FontColor = Global::ColorTable.MenuItemHighlightForeColor;
				}
				else
				{
					lblText->FontColor = Global::ColorTable.MenuItemForeColor;
					lblShortcut->FontColor = Global::ColorTable.MenuItemForeColor;
				}
			}
			lblText->Draw(height + Margin, top);
			lblShortcut->Draw(width - Margin-8-lblShortcut->GetWidth(), top);
			if (SubMenu && SubMenu->Count())
			{
				int x1 = Width - 12;
				int y1 = height/2 - 5;
				Array<Vec2, 3> polygon;
				polygon.Add(Vec2::Create((float)x1, (float)y1));
				polygon.Add(Vec2::Create((float)x1, (float)y1 + 10));
				polygon.Add(Vec2::Create((float)x1 + 5, (float)y1 + 5));
				entry->System->FillPolygon(lblText->FontColor, polygon.GetArrayView());
			}
			if (Checked)
			{
				// Draw Checkmark
				if (Selected)
					Graphics::SolidBrushColor = Global::ColorTable.ToolButtonBackColorPressed1;
				else
					Graphics::SolidBrushColor = Global::ColorTable.ToolButtonBackColorHighlight1;
				const int IconMargin = 2;
				Graphics::FillRectangle(entry->System, 0, 0, Height - IconMargin, Height-IconMargin);
				if (!Selected)
				{
					Graphics::PenColor = Global::ColorTable.ToolButtonBorderHighLight;
					Graphics::DrawRectangle(entry->System, IconMargin, IconMargin, Height - IconMargin, Height-IconMargin);
				}
				entry->CheckmarkLabel->FontColor = lblText->FontColor;
				entry->CheckmarkLabel->Draw((Height- entry->CheckmarkLabel->GetHeight())/2 + 2,
					(Height- entry->CheckmarkLabel->GetHeight())/2);
			}
			
		}
	}

	void MenuItem::HandleMessage(const UI_MsgArgs * Args)
	{
		Control::HandleMessage(Args);
	}

	bool MenuItem::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		bool vis = (((Menu*)Parent)->curSubMenu == SubMenu);
		Control::DoMouseDown(X,Y, Shift);
		if (IsPointInClient(X, Y))
		{
			Menu * mn = (Menu*)Parent;
			if (Parent && SubMenu && SubMenu->Count())
			{
				if (!vis)
				{
					if (isButton)
						mn->PopupSubMenu(SubMenu, 0, Height + 2);
					else
						mn->PopupSubMenu(SubMenu, Width - 2, 0);
				}
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
			image->Draw(absX, absY);
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
		if ((GetAsyncKeyState(VK_LBUTTON)<0 || GetAsyncKeyState(VK_RBUTTON)<0 || GetAsyncKeyState(VK_MBUTTON)<0))
			return false;
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
		int textWidth = 4 + lblText->GetWidth();
		if (ButtonStyle == bsNormal)
			return imgSize + Margin * 2 + (ShowText?textWidth:0);
		else if (ButtonStyle == bsDropDown)
			return DropDownButtonWidth;
		else
			return 3;
	}

	int ToolButton::MeasureHeight()
	{
		int imgSize = image?image->GetHeight():0;
		if (lblText->GetHeight() > imgSize)
			imgSize = lblText->GetHeight();
		return imgSize + Margin * 2;
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
		if (ButtonStyle == bsSeperator)
		{
			Graphics::PenColor = Global::ColorTable.ToolButtonSeperatorColor;
			Graphics::DrawLine(entry->System, absX+1,absY+1, absX+1, absY+Height-2);
			return;
		}
		bool drawbkg = true;
		if (Selected || Global::PointedComponent == this || Global::PointedComponent->IsChildOf(this))
		{
			if (Checked || Pressed)
			{
				Graphics::SolidBrushColor = Global::ColorTable.ToolButtonBackColorPressed1;
			}
			else
			{
				Graphics::SolidBrushColor = Global::ColorTable.ToolButtonBackColorHighlight1;
			}

		}
		else
		{
			if (Checked)
			{
				Graphics::SolidBrushColor = Global::ColorTable.ToolButtonBackColorChecked1;
			}
			else
			{
				drawbkg = false;
			}
		}
		if (drawbkg)
			Graphics::FillRectangle(entry->System, absX,absY,absX+Width-1,absY+Height-1);
		if (Selected || Checked)
		{
			Graphics::PenColor = Global::ColorTable.ToolButtonBorderHighLight;
			Graphics::DrawRectangle(entry->System, absX,absY,absX+Width-1,absY+Height-1);
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
					imgX += Margin;
					imgY += (Height-image->GetHeight())/2;
				}
			}
			if (Enabled)
			{
				if (image)
				{
					image->Draw(imgX,imgY);
				}
			}
			else
			{
				if (imageDisabled)
				{
					imageDisabled->Draw(imgX,imgY);
				}
			}
			if (ShowText)
			{
				int imgw = (image?image->GetWidth():0);
				lblText->Draw(imgX+imgw+Margin, absY + (Height-lblText->GetHeight())/2);
			}

		}
		else
		{
			Color color;
			if (Enabled)
				color = Color(0,0,0,255);
			else
				color = Global::ColorTable.ToolButtonSeperatorColor;
			Array<Vec2, 3> polygon;
			polygon.Add(Vec2::Create((float)absX + 3, (float)absY + 10));
			polygon.Add(Vec2::Create((float)absX + 7, (float)absY + 10));
			polygon.Add(Vec2::Create((float)absX + 5, (float)absY + 12));
			entry->System->FillPolygon(color, polygon.GetArrayView());
		}
				
	}

	ToolStrip::ToolStrip(Container * parent)
		: Container(parent)
	{
		DockStyle = dsTop;
		MultiLine = false;
		FullLineFill = true;
		ShowText = false;
		Orientation = Horizontal;
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
		int left = LeftMargin;
		if (Orientation == Horizontal)
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
					buttons[i]->Posit(left, TopMargin, buttons[i]->MeasureWidth(), maxH);
					left += buttons[i]->GetWidth();
				}
				left += LeftMargin;
				Width = left;
				Height = maxH + TopMargin * 2;
			}
		}
		else
		{
			int maxW = 0, maxH = 0;
			int top = TopMargin;
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
			top += TopMargin;
			Height = top;
		}
	}

	void ToolStrip::Draw(int absX, int absY)
	{
		Graphics::SolidBrushColor = Global::ColorTable.ToolButtonBackColor1;
		Graphics::FillRectangle(GetEntry()->System, absX+Left,absY+Top,absX+Left+Width-1,absY+Top+Height-1);
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
		parent->AddItem(this);
	}

	void StatusPanel::Init()
	{
		BackColor = Color(0,0,0,0);
		BorderStyle = BS_NONE;
		FillMode = Fixed;
		Width = 50;
		DockStyle = dsBottom;
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
		text->Draw(absX+Left, absY+Top);
	}
		
	StatusStrip::StatusStrip(Container * parent)
		: Container(parent)
	{
		LeftMargin = 8;
		TopMargin = 6;
	}

	void StatusStrip::AddItem(GraphicsUI::StatusPanel *pannel)
	{
		panels.Add(pannel);
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
		int w = Width-LeftMargin;
		for (int i=0; i<panels.Count(); i++)
		{
			int cw = panels[i]->MeasureWidth();
			if (cw!=-1)
				w -= cw;
			else
				fc ++;
		}
		w-=LeftMargin;
		if (fc == 0)
			fc = 1;
		int fw = w/fc;
		int h = Height - TopMargin*2;
		int left = LeftMargin;
		for (int i=0; i<panels.Count(); i++)
		{
			int cw = panels[i]->MeasureWidth();
			if (cw != -1)
			{
				panels[i]->Posit(left, TopMargin, cw, h);
				left += cw;
			}
			else
			{
				panels[i]->Posit(left, TopMargin, fw, h);
				left += fw;
			}
		}
	}

	void StatusStrip::Draw(int absX, int absY)
	{
		absX += Left;
		absY += Top;
		PositItems();
		Graphics::SolidBrushColor = Global::ColorTable.StatusStripBackColor1;
		Graphics::FillRectangle(GetEntry()->System, absX, absY, absX+Width, absY+Height);
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
		BorderStyle = BS_NONE;
		parent->AddItem(this);
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
			return (image?image->GetWidth():0)+LeftMargin*2;
		case TabControl::tsText:
			return text->GetWidth() + LeftMargin * 2;
		case TabControl::tsTextImage:
			return text->GetWidth() + (image?image->GetWidth()+LeftMargin:0) + LeftMargin * 2;
		default:
			return 0;
		}
	}

	int TabPage::MeasureHeight(TabControl::_TabStyle style)
	{
		switch (style)
		{
		case TabControl::tsImage:
			return (image?image->GetHeight():0)+TopMargin*2;
		case TabControl::tsText:
			return text->GetHeight() + TopMargin * 2;
		case TabControl::tsTextImage:
			return max(text->GetHeight(), (image?image->GetHeight():0))+ TopMargin * 2;
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

	void TabPage::DrawHeader(int x, int y, int h, TabControl::_TabStyle style)
	{
		switch (style)
		{
		case TabControl::tsTextImage:
			{
				int cw = x + LeftMargin;
				if (image)
				{
					image->Draw(cw, y+TopMargin);
					cw += image->GetWidth() + LeftMargin;
				}
				text->Draw(cw, y + (h-text->GetHeight())/2);
			}
			break;
		case TabControl::tsText:
			{
				text->Draw(x+LeftMargin, y+TopMargin);
			}
			break;
		case TabControl::tsImage:
			{
				if (image)
					image->Draw(x+LeftMargin, y+TopMargin);
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
		return h;
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
				int pw = pages[i]->MeasureWidth(TabStyle);
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
		Container::SizeChanged();
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

	void TabControl::Draw(int absX, int absY)
	{
		SetClient();
		headerHeight = MeasureHeight();
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
		entry->System->SetRenderTransform(absX, absY);
		int h0 = Height-headerHeight-1;
		for (int i=0; i<pages.Count(); i++)
		{
			int pw = pages[i]->MeasureWidth(TabStyle);
			if (cw + pw > maxWidth)
				break;
			if (SelectedIndex != i && highlightItem != i)
			{
				Graphics::SolidBrushColor = Global::ColorTable.TabPageItemBackColor1;
			}
			else if (SelectedIndex == i)
			{
				Graphics::SolidBrushColor = Global::ColorTable.TabPageItemSelectedBackColor1;
			}
			else
			{
				Graphics::SolidBrushColor = Global::ColorTable.TabPageItemHighlightBackColor1;
			}
			Graphics::PenColor = Global::ColorTable.TabPageBorderColor;
			if (TabPosition == tpTop)
			{
				Graphics::FillRectangle(entry->System, cw, 0, cw+pw, headerHeight);
				Graphics::DrawLine(entry->System, cw,0,cw+pw,0);
				Graphics::DrawLine(entry->System, cw,0, cw, headerHeight);
				Graphics::DrawLine(entry->System, cw+pw,0, cw+pw, headerHeight);
				if (SelectedIndex != i)
				{
					Graphics::DrawLine(entry->System, cw, headerHeight, cw+pw, headerHeight);
				}
				pages[i]->DrawHeader(cw, 0, headerHeight, TabStyle);
			}
			else
			{
				Graphics::FillRectangle(entry->System, cw, h0+headerHeight, cw+pw, h0);
				Graphics::DrawLine(entry->System, cw,h0, cw, h0+headerHeight);
				Graphics::DrawLine(entry->System, cw+pw,h0, cw+pw, h0+headerHeight);
				Graphics::DrawLine(entry->System, cw, h0+headerHeight, cw+pw, h0+headerHeight);
				if (SelectedIndex != i)
				{
					Graphics::DrawLine(entry->System, cw,h0,cw+pw,h0);
				}
				pages[i]->DrawHeader(cw, h0, headerHeight, TabStyle);
			}
				
			cw += pw;
		}
			
		if (TabPosition == tpTop)
		{
			Graphics::DrawLine(entry->System, cw,headerHeight, Width, headerHeight);
			Graphics::DrawLine(entry->System, 0,headerHeight,0,Height-1);
			Graphics::DrawLine(entry->System, Width-1,headerHeight,Width-1,Height-1);
			Graphics::DrawLine(entry->System, 0,Height-1,Width,Height-1);
		}
		else
		{
			Graphics::DrawLine(entry->System, cw,h0, Width, h0);
			Graphics::DrawLine(entry->System, 0,0,0,Height-headerHeight);
			Graphics::DrawLine(entry->System, Width-1,0,Width-1,Height-1-headerHeight);
			Graphics::DrawLine(entry->System, 0,0,Width,0);
		}
		entry->System->SetRenderTransform(0, 0);
			
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
		tmrHover.OnTick.Bind(this, &UpDown::tmrHoverTick);
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
		tmrHoverTick(this, WinForm::EventArgs());
		tmrHover.Interval = 500;
		tmrHover.StartTimer();
		Global::MouseCaptureControl = this;
		return false;
	}

	bool UpDown::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X,Y,Shift);
		state = 0;
		tmrHover.StopTimer();
		ReleaseMouse();
		return false;
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

	void UpDown::tmrHoverTick(Object *, WinForm::EventArgs)
	{
		float val = (float)StringToDouble(text->GetText());
		if (state == 1)
			val += inc;
		else
			val -= inc;
		val = max(Min,val);
		val = min(Max,val);
		text->SetText(String(val, (L"%." + String(Digits) + L"f").Buffer()));
		tmrHover.Interval = 50;
		tmrHover.StartTimer();
	}

	UIEntry * Control::GetEntry()
	{
		Control * parent = Parent;
		if (parent)
			return parent->GetEntry();
		return nullptr;
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
		BorderStyle = content->BorderStyle = BS_NONE;
		BackColor.A = content->BackColor.A = 0;
	}
	void VScrollPanel::SizeChanged()
	{
		int maxY = 0;
		for (auto & ctrl : content->Controls)
			maxY = Math::Max(ctrl->Top + ctrl->GetHeight(), maxY);
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
	void VScrollPanel::ClearChildren()
	{
		for (auto & child : content->Controls)
			child = nullptr;
		content->Controls.Clear();
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
		Graphics::PenColor = BorderColor;
		Graphics::DrawLine(GetEntry()->System, absX + Left, absY + Top, absX + Left + Width, absY + Top + Height);
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
