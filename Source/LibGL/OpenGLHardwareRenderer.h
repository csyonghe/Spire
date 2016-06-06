#ifndef OPENGL_HARDWARE_RENDERER_H
#define OPENGL_HARDWARE_RENDERER_H
#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#include "CoreLib/WinForm/Debug.h"
#include "glew/glew.h"
#include "glew/wglew.h"
#include "NVCommandList.h"
#include "CoreLib/VectorMath.h"

namespace GL
{
	const int TargetOpenGLVersion_Major = 4;
	const int TargetOpenGLVersion_Minor = 2;
	using namespace VectorMath;
	class GUIWindow
	{};

	typedef GUIWindow* GUIHandle;

	class HardwareRendererException : public Exception
	{
	public:
		HardwareRendererException()
		{}
		HardwareRendererException(const CoreLib::String & message)
			: Exception(message)
		{
		}
	};

	enum class CullMode
	{
		Disabled, CullBackFace, CullFrontFace
	};

	enum class BlendMode
	{
		Replace, Add, AlphaBlend
	};

	enum class BlendOperator : char
	{
		Disabled, Greater, GreaterEqual, Less, LessEqual, Equal, NotEqual
	};

	enum class CompareFunc
	{
		Disabled, Greater, GreaterEqual, Less, LessEqual, Equal, NotEqual, Always, Never
	};

	enum class StencilOp : char
	{
		Keep, Zero, Replace, Increment, IncrementWrap, Decrement, DecrementWrap, Invert
	};

	class StencilMode
	{
	public:
		union
		{
			struct
			{
				CompareFunc StencilFunc;
				StencilOp Fail, DepthFail, DepthPass;
#pragma warning(suppress : 4201)
			};
			int Bits;
		};
		unsigned int StencilMask, StencilReference;
		bool operator == (const StencilMode & mode)
		{
			return Bits == mode.Bits && StencilMask == mode.StencilMask && StencilReference == mode.StencilReference;
		}
		bool operator != (const StencilMode & mode)
		{
			return Bits != mode.Bits || StencilMask != mode.StencilMask || StencilReference != mode.StencilReference;
		}
		StencilMode()
		{
			StencilFunc = CompareFunc::Disabled;
			Fail = StencilOp::Keep;
			DepthFail = StencilOp::Keep;
			DepthPass = StencilOp::Keep;
			StencilMask = 0xFFFFFFFF;
			StencilReference = 0;
		}
	};

	enum class DataType
	{
		Float = 0x50, Int = 0x40, Float2 = 0x51, Int2 = 0x41, Float3 = 0x52, Int3 = 0x42, Float4 = 0x53, Int4 = 0x43,
		Byte = 0x10, Byte2 = 0x11, Byte3 = 0x12, Byte4 = 0x13,
		Short = 0x20, Short2 = 0x21, Short3 = 0x22, Short4 = 0x23,
		Char = 0x60, Char2 = 0x61, Char3 = 0x62, Char4 = 0x63,
		UShort = 0x70, UShort2 = 0x71, UShort3 = 0x72, UShort4 = 0x73,
		UInt4_10_10_10_2 = 0x83,
		Half = 0x90, Half2 = 0x91, Half3 = 0x92, Half4 = 0x93
	};

	inline DataType GetSingularDataType(DataType type)
	{
		if (type == DataType::UInt4_10_10_10_2)
			return DataType::Int;
		return (DataType)(((int)type) & 0xF0);
	}

	inline int GetDataTypeComponenets(DataType type)
	{
		return (((int)type) & 0xF) + 1;
	}

	enum class ShaderDataType
	{
		Float, Int, Float2, Int2, Float3, Int3, Float4, Int4,
		Float3x3, Float4x4, Sampler2D, SamplerCube, Sampler2DMS, SamplerBuffer
	};

	enum class StorageFormat
	{
		Float32, Int32_Raw, Float16, Int16, Int8,
		RG_F32, RG_F16, RG_I8, RG_I16, RG_I32_Raw,
		RGB_F32, RGB_F16, RGB_I8, RGB_I16, RGB_I32_Raw,
		RGBA_F32, RGBA_F16, RGBA_I8, RGBA_I16, RGBA_I32_Raw,
		RGBA_Compressed, R11F_G11F_B10F, RGB10_A2,
		Depth32, Depth24Stencil8
	};

	enum class TextureFilter
	{
		Nearest, Linear, Trilinear, Anisotropic4x, Anisotropic8x, Anisotropic16x
	};

	enum class WrapMode
	{
		Repeat, Clamp, Mirror
	};

	enum class TextureCubeFace
	{
		PositiveX, NegativeX, PositiveY, NegativeY, PositiveZ, NegativeZ
	};

	enum class BufferUsage
	{
		ArrayBuffer, IndexBuffer, UniformBuffer, ShadeStorageBuffer
	};

	enum class ShaderType
	{
		VertexShader, FragmentShader, ComputeShader
	};

	class VertexAttributeDesc
	{
	public:
		DataType Type;
		int Normalized : 1;
		int StartOffset : 31;
		int Binding;
		VertexAttributeDesc()
		{
			Binding = -1;
		}
		VertexAttributeDesc(DataType type, int normalized, int offset, int binding)
		{
			this->Type = type;
			this->Normalized = normalized;
			this->StartOffset = offset;
			this->Binding = binding;
		}
	};

	enum class PrimitiveType
	{
		Points = 0, Lines = 1, LineLoops = 2, LineStrips = 3, Triangles = 4, TriangleStrips = 5, TriangleFans = 6, 
	};

	enum class PrimitiveMode
	{
		Points = 0, Triangles = 0x004
	};

	enum class FeedbackStorageMode
	{
		Interleaved, Split
	};

	inline int TranslateBufferUsage(BufferUsage usage)
	{
		switch (usage)
		{
		case BufferUsage::ArrayBuffer:
			return GL_ARRAY_BUFFER;
			break;
		case BufferUsage::IndexBuffer:
			return GL_ELEMENT_ARRAY_BUFFER;
			break;
		case BufferUsage::ShadeStorageBuffer:
			return GL_SHADER_STORAGE_BUFFER;
			break;
		case BufferUsage::UniformBuffer:
			return GL_UNIFORM_BUFFER;
			break;
		default:
			throw HardwareRendererException(L"Unsupported buffer usage.");
		}
	}

	inline int TranslateStorageFormat(StorageFormat format)
	{
		int internalFormat = 0;
		switch (format)
		{
		case StorageFormat::Float16:
			internalFormat = GL_R16F;
			break;
		case StorageFormat::Float32:
			internalFormat = GL_R32F;
			break;
		case StorageFormat::Int16:
			internalFormat = GL_R16;
			break;
		case StorageFormat::Int32_Raw:
			internalFormat = GL_R32I;
			break;
		case StorageFormat::Int8:
			internalFormat = GL_R8;
			break;
		case StorageFormat::RG_F16:
			internalFormat = GL_RG16F;
			break;
		case StorageFormat::RG_F32:
			internalFormat = GL_RG32F;
			break;
		case StorageFormat::RG_I16:
			internalFormat = GL_RG16;
			break;
		case StorageFormat::RG_I32_Raw:
			internalFormat = GL_RG32I;
			break;
		case StorageFormat::RG_I8:
			internalFormat = GL_RG8;
			break;
		case StorageFormat::RGB_F16:
			internalFormat = GL_RGB16F;
			break;
		case StorageFormat::RGB_F32:
			internalFormat = GL_RGB32F;
			break;
		case StorageFormat::RGB_I16:
			internalFormat = GL_RGB16;
			break;
		case StorageFormat::RGB_I32_Raw:
			internalFormat = GL_RGB32I;
			break;
		case StorageFormat::RGB_I8:
			internalFormat = GL_RGB8;
			break;
		case StorageFormat::RGBA_F16:
			internalFormat = GL_RGBA16F;
			break;
		case StorageFormat::RGBA_F32:
			internalFormat = GL_RGBA32F;
			break;
		case StorageFormat::R11F_G11F_B10F:
			internalFormat = GL_R11F_G11F_B10F;
			break;
		case StorageFormat::RGB10_A2:
			internalFormat = GL_RGB10_A2;
			break;
		case StorageFormat::RGBA_I16:
			internalFormat = GL_RGBA16;
			break;
		case StorageFormat::RGBA_I32_Raw:
			internalFormat = GL_RGBA32I;
			break;
		case StorageFormat::RGBA_I8:
			internalFormat = GL_RGBA;
			break;
		case StorageFormat::RGBA_Compressed:
#ifdef _DEBUG
			internalFormat = GL_COMPRESSED_RGBA;
#else
			internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
#endif
			break;
		case StorageFormat::Depth24Stencil8:
			internalFormat = GL_DEPTH24_STENCIL8;
			break;
		case StorageFormat::Depth32:
			internalFormat = GL_DEPTH_COMPONENT32;
			break;
		default:
			throw HardwareRendererException(L"Unsupported storage format.");
		}
		return internalFormat;
	}

	inline int TranslateDataTypeToFormat(DataType type)
	{
		switch (GetDataTypeComponenets(type))
		{
		case 1:
			return GL_RED;
		case 2:
			return GL_RG;
		case 3:
			return GL_RGB;
		case 4:
			return GL_RGBA;
		default:
			throw HardwareRendererException(L"Unsupported data type.");
		}
	}

	inline int TranslateDataTypeToInputType(DataType type)
	{
		switch (type)
		{
		case DataType::Int:
		case DataType::Int2:
		case DataType::Int3:
		case DataType::Int4:
			return GL_INT;
			break;
		case DataType::Byte:
		case DataType::Byte2:
		case DataType::Byte3:
		case DataType::Byte4:
			return GL_UNSIGNED_BYTE;
			break;
		case DataType::Char:
		case DataType::Char2:
		case DataType::Char3:
		case DataType::Char4:
			return GL_BYTE;
			break;
		case DataType::Short:
		case DataType::Short2:
		case DataType::Short3:
		case DataType::Short4:
			return GL_SHORT;
		case DataType::UShort:
		case DataType::UShort2:
		case DataType::UShort3:
		case DataType::UShort4:
			return GL_UNSIGNED_SHORT;
		case DataType::Float:
		case DataType::Float2:
		case DataType::Float3:
		case DataType::Float4:
			return GL_FLOAT;
		case DataType::Half:
		case DataType::Half2:
		case DataType::Half3:
		case DataType::Half4:
			return GL_HALF_FLOAT;
		case DataType::UInt4_10_10_10_2:
			return GL_UNSIGNED_INT_2_10_10_10_REV;
		default:
			throw HardwareRendererException(L"Unsupported data type.");
		}
	}

	class GL_Object
	{
	public:
		GLuint Handle;
		GL_Object()
		{
			Handle = 0;
		}
	};

	class RenderBuffer : public GL_Object
	{
	public:
		StorageFormat storageFormat;
		GLint internalFormat;
		StorageFormat GetFormat()
		{
			return storageFormat;
		}
		void GetSize(int & w, int &h)
		{
			glGetNamedRenderbufferParameterivEXT(Handle, GL_RENDERBUFFER_WIDTH, &w);
			glGetNamedRenderbufferParameterivEXT(Handle, GL_RENDERBUFFER_HEIGHT, &h);

		}
		void Resize(int width, int height, int samples)
		{
			if (samples <= 1)
				glNamedRenderbufferStorageEXT(Handle, internalFormat, width, height);
			else
				glNamedRenderbufferStorageMultisampleEXT(Handle, samples, internalFormat, width, height);
		}
	};

	class TextureSampler : public GL_Object
	{
	public:
		TextureFilter GetFilter()
		{
			GLint filter;
			float aniso = 0.0f;
			glGetSamplerParameteriv(Handle, GL_TEXTURE_MIN_FILTER, &filter);
			switch (filter)
			{
			case GL_NEAREST:
				return TextureFilter::Nearest;
			case GL_LINEAR:
				return TextureFilter::Linear;
			case GL_LINEAR_MIPMAP_LINEAR:
				glGetSamplerParameterfv(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, &aniso);
				if (aniso < 3.99f)
					return TextureFilter::Trilinear;
				else if (aniso < 7.99f)
					return TextureFilter::Anisotropic4x;
				else if (aniso < 15.99f)
					return TextureFilter::Anisotropic8x;
				else
					return TextureFilter::Anisotropic16x;
			default:
				return TextureFilter::Trilinear;
			}
		}
		void SetFilter(TextureFilter filter)
		{
			switch (filter)
			{
			case TextureFilter::Nearest:
				glSamplerParameterf(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
				glSamplerParameteri(Handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glSamplerParameteri(Handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				break;
			case TextureFilter::Linear:
				glSamplerParameterf(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
				glSamplerParameteri(Handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glSamplerParameteri(Handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				break;
			case TextureFilter::Trilinear:
				glSamplerParameteri(Handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glSamplerParameteri(Handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glSamplerParameterf(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1.0f);
				break;
			case TextureFilter::Anisotropic4x:
				glSamplerParameteri(Handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glSamplerParameteri(Handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glSamplerParameterf(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4.0f);
				break;
			case TextureFilter::Anisotropic8x:
				glSamplerParameteri(Handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glSamplerParameteri(Handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glSamplerParameterf(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
				break;
			case TextureFilter::Anisotropic16x:
				glSamplerParameteri(Handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glSamplerParameteri(Handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glSamplerParameterf(Handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 16.0f);
				break;
			}
		}
		WrapMode GetWrapMode()
		{
			GLint mode;
			glGetSamplerParameteriv(Handle, GL_TEXTURE_WRAP_S, &mode);
			switch (mode)
			{
			case GL_REPEAT:
				return WrapMode::Repeat;
			default:
				return WrapMode::Clamp;
			}
		}
		void SetWrapMode(WrapMode wrap)
		{
			int mode = GL_CLAMP_TO_EDGE;
			switch (wrap)
			{
			case WrapMode::Clamp:
				mode = GL_CLAMP_TO_EDGE;
				break;
			case WrapMode::Repeat:
				mode = GL_REPEAT;
				break;
			case WrapMode::Mirror:
				mode = GL_MIRRORED_REPEAT;
				break;
			}
			glSamplerParameteri(Handle, GL_TEXTURE_WRAP_S, mode);
			glSamplerParameteri(Handle, GL_TEXTURE_WRAP_T, mode);
			glSamplerParameteri(Handle, GL_TEXTURE_WRAP_R, mode);
		}
		void SetDepthCompare(CompareFunc op)
		{
			if (op == CompareFunc::Disabled)
			{
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_NONE);
			}
			switch (op)
			{
			case CompareFunc::Disabled:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_NONE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
				break;
			case CompareFunc::Equal:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_EQUAL);
				break;
			case CompareFunc::Less:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_LESS);
				break;
			case CompareFunc::Greater:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
				break;
			case CompareFunc::LessEqual:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
				break;
			case CompareFunc::GreaterEqual:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_GEQUAL);
				break;
			case CompareFunc::NotEqual:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_NOTEQUAL);
				break;
			case CompareFunc::Always:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_ALWAYS);
				break;
			case CompareFunc::Never:
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
				glSamplerParameteri(Handle, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
				break;
			}
		}
	};

	class TextureHandle
	{
	public:
		uint64_t Handle = 0;
		TextureHandle() = default;
		TextureHandle(uint64_t handle)
		{
			this->Handle = handle;
		}
		void MakeResident() const
		{
			if (!glIsTextureHandleResidentARB(Handle))
				glMakeTextureHandleResidentARB(Handle);
		}
		void MakeNonResident() const
		{
			if (glIsTextureHandleResidentARB(Handle))
				glMakeTextureHandleNonResidentARB(Handle);
		}
	};

	class Texture : public GL_Object
	{
	public:
		GLuint BindTarget;
		//GLuint64 TextureHandle;
		GLint internalFormat;
		StorageFormat storageFormat;
		GLenum format, type;
		Texture()
		{
			BindTarget = GL_TEXTURE_2D;
			internalFormat = GL_RGBA;
			format = GL_RGBA;
			storageFormat = StorageFormat::RGBA_I8;
			type = GL_UNSIGNED_BYTE;
		}
		bool operator == (const Texture &t) const
		{
			return Handle == t.Handle;
		}
		TextureHandle GetTextureHandle() const
		{
			return TextureHandle(glGetTextureHandleARB(Handle));
		}
		TextureHandle GetTextureHandle(TextureSampler sampler) const
		{
			return TextureHandle(glGetTextureSamplerHandleARB(Handle, sampler.Handle));
		}
	};

	class Texture2D : public Texture
	{
	public:
		Texture2D()
		{
			BindTarget = GL_TEXTURE_2D;
		}
		StorageFormat GetFormat()
		{
			return storageFormat;
		}
		void GetSize(int & width, int & height)
		{
			glBindTexture(GL_TEXTURE_2D, Handle);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		void Clear(Vec4 data)
		{
			glClearTexImage(Handle, 0, format, type, &data);
		}
		void Resize(int width, int height, int samples)
		{
			glBindTexture(GL_TEXTURE_2D, Handle);
			if (samples <= 1)
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);
			else
			{
				glTexImage2DMultisample(GL_TEXTURE_2D, samples, internalFormat, width, height, GL_TRUE);
			}
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		void SetData(StorageFormat pFormat, int level, int width, int height, int samples, DataType inputType, void * data)
		{
			this->storageFormat = pFormat;
			this->internalFormat = TranslateStorageFormat(pFormat);
			this->format = TranslateDataTypeToFormat(inputType);
			this->type = TranslateDataTypeToInputType(inputType);
			if (this->internalFormat == GL_DEPTH_COMPONENT16 || this->internalFormat == GL_DEPTH_COMPONENT24 || this->internalFormat == GL_DEPTH_COMPONENT32)
				this->format = GL_DEPTH_COMPONENT;
			else if (this->internalFormat == GL_DEPTH24_STENCIL8)
				this->format = GL_DEPTH_STENCIL;
				
			if (samples > 1)
			{
				glBindTexture(GL_TEXTURE_2D, Handle);
				glTexImage2DMultisample(GL_TEXTURE_2D, samples, this->internalFormat, width, height, GL_TRUE);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			else
			{
				glBindTexture(GL_TEXTURE_2D, Handle);
				glTexImage2D(GL_TEXTURE_2D, level, this->internalFormat, width, height, 0, this->format, this->type, data);
				glBindTexture(GL_TEXTURE_2D, 0);

			}
		}
		void SetData(StorageFormat pFormat, int width, int height, int samples, DataType inputType, void * data)
		{
			SetData(pFormat, 0, width, height, samples, inputType, data);
		}
		int GetComponents()
		{
			switch (this->format)
			{
			case GL_RED:
				return 1;
			case GL_RG:
				return 2;
			case GL_RGB:
				return 3;
			case GL_RGBA:
				return 4;
			}
			return 4;
		}
		void GetData(int level, DataType outputType, void * data, int /*bufSize*/)
		{
			glBindTexture(GL_TEXTURE_2D, Handle);
			if (format == GL_DEPTH_COMPONENT)
			{
				glGetTexImage(GL_TEXTURE_2D, level, format, GL_UNSIGNED_INT, data);

			}
			else
				glGetTexImage(GL_TEXTURE_2D, level, format, TranslateDataTypeToInputType(outputType), data);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		void DebugDump(String fileName);
		void BuildMipmaps()
		{
			glBindTexture(GL_TEXTURE_2D, Handle);
			glGenerateMipmap(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	};

	class TextureCube : public Texture
	{
	public:
		TextureCube()
		{
			BindTarget = GL_TEXTURE_CUBE_MAP;
		}
		StorageFormat GetFormat()
		{
			return storageFormat;
		}
		void Resize(int width, int height, int samples)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, Handle);
			for (int i = 0; i < 6; i++)
			{
				if (samples <= 1)
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internalFormat, width, height, 0, format, type, 0);
				else
					glTexImage2DMultisample(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, samples, internalFormat, width, height, GL_TRUE);
			}
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}
		void SetData(TextureCubeFace face, StorageFormat pFormat, int width, int height, int samples, DataType inputType, void * data)
		{
			this->internalFormat = TranslateStorageFormat(pFormat);
			this->format = TranslateDataTypeToFormat(inputType);
			this->type = TranslateDataTypeToInputType(inputType);
			if (this->internalFormat == GL_DEPTH_COMPONENT16 || this->internalFormat == GL_DEPTH_COMPONENT24 || this->internalFormat == GL_DEPTH_COMPONENT32)
				this->format = GL_DEPTH_COMPONENT;
			else if (this->internalFormat == GL_DEPTH24_STENCIL8)
				this->format = GL_DEPTH_STENCIL;
			glBindTexture(GL_TEXTURE_CUBE_MAP, Handle);
			if (samples <= 1)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, 0, internalFormat, width, height, 0, this->format, this->type, data);
			else
				glTexImage2DMultisample(GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, samples, internalFormat, width, height, GL_TRUE);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}

		void GetData(TextureCubeFace face, int level, DataType outputType, void * data)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, Handle);
			glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, level, TranslateDataTypeToFormat(outputType), TranslateDataTypeToInputType(outputType), data);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}

		void BuildMipmaps()
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, Handle);
			glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
		}
	};

	class FrameBuffer : public GL_Object
	{
	public:
		static FrameBuffer DefaultFrameBuffer;
		void SetColorRenderTarget(int attachmentPoint, const Texture2D &texture, int level = 0)
		{
			glNamedFramebufferTexture(Handle, GL_COLOR_ATTACHMENT0 + attachmentPoint, texture.Handle, level);
		}
		void SetColorRenderTarget(int attachmentPoint, const TextureCube &texture, TextureCubeFace face)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Handle);
			glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentPoint, GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, texture.Handle, 0);
		}
		void SetColorRenderTarget(int attachmentPoint, const RenderBuffer &buffer)
		{
			glNamedFramebufferRenderbuffer(Handle, GL_COLOR_ATTACHMENT0 + attachmentPoint, buffer.Handle, 0);
		}
		void EnableRenderTargets(int mask)
		{
			Array<GLenum, 16> buffers;
			for (int i = 0; i < 16; i++)
			{
				if (mask & (1 << i))
				{
					buffers.Add(GL_COLOR_ATTACHMENT0 + i);
				}
			}
			//glNamedFramebufferDrawBuffers(Handle, buffers.Count(), buffers.Buffer());
			//if (buffers.Count())
			glNamedFramebufferDrawBuffers(Handle, buffers.Count(), buffers.Buffer());
			/*else
			{
				GLenum none = GL_NONE;
				glDrawBuffers(1, &none);
			}*/
		}

		void SetDepthStencilRenderTarget(const Texture2D &texture)
		{
			if (texture.internalFormat == GL_DEPTH24_STENCIL8)
				glNamedFramebufferTexture(Handle, GL_DEPTH_STENCIL_ATTACHMENT, texture.Handle, 0);
			else
				glNamedFramebufferTexture(Handle, GL_DEPTH_ATTACHMENT, texture.Handle, 0);
		}

		void SetDepthStencilRenderTarget(const TextureCube &texture, TextureCubeFace face)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, Handle);
			if (texture.internalFormat == GL_DEPTH24_STENCIL8)
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X+(int)face, texture.Handle, 0);
			else
				glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_CUBE_MAP_POSITIVE_X + (int)face, texture.Handle, 0);
		}

		void SetDepthStencilRenderTarget(const RenderBuffer &buffer)
		{
			if (buffer.internalFormat == GL_DEPTH24_STENCIL8)
				glNamedFramebufferRenderbuffer(Handle, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffer.Handle);
			else
				glNamedFramebufferRenderbuffer(Handle, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, buffer.Handle);
		}

		void SetStencilRenderTarget(const RenderBuffer &buffer)
		{
			glNamedFramebufferRenderbuffer(Handle, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, buffer.Handle);
		}

		void Check()
		{
			auto rs = glCheckNamedFramebufferStatus(Handle, GL_FRAMEBUFFER);
			if (rs != GL_FRAMEBUFFER_COMPLETE)
			{
				printf("Framebuffer check result: %d", rs);
				throw HardwareRendererException(L"Inconsistent frame buffer object setup.");
			}
		}
	};

	enum class BufferType
	{
		UniformBuffer = GL_UNIFORM_BUFFER, ArrayBuffer = GL_ARRAY_BUFFER, StorageBuffer = GL_SHADER_STORAGE_BUFFER,
		ElementBuffer = GL_ELEMENT_ARRAY_BUFFER,
	};

	class BufferObject : public GL_Object
	{
	public:
		GLuint BindTarget;
		static float CheckBufferData(int bufferHandle)
		{
			float buffer;
			glGetNamedBufferSubData(bufferHandle, 0, 4, &buffer);
			return buffer;
		}
		void SetData(void * data, int sizeInBytes)
		{
			GLenum usage = GL_STATIC_DRAW;
			if (BindTarget == GL_UNIFORM_BUFFER)
				usage = GL_DYNAMIC_READ;
			else if (BindTarget == GL_ARRAY_BUFFER || BindTarget == GL_ELEMENT_ARRAY_BUFFER)
				usage = GL_STREAM_DRAW;
			glNamedBufferData(Handle, sizeInBytes, data, usage);
		}
		void SubData(int offset, int size, void * data)
		{
			glNamedBufferSubData(Handle, (GLintptr)offset, (GLsizeiptr)size, data);
		}
		bool GetData(void * buffer, int & bufferSize)
		{
			int sizeInBytes = 0;
			glGetNamedBufferParameteriv(Handle, GL_BUFFER_SIZE, &sizeInBytes);
			if (sizeInBytes > bufferSize)
			{
				bufferSize = sizeInBytes;
				return false;
			}
			glGetNamedBufferSubData(Handle, 0, sizeInBytes, buffer);
			return true;
		}
		void MakeResident(bool isReadOnly)
		{
			glMakeNamedBufferResidentNV(Handle, isReadOnly ? GL_READ_ONLY : GL_READ_WRITE);
		}
		void MakeNonResident()
		{
			glMakeNamedBufferNonResidentNV(Handle);
		}
		uint64_t GetGpuAddress()
		{
			uint64_t addr;
			glGetNamedBufferParameterui64vNV(Handle, GL_BUFFER_GPU_ADDRESS_NV, &addr);
			return addr;
		}
	};

	class VertexArray : public GL_Object
	{
	public:
		void SetIndex(BufferObject indices)
		{
			//glVertexArrayElementBuffer(Handle, indices->Handle);
			glBindVertexArray(Handle);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices.Handle);
			glBindVertexArray(0);
		}
		void SetVertex(BufferObject vertices, ArrayView<VertexAttributeDesc> attribs, int vertSize, int startId = 0, int instanceDivisor = 0)
		{
			glBindVertexArray(Handle);
			glBindBuffer(GL_ARRAY_BUFFER, vertices.Handle);
			for (int i = 0; i < attribs.Count(); i++)
			{
				//glVertexArrayVertexBuffer(Handle, i, ((GL_BufferObject*)vertices)->Handle, attribs[i].StartOffset, vertSize);
				//glVertexArrayAttribFormat(Handle, i, GetDataTypeComponenets(attribs[i].Type), TranslateDataTypeToInputType(attribs[i].Type), attribs[i].Normalized, attribs[i].StartOffset);
				int id = i + startId;
				if (attribs[i].Binding != -1)
					id = attribs[i].Binding;
				glEnableVertexAttribArray(id);
				glVertexAttribPointer(id, GetDataTypeComponenets(attribs[i].Type), TranslateDataTypeToInputType(attribs[i].Type), attribs[i].Normalized, vertSize, (void*)(CoreLib::PtrInt)attribs[i].StartOffset);
				glVertexAttribDivisor(id, instanceDivisor);
			}

			glBindVertexArray(0);
		}
	};

	class TransformFeedback : public GL_Object
	{
	public:
		void BindBuffer(int index, BufferObject buffer)
		{
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, Handle);
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, index, buffer.Handle);
		}
		void Use(PrimitiveMode mode)
		{
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, Handle);
			glBeginTransformFeedback((int)mode);
		}
		void Unuse()
		{
			glEndTransformFeedback();
			glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
		}
	};

	class Shader : public GL_Object
	{
	public:
		long long GetHandle()
		{
			return Handle;
		}
	};

	class Program : public GL_Object 
	{
	public:
		void StorageBlockBinding(String blockName, int binding)
		{
			int index = glGetProgramResourceIndex(Handle, GL_SHADER_STORAGE_BLOCK, blockName.ToMultiByteString());
			glShaderStorageBlockBinding(Handle, index, binding);
		}
		void UniformBlockBinding(String blockName, int binding)
		{
			int index = glGetProgramResourceIndex(Handle, GL_UNIFORM_BLOCK, blockName.ToMultiByteString());
			glUniformBlockBinding(Handle, index, binding);
		}
		void UniformBlockBinding(int index, int binding)
		{
			glUniformBlockBinding(Handle, index, binding);
		}
		int GetAttribBinding(String name)
		{
			return glGetAttribLocation(Handle, name.ToMultiByteString());
		}
		int GetOutputLocation(String name)
		{
			return glGetFragDataLocation(Handle, name.ToMultiByteString());
		}
		void SetUniform(String name, unsigned int value)
		{
			int loc = glGetUniformLocation(Handle, name.ToMultiByteString());
			if (loc != -1)
				glProgramUniform1ui(Handle, loc, value);
		}
		void SetUniform(String name, int value)
		{
			int loc = glGetUniformLocation(Handle, name.ToMultiByteString());
			if (loc != -1)
				glProgramUniform1i(Handle, loc, value);
		}
		void SetUniform(String name, float value)
		{
			int loc = glGetUniformLocation(Handle, name.ToMultiByteString());
			if (loc != -1)
				glProgramUniform1f(Handle, loc, value);
		}
		void SetUniform(String name, Vec2 value)
		{
			int loc = glGetUniformLocation(Handle, name.ToMultiByteString());
			if (loc != -1)
				glProgramUniform2fv(Handle, loc, 1, (float*)&value);
		}
		void SetUniform(String name, Vec3 value)
		{
			int loc = glGetUniformLocation(Handle, name.ToMultiByteString());
			if (loc != -1)
				glProgramUniform3fv(Handle, loc, 1, (float*)&value);
		}
		void SetUniform(String name, Vec4 value)
		{
			int loc = glGetUniformLocation(Handle, name.ToMultiByteString());
			if (loc != -1)
				glProgramUniform4fv(Handle, loc, 1, (float*)&value);
		}
		void SetUniform(String name, Matrix3 value)
		{
			int loc = glGetUniformLocation(Handle, name.ToMultiByteString());
			if (loc != -1)
				glProgramUniformMatrix3fv(Handle, loc, 1, false, (float*)&value);
		}
		void SetUniform(String name, Matrix4 value)
		{
			int loc = glGetUniformLocation(Handle, name.ToMultiByteString());
			if (loc != -1)
				glProgramUniformMatrix4fv(Handle, loc, 1, false, (float*)&value);
		}
		void SetUniform(String name, uint64_t value)
		{
			int loc = glGetUniformLocation(Handle, name.ToMultiByteString());
			if (loc != -1)
				glProgramUniformui64NV(Handle, loc, value);
		}
		void SetUniform(int loc, int value)
		{
			if (loc != -1)
				glProgramUniform1i(Handle, loc, value);
		}
		void SetUniform(int loc, uint64_t value)
		{
			if (loc != -1)
				glProgramUniformui64NV(Handle, loc, value);
		}
		void SetUniform(int loc, float value)
		{
			if (loc != -1)
				glProgramUniform1f(Handle, loc, value);
		}
		void SetUniform(int loc, Vec2 value)
		{
			if (loc != -1)
				glProgramUniform2fv(Handle, loc, 1, (float*)&value);
		}
		void SetUniform(int loc, Vec3 value)
		{
			if (loc != -1)
				glProgramUniform3fv(Handle, loc, 1, (float*)&value);
		}
		void SetUniform(int loc, Vec4 value)
		{
			if (loc != -1)
				glProgramUniform4fv(Handle, loc, 1, (float*)&value);
		}
		void SetUniform(int loc, Matrix3 value)
		{
			if (loc != -1)
				glProgramUniformMatrix3fv(Handle, loc, 1, false, (float*)&value);
		}
		void SetUniform(int loc, Matrix4 value)
		{
			if (loc != -1)
				glProgramUniformMatrix4fv(Handle, loc, 1, false, (float*)&value);
			//	glUniformMatrix4fv(loc, 1, false, (float*)&value);
		}
		int GetUniformLoc(String name)
		{
			return glGetUniformLocation(Handle, name.ToMultiByteString());
		}
		int GetUniformLoc(const char * name)
		{
			return glGetUniformLocation(Handle, name);
		}
		int GetUniformBlockIndex(String name)
		{
			return glGetUniformBlockIndex(Handle, name.ToMultiByteString());
		}
		void Use()
		{
			glUseProgram(Handle);
		}
		void Link()
		{
			int length, compileStatus;
			List<char> buffer;
			CoreLib::Diagnostics::DebugWriter dbgWriter;

			glLinkProgram(Handle);

			glGetProgramiv(Handle, GL_INFO_LOG_LENGTH, &length);
			buffer.SetSize(length);
			glGetProgramInfoLog(Handle, length, &length, buffer.Buffer());
			glGetProgramiv(Handle, GL_LINK_STATUS, &compileStatus);
			String logOutput(buffer.Buffer());
			if (length > 0)
				dbgWriter << logOutput;
			if (compileStatus != GL_TRUE)
			{
				throw HardwareRendererException(L"program linking\n" + logOutput);
			}

			glValidateProgram(Handle);
			glGetProgramiv(Handle, GL_INFO_LOG_LENGTH, &length);
			buffer.SetSize(length);
			glGetProgramInfoLog(Handle, length, &length, buffer.Buffer());
			logOutput = buffer.Buffer();
			if (length > 0)
				dbgWriter << logOutput;
			glGetProgramiv(Handle, GL_VALIDATE_STATUS, &compileStatus);
			if (compileStatus != GL_TRUE)
			{
				throw HardwareRendererException(L"program validation\n" + logOutput);
			}
		}
	};

	void __stdcall GL_DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam);

	class IndirectDrawCommands
	{
	public:
		int Count;
		int InstanceCount;
		int FirstIndex;
		int BaseInstance;
	};

	class StateObject : public GL_Object
	{};

	class CommandList : public GL_Object
	{
	public:
		void Execute()
		{
			glCallCommandListNV(Handle);
		}
	};

	class CommandBuffer
	{
	private:
		List<unsigned char> buffer;
		FrameBuffer frameBuffer;
		StateObject stateObj;
	public:
		CommandBuffer() = default;
		CommandBuffer(FrameBuffer pFrameBuffer, StateObject pStateObj)
			: frameBuffer(pFrameBuffer), stateObj(pStateObj)
		{
		}
		void * GetBufferPtr()
		{
			return buffer.Buffer();
		}
		int GetSize()
		{
			return buffer.Count();
		}
		FrameBuffer GetFrameBuffer()
		{
			return frameBuffer;
		}
		StateObject GetState()
		{
			return stateObj;
		}
		void TerminateSequence();
		void NoOp();
		void DrawElements(GLuint firstIndex, GLuint baseVertex, GLuint count);
		void DrawArrays(GLuint first, GLuint count);
		void ElementAddress(GLuint64 addr, GLuint typeSizeInByte);
		void AttributeAddress(GLuint index, GLuint64 addr);
		void UniformAddress(GLushort index, ShaderType stage, GLuint64 addr);
		void PolygonOffset(float scale, float bias);
		void FrontFace(int frontFace);
		void Viewport(int x, int y, int w, int h);
	};

	class CommandListBuilder : public GL_Object
	{
	private:
		List<List<CommandBuffer>> segments;
	public:
		void SetSegments(int count)
		{
			segments.SetSize(count);
		}
		CommandBuffer * NewCommandBuffer(int segmentId, const FrameBuffer &fb, StateObject state)
		{
			segments[segmentId].Add(CommandBuffer(fb, state));
			return &segments[segmentId].Last();
		}
		void AddCommandBuffer(int segmentId, const CommandBuffer & cmdBuffer)
		{
			segments[segmentId].Add(cmdBuffer);
		}
		void AddCommandBuffer(int segmentId, CommandBuffer && cmdBuffer)
		{
			segments[segmentId].Add(_Move(cmdBuffer));
		}
		void CompileInto(CommandList list)
		{
			glCommandListSegmentsNV(list.Handle, segments.Count());
			for (int i = 0; i < segments.Count(); i++)
			{
				List<void *> indirects;
				List<int> sizes;
				List<GLuint> states, fbos;
				for (auto & seg : segments[i])
				{
					indirects.Add(seg.GetBufferPtr());
					sizes.Add(seg.GetSize());
					states.Add(seg.GetState().Handle);
					fbos.Add(seg.GetFrameBuffer().Handle);
				}
				glListDrawCommandsStatesClientNV(list.Handle, i, (const void**)indirects.Buffer(), (const GLsizei*)sizes.Buffer(),
					(const GLuint*)states.Buffer(), (const GLuint*)fbos.Buffer(), segments[i].Count());
			}
			glCompileCommandListNV(list.Handle);
		}
		void CompileInto(List<CommandList> & lists) // one list for each CommandBuffer
		{
			for (auto & seg : segments)
			{
				List<CommandList> cmdLists;
				cmdLists.SetSize(seg.Count());
				glCreateCommandListsNV(seg.Count(), (GLuint*)cmdLists.Buffer());
				for (int i = 0; i < cmdLists.Count(); i++)
				{
					auto &cmdList = cmdLists[i];
					glCommandListSegmentsNV(cmdList.Handle, 1);
					void * indirect = seg[i].GetBufferPtr();
					GLsizei size = seg[i].GetSize();
					auto state = seg[i].GetState();
					auto fbo = seg[i].GetFrameBuffer();
					glListDrawCommandsStatesClientNV(cmdList.Handle, 0, (const void**)&indirect, &size,
						(const GLuint*)&state.Handle, (const GLuint*)&fbo.Handle, 1);
					glCompileCommandListNV(cmdList.Handle);
				}
				lists.AddRange(cmdLists);
			}
		}
	};

	class HardwareRenderer
	{
	private:
		HWND hwnd;
		HDC hdc;
		HGLRC hrc;
	public:
		HardwareRenderer()
		{
			hwnd = 0;
			hdc = 0;
			hrc = 0;
		}
		void Initialize(GUIHandle handle)
		{
			this->hwnd = (HWND)handle; 
			hdc = GetDC(hwnd); // Get the device context for our window

			PIXELFORMATDESCRIPTOR pfd; // Create a new PIXELFORMATDESCRIPTOR (PFD)
			memset(&pfd, 0, sizeof(PIXELFORMATDESCRIPTOR)); // Clear our  PFD
			pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR); // Set the size of the PFD to the size of the class
			pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW; // Enable double buffering, opengl support and drawing to a window
			pfd.iPixelType = PFD_TYPE_RGBA; // Set our application to use RGBA pixels
			pfd.cColorBits = 32; // Give us 32 bits of color information (the higher, the more colors)
			pfd.cDepthBits = 24; // Give us 32 bits of depth information (the higher, the more depth levels)
			pfd.cStencilBits = 8;
			pfd.iLayerType = PFD_MAIN_PLANE; // Set the layer of the PFD

			int nPixelFormat = ChoosePixelFormat(hdc, &pfd); // Check if our PFD is valid and get a pixel format back
			if (nPixelFormat == 0) // If it fails
				throw HardwareRendererException(L"Requried pixel format is not supported.");

			auto bResult = SetPixelFormat(hdc, nPixelFormat, &pfd); // Try and set the pixel format based on our PFD
			if (!bResult) // If it fails
				throw HardwareRendererException(L"Requried pixel format is not supported.");

			HGLRC tempOpenGLContext = wglCreateContext(hdc); // Create an OpenGL 2.1 context for our device context
			wglMakeCurrent(hdc, tempOpenGLContext); // Make the OpenGL 2.1 context current and active

			GLenum error = glewInit(); // Enable GLEW
			if (error != GLEW_OK) // If GLEW fails
				throw HardwareRendererException(L"Failed to load OpenGL.");

			int contextFlags = WGL_CONTEXT_CORE_PROFILE_BIT_ARB;
#ifdef _DEBUG
			contextFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
#endif

			int attributes[] = 
			{
				WGL_CONTEXT_MAJOR_VERSION_ARB, TargetOpenGLVersion_Major, 
				WGL_CONTEXT_MINOR_VERSION_ARB, TargetOpenGLVersion_Minor, 
				WGL_CONTEXT_FLAGS_ARB, contextFlags, // Set our OpenGL context to be forward compatible
				0
			};

			if (wglewIsSupported("WGL_ARB_create_context") == 1) 
			{ 
				// If the OpenGL 3.x context creation extension is available
				hrc = wglCreateContextAttribsARB(hdc, NULL, attributes); // Create and OpenGL 3.x context based on the given attributes
				wglMakeCurrent(NULL, NULL); // Remove the temporary context from being active
				wglDeleteContext(tempOpenGLContext); // Delete the temporary OpenGL 2.1 context
				wglMakeCurrent(hdc, hrc); // Make our OpenGL 3.0 context current
			}
			else 
			{
				hrc = tempOpenGLContext; // If we didn't have support for OpenGL 3.x and up, use the OpenGL 2.1 context
			}

			int glVersion[2] = { -1, -1 }; // Set some default values for the version
			glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]); // Get back the OpenGL MAJOR version we are using
			glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]); // Get back the OpenGL MAJOR version we are using

			CoreLib::Diagnostics::Debug::WriteLine(L"Using OpenGL: " + String(glVersion[0]) + L"." + String(glVersion[1])); // Output which version of OpenGL we are using
			if (glVersion[0] < TargetOpenGLVersion_Major || (glVersion[0] == TargetOpenGLVersion_Major && glVersion[1] < TargetOpenGLVersion_Minor))
			{
				// supported OpenGL version is too low
				throw HardwareRendererException(L"OpenGL" + String(TargetOpenGLVersion_Major) + L"." + String(TargetOpenGLVersion_Minor) + L" is not supported.");
			}
			init_NV_command_list((GetProcFunc)wglGetProcAddress);
			glDebugMessageCallback(GL_DebugCallback, this);
//#ifdef _DEBUG
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			contextFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
//#endif
			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		}

		void Destroy()
		{
			if (hrc)
			{
				glFinish();
				wglMakeCurrent(hdc, 0); // Remove the rendering context from our device context
				wglDeleteContext(hrc); // Delete our rendering context
			}
			if (hdc)
				ReleaseDC(hwnd, hdc); // Release the device context from our window
		}

		void SetClearColor(const Vec4 & color)
		{
			glClearColor(color.x, color.y, color.z, color.w);
		}
		void Clear(bool depth, bool color, bool stencil)
		{
			GLbitfield bitmask = 0;
			if (depth) bitmask |= GL_DEPTH_BUFFER_BIT;
			if (color) bitmask |= GL_COLOR_BUFFER_BIT;
			if (stencil) bitmask |= GL_STENCIL_BUFFER_BIT;
			glClear(bitmask);
		}
		void BindBuffer(BufferType bufferType, int index, BufferObject buffer)
		{
			glBindBufferBase((int)bufferType, index, buffer.Handle);
		}
		inline void BindBufferAddr(BufferType bufferType, int index, uint64_t addr, int length)
		{
			switch (bufferType)
			{
			case BufferType::ArrayBuffer:
				glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, index, addr, length);
				break;
			case BufferType::UniformBuffer:
				glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, index, addr, length);
				break;
			case BufferType::ElementBuffer:
				glBufferAddressRangeNV(GL_ELEMENT_ARRAY_ADDRESS_NV, index, addr, length);
				break;
			default:
				throw NotImplementedException();
			}
		}
		void ExecuteComputeShader(int numGroupsX, int numGroupsY, int numGroupsZ)
		{
			glDispatchCompute((GLuint)numGroupsX, (GLuint)numGroupsY, (GLuint)numGroupsZ);
		}
		void SetViewport(int x, int y, int width, int height)
		{
			glViewport(x, y, width, height);
		}
		void SetCullMode(CullMode cullMode)
		{
			switch (cullMode)
			{
			case CullMode::Disabled:
				glDisable(GL_CULL_FACE);
				break;
			case CullMode::CullBackFace:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				break;
			case CullMode::CullFrontFace:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				break;
			}
		}
		void SetDepthMask(bool write)
		{
			glDepthMask(write?GL_TRUE:GL_FALSE);
		}
		void SetColorMask(bool r, bool g, bool b, bool a)
		{
			glColorMask((GLboolean)r, (GLboolean)g, (GLboolean)b, (GLboolean)a);
		}

		void SetBlendMode(BlendMode blendMode)
		{
			switch (blendMode)
			{
			case BlendMode::Replace:
				glDisable(GL_BLEND);
				break;
			case BlendMode::AlphaBlend:
				glEnable(GL_BLEND);
				glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
				break;
			case BlendMode::Add:
				glEnable(GL_BLEND);
				glBlendFunc(GL_ONE, GL_ONE);
				break;
			default:
				throw HardwareRendererException(L"Unsupported blend mode.");
			}
		}
		void SetZTestMode(BlendOperator ztestMode)
		{
			switch (ztestMode)
			{
			case BlendOperator::Less:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				break;
			case BlendOperator::Equal:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_EQUAL);
				break;
			case BlendOperator::Disabled:
				glDisable(GL_DEPTH_TEST);
				break;
			case BlendOperator::LessEqual:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LEQUAL);
				break;
			case BlendOperator::Greater:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_GREATER);
				break;
			case BlendOperator::GreaterEqual:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_GEQUAL);
				break;
			case BlendOperator::NotEqual:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_NOTEQUAL);
				break;
			}
		}
		int TranslateStencilOp(StencilOp op)
		{
			switch (op)
			{
			case StencilOp::Keep:
				return GL_KEEP;
			case StencilOp::Replace:
				return GL_REPLACE;
			case StencilOp::Increment:
				return GL_INCR;
			case StencilOp::IncrementWrap:
				return GL_INCR_WRAP;
			case StencilOp::Decrement:
				return GL_DECR;
			case StencilOp::DecrementWrap:
				return GL_DECR_WRAP;
			case StencilOp::Invert:
				return GL_INVERT;
			}
			return GL_KEEP;
		}
		void SetStencilMode(StencilMode stencilMode)
		{
			switch (stencilMode.StencilFunc)
			{
			case CompareFunc::Disabled:
				glDisable(GL_STENCIL_TEST);
				break;
			case CompareFunc::Always:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_ALWAYS, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			case CompareFunc::Less:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_LESS, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			case CompareFunc::Equal:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_EQUAL, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			case CompareFunc::LessEqual:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_LEQUAL, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			case CompareFunc::Greater:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_GREATER, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			case CompareFunc::GreaterEqual:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_GEQUAL, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			case CompareFunc::NotEqual:
				glEnable(GL_STENCIL_TEST);
				glStencilFunc(GL_NOTEQUAL, stencilMode.StencilReference, stencilMode.StencilMask);
				break;
			}
			if (stencilMode.StencilFunc != CompareFunc::Disabled)
				glStencilMask(stencilMode.StencilMask);
			glStencilOp(TranslateStencilOp(stencilMode.Fail), TranslateStencilOp(stencilMode.DepthFail), TranslateStencilOp(stencilMode.DepthPass));
		}
		void SwapBuffers()
		{
			::SwapBuffers(hdc);
		}

		void Flush()
		{
			glFlush();
		}

		void Finish()
		{
			glFinish();
		}

		RenderBuffer CreateRenderBuffer(StorageFormat format, int width, int height, int samples)
		{
			auto rs = RenderBuffer();
			glCreateRenderbuffers(1, &rs.Handle);
			rs.storageFormat = format;
			rs.internalFormat = TranslateStorageFormat(format);
			if (samples <= 1)
				glNamedRenderbufferStorageEXT(rs.Handle, rs.internalFormat, width, height);
			else
				glNamedRenderbufferStorageMultisampleEXT(rs.Handle, samples, rs.internalFormat, width, height);
			return rs;
		}

		StateObject CaptureState(PrimitiveType ptype)
		{
			StateObject obj;
			glCreateStatesNV(1, &obj.Handle);
			glStateCaptureNV(obj.Handle, (int)ptype);
			return obj;
		}

		void DestroyState(StateObject obj)
		{
			glDeleteStatesNV(1, &obj.Handle);
		}

		CommandList CreateCommandList()
		{
			CommandList list;
			glCreateCommandListsNV(1, &list.Handle);
			return list;
		}

		void DestroyCommandList(CommandList & list)
		{
			glDeleteCommandListsNV(1, &list.Handle);
			list.Handle = 0;
		}

		FrameBuffer CreateFrameBuffer()
		{	
			GLuint handle = 0;
			glCreateFramebuffers(1, &handle);
			auto rs = FrameBuffer();
			rs.Handle = handle;
			return rs;
		}

		TransformFeedback CreateTransformFeedback()
		{
			TransformFeedback rs;
			glCreateTransformFeedbacks(1, &rs.Handle);
			return rs;
		}

		Shader CreateShader(ShaderType type, const String & source)
		{
			GLuint handle = 0;
			switch (type)
			{
			case ShaderType::VertexShader:
				handle = glCreateShader(GL_VERTEX_SHADER);
				break;
			case ShaderType::FragmentShader:
				handle = glCreateShader(GL_FRAGMENT_SHADER);
				break;
			case ShaderType::ComputeShader:
				handle = glCreateShader(GL_COMPUTE_SHADER);
				break;
			default:
				throw HardwareRendererException(L"OpenGL hardware renderer does not support specified shader type.");
			}
			GLchar * src = source.ToMultiByteString();
			GLint length = source.Length();
			glShaderSource(handle, 1, &src, &length);
			glCompileShader(handle);
			glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &length);
			List<char> buffer;
			buffer.SetSize(length);
			glGetShaderInfoLog(handle, length, &length, buffer.Buffer());
			int compileStatus;
			glGetShaderiv(handle, GL_COMPILE_STATUS, &compileStatus);
			if (compileStatus != GL_TRUE)
			{
				CoreLib::Diagnostics::Debug::WriteLine(String(buffer.Buffer()));
				throw HardwareRendererException(L"Shader compilation failed\n" + String(buffer.Buffer()));
			}
			auto rs = Shader();
			rs.Handle = handle;
			return rs;
		}

		Program CreateTransformFeedbackProgram(const Shader &vertexShader, const List<String> & varyings, FeedbackStorageMode format)
		{
			auto handle = glCreateProgram();
			if (vertexShader.Handle)
				glAttachShader(handle, vertexShader.Handle);

			List<char*> varyingPtrs;
			varyingPtrs.Reserve(varyings.Count());
			for (auto & v : varyings)
				varyingPtrs.Add(v.ToMultiByteString());
			glTransformFeedbackVaryings(handle, varyingPtrs.Count(), varyingPtrs.Buffer(), format == FeedbackStorageMode::Interleaved ? GL_INTERLEAVED_ATTRIBS : GL_SEPARATE_ATTRIBS);
			auto rs = Program();
			rs.Handle = handle;
			rs.Link();
			return rs;
		}

		Program CreateProgram(const Shader &computeProgram)
		{
			auto handle = glCreateProgram();
			if (computeProgram.Handle)
				glAttachShader(handle, computeProgram.Handle);
			auto rs = Program();
			rs.Handle = handle;
			rs.Link();
			return rs;
		}

		Program CreateProgram(const Shader &vertexShader, const Shader &fragmentShader)
		{
			auto handle = glCreateProgram();
			if (vertexShader.Handle)
				glAttachShader(handle, vertexShader.Handle);
			if (fragmentShader.Handle)
				glAttachShader(handle, fragmentShader.Handle);
			auto rs = Program();
			rs.Handle = handle;
			rs.Link();
			return rs;
		}

		Program CreateProgram(const Shader &vertexShader, const Shader &fragmentShader, EnumerableDictionary<String, int> & vertexAttributeBindings)
		{
			auto handle = glCreateProgram();
			if (vertexShader.Handle)
				glAttachShader(handle, vertexShader.Handle);
			if (fragmentShader.Handle)
				glAttachShader(handle, fragmentShader.Handle);
			auto rs = Program();
			rs.Handle = handle;
			for (auto & binding : vertexAttributeBindings)
			{
				glBindAttribLocation(handle, binding.Value, binding.Key.ToMultiByteString());
			}
			rs.Link();
			return rs;
		}

		TextureSampler CreateTextureSampler()
		{
			auto rs = TextureSampler();
			glGenSamplers(1, &rs.Handle);
			return rs;
		}

		Texture2D CreateTexture2D()
		{
			GLuint handle = 0;
			glCreateTextures(GL_TEXTURE_2D, 1, &handle);
			glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTextureParameterf(handle, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
			auto rs = Texture2D();
			rs.Handle = handle;
			rs.BindTarget = GL_TEXTURE_2D;
			return rs;
		}

		TextureCube CreateTextureCube()
		{
			GLuint handle = 0;
			glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &handle);
			glTextureParameteri(handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTextureParameteri(handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
			auto rs = TextureCube();
			rs.Handle = handle;
			rs.BindTarget = GL_TEXTURE_CUBE_MAP;
			return rs;
		}

		VertexArray CreateVertexArray()
		{
			auto rs = VertexArray();
			glCreateVertexArrays(1, &rs.Handle);
			return rs;
		}

		void BindVertexArray(VertexArray vertArray)
		{
			glBindVertexArray(vertArray.Handle);
		}

		void DrawArray(PrimitiveType primType, int first, int count)
		{
			glDrawArrays((GLenum)primType, first, count);
		}

		void DrawArrayInstancesIndirect(PrimitiveType primType, IndirectDrawCommands * indirect, int commandCount, int commandBufferStride)
		{
			glMultiDrawArraysIndirect((GLenum)primType, indirect, commandCount, commandBufferStride);
		}

		void DrawArrayInstances(PrimitiveType primType, int vertCount, int numInstances)
		{
			glDrawArraysInstanced((GLenum)primType, 0,  vertCount, numInstances);
		}

		void DrawInstances(DataType type, PrimitiveType primType, int numInstances, int indexCount)
		{
			GLenum gl_type;
			switch (GetSingularDataType(type))
			{
			case DataType::Int:
				gl_type = GL_UNSIGNED_INT;
				break;
			case DataType::Short:
			case DataType::UShort:
				gl_type = GL_UNSIGNED_SHORT;
				break;
			case DataType::Byte:
				gl_type = GL_UNSIGNED_BYTE;
				break;
			default:
				gl_type = GL_UNSIGNED_INT;
				break;
			}
			glDrawElementsInstanced((GLenum)primType, indexCount, gl_type, 0, numInstances);
		}

		void DestroyBuffer(BufferObject & obj)
		{
			glDeleteBuffers(1, &obj.Handle);
			obj.Handle = 0;
		}

		void DestroyFrameBuffer(FrameBuffer & obj)
		{
			glDeleteFramebuffers(1, &obj.Handle);
			obj.Handle = 0;
		}

		void DestroyRenderBuffer(RenderBuffer & obj)
		{
			glDeleteRenderbuffers(1, &obj.Handle);
			obj.Handle = 0;

		}

		void DestroyShader(Shader & obj)
		{
			glDeleteShader(obj.Handle);
			obj.Handle = 0;

		}

		void DestroyProgram(Program & obj)
		{
			glDeleteProgram(obj.Handle); 
			obj.Handle = 0;
		}

		void DestroyTransformFeedback(TransformFeedback & obj)
		{
			glDeleteTransformFeedbacks(1, &obj.Handle);
			obj.Handle = 0;
		}

		void DestroyTexture(Texture & obj)
		{
			glDeleteTextures(1, &obj.Handle);
			obj.Handle = 0;

		}

		void DestroyVertexArray(VertexArray & obj)
		{
			glDeleteVertexArrays(1, &obj.Handle);
			obj.Handle = 0;

		}

		void DestroyTextureSampler(TextureSampler & obj)
		{
			glDeleteSamplers(1, &obj.Handle);
			obj.Handle = 0;

		}

		BufferObject CreateBuffer(BufferUsage usage)
		{
			auto rs = BufferObject();
			rs.BindTarget = TranslateBufferUsage(usage);
			glCreateBuffers(1, &rs.Handle);
			return rs;
		}

		void SetReadFrameBuffer(const FrameBuffer &buffer)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, buffer.Handle);
		}

		void SetWriteFrameBuffer(const FrameBuffer &buffer)
		{
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, buffer.Handle);
		}

		void CopyFrameBuffer(int srcX0, int srcY0, int srcX1, int srcY1, int dstX0, int dstY0, int dstX1, int dstY1, bool color, bool depth, bool stencil)
		{
			GLbitfield bitmask = 0;
			if (depth) bitmask |= GL_DEPTH_BUFFER_BIT;
			if (color) bitmask |= GL_COLOR_BUFFER_BIT;
			if (stencil) bitmask |= GL_STENCIL_BUFFER_BIT;
			glBlitFramebuffer(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, bitmask, GL_LINEAR);
		}
		void UseTexture(int channel, const Texture &tex, TextureSampler sampler)
		{
			glActiveTexture(GL_TEXTURE0 + channel);
			glBindTexture(tex.BindTarget, tex.Handle);
			glBindSampler(channel, sampler.Handle);
		}
		void UseTextures(ArrayView<Texture> textures, ArrayView<TextureSampler> samplers)
		{
			for (int i = 0; i < textures.Count(); i++)
			{
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(textures[i].BindTarget, textures[i].Handle);
				glBindSampler(i, samplers[i].Handle);
			}
			/*for (auto tex : textures)
			{
				glMakeTextureHandleResidentARB(((GL_Texture*)tex)->TextureHandle);
			}*/
		}
		void FinishUsingTextures(ArrayView<Texture> textures)
		{
			for (int i = 0; i < textures.Count(); i++)
			{
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(textures[i].BindTarget, 0);
				glBindSampler(i, 0);
			}
			/*for (auto tex : textures)
			{
				glMakeTextureHandleNonResidentARB(((GL_Texture*)tex)->TextureHandle);
			}*/
		}
		void BindShaderBuffers(ArrayView<BufferObject> buffers)
		{
			for (int i = 0; i < buffers.Count(); i++)
			{
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, buffers[i].Handle);
			}
		}
	};


	inline HardwareRenderer * CreateOpenGLHardwareRenderer()
	{
		return new HardwareRenderer();
	}
}

#endif