#include "RenderPassProvider.h"
#include "ShadowMap.h"
#include "Spire.h"
#include "Scene.h"
#include "DeviceResourcePool.h"
using namespace GL;

namespace RealtimeEngine
{
	extern const wchar_t * fsEpilog;
	class LowResRenderPassProvider : public RenderPassProvider
	{
	private:
		ShadowMapAlgorithm *shadowMap = nullptr;
		DeviceResourcePool * resourcePool = nullptr;
		GL::FrameBuffer frameBuffer;
	public:
		LowResRenderPassProvider(DeviceResourcePool * pResourcePool, ShadowMapAlgorithm *pShadowMap)
			: resourcePool(pResourcePool), shadowMap(pShadowMap)
		{
			frameBuffer = resourcePool->GetHardwareRenderer()->CreateFrameBuffer();
		}

		~LowResRenderPassProvider()
		{
			resourcePool->GetHardwareRenderer()->DestroyFrameBuffer(frameBuffer);
		}

		virtual GL::Program LoadProgram(GpuShaderStore & shaderStore, const SpireLib::ShaderLib & shaderLib) override
		{
			if (shaderLib.Sources.ContainsKey(L"lowResVs"))
			{
				auto additionalFS = shadowMap->GetShaderDefinition();
				if (!shaderLib.Sources[L"lowRes"]().MainCode.Contains(L"computeShadow"))
					additionalFS = L"";
				String fsSrc;
				auto & fs = shaderLib.Sources[L"lowRes"]();
				if (fs.ComponentAccessNames.ContainsKey(L"opacity"))
					fsSrc = shaderLib.Sources[L"lowRes"]().GetAllCodeGLSL(L"", additionalFS, L"", fsEpilog);
				else
					fsSrc = shaderLib.Sources[L"lowRes"]().GetAllCodeGLSL(L"", additionalFS, L"", L"");
				return shaderStore.LoadProgram(shaderLib.Sources[L"lowResVs"]().GetAllCodeGLSL(), fsSrc);
			}
			else
				return GL::Program();
		}
		virtual GL::FrameBuffer GetFramebuffer() override
		{
			return frameBuffer;
		}
		virtual int GetRenderTargetMask(const ShadingTechnique & tech) override
		{
			return tech.LowResRenderTargetMask;
		}
		virtual void SetRenderState(GL::HardwareRenderer * renderer) override
		{
			renderer->SetZTestMode(BlendOperator::LessEqual);
		}
		virtual GL::Texture2D GetResultTexture() override
		{
			return resourcePool->LoadGBufferTexture(L"s0", StorageFormat::RGBA_F16, DataType::Float, 0.5f, 1);
		}
		virtual void FrameResized() override
		{
			resourcePool->GetHardwareRenderer()->DestroyFrameBuffer(frameBuffer);
			frameBuffer = resourcePool->CreateFrameBuffer();
			frameBuffer.SetDepthStencilRenderTarget(resourcePool->LoadGBufferRenderBuffer(L"sdepth", StorageFormat::Depth24Stencil8, 0.5f, 1));
			int mask = 0;
			for (int i = 0; i < 8; i++)
			{
				auto tex = resourcePool->LoadGBufferTexture(L"s" + String(i), StorageFormat::RGBA_F16, DataType::Float, 0.5f, 1);
				frameBuffer.SetColorRenderTarget(i, tex);
				mask |= (1 << i);
			}
			frameBuffer.EnableRenderTargets(mask);
		}

		virtual SizeI GetViewport() override
		{
			SizeI rs;
			rs.Width = resourcePool->GetScreenWidth() / 2;
			rs.Height = resourcePool->GetScreenHeight() / 2;
			return rs;
		}
	};

	RenderPassProvider * CreateLowResRenderPass(DeviceResourcePool * resourcePool, ShadowMapAlgorithm * shadowMap)
	{
		return new LowResRenderPassProvider(resourcePool, shadowMap);
	}
}