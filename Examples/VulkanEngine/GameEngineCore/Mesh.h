#ifndef REALTIME_ENGINE_MESH_H
#define REALTIME_ENGINE_MESH_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "CoreLib/Graphics/BBox.h"
#include "CoreLib/LibIO.h"

namespace GameEngine
{
	class MeshVertexFormat
	{
	private:
		int numColors = 0;
		int numUVs = 0;
		bool hasTangent = false;
		bool hasSkinning = false;
		int vertSize = 12;
		CoreLib::String shaderDef;
		int CalcVertexSize()
		{
			return 3 * sizeof(float) + (numColors + numUVs) * sizeof(unsigned int) + (hasTangent ? 4 : 0) + (hasSkinning ? 8 : 0);
		}
	public:
		MeshVertexFormat() {}
		MeshVertexFormat(int typeId);
		MeshVertexFormat(int colorChannels, int uvChannels, bool pHasTangent, bool pHasSkinning)
		{
			numColors = colorChannels;
			numUVs = uvChannels;
			hasTangent = pHasTangent;
			hasSkinning = pHasSkinning;
			vertSize = CalcVertexSize();
		}
		CoreLib::String GetShaderDefinition();
		int GetVertexSize()
		{
			return vertSize;
		}
		int GetColorChannelCount() { return numColors; }
		int GetUVChannelCount() { return numUVs; }
		bool HasTangent() { return hasTangent; }
		bool HasSkinning() { return hasSkinning; };
		int GetPositionOffset() { return 0; }
		int GetUVOffset(int channelId) { return 12 + channelId * sizeof(unsigned int); }
		int GetTangentFrameOffset() { return 12 + numUVs * sizeof(unsigned int); }
		int GetColorOffset(int channelId) { return 12 + numUVs * sizeof(unsigned int) + (hasTangent ? 4 : 0) + channelId * sizeof(unsigned int); }
		int GetBoneIdsOffset()
		{
			return 12 + numUVs * sizeof(unsigned int) + (hasTangent ? 4 : 0) + numColors * sizeof(unsigned int);
		}
		int GetBoneWeightsOffset()
		{
			return GetBoneIdsOffset() + 4;
		}
		
		int GetTypeId();
	};

	class Skeleton;

	class Mesh : public CoreLib::Object 
	{
	private:
		MeshVertexFormat vertexFormat;
		CoreLib::Basic::List<unsigned char> vertexData;
		int vertCount = 0;
	public:
		CoreLib::Graphics::BBox Bounds;
		CoreLib::Basic::List<int> Indices;
		MeshVertexFormat GetVertexFormat() { return vertexFormat; }
		void SetVertexFormat(const MeshVertexFormat & value) { vertexFormat = value; }
		void SetVertexPosition(int vertId, const VectorMath::Vec3 & pos)
		{
			*(VectorMath::Vec3*)((unsigned char *)vertexData.Buffer() + vertId * vertexFormat.GetVertexSize()) = pos;
		}
		VectorMath::Vec3 GetVertexPosition(int vertId)
		{
			return *(VectorMath::Vec3*)((unsigned char *)vertexData.Buffer() + vertId * vertexFormat.GetVertexSize());
		}
		void SetVertexUV(int vertId, int channelId, const VectorMath::Vec2 & uv)
		{
			auto destUV = (unsigned short*)((unsigned char *)vertexData.Buffer() + vertId * vertexFormat.GetVertexSize() + vertexFormat.GetUVOffset(channelId));
			destUV[0] = CoreLib::FloatToHalf(uv.x);
			destUV[1] = CoreLib::FloatToHalf(uv.y);
		}
		VectorMath::Vec2 GetVertexUV(int vertId, int channelId)
		{
			auto destUV = (unsigned short*)((unsigned char *)vertexData.Buffer() + vertId * vertexFormat.GetVertexSize() + vertexFormat.GetUVOffset(channelId));
			return VectorMath::Vec2::Create(CoreLib::HalfToFloat(destUV[0]), CoreLib::HalfToFloat(destUV[1]));
		}
		void SetVertexTangentFrame(int vertId, const VectorMath::Quaternion & vq)
		{
			unsigned char packedQ[4];
			packedQ[0] = (unsigned char)CoreLib::Math::Clamp((int)((vq.x + 1.0f) * 255.0f * 0.5f), 0, 255);
			packedQ[1] = (unsigned char)CoreLib::Math::Clamp((int)((vq.y + 1.0f) * 255.0f * 0.5f), 0, 255);
			packedQ[2] = (unsigned char)CoreLib::Math::Clamp((int)((vq.z + 1.0f) * 255.0f * 0.5f), 0, 255);
			packedQ[3] = (unsigned char)CoreLib::Math::Clamp((int)((vq.w + 1.0f) * 255.0f * 0.5f), 0, 255);
			*(unsigned int*)((unsigned char *)vertexData.Buffer() + vertId * vertexFormat.GetVertexSize() + vertexFormat.GetTangentFrameOffset()) = packedQ[0] + (packedQ[1] << 8) + (packedQ[2] << 16) + (packedQ[3] << 24);
		}
		VectorMath::Quaternion GetVertexTangentFrame(int vertId)
		{
			unsigned int quat = *(unsigned int*)((unsigned char *)vertexData.Buffer() + vertId * vertexFormat.GetVertexSize() + vertexFormat.GetTangentFrameOffset());
			VectorMath::Quaternion result;
			result.x = (quat & 255) * (2.0f / 255.0f) - 1.0f;
			result.y = ((quat >> 8) & 255) * (2.0f / 255.0f) - 1.0f;
			result.z = ((quat >> 16) & 255) * (2.0f / 255.0f) - 1.0f;
			result.w = ((quat >> 24) & 255) * (2.0f / 255.0f) - 1.0f;
			return result;
		}
		void SetVertexColor(int vertId, int channelId, const VectorMath::Vec4 & color)
		{
			unsigned char packedQ[4];
			packedQ[0] = (unsigned char)CoreLib::Math::Clamp((int)((color.x) * 255.0f), 0, 255);
			packedQ[1] = (unsigned char)CoreLib::Math::Clamp((int)((color.y) * 255.0f), 0, 255);
			packedQ[2] = (unsigned char)CoreLib::Math::Clamp((int)((color.z) * 255.0f), 0, 255);
			packedQ[3] = (unsigned char)CoreLib::Math::Clamp((int)((color.w) * 255.0f), 0, 255);
			*(unsigned int*)((unsigned char *)vertexData.Buffer() + vertId * vertexFormat.GetVertexSize() + vertexFormat.GetColorOffset(channelId)) = packedQ[0] + (packedQ[1] << 8) + (packedQ[2] << 16) + (packedQ[3] << 24);
		}
		VectorMath::Vec4 GetVertexColor(int vertId, int channelId)
		{
			unsigned int quat = *(unsigned int*)((unsigned char *)vertexData.Buffer() + vertId * vertexFormat.GetVertexSize() + vertexFormat.GetColorOffset(channelId));
			VectorMath::Vec4 result;
			result.x = (quat & 255) * (1.0f / 255.0f);
			result.y = ((quat >> 8) & 255) * (1.0f / 255.0f);
			result.z = ((quat >> 16) & 255) * (1.0f / 255.0f);
			result.w = ((quat >> 24) & 255) * (1.0f / 255.0f);
			return result;
		}
		void GetVertexSkinningBinding(int vertId, CoreLib::Array<int, 8> & boneIds, CoreLib::Array<float, 8> & boneWeights)
		{
			unsigned int vBoneIds = *(unsigned int*)((unsigned char *)vertexData.Buffer() + vertId * vertexFormat.GetVertexSize() + vertexFormat.GetBoneIdsOffset());
			unsigned int vBoneWeights = *(unsigned int*)((unsigned char *)vertexData.Buffer() + vertId * vertexFormat.GetVertexSize() + vertexFormat.GetBoneWeightsOffset());
			boneIds.Clear();
			boneWeights.Clear();
			for (int i = 0; i < 4; i++)
			{
				int id = (int)((vBoneIds >> (8 * i)) & 255);
				if (id != 255)
				{
					float weight = ((vBoneWeights >> (8 * i)) & 255) * (1.0f / 255.0f);
					boneIds.Add(id);
					boneWeights.Add(weight);
				}
			}
		}
		void SetVertexSkinningBinding(int vertId, const CoreLib::ArrayView<int> & boneIds, const CoreLib::ArrayView<float> & boneWeights)
		{
			unsigned int & vBoneIds = *(unsigned int*)((unsigned char *)vertexData.Buffer() + vertId * vertexFormat.GetVertexSize() + vertexFormat.GetBoneIdsOffset());
			unsigned int & vBoneWeights = *(unsigned int*)((unsigned char *)vertexData.Buffer() + vertId * vertexFormat.GetVertexSize() + vertexFormat.GetBoneWeightsOffset());
			unsigned char cBoneIds[4], cWeights[4];
			for (int i = 0; i < CoreLib::Math::Min(4, boneIds.Count()); i++)
			{
				cBoneIds[i] = boneIds[i] == -1 ? 255 : (unsigned char)boneIds[i];
				cWeights[i] = (unsigned char)CoreLib::Math::Clamp((int)(boneWeights[i] * 255.0f), 0, 255);
			}
			vBoneIds = (unsigned int)(cBoneIds[0] + (cBoneIds[1] << 8) + (cBoneIds[2] << 16) + (cBoneIds[3] << 24));
			vBoneWeights = (unsigned int)(cWeights[0] + (cWeights[1] << 8) + (cWeights[2] << 16) + (cWeights[3] << 24));
		}
		int GetVertexSize() { return vertexFormat.GetVertexSize(); }
		void * GetVertexBuffer() { return vertexData.Buffer(); }
		int GetVertexCount() { return vertCount; }
		int GetVertexTypeId() { return vertexFormat.GetTypeId(); }
		void AllocVertexBuffer(int numVerts) 
		{
			vertexData.SetSize(vertexFormat.GetVertexSize() * numVerts);
			vertCount = numVerts;
		}
		void SaveToStream(CoreLib::IO::Stream * stream);
		void SaveToFile(const CoreLib::String & fileName);
		void LoadFromStream(CoreLib::IO::Stream * stream);
		void LoadFromFile(const CoreLib::String & fileName);
		void FromSkeleton(Skeleton * skeleton, float width);
	};

}

#endif
