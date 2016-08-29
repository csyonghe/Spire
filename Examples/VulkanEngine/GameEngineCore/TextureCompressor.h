#ifndef TEXTURE_COMPRESSOR_H
#define TEXTURE_COMPRESSOR_H

#include "CoreLib/Basic.h"
#include "CoreLib/Graphics/TextureFile.h"

namespace GameEngine
{
	class TextureCompressor
	{
	public:
		static void CompressRGBA_BC1(CoreLib::Graphics::TextureFile & result, const CoreLib::ArrayView<unsigned char> & rgbaPixels, int width, int height);
		static void CompressRG_BC5(CoreLib::Graphics::TextureFile & result, const CoreLib::ArrayView<unsigned char> & rgbaPixels, int width, int height);
	};
}

#endif