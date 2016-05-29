#ifndef CORELIB_GRAPHICS_GGX_H
#define CORELIB_GRAPHICS_GGX_H

#include "../VectorMath.h"
#include "../LibMath.h"
#include "../List.h"
#include "TextureFile.h"

namespace CoreLib
{
	namespace Graphics
	{
		CoreLib::Basic::List<VectorMath::Vec2> ComputeTextureFV(float maxRoughness, int size);
		CoreLib::Basic::List<float> ComputeTextureD(float maxRoughness, int size);
		TextureFile ComputeTextureFileFV(float maxRoughness, int size);
		TextureFile ComputeTextureFileD(float maxRoughness, int size);
	}
}

#endif