#include "Scene.h"
#include "CoreLib/LibMath.h"
#include "CoreLib/Imaging/Bitmap.h"

using namespace CoreLib::Imaging;
using namespace GL;

namespace RealtimeEngine
{
	float TerrainGeometry::GetAltitude(const Vec3 & v)
	{
		if (!heightMapData.Count())
			return -500.0f;
		float fi = (v.z / cellSpace) + heightMapSize / 2;
		float fj = (v.x / cellSpace) + heightMapSize / 2;
		int i = (int)floor(fi);
		int j = (int)floor(fj);
		if (i < 0 || i >= heightMapSize)
			return 0.0f;
		if (j < 0 || j >= heightMapSize)
			return 0.0f;
		float tu = fi - (float)i;
		float tv = fj - (float)j;
		if (i == heightMapSize - 1 || j == heightMapSize - 1)
			return heightMapData[i * heightMapSize + j];
		else
			return (heightMapData[i*heightMapSize + j] * (1.0f - tu) + heightMapData[(i + 1)*heightMapSize + j] * tu) * (1.0f - tv) +
			(heightMapData[i*heightMapSize + j + 1] * (1.0f - tu) + heightMapData[(i + 1)*heightMapSize + j + 1] * tu) * tv;
	}

	void TerrainGeometry::Init(GL::HardwareRenderer * renderer, float pCellSpace, float heightScale, String heightMap)
	{
		cellSpace = pCellSpace;
		BitmapF bmp(heightMap);
		List<VertexData> vertices;
		heightMapSize = bmp.GetWidth();

		int maxLod = 5;
		int blockSize = 128;

		heightMapData.SetSize(bmp.GetWidth()*bmp.GetHeight());
		for (int i = 0; i < bmp.GetHeight(); i++)
		{
			float z = (i - (bmp.GetHeight() >> 1)) * cellSpace;
			for (int j = 0; j < bmp.GetWidth(); j++)
			{
				float x = (j - (bmp.GetWidth() >> 1)) * cellSpace;
				float y = bmp.GetPixels()[i*bmp.GetWidth() + j].x * heightScale;
				heightMapData[i*bmp.GetWidth() + j] = y;
				VertexData vert;
				vert.Position = Vec3::Create(x, y, z);
				vert.UV.x = (float)(j%blockSize) / (float)blockSize;
				vert.UV.y = (float)(i%blockSize) / (float)blockSize;
				vert.Tangent = Vec3::Create(1.0f, 0.0f, 0.0f);
				vert.Normal = Vec3::Create(0.0f, 1.0f, 0.0f);
				vertices.Add(vert);
			}
		}
		for (int i = 0; i < bmp.GetHeight() - 1; i++)
		{
			for (int j = 0; j < bmp.GetWidth() - 1; j++)
			{
				Vec3 a = vertices[i*bmp.GetWidth() + j + 1].Position
					- vertices[i*bmp.GetWidth() + j].Position;
				Vec3 b = vertices[(i + 1)*bmp.GetWidth() + j].Position
					- vertices[i*bmp.GetWidth() + j].Position;
				Vec3 norm;
				Vec3::Cross(norm, b, a);
				Vec3::Normalize(norm, norm);
				vertices[i*bmp.GetWidth() + j].Normal = norm;
			}
		}
		List<Vec3> newNormal;
		newNormal.SetSize(vertices.Count());
		for (int i = 1; i < bmp.GetHeight() - 1; i++)
		{
			for (int j = 1; j < bmp.GetWidth() - 1; j++)
			{
				Vec3 normal;
				normal.SetZero();
				normal += vertices[i*bmp.GetWidth() + j].Normal;
				normal += vertices[(i + 1)*bmp.GetWidth() + j].Normal;
				normal += vertices[i*bmp.GetWidth() + j + 1].Normal;
				normal += vertices[(i + 1)*bmp.GetWidth() + j + 1].Normal;
				normal *= 0.2f;
				newNormal[i * bmp.GetWidth() + j] = normal;
			}
		}
		for (int i = 1; i < bmp.GetHeight() - 1; i++)
		{
			for (int j = 1; j < bmp.GetWidth() - 1; j++)
			{
				vertices[i*bmp.GetWidth() + j].Normal = newNormal[i*bmp.GetWidth() + j];
			}
		}
		Free(renderer);
		List<int> indexCounts;

		for (int l = 1; l <= maxLod; l++)
		{
			List<int> indexBuffer;
			int step = 1 << (l - 1);
			int rowSize = blockSize / step;
			int stride = rowSize + 1;
			for (int y = 0; y < rowSize; y++)
				for (int x = 0; x < rowSize; x++)
				{
					indexBuffer.Add(y*stride + x);
					indexBuffer.Add((y + 1)*stride + x);
					indexBuffer.Add((y + 1)*stride + x + 1);

					indexBuffer.Add(y*stride + x);
					indexBuffer.Add((y + 1)*stride + x + 1);
					indexBuffer.Add(y*stride + x + 1);
				}
			if (step >= 2)
			{
				int vertexBase = stride*stride;
				for (int edge = 0; edge < 4; edge++)
				{
					for (int k = 0; k < blockSize / step; k++)
					{
						indexBuffer.Add(vertexBase++);
						indexBuffer.Add(vertexBase++);
						indexBuffer.Add(vertexBase++);

					}
				}
			}
			indexCounts.Add(indexBuffer.Count());
			auto gpuIndexBuffer = renderer->CreateBuffer(BufferUsage::IndexBuffer);
			indexBuffers.Add(gpuIndexBuffer);
			gpuIndexBuffer.SetData(indexBuffer.Buffer(), indexBuffer.Count() * sizeof(int));
			gpuIndexBuffer.MakeResident(true);
		}

		for (int i = 0; i < bmp.GetHeight() - 1; i += blockSize)
		{
			for (int j = 0; j < bmp.GetWidth() - 1; j += blockSize)
			{
				TerrainBlock block;
				block.Bounds.Init();
				block.UV0 = Vec2::Create(j / (float)bmp.GetWidth(), i / (float)bmp.GetHeight());
				block.UVSize = Vec2::Create(blockSize / (float)bmp.GetWidth(), blockSize / (float)bmp.GetHeight());
				for (int l = 1; l <= maxLod; l++)
				{
					RefPtr<DeviceMesh> mesh = new DeviceMesh(renderer);
					mesh->IndexBuffer = &indexBuffers[l - 1];
					mesh->IndexCount = indexCounts[l - 1];
					mesh->Attributes.Clear();
					mesh->Attributes.Add(VertexAttribute(L"vert_pos", VertexDataType::Float, 3, false));
					mesh->Attributes.Add(VertexAttribute(L"vert_normal", VertexDataType::Float, 3, false));
					mesh->Attributes.Add(VertexAttribute(L"vert_tangent", VertexDataType::Float, 3, false));
					mesh->Attributes.Add(VertexAttribute(L"vert_uv", VertexDataType::Float, 2, false));

					List<unsigned char> vertexBuffer;

					int step = 1 << (l - 1);
					for (int y = 0; y <= blockSize; y += step)
						for (int x = 0; x <= blockSize; x += step)
						{
							int ix0 = j + x;
							int iy0 = i + y;
							if (ix0 == bmp.GetWidth()) ix0 -= 1;
							if (iy0 == bmp.GetHeight()) iy0 -= 1;
							auto & vert0 = vertices[iy0*bmp.GetWidth() + ix0];
							vert0.UV = Vec2::Create(x / (float)blockSize, y / (float)blockSize);
							block.Bounds.Union(vert0.Position);
							vertexBuffer.AddRange((unsigned char*)&vert0, sizeof(VertexData));
						}


					if (step >= 2)
					{
						for (int edge = 0; edge < 4; edge++)
						{
							int dx = 0;
							int dy = 0;
							int x0 = 0, y0 = 0;
							switch (edge)
							{
							case 0:
								dx = 1;	dy = 0;
								x0 = j; y0 = i;
								break;
							case 1:
								dx = 0;	dy = 1;
								x0 = Math::Min(bmp.GetWidth() - 1, j + blockSize); y0 = i;
								break;
							case 2:
								dx = -1;
								dy = 0;
								x0 = Math::Min(bmp.GetWidth() - 1, j + blockSize);
								y0 = Math::Min(bmp.GetHeight() - 1, i + blockSize);
								break;
							default:
								dx = 0;
								dy = -1;
								x0 = j;
								y0 = Math::Min(bmp.GetHeight() - 1, i + blockSize);
								break;
							}
							for (int k = 0; k < blockSize / step; k++)
							{
								int idx[3][2] = {
									{ x0 + dx * k * step, y0 + dy * k * step },
									{ x0 + dx * (k + 1) * step, y0 + dy*(k + 1)*step },
									{ x0 + dx * (k)* step + dx * step / 2, y0 + dy*k*step + dy *step / 2 },
								};
								for (int m = 0; m < 3; m++)
								{
									idx[m][0] = Math::Clamp(idx[m][0], 0, bmp.GetWidth() - 1);
									idx[m][1] = Math::Clamp(idx[m][1], 0, bmp.GetHeight() - 1);
									auto vert0 = vertices[idx[m][1] * bmp.GetWidth() + idx[m][0]];
									vertexBuffer.AddRange((unsigned char*)&vert0, sizeof(VertexData));
								}
							}
						}
					}
					mesh->MeshBuffer.SetData(vertexBuffer.Buffer(), vertexBuffer.Count());
					mesh->MeshBuffer.MakeResident(true);
					mesh->BufferSize = vertexBuffer.Count();
					block.RangeLods.Add(mesh);
				}
				Blocks.Add(block);
			}
		}

	}
}