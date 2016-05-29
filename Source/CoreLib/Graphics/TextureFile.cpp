#include "TextureFile.h"

using namespace CoreLib::Basic;
using namespace CoreLib::IO;

namespace CoreLib
{
	namespace Graphics
	{
		TextureFile::TextureFile(String fileName)
		{
			FileStream stream(fileName);
			LoadFromStream(&stream);
		}
		TextureFile::TextureFile(Stream * stream)
		{
			LoadFromStream(stream);
		}
		int GetPixelSize(TextureStorageFormat format)
		{
			switch (format)
			{
			case TextureStorageFormat::R8:
				return 1;
			case TextureStorageFormat::RG8:
				return 2;
			case TextureStorageFormat::RGB8:
				return 3;
			case TextureStorageFormat::RGBA8:
				return 4;
			case TextureStorageFormat::R_F32:
				return 4;
			case TextureStorageFormat::RG_F32:
				return 8;
			case TextureStorageFormat::RGB_F32:
				return 12;
			case TextureStorageFormat::RGBA_F32:
				return 16;
			default:
				return 0;
			}
		}
		void TextureFile::LoadFromStream(Stream * stream)
		{
			BinaryReader reader(stream);
			TextureFileHeader header;
			int headerSize = reader.ReadInt32();
			reader.Read((unsigned char*)&header, headerSize);
			if (header.Type == TextureType::Texture2D)
			{
				width = header.Width;
				height = header.Height;
				format = header.Format;
				int pixelSize = GetPixelSize(format);
				buffer.SetSize(pixelSize * width * height);
				reader.Read(buffer.Buffer(), buffer.Count());
			}
			reader.ReleaseStream();
		}
		void TextureFile::SetData(TextureStorageFormat storageFormat, int w, int h, CoreLib::Basic::ArrayView<unsigned char> data)
		{
			int pixelSize = GetPixelSize(storageFormat);
			if (data.Count() != pixelSize * w * h)
				throw InvalidOperationException(L"Data size does not match texture format.");
			buffer.SetSize(data.Count());
			memcpy(buffer.Buffer(), data.Buffer(), data.Count());
			this->format = storageFormat;
			this->width = w;
			this->height = h;
		}
		void TextureFile::SaveToStream(Stream * stream)
		{
			BinaryWriter writer(stream);
			int headerSize = sizeof(TextureFileHeader);
			writer.Write(headerSize);
			TextureFileHeader header;
			header.Format = format;
			header.Width = width;
			header.Height = height;
			header.Type = TextureType::Texture2D;
			writer.Write(header);
			writer.Write(buffer.Buffer(), buffer.Count());
			writer.ReleaseStream();
		}
		void TextureFile::SaveToFile(String fileName)
		{
			FileStream stream(fileName, FileMode::Create);
			SaveToStream(&stream);
		}
	}
}