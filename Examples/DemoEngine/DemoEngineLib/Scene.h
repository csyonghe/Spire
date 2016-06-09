#ifndef REALTIME_ENGINE_SCENE_H
#define REALTIME_ENGINE_SCENE_H

#include "CoreLib/Basic.h"
#include "CoreLib/LibGL.h"
#include "DeviceMarshalling.h"

namespace RealtimeEngine
{
	struct ShadingTechnique : public Object
	{
		float LodDistance = 0.0f;
		int ShaderId = 0;
		RefPtr<DeviceMesh> SourceMesh, PrebakedMesh;
		List<GL::Texture2D> ObjSpaceTextures;
		GL::RenderBuffer DepthRenderBuffer;
		GL::FrameBuffer ObjSpaceFBO;
		int LowResRenderTargetMask = 0;
		BufferRange UniformRange, ModelUniformRange;
		BufferRange PrecomputeTexUniformRange, RootTexUniformRange, ObjTexUniformRange, LowResUniformRange, PrecomputeUniformRange;
		ShadingTechnique() = default;
		ShadingTechnique(int shaderId, float distance = 0.0f)
		{
			this->ShaderId = shaderId;
			this->LodDistance = distance;
		}
		void FreeObjectSpaceTextures(GL::HardwareRenderer * renderer)
		{
			for (auto & tex : ObjSpaceTextures)
				renderer->DestroyTexture(tex);
			ObjSpaceTextures.Clear();
			if (DepthRenderBuffer.Handle)
				renderer->DestroyRenderBuffer(DepthRenderBuffer);
			if (ObjSpaceFBO.Handle)
				renderer->DestroyFrameBuffer(ObjSpaceFBO);
		}
		void Free(GL::HardwareRenderer * renderer)
		{
			FreeObjectSpaceTextures(renderer);
		}
	};
	class DrawableInstance
	{
	public:
		List<ShadingTechnique> Techniques;
		Vec3 Position;
		CoreLib::Graphics::BBox Bounds;
		String GUID;
		EnumerableDictionary<String, UniformValue> Uniforms; // uniforms from input file
		void Free(GL::HardwareRenderer * renderer)
		{
			for (auto & tech : Techniques)
			{
				tech.Free(renderer);
				tech.SourceMesh = nullptr;
				tech.PrebakedMesh = nullptr;
			}
		}
	};
	class VertexData
	{
	public:
		Vec3 Position;
		Vec3 Normal;
		Vec3 Tangent;
		Vec2 UV;
	};

	class TerrainBlock
	{
	public:
		CoreLib::Graphics::BBox Bounds;
		List<RefPtr<DeviceMesh>> RangeLods;
		Vec2 UV0, UVSize;
	};

	class TerrainGeometry
	{
	private:
		List<float> heightMapData;
		int heightMapSize;
		float cellSpace = 0.0f;
		List<GL::BufferObject> indexBuffers;
	public:
		List<TerrainBlock> Blocks;
		void Init(GL::HardwareRenderer * hw, float cellSpace, float heightScale, String heightMap);
		float GetAltitude(const Vec3 & v);
		void Free(GL::HardwareRenderer * hw)
		{
			if (indexBuffers.Count())
			{
				for (auto & idxBuf : indexBuffers)
					hw->DestroyBuffer(idxBuf);
				indexBuffers.Clear();
			}
		}
	};
}


#endif