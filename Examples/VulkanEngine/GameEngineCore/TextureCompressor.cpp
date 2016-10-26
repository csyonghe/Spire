#include "TextureCompressor.h"
#include "TextureTool/LibSquish.h"

namespace GameEngine
{
	using namespace CoreLib;
	using namespace CoreLib::Graphics;

	CoreLib::List<unsigned char> Resample(const CoreLib::List<unsigned char> &rgbaPixels, int w, int h, int & nw, int & nh)
	{
		nw = w / 2;
		nh = h / 2;
		if (nw < 1)
			nw = 1;
		if (nh < 1)
			nh = 1;
		CoreLib::List<unsigned char> rs;
		rs.SetSize(nh * nw * 4);
		for (int i = 0; i < nh; i++)
		{
			int i0 = Math::Clamp(i * 2, 0, h - 1);
			int i1 = Math::Clamp(i * 2 + 1, 0, h - 1);
			for (int j = 0; j < nw; j++)
			{
				int j0 = Math::Clamp(j * 2, 0, w - 1);
				int j1 = Math::Clamp(j * 2 + 1, 0, w - 1);
				for (int k = 0; k < 4; k++)
					rs[(i * nw + j) * 4 + k] = (rgbaPixels[(i0 * w + j0) * 4 + k] 
						+ rgbaPixels[(i0 * w + j1) * 4 + k] 
						+ rgbaPixels[(i1 * w + j0) * 4 + k] 
						+ rgbaPixels[(i1 * w + j1) * 4 + k]) / 4;
			}
		}
		return rs;
	}

	void TextureCompressor::CompressRGBA_BC1(TextureFile & result, const CoreLib::ArrayView<unsigned char> & rgbaPixels, int width, int height)
	{
		List<unsigned char> input;
		input.AddRange(rgbaPixels.Buffer(), rgbaPixels.Count());
		int w = width;
		int h = height;
		int level = 0;
		while (w >= 1 || h >= 1)
		{
			List<unsigned char> data;
			data.SetSize((int)(ceil(w / 4.0f) * ceil(h / 4.0f) * 8));
			squish::CompressImage(input.Buffer(), w, h, data.Buffer(), squish::kDxt1);
			result.SetData(TextureStorageFormat::BC1, w, h, level, data.GetArrayView());
			if (w == 1 && h == 1) break;
			int nw, nh;
			input = Resample(input, w, h, nw, nh);
			w = nw;
			h = nh;
			level++;
		}
		
	}

	void CompressBlockBC5(unsigned char * block, unsigned char * input)
	{
		unsigned char red0 = 255, red1 = 0, green0 = 255, green1 = 0;
		for (int i = 0; i < 16; i++)
		{
			auto red = input[i * 4];
			auto green = input[i * 4 + 1];
			if (red < red0) red0 = red;
			if (red > red1) red1 = red;
			if (green < green0) green0 = green;
			if (green > green1) green1 = green;
		}
		unsigned long long redWord = 0, greenWord = 0;
		for (int i = 0; i < 16; i++)
		{
			auto red = input[i * 4];
			auto green = input[i * 4 + 1];
			int redI = Math::FastFloor((red - red0) / (float)(red1 - red0) * 7.0f + 0.5f);
			if (redI == 7) redI = 1;
			else if (redI > 0) redI++;
			redWord |= (redI << (i * 3));
			int greenI = Math::FastFloor((green - green0) / (float)(green1 - green0) * 7.0f + 0.5f);
			if (greenI == 7) greenI = 1;
			else if (greenI > 0) greenI++;
			greenWord |= (greenI << (i * 3));
		}
		block[0] = red0;
		block[1] = red1;
		block[2] = redWord & 255;
		block[3] = (redWord >> 8) & 255;
		block[4] = (redWord >> 16) & 255;
		block[5] = (redWord >> 24) & 255;
		block[6] = (redWord >> 32) & 255;
		block[7] = (redWord >> 40) & 255;

		block[8] = green0;
		block[9] = green1;
		block[10] = greenWord & 255;
		block[11] = (greenWord >> 8) & 255;
		block[12] = (greenWord >> 16) & 255;
		block[13] = (greenWord >> 24) & 255;
		block[14] = (greenWord >> 32) & 255;
		block[15] = (greenWord >> 40) & 255;
	}
	
	void TextureCompressor::CompressRG_BC5(TextureFile & result, const CoreLib::ArrayView<unsigned char> & rgbaPixels, int width, int height)
	{
		List<unsigned char> input;
		input.AddRange(rgbaPixels.Buffer(), rgbaPixels.Count());
		int w = width;
		int h = height;
		int level = 0;
		while (w >= 1 || h >= 1)
		{
			List<unsigned char> data;
			data.SetSize((int)(ceil(w / 4.0f) * ceil(h / 4.0f) * 16));
			int ptr = 0;
			for (int i = 0; i < h; i += 4)
			{
				for (int j = 0; j < w; j += 4)
				{
					unsigned char block[64], outBlock[16];
					for (int ki = 0; ki < 4; ki++)
					{
						int ni = Math::Clamp(i + ki, 0, h - 1);
						for (int kj = 0; kj < 4; kj++)
						{
							int nj = Math::Clamp(j + kj, 0, w - 1);
							for (int c = 0; c < 4; c++)
								block[(ki * 4 + kj) * 4 + c] = input[(ni * h + nj) * 4 + c];
						}
					}
					CompressBlockBC5(outBlock, block);
					memcpy(data.Buffer() + ptr, outBlock, 16);
					ptr += 16;
				}
			}
			result.SetData(TextureStorageFormat::BC5, w, h, level, data.GetArrayView());
			if (w == 1 && h == 1) break;
			int nw, nh;
			input = Resample(input, w, h, nw, nh);
			w = nw;
			h = nh;
			level++;
		}
	}

}

