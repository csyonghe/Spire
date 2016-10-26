#ifndef GLEW_STATIC
#define GLEW_STATIC
#endif

#include "../GameEngineCore/HardwareRenderer.h"
#include "CoreLib/PerformanceCounter.h"

#include "../WinForm/Debug.h"
#include <glew/glew.h>
#include <glew/wglew.h>
#include "../VectorMath.h"
#include <assert.h>
#pragma comment(lib, "opengl32.lib")

using namespace GameEngine;

namespace GLL
{
	const int TargetOpenGLVersion_Major = 4;
	const int TargetOpenGLVersion_Minor = 3;
	using namespace VectorMath;
	class GUIWindow
	{};

	typedef GUIWindow* GUIHandle;

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

	DataType GetSingularDataType(DataType type)
	{
		if (type == DataType::UInt4_10_10_10_2)
			return DataType::Int;
		return (DataType)(((int)type) & 0xF0);
	}

	int GetDataTypeComponenets(DataType type)
	{
		return (((int)type) & 0xF) + 1;
	}

	GLuint TranslateBufferUsage(BufferUsage usage)
	{
		switch (usage)
		{
		case BufferUsage::ArrayBuffer: return GL_ARRAY_BUFFER;
		case BufferUsage::IndexBuffer: return GL_ELEMENT_ARRAY_BUFFER;
		case BufferUsage::StorageBuffer: return GL_SHADER_STORAGE_BUFFER;
		case BufferUsage::UniformBuffer: return GL_UNIFORM_BUFFER;
		default: throw HardwareRendererException(L"Unsupported buffer usage.");
		}
	}

	GLuint TranslateStorageFormat(StorageFormat format)
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
		case StorageFormat::RGBA_8:
			internalFormat = GL_RGBA;
			break;
		case StorageFormat::RGBA_Compressed:
#ifdef _DEBUG
			internalFormat = GL_COMPRESSED_RGBA;
#else
			internalFormat = GL_COMPRESSED_RGBA_BPTC_UNORM;
#endif
			break;
		case StorageFormat::BC1:
			internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			break;
		case StorageFormat::BC5:
			internalFormat = GL_COMPRESSED_RG_RGTC2;
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

	GLuint TranslateDataTypeToFormat(DataType type)
	{
		switch (GetDataTypeComponenets(type))
		{
		case 1: return GL_RED;
		case 2: return GL_RG;
		case 3: return GL_RGB;
		case 4: return GL_RGBA;
		default: throw HardwareRendererException(L"Unsupported data type.");
		}
	}

	GLuint TranslateDataTypeToInputType(DataType type)
	{
		switch (type)
		{
		case DataType::Int:
		case DataType::Int2:
		case DataType::Int3:
		case DataType::Int4:
			return GL_INT;
		case DataType::UInt:
			return GL_UNSIGNED_INT;
		case DataType::Byte:
		case DataType::Byte2:
		case DataType::Byte3:
		case DataType::Byte4:
			return GL_UNSIGNED_BYTE;
		case DataType::Char:
		case DataType::Char2:
		case DataType::Char3:
		case DataType::Char4:
			return GL_BYTE;
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

	GLuint TranslateBufferType(BufferType type)
	{
		switch (type)
		{
		case BufferType::UniformBuffer: return GL_UNIFORM_BUFFER;
		case BufferType::ArrayBuffer: return GL_ARRAY_BUFFER;
		case BufferType::StorageBuffer: return GL_SHADER_STORAGE_BUFFER;
		case BufferType::ElementBuffer: return GL_ELEMENT_ARRAY_BUFFER;
		default: throw NotImplementedException();
		};
	}

	GLenum TranslateBufferAccess(BufferAccess access)
	{
		switch (access)
		{
		case BufferAccess::Read: return GL_MAP_READ_BIT;
		case BufferAccess::Write: return GL_MAP_WRITE_BIT;
		case BufferAccess::ReadWrite: return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT;
		case BufferAccess::ReadWritePersistent: return GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT;
		default: throw NotImplementedException();
		}
	}

	GLbitfield TranslateBufferStorageFlags(BufferStorageFlag flag)
	{
		GLbitfield result = 0;
		if ((int)flag & (int)BufferStorageFlag::DynamicStorage) result |= GL_DYNAMIC_STORAGE_BIT;
		if ((int)flag & (int)BufferStorageFlag::MapRead) result |= GL_MAP_READ_BIT;
		if ((int)flag & (int)BufferStorageFlag::MapWrite) result |= GL_MAP_WRITE_BIT;
		if ((int)flag & (int)BufferStorageFlag::MapPersistent) result |= GL_MAP_PERSISTENT_BIT;
		if ((int)flag & (int)BufferStorageFlag::MapCoherent) result |= GL_MAP_COHERENT_BIT;
		if ((int)flag & (int)BufferStorageFlag::ClientStorage) result |= GL_CLIENT_STORAGE_BIT;
		return result;
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

	class TextureSampler : public GL_Object, public GameEngine::TextureSampler
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
		CompareFunc GetCompareFunc()
		{
			GLint compareMode;
			glGetSamplerParameterIiv(Handle, GL_TEXTURE_COMPARE_MODE, &compareMode);
			if (compareMode == GL_NONE) return CompareFunc::Disabled;

			GLint compareFunc;
			glGetSamplerParameterIiv(Handle, GL_TEXTURE_COMPARE_FUNC, &compareFunc);
			switch (compareFunc)
			{
			case GL_EQUAL: return CompareFunc::Equal;
			case GL_LESS: return CompareFunc::Less;
			case GL_GREATER: return CompareFunc::Greater;
			case GL_LEQUAL: return CompareFunc::LessEqual;
			case GL_GEQUAL: return CompareFunc::GreaterEqual;
			case GL_NOTEQUAL: return CompareFunc::NotEqual;
			case GL_ALWAYS: return CompareFunc::Always;
			case GL_NEVER: return CompareFunc::Never;
			default: throw HardwareRendererException(L"Not implemented");
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

	class Texture2D : public Texture, public GameEngine::Texture2D
	{
	public:
		Texture2D()
		{
			BindTarget = GL_TEXTURE_2D;
		}
		~Texture2D()
		{
			if (Handle)
			{
				glDeleteTextures(1, &Handle);
				Handle = 0;
			}
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
		void Resize(int width, int height, int samples, int /*mipLevels*/, bool /*preserveData*/)
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
		void SetData(StorageFormat pFormat, int level, int width, int height, int samples, DataType inputType, void * data, bool /*mipmapped*/)
		{
			this->storageFormat = pFormat;
			this->internalFormat = TranslateStorageFormat(pFormat);
			this->format = TranslateDataTypeToFormat(inputType);
			this->type = TranslateDataTypeToInputType(inputType);
			if (this->internalFormat == GL_DEPTH_COMPONENT16 || this->internalFormat == GL_DEPTH_COMPONENT24 || this->internalFormat == GL_DEPTH_COMPONENT32)
				this->format = GL_DEPTH_COMPONENT;
			else if (this->internalFormat == GL_DEPTH24_STENCIL8)
				this->format = GL_DEPTH_STENCIL;
			if (pFormat == StorageFormat::BC1 || pFormat == StorageFormat::BC5)
			{
				int blocks = (int)(ceil(width / 4.0f) * ceil(height / 4.0f));
				int bufferSize = pFormat == StorageFormat::BC5 ? blocks * 16 : blocks * 8;
				glBindTexture(GL_TEXTURE_2D, Handle);
				glCompressedTexImage2D(GL_TEXTURE_2D, level, internalFormat, width, height, 0, bufferSize, data);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			else
			{
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
		}
		void SetData(StorageFormat pFormat, int width, int height, int samples, DataType inputType, void * data, bool mipmapped = true)
		{
			SetData(pFormat, 0, width, height, samples, inputType, data, mipmapped);
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
		void GetData(int mipLevel, void * data, int /*bufSize*/)
		{
			glBindTexture(GL_TEXTURE_2D, Handle);
			glGetTexImage(GL_TEXTURE_2D, mipLevel, format, GL_UNSIGNED_BYTE, data);
			glBindTexture(GL_TEXTURE_2D, 0);
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
			if (glNamedFramebufferDrawBuffers)
				glNamedFramebufferDrawBuffers(Handle, buffers.Count(), buffers.Buffer());
			else
			{
				glBindFramebuffer(GL_FRAMEBUFFER, Handle);
				glDrawBuffers(buffers.Count(), buffers.Buffer());
			}
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

	class FrameBufferDescriptor : public GameEngine::FrameBuffer
	{
	public:
		CoreLib::List<GLL::Texture2D*> attachments;
	};

	class RenderTargetLayout : public GameEngine::RenderTargetLayout
	{
	public:
		CoreLib::List<TextureUsage> attachments;
	private:
		void Resize(int size)
		{
			if (attachments.Count() < size)
				attachments.SetSize(size);
		}

		void SetColorAttachment(int binding)
		{
			Resize(binding + 1);
			attachments[binding] = TextureUsage::ColorAttachment;

			//if (samples > 1)
			//{
			//	//TODO: need to resolve?
			//}
		}

		void SetDepthAttachment(int binding)
		{
			for (auto attachment : attachments)
			{
				if (attachment == TextureUsage::DepthAttachment)
					throw HardwareRendererException(L"Only 1 depth/stencil attachment allowed.");
			}

			Resize(binding + 1);
			attachments[binding] = TextureUsage::DepthAttachment;
		}
	public:
		RenderTargetLayout(CoreLib::ArrayView<TextureUsage> bindings)
		{
			int location = 0;
			for (auto binding : bindings)
			{
				switch (binding)
				{
				case TextureUsage::ColorAttachment:
				case TextureUsage::SampledColorAttachment:
					SetColorAttachment(location);
					break;
				case TextureUsage::DepthAttachment:
				case TextureUsage::SampledDepthAttachment:
					SetDepthAttachment(location);
					break;
				case TextureUsage::Unused:
					break;
				default:
					throw HardwareRendererException(L"Unsupported attachment usage");
				}
				location++;
			}
		}
		~RenderTargetLayout() {}

		virtual FrameBufferDescriptor* CreateFrameBuffer(const RenderAttachments& renderAttachments) override
		{
#if _DEBUG
			//// Ensure the RenderAttachments are compatible with this RenderTargetLayout
			//for (auto renderAttachment : renderAttachments)
			//{

			//}
			//
			//// Ensure the RenderAttachments are compatible with this RenderTargetLayout
			//for (auto colorReference : colorReferences)
			//{
			//	if (dynamic_cast<RenderAttachments*>(attachments)->usages[colorReference.attachment] != vk::ImageUsageFlagBits::eColorAttachment)
			//		throw HardwareRendererException(L"Incompatible RenderTargetLayout and RenderAttachments");
			//}
			//if (depthReference.layout != vk::ImageLayout::eUndefined)
			//{
			//	if (dynamic_cast<RenderAttachments*>(attachments)->usages[depthReference.attachment] != vk::ImageUsageFlagBits::eDepthStencilAttachment)
			//		throw HardwareRendererException(L"Incompatible RenderTargetLayout and RenderAttachments");
			//}
	#endif
			FrameBufferDescriptor* result = new FrameBufferDescriptor;

			for (auto renderAttachment : renderAttachments.attachments)
			{
				result->attachments.Add(dynamic_cast<Texture2D*>(renderAttachment));
			}

			return result;
		}
	};

	class BufferObject : public GL_Object, public GameEngine::Buffer
	{
	public:
		bool persistentMapping = false;
		GLuint BindTarget;
		static float CheckBufferData(int bufferHandle)
		{
			float buffer;
			glGetNamedBufferSubData(bufferHandle, 0, 4, &buffer);
			return buffer;
		}
		int GetSize()
		{
			int sizeInBytes = 0;
			glGetNamedBufferParameteriv(Handle, GL_BUFFER_SIZE, &sizeInBytes);
			return sizeInBytes;
		}
		void SetData(void * data, int sizeInBytes)
		{
			GLenum usage = GL_STATIC_DRAW;
			if (BindTarget == GL_UNIFORM_BUFFER)
				usage = GL_DYNAMIC_DRAW;
			else if (BindTarget == GL_ARRAY_BUFFER || BindTarget == GL_ELEMENT_ARRAY_BUFFER)
				usage = GL_STREAM_DRAW;
			if (sizeInBytes == 0)
				return;
			if (persistentMapping)
			{
				glDeleteBuffers(1, &Handle);
				glGenBuffers(1, &Handle);
				glBindBuffer(BindTarget, Handle);
				glNamedBufferStorage(Handle, sizeInBytes, data, GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT);
				glBindBuffer(BindTarget, 0);
			}
			else
			{
				glBindBuffer(BindTarget, Handle);
				glBufferData(BindTarget, sizeInBytes, data, usage);
				glBindBuffer(BindTarget, 0);
			}
		}
		void SetData(int offset, void * data, int size)
		{
			glNamedBufferSubData(Handle, (GLintptr)offset, (GLsizeiptr)size, data);
		}
		void BufferStorage(int size, void * data, BufferStorageFlag storageFlags)
		{
			glNamedBufferStorage(Handle, (GLsizeiptr)size, data, TranslateBufferStorageFlags(storageFlags));
		}
		void * Map(BufferAccess access, int offset, int len)
		{
			if (persistentMapping)
				return glMapNamedBufferRange(Handle, offset, len, GL_MAP_PERSISTENT_BIT | GL_MAP_READ_BIT | GL_MAP_WRITE_BIT | GL_MAP_COHERENT_BIT);
			else
				return glMapNamedBufferRange(Handle, offset, len, TranslateBufferAccess(access));
		}
		void * Map(int offset, int size)
		{
			return Map(BufferAccess::ReadWrite, offset, size);
		}
		void * Map()
		{
			return Map(0, GetSize());
		}
		void Unmap()
		{
			glUnmapNamedBuffer(Handle);
		}
		void Flush(int /*offset*/, int /*size*/)
		{
			//glFlushMappedNamedBufferRange(Handle, offset, size);
		}
		void Flush()
		{
			//glFlushMappedNamedBufferRange();
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
				if (attribs[i].Location != -1)
					id = attribs[i].Location;
				glEnableVertexAttribArray(id);
				if (attribs[i].Type == DataType::Int || attribs[i].Type == DataType::Int2 || attribs[i].Type == DataType::Int3 || attribs[i].Type == DataType::Int4
					|| attribs[i].Type == DataType::UInt)
					glVertexAttribIPointer(id, GetDataTypeComponenets(attribs[i].Type), TranslateDataTypeToInputType(attribs[i].Type), vertSize, (void*)(CoreLib::PtrInt)attribs[i].StartOffset);
				else
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

	class Shader : public GL_Object, public GameEngine::Shader
	{
	public:
		ShaderType stage;
		GLuint GetHandle()
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
		void SetUniform(String name, Vec2i value)
		{
			int loc = glGetUniformLocation(Handle, name.ToMultiByteString());
			if (loc != -1)
				glProgramUniform2iv(Handle, loc, 1, (int*)&value);
		}
		void SetUniform(String name, Vec3i value)
		{
			int loc = glGetUniformLocation(Handle, name.ToMultiByteString());
			if (loc != -1)
				glProgramUniform3iv(Handle, loc, 1, (int*)&value);
		}
		void SetUniform(String name, Vec4i value)
		{
			int loc = glGetUniformLocation(Handle, name.ToMultiByteString());
			if (loc != -1)
				glProgramUniform4iv(Handle, loc, 1, (int*)&value);
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
		void SetUniform(int loc, Vec2i value)
		{
			if (loc != -1)
				glProgramUniform2iv(Handle, loc, 1, (int*)&value);
		}
		void SetUniform(int loc, Vec3i value)
		{
			if (loc != -1)
				glProgramUniform3iv(Handle, loc, 1, (int*)&value);
		}
		void SetUniform(int loc, Vec4i value)
		{
			if (loc != -1)
				glProgramUniform4iv(Handle, loc, 1, (int*)&value);
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

	void __stdcall GL_DebugCallback(GLenum /*source*/, GLenum type, GLuint /*id*/, GLenum /*severity*/, GLsizei /*length*/, const GLchar* message, const void* /*userParam*/)
	{
		switch (type)
		{
		case GL_DEBUG_TYPE_ERROR:
		case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
			CoreLib::Diagnostics::Debug::Write(L"[GL Error] ");
			break;
		case GL_DEBUG_TYPE_PERFORMANCE:
			CoreLib::Diagnostics::Debug::Write(L"[GL Performance] ");
			break;
		case GL_DEBUG_TYPE_PORTABILITY:
			CoreLib::Diagnostics::Debug::Write(L"[GL Portability] ");
			break;
		default:
			return;
		}
		CoreLib::Diagnostics::Debug::WriteLine(String(message));
		if (type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
		{
			printf("%s\n", message);
			CoreLib::Diagnostics::Debug::WriteLine(L"--------");
		}
	}

	struct PipelineSettings
	{
		Program program;
		VertexFormat format;
		bool primitiveRestart;
		PrimitiveType primitiveType;
		int patchSize;
		CompareFunc DepthCompareFunc;
		CompareFunc StencilCompareFunc;
		StencilOp StencilFailOp, StencilDepthFailOp, StencilDepthPassOp;
		BlendMode BlendMode;
		unsigned int StencilMask, StencilReference;
		CullMode CullMode;
	};

	class PipelineInstance : public GameEngine::PipelineInstance
	{
	public:
		PipelineSettings settings;
		PipelineBinding binding;
		int stride;
		PipelineInstance(const PipelineSettings & pSettings, PipelineBinding pipelineBinding)
			: settings(pSettings), binding(pipelineBinding)
		{
			int maxOffset = -1;
			stride = 0;
			for (auto attribute : settings.format.Attributes)
			{
				if (attribute.StartOffset > maxOffset)
				{
					maxOffset = attribute.StartOffset;
					stride = maxOffset + DataTypeSize(attribute.Type);
				}
			}
		}
	};

	class Pipeline : public GameEngine::Pipeline
	{
	public:
		PipelineSettings settings;

		Pipeline(const PipelineSettings & pSettings)
			: settings(pSettings)
		{}

		virtual PipelineInstance* CreateInstance(const PipelineBinding& pipelineBinding) override
		{
			return new PipelineInstance(settings, pipelineBinding);
		}
	};

	class PipelineBuilder : public GameEngine::PipelineBuilder
	{
	public:
		Program shaderProgram;
		VertexFormat format;
		bool isTessellation = false;
		virtual void SetShaders(CoreLib::ArrayView<GameEngine::Shader*> shaders) override
		{
#if _DEBUG
			bool vertPresent = false;
			bool tessControlPresent = false;
			bool tessEvalPresent = false;
			//bool geometryPresent = false;
			bool fragPresent = false;
			bool computePresent = false;
#endif
			auto handle = glCreateProgram();
			for (auto shader : shaders)
			{
#if _DEBUG
				// Ensure only one of any shader stage is present in the requested shader stage
				switch (dynamic_cast<Shader*>(shader)->stage)
				{
				case ShaderType::VertexShader:
					assert(vertPresent == false);
					vertPresent = true;
					break;
				case ShaderType::FragmentShader:
					assert(fragPresent == false);
					fragPresent = true;
					break;
				case ShaderType::ComputeShader:
					assert(computePresent == false);
					computePresent = true;
					break;
				case ShaderType::HullShader:
					assert(tessControlPresent == false);
					tessControlPresent = true;
					break;
				case ShaderType::DomainShader:
					assert(tessEvalPresent == false);
					tessEvalPresent = true;
					break;
				default:
					throw HardwareRendererException(L"Unknown shader stage");
				}
#endif
				if (dynamic_cast<Shader*>(shader)->stage == ShaderType::HullShader)
					isTessellation = true;
				glAttachShader(handle, dynamic_cast<Shader*>(shader)->GetHandle());
			}

			shaderProgram.Handle = handle;
			shaderProgram.Link();
			//TODO: need to destroy this at some point
		}
		virtual void SetVertexLayout(VertexFormat vertexFormat) override
		{
			format = vertexFormat;
		}
		virtual void SetBindingLayout(int /*bindingId*/, BindingType /*bindType*/) override
		{
			// Do nothing
		}
		virtual Pipeline* ToPipeline(GameEngine::RenderTargetLayout* /*renderTargetLayout*/) override
		{
			PipelineSettings settings;
			settings.format = format;
			settings.primitiveRestart = PrimitiveRestartEnabled;
			if (isTessellation)
			{
				switch (PrimitiveTopology)
				{
				case PrimitiveType::Lines:
					settings.patchSize = 2;
					break;
				case PrimitiveType::Triangles:
					settings.patchSize = 3;
					break;
				case PrimitiveType::Quads:
					settings.patchSize = 4;
					break;
				case PrimitiveType::Patches:
					settings.patchSize = PatchSize;
					break;
				default:
					throw InvalidOperationException(L"invalid primitive type for tessellation.");
				}
				settings.primitiveType = PrimitiveType::Patches;
			}
			else
				settings.primitiveType = PrimitiveTopology;
			settings.program = shaderProgram;
			
			settings.DepthCompareFunc = DepthCompareFunc;
			settings.StencilCompareFunc = StencilCompareFunc;
			settings.StencilFailOp = StencilFailOp;
			settings.StencilDepthFailOp = StencilDepthFailOp;
			settings.StencilDepthPassOp = StencilDepthPassOp;
			settings.StencilMask = StencilMask;
			settings.StencilReference = StencilReference;
			settings.CullMode = CullMode;

			settings.BlendMode = BlendMode;


			return new Pipeline(settings);
		}
	};

	enum class Command
	{
		SetViewport,
		BindVertexBuffer,
		BindIndexBuffer,
		BindPipeline,
		Draw,
		DrawInstanced,
		DrawIndexed,
		DrawIndexedInstanced,
		Blit,
		ClearAttachments,
	};

	struct SetViewportData
	{
		int x, y, width, height;
	};
	struct PipelineData
	{
		PipelineInstance* instance;
		int vertSize;
		bool primitiveRestart = false;
		PrimitiveType primitiveType;
	};
	struct DrawData
	{
		int instances;
		int first;
		int count;
	};
	struct BlitData
	{
		GameEngine::Texture2D* dst;
		GameEngine::Texture2D* src;
	};
	struct AttachmentData
	{
		CoreLib::ArrayView<GameEngine::Texture2D*> attachments;
	};

	class CommandData
	{
	public:
		CommandData() {}
		~CommandData() {}

		Command command;
		union
		{
			SetViewportData viewport;
			BufferObject* vertexBuffer;
			BufferObject* indexBuffer;
			PipelineData pipeline;
			DrawData draw;
			BlitData blit;
			AttachmentData clear;
		};
	};

	class CommandBuffer : public GameEngine::CommandBuffer
	{
	public:
		CoreLib::List<CommandData> buffer;
	public:
		virtual void BeginRecording(GameEngine::RenderTargetLayout* /*renderTargetLayout*/, GameEngine::FrameBuffer* /*frameBuffer*/)
		{
			buffer.Clear();
		}
		virtual void BeginRecording(GameEngine::RenderTargetLayout* /*renderTargetLayout*/)
		{
			buffer.Clear();
		}
		virtual void EndRecording() { /*Do nothing*/ }
		virtual void SetViewport(int x, int y, int width, int height)
		{
			CommandData data;
			data.command = Command::SetViewport;
			data.viewport.x = x;
			data.viewport.y = y;
			data.viewport.width = width;
			data.viewport.height = height;
			buffer.Add(data);
		}
		virtual void BindVertexBuffer(GameEngine::Buffer* vertexBuffer)
		{
			CommandData data;
			data.command = Command::BindVertexBuffer;
			data.vertexBuffer = dynamic_cast<BufferObject*>(vertexBuffer);
			buffer.Add(data);
		}
		virtual void BindIndexBuffer(GameEngine::Buffer* indexBuffer)
		{
			CommandData data;
			data.command = Command::BindIndexBuffer;
			data.indexBuffer = dynamic_cast<BufferObject*>(indexBuffer);
			buffer.Add(data);
		}
		virtual void BindPipeline(GameEngine::PipelineInstance* pipelineInstance)
		{
			CommandData data;
			data.command = Command::BindPipeline;
			data.pipeline.instance = dynamic_cast<PipelineInstance*>(pipelineInstance);
			data.pipeline.vertSize = dynamic_cast<PipelineInstance*>(pipelineInstance)->stride;
			data.pipeline.primitiveRestart = ((GLL::PipelineInstance*)(pipelineInstance))->settings.primitiveRestart;
			data.pipeline.primitiveType = ((GLL::PipelineInstance*)(pipelineInstance))->settings.primitiveType;
			buffer.Add(data);
		}
		virtual void Draw(int firstVertex, int vertexCount)
		{
			CommandData data;
			data.command = Command::Draw;
			data.draw.first = firstVertex;
			data.draw.count = vertexCount;
			buffer.Add(data);
		}
		virtual void DrawInstanced(int numInstances, int firstVertex, int vertexCount)
		{
			CommandData data;
			data.command = Command::DrawInstanced;
			data.draw.instances = numInstances;
			data.draw.first = firstVertex;
			data.draw.count = vertexCount;
			buffer.Add(data);
		}
		virtual void DrawIndexed(int firstIndex, int indexCount)
		{
			CommandData data;
			data.command = Command::DrawIndexed;
			data.draw.first = firstIndex;
			data.draw.count = indexCount;
			buffer.Add(data);
		}
		virtual void DrawIndexedInstanced(int numInstances, int firstIndex, int indexCount)
		{
			CommandData data;
			data.command = Command::DrawIndexedInstanced;
			data.draw.instances = numInstances;
			data.draw.first = firstIndex;
			data.draw.count = indexCount;
			buffer.Add(data);
		}
		virtual void Blit(GameEngine::Texture2D* dstImage, GameEngine::Texture2D* srcImage) override
		{
			CommandData data;
			data.command = Command::Blit;
			data.blit.dst = dstImage;
			data.blit.src = srcImage;
			buffer.Add(data);
		}
		virtual void ClearAttachments(RenderAttachments renderAttachments) override
		{
			CommandData data;
			data.command = Command::ClearAttachments;
			data.clear.attachments = renderAttachments.attachments.GetArrayView();
			buffer.Add(data);
		}
	};

	class HardwareRenderer : public GameEngine::HardwareRenderer
	{
	private:
		HWND hwnd;
		HDC hdc;
		HGLRC hrc;
		int width;
		int height;
		VertexArray currentVAO;
		FrameBuffer srcFrameBuffer;
		FrameBuffer dstFrameBuffer;
	private:
		void FindExtensionSubstitutes()
		{
			if (!glNamedBufferData)
			{
				glNamedBufferData = glNamedBufferDataEXT;
				glNamedBufferStorage = glNamedBufferStorageEXT;
				glMapNamedBuffer = glMapNamedBufferEXT;
				glMapNamedBufferRange = glMapNamedBufferRangeEXT;
				glUnmapNamedBuffer = glUnmapNamedBufferEXT;
				glGetNamedBufferParameteriv = glGetNamedBufferParameterivEXT;
				glGetNamedBufferSubData = glGetNamedBufferSubDataEXT;
				glNamedFramebufferRenderbuffer = glNamedFramebufferRenderbufferEXT;
				glNamedFramebufferTexture = glNamedFramebufferTextureEXT;
				glCheckNamedFramebufferStatus = glCheckNamedFramebufferStatusEXT;
			}
		}
	public:
		HardwareRenderer()
		{
			hwnd = 0;
			hdc = 0;
			hrc = 0;
		}
		~HardwareRenderer()
		{
			DestroyVertexArray(currentVAO);
			DestroyFrameBuffer(srcFrameBuffer);
			DestroyFrameBuffer(dstFrameBuffer);
		}
		virtual void * GetWindowHandle() override
		{
			return this->hwnd;
		}
		void BindWindow(void* windowHandle, int pwidth, int pheight)
		{
			this->hwnd = (HWND)windowHandle;
			this->width = pwidth;
			this->height = pheight;
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

			int glVersion[2] = { 0, 0 }; // Set some default values for the version
			glGetIntegerv(GL_MAJOR_VERSION, &glVersion[0]); // Get back the OpenGL MAJOR version we are using
			glGetIntegerv(GL_MINOR_VERSION, &glVersion[1]); // Get back the OpenGL MAJOR version we are using

			CoreLib::Diagnostics::Debug::WriteLine(L"Using OpenGL: " + String(glVersion[0]) + L"." + String(glVersion[1])); // Output which version of OpenGL we are using
			if (glVersion[0] < TargetOpenGLVersion_Major || (glVersion[0] == TargetOpenGLVersion_Major && glVersion[1] < TargetOpenGLVersion_Minor))
			{
				// supported OpenGL version is too low
				throw HardwareRendererException(L"OpenGL" + String(TargetOpenGLVersion_Major) + L"." + String(TargetOpenGLVersion_Minor) + L" is not supported.");
			}
			if (glDebugMessageCallback)
				glDebugMessageCallback(GL_DebugCallback, this);
//#ifdef _DEBUG
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			contextFlags |= WGL_CONTEXT_DEBUG_BIT_ARB;
//#endif
			glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

			FindExtensionSubstitutes();
			// Set up source/dest framebuffer for presenting images
			srcFrameBuffer = CreateFrameBuffer();
			dstFrameBuffer = CreateFrameBuffer();
			currentVAO = CreateVertexArray();
			wglSwapIntervalEXT(0);
		}

		virtual void Resize(int pwidth, int pheight) override
		{
			width = pwidth;
			height = pheight;
		}

		virtual String GetSpireBackendName() override
		{
			return L"glsl";
		}

		virtual void ClearTexture(GameEngine::Texture2D* texture) override
		{
			SetWriteFrameBuffer(srcFrameBuffer);
			switch (reinterpret_cast<GLL::Texture2D*>(texture)->format)
			{
			case GL_DEPTH_COMPONENT:
				srcFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(texture));
				Clear(1, 0, 0);
				srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				break;
			case GL_DEPTH_STENCIL:
				srcFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(texture));
				Clear(1, 0, 1);
				srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				break;
			default:
				srcFrameBuffer.SetColorRenderTarget(0, *reinterpret_cast<GLL::Texture2D*>(texture));
				srcFrameBuffer.EnableRenderTargets(1);
				Clear(0, 1, 0);
				srcFrameBuffer.SetColorRenderTarget(0, Texture2D());
				break;
			}
		}

		virtual void ExecuteCommandBuffers(GameEngine::RenderTargetLayout* renderTargetLayout, GameEngine::FrameBuffer* frameBuffer, CoreLib::ArrayView<GameEngine::CommandBuffer*> commands) override
		{
			glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
			auto setupFrameBuffer = [&]()
			{
				int location = 0;
				int mask = 0;
				for (auto renderAttachment : dynamic_cast<GLL::FrameBufferDescriptor*>(frameBuffer)->attachments)
				{
					switch (dynamic_cast<GLL::RenderTargetLayout*>(renderTargetLayout)->attachments[location])
					{
					case TextureUsage::ColorAttachment:
					case TextureUsage::SampledColorAttachment:
						srcFrameBuffer.SetColorRenderTarget(location, *renderAttachment);
						mask |= (1 << location);
						break;
					case TextureUsage::DepthAttachment:
					case TextureUsage::SampledDepthAttachment:
						srcFrameBuffer.SetDepthStencilRenderTarget(*renderAttachment);
						break;
					case TextureUsage::Sampled:
						throw HardwareRendererException(L"Can't use sampled image as a RenderAttachment");
					}
					location++;
				}
				srcFrameBuffer.EnableRenderTargets(mask);
			};

			setupFrameBuffer();

			// Prepare framebuffer
			SetWriteFrameBuffer(srcFrameBuffer);
			SetViewport(0, 0, width, height);
			SetZTestMode(CompareFunc::Disabled);
			SetStencilMode(StencilMode());
			// Execute command buffer
			
			//
			BufferObject* currentVertexBuffer = nullptr;
			PrimitiveType primType = PrimitiveType::Triangles;
			PipelineBinding currentBinding;
			auto applyBinding = [&]()
			{
				for (auto binding : currentBinding.GetBindings())
				{
					switch (binding.type)
					{
					case BindingType::UniformBuffer:
						if (binding.buf.offset == 0 && binding.buf.range == 0)
							BindBuffer(BufferType::UniformBuffer, binding.location, *reinterpret_cast<BufferObject*>(binding.buf.buffer));
						else
							BindBuffer(BufferType::UniformBuffer, binding.location, *reinterpret_cast<BufferObject*>(binding.buf.buffer), binding.buf.offset, binding.buf.range);
						break;
					case BindingType::StorageBuffer:
						if (binding.buf.offset == 0 && binding.buf.range == 0)
							BindBuffer(BufferType::StorageBuffer, binding.location, *reinterpret_cast<BufferObject*>(binding.buf.buffer));
						else
							BindBuffer(BufferType::StorageBuffer, binding.location, *reinterpret_cast<BufferObject*>(binding.buf.buffer), binding.buf.offset, binding.buf.range);
						break;
					case BindingType::Texture:
						UseTexture(binding.location, *reinterpret_cast<Texture2D*>(binding.tex.texture), *reinterpret_cast<TextureSampler*>(binding.tex.sampler));
						break;
					default:
						break;
					}
				}
			};
			for (auto commandBuffer : commands)
			{
				for (auto command : reinterpret_cast<GLL::CommandBuffer*>(commandBuffer)->buffer)
				{
					switch (command.command)
					{
					case Command::SetViewport:
						SetViewport(command.viewport.x, command.viewport.y, command.viewport.width, command.viewport.height);
						break;
					case Command::BindVertexBuffer:
					{
						currentVertexBuffer = command.vertexBuffer;
						break;
					}
					case Command::BindIndexBuffer:
						currentVAO.SetIndex(*command.indexBuffer);
						break;
					case Command::BindPipeline:
					{
						auto & pipelineSettings = command.pipeline.instance->settings;
						pipelineSettings.program.Use();
						if (currentVertexBuffer == nullptr) throw HardwareRendererException(L"For OpenGL, must BindVertexBuffer before BindPipeline.");
						currentVAO.SetVertex(*currentVertexBuffer, pipelineSettings.format.Attributes.GetArrayView(), command.pipeline.vertSize, 0, 0);
						primType = command.pipeline.primitiveType;
						if (command.pipeline.primitiveRestart)
						{
							glEnable(GL_PRIMITIVE_RESTART);
							glPrimitiveRestartIndex(0xFFFFFFFF);
						}
						else
						{
							glDisable(GL_PRIMITIVE_RESTART);
						}
						if (pipelineSettings.primitiveType == PrimitiveType::Patches)
							glPatchParameteri(GL_PATCH_VERTICES, pipelineSettings.patchSize);
						SetZTestMode(pipelineSettings.DepthCompareFunc);
						SetBlendMode(pipelineSettings.BlendMode);
						StencilMode smode;
						smode.DepthFail = pipelineSettings.StencilDepthFailOp;
						smode.DepthPass = pipelineSettings.StencilDepthPassOp;
						smode.Fail = pipelineSettings.StencilFailOp;
						smode.StencilFunc = pipelineSettings.StencilCompareFunc;
						smode.StencilMask = pipelineSettings.StencilMask;
						smode.StencilReference = pipelineSettings.StencilReference;
						SetStencilMode(smode);
						SetCullMode(pipelineSettings.CullMode);
						currentBinding = command.pipeline.instance->binding;
						break;
					}
					//TODO: I have no idea if these draw functions are even remotely correct
					case Command::Draw:
						glBindVertexArray(currentVAO.Handle);
						applyBinding();
						glDrawArrays((GLenum)primType, command.draw.first, command.draw.count);
						break;
					case Command::DrawInstanced:
						glBindVertexArray(currentVAO.Handle);
						applyBinding();
						glDrawArraysInstanced((GLenum)primType, command.draw.first, command.draw.count, command.draw.instances);
						break;
					case Command::DrawIndexed:
						glBindVertexArray(currentVAO.Handle);
						applyBinding();
						glDrawElements((GLenum)primType, command.draw.count, GL_UNSIGNED_INT, (void*)(CoreLib::PtrInt)(command.draw.first * 4));
						break;
					case Command::DrawIndexedInstanced:
						glBindVertexArray(currentVAO.Handle);
						applyBinding();
						glDrawElementsInstanced((GLenum)primType, command.draw.count, GL_UNSIGNED_INT, (void*)(CoreLib::PtrInt)(command.draw.first * 4), command.draw.instances);
						break;
					case Command::Blit:
						Blit(command.blit.dst, command.blit.src);
						setupFrameBuffer();
						SetWriteFrameBuffer(srcFrameBuffer);
						break;
					case Command::ClearAttachments:
						Clear(1, 1, 1);
						break;
					}
				}
			}

			int location = 0;
			auto layout = (GLL::RenderTargetLayout*)renderTargetLayout;
			for (location = 0; location < layout->attachments.Count(); location++)
			{
				switch (layout->attachments[location])
				{
				case TextureUsage::ColorAttachment:
				case TextureUsage::SampledColorAttachment:
					srcFrameBuffer.SetColorRenderTarget(location, Texture2D());
					break;
				case TextureUsage::DepthAttachment:
				case TextureUsage::SampledDepthAttachment:
					srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
					break;
				default:
					break;
				}
			}
		}

		void Blit(GameEngine::Texture2D* dstImage, GameEngine::Texture2D* srcImage)
		{
			switch (reinterpret_cast<GLL::Texture2D*>(srcImage)->format)
			{
			case GL_DEPTH_COMPONENT:
				srcFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(srcImage));
				dstFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(dstImage));
				break;
			case GL_DEPTH_STENCIL:
				srcFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(srcImage));
				dstFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(dstImage));
				break;
			default:
				srcFrameBuffer.SetColorRenderTarget(0, *reinterpret_cast<GLL::Texture2D*>(srcImage));
				dstFrameBuffer.SetColorRenderTarget(0, *reinterpret_cast<GLL::Texture2D*>(dstImage));
				srcFrameBuffer.EnableRenderTargets(1);
				dstFrameBuffer.EnableRenderTargets(1);
				break;
			}

			// Blit from src to dst
			SetReadFrameBuffer(srcFrameBuffer);
			SetWriteFrameBuffer(dstFrameBuffer);
			CopyFrameBuffer(0, 0, width, height, 0, 0, width, height, true, false, false);

			switch (reinterpret_cast<GLL::Texture2D*>(srcImage)->format)
			{
			case GL_DEPTH_COMPONENT:
				srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				dstFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				break;
			case GL_DEPTH_STENCIL:
				srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				dstFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				break;
			default:
				srcFrameBuffer.SetColorRenderTarget(0, Texture2D());
				dstFrameBuffer.SetColorRenderTarget(0, Texture2D());
				srcFrameBuffer.EnableRenderTargets(0);
				dstFrameBuffer.EnableRenderTargets(0);
				break;
			}
		}

		void Present(GameEngine::Texture2D* srcImage)
		{
			switch (reinterpret_cast<GLL::Texture2D*>(srcImage)->format)
			{
			case GL_DEPTH_COMPONENT:
				srcFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(srcImage));
				break;
			case GL_DEPTH_STENCIL:
				srcFrameBuffer.SetDepthStencilRenderTarget(*reinterpret_cast<GLL::Texture2D*>(srcImage));
				break;
			default:
				srcFrameBuffer.SetColorRenderTarget(0, *reinterpret_cast<GLL::Texture2D*>(srcImage));
				srcFrameBuffer.EnableRenderTargets(1);
				break;
			}

			// Present rendered image to screen
			SetReadFrameBuffer(srcFrameBuffer);
			SetWriteFrameBuffer(GLL::FrameBuffer());
			CopyFrameBuffer(0, 0, width, height, 0, 0, width, height, true, false, false);
			SwapBuffers();

			switch (reinterpret_cast<GLL::Texture2D*>(srcImage)->format)
			{
			case GL_DEPTH_COMPONENT:
				srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				break;
			case GL_DEPTH_STENCIL:
				srcFrameBuffer.SetDepthStencilRenderTarget(Texture2D());
				break;
			default:
				srcFrameBuffer.SetColorRenderTarget(0, Texture2D());
				srcFrameBuffer.EnableRenderTargets(0);
				break;
			}
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
			glBindBufferBase(TranslateBufferType(bufferType), index, buffer.Handle);
		}
		void BindBuffer(BufferType bufferType, int index, BufferObject buffer, int start, int count)
		{
			if (count > 0) 
				glBindBufferRange(TranslateBufferType(bufferType), index, buffer.Handle, start, count);
		}
		void BindBufferAddr(BufferType bufferType, int index, uint64_t addr, int length)
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
		void SetViewport(int x, int y, int pwidth, int pheight)
		{
			glViewport(x, y, pwidth, pheight);
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
		void SetZTestMode(CompareFunc ztestMode)
		{
			switch (ztestMode)
			{
			case CompareFunc::Less:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				break;
			case CompareFunc::Equal:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_EQUAL);
				break;
			case CompareFunc::Disabled:
				glDisable(GL_DEPTH_TEST);
				break;
			case CompareFunc::LessEqual:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LEQUAL);
				break;
			case CompareFunc::Greater:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_GREATER);
				break;
			case CompareFunc::GreaterEqual:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_GEQUAL);
				break;
			case CompareFunc::NotEqual:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_NOTEQUAL);
				break;
			case CompareFunc::Always:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_ALWAYS);
				break;
			case CompareFunc::Never:
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_NEVER);
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

		RenderBuffer CreateRenderBuffer(StorageFormat format, int pwidth, int pheight, int samples)
		{
			auto rs = RenderBuffer();
			if (glCreateRenderbuffers)
				glCreateRenderbuffers(1, &rs.Handle);
			else
			{
				glGenRenderbuffers(1, &rs.Handle);
				glBindRenderbuffer(GL_RENDERBUFFER, rs.Handle);
			}
			rs.storageFormat = format;
			rs.internalFormat = TranslateStorageFormat(format);
			if (samples <= 1)
				glNamedRenderbufferStorageEXT(rs.Handle, rs.internalFormat, pwidth, pheight);
			else
				glNamedRenderbufferStorageMultisampleEXT(rs.Handle, samples, rs.internalFormat, pwidth, pheight);
			return rs;
		}

		FrameBuffer CreateFrameBuffer()
		{	
			GLuint handle = 0;
			if (glCreateFramebuffers)
				glCreateFramebuffers(1, &handle);
			else
			{
				glGenFramebuffers(1, &handle);
				glBindFramebuffer(GL_FRAMEBUFFER, handle);
			}
			auto rs = FrameBuffer();
			rs.Handle = handle;
			return rs;
		}

		TransformFeedback CreateTransformFeedback()
		{
			TransformFeedback rs;
			if (glCreateTransformFeedbacks)
				glCreateTransformFeedbacks(1, &rs.Handle);
			else
			{
				glGenTransformFeedbacks(1, &rs.Handle);
				glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, rs.Handle);
			}

			return rs;
		}

		Shader* CreateShader(ShaderType type, const char* data, int size)
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
			case ShaderType::HullShader:
				handle = glCreateShader(GL_TESS_CONTROL_SHADER);
				break;
			case ShaderType::DomainShader:
				handle = glCreateShader(GL_TESS_EVALUATION_SHADER);
				break;
			case ShaderType::ComputeShader:
				handle = glCreateShader(GL_COMPUTE_SHADER);
				break;
			default:
				throw HardwareRendererException(L"OpenGL hardware renderer does not support specified shader type.");
			}
			GLchar * src = (GLchar*)data;
			GLint length = size;
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
			auto rs = new Shader();
			rs->Handle = handle;
			rs->stage = type;
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

		TextureSampler* CreateTextureSampler()
		{
			auto rs = new TextureSampler();
			glGenSamplers(1, &rs->Handle);
			return rs;
		}

		Texture2D* CreateTexture2D(TextureUsage /*usage*/)
		{
			GLuint handle = 0;
			if (glCreateTextures)
				glCreateTextures(GL_TEXTURE_2D, 1, &handle);
			else
			{
				glGenTextures(1, &handle);
				glBindTexture(GL_TEXTURE_2D, handle);
			}
			glBindTexture(GL_TEXTURE_2D, handle);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
			glBindTexture(GL_TEXTURE_2D, 0);

			auto rs = new Texture2D();
			rs->Handle = handle;
			rs->BindTarget = GL_TEXTURE_2D;
			return rs;
		}

		TextureCube CreateTextureCube()
		{
			GLuint handle = 0;
			if (glCreateTextures)
				glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &handle);
			else
			{
				glGenTextures(1, &handle);
				glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
			}
			glBindTexture(GL_TEXTURE_CUBE_MAP, handle);

			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//glTexParameterf(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_ANISOTROPY_EXT, 8.0f);
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

			auto rs = TextureCube();
			rs.Handle = handle;
			rs.BindTarget = GL_TEXTURE_CUBE_MAP;
			return rs;
		}

		VertexArray CreateVertexArray()
		{
			auto rs = VertexArray();
			if (glCreateVertexArrays)
				glCreateVertexArrays(1, &rs.Handle);
			else
			{
				glGenVertexArrays(1, &rs.Handle);
				glBindVertexArray(rs.Handle);
			}
			return rs;
		}

		void BindVertexArray(VertexArray vertArray)
		{
			glBindVertexArray(vertArray.Handle);
		}

		void DrawElements(PrimitiveType primType, int count, DataType indexType)
		{
			glDrawElements((GLenum)primType, count, TranslateDataTypeToInputType(indexType), nullptr);
		}

		void DrawRangeElements(PrimitiveType primType, int lowIndex, int highIndex, int startLoc, int count, DataType indexType)
		{
			int indexSize = 4;
			if (indexType == DataType::UShort)
				indexSize = 2;
			else if (indexType == DataType::Byte)
				indexSize = 1;
			glDrawRangeElements((GLenum)primType, lowIndex, highIndex, count, TranslateDataTypeToInputType(indexType), (void*)(CoreLib::PtrInt)(startLoc * indexSize));
		}

		void DrawArray(PrimitiveType primType, int first, int count)
		{
			glDrawArrays((GLenum)primType, first, count);
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

		BufferObject* CreateBuffer(BufferUsage usage)
		{
			auto rs = new BufferObject();
			rs->BindTarget = TranslateBufferUsage(usage);
			if (glCreateBuffers)
				glCreateBuffers(1, &rs->Handle);
			else
			{
				glGenBuffers(1, &rs->Handle);
				glBindBuffer(rs->BindTarget, rs->Handle);
			}
			return rs;
		}

		BufferObject* CreateMappedBuffer(BufferUsage usage)
		{
			auto result = CreateBuffer(usage);
			result->persistentMapping = true;
			return result;
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
		void UseTextures(ArrayView<Texture> textures, TextureSampler sampler)
		{
			for (int i = 0; i < textures.Count(); i++)
			{
				glActiveTexture(GL_TEXTURE0 + i);
				glBindTexture(textures[i].BindTarget, textures[i].Handle);
				glBindSampler(i, sampler.Handle);
			}
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

		int UniformBufferAlignment() override
		{
			GLint uniformBufferAlignment;
			glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformBufferAlignment);
			return uniformBufferAlignment;
		}

		int StorageBufferAlignment() override
		{
			GLint rs;
			glGetIntegerv(GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT, &rs);
			return rs;
		}

		RenderTargetLayout* CreateRenderTargetLayout(CoreLib::ArrayView<TextureUsage> bindings)
		{
			return new RenderTargetLayout(bindings);
		}

		PipelineBuilder* CreatePipelineBuilder()
		{
			return new PipelineBuilder();
		}

		GameEngine::CommandBuffer* CreateCommandBuffer() 
		{
			return new CommandBuffer();
		}

		void Wait()
		{
			glFinish();
		}
	};
}

namespace GameEngine
{
	HardwareRenderer * CreateGLHardwareRenderer()
	{
		return new GLL::HardwareRenderer();
	}
}