#ifndef DIB_SECTION_H
#define DIB_SECTION_H

#include <Windows.h>
#include <ppl.h>
#include "CoreLib/VectorMath.h"

using namespace VectorMath;

namespace CoreLib
{
	namespace WinForm
	{
		class DIBSection
		{
		private:
			HDC hdc;
			HBITMAP handle;
			int width, height;
			List<BYTE*> scanLine;
		public:
			DIBSection(int width, int height)
			{
				this->width = width;
				this->height = height;
				hdc = CreateCompatibleDC(NULL);
				BITMAPINFO* Info=(BITMAPINFO*)malloc(sizeof(BITMAPINFOHEADER)+2*sizeof(RGBQUAD));
				Info->bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
				Info->bmiHeader.biWidth=(int)width;
				Info->bmiHeader.biHeight=(int)height;
				Info->bmiHeader.biPlanes=1;
				Info->bmiHeader.biBitCount=32;
				Info->bmiHeader.biCompression=BI_RGB;
				Info->bmiHeader.biSizeImage=0;
				Info->bmiHeader.biXPelsPerMeter=0;
				Info->bmiHeader.biYPelsPerMeter=0;
				Info->bmiHeader.biClrUsed=0;
				Info->bmiHeader.biClrImportant=0;
				Info->bmiColors[0].rgbBlue=0;
				Info->bmiColors[0].rgbGreen=0;
				Info->bmiColors[0].rgbRed=0;
				Info->bmiColors[0].rgbReserved=0;
				Info->bmiColors[1].rgbBlue=255;
				Info->bmiColors[1].rgbGreen=255;
				Info->bmiColors[1].rgbRed=255;
				Info->bmiColors[1].rgbReserved=255;
				BYTE* FirstLine=0;
				handle = CreateDIBSection(hdc, Info, DIB_RGB_COLORS, (void**)&FirstLine, NULL, 0);
				SelectObject(hdc, handle);
				scanLine.SetSize(height);
				const int Bits=32;
				int LineBits=width*Bits;
				int AlignBits=sizeof(DWORD)*8;
				LineBits+=(AlignBits-LineBits%AlignBits)%AlignBits;
				int LineBytes=LineBits/8;
				for(int i=0;i<height;i++)
				{
					scanLine[i]=FirstLine+LineBytes*i;
				}
				free(Info);
			}

			void Draw(HDC dc, int x, int y)
			{
				BOOL rs = BitBlt(dc, x, y, width, height, hdc, 0, 0, SRCCOPY);
			}

			inline void SetPixel(int x, int y, Vec4 & color)
			{
				int r = Math::Clamp((int)(color.x*255), 0, 255);
				int g = Math::Clamp((int)(color.y*255), 0, 255);
				int b = Math::Clamp((int)(color.z*255), 0, 255);
				int a = 255;
				scanLine[y][x*4] = b;
				scanLine[y][x*4+1] = g;
				scanLine[y][x*4+2] = r;
				scanLine[y][x*4+3] = a;
			}

			void Clear()
			{
				concurrency::parallel_for(0, height, [&](int y)
				{
					for (int x = 0; x<width; x++)
					{
						((int*)(scanLine[y]))[x] = 0;
					}
				});
			}

			void SetBuffer(Vec4* colors)
			{
				concurrency::parallel_for(0, height, [&](int y)
				{
					for (int x = 0; x<width; x++)
					{
						SetPixel(x, y, colors[y*width+x]);
					}
				});
			}
			
			~DIBSection()
			{
				DeleteObject(handle);
				DeleteDC(hdc);
			}
		};
	}
}

#endif