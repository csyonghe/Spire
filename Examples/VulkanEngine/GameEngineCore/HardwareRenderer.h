#ifndef HARDWARE_RENDERER_H
#define HARDWARE_RENDERER_H

#include "CoreLib/Basic.h"

namespace GameEngine
{
	/*
	 * Exceptions
	 */
	class HardwareRendererException : public CoreLib::Exception
	{
	public:
		HardwareRendererException()
		{}
		HardwareRendererException(const CoreLib::String & message)
			: CoreLib::Exception(message)
		{
		}
	};

	/*
	 * Enum Classes
	 */
	enum class BlendMode
	{
		Replace, Add, AlphaBlend
	};

	enum class BufferAccess
	{
		//TODO: Add more types
		Read, Write, ReadWrite, ReadWritePersistent
	};

	enum class BufferStorageFlag
	{
		DynamicStorage = 0x1,
		MapRead = 0x2,
		MapWrite = 0x4,
		MapPersistent = 0x8,
		MapCoherent = 0xf,
		ClientStorage = 0x10
	};

	enum class BufferType
	{
		ArrayBuffer, ElementBuffer, UniformBuffer, StorageBuffer
	};

	enum class BufferUsage
	{
		ArrayBuffer, IndexBuffer, UniformBuffer, StorageBuffer
	};

	enum class CompareFunc
	{
		Disabled, Greater, GreaterEqual, Less, LessEqual, Equal, NotEqual, Always, Never
	};

	enum class CullMode
	{
		Disabled, CullBackFace, CullFrontFace
	};

	enum class BindingType
	{
		Unused, UniformBuffer, StorageBuffer, Texture
	};

	enum class DataType
	{
		Byte = 0x10, Byte2 = 0x11, Byte3 = 0x12, Byte4 = 0x13,
		Char = 0x60, Char2 = 0x61, Char3 = 0x62, Char4 = 0x63,
		Short = 0x20, Short2 = 0x21, Short3 = 0x22, Short4 = 0x23,
		UShort = 0x70, UShort2 = 0x71, UShort3 = 0x72, UShort4 = 0x73,
		Half = 0x90, Half2 = 0x91, Half3 = 0x92, Half4 = 0x93,
		Int = 0x40, Int2 = 0x41, Int3 = 0x42, Int4 = 0x43,
		UInt = 0x100,
		Float = 0x50, Float2 = 0x51, Float3 = 0x52, Float4 = 0x53,
		UInt4_10_10_10_2 = 0x83
	};

	enum class FeedbackStorageMode
	{
		Interleaved, Split
	};

	enum class PrimitiveMode
	{
		Points = 0, Triangles = 0x004
	};

	enum class PrimitiveType
	{
		Points = 0, Lines = 1, LineStrips = 3, Triangles = 4, TriangleStrips = 5, TriangleFans = 6, Quads = 7, Patches = 14
	};

	enum class ShaderDataType
	{
		Float, Int, Float2, Int2, Float3, Int3, Float4, Int4,
		Float3x3, Float4x4, Sampler2D, SamplerCube, Sampler2DMS, SamplerBuffer
	};

	enum class ShaderType
	{
		VertexShader, FragmentShader, HullShader, DomainShader, ComputeShader
	};

	enum class StencilOp : char
	{
		Keep, Zero, Replace, Increment, IncrementWrap, Decrement, DecrementWrap, Invert
	};

	enum class StorageFormat
	{
		Int8, Int16, Int32_Raw,
		Float16, Float32,
		RG_I8, RG_I16, RG_I32_Raw,
		RG_F16, RG_F32,
		RGB_I8, RGB_I16, RGB_I32_Raw,
		RGB_F16, RGB_F32,
		RGBA_8, RGBA_I8, RGBA_I16, RGBA_I32_Raw,
		RGBA_F16, RGBA_F32,
		RGBA_Compressed, R11F_G11F_B10F, RGB10_A2,
		BC1, BC5,
		Depth32, Depth24Stencil8
	};

	enum class TextureCubeFace
	{
		PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ
	};

	enum class TextureFilter
	{
		Nearest, Linear, Trilinear, Anisotropic4x, Anisotropic8x, Anisotropic16x
	};

	enum class TextureUsage
	{
		Unused = 0x0,
		Sampled = 0x1,
		ColorAttachment = 0x2,
		SampledColorAttachment = 0x3,
		DepthAttachment = 0x4,
		SampledDepthAttachment = 0x5
	};

	inline constexpr TextureUsage operator&(TextureUsage lhs, TextureUsage rhs)
	{
		return static_cast<TextureUsage>(static_cast<int>(lhs) & static_cast<int>(rhs));
	}

	inline constexpr TextureUsage operator|(TextureUsage lhs, TextureUsage rhs)
	{
		return static_cast<TextureUsage>(static_cast<int>(lhs) | static_cast<int>(rhs));
	}

	inline constexpr bool operator!(TextureUsage x)
	{
		return x == TextureUsage::Unused;
	}

	enum class WrapMode
	{
		Repeat, Clamp, Mirror
	};

	enum class LoadOp
	{
		Load, Clear, DontCare
	};

	enum class StoreOp
	{
		Store, DontCare
	};



	// Helper Functions
	// Returns size in bytes of a StorageFormat
	inline int StorageFormatSize(StorageFormat format)
	{
		switch (format)
		{
		case StorageFormat::Int8:
			return 1;
		case StorageFormat::RG_I8:
		case StorageFormat::Int16:
		case StorageFormat::Float16:
			return 2;
		case StorageFormat::RGB_I8:
			return 3;
		case StorageFormat::RGBA_8:
		case StorageFormat::RGBA_I8:
		case StorageFormat::RG_F16:
		case StorageFormat::RG_I16:
		case StorageFormat::Int32_Raw:
		case StorageFormat::Float32:
		case StorageFormat::R11F_G11F_B10F:
		case StorageFormat::RGB10_A2:
		case StorageFormat::Depth32:
		case StorageFormat::Depth24Stencil8:
			return 4;
		case StorageFormat::RGB_I16:
		case StorageFormat::RGB_F16:
			return 6;
		case StorageFormat::RGBA_I16:
		case StorageFormat::RGBA_F16:
		case StorageFormat::RG_I32_Raw:
		case StorageFormat::RG_F32:
			return 8;
		case StorageFormat::RGB_I32_Raw:
		case StorageFormat::RGB_F32:
			return 12;
		case StorageFormat::RGBA_I32_Raw:
		case StorageFormat::RGBA_F32:
			return 16;
		case StorageFormat::BC1:
		case StorageFormat::BC5:
		case StorageFormat::RGBA_Compressed:
		default: throw HardwareRendererException(L"Unsupported storage format.");
		}
	}
	// Returns size in bytes of a DataType
	inline int DataTypeSize(DataType type)
	{
		switch (type)
		{
		case DataType::Byte:
		case DataType::Char:
			return 1;
		case DataType::Byte2:
		case DataType::Char2:
		case DataType::Short:
		case DataType::UShort:
		case DataType::Half:
			return 2;
		case DataType::Byte3:
		case DataType::Char3:
			return 3;
		case DataType::Byte4:
		case DataType::Char4:
		case DataType::Short2:
		case DataType::UShort2:
		case DataType::Half2:
		case DataType::Int:
		case DataType::UInt:
		case DataType::Float:
		case DataType::UInt4_10_10_10_2:
			return 4;
		case DataType::Short3:
		case DataType::UShort3:
		case DataType::Half3:
			return 6;
		case DataType::Short4:
		case DataType::UShort4:
		case DataType::Half4:
		case DataType::Int2:
		case DataType::Float2:
			return 8;
		case DataType::Int3:
		case DataType::Float3:
			return 12;
		case DataType::Int4:
		case DataType::Float4:
			return 16;
		default:
			throw HardwareRendererException(L"Unsupported data type.");
		}
	}

	// Returns number of elements for a specific DataType
	inline int NumDataTypeElems(DataType type)
	{
		switch (type)
		{
		case DataType::Byte:
		case DataType::Char:
		case DataType::Short:
		case DataType::UShort:
		case DataType::Half:
		case DataType::Int:
		case DataType::UInt:
		case DataType::Float:
			return 1;
		case DataType::Byte2:
		case DataType::Char2:
		case DataType::Short2:
		case DataType::UShort2:
		case DataType::Half2:
		case DataType::Int2:
		case DataType::Float2:
			return 2;
		case DataType::Byte3:
		case DataType::Char3:
		case DataType::Short3:
		case DataType::UShort3:
		case DataType::Half3:
		case DataType::Int3:
		case DataType::Float3:
			return 3;
		case DataType::Byte4:
		case DataType::Char4:
		case DataType::Short4:
		case DataType::UShort4:
		case DataType::Half4:
		case DataType::Int4:
		case DataType::UInt4_10_10_10_2:
		case DataType::Float4:
			return 4;
		default:
			throw HardwareRendererException(L"Unsupported data type.");
		}
	}

	/*
	* ???
	*/
	class VertexAttributeDesc
	{
	public:
		DataType Type;
		int Normalized : 1;
		int StartOffset : 31;
		int Location;
		VertexAttributeDesc()
		{
			Location = -1;
		}
		VertexAttributeDesc(DataType type, int normalized, int offset, int location)
		{
			this->Type = type;
			this->Normalized = normalized;
			this->StartOffset = offset;
			this->Location = location;
		}
	};
	
	class VertexFormat
	{
	public:
		CoreLib::List<VertexAttributeDesc> Attributes;
	};

	/*
	 * Object Classes
	 */
	class Buffer : public CoreLib::Object
	{
	protected:
		Buffer() {};
	public:
		virtual void SetData(int offset, void* data, int size) = 0;
		virtual void SetData(void* data, int size) = 0;
		//virtual void GetData(int offset, int size) = 0;
		//virtual void GetData() = 0;
		virtual int GetSize() = 0;
		virtual void* Map(int offset, int size) = 0;
		virtual void* Map() = 0;
		virtual void Flush(int offset, int size) = 0;
		virtual void Flush() = 0;
		virtual void Unmap() = 0;
	};

	class Texture2D : public CoreLib::Object
	{
	protected:
		Texture2D() {};
	public:
		virtual void GetSize(int& width, int& height) = 0;
		virtual void Resize(int width, int height, int samples, int miplevels = 1, bool preserveData = false) = 0;
		virtual void SetData(StorageFormat format, int level, int width, int height, int samples, DataType inputType, void* data, bool mipmapped = true) = 0;
		virtual void SetData(StorageFormat format, int width, int height, int samples, DataType inputType, void* data, bool mipmapped = true) = 0;
		virtual void GetData(int mipLevel, void* data, int bufSize) = 0;
		virtual void BuildMipmaps() = 0;
	};

	class TextureSampler : public CoreLib::Object
	{
	protected:
		TextureSampler() {};
	public:
		virtual TextureFilter GetFilter() = 0;
		virtual void SetFilter(TextureFilter filter) = 0;
		virtual WrapMode GetWrapMode() = 0;
		virtual void SetWrapMode(WrapMode wrap) = 0;
		virtual CompareFunc GetCompareFunc() = 0;
		virtual void SetDepthCompare(CompareFunc op) = 0;
	};

	class Shader : public CoreLib::Object
	{
	protected:
		Shader() {};
	};

	class FrameBuffer : public CoreLib::Object
	{
	protected:
		FrameBuffer() {};
	};

	class RenderAttachments
	{
	public:
		int width = -1;
		int height = -1;
		CoreLib::List<Texture2D*> attachments;
	private:
		void Resize(int size)
		{
			if (attachments.Count() < size)
			{
				attachments.SetSize(size);
			}
		}
	public:
		RenderAttachments() {}
		RenderAttachments(CoreLib::ArrayView<Texture2D*> pAttachments)
		{
			attachments.AddRange(pAttachments.Buffer(), pAttachments.Count());
			attachments[0]->GetSize(width, height);
		}
		void SetAttachment(int binding, GameEngine::Texture2D* attachment)
		{
			if (width == -1 && height == -1)
			{
				attachment->GetSize(width, height);
			}
#if _DEBUG
			else
			{
				int thiswidth;
				int thisheight;
				dynamic_cast<Texture2D*>(attachment)->GetSize(thiswidth, thisheight);
				if (thiswidth != width || thisheight != height)
					throw HardwareRendererException(L"Attachment images must have the same dimensions.");
			}
#endif
			Resize(binding + 1);

			attachments[binding] = attachment;
		}
	};

	class RenderTargetLayout : public CoreLib::Object
	{
	protected:
		RenderTargetLayout() {}
	public:
		virtual FrameBuffer* CreateFrameBuffer(const RenderAttachments& attachments) = 0;
	};

	struct TextureBinding
	{
		Texture2D* texture;
		TextureSampler* sampler;
	};
	struct BufferBinding
	{
		Buffer* buffer;
		int offset;
		int range;
	};
	struct BindingDescription
	{
		BindingType type;
		int location;
		union
		{
			TextureBinding tex;
			BufferBinding buf;
		};
	};

	class PipelineBinding
	{
	private:		
		CoreLib::List<BindingDescription> bindings;

	public:
		const CoreLib::List<BindingDescription>& GetBindings() const
		{
			return bindings;
		}

		void BindUniformBuffer(int binding, Buffer* buffer, int offset, int range)
		{
			BindingDescription description;
			description.type = BindingType::UniformBuffer;
			description.location = binding;
			description.buf.buffer = buffer;
			description.buf.offset = offset;
			description.buf.range = range;

			bindings.Add(description);
		}
		void BindUniformBuffer(int binding, Buffer* buffer)
		{
			BindingDescription description;
			description.type = BindingType::UniformBuffer;
			description.location = binding;
			description.buf.buffer = buffer;
			description.buf.offset = 0;
			description.buf.range = 0;

			bindings.Add(description);
		}
		void BindStorageBuffer(int binding, Buffer* buffer, int offset, int range)
		{
			BindingDescription description;
			description.type = BindingType::StorageBuffer;
			description.location = binding;
			description.buf.buffer = buffer;
			description.buf.offset = offset;
			description.buf.range = range;

			bindings.Add(description);
		}
		void BindStorageBuffer(int binding, Buffer* buffer)
		{
			BindingDescription description;
			description.type = BindingType::StorageBuffer;
			description.location = binding;
			description.buf.buffer = buffer;
			description.buf.offset = 0;
			description.buf.range = 0;

			bindings.Add(description);
		}
		void BindTexture(int binding, Texture2D* texture, TextureSampler* sampler)
		{
			BindingDescription description;
			description.type = BindingType::Texture;
			description.location = binding;
			description.tex.texture = texture;
			description.tex.sampler = sampler;

			bindings.Add(description);
		}
	};

	class PipelineInstance : public CoreLib::Object
	{
	protected:
		PipelineInstance() {}
	};

	class Pipeline : public CoreLib::Object
	{
	protected:
		Pipeline() {}
	public:
		virtual PipelineInstance* CreateInstance(const PipelineBinding& pipelineBinding) = 0;
	};

	class PipelineBuilder : public CoreLib::Object
	{
	protected:
		PipelineBuilder() {}
	public:
		bool PrimitiveRestartEnabled = false;
		PrimitiveType PrimitiveTopology = GameEngine::PrimitiveType::Triangles;
		int PatchSize = 3;
		CompareFunc DepthCompareFunc = CompareFunc::Disabled, StencilCompareFunc = CompareFunc::Disabled;
		StencilOp StencilFailOp = StencilOp::Keep, StencilDepthFailOp = StencilOp::Keep, StencilDepthPassOp = StencilOp::Keep;
		BlendMode BlendMode = GameEngine::BlendMode::Replace;
		unsigned int StencilMask = 0xFFFFFFFF;
		unsigned int StencilReference = 0;
		CullMode CullMode = GameEngine::CullMode::CullBackFace;

		virtual void SetShaders(CoreLib::ArrayView<Shader*> shaders) = 0;
		virtual void SetVertexLayout(VertexFormat vertexFormat) = 0;
		virtual void SetBindingLayout(int bindingId, BindingType bindType) = 0;
		virtual Pipeline* ToPipeline(RenderTargetLayout* renderTargetLayout) = 0;
	};

	class CommandBuffer : public CoreLib::Object
	{
	protected:
		CommandBuffer() {};
	public:
		// Specifying the FrameBuffer can result in better performance, but will need to be re-recorded when FrameBuffer changes
		virtual void BeginRecording(RenderTargetLayout* renderTargetLayout, FrameBuffer* frameBuffer) = 0;
		// Not specifying a specific FrameBuffer may result in worse performance, but can be used with any compatible FrameBuffer
		virtual void BeginRecording(RenderTargetLayout* renderTargetLayout) = 0;
		virtual void EndRecording() = 0;
		virtual void SetViewport(int x, int y, int width, int height) = 0;
		virtual void BindVertexBuffer(Buffer* vertexBuffer) = 0;
		virtual void BindIndexBuffer(Buffer* indexBuffer) = 0;
		virtual void BindPipeline(PipelineInstance* pipelineInstance) = 0;
		virtual void Draw(int firstVertex, int vertexCount) = 0;
		virtual void DrawInstanced(int numInstances, int firstVertex, int vertexCount) = 0;
		virtual void DrawIndexed(int firstIndex, int indexCount) = 0;
		virtual void DrawIndexedInstanced(int numInstances, int firstIndex, int indexCount) = 0;
		virtual void Blit(Texture2D* dstImage, Texture2D* srcImage) = 0;
		virtual void ClearAttachments(RenderAttachments renderAttachments) = 0;
	};

	class HardwareRenderer : public CoreLib::Object
	{
	protected:
		HardwareRenderer() {};
	public:
		virtual void BindWindow(void* windowHandle, int width, int height) = 0;
		virtual void * GetWindowHandle() = 0;
		virtual void Resize(int width, int height) = 0;
		virtual void ClearTexture(GameEngine::Texture2D* texture) = 0;
		virtual void ExecuteCommandBuffers(RenderTargetLayout* renderTargetLayout, FrameBuffer* frameBuffer, CoreLib::ArrayView<CommandBuffer*> commands) = 0;
		virtual void Present(Texture2D* srcImage) = 0;
		virtual void Blit(Texture2D* dstImage, Texture2D* srcImage) = 0;
		virtual void Wait() = 0;
		virtual Buffer* CreateBuffer(BufferUsage usage) = 0;
		virtual Buffer* CreateMappedBuffer(BufferUsage usage) = 0;
		virtual Texture2D* CreateTexture2D(TextureUsage usage) = 0;
		virtual TextureSampler * CreateTextureSampler() = 0;
		virtual Shader* CreateShader(ShaderType stage, const char* data, int size) = 0;
		virtual RenderTargetLayout* CreateRenderTargetLayout(CoreLib::ArrayView<TextureUsage> bindings) = 0;
		virtual PipelineBuilder* CreatePipelineBuilder() = 0;
		virtual CommandBuffer* CreateCommandBuffer() = 0;
		virtual CoreLib::String GetSpireBackendName() = 0;
		virtual int UniformBufferAlignment() = 0;
		virtual int StorageBufferAlignment() = 0;
	};

	// HardwareRenderer instance constructors
	HardwareRenderer* CreateGLHardwareRenderer();
	HardwareRenderer* CreateVulkanHardwareRenderer(int gpuId);
}

#endif