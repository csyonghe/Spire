#ifndef REALTIME_ENGINE_RENDER_PASS_PROVIDER_H
#define REALTIME_ENGINE_RENDER_PASS_PROVIDER_H

#include "CoreLib/Basic.h"
#include "CoreLib/LibGL.h"
#include "GpuShaderStore.h"

namespace SpireLib
{
	class ShaderLib;
}

namespace RealtimeEngine
{
	struct ShadingTechnique;
	struct SizeI { int Width, Height; };
	class RenderPassProvider : public CoreLib::Object
	{
	public:
		virtual GL::Program LoadProgram(GpuShaderStore & shaderStore, const SpireLib::ShaderLib & shaderLib) = 0;
		virtual GL::FrameBuffer GetFramebuffer() = 0;
		virtual GL::Texture2D GetResultTexture() = 0;
		virtual SizeI GetViewport() = 0;
		virtual int GetRenderTargetMask(const ShadingTechnique & tech) = 0;
		virtual void SetRenderState(GL::HardwareRenderer * renderer) = 0;
		virtual void FrameResized() = 0;
	};

	class ShadowMapAlgorithm;
	class DeviceResourcePool;

	RenderPassProvider * CreateMainRenderPass(DeviceResourcePool * resourcePool, ShadowMapAlgorithm * shadowMap);
	RenderPassProvider * CreateLowQualityRenderPass(DeviceResourcePool * resourcePool, ShadowMapAlgorithm * shadowMap);
	RenderPassProvider * CreateLowResRenderPass(DeviceResourcePool * resourcePool, ShadowMapAlgorithm * shadowMap);
}
#endif