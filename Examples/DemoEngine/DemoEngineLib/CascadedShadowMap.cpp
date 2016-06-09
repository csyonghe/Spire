#include "ShadowMap.h"
#include "Spire.h"

using namespace CoreLib::IO;
using namespace CoreLib::Diagnostics;
using namespace CoreLib::Graphics;
using namespace GL;

namespace RealtimeEngine
{
	const int numCascades = 2;
	const float exponentialFactor = 64.0f;
	struct UInt64Pair
	{
		uint64_t x, y;
	};
	struct ShadowMapUniformBuffer
	{
		Vec4 LightProjScale[numCascades];
		Vec4 LightProjTranslate[numCascades];
		Matrix4 LightViewMatrix;
		UInt64Pair CascadeShadowMaps[numCascades]; //padding to vec4
		Vec4 zPlanes[numCascades];
	};

	class CascadedShadowMapAlgorithm;
	RenderPassProvider * CreateCascadedShadowMapPassProvider(CascadedShadowMapAlgorithm * shadowMap, GL::FrameBuffer frameBuffer);
	
	class CascadedShadowMapAlgorithm : public ShadowMapAlgorithm
	{
	private:
		Array<GL::FrameBuffer, numCascades> frameBuffer;
		GL::RenderBuffer depthRenderBuffer;
		Array<GL::Texture2D, numCascades> shadowMaps;
		DeviceResourcePool * resourcePool;
		int shadowMapSize = 2048;
		Vec3 lightDir;
		ShadowMapUniformBuffer shadowMapUniformBufferContent;
		GL::TextureSampler shadowSampler;
		GL::BufferObject shadowMapUniformBuffer;
		uint64_t shadowMapUniformBufferAddr;
	public:
		int GetShadowMapSize()
		{
			return shadowMapSize;
		}
		void ComputeMatrix(const BBox & bounds, const ViewFrustum & viewFrustum)
		{
			BBox sceneLightBounds;
			TransformBBox(sceneLightBounds, shadowMapUniformBufferContent.LightViewMatrix, bounds);
			
			const float lambda = 0.96f;
			float farOverNear = viewFrustum.zMax / viewFrustum.zMin;
			for (int cascade = 0; cascade < numCascades; ++cascade)
			{
				float iOverN = (cascade + 1) / (float)(numCascades);
				float zmax = lambda * viewFrustum.zMin * powf(farOverNear, iOverN) + (1.0f - lambda) * (viewFrustum.zMin + iOverN * (viewFrustum.zMax - viewFrustum.zMin));
				shadowMapUniformBufferContent.zPlanes[cascade] = Vec4::Create(zmax);
				auto viewVerts = viewFrustum.GetVertices(viewFrustum.zMin, zmax);
				BBox bboxLightSpace;
				bboxLightSpace.Init();
				for (auto & point : viewVerts)
				{
					Vec3 transformedPoint;
					shadowMapUniformBufferContent.LightViewMatrix.TransformHomogeneous(transformedPoint, point);
					bboxLightSpace.Union(transformedPoint);
				}
				bboxLightSpace.zMax = sceneLightBounds.zMax;
				bboxLightSpace.xMin = Math::Max(bboxLightSpace.xMin, sceneLightBounds.xMin);
				bboxLightSpace.yMin = Math::Max(bboxLightSpace.yMin, sceneLightBounds.yMin);
				bboxLightSpace.xMax = Math::Min(bboxLightSpace.xMax, sceneLightBounds.xMax);
				bboxLightSpace.yMax = Math::Min(bboxLightSpace.yMax, sceneLightBounds.yMax);
				float xStep = (bboxLightSpace.xMax - bboxLightSpace.xMin) / (float)shadowMapSize;
				float yStep = (bboxLightSpace.yMax - bboxLightSpace.yMin) / (float)shadowMapSize;
				bboxLightSpace.xMin =  floor(bboxLightSpace.xMin / xStep) * xStep;
				bboxLightSpace.yMin =  floor(bboxLightSpace.yMin / yStep) * yStep;
				bboxLightSpace.xMax =  floor(bboxLightSpace.xMax / xStep) * xStep;
				bboxLightSpace.yMax =  floor(bboxLightSpace.yMax / yStep) * yStep;
				shadowMapUniformBufferContent.LightProjScale[cascade] = Vec4::Create(
					1.0f / (bboxLightSpace.xMax - bboxLightSpace.xMin),
					1.0f / (bboxLightSpace.yMax - bboxLightSpace.yMin),
					-1.0f / (bboxLightSpace.zMax - bboxLightSpace.zMin),
					1.0f);
				shadowMapUniformBufferContent.LightProjTranslate[cascade] = Vec4::Create(
					-0.5f*(bboxLightSpace.xMin + bboxLightSpace.xMax) / (bboxLightSpace.xMax - bboxLightSpace.xMin) + 0.5f,
					-0.5f*(bboxLightSpace.yMax + bboxLightSpace.yMin) / (bboxLightSpace.yMax - bboxLightSpace.yMin) + 0.5f,
					0.5f*(bboxLightSpace.zMax + bboxLightSpace.zMin) / (bboxLightSpace.zMax - bboxLightSpace.zMin) + 0.5f, 1.0f);
			}
		}
		virtual String GetShaderDefinition() override
		{
			StringBuilder sb;
			sb << L"#define NUM_CASCADES " << numCascades << EndLine;
			sb << L"#define EXPONENTIAL " << exponentialFactor << EndLine;
			sb << LR"(
				layout(std140, binding = 8, commandBindableNV) uniform shadowMapUniform
				{
					vec3 lightProjScale[NUM_CASCADES];
					vec3 lightProjTranslate[NUM_CASCADES];
					mat4 lightViewMatrix;
					sampler2D shadowMaps[NUM_CASCADES];
					float zplanes[NUM_CASCADES];
					
				} blkshadowMapUniform;

								float computeShadow(vec3 pos)
				{
				)";
			if (Enabled)
				sb << LR"(
						int index = NUM_CASCADES - 1;
						float z = -(blkviewUniform.viewMatrix * vec4(pos, 1.0)).z;
						for (int i = 0; i < NUM_CASCADES - 1; i++)
						{
							if (z < blkshadowMapUniform.zplanes[i])
							{
								index = i;
								break;
							}
						}
						vec4 shadowViewCoord = blkshadowMapUniform.lightViewMatrix * vec4(pos, 1.0);
						vec3 lightProjScale = blkshadowMapUniform.lightProjScale[index];
						vec3 lightProjTranslate = blkshadowMapUniform.lightProjTranslate[index];
						vec3 shadowCoord = lightProjScale * shadowViewCoord.xyz +  lightProjTranslate;
						vec2 coordDDX = lightProjScale.xy * dFdx(shadowViewCoord.xy);
						vec2 coordDDY = lightProjScale.xy * dFdy(shadowViewCoord.xy);
						float texDepth = textureGrad(blkshadowMapUniform.shadowMaps[index], shadowCoord.xy, coordDDX, coordDDY).x;
						float depth = shadowCoord.z;
						float result = clamp(pow(texDepth / exp(EXPONENTIAL * depth), 16), 0.0, 1.0);
						return result;
					)";
			else
				sb << L"return 1.0;";
			sb << LR"(
				}
			)";
			return sb.ProduceString();
		}

		void InitShadowMap()
		{
			auto hw = resourcePool->GetHardwareRenderer();
			depthRenderBuffer = hw->CreateRenderBuffer(StorageFormat::Depth32, shadowMapSize, shadowMapSize, 1);
			shadowSampler = hw->CreateTextureSampler();
			shadowSampler.SetFilter(TextureFilter::Anisotropic16x);
			for (int i = 0; i < shadowMaps.Count(); i++)
			{
				shadowMaps[i] = hw->CreateTexture2D();
				shadowMaps[i].SetData(StorageFormat::Float32, shadowMapSize, shadowMapSize, 1, DataType::Float, nullptr);
				shadowMaps[i].BuildMipmaps();
				auto texHandle = shadowMaps[i].GetTextureHandle(shadowSampler);
				texHandle.MakeResident();
				shadowMapUniformBufferContent.CascadeShadowMaps[i].x = texHandle.Handle;
				frameBuffer[i].SetColorRenderTarget(0, shadowMaps[i]);
				frameBuffer[i].SetDepthStencilRenderTarget(depthRenderBuffer);
				frameBuffer[i].EnableRenderTargets(1);
			}
		}

		void FreeShadowMap()
		{
			auto hw = resourcePool->GetHardwareRenderer();
			for (int i = 0; i < shadowMaps.Count(); i++)
			{
				if (shadowMaps[i].Handle)
				{
					hw->DestroyTexture(shadowMaps[i]);
				}
			}
			hw->DestroyTextureSampler(shadowSampler);
			hw->DestroyRenderBuffer(depthRenderBuffer);
		}

		CascadedShadowMapAlgorithm(DeviceResourcePool * pResourcePool)
		{
			this->resourcePool = pResourcePool;
			frameBuffer.SetSize(numCascades);
			for (int i = 0; i < numCascades; i++)
				frameBuffer[i] = resourcePool->GetHardwareRenderer()->CreateFrameBuffer();
			shadowMaps.SetSize(shadowMaps.GetCapacity());
			shadowMapUniformBuffer = resourcePool->GetHardwareRenderer()->CreateBuffer(BufferUsage::UniformBuffer);
			ShadowMapUniformBuffer emptyData;
			shadowMapUniformBuffer.SetData(&emptyData, sizeof(emptyData));
			shadowMapUniformBuffer.MakeResident(true);
			shadowMapUniformBufferAddr = shadowMapUniformBuffer.GetGpuAddress();
		}

		~CascadedShadowMapAlgorithm()
		{
			for (int i = 0; i < numCascades; i++)
				resourcePool->GetHardwareRenderer()->DestroyFrameBuffer(frameBuffer[i]);
			FreeShadowMap();
			resourcePool->GetHardwareRenderer()->DestroyBuffer(shadowMapUniformBuffer);
		}

		virtual void SetShadowMapSize(int size) override
		{
			shadowMapSize = size;
			FreeShadowMap();
			InitShadowMap();
		}

		virtual void BindShadowMapResources() override
		{
			glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, 8, shadowMapUniformBufferAddr,
				(GLsizeiptr)(sizeof(ShadowMapUniformBuffer)));
		}

		virtual void BindShadowMapResources(GL::CommandBuffer & cmdBuffer) override
		{
			cmdBuffer.UniformAddress(8, ShaderType::FragmentShader, shadowMapUniformBufferAddr);
		}

		virtual void GenerateShadowMap(const Vec3 & pLightDir, const BBox & sceneBounds, const ViewFrustum & viewFrustum) override
		{
			this->lightDir = pLightDir;
			Vec3 lightUp;
			GetOrthoVec(lightUp, lightDir);
			auto zero = Vec3::Create(0.0f);
			Matrix4::LookAt(shadowMapUniformBufferContent.LightViewMatrix, zero, -lightDir, lightUp);
			ComputeMatrix(sceneBounds, viewFrustum);
			auto hw = resourcePool->GetHardwareRenderer();
			hw->SetZTestMode(BlendOperator::LessEqual);
			for (int i = 0; i < numCascades; i++)
			{
				resourcePool->GetHardwareRenderer()->SetViewport(0, 0, shadowMapSize, shadowMapSize);
				hw->SetWriteFrameBuffer(frameBuffer[i]);
				hw->Clear(true, true, false);
				Matrix4 lightProjMatrix, lightMatrix;
				Matrix4::CreateIdentityMatrix(lightProjMatrix);
				lightProjMatrix.m[0][0] = shadowMapUniformBufferContent.LightProjScale[i].x * 2.0f;
				lightProjMatrix.m[1][1] = shadowMapUniformBufferContent.LightProjScale[i].y * 2.0f;
				lightProjMatrix.m[2][2] = shadowMapUniformBufferContent.LightProjScale[i].z * 2.0f;
				lightProjMatrix.m[3][0] = (shadowMapUniformBufferContent.LightProjTranslate[i].x - 0.5f) * 2.0f;
				lightProjMatrix.m[3][1] = (shadowMapUniformBufferContent.LightProjTranslate[i].y - 0.5f) * 2.0f;
				lightProjMatrix.m[3][2] = (shadowMapUniformBufferContent.LightProjTranslate[i].z - 0.5f) * 2.0f;
				Matrix4::Multiply(lightMatrix, lightProjMatrix, shadowMapUniformBufferContent.LightViewMatrix);
				DrawShadowMap(lightMatrix, i);
				shadowMaps[i].BuildMipmaps();
			}
			shadowMapUniformBuffer.SubData(0, sizeof(shadowMapUniformBufferContent), &shadowMapUniformBufferContent);
		}

		virtual List<RefPtr<RenderPassProvider>> GetRenderPasses() override
		{
			List<RefPtr<RenderPassProvider>> rs;
			for (int i = 0; i < numCascades; i++)
				rs.Add(CreateCascadedShadowMapPassProvider(this, frameBuffer[i]));
			return rs;
		}
	};

	extern const wchar_t * fsEpilog;
	class CascadedShadowMapPassProvider : public RenderPassProvider
	{
	private:
		String GetShadowPassShaderDefinition()
		{
			StringBuilder sb;
			sb << L"#define EXPONENTIAL " << exponentialFactor << EndLine;
			sb << LR"(
				layout(location = 0) out float depth;	
			)";
			return sb.ProduceString();
		}
		GL::FrameBuffer frameBuffer;
		CascadedShadowMapAlgorithm * shadowMap;
	public:
		CascadedShadowMapPassProvider(CascadedShadowMapAlgorithm * pShadowMap, GL::FrameBuffer pFrameBuffer)
			: shadowMap(pShadowMap), frameBuffer(pFrameBuffer)
		{}
		virtual GL::Program LoadProgram(GpuShaderStore & shaderStore, const SpireLib::ShaderLib & shaderLib) override
		{
			auto shadowPassShaderCode = LR"(
					float linearZ = $projCoord.z * 0.5 + 0.5;
					depth = exp(EXPONENTIAL * linearZ);
				)";
			Spire::Compiler::WorldMetaData wmeta;
			if (shaderLib.MetaData.Worlds.TryGetValue(L"shadowVs", wmeta))
			{
				auto & fs = shaderLib.Sources[L"shadowFs"]();
				StringBuilder epilog;
				if (fs.ComponentAccessNames.ContainsKey(L"opacity"))
					epilog << fsEpilog;
				epilog << shadowPassShaderCode;
				return shaderStore.LoadProgram(shaderLib.Sources[L"shadowVs"]().GetAllCodeGLSL(), fs.GetAllCodeGLSL(L"", GetShadowPassShaderDefinition(),
					L"", epilog.ProduceString()));
			}
			else
				return GL::Program();
		}
		virtual GL::Texture2D GetResultTexture() override
		{
			return GL::Texture2D();
		}
		virtual GL::FrameBuffer GetFramebuffer() override
		{
			return frameBuffer;
		}
		virtual SizeI GetViewport() override
		{
			SizeI rs;
			rs.Height = rs.Width = shadowMap->GetShadowMapSize();
			return rs;
		}
		virtual int GetRenderTargetMask(const ShadingTechnique &) override
		{
			return 1;
		}
		virtual void SetRenderState(GL::HardwareRenderer * renderer) override
		{
			renderer->SetZTestMode(BlendOperator::LessEqual);
		}
		virtual void FrameResized() override
		{
		}
	};

	RenderPassProvider * CreateCascadedShadowMapPassProvider(CascadedShadowMapAlgorithm * shadowMap, GL::FrameBuffer frameBuffer)
	{
		return new CascadedShadowMapPassProvider(shadowMap, frameBuffer);
	}

	ShadowMapAlgorithm * CreateCascadedShadowMapAlgorithm(DeviceResourcePool * resourcePool)
	{
		return new CascadedShadowMapAlgorithm(resourcePool);
	}
}