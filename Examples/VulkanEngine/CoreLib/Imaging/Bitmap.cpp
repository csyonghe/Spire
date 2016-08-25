#include "Bitmap.h"
#include "../Stream.h"
#include "../LibIO.h"

#include "stb_image.h"
#include "lodepng.h"
#include "../LibIO.h"

namespace CoreLib
{
	namespace Imaging
	{
		using namespace CoreLib::Basic;

		VectorMath::Vec4 * LoadPFM(String fileName, int & w, int & h)
		{
			List<float> buffer;
			try
			{
				CoreLib::IO::BinaryReader reader(new CoreLib::IO::FileStream(fileName));
				for (int i = 0; i < 3; i++) reader.ReadChar();
				StringBuilder strW, strH;
				char ch = 0;
				do
				{
					ch = reader.ReadChar();
					if (ch != ' ' && ch != '\n' && ch != '\r')
						strW.Append((wchar_t)ch);
				} while (ch != ' ' && ch != '\n' && ch != '\r');
				do
				{
					ch = reader.ReadChar();
					if (ch != '\n' && ch != '\r')
						strH.Append((wchar_t)ch);
				} while (ch != '\n' && ch != '\r');
				w = StringToInt(strW.ProduceString());
				h = StringToInt(strH.ProduceString());
				do
				{
					ch = reader.ReadChar();
				} while (ch != '\n');
				
				buffer.SetSize(w*h * 3);
				reader.Read(buffer.Buffer(), buffer.Count());
			}
			catch (Exception)
			{
				return nullptr;
			}
			VectorMath::Vec4 * rs = (VectorMath::Vec4 *)malloc(sizeof(VectorMath::Vec4) * w * h);
			for (int i = 0; i < w*h; i++)
			{
				rs[i].x = buffer[i * 3];
				rs[i].y = buffer[i * 3 + 1];
				rs[i].z = buffer[i * 3 + 2];
			}
			return rs;
		}

		BitmapF::BitmapF(String fileName)
		{
			int channel = 4;
			if (fileName.EndsWith(L"pfm") || fileName.EndsWith(L"PFM"))
				pixels = LoadPFM(fileName, width, height);
			else
				pixels = (VectorMath::Vec4 *)stbi_loadf(fileName.ToMultiByteString(), &width, &height, &channel, 4);
			if (!pixels)
				throw IO::IOException(L"Cannot load image \"" + fileName + L"\"");
		}

		Bitmap::Bitmap(String fileName)
		{
			int channel;
			pixels = stbi_load(fileName.ToMultiByteString(), &width, &height, &channel, 4);
			isTransparent = (channel == 4);
			if (!pixels)
				throw IO::IOException(L"Cannot load image \"" + fileName + L"\"");
		}

		void ImageRef::SaveAsBmpFile(Basic::String fileName, bool reverseY)
		{
			FILE *f = 0;
			int filesize = 54 + 3*Width*Height;
			Basic::List<unsigned char> img;
			img.SetSize(3*Width*Height);
			memset(img.Buffer(),0,3*Width*Height);
			for(int j=0; j<Height; j++)
			{
				VectorMath::Vec4 * scanLine = Pixels + j*Width;
				for(int i=0; i<Width; i++)
				{
					int x = i; 
					int y = reverseY?j:(Height-1)-j;

					int r = (int)(scanLine[i].x*255);
					int g = (int)(scanLine[i].y*255);
					int b = (int)(scanLine[i].z*255);
					if (r > 255) r=255;
					if (g > 255) g=255;
					if (b > 255) b=255;
					if (r < 0) r = 0;
					if (g < 0) g = 0;
					if (b < 0) b = 0;
					img[(x+y*Width)*3+2] = (unsigned char)(r);
					img[(x+y*Width)*3+1] = (unsigned char)(g);
					img[(x+y*Width)*3+0] = (unsigned char)(b);
				}
			}

			unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
			unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
			unsigned char bmppad[3] = {0,0,0};

			bmpfileheader[ 2] = (unsigned char)(filesize    );
			bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
			bmpfileheader[ 4] = (unsigned char)(filesize>>16);
			bmpfileheader[ 5] = (unsigned char)(filesize>>24);

			bmpinfoheader[ 4] = (unsigned char)(       Width    );
			bmpinfoheader[ 5] = (unsigned char)(       Width>> 8);
			bmpinfoheader[ 6] = (unsigned char)(       Width>>16);
			bmpinfoheader[ 7] = (unsigned char)(       Width>>24);
			bmpinfoheader[ 8] = (unsigned char)(       Height    );
			bmpinfoheader[ 9] = (unsigned char)(       Height>> 8);
			bmpinfoheader[10] = (unsigned char)(       Height>>16);
			bmpinfoheader[11] = (unsigned char)(       Height>>24);

			fopen_s(&f, fileName.ToMultiByteString(), "wb");
			if (f)
			{
				fwrite(bmpfileheader,1,14,f);
				fwrite(bmpinfoheader,1,40,f);
				for(int i=0; i<Height; i++)
				{
					fwrite(img.Buffer()+(Width*i*3),3,Width,f);
					fwrite(bmppad,1,(4-(Width*3)%4)%4,f);
				}
				fclose(f);
			}
			else
			{
				throw IO::IOException(L"Failed to open file for writing the bitmap.");
			}
		}

		void ImageRef::SaveAsPngFile(Basic::String fileName, bool reverseY)
		{
			Basic::List<unsigned char> img;
			img.SetSize(4 * Width*Height);
			memset(img.Buffer(), 0, 4 * Width*Height);
			for (int j = 0; j<Height; j++)
			{
				VectorMath::Vec4 * scanLine = Pixels + j*Width;
				for (int i = 0; i<Width; i++)
				{
					int x = i;
					int y = reverseY ? (Height - 1) - j : j;

					int r = (int)(scanLine[i].x * 255);
					int g = (int)(scanLine[i].y * 255);
					int b = (int)(scanLine[i].z * 255);
					int a = (int)(scanLine[i].w * 255);
					if (r > 255) r = 255;
					if (g > 255) g = 255;
					if (b > 255) b = 255;
					if (a > 255) a = 255;
					if (r < 0) r = 0;
					if (g < 0) g = 0;
					if (b < 0) b = 0;
					if (a < 0) a = 0;
					img[(x + y*Width) * 4 + 3] = (unsigned char)(a);
					img[(x + y*Width) * 4 + 2] = (unsigned char)(b);
					img[(x + y*Width) * 4 + 1] = (unsigned char)(g);
					img[(x + y*Width) * 4 + 0] = (unsigned char)(r);
				}
			}
			int error = lodepng::encode(fileName.ToMultiByteString(), img.Buffer(), Width, Height, LCT_RGBA);
			if (error)
			{
				throw IO::IOException(L"Failed to open file for writing the bitmap.");
			}
		}

		void ImageRef::SaveAsPfmFile(Basic::String fileName, bool reverseY)
		{
			Basic::List<float> pixels;
			pixels.SetSize(Width*Height*3);
			if (reverseY)
			{
				for (int i=0; i<Height; i++)
					for (int j = 0; j<Width; j++)
					{
						pixels[(i*Width+j)*3] = Pixels[(Height-i-1)*Width+j].x;
						pixels[(i*Width+j)*3+1] = Pixels[(Height-i-1)*Width+j].y;
						pixels[(i*Width+j)*3+2] = Pixels[(Height-i-1)*Width+j].z;
					}
			}
			else
			{
				for (int i=0; i<Width*Height; i++)
				{
					pixels[i*3] = Pixels[i].x;
					pixels[i*3+1] = Pixels[i].y;
					pixels[i*3+2] = Pixels[i].z;
				}
			}
			IO::FileStream stream(fileName, IO::FileMode::Create);
			stream.Write("PF\n", 3);
			Basic::String s(Width);
			stream.Write(s.ToMultiByteString(), s.Length());
			stream.Write(" ", 1);
			s = Basic::String(Height);
			stream.Write(s.ToMultiByteString(), s.Length());
			stream.Write("\n-1.000000\n", 11);
			for (int h = Height-1; h>=0; h--)
			{
				stream.Write(&pixels[h*Width*3], Width*3*sizeof(float));
			}
		}

		void WriteBitmask(int * bits, int width, int height, Basic::String fileName)
		{
			List<VectorMath::Vec4> imageData;
			imageData.SetSize(width*height);
			ImageRef img(width, height, (float*)imageData.Buffer());
			for (int j = 0; j<height; j++)
			{
				for (int i = 0; i<width; i++)
				{
					int id = i+j*width;
					if ((bits[id>>5] & (1<<(id&31))) != 0)
					{
						imageData[id] = VectorMath::Vec4::Create(1.0f, 1.0f, 1.0f, 1.0f);
					}
					else
					{
						imageData[id] = VectorMath::Vec4::Create(0.0f, 0.0f, 0.0f, 1.0f);
					}
				}
			}
			img.SaveAsBmpFile(fileName);
		}
		BitmapF CombineImageFromChannels(Bitmap * r, Bitmap * g, Bitmap * b, Bitmap * a)
		{
			int w = 0; 
			int h = 0;
			if (r)
			{
				w = r->GetWidth();
				h = r->GetHeight();
			}
			else
				throw ArgumentException(L"r channel cannot be null");
			if (g)
			{
				if (g->GetWidth() != w || g->GetHeight() != h)
					throw ArgumentException(L"g - dimension mismatch.");
			}
			if (b)
			{
				if (b->GetWidth() != w || b->GetHeight() != h)
					throw ArgumentException(L"b - dimension mismatch.");
			}
			if (a)
			{
				if (a->GetWidth() != w || a->GetHeight() != h)
					throw ArgumentException(L"b - dimension mismatch.");
			}
			BitmapF rs = BitmapF(w, h);
			auto pix = rs.GetPixels();
			for (int i = 0; i < w*h; i++)
			{
				pix[i].x = r->GetPixels()[i * 4] / 255.0f;
				if (g)
					pix[i].y = g->GetPixels()[i * 4] / 255.0f;
				else
					pix[i].y = 0;
				if (b)
					pix[i].z = b->GetPixels()[i * 4] / 255.0f;
				else
					pix[i].z = 0;
				if (a)
					pix[i].w = a->GetPixels()[i * 4] / 255.0f;
				else
					pix[i].w = 0;
			}
			return rs;
		}
	}
}