#ifndef RAY_TRACE_PRO_TEXTURE_DATA_H
#define RAY_TRACE_PRO_TEXTURE_DATA_H

#include "../Basic.h"
#include "../VectorMath.h"
#include "../LibMath.h"
#include "Bitmap.h"
#include "../Graphics/TextureFile.h"
#include <math.h>

#define TEXTURE_ACCESS_DUMP

#ifdef TEXTURE_ACCESS_DUMP
#include "../LibIO.h"
using namespace CoreLib::IO;
extern bool EnableTextureAccessDump;
extern TextWriter * TextureAccessDebugWriter;
#endif

#ifdef _MSC_VER
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline
#endif

namespace CoreLib
{
	namespace Imaging
	{
		using namespace VectorMath;
		struct Color
		{
			union
			{
				struct
				{
					unsigned char R, G, B, A;
#pragma warning(suppress : 4201)
				};
				int Value;
			};
			Color()
			{}
			Color(int r, int g, int b, int a)
			{
				R = (unsigned char)r;
				G = (unsigned char)g;
				B = (unsigned char)b;
				A = (unsigned char)a;
			}
			Vec4 ToVec4()
			{
				Vec4 result;
				static const float inv255 = 1.0f / 255.0f;
				result.x = R * inv255;
				result.y = G * inv255;
				result.z = B * inv255;
				result.w = A * inv255;
				return result;
			}
			void FromVec4(const Vec4 & color)
			{
				R = (unsigned char)Math::Clamp(Math::FastFloor(color.x * 255.0f), 0, 255);
				G = (unsigned char)Math::Clamp(Math::FastFloor(color.y * 255.0f), 0, 255);
				B = (unsigned char)Math::Clamp(Math::FastFloor(color.z * 255.0f), 0, 255);
				A = (unsigned char)Math::Clamp(Math::FastFloor(color.w * 255.0f), 0, 255);
			}
			static FORCE_INLINE Color Lerp(const Color & c1, const Color & c2, float t, float invT)
			{
				Color result;
				result.R = (unsigned char)(c1.R * invT + c2.R * t);
				result.G = (unsigned char)(c1.G * invT + c2.G * t);
				result.B = (unsigned char)(c1.B * invT + c2.B * t);
				result.A = (unsigned char)(c1.A * invT + c2.A * t);
				return result;
			}
			static FORCE_INLINE Color DownSample(const Color & c1, const Color & c2, const Color & c3, const Color & c4)
			{
				Color result;
				result.R = (c1.R + c2.R + c3.R + c4.R) >> 2;
				result.G = (c1.G + c2.G + c3.G + c4.G) >> 2;
				result.B = (c1.B + c2.B + c3.B + c4.B) >> 2;
				result.A = (c1.A + c2.A + c3.A + c4.A) >> 2;
				return result;
			}
		};

		typedef CoreLib::Imaging::Color Color4ub;

		struct Color1F
		{
			float x;
			Color1F()
			{}
			Color1F(float val)
			{
				x = val;
			}
			Vec4 ToVec4()
			{
				Vec4 result;
				result.x = x;
				result.y = x;
				result.z = x;
				result.w = x;
				return result;
			}
			static FORCE_INLINE Color1F Lerp(const Color1F & c1, const Color1F & c2, float t, float invT)
			{
				Color1F result;
				result.x = (c1.x * invT + c2.x * t);
				return result;
			}
			static FORCE_INLINE Color1F DownSample(const Color1F & c1, const Color1F & c2, const Color1F & c3, const Color1F & c4)
			{
				Color1F result;
				result.x = (c1.x + c2.x + c3.x + c4.x) * 0.25f;
				return result;
			}
		};

		struct Color4F
		{
			float x, y, z, w;
			Color4F()
			{}
			Color4F(float val)
			{
				x = val;
			}
			Color4F(Vec4 v)
			{
				x = v.x;
				y = v.y;
				z = v.z;
				w = v.w;
			}
			Vec4 ToVec4()
			{
				Vec4 result;
				result.x = x;
				result.y = y;
				result.z = z;
				result.w = w;
				return result;
			}
			static FORCE_INLINE Color4F Lerp(const Color4F & c1, const Color4F & c2, float t, float invT)
			{
				Color4F result;
				result.x = (c1.x * invT + c2.x * t);
				result.y = (c1.y * invT + c2.y * t);
				result.z = (c1.z * invT + c2.z * t);
				result.w = (c1.w * invT + c2.w * t);
				return result;
			}
			static FORCE_INLINE Color4F DownSample(const Color4F & c1, const Color4F & c2, const Color4F & c3, const Color4F & c4)
			{
				Color4F result;
				result.x = (c1.x + c2.x + c3.x + c4.x) * 0.25f;
				result.y = (c1.y + c2.y + c3.y + c4.y) * 0.25f;
				result.z = (c1.z + c2.z + c3.z + c4.z) * 0.25f;
				result.w = (c1.w + c2.w + c3.w + c4.w) * 0.25f;
				return result;
			}
		};

		template<typename ColorType>
		struct TextureLevel
		{
			Basic::List<ColorType> Pixels;
			int Width, Height;
		};

		inline int CeilLog2(int val)
		{
			int rs = 0;
			while (val)
			{
				val >>= 1;
				rs++;
			}
			return rs;
		}

		template<typename ColorType>
		class TextureData : public Object
		{
		public:
			int RefCount;
			Basic::String FileName;
			int Width, Height;
			float InvWidth, InvHeight;
			bool IsTransparent;
			Basic::List<TextureLevel<ColorType>> Levels;

			void GenerateMipmaps()
			{
				Width = Levels[0].Width;
				Height = Levels[0].Height;
				InvWidth = 1.0f / Width;
				InvHeight = 1.0f / Height;
				// Generate mipmaps
				Levels.SetSize(CeilLog2(Math::Max(Levels[0].Width, Levels[0].Height)));
				int level = 0;
				int lwidth = Width, lheight = Height;
				do
				{
					int oldWidth = lwidth;
					int oldHeight = lheight;
					int oldLevel = level;
					level++;
					lwidth >>= 1;
					lheight >>= 1;
					if (lwidth == 0) lwidth = 1;
					if (lheight == 0) lheight = 1;
					Levels[level].Width = lwidth;
					Levels[level].Height = lheight;
					Levels[level].Pixels.SetSize(lwidth * lheight);
					for (int i = 0; i < lheight; i++)
					{
						int i1, i2;
						if (lheight < oldHeight)
						{
							i1 = i * 2;
							i2 = i1 + 1;
						}
						else
						{

							i1 = i2 = i;
						}
						for (int j = 0; j < lwidth; j++)
						{
							int j1, j2;
							if (lwidth < oldWidth)
							{
								j1 = j * 2;
								j2 = j1 + 1;
							}
							else
							{
								j1 = j2 = j;
							}
							ColorType c1, c2, c3, c4;
							c1 = Levels[oldLevel].Pixels[i1*oldWidth + j1];
							c2 = Levels[oldLevel].Pixels[i1*oldWidth + j2];
							c3 = Levels[oldLevel].Pixels[i2*oldWidth + j1];
							c4 = Levels[oldLevel].Pixels[i2*oldWidth + j2];
							Levels[level].Pixels[i*lwidth + j] = ColorType::DownSample(c1, c2, c3, c4);
						}
					}
				} while (lwidth != 1 || lheight != 1);
			}
		};

		void CreateDefaultTextureData(TextureData<Color> & tex);
		void CreateTextureDataFromBitmap(TextureData<Color4F> & tex, BitmapF & image);
		void CreateTextureDataFromTextureFile(TextureData<Color4F> & tex, CoreLib::Graphics::TextureFile & texFile);
		void CreateTextureFile(CoreLib::Graphics::TextureFile & file, CoreLib::Graphics::TextureStorageFormat storageFormat, TextureData<Color4F> & tex);
		void CreateTextureDataFromBitmap(TextureData<Color> & tex, Bitmap & image);
		void CreateTextureDataFromFile(TextureData<Color> & tex, const Basic::String & fileName);

		template<typename ColorType>
		class Cubemap
		{
		public:
			RefPtr<TextureData<ColorType>> Maps[6]; // {+x, -x, +y, -y, +z, -z}
			Matrix4 Transforms[6];
		};

		enum class TextureWrapMode
		{
			Clamp, Repeat, Mirror
		};

		inline void WrapCoords(int & i0, int & j0, int width, int height, TextureWrapMode wrap)
		{
			if (i0 < 0) i0 = -i0;
			if (j0 < 0) j0 = -j0;
			switch (wrap)
			{
			case CoreLib::Imaging::TextureWrapMode::Clamp:
				i0 = Math::Clamp(i0, 0, width - 1);
				j0 = Math::Clamp(j0, 0, height - 1);
				break;
			case CoreLib::Imaging::TextureWrapMode::Repeat:
				i0 = i0 % width;
				j0 = j0 % height;
				break;
			case CoreLib::Imaging::TextureWrapMode::Mirror:
				i0 = i0 % (width << 1);
				if (i0 >= width)
					i0 = (width << 1) - i0 - 1;
				j0 = j0 % (height << 1);
				if (j0 >= height)
					j0 = (height << 1) - j0 - 1;
				break;
			default:
				break;
			}
		}

		inline void WrapCoords3D(int & i0, int & j0, int & k0, int width, int height, int depth, TextureWrapMode wrap)
		{
			if (i0 < 0) i0 = -i0;
			if (j0 < 0) j0 = -j0;
			if (k0 < 0) k0 = -k0;
			switch (wrap)
			{
			case CoreLib::Imaging::TextureWrapMode::Clamp:
				i0 = Math::Clamp(i0, 0, width - 1);
				j0 = Math::Clamp(j0, 0, height - 1);
				k0 = Math::Clamp(k0, 0, depth - 1);
				break;
			case CoreLib::Imaging::TextureWrapMode::Repeat:
				i0 = i0 % width;
				j0 = j0 % height;
				k0 = k0 % depth;
				break;
			case CoreLib::Imaging::TextureWrapMode::Mirror:
				i0 = i0 % (width << 1);
				if (i0 >= width)
					i0 = (width << 1) - i0 - 1;
				j0 = j0 % (height << 1);
				if (j0 >= height)
					j0 = (height << 1) - j0 - 1;
				k0 = k0 % (depth << 1);
				if (k0 >= depth)
					k0 = (depth << 1) - k0 - 1;
				break;
			default:
				break;
			}
		}

		inline void WrapCoords(int & i0, int & j0, int & i1, int & j1, int width, int height, TextureWrapMode wrap)
		{
			if (i0 < 0) i0 = -i0;
			if (i1 < 0) i1 = -i1;
			if (j0 < 0) j0 = -j0;
			if (j1 < 0) j1 = -j1;
			switch (wrap)
			{
			case CoreLib::Imaging::TextureWrapMode::Clamp:
				i0 = Math::Clamp(i0, 0, width - 1);
				j0 = Math::Clamp(j0, 0, height - 1);
				i1 = Math::Clamp(i1, 0, width - 1);
				j1 = Math::Clamp(j1, 0, height - 1);
				break;
			case CoreLib::Imaging::TextureWrapMode::Repeat:
				i0 = i0 % width;
				j0 = j0 % height;
				i1 = i1 % width;
				j1 = j1 % height;
				break;
			case CoreLib::Imaging::TextureWrapMode::Mirror:
				i0 = i0 % (width << 1);
				if (i0 >= width)
					i0 = (width << 1) - i0 - 1;
				j0 = j0 % (height << 1);
				if (j0 >= height)
					j0 = (height << 1) - j0 - 1;

				i1 = i1 % (width << 1);
				if (i1 >= width)
					i1 = (width << 1) - i1 - 1;
				j1 = j1 % (height << 1);
				if (j1 >= height)
					j1 = (height << 1) - j1 - 1;
				break;
			default:
				break;
			}
		}

		inline void WrapCoords3D(int & i0, int & j0, int & k0, int & i1, int & j1, int & k1, int width, int height, int depth, TextureWrapMode wrap)
		{
			if (i0 < 0) i0 = -i0;
			if (i1 < 0) i1 = -i1;
			if (j0 < 0) j0 = -j0;
			if (j1 < 0) j1 = -j1;
			switch (wrap)
			{
			case CoreLib::Imaging::TextureWrapMode::Clamp:
				i0 = Math::Clamp(i0, 0, width - 1);
				j0 = Math::Clamp(j0, 0, height - 1);
				k0 = Math::Clamp(k0, 0, depth - 1);
				i1 = Math::Clamp(i1, 0, width - 1);
				j1 = Math::Clamp(j1, 0, height - 1);
				k1 = Math::Clamp(k1, 0, depth - 1);
				break;
			case CoreLib::Imaging::TextureWrapMode::Repeat:
				i0 = i0 % width;
				j0 = j0 % height;
				k0 = k0 % depth;
				i1 = i1 % width;
				j1 = j1 % height;
				k1 = k1 % depth;
				break;
			case CoreLib::Imaging::TextureWrapMode::Mirror:
				i0 = i0 % (width << 1);
				if (i0 >= width)
					i0 = (width << 1) - i0 - 1;
				j0 = j0 % (height << 1);
				if (j0 >= height)
					j0 = (height << 1) - j0 - 1;
				k0 = k0 % (depth << 1);
				if (k0 >= depth)
					k0 = (depth << 1) - k0 - 1;

				i1 = i1 % (width << 1);
				if (i1 >= width)
					i1 = (width << 1) - i1 - 1;
				j1 = j1 % (height << 1);
				if (j1 >= height)
					j1 = (height << 1) - j1 - 1;
				k1 = k1 % (depth << 1);
				if (k1 >= depth)
					k1 = (depth << 1) - k1 - 1;
				break;
			default:
				break;
			}
		}

		template<typename ColorType>
		inline void SampleTextureLevel_Neareast(VectorMath::Vec4 * result, TextureData<ColorType> * texture, int lod, VectorMath::Vec2 & _uv, TextureWrapMode wrap = TextureWrapMode::Repeat)
		{
			Vec2 uv = _uv;
			auto & level = texture->Levels[lod];
			uv.x *= level.Width;
			uv.y *= level.Height;
			uv.x -= 0.5f;
			uv.y -= 0.5f;
			int i0 = Math::FastFloor(uv.x);
			int j0 = Math::FastFloor(uv.y);
			WrapCoords(i0, j0, level.Width, level.Height, wrap);
			*result = level.Pixels[j0 * level.Width + i0].ToVec4();
		}

		template<typename ColorType>
		FORCE_INLINE void SampleTextureLevel(VectorMath::Vec4 * result, TextureData<ColorType> * texture, int lod, VectorMath::Vec2 & _uv, TextureWrapMode wrap = TextureWrapMode::Repeat)
		{
			Vec2 uv = _uv;
			auto & level = texture->Levels[lod];
			uv.x *= level.Width;
			uv.y *= level.Height;
			uv.x -= 0.5f;
			uv.y -= 0.5f;
			int i0 = Math::FastFloor(uv.x);
			int i1 = i0 + 1;
			int j0 = Math::FastFloor(uv.y);
			int j1 = j0 + 1;

			float it = uv.x - i0;
			float jt = uv.y - j0;

			WrapCoords(i0, j0, i1, j1, level.Width, level.Height, wrap);
			ColorType c1,c2,c3,c4;
			c1 = level.Pixels[j0 * level.Width + i0];
			c2 = level.Pixels[j0 * level.Width + i1];
			c3 = level.Pixels[j1 * level.Width + i0];
			c4 = level.Pixels[j1 * level.Width + i1];
#ifdef TEXTURE_ACCESS_DUMP
			if (EnableTextureAccessDump)
			{
				(*TextureAccessDebugWriter) << L"tex " << String((int)level.Pixels.Buffer(), 16) << L" " << i0 << L" " << j0 << L" " << i1 << L" " << j1 << L"\n";
			}
#endif
			ColorType ci0, ci1;
			
			float invIt = 1.0f - it;
			float invJt = 1.0f - jt;
			ci0 = ColorType::Lerp(c1, c2, it, invIt);
			ci1 = ColorType::Lerp(c3, c4, it, invIt);
			ColorType c = ColorType::Lerp(ci0, ci1, jt, invJt);
			*result = c.ToVec4();
		}


		template<typename ColorType>
		inline void NeareastSampling(Vec4 * result, TextureData<ColorType> * texture, Vec2 uv)
		{
			if (_finite(uv.x) && _finite(uv.y))
				SampleTextureLevel_Neareast(result, texture, 0, uv);
		}

		template<typename ColorType>
		inline void LinearSampling(Vec4 * result, TextureData<ColorType> * texture, Vec2 uv)
		{
			if (_finite(uv.x) && _finite(uv.y))
				SampleTextureLevel(result, texture, 0, uv);
		}

		inline float fast_log2(float val)
		{
			int * const    exp_ptr = reinterpret_cast <int *> (&val);
			int            x = *exp_ptr;
			const int      log_2 = ((x >> 23) & 255) - 128;
			x &= ~(255 << 23);
			x += 127 << 23;
			*exp_ptr = x;

			val = ((-1.0f / 3) * val + 2) * val - 2.0f / 3;   // (1)

			return (val + log_2);
		}

		template<typename ColorType>
		inline void TrilinearSampling(Vec4 * result, TextureData<ColorType> * texture, float du, float dv, Vec2 & uv, int minLod = 0)
		{
			if (_finite(du) && _finite(dv) && _finite(uv.x) && _finite(uv.y))
			{
				du *= texture->Width;
				dv *= texture->Height;
				float maxDudv = Basic::Math::Max(du, dv);
				float lod;
				int lod1, lod2;
				if (maxDudv < 0.0001f)
				{
					lod = 0.0f;
					lod1 = lod2 = 0;
				}
				else
				{
					lod = Basic::Math::Max(0.0f, fast_log2(maxDudv));
					lod1 = Basic::Math::Min((int)lod, texture->Levels.Count() - 1);
					lod2 = (lod == lod1 ? lod1 : Basic::Math::Min(lod1 + 1, texture->Levels.Count() - 1));
				}
				lod1 = Basic::Math::Max(lod1, minLod);
				lod2 = Basic::Math::Max(lod2, minLod);
				if (lod1 == lod2)
				{
					SampleTextureLevel(result, texture, lod1, uv);
				}
				else
				{
					float lodt = lod - lod1;
					float invLodt = 1.0f - lodt;
					Vec4 v1, v2;
					SampleTextureLevel(&v1, texture, lod1, uv);
					SampleTextureLevel(&v2, texture, lod2, uv);
					result->x = v1.x * invLodt + v2.x * lodt;
					result->y = v1.y * invLodt + v2.y * lodt;
					result->z = v1.z * invLodt + v2.z * lodt;
					result->w = v1.w * invLodt + v2.w * lodt;
				}
			}
			else
			{
				result->x = 0.0f;
				result->y = 0.0f;
				result->z = 0.0f;
				result->w = 0.0f;
			}
		}

		template<typename ColorType>
		inline void AnisotropicSampling(Vec4 * result, TextureData<ColorType> * texture, int maxRate, float dudx, float dvdx, float dudy, float dvdy, Vec2 & uv)
		{
			float A = dvdx * dvdx + dvdy * dvdy;
			float B = -2.0f * (dudx * dvdx + dudy * dvdy);
			float C = dudx * dudx + dudy * dudy;
			float F = (dudx * dvdy - dudy * dvdx);
			F *= F;
			float p = A - C;
			float q = A + C;
			float t = sqrt(p * p + B * B);

			dudx *= texture->Width; dudy *= texture->Width;
			dvdx *= texture->Height; dvdy *= texture->Height;

			float squaredLengthX = dudx*dudx + dvdx*dvdx;
			float squaredLengthY = dudy*dudy + dvdy*dvdy;
			float determinant = abs(dudx*dvdy - dvdx*dudy);
			bool isMajorX = squaredLengthX > squaredLengthY;
			float squaredLengthMajor = isMajorX ? squaredLengthX : squaredLengthY;
			float lengthMajor = sqrt(squaredLengthMajor);
			float normMajor = 1.f / lengthMajor;

			Vec2 anisoDir;
			anisoDir.x = (isMajorX ? dudx : dudy) * normMajor;
			anisoDir.y = (isMajorX ? dvdx : dvdy) * normMajor;

			float ratioOfAnisotropy = squaredLengthMajor / determinant;

				// clamp ratio and compute LOD
			float lengthMinor;
			if (ratioOfAnisotropy > maxRate) // maxAniso comes from a Sampler state.
			{
				// ratio is clamped - LOD is based on ratio (preserves area)
				ratioOfAnisotropy = (float)maxRate;
				lengthMinor = lengthMajor / ratioOfAnisotropy;
			}
			else
			{
				// ratio not clamped - LOD is based on area
				lengthMinor = determinant / lengthMajor;
			}

			// clamp to top LOD
			if (lengthMinor < 1.0f)
			{
				ratioOfAnisotropy = Math::Max(1.0f, ratioOfAnisotropy*lengthMinor);
				lengthMinor = 1.0f; 
			}

			float LOD = log(lengthMinor)* 1.442695f;
			float invRate = 1.0f / (int)ratioOfAnisotropy;
			float startU = uv.x * texture->Width - lengthMajor*anisoDir.x*0.5f;
			float startV = uv.y * texture->Height - lengthMajor*anisoDir.y*0.5f;
			float stepU = lengthMajor*anisoDir.x*invRate;
			float stepV = lengthMajor*anisoDir.y*invRate;
			result->SetZero();
			int lod1, lod2;
			lod1 = Basic::Math::Min((int)LOD, texture->Levels.Count() - 1);
			lod2 = Basic::Math::Min((int)ceil(LOD), texture->Levels.Count() - 1);
			float lodt = LOD - lod1;
			float invLodt = 1.0f - lodt;
			for (int i = 0; i<(int)ratioOfAnisotropy; i++)
			{
				uv.x = (startU + stepU * (i + 0.5f)) * texture->InvWidth;
				uv.y = (startV + stepV * (i + 0.5f)) * texture->InvHeight;
				if (lod1 == lod2)
				{
					Vec4 rs;
					SampleTextureLevel(&rs, texture, lod1, uv);
					(*result) += rs;
				}
				else
				{
					Vec4 v1, v2;
					SampleTextureLevel(&v1, texture, lod1, uv);
					SampleTextureLevel(&v2, texture, lod2, uv);
					result->x += v1.x * invLodt + v2.x * lodt;
					result->y += v1.y * invLodt + v2.y * lodt;
					result->z += v1.z * invLodt + v2.z * lodt;
					result->w += v1.w * invLodt + v2.w * lodt;
				}
			}
			(*result) *= invRate;
		}
	}
}

#endif
