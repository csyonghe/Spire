#include "GGX.h"
#include <math.h>
using namespace CoreLib::Basic;
using namespace VectorMath;

namespace CoreLib
{
	namespace Graphics
	{
		List<Vec2> ComputeTextureFV(float maxRoughness, int size)
		{
			List<Vec2> result;
			result.SetSize(size * size);
			for (int j = 0; j < size; j++)
			{
				float roughness = maxRoughness * ((float)j + 0.5f) / (float)(size);
				float alpha = roughness*roughness;
				for (int i = 0; i < size; i++)
				{
					float dotLH = ((float)i + 0.5f) / (float)(size);
					if (dotLH < 0.1f) dotLH = 0.1f;
					// F
					float F_a, F_b;
					float tmp = 1.0f - dotLH;
					float dotLH5 = (tmp*tmp) * (tmp*tmp) *tmp;
					F_a = 1.0f;
					F_b = dotLH5;

					// V
					float vis;
					float k = alpha / 2.0f;
					float k2 = k*k;
					float invK2 = 1.0f - k2;
					vis = 1.0f/(dotLH*dotLH*invK2 + k2);
					Vec2 fv;
					fv.x = F_a*vis;
					fv.y = F_b*vis;
					if (Math::IsNaN(fv.x))
						fv.x = 10000.0f;
					if (Math::IsNaN(fv.y))
						fv.y = 10000.0f;
					result[j*size + i] = fv;
				}
			}
			return result;
		}

		List<float> ComputeTextureD(float maxRoughness, int size)
		{
			List<float> result;
			result.SetSize(size * size);
			for (int j = 0; j < size; j++)
			{
				float roughness = maxRoughness * ((float)j + 0.5f) / (float)(size);
				float alpha = roughness*roughness;
				float alphaSqr = alpha*alpha;
				for (int i = 0; i < size; i++)
				{
					float dotNH = sqrt(sqrt(((float)i + 0.5f)/ (float)(size)));
				
					float pi = 3.14159f;
					float denom = dotNH * dotNH *(alphaSqr - 1.0f) + 1.0f;

					float D = alphaSqr / (pi * denom * denom);
					if (Math::IsNaN(D))
						D = 10000.0f;
					result[j*size + i] = D;
				}
			}
			return result;
		}
		TextureFile ComputeTextureFileFV(float maxRoughness, int size)
		{
			auto data = ComputeTextureFV(maxRoughness, size);
			TextureFile file;
			List<unsigned char> udata;
			udata.SetSize(size * size * 2);
			for (int i = 0; i < data.Count(); i++)
			{
				udata[i * 2] = (unsigned char)Math::Clamp((int)(data[i].x * 255.0f), 0, 255);
				udata[i * 2 + 1] = (unsigned char)Math::Clamp((int)(data[i].y * 255.0f), 0, 255);
			}
			file.SetData(TextureStorageFormat::RG_F32, size, size, 0, udata.GetArrayView());
			return file;
		}
		TextureFile ComputeTextureFileD(float maxRoughness, int size)
		{
			auto data = ComputeTextureD(maxRoughness, size);
			List<unsigned char> udata;
			udata.SetSize(data.Count() * sizeof(float));
			memcpy(udata.Buffer(), data.Buffer(), udata.Count());
			TextureFile file;
			file.SetData(TextureStorageFormat::R_F32, size, size, 0, udata.GetArrayView());
			return file;
		}
	}
}