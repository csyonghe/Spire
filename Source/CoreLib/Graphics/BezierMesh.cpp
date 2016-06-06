#include "BezierMesh.h"
#include "ObjModel.h"
#include <algorithm>

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
