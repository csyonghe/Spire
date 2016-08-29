#include "TextureFile.h"

namespace CoreLib
{
	namespace Graphics
	{
		using namespace CoreLib::Basic;
		using namespace CoreLib::IO;

		TextureFile::TextureFile(String fileName)
		{
			FileStream stream(fileName);
			LoadFromStream(&stream);
		}
		TextureFile::TextureFile(Stream * stream)
		{
			LoadFromStream(stream);
		}
		double GetPixelSize(TextureStorageFormat format)
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
			case TextureStorageFormat::BC1:
				return 0.5f;
			case TextureStorageFormat::BC5:
				return 1;
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
				int levels = reader.ReadInt32();
				buffer.SetSize(levels);
				for (int i = 0; i < levels; i++)
				{
					int bufSize = reader.ReadInt32();
					buffer[i].SetSize(bufSize);
					reader.Read(buffer[i].Buffer(), buffer[i].Count());
				}
			}
			reader.ReleaseStream();
		}
		void TextureFile::SetData(TextureStorageFormat storageFormat, int w, int h, int level, CoreLib::Basic::ArrayView<unsigned char> data)
		{
			auto pixelSize = GetPixelSize(storageFormat);
			if (storageFormat == TextureStorageFormat::BC1 || storageFormat == TextureStorageFormat::BC5)
			{
				if (data.Count() != (int)(ceil(w / 4.0f) * ceil(h / 4.0f) * 16 * pixelSize))
					throw InvalidOperationException(L"Data size does not match texture format.");
			}
			else
			{
				if (data.Count() != (int)(w * h * pixelSize))
					throw InvalidOperationException(L"Data size does not match texture format.");
			}
			if (level >= buffer.Count())
				buffer.SetSize(level + 1);
			buffer[level].SetSize(data.Count());
			memcpy(buffer[level].Buffer(), data.Buffer(), data.Count());
			this->format = storageFormat;
			if (level == 0)
			{
				this->width = w;
				this->height = h;
			}
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
			writer.Write(buffer.Count());
			for (int i = 0; i < buffer.Count(); i++)
			{
				writer.Write(buffer[i].Count());
				writer.Write(buffer[i].Buffer(), buffer[i].Count());
			}
			writer.ReleaseStream();
		}
		void TextureFile::SaveToFile(String fileName)
		{
			FileStream stream(fileName, FileMode::Create);
			SaveToStream(&stream);
		}
	}
}