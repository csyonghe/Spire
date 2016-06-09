#include "RenderPassProvider.h"
#include "ShadowMap.h"
#include "Spire.h"
#include "Scene.h"
#include "DeviceResourcePool.h"
using namespace GL;

namespace RealtimeEngine
{
	extern const wchar_t * fsEpilog;
	class LowQualityRenderPassProvider : public RenderPassProvider
	{
	private:
		ShadowMapAlgorithm *shadowMap = nullptr;
		DeviceResourcePool * resourcePool = nullptr;
		GL::FrameBuffer frameBuffer;
	public:
		LowQualityRenderPassProvider(DeviceResourcePool * pResourcePool, ShadowMapAlgorithm *pShadowMap)
			: resourcePool(pResourcePool), shadowMap(pShadowMap)
		{
			frameBuffer = resourcePool->GetHardwareRenderer()->CreateFrameBuffer();
		}

		~LowQualityRenderPassProvider()
		{
			resourcePool->GetHardwareRenderer()->DestroyFrameBuffer(frameBuffer);
		}

		virtual GL::Program LoadProgram(GpuShaderStore & shaderStore, const SpireLib::ShaderLib & shaderLib) override
		{
			if (shaderLib.Sources.ContainsKey(L"lqfs"))
			{
				auto fsCode = shaderLib.Sources[L"lqfs"]().GetAllCodeGLSL(L"", L"", L"", fsEpilog);
				String additionalFS = L"";
				if (fsCode.Contains(L"blkviewUniform"))
				{
					additionalFS = shadowMap->GetShaderDefinition();
					fsCode = shaderLib.Sources[L"lqfs"]().GetAllCodeGLSL(L"", additionalFS, L"", fsEpilog);
				}
				return shaderStore.LoadProgram(shaderLib.Sources[L"vs"]().GetAllCodeGLSL(), fsCode);
			}
			else
				return GL::Program();
		}
		virtual GL::FrameBuffer GetFramebuffer() override
		{
			return frameBuffer;
		}
		virtual int GetRenderTargetMask(const ShadingTechnique &) override
		{
			return 1;
		}
		virtual void SetRenderState(GL::HardwareRenderer * renderer) override
		{
			renderer->SetZTestMode(BlendOperator::LessEqual);
		}
		virtual GL::Texture2D GetResultTexture() override
		{
			return resourcePool->LoadGBufferTexture(L"color", StorageFormat::RGBA_I8, DataType::Byte, 1.0f, 1);
		}

		virtual void FrameResized() override
		{
			resourcePool->GetHardwareRenderer()->DestroyFrameBuffer(frameBuffer);
			frameBuffer = resourcePool->CreateFrameBuffer();
			auto renderBuffer = resourcePool->LoadGBufferRenderBuffer(L"depth", StorageFormat::Depth24Stencil8, 1.0f, 1);
			frameBuffer.SetDepthStencilRenderTarget(renderBuffer);
			auto colorBuffer = resourcePool->LoadGBufferTexture(L"color", StorageFormat::RGBA_I8, DataType::Byte, 1.0f, 1);
			frameBuffer.SetColorRenderTarget(0, colorBuffer);
			frameBuffer.EnableRenderTargets(1);
		}

		virtual SizeI GetViewport() override
		{
			SizeI rs;
			rs.Width = resourcePool->GetScreenWidth();
			rs.Height = resourcePool->GetScreenHeight();
			return rs;
		}
	};

	RenderPassProvider * CreateLowQualityRenderPass(DeviceResourcePool * resourcePool, ShadowMapAlgorithm * shadowMap)
	{
		return new LowQualityRenderPassProvider(resourcePool, shadowMap);
	}
}