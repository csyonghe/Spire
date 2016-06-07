#include "ObjModel.h"
#include "../LibIO.h"
#include "../SecureCRT.h"
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
