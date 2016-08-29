#ifndef CORELIB_GRAPHICS_TEXTURE_FILE_H
#define CORELIB_GRAPHICS_TEXTURE_FILE_H

#include "../Basic.h"
#include "../LibIO.h"

namespace CoreLib
{
	namespace Graphics
	{
		enum class TextureType : short
		{
			Texture2D
		};
		enum class TextureStorageFormat : short
		{
			R8, RG8, RGB8, RGBA8,
			R_F32, RG_F32, RGB_F32, RGBA_F32, BC1, BC5,
		};
		class TextureFileHeader
		{
		public:
			TextureType Type;
			TextureStorageFormat Format;
			int Width, Height;
		};

		class TextureFile
		{
		private:
			CoreLib::Basic::List<CoreLib::Basic::List<unsigned char>> buffer;
			TextureStorageFormat format;
			int width, height;
			void LoadFromStream(CoreLib::IO::Stream * stream);
		public:
			TextureFile()
			{
				width = height = 0;
				format = TextureStorageFormat::RGBA8;
			}
			TextureFile(CoreLib::Basic::String fileName);
			TextureFile(CoreLib::IO::Stream * stream);
			TextureStorageFormat GetFormat()
			{
				return format;
			}
			int GetWidth()
			{
				return width;
			}
			int GetHeight()
			{
				return height;
			}
			int GetMipLevels()
			{
				return buffer.Count();
			}
			void SaveToFile(CoreLib::Basic::String fileName);
			void SaveToStream(CoreLib::IO::Stream * stream);
			void SetData(TextureStorageFormat format, int width, int height, int level, CoreLib::Basic::ArrayView<unsigned char> data);
			CoreLib::Basic::ArrayView<unsigned char> GetData(int level = 0)
			{
				return buffer[level].GetArrayView();
			}
			CoreLib::Basic::List<float> GetPixels()
			{
				CoreLib::Basic::List<float> pixels;
				pixels.SetSize(width * height * 4);
				for (int i = 0; i < width*height; i++)
				{
					float color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
					switch (format)
					{
					case TextureStorageFormat::R8:
						color[0] = buffer[0][i] / 255.0f;
						break;
					case TextureStorageFormat::RG8:
						color[0] = buffer[0][i*2] / 255.0f;
						color[1] = buffer[0][i*2 + 1] / 255.0f;
						break;
					case TextureStorageFormat::RGB8:
						color[0] = buffer[0][i * 3] / 255.0f;
						color[1] = buffer[0][i * 3 + 1] / 255.0f;
						color[2] = buffer[0][i * 3 + 2] / 255.0f;
						break;
					case TextureStorageFormat::RGBA8:
						color[0] = buffer[0][i * 4] / 255.0f;
						color[1] = buffer[0][i * 4 + 1] / 255.0f;
						color[2] = buffer[0][i * 4 + 2] / 255.0f;
						color[3] = buffer[0][i * 4 + 3] / 255.0f;
						break;
					case TextureStorageFormat::R_F32:
						color[0] = ((float*)buffer[0].Buffer())[i];
						break;
					case TextureStorageFormat::RG_F32:
						color[0] = ((float*)buffer[0].Buffer())[i*2];
						color[1] = ((float*)buffer[0].Buffer())[i*2 + 1];
						break;
					case TextureStorageFormat::RGB_F32:
						color[0] = ((float*)buffer[0].Buffer())[i * 3];
						color[1] = ((float*)buffer[0].Buffer())[i * 3 + 1];
						color[2] = ((float*)buffer[0].Buffer())[i * 3 + 2];
						break;
					case TextureStorageFormat::RGBA_F32:
						color[0] = ((float*)buffer[0].Buffer())[i * 4];
						color[1] = ((float*)buffer[0].Buffer())[i * 4 + 1];
						color[2] = ((float*)buffer[0].Buffer())[i * 4 + 2];
						color[3] = ((float*)buffer[0].Buffer())[i * 4 + 3];
						break;
					default:
						throw NotImplementedException();
					}
					pixels[i * 4] = color[0];
					pixels[i * 4 + 1] = color[1];
					pixels[i * 4 + 2] = color[2];
					pixels[i * 4 + 3] = color[3];

				}
				return pixels;
			}
		};
	}
}

#endif