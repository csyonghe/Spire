#ifndef CORE_LIB_BITMAP_H
#define CORE_LIB_BITMAP_H

#include "../Basic.h"
#include "../VectorMath.h"

namespace CoreLib
{
	namespace Imaging
	{
		class ImageRef
		{
		public:
			VectorMath::Vec4 * Pixels;
			int Width, Height;
			ImageRef()
			{
				Width = Height = 0;
				Pixels = 0;
			}
			ImageRef(int width, int height, float * rgbData)
			{
				Width = width;
				Height = height;
				Pixels = (VectorMath::Vec4*)rgbData;
			}
			void SaveAsBmpFile(Basic::String fileName, bool reverseY = false);
			void SaveAsPngFile(Basic::String fileName, bool reverseY = false);
			void SaveAsPfmFile(Basic::String fileName, bool reverseY = false);
		};

		class BitmapF
		{
		private:
			VectorMath::Vec4 * pixels;
			int width;
			int height;
		public:
			inline VectorMath::Vec4 * GetPixels()
			{
				return pixels;
			}
			inline int GetWidth()
			{
				return width;
			}
			inline int GetHeight()
			{
				return height;
			}
			BitmapF()
			{
				width = 0;
				height = 0;
			}
			BitmapF(int width, int height)
			{
				this->width = width;
				this->height = height;
				pixels = (VectorMath::Vec4 *)malloc(width * height * sizeof(VectorMath::Vec4));
			}
			~BitmapF()
			{
				free(pixels);
			}

			BitmapF(Basic::String fileName);

			inline ImageRef GetImageRef()
			{
				ImageRef rs;
				rs.Pixels = pixels;
				rs.Width = width;
				rs.Height = height;
				return rs;
			}
		};

		class Bitmap
		{
		private:
			unsigned char * pixels;
			int width;
			int height;
			bool isTransparent;
		public:
			inline unsigned char * GetPixels() const
			{
				return pixels;
			}
			inline int GetWidth() const
			{
				return width;
			}
			inline int GetHeight() const
			{
				return height;
			}
			inline bool GetIsTransparent() const
			{
				return isTransparent;
			}
			Bitmap()
			{
				width = 0;
				height = 0;
				isTransparent = false;
			}
			Bitmap(int width, int height)
			{
				this->width = width;
				this->height = height;
				pixels = (unsigned char*)malloc(width * height * 4);
				isTransparent = false;
			}
			~Bitmap()
			{
				if (pixels)
					free(pixels);
			}

			Bitmap(Basic::String fileName);
		};

		void WriteBitmask(int * bits, int width, int height, Basic::String fileName);
		BitmapF CombineImageFromChannels(Bitmap * r, Bitmap * g, Bitmap * b, Bitmap * a);
	}
}

#endif