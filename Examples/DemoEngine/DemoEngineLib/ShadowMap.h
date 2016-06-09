#ifndef SHADOW_MAP_ALGORITHM_H
#define SHADOW_MAP_ALGORITHM_H

#include "DeviceResourcePool.h"
#include "CoreLib/Graphics.h"
#include "RenderPassProvider.h"

namespace DemoEngine
{
	class ShadowMapAlgorithm : public Object
	{
	public:
		CoreLib::Procedure<const VectorMath::Matrix4 &, int> DrawShadowMap;
		bool Enabled = true;

		virtual void BindShadowMapResources() = 0;
		virtual void BindShadowMapResources(GL::CommandBuffer & cmdBuffer) = 0;
		virtual void GenerateShadowMap(const Vec3 & lightDir, const CoreLib::Graphics::BBox & sceneBounds, const CoreLib::Graphics::ViewFrustum & viewFrustum) = 0;
		virtual void SetShadowMapSize(int size) = 0;
		virtual List<RefPtr<RenderPassProvider>> GetRenderPasses() = 0;
		virtual String GetShaderDefinition() = 0;
	};
	ShadowMapAlgorithm * CreateCascadedShadowMapAlgorithm(DeviceResourcePool * resourcePool);
}
#endif