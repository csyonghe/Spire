#include "TextureData.h"

#ifdef TEXTURE_ACCESS_DUMP
bool EnableTextureAccessDump;
TextWriter * TextureAccessDebugWriter;
#endif

namespace CoreLib
{
	namespace Imaging
	{
		using namespace CoreLib::Basic;

		void CreateDefaultTextureData(TextureData<Color> & tex)
		{
			tex.Levels.SetSize(2);
			tex.Levels[0].Width = 2;
			tex.Levels[0].Height = 2;
			tex.Levels[0].Pixels.SetSize(4);
			tex.Levels[0].Pixels[2] = tex.Levels[0].Pixels[0] = Color(255, 0, 255, 255);
			tex.Levels[0].Pixels[1] = tex.Levels[0].Pixels[3] = Color(0, 255, 0, 255);
			tex.Levels[1].Width = 1;
			tex.Levels[1].Height = 1;
			tex.Levels[1].Pixels.SetSize(1);
			tex.Levels[1].Pixels[0] = Color(127, 127, 127, 255);
			tex.Width = tex.Height = 2;
			tex.InvHeight = tex.InvWidth = 0.5f;
			tex.IsTransparent = false;
		}

		void CreateTextureDataFromBitmap(TextureData<Color4F> & tex, Bitmap & image)
		{
			tex.Levels.SetSize(CeilLog2(Math::Max(image.GetWidth(), image.GetHeight())) + 1);
			tex.Levels[0].Pixels.Reserve(image.GetWidth()*image.GetHeight());
			for (int i = image.GetHeight() - 1; i >= 0; i--)
				for (int j = 0; j < image.GetWidth(); j++)
				{
					auto color = *((Color*)image.GetPixels() + i * image.GetWidth() + j);
					Color4F cf;
					cf.x = color.R / 255.0f;
					cf.y = color.G / 255.0f;
					cf.z = color.B / 255.0f;
					cf.w = color.A / 255.0f;
					tex.Levels[0].Pixels.Add(cf);

				}
			tex.Levels[0].Width = image.GetWidth();
			tex.Levels[0].Height = image.GetHeight();
			tex.Width = image.GetWidth();
			tex.Height = image.GetHeight();
			tex.IsTransparent = image.GetIsTransparent();
			//tex.GenerateMipmaps();
		}

		void CreateTextureDataFromTextureFile(TextureData<Color4F> & tex, CoreLib::Graphics::TextureFile & texFile)
		{
			List<float> pix = texFile.GetPixels();
			tex.Levels.SetSize(CeilLog2(Math::Max(texFile.GetWidth(), texFile.GetHeight())) + 1);
			tex.Levels[0].Pixels.Reserve(texFile.GetWidth()*texFile.GetHeight());
			for (int i = 0; i < texFile.GetHeight(); i++)
				for (int j = 0; j < texFile.GetWidth(); j++)
				{
					auto cf = *(((Color4F*)pix.Buffer()) + i * texFile.GetWidth() + j);
					tex.Levels[0].Pixels.Add(cf);

				}
			tex.Levels[0].Width = texFile.GetWidth();
			tex.Levels[0].Height = texFile.GetHeight();
			tex.Width = texFile.GetWidth();
			tex.Height = texFile.GetHeight();
			tex.IsTransparent = false;
			//tex.GenerateMipmaps();
		}

		void CreateTextureFile(CoreLib::Graphics::TextureFile & file, CoreLib::Graphics::TextureStorageFormat storageFormat, TextureData<Color4F>& tex)
		{
			List<unsigned char> buffer;
			for (auto & pix : tex.Levels[0].Pixels)
			{
				switch (storageFormat)
				{
				case CoreLib::Graphics::TextureStorageFormat::R8:
					buffer.Add((unsigned char)Math::Clamp((pix.x * 255.0f), 0.0f, 255.0f));
					break;
				case CoreLib::Graphics::TextureStorageFormat::RG8:
					buffer.Add((unsigned char)Math::Clamp((pix.x * 255.0f), 0.0f, 255.0f));
					buffer.Add((unsigned char)Math::Clamp((pix.y * 255.0f), 0.0f, 255.0f));
					break;
				case CoreLib::Graphics::TextureStorageFormat::RGB8:
					buffer.Add((unsigned char)Math::Clamp((pix.x * 255.0f), 0.0f, 255.0f));
					buffer.Add((unsigned char)Math::Clamp((pix.y * 255.0f), 0.0f, 255.0f));
					buffer.Add((unsigned char)Math::Clamp((pix.z * 255.0f), 0.0f, 255.0f));
					break;
				case CoreLib::Graphics::TextureStorageFormat::RGBA8:
					buffer.Add((unsigned char)Math::Clamp((pix.x * 255.0f), 0.0f, 255.0f));
					buffer.Add((unsigned char)Math::Clamp((pix.y * 255.0f), 0.0f, 255.0f));
					buffer.Add((unsigned char)Math::Clamp((pix.z * 255.0f), 0.0f, 255.0f));
					buffer.Add((unsigned char)Math::Clamp((pix.w * 255.0f), 0.0f, 255.0f));
					break;
				case CoreLib::Graphics::TextureStorageFormat::R_F32:
					buffer.AddRange((unsigned char*)&pix.x, sizeof(float));
					break;
				case CoreLib::Graphics::TextureStorageFormat::RG_F32:
					buffer.AddRange((unsigned char*)&pix.x, sizeof(float));
					buffer.AddRange((unsigned char*)&pix.y, sizeof(float));
					break;
				case CoreLib::Graphics::TextureStorageFormat::RGB_F32:
					buffer.AddRange((unsigned char*)&pix.x, sizeof(float));
					buffer.AddRange((unsigned char*)&pix.y, sizeof(float));
					buffer.AddRange((unsigned char*)&pix.z, sizeof(float));
					break;
				case CoreLib::Graphics::TextureStorageFormat::RGBA_F32:
					buffer.AddRange((unsigned char*)&pix.x, sizeof(float));
					buffer.AddRange((unsigned char*)&pix.y, sizeof(float));
					buffer.AddRange((unsigned char*)&pix.z, sizeof(float));
					buffer.AddRange((unsigned char*)&pix.w, sizeof(float));
					break;
				}
			}
			file.SetData(storageFormat, tex.Width, tex.Height, 0, buffer.GetArrayView());
		}

		void CreateTextureDataFromBitmap(TextureData<Color4F> & tex, BitmapF & image)
		{
			tex.Levels.SetSize(CeilLog2(Math::Max(image.GetWidth(), image.GetHeight())) + 1);
			tex.Levels[0].Pixels.Reserve(image.GetWidth()*image.GetHeight());
			for (int i = image.GetHeight() - 1; i >= 0; i--)
			{
				for (int j = 0; j < image.GetWidth(); j++)
				{
					Color4F c(image.GetPixels()[i*image.GetWidth() + j]);
					tex.Levels[0].Pixels.Add(c);
				}
			}
			tex.Levels[0].Width = image.GetWidth();
			tex.Levels[0].Height = image.GetHeight();
			tex.Width = image.GetWidth();
			tex.Height = image.GetHeight();
			tex.IsTransparent = false;
			tex.GenerateMipmaps();
		}

		void CreateTextureDataFromBitmap(TextureData<Color> & tex, Bitmap & image)
		{
			tex.Levels.SetSize(CeilLog2(Math::Max(image.GetWidth(), image.GetHeight())) + 1);
			tex.Levels[0].Pixels.Reserve(image.GetWidth()*image.GetHeight());
			for (int i = image.GetHeight() - 1; i >= 0; i--)
				tex.Levels[0].Pixels.AddRange((Color*)image.GetPixels() + i *image.GetWidth(), image.GetWidth());
			tex.Levels[0].Width = image.GetWidth();
			tex.Levels[0].Height = image.GetHeight();
			tex.Width = image.GetWidth();
			tex.Height = image.GetHeight();
			tex.IsTransparent = image.GetIsTransparent();
			tex.GenerateMipmaps();
		}


		void CreateTextureDataFromFile(TextureData<Color> & tex, const Basic::String & fileName)
		{
			// Read file to Levels[0]
			Bitmap image(fileName);
			CreateTextureDataFromBitmap(tex, image);
		}

		
	}
}