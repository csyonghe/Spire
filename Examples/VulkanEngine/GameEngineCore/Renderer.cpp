#include "Renderer.h"
#include "HardwareRenderer.h"
#include "HardwareApiFactory.h"

#include "Engine.h"
#include "SkeletalMeshActor.h"
#include "CoreLib/Graphics/TextureFile.h"
#include "CoreLib/Imaging/Bitmap.h"
#include "CoreLib/Imaging/TextureData.h"
#include "CoreLib/WinForm/Debug.h"

#include "Spire/Spire.h"
#include "TextureCompressor.h"
#include <fstream>

#define DEFERRED 0
const wchar_t* lightingString = DEFERRED ? L"DeferredLighting" : L"ForwardLighting";

using namespace CoreLib;
using namespace VectorMath;

namespace GameEngine
{
	struct MeshIndexRange
	{
		int Start = 0, End = 0;
		int MinVertId = 0, MaxVertId = 0;
	};

	class SystemUniforms
	{
	public:
		Matrix4 ViewTransform, ViewProjectionTransform, InvViewTransform, InvViewProjTransform;
		Vec4 CameraPos;
	};

	struct BoneTransform
	{
		Matrix4 TransformMatrix;
		Vec4 NormalMatrix[3];
	};

	class StaticRenderContextImpl : public StaticRenderContext
	{
	public:
		MeshIndexRange MeshRange;
		int InstanceUniformStart, InstanceUniformEnd;
		int TransformUniformStart, TransformUniformEnd;
		Array<Texture2D*, 8> Textures;
		RefPtr<PipelineInstance> pipelineInstance; //TODO: should this be pipeline instance or pipeline and bindings?
	};

	class SkeletalMeshRenderContextImpl : public SkeletalMeshRenderContext
	{
	public:
		MeshIndexRange MeshRange;
		int InstanceUniformStart, InstanceUniformEnd;
		int TransformUniformStart, TransformUniformEnd;
		Array<Texture2D*, 8> Textures;
		RefPtr<PipelineInstance> pipelineInstance;
	};

	class MaterialInstance
	{
	public:
		List<Shader*> shaders;
		RefPtr<Pipeline> pipeline;
	};

	class RendererImpl : public Renderer
	{
	private:
		RenderAPI api;

		int screenWidth = 0, screenHeight = 0;
		RefPtr<FrameBuffer> frameBuffer;
		RefPtr<FrameBuffer> deferredFrameBuffer;
		RefPtr<Texture2D> presentTexture;
#if DEFERRED
		RefPtr<Texture2D> colorTexture;
		RefPtr<Texture2D> pbrTexture;
		RefPtr<Texture2D> normalTexture;
#endif
		RefPtr<Texture2D> depthTexture;

		Level* level = nullptr;

		RefPtr<HardwareRenderer> hardwareRenderer;
		RefPtr<HardwareApiFactory> hardwareFactory;
#if DEFERRED
		RefPtr<CommandBuffer> deferredCommandBuffer;
#endif
		RefPtr<CommandBuffer> clearCommandBuffer;
		RefPtr<CommandBuffer> staticCommandBuffer;
		RefPtr<CommandBuffer> dynamicCommandBuffer;

		RefPtr<Pipeline> deferredPipeline;
		RefPtr<PipelineInstance> deferredPipelineInstance;

		SystemUniforms sysUniforms;
		EnumerableDictionary<Mesh*, MeshIndexRange> meshIndexRange;
		EnumerableDictionary<String, RefPtr<Shader>> shaders;
		EnumerableDictionary<String, MaterialInstance> materialCache;
		EnumerableDictionary<String, RefPtr<Texture2D>> textures;

		Dictionary<int, RefPtr<Buffer>> vertexArrays;
		Dictionary<int, VertexFormat> vertexFormats;

		RefPtr<Buffer> deferredFullscreen;
		RefPtr<Buffer> meshIndexBuffer;
		RefPtr<Buffer> staticInstanceUniformBuffer;
		RefPtr<Buffer> staticTransformUniformBuffer;
		RefPtr<Buffer> sysUniformBuffer;
		RefPtr<Buffer> skeletalTransformBuffer;
		RefPtr<Buffer> dynamicInstanceUniformBuffer;
		int uniformBufferAlignment = 1;

		RefPtr<TextureSampler> textureSampler;
		RefPtr<TextureSampler> deferredSampler;

		RefPtr<RenderTargetLayout> renderTargetLayout;
		RefPtr<RenderTargetLayout> deferredLayout;

		CoreLib::String meshProcessingDef;
	private:
		void Align(MemoryStream * mem, int alignment)
		{
			int m = mem->GetPosition() % alignment;
			if (m)
			{
				int padding = alignment - m;
				mem->Write(nullptr, padding);
			}
		}

		template<typename TContext>
		void FillInstanceUniformBuffer(TContext & ctx, Material * material, MemoryStream * uniformStream, BinaryWriter & uniformWriter)
		{
			ctx->InstanceUniformStart = uniformStream->GetBufferSize();
			ctx->Textures.Clear();
			for (auto & mvar : material->Variables)
			{
				if (mvar.Value.VarType == DynamicVariableType::Texture)
				{
					ctx->Textures.Add(LoadTexture(mvar.Value.StringValue));
				}
				else if (mvar.Value.VarType == DynamicVariableType::Float)
					uniformWriter.Write(mvar.Value.FloatValue);
				else if (mvar.Value.VarType == DynamicVariableType::Int)
					uniformWriter.Write(mvar.Value.IntValue);
				else if (mvar.Value.VarType == DynamicVariableType::Vec2)
				{
					Align(uniformStream, 8);
					uniformWriter.Write(mvar.Value.Vec2Value);
				}
				else if (mvar.Value.VarType == DynamicVariableType::Vec3)
				{
					Align(uniformStream, 16);
					uniformWriter.Write(mvar.Value.Vec2Value);
				}
				else if (mvar.Value.VarType == DynamicVariableType::Vec4)
				{
					Align(uniformStream, 16);
					uniformWriter.Write(mvar.Value.Vec2Value);
				}
			}
			ctx->InstanceUniformEnd = uniformStream->GetBufferSize();
		}


		Texture2D* LoadTexture2D(const String & name, CoreLib::Graphics::TextureFile & data)
		{

			RefPtr<Texture2D> value;
			if (textures.TryGetValue(name, value))
				return value.Ptr();
			StorageFormat format;
			DataType dataType = DataType::Byte4;
			switch (data.GetFormat())
			{
			case CoreLib::Graphics::TextureStorageFormat::R8:
				format = StorageFormat::Int8;
				dataType = DataType::Byte;
				break;
			case CoreLib::Graphics::TextureStorageFormat::RG8:
				format = StorageFormat::RG_I8;
				dataType = DataType::Byte2;
				break;
			case CoreLib::Graphics::TextureStorageFormat::RGB8:
				format = StorageFormat::RGB_I8;
				dataType = DataType::Byte3;
				break;
			case CoreLib::Graphics::TextureStorageFormat::RGBA8:
				format = StorageFormat::RGBA_8;
				dataType = DataType::Byte4;
				break;
			case CoreLib::Graphics::TextureStorageFormat::R_F32:
				format = StorageFormat::Float32;
				dataType = DataType::Float;
				break;
			case CoreLib::Graphics::TextureStorageFormat::RG_F32:
				format = StorageFormat::RG_F32;
				dataType = DataType::Float2;
				break;
			case CoreLib::Graphics::TextureStorageFormat::RGB_F32:
				format = StorageFormat::RGB_F32;
				dataType = DataType::Float3;
				break;
			case CoreLib::Graphics::TextureStorageFormat::RGBA_F32:
				format = StorageFormat::RGBA_F32;
				dataType = DataType::Float4;
				break;
			case CoreLib::Graphics::TextureStorageFormat::BC1:
				format = StorageFormat::BC1;
				break;
			case CoreLib::Graphics::TextureStorageFormat::BC5:
				format = StorageFormat::BC5;
				break;
			default:
				throw NotImplementedException(L"unsupported texture format.");
			}
			auto rs = hardwareRenderer->CreateTexture2D(TextureUsage::Sampled);
			for (int i = 0; i < data.GetMipLevels(); i++)
				rs->SetData(format, i, Math::Max(data.GetWidth() >> i, 1), Math::Max(data.GetHeight() >> i, 1), 1, dataType, data.GetData(i).Buffer());
			if (format != StorageFormat::BC1 && format != StorageFormat::BC5)
				rs->BuildMipmaps();
			textures[name] = rs;
			return rs;
		}

		Texture2D* LoadTexture(const String & filename)
		{
			RefPtr<Texture2D> value;
			if (textures.TryGetValue(filename, value))
				return value.Ptr();

			auto actualFilename = Engine::Instance()->FindFile(Path::ReplaceExt(filename, L"texture"), ResourceType::Texture);
			if (!actualFilename.Length())
				actualFilename = Engine::Instance()->FindFile(filename, ResourceType::Texture);
			if (actualFilename.Length())
			{
				if (actualFilename.ToLower().EndsWith(L".texture"))
				{
					CoreLib::Graphics::TextureFile file(actualFilename);
					return LoadTexture2D(filename, file);
				}
				else
				{
					CoreLib::Imaging::Bitmap bmp(actualFilename);
					List<unsigned int> pixelsInversed;
					int * sourcePixels = (int*)bmp.GetPixels();
					pixelsInversed.SetSize(bmp.GetWidth() * bmp.GetHeight());
					for (int i = 0; i < bmp.GetHeight(); i++)
					{
						for (int j = 0; j < bmp.GetWidth(); j++)
							pixelsInversed[i*bmp.GetWidth() + j] = sourcePixels[(bmp.GetHeight() - 1 - i)*bmp.GetWidth() + j];
					}
					CoreLib::Graphics::TextureFile texFile;
					TextureCompressor::CompressRGBA_BC1(texFile, MakeArrayView((unsigned char*)pixelsInversed.Buffer(), pixelsInversed.Count() * 4), bmp.GetWidth(), bmp.GetHeight());
					texFile.SaveToFile(Path::ReplaceExt(actualFilename, L"texture"));
					return LoadTexture2D(filename, texFile);
				}
			}
			else
			{
				CoreLib::Graphics::TextureFile errTex;
				unsigned char errorTexContent[] =
				{
					255,0,255,255,   0,0,0,255,
					0,0,0,255,       255,0,255,255
				};
				errTex.SetData(CoreLib::Graphics::TextureStorageFormat::RGBA8, 2, 2, 0, ArrayView<unsigned char>(errorTexContent, 16));
				return LoadTexture2D(L"ERROR_TEXTURE", errTex);
			}
		}
		
		Shader* LoadShader(const String & src, void* data, int size, ShaderType shaderType)
		{
			{
				String debugFileName = src + L".spv";
				BinaryWriter debugWriter(new FileStream(debugFileName, FileMode::Create));
				debugWriter.Write((unsigned char*)data, size);
			}
			RefPtr<Shader> result;
			if (shaders.TryGetValue(src, result))
				return result.Ptr();
			result = hardwareRenderer->CreateShader(shaderType, (char*)data, size);
			shaders[src] = result;
			return result.Ptr();
		}

		MaterialInstance LoadMaterial(Material * material, MeshVertexFormat meshVertexFormat, const String& symbol)
		{
			MaterialInstance materialInstance;
			auto identifier = material->ShaderFile + L"!" + symbol;
			if (materialCache.TryGetValue(identifier, materialInstance))
			{
				return materialInstance;
			}

			RefPtr<PipelineBuilder> pipelineBuilder = hardwareRenderer->CreatePipelineBuilder();

			int offset = (api == RenderAPI::Vulkan) ? 4 : 0;
			int location = 0;
			pipelineBuilder->SetBindingLayout(0, BindingType::UniformBuffer);
			pipelineBuilder->SetBindingLayout(1, BindingType::UniformBuffer);
			pipelineBuilder->SetBindingLayout(2, BindingType::UniformBuffer);
			pipelineBuilder->SetBindingLayout(3, BindingType::StorageBuffer);
			pipelineBuilder->DepthCompareFunc = CompareFunc::LessEqual;
			for (auto var : material->Variables)
			{
				if (var.Value.VarType == DynamicVariableType::Texture)
				{
					pipelineBuilder->SetBindingLayout(offset + location, BindingType::Texture);
				}
				location++;
			}

			// Set vertex layout
			pipelineBuilder->SetVertexLayout(vertexFormats[meshVertexFormat.GetTypeId()]);

			// Compile shaders
			ShaderCompilationResult rs;
			if (!hardwareFactory->CompileShader(rs, material->ShaderFile, meshProcessingDef, meshVertexFormat.GetShaderDefinition(), symbol))
				throw HardwareRendererException(L"Shader compilation failure");

			for (auto& compiledShader : rs.Shaders)
			{
				Shader* shader;
				if (compiledShader.Key == L"vs")
				{
					shader = LoadShader(Path::ReplaceExt(material->ShaderFile, compiledShader.Key.Buffer()), compiledShader.Value.Buffer(), compiledShader.Value.Count(), ShaderType::VertexShader);
				}
				else if (compiledShader.Key == L"fs")
				{
					shader = LoadShader(Path::ReplaceExt(material->ShaderFile, compiledShader.Key.Buffer()), compiledShader.Value.Buffer(), compiledShader.Value.Count(), ShaderType::FragmentShader);
				}
				else if (compiledShader.Key == L"tcs")
				{
					shader = LoadShader(Path::ReplaceExt(material->ShaderFile, compiledShader.Key.Buffer()), compiledShader.Value.Buffer(), compiledShader.Value.Count(), ShaderType::HullShader);
				}
				else if (compiledShader.Key == L"tes")
				{
					shader = LoadShader(Path::ReplaceExt(material->ShaderFile, compiledShader.Key.Buffer()), compiledShader.Value.Buffer(), compiledShader.Value.Count(), ShaderType::DomainShader);
				}
				materialInstance.shaders.Add(shader);
			}

			pipelineBuilder->SetShaders(materialInstance.shaders.GetArrayView());

			materialInstance.pipeline = pipelineBuilder->ToPipeline(renderTargetLayout.Ptr());
			materialCache[identifier] = materialInstance;
			return materialInstance;
		}

#define CLEAR_INDIVIDUAL_TEX 0
		void RecordClearCommandBuffer()
		{
#if !CLEAR_INDIVIDUAL_TEX
			if (!level) return;

			RenderAttachments renderAttachments;
#if DEFERRED
			renderAttachments.SetAttachment(0, colorTexture.Ptr());
			renderAttachments.SetAttachment(1, pbrTexture.Ptr());
			renderAttachments.SetAttachment(2, normalTexture.Ptr());
			renderAttachments.SetAttachment(3, depthTexture.Ptr());
#else
			renderAttachments.SetAttachment(0, presentTexture.Ptr());
			renderAttachments.SetAttachment(1, depthTexture.Ptr());
#endif

			clearCommandBuffer->BeginRecording(renderTargetLayout.Ptr(), frameBuffer.Ptr());
			clearCommandBuffer->ClearAttachments(renderAttachments);
			clearCommandBuffer->EndRecording();
#endif
		}

		void RecordDeferredCommandBuffer()
		{
#if DEFERRED
			// Create deferred pipeline
			VertexFormat deferredVertexFormat;
			deferredVertexFormat.Attributes.Add(VertexAttributeDesc(DataType::Float2, 0, 0, 0));
			deferredVertexFormat.Attributes.Add(VertexAttributeDesc(DataType::Float2, 0, 2 * sizeof(float), 1));
			int offset = (api == RenderAPI::Vulkan) ? 4 : 0;
			RefPtr<PipelineBuilder> pipelineBuilder = hardwareRenderer->CreatePipelineBuilder();
			pipelineBuilder->SetBindingLayout(1, BindingType::UniformBuffer);
			pipelineBuilder->SetBindingLayout(offset + 0, BindingType::Texture); // Albedo
			pipelineBuilder->SetBindingLayout(offset + 1, BindingType::Texture); // PBR
			pipelineBuilder->SetBindingLayout(offset + 2, BindingType::Texture); // Normal
			pipelineBuilder->SetBindingLayout(offset + 3, BindingType::Texture); // Depth
			pipelineBuilder->SetVertexLayout(deferredVertexFormat);
			CoreLib::List<Shader*> shaderList;
			ShaderCompilationResult rs;
			if (!hardwareFactory->CompileShader(rs, L"DeferredLighting.shader", L"", L"", L"DeferredLighting"))
				throw HardwareRendererException(L"Shader compilation failure");

			for (auto& compiledShader : rs.Shaders)
			{
				Shader* shader;
				if (compiledShader.Key == L"vs")
				{
					shader = LoadShader(Path::ReplaceExt(L"DeferredLighting.shader", compiledShader.Key.Buffer()), compiledShader.Value.Buffer(), compiledShader.Value.Count(), ShaderType::VertexShader);
				}
				else if (compiledShader.Key == L"fs")
				{
					shader = LoadShader(Path::ReplaceExt(L"DeferredLighting.shader", compiledShader.Key.Buffer()), compiledShader.Value.Buffer(), compiledShader.Value.Count(), ShaderType::FragmentShader);
				}
				shaderList.Add(shader);
			}
			pipelineBuilder->SetShaders(shaderList.GetArrayView());
			PipelineBinding pipelineBinding;
			pipelineBinding.BindUniformBuffer(1, sysUniformBuffer.Ptr());
			pipelineBinding.BindTexture(offset + 0, colorTexture.Ptr(), deferredSampler.Ptr());
			pipelineBinding.BindTexture(offset + 1, pbrTexture.Ptr(), deferredSampler.Ptr());
			pipelineBinding.BindTexture(offset + 2, normalTexture.Ptr(), deferredSampler.Ptr());
			pipelineBinding.BindTexture(offset + 3, depthTexture.Ptr(), deferredSampler.Ptr());
			deferredPipeline = pipelineBuilder->ToPipeline(deferredLayout.Ptr());
			deferredPipelineInstance = deferredPipeline->CreateInstance(pipelineBinding);

			RenderAttachments deferredAttachments;
			deferredAttachments.SetAttachment(0, presentTexture.Ptr());

			deferredCommandBuffer->BeginRecording(deferredLayout.Ptr(), deferredFrameBuffer.Ptr());
#if !CLEAR_INDIVIDUAL_TEX
			deferredCommandBuffer->ClearAttachments(deferredAttachments);
#endif
			deferredCommandBuffer->SetViewport(0, 0, screenWidth, screenHeight);
			deferredCommandBuffer->BindVertexBuffer(deferredFullscreen.Ptr());
			deferredCommandBuffer->BindPipeline(deferredPipelineInstance.Ptr());
			deferredCommandBuffer->Draw(0, 3);
			deferredCommandBuffer->EndRecording();
#endif
		}

		void RecordStaticCommandBuffer()
		{
			if (!level) return;

			staticCommandBuffer->BeginRecording(renderTargetLayout.Ptr(), frameBuffer.Ptr());
			staticCommandBuffer->BindIndexBuffer(meshIndexBuffer.Ptr());
			staticCommandBuffer->SetViewport(0, 0, screenWidth, screenHeight);

			for (auto& obj : level->StaticActors)
			{
				auto ctx = (StaticRenderContextImpl*)obj->RenderContext.Ptr();
				staticCommandBuffer->BindVertexBuffer(vertexArrays[obj->Mesh->GetVertexTypeId()]().Ptr());
				staticCommandBuffer->BindPipeline(ctx->pipelineInstance.Ptr());
				staticCommandBuffer->DrawIndexed(ctx->MeshRange.Start, ctx->MeshRange.End - ctx->MeshRange.Start);
			}

			staticCommandBuffer->EndRecording();
		}

		void RecordDynamicCommandBuffer()
		{
			if (!level) return;

			dynamicCommandBuffer->BeginRecording(renderTargetLayout.Ptr(), frameBuffer.Ptr());
			dynamicCommandBuffer->BindIndexBuffer(meshIndexBuffer.Ptr());
			dynamicCommandBuffer->SetViewport(0, 0, screenWidth, screenHeight);

			for (auto& obj : level->GeneralActors)
			{
				if (obj->GetEngineType() == EngineActorType::SkeletalMesh)
				{
					auto skeletalActor = (SkeletalMeshActor*)obj.Ptr();
					if (skeletalActor->RenderContext)
					{
						auto ctx = (SkeletalMeshRenderContextImpl*)(skeletalActor->RenderContext.Ptr());
						dynamicCommandBuffer->BindVertexBuffer(vertexArrays[skeletalActor->Mesh->GetVertexTypeId()]().Ptr());
						dynamicCommandBuffer->BindPipeline(ctx->pipelineInstance.Ptr());
						dynamicCommandBuffer->DrawIndexed(ctx->MeshRange.Start, ctx->MeshRange.End - ctx->MeshRange.Start);
					}
				}
			}

			dynamicCommandBuffer->EndRecording();
		}
	public:
		RendererImpl(WindowHandle window, RenderAPI api)
		{
			this->api = api;
			switch (api)
			{
			case RenderAPI::Vulkan:
				hardwareFactory = CreateVulkanFactory();
				break;
			case RenderAPI::OpenGL:
				hardwareFactory = CreateOpenGLFactory();
				break;
			}
			hardwareRenderer = hardwareFactory->CreateRenderer(Engine::Instance()->GpuId);
			hardwareRenderer->BindWindow(window, 640, 480);

			// Create command buffers
#if DEFERRED
			deferredCommandBuffer = hardwareRenderer->CreateCommandBuffer();
#endif
			clearCommandBuffer = hardwareRenderer->CreateCommandBuffer();
			staticCommandBuffer = hardwareRenderer->CreateCommandBuffer();
			dynamicCommandBuffer = hardwareRenderer->CreateCommandBuffer();

#if DEFERRED
			// Vertex buffer for VS bypass
			const float fsTri[] =
			{
				-1.0f, -1.0f, 0.0f, 0.0f,
				3.0f, -1.0f, 2.0f, 0.0f,
				-1.0f, 3.0f, 0.0f, 2.0f
			};
			deferredFullscreen = hardwareRenderer->CreateBuffer(BufferUsage::ArrayBuffer);
			deferredFullscreen->SetData((void*)&fsTri[0], sizeof(fsTri));
#endif

			// Create buffers for shader resources
			meshIndexBuffer = hardwareRenderer->CreateBuffer(BufferUsage::IndexBuffer);
			staticInstanceUniformBuffer = hardwareRenderer->CreateBuffer(BufferUsage::UniformBuffer);
			staticTransformUniformBuffer = hardwareRenderer->CreateBuffer(BufferUsage::UniformBuffer);
			skeletalTransformBuffer = hardwareRenderer->CreateMappedBuffer(BufferUsage::StorageBuffer);
			dynamicInstanceUniformBuffer = hardwareRenderer->CreateMappedBuffer(BufferUsage::UniformBuffer);

			// Fetch uniform buffer alignment requirements
			uniformBufferAlignment = hardwareRenderer->UniformBufferAlignment();

			// Create and resize uniform buffer
			sysUniformBuffer = hardwareRenderer->CreateMappedBuffer(BufferUsage::UniformBuffer);
			sysUniformBuffer->SetData(nullptr, sizeof(SystemUniforms));

			// Create default texture sampler
			deferredSampler = hardwareRenderer->CreateTextureSampler();
			deferredSampler->SetFilter(TextureFilter::Nearest);

			textureSampler = hardwareRenderer->CreateTextureSampler();
			textureSampler->SetFilter(TextureFilter::Anisotropic16x);

			// Create our render target layout
			List<TextureUsage> targetLayout;
			targetLayout.Add(TextureUsage::ColorAttachment); // Albedo
#if DEFERRED
			deferredLayout = hardwareRenderer->CreateRenderTargetLayout(ArrayView<TextureUsage>(targetLayout.Buffer(), targetLayout.Count()));

			targetLayout.Add(TextureUsage::ColorAttachment); // Roughness / Metallic / Specular
			targetLayout.Add(TextureUsage::ColorAttachment); // Normal
#endif
			targetLayout.Add(TextureUsage::DepthAttachment);

			renderTargetLayout = hardwareRenderer->CreateRenderTargetLayout(ArrayView<TextureUsage>(targetLayout.Buffer(), targetLayout.Count()));


			auto meshProcessingFile = Engine::Instance()->FindFile(L"MeshProcessing.shader", ResourceType::Shader);
			if (!meshProcessingFile.Length())
				throw InvalidOperationException(L"'MeshProcessing.shader' not found. Engine directory is not setup correctly.");
			meshProcessingDef = CoreLib::IO::File::ReadAllText(meshProcessingFile);

		}
		~RendererImpl()
		{
			clearCommandBuffer = nullptr;
			staticCommandBuffer = nullptr;
			dynamicCommandBuffer = nullptr;

			deferredFrameBuffer = nullptr;
			frameBuffer = nullptr;
			presentTexture = nullptr;
#if DEFERRED
			colorTexture = nullptr;
			pbrTexture = nullptr;
			normalTexture = nullptr;
#endif
			depthTexture = nullptr;
		}

		virtual void Wait() override
		{
			hardwareRenderer->Wait();
		}

		virtual HardwareRenderer * GetHardwareRenderer() override
		{
			return hardwareRenderer.Ptr();
		}
		virtual void InitializeLevel(Level* pLevel) override
		{
			if (!pLevel) return;

			// Load static objects
			{
				EnumerableDictionary<int, RefPtr<BinaryWriter>> vertexBufferWriters;
				List<int> staticIndices;

				RefPtr<MemoryStream> uniformStream = new CoreLib::IO::MemoryStream();
				BinaryWriter uniformWriter(uniformStream);

				RefPtr<MemoryStream> transformUniformStream = new CoreLib::IO::MemoryStream();
				BinaryWriter transformUniformWriter(transformUniformStream);

				// Load meshes
				for (auto & meshKV : pLevel->Meshes)
				{
					auto mesh = meshKV.Value.Ptr();
					int vertTypeId = mesh->GetVertexTypeId();
					auto vertFormat = mesh->GetVertexFormat();
					if (!vertexArrays.ContainsKey(vertTypeId))
					{
						VertexFormat vertexFormat;
						int location = 0;

						const int UNNORMALIZED = 0;
						const int NORMALIZED = 1;

						// Always starts with vec3 pos
						vertexFormat.Attributes.Add(VertexAttributeDesc(DataType::Float3, UNNORMALIZED, 0, location));
						location++;

						for (int i = 0; i < vertFormat.GetUVChannelCount(); i++)
						{
							vertexFormat.Attributes.Add(VertexAttributeDesc(DataType::Half2, UNNORMALIZED, vertFormat.GetUVOffset(i), location));
							location++;
						}
						if (vertFormat.HasTangent())
						{
							vertexFormat.Attributes.Add(VertexAttributeDesc(DataType::UInt, UNNORMALIZED, vertFormat.GetTangentFrameOffset(), location));
							location++;
						}
						for (int i = 0; i < vertFormat.GetColorChannelCount(); i++)
						{
							vertexFormat.Attributes.Add(VertexAttributeDesc(DataType::Byte4, NORMALIZED, vertFormat.GetColorOffset(i), location));
							location++;
						}
						if (vertFormat.HasSkinning())
						{
							vertexFormat.Attributes.Add(VertexAttributeDesc(DataType::UInt, UNNORMALIZED, vertFormat.GetBoneIdsOffset(), location));
							location++;
							vertexFormat.Attributes.Add(VertexAttributeDesc(DataType::UInt, UNNORMALIZED, vertFormat.GetBoneWeightsOffset(), location));
							location++;
						}

						vertexArrays[vertTypeId] = hardwareRenderer->CreateBuffer(BufferUsage::ArrayBuffer);
						vertexFormats[vertTypeId] = vertexFormat;
					}
					RefPtr<BinaryWriter> vertexWriter;
					if (!vertexBufferWriters.TryGetValue(vertTypeId, vertexWriter))
					{
						vertexWriter = new BinaryWriter(new MemoryStream());
						vertexBufferWriters[vertTypeId] = vertexWriter;
					}
					int startingVertex = (int)(vertexWriter->GetStream()->GetPosition() / vertFormat.GetVertexSize());
					MeshIndexRange range;
					range.MinVertId = startingVertex;
					range.Start = staticIndices.Count();
					vertexWriter->Write((unsigned char*)mesh->GetVertexBuffer(), mesh->GetVertexCount() * mesh->GetVertexSize());
					for (int id : mesh->Indices)
						staticIndices.Add(id + startingVertex);
					range.MaxVertId = (int)(vertexWriter->GetStream()->GetPosition() / vertFormat.GetVertexSize() - 1);
					range.End = staticIndices.Count();
					meshIndexRange[mesh] = range;
				}
				meshIndexBuffer->SetData(staticIndices.Buffer(), staticIndices.Count() * sizeof(int));

				// Write vertex buffers
				for (auto & vWriter : vertexBufferWriters)
				{
					auto ms = ((MemoryStream*)vWriter.Value->GetStream());
					vertexArrays[vWriter.Key]()->SetData(ms->GetBuffer(), (int)ms->GetPosition());
				}

				// Create static uniform buffers
				for (auto & obj : pLevel->StaticActors)
				{
					Align(transformUniformStream.Ptr(), uniformBufferAlignment);
					Align(uniformStream.Ptr(), uniformBufferAlignment);

					auto ctx = new StaticRenderContextImpl();
					obj->RenderContext = ctx;
					meshIndexRange.TryGetValue(obj->Mesh, ctx->MeshRange);
					FillInstanceUniformBuffer(ctx, obj->MaterialInstance, uniformStream.Ptr(), uniformWriter);
					// write model matrix and normal matrix
					ctx->TransformUniformStart = transformUniformStream->GetBufferSize();
					transformUniformWriter.Write(obj->LocalTransform);
					Matrix4 tmpMatrix = obj->LocalTransform;
					tmpMatrix.values[12] = tmpMatrix.values[13] = tmpMatrix.values[14] = 0.0f;
					Matrix4 normalMatrix;
					tmpMatrix.Inverse(normalMatrix);
					normalMatrix.Transpose();
					transformUniformWriter.Write(normalMatrix);
					ctx->TransformUniformEnd = transformUniformStream->GetBufferSize();
				}
				staticInstanceUniformBuffer->SetData(uniformStream->GetBuffer(), uniformStream->GetBufferSize());
				staticTransformUniformBuffer->SetData(transformUniformStream->GetBuffer(), transformUniformStream->GetBufferSize());

				// Create static contexts
				for (auto& obj : pLevel->StaticActors)
				{
					auto ctx = (StaticRenderContextImpl*)obj->RenderContext.Ptr();
					MaterialInstance material = LoadMaterial(obj->MaterialInstance, obj->Mesh->GetVertexFormat(), String(L"StaticMesh") + lightingString);

					PipelineBinding pipelineBinding;
					pipelineBinding.BindUniformBuffer(0, staticTransformUniformBuffer.Ptr(), ctx->TransformUniformStart, ctx->TransformUniformEnd - ctx->TransformUniformStart);
					pipelineBinding.BindUniformBuffer(1, sysUniformBuffer.Ptr());
					pipelineBinding.BindUniformBuffer(2, staticInstanceUniformBuffer.Ptr(), ctx->InstanceUniformStart, ctx->InstanceUniformEnd - ctx->InstanceUniformStart);
					int k = 0;
					int offset = (api == RenderAPI::Vulkan) ? 4 : 0;
					for (auto texture : ctx->Textures)
						pipelineBinding.BindTexture(offset + k++, texture, textureSampler.Ptr());

					ctx->pipelineInstance = material.pipeline->CreateInstance(pipelineBinding);
				}
			}

			// Load dynamic objects
			{
				RefPtr<MemoryStream> uniformStream = new CoreLib::IO::MemoryStream();
				BinaryWriter uniformWriter(uniformStream);
				RefPtr<MemoryStream> transformUniformStream = new CoreLib::IO::MemoryStream();
				BinaryWriter transformUniformWriter(transformUniformStream);

				// Update the dynamic uniform buffers
				for (auto & actor : pLevel->GeneralActors)
				{
					if (actor->GetEngineType() == EngineActorType::SkeletalMesh)
					{
						auto skeletalActor = (SkeletalMeshActor*)actor.Ptr();
						auto ctx = (SkeletalMeshRenderContextImpl*)skeletalActor->RenderContext.Ptr();
						if (!ctx)
						{
							ctx = new SkeletalMeshRenderContextImpl();
							skeletalActor->RenderContext = ctx;
						}
						meshIndexRange.TryGetValue(skeletalActor->Mesh, ctx->MeshRange);
						FillInstanceUniformBuffer(ctx, skeletalActor->MaterialInstance, uniformStream.Ptr(), uniformWriter);

						ctx->TransformUniformStart = transformUniformStream->GetBufferSize();
						List<Matrix4> matrices;
						skeletalActor->GetCurrentPose().GetMatrices(skeletalActor->Skeleton, matrices);
						for (int i = 0; i < matrices.Count(); i++)
						{
							transformUniformWriter.Write(matrices[i]);
							Matrix4 normMat;
							matrices[i].Inverse(normMat);
							normMat.Transpose();
							transformUniformWriter.Write(normMat.values, 4);
							transformUniformWriter.Write(normMat.values + 4, 4);
							transformUniformWriter.Write(normMat.values + 8, 4);
						}
						ctx->TransformUniformEnd = transformUniformStream->GetBufferSize();
					}
				}

				dynamicInstanceUniformBuffer->SetData(uniformStream->GetBuffer(), uniformStream->GetBufferSize());
				skeletalTransformBuffer->SetData(transformUniformStream->GetBuffer(), transformUniformStream->GetBufferSize());

				// Create dynamic contexts
				for (auto & actor : pLevel->GeneralActors)
				{
					if (actor->GetEngineType() == EngineActorType::SkeletalMesh)
					{
						auto skeletalActor = (SkeletalMeshActor*)actor.Ptr();
						auto ctx = (SkeletalMeshRenderContextImpl*)skeletalActor->RenderContext.Ptr();
						MaterialInstance material = LoadMaterial(skeletalActor->MaterialInstance, skeletalActor->Mesh->GetVertexFormat(), String(L"SkeletalMesh") + lightingString);

						PipelineBinding pipelineBinding;
						pipelineBinding.BindUniformBuffer(1, sysUniformBuffer.Ptr());
						pipelineBinding.BindUniformBuffer(2, dynamicInstanceUniformBuffer.Ptr(), ctx->InstanceUniformStart, ctx->InstanceUniformEnd - ctx->InstanceUniformStart);
						pipelineBinding.BindStorageBuffer(3, skeletalTransformBuffer.Ptr(), ctx->TransformUniformStart, ctx->TransformUniformEnd - ctx->TransformUniformStart);
						int k = 0;
						int offset = (api == RenderAPI::Vulkan) ? 4 : 0;
						for (auto texture : ctx->Textures)
							pipelineBinding.BindTexture(offset + k++, texture, textureSampler.Ptr());

						ctx->pipelineInstance = material.pipeline->CreateInstance(pipelineBinding);
					}
				}
			}

			level = pLevel;
			RecordClearCommandBuffer();
			RecordStaticCommandBuffer();
			RecordDynamicCommandBuffer();
		}
		virtual void TakeSnapshot() override
		{
			if (!level)
				return;

			//TODO: The code between these two TODO: blocks consumes a lot of memory
			RefPtr<MemoryStream> uniformStream = new CoreLib::IO::MemoryStream();
			BinaryWriter uniformWriter(uniformStream);
			RefPtr<MemoryStream> transformUniformStream = new CoreLib::IO::MemoryStream();
			BinaryWriter transformUniformWriter(transformUniformStream);

			// Update the dynamic uniform buffers
			for (auto & actor : level->GeneralActors)
			{
				if (actor->GetEngineType() == EngineActorType::SkeletalMesh)
				{
					auto skeletalActor = (SkeletalMeshActor*)actor.Ptr();
					auto ctx = (SkeletalMeshRenderContextImpl*)skeletalActor->RenderContext.Ptr();
					if (!ctx)
					{
						ctx = new SkeletalMeshRenderContextImpl();
						skeletalActor->RenderContext = ctx;
					}
					meshIndexRange.TryGetValue(skeletalActor->Mesh, ctx->MeshRange);
					FillInstanceUniformBuffer(ctx, skeletalActor->MaterialInstance, uniformStream.Ptr(), uniformWriter);

					ctx->TransformUniformStart = transformUniformStream->GetBufferSize();
					List<Matrix4> matrices;
					skeletalActor->GetCurrentPose().GetMatrices(skeletalActor->Skeleton, matrices);
					for (int i = 0; i < matrices.Count(); i++)
					{
						transformUniformWriter.Write(matrices[i]);
						Matrix4 normMat;
						matrices[i].Inverse(normMat);
						normMat.Transpose();
						transformUniformWriter.Write(normMat.values, 4);
						transformUniformWriter.Write(normMat.values + 4, 4);
						transformUniformWriter.Write(normMat.values + 8, 4);
					}
					ctx->TransformUniformEnd = transformUniformStream->GetBufferSize();
				}
			}

			dynamicInstanceUniformBuffer->SetData(uniformStream->GetBuffer(), uniformStream->GetBufferSize());
			skeletalTransformBuffer->SetData(transformUniformStream->GetBuffer(), transformUniformStream->GetBufferSize());

			// Create dynamic contexts
			//for (auto & actor : level->GeneralActors)
			//{
			//	if (actor->GetEngineType() == EngineActorType::SkeletalMesh)
			//	{
			//		auto skeletalActor = (SkeletalMeshActor*)actor.Ptr();
			//		auto ctx = (SkeletalMeshRenderContextImpl*)skeletalActor->RenderContext.Ptr();
			//		MaterialInstance material = LoadMaterial(skeletalActor->MaterialInstance, skeletalActor->Mesh->GetVertexFormat(), String(L"SkeletalMesh") + lightingString);

			//		PipelineBinding pipelineBinding;
			//		pipelineBinding.BindUniformBuffer(1, sysUniformBuffer.Ptr());
			//		pipelineBinding.BindUniformBuffer(2, dynamicInstanceUniformBuffer.Ptr(), ctx->InstanceUniformStart, ctx->InstanceUniformEnd - ctx->InstanceUniformStart);
			//		pipelineBinding.BindStorageBuffer(3, skeletalTransformBuffer.Ptr(), ctx->TransformUniformStart, ctx->TransformUniformEnd - ctx->TransformUniformStart);
			//		int k = 0;
			//		int offset = (api == RenderAPI::Vulkan) ? 4 : 0;
			//		for (auto texture : ctx->Textures)
			//			pipelineBinding.BindTexture(offset + k++, texture, textureSampler.Ptr());

			//		ctx->pipelineInstance = material.pipeline->CreateInstance(pipelineBinding);
			//	}
			//}

			RecordDynamicCommandBuffer();
			//TODO: See top of function

			// Update system uniforms
			if (level->CurrentCamera)
			{
				this->sysUniforms.CameraPos = Vec4::Create(level->CurrentCamera->GetPosition(), 1.0f);
				this->sysUniforms.ViewTransform = level->CurrentCamera->LocalTransform;
				Matrix4 projMatrix;
				Matrix4::CreatePerspectiveMatrixFromViewAngle(projMatrix,
					level->CurrentCamera->FOV, screenWidth / (float)screenHeight,
					level->CurrentCamera->ZNear, level->CurrentCamera->ZFar);
				Matrix4::Multiply(this->sysUniforms.ViewProjectionTransform, projMatrix, this->sysUniforms.ViewTransform);
			}
			else
			{
				this->sysUniforms.CameraPos = Vec4::Create(0.0f);
				Matrix4::CreateIdentityMatrix(this->sysUniforms.ViewTransform);
				Matrix4::CreatePerspectiveMatrixFromViewAngle(this->sysUniforms.ViewProjectionTransform,
					75.0f, screenWidth / (float)screenHeight, 40.0f, 200000.0f);
			}
			this->sysUniforms.ViewTransform.Inverse(this->sysUniforms.InvViewTransform);
			this->sysUniforms.ViewProjectionTransform.Inverse(this->sysUniforms.InvViewProjTransform);
			sysUniformBuffer->SetData(&sysUniforms, sizeof(SystemUniforms));
		}
		virtual void RenderFrame() override
		{
			if (!level) return;
#if !CLEAR_INDIVIDUAL_TEX
			Array<GameEngine::CommandBuffer*, 3> commandBufferList1;
			commandBufferList1.Add(clearCommandBuffer.Ptr());
#else
			Array<GameEngine::CommandBuffer*, 2> commandBufferList1;
#endif
			commandBufferList1.Add(staticCommandBuffer.Ptr());
			commandBufferList1.Add(dynamicCommandBuffer.Ptr());
#if DEFERRED
			Array<GameEngine::CommandBuffer*, 1> commandBufferList2;
			commandBufferList2.Add(deferredCommandBuffer.Ptr());
#endif
#if CLEAR_INDIVIDUAL_TEX
			hardwareRenderer->ClearTexture(presentTexture.Ptr());
			hardwareRenderer->ClearTexture(depthTexture.Ptr());
#if DEFERRED
			hardwareRenderer->ClearTexture(colorTexture.Ptr());
			hardwareRenderer->ClearTexture(pbrTexture.Ptr());
			hardwareRenderer->ClearTexture(normalTexture.Ptr());
#endif
#endif
			hardwareRenderer->ExecuteCommandBuffers(renderTargetLayout.Ptr(), frameBuffer.Ptr(), commandBufferList1.GetArrayView());
#if DEFERRED
			hardwareRenderer->ExecuteCommandBuffers(deferredLayout.Ptr(), deferredFrameBuffer.Ptr(), commandBufferList2.GetArrayView());
#endif
			//hardwareRenderer->Present(presentTexture.Ptr());
		}
		virtual void DestroyContext() override
		{
			textures.Clear();
			shaders.Clear();
		}
		virtual void Resize(int w, int h) override
		{
			presentTexture = hardwareRenderer->CreateTexture2D(TextureUsage::ColorAttachment);
			presentTexture->SetData(StorageFormat::RGBA_8, w, h, 1, DataType::Byte4, nullptr, false);
#if DEFERRED
			colorTexture = hardwareRenderer->CreateTexture2D(TextureUsage::SampledColorAttachment);
			colorTexture->SetData(StorageFormat::RGBA_8, w, h, 1, DataType::Byte4, nullptr, false);
			pbrTexture = hardwareRenderer->CreateTexture2D(TextureUsage::SampledColorAttachment);
			pbrTexture->SetData(StorageFormat::RGBA_8, w, h, 1, DataType::Byte4, nullptr, false);
			normalTexture = hardwareRenderer->CreateTexture2D(TextureUsage::SampledColorAttachment);
			normalTexture->SetData(StorageFormat::RGBA_8, w, h, 1, DataType::Byte4, nullptr, false);
			depthTexture = hardwareRenderer->CreateTexture2D(TextureUsage::SampledDepthAttachment);
			depthTexture->SetData(StorageFormat::Depth32, w, h, 1, DataType::Byte4, nullptr, false);
#else
			depthTexture = hardwareRenderer->CreateTexture2D(TextureUsage::DepthAttachment);
			depthTexture->SetData(StorageFormat::Depth32, w, h, 1, DataType::Byte4, nullptr, false);
#endif

			RenderAttachments renderAttachments;
#if DEFERRED
			renderAttachments.SetAttachment(0, colorTexture.Ptr());
			renderAttachments.SetAttachment(1, pbrTexture.Ptr());
			renderAttachments.SetAttachment(2, normalTexture.Ptr());
			renderAttachments.SetAttachment(3, depthTexture.Ptr());
#else
			renderAttachments.SetAttachment(0, presentTexture.Ptr());
			renderAttachments.SetAttachment(1, depthTexture.Ptr());
#endif

			frameBuffer = renderTargetLayout->CreateFrameBuffer(renderAttachments);

#if DEFERRED
			RenderAttachments deferredAttachments;
			deferredAttachments.SetAttachment(0, presentTexture.Ptr());

			deferredFrameBuffer = deferredLayout->CreateFrameBuffer(deferredAttachments);
#endif

			hardwareRenderer->Resize(w, h);

			screenWidth = w;
			screenHeight = h;

			RecordClearCommandBuffer();
			RecordDeferredCommandBuffer();
			RecordStaticCommandBuffer();
			RecordDynamicCommandBuffer();
		}
		Texture2D * GetRenderedImage()
		{
			return presentTexture.Ptr();
		}
	};

	Renderer* CreateRenderer(WindowHandle window, RenderAPI api)
	{
		return new RendererImpl(window, api);
	}
}