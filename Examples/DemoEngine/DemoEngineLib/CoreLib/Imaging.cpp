/***********************************************************************

CoreLib - The MIT License (MIT)
Copyright (c) 2016, Yong He

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.

***********************************************************************/

/***********************************************************************
WARNING: This is an automatically generated file.
***********************************************************************/
#include "Imaging.h"

/***********************************************************************
BITMAP.CPP
***********************************************************************/


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

/***********************************************************************
LODEPNG.CPP
***********************************************************************/
/*
LodePNG version 20131115

Copyright (c) 2005-2013 Lode Vandevenne

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution.
*/

/*
The manual and changelog are in the header file "lodepng.h"
Rename this file to lodepng.cpp to use it for C++, or to lodepng.c to use it for C.
*/



#ifdef LODEPNG_COMPILE_CPP
#include <fstream>
#endif /*LODEPNG_COMPILE_CPP*/

#define VERSION_STRING "20131115"

/*
This source file is built up in the following large parts. The code sections
with the "LODEPNG_COMPILE_" #defines divide this up further in an intermixed way.
-Tools for C and common code for PNG and Zlib
-C Code for Zlib (huffman, deflate, ...)
-C Code for PNG (file format chunks, adam7, PNG filters, color conversions, ...)
-The C++ wrapper around all of the above
*/

/*The malloc, realloc and free functions defined here with "lodepng_" in front
of the name, so that you can easily change them to others related to your
platform if needed. Everything else in the code calls these. Pass
-DLODEPNG_NO_COMPILE_ALLOCATORS to the compiler, or comment out
#define LODEPNG_COMPILE_ALLOCATORS in the header, to disable the ones here and
define them in your own project's source files without needing to change
lodepng source code. Don't forget to remove "static" if you copypaste them
from here.*/
#ifdef LODEPNG_COMPILE_ALLOCATORS
static void* lodepng_malloc(size_t size)
{
  return malloc(size);
}

static void* lodepng_realloc(void* ptr, size_t new_size)
{
  return realloc(ptr, new_size);
}

static void lodepng_free(void* ptr)
{
  free(ptr);
}
#else /*LODEPNG_COMPILE_ALLOCATORS*/
void* lodepng_malloc(size_t size);
void* lodepng_realloc(void* ptr, size_t new_size);
void lodepng_free(void* ptr);
#endif /*LODEPNG_COMPILE_ALLOCATORS*/

/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */
/* // Tools for C, and common code for PNG and Zlib.                       // */
/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */

/*
Often in case of an error a value is assigned to a variable and then it breaks
out of a loop (to go to the cleanup phase of a function). This macro does that.
It makes the error handling code shorter and more readable.

Example: if(!uivector_resizev(&frequencies_ll, 286, 0)) ERROR_BREAK(83);
*/
#define CERROR_BREAK(errorvar, code)\
{\
  errorvar = code;\
  break;\
}

/*version of CERROR_BREAK that assumes the common case where the error variable is named "error"*/
#define ERROR_BREAK(code) CERROR_BREAK(error, code)

/*Set error var to the error code, and return it.*/
#define CERROR_RETURN_ERROR(errorvar, code)\
{\
  errorvar = code;\
  return code;\
}

/*Try the code, if it returns error, also return the error.*/
#define CERROR_TRY_RETURN(call)\
{\
  unsigned error = call;\
  if(error) return error;\
}

/*
About uivector, ucvector and string:
-All of them wrap dynamic arrays or text strings in a similar way.
-LodePNG was originally written in C++. The vectors replace the std::vectors that were used in the C++ version.
-The string tools are made to avoid problems with compilers that declare things like strncat as deprecated.
-They're not used in the interface, only internally in this file as static functions.
-As with many other structs in this file, the init and cleanup functions serve as ctor and dtor.
*/

#ifdef LODEPNG_COMPILE_ZLIB
/*dynamic vector of unsigned ints*/
typedef struct uivector
{
  unsigned* data;
  size_t size; /*size in number of unsigned longs*/
  size_t allocsize; /*allocated size in bytes*/
} uivector;

static void uivector_cleanup(void* p)
{
  ((uivector*)p)->size = ((uivector*)p)->allocsize = 0;
  lodepng_free(((uivector*)p)->data);
  ((uivector*)p)->data = NULL;
}

/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned uivector_resize(uivector* p, size_t size)
{
  if(size * sizeof(unsigned) > p->allocsize)
  {
    size_t newsize = size * sizeof(unsigned) * 2;
    void* data = lodepng_realloc(p->data, newsize);
    if(data)
    {
      p->allocsize = newsize;
      p->data = (unsigned*)data;
      p->size = size;
    }
    else return 0;
  }
  else p->size = size;
  return 1;
}

/*resize and give all new elements the value*/
static unsigned uivector_resizev(uivector* p, size_t size, unsigned value)
{
  size_t oldsize = p->size, i;
  if(!uivector_resize(p, size)) return 0;
  for(i = oldsize; i < size; i++) p->data[i] = value;
  return 1;
}

static void uivector_init(uivector* p)
{
  p->data = NULL;
  p->size = p->allocsize = 0;
}

#ifdef LODEPNG_COMPILE_ENCODER
/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned uivector_push_back(uivector* p, unsigned c)
{
  if(!uivector_resize(p, p->size + 1)) return 0;
  p->data[p->size - 1] = c;
  return 1;
}

/*copy q to p, returns 1 if success, 0 if failure ==> nothing done*/
static unsigned uivector_copy(uivector* p, const uivector* q)
{
  size_t i;
  if(!uivector_resize(p, q->size)) return 0;
  for(i = 0; i < q->size; i++) p->data[i] = q->data[i];
  return 1;
}

static void uivector_swap(uivector* p, uivector* q)
{
  size_t tmp;
  unsigned* tmpp;
  tmp = p->size; p->size = q->size; q->size = tmp;
  tmp = p->allocsize; p->allocsize = q->allocsize; q->allocsize = tmp;
  tmpp = p->data; p->data = q->data; q->data = tmpp;
}
#endif /*LODEPNG_COMPILE_ENCODER*/
#endif /*LODEPNG_COMPILE_ZLIB*/

/* /////////////////////////////////////////////////////////////////////////// */

/*dynamic vector of unsigned chars*/
typedef struct ucvector
{
  unsigned char* data;
  size_t size; /*used size*/
  size_t allocsize; /*allocated size*/
} ucvector;

/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned ucvector_resize(ucvector* p, size_t size)
{
  if(size * sizeof(unsigned char) > p->allocsize)
  {
    size_t newsize = size * sizeof(unsigned char) * 2;
    void* data = lodepng_realloc(p->data, newsize);
    if(data)
    {
      p->allocsize = newsize;
      p->data = (unsigned char*)data;
      p->size = size;
    }
    else return 0; /*error: not enough memory*/
  }
  else p->size = size;
  return 1;
}

#ifdef LODEPNG_COMPILE_PNG

static void ucvector_cleanup(void* p)
{
  ((ucvector*)p)->size = ((ucvector*)p)->allocsize = 0;
  lodepng_free(((ucvector*)p)->data);
  ((ucvector*)p)->data = NULL;
}

static void ucvector_init(ucvector* p)
{
  p->data = NULL;
  p->size = p->allocsize = 0;
}

#ifdef LODEPNG_COMPILE_DECODER
/*resize and give all new elements the value*/
static unsigned ucvector_resizev(ucvector* p, size_t size, unsigned char value)
{
  size_t oldsize = p->size, i;
  if(!ucvector_resize(p, size)) return 0;
  for(i = oldsize; i < size; i++) p->data[i] = value;
  return 1;
}
#endif /*LODEPNG_COMPILE_DECODER*/
#endif /*LODEPNG_COMPILE_PNG*/

#ifdef LODEPNG_COMPILE_ZLIB
/*you can both convert from vector to buffer&size and vica versa. If you use
init_buffer to take over a buffer and size, it is not needed to use cleanup*/
static void ucvector_init_buffer(ucvector* p, unsigned char* buffer, size_t size)
{
  p->data = buffer;
  p->allocsize = p->size = size;
}
#endif /*LODEPNG_COMPILE_ZLIB*/

#if (defined(LODEPNG_COMPILE_PNG) && defined(LODEPNG_COMPILE_ANCILLARY_CHUNKS)) || defined(LODEPNG_COMPILE_ENCODER)
/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned ucvector_push_back(ucvector* p, unsigned char c)
{
  if(!ucvector_resize(p, p->size + 1)) return 0;
  p->data[p->size - 1] = c;
  return 1;
}
#endif /*defined(LODEPNG_COMPILE_PNG) || defined(LODEPNG_COMPILE_ENCODER)*/


/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_PNG
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
/*returns 1 if success, 0 if failure ==> nothing done*/
static unsigned string_resize(char** out, size_t size)
{
  char* data = (char*)lodepng_realloc(*out, size + 1);
  if(data)
  {
    data[size] = 0; /*null termination char*/
    *out = data;
  }
  return data != 0;
}

/*init a {char*, size_t} pair for use as string*/
static void string_init(char** out)
{
  *out = NULL;
  string_resize(out, 0);
}

/*free the above pair again*/
static void string_cleanup(char** out)
{
  lodepng_free(*out);
  *out = NULL;
}

static void string_set(char** out, const char* in)
{
  size_t insize = strlen(in), i = 0;
  if(string_resize(out, insize))
  {
    for(i = 0; i < insize; i++)
    {
      (*out)[i] = in[i];
    }
  }
}
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
#endif /*LODEPNG_COMPILE_PNG*/

/* ////////////////////////////////////////////////////////////////////////// */

unsigned lodepng_read32bitInt(const unsigned char* buffer)
{
  return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}

#if defined(LODEPNG_COMPILE_PNG) || defined(LODEPNG_COMPILE_ENCODER)
/*buffer must have at least 4 allocated bytes available*/
static void lodepng_set32bitInt(unsigned char* buffer, unsigned value)
{
  buffer[0] = (unsigned char)((value >> 24) & 0xff);
  buffer[1] = (unsigned char)((value >> 16) & 0xff);
  buffer[2] = (unsigned char)((value >>  8) & 0xff);
  buffer[3] = (unsigned char)((value      ) & 0xff);
}
#endif /*defined(LODEPNG_COMPILE_PNG) || defined(LODEPNG_COMPILE_ENCODER)*/

#ifdef LODEPNG_COMPILE_ENCODER
static void lodepng_add32bitInt(ucvector* buffer, unsigned value)
{
  ucvector_resize(buffer, buffer->size + 4); /*todo: give error if resize failed*/
  lodepng_set32bitInt(&buffer->data[buffer->size - 4], value);
}
#endif /*LODEPNG_COMPILE_ENCODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* / File IO                                                                / */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_DISK

unsigned lodepng_load_file(unsigned char** out, size_t* outsize, const char* filename)
{
  FILE* file;
  long size;

  /*provide some proper output values if error will happen*/
  *out = 0;
  *outsize = 0;

  fopen_s(&file, filename, "rb");
  if(!file) return 78;

  /*get filesize:*/
  fseek(file , 0 , SEEK_END);
  size = ftell(file);
  rewind(file);

  /*read contents of the file into the vector*/
  *outsize = 0;
  *out = (unsigned char*)lodepng_malloc((size_t)size);
  if(size && (*out)) (*outsize) = fread(*out, 1, (size_t)size, file);

  fclose(file);
  if(!(*out) && size) return 83; /*the above malloc failed*/
  return 0;
}

/*write given buffer to the file, overwriting the file, it doesn't append to it.*/
unsigned lodepng_save_file(const unsigned char* buffer, size_t buffersize, const char* filename)
{
  FILE* file;
  fopen_s(&file, filename, "wb" );
  if(!file) return 79;
  fwrite((char*)buffer , 1 , buffersize, file);
  fclose(file);
  return 0;
}

#endif /*LODEPNG_COMPILE_DISK*/

/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */
/* // End of common code and tools. Begin of Zlib related code.            // */
/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_ZLIB
#ifdef LODEPNG_COMPILE_ENCODER
/*TODO: this ignores potential out of memory errors*/
static void addBitToStream(size_t* bitpointer, ucvector* bitstream, unsigned char bit)
{
  /*add a new byte at the end*/
  if((*bitpointer) % 8 == 0) ucvector_push_back(bitstream, (unsigned char)0);
  /*earlier bit of huffman code is in a lesser significant bit of an earlier byte*/
  (bitstream->data[bitstream->size - 1]) |= (bit << ((*bitpointer) & 0x7));
  (*bitpointer)++;
}

static void addBitsToStream(size_t* bitpointer, ucvector* bitstream, unsigned value, size_t nbits)
{
  size_t i;
  for(i = 0; i < nbits; i++) addBitToStream(bitpointer, bitstream, (unsigned char)((value >> i) & 1));
}

static void addBitsToStreamReversed(size_t* bitpointer, ucvector* bitstream, unsigned value, size_t nbits)
{
  size_t i;
  for(i = 0; i < nbits; i++) addBitToStream(bitpointer, bitstream, (unsigned char)((value >> (nbits - 1 - i)) & 1));
}
#endif /*LODEPNG_COMPILE_ENCODER*/

#ifdef LODEPNG_COMPILE_DECODER

#define READBIT(bitpointer, bitstream) ((bitstream[bitpointer >> 3] >> (bitpointer & 0x7)) & (unsigned char)1)

static unsigned char readBitFromStream(size_t* bitpointer, const unsigned char* bitstream)
{
  unsigned char result = (unsigned char)(READBIT(*bitpointer, bitstream));
  (*bitpointer)++;
  return result;
}

static unsigned readBitsFromStream(size_t* bitpointer, const unsigned char* bitstream, size_t nbits)
{
  unsigned result = 0, i;
  for(i = 0; i < nbits; i++)
  {
    result += ((unsigned)READBIT(*bitpointer, bitstream)) << i;
    (*bitpointer)++;
  }
  return result;
}
#endif /*LODEPNG_COMPILE_DECODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* / Deflate - Huffman                                                      / */
/* ////////////////////////////////////////////////////////////////////////// */

#define FIRST_LENGTH_CODE_INDEX 257
#define LAST_LENGTH_CODE_INDEX 285
/*256 literals, the end code, some length codes, and 2 unused codes*/
#define NUM_DEFLATE_CODE_SYMBOLS 288
/*the distance codes have their own symbols, 30 used, 2 unused*/
#define NUM_DISTANCE_SYMBOLS 32
/*the code length codes. 0-15: code lengths, 16: copy previous 3-6 times, 17: 3-10 zeros, 18: 11-138 zeros*/
#define NUM_CODE_LENGTH_CODES 19

/*the base lengths represented by codes 257-285*/
static const unsigned LENGTHBASE[29]
  = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59,
     67, 83, 99, 115, 131, 163, 195, 227, 258};

/*the extra bits used by codes 257-285 (added to base length)*/
static const unsigned LENGTHEXTRA[29]
  = {0, 0, 0, 0, 0, 0, 0,  0,  1,  1,  1,  1,  2,  2,  2,  2,  3,  3,  3,  3,
      4,  4,  4,   4,   5,   5,   5,   5,   0};

/*the base backwards distances (the bits of distance codes appear after length codes and use their own huffman tree)*/
static const unsigned DISTANCEBASE[30]
  = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513,
     769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

/*the extra bits of backwards distances (added to base)*/
static const unsigned DISTANCEEXTRA[30]
  = {0, 0, 0, 0, 1, 1, 2,  2,  3,  3,  4,  4,  5,  5,   6,   6,   7,   7,   8,
       8,    9,    9,   10,   10,   11,   11,   12,    12,    13,    13};

/*the order in which "code length alphabet code lengths" are stored, out of this
the huffman tree of the dynamic huffman tree lengths is generated*/
static const unsigned CLCL_ORDER[NUM_CODE_LENGTH_CODES]
  = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

/* ////////////////////////////////////////////////////////////////////////// */

/*
Huffman tree struct, containing multiple representations of the tree
*/
typedef struct HuffmanTree
{
  unsigned* tree2d;
  unsigned* tree1d;
  unsigned* lengths; /*the lengths of the codes of the 1d-tree*/
  unsigned maxbitlen; /*maximum number of bits a single code can get*/
  unsigned numcodes; /*number of symbols in the alphabet = number of codes*/
} HuffmanTree;

/*function used for debug purposes to draw the tree in ascii art with C++*/
/*
static void HuffmanTree_draw(HuffmanTree* tree)
{
  std::cout << "tree. length: " << tree->numcodes << " maxbitlen: " << tree->maxbitlen << std::endl;
  for(size_t i = 0; i < tree->tree1d.size; i++)
  {
    if(tree->lengths.data[i])
      std::cout << i << " " << tree->tree1d.data[i] << " " << tree->lengths.data[i] << std::endl;
  }
  std::cout << std::endl;
}*/

static void HuffmanTree_init(HuffmanTree* tree)
{
  tree->tree2d = 0;
  tree->tree1d = 0;
  tree->lengths = 0;
}

static void HuffmanTree_cleanup(HuffmanTree* tree)
{
  lodepng_free(tree->tree2d);
  lodepng_free(tree->tree1d);
  lodepng_free(tree->lengths);
}

/*the tree representation used by the decoder. return value is error*/
static unsigned HuffmanTree_make2DTree(HuffmanTree* tree)
{
  unsigned nodefilled = 0; /*up to which node it is filled*/
  unsigned treepos = 0; /*position in the tree (1 of the numcodes columns)*/
  unsigned n, i;

  tree->tree2d = (unsigned*)lodepng_malloc(tree->numcodes * 2 * sizeof(unsigned));
  if(!tree->tree2d) return 83; /*alloc fail*/

  /*
  convert tree1d[] to tree2d[][]. In the 2D array, a value of 32767 means
  uninited, a value >= numcodes is an address to another bit, a value < numcodes
  is a code. The 2 rows are the 2 possible bit values (0 or 1), there are as
  many columns as codes - 1.
  A good huffmann tree has N * 2 - 1 nodes, of which N - 1 are internal nodes.
  Here, the internal nodes are stored (what their 0 and 1 option point to).
  There is only memory for such good tree currently, if there are more nodes
  (due to too long length codes), error 55 will happen
  */
  for(n = 0; n < tree->numcodes * 2; n++)
  {
    tree->tree2d[n] = 32767; /*32767 here means the tree2d isn't filled there yet*/
  }

  for(n = 0; n < tree->numcodes; n++) /*the codes*/
  {
    for(i = 0; i < tree->lengths[n]; i++) /*the bits for this code*/
    {
      unsigned char bit = (unsigned char)((tree->tree1d[n] >> (tree->lengths[n] - i - 1)) & 1);
      if(treepos > tree->numcodes - 2) return 55; /*oversubscribed, see comment in lodepng_error_text*/
      if(tree->tree2d[2 * treepos + bit] == 32767) /*not yet filled in*/
      {
        if(i + 1 == tree->lengths[n]) /*last bit*/
        {
          tree->tree2d[2 * treepos + bit] = n; /*put the current code in it*/
          treepos = 0;
        }
        else
        {
          /*put address of the next step in here, first that address has to be found of course
          (it's just nodefilled + 1)...*/
          nodefilled++;
          /*addresses encoded with numcodes added to it*/
          tree->tree2d[2 * treepos + bit] = nodefilled + tree->numcodes;
          treepos = nodefilled;
        }
      }
      else treepos = tree->tree2d[2 * treepos + bit] - tree->numcodes;
    }
  }

  for(n = 0; n < tree->numcodes * 2; n++)
  {
    if(tree->tree2d[n] == 32767) tree->tree2d[n] = 0; /*remove possible remaining 32767's*/
  }

  return 0;
}

/*
Second step for the ...makeFromLengths and ...makeFromFrequencies functions.
numcodes, lengths and maxbitlen must already be filled in correctly. return
value is error.
*/
static unsigned HuffmanTree_makeFromLengths2(HuffmanTree* tree)
{
  uivector blcount;
  uivector nextcode;
  unsigned bits, n, error = 0;

  uivector_init(&blcount);
  uivector_init(&nextcode);

  tree->tree1d = (unsigned*)lodepng_malloc(tree->numcodes * sizeof(unsigned));
  if(!tree->tree1d) error = 83; /*alloc fail*/

  if(!uivector_resizev(&blcount, tree->maxbitlen + 1, 0)
  || !uivector_resizev(&nextcode, tree->maxbitlen + 1, 0))
    error = 83; /*alloc fail*/

  if(!error)
  {
    /*step 1: count number of instances of each code length*/
    for(bits = 0; bits < tree->numcodes; bits++) blcount.data[tree->lengths[bits]]++;
    /*step 2: generate the nextcode values*/
    for(bits = 1; bits <= tree->maxbitlen; bits++)
    {
      nextcode.data[bits] = (nextcode.data[bits - 1] + blcount.data[bits - 1]) << 1;
    }
    /*step 3: generate all the codes*/
    for(n = 0; n < tree->numcodes; n++)
    {
      if(tree->lengths[n] != 0) tree->tree1d[n] = nextcode.data[tree->lengths[n]]++;
    }
  }

  uivector_cleanup(&blcount);
  uivector_cleanup(&nextcode);

  if(!error) return HuffmanTree_make2DTree(tree);
  else return error;
}

/*
given the code lengths (as stored in the PNG file), generate the tree as defined
by Deflate. maxbitlen is the maximum bits that a code in the tree can have.
return value is error.
*/
static unsigned HuffmanTree_makeFromLengths(HuffmanTree* tree, const unsigned* bitlen,
                                            size_t numcodes, unsigned maxbitlen)
{
  unsigned i;
  tree->lengths = (unsigned*)lodepng_malloc(numcodes * sizeof(unsigned));
  if(!tree->lengths) return 83; /*alloc fail*/
  for(i = 0; i < numcodes; i++) tree->lengths[i] = bitlen[i];
  tree->numcodes = (unsigned)numcodes; /*number of symbols*/
  tree->maxbitlen = maxbitlen;
  return HuffmanTree_makeFromLengths2(tree);
}

#ifdef LODEPNG_COMPILE_ENCODER

/*
A coin, this is the terminology used for the package-merge algorithm and the
coin collector's problem. This is used to generate the huffman tree.
A coin can be multiple coins (when they're merged)
*/
typedef struct Coin
{
  uivector symbols;
  float weight; /*the sum of all weights in this coin*/
} Coin;

static void coin_init(Coin* c)
{
  uivector_init(&c->symbols);
}

/*argument c is void* so that this dtor can be given as function pointer to the vector resize function*/
static void coin_cleanup(void* c)
{
  uivector_cleanup(&((Coin*)c)->symbols);
}

static void coin_copy(Coin* c1, const Coin* c2)
{
  c1->weight = c2->weight;
  uivector_copy(&c1->symbols, &c2->symbols);
}

static void add_coins(Coin* c1, const Coin* c2)
{
  size_t i;
  for(i = 0; i < c2->symbols.size; i++) uivector_push_back(&c1->symbols, c2->symbols.data[i]);
  c1->weight += c2->weight;
}

static void init_coins(Coin* coins, size_t num)
{
  size_t i;
  for(i = 0; i < num; i++) coin_init(&coins[i]);
}

static void cleanup_coins(Coin* coins, size_t num)
{
  size_t i;
  for(i = 0; i < num; i++) coin_cleanup(&coins[i]);
}

/*
This uses a simple combsort to sort the data. This function is not critical for
overall encoding speed and the data amount isn't that large.
*/
static void sort_coins(Coin* data, size_t amount)
{
  size_t gap = amount;
  unsigned char swapped = 0;
  while((gap > 1) || swapped)
  {
    size_t i;
    gap = (gap * 10) / 13; /*shrink factor 1.3*/
    if(gap == 9 || gap == 10) gap = 11; /*combsort11*/
    if(gap < 1) gap = 1;
    swapped = 0;
    for(i = 0; i < amount - gap; i++)
    {
      size_t j = i + gap;
      if(data[j].weight < data[i].weight)
      {
        float temp = data[j].weight; data[j].weight = data[i].weight; data[i].weight = temp;
        uivector_swap(&data[i].symbols, &data[j].symbols);
        swapped = 1;
      }
    }
  }
}

static unsigned append_symbol_coins(Coin* coins, const unsigned* frequencies, unsigned numcodes, size_t sum)
{
  unsigned i;
  unsigned j = 0; /*index of present symbols*/
  for(i = 0; i < numcodes; i++)
  {
    if(frequencies[i] != 0) /*only include symbols that are present*/
    {
      coins[j].weight = frequencies[i] / (float)sum;
      uivector_push_back(&coins[j].symbols, i);
      j++;
    }
  }
  return 0;
}

unsigned lodepng_huffman_code_lengths(unsigned* lengths, const unsigned* frequencies,
                                      size_t numcodes, unsigned maxbitlen)
{
  unsigned i, j;
  size_t sum = 0, numpresent = 0;
  unsigned error = 0;
  Coin* coins; /*the coins of the currently calculated row*/
  Coin* prev_row; /*the previous row of coins*/
  unsigned numcoins;
  unsigned coinmem;

  if(numcodes == 0) return 80; /*error: a tree of 0 symbols is not supposed to be made*/

  for(i = 0; i < numcodes; i++)
  {
    if(frequencies[i] > 0)
    {
      numpresent++;
      sum += frequencies[i];
    }
  }

  for(i = 0; i < numcodes; i++) lengths[i] = 0;

  /*ensure at least two present symbols. There should be at least one symbol
  according to RFC 1951 section 3.2.7. To decoders incorrectly require two. To
  make these work as well ensure there are at least two symbols. The
  Package-Merge code below also doesn't work correctly if there's only one
  symbol, it'd give it the theoritical 0 bits but in practice zlib wants 1 bit*/
  if(numpresent == 0)
  {
    lengths[0] = lengths[1] = 1; /*note that for RFC 1951 section 3.2.7, only lengths[0] = 1 is needed*/
  }
  else if(numpresent == 1)
  {
    for(i = 0; i < numcodes; i++)
    {
      if(frequencies[i])
      {
        lengths[i] = 1;
        lengths[i == 0 ? 1 : 0] = 1;
        break;
      }
    }
  }
  else
  {
    /*Package-Merge algorithm represented by coin collector's problem
    For every symbol, maxbitlen coins will be created*/

    coinmem = (unsigned int)numpresent * 2; /*max amount of coins needed with the current algo*/
    coins = (Coin*)lodepng_malloc(sizeof(Coin) * coinmem);
    prev_row = (Coin*)lodepng_malloc(sizeof(Coin) * coinmem);
    if(!coins || !prev_row)
    {
      lodepng_free(coins);
      lodepng_free(prev_row);
      return 83; /*alloc fail*/
    }
    init_coins(coins, coinmem);
    init_coins(prev_row, coinmem);

    /*first row, lowest denominator*/
	error = append_symbol_coins(coins, frequencies, (unsigned int)numcodes, sum);
	numcoins = (unsigned int)numpresent;
    sort_coins(coins, numcoins);
    if(!error)
    {
      unsigned numprev = 0;
      for(j = 1; j <= maxbitlen && !error; j++) /*each of the remaining rows*/
      {
        unsigned tempnum;
        Coin* tempcoins;
        /*swap prev_row and coins, and their amounts*/
        tempcoins = prev_row; prev_row = coins; coins = tempcoins;
        tempnum = numprev; numprev = numcoins; numcoins = tempnum;

        cleanup_coins(coins, numcoins);
        init_coins(coins, numcoins);

        numcoins = 0;

        /*fill in the merged coins of the previous row*/
        for(i = 0; i + 1 < numprev; i += 2)
        {
          /*merge prev_row[i] and prev_row[i + 1] into new coin*/
          Coin* coin = &coins[numcoins++];
          coin_copy(coin, &prev_row[i]);
          add_coins(coin, &prev_row[i + 1]);
        }
        /*fill in all the original symbols again*/
        if(j < maxbitlen)
        {
			error = append_symbol_coins(coins + numcoins, frequencies, (unsigned int)numcodes, sum);
			numcoins += (unsigned int)numpresent;
        }
        sort_coins(coins, numcoins);
      }
    }

    if(!error)
    {
      /*calculate the lenghts of each symbol, as the amount of times a coin of each symbol is used*/
      for(i = 0; i < numpresent - 1; i++)
      {
        Coin* coin = &coins[i];
        for(j = 0; j < coin->symbols.size; j++) lengths[coin->symbols.data[j]]++;
      }
    }

    cleanup_coins(coins, coinmem);
    lodepng_free(coins);
    cleanup_coins(prev_row, coinmem);
    lodepng_free(prev_row);
  }

  return error;
}

/*Create the Huffman tree given the symbol frequencies*/
static unsigned HuffmanTree_makeFromFrequencies(HuffmanTree* tree, const unsigned* frequencies,
                                                size_t mincodes, size_t numcodes, unsigned maxbitlen)
{
  unsigned error = 0;
  while(!frequencies[numcodes - 1] && numcodes > mincodes) numcodes--; /*trim zeroes*/
  tree->maxbitlen = maxbitlen;
  tree->numcodes = (unsigned)numcodes; /*number of symbols*/
  tree->lengths = (unsigned*)lodepng_realloc(tree->lengths, numcodes * sizeof(unsigned));
  if(!tree->lengths) return 83; /*alloc fail*/
  /*initialize all lengths to 0*/
  memset(tree->lengths, 0, numcodes * sizeof(unsigned));

  error = lodepng_huffman_code_lengths(tree->lengths, frequencies, numcodes, maxbitlen);
  if(!error) error = HuffmanTree_makeFromLengths2(tree);
  return error;
}

static unsigned HuffmanTree_getCode(const HuffmanTree* tree, unsigned index)
{
  return tree->tree1d[index];
}

static unsigned HuffmanTree_getLength(const HuffmanTree* tree, unsigned index)
{
  return tree->lengths[index];
}
#endif /*LODEPNG_COMPILE_ENCODER*/

/*get the literal and length code tree of a deflated block with fixed tree, as per the deflate specification*/
static unsigned generateFixedLitLenTree(HuffmanTree* tree)
{
  unsigned i, error = 0;
  unsigned* bitlen = (unsigned*)lodepng_malloc(NUM_DEFLATE_CODE_SYMBOLS * sizeof(unsigned));
  if(!bitlen) return 83; /*alloc fail*/

  /*288 possible codes: 0-255=literals, 256=endcode, 257-285=lengthcodes, 286-287=unused*/
  for(i =   0; i <= 143; i++) bitlen[i] = 8;
  for(i = 144; i <= 255; i++) bitlen[i] = 9;
  for(i = 256; i <= 279; i++) bitlen[i] = 7;
  for(i = 280; i <= 287; i++) bitlen[i] = 8;

  error = HuffmanTree_makeFromLengths(tree, bitlen, NUM_DEFLATE_CODE_SYMBOLS, 15);

  lodepng_free(bitlen);
  return error;
}

/*get the distance code tree of a deflated block with fixed tree, as specified in the deflate specification*/
static unsigned generateFixedDistanceTree(HuffmanTree* tree)
{
  unsigned i, error = 0;
  unsigned* bitlen = (unsigned*)lodepng_malloc(NUM_DISTANCE_SYMBOLS * sizeof(unsigned));
  if(!bitlen) return 83; /*alloc fail*/

  /*there are 32 distance codes, but 30-31 are unused*/
  for(i = 0; i < NUM_DISTANCE_SYMBOLS; i++) bitlen[i] = 5;
  error = HuffmanTree_makeFromLengths(tree, bitlen, NUM_DISTANCE_SYMBOLS, 15);

  lodepng_free(bitlen);
  return error;
}

#ifdef LODEPNG_COMPILE_DECODER

/*
returns the code, or (unsigned)(-1) if error happened
inbitlength is the length of the complete buffer, in bits (so its byte length times 8)
*/
static unsigned huffmanDecodeSymbol(const unsigned char* in, size_t* bp,
                                    const HuffmanTree* codetree, size_t inbitlength)
{
  unsigned treepos = 0, ct;
  for(;;)
  {
    if(*bp >= inbitlength) return (unsigned)(-1); /*error: end of input memory reached without endcode*/
    /*
    decode the symbol from the tree. The "readBitFromStream" code is inlined in
    the expression below because this is the biggest bottleneck while decoding
    */
    ct = codetree->tree2d[(treepos << 1) + READBIT(*bp, in)];
    (*bp)++;
    if(ct < codetree->numcodes) return ct; /*the symbol is decoded, return it*/
    else treepos = ct - codetree->numcodes; /*symbol not yet decoded, instead move tree position*/

    if(treepos >= codetree->numcodes) return (unsigned)(-1); /*error: it appeared outside the codetree*/
  }
}
#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_DECODER

/* ////////////////////////////////////////////////////////////////////////// */
/* / Inflator (Decompressor)                                                / */
/* ////////////////////////////////////////////////////////////////////////// */

/*get the tree of a deflated block with fixed tree, as specified in the deflate specification*/
static void getTreeInflateFixed(HuffmanTree* tree_ll, HuffmanTree* tree_d)
{
  /*TODO: check for out of memory errors*/
  generateFixedLitLenTree(tree_ll);
  generateFixedDistanceTree(tree_d);
}

/*get the tree of a deflated block with dynamic tree, the tree itself is also Huffman compressed with a known tree*/
static unsigned getTreeInflateDynamic(HuffmanTree* tree_ll, HuffmanTree* tree_d,
                                      const unsigned char* in, size_t* bp, size_t inlength)
{
  /*make sure that length values that aren't filled in will be 0, or a wrong tree will be generated*/
  unsigned error = 0;
  unsigned n, HLIT, HDIST, HCLEN, i;
  size_t inbitlength = inlength * 8;

  /*see comments in deflateDynamic for explanation of the context and these variables, it is analogous*/
  unsigned* bitlen_ll = 0; /*lit,len code lengths*/
  unsigned* bitlen_d = 0; /*dist code lengths*/
  /*code length code lengths ("clcl"), the bit lengths of the huffman tree used to compress bitlen_ll and bitlen_d*/
  unsigned* bitlen_cl = 0;
  HuffmanTree tree_cl; /*the code tree for code length codes (the huffman tree for compressed huffman trees)*/

  if((*bp) >> 3 >= inlength - 2) return 49; /*error: the bit pointer is or will go past the memory*/

  /*number of literal/length codes + 257. Unlike the spec, the value 257 is added to it here already*/
  HLIT =  readBitsFromStream(bp, in, 5) + 257;
  /*number of distance codes. Unlike the spec, the value 1 is added to it here already*/
  HDIST = readBitsFromStream(bp, in, 5) + 1;
  /*number of code length codes. Unlike the spec, the value 4 is added to it here already*/
  HCLEN = readBitsFromStream(bp, in, 4) + 4;

  HuffmanTree_init(&tree_cl);

  while(!error)
  {
    /*read the code length codes out of 3 * (amount of code length codes) bits*/

    bitlen_cl = (unsigned*)lodepng_malloc(NUM_CODE_LENGTH_CODES * sizeof(unsigned));
    if(!bitlen_cl) ERROR_BREAK(83 /*alloc fail*/);

    for(i = 0; i < NUM_CODE_LENGTH_CODES; i++)
    {
      if(i < HCLEN) bitlen_cl[CLCL_ORDER[i]] = readBitsFromStream(bp, in, 3);
      else bitlen_cl[CLCL_ORDER[i]] = 0; /*if not, it must stay 0*/
    }

    error = HuffmanTree_makeFromLengths(&tree_cl, bitlen_cl, NUM_CODE_LENGTH_CODES, 7);
    if(error) break;

    /*now we can use this tree to read the lengths for the tree that this function will return*/
    bitlen_ll = (unsigned*)lodepng_malloc(NUM_DEFLATE_CODE_SYMBOLS * sizeof(unsigned));
    bitlen_d = (unsigned*)lodepng_malloc(NUM_DISTANCE_SYMBOLS * sizeof(unsigned));
    if(!bitlen_ll || !bitlen_d) ERROR_BREAK(83 /*alloc fail*/);
    for(i = 0; i < NUM_DEFLATE_CODE_SYMBOLS; i++) bitlen_ll[i] = 0;
    for(i = 0; i < NUM_DISTANCE_SYMBOLS; i++) bitlen_d[i] = 0;

    /*i is the current symbol we're reading in the part that contains the code lengths of lit/len and dist codes*/
    i = 0;
    while(i < HLIT + HDIST)
    {
      unsigned code = huffmanDecodeSymbol(in, bp, &tree_cl, inbitlength);
      if(code <= 15) /*a length code*/
      {
        if(i < HLIT) bitlen_ll[i] = code;
        else bitlen_d[i - HLIT] = code;
        i++;
      }
      else if(code == 16) /*repeat previous*/
      {
        unsigned replength = 3; /*read in the 2 bits that indicate repeat length (3-6)*/
        unsigned value; /*set value to the previous code*/

        if(*bp >= inbitlength) ERROR_BREAK(50); /*error, bit pointer jumps past memory*/
        if (i == 0) ERROR_BREAK(54); /*can't repeat previous if i is 0*/

        replength += readBitsFromStream(bp, in, 2);

        if(i < HLIT + 1) value = bitlen_ll[i - 1];
        else value = bitlen_d[i - HLIT - 1];
        /*repeat this value in the next lengths*/
        for(n = 0; n < replength; n++)
        {
          if(i >= HLIT + HDIST) ERROR_BREAK(13); /*error: i is larger than the amount of codes*/
          if(i < HLIT) bitlen_ll[i] = value;
          else bitlen_d[i - HLIT] = value;
          i++;
        }
      }
      else if(code == 17) /*repeat "0" 3-10 times*/
      {
        unsigned replength = 3; /*read in the bits that indicate repeat length*/
        if(*bp >= inbitlength) ERROR_BREAK(50); /*error, bit pointer jumps past memory*/

        replength += readBitsFromStream(bp, in, 3);

        /*repeat this value in the next lengths*/
        for(n = 0; n < replength; n++)
        {
          if(i >= HLIT + HDIST) ERROR_BREAK(14); /*error: i is larger than the amount of codes*/

          if(i < HLIT) bitlen_ll[i] = 0;
          else bitlen_d[i - HLIT] = 0;
          i++;
        }
      }
      else if(code == 18) /*repeat "0" 11-138 times*/
      {
        unsigned replength = 11; /*read in the bits that indicate repeat length*/
        if(*bp >= inbitlength) ERROR_BREAK(50); /*error, bit pointer jumps past memory*/

        replength += readBitsFromStream(bp, in, 7);

        /*repeat this value in the next lengths*/
        for(n = 0; n < replength; n++)
        {
          if(i >= HLIT + HDIST) ERROR_BREAK(15); /*error: i is larger than the amount of codes*/

          if(i < HLIT) bitlen_ll[i] = 0;
          else bitlen_d[i - HLIT] = 0;
          i++;
        }
      }
      else /*if(code == (unsigned)(-1))*/ /*huffmanDecodeSymbol returns (unsigned)(-1) in case of error*/
      {
        if(code == (unsigned)(-1))
        {
          /*return error code 10 or 11 depending on the situation that happened in huffmanDecodeSymbol
          (10=no endcode, 11=wrong jump outside of tree)*/
          error = (*bp) > inbitlength ? 10 : 11;
        }
        else error = 16; /*unexisting code, this can never happen*/
        break;
      }
    }
    if(error) break;

    if(bitlen_ll[256] == 0) ERROR_BREAK(64); /*the length of the end code 256 must be larger than 0*/

    /*now we've finally got HLIT and HDIST, so generate the code trees, and the function is done*/
    error = HuffmanTree_makeFromLengths(tree_ll, bitlen_ll, NUM_DEFLATE_CODE_SYMBOLS, 15);
    if(error) break;
    error = HuffmanTree_makeFromLengths(tree_d, bitlen_d, NUM_DISTANCE_SYMBOLS, 15);

    break; /*end of error-while*/
  }

  lodepng_free(bitlen_cl);
  lodepng_free(bitlen_ll);
  lodepng_free(bitlen_d);
  HuffmanTree_cleanup(&tree_cl);

  return error;
}

/*inflate a block with dynamic of fixed Huffman tree*/
static unsigned inflateHuffmanBlock(ucvector* out, const unsigned char* in, size_t* bp,
                                    size_t* pos, size_t inlength, unsigned btype)
{
  unsigned error = 0;
  HuffmanTree tree_ll; /*the huffman tree for literal and length codes*/
  HuffmanTree tree_d; /*the huffman tree for distance codes*/
  size_t inbitlength = inlength * 8;

  HuffmanTree_init(&tree_ll);
  HuffmanTree_init(&tree_d);

  if(btype == 1) getTreeInflateFixed(&tree_ll, &tree_d);
  else if(btype == 2) error = getTreeInflateDynamic(&tree_ll, &tree_d, in, bp, inlength);

  while(!error) /*decode all symbols until end reached, breaks at end code*/
  {
    /*code_ll is literal, length or end code*/
    unsigned code_ll = huffmanDecodeSymbol(in, bp, &tree_ll, inbitlength);
    if(code_ll <= 255) /*literal symbol*/
    {
      if((*pos) >= out->size)
      {
        /*reserve more room at once*/
        if(!ucvector_resize(out, ((*pos) + 1) * 2)) ERROR_BREAK(83 /*alloc fail*/);
      }
      out->data[(*pos)] = (unsigned char)(code_ll);
      (*pos)++;
    }
    else if(code_ll >= FIRST_LENGTH_CODE_INDEX && code_ll <= LAST_LENGTH_CODE_INDEX) /*length code*/
    {
      unsigned code_d, distance;
      unsigned numextrabits_l, numextrabits_d; /*extra bits for length and distance*/
      size_t start, forward, backward, length;

      /*part 1: get length base*/
      length = LENGTHBASE[code_ll - FIRST_LENGTH_CODE_INDEX];

      /*part 2: get extra bits and add the value of that to length*/
      numextrabits_l = LENGTHEXTRA[code_ll - FIRST_LENGTH_CODE_INDEX];
      if(*bp >= inbitlength) ERROR_BREAK(51); /*error, bit pointer will jump past memory*/
      length += readBitsFromStream(bp, in, numextrabits_l);

      /*part 3: get distance code*/
      code_d = huffmanDecodeSymbol(in, bp, &tree_d, inbitlength);
      if(code_d > 29)
      {
        if(code_ll == (unsigned)(-1)) /*huffmanDecodeSymbol returns (unsigned)(-1) in case of error*/
        {
          /*return error code 10 or 11 depending on the situation that happened in huffmanDecodeSymbol
          (10=no endcode, 11=wrong jump outside of tree)*/
          error = (*bp) > inlength * 8 ? 10 : 11;
        }
        else error = 18; /*error: invalid distance code (30-31 are never used)*/
        break;
      }
      distance = DISTANCEBASE[code_d];

      /*part 4: get extra bits from distance*/
      numextrabits_d = DISTANCEEXTRA[code_d];
      if(*bp >= inbitlength) ERROR_BREAK(51); /*error, bit pointer will jump past memory*/

      distance += readBitsFromStream(bp, in, numextrabits_d);

      /*part 5: fill in all the out[n] values based on the length and dist*/
      start = (*pos);
      if(distance > start) ERROR_BREAK(52); /*too long backward distance*/
      backward = start - distance;
      if((*pos) + length >= out->size)
      {
        /*reserve more room at once*/
        if(!ucvector_resize(out, ((*pos) + length) * 2)) ERROR_BREAK(83 /*alloc fail*/);
      }

      for(forward = 0; forward < length; forward++)
      {
        out->data[(*pos)] = out->data[backward];
        (*pos)++;
        backward++;
        if(backward >= start) backward = start - distance;
      }
    }
    else if(code_ll == 256)
    {
      break; /*end code, break the loop*/
    }
    else /*if(code == (unsigned)(-1))*/ /*huffmanDecodeSymbol returns (unsigned)(-1) in case of error*/
    {
      /*return error code 10 or 11 depending on the situation that happened in huffmanDecodeSymbol
      (10=no endcode, 11=wrong jump outside of tree)*/
      error = (*bp) > inlength * 8 ? 10 : 11;
      break;
    }
  }

  HuffmanTree_cleanup(&tree_ll);
  HuffmanTree_cleanup(&tree_d);

  return error;
}

static unsigned inflateNoCompression(ucvector* out, const unsigned char* in, size_t* bp, size_t* pos, size_t inlength)
{
  /*go to first boundary of byte*/
  size_t p;
  unsigned LEN, NLEN, n, error = 0;
  while(((*bp) & 0x7) != 0) (*bp)++;
  p = (*bp) / 8; /*byte position*/

  /*read LEN (2 bytes) and NLEN (2 bytes)*/
  if(p >= inlength - 4) return 52; /*error, bit pointer will jump past memory*/
  LEN = in[p] + 256 * in[p + 1]; p += 2;
  NLEN = in[p] + 256 * in[p + 1]; p += 2;

  /*check if 16-bit NLEN is really the one's complement of LEN*/
  if(LEN + NLEN != 65535) return 21; /*error: NLEN is not one's complement of LEN*/

  if((*pos) + LEN >= out->size)
  {
    if(!ucvector_resize(out, (*pos) + LEN)) return 83; /*alloc fail*/
  }

  /*read the literal data: LEN bytes are now stored in the out buffer*/
  if(p + LEN > inlength) return 23; /*error: reading outside of in buffer*/
  for(n = 0; n < LEN; n++) out->data[(*pos)++] = in[p++];

  (*bp) = p * 8;

  return error;
}

static unsigned lodepng_inflatev(ucvector* out,
                                 const unsigned char* in, size_t insize,
                                 const LodePNGDecompressSettings* settings)
{
  /*bit pointer in the "in" data, current byte is bp >> 3, current bit is bp & 0x7 (from lsb to msb of the byte)*/
  size_t bp = 0;
  unsigned BFINAL = 0;
  size_t pos = 0; /*byte position in the out buffer*/

  unsigned error = 0;

  (void)settings;

  while(!BFINAL)
  {
    unsigned BTYPE;
    if(bp + 2 >= insize * 8) return 52; /*error, bit pointer will jump past memory*/
    BFINAL = readBitFromStream(&bp, in);
    BTYPE = 1 * readBitFromStream(&bp, in);
    BTYPE += 2 * readBitFromStream(&bp, in);

    if(BTYPE == 3) return 20; /*error: invalid BTYPE*/
    else if(BTYPE == 0) error = inflateNoCompression(out, in, &bp, &pos, insize); /*no compression*/
    else error = inflateHuffmanBlock(out, in, &bp, &pos, insize, BTYPE); /*compression, BTYPE 01 or 10*/

    if(error) return error;
  }

  /*Only now we know the true size of out, resize it to that*/
  if(!ucvector_resize(out, pos)) error = 83; /*alloc fail*/

  return error;
}

unsigned lodepng_inflate(unsigned char** out, size_t* outsize,
                         const unsigned char* in, size_t insize,
                         const LodePNGDecompressSettings* settings)
{
  unsigned error;
  ucvector v;
  ucvector_init_buffer(&v, *out, *outsize);
  error = lodepng_inflatev(&v, in, insize, settings);
  *out = v.data;
  *outsize = v.size;
  return error;
}

static unsigned inflate(unsigned char** out, size_t* outsize,
                        const unsigned char* in, size_t insize,
                        const LodePNGDecompressSettings* settings)
{
  if(settings->custom_inflate)
  {
    return settings->custom_inflate(out, outsize, in, insize, settings);
  }
  else
  {
    return lodepng_inflate(out, outsize, in, insize, settings);
  }
}

#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_ENCODER

/* ////////////////////////////////////////////////////////////////////////// */
/* / Deflator (Compressor)                                                  / */
/* ////////////////////////////////////////////////////////////////////////// */

static const size_t MAX_SUPPORTED_DEFLATE_LENGTH = 258;

/*bitlen is the size in bits of the code*/
static void addHuffmanSymbol(size_t* bp, ucvector* compressed, unsigned code, unsigned bitlen)
{
  addBitsToStreamReversed(bp, compressed, code, bitlen);
}

/*search the index in the array, that has the largest value smaller than or equal to the given value,
given array must be sorted (if no value is smaller, it returns the size of the given array)*/
static size_t searchCodeIndex(const unsigned* array, size_t array_size, size_t value)
{
  /*linear search implementation*/
  /*for(size_t i = 1; i < array_size; i++) if(array[i] > value) return i - 1;
  return array_size - 1;*/

  /*binary search implementation (not that much faster) (precondition: array_size > 0)*/
  size_t left  = 1;
  size_t right = array_size - 1;
  while(left <= right)
  {
    size_t mid = (left + right) / 2;
    if(array[mid] <= value) left = mid + 1; /*the value to find is more to the right*/
    else if(array[mid - 1] > value) right = mid - 1; /*the value to find is more to the left*/
    else return mid - 1;
  }
  return array_size - 1;
}

static void addLengthDistance(uivector* values, size_t length, size_t distance)
{
  /*values in encoded vector are those used by deflate:
  0-255: literal bytes
  256: end
  257-285: length/distance pair (length code, followed by extra length bits, distance code, extra distance bits)
  286-287: invalid*/

  unsigned length_code = (unsigned)searchCodeIndex(LENGTHBASE, 29, length);
  unsigned extra_length = (unsigned)(length - LENGTHBASE[length_code]);
  unsigned dist_code = (unsigned)searchCodeIndex(DISTANCEBASE, 30, distance);
  unsigned extra_distance = (unsigned)(distance - DISTANCEBASE[dist_code]);

  uivector_push_back(values, length_code + FIRST_LENGTH_CODE_INDEX);
  uivector_push_back(values, extra_length);
  uivector_push_back(values, dist_code);
  uivector_push_back(values, extra_distance);
}

static const unsigned HASH_NUM_VALUES = 65536;
static const unsigned HASH_NUM_CHARACTERS = 3;
static const unsigned HASH_SHIFT = 2;
/*
The HASH_NUM_CHARACTERS value is used to make encoding faster by using longer
sequences to generate a hash value from the stream bytes. Setting it to 3
gives exactly the same compression as the brute force method, since deflate's
run length encoding starts with lengths of 3. Setting it to higher values,
like 6, can make the encoding faster (not always though!), but will cause the
encoding to miss any length between 3 and this value, so that the compression
may be worse (but this can vary too depending on the image, sometimes it is
even a bit better instead).
The HASH_NUM_VALUES is the amount of unique possible hash values that
combinations of bytes can give, the higher it is the more memory is needed, but
if it's too low the advantage of hashing is gone.
*/

typedef struct LodePNG_Hash
{
  int* head; /*hash value to head circular pos*/
  int* val; /*circular pos to hash value*/
  /*circular pos to prev circular pos*/
  unsigned short* chain;
  unsigned short* zeros;
} LodePNG_Hash;

static unsigned hash_init(LodePNG_Hash* hash, unsigned windowsize)
{
  unsigned i;
  hash->head = (int*)lodepng_malloc(sizeof(int) * HASH_NUM_VALUES);
  hash->val = (int*)lodepng_malloc(sizeof(int) * windowsize);
  hash->chain = (unsigned short*)lodepng_malloc(sizeof(unsigned short) * windowsize);
  hash->zeros = (unsigned short*)lodepng_malloc(sizeof(unsigned short) * windowsize);

  if(!hash->head || !hash->val || !hash->chain || !hash->zeros) return 83; /*alloc fail*/

  /*initialize hash table*/
  for(i = 0; i < HASH_NUM_VALUES; i++) hash->head[i] = -1;
  for(i = 0; i < windowsize; i++) hash->val[i] = -1;
  for(i = 0; i < windowsize; i++) hash->chain[i] = (unsigned short)i; /*same value as index indicates uninitialized*/

  return 0;
}

static void hash_cleanup(LodePNG_Hash* hash)
{
  lodepng_free(hash->head);
  lodepng_free(hash->val);
  lodepng_free(hash->chain);
  lodepng_free(hash->zeros);
}

static unsigned getHash(const unsigned char* data, size_t size, size_t pos)
{
  unsigned result = 0;
  size_t amount, i;
  if(pos >= size) return 0;
  amount = HASH_NUM_CHARACTERS;
  if(pos + amount >= size) amount = size - pos;
  for(i = 0; i < amount; i++) result ^= (data[pos + i] << (i * HASH_SHIFT));
  return result % HASH_NUM_VALUES;
}

static unsigned countZeros(const unsigned char* data, size_t size, size_t pos)
{
  const unsigned char* start = data + pos;
  const unsigned char* end = start + MAX_SUPPORTED_DEFLATE_LENGTH;
  if(end > data + size) end = data + size;
  data = start;
  while (data != end && *data == 0) data++;
  /*subtracting two addresses returned as 32-bit number (max value is MAX_SUPPORTED_DEFLATE_LENGTH)*/
  return (unsigned)(data - start);
}

static void updateHashChain(LodePNG_Hash* hash, size_t pos, int hashval, unsigned windowsize)
{
  unsigned wpos = pos % windowsize;
  hash->val[wpos] = hashval;
  if (hash->head[hashval] != -1) hash->chain[wpos] = (unsigned short)hash->head[hashval];
  hash->head[hashval] = wpos;
}

/*
LZ77-encode the data. Return value is error code. The input are raw bytes, the output
is in the form of unsigned integers with codes representing for example literal bytes, or
length/distance pairs.
It uses a hash table technique to let it encode faster. When doing LZ77 encoding, a
sliding window (of windowsize) is used, and all past bytes in that window can be used as
the "dictionary". A brute force search through all possible distances would be slow, and
this hash technique is one out of several ways to speed this up.
*/
static unsigned encodeLZ77(uivector* out, LodePNG_Hash* hash,
                           const unsigned char* in, size_t inpos, size_t insize, unsigned windowsize,
                           unsigned minmatch, unsigned nicematch, unsigned lazymatching)
{
  unsigned short numzeros = 0;
  int usezeros = windowsize >= 8192; /*for small window size, the 'max chain length' optimization does a better job*/
  unsigned pos, i, error = 0;
  /*for large window lengths, assume the user wants no compression loss. Otherwise, max hash chain length speedup.*/
  unsigned maxchainlength = windowsize >= 8192 ? windowsize : windowsize / 8;
  unsigned maxlazymatch = windowsize >= 8192 ? MAX_SUPPORTED_DEFLATE_LENGTH : 64;

  if(!error)
  {
    unsigned offset; /*the offset represents the distance in LZ77 terminology*/
    unsigned length;
    unsigned lazy = 0;
    unsigned lazylength = 0, lazyoffset = 0;
    unsigned hashval;
    unsigned current_offset, current_length;
    const unsigned char *lastptr, *foreptr, *backptr;
    unsigned short hashpos, prevpos;

	for (pos = (unsigned int)inpos; pos < (unsigned int)insize; pos++)
    {
      size_t wpos = pos % windowsize; /*position for in 'circular' hash buffers*/

      hashval = getHash(in, insize, pos);
      updateHashChain(hash, pos, hashval, windowsize);

      if(usezeros && hashval == 0)
      {
		  numzeros = (unsigned short) countZeros(in, insize, pos);
		  hash->zeros[wpos] = numzeros;
      }

      /*the length and offset found for the current position*/
      length = 0;
      offset = 0;

	  prevpos = (unsigned short)hash->head[hashval];
	  hashpos = (unsigned short)hash->chain[prevpos];

      lastptr = &in[insize < pos + MAX_SUPPORTED_DEFLATE_LENGTH ? insize : pos + MAX_SUPPORTED_DEFLATE_LENGTH];

      /*search for the longest string*/
      if(hash->val[wpos] == (int)hashval)
      {
        unsigned chainlength = 0;
        for(;;)
        {
          /*stop when went completely around the circular buffer*/
          if(prevpos < wpos && hashpos > prevpos && hashpos <= wpos) break;
          if(prevpos > wpos && (hashpos <= wpos || hashpos > prevpos)) break;
          if(chainlength++ >= maxchainlength) break;

		  current_offset = hashpos <= wpos ? (unsigned int)wpos - hashpos : (unsigned int)wpos - hashpos + windowsize;
          if(current_offset > 0)
          {
            /*test the next characters*/
            foreptr = &in[pos];
            backptr = &in[pos - current_offset];

            /*common case in PNGs is lots of zeros. Quickly skip over them as a speedup*/
            if(usezeros && hashval == 0 && hash->val[hashpos] == 0 /*hashval[hashpos] may be out of date*/)
            {
              unsigned short skip = hash->zeros[hashpos];
              if(skip > numzeros) skip = numzeros;
              backptr += skip;
              foreptr += skip;
            }

            /* multiple checks at once per array bounds check */
            while(foreptr != lastptr && *backptr == *foreptr) /*maximum supported length by deflate is max length*/
            {
              ++backptr;
              ++foreptr;
            }
            current_length = (unsigned)(foreptr - &in[pos]);

            if(current_length > length)
            {
              length = current_length; /*the longest length*/
              offset = current_offset; /*the offset that is related to this longest length*/
              /*jump out once a length of max length is found (speed gain)*/
              if(current_length >= nicematch || current_length == MAX_SUPPORTED_DEFLATE_LENGTH) break;
            }
          }

          if(hashpos == hash->chain[hashpos]) break;

          prevpos = hashpos;
          hashpos = hash->chain[hashpos];
        }
      }

      if(lazymatching)
      {
        if(!lazy && length >= 3 && length <= maxlazymatch && length < MAX_SUPPORTED_DEFLATE_LENGTH)
        {
          lazy = 1;
          lazylength = length;
          lazyoffset = offset;
          continue; /*try the next byte*/
        }
        if(lazy)
        {
          lazy = 0;
          if(pos == 0) ERROR_BREAK(81);
          if(length > lazylength + 1)
          {
            /*push the previous character as literal*/
            if(!uivector_push_back(out, in[pos - 1])) ERROR_BREAK(83 /*alloc fail*/);
          }
          else
          {
            length = lazylength;
            offset = lazyoffset;
            hash->head[hashval] = -1; /*the same hashchain update will be done, this ensures no wrong alteration*/
            pos--;
          }
        }
      }
      if(length >= 3 && offset > windowsize) ERROR_BREAK(86 /*too big (or overflown negative) offset*/);

      /**encode it as length/distance pair or literal value**/
      if(length < 3) /*only lengths of 3 or higher are supported as length/distance pair*/
      {
        if(!uivector_push_back(out, in[pos])) ERROR_BREAK(83 /*alloc fail*/);
      }
      else if(length < minmatch || (length == 3 && offset > 4096))
      {
        /*compensate for the fact that longer offsets have more extra bits, a
        length of only 3 may be not worth it then*/
        if(!uivector_push_back(out, in[pos])) ERROR_BREAK(83 /*alloc fail*/);
      }
      else
      {
        addLengthDistance(out, length, offset);
        for(i = 1; i < length; i++)
        {
          pos++;
          hashval = getHash(in, insize, pos);
          updateHashChain(hash, pos, hashval, windowsize);
          if(usezeros && hashval == 0)
          {
			  hash->zeros[pos % windowsize] = (unsigned short)countZeros(in, insize, pos);
          }
        }
      }

    } /*end of the loop through each character of input*/
  } /*end of "if(!error)"*/

  return error;
}

/* /////////////////////////////////////////////////////////////////////////// */

static unsigned deflateNoCompression(ucvector* out, const unsigned char* data, size_t datasize)
{
  /*non compressed deflate block data: 1 bit BFINAL,2 bits BTYPE,(5 bits): it jumps to start of next byte,
  2 bytes LEN, 2 bytes NLEN, LEN bytes literal DATA*/

  size_t i, j, numdeflateblocks = (datasize + 65534) / 65535;
  unsigned datapos = 0;
  for(i = 0; i < numdeflateblocks; i++)
  {
    unsigned BFINAL, BTYPE, LEN, NLEN;
    unsigned char firstbyte;

    BFINAL = (i == numdeflateblocks - 1);
    BTYPE = 0;

    firstbyte = (unsigned char)(BFINAL + ((BTYPE & 1) << 1) + ((BTYPE & 2) << 1));
    ucvector_push_back(out, firstbyte);

    LEN = 65535;
    if(datasize - datapos < 65535) LEN = (unsigned)datasize - datapos;
    NLEN = 65535 - LEN;

    ucvector_push_back(out, (unsigned char)(LEN % 256));
    ucvector_push_back(out, (unsigned char)(LEN / 256));
    ucvector_push_back(out, (unsigned char)(NLEN % 256));
    ucvector_push_back(out, (unsigned char)(NLEN / 256));

    /*Decompressed data*/
    for(j = 0; j < 65535 && datapos < datasize; j++)
    {
      ucvector_push_back(out, data[datapos++]);
    }
  }

  return 0;
}

/*
write the lz77-encoded data, which has lit, len and dist codes, to compressed stream using huffman trees.
tree_ll: the tree for lit and len codes.
tree_d: the tree for distance codes.
*/
static void writeLZ77data(size_t* bp, ucvector* out, const uivector* lz77_encoded,
                          const HuffmanTree* tree_ll, const HuffmanTree* tree_d)
{
  size_t i = 0;
  for(i = 0; i < lz77_encoded->size; i++)
  {
    unsigned val = lz77_encoded->data[i];
    addHuffmanSymbol(bp, out, HuffmanTree_getCode(tree_ll, val), HuffmanTree_getLength(tree_ll, val));
    if(val > 256) /*for a length code, 3 more things have to be added*/
    {
      unsigned length_index = val - FIRST_LENGTH_CODE_INDEX;
      unsigned n_length_extra_bits = LENGTHEXTRA[length_index];
      unsigned length_extra_bits = lz77_encoded->data[++i];

      unsigned distance_code = lz77_encoded->data[++i];

      unsigned distance_index = distance_code;
      unsigned n_distance_extra_bits = DISTANCEEXTRA[distance_index];
      unsigned distance_extra_bits = lz77_encoded->data[++i];

      addBitsToStream(bp, out, length_extra_bits, n_length_extra_bits);
      addHuffmanSymbol(bp, out, HuffmanTree_getCode(tree_d, distance_code),
                       HuffmanTree_getLength(tree_d, distance_code));
      addBitsToStream(bp, out, distance_extra_bits, n_distance_extra_bits);
    }
  }
}

/*Deflate for a block of type "dynamic", that is, with freely, optimally, created huffman trees*/
static unsigned deflateDynamic(ucvector* out, size_t* bp, LodePNG_Hash* hash,
                               const unsigned char* data, size_t datapos, size_t dataend,
                               const LodePNGCompressSettings* settings, int final)
{
  unsigned error = 0;

  /*
  A block is compressed as follows: The PNG data is lz77 encoded, resulting in
  literal bytes and length/distance pairs. This is then huffman compressed with
  two huffman trees. One huffman tree is used for the lit and len values ("ll"),
  another huffman tree is used for the dist values ("d"). These two trees are
  stored using their code lengths, and to compress even more these code lengths
  are also run-length encoded and huffman compressed. This gives a huffman tree
  of code lengths "cl". The code lenghts used to describe this third tree are
  the code length code lengths ("clcl").
  */

  /*The lz77 encoded data, represented with integers since there will also be length and distance codes in it*/
  uivector lz77_encoded;
  HuffmanTree tree_ll; /*tree for lit,len values*/
  HuffmanTree tree_d; /*tree for distance codes*/
  HuffmanTree tree_cl; /*tree for encoding the code lengths representing tree_ll and tree_d*/
  uivector frequencies_ll; /*frequency of lit,len codes*/
  uivector frequencies_d; /*frequency of dist codes*/
  uivector frequencies_cl; /*frequency of code length codes*/
  uivector bitlen_lld; /*lit,len,dist code lenghts (int bits), literally (without repeat codes).*/
  uivector bitlen_lld_e; /*bitlen_lld encoded with repeat codes (this is a rudemtary run length compression)*/
  /*bitlen_cl is the code length code lengths ("clcl"). The bit lengths of codes to represent tree_cl
  (these are written as is in the file, it would be crazy to compress these using yet another huffman
  tree that needs to be represented by yet another set of code lengths)*/
  uivector bitlen_cl;
  size_t datasize = dataend - datapos;

  /*
  Due to the huffman compression of huffman tree representations ("two levels"), there are some anologies:
  bitlen_lld is to tree_cl what data is to tree_ll and tree_d.
  bitlen_lld_e is to bitlen_lld what lz77_encoded is to data.
  bitlen_cl is to bitlen_lld_e what bitlen_lld is to lz77_encoded.
  */

  unsigned BFINAL = final;
  size_t numcodes_ll, numcodes_d, i;
  unsigned HLIT, HDIST, HCLEN;

  uivector_init(&lz77_encoded);
  HuffmanTree_init(&tree_ll);
  HuffmanTree_init(&tree_d);
  HuffmanTree_init(&tree_cl);
  uivector_init(&frequencies_ll);
  uivector_init(&frequencies_d);
  uivector_init(&frequencies_cl);
  uivector_init(&bitlen_lld);
  uivector_init(&bitlen_lld_e);
  uivector_init(&bitlen_cl);

  /*This while loop never loops due to a break at the end, it is here to
  allow breaking out of it to the cleanup phase on error conditions.*/
  while(!error)
  {
    if(settings->use_lz77)
    {
      error = encodeLZ77(&lz77_encoded, hash, data, datapos, dataend, settings->windowsize,
                         settings->minmatch, settings->nicematch, settings->lazymatching);
      if(error) break;
    }
    else
    {
      if(!uivector_resize(&lz77_encoded, datasize)) ERROR_BREAK(83 /*alloc fail*/);
      for(i = datapos; i < dataend; i++) lz77_encoded.data[i] = data[i]; /*no LZ77, but still will be Huffman compressed*/
    }

    if(!uivector_resizev(&frequencies_ll, 286, 0)) ERROR_BREAK(83 /*alloc fail*/);
    if(!uivector_resizev(&frequencies_d, 30, 0)) ERROR_BREAK(83 /*alloc fail*/);

    /*Count the frequencies of lit, len and dist codes*/
    for(i = 0; i < lz77_encoded.size; i++)
    {
      unsigned symbol = lz77_encoded.data[i];
      frequencies_ll.data[symbol]++;
      if(symbol > 256)
      {
        unsigned dist = lz77_encoded.data[i + 2];
        frequencies_d.data[dist]++;
        i += 3;
      }
    }
    frequencies_ll.data[256] = 1; /*there will be exactly 1 end code, at the end of the block*/

    /*Make both huffman trees, one for the lit and len codes, one for the dist codes*/
    error = HuffmanTree_makeFromFrequencies(&tree_ll, frequencies_ll.data, 257, frequencies_ll.size, 15);
    if(error) break;
    /*2, not 1, is chosen for mincodes: some buggy PNG decoders require at least 2 symbols in the dist tree*/
    error = HuffmanTree_makeFromFrequencies(&tree_d, frequencies_d.data, 2, frequencies_d.size, 15);
    if(error) break;

    numcodes_ll = tree_ll.numcodes; if(numcodes_ll > 286) numcodes_ll = 286;
    numcodes_d = tree_d.numcodes; if(numcodes_d > 30) numcodes_d = 30;
    /*store the code lengths of both generated trees in bitlen_lld*/
    for(i = 0; i < numcodes_ll; i++) uivector_push_back(&bitlen_lld, HuffmanTree_getLength(&tree_ll, (unsigned)i));
    for(i = 0; i < numcodes_d; i++) uivector_push_back(&bitlen_lld, HuffmanTree_getLength(&tree_d, (unsigned)i));

    /*run-length compress bitlen_ldd into bitlen_lld_e by using repeat codes 16 (copy length 3-6 times),
    17 (3-10 zeroes), 18 (11-138 zeroes)*/
    for(i = 0; i < (unsigned)bitlen_lld.size; i++)
    {
      unsigned j = 0; /*amount of repititions*/
      while(i + j + 1 < (unsigned)bitlen_lld.size && bitlen_lld.data[i + j + 1] == bitlen_lld.data[i]) j++;

      if(bitlen_lld.data[i] == 0 && j >= 2) /*repeat code for zeroes*/
      {
        j++; /*include the first zero*/
        if(j <= 10) /*repeat code 17 supports max 10 zeroes*/
        {
          uivector_push_back(&bitlen_lld_e, 17);
          uivector_push_back(&bitlen_lld_e, j - 3);
        }
        else /*repeat code 18 supports max 138 zeroes*/
        {
          if(j > 138) j = 138;
          uivector_push_back(&bitlen_lld_e, 18);
          uivector_push_back(&bitlen_lld_e, j - 11);
        }
        i += (j - 1);
      }
      else if(j >= 3) /*repeat code for value other than zero*/
      {
        size_t k;
        unsigned num = j / 6, rest = j % 6;
        uivector_push_back(&bitlen_lld_e, bitlen_lld.data[i]);
        for(k = 0; k < num; k++)
        {
          uivector_push_back(&bitlen_lld_e, 16);
          uivector_push_back(&bitlen_lld_e, 6 - 3);
        }
        if(rest >= 3)
        {
          uivector_push_back(&bitlen_lld_e, 16);
          uivector_push_back(&bitlen_lld_e, rest - 3);
        }
        else j -= rest;
        i += j;
      }
      else /*too short to benefit from repeat code*/
      {
        uivector_push_back(&bitlen_lld_e, bitlen_lld.data[i]);
      }
    }

    /*generate tree_cl, the huffmantree of huffmantrees*/

    if(!uivector_resizev(&frequencies_cl, NUM_CODE_LENGTH_CODES, 0)) ERROR_BREAK(83 /*alloc fail*/);
    for(i = 0; i < bitlen_lld_e.size; i++)
    {
      frequencies_cl.data[bitlen_lld_e.data[i]]++;
      /*after a repeat code come the bits that specify the number of repetitions,
      those don't need to be in the frequencies_cl calculation*/
      if(bitlen_lld_e.data[i] >= 16) i++;
    }

    error = HuffmanTree_makeFromFrequencies(&tree_cl, frequencies_cl.data,
                                            frequencies_cl.size, frequencies_cl.size, 7);
    if(error) break;

    if(!uivector_resize(&bitlen_cl, tree_cl.numcodes)) ERROR_BREAK(83 /*alloc fail*/);
    for(i = 0; i < tree_cl.numcodes; i++)
    {
      /*lenghts of code length tree is in the order as specified by deflate*/
      bitlen_cl.data[i] = HuffmanTree_getLength(&tree_cl, CLCL_ORDER[i]);
    }
    while(bitlen_cl.data[bitlen_cl.size - 1] == 0 && bitlen_cl.size > 4)
    {
      /*remove zeros at the end, but minimum size must be 4*/
      if(!uivector_resize(&bitlen_cl, bitlen_cl.size - 1)) ERROR_BREAK(83 /*alloc fail*/);
    }
    if(error) break;

    /*
    Write everything into the output

    After the BFINAL and BTYPE, the dynamic block consists out of the following:
    - 5 bits HLIT, 5 bits HDIST, 4 bits HCLEN
    - (HCLEN+4)*3 bits code lengths of code length alphabet
    - HLIT + 257 code lenghts of lit/length alphabet (encoded using the code length
      alphabet, + possible repetition codes 16, 17, 18)
    - HDIST + 1 code lengths of distance alphabet (encoded using the code length
      alphabet, + possible repetition codes 16, 17, 18)
    - compressed data
    - 256 (end code)
    */

    /*Write block type*/
    addBitToStream(bp, out, (unsigned char)BFINAL);
    addBitToStream(bp, out, 0); /*first bit of BTYPE "dynamic"*/
    addBitToStream(bp, out, 1); /*second bit of BTYPE "dynamic"*/

    /*write the HLIT, HDIST and HCLEN values*/
    HLIT = (unsigned)(numcodes_ll - 257);
    HDIST = (unsigned)(numcodes_d - 1);
    HCLEN = (unsigned)bitlen_cl.size - 4;
    /*trim zeroes for HCLEN. HLIT and HDIST were already trimmed at tree creation*/
    while(!bitlen_cl.data[HCLEN + 4 - 1] && HCLEN > 0) HCLEN--;
    addBitsToStream(bp, out, HLIT, 5);
    addBitsToStream(bp, out, HDIST, 5);
    addBitsToStream(bp, out, HCLEN, 4);

    /*write the code lenghts of the code length alphabet*/
    for(i = 0; i < HCLEN + 4; i++) addBitsToStream(bp, out, bitlen_cl.data[i], 3);

    /*write the lenghts of the lit/len AND the dist alphabet*/
    for(i = 0; i < bitlen_lld_e.size; i++)
    {
      addHuffmanSymbol(bp, out, HuffmanTree_getCode(&tree_cl, bitlen_lld_e.data[i]),
                       HuffmanTree_getLength(&tree_cl, bitlen_lld_e.data[i]));
      /*extra bits of repeat codes*/
      if(bitlen_lld_e.data[i] == 16) addBitsToStream(bp, out, bitlen_lld_e.data[++i], 2);
      else if(bitlen_lld_e.data[i] == 17) addBitsToStream(bp, out, bitlen_lld_e.data[++i], 3);
      else if(bitlen_lld_e.data[i] == 18) addBitsToStream(bp, out, bitlen_lld_e.data[++i], 7);
    }

    /*write the compressed data symbols*/
    writeLZ77data(bp, out, &lz77_encoded, &tree_ll, &tree_d);
    /*error: the length of the end code 256 must be larger than 0*/
    if(HuffmanTree_getLength(&tree_ll, 256) == 0) ERROR_BREAK(64);

    /*write the end code*/
    addHuffmanSymbol(bp, out, HuffmanTree_getCode(&tree_ll, 256), HuffmanTree_getLength(&tree_ll, 256));

    break; /*end of error-while*/
  }

  /*cleanup*/
  uivector_cleanup(&lz77_encoded);
  HuffmanTree_cleanup(&tree_ll);
  HuffmanTree_cleanup(&tree_d);
  HuffmanTree_cleanup(&tree_cl);
  uivector_cleanup(&frequencies_ll);
  uivector_cleanup(&frequencies_d);
  uivector_cleanup(&frequencies_cl);
  uivector_cleanup(&bitlen_lld_e);
  uivector_cleanup(&bitlen_lld);
  uivector_cleanup(&bitlen_cl);

  return error;
}

static unsigned deflateFixed(ucvector* out, size_t* bp, LodePNG_Hash* hash,
                             const unsigned char* data,
                             size_t datapos, size_t dataend,
                             const LodePNGCompressSettings* settings, int final)
{
  HuffmanTree tree_ll; /*tree for literal values and length codes*/
  HuffmanTree tree_d; /*tree for distance codes*/

  unsigned BFINAL = final;
  unsigned error = 0;
  size_t i;

  HuffmanTree_init(&tree_ll);
  HuffmanTree_init(&tree_d);

  generateFixedLitLenTree(&tree_ll);
  generateFixedDistanceTree(&tree_d);

  addBitToStream(bp, out, (unsigned char)BFINAL);
  addBitToStream(bp, out, 1); /*first bit of BTYPE*/
  addBitToStream(bp, out, 0); /*second bit of BTYPE*/

  if(settings->use_lz77) /*LZ77 encoded*/
  {
    uivector lz77_encoded;
    uivector_init(&lz77_encoded);
    error = encodeLZ77(&lz77_encoded, hash, data, datapos, dataend, settings->windowsize,
                       settings->minmatch, settings->nicematch, settings->lazymatching);
    if(!error) writeLZ77data(bp, out, &lz77_encoded, &tree_ll, &tree_d);
    uivector_cleanup(&lz77_encoded);
  }
  else /*no LZ77, but still will be Huffman compressed*/
  {
    for(i = datapos; i < dataend; i++)
    {
      addHuffmanSymbol(bp, out, HuffmanTree_getCode(&tree_ll, data[i]), HuffmanTree_getLength(&tree_ll, data[i]));
    }
  }
  /*add END code*/
  if(!error) addHuffmanSymbol(bp, out, HuffmanTree_getCode(&tree_ll, 256), HuffmanTree_getLength(&tree_ll, 256));

  /*cleanup*/
  HuffmanTree_cleanup(&tree_ll);
  HuffmanTree_cleanup(&tree_d);

  return error;
}

static unsigned lodepng_deflatev(ucvector* out, const unsigned char* in, size_t insize,
                                 const LodePNGCompressSettings* settings)
{
  unsigned error = 0;
  size_t i, blocksize, numdeflateblocks;
  size_t bp = 0; /*the bit pointer*/
  LodePNG_Hash hash;

  if(settings->btype > 2) return 61;
  else if(settings->btype == 0) return deflateNoCompression(out, in, insize);
  else if(settings->btype == 1) blocksize = insize;
  else /*if(settings->btype == 2)*/
  {
    blocksize = insize / 8 + 8;
    if(blocksize < 65535) blocksize = 65535;
  }

  numdeflateblocks = (insize + blocksize - 1) / blocksize;
  if(numdeflateblocks == 0) numdeflateblocks = 1;

  error = hash_init(&hash, settings->windowsize);
  if(error) return error;

  for(i = 0; i < numdeflateblocks && !error; i++)
  {
    int final = i == numdeflateblocks - 1;
    size_t start = i * blocksize;
    size_t end = start + blocksize;
    if(end > insize) end = insize;

    if(settings->btype == 1) error = deflateFixed(out, &bp, &hash, in, start, end, settings, final);
    else if(settings->btype == 2) error = deflateDynamic(out, &bp, &hash, in, start, end, settings, final);
  }

  hash_cleanup(&hash);

  return error;
}

unsigned lodepng_deflate(unsigned char** out, size_t* outsize,
                         const unsigned char* in, size_t insize,
                         const LodePNGCompressSettings* settings)
{
  unsigned error;
  ucvector v;
  ucvector_init_buffer(&v, *out, *outsize);
  error = lodepng_deflatev(&v, in, insize, settings);
  *out = v.data;
  *outsize = v.size;
  return error;
}

static unsigned deflate(unsigned char** out, size_t* outsize,
                        const unsigned char* in, size_t insize,
                        const LodePNGCompressSettings* settings)
{
  if(settings->custom_deflate)
  {
    return settings->custom_deflate(out, outsize, in, insize, settings);
  }
  else
  {
    return lodepng_deflate(out, outsize, in, insize, settings);
  }
}

#endif /*LODEPNG_COMPILE_DECODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* / Adler32                                                                  */
/* ////////////////////////////////////////////////////////////////////////// */

static unsigned update_adler32(unsigned adler, const unsigned char* data, unsigned len)
{
   unsigned s1 = adler & 0xffff;
   unsigned s2 = (adler >> 16) & 0xffff;

  while(len > 0)
  {
    /*at least 5550 sums can be done before the sums overflow, saving a lot of module divisions*/
    unsigned amount = len > 5550 ? 5550 : len;
    len -= amount;
    while(amount > 0)
    {
      s1 += (*data++);
      s2 += s1;
      amount--;
    }
    s1 %= 65521;
    s2 %= 65521;
  }

  return (s2 << 16) | s1;
}

/*Return the adler32 of the bytes data[0..len-1]*/
static unsigned adler32(const unsigned char* data, unsigned len)
{
  return update_adler32(1L, data, len);
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Zlib                                                                   / */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_DECODER

unsigned lodepng_zlib_decompress(unsigned char** out, size_t* outsize, const unsigned char* in,
                                 size_t insize, const LodePNGDecompressSettings* settings)
{
  unsigned error = 0;
  unsigned CM, CINFO, FDICT;

  if(insize < 2) return 53; /*error, size of zlib data too small*/
  /*read information from zlib header*/
  if((in[0] * 256 + in[1]) % 31 != 0)
  {
    /*error: 256 * in[0] + in[1] must be a multiple of 31, the FCHECK value is supposed to be made that way*/
    return 24;
  }

  CM = in[0] & 15;
  CINFO = (in[0] >> 4) & 15;
  /*FCHECK = in[1] & 31;*/ /*FCHECK is already tested above*/
  FDICT = (in[1] >> 5) & 1;
  /*FLEVEL = (in[1] >> 6) & 3;*/ /*FLEVEL is not used here*/

  if(CM != 8 || CINFO > 7)
  {
    /*error: only compression method 8: inflate with sliding window of 32k is supported by the PNG spec*/
    return 25;
  }
  if(FDICT != 0)
  {
    /*error: the specification of PNG says about the zlib stream:
      "The additional flags shall not specify a preset dictionary."*/
    return 26;
  }

  error = inflate(out, outsize, in + 2, insize - 2, settings);
  if(error) return error;

  if(!settings->ignore_adler32)
  {
    unsigned ADLER32 = lodepng_read32bitInt(&in[insize - 4]);
    unsigned checksum = adler32(*out, (unsigned)(*outsize));
    if(checksum != ADLER32) return 58; /*error, adler checksum not correct, data must be corrupted*/
  }

  return 0; /*no error*/
}

static unsigned zlib_decompress(unsigned char** out, size_t* outsize, const unsigned char* in,
                                size_t insize, const LodePNGDecompressSettings* settings)
{
  if(settings->custom_zlib)
    return settings->custom_zlib(out, outsize, in, insize, settings);
  else
    return lodepng_zlib_decompress(out, outsize, in, insize, settings);
}

#endif /*LODEPNG_COMPILE_DECODER*/

#ifdef LODEPNG_COMPILE_ENCODER

unsigned lodepng_zlib_compress(unsigned char** out, size_t* outsize, const unsigned char* in,
                               size_t insize, const LodePNGCompressSettings* settings)
{
  /*initially, *out must be NULL and outsize 0, if you just give some random *out
  that's pointing to a non allocated buffer, this'll crash*/
  ucvector outv;
  size_t i;
  unsigned error;
  unsigned char* deflatedata = 0;
  size_t deflatesize = 0;

  unsigned ADLER32;
  /*zlib data: 1 byte CMF (CM+CINFO), 1 byte FLG, deflate data, 4 byte ADLER32 checksum of the Decompressed data*/
  unsigned CMF = 120; /*0b01111000: CM 8, CINFO 7. With CINFO 7, any window size up to 32768 can be used.*/
  unsigned FLEVEL = 0;
  unsigned FDICT = 0;
  unsigned CMFFLG = 256 * CMF + FDICT * 32 + FLEVEL * 64;
  unsigned FCHECK = 31 - CMFFLG % 31;
  CMFFLG += FCHECK;

  /*ucvector-controlled version of the output buffer, for dynamic array*/
  ucvector_init_buffer(&outv, *out, *outsize);

  ucvector_push_back(&outv, (unsigned char)(CMFFLG / 256));
  ucvector_push_back(&outv, (unsigned char)(CMFFLG % 256));

  error = deflate(&deflatedata, &deflatesize, in, insize, settings);

  if(!error)
  {
    ADLER32 = adler32(in, (unsigned)insize);
    for(i = 0; i < deflatesize; i++) ucvector_push_back(&outv, deflatedata[i]);
    lodepng_free(deflatedata);
    lodepng_add32bitInt(&outv, ADLER32);
  }

  *out = outv.data;
  *outsize = outv.size;

  return error;
}

/* compress using the default or custom zlib function */
static unsigned zlib_compress(unsigned char** out, size_t* outsize, const unsigned char* in,
                              size_t insize, const LodePNGCompressSettings* settings)
{
  if(settings->custom_zlib)
  {
    return settings->custom_zlib(out, outsize, in, insize, settings);
  }
  else
  {
    return lodepng_zlib_compress(out, outsize, in, insize, settings);
  }
}

#endif /*LODEPNG_COMPILE_ENCODER*/

#else /*no LODEPNG_COMPILE_ZLIB*/

#ifdef LODEPNG_COMPILE_DECODER
static unsigned zlib_decompress(unsigned char** out, size_t* outsize, const unsigned char* in,
                                size_t insize, const LodePNGDecompressSettings* settings)
{
  if (!settings->custom_zlib) return 87; /*no custom zlib function provided */
  return settings->custom_zlib(out, outsize, in, insize, settings);
}
#endif /*LODEPNG_COMPILE_DECODER*/
#ifdef LODEPNG_COMPILE_ENCODER
static unsigned zlib_compress(unsigned char** out, size_t* outsize, const unsigned char* in,
                              size_t insize, const LodePNGCompressSettings* settings)
{
  if (!settings->custom_zlib) return 87; /*no custom zlib function provided */
  return settings->custom_zlib(out, outsize, in, insize, settings);
}
#endif /*LODEPNG_COMPILE_ENCODER*/

#endif /*LODEPNG_COMPILE_ZLIB*/

/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_ENCODER

/*this is a good tradeoff between speed and compression ratio*/
#define DEFAULT_WINDOWSIZE 2048

void lodepng_compress_settings_init(LodePNGCompressSettings* settings)
{
  /*compress with dynamic huffman tree (not in the mathematical sense, just not the predefined one)*/
  settings->btype = 2;
  settings->use_lz77 = 1;
  settings->windowsize = DEFAULT_WINDOWSIZE;
  settings->minmatch = 3;
  settings->nicematch = 128;
  settings->lazymatching = 1;

  settings->custom_zlib = 0;
  settings->custom_deflate = 0;
  settings->custom_context = 0;
}

const LodePNGCompressSettings lodepng_default_compress_settings = {2, 1, DEFAULT_WINDOWSIZE, 3, 128, 1, 0, 0, 0};


#endif /*LODEPNG_COMPILE_ENCODER*/

#ifdef LODEPNG_COMPILE_DECODER

void lodepng_decompress_settings_init(LodePNGDecompressSettings* settings)
{
  settings->ignore_adler32 = 0;

  settings->custom_zlib = 0;
  settings->custom_inflate = 0;
  settings->custom_context = 0;
}

const LodePNGDecompressSettings lodepng_default_decompress_settings = {0, 0, 0, 0};

#endif /*LODEPNG_COMPILE_DECODER*/

/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */
/* // End of Zlib related code. Begin of PNG related code.                 // */
/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */

#ifdef LODEPNG_COMPILE_PNG

/* ////////////////////////////////////////////////////////////////////////// */
/* / CRC32                                                                  / */
/* ////////////////////////////////////////////////////////////////////////// */

/* CRC polynomial: 0xedb88320 */
static unsigned lodepng_crc32_table[256] = {
           0u, 1996959894u, 3993919788u, 2567524794u,  124634137u, 1886057615u, 3915621685u, 2657392035u,
   249268274u, 2044508324u, 3772115230u, 2547177864u,  162941995u, 2125561021u, 3887607047u, 2428444049u,
   498536548u, 1789927666u, 4089016648u, 2227061214u,  450548861u, 1843258603u, 4107580753u, 2211677639u,
   325883990u, 1684777152u, 4251122042u, 2321926636u,  335633487u, 1661365465u, 4195302755u, 2366115317u,
   997073096u, 1281953886u, 3579855332u, 2724688242u, 1006888145u, 1258607687u, 3524101629u, 2768942443u,
   901097722u, 1119000684u, 3686517206u, 2898065728u,  853044451u, 1172266101u, 3705015759u, 2882616665u,
   651767980u, 1373503546u, 3369554304u, 3218104598u,  565507253u, 1454621731u, 3485111705u, 3099436303u,
   671266974u, 1594198024u, 3322730930u, 2970347812u,  795835527u, 1483230225u, 3244367275u, 3060149565u,
  1994146192u,   31158534u, 2563907772u, 4023717930u, 1907459465u,  112637215u, 2680153253u, 3904427059u,
  2013776290u,  251722036u, 2517215374u, 3775830040u, 2137656763u,  141376813u, 2439277719u, 3865271297u,
  1802195444u,  476864866u, 2238001368u, 4066508878u, 1812370925u,  453092731u, 2181625025u, 4111451223u,
  1706088902u,  314042704u, 2344532202u, 4240017532u, 1658658271u,  366619977u, 2362670323u, 4224994405u,
  1303535960u,  984961486u, 2747007092u, 3569037538u, 1256170817u, 1037604311u, 2765210733u, 3554079995u,
  1131014506u,  879679996u, 2909243462u, 3663771856u, 1141124467u,  855842277u, 2852801631u, 3708648649u,
  1342533948u,  654459306u, 3188396048u, 3373015174u, 1466479909u,  544179635u, 3110523913u, 3462522015u,
  1591671054u,  702138776u, 2966460450u, 3352799412u, 1504918807u,  783551873u, 3082640443u, 3233442989u,
  3988292384u, 2596254646u,   62317068u, 1957810842u, 3939845945u, 2647816111u,   81470997u, 1943803523u,
  3814918930u, 2489596804u,  225274430u, 2053790376u, 3826175755u, 2466906013u,  167816743u, 2097651377u,
  4027552580u, 2265490386u,  503444072u, 1762050814u, 4150417245u, 2154129355u,  426522225u, 1852507879u,
  4275313526u, 2312317920u,  282753626u, 1742555852u, 4189708143u, 2394877945u,  397917763u, 1622183637u,
  3604390888u, 2714866558u,  953729732u, 1340076626u, 3518719985u, 2797360999u, 1068828381u, 1219638859u,
  3624741850u, 2936675148u,  906185462u, 1090812512u, 3747672003u, 2825379669u,  829329135u, 1181335161u,
  3412177804u, 3160834842u,  628085408u, 1382605366u, 3423369109u, 3138078467u,  570562233u, 1426400815u,
  3317316542u, 2998733608u,  733239954u, 1555261956u, 3268935591u, 3050360625u,  752459403u, 1541320221u,
  2607071920u, 3965973030u, 1969922972u,   40735498u, 2617837225u, 3943577151u, 1913087877u,   83908371u,
  2512341634u, 3803740692u, 2075208622u,  213261112u, 2463272603u, 3855990285u, 2094854071u,  198958881u,
  2262029012u, 4057260610u, 1759359992u,  534414190u, 2176718541u, 4139329115u, 1873836001u,  414664567u,
  2282248934u, 4279200368u, 1711684554u,  285281116u, 2405801727u, 4167216745u, 1634467795u,  376229701u,
  2685067896u, 3608007406u, 1308918612u,  956543938u, 2808555105u, 3495958263u, 1231636301u, 1047427035u,
  2932959818u, 3654703836u, 1088359270u,  936918000u, 2847714899u, 3736837829u, 1202900863u,  817233897u,
  3183342108u, 3401237130u, 1404277552u,  615818150u, 3134207493u, 3453421203u, 1423857449u,  601450431u,
  3009837614u, 3294710456u, 1567103746u,  711928724u, 3020668471u, 3272380065u, 1510334235u,  755167117u
};

/*Return the CRC of the bytes buf[0..len-1].*/
unsigned lodepng_crc32(const unsigned char* buf, size_t len)
{
  unsigned c = 0xffffffffL;
  size_t n;

  for(n = 0; n < len; n++)
  {
    c = lodepng_crc32_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
  }
  return c ^ 0xffffffffL;
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Reading and writing single bits and bytes from/to stream for LodePNG   / */
/* ////////////////////////////////////////////////////////////////////////// */

static unsigned char readBitFromReversedStream(size_t* bitpointer, const unsigned char* bitstream)
{
  unsigned char result = (unsigned char)((bitstream[(*bitpointer) >> 3] >> (7 - ((*bitpointer) & 0x7))) & 1);
  (*bitpointer)++;
  return result;
}

static unsigned readBitsFromReversedStream(size_t* bitpointer, const unsigned char* bitstream, size_t nbits)
{
  unsigned result = 0;
  size_t i;
  for(i = nbits - 1; i < nbits; i--)
  {
    result += (unsigned)readBitFromReversedStream(bitpointer, bitstream) << i;
  }
  return result;
}

#ifdef LODEPNG_COMPILE_DECODER
static void setBitOfReversedStream0(size_t* bitpointer, unsigned char* bitstream, unsigned char bit)
{
  /*the current bit in bitstream must be 0 for this to work*/
  if(bit)
  {
    /*earlier bit of huffman code is in a lesser significant bit of an earlier byte*/
    bitstream[(*bitpointer) >> 3] |= (bit << (7 - ((*bitpointer) & 0x7)));
  }
  (*bitpointer)++;
}
#endif /*LODEPNG_COMPILE_DECODER*/

static void setBitOfReversedStream(size_t* bitpointer, unsigned char* bitstream, unsigned char bit)
{
  /*the current bit in bitstream may be 0 or 1 for this to work*/
  if(bit == 0) bitstream[(*bitpointer) >> 3] &=  (unsigned char)(~(1 << (7 - ((*bitpointer) & 0x7))));
  else         bitstream[(*bitpointer) >> 3] |=  (1 << (7 - ((*bitpointer) & 0x7)));
  (*bitpointer)++;
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG chunks                                                             / */
/* ////////////////////////////////////////////////////////////////////////// */

unsigned lodepng_chunk_length(const unsigned char* chunk)
{
  return lodepng_read32bitInt(&chunk[0]);
}

void lodepng_chunk_type(char type[5], const unsigned char* chunk)
{
  unsigned i;
  for(i = 0; i < 4; i++) type[i] = chunk[4 + i];
  type[4] = 0; /*null termination char*/
}

unsigned char lodepng_chunk_type_equals(const unsigned char* chunk, const char* type)
{
  if(strlen(type) != 4) return 0;
  return (chunk[4] == type[0] && chunk[5] == type[1] && chunk[6] == type[2] && chunk[7] == type[3]);
}

unsigned char lodepng_chunk_ancillary(const unsigned char* chunk)
{
  return((chunk[4] & 32) != 0);
}

unsigned char lodepng_chunk_private(const unsigned char* chunk)
{
  return((chunk[6] & 32) != 0);
}

unsigned char lodepng_chunk_safetocopy(const unsigned char* chunk)
{
  return((chunk[7] & 32) != 0);
}

unsigned char* lodepng_chunk_data(unsigned char* chunk)
{
  return &chunk[8];
}

const unsigned char* lodepng_chunk_data_const(const unsigned char* chunk)
{
  return &chunk[8];
}

unsigned lodepng_chunk_check_crc(const unsigned char* chunk)
{
  unsigned length = lodepng_chunk_length(chunk);
  unsigned CRC = lodepng_read32bitInt(&chunk[length + 8]);
  /*the CRC is taken of the data and the 4 chunk type letters, not the length*/
  unsigned checksum = lodepng_crc32(&chunk[4], length + 4);
  if(CRC != checksum) return 1;
  else return 0;
}

void lodepng_chunk_generate_crc(unsigned char* chunk)
{
  unsigned length = lodepng_chunk_length(chunk);
  unsigned CRC = lodepng_crc32(&chunk[4], length + 4);
  lodepng_set32bitInt(chunk + 8 + length, CRC);
}

unsigned char* lodepng_chunk_next(unsigned char* chunk)
{
  unsigned total_chunk_length = lodepng_chunk_length(chunk) + 12;
  return &chunk[total_chunk_length];
}

const unsigned char* lodepng_chunk_next_const(const unsigned char* chunk)
{
  unsigned total_chunk_length = lodepng_chunk_length(chunk) + 12;
  return &chunk[total_chunk_length];
}

unsigned lodepng_chunk_append(unsigned char** out, size_t* outlength, const unsigned char* chunk)
{
  unsigned i;
  unsigned total_chunk_length = lodepng_chunk_length(chunk) + 12;
  unsigned char *chunk_start, *new_buffer;
  size_t new_length = (*outlength) + total_chunk_length;
  if(new_length < total_chunk_length || new_length < (*outlength)) return 77; /*integer overflow happened*/

  new_buffer = (unsigned char*)lodepng_realloc(*out, new_length);
  if(!new_buffer) return 83; /*alloc fail*/
  (*out) = new_buffer;
  (*outlength) = new_length;
  chunk_start = &(*out)[new_length - total_chunk_length];

  for(i = 0; i < total_chunk_length; i++) chunk_start[i] = chunk[i];

  return 0;
}

unsigned lodepng_chunk_create(unsigned char** out, size_t* outlength, unsigned length,
                              const char* type, const unsigned char* data)
{
  unsigned i;
  unsigned char *chunk, *new_buffer;
  size_t new_length = (*outlength) + length + 12;
  if(new_length < length + 12 || new_length < (*outlength)) return 77; /*integer overflow happened*/
  new_buffer = (unsigned char*)lodepng_realloc(*out, new_length);
  if(!new_buffer) return 83; /*alloc fail*/
  (*out) = new_buffer;
  (*outlength) = new_length;
  chunk = &(*out)[(*outlength) - length - 12];

  /*1: length*/
  lodepng_set32bitInt(chunk, (unsigned)length);

  /*2: chunk name (4 letters)*/
  chunk[4] = type[0];
  chunk[5] = type[1];
  chunk[6] = type[2];
  chunk[7] = type[3];

  /*3: the data*/
  for(i = 0; i < length; i++) chunk[8 + i] = data[i];

  /*4: CRC (of the chunkname characters and the data)*/
  lodepng_chunk_generate_crc(chunk);

  return 0;
}

/* ////////////////////////////////////////////////////////////////////////// */
/* / Color types and such                                                   / */
/* ////////////////////////////////////////////////////////////////////////// */

/*return type is a LodePNG error code*/
static unsigned checkColorValidity(LodePNGColorType colortype, unsigned bd) /*bd = bitdepth*/
{
  switch(colortype)
  {
    case 0: if(!(bd == 1 || bd == 2 || bd == 4 || bd == 8 || bd == 16)) return 37; break; /*grey*/
    case 2: if(!(                                 bd == 8 || bd == 16)) return 37; break; /*RGB*/
    case 3: if(!(bd == 1 || bd == 2 || bd == 4 || bd == 8            )) return 37; break; /*palette*/
    case 4: if(!(                                 bd == 8 || bd == 16)) return 37; break; /*grey + alpha*/
    case 6: if(!(                                 bd == 8 || bd == 16)) return 37; break; /*RGBA*/
    default: return 31;
  }
  return 0; /*allowed color type / bits combination*/
}

static unsigned getNumColorChannels(LodePNGColorType colortype)
{
  switch(colortype)
  {
    case 0: return 1; /*grey*/
    case 2: return 3; /*RGB*/
    case 3: return 1; /*palette*/
    case 4: return 2; /*grey + alpha*/
    case 6: return 4; /*RGBA*/
  }
  return 0; /*unexisting color type*/
}

static unsigned lodepng_get_bpp_lct(LodePNGColorType colortype, unsigned bitdepth)
{
  /*bits per pixel is amount of channels * bits per channel*/
  return getNumColorChannels(colortype) * bitdepth;
}

/* ////////////////////////////////////////////////////////////////////////// */

void lodepng_color_mode_init(LodePNGColorMode* info)
{
  info->key_defined = 0;
  info->key_r = info->key_g = info->key_b = 0;
  info->colortype = LCT_RGBA;
  info->bitdepth = 8;
  info->palette = 0;
  info->palettesize = 0;
}

void lodepng_color_mode_cleanup(LodePNGColorMode* info)
{
  lodepng_palette_clear(info);
}

unsigned lodepng_color_mode_copy(LodePNGColorMode* dest, const LodePNGColorMode* source)
{
  size_t i;
  lodepng_color_mode_cleanup(dest);
  *dest = *source;
  if(source->palette)
  {
    dest->palette = (unsigned char*)lodepng_malloc(1024);
    if(!dest->palette && source->palettesize) return 83; /*alloc fail*/
    for(i = 0; i < source->palettesize * 4; i++) dest->palette[i] = source->palette[i];
  }
  return 0;
}

static int lodepng_color_mode_equal(const LodePNGColorMode* a, const LodePNGColorMode* b)
{
  size_t i;
  if(a->colortype != b->colortype) return 0;
  if(a->bitdepth != b->bitdepth) return 0;
  if(a->key_defined != b->key_defined) return 0;
  if(a->key_defined)
  {
    if(a->key_r != b->key_r) return 0;
    if(a->key_g != b->key_g) return 0;
    if(a->key_b != b->key_b) return 0;
  }
  if(a->palettesize != b->palettesize) return 0;
  for(i = 0; i < a->palettesize * 4; i++)
  {
    if(a->palette[i] != b->palette[i]) return 0;
  }
  return 1;
}

void lodepng_palette_clear(LodePNGColorMode* info)
{
  if(info->palette) lodepng_free(info->palette);
  info->palette = 0;
  info->palettesize = 0;
}

unsigned lodepng_palette_add(LodePNGColorMode* info,
                             unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  unsigned char* data;
  /*the same resize technique as C++ std::vectors is used, and here it's made so that for a palette with
  the max of 256 colors, it'll have the exact alloc size*/
  if(!info->palette) /*allocate palette if empty*/
  {
    /*room for 256 colors with 4 bytes each*/
    data = (unsigned char*)lodepng_realloc(info->palette, 1024);
    if(!data) return 83; /*alloc fail*/
    else info->palette = data;
  }
  info->palette[4 * info->palettesize + 0] = r;
  info->palette[4 * info->palettesize + 1] = g;
  info->palette[4 * info->palettesize + 2] = b;
  info->palette[4 * info->palettesize + 3] = a;
  info->palettesize++;
  return 0;
}

unsigned lodepng_get_bpp(const LodePNGColorMode* info)
{
  /*calculate bits per pixel out of colortype and bitdepth*/
  return lodepng_get_bpp_lct(info->colortype, info->bitdepth);
}

unsigned lodepng_get_channels(const LodePNGColorMode* info)
{
  return getNumColorChannels(info->colortype);
}

unsigned lodepng_is_greyscale_type(const LodePNGColorMode* info)
{
  return info->colortype == LCT_GREY || info->colortype == LCT_GREY_ALPHA;
}

unsigned lodepng_is_alpha_type(const LodePNGColorMode* info)
{
  return (info->colortype & 4) != 0; /*4 or 6*/
}

unsigned lodepng_is_palette_type(const LodePNGColorMode* info)
{
  return info->colortype == LCT_PALETTE;
}

unsigned lodepng_has_palette_alpha(const LodePNGColorMode* info)
{
  size_t i;
  for(i = 0; i < info->palettesize; i++)
  {
    if(info->palette[i * 4 + 3] < 255) return 1;
  }
  return 0;
}

unsigned lodepng_can_have_alpha(const LodePNGColorMode* info)
{
  return info->key_defined
      || lodepng_is_alpha_type(info)
      || lodepng_has_palette_alpha(info);
}

size_t lodepng_get_raw_size(unsigned w, unsigned h, const LodePNGColorMode* color)
{
  return (w * h * lodepng_get_bpp(color) + 7) / 8;
}

size_t lodepng_get_raw_size_lct(unsigned w, unsigned h, LodePNGColorType colortype, unsigned bitdepth)
{
  return (w * h * lodepng_get_bpp_lct(colortype, bitdepth) + 7) / 8;
}

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS

static void LodePNGUnknownChunks_init(LodePNGInfo* info)
{
  unsigned i;
  for(i = 0; i < 3; i++) info->unknown_chunks_data[i] = 0;
  for(i = 0; i < 3; i++) info->unknown_chunks_size[i] = 0;
}

static void LodePNGUnknownChunks_cleanup(LodePNGInfo* info)
{
  unsigned i;
  for(i = 0; i < 3; i++) lodepng_free(info->unknown_chunks_data[i]);
}

static unsigned LodePNGUnknownChunks_copy(LodePNGInfo* dest, const LodePNGInfo* src)
{
  unsigned i;

  LodePNGUnknownChunks_cleanup(dest);

  for(i = 0; i < 3; i++)
  {
    size_t j;
    dest->unknown_chunks_size[i] = src->unknown_chunks_size[i];
    dest->unknown_chunks_data[i] = (unsigned char*)lodepng_malloc(src->unknown_chunks_size[i]);
    if(!dest->unknown_chunks_data[i] && dest->unknown_chunks_size[i]) return 83; /*alloc fail*/
    for(j = 0; j < src->unknown_chunks_size[i]; j++)
    {
      dest->unknown_chunks_data[i][j] = src->unknown_chunks_data[i][j];
    }
  }

  return 0;
}

/******************************************************************************/

static void LodePNGText_init(LodePNGInfo* info)
{
  info->text_num = 0;
  info->text_keys = NULL;
  info->text_strings = NULL;
}

static void LodePNGText_cleanup(LodePNGInfo* info)
{
  size_t i;
  for(i = 0; i < info->text_num; i++)
  {
    string_cleanup(&info->text_keys[i]);
    string_cleanup(&info->text_strings[i]);
  }
  lodepng_free(info->text_keys);
  lodepng_free(info->text_strings);
}

static unsigned LodePNGText_copy(LodePNGInfo* dest, const LodePNGInfo* source)
{
  size_t i = 0;
  dest->text_keys = 0;
  dest->text_strings = 0;
  dest->text_num = 0;
  for(i = 0; i < source->text_num; i++)
  {
    CERROR_TRY_RETURN(lodepng_add_text(dest, source->text_keys[i], source->text_strings[i]));
  }
  return 0;
}

void lodepng_clear_text(LodePNGInfo* info)
{
  LodePNGText_cleanup(info);
}

unsigned lodepng_add_text(LodePNGInfo* info, const char* key, const char* str)
{
  char** new_keys = (char**)(lodepng_realloc(info->text_keys, sizeof(char*) * (info->text_num + 1)));
  char** new_strings = (char**)(lodepng_realloc(info->text_strings, sizeof(char*) * (info->text_num + 1)));
  if(!new_keys || !new_strings)
  {
    lodepng_free(new_keys);
    lodepng_free(new_strings);
    return 83; /*alloc fail*/
  }

  info->text_num++;
  info->text_keys = new_keys;
  info->text_strings = new_strings;

  string_init(&info->text_keys[info->text_num - 1]);
  string_set(&info->text_keys[info->text_num - 1], key);

  string_init(&info->text_strings[info->text_num - 1]);
  string_set(&info->text_strings[info->text_num - 1], str);

  return 0;
}

/******************************************************************************/

static void LodePNGIText_init(LodePNGInfo* info)
{
  info->itext_num = 0;
  info->itext_keys = NULL;
  info->itext_langtags = NULL;
  info->itext_transkeys = NULL;
  info->itext_strings = NULL;
}

static void LodePNGIText_cleanup(LodePNGInfo* info)
{
  size_t i;
  for(i = 0; i < info->itext_num; i++)
  {
    string_cleanup(&info->itext_keys[i]);
    string_cleanup(&info->itext_langtags[i]);
    string_cleanup(&info->itext_transkeys[i]);
    string_cleanup(&info->itext_strings[i]);
  }
  lodepng_free(info->itext_keys);
  lodepng_free(info->itext_langtags);
  lodepng_free(info->itext_transkeys);
  lodepng_free(info->itext_strings);
}

static unsigned LodePNGIText_copy(LodePNGInfo* dest, const LodePNGInfo* source)
{
  size_t i = 0;
  dest->itext_keys = 0;
  dest->itext_langtags = 0;
  dest->itext_transkeys = 0;
  dest->itext_strings = 0;
  dest->itext_num = 0;
  for(i = 0; i < source->itext_num; i++)
  {
    CERROR_TRY_RETURN(lodepng_add_itext(dest, source->itext_keys[i], source->itext_langtags[i],
                                        source->itext_transkeys[i], source->itext_strings[i]));
  }
  return 0;
}

void lodepng_clear_itext(LodePNGInfo* info)
{
  LodePNGIText_cleanup(info);
}

unsigned lodepng_add_itext(LodePNGInfo* info, const char* key, const char* langtag,
                           const char* transkey, const char* str)
{
  char** new_keys = (char**)(lodepng_realloc(info->itext_keys, sizeof(char*) * (info->itext_num + 1)));
  char** new_langtags = (char**)(lodepng_realloc(info->itext_langtags, sizeof(char*) * (info->itext_num + 1)));
  char** new_transkeys = (char**)(lodepng_realloc(info->itext_transkeys, sizeof(char*) * (info->itext_num + 1)));
  char** new_strings = (char**)(lodepng_realloc(info->itext_strings, sizeof(char*) * (info->itext_num + 1)));
  if(!new_keys || !new_langtags || !new_transkeys || !new_strings)
  {
    lodepng_free(new_keys);
    lodepng_free(new_langtags);
    lodepng_free(new_transkeys);
    lodepng_free(new_strings);
    return 83; /*alloc fail*/
  }

  info->itext_num++;
  info->itext_keys = new_keys;
  info->itext_langtags = new_langtags;
  info->itext_transkeys = new_transkeys;
  info->itext_strings = new_strings;

  string_init(&info->itext_keys[info->itext_num - 1]);
  string_set(&info->itext_keys[info->itext_num - 1], key);

  string_init(&info->itext_langtags[info->itext_num - 1]);
  string_set(&info->itext_langtags[info->itext_num - 1], langtag);

  string_init(&info->itext_transkeys[info->itext_num - 1]);
  string_set(&info->itext_transkeys[info->itext_num - 1], transkey);

  string_init(&info->itext_strings[info->itext_num - 1]);
  string_set(&info->itext_strings[info->itext_num - 1], str);

  return 0;
}
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

void lodepng_info_init(LodePNGInfo* info)
{
  lodepng_color_mode_init(&info->color);
  info->interlace_method = 0;
  info->compression_method = 0;
  info->filter_method = 0;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  info->background_defined = 0;
  info->background_r = info->background_g = info->background_b = 0;

  LodePNGText_init(info);
  LodePNGIText_init(info);

  info->time_defined = 0;
  info->phys_defined = 0;

  LodePNGUnknownChunks_init(info);
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
}

void lodepng_info_cleanup(LodePNGInfo* info)
{
  lodepng_color_mode_cleanup(&info->color);
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  LodePNGText_cleanup(info);
  LodePNGIText_cleanup(info);

  LodePNGUnknownChunks_cleanup(info);
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
}

unsigned lodepng_info_copy(LodePNGInfo* dest, const LodePNGInfo* source)
{
  lodepng_info_cleanup(dest);
  *dest = *source;
  lodepng_color_mode_init(&dest->color);
  CERROR_TRY_RETURN(lodepng_color_mode_copy(&dest->color, &source->color));

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  CERROR_TRY_RETURN(LodePNGText_copy(dest, source));
  CERROR_TRY_RETURN(LodePNGIText_copy(dest, source));

  LodePNGUnknownChunks_init(dest);
  CERROR_TRY_RETURN(LodePNGUnknownChunks_copy(dest, source));
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
  return 0;
}

void lodepng_info_swap(LodePNGInfo* a, LodePNGInfo* b)
{
  LodePNGInfo temp = *a;
  *a = *b;
  *b = temp;
}

/* ////////////////////////////////////////////////////////////////////////// */

/*index: bitgroup index, bits: bitgroup size(1, 2 or 4, in: bitgroup value, out: octet array to add bits to*/
static void addColorBits(unsigned char* out, size_t index, unsigned bits, unsigned in)
{
  /*p = the partial index in the byte, e.g. with 4 palettebits it is 0 for first half or 1 for second half*/
  unsigned p = index % (8 / bits);
  in &= (1 << bits) - 1; /*filter out any other bits of the input value*/
  in = in << (bits * (8 / bits - p - 1));
  if (p == 0) out[index * bits / 8] = (unsigned char)in;
  else out[index * bits / 8] |= in;
}

typedef struct ColorTree ColorTree;

/*
One node of a color tree
This is the data structure used to count the number of unique colors and to get a palette
index for a color. It's like an octree, but because the alpha channel is used too, each
node has 16 instead of 8 children.
*/
struct ColorTree
{
  ColorTree* children[16]; /*up to 16 pointers to ColorTree of next level*/
  int index; /*the payload. Only has a meaningful value if this is in the last level*/
};

static void color_tree_init(ColorTree* tree)
{
  int i;
  for(i = 0; i < 16; i++) tree->children[i] = 0;
  tree->index = -1;
}

static void color_tree_cleanup(ColorTree* tree)
{
  int i;
  for(i = 0; i < 16; i++)
  {
    if(tree->children[i])
    {
      color_tree_cleanup(tree->children[i]);
      lodepng_free(tree->children[i]);
    }
  }
}

/*returns -1 if color not present, its index otherwise*/
static int color_tree_get(ColorTree* tree, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  int bit = 0;
  for(bit = 0; bit < 8; bit++)
  {
    int i = 8 * ((r >> bit) & 1) + 4 * ((g >> bit) & 1) + 2 * ((b >> bit) & 1) + 1 * ((a >> bit) & 1);
    if(!tree->children[i]) return -1;
    else tree = tree->children[i];
  }
  return tree ? tree->index : -1;
}

#ifdef LODEPNG_COMPILE_ENCODER
static int color_tree_has(ColorTree* tree, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  return color_tree_get(tree, r, g, b, a) >= 0;
}
#endif /*LODEPNG_COMPILE_ENCODER*/

/*color is not allowed to already exist.
Index should be >= 0 (it's signed to be compatible with using -1 for "doesn't exist")*/
static void color_tree_add(ColorTree* tree,
                           unsigned char r, unsigned char g, unsigned char b, unsigned char a, int index)
{
  int bit;
  for(bit = 0; bit < 8; bit++)
  {
    int i = 8 * ((r >> bit) & 1) + 4 * ((g >> bit) & 1) + 2 * ((b >> bit) & 1) + 1 * ((a >> bit) & 1);
    if(!tree->children[i])
    {
      tree->children[i] = (ColorTree*)lodepng_malloc(sizeof(ColorTree));
      color_tree_init(tree->children[i]);
    }
    tree = tree->children[i];
  }
  tree->index = index;
}

/*put a pixel, given its RGBA color, into image of any color type*/
static unsigned rgba8ToPixel(unsigned char* out, size_t i,
                             const LodePNGColorMode* mode, ColorTree* tree /*for palette*/,
                             unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  if(mode->colortype == LCT_GREY)
  {
    unsigned char grey = r; /*((unsigned short)r + g + b) / 3*/;
    if(mode->bitdepth == 8) out[i] = grey;
    else if(mode->bitdepth == 16) out[i * 2 + 0] = out[i * 2 + 1] = grey;
    else
    {
      /*take the most significant bits of grey*/
      grey = (grey >> (8 - mode->bitdepth)) & ((1 << mode->bitdepth) - 1);
      addColorBits(out, i, mode->bitdepth, grey);
    }
  }
  else if(mode->colortype == LCT_RGB)
  {
    if(mode->bitdepth == 8)
    {
      out[i * 3 + 0] = r;
      out[i * 3 + 1] = g;
      out[i * 3 + 2] = b;
    }
    else
    {
      out[i * 6 + 0] = out[i * 6 + 1] = r;
      out[i * 6 + 2] = out[i * 6 + 3] = g;
      out[i * 6 + 4] = out[i * 6 + 5] = b;
    }
  }
  else if(mode->colortype == LCT_PALETTE)
  {
    int index = color_tree_get(tree, r, g, b, a);
    if(index < 0) return 82; /*color not in palette*/
	if (mode->bitdepth == 8) out[i] = (unsigned char)index;
    else addColorBits(out, i, mode->bitdepth, index);
  }
  else if(mode->colortype == LCT_GREY_ALPHA)
  {
    unsigned char grey = r; /*((unsigned short)r + g + b) / 3*/;
    if(mode->bitdepth == 8)
    {
      out[i * 2 + 0] = grey;
      out[i * 2 + 1] = a;
    }
    else if(mode->bitdepth == 16)
    {
      out[i * 4 + 0] = out[i * 4 + 1] = grey;
      out[i * 4 + 2] = out[i * 4 + 3] = a;
    }
  }
  else if(mode->colortype == LCT_RGBA)
  {
    if(mode->bitdepth == 8)
    {
      out[i * 4 + 0] = r;
      out[i * 4 + 1] = g;
      out[i * 4 + 2] = b;
      out[i * 4 + 3] = a;
    }
    else
    {
      out[i * 8 + 0] = out[i * 8 + 1] = r;
      out[i * 8 + 2] = out[i * 8 + 3] = g;
      out[i * 8 + 4] = out[i * 8 + 5] = b;
      out[i * 8 + 6] = out[i * 8 + 7] = a;
    }
  }

  return 0; /*no error*/
}

/*put a pixel, given its RGBA16 color, into image of any color 16-bitdepth type*/
static unsigned rgba16ToPixel(unsigned char* out, size_t i,
                              const LodePNGColorMode* mode,
                              unsigned short r, unsigned short g, unsigned short b, unsigned short a)
{
  if(mode->bitdepth != 16) return 85; /*must be 16 for this function*/
  if(mode->colortype == LCT_GREY)
  {
    unsigned short grey = r; /*((unsigned)r + g + b) / 3*/;
    out[i * 2 + 0] = (grey >> 8) & 255;
    out[i * 2 + 1] = grey & 255;
  }
  else if(mode->colortype == LCT_RGB)
  {
    out[i * 6 + 0] = (r >> 8) & 255;
    out[i * 6 + 1] = r & 255;
    out[i * 6 + 2] = (g >> 8) & 255;
    out[i * 6 + 3] = g & 255;
    out[i * 6 + 4] = (b >> 8) & 255;
    out[i * 6 + 5] = b & 255;
  }
  else if(mode->colortype == LCT_GREY_ALPHA)
  {
    unsigned short grey = r; /*((unsigned)r + g + b) / 3*/;
    out[i * 4 + 0] = (grey >> 8) & 255;
    out[i * 4 + 1] = grey & 255;
    out[i * 4 + 2] = (a >> 8) & 255;
    out[i * 4 + 3] = a & 255;
  }
  else if(mode->colortype == LCT_RGBA)
  {
    out[i * 8 + 0] = (r >> 8) & 255;
    out[i * 8 + 1] = r & 255;
    out[i * 8 + 2] = (g >> 8) & 255;
    out[i * 8 + 3] = g & 255;
    out[i * 8 + 4] = (b >> 8) & 255;
    out[i * 8 + 5] = b & 255;
    out[i * 8 + 6] = (a >> 8) & 255;
    out[i * 8 + 7] = a & 255;
  }

  return 0; /*no error*/
}

/*Get RGBA8 color of pixel with index i (y * width + x) from the raw image with given color type.*/
static unsigned getPixelColorRGBA8(unsigned char* r, unsigned char* g,
                                   unsigned char* b, unsigned char* a,
                                   const unsigned char* in, size_t i,
                                   const LodePNGColorMode* mode,
                                   unsigned fix_png)
{
  if(mode->colortype == LCT_GREY)
  {
    if(mode->bitdepth == 8)
    {
      *r = *g = *b = in[i];
      if(mode->key_defined && *r == mode->key_r) *a = 0;
      else *a = 255;
    }
    else if(mode->bitdepth == 16)
    {
      *r = *g = *b = in[i * 2 + 0];
      if(mode->key_defined && 256U * in[i * 2 + 0] + in[i * 2 + 1] == mode->key_r) *a = 0;
      else *a = 255;
    }
    else
    {
      unsigned highest = ((1U << mode->bitdepth) - 1U); /*highest possible value for this bit depth*/
      size_t j = i * mode->bitdepth;
      unsigned value = readBitsFromReversedStream(&j, in, mode->bitdepth);
	  *r = *g = *b = (unsigned char)((value * 255) / highest);
      if(mode->key_defined && value == mode->key_r) *a = 0;
      else *a = 255;
    }
  }
  else if(mode->colortype == LCT_RGB)
  {
    if(mode->bitdepth == 8)
    {
      *r = in[i * 3 + 0]; *g = in[i * 3 + 1]; *b = in[i * 3 + 2];
      if(mode->key_defined && *r == mode->key_r && *g == mode->key_g && *b == mode->key_b) *a = 0;
      else *a = 255;
    }
    else
    {
      *r = in[i * 6 + 0];
      *g = in[i * 6 + 2];
      *b = in[i * 6 + 4];
      if(mode->key_defined && 256U * in[i * 6 + 0] + in[i * 6 + 1] == mode->key_r
         && 256U * in[i * 6 + 2] + in[i * 6 + 3] == mode->key_g
         && 256U * in[i * 6 + 4] + in[i * 6 + 5] == mode->key_b) *a = 0;
      else *a = 255;
    }
  }
  else if(mode->colortype == LCT_PALETTE)
  {
    unsigned index;
    if(mode->bitdepth == 8) index = in[i];
    else
    {
      size_t j = i * mode->bitdepth;
      index = readBitsFromReversedStream(&j, in, mode->bitdepth);
    }

    if(index >= mode->palettesize)
    {
      /*This is an error according to the PNG spec, but fix_png can ignore it*/
      if(!fix_png) return (mode->bitdepth == 8 ? 46 : 47); /*index out of palette*/
      *r = *g = *b = 0;
      *a = 255;
    }
    else
    {
      *r = mode->palette[index * 4 + 0];
      *g = mode->palette[index * 4 + 1];
      *b = mode->palette[index * 4 + 2];
      *a = mode->palette[index * 4 + 3];
    }
  }
  else if(mode->colortype == LCT_GREY_ALPHA)
  {
    if(mode->bitdepth == 8)
    {
      *r = *g = *b = in[i * 2 + 0];
      *a = in[i * 2 + 1];
    }
    else
    {
      *r = *g = *b = in[i * 4 + 0];
      *a = in[i * 4 + 2];
    }
  }
  else if(mode->colortype == LCT_RGBA)
  {
    if(mode->bitdepth == 8)
    {
      *r = in[i * 4 + 0];
      *g = in[i * 4 + 1];
      *b = in[i * 4 + 2];
      *a = in[i * 4 + 3];
    }
    else
    {
      *r = in[i * 8 + 0];
      *g = in[i * 8 + 2];
      *b = in[i * 8 + 4];
      *a = in[i * 8 + 6];
    }
  }

  return 0; /*no error*/
}

/*Similar to getPixelColorRGBA8, but with all the for loops inside of the color
mode test cases, optimized to convert the colors much faster, when converting
to RGBA or RGB with 8 bit per cannel. buffer must be RGBA or RGB output with
enough memory, if has_alpha is true the output is RGBA. mode has the color mode
of the input buffer.*/
static unsigned getPixelColorsRGBA8(unsigned char* buffer, size_t numpixels,
                                    unsigned has_alpha, const unsigned char* in,
                                    const LodePNGColorMode* mode,
                                    unsigned fix_png)
{
  unsigned num_channels = has_alpha ? 4 : 3;
  size_t i;
  if(mode->colortype == LCT_GREY)
  {
    if(mode->bitdepth == 8)
    {
      for(i = 0; i < numpixels; i++, buffer += num_channels)
      {
        buffer[0] = buffer[1] = buffer[2] = in[i];
        if(has_alpha) buffer[3] = mode->key_defined && in[i] == mode->key_r ? 0 : 255;
      }
    }
    else if(mode->bitdepth == 16)
    {
      for(i = 0; i < numpixels; i++, buffer += num_channels)
      {
        buffer[0] = buffer[1] = buffer[2] = in[i * 2];
        if(has_alpha) buffer[3] = mode->key_defined && 256U * in[i * 2 + 0] + in[i * 2 + 1] == mode->key_r ? 0 : 255;
      }
    }
    else
    {
      unsigned highest = ((1U << mode->bitdepth) - 1U); /*highest possible value for this bit depth*/
      size_t j = 0;
      for(i = 0; i < numpixels; i++, buffer += num_channels)
      {
        unsigned value = readBitsFromReversedStream(&j, in, mode->bitdepth);
		buffer[0] = buffer[1] = buffer[2] = (unsigned char)((value * 255) / highest);
        if(has_alpha) buffer[3] = mode->key_defined && value == mode->key_r ? 0 : 255;
      }
    }
  }
  else if(mode->colortype == LCT_RGB)
  {
    if(mode->bitdepth == 8)
    {
      for(i = 0; i < numpixels; i++, buffer += num_channels)
      {
        buffer[0] = in[i * 3 + 0];
        buffer[1] = in[i * 3 + 1];
        buffer[2] = in[i * 3 + 2];
        if(has_alpha) buffer[3] = mode->key_defined && buffer[0] == mode->key_r
           && buffer[1]== mode->key_g && buffer[2] == mode->key_b ? 0 : 255;
      }
    }
    else
    {
      for(i = 0; i < numpixels; i++, buffer += num_channels)
      {
        buffer[0] = in[i * 6 + 0];
        buffer[1] = in[i * 6 + 2];
        buffer[2] = in[i * 6 + 4];
        if(has_alpha) buffer[3] = mode->key_defined
           && 256U * in[i * 6 + 0] + in[i * 6 + 1] == mode->key_r
           && 256U * in[i * 6 + 2] + in[i * 6 + 3] == mode->key_g
           && 256U * in[i * 6 + 4] + in[i * 6 + 5] == mode->key_b ? 0 : 255;
      }
    }
  }
  else if(mode->colortype == LCT_PALETTE)
  {
    unsigned index;
    size_t j = 0;
    for(i = 0; i < numpixels; i++, buffer += num_channels)
    {
      if(mode->bitdepth == 8) index = in[i];
      else index = readBitsFromReversedStream(&j, in, mode->bitdepth);

      if(index >= mode->palettesize)
      {
        /*This is an error according to the PNG spec, but fix_png can ignore it*/
        if(!fix_png) return (mode->bitdepth == 8 ? 46 : 47); /*index out of palette*/
        buffer[0] = buffer[1] = buffer[2] = 0;
        if(has_alpha) buffer[3] = 255;
      }
      else
      {
        buffer[0] = mode->palette[index * 4 + 0];
        buffer[1] = mode->palette[index * 4 + 1];
        buffer[2] = mode->palette[index * 4 + 2];
        if(has_alpha) buffer[3] = mode->palette[index * 4 + 3];
      }
    }
  }
  else if(mode->colortype == LCT_GREY_ALPHA)
  {
    if(mode->bitdepth == 8)
    {
      for(i = 0; i < numpixels; i++, buffer += num_channels)
      {
        buffer[0] = buffer[1] = buffer[2] = in[i * 2 + 0];
        if(has_alpha) buffer[3] = in[i * 2 + 1];
      }
    }
    else
    {
      for(i = 0; i < numpixels; i++, buffer += num_channels)
      {
        buffer[0] = buffer[1] = buffer[2] = in[i * 4 + 0];
        if(has_alpha) buffer[3] = in[i * 4 + 2];
      }
    }
  }
  else if(mode->colortype == LCT_RGBA)
  {
    if(mode->bitdepth == 8)
    {
      for(i = 0; i < numpixels; i++, buffer += num_channels)
      {
        buffer[0] = in[i * 4 + 0];
        buffer[1] = in[i * 4 + 1];
        buffer[2] = in[i * 4 + 2];
        if(has_alpha) buffer[3] = in[i * 4 + 3];
      }
    }
    else
    {
      for(i = 0; i < numpixels; i++, buffer += num_channels)
      {
        buffer[0] = in[i * 8 + 0];
        buffer[1] = in[i * 8 + 2];
        buffer[2] = in[i * 8 + 4];
        if(has_alpha) buffer[3] = in[i * 8 + 6];
      }
    }
  }

  return 0; /*no error*/
}

/*Get RGBA16 color of pixel with index i (y * width + x) from the raw image with
given color type, but the given color type must be 16-bit itself.*/
static unsigned getPixelColorRGBA16(unsigned short* r, unsigned short* g, unsigned short* b, unsigned short* a,
                                    const unsigned char* in, size_t i, const LodePNGColorMode* mode)
{
  if(mode->bitdepth != 16) return 85; /*error: this function only supports 16-bit input*/

  if(mode->colortype == LCT_GREY)
  {
    *r = *g = *b = 256 * in[i * 2 + 0] + in[i * 2 + 1];
    if(mode->key_defined && 256U * in[i * 2 + 0] + in[i * 2 + 1] == mode->key_r) *a = 0;
    else *a = 65535;
  }
  else if(mode->colortype == LCT_RGB)
  {
    *r = 256 * in[i * 6 + 0] + in[i * 6 + 1];
    *g = 256 * in[i * 6 + 2] + in[i * 6 + 3];
    *b = 256 * in[i * 6 + 4] + in[i * 6 + 5];
    if(mode->key_defined && 256U * in[i * 6 + 0] + in[i * 6 + 1] == mode->key_r
       && 256U * in[i * 6 + 2] + in[i * 6 + 3] == mode->key_g
       && 256U * in[i * 6 + 4] + in[i * 6 + 5] == mode->key_b) *a = 0;
    else *a = 65535;
  }
  else if(mode->colortype == LCT_GREY_ALPHA)
  {
    *r = *g = *b = 256 * in[i * 4 + 0] + in[i * 4 + 1];
    *a = 256 * in[i * 4 + 2] + in[i * 4 + 3];
  }
  else if(mode->colortype == LCT_RGBA)
  {
    *r = 256 * in[i * 8 + 0] + in[i * 8 + 1];
    *g = 256 * in[i * 8 + 2] + in[i * 8 + 3];
    *b = 256 * in[i * 8 + 4] + in[i * 8 + 5];
    *a = 256 * in[i * 8 + 6] + in[i * 8 + 7];
  }
  else return 85; /*error: this function only supports 16-bit input, not palettes*/

  return 0; /*no error*/
}

/*
converts from any color type to 24-bit or 32-bit (later maybe more supported). return value = LodePNG error code
the out buffer must have (w * h * bpp + 7) / 8 bytes, where bpp is the bits per pixel of the output color type
(lodepng_get_bpp) for < 8 bpp images, there may _not_ be padding bits at the end of scanlines.
*/
unsigned lodepng_convert(unsigned char* out, const unsigned char* in,
                         LodePNGColorMode* mode_out, const LodePNGColorMode* mode_in,
                         unsigned w, unsigned h, unsigned fix_png)
{
  unsigned error = 0;
  size_t i;
  ColorTree tree;
  size_t numpixels = w * h;

  if(lodepng_color_mode_equal(mode_out, mode_in))
  {
    size_t numbytes = lodepng_get_raw_size(w, h, mode_in);
    for(i = 0; i < numbytes; i++) out[i] = in[i];
    return error;
  }

  if(mode_out->colortype == LCT_PALETTE)
  {
    size_t palsize = 1 << mode_out->bitdepth;
    if(mode_out->palettesize < palsize) palsize = mode_out->palettesize;
    color_tree_init(&tree);
    for(i = 0; i < palsize; i++)
    {
      unsigned char* p = &mode_out->palette[i * 4];
	  color_tree_add(&tree, p[0], p[1], p[2], p[3], (unsigned int)i);
    }
  }

  if(mode_in->bitdepth == 16 && mode_out->bitdepth == 16)
  {
    for(i = 0; i < numpixels; i++)
    {
      unsigned short r = 0, g = 0, b = 0, a = 0;
      error = getPixelColorRGBA16(&r, &g, &b, &a, in, i, mode_in);
      if(error) break;
      error = rgba16ToPixel(out, i, mode_out, r, g, b, a);
      if(error) break;
    }
  }
  else if(mode_out->bitdepth == 8 && mode_out->colortype == LCT_RGBA)
  {
    error = getPixelColorsRGBA8(out, numpixels, 1, in, mode_in, fix_png);
  }
  else if(mode_out->bitdepth == 8 && mode_out->colortype == LCT_RGB)
  {
    error = getPixelColorsRGBA8(out, numpixels, 0, in, mode_in, fix_png);
  }
  else
  {
    unsigned char r = 0, g = 0, b = 0, a = 0;
    for(i = 0; i < numpixels; i++)
    {
      error = getPixelColorRGBA8(&r, &g, &b, &a, in, i, mode_in, fix_png);
      if(error) break;
      error = rgba8ToPixel(out, i, mode_out, &tree, r, g, b, a);
      if(error) break;
    }
  }

  if(mode_out->colortype == LCT_PALETTE)
  {
    color_tree_cleanup(&tree);
  }

  return error;
}

#ifdef LODEPNG_COMPILE_ENCODER

typedef struct ColorProfile
{
  unsigned char sixteenbit; /*needs more than 8 bits per channel*/
  unsigned char sixteenbit_done;


  unsigned char colored; /*not greyscale*/
  unsigned char colored_done;

  unsigned char key; /*a color key is required, or more*/
  unsigned short key_r; /*these values are always in 16-bit bitdepth in the profile*/
  unsigned short key_g;
  unsigned short key_b;
  unsigned char alpha; /*alpha channel, or alpha palette, required*/
  unsigned char alpha_done;

  unsigned numcolors;
  ColorTree tree; /*for listing the counted colors, up to 256*/
  unsigned char* palette; /*size 1024. Remember up to the first 256 RGBA colors*/
  unsigned maxnumcolors; /*if more than that amount counted*/
  unsigned char numcolors_done;

  unsigned greybits; /*amount of bits required for greyscale (1, 2, 4, 8). Does not take 16 bit into account.*/
  unsigned char greybits_done;

} ColorProfile;

static void color_profile_init(ColorProfile* profile, const LodePNGColorMode* mode)
{
  profile->sixteenbit = 0;
  profile->sixteenbit_done = mode->bitdepth == 16 ? 0 : 1;

  profile->colored = 0;
  profile->colored_done = lodepng_is_greyscale_type(mode) ? 1 : 0;

  profile->key = 0;
  profile->alpha = 0;
  profile->alpha_done = lodepng_can_have_alpha(mode) ? 0 : 1;

  profile->numcolors = 0;
  color_tree_init(&profile->tree);
  profile->palette = (unsigned char*)lodepng_malloc(1024);
  profile->maxnumcolors = 257;
  if(lodepng_get_bpp(mode) <= 8)
  {
    int bpp = lodepng_get_bpp(mode);
    profile->maxnumcolors = bpp == 1 ? 2 : (bpp == 2 ? 4 : (bpp == 4 ? 16 : 256));
  }
  profile->numcolors_done = 0;

  profile->greybits = 1;
  profile->greybits_done = lodepng_get_bpp(mode) == 1 ? 1 : 0;
}

static void color_profile_cleanup(ColorProfile* profile)
{
  color_tree_cleanup(&profile->tree);
  lodepng_free(profile->palette);
}

/*function used for debug purposes with C++*/
/*void printColorProfile(ColorProfile* p)
{
  std::cout << "sixteenbit: " << (int)p->sixteenbit << std::endl;
  std::cout << "sixteenbit_done: " << (int)p->sixteenbit_done << std::endl;
  std::cout << "colored: " << (int)p->colored << std::endl;
  std::cout << "colored_done: " << (int)p->colored_done << std::endl;
  std::cout << "key: " << (int)p->key << std::endl;
  std::cout << "key_r: " << (int)p->key_r << std::endl;
  std::cout << "key_g: " << (int)p->key_g << std::endl;
  std::cout << "key_b: " << (int)p->key_b << std::endl;
  std::cout << "alpha: " << (int)p->alpha << std::endl;
  std::cout << "alpha_done: " << (int)p->alpha_done << std::endl;
  std::cout << "numcolors: " << (int)p->numcolors << std::endl;
  std::cout << "maxnumcolors: " << (int)p->maxnumcolors << std::endl;
  std::cout << "numcolors_done: " << (int)p->numcolors_done << std::endl;
  std::cout << "greybits: " << (int)p->greybits << std::endl;
  std::cout << "greybits_done: " << (int)p->greybits_done << std::endl;
}*/

/*Returns how many bits needed to represent given value (max 8 bit)*/
unsigned getValueRequiredBits(unsigned short value)
{
  if(value == 0 || value == 255) return 1;
  /*The scaling of 2-bit and 4-bit values uses multiples of 85 and 17*/
  if(value % 17 == 0) return value % 85 == 0 ? 2 : 4;
  return 8;
}

/*profile must already have been inited with mode.
It's ok to set some parameters of profile to done already.*/
static unsigned get_color_profile(ColorProfile* profile,
                                  const unsigned char* in,
                                  size_t numpixels /*must be full image size, for certain filesize based choices*/,
                                  const LodePNGColorMode* mode,
                                  unsigned fix_png)
{
  unsigned error = 0;
  size_t i;

  if(mode->bitdepth == 16)
  {
    for(i = 0; i < numpixels; i++)
    {
      unsigned short r, g, b, a;
      error = getPixelColorRGBA16(&r, &g, &b, &a, in, i, mode);
      if(error) break;

      /*a color is considered good for 8-bit if the first byte and the second byte are equal,
        (so if it's divisible through 257), NOT necessarily if the second byte is 0*/
      if(!profile->sixteenbit_done
          && (((r & 255) != ((r >> 8) & 255))
           || ((g & 255) != ((g >> 8) & 255))
           || ((b & 255) != ((b >> 8) & 255))))
      {
        profile->sixteenbit = 1;
        profile->sixteenbit_done = 1;
        profile->greybits_done = 1; /*greybits is not applicable anymore at 16-bit*/
        profile->numcolors_done = 1; /*counting colors no longer useful, palette doesn't support 16-bit*/
      }

      if(!profile->colored_done && (r != g || r != b))
      {
        profile->colored = 1;
        profile->colored_done = 1;
        profile->greybits_done = 1; /*greybits is not applicable anymore*/
      }

      if(!profile->alpha_done && a != 65535)
      {
        /*only use color key if numpixels large enough to justify tRNS chunk size*/
        if(a == 0 && numpixels > 16 && !(profile->key && (r != profile->key_r || g != profile->key_g || b != profile->key_b)))
        {
          if(!profile->alpha && !profile->key)
          {
            profile->key = 1;
            profile->key_r = r;
            profile->key_g = g;
            profile->key_b = b;
          }
        }
        else
        {
          profile->alpha = 1;
          profile->alpha_done = 1;
          profile->greybits_done = 1; /*greybits is not applicable anymore*/
        }
      }

      /* Color key cannot be used if an opaque pixel also has that RGB color. */
      if(!profile->alpha_done && a == 65535 && profile->key
          && r == profile->key_r && g == profile->key_g && b == profile->key_b)
      {
          profile->alpha = 1;
          profile->alpha_done = 1;
          profile->greybits_done = 1; /*greybits is not applicable anymore*/
      }

      if(!profile->greybits_done)
      {
        /*assuming 8-bit r, this test does not care about 16-bit*/
        unsigned bits = getValueRequiredBits(r);
        if(bits > profile->greybits) profile->greybits = bits;
        if(profile->greybits >= 8) profile->greybits_done = 1;
      }

      if(!profile->numcolors_done)
      {
        /*assuming 8-bit rgba, this test does not care about 16-bit*/
        if(!color_tree_has(&profile->tree, (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a))
        {
          color_tree_add(&profile->tree, (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a,
            profile->numcolors);
          if(profile->numcolors < 256)
          {
            unsigned char* p = profile->palette;
            unsigned ni = profile->numcolors;
            p[ni * 4 + 0] = (unsigned char)r;
            p[ni * 4 + 1] = (unsigned char)g;
            p[ni * 4 + 2] = (unsigned char)b;
            p[ni * 4 + 3] = (unsigned char)a;
          }
          profile->numcolors++;
          if(profile->numcolors >= profile->maxnumcolors) profile->numcolors_done = 1;
        }
      }

      if(profile->alpha_done && profile->numcolors_done
      && profile->colored_done && profile->sixteenbit_done && profile->greybits_done)
      {
        break;
      }
    };
  }
  else /* < 16-bit */
  {
    for(i = 0; i < numpixels; i++)
    {
      unsigned char r = 0, g = 0, b = 0, a = 0;
      error = getPixelColorRGBA8(&r, &g, &b, &a, in, i, mode, fix_png);
      if(error) break;

      if(!profile->colored_done && (r != g || r != b))
      {
        profile->colored = 1;
        profile->colored_done = 1;
        profile->greybits_done = 1; /*greybits is not applicable anymore*/
      }

      if(!profile->alpha_done && a != 255)
      {
        if(a == 0 && !(profile->key && (r != profile->key_r || g != profile->key_g || b != profile->key_b)))
        {
          if(!profile->key)
          {
            profile->key = 1;
            profile->key_r = r;
            profile->key_g = g;
            profile->key_b = b;
          }
        }
        else
        {
          profile->alpha = 1;
          profile->alpha_done = 1;
          profile->greybits_done = 1; /*greybits is not applicable anymore*/
        }
      }

      /* Color key cannot be used if an opaque pixel also has that RGB color. */
      if(!profile->alpha_done && a == 255 && profile->key
          && r == profile->key_r && g == profile->key_g && b == profile->key_b)
      {
          profile->alpha = 1;
          profile->alpha_done = 1;
          profile->greybits_done = 1; /*greybits is not applicable anymore*/
      }

      if(!profile->greybits_done)
      {
        unsigned bits = getValueRequiredBits(r);
        if(bits > profile->greybits) profile->greybits = bits;
        if(profile->greybits >= 8) profile->greybits_done = 1;
      }

      if(!profile->numcolors_done)
      {
        if(!color_tree_has(&profile->tree, r, g, b, a))
        {

          color_tree_add(&profile->tree, r, g, b, a, profile->numcolors);
          if(profile->numcolors < 256)
          {
            unsigned char* p = profile->palette;
            unsigned ni = profile->numcolors;
            p[ni * 4 + 0] = r;
            p[ni * 4 + 1] = g;
            p[ni * 4 + 2] = b;
            p[ni * 4 + 3] = a;
          }
          profile->numcolors++;
          if(profile->numcolors >= profile->maxnumcolors) profile->numcolors_done = 1;
        }
      }

      if(profile->alpha_done && profile->numcolors_done && profile->colored_done && profile->greybits_done)
      {
        break;
      }
    };
  }

  /*make the profile's key always 16-bit for consistency*/
  if(mode->bitdepth < 16)
  {
    /*repeat each byte twice*/
    profile->key_r *= 257;
    profile->key_g *= 257;
    profile->key_b *= 257;
  }

  return error;
}

static void setColorKeyFrom16bit(LodePNGColorMode* mode_out, unsigned r, unsigned g, unsigned b, unsigned bitdepth)
{
  unsigned mask = (1 << bitdepth) - 1;
  mode_out->key_defined = 1;
  mode_out->key_r = r & mask;
  mode_out->key_g = g & mask;
  mode_out->key_b = b & mask;
}

/*updates values of mode with a potentially smaller color model. mode_out should
contain the user chosen color model, but will be overwritten with the new chosen one.*/
unsigned lodepng_auto_choose_color(LodePNGColorMode* mode_out,
                                   const unsigned char* image, unsigned w, unsigned h,
                                   const LodePNGColorMode* mode_in,
                                   LodePNGAutoConvert auto_convert)
{
  ColorProfile profile;
  unsigned error = 0;
  int no_nibbles = auto_convert == LAC_AUTO_NO_NIBBLES || auto_convert == LAC_AUTO_NO_NIBBLES_NO_PALETTE;
  int no_palette = auto_convert == LAC_AUTO_NO_PALETTE || auto_convert == LAC_AUTO_NO_NIBBLES_NO_PALETTE;

  if(auto_convert == LAC_ALPHA)
  {
    if(mode_out->colortype != LCT_RGBA && mode_out->colortype != LCT_GREY_ALPHA) return 0;
  }

  color_profile_init(&profile, mode_in);
  if(auto_convert == LAC_ALPHA)
  {
    profile.colored_done = 1;
    profile.greybits_done = 1;
    profile.numcolors_done = 1;
    profile.sixteenbit_done = 1;
  }
  error = get_color_profile(&profile, image, w * h, mode_in, 0 /*fix_png*/);
  if(!error && auto_convert == LAC_ALPHA)
  {
    if(!profile.alpha)
    {
      mode_out->colortype = (mode_out->colortype == LCT_RGBA ? LCT_RGB : LCT_GREY);
      if(profile.key) setColorKeyFrom16bit(mode_out, profile.key_r, profile.key_g, profile.key_b, mode_out->bitdepth);
    }
  }
  else if(!error && auto_convert != LAC_ALPHA)
  {
    mode_out->key_defined = 0;

    if(profile.sixteenbit)
    {
      mode_out->bitdepth = 16;
      if(profile.alpha)
      {
        mode_out->colortype = profile.colored ? LCT_RGBA : LCT_GREY_ALPHA;
      }
      else
      {
        mode_out->colortype = profile.colored ? LCT_RGB : LCT_GREY;
        if(profile.key) setColorKeyFrom16bit(mode_out, profile.key_r, profile.key_g, profile.key_b, mode_out->bitdepth);
      }
    }
    else /*less than 16 bits per channel*/
    {
      /*don't add palette overhead if image hasn't got a lot of pixels*/
      unsigned n = profile.numcolors;
      int palette_ok = !no_palette && n <= 256 && (n * 2 < w * h);
      unsigned palettebits = n <= 2 ? 1 : (n <= 4 ? 2 : (n <= 16 ? 4 : 8));
      int grey_ok = !profile.colored && !profile.alpha; /*grey without alpha, with potentially low bits*/
      if(palette_ok || grey_ok)
      {
        if(!palette_ok || (grey_ok && profile.greybits <= palettebits))
        {
          unsigned grey = profile.key_r;
          mode_out->colortype = LCT_GREY;
          mode_out->bitdepth = profile.greybits;
          if(profile.key) setColorKeyFrom16bit(mode_out, grey, grey, grey, mode_out->bitdepth);
        }
        else
        {
          /*fill in the palette*/
          unsigned i;
          unsigned char* p = profile.palette;
          /*remove potential earlier palette*/
          lodepng_palette_clear(mode_out);
          for(i = 0; i < profile.numcolors; i++)
          {
            error = lodepng_palette_add(mode_out, p[i * 4 + 0], p[i * 4 + 1], p[i * 4 + 2], p[i * 4 + 3]);
            if(error) break;
          }

          mode_out->colortype = LCT_PALETTE;
          mode_out->bitdepth = palettebits;
        }
      }
      else /*8-bit per channel*/
      {
        mode_out->bitdepth = 8;
        if(profile.alpha)
        {
          mode_out->colortype = profile.colored ? LCT_RGBA : LCT_GREY_ALPHA;
        }
        else
        {
          mode_out->colortype = profile.colored ? LCT_RGB : LCT_GREY /*LCT_GREY normally won't occur, already done earlier*/;
          if(profile.key) setColorKeyFrom16bit(mode_out, profile.key_r, profile.key_g, profile.key_b, mode_out->bitdepth);
        }
      }
    }
  }

  color_profile_cleanup(&profile);

  if(mode_out->colortype == LCT_PALETTE && mode_in->palettesize == mode_out->palettesize)
  {
    /*In this case keep the palette order of the input, so that the user can choose an optimal one*/
    size_t i;
    for(i = 0; i < mode_in->palettesize * 4; i++)
    {
      mode_out->palette[i] = mode_in->palette[i];
    }
  }

  if(no_nibbles && mode_out->bitdepth < 8)
  {
    /*palette can keep its small amount of colors, as long as no indices use it*/
    mode_out->bitdepth = 8;
  }

  return error;
}

#endif /* #ifdef LODEPNG_COMPILE_ENCODER */

/*
Paeth predicter, used by PNG filter type 4
The parameters are of type short, but should come from unsigned chars, the shorts
are only needed to make the paeth calculation correct.
*/
static unsigned char paethPredictor(short a, short b, short c)
{
  short pa = (short)abs(b - c);
  short pb = (short)abs(a - c);
  short pc = (short)abs(a + b - c - c);

  if(pc < pa && pc < pb) return (unsigned char)c;
  else if(pb < pa) return (unsigned char)b;
  else return (unsigned char)a;
}

/*shared values used by multiple Adam7 related functions*/

static const unsigned ADAM7_IX[7] = { 0, 4, 0, 2, 0, 1, 0 }; /*x start values*/
static const unsigned ADAM7_IY[7] = { 0, 0, 4, 0, 2, 0, 1 }; /*y start values*/
static const unsigned ADAM7_DX[7] = { 8, 8, 4, 4, 2, 2, 1 }; /*x delta values*/
static const unsigned ADAM7_DY[7] = { 8, 8, 8, 4, 4, 2, 2 }; /*y delta values*/

/*
Outputs various dimensions and positions in the image related to the Adam7 reduced images.
passw: output containing the width of the 7 passes
passh: output containing the height of the 7 passes
filter_passstart: output containing the index of the start and end of each
 reduced image with filter bytes
padded_passstart output containing the index of the start and end of each
 reduced image when without filter bytes but with padded scanlines
passstart: output containing the index of the start and end of each reduced
 image without padding between scanlines, but still padding between the images
w, h: width and height of non-interlaced image
bpp: bits per pixel
"padded" is only relevant if bpp is less than 8 and a scanline or image does not
 end at a full byte
*/
static void Adam7_getpassvalues(unsigned passw[7], unsigned passh[7], size_t filter_passstart[8],
                                size_t padded_passstart[8], size_t passstart[8], unsigned w, unsigned h, unsigned bpp)
{
  /*the passstart values have 8 values: the 8th one indicates the byte after the end of the 7th (= last) pass*/
  unsigned i;

  /*calculate width and height in pixels of each pass*/
  for(i = 0; i < 7; i++)
  {
    passw[i] = (w + ADAM7_DX[i] - ADAM7_IX[i] - 1) / ADAM7_DX[i];
    passh[i] = (h + ADAM7_DY[i] - ADAM7_IY[i] - 1) / ADAM7_DY[i];
    if(passw[i] == 0) passh[i] = 0;
    if(passh[i] == 0) passw[i] = 0;
  }

  filter_passstart[0] = padded_passstart[0] = passstart[0] = 0;
  for(i = 0; i < 7; i++)
  {
    /*if passw[i] is 0, it's 0 bytes, not 1 (no filtertype-byte)*/
    filter_passstart[i + 1] = filter_passstart[i]
                            + ((passw[i] && passh[i]) ? passh[i] * (1 + (passw[i] * bpp + 7) / 8) : 0);
    /*bits padded if needed to fill full byte at end of each scanline*/
    padded_passstart[i + 1] = padded_passstart[i] + passh[i] * ((passw[i] * bpp + 7) / 8);
    /*only padded at end of reduced image*/
    passstart[i + 1] = passstart[i] + (passh[i] * passw[i] * bpp + 7) / 8;
  }
}

#ifdef LODEPNG_COMPILE_DECODER

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG Decoder                                                            / */
/* ////////////////////////////////////////////////////////////////////////// */

/*read the information from the header and store it in the LodePNGInfo. return value is error*/
unsigned lodepng_inspect(unsigned* w, unsigned* h, LodePNGState* state,
                         const unsigned char* in, size_t insize)
{
  LodePNGInfo* info = &state->info_png;
  if(insize == 0 || in == 0)
  {
    CERROR_RETURN_ERROR(state->error, 48); /*error: the given data is empty*/
  }
  if(insize < 29)
  {
    CERROR_RETURN_ERROR(state->error, 27); /*error: the data length is smaller than the length of a PNG header*/
  }

  /*when decoding a new PNG image, make sure all parameters created after previous decoding are reset*/
  lodepng_info_cleanup(info);
  lodepng_info_init(info);

  if(in[0] != 137 || in[1] != 80 || in[2] != 78 || in[3] != 71
     || in[4] != 13 || in[5] != 10 || in[6] != 26 || in[7] != 10)
  {
    CERROR_RETURN_ERROR(state->error, 28); /*error: the first 8 bytes are not the correct PNG signature*/
  }
  if(in[12] != 'I' || in[13] != 'H' || in[14] != 'D' || in[15] != 'R')
  {
    CERROR_RETURN_ERROR(state->error, 29); /*error: it doesn't start with a IHDR chunk!*/
  }

  /*read the values given in the header*/
  *w = lodepng_read32bitInt(&in[16]);
  *h = lodepng_read32bitInt(&in[20]);
  info->color.bitdepth = in[24];
  info->color.colortype = (LodePNGColorType)in[25];
  info->compression_method = in[26];
  info->filter_method = in[27];
  info->interlace_method = in[28];

  if(!state->decoder.ignore_crc)
  {
    unsigned CRC = lodepng_read32bitInt(&in[29]);
    unsigned checksum = lodepng_crc32(&in[12], 17);
    if(CRC != checksum)
    {
      CERROR_RETURN_ERROR(state->error, 57); /*invalid CRC*/
    }
  }

  /*error: only compression method 0 is allowed in the specification*/
  if(info->compression_method != 0) CERROR_RETURN_ERROR(state->error, 32);
  /*error: only filter method 0 is allowed in the specification*/
  if(info->filter_method != 0) CERROR_RETURN_ERROR(state->error, 33);
  /*error: only interlace methods 0 and 1 exist in the specification*/
  if(info->interlace_method > 1) CERROR_RETURN_ERROR(state->error, 34);

  state->error = checkColorValidity(info->color.colortype, info->color.bitdepth);
  return state->error;
}

static unsigned unfilterScanline(unsigned char* recon, const unsigned char* scanline, const unsigned char* precon,
                                 size_t bytewidth, unsigned char filterType, size_t length)
{
  /*
  For PNG filter method 0
  unfilter a PNG image scanline by scanline. when the pixels are smaller than 1 byte,
  the filter works byte per byte (bytewidth = 1)
  precon is the previous unfiltered scanline, recon the result, scanline the current one
  the incoming scanlines do NOT include the filtertype byte, that one is given in the parameter filterType instead
  recon and scanline MAY be the same memory address! precon must be disjoint.
  */

  size_t i;
  switch(filterType)
  {
    case 0:
      for(i = 0; i < length; i++) recon[i] = scanline[i];
      break;
    case 1:
      for(i = 0; i < bytewidth; i++) recon[i] = scanline[i];
      for(i = bytewidth; i < length; i++) recon[i] = scanline[i] + recon[i - bytewidth];
      break;
    case 2:
      if(precon)
      {
        for(i = 0; i < length; i++) recon[i] = scanline[i] + precon[i];
      }
      else
      {
        for(i = 0; i < length; i++) recon[i] = scanline[i];
      }
      break;
    case 3:
      if(precon)
      {
        for(i = 0; i < bytewidth; i++) recon[i] = scanline[i] + precon[i] / 2;
        for(i = bytewidth; i < length; i++) recon[i] = scanline[i] + ((recon[i - bytewidth] + precon[i]) / 2);
      }
      else
      {
        for(i = 0; i < bytewidth; i++) recon[i] = scanline[i];
        for(i = bytewidth; i < length; i++) recon[i] = scanline[i] + recon[i - bytewidth] / 2;
      }
      break;
    case 4:
      if(precon)
      {
        for(i = 0; i < bytewidth; i++)
        {
          recon[i] = (scanline[i] + precon[i]); /*paethPredictor(0, precon[i], 0) is always precon[i]*/
        }
        for(i = bytewidth; i < length; i++)
        {
          recon[i] = (scanline[i] + paethPredictor(recon[i - bytewidth], precon[i], precon[i - bytewidth]));
        }
      }
      else
      {
        for(i = 0; i < bytewidth; i++)
        {
          recon[i] = scanline[i];
        }
        for(i = bytewidth; i < length; i++)
        {
          /*paethPredictor(recon[i - bytewidth], 0, 0) is always recon[i - bytewidth]*/
          recon[i] = (scanline[i] + recon[i - bytewidth]);
        }
      }
      break;
    default: return 36; /*error: unexisting filter type given*/
  }
  return 0;
}

static unsigned unfilter(unsigned char* out, const unsigned char* in, unsigned w, unsigned h, unsigned bpp)
{
  /*
  For PNG filter method 0
  this function unfilters a single image (e.g. without interlacing this is called once, with Adam7 seven times)
  out must have enough bytes allocated already, in must have the scanlines + 1 filtertype byte per scanline
  w and h are image dimensions or dimensions of reduced image, bpp is bits per pixel
  in and out are allowed to be the same memory address (but aren't the same size since in has the extra filter bytes)
  */

  unsigned y;
  unsigned char* prevline = 0;

  /*bytewidth is used for filtering, is 1 when bpp < 8, number of bytes per pixel otherwise*/
  size_t bytewidth = (bpp + 7) / 8;
  size_t linebytes = (w * bpp + 7) / 8;

  for(y = 0; y < h; y++)
  {
    size_t outindex = linebytes * y;
    size_t inindex = (1 + linebytes) * y; /*the extra filterbyte added to each row*/
    unsigned char filterType = in[inindex];

    CERROR_TRY_RETURN(unfilterScanline(&out[outindex], &in[inindex + 1], prevline, bytewidth, filterType, linebytes));

    prevline = &out[outindex];
  }

  return 0;
}

/*
in: Adam7 interlaced image, with no padding bits between scanlines, but between
 reduced images so that each reduced image starts at a byte.
out: the same pixels, but re-ordered so that they're now a non-interlaced image with size w*h
bpp: bits per pixel
out has the following size in bits: w * h * bpp.
in is possibly bigger due to padding bits between reduced images.
out must be big enough AND must be 0 everywhere if bpp < 8 in the current implementation
(because that's likely a little bit faster)
NOTE: comments about padding bits are only relevant if bpp < 8
*/
static void Adam7_deinterlace(unsigned char* out, const unsigned char* in, unsigned w, unsigned h, unsigned bpp)
{
  unsigned passw[7], passh[7];
  size_t filter_passstart[8], padded_passstart[8], passstart[8];
  unsigned i;

  Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

  if(bpp >= 8)
  {
    for(i = 0; i < 7; i++)
    {
      unsigned x, y, b;
      size_t bytewidth = bpp / 8;
      for(y = 0; y < passh[i]; y++)
      for(x = 0; x < passw[i]; x++)
      {
        size_t pixelinstart = passstart[i] + (y * passw[i] + x) * bytewidth;
        size_t pixeloutstart = ((ADAM7_IY[i] + y * ADAM7_DY[i]) * w + ADAM7_IX[i] + x * ADAM7_DX[i]) * bytewidth;
        for(b = 0; b < bytewidth; b++)
        {
          out[pixeloutstart + b] = in[pixelinstart + b];
        }
      }
    }
  }
  else /*bpp < 8: Adam7 with pixels < 8 bit is a bit trickier: with bit pointers*/
  {
    for(i = 0; i < 7; i++)
    {
      unsigned x, y, b;
      unsigned ilinebits = bpp * passw[i];
      unsigned olinebits = bpp * w;
      size_t obp, ibp; /*bit pointers (for out and in buffer)*/
      for(y = 0; y < passh[i]; y++)
      for(x = 0; x < passw[i]; x++)
      {
        ibp = (8 * passstart[i]) + (y * ilinebits + x * bpp);
        obp = (ADAM7_IY[i] + y * ADAM7_DY[i]) * olinebits + (ADAM7_IX[i] + x * ADAM7_DX[i]) * bpp;
        for(b = 0; b < bpp; b++)
        {
          unsigned char bit = readBitFromReversedStream(&ibp, in);
          /*note that this function assumes the out buffer is completely 0, use setBitOfReversedStream otherwise*/
          setBitOfReversedStream0(&obp, out, bit);
        }
      }
    }
  }
}

static void removePaddingBits(unsigned char* out, const unsigned char* in,
                              size_t olinebits, size_t ilinebits, unsigned h)
{
  /*
  After filtering there are still padding bits if scanlines have non multiple of 8 bit amounts. They need
  to be removed (except at last scanline of (Adam7-reduced) image) before working with pure image buffers
  for the Adam7 code, the color convert code and the output to the user.
  in and out are allowed to be the same buffer, in may also be higher but still overlapping; in must
  have >= ilinebits*h bits, out must have >= olinebits*h bits, olinebits must be <= ilinebits
  also used to move bits after earlier such operations happened, e.g. in a sequence of reduced images from Adam7
  only useful if (ilinebits - olinebits) is a value in the range 1..7
  */
  unsigned y;
  size_t diff = ilinebits - olinebits;
  size_t ibp = 0, obp = 0; /*input and output bit pointers*/
  for(y = 0; y < h; y++)
  {
    size_t x;
    for(x = 0; x < olinebits; x++)
    {
      unsigned char bit = readBitFromReversedStream(&ibp, in);
      setBitOfReversedStream(&obp, out, bit);
    }
    ibp += diff;
  }
}

/*out must be buffer big enough to contain full image, and in must contain the full decompressed data from
the IDAT chunks (with filter index bytes and possible padding bits)
return value is error*/
static unsigned postProcessScanlines(unsigned char* out, unsigned char* in,
                                     unsigned w, unsigned h, const LodePNGInfo* info_png)
{
  /*
  This function converts the filtered-padded-interlaced data into pure 2D image buffer with the PNG's colortype.
  Steps:
  *) if no Adam7: 1) unfilter 2) remove padding bits (= posible extra bits per scanline if bpp < 8)
  *) if adam7: 1) 7x unfilter 2) 7x remove padding bits 3) Adam7_deinterlace
  NOTE: the in buffer will be overwritten with intermediate data!
  */
  unsigned bpp = lodepng_get_bpp(&info_png->color);
  if(bpp == 0) return 31; /*error: invalid colortype*/

  if(info_png->interlace_method == 0)
  {
    if(bpp < 8 && w * bpp != ((w * bpp + 7) / 8) * 8)
    {
      CERROR_TRY_RETURN(unfilter(in, in, w, h, bpp));
      removePaddingBits(out, in, w * bpp, ((w * bpp + 7) / 8) * 8, h);
    }
    /*we can immediatly filter into the out buffer, no other steps needed*/
    else CERROR_TRY_RETURN(unfilter(out, in, w, h, bpp));
  }
  else /*interlace_method is 1 (Adam7)*/
  {
    unsigned passw[7], passh[7]; size_t filter_passstart[8], padded_passstart[8], passstart[8];
    unsigned i;

    Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

    for(i = 0; i < 7; i++)
    {
      CERROR_TRY_RETURN(unfilter(&in[padded_passstart[i]], &in[filter_passstart[i]], passw[i], passh[i], bpp));
      /*TODO: possible efficiency improvement: if in this reduced image the bits fit nicely in 1 scanline,
      move bytes instead of bits or move not at all*/
      if(bpp < 8)
      {
        /*remove padding bits in scanlines; after this there still may be padding
        bits between the different reduced images: each reduced image still starts nicely at a byte*/
        removePaddingBits(&in[passstart[i]], &in[padded_passstart[i]], passw[i] * bpp,
                          ((passw[i] * bpp + 7) / 8) * 8, passh[i]);
      }
    }

    Adam7_deinterlace(out, in, w, h, bpp);
  }

  return 0;
}

static unsigned readChunk_PLTE(LodePNGColorMode* color, const unsigned char* data, size_t chunkLength)
{
  unsigned pos = 0, i;
  if(color->palette) lodepng_free(color->palette);
  color->palettesize = chunkLength / 3;
  color->palette = (unsigned char*)lodepng_malloc(4 * color->palettesize);
  if(!color->palette && color->palettesize)
  {
    color->palettesize = 0;
    return 83; /*alloc fail*/
  }
  if(color->palettesize > 256) return 38; /*error: palette too big*/

  for(i = 0; i < color->palettesize; i++)
  {
    color->palette[4 * i + 0] = data[pos++]; /*R*/
    color->palette[4 * i + 1] = data[pos++]; /*G*/
    color->palette[4 * i + 2] = data[pos++]; /*B*/
    color->palette[4 * i + 3] = 255; /*alpha*/
  }

  return 0; /* OK */
}

static unsigned readChunk_tRNS(LodePNGColorMode* color, const unsigned char* data, size_t chunkLength)
{
  unsigned i;
  if(color->colortype == LCT_PALETTE)
  {
    /*error: more alpha values given than there are palette entries*/
    if(chunkLength > color->palettesize) return 38;

    for(i = 0; i < chunkLength; i++) color->palette[4 * i + 3] = data[i];
  }
  else if(color->colortype == LCT_GREY)
  {
    /*error: this chunk must be 2 bytes for greyscale image*/
    if(chunkLength != 2) return 30;

    color->key_defined = 1;
    color->key_r = color->key_g = color->key_b = 256 * data[0] + data[1];
  }
  else if(color->colortype == LCT_RGB)
  {
    /*error: this chunk must be 6 bytes for RGB image*/
    if(chunkLength != 6) return 41;

    color->key_defined = 1;
    color->key_r = 256 * data[0] + data[1];
    color->key_g = 256 * data[2] + data[3];
    color->key_b = 256 * data[4] + data[5];
  }
  else return 42; /*error: tRNS chunk not allowed for other color models*/

  return 0; /* OK */
}


#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
/*background color chunk (bKGD)*/
static unsigned readChunk_bKGD(LodePNGInfo* info, const unsigned char* data, size_t chunkLength)
{
  if(info->color.colortype == LCT_PALETTE)
  {
    /*error: this chunk must be 1 byte for indexed color image*/
    if(chunkLength != 1) return 43;

    info->background_defined = 1;
    info->background_r = info->background_g = info->background_b = data[0];
  }
  else if(info->color.colortype == LCT_GREY || info->color.colortype == LCT_GREY_ALPHA)
  {
    /*error: this chunk must be 2 bytes for greyscale image*/
    if(chunkLength != 2) return 44;

    info->background_defined = 1;
    info->background_r = info->background_g = info->background_b
                                 = 256 * data[0] + data[1];
  }
  else if(info->color.colortype == LCT_RGB || info->color.colortype == LCT_RGBA)
  {
    /*error: this chunk must be 6 bytes for greyscale image*/
    if(chunkLength != 6) return 45;

    info->background_defined = 1;
    info->background_r = 256 * data[0] + data[1];
    info->background_g = 256 * data[2] + data[3];
    info->background_b = 256 * data[4] + data[5];
  }

  return 0; /* OK */
}

/*text chunk (tEXt)*/
static unsigned readChunk_tEXt(LodePNGInfo* info, const unsigned char* data, size_t chunkLength)
{
  unsigned error = 0;
  char *key = 0, *str = 0;
  unsigned i;

  while(!error) /*not really a while loop, only used to break on error*/
  {
    unsigned length, string2_begin;

    length = 0;
    while(length < chunkLength && data[length] != 0) length++;
    /*even though it's not allowed by the standard, no error is thrown if
    there's no null termination char, if the text is empty*/
    if(length < 1 || length > 79) CERROR_BREAK(error, 89); /*keyword too short or long*/

    key = (char*)lodepng_malloc(length + 1);
    if(!key) CERROR_BREAK(error, 83); /*alloc fail*/

    key[length] = 0;
    for(i = 0; i < length; i++) key[i] = data[i];

    string2_begin = length + 1; /*skip keyword null terminator*/

	length = chunkLength < string2_begin ? 0 : (unsigned int)chunkLength - string2_begin;
    str = (char*)lodepng_malloc(length + 1);
    if(!str) CERROR_BREAK(error, 83); /*alloc fail*/

    str[length] = 0;
    for(i = 0; i < length; i++) str[i] = data[string2_begin + i];

    error = lodepng_add_text(info, key, str);

    break;
  }

  lodepng_free(key);
  lodepng_free(str);

  return error;
}

/*compressed text chunk (zTXt)*/
static unsigned readChunk_zTXt(LodePNGInfo* info, const LodePNGDecompressSettings* zlibsettings,
                               const unsigned char* data, size_t chunkLength)
{
  unsigned error = 0;
  unsigned i;

  unsigned length, string2_begin;
  char *key = 0;
  ucvector decoded;

  ucvector_init(&decoded);

  while(!error) /*not really a while loop, only used to break on error*/
  {
    for(length = 0; length < chunkLength && data[length] != 0; length++) ;
    if(length + 2 >= chunkLength) CERROR_BREAK(error, 75); /*no null termination, corrupt?*/
    if(length < 1 || length > 79) CERROR_BREAK(error, 89); /*keyword too short or long*/

    key = (char*)lodepng_malloc(length + 1);
    if(!key) CERROR_BREAK(error, 83); /*alloc fail*/

    key[length] = 0;
    for(i = 0; i < length; i++) key[i] = data[i];

    if(data[length + 1] != 0) CERROR_BREAK(error, 72); /*the 0 byte indicating compression must be 0*/

    string2_begin = length + 2;
    if(string2_begin > chunkLength) CERROR_BREAK(error, 75); /*no null termination, corrupt?*/

	length = (unsigned int)chunkLength - string2_begin;
    /*will fail if zlib error, e.g. if length is too small*/
    error = zlib_decompress(&decoded.data, &decoded.size,
                            (unsigned char*)(&data[string2_begin]),
                            length, zlibsettings);
    if(error) break;
    ucvector_push_back(&decoded, 0);

    error = lodepng_add_text(info, key, (char*)decoded.data);

    break;
  }

  lodepng_free(key);
  ucvector_cleanup(&decoded);

  return error;
}

/*international text chunk (iTXt)*/
static unsigned readChunk_iTXt(LodePNGInfo* info, const LodePNGDecompressSettings* zlibsettings,
                               const unsigned char* data, size_t chunkLength)
{
  unsigned error = 0;
  unsigned i;

  unsigned length, begin, compressed;
  char *key = 0, *langtag = 0, *transkey = 0;
  ucvector decoded;
  ucvector_init(&decoded);

  while(!error) /*not really a while loop, only used to break on error*/
  {
    /*Quick check if the chunk length isn't too small. Even without check
    it'd still fail with other error checks below if it's too short. This just gives a different error code.*/
    if(chunkLength < 5) CERROR_BREAK(error, 30); /*iTXt chunk too short*/

    /*read the key*/
    for(length = 0; length < chunkLength && data[length] != 0; length++) ;
    if(length + 3 >= chunkLength) CERROR_BREAK(error, 75); /*no null termination char, corrupt?*/
    if(length < 1 || length > 79) CERROR_BREAK(error, 89); /*keyword too short or long*/

    key = (char*)lodepng_malloc(length + 1);
    if(!key) CERROR_BREAK(error, 83); /*alloc fail*/

    key[length] = 0;
    for(i = 0; i < length; i++) key[i] = data[i];

    /*read the compression method*/
    compressed = data[length + 1];
    if(data[length + 2] != 0) CERROR_BREAK(error, 72); /*the 0 byte indicating compression must be 0*/

    /*even though it's not allowed by the standard, no error is thrown if
    there's no null termination char, if the text is empty for the next 3 texts*/

    /*read the langtag*/
    begin = length + 3;
    length = 0;
    for(i = begin; i < chunkLength && data[i] != 0; i++) length++;

    langtag = (char*)lodepng_malloc(length + 1);
    if(!langtag) CERROR_BREAK(error, 83); /*alloc fail*/

    langtag[length] = 0;
    for(i = 0; i < length; i++) langtag[i] = data[begin + i];

    /*read the transkey*/
    begin += length + 1;
    length = 0;
    for(i = begin; i < chunkLength && data[i] != 0; i++) length++;

    transkey = (char*)lodepng_malloc(length + 1);
    if(!transkey) CERROR_BREAK(error, 83); /*alloc fail*/

    transkey[length] = 0;
    for(i = 0; i < length; i++) transkey[i] = data[begin + i];

    /*read the actual text*/
    begin += length + 1;

	length = chunkLength < begin ? 0 : (unsigned int)chunkLength - begin;

    if(compressed)
    {
      /*will fail if zlib error, e.g. if length is too small*/
      error = zlib_decompress(&decoded.data, &decoded.size,
                              (unsigned char*)(&data[begin]),
                              length, zlibsettings);
      if(error) break;
      if(decoded.allocsize < decoded.size) decoded.allocsize = decoded.size;
      ucvector_push_back(&decoded, 0);
    }
    else
    {
      if(!ucvector_resize(&decoded, length + 1)) CERROR_BREAK(error, 83 /*alloc fail*/);

      decoded.data[length] = 0;
      for(i = 0; i < length; i++) decoded.data[i] = data[begin + i];
    }

    error = lodepng_add_itext(info, key, langtag, transkey, (char*)decoded.data);

    break;
  }

  lodepng_free(key);
  lodepng_free(langtag);
  lodepng_free(transkey);
  ucvector_cleanup(&decoded);

  return error;
}

static unsigned readChunk_tIME(LodePNGInfo* info, const unsigned char* data, size_t chunkLength)
{
  if(chunkLength != 7) return 73; /*invalid tIME chunk size*/

  info->time_defined = 1;
  info->time.year = 256 * data[0] + data[+ 1];
  info->time.month = data[2];
  info->time.day = data[3];
  info->time.hour = data[4];
  info->time.minute = data[5];
  info->time.second = data[6];

  return 0; /* OK */
}

static unsigned readChunk_pHYs(LodePNGInfo* info, const unsigned char* data, size_t chunkLength)
{
  if(chunkLength != 9) return 74; /*invalid pHYs chunk size*/

  info->phys_defined = 1;
  info->phys_x = 16777216 * data[0] + 65536 * data[1] + 256 * data[2] + data[3];
  info->phys_y = 16777216 * data[4] + 65536 * data[5] + 256 * data[6] + data[7];
  info->phys_unit = data[8];

  return 0; /* OK */
}
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

/*read a PNG, the result will be in the same color type as the PNG (hence "generic")*/
static void decodeGeneric(unsigned char** out, unsigned* w, unsigned* h,
                          LodePNGState* state,
                          const unsigned char* in, size_t insize)
{
  unsigned char IEND = 0;
  const unsigned char* chunk;
  size_t i;
  ucvector idat; /*the data from idat chunks*/

  /*for unknown chunk order*/
  unsigned unknown = 0;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  unsigned critical_pos = 1; /*1 = after IHDR, 2 = after PLTE, 3 = after IDAT*/
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

  /*provide some proper output values if error will happen*/
  *out = 0;

  state->error = lodepng_inspect(w, h, state, in, insize); /*reads header and resets other parameters in state->info_png*/
  if(state->error) return;

  ucvector_init(&idat);
  chunk = &in[33]; /*first byte of the first chunk after the header*/

  /*loop through the chunks, ignoring unknown chunks and stopping at IEND chunk.
  IDAT data is put at the start of the in buffer*/
  while(!IEND && !state->error)
  {
    unsigned chunkLength;
    const unsigned char* data; /*the data in the chunk*/

    /*error: size of the in buffer too small to contain next chunk*/
    if((size_t)((chunk - in) + 12) > insize || chunk < in) CERROR_BREAK(state->error, 30);

    /*length of the data of the chunk, excluding the length bytes, chunk type and CRC bytes*/
    chunkLength = lodepng_chunk_length(chunk);
    /*error: chunk length larger than the max PNG chunk size*/
    if(chunkLength > 2147483647) CERROR_BREAK(state->error, 63);

    if((size_t)((chunk - in) + chunkLength + 12) > insize || (chunk + chunkLength + 12) < in)
    {
      CERROR_BREAK(state->error, 64); /*error: size of the in buffer too small to contain next chunk*/
    }

    data = lodepng_chunk_data_const(chunk);

    /*IDAT chunk, containing compressed image data*/
    if(lodepng_chunk_type_equals(chunk, "IDAT"))
    {
      size_t oldsize = idat.size;
      if(!ucvector_resize(&idat, oldsize + chunkLength)) CERROR_BREAK(state->error, 83 /*alloc fail*/);
      for(i = 0; i < chunkLength; i++) idat.data[oldsize + i] = data[i];
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
      critical_pos = 3;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    }
    /*IEND chunk*/
    else if(lodepng_chunk_type_equals(chunk, "IEND"))
    {
      IEND = 1;
    }
    /*palette chunk (PLTE)*/
    else if(lodepng_chunk_type_equals(chunk, "PLTE"))
    {
      state->error = readChunk_PLTE(&state->info_png.color, data, chunkLength);
      if(state->error) break;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
      critical_pos = 2;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    }
    /*palette transparency chunk (tRNS)*/
    else if(lodepng_chunk_type_equals(chunk, "tRNS"))
    {
      state->error = readChunk_tRNS(&state->info_png.color, data, chunkLength);
      if(state->error) break;
    }
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    /*background color chunk (bKGD)*/
    else if(lodepng_chunk_type_equals(chunk, "bKGD"))
    {
      state->error = readChunk_bKGD(&state->info_png, data, chunkLength);
      if(state->error) break;
    }
    /*text chunk (tEXt)*/
    else if(lodepng_chunk_type_equals(chunk, "tEXt"))
    {
      if(state->decoder.read_text_chunks)
      {
        state->error = readChunk_tEXt(&state->info_png, data, chunkLength);
        if(state->error) break;
      }
    }
    /*compressed text chunk (zTXt)*/
    else if(lodepng_chunk_type_equals(chunk, "zTXt"))
    {
      if(state->decoder.read_text_chunks)
      {
        state->error = readChunk_zTXt(&state->info_png, &state->decoder.zlibsettings, data, chunkLength);
        if(state->error) break;
      }
    }
    /*international text chunk (iTXt)*/
    else if(lodepng_chunk_type_equals(chunk, "iTXt"))
    {
      if(state->decoder.read_text_chunks)
      {
        state->error = readChunk_iTXt(&state->info_png, &state->decoder.zlibsettings, data, chunkLength);
        if(state->error) break;
      }
    }
    else if(lodepng_chunk_type_equals(chunk, "tIME"))
    {
      state->error = readChunk_tIME(&state->info_png, data, chunkLength);
      if(state->error) break;
    }
    else if(lodepng_chunk_type_equals(chunk, "pHYs"))
    {
      state->error = readChunk_pHYs(&state->info_png, data, chunkLength);
      if(state->error) break;
    }
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    else /*it's not an implemented chunk type, so ignore it: skip over the data*/
    {
      /*error: unknown critical chunk (5th bit of first byte of chunk type is 0)*/
      if(!lodepng_chunk_ancillary(chunk)) CERROR_BREAK(state->error, 69);

      unknown = 1;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
      if(state->decoder.remember_unknown_chunks)
      {
        state->error = lodepng_chunk_append(&state->info_png.unknown_chunks_data[critical_pos - 1],
                                            &state->info_png.unknown_chunks_size[critical_pos - 1], chunk);
        if(state->error) break;
      }
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    }

    if(!state->decoder.ignore_crc && !unknown) /*check CRC if wanted, only on known chunk types*/
    {
      if(lodepng_chunk_check_crc(chunk)) CERROR_BREAK(state->error, 57); /*invalid CRC*/
    }

    if(!IEND) chunk = lodepng_chunk_next_const(chunk);
  }

  if(!state->error)
  {
    ucvector scanlines;
    ucvector_init(&scanlines);

    /*maximum final image length is already reserved in the vector's length - this is not really necessary*/
    if(!ucvector_resize(&scanlines, lodepng_get_raw_size(*w, *h, &state->info_png.color) + *h))
    {
      state->error = 83; /*alloc fail*/
    }
    if(!state->error)
    {
      /*decompress with the Zlib decompressor*/
      state->error = zlib_decompress(&scanlines.data, &scanlines.size, idat.data,
                                     idat.size, &state->decoder.zlibsettings);
    }

    if(!state->error)
    {
      ucvector outv;
      ucvector_init(&outv);
      if(!ucvector_resizev(&outv,
          lodepng_get_raw_size(*w, *h, &state->info_png.color), 0)) state->error = 83; /*alloc fail*/
      if(!state->error) state->error = postProcessScanlines(outv.data, scanlines.data, *w, *h, &state->info_png);
      *out = outv.data;
    }
    ucvector_cleanup(&scanlines);
  }

  ucvector_cleanup(&idat);
}

unsigned lodepng_decode(unsigned char** out, unsigned* w, unsigned* h,
                        LodePNGState* state,
                        const unsigned char* in, size_t insize)
{
  *out = 0;
  decodeGeneric(out, w, h, state, in, insize);
  if(state->error) return state->error;
  if(!state->decoder.color_convert || lodepng_color_mode_equal(&state->info_raw, &state->info_png.color))
  {
    /*same color type, no copying or converting of data needed*/
    /*store the info_png color settings on the info_raw so that the info_raw still reflects what colortype
    the raw image has to the end user*/
    if(!state->decoder.color_convert)
    {
      state->error = lodepng_color_mode_copy(&state->info_raw, &state->info_png.color);
      if(state->error) return state->error;
    }
  }
  else
  {
    /*color conversion needed; sort of copy of the data*/
    unsigned char* data = *out;
    size_t outsize;

    /*TODO: check if this works according to the statement in the documentation: "The converter can convert
    from greyscale input color type, to 8-bit greyscale or greyscale with alpha"*/
    if(!(state->info_raw.colortype == LCT_RGB || state->info_raw.colortype == LCT_RGBA)
       && !(state->info_raw.bitdepth == 8))
    {
      return 56; /*unsupported color mode conversion*/
    }

    outsize = lodepng_get_raw_size(*w, *h, &state->info_raw);
    *out = (unsigned char*)lodepng_malloc(outsize);
    if(!(*out))
    {
      state->error = 83; /*alloc fail*/
    }
    else state->error = lodepng_convert(*out, data, &state->info_raw, &state->info_png.color, *w, *h, state->decoder.fix_png);
    lodepng_free(data);
  }
  return state->error;
}

unsigned lodepng_decode_memory(unsigned char** out, unsigned* w, unsigned* h, const unsigned char* in,
                               size_t insize, LodePNGColorType colortype, unsigned bitdepth)
{
  unsigned error;
  LodePNGState state;
  lodepng_state_init(&state);
  state.info_raw.colortype = colortype;
  state.info_raw.bitdepth = bitdepth;
  error = lodepng_decode(out, w, h, &state, in, insize);
  lodepng_state_cleanup(&state);
  return error;
}

unsigned lodepng_decode32(unsigned char** out, unsigned* w, unsigned* h, const unsigned char* in, size_t insize)
{
  return lodepng_decode_memory(out, w, h, in, insize, LCT_RGBA, 8);
}

unsigned lodepng_decode24(unsigned char** out, unsigned* w, unsigned* h, const unsigned char* in, size_t insize)
{
  return lodepng_decode_memory(out, w, h, in, insize, LCT_RGB, 8);
}

#ifdef LODEPNG_COMPILE_DISK
unsigned lodepng_decode_file(unsigned char** out, unsigned* w, unsigned* h, const char* filename,
                             LodePNGColorType colortype, unsigned bitdepth)
{
  unsigned char* buffer;
  size_t buffersize;
  unsigned error;
  error = lodepng_load_file(&buffer, &buffersize, filename);
  if(!error) error = lodepng_decode_memory(out, w, h, buffer, buffersize, colortype, bitdepth);
  lodepng_free(buffer);
  return error;
}

unsigned lodepng_decode32_file(unsigned char** out, unsigned* w, unsigned* h, const char* filename)
{
  return lodepng_decode_file(out, w, h, filename, LCT_RGBA, 8);
}

unsigned lodepng_decode24_file(unsigned char** out, unsigned* w, unsigned* h, const char* filename)
{
  return lodepng_decode_file(out, w, h, filename, LCT_RGB, 8);
}
#endif /*LODEPNG_COMPILE_DISK*/

void lodepng_decoder_settings_init(LodePNGDecoderSettings* settings)
{
  settings->color_convert = 1;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  settings->read_text_chunks = 1;
  settings->remember_unknown_chunks = 0;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
  settings->ignore_crc = 0;
  settings->fix_png = 0;
  lodepng_decompress_settings_init(&settings->zlibsettings);
}

#endif /*LODEPNG_COMPILE_DECODER*/

#if defined(LODEPNG_COMPILE_DECODER) || defined(LODEPNG_COMPILE_ENCODER)

void lodepng_state_init(LodePNGState* state)
{
#ifdef LODEPNG_COMPILE_DECODER
  lodepng_decoder_settings_init(&state->decoder);
#endif /*LODEPNG_COMPILE_DECODER*/
#ifdef LODEPNG_COMPILE_ENCODER
  lodepng_encoder_settings_init(&state->encoder);
#endif /*LODEPNG_COMPILE_ENCODER*/
  lodepng_color_mode_init(&state->info_raw);
  lodepng_info_init(&state->info_png);
  state->error = 1;
}

void lodepng_state_cleanup(LodePNGState* state)
{
  lodepng_color_mode_cleanup(&state->info_raw);
  lodepng_info_cleanup(&state->info_png);
}

void lodepng_state_copy(LodePNGState* dest, const LodePNGState* source)
{
  lodepng_state_cleanup(dest);
  *dest = *source;
  lodepng_color_mode_init(&dest->info_raw);
  lodepng_info_init(&dest->info_png);
  dest->error = lodepng_color_mode_copy(&dest->info_raw, &source->info_raw); if(dest->error) return;
  dest->error = lodepng_info_copy(&dest->info_png, &source->info_png); if(dest->error) return;
}

#endif /* defined(LODEPNG_COMPILE_DECODER) || defined(LODEPNG_COMPILE_ENCODER) */

#ifdef LODEPNG_COMPILE_ENCODER

/* ////////////////////////////////////////////////////////////////////////// */
/* / PNG Encoder                                                            / */
/* ////////////////////////////////////////////////////////////////////////// */

/*chunkName must be string of 4 characters*/
static unsigned addChunk(ucvector* out, const char* chunkName, const unsigned char* data, size_t length)
{
  CERROR_TRY_RETURN(lodepng_chunk_create(&out->data, &out->size, (unsigned)length, chunkName, data));
  out->allocsize = out->size; /*fix the allocsize again*/
  return 0;
}

static void writeSignature(ucvector* out)
{
  /*8 bytes PNG signature, aka the magic bytes*/
  ucvector_push_back(out, 137);
  ucvector_push_back(out, 80);
  ucvector_push_back(out, 78);
  ucvector_push_back(out, 71);
  ucvector_push_back(out, 13);
  ucvector_push_back(out, 10);
  ucvector_push_back(out, 26);
  ucvector_push_back(out, 10);
}

static unsigned addChunk_IHDR(ucvector* out, unsigned w, unsigned h,
                              LodePNGColorType colortype, unsigned bitdepth, unsigned interlace_method)
{
  unsigned error = 0;
  ucvector header;
  ucvector_init(&header);

  lodepng_add32bitInt(&header, w); /*width*/
  lodepng_add32bitInt(&header, h); /*height*/
  ucvector_push_back(&header, (unsigned char)bitdepth); /*bit depth*/
  ucvector_push_back(&header, (unsigned char)colortype); /*color type*/
  ucvector_push_back(&header, 0); /*compression method*/
  ucvector_push_back(&header, 0); /*filter method*/
  ucvector_push_back(&header, (unsigned char)interlace_method); /*interlace method*/

  error = addChunk(out, "IHDR", header.data, header.size);
  ucvector_cleanup(&header);

  return error;
}

static unsigned addChunk_PLTE(ucvector* out, const LodePNGColorMode* info)
{
  unsigned error = 0;
  size_t i;
  ucvector PLTE;
  ucvector_init(&PLTE);
  for(i = 0; i < info->palettesize * 4; i++)
  {
    /*add all channels except alpha channel*/
    if(i % 4 != 3) ucvector_push_back(&PLTE, info->palette[i]);
  }
  error = addChunk(out, "PLTE", PLTE.data, PLTE.size);
  ucvector_cleanup(&PLTE);

  return error;
}

static unsigned addChunk_tRNS(ucvector* out, const LodePNGColorMode* info)
{
  unsigned error = 0;
  size_t i;
  ucvector tRNS;
  ucvector_init(&tRNS);
  if(info->colortype == LCT_PALETTE)
  {
    size_t amount = info->palettesize;
    /*the tail of palette values that all have 255 as alpha, does not have to be encoded*/
    for(i = info->palettesize; i > 0; i--)
    {
      if(info->palette[4 * (i - 1) + 3] == 255) amount--;
      else break;
    }
    /*add only alpha channel*/
    for(i = 0; i < amount; i++) ucvector_push_back(&tRNS, info->palette[4 * i + 3]);
  }
  else if(info->colortype == LCT_GREY)
  {
    if(info->key_defined)
    {
      ucvector_push_back(&tRNS, (unsigned char)(info->key_r / 256));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_r % 256));
    }
  }
  else if(info->colortype == LCT_RGB)
  {
    if(info->key_defined)
    {
      ucvector_push_back(&tRNS, (unsigned char)(info->key_r / 256));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_r % 256));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_g / 256));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_g % 256));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_b / 256));
      ucvector_push_back(&tRNS, (unsigned char)(info->key_b % 256));
    }
  }

  error = addChunk(out, "tRNS", tRNS.data, tRNS.size);
  ucvector_cleanup(&tRNS);

  return error;
}

static unsigned addChunk_IDAT(ucvector* out, const unsigned char* data, size_t datasize,
                              LodePNGCompressSettings* zlibsettings)
{
  ucvector zlibdata;
  unsigned error = 0;

  /*compress with the Zlib compressor*/
  ucvector_init(&zlibdata);
  error = zlib_compress(&zlibdata.data, &zlibdata.size, data, datasize, zlibsettings);
  if(!error) error = addChunk(out, "IDAT", zlibdata.data, zlibdata.size);
  ucvector_cleanup(&zlibdata);

  return error;
}

static unsigned addChunk_IEND(ucvector* out)
{
  unsigned error = 0;
  error = addChunk(out, "IEND", 0, 0);
  return error;
}

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS

static unsigned addChunk_tEXt(ucvector* out, const char* keyword, const char* textstring)
{
  unsigned error = 0;
  size_t i;
  ucvector text;
  ucvector_init(&text);
  for(i = 0; keyword[i] != 0; i++) ucvector_push_back(&text, (unsigned char)keyword[i]);
  if(i < 1 || i > 79) return 89; /*error: invalid keyword size*/
  ucvector_push_back(&text, 0); /*0 termination char*/
  for(i = 0; textstring[i] != 0; i++) ucvector_push_back(&text, (unsigned char)textstring[i]);
  error = addChunk(out, "tEXt", text.data, text.size);
  ucvector_cleanup(&text);

  return error;
}

static unsigned addChunk_zTXt(ucvector* out, const char* keyword, const char* textstring,
                              LodePNGCompressSettings* zlibsettings)
{
  unsigned error = 0;
  ucvector data, compressed;
  size_t i, textsize = strlen(textstring);

  ucvector_init(&data);
  ucvector_init(&compressed);
  for(i = 0; keyword[i] != 0; i++) ucvector_push_back(&data, (unsigned char)keyword[i]);
  if(i < 1 || i > 79) return 89; /*error: invalid keyword size*/
  ucvector_push_back(&data, 0); /*0 termination char*/
  ucvector_push_back(&data, 0); /*compression method: 0*/

  error = zlib_compress(&compressed.data, &compressed.size,
                        (unsigned char*)textstring, textsize, zlibsettings);
  if(!error)
  {
    for(i = 0; i < compressed.size; i++) ucvector_push_back(&data, compressed.data[i]);
    error = addChunk(out, "zTXt", data.data, data.size);
  }

  ucvector_cleanup(&compressed);
  ucvector_cleanup(&data);
  return error;
}

static unsigned addChunk_iTXt(ucvector* out, unsigned compressed, const char* keyword, const char* langtag,
                              const char* transkey, const char* textstring, LodePNGCompressSettings* zlibsettings)
{
  unsigned error = 0;
  ucvector data;
  size_t i, textsize = strlen(textstring);

  ucvector_init(&data);

  for(i = 0; keyword[i] != 0; i++) ucvector_push_back(&data, (unsigned char)keyword[i]);
  if(i < 1 || i > 79) return 89; /*error: invalid keyword size*/
  ucvector_push_back(&data, 0); /*null termination char*/
  ucvector_push_back(&data, compressed ? 1 : 0); /*compression flag*/
  ucvector_push_back(&data, 0); /*compression method*/
  for(i = 0; langtag[i] != 0; i++) ucvector_push_back(&data, (unsigned char)langtag[i]);
  ucvector_push_back(&data, 0); /*null termination char*/
  for(i = 0; transkey[i] != 0; i++) ucvector_push_back(&data, (unsigned char)transkey[i]);
  ucvector_push_back(&data, 0); /*null termination char*/

  if(compressed)
  {
    ucvector compressed_data;
    ucvector_init(&compressed_data);
    error = zlib_compress(&compressed_data.data, &compressed_data.size,
                          (unsigned char*)textstring, textsize, zlibsettings);
    if(!error)
    {
      for(i = 0; i < compressed_data.size; i++) ucvector_push_back(&data, compressed_data.data[i]);
    }
    ucvector_cleanup(&compressed_data);
  }
  else /*not compressed*/
  {
    for(i = 0; textstring[i] != 0; i++) ucvector_push_back(&data, (unsigned char)textstring[i]);
  }

  if(!error) error = addChunk(out, "iTXt", data.data, data.size);
  ucvector_cleanup(&data);
  return error;
}

static unsigned addChunk_bKGD(ucvector* out, const LodePNGInfo* info)
{
  unsigned error = 0;
  ucvector bKGD;
  ucvector_init(&bKGD);
  if(info->color.colortype == LCT_GREY || info->color.colortype == LCT_GREY_ALPHA)
  {
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r / 256));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r % 256));
  }
  else if(info->color.colortype == LCT_RGB || info->color.colortype == LCT_RGBA)
  {
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r / 256));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r % 256));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_g / 256));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_g % 256));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_b / 256));
    ucvector_push_back(&bKGD, (unsigned char)(info->background_b % 256));
  }
  else if(info->color.colortype == LCT_PALETTE)
  {
    ucvector_push_back(&bKGD, (unsigned char)(info->background_r % 256)); /*palette index*/
  }

  error = addChunk(out, "bKGD", bKGD.data, bKGD.size);
  ucvector_cleanup(&bKGD);

  return error;
}

static unsigned addChunk_tIME(ucvector* out, const LodePNGTime* time)
{
  unsigned error = 0;
  unsigned char* data = (unsigned char*)lodepng_malloc(7);
  if(!data) return 83; /*alloc fail*/
  data[0] = (unsigned char)(time->year / 256);
  data[1] = (unsigned char)(time->year % 256);
  data[2] = (unsigned char)time->month;
  data[3] = (unsigned char)time->day;
  data[4] = (unsigned char)time->hour;
  data[5] = (unsigned char)time->minute;
  data[6] = (unsigned char)time->second;
  error = addChunk(out, "tIME", data, 7);
  lodepng_free(data);
  return error;
}

static unsigned addChunk_pHYs(ucvector* out, const LodePNGInfo* info)
{
  unsigned error = 0;
  ucvector data;
  ucvector_init(&data);

  lodepng_add32bitInt(&data, info->phys_x);
  lodepng_add32bitInt(&data, info->phys_y);
  ucvector_push_back(&data, (unsigned char)info->phys_unit);

  error = addChunk(out, "pHYs", data.data, data.size);
  ucvector_cleanup(&data);

  return error;
}

#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

static void filterScanline(unsigned char* out, const unsigned char* scanline, const unsigned char* prevline,
                           size_t length, size_t bytewidth, unsigned char filterType)
{
  size_t i;
  switch(filterType)
  {
    case 0: /*None*/
      for(i = 0; i < length; i++) out[i] = scanline[i];
      break;
    case 1: /*Sub*/
      if(prevline)
      {
        for(i = 0; i < bytewidth; i++) out[i] = scanline[i];
        for(i = bytewidth; i < length; i++) out[i] = scanline[i] - scanline[i - bytewidth];
      }
      else
      {
        for(i = 0; i < bytewidth; i++) out[i] = scanline[i];
        for(i = bytewidth; i < length; i++) out[i] = scanline[i] - scanline[i - bytewidth];
      }
      break;
    case 2: /*Up*/
      if(prevline)
      {
        for(i = 0; i < length; i++) out[i] = scanline[i] - prevline[i];
      }
      else
      {
        for(i = 0; i < length; i++) out[i] = scanline[i];
      }
      break;
    case 3: /*Average*/
      if(prevline)
      {
        for(i = 0; i < bytewidth; i++) out[i] = scanline[i] - prevline[i] / 2;
        for(i = bytewidth; i < length; i++) out[i] = scanline[i] - ((scanline[i - bytewidth] + prevline[i]) / 2);
      }
      else
      {
        for(i = 0; i < bytewidth; i++) out[i] = scanline[i];
        for(i = bytewidth; i < length; i++) out[i] = scanline[i] - scanline[i - bytewidth] / 2;
      }
      break;
    case 4: /*Paeth*/
      if(prevline)
      {
        /*paethPredictor(0, prevline[i], 0) is always prevline[i]*/
        for(i = 0; i < bytewidth; i++) out[i] = (scanline[i] - prevline[i]);
        for(i = bytewidth; i < length; i++)
        {
          out[i] = (scanline[i] - paethPredictor(scanline[i - bytewidth], prevline[i], prevline[i - bytewidth]));
        }
      }
      else
      {
        for(i = 0; i < bytewidth; i++) out[i] = scanline[i];
        /*paethPredictor(scanline[i - bytewidth], 0, 0) is always scanline[i - bytewidth]*/
        for(i = bytewidth; i < length; i++) out[i] = (scanline[i] - scanline[i - bytewidth]);
      }
      break;
    default: return; /*unexisting filter type given*/
  }
}

/* log2 approximation. A slight bit faster than std::log. */
static float flog2(float f)
{
  float result = 0;
  while(f > 32) { result += 4; f /= 16; }
  while(f > 2) { result++; f /= 2; }
  return result + 1.442695f * (f * f * f / 3 - 3 * f * f / 2 + 3 * f - 1.83333f);
}

static unsigned filter(unsigned char* out, const unsigned char* in, unsigned w, unsigned h,
                       const LodePNGColorMode* info, const LodePNGEncoderSettings* settings)
{
  /*
  For PNG filter method 0
  out must be a buffer with as size: h + (w * h * bpp + 7) / 8, because there are
  the scanlines with 1 extra byte per scanline
  */

  unsigned bpp = lodepng_get_bpp(info);
  /*the width of a scanline in bytes, not including the filter type*/
  size_t linebytes = (w * bpp + 7) / 8;
  /*bytewidth is used for filtering, is 1 when bpp < 8, number of bytes per pixel otherwise*/
  size_t bytewidth = (bpp + 7) / 8;
  const unsigned char* prevline = 0;
  unsigned x, y;
  unsigned error = 0;
  LodePNGFilterStrategy strategy = settings->filter_strategy;

  /*
  There is a heuristic called the minimum sum of absolute differences heuristic, suggested by the PNG standard:
   *  If the image type is Palette, or the bit depth is smaller than 8, then do not filter the image (i.e.
      use fixed filtering, with the filter None).
   * (The other case) If the image type is Grayscale or RGB (with or without Alpha), and the bit depth is
     not smaller than 8, then use adaptive filtering heuristic as follows: independently for each row, apply
     all five filters and select the filter that produces the smallest sum of absolute values per row.
  This heuristic is used if filter strategy is LFS_MINSUM and filter_palette_zero is true.

  If filter_palette_zero is true and filter_strategy is not LFS_MINSUM, the above heuristic is followed,
  but for "the other case", whatever strategy filter_strategy is set to instead of the minimum sum
  heuristic is used.
  */
  if(settings->filter_palette_zero &&
     (info->colortype == LCT_PALETTE || info->bitdepth < 8)) strategy = LFS_ZERO;

  if(bpp == 0) return 31; /*error: invalid color type*/

  if(strategy == LFS_ZERO)
  {
    for(y = 0; y < h; y++)
    {
      size_t outindex = (1 + linebytes) * y; /*the extra filterbyte added to each row*/
      size_t inindex = linebytes * y;
      out[outindex] = 0; /*filter type byte*/
      filterScanline(&out[outindex + 1], &in[inindex], prevline, linebytes, bytewidth, 0);
      prevline = &in[inindex];
    }
  }
  else if(strategy == LFS_MINSUM)
  {
    /*adaptive filtering*/
    size_t sum[5];
    ucvector attempt[5]; /*five filtering attempts, one for each filter type*/
    size_t smallest = 0;
    unsigned type, bestType = 0;

    for(type = 0; type < 5; type++)
    {
      ucvector_init(&attempt[type]);
      if(!ucvector_resize(&attempt[type], linebytes)) return 83; /*alloc fail*/
    }

    if(!error)
    {
      for(y = 0; y < h; y++)
      {
        /*try the 5 filter types*/
        for(type = 0; type < 5; type++)
        {
			filterScanline(attempt[type].data, &in[y * linebytes], prevline, linebytes, bytewidth, (unsigned char)type);

          /*calculate the sum of the result*/
          sum[type] = 0;
          if(type == 0)
          {
            for(x = 0; x < linebytes; x++) sum[type] += (unsigned char)(attempt[type].data[x]);
          }
          else
          {
            for(x = 0; x < linebytes; x++)
            {
              /*For differences, each byte should be treated as signed, values above 127 are negative
              (converted to signed char). Filtertype 0 isn't a difference though, so use unsigned there.
              This means filtertype 0 is almost never chosen, but that is justified.*/
              signed char s = (signed char)(attempt[type].data[x]);
              sum[type] += s < 0 ? -s : s;
            }
          }

          /*check if this is smallest sum (or if type == 0 it's the first case so always store the values)*/
          if(type == 0 || sum[type] < smallest)
          {
            bestType = type;
            smallest = sum[type];
          }
        }

        prevline = &in[y * linebytes];

        /*now fill the out values*/
		out[y * (linebytes + 1)] = (unsigned char)bestType; /*the first byte of a scanline will be the filter type*/
        for(x = 0; x < linebytes; x++) out[y * (linebytes + 1) + 1 + x] = attempt[bestType].data[x];
      }
    }

    for(type = 0; type < 5; type++) ucvector_cleanup(&attempt[type]);
  }
  else if(strategy == LFS_ENTROPY)
  {
    float sum[5];
    ucvector attempt[5]; /*five filtering attempts, one for each filter type*/
    float smallest = 0;
    unsigned type, bestType = 0;
    unsigned count[256];

    for(type = 0; type < 5; type++)
    {
      ucvector_init(&attempt[type]);
      if(!ucvector_resize(&attempt[type], linebytes)) return 83; /*alloc fail*/
    }

    for(y = 0; y < h; y++)
    {
      /*try the 5 filter types*/
      for(type = 0; type < 5; type++)
      {
		  filterScanline(attempt[type].data, &in[y * linebytes], prevline, linebytes, bytewidth, (unsigned char)type);
        for(x = 0; x < 256; x++) count[x] = 0;
        for(x = 0; x < linebytes; x++) count[attempt[type].data[x]]++;
        count[type]++; /*the filter type itself is part of the scanline*/
        sum[type] = 0;
        for(x = 0; x < 256; x++)
        {
          float p = count[x] / (float)(linebytes + 1);
          sum[type] += count[x] == 0 ? 0 : flog2(1 / p) * p;
        }
        /*check if this is smallest sum (or if type == 0 it's the first case so always store the values)*/
        if(type == 0 || sum[type] < smallest)
        {
          bestType = type;
          smallest = sum[type];
        }
      }

      prevline = &in[y * linebytes];

      /*now fill the out values*/
	  out[y * (linebytes + 1)] = (unsigned char)bestType; /*the first byte of a scanline will be the filter type*/
      for(x = 0; x < linebytes; x++) out[y * (linebytes + 1) + 1 + x] = attempt[bestType].data[x];
    }

    for(type = 0; type < 5; type++) ucvector_cleanup(&attempt[type]);
  }
  else if(strategy == LFS_PREDEFINED)
  {
    for(y = 0; y < h; y++)
    {
      size_t outindex = (1 + linebytes) * y; /*the extra filterbyte added to each row*/
      size_t inindex = linebytes * y;
      unsigned type = settings->predefined_filters[y];
	  out[outindex] = (unsigned char)type; /*filter type byte*/
	  filterScanline(&out[outindex + 1], &in[inindex], prevline, linebytes, bytewidth, (unsigned char)type);
      prevline = &in[inindex];
    }
  }
  else if(strategy == LFS_BRUTE_FORCE)
  {
    /*brute force filter chooser.
    deflate the scanline after every filter attempt to see which one deflates best.
    This is very slow and gives only slightly smaller, sometimes even larger, result*/
    size_t size[5];
    ucvector attempt[5]; /*five filtering attempts, one for each filter type*/
    size_t smallest = 0;
    unsigned type = 0, bestType = 0;
    unsigned char* dummy;
    LodePNGCompressSettings zlibsettings = settings->zlibsettings;
    /*use fixed tree on the attempts so that the tree is not adapted to the filtertype on purpose,
    to simulate the true case where the tree is the same for the whole image. Sometimes it gives
    better result with dynamic tree anyway. Using the fixed tree sometimes gives worse, but in rare
    cases better compression. It does make this a bit less slow, so it's worth doing this.*/
    zlibsettings.btype = 1;
    /*a custom encoder likely doesn't read the btype setting and is optimized for complete PNG
    images only, so disable it*/
    zlibsettings.custom_zlib = 0;
    zlibsettings.custom_deflate = 0;
    for(type = 0; type < 5; type++)
    {
      ucvector_init(&attempt[type]);
      ucvector_resize(&attempt[type], linebytes); /*todo: give error if resize failed*/
    }
    for(y = 0; y < h; y++) /*try the 5 filter types*/
    {
      for(type = 0; type < 5; type++)
      {
		  unsigned testsize = (unsigned int)attempt[type].size;
        /*if(testsize > 8) testsize /= 8;*/ /*it already works good enough by testing a part of the row*/

		  filterScanline(attempt[type].data, &in[y * linebytes], prevline, linebytes, bytewidth, (unsigned char)type);
        size[type] = 0;
        dummy = 0;
        zlib_compress(&dummy, &size[type], attempt[type].data, testsize, &zlibsettings);
        lodepng_free(dummy);
        /*check if this is smallest size (or if type == 0 it's the first case so always store the values)*/
        if(type == 0 || size[type] < smallest)
        {
          bestType = type;
          smallest = size[type];
        }
      }
      prevline = &in[y * linebytes];
	  out[y * (linebytes + 1)] = (unsigned char)bestType; /*the first byte of a scanline will be the filter type*/
      for(x = 0; x < linebytes; x++) out[y * (linebytes + 1) + 1 + x] = attempt[bestType].data[x];
    }
    for(type = 0; type < 5; type++) ucvector_cleanup(&attempt[type]);
  }
  else return 88; /* unknown filter strategy */

  return error;
}

static void addPaddingBits(unsigned char* out, const unsigned char* in,
                           size_t olinebits, size_t ilinebits, unsigned h)
{
  /*The opposite of the removePaddingBits function
  olinebits must be >= ilinebits*/
  unsigned y;
  size_t diff = olinebits - ilinebits;
  size_t obp = 0, ibp = 0; /*bit pointers*/
  for(y = 0; y < h; y++)
  {
    size_t x;
    for(x = 0; x < ilinebits; x++)
    {
      unsigned char bit = readBitFromReversedStream(&ibp, in);
      setBitOfReversedStream(&obp, out, bit);
    }
    /*obp += diff; --> no, fill in some value in the padding bits too, to avoid
    "Use of uninitialised value of size ###" warning from valgrind*/
    for(x = 0; x < diff; x++) setBitOfReversedStream(&obp, out, 0);
  }
}

/*
in: non-interlaced image with size w*h
out: the same pixels, but re-ordered according to PNG's Adam7 interlacing, with
 no padding bits between scanlines, but between reduced images so that each
 reduced image starts at a byte.
bpp: bits per pixel
there are no padding bits, not between scanlines, not between reduced images
in has the following size in bits: w * h * bpp.
out is possibly bigger due to padding bits between reduced images
NOTE: comments about padding bits are only relevant if bpp < 8
*/
static void Adam7_interlace(unsigned char* out, const unsigned char* in, unsigned w, unsigned h, unsigned bpp)
{
  unsigned passw[7], passh[7];
  size_t filter_passstart[8], padded_passstart[8], passstart[8];
  unsigned i;

  Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

  if(bpp >= 8)
  {
    for(i = 0; i < 7; i++)
    {
      unsigned x, y, b;
      size_t bytewidth = bpp / 8;
      for(y = 0; y < passh[i]; y++)
      for(x = 0; x < passw[i]; x++)
      {
        size_t pixelinstart = ((ADAM7_IY[i] + y * ADAM7_DY[i]) * w + ADAM7_IX[i] + x * ADAM7_DX[i]) * bytewidth;
        size_t pixeloutstart = passstart[i] + (y * passw[i] + x) * bytewidth;
        for(b = 0; b < bytewidth; b++)
        {
          out[pixeloutstart + b] = in[pixelinstart + b];
        }
      }
    }
  }
  else /*bpp < 8: Adam7 with pixels < 8 bit is a bit trickier: with bit pointers*/
  {
    for(i = 0; i < 7; i++)
    {
      unsigned x, y, b;
      unsigned ilinebits = bpp * passw[i];
      unsigned olinebits = bpp * w;
      size_t obp, ibp; /*bit pointers (for out and in buffer)*/
      for(y = 0; y < passh[i]; y++)
      for(x = 0; x < passw[i]; x++)
      {
        ibp = (ADAM7_IY[i] + y * ADAM7_DY[i]) * olinebits + (ADAM7_IX[i] + x * ADAM7_DX[i]) * bpp;
        obp = (8 * passstart[i]) + (y * ilinebits + x * bpp);
        for(b = 0; b < bpp; b++)
        {
          unsigned char bit = readBitFromReversedStream(&ibp, in);
          setBitOfReversedStream(&obp, out, bit);
        }
      }
    }
  }
}

/*out must be buffer big enough to contain uncompressed IDAT chunk data, and in must contain the full image.
return value is error**/
static unsigned preProcessScanlines(unsigned char** out, size_t* outsize, const unsigned char* in,
                                    unsigned w, unsigned h,
                                    const LodePNGInfo* info_png, const LodePNGEncoderSettings* settings)
{
  /*
  This function converts the pure 2D image with the PNG's colortype, into filtered-padded-interlaced data. Steps:
  *) if no Adam7: 1) add padding bits (= posible extra bits per scanline if bpp < 8) 2) filter
  *) if adam7: 1) Adam7_interlace 2) 7x add padding bits 3) 7x filter
  */
  unsigned bpp = lodepng_get_bpp(&info_png->color);
  unsigned error = 0;

  if(info_png->interlace_method == 0)
  {
    *outsize = h + (h * ((w * bpp + 7) / 8)); /*image size plus an extra byte per scanline + possible padding bits*/
    *out = (unsigned char*)lodepng_malloc(*outsize);
    if(!(*out) && (*outsize)) error = 83; /*alloc fail*/

    if(!error)
    {
      /*non multiple of 8 bits per scanline, padding bits needed per scanline*/
      if(bpp < 8 && w * bpp != ((w * bpp + 7) / 8) * 8)
      {
        unsigned char* padded = (unsigned char*)lodepng_malloc(h * ((w * bpp + 7) / 8));
        if(!padded) error = 83; /*alloc fail*/
        if(!error)
        {
          addPaddingBits(padded, in, ((w * bpp + 7) / 8) * 8, w * bpp, h);
          error = filter(*out, padded, w, h, &info_png->color, settings);
        }
        lodepng_free(padded);
      }
      else
      {
        /*we can immediatly filter into the out buffer, no other steps needed*/
        error = filter(*out, in, w, h, &info_png->color, settings);
      }
    }
  }
  else /*interlace_method is 1 (Adam7)*/
  {
    unsigned passw[7], passh[7];
    size_t filter_passstart[8], padded_passstart[8], passstart[8];
    unsigned char* adam7;

    Adam7_getpassvalues(passw, passh, filter_passstart, padded_passstart, passstart, w, h, bpp);

    *outsize = filter_passstart[7]; /*image size plus an extra byte per scanline + possible padding bits*/
    *out = (unsigned char*)lodepng_malloc(*outsize);
    if(!(*out)) error = 83; /*alloc fail*/

    adam7 = (unsigned char*)lodepng_malloc(passstart[7]);
    if(!adam7 && passstart[7]) error = 83; /*alloc fail*/

    if(!error)
    {
      unsigned i;

      Adam7_interlace(adam7, in, w, h, bpp);
      for(i = 0; i < 7; i++)
      {
        if(bpp < 8)
        {
          unsigned char* padded = (unsigned char*)lodepng_malloc(padded_passstart[i + 1] - padded_passstart[i]);
          if(!padded) ERROR_BREAK(83); /*alloc fail*/
          addPaddingBits(padded, &adam7[passstart[i]],
                         ((passw[i] * bpp + 7) / 8) * 8, passw[i] * bpp, passh[i]);
          error = filter(&(*out)[filter_passstart[i]], padded,
                         passw[i], passh[i], &info_png->color, settings);
          lodepng_free(padded);
        }
        else
        {
          error = filter(&(*out)[filter_passstart[i]], &adam7[padded_passstart[i]],
                         passw[i], passh[i], &info_png->color, settings);
        }

        if(error) break;
      }
    }

    lodepng_free(adam7);
  }

  return error;
}

/*
palette must have 4 * palettesize bytes allocated, and given in format RGBARGBARGBARGBA...
returns 0 if the palette is opaque,
returns 1 if the palette has a single color with alpha 0 ==> color key
returns 2 if the palette is semi-translucent.
*/
static unsigned getPaletteTranslucency(const unsigned char* palette, size_t palettesize)
{
  size_t i, key = 0;
  unsigned r = 0, g = 0, b = 0; /*the value of the color with alpha 0, so long as color keying is possible*/
  for(i = 0; i < palettesize; i++)
  {
    if(!key && palette[4 * i + 3] == 0)
    {
      r = palette[4 * i + 0]; g = palette[4 * i + 1]; b = palette[4 * i + 2];
      key = 1;
      i = (size_t)(-1); /*restart from beginning, to detect earlier opaque colors with key's value*/
    }
    else if(palette[4 * i + 3] != 255) return 2;
    /*when key, no opaque RGB may have key's RGB*/
    else if(key && r == palette[i * 4 + 0] && g == palette[i * 4 + 1] && b == palette[i * 4 + 2]) return 2;
  }
  return (unsigned int)key;
}

#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
static unsigned addUnknownChunks(ucvector* out, unsigned char* data, size_t datasize)
{
  unsigned char* inchunk = data;
  while((size_t)(inchunk - data) < datasize)
  {
    CERROR_TRY_RETURN(lodepng_chunk_append(&out->data, &out->size, inchunk));
    out->allocsize = out->size; /*fix the allocsize again*/
    inchunk = lodepng_chunk_next(inchunk);
  }
  return 0;
}
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/

unsigned lodepng_encode(unsigned char** out, size_t* outsize,
                        const unsigned char* image, unsigned w, unsigned h,
                        LodePNGState* state)
{
  LodePNGInfo info;
  ucvector outv;
  unsigned char* data = 0; /*uncompressed version of the IDAT chunk data*/
  size_t datasize = 0;

  /*provide some proper output values if error will happen*/
  *out = 0;
  *outsize = 0;
  state->error = 0;

  lodepng_info_init(&info);
  lodepng_info_copy(&info, &state->info_png);

  if((info.color.colortype == LCT_PALETTE || state->encoder.force_palette)
      && (info.color.palettesize == 0 || info.color.palettesize > 256))
  {
    state->error = 68; /*invalid palette size, it is only allowed to be 1-256*/
    return state->error;
  }

  if(state->encoder.auto_convert != LAC_NO)
  {
    state->error = lodepng_auto_choose_color(&info.color, image, w, h, &state->info_raw,
                                             state->encoder.auto_convert);
  }
  if(state->error) return state->error;

  if(state->encoder.zlibsettings.windowsize > 32768)
  {
    CERROR_RETURN_ERROR(state->error, 60); /*error: windowsize larger than allowed*/
  }
  if(state->encoder.zlibsettings.btype > 2)
  {
    CERROR_RETURN_ERROR(state->error, 61); /*error: unexisting btype*/
  }
  if(state->info_png.interlace_method > 1)
  {
    CERROR_RETURN_ERROR(state->error, 71); /*error: unexisting interlace mode*/
  }

  state->error = checkColorValidity(info.color.colortype, info.color.bitdepth);
  if(state->error) return state->error; /*error: unexisting color type given*/
  state->error = checkColorValidity(state->info_raw.colortype, state->info_raw.bitdepth);
  if(state->error) return state->error; /*error: unexisting color type given*/

  if(!lodepng_color_mode_equal(&state->info_raw, &info.color))
  {
    unsigned char* converted;
    size_t size = (w * h * lodepng_get_bpp(&info.color) + 7) / 8;

    converted = (unsigned char*)lodepng_malloc(size);
    if(!converted && size) state->error = 83; /*alloc fail*/
    if(!state->error)
    {
      state->error = lodepng_convert(converted, image, &info.color, &state->info_raw, w, h, 0 /*fix_png*/);
    }
    if(!state->error) preProcessScanlines(&data, &datasize, converted, w, h, &info, &state->encoder);
    lodepng_free(converted);
  }
  else preProcessScanlines(&data, &datasize, image, w, h, &info, &state->encoder);

  ucvector_init(&outv);
  while(!state->error) /*while only executed once, to break on error*/
  {
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    size_t i;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    /*write signature and chunks*/
    writeSignature(&outv);
    /*IHDR*/
    addChunk_IHDR(&outv, w, h, info.color.colortype, info.color.bitdepth, info.interlace_method);
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    /*unknown chunks between IHDR and PLTE*/
    if(info.unknown_chunks_data[0])
    {
      state->error = addUnknownChunks(&outv, info.unknown_chunks_data[0], info.unknown_chunks_size[0]);
      if(state->error) break;
    }
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    /*PLTE*/
    if(info.color.colortype == LCT_PALETTE)
    {
      addChunk_PLTE(&outv, &info.color);
    }
    if(state->encoder.force_palette && (info.color.colortype == LCT_RGB || info.color.colortype == LCT_RGBA))
    {
      addChunk_PLTE(&outv, &info.color);
    }
    /*tRNS*/
    if(info.color.colortype == LCT_PALETTE && getPaletteTranslucency(info.color.palette, info.color.palettesize) != 0)
    {
      addChunk_tRNS(&outv, &info.color);
    }
    if((info.color.colortype == LCT_GREY || info.color.colortype == LCT_RGB) && info.color.key_defined)
    {
      addChunk_tRNS(&outv, &info.color);
    }
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    /*bKGD (must come between PLTE and the IDAt chunks*/
    if(info.background_defined) addChunk_bKGD(&outv, &info);
    /*pHYs (must come before the IDAT chunks)*/
    if(info.phys_defined) addChunk_pHYs(&outv, &info);

    /*unknown chunks between PLTE and IDAT*/
    if(info.unknown_chunks_data[1])
    {
      state->error = addUnknownChunks(&outv, info.unknown_chunks_data[1], info.unknown_chunks_size[1]);
      if(state->error) break;
    }
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    /*IDAT (multiple IDAT chunks must be consecutive)*/
    state->error = addChunk_IDAT(&outv, data, datasize, &state->encoder.zlibsettings);
    if(state->error) break;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
    /*tIME*/
    if(info.time_defined) addChunk_tIME(&outv, &info.time);
    /*tEXt and/or zTXt*/
    for(i = 0; i < info.text_num; i++)
    {
      if(strlen(info.text_keys[i]) > 79)
      {
        state->error = 66; /*text chunk too large*/
        break;
      }
      if(strlen(info.text_keys[i]) < 1)
      {
        state->error = 67; /*text chunk too small*/
        break;
      }
      if(state->encoder.text_compression)
        addChunk_zTXt(&outv, info.text_keys[i], info.text_strings[i], &state->encoder.zlibsettings);
      else
        addChunk_tEXt(&outv, info.text_keys[i], info.text_strings[i]);
    }
    /*LodePNG version id in text chunk*/
    if(state->encoder.add_id)
    {
      unsigned alread_added_id_text = 0;
      for(i = 0; i < info.text_num; i++)
      {
        if(!strcmp(info.text_keys[i], "LodePNG"))
        {
          alread_added_id_text = 1;
          break;
        }
      }
      if(alread_added_id_text == 0)
        addChunk_tEXt(&outv, "LodePNG", VERSION_STRING); /*it's shorter as tEXt than as zTXt chunk*/
    }
    /*iTXt*/
    for(i = 0; i < info.itext_num; i++)
    {
      if(strlen(info.itext_keys[i]) > 79)
      {
        state->error = 66; /*text chunk too large*/
        break;
      }
      if(strlen(info.itext_keys[i]) < 1)
      {
        state->error = 67; /*text chunk too small*/
        break;
      }
      addChunk_iTXt(&outv, state->encoder.text_compression,
                    info.itext_keys[i], info.itext_langtags[i], info.itext_transkeys[i], info.itext_strings[i],
                    &state->encoder.zlibsettings);
    }

    /*unknown chunks between IDAT and IEND*/
    if(info.unknown_chunks_data[2])
    {
      state->error = addUnknownChunks(&outv, info.unknown_chunks_data[2], info.unknown_chunks_size[2]);
      if(state->error) break;
    }
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
    /*IEND*/
    addChunk_IEND(&outv);

    break; /*this isn't really a while loop; no error happened so break out now!*/
  }

  lodepng_info_cleanup(&info);
  lodepng_free(data);
  /*instead of cleaning the vector up, give it to the output*/
  *out = outv.data;
  *outsize = outv.size;

  return state->error;
}

unsigned lodepng_encode_memory(unsigned char** out, size_t* outsize, const unsigned char* image,
                               unsigned w, unsigned h, LodePNGColorType colortype, unsigned bitdepth)
{
  unsigned error;
  LodePNGState state;
  lodepng_state_init(&state);
  state.info_raw.colortype = colortype;
  state.info_raw.bitdepth = bitdepth;
  state.info_png.color.colortype = colortype;
  state.info_png.color.bitdepth = bitdepth;
  lodepng_encode(out, outsize, image, w, h, &state);
  error = state.error;
  lodepng_state_cleanup(&state);
  return error;
}

unsigned lodepng_encode32(unsigned char** out, size_t* outsize, const unsigned char* image, unsigned w, unsigned h)
{
  return lodepng_encode_memory(out, outsize, image, w, h, LCT_RGBA, 8);
}

unsigned lodepng_encode24(unsigned char** out, size_t* outsize, const unsigned char* image, unsigned w, unsigned h)
{
  return lodepng_encode_memory(out, outsize, image, w, h, LCT_RGB, 8);
}

#ifdef LODEPNG_COMPILE_DISK
unsigned lodepng_encode_file(const char* filename, const unsigned char* image, unsigned w, unsigned h,
                             LodePNGColorType colortype, unsigned bitdepth)
{
  unsigned char* buffer;
  size_t buffersize;
  unsigned error = lodepng_encode_memory(&buffer, &buffersize, image, w, h, colortype, bitdepth);
  if(!error) error = lodepng_save_file(buffer, buffersize, filename);
  lodepng_free(buffer);
  return error;
}

unsigned lodepng_encode32_file(const char* filename, const unsigned char* image, unsigned w, unsigned h)
{
  return lodepng_encode_file(filename, image, w, h, LCT_RGBA, 8);
}

unsigned lodepng_encode24_file(const char* filename, const unsigned char* image, unsigned w, unsigned h)
{
  return lodepng_encode_file(filename, image, w, h, LCT_RGB, 8);
}
#endif /*LODEPNG_COMPILE_DISK*/

void lodepng_encoder_settings_init(LodePNGEncoderSettings* settings)
{
  lodepng_compress_settings_init(&settings->zlibsettings);
  settings->filter_palette_zero = 1;
  settings->filter_strategy = LFS_MINSUM;
  settings->auto_convert = LAC_AUTO;
  settings->force_palette = 0;
  settings->predefined_filters = 0;
#ifdef LODEPNG_COMPILE_ANCILLARY_CHUNKS
  settings->add_id = 0;
  settings->text_compression = 1;
#endif /*LODEPNG_COMPILE_ANCILLARY_CHUNKS*/
}

#endif /*LODEPNG_COMPILE_ENCODER*/
#endif /*LODEPNG_COMPILE_PNG*/

#ifdef LODEPNG_COMPILE_ERROR_TEXT
/*
This returns the description of a numerical error code in English. This is also
the documentation of all the error codes.
*/
const char* lodepng_error_text(unsigned code)
{
  switch(code)
  {
    case 0: return "no error, everything went ok";
    case 1: return "nothing done yet"; /*the Encoder/Decoder has done nothing yet, error checking makes no sense yet*/
    case 10: return "end of input memory reached without huffman end code"; /*while huffman decoding*/
    case 11: return "error in code tree made it jump outside of huffman tree"; /*while huffman decoding*/
    case 13: return "problem while processing dynamic deflate block";
    case 14: return "problem while processing dynamic deflate block";
    case 15: return "problem while processing dynamic deflate block";
    case 16: return "unexisting code while processing dynamic deflate block";
    case 17: return "end of out buffer memory reached while inflating";
    case 18: return "invalid distance code while inflating";
    case 19: return "end of out buffer memory reached while inflating";
    case 20: return "invalid deflate block BTYPE encountered while decoding";
    case 21: return "NLEN is not ones complement of LEN in a deflate block";
     /*end of out buffer memory reached while inflating:
     This can happen if the inflated deflate data is longer than the amount of bytes required to fill up
     all the pixels of the image, given the color depth and image dimensions. Something that doesn't
     happen in a normal, well encoded, PNG image.*/
    case 22: return "end of out buffer memory reached while inflating";
    case 23: return "end of in buffer memory reached while inflating";
    case 24: return "invalid FCHECK in zlib header";
    case 25: return "invalid compression method in zlib header";
    case 26: return "FDICT encountered in zlib header while it's not used for PNG";
    case 27: return "PNG file is smaller than a PNG header";
    /*Checks the magic file header, the first 8 bytes of the PNG file*/
    case 28: return "incorrect PNG signature, it's no PNG or corrupted";
    case 29: return "first chunk is not the header chunk";
    case 30: return "chunk length too large, chunk broken off at end of file";
    case 31: return "illegal PNG color type or bpp";
    case 32: return "illegal PNG compression method";
    case 33: return "illegal PNG filter method";
    case 34: return "illegal PNG interlace method";
    case 35: return "chunk length of a chunk is too large or the chunk too small";
    case 36: return "illegal PNG filter type encountered";
    case 37: return "illegal bit depth for this color type given";
    case 38: return "the palette is too big"; /*more than 256 colors*/
    case 39: return "more palette alpha values given in tRNS chunk than there are colors in the palette";
    case 40: return "tRNS chunk has wrong size for greyscale image";
    case 41: return "tRNS chunk has wrong size for RGB image";
    case 42: return "tRNS chunk appeared while it was not allowed for this color type";
    case 43: return "bKGD chunk has wrong size for palette image";
    case 44: return "bKGD chunk has wrong size for greyscale image";
    case 45: return "bKGD chunk has wrong size for RGB image";
    /*Is the palette too small?*/
    case 46: return "a value in indexed image is larger than the palette size (bitdepth = 8)";
    /*Is the palette too small?*/
    case 47: return "a value in indexed image is larger than the palette size (bitdepth < 8)";
    /*the input data is empty, maybe a PNG file doesn't exist or is in the wrong path*/
    case 48: return "empty input or file doesn't exist";
    case 49: return "jumped past memory while generating dynamic huffman tree";
    case 50: return "jumped past memory while generating dynamic huffman tree";
    case 51: return "jumped past memory while inflating huffman block";
    case 52: return "jumped past memory while inflating";
    case 53: return "size of zlib data too small";
    case 54: return "repeat symbol in tree while there was no value symbol yet";
    /*jumped past tree while generating huffman tree, this could be when the
    tree will have more leaves than symbols after generating it out of the
    given lenghts. They call this an oversubscribed dynamic bit lengths tree in zlib.*/
    case 55: return "jumped past tree while generating huffman tree";
    case 56: return "given output image colortype or bitdepth not supported for color conversion";
    case 57: return "invalid CRC encountered (checking CRC can be disabled)";
    case 58: return "invalid ADLER32 encountered (checking ADLER32 can be disabled)";
    case 59: return "requested color conversion not supported";
    case 60: return "invalid window size given in the settings of the encoder (must be 0-32768)";
    case 61: return "invalid BTYPE given in the settings of the encoder (only 0, 1 and 2 are allowed)";
    /*LodePNG leaves the choice of RGB to greyscale conversion formula to the user.*/
    case 62: return "conversion from color to greyscale not supported";
    case 63: return "length of a chunk too long, max allowed for PNG is 2147483647 bytes per chunk"; /*(2^31-1)*/
    /*this would result in the inability of a deflated block to ever contain an end code. It must be at least 1.*/
    case 64: return "the length of the END symbol 256 in the Huffman tree is 0";
    case 66: return "the length of a text chunk keyword given to the encoder is longer than the maximum of 79 bytes";
    case 67: return "the length of a text chunk keyword given to the encoder is smaller than the minimum of 1 byte";
    case 68: return "tried to encode a PLTE chunk with a palette that has less than 1 or more than 256 colors";
    case 69: return "unknown chunk type with 'critical' flag encountered by the decoder";
    case 71: return "unexisting interlace mode given to encoder (must be 0 or 1)";
    case 72: return "while decoding, unexisting compression method encountering in zTXt or iTXt chunk (it must be 0)";
    case 73: return "invalid tIME chunk size";
    case 74: return "invalid pHYs chunk size";
    /*length could be wrong, or data chopped off*/
    case 75: return "no null termination char found while decoding text chunk";
    case 76: return "iTXt chunk too short to contain required bytes";
    case 77: return "integer overflow in buffer size";
    case 78: return "failed to open file for reading"; /*file doesn't exist or couldn't be opened for reading*/
    case 79: return "failed to open file for writing";
    case 80: return "tried creating a tree of 0 symbols";
    case 81: return "lazy matching at pos 0 is impossible";
    case 82: return "color conversion to palette requested while a color isn't in palette";
    case 83: return "memory allocation failed";
    case 84: return "given image too small to contain all pixels to be encoded";
    case 85: return "internal color conversion bug";
    case 86: return "impossible offset in lz77 encoding (internal bug)";
    case 87: return "must provide custom zlib function pointer if LODEPNG_COMPILE_ZLIB is not defined";
    case 88: return "invalid filter strategy given for LodePNGEncoderSettings.filter_strategy";
    case 89: return "text chunk keyword too short or long: must have size 1-79";
  }
  return "unknown error code";
}
#endif /*LODEPNG_COMPILE_ERROR_TEXT*/

/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */
/* // C++ Wrapper                                                          // */
/* ////////////////////////////////////////////////////////////////////////// */
/* ////////////////////////////////////////////////////////////////////////// */


#ifdef LODEPNG_COMPILE_CPP
namespace lodepng
{

#ifdef LODEPNG_COMPILE_DISK
void load_file(std::vector<unsigned char>& buffer, const std::string& filename)
{
  std::ifstream file(filename.c_str(), std::ios::in|std::ios::binary|std::ios::ate);

  /*get filesize*/
  std::streamsize size = 0;
  if(file.seekg(0, std::ios::end).good()) size = file.tellg();
  if(file.seekg(0, std::ios::beg).good()) size -= file.tellg();

  /*read contents of the file into the vector*/
  buffer.resize(size_t(size));
  if(size > 0) file.read((char*)(&buffer[0]), size);
}

/*write given buffer to the file, overwriting the file, it doesn't append to it.*/
void save_file(const std::vector<unsigned char>& buffer, const std::string& filename)
{
  std::ofstream file(filename.c_str(), std::ios::out|std::ios::binary);
  file.write(buffer.empty() ? 0 : (char*)&buffer[0], std::streamsize(buffer.size()));
}
#endif //LODEPNG_COMPILE_DISK

#ifdef LODEPNG_COMPILE_ZLIB
#ifdef LODEPNG_COMPILE_DECODER
unsigned decompress(std::vector<unsigned char>& out, const unsigned char* in, size_t insize,
                    const LodePNGDecompressSettings& settings)
{
  unsigned char* buffer = 0;
  size_t buffersize = 0;
  unsigned error = zlib_decompress(&buffer, &buffersize, in, insize, &settings);
  if(buffer)
  {
    out.insert(out.end(), &buffer[0], &buffer[buffersize]);
    lodepng_free(buffer);
  }
  return error;
}

unsigned decompress(std::vector<unsigned char>& out, const std::vector<unsigned char>& in,
                    const LodePNGDecompressSettings& settings)
{
  return decompress(out, in.empty() ? 0 : &in[0], in.size(), settings);
}
#endif //LODEPNG_COMPILE_DECODER

#ifdef LODEPNG_COMPILE_ENCODER
unsigned compress(std::vector<unsigned char>& out, const unsigned char* in, size_t insize,
                  const LodePNGCompressSettings& settings)
{
  unsigned char* buffer = 0;
  size_t buffersize = 0;
  unsigned error = zlib_compress(&buffer, &buffersize, in, insize, &settings);
  if(buffer)
  {
    out.insert(out.end(), &buffer[0], &buffer[buffersize]);
    lodepng_free(buffer);
  }
  return error;
}

unsigned compress(std::vector<unsigned char>& out, const std::vector<unsigned char>& in,
                  const LodePNGCompressSettings& settings)
{
  return compress(out, in.empty() ? 0 : &in[0], in.size(), settings);
}
#endif //LODEPNG_COMPILE_ENCODER
#endif //LODEPNG_COMPILE_ZLIB


#ifdef LODEPNG_COMPILE_PNG

State::State()
{
  lodepng_state_init(this);
}

State::State(const State& other)
{
  lodepng_state_init(this);
  lodepng_state_copy(this, &other);
}

State::~State()
{
  lodepng_state_cleanup(this);
}

State& State::operator=(const State& other)
{
  lodepng_state_copy(this, &other);
  return *this;
}

#ifdef LODEPNG_COMPILE_DECODER

unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h, const unsigned char* in,
                size_t insize, LodePNGColorType colortype, unsigned bitdepth)
{
  unsigned char* buffer;
  unsigned error = lodepng_decode_memory(&buffer, &w, &h, in, insize, colortype, bitdepth);
  if(buffer && !error)
  {
    State state;
    state.info_raw.colortype = colortype;
    state.info_raw.bitdepth = bitdepth;
    size_t buffersize = lodepng_get_raw_size(w, h, &state.info_raw);
    out.insert(out.end(), &buffer[0], &buffer[buffersize]);
    lodepng_free(buffer);
  }
  return error;
}

unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h,
                const std::vector<unsigned char>& in, LodePNGColorType colortype, unsigned bitdepth)
{
  return decode(out, w, h, in.empty() ? 0 : &in[0], (unsigned)in.size(), colortype, bitdepth);
}

unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h,
                State& state,
                const unsigned char* in, size_t insize)
{
  unsigned char* buffer;
  unsigned error = lodepng_decode(&buffer, &w, &h, &state, in, insize);
  if(buffer && !error)
  {
    size_t buffersize = lodepng_get_raw_size(w, h, &state.info_raw);
    out.insert(out.end(), &buffer[0], &buffer[buffersize]);
    lodepng_free(buffer);
  }
  return error;
}

unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h,
                State& state,
                const std::vector<unsigned char>& in)
{
  return decode(out, w, h, state, in.empty() ? 0 : &in[0], in.size());
}

#ifdef LODEPNG_COMPILE_DISK
unsigned decode(std::vector<unsigned char>& out, unsigned& w, unsigned& h, const std::string& filename,
                LodePNGColorType colortype, unsigned bitdepth)
{
  std::vector<unsigned char> buffer;
  load_file(buffer, filename);
  return decode(out, w, h, buffer, colortype, bitdepth);
}
#endif //LODEPNG_COMPILE_DECODER
#endif //LODEPNG_COMPILE_DISK

#ifdef LODEPNG_COMPILE_ENCODER
unsigned encode(std::vector<unsigned char>& out, const unsigned char* in, unsigned w, unsigned h,
                LodePNGColorType colortype, unsigned bitdepth)
{
  unsigned char* buffer;
  size_t buffersize;
  unsigned error = lodepng_encode_memory(&buffer, &buffersize, in, w, h, colortype, bitdepth);
  if(buffer)
  {
    out.insert(out.end(), &buffer[0], &buffer[buffersize]);
    lodepng_free(buffer);
  }
  return error;
}

unsigned encode(std::vector<unsigned char>& out,
                const std::vector<unsigned char>& in, unsigned w, unsigned h,
                LodePNGColorType colortype, unsigned bitdepth)
{
  if(lodepng_get_raw_size_lct(w, h, colortype, bitdepth) > in.size()) return 84;
  return encode(out, in.empty() ? 0 : &in[0], w, h, colortype, bitdepth);
}

unsigned encode(std::vector<unsigned char>& out,
                const unsigned char* in, unsigned w, unsigned h,
                State& state)
{
  unsigned char* buffer;
  size_t buffersize;
  unsigned error = lodepng_encode(&buffer, &buffersize, in, w, h, &state);
  if(buffer)
  {
    out.insert(out.end(), &buffer[0], &buffer[buffersize]);
    lodepng_free(buffer);
  }
  return error;
}

unsigned encode(std::vector<unsigned char>& out,
                const std::vector<unsigned char>& in, unsigned w, unsigned h,
                State& state)
{
  if(lodepng_get_raw_size(w, h, &state.info_raw) > in.size()) return 84;
  return encode(out, in.empty() ? 0 : &in[0], w, h, state);
}

#ifdef LODEPNG_COMPILE_DISK
unsigned encode(const std::string& filename,
                const unsigned char* in, unsigned w, unsigned h,
                LodePNGColorType colortype, unsigned bitdepth)
{
  std::vector<unsigned char> buffer;
  unsigned error = encode(buffer, in, w, h, colortype, bitdepth);
  if(!error) save_file(buffer, filename);
  return error;
}

unsigned encode(const std::string& filename,
                const std::vector<unsigned char>& in, unsigned w, unsigned h,
                LodePNGColorType colortype, unsigned bitdepth)
{
  if(lodepng_get_raw_size_lct(w, h, colortype, bitdepth) > in.size()) return 84;
  return encode(filename, in.empty() ? 0 : &in[0], w, h, colortype, bitdepth);
}
#endif //LODEPNG_COMPILE_DISK
#endif //LODEPNG_COMPILE_ENCODER
#endif //LODEPNG_COMPILE_PNG
} //namespace lodepng
#endif /*LODEPNG_COMPILE_CPP*/

/***********************************************************************
STB_IMAGE.CPP
***********************************************************************/

#define STBI_NO_FAILURE_STRINGS

#ifndef STBI_NO_HDR
#include <math.h>  // ldexp
#include <string.h> // strcmp, strtok
#endif

#ifndef STBI_NO_STDIO
#endif
#include <assert.h>

#ifndef _MSC_VER
   #ifdef __cplusplus
   #define stbi_inline inline
   #else
   #define stbi_inline
   #endif
#else
   #define stbi_inline __forceinline
#endif

#ifdef _MSC_VER
FILE * openFile(const char * fileName)
{
	FILE * f;
	fopen_s(&f, fileName, "rb");
	return f;
}
#else
FILE * openFile(const char * fileName)
{
	return fopen(fileName, "rb");
}
#endif

// implementation:
typedef unsigned char  uint8;
typedef unsigned short uint16;
typedef   signed short  int16;
typedef unsigned int   uint32;
typedef   signed int    int32;
typedef unsigned int   uint;

// should produce compiler error if size is wrong
typedef unsigned char validate_uint32[sizeof(uint32)==4 ? 1 : -1];

#if defined(STBI_NO_STDIO) && !defined(STBI_NO_WRITE)
#define STBI_NO_WRITE
#endif
template<typename T>
inline void NotUsedProc(const T & /*v*/)
{}
#define STBI_NOTUSED(v)  NotUsedProc(v)

#ifdef _MSC_VER
#define STBI_HAS_LROTL
#endif

#ifdef STBI_HAS_LROTL
   #define stbi_lrot(x,y)  _lrotl(x,y)
#else
   #define stbi_lrot(x,y)  (((x) << (y)) | ((x) >> (32 - (y))))
#endif

///////////////////////////////////////////////
//
//  stbi struct and start_xxx functions

// stbi structure is our basic context used by all images, so it
// contains all the IO context, plus some basic image information
typedef struct
{
   uint32 img_x, img_y;
   int img_n, img_out_n;
   
   stbi_io_callbacks io;
   void *io_user_data;

   int read_from_callbacks;
   int buflen;
   uint8 buffer_start[128];

   uint8 *img_buffer, *img_buffer_end;
   uint8 *img_buffer_original;
} stbi;


static void refill_buffer(stbi *s);

// initialize a memory-decode context
static void start_mem(stbi *s, uint8 const *buffer, int len)
{
   s->io.read = NULL;
   s->read_from_callbacks = 0;
   s->img_buffer = s->img_buffer_original = (uint8 *) buffer;
   s->img_buffer_end = (uint8 *) buffer+len;
}

// initialize a callback-based context
static void start_callbacks(stbi *s, stbi_io_callbacks *c, void *user)
{
   s->io = *c;
   s->io_user_data = user;
   s->buflen = sizeof(s->buffer_start);
   s->read_from_callbacks = 1;
   s->img_buffer_original = s->buffer_start;
   refill_buffer(s);
}

#ifndef STBI_NO_STDIO

static int stdio_read(void *user, char *data, int size)
{
   return (int) fread(data,1,size,(FILE*) user);
}

static void stdio_skip(void *user, unsigned n)
{
   fseek((FILE*) user, n, SEEK_CUR);
}

static int stdio_eof(void *user)
{
   return feof((FILE*) user);
}

static stbi_io_callbacks stbi_stdio_callbacks =
{
   stdio_read,
   stdio_skip,
   stdio_eof,
};

static void start_file(stbi *s, FILE *f)
{
   start_callbacks(s, &stbi_stdio_callbacks, (void *) f);
}

//static void stop_file(stbi *s) { }

#endif // !STBI_NO_STDIO

static void stbi_rewind(stbi *s)
{
   // conceptually rewind SHOULD rewind to the beginning of the stream,
   // but we just rewind to the beginning of the initial buffer, because
   // we only use it after doing 'test', which only ever looks at at most 92 bytes
   s->img_buffer = s->img_buffer_original;
}

static int      stbi_jpeg_test(stbi *s);
static stbi_uc *stbi_jpeg_load(stbi *s, int *x, int *y, int *comp, int req_comp);
static int      stbi_jpeg_info(stbi *s, int *x, int *y, int *comp);
static int      stbi_png_test(stbi *s);
static stbi_uc *stbi_png_load(stbi *s, int *x, int *y, int *comp, int req_comp);
static int      stbi_png_info(stbi *s, int *x, int *y, int *comp);
static int      stbi_bmp_test(stbi *s);
static stbi_uc *stbi_bmp_load(stbi *s, int *x, int *y, int *comp, int req_comp);
static int      stbi_tga_test(stbi *s);
static stbi_uc *stbi_tga_load(stbi *s, int *x, int *y, int *comp, int req_comp);
static int      stbi_tga_info(stbi *s, int *x, int *y, int *comp);
static int      stbi_psd_test(stbi *s);
static stbi_uc *stbi_psd_load(stbi *s, int *x, int *y, int *comp, int req_comp);
static int      stbi_hdr_test(stbi *s);
static float   *stbi_hdr_load(stbi *s, int *x, int *y, int *comp, int req_comp);
static int      stbi_pic_test(stbi *s);
static stbi_uc *stbi_pic_load(stbi *s, int *x, int *y, int *comp, int req_comp);
static int      stbi_gif_test(stbi *s);
static stbi_uc *stbi_gif_load(stbi *s, int *x, int *y, int *comp, int req_comp);
static int      stbi_gif_info(stbi *s, int *x, int *y, int *comp);


// this is not threadsafe
static const char *failure_reason;

const char *stbi_failure_reason(void)
{
   return failure_reason;
}

#ifndef STBI_NO_FAILURE_STRINGS
static int e(const char *str)
{
   failure_reason = str;
   return 0;
}
#endif

// e - error
// epf - error returning pointer to float
// epuc - error returning pointer to unsigned char

#ifdef STBI_NO_FAILURE_STRINGS
   #define e(x,y)  0
#elif defined(STBI_FAILURE_USERMSG)
   #define e(x,y)  e(y)
#else
   #define e(x,y)  e(x)
#endif

#define epf(x,y)   ((float *) (e(x,y)?NULL:NULL))
#define epuc(x,y)  ((unsigned char *) (e(x,y)?NULL:NULL))

void stbi_image_free(void *retval_from_stbi_load)
{
   free(retval_from_stbi_load);
}

#ifndef STBI_NO_HDR
static float   *ldr_to_hdr(stbi_uc *data, int x, int y, int comp);
static stbi_uc *hdr_to_ldr(float   *data, int x, int y, int comp);
#endif

static unsigned char *stbi_load_main(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   if (stbi_jpeg_test(s)) return stbi_jpeg_load(s,x,y,comp,req_comp);
   if (stbi_png_test(s))  return stbi_png_load(s,x,y,comp,req_comp);
   if (stbi_bmp_test(s))  return stbi_bmp_load(s,x,y,comp,req_comp);
   if (stbi_gif_test(s))  return stbi_gif_load(s,x,y,comp,req_comp);
   if (stbi_psd_test(s))  return stbi_psd_load(s,x,y,comp,req_comp);
   if (stbi_pic_test(s))  return stbi_pic_load(s,x,y,comp,req_comp);

   #ifndef STBI_NO_HDR
   if (stbi_hdr_test(s)) {
      float *hdr = stbi_hdr_load(s, x,y,comp,req_comp);
      return hdr_to_ldr(hdr, *x, *y, req_comp ? req_comp : *comp);
   }
   #endif

   // test tga last because it's a crappy test!
   if (stbi_tga_test(s))
      return stbi_tga_load(s,x,y,comp,req_comp);
   return epuc("unknown image type", "Image not of any known type, or corrupt");
}

#ifndef STBI_NO_STDIO
unsigned char *stbi_load(char const *filename, int *x, int *y, int *comp, int req_comp)
{
   FILE *f = openFile(filename);
   unsigned char *result;
   if (!f) return epuc("can't fopen", "Unable to open file");
   result = stbi_load_from_file(f,x,y,comp,req_comp);
   fclose(f);
   return result;
}

unsigned char *stbi_load_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   stbi s;
   start_file(&s,f);
   return stbi_load_main(&s,x,y,comp,req_comp);
}
#endif //!STBI_NO_STDIO

unsigned char *stbi_load_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   stbi s;
   start_mem(&s,buffer,len);
   return stbi_load_main(&s,x,y,comp,req_comp);
}

unsigned char *stbi_load_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp)
{
   stbi s;
   start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
   return stbi_load_main(&s,x,y,comp,req_comp);
}

#ifndef STBI_NO_HDR

float *stbi_loadf_main(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   unsigned char *data;
   #ifndef STBI_NO_HDR
   if (stbi_hdr_test(s))
      return stbi_hdr_load(s,x,y,comp,req_comp);
   #endif
   data = stbi_load_main(s, x, y, comp, req_comp);
   if (data)
      return ldr_to_hdr(data, *x, *y, req_comp ? req_comp : *comp);
   return epf("unknown image type", "Image not of any known type, or corrupt");
}

float *stbi_loadf_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp, int req_comp)
{
   stbi s;
   start_mem(&s,buffer,len);
   return stbi_loadf_main(&s,x,y,comp,req_comp);
}

float *stbi_loadf_from_callbacks(stbi_io_callbacks const *clbk, void *user, int *x, int *y, int *comp, int req_comp)
{
   stbi s;
   start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
   return stbi_loadf_main(&s,x,y,comp,req_comp);
}

#ifndef STBI_NO_STDIO
float *stbi_loadf(char const *filename, int *x, int *y, int *comp, int req_comp)
{
   FILE *f = openFile(filename);
   float *result;
   if (!f) return epf("can't fopen", "Unable to open file");
   result = stbi_loadf_from_file(f,x,y,comp,req_comp);
   fclose(f);
   return result;
}

float *stbi_loadf_from_file(FILE *f, int *x, int *y, int *comp, int req_comp)
{
   stbi s;
   start_file(&s,f);
   return stbi_loadf_main(&s,x,y,comp,req_comp);
}
#endif // !STBI_NO_STDIO

#endif // !STBI_NO_HDR

// these is-hdr-or-not is defined independent of whether STBI_NO_HDR is
// defined, for API simplicity; if STBI_NO_HDR is defined, it always
// reports false!

int stbi_is_hdr_from_memory(stbi_uc const *buffer, int len)
{
   #ifndef STBI_NO_HDR
   stbi s;
   start_mem(&s,buffer,len);
   return stbi_hdr_test(&s);
   #else
   STBI_NOTUSED(buffer);
   STBI_NOTUSED(len);
   return 0;
   #endif
}

#ifndef STBI_NO_STDIO
extern int      stbi_is_hdr          (char const *filename)
{
   FILE *f = openFile(filename);
   int result=0;
   if (f) {
      result = stbi_is_hdr_from_file(f);
      fclose(f);
   }
   return result;
}

extern int      stbi_is_hdr_from_file(FILE *f)
{
   #ifndef STBI_NO_HDR
   stbi s;
   start_file(&s,f);
   return stbi_hdr_test(&s);
   #else
   return 0;
   #endif
}
#endif // !STBI_NO_STDIO

extern int      stbi_is_hdr_from_callbacks(stbi_io_callbacks const *clbk, void *user)
{
   #ifndef STBI_NO_HDR
   stbi s;
   start_callbacks(&s, (stbi_io_callbacks *) clbk, user);
   return stbi_hdr_test(&s);
   #else
   return 0;
   #endif
}

#ifndef STBI_NO_HDR
static float h2l_gamma_i=1.0f/2.2f, h2l_scale_i=1.0f;
static float l2h_gamma=2.2f, l2h_scale=1.0f;

void   stbi_hdr_to_ldr_gamma(float gamma) { h2l_gamma_i = 1/gamma; }
void   stbi_hdr_to_ldr_scale(float scale) { h2l_scale_i = 1/scale; }

void   stbi_ldr_to_hdr_gamma(float gamma) { l2h_gamma = gamma; }
void   stbi_ldr_to_hdr_scale(float scale) { l2h_scale = scale; }
#endif


//////////////////////////////////////////////////////////////////////////////
//
// Common code used by all image loaders
//

enum
{
   SCAN_load=0,
   SCAN_type,
   SCAN_header
};

static void refill_buffer(stbi *s)
{
   int n = (s->io.read)(s->io_user_data,(char*)s->buffer_start,s->buflen);
   if (n == 0) {
      // at end of file, treat same as if from memory
      s->read_from_callbacks = 0;
      s->img_buffer = s->img_buffer_end-1;
      *s->img_buffer = 0;
   } else {
      s->img_buffer = s->buffer_start;
      s->img_buffer_end = s->buffer_start + n;
   }
}

stbi_inline static int get8(stbi *s)
{
   if (s->img_buffer < s->img_buffer_end)
      return *s->img_buffer++;
   if (s->read_from_callbacks) {
      refill_buffer(s);
      return *s->img_buffer++;
   }
   return 0;
}

stbi_inline static int at_eof(stbi *s)
{
   if (s->io.read) {
      if (!(s->io.eof)(s->io_user_data)) return 0;
      // if feof() is true, check if buffer = end
      // special case: we've only got the special 0 character at the end
      if (s->read_from_callbacks == 0) return 1;
   }

   return s->img_buffer >= s->img_buffer_end;   
}

stbi_inline static uint8 get8u(stbi *s)
{
   return (uint8) get8(s);
}

static void skip(stbi *s, int n)
{
   if (s->io.read) {
      int blen = (int)(s->img_buffer_end - s->img_buffer);
      if (blen < n) {
         s->img_buffer = s->img_buffer_end;
         (s->io.skip)(s->io_user_data, n - blen);
         return;
      }
   }
   s->img_buffer += n;
}

static int getn(stbi *s, stbi_uc *buffer, int n)
{
   if (s->io.read) {
      int blen = (int)(s->img_buffer_end - s->img_buffer);
      if (blen < n) {
         int res, count;

         memcpy(buffer, s->img_buffer, blen);
         
         count = (s->io.read)(s->io_user_data, (char*) buffer + blen, n - blen);
         res = (count == (n-blen));
         s->img_buffer = s->img_buffer_end;
         return res;
      }
   }

   if (s->img_buffer+n <= s->img_buffer_end) {
      memcpy(buffer, s->img_buffer, n);
      s->img_buffer += n;
      return 1;
   } else
      return 0;
}

static int get16(stbi *s)
{
   int z = get8(s);
   return (z << 8) + get8(s);
}

static uint32 get32(stbi *s)
{
   uint32 z = get16(s);
   return (z << 16) + get16(s);
}

static int get16le(stbi *s)
{
   int z = get8(s);
   return z + (get8(s) << 8);
}

static uint32 get32le(stbi *s)
{
   uint32 z = get16le(s);
   return z + (get16le(s) << 16);
}

//////////////////////////////////////////////////////////////////////////////
//
//  generic converter from built-in img_n to req_comp
//    individual types do this automatically as much as possible (e.g. jpeg
//    does all cases internally since it needs to colorspace convert anyway,
//    and it never has alpha, so very few cases ). png can automatically
//    interleave an alpha=255 channel, but falls back to this for other cases
//
//  assume data buffer is malloced, so malloc a new one and free that one
//  only failure mode is malloc failing

static uint8 compute_y(int r, int g, int b)
{
   return (uint8) (((r*77) + (g*150) +  (29*b)) >> 8);
}

static unsigned char *convert_format(unsigned char *data, int img_n, int req_comp, uint x, uint y)
{
   int i,j;
   unsigned char *good;

   if (req_comp == img_n) return data;
   assert(req_comp >= 1 && req_comp <= 4);

   good = (unsigned char *) malloc(req_comp * x * y);
   if (good == NULL) {
      free(data);
      return epuc("outofmem", "Out of memory");
   }

   for (j=0; j < (int) y; ++j) {
      unsigned char *src  = data + j * x * img_n   ;
      unsigned char *dest = good + j * x * req_comp;

      #define COMBO(a,b)  ((a)*8+(b))
      #define CASE(a,b)   case COMBO(a,b): for(i=x-1; i >= 0; --i, src += a, dest += b)
      // convert source image with img_n components to one with req_comp components;
      // avoid switch per pixel, so use switch per scanline and massive macros
      switch (COMBO(img_n, req_comp)) {
         CASE(1,2) dest[0]=src[0], dest[1]=255; break;
         CASE(1,3) dest[0]=dest[1]=dest[2]=src[0]; break;
         CASE(1,4) dest[0]=dest[1]=dest[2]=src[0], dest[3]=255; break;
         CASE(2,1) dest[0]=src[0]; break;
         CASE(2,3) dest[0]=dest[1]=dest[2]=src[0]; break;
         CASE(2,4) dest[0]=dest[1]=dest[2]=src[0], dest[3]=src[1]; break;
         CASE(3,4) dest[0]=src[0],dest[1]=src[1],dest[2]=src[2],dest[3]=255; break;
         CASE(3,1) dest[0]=compute_y(src[0],src[1],src[2]); break;
         CASE(3,2) dest[0]=compute_y(src[0],src[1],src[2]), dest[1] = 255; break;
         CASE(4,1) dest[0]=compute_y(src[0],src[1],src[2]); break;
         CASE(4,2) dest[0]=compute_y(src[0],src[1],src[2]), dest[1] = src[3]; break;
         CASE(4,3) dest[0]=src[0],dest[1]=src[1],dest[2]=src[2]; break;
         default: assert(0);
      }
      #undef CASE
   }

   free(data);
   return good;
}

#ifndef STBI_NO_HDR
static float   *ldr_to_hdr(stbi_uc *data, int x, int y, int comp)
{
   int i,k,n;
   float *output = (float *) malloc(x * y * comp * sizeof(float));
   if (output == NULL) { free(data); return epf("outofmem", "Out of memory"); }
   // compute number of non-alpha components
   if (comp & 1) n = comp; else n = comp-1;
   for (i=0; i < x*y; ++i) {
      for (k=0; k < n; ++k) {
         output[i*comp + k] = (float) pow(data[i*comp+k]/255.0f, l2h_gamma) * l2h_scale;
      }
      if (k < comp) output[i*comp + k] = data[i*comp+k]/255.0f;
   }
   free(data);
   return output;
}

#define float2int(x)   ((int) (x))
static stbi_uc *hdr_to_ldr(float   *data, int x, int y, int comp)
{
   int i,k,n;
   stbi_uc *output = (stbi_uc *) malloc(x * y * comp);
   if (output == NULL) { free(data); return epuc("outofmem", "Out of memory"); }
   // compute number of non-alpha components
   if (comp & 1) n = comp; else n = comp-1;
   for (i=0; i < x*y; ++i) {
      for (k=0; k < n; ++k) {
         float z = (float) pow(data[i*comp+k]*h2l_scale_i, h2l_gamma_i) * 255 + 0.5f;
         if (z < 0) z = 0;
         if (z > 255) z = 255;
         output[i*comp + k] = (uint8) float2int(z);
      }
      if (k < comp) {
         float z = data[i*comp+k] * 255 + 0.5f;
         if (z < 0) z = 0;
         if (z > 255) z = 255;
         output[i*comp + k] = (uint8) float2int(z);
      }
   }
   free(data);
   return output;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
//  "baseline" JPEG/JFIF decoder (not actually fully baseline implementation)
//
//    simple implementation
//      - channel subsampling of at most 2 in each dimension
//      - doesn't support delayed output of y-dimension
//      - simple interface (only one output format: 8-bit interleaved RGB)
//      - doesn't try to recover corrupt jpegs
//      - doesn't allow partial loading, loading multiple at once
//      - still fast on x86 (copying globals into locals doesn't help x86)
//      - allocates lots of intermediate memory (full size of all components)
//        - non-interleaved case requires this anyway
//        - allows good upsampling (see next)
//    high-quality
//      - upsampled channels are bilinearly interpolated, even across blocks
//      - quality integer IDCT derived from IJG's 'slow'
//    performance
//      - fast huffman; reasonable integer IDCT
//      - uses a lot of intermediate memory, could cache poorly
//      - load http://nothings.org/remote/anemones.jpg 3 times on 2.8Ghz P4
//          stb_jpeg:   1.34 seconds (MSVC6, default release build)
//          stb_jpeg:   1.06 seconds (MSVC6, processor = Pentium Pro)
//          IJL11.dll:  1.08 seconds (compiled by intel)
//          IJG 1998:   0.98 seconds (MSVC6, makefile provided by IJG)
//          IJG 1998:   0.95 seconds (MSVC6, makefile + proc=PPro)

// huffman decoding acceleration
#define FAST_BITS   9  // larger handles more cases; smaller stomps less cache

typedef struct
{
   uint8  fast[1 << FAST_BITS];
   // weirdly, repacking this into AoS is a 10% speed loss, instead of a win
   uint16 code[256];
   uint8  values[256];
   uint8  size[257];
   unsigned int maxcode[18];
   int    delta[17];   // old 'firstsymbol' - old 'firstcode'
} huffman;

typedef struct
{
   #ifdef STBI_SIMD
   unsigned short dequant2[4][64];
   #endif
   stbi *s;
   huffman huff_dc[4];
   huffman huff_ac[4];
   uint8 dequant[4][64];

// sizes for components, interleaved MCUs
   int img_h_max, img_v_max;
   int img_mcu_x, img_mcu_y;
   int img_mcu_w, img_mcu_h;

// definition of jpeg image component
   struct
   {
      int id;
      int h,v;
      int tq;
      int hd,ha;
      int dc_pred;

      int x,y,w2,h2;
      uint8 *data;
      void *raw_data;
      uint8 *linebuf;
   } img_comp[4];

   uint32         code_buffer; // jpeg entropy-coded buffer
   int            code_bits;   // number of valid bits
   unsigned char  marker;      // marker seen while filling entropy buffer
   int            nomore;      // flag if we saw a marker so must stop

   int scan_n, order[4];
   int restart_interval, todo;
} jpeg;

static int build_huffman(huffman *h, int *count)
{
   int i,j,k=0,code;
   // build size list for each symbol (from JPEG spec)
   for (i=0; i < 16; ++i)
      for (j=0; j < count[i]; ++j)
         h->size[k++] = (uint8) (i+1);
   h->size[k] = 0;

   // compute actual symbols (from jpeg spec)
   code = 0;
   k = 0;
   for(j=1; j <= 16; ++j) {
      // compute delta to add to code to compute symbol id
      h->delta[j] = k - code;
      if (h->size[k] == j) {
         while (h->size[k] == j)
            h->code[k++] = (uint16) (code++);
         if (code-1 >= (1 << j)) return e("bad code lengths","Corrupt JPEG");
      }
      // compute largest code + 1 for this size, preshifted as needed later
      h->maxcode[j] = code << (16-j);
      code <<= 1;
   }
   h->maxcode[j] = 0xffffffff;

   // build non-spec acceleration table; 255 is flag for not-accelerated
   memset(h->fast, 255, 1 << FAST_BITS);
   for (i=0; i < k; ++i) {
      int s = h->size[i];
      if (s <= FAST_BITS) {
         int c = h->code[i] << (FAST_BITS-s);
         int m = 1 << (FAST_BITS-s);
         for (j=0; j < m; ++j) {
            h->fast[c+j] = (uint8) i;
         }
      }
   }
   return 1;
}

static void grow_buffer_unsafe(jpeg *j)
{
   do {
      int b = j->nomore ? 0 : get8(j->s);
      if (b == 0xff) {
         int c = get8(j->s);
         if (c != 0) {
            j->marker = (unsigned char) c;
            j->nomore = 1;
            return;
         }
      }
      j->code_buffer |= b << (24 - j->code_bits);
      j->code_bits += 8;
   } while (j->code_bits <= 24);
}

// (1 << n) - 1
static uint32 bmask[17]={0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767,65535};

// decode a jpeg huffman value from the bitstream
stbi_inline static int decode(jpeg *j, huffman *h)
{
   unsigned int temp;
   int c,k;

   if (j->code_bits < 16) grow_buffer_unsafe(j);

   // look at the top FAST_BITS and determine what symbol ID it is,
   // if the code is <= FAST_BITS
   c = (j->code_buffer >> (32 - FAST_BITS)) & ((1 << FAST_BITS)-1);
   k = h->fast[c];
   if (k < 255) {
      int s = h->size[k];
      if (s > j->code_bits)
         return -1;
      j->code_buffer <<= s;
      j->code_bits -= s;
      return h->values[k];
   }

   // naive test is to shift the code_buffer down so k bits are
   // valid, then test against maxcode. To speed this up, we've
   // preshifted maxcode left so that it has (16-k) 0s at the
   // end; in other words, regardless of the number of bits, it
   // wants to be compared against something shifted to have 16;
   // that way we don't need to shift inside the loop.
   temp = j->code_buffer >> 16;
   for (k=FAST_BITS+1 ; ; ++k)
      if (temp < h->maxcode[k])
         break;
   if (k == 17) {
      // error! code not found
      j->code_bits -= 16;
      return -1;
   }

   if (k > j->code_bits)
      return -1;

   // convert the huffman code to the symbol id
   c = ((j->code_buffer >> (32 - k)) & bmask[k]) + h->delta[k];
   assert((((j->code_buffer) >> (32 - h->size[c])) & bmask[h->size[c]]) == h->code[c]);

   // convert the id to a symbol
   j->code_bits -= k;
   j->code_buffer <<= k;
   return h->values[c];
}

// combined JPEG 'receive' and JPEG 'extend', since baseline
// always extends everything it receives.
stbi_inline static int extend_receive(jpeg *j, int n)
{
   unsigned int m = 1 << (n-1);
   unsigned int k;
   if (j->code_bits < n) grow_buffer_unsafe(j);

   #if 1
   k = stbi_lrot(j->code_buffer, n);
   j->code_buffer = k & ~bmask[n];
   k &= bmask[n];
   j->code_bits -= n;
   #else
   k = (j->code_buffer >> (32 - n)) & bmask[n];
   j->code_bits -= n;
   j->code_buffer <<= n;
   #endif
   // the following test is probably a random branch that won't
   // predict well. I tried to table accelerate it but failed.
   // maybe it's compiling as a conditional move?
   if (k < m)
      return (-1 << n) + k + 1;
   else
      return k;
}

// given a value that's at position X in the zigzag stream,
// where does it appear in the 8x8 matrix coded as row-major?
static uint8 dezigzag[64+15] =
{
    0,  1,  8, 16,  9,  2,  3, 10,
   17, 24, 32, 25, 18, 11,  4,  5,
   12, 19, 26, 33, 40, 48, 41, 34,
   27, 20, 13,  6,  7, 14, 21, 28,
   35, 42, 49, 56, 57, 50, 43, 36,
   29, 22, 15, 23, 30, 37, 44, 51,
   58, 59, 52, 45, 38, 31, 39, 46,
   53, 60, 61, 54, 47, 55, 62, 63,
   // let corrupt input sample past end
   63, 63, 63, 63, 63, 63, 63, 63,
   63, 63, 63, 63, 63, 63, 63
};

// decode one 64-entry block--
static int decode_block(jpeg *j, short data[64], huffman *hdc, huffman *hac, int b)
{
   int diff,dc,k;
   int t = decode(j, hdc);
   if (t < 0) return e("bad huffman code","Corrupt JPEG");

   // 0 all the ac values now so we can do it 32-bits at a time
   memset(data,0,64*sizeof(data[0]));

   diff = t ? extend_receive(j, t) : 0;
   dc = j->img_comp[b].dc_pred + diff;
   j->img_comp[b].dc_pred = dc;
   data[0] = (short) dc;

   // decode AC components, see JPEG spec
   k = 1;
   do {
      int r,s;
      int rs = decode(j, hac);
      if (rs < 0) return e("bad huffman code","Corrupt JPEG");
      s = rs & 15;
      r = rs >> 4;
      if (s == 0) {
         if (rs != 0xf0) break; // end block
         k += 16;
      } else {
         k += r;
         // decode into unzigzag'd location
         data[dezigzag[k++]] = (short) extend_receive(j,s);
      }
   } while (k < 64);
   return 1;
}

// take a -128..127 value and clamp it and convert to 0..255
stbi_inline static uint8 clamp(int x)
{
   // trick to use a single test to catch both cases
   if ((unsigned int) x > 255) {
      if (x < 0) return 0;
      if (x > 255) return 255;
   }
   return (uint8) x;
}

#define f2f(x)  (int) (((x) * 4096 + 0.5))
#define fsh(x)  ((x) << 12)

// derived from jidctint -- DCT_ISLOW
#define IDCT_1D(s0,s1,s2,s3,s4,s5,s6,s7)       \
   int t0,t1,t2,t3,p1,p2,p3,p4,p5,x0,x1,x2,x3; \
   p2 = s2;                                    \
   p3 = s6;                                    \
   p1 = (p2+p3) * f2f(0.5411961f);             \
   t2 = p1 + p3*f2f(-1.847759065f);            \
   t3 = p1 + p2*f2f( 0.765366865f);            \
   p2 = s0;                                    \
   p3 = s4;                                    \
   t0 = fsh(p2+p3);                            \
   t1 = fsh(p2-p3);                            \
   x0 = t0+t3;                                 \
   x3 = t0-t3;                                 \
   x1 = t1+t2;                                 \
   x2 = t1-t2;                                 \
   t0 = s7;                                    \
   t1 = s5;                                    \
   t2 = s3;                                    \
   t3 = s1;                                    \
   p3 = t0+t2;                                 \
   p4 = t1+t3;                                 \
   p1 = t0+t3;                                 \
   p2 = t1+t2;                                 \
   p5 = (p3+p4)*f2f( 1.175875602f);            \
   t0 = t0*f2f( 0.298631336f);                 \
   t1 = t1*f2f( 2.053119869f);                 \
   t2 = t2*f2f( 3.072711026f);                 \
   t3 = t3*f2f( 1.501321110f);                 \
   p1 = p5 + p1*f2f(-0.899976223f);            \
   p2 = p5 + p2*f2f(-2.562915447f);            \
   p3 = p3*f2f(-1.961570560f);                 \
   p4 = p4*f2f(-0.390180644f);                 \
   t3 += p1+p4;                                \
   t2 += p2+p3;                                \
   t1 += p2+p4;                                \
   t0 += p1+p3;

#ifdef STBI_SIMD
typedef unsigned short stbi_dequantize_t;
#else
typedef uint8 stbi_dequantize_t;
#endif

// .344 seconds on 3*anemones.jpg
static void idct_block(uint8 *out, int out_stride, short data[64], stbi_dequantize_t *dequantize)
{
   int i,val[64],*v=val;
   stbi_dequantize_t *dq = dequantize;
   uint8 *o;
   short *d = data;

   // columns
   for (i=0; i < 8; ++i,++d,++dq, ++v) {
      // if all zeroes, shortcut -- this avoids dequantizing 0s and IDCTing
      if (d[ 8]==0 && d[16]==0 && d[24]==0 && d[32]==0
           && d[40]==0 && d[48]==0 && d[56]==0) {
         //    no shortcut                 0     seconds
         //    (1|2|3|4|5|6|7)==0          0     seconds
         //    all separate               -0.047 seconds
         //    1 && 2|3 && 4|5 && 6|7:    -0.047 seconds
         int dcterm = d[0] * dq[0] << 2;
         v[0] = v[8] = v[16] = v[24] = v[32] = v[40] = v[48] = v[56] = dcterm;
      } else {
         IDCT_1D(d[ 0]*dq[ 0],d[ 8]*dq[ 8],d[16]*dq[16],d[24]*dq[24],
                 d[32]*dq[32],d[40]*dq[40],d[48]*dq[48],d[56]*dq[56])
         // constants scaled things up by 1<<12; let's bring them back
         // down, but keep 2 extra bits of precision
         x0 += 512; x1 += 512; x2 += 512; x3 += 512;
         v[ 0] = (x0+t3) >> 10;
         v[56] = (x0-t3) >> 10;
         v[ 8] = (x1+t2) >> 10;
         v[48] = (x1-t2) >> 10;
         v[16] = (x2+t1) >> 10;
         v[40] = (x2-t1) >> 10;
         v[24] = (x3+t0) >> 10;
         v[32] = (x3-t0) >> 10;
      }
   }

   for (i=0, v=val, o=out; i < 8; ++i,v+=8,o+=out_stride) {
      // no fast case since the first 1D IDCT spread components out
      IDCT_1D(v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7])
      // constants scaled things up by 1<<12, plus we had 1<<2 from first
      // loop, plus horizontal and vertical each scale by sqrt(8) so together
      // we've got an extra 1<<3, so 1<<17 total we need to remove.
      // so we want to round that, which means adding 0.5 * 1<<17,
      // aka 65536. Also, we'll end up with -128 to 127 that we want
      // to encode as 0..255 by adding 128, so we'll add that before the shift
      x0 += 65536 + (128<<17);
      x1 += 65536 + (128<<17);
      x2 += 65536 + (128<<17);
      x3 += 65536 + (128<<17);
      // tried computing the shifts into temps, or'ing the temps to see
      // if any were out of range, but that was slower
      o[0] = clamp((x0+t3) >> 17);
      o[7] = clamp((x0-t3) >> 17);
      o[1] = clamp((x1+t2) >> 17);
      o[6] = clamp((x1-t2) >> 17);
      o[2] = clamp((x2+t1) >> 17);
      o[5] = clamp((x2-t1) >> 17);
      o[3] = clamp((x3+t0) >> 17);
      o[4] = clamp((x3-t0) >> 17);
   }
}

#ifdef STBI_SIMD
static stbi_idct_8x8 stbi_idct_installed = idct_block;

void stbi_install_idct(stbi_idct_8x8 func)
{
   stbi_idct_installed = func;
}
#endif

#define MARKER_none  0xff
// if there's a pending marker from the entropy stream, return that
// otherwise, fetch from the stream and get a marker. if there's no
// marker, return 0xff, which is never a valid marker value
static uint8 get_marker(jpeg *j)
{
   uint8 x;
   if (j->marker != MARKER_none) { x = j->marker; j->marker = MARKER_none; return x; }
   x = get8u(j->s);
   if (x != 0xff) return MARKER_none;
   while (x == 0xff)
      x = get8u(j->s);
   return x;
}

// in each scan, we'll have scan_n components, and the order
// of the components is specified by order[]
#define RESTART(x)     ((x) >= 0xd0 && (x) <= 0xd7)

// after a restart interval, reset the entropy decoder and
// the dc prediction
static void reset(jpeg *j)
{
   j->code_bits = 0;
   j->code_buffer = 0;
   j->nomore = 0;
   j->img_comp[0].dc_pred = j->img_comp[1].dc_pred = j->img_comp[2].dc_pred = 0;
   j->marker = MARKER_none;
   j->todo = j->restart_interval ? j->restart_interval : 0x7fffffff;
   // no more than 1<<31 MCUs if no restart_interal? that's plenty safe,
   // since we don't even allow 1<<30 pixels
}

static int parse_entropy_coded_data(jpeg *z)
{
   reset(z);
   if (z->scan_n == 1) {
      int i,j;
      #ifdef STBI_SIMD
      __declspec(align(16))
      #endif
      short data[64];
      int n = z->order[0];
      // non-interleaved data, we just need to process one block at a time,
      // in trivial scanline order
      // number of blocks to do just depends on how many actual "pixels" this
      // component has, independent of interleaved MCU blocking and such
      int w = (z->img_comp[n].x+7) >> 3;
      int h = (z->img_comp[n].y+7) >> 3;
      for (j=0; j < h; ++j) {
         for (i=0; i < w; ++i) {
            if (!decode_block(z, data, z->huff_dc+z->img_comp[n].hd, z->huff_ac+z->img_comp[n].ha, n)) return 0;
            #ifdef STBI_SIMD
            stbi_idct_installed(z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8, z->img_comp[n].w2, data, z->dequant2[z->img_comp[n].tq]);
            #else
            idct_block(z->img_comp[n].data+z->img_comp[n].w2*j*8+i*8, z->img_comp[n].w2, data, z->dequant[z->img_comp[n].tq]);
            #endif
            // every data block is an MCU, so countdown the restart interval
            if (--z->todo <= 0) {
               if (z->code_bits < 24) grow_buffer_unsafe(z);
               // if it's NOT a restart, then just bail, so we get corrupt data
               // rather than no data
               if (!RESTART(z->marker)) return 1;
               reset(z);
            }
         }
      }
   } else { // interleaved!
      int i,j,k,x,y;
      short data[64];
      for (j=0; j < z->img_mcu_y; ++j) {
         for (i=0; i < z->img_mcu_x; ++i) {
            // scan an interleaved mcu... process scan_n components in order
            for (k=0; k < z->scan_n; ++k) {
               int n = z->order[k];
               // scan out an mcu's worth of this component; that's just determined
               // by the basic H and V specified for the component
               for (y=0; y < z->img_comp[n].v; ++y) {
                  for (x=0; x < z->img_comp[n].h; ++x) {
                     int x2 = (i*z->img_comp[n].h + x)*8;
                     int y2 = (j*z->img_comp[n].v + y)*8;
                     if (!decode_block(z, data, z->huff_dc+z->img_comp[n].hd, z->huff_ac+z->img_comp[n].ha, n)) return 0;
                     #ifdef STBI_SIMD
                     stbi_idct_installed(z->img_comp[n].data+z->img_comp[n].w2*y2+x2, z->img_comp[n].w2, data, z->dequant2[z->img_comp[n].tq]);
                     #else
                     idct_block(z->img_comp[n].data+z->img_comp[n].w2*y2+x2, z->img_comp[n].w2, data, z->dequant[z->img_comp[n].tq]);
                     #endif
                  }
               }
            }
            // after all interleaved components, that's an interleaved MCU,
            // so now count down the restart interval
            if (--z->todo <= 0) {
               if (z->code_bits < 24) grow_buffer_unsafe(z);
               // if it's NOT a restart, then just bail, so we get corrupt data
               // rather than no data
               if (!RESTART(z->marker)) return 1;
               reset(z);
            }
         }
      }
   }
   return 1;
}

static int process_marker(jpeg *z, int m)
{
   int L;
   switch (m) {
      case MARKER_none: // no marker found
         return e("expected marker","Corrupt JPEG");

      case 0xC2: // SOF - progressive
         return e("progressive jpeg","JPEG format not supported (progressive)");

      case 0xDD: // DRI - specify restart interval
         if (get16(z->s) != 4) return e("bad DRI len","Corrupt JPEG");
         z->restart_interval = get16(z->s);
         return 1;

      case 0xDB: // DQT - define quantization table
         L = get16(z->s)-2;
         while (L > 0) {
            int q = get8(z->s);
            int p = q >> 4;
            int t = q & 15,i;
            if (p != 0) return e("bad DQT type","Corrupt JPEG");
            if (t > 3) return e("bad DQT table","Corrupt JPEG");
            for (i=0; i < 64; ++i)
               z->dequant[t][dezigzag[i]] = get8u(z->s);
            #ifdef STBI_SIMD
            for (i=0; i < 64; ++i)
               z->dequant2[t][i] = z->dequant[t][i];
            #endif
            L -= 65;
         }
         return L==0;

      case 0xC4: // DHT - define huffman table
         L = get16(z->s)-2;
         while (L > 0) {
            uint8 *v;
            int sizes[16],i,nm=0;
            int q = get8(z->s);
            int tc = q >> 4;
            int th = q & 15;
            if (tc > 1 || th > 3) return e("bad DHT header","Corrupt JPEG");
            for (i=0; i < 16; ++i) {
               sizes[i] = get8(z->s);
               nm += sizes[i];
            }
            L -= 17;
            if (tc == 0) {
               if (!build_huffman(z->huff_dc+th, sizes)) return 0;
               v = z->huff_dc[th].values;
            } else {
               if (!build_huffman(z->huff_ac+th, sizes)) return 0;
               v = z->huff_ac[th].values;
            }
            for (i=0; i < nm; ++i)
               v[i] = get8u(z->s);
            L -= nm;
         }
         return L==0;
   }
   // check for comment block or APP blocks
   if ((m >= 0xE0 && m <= 0xEF) || m == 0xFE) {
      skip(z->s, get16(z->s)-2);
      return 1;
   }
   return 0;
}

// after we see SOS
static int process_scan_header(jpeg *z)
{
   int i;
   int Ls = get16(z->s);
   z->scan_n = get8(z->s);
   if (z->scan_n < 1 || z->scan_n > 4 || z->scan_n > (int) z->s->img_n) return e("bad SOS component count","Corrupt JPEG");
   if (Ls != 6+2*z->scan_n) return e("bad SOS len","Corrupt JPEG");
   for (i=0; i < z->scan_n; ++i) {
      int id = get8(z->s), which;
      int q = get8(z->s);
      for (which = 0; which < z->s->img_n; ++which)
         if (z->img_comp[which].id == id)
            break;
      if (which == z->s->img_n) return 0;
      z->img_comp[which].hd = q >> 4;   if (z->img_comp[which].hd > 3) return e("bad DC huff","Corrupt JPEG");
      z->img_comp[which].ha = q & 15;   if (z->img_comp[which].ha > 3) return e("bad AC huff","Corrupt JPEG");
      z->order[i] = which;
   }
   if (get8(z->s) != 0) return e("bad SOS","Corrupt JPEG");
   get8(z->s); // should be 63, but might be 0
   if (get8(z->s) != 0) return e("bad SOS","Corrupt JPEG");

   return 1;
}

static int process_frame_header(jpeg *z, int scan)
{
   stbi *s = z->s;
   int Lf,p,i,q, h_max=1,v_max=1,c;
   Lf = get16(s);         if (Lf < 11) return e("bad SOF len","Corrupt JPEG"); // JPEG
   p  = get8(s);          if (p != 8) return e("only 8-bit","JPEG format not supported: 8-bit only"); // JPEG baseline
   s->img_y = get16(s);   if (s->img_y == 0) return e("no header height", "JPEG format not supported: delayed height"); // Legal, but we don't handle it--but neither does IJG
   s->img_x = get16(s);   if (s->img_x == 0) return e("0 width","Corrupt JPEG"); // JPEG requires
   c = get8(s);
   if (c != 3 && c != 1) return e("bad component count","Corrupt JPEG");    // JFIF requires
   s->img_n = c;
   for (i=0; i < c; ++i) {
      z->img_comp[i].data = NULL;
      z->img_comp[i].linebuf = NULL;
   }

   if (Lf != 8+3*s->img_n) return e("bad SOF len","Corrupt JPEG");

   for (i=0; i < s->img_n; ++i) {
      z->img_comp[i].id = get8(s);
      if (z->img_comp[i].id != i+1)   // JFIF requires
         if (z->img_comp[i].id != i)  // some version of jpegtran outputs non-JFIF-compliant files!
            return e("bad component ID","Corrupt JPEG");
      q = get8(s);
      z->img_comp[i].h = (q >> 4);  if (!z->img_comp[i].h || z->img_comp[i].h > 4) return e("bad H","Corrupt JPEG");
      z->img_comp[i].v = q & 15;    if (!z->img_comp[i].v || z->img_comp[i].v > 4) return e("bad V","Corrupt JPEG");
      z->img_comp[i].tq = get8(s);  if (z->img_comp[i].tq > 3) return e("bad TQ","Corrupt JPEG");
   }

   if (scan != SCAN_load) return 1;

   if ((1 << 30) / s->img_x / s->img_n < s->img_y) return e("too large", "Image too large to decode");

   for (i=0; i < s->img_n; ++i) {
      if (z->img_comp[i].h > h_max) h_max = z->img_comp[i].h;
      if (z->img_comp[i].v > v_max) v_max = z->img_comp[i].v;
   }

   // compute interleaved mcu info
   z->img_h_max = h_max;
   z->img_v_max = v_max;
   z->img_mcu_w = h_max * 8;
   z->img_mcu_h = v_max * 8;
   z->img_mcu_x = (s->img_x + z->img_mcu_w-1) / z->img_mcu_w;
   z->img_mcu_y = (s->img_y + z->img_mcu_h-1) / z->img_mcu_h;

   for (i=0; i < s->img_n; ++i) {
      // number of effective pixels (e.g. for non-interleaved MCU)
      z->img_comp[i].x = (s->img_x * z->img_comp[i].h + h_max-1) / h_max;
      z->img_comp[i].y = (s->img_y * z->img_comp[i].v + v_max-1) / v_max;
      // to simplify generation, we'll allocate enough memory to decode
      // the bogus oversized data from using interleaved MCUs and their
      // big blocks (e.g. a 16x16 iMCU on an image of width 33); we won't
      // discard the extra data until colorspace conversion
      z->img_comp[i].w2 = z->img_mcu_x * z->img_comp[i].h * 8;
      z->img_comp[i].h2 = z->img_mcu_y * z->img_comp[i].v * 8;
      z->img_comp[i].raw_data = malloc(z->img_comp[i].w2 * z->img_comp[i].h2+15);
      if (z->img_comp[i].raw_data == NULL) {
         for(--i; i >= 0; --i) {
            free(z->img_comp[i].raw_data);
            z->img_comp[i].data = NULL;
         }
         return e("outofmem", "Out of memory");
      }
      // align blocks for installable-idct using mmx/sse
      z->img_comp[i].data = (uint8*) (((size_t) z->img_comp[i].raw_data + 15) & ~15);
      z->img_comp[i].linebuf = NULL;
   }

   return 1;
}

// use comparisons since in some cases we handle more than one case (e.g. SOF)
#define DNL(x)         ((x) == 0xdc)
#define SOI(x)         ((x) == 0xd8)
#define EOI(x)         ((x) == 0xd9)
#define SOF(x)         ((x) == 0xc0 || (x) == 0xc1)
#define SOS(x)         ((x) == 0xda)

static int decode_jpeg_header(jpeg *z, int scan)
{
   int m;
   z->marker = MARKER_none; // initialize cached marker to empty
   m = get_marker(z);
   if (!SOI(m)) return e("no SOI","Corrupt JPEG");
   if (scan == SCAN_type) return 1;
   m = get_marker(z);
   while (!SOF(m)) {
      if (!process_marker(z,m)) return 0;
      m = get_marker(z);
      while (m == MARKER_none) {
         // some files have extra padding after their blocks, so ok, we'll scan
         if (at_eof(z->s)) return e("no SOF", "Corrupt JPEG");
         m = get_marker(z);
      }
   }
   if (!process_frame_header(z, scan)) return 0;
   return 1;
}

static int decode_jpeg_image(jpeg *j)
{
   int m;
   j->restart_interval = 0;
   if (!decode_jpeg_header(j, SCAN_load)) return 0;
   m = get_marker(j);
   while (!EOI(m)) {
      if (SOS(m)) {
         if (!process_scan_header(j)) return 0;
         if (!parse_entropy_coded_data(j)) return 0;
         if (j->marker == MARKER_none ) {
            // handle 0s at the end of image data from IP Kamera 9060
            while (!at_eof(j->s)) {
               int x = get8(j->s);
               if (x == 255) {
                  j->marker = get8u(j->s);
                  break;
               } else if (x != 0) {
                  return 0;
               }
            }
            // if we reach eof without hitting a marker, get_marker() below will fail and we'll eventually return 0
         }
      } else {
         if (!process_marker(j, m)) return 0;
      }
      m = get_marker(j);
   }
   return 1;
}

// static jfif-centered resampling (across block boundaries)

typedef uint8 *(*resample_row_func)(uint8 *out, uint8 *in0, uint8 *in1,
                                    int w, int hs);

#define div4(x) ((uint8) ((x) >> 2))

static uint8 *resample_row_1(uint8 *out, uint8 *in_near, uint8 *in_far, int w, int hs)
{
   STBI_NOTUSED(out);
   STBI_NOTUSED(in_far);
   STBI_NOTUSED(w);
   STBI_NOTUSED(hs);
   return in_near;
}

static uint8* resample_row_v_2(uint8 *out, uint8 *in_near, uint8 *in_far, int w, int hs)
{
   // need to generate two samples vertically for every one in input
   int i;
   STBI_NOTUSED(hs);
   for (i=0; i < w; ++i)
      out[i] = div4(3*in_near[i] + in_far[i] + 2);
   return out;
}

static uint8*  resample_row_h_2(uint8 *out, uint8 *in_near, uint8 *in_far, int w, int hs)
{
   // need to generate two samples horizontally for every one in input
   int i;
   uint8 *input = in_near;

   if (w == 1) {
      // if only one sample, can't do any interpolation
      out[0] = out[1] = input[0];
      return out;
   }

   out[0] = input[0];
   out[1] = div4(input[0]*3 + input[1] + 2);
   for (i=1; i < w-1; ++i) {
      int n = 3*input[i]+2;
      out[i*2+0] = div4(n+input[i-1]);
      out[i*2+1] = div4(n+input[i+1]);
   }
   out[i*2+0] = div4(input[w-2]*3 + input[w-1] + 2);
   out[i*2+1] = input[w-1];

   STBI_NOTUSED(in_far);
   STBI_NOTUSED(hs);

   return out;
}

#define div16(x) ((uint8) ((x) >> 4))

static uint8 *resample_row_hv_2(uint8 *out, uint8 *in_near, uint8 *in_far, int w, int hs)
{
   // need to generate 2x2 samples for every one in input
   int i,t0,t1;
   if (w == 1) {
      out[0] = out[1] = div4(3*in_near[0] + in_far[0] + 2);
      return out;
   }

   t1 = 3*in_near[0] + in_far[0];
   out[0] = div4(t1+2);
   for (i=1; i < w; ++i) {
      t0 = t1;
      t1 = 3*in_near[i]+in_far[i];
      out[i*2-1] = div16(3*t0 + t1 + 8);
      out[i*2  ] = div16(3*t1 + t0 + 8);
   }
   out[w*2-1] = div4(t1+2);

   STBI_NOTUSED(hs);

   return out;
}

static uint8 *resample_row_generic(uint8 *out, uint8 *in_near, uint8 *in_far, int w, int hs)
{
   // resample with nearest-neighbor
   int i,j;
   in_far = in_far;
   for (i=0; i < w; ++i)
      for (j=0; j < hs; ++j)
         out[i*hs+j] = in_near[i];
   return out;
}

#define float2fixed(x)  ((int) ((x) * 65536 + 0.5))

// 0.38 seconds on 3*anemones.jpg   (0.25 with processor = Pro)
// VC6 without processor=Pro is generating multiple LEAs per multiply!
static void YCbCr_to_RGB_row(uint8 *out, const uint8 *y, const uint8 *pcb, const uint8 *pcr, int count, int step)
{
   int i;
   for (i=0; i < count; ++i) {
      int y_fixed = (y[i] << 16) + 32768; // rounding
      int r,g,b;
      int cr = pcr[i] - 128;
      int cb = pcb[i] - 128;
      r = y_fixed + cr*float2fixed(1.40200f);
      g = y_fixed - cr*float2fixed(0.71414f) - cb*float2fixed(0.34414f);
      b = y_fixed                            + cb*float2fixed(1.77200f);
      r >>= 16;
      g >>= 16;
      b >>= 16;
      if ((unsigned) r > 255) { if (r < 0) r = 0; else r = 255; }
      if ((unsigned) g > 255) { if (g < 0) g = 0; else g = 255; }
      if ((unsigned) b > 255) { if (b < 0) b = 0; else b = 255; }
      out[0] = (uint8)r;
      out[1] = (uint8)g;
      out[2] = (uint8)b;
      out[3] = 255;
      out += step;
   }
}

#ifdef STBI_SIMD
static stbi_YCbCr_to_RGB_run stbi_YCbCr_installed = YCbCr_to_RGB_row;

void stbi_install_YCbCr_to_RGB(stbi_YCbCr_to_RGB_run func)
{
   stbi_YCbCr_installed = func;
}
#endif


// clean up the temporary component buffers
static void cleanup_jpeg(jpeg *j)
{
   int i;
   for (i=0; i < j->s->img_n; ++i) {
      if (j->img_comp[i].data) {
         free(j->img_comp[i].raw_data);
         j->img_comp[i].data = NULL;
      }
      if (j->img_comp[i].linebuf) {
         free(j->img_comp[i].linebuf);
         j->img_comp[i].linebuf = NULL;
      }
   }
}

typedef struct
{
   resample_row_func resample;
   uint8 *line0,*line1;
   int hs,vs;   // expansion factor in each axis
   int w_lores; // horizontal pixels pre-expansion 
   int ystep;   // how far through vertical expansion we are
   int ypos;    // which pre-expansion row we're on
} stbi_resample;

static uint8 *load_jpeg_image(jpeg *z, int *out_x, int *out_y, int *comp, int req_comp)
{
   int n, decode_n;
   // validate req_comp
   if (req_comp < 0 || req_comp > 4) return epuc("bad req_comp", "Internal error");
   z->s->img_n = 0;

   // load a jpeg image from whichever source
   if (!decode_jpeg_image(z)) { cleanup_jpeg(z); return NULL; }

   // determine actual number of components to generate
   n = req_comp ? req_comp : z->s->img_n;

   if (z->s->img_n == 3 && n < 3)
      decode_n = 1;
   else
      decode_n = z->s->img_n;

   // resample and color-convert
   {
      int k;
      uint i,j;
      uint8 *output;
      uint8 *coutput[4];

      stbi_resample res_comp[4];

      for (k=0; k < decode_n; ++k) {
         stbi_resample *r = &res_comp[k];

         // allocate line buffer big enough for upsampling off the edges
         // with upsample factor of 4
         z->img_comp[k].linebuf = (uint8 *) malloc(z->s->img_x + 3);
         if (!z->img_comp[k].linebuf) { cleanup_jpeg(z); return epuc("outofmem", "Out of memory"); }

         r->hs      = z->img_h_max / z->img_comp[k].h;
         r->vs      = z->img_v_max / z->img_comp[k].v;
         r->ystep   = r->vs >> 1;
         r->w_lores = (z->s->img_x + r->hs-1) / r->hs;
         r->ypos    = 0;
         r->line0   = r->line1 = z->img_comp[k].data;

         if      (r->hs == 1 && r->vs == 1) r->resample = resample_row_1;
         else if (r->hs == 1 && r->vs == 2) r->resample = resample_row_v_2;
         else if (r->hs == 2 && r->vs == 1) r->resample = resample_row_h_2;
         else if (r->hs == 2 && r->vs == 2) r->resample = resample_row_hv_2;
         else                               r->resample = resample_row_generic;
      }

      // can't error after this so, this is safe
      output = (uint8 *) malloc(n * z->s->img_x * z->s->img_y + 1);
      if (!output) { cleanup_jpeg(z); return epuc("outofmem", "Out of memory"); }

      // now go ahead and resample
      for (j=0; j < z->s->img_y; ++j) {
         uint8 *out = output + n * z->s->img_x * j;
         for (k=0; k < decode_n; ++k) {
            stbi_resample *r = &res_comp[k];
            int y_bot = r->ystep >= (r->vs >> 1);
            coutput[k] = r->resample(z->img_comp[k].linebuf,
                                     y_bot ? r->line1 : r->line0,
                                     y_bot ? r->line0 : r->line1,
                                     r->w_lores, r->hs);
            if (++r->ystep >= r->vs) {
               r->ystep = 0;
               r->line0 = r->line1;
               if (++r->ypos < z->img_comp[k].y)
                  r->line1 += z->img_comp[k].w2;
            }
         }
         if (n >= 3) {
            uint8 *y = coutput[0];
            if (z->s->img_n == 3) {
               #ifdef STBI_SIMD
               stbi_YCbCr_installed(out, y, coutput[1], coutput[2], z->s.img_x, n);
               #else
               YCbCr_to_RGB_row(out, y, coutput[1], coutput[2], z->s->img_x, n);
               #endif
            } else
               for (i=0; i < z->s->img_x; ++i) {
                  out[0] = out[1] = out[2] = y[i];
                  out[3] = 255; // not used if n==3
                  out += n;
               }
         } else {
            uint8 *y = coutput[0];
            if (n == 1)
               for (i=0; i < z->s->img_x; ++i) out[i] = y[i];
            else
               for (i=0; i < z->s->img_x; ++i) *out++ = y[i], *out++ = 255;
         }
      }
      cleanup_jpeg(z);
      *out_x = z->s->img_x;
      *out_y = z->s->img_y;
      if (comp) *comp  = z->s->img_n; // report original components, not output
      return output;
   }
}

static unsigned char *stbi_jpeg_load(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   jpeg j;
   j.s = s;
   return load_jpeg_image(&j, x,y,comp,req_comp);
}

static int stbi_jpeg_test(stbi *s)
{
   int r;
   jpeg j;
   j.s = s;
   r = decode_jpeg_header(&j, SCAN_type);
   stbi_rewind(s);
   return r;
}

static int stbi_jpeg_info_raw(jpeg *j, int *x, int *y, int *comp)
{
   if (!decode_jpeg_header(j, SCAN_header)) {
      stbi_rewind( j->s );
      return 0;
   }
   if (x) *x = j->s->img_x;
   if (y) *y = j->s->img_y;
   if (comp) *comp = j->s->img_n;
   return 1;
}

static int stbi_jpeg_info(stbi *s, int *x, int *y, int *comp)
{
   jpeg j;
   j.s = s;
   return stbi_jpeg_info_raw(&j, x, y, comp);
}

// public domain zlib decode    v0.2  Sean Barrett 2006-11-18
//    simple implementation
//      - all input must be provided in an upfront buffer
//      - all output is written to a single output buffer (can malloc/realloc)
//    performance
//      - fast huffman

// fast-way is faster to check than jpeg huffman, but slow way is slower
#define ZFAST_BITS  9 // accelerate all cases in default tables
#define ZFAST_MASK  ((1 << ZFAST_BITS) - 1)

// zlib-style huffman encoding
// (jpegs packs from left, zlib from right, so can't share code)
typedef struct
{
   uint16 fast[1 << ZFAST_BITS];
   uint16 firstcode[16];
   int maxcode[17];
   uint16 firstsymbol[16];
   uint8  size[288];
   uint16 value[288]; 
} zhuffman;

stbi_inline static int bitreverse16(int n)
{
  n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
  return n;
}

stbi_inline static int bit_reverse(int v, int bits)
{
   assert(bits <= 16);
   // to bit reverse n bits, reverse 16 and shift
   // e.g. 11 bits, bit reverse and shift away 5
   return bitreverse16(v) >> (16-bits);
}

static int zbuild_huffman(zhuffman *z, uint8 *sizelist, int num)
{
   int i,k=0;
   int code, next_code[16], sizes[17];

   // DEFLATE spec for generating codes
   memset(sizes, 0, sizeof(sizes));
   memset(z->fast, 255, sizeof(z->fast));
   for (i=0; i < num; ++i) 
      ++sizes[sizelist[i]];
   sizes[0] = 0;
   for (i=1; i < 16; ++i)
      assert(sizes[i] <= (1 << i));
   code = 0;
   for (i=1; i < 16; ++i) {
      next_code[i] = code;
      z->firstcode[i] = (uint16) code;
      z->firstsymbol[i] = (uint16) k;
      code = (code + sizes[i]);
      if (sizes[i])
         if (code-1 >= (1 << i)) return e("bad codelengths","Corrupt JPEG");
      z->maxcode[i] = code << (16-i); // preshift for inner loop
      code <<= 1;
      k += sizes[i];
   }
   z->maxcode[16] = 0x10000; // sentinel
   for (i=0; i < num; ++i) {
      int s = sizelist[i];
      if (s) {
         int c = next_code[s] - z->firstcode[s] + z->firstsymbol[s];
         z->size[c] = (uint8)s;
         z->value[c] = (uint16)i;
         if (s <= ZFAST_BITS) {
            int nk = bit_reverse(next_code[s],s);
            while (nk < (1 << ZFAST_BITS)) {
               z->fast[nk] = (uint16) c;
               nk += (1 << s);
            }
         }
         ++next_code[s];
      }
   }
   return 1;
}

// zlib-from-memory implementation for PNG reading
//    because PNG allows splitting the zlib stream arbitrarily,
//    and it's annoying structurally to have PNG call ZLIB call PNG,
//    we require PNG read all the IDATs and combine them into a single
//    memory buffer

typedef struct
{
   uint8 *zbuffer, *zbuffer_end;
   int num_bits;
   uint32 code_buffer;

   char *zout;
   char *zout_start;
   char *zout_end;
   int   z_expandable;

   zhuffman z_length, z_distance;
} zbuf;

stbi_inline static int zget8(zbuf *z)
{
   if (z->zbuffer >= z->zbuffer_end) return 0;
   return *z->zbuffer++;
}

static void fill_bits(zbuf *z)
{
   do {
      assert(z->code_buffer < (1U << z->num_bits));
      z->code_buffer |= zget8(z) << z->num_bits;
      z->num_bits += 8;
   } while (z->num_bits <= 24);
}

stbi_inline static unsigned int zreceive(zbuf *z, int n)
{
   unsigned int k;
   if (z->num_bits < n) fill_bits(z);
   k = z->code_buffer & ((1 << n) - 1);
   z->code_buffer >>= n;
   z->num_bits -= n;
   return k;   
}

stbi_inline static int zhuffman_decode(zbuf *a, zhuffman *z)
{
   int b,s,k;
   if (a->num_bits < 16) fill_bits(a);
   b = z->fast[a->code_buffer & ZFAST_MASK];
   if (b < 0xffff) {
      s = z->size[b];
      a->code_buffer >>= s;
      a->num_bits -= s;
      return z->value[b];
   }

   // not resolved by fast table, so compute it the slow way
   // use jpeg approach, which requires MSbits at top
   k = bit_reverse(a->code_buffer, 16);
   for (s=ZFAST_BITS+1; ; ++s)
      if (k < z->maxcode[s])
         break;
   if (s == 16) return -1; // invalid code!
   // code size is s, so:
   b = (k >> (16-s)) - z->firstcode[s] + z->firstsymbol[s];
   assert(z->size[b] == s);
   a->code_buffer >>= s;
   a->num_bits -= s;
   return z->value[b];
}

static int expand(zbuf *z, int n)  // need to make room for n bytes
{
   char *q;
   int cur, limit;
   if (!z->z_expandable) return e("output buffer limit","Corrupt PNG");
   cur   = (int) (z->zout     - z->zout_start);
   limit = (int) (z->zout_end - z->zout_start);
   while (cur + n > limit)
      limit *= 2;
   q = (char *) realloc(z->zout_start, limit);
   if (q == NULL) return e("outofmem", "Out of memory");
   z->zout_start = q;
   z->zout       = q + cur;
   z->zout_end   = q + limit;
   return 1;
}

static int length_base[31] = {
   3,4,5,6,7,8,9,10,11,13,
   15,17,19,23,27,31,35,43,51,59,
   67,83,99,115,131,163,195,227,258,0,0 };

static int length_extra[31]= 
{ 0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,0,0,0 };

static int dist_base[32] = { 1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577,0,0};

static int dist_extra[32] =
{ 0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13};

static int parse_huffman_block(zbuf *a)
{
   for(;;) {
      int z = zhuffman_decode(a, &a->z_length);
      if (z < 256) {
         if (z < 0) return e("bad huffman code","Corrupt PNG"); // error in huffman codes
         if (a->zout >= a->zout_end) if (!expand(a, 1)) return 0;
         *a->zout++ = (char) z;
      } else {
         uint8 *p;
         int len,dist;
         if (z == 256) return 1;
         z -= 257;
         len = length_base[z];
         if (length_extra[z]) len += zreceive(a, length_extra[z]);
         z = zhuffman_decode(a, &a->z_distance);
         if (z < 0) return e("bad huffman code","Corrupt PNG");
         dist = dist_base[z];
         if (dist_extra[z]) dist += zreceive(a, dist_extra[z]);
         if (a->zout - a->zout_start < dist) return e("bad dist","Corrupt PNG");
         if (a->zout + len > a->zout_end) if (!expand(a, len)) return 0;
         p = (uint8 *) (a->zout - dist);
         while (len--)
            *a->zout++ = *p++;
      }
   }
}

static int compute_huffman_codes(zbuf *a)
{
   static uint8 length_dezigzag[19] = { 16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15 };
   zhuffman z_codelength;
   uint8 lencodes[286+32+137];//padding for maximum single op
   uint8 codelength_sizes[19];
   int i,n;

   int hlit  = zreceive(a,5) + 257;
   int hdist = zreceive(a,5) + 1;
   int hclen = zreceive(a,4) + 4;

   memset(codelength_sizes, 0, sizeof(codelength_sizes));
   for (i=0; i < hclen; ++i) {
      int s = zreceive(a,3);
      codelength_sizes[length_dezigzag[i]] = (uint8) s;
   }
   if (!zbuild_huffman(&z_codelength, codelength_sizes, 19)) return 0;

   n = 0;
   while (n < hlit + hdist) {
      int c = zhuffman_decode(a, &z_codelength);
      assert(c >= 0 && c < 19);
      if (c < 16)
         lencodes[n++] = (uint8) c;
      else if (c == 16) {
         c = zreceive(a,2)+3;
         memset(lencodes+n, lencodes[n-1], c);
         n += c;
      } else if (c == 17) {
         c = zreceive(a,3)+3;
         memset(lencodes+n, 0, c);
         n += c;
      } else {
         assert(c == 18);
         c = zreceive(a,7)+11;
         memset(lencodes+n, 0, c);
         n += c;
      }
   }
   if (n != hlit+hdist) return e("bad codelengths","Corrupt PNG");
   if (!zbuild_huffman(&a->z_length, lencodes, hlit)) return 0;
   if (!zbuild_huffman(&a->z_distance, lencodes+hlit, hdist)) return 0;
   return 1;
}

static int parse_uncompressed_block(zbuf *a)
{
   uint8 header[4];
   int len,nlen,k;
   if (a->num_bits & 7)
      zreceive(a, a->num_bits & 7); // discard
   // drain the bit-packed data into header
   k = 0;
   while (a->num_bits > 0) {
      header[k++] = (uint8) (a->code_buffer & 255); // wtf this warns?
      a->code_buffer >>= 8;
      a->num_bits -= 8;
   }
   assert(a->num_bits == 0);
   // now fill header the normal way
   while (k < 4)
      header[k++] = (uint8) zget8(a);
   len  = header[1] * 256 + header[0];
   nlen = header[3] * 256 + header[2];
   if (nlen != (len ^ 0xffff)) return e("zlib corrupt","Corrupt PNG");
   if (a->zbuffer + len > a->zbuffer_end) return e("read past buffer","Corrupt PNG");
   if (a->zout + len > a->zout_end)
      if (!expand(a, len)) return 0;
   memcpy(a->zout, a->zbuffer, len);
   a->zbuffer += len;
   a->zout += len;
   return 1;
}

static int parse_zlib_header(zbuf *a)
{
   int cmf   = zget8(a);
   int cm    = cmf & 15;
   /* int cinfo = cmf >> 4; */
   int flg   = zget8(a);
   if ((cmf*256+flg) % 31 != 0) return e("bad zlib header","Corrupt PNG"); // zlib spec
   if (flg & 32) return e("no preset dict","Corrupt PNG"); // preset dictionary not allowed in png
   if (cm != 8) return e("bad compression","Corrupt PNG"); // DEFLATE required for png
   // window = 1 << (8 + cinfo)... but who cares, we fully buffer output
   return 1;
}

// @TODO: should statically initialize these for optimal thread safety
static uint8 default_length[288], default_distance[32];
static void init_defaults(void)
{
   int i;   // use <= to match clearly with spec
   for (i=0; i <= 143; ++i)     default_length[i]   = 8;
   for (   ; i <= 255; ++i)     default_length[i]   = 9;
   for (   ; i <= 279; ++i)     default_length[i]   = 7;
   for (   ; i <= 287; ++i)     default_length[i]   = 8;

   for (i=0; i <=  31; ++i)     default_distance[i] = 5;
}

int stbi_png_partial; // a quick hack to only allow decoding some of a PNG... I should implement real streaming support instead
static int parse_zlib(zbuf *a, int parse_header)
{
   int final, type;
   if (parse_header)
      if (!parse_zlib_header(a)) return 0;
   a->num_bits = 0;
   a->code_buffer = 0;
   do {
      final = zreceive(a,1);
      type = zreceive(a,2);
      if (type == 0) {
         if (!parse_uncompressed_block(a)) return 0;
      } else if (type == 3) {
         return 0;
      } else {
         if (type == 1) {
            // use fixed code lengths
            if (!default_distance[31]) init_defaults();
            if (!zbuild_huffman(&a->z_length  , default_length  , 288)) return 0;
            if (!zbuild_huffman(&a->z_distance, default_distance,  32)) return 0;
         } else {
            if (!compute_huffman_codes(a)) return 0;
         }
         if (!parse_huffman_block(a)) return 0;
      }
      if (stbi_png_partial && a->zout - a->zout_start > 65536)
         break;
   } while (!final);
   return 1;
}

static int do_zlib(zbuf *a, char *obuf, int olen, int exp, int parse_header)
{
   a->zout_start = obuf;
   a->zout       = obuf;
   a->zout_end   = obuf + olen;
   a->z_expandable = exp;

   return parse_zlib(a, parse_header);
}

char *stbi_zlib_decode_malloc_guesssize(const char *buffer, int len, int initial_size, int *outlen)
{
   zbuf a;
   char *p = (char *) malloc(initial_size);
   if (p == NULL) return NULL;
   a.zbuffer = (uint8 *) buffer;
   a.zbuffer_end = (uint8 *) buffer + len;
   if (do_zlib(&a, p, initial_size, 1, 1)) {
      if (outlen) *outlen = (int) (a.zout - a.zout_start);
      return a.zout_start;
   } else {
      free(a.zout_start);
      return NULL;
   }
}

char *stbi_zlib_decode_malloc(char const *buffer, int len, int *outlen)
{
   return stbi_zlib_decode_malloc_guesssize(buffer, len, 16384, outlen);
}

char *stbi_zlib_decode_malloc_guesssize_headerflag(const char *buffer, int len, int initial_size, int *outlen, int parse_header)
{
   zbuf a;
   char *p = (char *) malloc(initial_size);
   if (p == NULL) return NULL;
   a.zbuffer = (uint8 *) buffer;
   a.zbuffer_end = (uint8 *) buffer + len;
   if (do_zlib(&a, p, initial_size, 1, parse_header)) {
      if (outlen) *outlen = (int) (a.zout - a.zout_start);
      return a.zout_start;
   } else {
      free(a.zout_start);
      return NULL;
   }
}

int stbi_zlib_decode_buffer(char *obuffer, int olen, char const *ibuffer, int ilen)
{
   zbuf a;
   a.zbuffer = (uint8 *) ibuffer;
   a.zbuffer_end = (uint8 *) ibuffer + ilen;
   if (do_zlib(&a, obuffer, olen, 0, 1))
      return (int) (a.zout - a.zout_start);
   else
      return -1;
}

char *stbi_zlib_decode_noheader_malloc(char const *buffer, int len, int *outlen)
{
   zbuf a;
   char *p = (char *) malloc(16384);
   if (p == NULL) return NULL;
   a.zbuffer = (uint8 *) buffer;
   a.zbuffer_end = (uint8 *) buffer+len;
   if (do_zlib(&a, p, 16384, 1, 0)) {
      if (outlen) *outlen = (int) (a.zout - a.zout_start);
      return a.zout_start;
   } else {
      free(a.zout_start);
      return NULL;
   }
}

int stbi_zlib_decode_noheader_buffer(char *obuffer, int olen, const char *ibuffer, int ilen)
{
   zbuf a;
   a.zbuffer = (uint8 *) ibuffer;
   a.zbuffer_end = (uint8 *) ibuffer + ilen;
   if (do_zlib(&a, obuffer, olen, 0, 0))
      return (int) (a.zout - a.zout_start);
   else
      return -1;
}

// public domain "baseline" PNG decoder   v0.10  Sean Barrett 2006-11-18
//    simple implementation
//      - only 8-bit samples
//      - no CRC checking
//      - allocates lots of intermediate memory
//        - avoids problem of streaming data between subsystems
//        - avoids explicit window management
//    performance
//      - uses stb_zlib, a PD zlib implementation with fast huffman decoding


typedef struct
{
   uint32 length;
   uint32 type;
} chunk;

#define PNG_TYPE(a,b,c,d)  (((a) << 24) + ((b) << 16) + ((c) << 8) + (d))

static chunk get_chunk_header(stbi *s)
{
   chunk c;
   c.length = get32(s);
   c.type   = get32(s);
   return c;
}

static int check_png_header(stbi *s)
{
   static uint8 png_sig[8] = { 137,80,78,71,13,10,26,10 };
   int i;
   for (i=0; i < 8; ++i)
      if (get8u(s) != png_sig[i]) return e("bad png sig","Not a PNG");
   return 1;
}

typedef struct
{
   stbi *s;
   uint8 *idata, *expanded, *out;
} png;


enum {
   F_none=0, F_sub=1, F_up=2, F_avg=3, F_paeth=4,
   F_avg_first, F_paeth_first
};

static uint8 first_row_filter[5] =
{
   F_none, F_sub, F_none, F_avg_first, F_paeth_first
};

static int paeth(int a, int b, int c)
{
   int p = a + b - c;
   int pa = abs(p-a);
   int pb = abs(p-b);
   int pc = abs(p-c);
   if (pa <= pb && pa <= pc) return a;
   if (pb <= pc) return b;
   return c;
}

// create the png data from post-deflated data
static int create_png_image_raw(png *a, uint8 *raw, uint32 raw_len, int out_n, uint32 x, uint32 y)
{
   stbi *s = a->s;
   uint32 i,j,stride = x*out_n;
   int k;
   int img_n = s->img_n; // copy it into a local for later
   assert(out_n == s->img_n || out_n == s->img_n+1);
   if (stbi_png_partial) y = 1;
   a->out = (uint8 *) malloc(x * y * out_n);
   if (!a->out) return e("outofmem", "Out of memory");
   if (!stbi_png_partial) {
      if (s->img_x == x && s->img_y == y) {
         if (raw_len != (img_n * x + 1) * y) return e("not enough pixels","Corrupt PNG");
      } else { // interlaced:
         if (raw_len < (img_n * x + 1) * y) return e("not enough pixels","Corrupt PNG");
      }
   }
   for (j=0; j < y; ++j) {
      uint8 *cur = a->out + stride*j;
      uint8 *prior = cur - stride;
      int filter = *raw++;
      if (filter > 4) return e("invalid filter","Corrupt PNG");
      // if first row, use special filter that doesn't sample previous row
      if (j == 0) filter = first_row_filter[filter];
      // handle first pixel explicitly
      for (k=0; k < img_n; ++k) {
         switch (filter) {
            case F_none       : cur[k] = raw[k]; break;
            case F_sub        : cur[k] = raw[k]; break;
            case F_up         : cur[k] = raw[k] + prior[k]; break;
            case F_avg        : cur[k] = raw[k] + (prior[k]>>1); break;
            case F_paeth      : cur[k] = (uint8) (raw[k] + paeth(0,prior[k],0)); break;
            case F_avg_first  : cur[k] = raw[k]; break;
            case F_paeth_first: cur[k] = raw[k]; break;
         }
      }
      if (img_n != out_n) cur[img_n] = 255;
      raw += img_n;
      cur += out_n;
      prior += out_n;
      // this is a little gross, so that we don't switch per-pixel or per-component
      if (img_n == out_n) {
         #define CASE(f) \
             case f:     \
                for (i=x-1; i >= 1; --i, raw+=img_n,cur+=img_n,prior+=img_n) \
                   for (k=0; k < img_n; ++k)
         switch (filter) {
            CASE(F_none)  cur[k] = raw[k]; break;
            CASE(F_sub)   cur[k] = raw[k] + cur[k-img_n]; break;
            CASE(F_up)    cur[k] = raw[k] + prior[k]; break;
            CASE(F_avg)   cur[k] = raw[k] + ((prior[k] + cur[k-img_n])>>1); break;
            CASE(F_paeth)  cur[k] = (uint8) (raw[k] + paeth(cur[k-img_n],prior[k],prior[k-img_n])); break;
            CASE(F_avg_first)    cur[k] = raw[k] + (cur[k-img_n] >> 1); break;
            CASE(F_paeth_first)  cur[k] = (uint8) (raw[k] + paeth(cur[k-img_n],0,0)); break;
         }
         #undef CASE
      } else {
         assert(img_n+1 == out_n);
         #define CASE(f) \
             case f:     \
                for (i=x-1; i >= 1; --i, cur[img_n]=255,raw+=img_n,cur+=out_n,prior+=out_n) \
                   for (k=0; k < img_n; ++k)
         switch (filter) {
            CASE(F_none)  cur[k] = raw[k]; break;
            CASE(F_sub)   cur[k] = raw[k] + cur[k-out_n]; break;
            CASE(F_up)    cur[k] = raw[k] + prior[k]; break;
            CASE(F_avg)   cur[k] = raw[k] + ((prior[k] + cur[k-out_n])>>1); break;
            CASE(F_paeth)  cur[k] = (uint8) (raw[k] + paeth(cur[k-out_n],prior[k],prior[k-out_n])); break;
            CASE(F_avg_first)    cur[k] = raw[k] + (cur[k-out_n] >> 1); break;
            CASE(F_paeth_first)  cur[k] = (uint8) (raw[k] + paeth(cur[k-out_n],0,0)); break;
         }
         #undef CASE
      }
   }
   return 1;
}

static int create_png_image(png *a, uint8 *raw, uint32 raw_len, int out_n, int interlaced)
{
   uint8 *final;
   int p;
   int save;
   if (!interlaced)
      return create_png_image_raw(a, raw, raw_len, out_n, a->s->img_x, a->s->img_y);
   save = stbi_png_partial;
   stbi_png_partial = 0;

   // de-interlacing
   final = (uint8 *) malloc(a->s->img_x * a->s->img_y * out_n);
   for (p=0; p < 7; ++p) {
      int xorig[] = { 0,4,0,2,0,1,0 };
      int yorig[] = { 0,0,4,0,2,0,1 };
      int xspc[]  = { 8,8,4,4,2,2,1 };
      int yspc[]  = { 8,8,8,4,4,2,2 };
      int i,j,x,y;
      // pass1_x[4] = 0, pass1_x[5] = 1, pass1_x[12] = 1
      x = (a->s->img_x - xorig[p] + xspc[p]-1) / xspc[p];
      y = (a->s->img_y - yorig[p] + yspc[p]-1) / yspc[p];
      if (x && y) {
         if (!create_png_image_raw(a, raw, raw_len, out_n, x, y)) {
            free(final);
            return 0;
         }
         for (j=0; j < y; ++j)
            for (i=0; i < x; ++i)
               memcpy(final + (j*yspc[p]+yorig[p])*a->s->img_x*out_n + (i*xspc[p]+xorig[p])*out_n,
                      a->out + (j*x+i)*out_n, out_n);
         free(a->out);
         raw += (x*out_n+1)*y;
         raw_len -= (x*out_n+1)*y;
      }
   }
   a->out = final;

   stbi_png_partial = save;
   return 1;
}

static int compute_transparency(png *z, uint8 tc[3], int out_n)
{
   stbi *s = z->s;
   uint32 i, pixel_count = s->img_x * s->img_y;
   uint8 *p = z->out;

   // compute color-based transparency, assuming we've
   // already got 255 as the alpha value in the output
   assert(out_n == 2 || out_n == 4);

   if (out_n == 2) {
      for (i=0; i < pixel_count; ++i) {
         p[1] = (p[0] == tc[0] ? 0 : 255);
         p += 2;
      }
   } else {
      for (i=0; i < pixel_count; ++i) {
         if (p[0] == tc[0] && p[1] == tc[1] && p[2] == tc[2])
            p[3] = 0;
         p += 4;
      }
   }
   return 1;
}

static int expand_palette(png *a, uint8 *palette, int len, int pal_img_n)
{
   uint32 i, pixel_count = a->s->img_x * a->s->img_y;
   uint8 *p, *temp_out, *orig = a->out;

   p = (uint8 *) malloc(pixel_count * pal_img_n);
   if (p == NULL) return e("outofmem", "Out of memory");

   // between here and free(out) below, exitting would leak
   temp_out = p;

   if (pal_img_n == 3) {
      for (i=0; i < pixel_count; ++i) {
         int n = orig[i]*4;
         p[0] = palette[n  ];
         p[1] = palette[n+1];
         p[2] = palette[n+2];
         p += 3;
      }
   } else {
      for (i=0; i < pixel_count; ++i) {
         int n = orig[i]*4;
         p[0] = palette[n  ];
         p[1] = palette[n+1];
         p[2] = palette[n+2];
         p[3] = palette[n+3];
         p += 4;
      }
   }
   free(a->out);
   a->out = temp_out;

   STBI_NOTUSED(len);

   return 1;
}

static int stbi_unpremultiply_on_load = 0;
static int stbi_de_iphone_flag = 0;

void stbi_set_unpremultiply_on_load(int flag_true_if_should_unpremultiply)
{
   stbi_unpremultiply_on_load = flag_true_if_should_unpremultiply;
}
void stbi_convert_iphone_png_to_rgb(int flag_true_if_should_convert)
{
   stbi_de_iphone_flag = flag_true_if_should_convert;
}

static void stbi_de_iphone(png *z)
{
   stbi *s = z->s;
   uint32 i, pixel_count = s->img_x * s->img_y;
   uint8 *p = z->out;

   if (s->img_out_n == 3) {  // convert bgr to rgb
      for (i=0; i < pixel_count; ++i) {
         uint8 t = p[0];
         p[0] = p[2];
         p[2] = t;
         p += 3;
      }
   } else {
      assert(s->img_out_n == 4);
      if (stbi_unpremultiply_on_load) {
         // convert bgr to rgb and unpremultiply
         for (i=0; i < pixel_count; ++i) {
            uint8 a = p[3];
            uint8 t = p[0];
            if (a) {
               p[0] = p[2] * 255 / a;
               p[1] = p[1] * 255 / a;
               p[2] =  t   * 255 / a;
            } else {
               p[0] = p[2];
               p[2] = t;
            } 
            p += 4;
         }
      } else {
         // convert bgr to rgb
         for (i=0; i < pixel_count; ++i) {
            uint8 t = p[0];
            p[0] = p[2];
            p[2] = t;
            p += 4;
         }
      }
   }
}

static int parse_png_file(png *z, int scan, int req_comp)
{
   uint8 palette[1024], pal_img_n=0;
   uint8 has_trans=0, tc[3];
   uint32 ioff=0, idata_limit=0, i, pal_len=0;
   int first=1,k,interlace=0, iphone=0;
   stbi *s = z->s;

   z->expanded = NULL;
   z->idata = NULL;
   z->out = NULL;

   if (!check_png_header(s)) return 0;

   if (scan == SCAN_type) return 1;

   for (;;) {
      chunk c = get_chunk_header(s);
      switch (c.type) {
         case PNG_TYPE('C','g','B','I'):
            iphone = stbi_de_iphone_flag;
            skip(s, c.length);
            break;
         case PNG_TYPE('I','H','D','R'): {
            int depth,color,comp,filter;
            if (!first) return e("multiple IHDR","Corrupt PNG");
            first = 0;
            if (c.length != 13) return e("bad IHDR len","Corrupt PNG");
            s->img_x = get32(s); if (s->img_x > (1 << 24)) return e("too large","Very large image (corrupt?)");
            s->img_y = get32(s); if (s->img_y > (1 << 24)) return e("too large","Very large image (corrupt?)");
            depth = get8(s);  if (depth != 8)        return e("8bit only","PNG not supported: 8-bit only");
            color = get8(s);  if (color > 6)         return e("bad ctype","Corrupt PNG");
            if (color == 3) pal_img_n = 3; else if (color & 1) return e("bad ctype","Corrupt PNG");
            comp  = get8(s);  if (comp) return e("bad comp method","Corrupt PNG");
            filter= get8(s);  if (filter) return e("bad filter method","Corrupt PNG");
            interlace = get8(s); if (interlace>1) return e("bad interlace method","Corrupt PNG");
            if (!s->img_x || !s->img_y) return e("0-pixel image","Corrupt PNG");
            if (!pal_img_n) {
               s->img_n = (color & 2 ? 3 : 1) + (color & 4 ? 1 : 0);
               if ((1 << 30) / s->img_x / s->img_n < s->img_y) return e("too large", "Image too large to decode");
               if (scan == SCAN_header) return 1;
            } else {
               // if paletted, then pal_n is our final components, and
               // img_n is # components to decompress/filter.
               s->img_n = 1;
               if ((1 << 30) / s->img_x / 4 < s->img_y) return e("too large","Corrupt PNG");
               // if SCAN_header, have to scan to see if we have a tRNS
            }
            break;
         }

         case PNG_TYPE('P','L','T','E'):  {
            if (first) return e("first not IHDR", "Corrupt PNG");
            if (c.length > 256*3) return e("invalid PLTE","Corrupt PNG");
            pal_len = c.length / 3;
            if (pal_len * 3 != c.length) return e("invalid PLTE","Corrupt PNG");
            for (i=0; i < pal_len; ++i) {
               palette[i*4+0] = get8u(s);
               palette[i*4+1] = get8u(s);
               palette[i*4+2] = get8u(s);
               palette[i*4+3] = 255;
            }
            break;
         }

         case PNG_TYPE('t','R','N','S'): {
            if (first) return e("first not IHDR", "Corrupt PNG");
            if (z->idata) return e("tRNS after IDAT","Corrupt PNG");
            if (pal_img_n) {
               if (scan == SCAN_header) { s->img_n = 4; return 1; }
               if (pal_len == 0) return e("tRNS before PLTE","Corrupt PNG");
               if (c.length > pal_len) return e("bad tRNS len","Corrupt PNG");
               pal_img_n = 4;
               for (i=0; i < c.length; ++i)
                  palette[i*4+3] = get8u(s);
            } else {
               if (!(s->img_n & 1)) return e("tRNS with alpha","Corrupt PNG");
               if (c.length != (uint32) s->img_n*2) return e("bad tRNS len","Corrupt PNG");
               has_trans = 1;
               for (k=0; k < s->img_n; ++k)
                  tc[k] = (uint8) get16(s); // non 8-bit images will be larger
            }
            break;
         }

         case PNG_TYPE('I','D','A','T'): {
            if (first) return e("first not IHDR", "Corrupt PNG");
            if (pal_img_n && !pal_len) return e("no PLTE","Corrupt PNG");
            if (scan == SCAN_header) { s->img_n = pal_img_n; return 1; }
            if (ioff + c.length > idata_limit) {
               uint8 *p;
               if (idata_limit == 0) idata_limit = c.length > 4096 ? c.length : 4096;
               while (ioff + c.length > idata_limit)
                  idata_limit *= 2;
               p = (uint8 *) realloc(z->idata, idata_limit); if (p == NULL) return e("outofmem", "Out of memory");
               z->idata = p;
            }
            if (!getn(s, z->idata+ioff,c.length)) return e("outofdata","Corrupt PNG");
            ioff += c.length;
            break;
         }

         case PNG_TYPE('I','E','N','D'): {
            uint32 raw_len;
            if (first) return e("first not IHDR", "Corrupt PNG");
            if (scan != SCAN_load) return 1;
            if (z->idata == NULL) return e("no IDAT","Corrupt PNG");
            z->expanded = (uint8 *) stbi_zlib_decode_malloc_guesssize_headerflag((char *) z->idata, ioff, 16384, (int *) &raw_len, !iphone);
            if (z->expanded == NULL) return 0; // zlib should set error
            free(z->idata); z->idata = NULL;
            if ((req_comp == s->img_n+1 && req_comp != 3 && !pal_img_n) || has_trans)
               s->img_out_n = s->img_n+1;
            else
               s->img_out_n = s->img_n;
            if (!create_png_image(z, z->expanded, raw_len, s->img_out_n, interlace)) return 0;
            if (has_trans)
               if (!compute_transparency(z, tc, s->img_out_n)) return 0;
            if (iphone && s->img_out_n > 2)
               stbi_de_iphone(z);
            if (pal_img_n) {
               // pal_img_n == 3 or 4
               s->img_n = pal_img_n; // record the actual colors we had
               s->img_out_n = pal_img_n;
               if (req_comp >= 3) s->img_out_n = req_comp;
               if (!expand_palette(z, palette, pal_len, s->img_out_n))
                  return 0;
            }
            free(z->expanded); z->expanded = NULL;
            return 1;
         }

         default:
            // if critical, fail
            if (first) return e("first not IHDR", "Corrupt PNG");
            if ((c.type & (1 << 29)) == 0) {
               #ifndef STBI_NO_FAILURE_STRINGS
               // not threadsafe
               static char invalid_chunk[] = "XXXX chunk not known";
               invalid_chunk[0] = (uint8) (c.type >> 24);
               invalid_chunk[1] = (uint8) (c.type >> 16);
               invalid_chunk[2] = (uint8) (c.type >>  8);
               invalid_chunk[3] = (uint8) (c.type >>  0);
               #endif
               return e(invalid_chunk, "PNG not supported: unknown chunk type");
            }
            skip(s, c.length);
            break;
      }
      // end of chunk, read and skip CRC
      get32(s);
   }
}

static unsigned char *do_png(png *p, int *x, int *y, int *n, int req_comp)
{
   unsigned char *result=NULL;
   if (req_comp < 0 || req_comp > 4) return epuc("bad req_comp", "Internal error");
   if (parse_png_file(p, SCAN_load, req_comp)) {
      result = p->out;
      p->out = NULL;
      if (req_comp && req_comp != p->s->img_out_n) {
         result = convert_format(result, p->s->img_out_n, req_comp, p->s->img_x, p->s->img_y);
         p->s->img_out_n = req_comp;
         if (result == NULL) return result;
      }
      *x = p->s->img_x;
      *y = p->s->img_y;
      if (n) *n = p->s->img_n;
   }
   free(p->out);      p->out      = NULL;
   free(p->expanded); p->expanded = NULL;
   free(p->idata);    p->idata    = NULL;

   return result;
}

static unsigned char *stbi_png_load(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   png p;
   p.s = s;
   return do_png(&p, x,y,comp,req_comp);
}

static int stbi_png_test(stbi *s)
{
   int r;
   r = check_png_header(s);
   stbi_rewind(s);
   return r;
}

static int stbi_png_info_raw(png *p, int *x, int *y, int *comp)
{
   if (!parse_png_file(p, SCAN_header, 0)) {
      stbi_rewind( p->s );
      return 0;
   }
   if (x) *x = p->s->img_x;
   if (y) *y = p->s->img_y;
   if (comp) *comp = p->s->img_n;
   return 1;
}

static int      stbi_png_info(stbi *s, int *x, int *y, int *comp)
{
   png p;
   p.s = s;
   return stbi_png_info_raw(&p, x, y, comp);
}

// Microsoft/Windows BMP image

static int bmp_test(stbi *s)
{
   int sz;
   if (get8(s) != 'B') return 0;
   if (get8(s) != 'M') return 0;
   get32le(s); // discard filesize
   get16le(s); // discard reserved
   get16le(s); // discard reserved
   get32le(s); // discard data offset
   sz = get32le(s);
   if (sz == 12 || sz == 40 || sz == 56 || sz == 108) return 1;
   return 0;
}

static int stbi_bmp_test(stbi *s)
{
   int r = bmp_test(s);
   stbi_rewind(s);
   return r;
}


// returns 0..31 for the highest set bit
static int high_bit(unsigned int z)
{
   int n=0;
   if (z == 0) return -1;
   if (z >= 0x10000) n += 16, z >>= 16;
   if (z >= 0x00100) n +=  8, z >>=  8;
   if (z >= 0x00010) n +=  4, z >>=  4;
   if (z >= 0x00004) n +=  2, z >>=  2;
   if (z >= 0x00002) n +=  1, z >>=  1;
   return n;
}

static int bitcount(unsigned int a)
{
   a = (a & 0x55555555) + ((a >>  1) & 0x55555555); // max 2
   a = (a & 0x33333333) + ((a >>  2) & 0x33333333); // max 4
   a = (a + (a >> 4)) & 0x0f0f0f0f; // max 8 per 4, now 8 bits
   a = (a + (a >> 8)); // max 16 per 8 bits
   a = (a + (a >> 16)); // max 32 per 8 bits
   return a & 0xff;
}

static int shiftsigned(int v, int shift, int bits)
{
   int result;
   int z=0;

   if (shift < 0) v <<= -shift;
   else v >>= shift;
   result = v;

   z = bits;
   while (z < 8) {
      result += v >> z;
      z += bits;
   }
   return result;
}

static stbi_uc *bmp_load(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   uint8 *out;
   unsigned int mr=0,mg=0,mb=0,ma=0, fake_a=0;
   stbi_uc pal[256][4];
   int psize=0,i,j,compress=0,width;
   int bpp, flip_vertically, pad, target, offset, hsz;
   if (get8(s) != 'B' || get8(s) != 'M') return epuc("not BMP", "Corrupt BMP");
   get32le(s); // discard filesize
   get16le(s); // discard reserved
   get16le(s); // discard reserved
   offset = get32le(s);
   hsz = get32le(s);
   if (hsz != 12 && hsz != 40 && hsz != 56 && hsz != 108) return epuc("unknown BMP", "BMP type not supported: unknown");
   if (hsz == 12) {
      s->img_x = get16le(s);
      s->img_y = get16le(s);
   } else {
      s->img_x = get32le(s);
      s->img_y = get32le(s);
   }
   if (get16le(s) != 1) return epuc("bad BMP", "bad BMP");
   bpp = get16le(s);
   if (bpp == 1) return epuc("monochrome", "BMP type not supported: 1-bit");
   flip_vertically = ((int) s->img_y) > 0;
   s->img_y = abs((int) s->img_y);
   if (hsz == 12) {
      if (bpp < 24)
         psize = (offset - 14 - 24) / 3;
   } else {
      compress = get32le(s);
      if (compress == 1 || compress == 2) return epuc("BMP RLE", "BMP type not supported: RLE");
      get32le(s); // discard sizeof
      get32le(s); // discard hres
      get32le(s); // discard vres
      get32le(s); // discard colorsused
      get32le(s); // discard max important
      if (hsz == 40 || hsz == 56) {
         if (hsz == 56) {
            get32le(s);
            get32le(s);
            get32le(s);
            get32le(s);
         }
         if (bpp == 16 || bpp == 32) {
            mr = mg = mb = 0;
            if (compress == 0) {
               if (bpp == 32) {
                  mr = 0xffu << 16;
                  mg = 0xffu <<  8;
                  mb = 0xffu <<  0;
                  ma = 0xffu << 24;
                  fake_a = 1; // @TODO: check for cases like alpha value is all 0 and switch it to 255
               } else {
                  mr = 31u << 10;
                  mg = 31u <<  5;
                  mb = 31u <<  0;
               }
            } else if (compress == 3) {
               mr = get32le(s);
               mg = get32le(s);
               mb = get32le(s);
               // not documented, but generated by photoshop and handled by mspaint
               if (mr == mg && mg == mb) {
                  // ?!?!?
                  return epuc("bad BMP", "bad BMP");
               }
            } else
               return epuc("bad BMP", "bad BMP");
         }
      } else {
         assert(hsz == 108);
         mr = get32le(s);
         mg = get32le(s);
         mb = get32le(s);
         ma = get32le(s);
         get32le(s); // discard color space
         for (i=0; i < 12; ++i)
            get32le(s); // discard color space parameters
      }
      if (bpp < 16)
         psize = (offset - 14 - hsz) >> 2;
   }
   s->img_n = ma ? 4 : 3;
   if (req_comp && req_comp >= 3) // we can directly decode 3 or 4
      target = req_comp;
   else
      target = s->img_n; // if they want monochrome, we'll post-convert
   out = (stbi_uc *) malloc(target * s->img_x * s->img_y);
   if (!out) return epuc("outofmem", "Out of memory");
   if (bpp < 16) {
      int z=0;
      if (psize == 0 || psize > 256) { free(out); return epuc("invalid", "Corrupt BMP"); }
      for (i=0; i < psize; ++i) {
         pal[i][2] = get8u(s);
         pal[i][1] = get8u(s);
         pal[i][0] = get8u(s);
         if (hsz != 12) get8(s);
         pal[i][3] = 255;
      }
      skip(s, offset - 14 - hsz - psize * (hsz == 12 ? 3 : 4));
      if (bpp == 4) width = (s->img_x + 1) >> 1;
      else if (bpp == 8) width = s->img_x;
      else { free(out); return epuc("bad bpp", "Corrupt BMP"); }
      pad = (-width)&3;
      for (j=0; j < (int) s->img_y; ++j) {
         for (i=0; i < (int) s->img_x; i += 2) {
            int v=get8(s),v2=0;
            if (bpp == 4) {
               v2 = v & 15;
               v >>= 4;
            }
            out[z++] = pal[v][0];
            out[z++] = pal[v][1];
            out[z++] = pal[v][2];
            if (target == 4) out[z++] = 255;
            if (i+1 == (int) s->img_x) break;
            v = (bpp == 8) ? get8(s) : v2;
            out[z++] = pal[v][0];
            out[z++] = pal[v][1];
            out[z++] = pal[v][2];
            if (target == 4) out[z++] = 255;
         }
         skip(s, pad);
      }
   } else {
      int rshift=0,gshift=0,bshift=0,ashift=0,rcount=0,gcount=0,bcount=0,acount=0;
      int z = 0;
      int easy=0;
      skip(s, offset - 14 - hsz);
      if (bpp == 24) width = 3 * s->img_x;
      else if (bpp == 16) width = 2*s->img_x;
      else /* bpp = 32 and pad = 0 */ width=0;
      pad = (-width) & 3;
      if (bpp == 24) {
         easy = 1;
      } else if (bpp == 32) {
         if (mb == 0xff && mg == 0xff00 && mr == 0x00ff0000 && ma == 0xff000000)
            easy = 2;
      }
      if (!easy) {
         if (!mr || !mg || !mb) { free(out); return epuc("bad masks", "Corrupt BMP"); }
         // right shift amt to put high bit in position #7
         rshift = high_bit(mr)-7; rcount = bitcount(mr);
         gshift = high_bit(mg)-7; gcount = bitcount(mr);
         bshift = high_bit(mb)-7; bcount = bitcount(mr);
         ashift = high_bit(ma)-7; acount = bitcount(mr);
      }
      for (j=0; j < (int) s->img_y; ++j) {
         if (easy) {
            for (i=0; i < (int) s->img_x; ++i) {
               int a;
               out[z+2] = get8u(s);
               out[z+1] = get8u(s);
               out[z+0] = get8u(s);
               z += 3;
               a = (easy == 2 ? get8(s) : 255);
               if (target == 4) out[z++] = (uint8) a;
            }
         } else {
            for (i=0; i < (int) s->img_x; ++i) {
               uint32 v = (bpp == 16 ? get16le(s) : get32le(s));
               int a;
               out[z++] = (uint8) shiftsigned(v & mr, rshift, rcount);
               out[z++] = (uint8) shiftsigned(v & mg, gshift, gcount);
               out[z++] = (uint8) shiftsigned(v & mb, bshift, bcount);
               a = (ma ? shiftsigned(v & ma, ashift, acount) : 255);
               if (target == 4) out[z++] = (uint8) a; 
            }
         }
         skip(s, pad);
      }
   }
   if (flip_vertically) {
      stbi_uc t;
      for (j=0; j < (int) s->img_y>>1; ++j) {
         stbi_uc *p1 = out +      j     *s->img_x*target;
         stbi_uc *p2 = out + (s->img_y-1-j)*s->img_x*target;
         for (i=0; i < (int) s->img_x*target; ++i) {
            t = p1[i], p1[i] = p2[i], p2[i] = t;
         }
      }
   }

   if (req_comp && req_comp != target) {
      out = convert_format(out, target, req_comp, s->img_x, s->img_y);
      if (out == NULL) return out; // convert_format frees input on failure
   }

   *x = s->img_x;
   *y = s->img_y;
   if (comp) *comp = s->img_n;
   return out;
}

static stbi_uc *stbi_bmp_load(stbi *s,int *x, int *y, int *comp, int req_comp)
{
   return bmp_load(s, x,y,comp,req_comp);
}


// Targa Truevision - TGA
// by Jonathan Dummer

static int tga_info(stbi *s, int *x, int *y, int *comp)
{
    int tga_w, tga_h, tga_comp;
    int sz;
    get8u(s);                   // discard Offset
    sz = get8u(s);              // color type
    if( sz > 1 ) {
        stbi_rewind(s);
        return 0;      // only RGB or indexed allowed
    }
    sz = get8u(s);              // image type
    // only RGB or grey allowed, +/- RLE
    if ((sz != 1) && (sz != 2) && (sz != 3) && (sz != 9) && (sz != 10) && (sz != 11)) return 0;
    skip(s,9);
    tga_w = get16le(s);
    if( tga_w < 1 ) {
        stbi_rewind(s);
        return 0;   // test width
    }
    tga_h = get16le(s);
    if( tga_h < 1 ) {
        stbi_rewind(s);
        return 0;   // test height
    }
    sz = get8(s);               // bits per pixel
    // only RGB or RGBA or grey allowed
    if ((sz != 8) && (sz != 16) && (sz != 24) && (sz != 32)) {
        stbi_rewind(s);
        return 0;
    }
    tga_comp = sz;
    if (x) *x = tga_w;
    if (y) *y = tga_h;
    if (comp) *comp = tga_comp / 8;
    return 1;                   // seems to have passed everything
}

int stbi_tga_info(stbi *s, int *x, int *y, int *comp)
{
    return tga_info(s, x, y, comp);
}

static int tga_test(stbi *s)
{
   int sz;
   get8u(s);      //   discard Offset
   sz = get8u(s);   //   color type
   if ( sz > 1 ) return 0;   //   only RGB or indexed allowed
   sz = get8u(s);   //   image type
   if ( (sz != 1) && (sz != 2) && (sz != 3) && (sz != 9) && (sz != 10) && (sz != 11) ) return 0;   //   only RGB or grey allowed, +/- RLE
   get16(s);      //   discard palette start
   get16(s);      //   discard palette length
   get8(s);         //   discard bits per palette color entry
   get16(s);      //   discard x origin
   get16(s);      //   discard y origin
   if ( get16(s) < 1 ) return 0;      //   test width
   if ( get16(s) < 1 ) return 0;      //   test height
   sz = get8(s);   //   bits per pixel
   if ( (sz != 8) && (sz != 16) && (sz != 24) && (sz != 32) ) return 0;   //   only RGB or RGBA or grey allowed
   return 1;      //   seems to have passed everything
}

static int stbi_tga_test(stbi *s)
{
   int res = tga_test(s);
   stbi_rewind(s);
   return res;
}

static stbi_uc *tga_load(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   //   read in the TGA header stuff
   int tga_offset = get8u(s);
   int tga_indexed = get8u(s);
   int tga_image_type = get8u(s);
   int tga_is_RLE = 0;
   int tga_palette_start = get16le(s);
   int tga_palette_len = get16le(s);
   int tga_palette_bits = get8u(s);
   int tga_x_origin = get16le(s);
   int tga_y_origin = get16le(s);
   int tga_width = get16le(s);
   int tga_height = get16le(s);
   int tga_bits_per_pixel = get8u(s);
   int tga_inverted = get8u(s);
   //   image data
   unsigned char *tga_data;
   unsigned char *tga_palette = NULL;
   int i, j;
   unsigned char raw_data[4];
   unsigned char trans_data[4];
   int RLE_count = 0;
   int RLE_repeating = 0;
   int read_next_pixel = 1;

   //   do a tiny bit of precessing
   if ( tga_image_type >= 8 )
   {
      tga_image_type -= 8;
      tga_is_RLE = 1;
   }
   /* int tga_alpha_bits = tga_inverted & 15; */
   tga_inverted = 1 - ((tga_inverted >> 5) & 1);

   //   error check
   if ( //(tga_indexed) ||
      (tga_width < 1) || (tga_height < 1) ||
      (tga_image_type < 1) || (tga_image_type > 3) ||
      ((tga_bits_per_pixel != 8) && (tga_bits_per_pixel != 16) &&
      (tga_bits_per_pixel != 24) && (tga_bits_per_pixel != 32))
      )
   {
      return NULL; // we don't report this as a bad TGA because we don't even know if it's TGA
   }

   //   If I'm paletted, then I'll use the number of bits from the palette
   if ( tga_indexed )
   {
      tga_bits_per_pixel = tga_palette_bits;
   }

   //   tga info
   *x = tga_width;
   *y = tga_height;
   if ( (req_comp < 1) || (req_comp > 4) )
   {
      //   just use whatever the file was
      req_comp = tga_bits_per_pixel / 8;
      *comp = req_comp;
   } else
   {
      //   force a new number of components
      *comp = tga_bits_per_pixel/8;
   }
   tga_data = (unsigned char*)malloc( tga_width * tga_height * req_comp );
   if (!tga_data) return epuc("outofmem", "Out of memory");

   //   skip to the data's starting position (offset usually = 0)
   skip(s, tga_offset );
   //   do I need to load a palette?
   if ( tga_indexed )
   {
      //   any data to skip? (offset usually = 0)
      skip(s, tga_palette_start );
      //   load the palette
      tga_palette = (unsigned char*)malloc( tga_palette_len * tga_palette_bits / 8 );
      if (!tga_palette) return epuc("outofmem", "Out of memory");
      if (!getn(s, tga_palette, tga_palette_len * tga_palette_bits / 8 )) {
         free(tga_data);
         free(tga_palette);
         return epuc("bad palette", "Corrupt TGA");
      }
   }
   //   load the data
   trans_data[0] = trans_data[1] = trans_data[2] = trans_data[3] = 0;
   for (i=0; i < tga_width * tga_height; ++i)
   {
      //   if I'm in RLE mode, do I need to get a RLE chunk?
      if ( tga_is_RLE )
      {
         if ( RLE_count == 0 )
         {
            //   yep, get the next byte as a RLE command
            int RLE_cmd = get8u(s);
            RLE_count = 1 + (RLE_cmd & 127);
            RLE_repeating = RLE_cmd >> 7;
            read_next_pixel = 1;
         } else if ( !RLE_repeating )
         {
            read_next_pixel = 1;
         }
      } else
      {
         read_next_pixel = 1;
      }
      //   OK, if I need to read a pixel, do it now
      if ( read_next_pixel )
      {
         //   load however much data we did have
         if ( tga_indexed )
         {
            //   read in 1 byte, then perform the lookup
            int pal_idx = get8u(s);
            if ( pal_idx >= tga_palette_len )
            {
               //   invalid index
               pal_idx = 0;
            }
            pal_idx *= tga_bits_per_pixel / 8;
            for (j = 0; j*8 < tga_bits_per_pixel; ++j)
            {
               raw_data[j] = tga_palette[pal_idx+j];
            }
         } else
         {
            //   read in the data raw
            for (j = 0; j*8 < tga_bits_per_pixel; ++j)
            {
               raw_data[j] = get8u(s);
            }
         }
         //   convert raw to the intermediate format
         switch (tga_bits_per_pixel)
         {
         case 8:
            //   Luminous => RGBA
            trans_data[0] = raw_data[0];
            trans_data[1] = raw_data[0];
            trans_data[2] = raw_data[0];
            trans_data[3] = 255;
            break;
         case 16:
            //   Luminous,Alpha => RGBA
            trans_data[0] = raw_data[0];
            trans_data[1] = raw_data[0];
            trans_data[2] = raw_data[0];
            trans_data[3] = raw_data[1];
            break;
         case 24:
            //   BGR => RGBA
            trans_data[0] = raw_data[2];
            trans_data[1] = raw_data[1];
            trans_data[2] = raw_data[0];
            trans_data[3] = 255;
            break;
         case 32:
            //   BGRA => RGBA
            trans_data[0] = raw_data[2];
            trans_data[1] = raw_data[1];
            trans_data[2] = raw_data[0];
            trans_data[3] = raw_data[3];
            break;
         }
         //   clear the reading flag for the next pixel
         read_next_pixel = 0;
      } // end of reading a pixel
      //   convert to final format
      switch (req_comp)
      {
      case 1:
         //   RGBA => Luminance
         tga_data[i*req_comp+0] = compute_y(trans_data[0],trans_data[1],trans_data[2]);
         break;
      case 2:
         //   RGBA => Luminance,Alpha
         tga_data[i*req_comp+0] = compute_y(trans_data[0],trans_data[1],trans_data[2]);
         tga_data[i*req_comp+1] = trans_data[3];
         break;
      case 3:
         //   RGBA => RGB
         tga_data[i*req_comp+0] = trans_data[0];
         tga_data[i*req_comp+1] = trans_data[1];
         tga_data[i*req_comp+2] = trans_data[2];
         break;
      case 4:
         //   RGBA => RGBA
         tga_data[i*req_comp+0] = trans_data[0];
         tga_data[i*req_comp+1] = trans_data[1];
         tga_data[i*req_comp+2] = trans_data[2];
         tga_data[i*req_comp+3] = trans_data[3];
         break;
      }
      //   in case we're in RLE mode, keep counting down
      --RLE_count;
   }
   //   do I need to invert the image?
   if ( tga_inverted )
   {
      for (j = 0; j*2 < tga_height; ++j)
      {
         int index1 = j * tga_width * req_comp;
         int index2 = (tga_height - 1 - j) * tga_width * req_comp;
         for (i = tga_width * req_comp; i > 0; --i)
         {
            unsigned char temp = tga_data[index1];
            tga_data[index1] = tga_data[index2];
            tga_data[index2] = temp;
            ++index1;
            ++index2;
         }
      }
   }
   //   clear my palette, if I had one
   if ( tga_palette != NULL )
   {
      free( tga_palette );
   }
   //   the things I do to get rid of an error message, and yet keep
   //   Microsoft's C compilers happy... [8^(
   tga_palette_start = tga_palette_len = tga_palette_bits =
         tga_x_origin = tga_y_origin = 0;
   //   OK, done
   return tga_data;
}

static stbi_uc *stbi_tga_load(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   return tga_load(s,x,y,comp,req_comp);
}


// *************************************************************************************************
// Photoshop PSD loader -- PD by Thatcher Ulrich, integration by Nicolas Schulz, tweaked by STB

static int psd_test(stbi *s)
{
   if (get32(s) != 0x38425053) return 0;   // "8BPS"
   else return 1;
}

static int stbi_psd_test(stbi *s)
{
   int r = psd_test(s);
   stbi_rewind(s);
   return r;
}

static stbi_uc *psd_load(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   int   pixelCount;
   int channelCount, compression;
   int channel, i, count, len;
   int w,h;
   uint8 *out;

   // Check identifier
   if (get32(s) != 0x38425053)   // "8BPS"
      return epuc("not PSD", "Corrupt PSD image");

   // Check file type version.
   if (get16(s) != 1)
      return epuc("wrong version", "Unsupported version of PSD image");

   // Skip 6 reserved bytes.
   skip(s, 6 );

   // Read the number of channels (R, G, B, A, etc).
   channelCount = get16(s);
   if (channelCount < 0 || channelCount > 16)
      return epuc("wrong channel count", "Unsupported number of channels in PSD image");

   // Read the rows and columns of the image.
   h = get32(s);
   w = get32(s);
   
   // Make sure the depth is 8 bits.
   if (get16(s) != 8)
      return epuc("unsupported bit depth", "PSD bit depth is not 8 bit");

   // Make sure the color mode is RGB.
   // Valid options are:
   //   0: Bitmap
   //   1: Grayscale
   //   2: Indexed color
   //   3: RGB color
   //   4: CMYK color
   //   7: Multichannel
   //   8: Duotone
   //   9: Lab color
   if (get16(s) != 3)
      return epuc("wrong color format", "PSD is not in RGB color format");

   // Skip the Mode Data.  (It's the palette for indexed color; other info for other modes.)
   skip(s,get32(s) );

   // Skip the image resources.  (resolution, pen tool paths, etc)
   skip(s, get32(s) );

   // Skip the reserved data.
   skip(s, get32(s) );

   // Find out if the data is compressed.
   // Known values:
   //   0: no compression
   //   1: RLE compressed
   compression = get16(s);
   if (compression > 1)
      return epuc("bad compression", "PSD has an unknown compression format");

   // Create the destination image.
   out = (stbi_uc *) malloc(4 * w*h);
   if (!out) return epuc("outofmem", "Out of memory");
   pixelCount = w*h;

   // Initialize the data to zero.
   //memset( out, 0, pixelCount * 4 );
   
   // Finally, the image data.
   if (compression) {
      // RLE as used by .PSD and .TIFF
      // Loop until you get the number of unpacked bytes you are expecting:
      //     Read the next source byte into n.
      //     If n is between 0 and 127 inclusive, copy the next n+1 bytes literally.
      //     Else if n is between -127 and -1 inclusive, copy the next byte -n+1 times.
      //     Else if n is 128, noop.
      // Endloop

      // The RLE-compressed data is preceeded by a 2-byte data count for each row in the data,
      // which we're going to just skip.
      skip(s, h * channelCount * 2 );

      // Read the RLE data by channel.
      for (channel = 0; channel < 4; channel++) {
         uint8 *p;
         
         p = out+channel;
         if (channel >= channelCount) {
            // Fill this channel with default data.
            for (i = 0; i < pixelCount; i++) *p = (channel == 3 ? 255 : 0), p += 4;
         } else {
            // Read the RLE data.
            count = 0;
            while (count < pixelCount) {
               len = get8(s);
               if (len == 128) {
                  // No-op.
               } else if (len < 128) {
                  // Copy next len+1 bytes literally.
                  len++;
                  count += len;
                  while (len) {
                     *p = get8u(s);
                     p += 4;
                     len--;
                  }
               } else if (len > 128) {
                  uint8   val;
                  // Next -len+1 bytes in the dest are replicated from next source byte.
                  // (Interpret len as a negative 8-bit int.)
                  len ^= 0x0FF;
                  len += 2;
                  val = get8u(s);
                  count += len;
                  while (len) {
                     *p = val;
                     p += 4;
                     len--;
                  }
               }
            }
         }
      }
      
   } else {
      // We're at the raw image data.  It's each channel in order (Red, Green, Blue, Alpha, ...)
      // where each channel consists of an 8-bit value for each pixel in the image.
      
      // Read the data by channel.
      for (channel = 0; channel < 4; channel++) {
         uint8 *p;
         
         p = out + channel;
         if (channel > channelCount) {
            // Fill this channel with default data.
            for (i = 0; i < pixelCount; i++) *p = channel == 3 ? 255 : 0, p += 4;
         } else {
            // Read the data.
            for (i = 0; i < pixelCount; i++)
               *p = get8u(s), p += 4;
         }
      }
   }

   if (req_comp && req_comp != 4) {
      out = convert_format(out, 4, req_comp, w, h);
      if (out == NULL) return out; // convert_format frees input on failure
   }

   if (comp) *comp = channelCount;
   *y = h;
   *x = w;
   
   return out;
}

static stbi_uc *stbi_psd_load(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   return psd_load(s,x,y,comp,req_comp);
}

// *************************************************************************************************
// Softimage PIC loader
// by Tom Seddon
//
// See http://softimage.wiki.softimage.com/index.php/INFO:_PIC_file_format
// See http://ozviz.wasp.uwa.edu.au/~pbourke/dataformats/softimagepic/

static int pic_is4(stbi *s,const char *str)
{
   int i;
   for (i=0; i<4; ++i)
      if (get8(s) != (stbi_uc)str[i])
         return 0;

   return 1;
}

static int pic_test(stbi *s)
{
   int i;

   if (!pic_is4(s,"\x53\x80\xF6\x34"))
      return 0;

   for(i=0;i<84;++i)
      get8(s);

   if (!pic_is4(s,"PICT"))
      return 0;

   return 1;
}

typedef struct
{
   stbi_uc size,type,channel;
} pic_packet_t;

static stbi_uc *pic_readval(stbi *s, int channel, stbi_uc *dest)
{
   int mask=0x80, i;

   for (i=0; i<4; ++i, mask>>=1) {
      if (channel & mask) {
         if (at_eof(s)) return epuc("bad file","PIC file too short");
         dest[i]=get8u(s);
      }
   }

   return dest;
}

static void pic_copyval(int channel,stbi_uc *dest,const stbi_uc *src)
{
   int mask=0x80,i;

   for (i=0;i<4; ++i, mask>>=1)
      if (channel&mask)
         dest[i]=src[i];
}

static stbi_uc *pic_load2(stbi *s,int width,int height,int *comp, stbi_uc *result)
{
   int act_comp=0,num_packets=0,y,chained;
   pic_packet_t packets[10];

   // this will (should...) cater for even some bizarre stuff like having data
    // for the same channel in multiple packets.
   do {
      pic_packet_t *packet;

      if (num_packets==sizeof(packets)/sizeof(packets[0]))
         return epuc("bad format","too many packets");

      packet = &packets[num_packets++];

      chained = get8(s);
      packet->size    = get8u(s);
      packet->type    = get8u(s);
      packet->channel = get8u(s);

      act_comp |= packet->channel;

      if (at_eof(s))          return epuc("bad file","file too short (reading packets)");
      if (packet->size != 8)  return epuc("bad format","packet isn't 8bpp");
   } while (chained);

   *comp = (act_comp & 0x10 ? 4 : 3); // has alpha channel?

   for(y=0; y<height; ++y) {
      int packet_idx;

      for(packet_idx=0; packet_idx < num_packets; ++packet_idx) {
         pic_packet_t *packet = &packets[packet_idx];
         stbi_uc *dest = result+y*width*4;

         switch (packet->type) {
            default:
               return epuc("bad format","packet has bad compression type");

            case 0: {//uncompressed
               int x;

               for(x=0;x<width;++x, dest+=4)
                  if (!pic_readval(s,packet->channel,dest))
                     return 0;
               break;
            }

            case 1://Pure RLE
               {
                  int left=width, i;

                  while (left>0) {
                     stbi_uc count,value[4];

                     count=get8u(s);
                     if (at_eof(s))   return epuc("bad file","file too short (pure read count)");

                     if (count > left)
                        count = (uint8) left;

                     if (!pic_readval(s,packet->channel,value))  return 0;

                     for(i=0; i<count; ++i,dest+=4)
                        pic_copyval(packet->channel,dest,value);
                     left -= count;
                  }
               }
               break;

            case 2: {//Mixed RLE
               int left=width;
               while (left>0) {
                  int count = get8(s), i;
                  if (at_eof(s))  return epuc("bad file","file too short (mixed read count)");

                  if (count >= 128) { // Repeated
                     stbi_uc value[4];
                     int ni;

                     if (count==128)
                        count = get16(s);
                     else
                        count -= 127;
                     if (count > left)
                        return epuc("bad file","scanline overrun");

                     if (!pic_readval(s,packet->channel,value))
                        return 0;

                     for(ni=0;ni<count;++ni, dest += 4)
                        pic_copyval(packet->channel,dest,value);
                  } else { // Raw
                     ++count;
                     if (count>left) return epuc("bad file","scanline overrun");

                     for(i=0;i<count;++i, dest+=4)
                        if (!pic_readval(s,packet->channel,dest))
                           return 0;
                  }
                  left-=count;
               }
               break;
            }
         }
      }
   }

   return result;
}

static stbi_uc *pic_load(stbi *s,int *px,int *py,int *comp,int req_comp)
{
   stbi_uc *result;
   int i, x,y;

   for (i=0; i<92; ++i)
      get8(s);

   x = get16(s);
   y = get16(s);
   if (at_eof(s))  return epuc("bad file","file too short (pic header)");
   if ((1 << 28) / x < y) return epuc("too large", "Image too large to decode");

   get32(s); //skip `ratio'
   get16(s); //skip `fields'
   get16(s); //skip `pad'

   // intermediate buffer is RGBA
   result = (stbi_uc *) malloc(x*y*4);
   memset(result, 0xff, x*y*4);

   if (!pic_load2(s,x,y,comp, result)) {
      free(result);
      result=0;
   }
   *px = x;
   *py = y;
   if (req_comp == 0) req_comp = *comp;
   result=convert_format(result,4,req_comp,x,y);

   return result;
}

static int stbi_pic_test(stbi *s)
{
   int r = pic_test(s);
   stbi_rewind(s);
   return r;
}

static stbi_uc *stbi_pic_load(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   return pic_load(s,x,y,comp,req_comp);
}

// *************************************************************************************************
// GIF loader -- public domain by Jean-Marc Lienher -- simplified/shrunk by stb
typedef struct stbi_gif_lzw_struct {
   int16 prefix;
   uint8 first;
   uint8 suffix;
} stbi_gif_lzw;

typedef struct stbi_gif_struct
{
   int w,h;
   stbi_uc *out;                 // output buffer (always 4 components)
   int flags, bgindex, ratio, transparent, eflags;
   uint8  pal[256][4];
   uint8 lpal[256][4];
   stbi_gif_lzw codes[4096];
   uint8 *color_table;
   int parse, step;
   int lflags;
   int start_x, start_y;
   int max_x, max_y;
   int cur_x, cur_y;
   int line_size;
} stbi_gif;

static int gif_test(stbi *s)
{
   int sz;
   if (get8(s) != 'G' || get8(s) != 'I' || get8(s) != 'F' || get8(s) != '8') return 0;
   sz = get8(s);
   if (sz != '9' && sz != '7') return 0;
   if (get8(s) != 'a') return 0;
   return 1;
}

static int stbi_gif_test(stbi *s)
{
   int r = gif_test(s);
   stbi_rewind(s);
   return r;
}

static void stbi_gif_parse_colortable(stbi *s, uint8 pal[256][4], int num_entries, int transp)
{
   int i;
   for (i=0; i < num_entries; ++i) {
      pal[i][2] = get8u(s);
      pal[i][1] = get8u(s);
      pal[i][0] = get8u(s);
      pal[i][3] = transp ? 0 : 255;
   }   
}

static int stbi_gif_header(stbi *s, stbi_gif *g, int *comp, int is_info)
{
   uint8 version;
   if (get8(s) != 'G' || get8(s) != 'I' || get8(s) != 'F' || get8(s) != '8')
      return e("not GIF", "Corrupt GIF");

   version = get8u(s);
   if (version != '7' && version != '9')    return e("not GIF", "Corrupt GIF");
   if (get8(s) != 'a')                      return e("not GIF", "Corrupt GIF");
 
   failure_reason = "";
   g->w = get16le(s);
   g->h = get16le(s);
   g->flags = get8(s);
   g->bgindex = get8(s);
   g->ratio = get8(s);
   g->transparent = -1;

   if (comp != 0) *comp = 4;  // can't actually tell whether it's 3 or 4 until we parse the comments

   if (is_info) return 1;

   if (g->flags & 0x80)
      stbi_gif_parse_colortable(s,g->pal, 2 << (g->flags & 7), -1);

   return 1;
}

static int stbi_gif_info_raw(stbi *s, int *x, int *y, int *comp)
{
   stbi_gif g;   
   if (!stbi_gif_header(s, &g, comp, 1)) {
      stbi_rewind( s );
      return 0;
   }
   if (x) *x = g.w;
   if (y) *y = g.h;
   return 1;
}

static void stbi_out_gif_code(stbi_gif *g, uint16 code)
{
   uint8 *p, *c;

   // recurse to decode the prefixes, since the linked-list is backwards,
   // and working backwards through an interleaved image would be nasty
   if (g->codes[code].prefix >= 0)
      stbi_out_gif_code(g, g->codes[code].prefix);

   if (g->cur_y >= g->max_y) return;
  
   p = &g->out[g->cur_x + g->cur_y];
   c = &g->color_table[g->codes[code].suffix * 4];

   if (c[3] >= 128) {
      p[0] = c[2];
      p[1] = c[1];
      p[2] = c[0];
      p[3] = c[3];
   }
   g->cur_x += 4;

   if (g->cur_x >= g->max_x) {
      g->cur_x = g->start_x;
      g->cur_y += g->step;

      while (g->cur_y >= g->max_y && g->parse > 0) {
         g->step = (1 << g->parse) * g->line_size;
         g->cur_y = g->start_y + (g->step >> 1);
         --g->parse;
      }
   }
}

static uint8 *stbi_process_gif_raster(stbi *s, stbi_gif *g)
{
   uint8 lzw_cs;
   int32 len, code;
   uint32 first;
   int32 codesize, codemask, avail, oldcode, bits, valid_bits, clear;
   stbi_gif_lzw *p;

   lzw_cs = get8u(s);
   clear = 1 << lzw_cs;
   first = 1;
   codesize = lzw_cs + 1;
   codemask = (1 << codesize) - 1;
   bits = 0;
   valid_bits = 0;
   for (code = 0; code < clear; code++) {
      g->codes[code].prefix = -1;
      g->codes[code].first = (uint8) code;
      g->codes[code].suffix = (uint8) code;
   }

   // support no starting clear code
   avail = clear+2;
   oldcode = -1;

   len = 0;
   for(;;) {
      if (valid_bits < codesize) {
         if (len == 0) {
            len = get8(s); // start new block
            if (len == 0) 
               return g->out;
         }
         --len;
         bits |= (int32) get8(s) << valid_bits;
         valid_bits += 8;
      } else {
         int32 ncode = bits & codemask;
         bits >>= codesize;
         valid_bits -= codesize;
         // @OPTIMIZE: is there some way we can accelerate the non-clear path?
         if (ncode == clear) {  // clear code
            codesize = lzw_cs + 1;
            codemask = (1 << codesize) - 1;
            avail = clear + 2;
            oldcode = -1;
            first = 0;
         } else if (ncode == clear + 1) { // end of stream code
            skip(s, len);
            while ((len = get8(s)) > 0)
               skip(s,len);
            return g->out;
         } else if (ncode <= avail) {
            if (first) return epuc("no clear code", "Corrupt GIF");

            if (oldcode >= 0) {
               p = &g->codes[avail++];
               if (avail > 4096)        return epuc("too many codes", "Corrupt GIF");
               p->prefix = (int16) oldcode;
               p->first = g->codes[oldcode].first;
               p->suffix = (ncode == avail) ? p->first : g->codes[ncode].first;
            } else if (ncode == avail)
               return epuc("illegal code in raster", "Corrupt GIF");

            stbi_out_gif_code(g, (uint16) ncode);

            if ((avail & codemask) == 0 && avail <= 0x0FFF) {
               codesize++;
               codemask = (1 << codesize) - 1;
            }

            oldcode = ncode;
         } else {
            return epuc("illegal code in raster", "Corrupt GIF");
         }
      } 
   }
}

static void stbi_fill_gif_background(stbi_gif *g)
{
   int i;
   uint8 *c = g->pal[g->bgindex];
   // @OPTIMIZE: write a dword at a time
   for (i = 0; i < g->w * g->h * 4; i += 4) {
      uint8 *p  = &g->out[i];
      p[0] = c[2];
      p[1] = c[1];
      p[2] = c[0];
      p[3] = c[3];
   }
}

// this function is designed to support animated gifs, although stb_image doesn't support it
static uint8 *stbi_gif_load_next(stbi *s, stbi_gif *g, int *comp, int req_comp)
{
   int i;
   uint8 *old_out = 0;

   if (g->out == 0) {
      if (!stbi_gif_header(s, g, comp,0))     return 0; // failure_reason set by stbi_gif_header
      g->out = (uint8 *) malloc(4 * g->w * g->h);
      if (g->out == 0)                      return epuc("outofmem", "Out of memory");
      stbi_fill_gif_background(g);
   } else {
      // animated-gif-only path
      if (((g->eflags & 0x1C) >> 2) == 3) {
         old_out = g->out;
         g->out = (uint8 *) malloc(4 * g->w * g->h);
         if (g->out == 0)                   return epuc("outofmem", "Out of memory");
         memcpy(g->out, old_out, g->w*g->h*4);
      }
   }
    
   for (;;) {
      switch (get8(s)) {
         case 0x2C: /* Image Descriptor */
         {
            int32 x, y, w, h;
            uint8 *o;

            x = get16le(s);
            y = get16le(s);
            w = get16le(s);
            h = get16le(s);
            if (((x + w) > (g->w)) || ((y + h) > (g->h)))
               return epuc("bad Image Descriptor", "Corrupt GIF");

            g->line_size = g->w * 4;
            g->start_x = x * 4;
            g->start_y = y * g->line_size;
            g->max_x   = g->start_x + w * 4;
            g->max_y   = g->start_y + h * g->line_size;
            g->cur_x   = g->start_x;
            g->cur_y   = g->start_y;

            g->lflags = get8(s);

            if (g->lflags & 0x40) {
               g->step = 8 * g->line_size; // first interlaced spacing
               g->parse = 3;
            } else {
               g->step = g->line_size;
               g->parse = 0;
            }

            if (g->lflags & 0x80) {
               stbi_gif_parse_colortable(s,g->lpal, 2 << (g->lflags & 7), g->eflags & 0x01 ? g->transparent : -1);
               g->color_table = (uint8 *) g->lpal;       
            } else if (g->flags & 0x80) {
               for (i=0; i < 256; ++i)  // @OPTIMIZE: reset only the previous transparent
                  g->pal[i][3] = 255; 
               if (g->transparent >= 0 && (g->eflags & 0x01))
                  g->pal[g->transparent][3] = 0;
               g->color_table = (uint8 *) g->pal;
            } else
               return epuc("missing color table", "Corrupt GIF");
   
            o = stbi_process_gif_raster(s, g);
            if (o == NULL) return NULL;

            if (req_comp && req_comp != 4)
               o = convert_format(o, 4, req_comp, g->w, g->h);
            return o;
         }

         case 0x21: // Comment Extension.
         {
            int len;
            if (get8(s) == 0xF9) { // Graphic Control Extension.
               len = get8(s);
               if (len == 4) {
                  g->eflags = get8(s);
                  get16le(s); // delay
                  g->transparent = get8(s);
               } else {
                  skip(s, len);
                  break;
               }
            }
            while ((len = get8(s)) != 0)
               skip(s, len);
            break;
         }

         case 0x3B: // gif stream termination code
            return (uint8 *) 1;

         default:
            return epuc("unknown code", "Corrupt GIF");
      }
   }
}

static stbi_uc *stbi_gif_load(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   uint8 *u = 0;
   stbi_gif g={0};

   u = stbi_gif_load_next(s, &g, comp, req_comp);
   if (u == (void *) 1) u = 0;  // end of animated gif marker
   if (u) {
      *x = g.w;
      *y = g.h;
   }

   return u;
}

static int stbi_gif_info(stbi *s, int *x, int *y, int *comp)
{
   return stbi_gif_info_raw(s,x,y,comp);
}


// *************************************************************************************************
// Radiance RGBE HDR loader
// originally by Nicolas Schulz
#ifndef STBI_NO_HDR
static int hdr_test(stbi *s)
{
   const char *signature = "#?RADIANCE\n";
   int i;
   for (i=0; signature[i]; ++i)
      if (get8(s) != signature[i])
         return 0;
   return 1;
}

static int stbi_hdr_test(stbi* s)
{
   int r = hdr_test(s);
   stbi_rewind(s);
   return r;
}

#define HDR_BUFLEN  1024
static char *hdr_gettoken(stbi *z, char *buffer)
{
   int len=0;
   char c = '\0';

   c = (char) get8(z);

   while (!at_eof(z) && c != '\n') {
      buffer[len++] = c;
      if (len == HDR_BUFLEN-1) {
         // flush to end of line
         while (!at_eof(z) && get8(z) != '\n')
            ;
         break;
      }
      c = (char) get8(z);
   }

   buffer[len] = 0;
   return buffer;
}

static void hdr_convert(float *output, stbi_uc *input, int req_comp)
{
   if ( input[3] != 0 ) {
      float f1;
      // Exponent
      f1 = (float) ldexp(1.0f, input[3] - (int)(128 + 8));
      if (req_comp <= 2)
         output[0] = (input[0] + input[1] + input[2]) * f1 / 3;
      else {
         output[0] = input[0] * f1;
         output[1] = input[1] * f1;
         output[2] = input[2] * f1;
      }
      if (req_comp == 2) output[1] = 1;
      if (req_comp == 4) output[3] = 1;
   } else {
      switch (req_comp) {
         case 4: output[3] = 1; /* fallthrough */
         case 3: output[0] = output[1] = output[2] = 0;
                 break;
         case 2: output[1] = 1; /* fallthrough */
         case 1: output[0] = 0;
                 break;
      }
   }
}

static float *hdr_load(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   char buffer[HDR_BUFLEN];
   char *token;
   int valid = 0;
   int width, height;
   stbi_uc *scanline;
   float *hdr_data;
   int len;
   unsigned char count, value;
   int i, j, k, c1,c2, z;


   // Check identifier
   if (strcmp(hdr_gettoken(s,buffer), "#?RADIANCE") != 0)
      return epf("not HDR", "Corrupt HDR image");
   
   // Parse header
   for(;;) {
      token = hdr_gettoken(s,buffer);
      if (token[0] == 0) break;
      if (strcmp(token, "FORMAT=32-bit_rle_rgbe") == 0) valid = 1;
   }

   if (!valid)    return epf("unsupported format", "Unsupported HDR format");

   // Parse width and height
   // can't use sscanf() if we're not using stdio!
   token = hdr_gettoken(s,buffer);
   if (strncmp(token, "-Y ", 3))  return epf("unsupported data layout", "Unsupported HDR format");
   token += 3;
   height = strtol(token, &token, 10);
   while (*token == ' ') ++token;
   if (strncmp(token, "+X ", 3))  return epf("unsupported data layout", "Unsupported HDR format");
   token += 3;
   width = strtol(token, NULL, 10);

   *x = width;
   *y = height;

   *comp = 3;
   if (req_comp == 0) req_comp = 3;

   // Read data
   hdr_data = (float *) malloc(height * width * req_comp * sizeof(float));

   // Load image data
   // image data is stored as some number of sca
   if ( width < 8 || width >= 32768) {
      // Read flat data
      for (j=0; j < height; ++j) {
         for (i=0; i < width; ++i) {
            stbi_uc rgbe[4];
           main_decode_loop:
            getn(s, rgbe, 4);
            hdr_convert(hdr_data + j * width * req_comp + i * req_comp, rgbe, req_comp);
         }
      }
   } else {
      // Read RLE-encoded data
      scanline = NULL;

      for (j = 0; j < height; ++j) {
         c1 = get8(s);
         c2 = get8(s);
         len = get8(s);
         if (c1 != 2 || c2 != 2 || (len & 0x80)) {
            // not run-length encoded, so we have to actually use THIS data as a decoded
            // pixel (note this can't be a valid pixel--one of RGB must be >= 128)
            uint8 rgbe[4];
            rgbe[0] = (uint8) c1;
            rgbe[1] = (uint8) c2;
            rgbe[2] = (uint8) len;
            rgbe[3] = (uint8) get8u(s);
            hdr_convert(hdr_data, rgbe, req_comp);
            i = 1;
            j = 0;
            free(scanline);
            goto main_decode_loop; // yes, this makes no sense
         }
         len <<= 8;
         len |= get8(s);
         if (len != width) { free(hdr_data); free(scanline); return epf("invalid decoded scanline length", "corrupt HDR"); }
         if (scanline == NULL) scanline = (stbi_uc *) malloc(width * 4);
            
         for (k = 0; k < 4; ++k) {
            i = 0;
            while (i < width) {
               count = get8u(s);
               if (count > 128) {
                  // Run
                  value = get8u(s);
                  count -= 128;
                  for (z = 0; z < count; ++z)
                     scanline[i++ * 4 + k] = value;
               } else {
                  // Dump
                  for (z = 0; z < count; ++z)
                     scanline[i++ * 4 + k] = get8u(s);
               }
            }
         }
         for (i=0; i < width; ++i)
            hdr_convert(hdr_data+(j*width + i)*req_comp, scanline + i*4, req_comp);
      }
      free(scanline);
   }

   return hdr_data;
}

static float *stbi_hdr_load(stbi *s, int *x, int *y, int *comp, int req_comp)
{
   return hdr_load(s,x,y,comp,req_comp);
}

static int stbi_hdr_info(stbi *s, int *x, int *y, int *comp)
{
   char buffer[HDR_BUFLEN];
   char *token;
   int valid = 0;

   if (strcmp(hdr_gettoken(s,buffer), "#?RADIANCE") != 0) {
       stbi_rewind( s );
       return 0;
   }

   for(;;) {
      token = hdr_gettoken(s,buffer);
      if (token[0] == 0) break;
      if (strcmp(token, "FORMAT=32-bit_rle_rgbe") == 0) valid = 1;
   }

   if (!valid) {
       stbi_rewind( s );
       return 0;
   }
   token = hdr_gettoken(s,buffer);
   if (strncmp(token, "-Y ", 3)) {
       stbi_rewind( s );
       return 0;
   }
   token += 3;
   *y = strtol(token, &token, 10);
   while (*token == ' ') ++token;
   if (strncmp(token, "+X ", 3)) {
       stbi_rewind( s );
       return 0;
   }
   token += 3;
   *x = strtol(token, NULL, 10);
   *comp = 3;
   return 1;
}
#endif // STBI_NO_HDR

static int stbi_bmp_info(stbi *s, int *x, int *y, int *comp)
{
   int hsz;
   if (get8(s) != 'B' || get8(s) != 'M') {
       stbi_rewind( s );
       return 0;
   }
   skip(s,12);
   hsz = get32le(s);
   if (hsz != 12 && hsz != 40 && hsz != 56 && hsz != 108) {
       stbi_rewind( s );
       return 0;
   }
   if (hsz == 12) {
      *x = get16le(s);
      *y = get16le(s);
   } else {
      *x = get32le(s);
      *y = get32le(s);
   }
   if (get16le(s) != 1) {
       stbi_rewind( s );
       return 0;
   }
   *comp = get16le(s) / 8;
   return 1;
}

static int stbi_psd_info(stbi *s, int *x, int *y, int *comp)
{
   int channelCount;
   if (get32(s) != 0x38425053) {
       stbi_rewind( s );
       return 0;
   }
   if (get16(s) != 1) {
       stbi_rewind( s );
       return 0;
   }
   skip(s, 6);
   channelCount = get16(s);
   if (channelCount < 0 || channelCount > 16) {
       stbi_rewind( s );
       return 0;
   }
   *y = get32(s);
   *x = get32(s);
   if (get16(s) != 8) {
       stbi_rewind( s );
       return 0;
   }
   if (get16(s) != 3) {
       stbi_rewind( s );
       return 0;
   }
   *comp = 4;
   return 1;
}

static int stbi_pic_info(stbi *s, int *x, int *y, int *comp)
{
   int act_comp=0,num_packets=0,chained;
   pic_packet_t packets[10];

   skip(s, 92);

   *x = get16(s);
   *y = get16(s);
   if (at_eof(s))  return 0;
   if ( (*x) != 0 && (1 << 28) / (*x) < (*y)) {
       stbi_rewind( s );
       return 0;
   }

   skip(s, 8);

   do {
      pic_packet_t *packet;

      if (num_packets==sizeof(packets)/sizeof(packets[0]))
         return 0;

      packet = &packets[num_packets++];
      chained = get8(s);
      packet->size    = get8u(s);
      packet->type    = get8u(s);
      packet->channel = get8u(s);
      act_comp |= packet->channel;

      if (at_eof(s)) {
          stbi_rewind( s );
          return 0;
      }
      if (packet->size != 8) {
          stbi_rewind( s );
          return 0;
      }
   } while (chained);

   *comp = (act_comp & 0x10 ? 4 : 3);

   return 1;
}

static int stbi_info_main(stbi *s, int *x, int *y, int *comp)
{
   if (stbi_jpeg_info(s, x, y, comp))
       return 1;
   if (stbi_png_info(s, x, y, comp))
       return 1;
   if (stbi_gif_info(s, x, y, comp))
       return 1;
   if (stbi_bmp_info(s, x, y, comp))
       return 1;
   if (stbi_psd_info(s, x, y, comp))
       return 1;
   if (stbi_pic_info(s, x, y, comp))
       return 1;
   #ifndef STBI_NO_HDR
   if (stbi_hdr_info(s, x, y, comp))
       return 1;
   #endif
   // test tga last because it's a crappy test!
   if (stbi_tga_info(s, x, y, comp))
       return 1;
   return e("unknown image type", "Image not of any known type, or corrupt");
}

#ifndef STBI_NO_STDIO
int stbi_info(char const *filename, int *x, int *y, int *comp)
{
    FILE *f = openFile(filename);
    int result;
    if (!f) return e("can't fopen", "Unable to open file");
    result = stbi_info_from_file(f, x, y, comp);
    fclose(f);
    return result;
}

int stbi_info_from_file(FILE *f, int *x, int *y, int *comp)
{
   int r;
   stbi s;
   long pos = ftell(f);
   start_file(&s, f);
   r = stbi_info_main(&s,x,y,comp);
   fseek(f,pos,SEEK_SET);
   return r;
}
#endif // !STBI_NO_STDIO

int stbi_info_from_memory(stbi_uc const *buffer, int len, int *x, int *y, int *comp)
{
   stbi s;
   start_mem(&s,buffer,len);
   return stbi_info_main(&s,x,y,comp);
}

int stbi_info_from_callbacks(stbi_io_callbacks const *c, void *user, int *x, int *y, int *comp)
{
   stbi s;
   start_callbacks(&s, (stbi_io_callbacks *) c, user);
   return stbi_info_main(&s,x,y,comp);
}

/*
   revision history:
      1.33 (2011-07-14)
             make stbi_is_hdr work in STBI_NO_HDR (as specified), minor compiler-friendly improvements
      1.32 (2011-07-13)
             support for "info" function for all supported filetypes (SpartanJ)
      1.31 (2011-06-20)
             a few more leak fixes, bug in PNG handling (SpartanJ)
      1.30 (2011-06-11)
             added ability to load files via callbacks to accomidate custom input streams (Ben Wenger)
             removed deprecated format-specific test/load functions
             removed support for installable file formats (stbi_loader) -- would have been broken for IO callbacks anyway
             error cases in bmp and tga give messages and don't leak (Raymond Barbiero, grisha)
             fix inefficiency in decoding 32-bit BMP (David Woo)
      1.29 (2010-08-16)
             various warning fixes from Aurelien Pocheville 
      1.28 (2010-08-01)
             fix bug in GIF palette transparency (SpartanJ)
      1.27 (2010-08-01)
             cast-to-uint8 to fix warnings
      1.26 (2010-07-24)
             fix bug in file buffering for PNG reported by SpartanJ
      1.25 (2010-07-17)
             refix trans_data warning (Won Chun)
      1.24 (2010-07-12)
             perf improvements reading from files on platforms with lock-heavy fgetc()
             minor perf improvements for jpeg
             deprecated type-specific functions so we'll get feedback if they're needed
             attempt to fix trans_data warning (Won Chun)
      1.23   fixed bug in iPhone support
      1.22 (2010-07-10)
             removed image *writing* support
             stbi_info support from Jetro Lauha
             GIF support from Jean-Marc Lienher
             iPhone PNG-extensions from James Brown
             warning-fixes from Nicolas Schulz and Janez Zemva (i.e. Janez (U+017D)emva)
      1.21   fix use of 'uint8' in header (reported by jon blow)
      1.20   added support for Softimage PIC, by Tom Seddon
      1.19   bug in interlaced PNG corruption check (found by ryg)
      1.18 2008-08-02
             fix a threading bug (local mutable static)
      1.17   support interlaced PNG
      1.16   major bugfix - convert_format converted one too many pixels
      1.15   initialize some fields for thread safety
      1.14   fix threadsafe conversion bug
             header-file-only version (#define STBI_HEADER_FILE_ONLY before including)
      1.13   threadsafe
      1.12   const qualifiers in the API
      1.11   Support installable IDCT, colorspace conversion routines
      1.10   Fixes for 64-bit (don't use "unsigned long")
             optimized upsampling by Fabian "ryg" Giesen
      1.09   Fix format-conversion for PSD code (bad global variables!)
      1.08   Thatcher Ulrich's PSD code integrated by Nicolas Schulz
      1.07   attempt to fix C++ warning/errors again
      1.06   attempt to fix C++ warning/errors again
      1.05   fix TGA loading to return correct *comp and use good luminance calc
      1.04   default float alpha is 1, not 255; use 'void *' for stbi_image_free
      1.03   bugfixes to STBI_NO_STDIO, STBI_NO_HDR
      1.02   support for (subset of) HDR files, float interface for preferred access to them
      1.01   fix bug: possible bug in handling right-side up bmps... not sure
             fix bug: the stbi_bmp_load() and stbi_tga_load() functions didn't work at all
      1.00   interface to zlib that skips zlib header
      0.99   correct handling of alpha in palette
      0.98   TGA loader by lonesock; dynamically add loaders (untested)
      0.97   jpeg errors on too large a file; also catch another malloc failure
      0.96   fix detection of invalid v value - particleman@mollyrocket forum
      0.95   during header scan, seek to markers in case of padding
      0.94   STBI_NO_STDIO to disable stdio usage; rename all #defines the same
      0.93   handle jpegtran output; verbose errors
      0.92   read 4,8,16,24,32-bit BMP files of several formats
      0.91   output 24-bit Windows 3.0 BMP files
      0.90   fix a few more warnings; bump version number to approach 1.0
      0.61   bugfixes due to Marc LeBlanc, Christopher Lloyd
      0.60   fix compiling as c++
      0.59   fix warnings: merge Dave Moore's -Wall fixes
      0.58   fix bug: zlib uncompressed mode len/nlen was wrong endian
      0.57   fix bug: jpg last huffman symbol before marker was >9 bits but less than 16 available
      0.56   fix bug: zlib uncompressed mode len vs. nlen
      0.55   fix bug: restart_interval not initialized to 0
      0.54   allow NULL for 'int *comp'
      0.53   fix bug in png 3->4; speedup png decoding
      0.52   png handles req_comp=3,4 directly; minor cleanup; jpeg comments
      0.51   obey req_comp requests, 1-component jpegs return as 1-component,
             on 'test' only check type, not whether we support this variant
      0.50   first released version
*/

/***********************************************************************
TEXTUREDATA.CPP
***********************************************************************/

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
					buffer.Add((unsigned char)Math::Clamp((pix.z * 255.0f), 0.0f, 255.0f));
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
			file.SetData(storageFormat, tex.Width, tex.Height, buffer.GetArrayView());
		}

		void CreateTextureDataFromBitmap(TextureData<Color> & tex, BitmapF & image)
		{
			tex.Levels.SetSize(CeilLog2(Math::Max(image.GetWidth(), image.GetHeight())) + 1);
			tex.Levels[0].Pixels.Reserve(image.GetWidth()*image.GetHeight());
			for (int i = image.GetHeight() - 1; i >= 0; i--)
			{
				for (int j = 0; j < image.GetWidth(); j++)
				{
					Color c;
					c.FromVec4(image.GetPixels()[i*image.GetWidth() + j]);
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
