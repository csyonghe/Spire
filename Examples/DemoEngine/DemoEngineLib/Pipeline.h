#ifndef REALTIME_ENGINE_PIPELINE_H
#define REALTIME_ENGINE_PIPELINE_H

#include "Scene.h"
#include "CoreLib/Graphics.h"
#include "Spire.h"
#include "Schedule.h"
#include "DeviceResourcePool.h"
#include "Background.h"
#include "ShadowMap.h"
#include "PTexMipmapBuilder.h"
#include "DeviceMarshalling.h"
#include "MaterialContext.h"
#include "RenderPassProvider.h"

namespace RealtimeEngine
{
	class ViewUniformBufferContent
	{
	public:
		Matrix4 ViewProjectionMatrix;
		Matrix4 ViewMatrix;
		Matrix4 ProjectionMatrix;
		Matrix4 InvViewMatrix;
		Matrix4 InvViewProjectionMatrix;
		Vec4 CameraPos;
		Vec4 LightColor;
		Vec4 LightDir;
		Vec4 LightParams;
		uint64_t texGGX_D, texGGX_FV;
		float Time;
	};

	class CompiledCommands
	{
	public:
		GL::Program Program;
		GL::CommandList Commands;
		int RenderTargets = 0;
	};

	class RenderPassContext
	{
	public:
		int ProgramIndex;
		GL::FrameBuffer FrameBuffer;
		List<List<List<CompiledCommands>>> CompiledCommandLists;
	};

	class EnginePipeline
	{
	private:
		PTexMipmapBuilder mipmapBuilder;
		List<float> heightMapData;
		int heightMapSize;
		float cellSpace = 0.0f;
		DeviceResourcePool * engine;
		TerrainGeometry terrain;
		RefPtr<Background> background;
		RefPtr<ShadowMapAlgorithm> shadowMap;
		uint64_t uniformAddr, viewUniformAddr;
		GL::TextureHandle texGGX_D, texGGX_FV;
		GL::BufferObject ViewUniformBuffer;
		EnumerableDictionary<String, int> shaderIds;

		EnumerableDictionary<String, String> shaderFileNames;
		List<DrawableInstance> objects;
		List<RefPtr<SpireLib::ShaderLib>> shaderLibs;
		GpuShaderStore gpuShaderStore;
		List<MaterialContext> gpuShaders;
		EnumerableDictionary<String, RefPtr<DeviceMesh>> meshBuffers;
		RefPtr<DeviceMesh> AddDrawableMesh(Mesh mesh, const Matrix4 & matrix);
		const int sceneGridSize = 16;
		int shadowMapSize = 2048;
		
		EnumerableDictionary<GLuint, List<CompiledCommands>> drawCommandSortBuffer;
		SystemUniforms sysUniforms;
		List<RenderPassContext> renderPassContexts;
		List<RefPtr<RenderPassProvider>> renderPasses;
		List<List<DrawableInstance*>> sceneGridObjLists;
		List<GL::StateObject> stateObjects;
		CoreLib::Graphics::BBox bounds;
		Vec3 camPos;
		void InitializeShader(int shaderId);
		void BuildUniformBuffer(int shaderId);
		void Precompute(int shaderId);
		void AllocObjectSpaceBuffers(int shaderId);
		void PrecomputeVertex(DrawableInstance & inst, ShadingTechnique & tech);
		void PrecomputeUniform(DrawableInstance & inst, List<unsigned char> & uniformBuffer, ShadingTechnique & tech);
		void PrecomputeTextures(DrawableInstance & inst, ShadingTechnique & tech, List<unsigned char> & precomputeTexUniformBuffer);
		inline RenderPassProvider * GetMainRenderPass()
		{
			return renderPasses[MaterialContext::MainProgramId].Ptr();
		}
		inline RenderPassContext & GetMainRenderPassContext()
		{
			return renderPassContexts[MaterialContext::MainProgramId];
		}
		void ReloadShader(MaterialContext & ctx, const SpireLib::ShaderLib & shader);
		bool enableFoveatedRendering = false;
	public:
		int CommandListsDrawn = 0;
		float cpuFrameTime = 0.0f;
		int SelectedLOD = -1;
		int FoveaX, FoveaY, FoveaRad;

		List<String> GetShaders();
		List<Spire::Compiler::ShaderChoice> GetChoices(String shaderName, const EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue> & existingChoices);
		void RecompileShader(String shaderName, String schedule);
		CoreLib::Graphics::BBox GetBounds();
		void SetFoveatedRendering(bool enable);
		bool GetFoveatedRendering()
		{
			return enableFoveatedRendering;
		}
		float GetAltitude(const Vec3 & v)
		{
			return terrain.GetAltitude(v);
		}
		bool GetUseShadowMap()
		{
			return shadowMap->Enabled;
		}
		void SetUseShadowMap(bool enable);
		void CompileCommandLists();
		void FrameResized();
		void Initialize(DeviceResourcePool * pEngine);
		int LoadShader(String shaderFileName, String sourceFileName);
		RefPtr<DeviceMesh> LoadMesh(String meshFileName);
		void LoadFromFile(String fileName);
		void UpdateSysUniform(SystemUniforms & sysUniform, int viewId = 0);
		void SubmitGeometryCommandList(int programIndex);
		void SubmitGeometry(int programIndex);
		void Draw(bool stereo);
		void DrawMainPass();
		void DrawObjSpace();
		void FreeCommandLists();
		void PrintStats();
		Spire::Compiler::ShaderMetaData GetShaderMetaData(String shaderName);
		EnumerableDictionary<String, GL::Texture2D> GetPrecomputedTextures(String shader);
		GL::Texture2D GetColorBuffer()
		{
			return GetMainRenderPass()->GetResultTexture();
		}
		GL::FrameBuffer GetFrameBuffer()
		{
			return GetMainRenderPass()->GetFramebuffer();
		}
		~EnginePipeline()
		{
			auto hw = engine->GetHardwareRenderer();
			hw->Finish();
			hw->DestroyBuffer(ViewUniformBuffer);
			terrain.Free(hw);
			for (auto & renderPass : renderPasses)
				renderPass = nullptr;
			for (auto & obj : objects)
				obj.Free(hw);
			for (auto & shader : gpuShaders)
				shader.Free(hw, gpuShaderStore);
			gpuShaderStore.Free();
			FreeCommandLists();
			mipmapBuilder.Free();
			shadowMap = nullptr;
		}
	};
	String GenerateSchedule(const EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue> & choices, const EnumerableDictionary<String, EnumerableDictionary<String, String>> & attribs);
}
#endif
