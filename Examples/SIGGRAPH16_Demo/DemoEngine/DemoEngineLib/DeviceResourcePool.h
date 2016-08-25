#ifndef REALTIME_ENGINE_SHADING_H
#define REALTIME_ENGINE_SHADING_H

#include "CoreLib/Basic.h"
#include "CoreLib/Imaging.h"
#include "CoreLib/Graphics.h"
#include "Mesh.h"
#include "CoreLib/LibGL.h"

using namespace CoreLib::Basic;
using namespace VectorMath;

namespace DemoEngine
{
	using namespace GL;
	enum class RenderTargetStorage
	{
		Texture, RenderBuffer
	};

	class RenderTarget
	{
	public:
		virtual void BindToColor(FrameBuffer fb, int index) = 0;
		virtual void BindToDepthStencil(FrameBuffer fb) = 0;
	};

	
	class Texture2DRenderTarget : public RenderTarget
	{
	public:
		GL::Texture2D Texture;
		Texture2DRenderTarget()
		{}
		Texture2DRenderTarget(GL::Texture2D texture)
		{
			this->Texture = texture;
		}
		virtual void BindToColor(FrameBuffer fb, int index) override
		{
			fb.SetColorRenderTarget(index, Texture);
		}
		virtual void BindToDepthStencil(FrameBuffer fb) override
		{
			fb.SetDepthStencilRenderTarget(Texture);
		}
	};

	
	class TextureCubeRenderTarget : public RenderTarget
	{
	public:
		GL::TextureCube Texture;
		GL::TextureCubeFace Face;
		virtual void BindToColor(FrameBuffer fb, int index) override
		{
			fb.SetColorRenderTarget(index, Texture, Face);
		}
		virtual void BindToDepthStencil(FrameBuffer fb) override
		{
			fb.SetDepthStencilRenderTarget(Texture, Face);
		}
	};

	
	class RenderBufferRenderTarget : public RenderTarget
	{
	public:
		RenderBuffer Buffer;
		virtual void BindToColor(FrameBuffer fb, int index) override
		{
			fb.SetColorRenderTarget(index, Buffer);

		}
		virtual void BindToDepthStencil(FrameBuffer fb) override
		{
			fb.SetDepthStencilRenderTarget(Buffer);
		}
	};

	class VertexFormat
	{
	public:
		Array<VertexAttributeDesc, 32> Attributes;
		int Size;
	};

	class StaticMesh : public Object
	{
		friend class DeviceResourcePool;
	private:
		DeviceResourcePool * engine = nullptr;
		BufferObject verticesBuffer, indicesBuffer;
		VertexArray vertexArray;
		DataType indexType; 
		int vertexBufferSize;
		int vertexSize;
	public:
		StaticMesh() = default;
		StaticMesh(DeviceResourcePool * engine);
		void Free();
		int VertexCount;
		int BaseAttributeCount;
		PrimitiveType PrimitiveType;
		void SetVertexBuffer(void * data, int sizeInBytes)
		{
			verticesBuffer.SetData(data, sizeInBytes);
			vertexBufferSize = sizeInBytes;
		}
		int GetVertexCount() const
		{
			return vertexBufferSize / vertexSize;
		}
		void GetVertexData(List<int> & buffer)
		{
			int size = buffer.Count() * sizeof(int);
			if (!verticesBuffer.GetData(buffer.Buffer(), size))
			{
				buffer.SetSize(size / sizeof(int));
				verticesBuffer.GetData(buffer.Buffer(), size);
			}
		}
		void GetIndexData(List<int> & buffer)
		{
			int size = buffer.Count() * sizeof(int);
			if (!indicesBuffer.GetData(buffer.Buffer(), size))
			{
				buffer.SetSize(size / sizeof(int));
				indicesBuffer.GetData(buffer.Buffer(), size);
			}
		}
		void SetIndexBuffer(DataType type, void * data, int sizeInBytes)
		{
			indicesBuffer.SetData(data, sizeInBytes);
			vertexArray.SetIndex(indicesBuffer);
			indexType = type;
		}
		void SetVertexFormat(const VertexFormat & format)
		{
			vertexSize = format.Size;
			vertexArray.SetVertex(verticesBuffer, format.Attributes.GetArrayView(), format.Size);
		}
		VertexArray GetVertexArray()
		{
			return vertexArray;
		}
	};

	inline void LoadCircleMesh(StaticMesh & mesh, float screenX, float screenY, float rad, float screenW, float screenH)
	{
		const int vertCount = 24;
		Vec2 vertices[vertCount + 1];
		char indices[] = { 0 };
		float invW = 2.0f / screenW;
		float invH = 2.0f / screenH;
		vertices[0] = Vec2::Create(screenX * invW - 1.0f, screenY * invH - 1.0f);
		for (int i = 0; i < vertCount; i++)
		{
			Vec2 point;
			float ang = i / (float)(vertCount - 1) * Math::Pi * 2.0f;
			point.x = (cos(ang) * rad + screenX) * invW - 1.0f;
			point.y = (sin(ang) * rad + screenY) * invH - 1.0f;
			vertices[i + 1] = point;
		}
		mesh.SetVertexBuffer(vertices, sizeof(float) * (vertCount + 1) * 2);
		mesh.SetIndexBuffer(DataType::Byte, indices, sizeof(indices));
		mesh.VertexCount = vertCount + 1;
		mesh.PrimitiveType = PrimitiveType::TriangleFans;
	}
	
	inline void LoadRectangleMesh(StaticMesh & mesh, float x, float y, float w, float h)
	{
		float vertices[16] =
		{
			x, y, 0.0f, 0.0f,
			x + w, y, 1.0f, 0.0f,
			x + w, y + h, 1.0f, 1.0f,
			x, y + h, 0.0f, 1.0f,
		};
		char indices[] = { 0, 1, 2, 0, 2, 3 };
		mesh.SetVertexBuffer(vertices, sizeof(float) * 16);
		mesh.SetIndexBuffer(DataType::Byte, indices, sizeof(indices));
		mesh.VertexCount = 4;
		mesh.PrimitiveType = PrimitiveType::TriangleFans;
	}


	const int MaxRenderTargets = 8;
	const int TextureBindingPoints = 32;
	const int BufferBindingPoints = 32;

	template<typename T>
	class GBufferItem
	{
	public:
		String Name;
		T Buffer;
		float SamplingRate;
		int FixedSamples; // valid if != 0, otherwise use screen settings
		int FixedWidth, FixedHeight; // use this size if samplingRate == 0.0f
	};

	enum class PresetSamplers
	{
		Default, NearestClamp, LinearClamp, TrilinearClamp,
		NearestRepeat, LinearRepeat, TrilinearRepeat, Aniso4xRepeat, Aniso8xRepeat, Aniso16xRepeat,
		LinearMirror,
	};

	inline VertexFormat CreateVertexFormat(List<VertexAttribute> & attribs)
	{
		VertexFormat format;
		format.Size = 0;
		for (auto & attr : attribs)
		{
			VertexAttributeDesc dsc;
			dsc.Normalized = attr.Normalized;
			dsc.StartOffset = format.Size;
			int attrSize = 0;
			switch (attr.Type)
			{
			case VertexDataType::Float:
				attrSize = attr.Components * 4;
				dsc.Type = (DataType)((int)DataType::Float | (attr.Components - 1));
				break;
			case VertexDataType::Char:
				attrSize = attr.Components;
				dsc.Type = (DataType)((int)DataType::Char | (attr.Components - 1));
				break;
			case VertexDataType::UChar:
				attrSize = attr.Components;
				dsc.Type = (DataType)((int)DataType::Byte | (attr.Components - 1));
				break;
			case VertexDataType::Short:
				attrSize = attr.Components * 2;
				dsc.Type = (DataType)((int)DataType::Short | (attr.Components - 1));
				break;
			case VertexDataType::UShort:
				attrSize = attr.Components * 2;
				dsc.Type = (DataType)((int)DataType::UShort | (attr.Components - 1));
				break;
			case VertexDataType::Int:
				attrSize = attr.Components * 4;
				dsc.Type = (DataType)((int)DataType::Int | (attr.Components - 1));
				break;
			case VertexDataType::Half:
				attrSize = attr.Components * 2;
				dsc.Type = (DataType)((int)DataType::Half | (attr.Components - 1));
				break;
			case VertexDataType::UInt4_10_10_10_2:
				attrSize = 4;
				dsc.Type = DataType::UInt4_10_10_10_2;
				break;
			default:
				throw InvalidOperationException(L"unkown vertex attribute data type.");
			}
			format.Attributes.Add(dsc);
			format.Size += attrSize;
		}
		return format;
	}

	class DeviceResourcePool : public Object
	{
	private:
		int screenWidth, screenHeight, samples;
		float invScreenWidth, invScreenHeight;
		HardwareRenderer * hwRenderer;
		EnumerableDictionary<String, VectorMath::Vec4> texturesCubeData;

		EnumerableDictionary<String, GL::Texture2D> textures2D;
		EnumerableDictionary<String, GL::TextureCube> texturesCube;
		EnumerableDictionary<String, Program> programs;
		EnumerableDictionary<String, Shader> shaders;
		EnumerableDictionary<String, RenderBuffer> renderBuffers;
		List<GBufferItem<GL::Texture2D>> gbufferTextures;
		List<GBufferItem<RenderBuffer>> gbufferRenderBuffers;
		List<FrameBuffer> frameBuffers;
	private:
		BlendMode lastBlendMode;
		StencilMode lastStencilMode;
		BlendOperator lastDepthMode;
		CullMode lastCullMode;
	private:
		Array<TextureSampler, 10> builtinSamplers;
		void InitSamplers()
		{
			for (int i = 0; i < 10; i++)
			{
				auto sampler = hwRenderer->CreateTextureSampler();
				switch ((PresetSamplers)i)
				{
				case PresetSamplers::Default:
					sampler.SetFilter(TextureFilter::Trilinear);
					sampler.SetWrapMode(WrapMode::Repeat);
					break;
				case PresetSamplers::LinearClamp:
					sampler.SetFilter(TextureFilter::Linear);
					sampler.SetWrapMode(WrapMode::Clamp);
					break;
				case PresetSamplers::NearestClamp:
					sampler.SetFilter(TextureFilter::Nearest);
					sampler.SetWrapMode(WrapMode::Clamp);
					break;
				case PresetSamplers::TrilinearClamp:
					sampler.SetFilter(TextureFilter::Trilinear);
					sampler.SetWrapMode(WrapMode::Clamp);
					break;
				case PresetSamplers::LinearRepeat:
					sampler.SetFilter(TextureFilter::Linear);
					sampler.SetWrapMode(WrapMode::Repeat);
					break;
				case PresetSamplers::NearestRepeat:
					sampler.SetFilter(TextureFilter::Nearest);
					sampler.SetWrapMode(WrapMode::Repeat);
					break;
				case PresetSamplers::TrilinearRepeat:
					sampler.SetFilter(TextureFilter::Trilinear);
					sampler.SetWrapMode(WrapMode::Repeat);
					break;
				case PresetSamplers::LinearMirror:
					sampler.SetFilter(TextureFilter::Linear);
					sampler.SetWrapMode(WrapMode::Mirror);
					break;
				case PresetSamplers::Aniso16xRepeat:
					sampler.SetWrapMode(WrapMode::Repeat);
					sampler.SetFilter(TextureFilter::Anisotropic16x);
					break;
				case PresetSamplers::Aniso8xRepeat:
					sampler.SetWrapMode(WrapMode::Repeat);
					sampler.SetFilter(TextureFilter::Anisotropic8x);
					break;
				case PresetSamplers::Aniso4xRepeat:
					sampler.SetWrapMode(WrapMode::Repeat);
					sampler.SetFilter(TextureFilter::Anisotropic4x);
					break;
				}
				builtinSamplers.Add(sampler);
			}
		}
	private:
		// buffers for PresentTexture feature
		StaticMesh quadMesh, circleMesh;
		Program quadProgram, circleProgram;
		void InitQuadRender()
		{
			const char * vertShaderSource = R"(
				#version 440
				layout(location = 0) in vec2 vert_Position;
				layout(location = 1) in vec2 vert_TexCoords;	
			
				out vec2 texCoord;
				void main()
				{
					gl_Position = vec4(vert_Position, 0.0, 1.0);
					texCoord = vert_TexCoords;
				}
			)";
			const char * fragShaderSource = R"(
				#version 440
				layout(binding = 0) uniform sampler2D texAlbedo;
				in vec2 texCoord;
				layout(location = 0) out vec4 color;
				void main()
				{
					color = texture(texAlbedo, texCoord);
				}
			)";
			const char * circleVertShaderSource = R"(
				#version 440
				layout(location = 0) in vec2 vert_Position;
			
				void main()
				{
					gl_Position = vec4(vert_Position, 0.0, 1.0);
				}
			)";
			const char * circleFragShaderSource = R"(
				#version 440
				layout(location = 0) out vec4 color;
				void main()
				{
					color = vec4(1.0,1.0,1.0,1.0);
				}
			)";
			quadMesh = StaticMesh(this);
			VertexFormat format;
			format.Size = sizeof(float) * 4;
			format.Attributes.SetSize(2);
			format.Attributes[0] = VertexAttributeDesc(DataType::Float2, 0, 0, -1);
			format.Attributes[1] = VertexAttributeDesc(DataType::Float2, 0, sizeof(float) * 2, -1);
			quadMesh.SetVertexFormat(format);
			auto vertShader = LoadShader(ShaderType::VertexShader, vertShaderSource);
			auto fragShader = LoadShader(ShaderType::FragmentShader, fragShaderSource);
			quadProgram = LoadProgram(vertShader, fragShader);
			circleMesh = StaticMesh(this);
			VertexFormat formatCircle;
			formatCircle.Size = sizeof(float) * 2;
			formatCircle.Attributes.SetSize(1);
			formatCircle.Attributes[0] = VertexAttributeDesc(DataType::Float2, 0, 0, -1);
			circleMesh.SetVertexFormat(formatCircle);
			circleProgram = LoadProgram(LoadShader(ShaderType::VertexShader, circleVertShaderSource),
				LoadShader(ShaderType::FragmentShader, circleFragShaderSource));

		}
	public:
		DeviceResourcePool(HardwareRenderer * hwRenderer)
		{
			this->hwRenderer = hwRenderer;
			screenWidth = screenHeight = 512;
			samples = 1;
			lastBlendMode = BlendMode::Replace;
			lastCullMode = CullMode::Disabled;
			lastDepthMode = BlendOperator::Disabled;
			InitSamplers();
			InitQuadRender();
		}
		~DeviceResourcePool()
		{
			quadMesh.Free();
			for (auto & obj : textures2D)
				hwRenderer->DestroyTexture(obj.Value);
			for (auto & obj : texturesCube)
				hwRenderer->DestroyTexture(obj.Value);
			for (auto & obj : programs)
				hwRenderer->DestroyProgram(obj.Value);
			for (auto & obj : shaders)
				hwRenderer->DestroyShader(obj.Value);
			for (auto & obj : renderBuffers)
				hwRenderer->DestroyRenderBuffer(obj.Value);
			for (auto & obj : builtinSamplers)
				hwRenderer->DestroyTextureSampler(obj);
			for (auto & obj : frameBuffers)
				hwRenderer->DestroyFrameBuffer(obj);
		}
		HardwareRenderer * GetHardwareRenderer()
		{
			return hwRenderer;
		}
		int GetScreenWidth()
		{
			return screenWidth;
		}
		int GetScreenHeight()
		{
			return screenHeight;
		}
		void SetScreenSize(int width, int height, int pSamples)
		{
			screenWidth = width;
			screenHeight = height;
			this->samples = pSamples == 0 ? 1 : pSamples;
			invScreenWidth = 1.0f / (float)width;
			invScreenHeight = 1.0f / (float)height;
			this->samples = pSamples;
			for (auto & rt : gbufferTextures)
			{
				if (rt.SamplingRate != 0.0f)
				{
					auto storageFormat = rt.Buffer.storageFormat;
					hwRenderer->DestroyTexture(rt.Buffer);
					rt.Buffer = hwRenderer->CreateTexture2D();
					rt.Buffer.SetData(storageFormat, (int)(width * rt.SamplingRate), (int)(height * rt.SamplingRate), 
						rt.FixedSamples == 0 ? pSamples : rt.FixedSamples, DataType::Float4, nullptr);
					rt.Buffer.BuildMipmaps();
					textures2D[rt.Name] = rt.Buffer;
				}
			}
			for (auto & rt : gbufferRenderBuffers)
			{
				if (rt.SamplingRate != 0.0f)
					rt.Buffer.Resize((int)(width * rt.SamplingRate), (int)(height * rt.SamplingRate), rt.FixedSamples == 0 ? pSamples : rt.FixedSamples);
			}
			hwRenderer->SetViewport(0, 0, width, height);
		}
		Shader LoadShader(ShaderType type, const String & source)
		{
			Shader value;
			if (shaders.TryGetValue(source, value))
				return value;
			auto rs = hwRenderer->CreateShader(type, source);
			shaders[source] = rs;
			return rs;
		}
		Program LoadProgram(Shader vertexShader, Shader fragmentShader)
		{
			auto programName = String((long long)vertexShader.GetHandle()) + L"_" + String((long long)fragmentShader.GetHandle());
			Program value;
			if (programs.TryGetValue(programName, value))
				return value;
			auto rs = hwRenderer->CreateProgram(vertexShader, fragmentShader);
			programs[programName] = rs;
			return rs;
		}
		Program LoadProgram(Shader vertexShader, Shader fragmentShader, CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, int> & attribBinding)
		{
			auto programName = String((long long)vertexShader.GetHandle()) + L"_" + String((long long)fragmentShader.GetHandle());
			Program value;
			if (programs.TryGetValue(programName, value))
				return value;
			auto rs = hwRenderer->CreateProgram(vertexShader, fragmentShader, attribBinding);
			programs[programName] = rs;
			return rs;
		}
		GL::Texture2D LoadTextureSingleSample(int r, int g, int b, int a)
		{
			StringBuilder texName;
			texName << L"#" << String(r,16) << String(g,16) << String(b,16) << String(a,16);
			auto name = texName.ProduceString();
			GL::Texture2D value;
			if (textures2D.TryGetValue(name, value))
				return value;
			auto rs = hwRenderer->CreateTexture2D();
			unsigned char data[4] = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
			rs.SetData(StorageFormat::RGBA_I8, 1, 1, 1, DataType::Byte4, data);
			textures2D[name] = rs;
			return rs;
		}
		GL::Texture2D LoadTexture2D(const String & name, StorageFormat format, int width, int height, int pSamples, DataType inputType, void * data)
		{
			GL::Texture2D value;
			if (textures2D.TryGetValue(name, value))
				return value;
			auto rs = hwRenderer->CreateTexture2D();
			rs.SetData(format, width, height, pSamples, inputType, data);
			rs.BuildMipmaps();
			textures2D[name] = rs;
			return rs;
		}
		GL::Texture2D LoadTexture2D(const String & pFilename, StorageFormat storage = StorageFormat::RGBA_I8)
		{
			GL::Texture2D value;
			String filename = pFilename;
			auto textureFileName = Path::ReplaceExt(filename, L"texture");
			if (File::Exists(textureFileName))
				filename = textureFileName;
			if (textures2D.TryGetValue(filename, value))
				return value;
			if (filename.ToLower().EndsWith(L".texture"))
			{
				CoreLib::Graphics::TextureFile file(filename);
				StorageFormat format;
				DataType dataType;
				switch (file.GetFormat())
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
					format = StorageFormat::RGB_I8;
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
				default:
					throw NotImplementedException(L"unsupported texture format.");
				}
				
				return LoadTexture2D(filename, format, file.GetWidth(), file.GetHeight(), 1, dataType, file.GetData().Buffer());
			}
			else
			{
				CoreLib::Imaging::Bitmap bmp(filename);
				List<unsigned int> pixelsInversed;
				int * sourcePixels = (int*)bmp.GetPixels();
				pixelsInversed.SetSize(bmp.GetWidth() * bmp.GetHeight());
				for (int i = 0; i < bmp.GetHeight(); i++)
				{
					for (int j = 0; j < bmp.GetWidth(); j++)
						pixelsInversed[i*bmp.GetWidth() + j] = sourcePixels[(bmp.GetHeight() - 1 - i)*bmp.GetWidth() + j];
				}
				CoreLib::Imaging::TextureData<CoreLib::Imaging::Color4F> tex;
				CoreLib::Imaging::CreateTextureDataFromBitmap(tex, bmp);
				CoreLib::Graphics::TextureFile texFile;
				CoreLib::Graphics::TextureStorageFormat texStorage = CoreLib::Graphics::TextureStorageFormat::RGBA8;
				switch (storage)
				{
				case StorageFormat::Int8:
					texStorage = CoreLib::Graphics::TextureStorageFormat::R8;
					break;
				case StorageFormat::RG_I8:
					texStorage = CoreLib::Graphics::TextureStorageFormat::RG8;
					break;
				case StorageFormat::RGB_I8:
					texStorage = CoreLib::Graphics::TextureStorageFormat::RGB8;
					break;
				case StorageFormat::RGBA_I8:
					texStorage = CoreLib::Graphics::TextureStorageFormat::RGBA8;
					break;
				case StorageFormat::Float16:
				case StorageFormat::Float32:
					texStorage = CoreLib::Graphics::TextureStorageFormat::R_F32;
					break;
				case StorageFormat::RG_F16:
				case StorageFormat::RG_F32:
					texStorage = CoreLib::Graphics::TextureStorageFormat::RG_F32;
					break;
				case StorageFormat::RGB_F16:
				case StorageFormat::RGB_F32:
					texStorage = CoreLib::Graphics::TextureStorageFormat::RGB_F32;
					break;
				case StorageFormat::RGBA_F16:
				case StorageFormat::RGBA_F32:
					texStorage = CoreLib::Graphics::TextureStorageFormat::RGBA_F32;
					break;
				}
				CoreLib::Imaging::CreateTextureFile(texFile, texStorage, tex);
				texFile.SaveToFile(Path::ReplaceExt(filename, L"texture"));
				return LoadTexture2D(filename, storage, bmp.GetWidth(), bmp.GetHeight(), 1, DataType::Byte4, pixelsInversed.Buffer());
			}
		}
		GL::Texture2D LoadErrorTexture2D()
		{
			GL::Texture2D value;
			if (textures2D.TryGetValue(L"<err_tex_2d>", value))
				return value;
			auto rs = hwRenderer->CreateTexture2D();
			auto makeRGB = [](int r, int g, int b)
			{
				union RGB_Int
				{
					struct
					{
						unsigned char r, g, b, a;
					} fields;
					unsigned int val;
				} convert;
				convert.fields.r = (unsigned char)r; convert.fields.g = (unsigned char)g; convert.fields.b = (unsigned char)b;
				convert.fields.a = 255;
				return convert.val;
			};
			unsigned int data[4];
			data[0] = data[3] = makeRGB(255, 0, 255);
			data[1] = data[2] = makeRGB(0, 255, 0);
			rs.SetData(StorageFormat::RGBA_I8, 2, 2, 1, DataType::Byte4, data);
			rs.BuildMipmaps();
			textures2D[L"<err_tex_2d>"] = rs;
			return rs;
		}
		RenderBuffer LoadGBufferRenderBuffer(const String & name, StorageFormat format, float samplingRate, int pSamples)
		{
			RenderBuffer value;
			if (renderBuffers.TryGetValue(name, value))
			{
				return value;
			}
			int bufferWidth = (int)(screenWidth * samplingRate);
			int bufferHeight = (int)(screenHeight * samplingRate);
			value = hwRenderer->CreateRenderBuffer(format, bufferWidth, bufferHeight, pSamples == 0 ? this->samples : pSamples);
			auto item = GBufferItem<RenderBuffer>();
			item.Buffer = value;
			item.FixedSamples = pSamples;
			item.SamplingRate = samplingRate;
			gbufferRenderBuffers.Add(item);
			renderBuffers[name] = value;
			return value;
		}
		Vec4 GetTextureCubeData(const String & name)
		{
			Vec4 rs;
			rs.SetZero();
			texturesCubeData.TryGetValue(name, rs);
			return rs;
		}
		GL::Texture2D GetTexture(const String & name)
		{
			GL::Texture2D tex;
			textures2D.TryGetValue(name, tex);
			return tex;
		}
		GL::Texture2D LoadGBufferTexture(const String & name, StorageFormat format, DataType type, int w, int h, int pSamples)
		{
			GL::Texture2D value;
			if (textures2D.TryGetValue(name, value))
			{
				return value;
			}
			int bufferWidth = w;
			int bufferHeight = h;
			value = hwRenderer->CreateTexture2D();
			value.SetData(format, bufferWidth, bufferHeight, pSamples==0?this->samples:pSamples, type, 0);
			auto item = GBufferItem<GL::Texture2D>();
			item.Buffer = value;
			item.Name = name;
			item.FixedSamples = pSamples;
			item.SamplingRate = 0.0f;
			item.FixedWidth = w;
			item.FixedHeight = h;
			gbufferTextures.Add(item);
			textures2D[name] = value;
			return value;
		}
		GL::TextureCube LoadTextureCube(const String & name, StorageFormat format, DataType type, int size, int pSamples)
		{
			GL::TextureCube value;
			if (texturesCube.TryGetValue(name, value))
			{
				return value;
			}
			value = hwRenderer->CreateTextureCube();
			for (int i = 0; i < 6; i++)
				value.SetData(TextureCubeFace(i), format, size, size, pSamples, type, 0);
			texturesCube[name] = value;
			return value;
		}
		GL::TextureCube LoadTextureCube(const ArrayView<String> fileNames, StorageFormat storage = StorageFormat::RGBA_I8)
		{
			GL::TextureCube value;
			if (texturesCube.TryGetValue(fileNames[0], value))
			{
				return value;
			}
			value = hwRenderer->CreateTextureCube();
			Vec4 summary;
			summary.SetZero();
			int pixelCount = 0;
			for (int f = 0; f < 6; f++)
			{
				CoreLib::Imaging::Bitmap bmp(fileNames[f]);
				List<unsigned int> pixelsInversed;
				int * sourcePixels = (int*)bmp.GetPixels();
				pixelsInversed.SetSize(bmp.GetWidth() * bmp.GetHeight());
				for (int i = 0; i < bmp.GetHeight(); i++)
				{
					for (int j = 0; j < bmp.GetWidth(); j++)
					{
						int color = sourcePixels[(bmp.GetHeight() - 1 - i)*bmp.GetWidth() + j];
						pixelsInversed[i*bmp.GetWidth() + j] = color;
						CoreLib::Imaging::Color c;
						c.Value = color;
						summary.x += c.R / (float)255.0f;
						summary.y += c.G / (float)255.0f;
						summary.z += c.B / (float)255.0f;
						summary.w += c.A / (float)255.0f;
						pixelCount++;
					}
				}
				auto face = (TextureCubeFace)((int)TextureCubeFace::PositiveX + f);
				value.SetData(face, storage, bmp.GetWidth(), bmp.GetHeight(), 1, DataType::Byte4, pixelsInversed.Buffer());
			}
			value.BuildMipmaps();
			texturesCube[fileNames[0]] = value;
			texturesCubeData[fileNames[0]] = summary;
			return value;
		}
		GL::Texture2D LoadGBufferTexture(const String & name, StorageFormat format, DataType type, float samplingRate, int pSamples)
		{
			GL::Texture2D value;
			if (textures2D.TryGetValue(name, value))
			{
				return value;
			}
			int bufferWidth = (int)(screenWidth * samplingRate);
			int bufferHeight = (int)(screenHeight * samplingRate);
			value = hwRenderer->CreateTexture2D();
			value.SetData(format, bufferWidth, bufferHeight, pSamples, type, 0);
			auto item = GBufferItem<GL::Texture2D>();
			item.Buffer = value;
			item.FixedSamples = pSamples;
			item.SamplingRate = samplingRate;
			item.FixedWidth = item.FixedHeight = 0;
			gbufferTextures.Add(item);
			textures2D[name] = value;
			return value;
		}
		void FreeGBufferTexture(const String & name)
		{
			GL::Texture2D val;
			if (textures2D.TryGetValue(name, val))
			{
				for (int i = 0; i < gbufferTextures.Count(); i++)
					if (gbufferTextures[i].Buffer == val)
					{
						gbufferTextures.RemoveAt(i);
						break;
					}
				textures2D.Remove(name);
				this->GetHardwareRenderer()->DestroyTexture(val);
			}
		}
		
		StaticMesh CreateMeshFromFile(const CoreLib::Basic::String & fileName, CoreLib::Graphics::BBox & bounds, VertexFormat & format, Mesh & mesh)
		{
			mesh.LoadFromFile(fileName);
			bounds = mesh.Bounds;
			StaticMesh apiMesh(this);
			format = CreateVertexFormat(mesh.Attributes);
			apiMesh.SetVertexBuffer(mesh.VertexData.Buffer(), mesh.VertexData.Count());
			apiMesh.PrimitiveType = PrimitiveType::Triangles;
			apiMesh.VertexCount = mesh.VertexData.Count()/format.Size;
			apiMesh.SetVertexFormat(format);
			apiMesh.BaseAttributeCount = format.Attributes.Count();
			return apiMesh;
		}
		FrameBuffer CreateFrameBuffer()
		{
			auto fb = hwRenderer->CreateFrameBuffer();
			frameBuffers.Add(fb);
			return fb;
		}
		TextureSampler GetPresetSampler(PresetSamplers sampler)
		{
			return builtinSamplers[(int)sampler];
		}
		void Render(const StaticMesh & mesh, int first, int count)
		{
			// draw call
			hwRenderer->BindVertexArray(mesh.vertexArray);
			hwRenderer->DrawArray(mesh.PrimitiveType, first, count);
			hwRenderer->BindVertexArray(VertexArray());
		}
		void Render(const StaticMesh & mesh, List<IndirectDrawCommands> & indirectBuffer)
		{
			// draw call
			hwRenderer->BindVertexArray(mesh.vertexArray);
			hwRenderer->DrawArrayInstancesIndirect(mesh.PrimitiveType, indirectBuffer.Buffer(), indirectBuffer.Count(), 0);
			hwRenderer->BindVertexArray(VertexArray());
		}
		void Render(const StaticMesh & mesh, int instances)
		{
			// draw call
			hwRenderer->BindVertexArray(mesh.vertexArray);
			hwRenderer->DrawArrayInstances(mesh.PrimitiveType, mesh.VertexCount, instances);
			hwRenderer->BindVertexArray(VertexArray());
		}

		void RenderTransformFeedbackPoints(const StaticMesh & mesh, int instances)
		{
			// draw call
			hwRenderer->BindVertexArray(mesh.vertexArray);
			hwRenderer->DrawArrayInstances(mesh.PrimitiveType, mesh.VertexCount, instances);
			hwRenderer->BindVertexArray(VertexArray());
		}

		void PresentTexture(Texture texture, int x, int y, int width, int height, PresetSamplers sampler)
		{
			auto h = (float)(height << 1) * this->invScreenHeight;
			quadProgram.Use();
			auto texView = MakeArrayView(texture);
			hwRenderer->UseTextures(texView, MakeArrayView(GetPresetSampler(sampler)));
			DrawFullScreenQuad((float)(x << 1) * this->invScreenWidth - 1.0f, 1.0f - (float)(y << 1) * this->invScreenHeight - h, (float)(width << 1) * this->invScreenWidth, h);
			hwRenderer->FinishUsingTextures(texView);
		}

		void DrawFullScreenQuad(float x, float y, float w, float h)
		{
			LoadRectangleMesh(quadMesh, x, y, w, h);
			hwRenderer->BindVertexArray(quadMesh.vertexArray);
			hwRenderer->DrawArrayInstances(quadMesh.PrimitiveType, 4, 1);
			hwRenderer->BindVertexArray(VertexArray());
		}

		void DrawScreenSpaceCircle(float x, float y, float rad) // coordinates in screen space
		{
			circleProgram.Use();
			LoadCircleMesh(circleMesh, x, y, rad, (float)screenWidth, (float)screenHeight);
			hwRenderer->BindVertexArray(circleMesh.vertexArray);
			hwRenderer->DrawArrayInstances(circleMesh.PrimitiveType, circleMesh.VertexCount, 1);
			hwRenderer->BindVertexArray(VertexArray());
		}

		List<String> GetRenderTargets()
		{
			List<String> result;
			for (auto & p : textures2D)
			{
				if (p.Key.StartsWith(L"$"))
					result.Add(p.Key);
			}
			return result;
		}
	};
}
#endif