#ifndef RENDER_PASS_H
#define RENDER_PASS_H

#include "Common.h"
#include "HardwareRenderer.h"

namespace GameEngine
{
	class SharedRenderResource
	{
	public:
		Texture2D * RequestFrameTexture(StorageFormat format, float resolutionRate);
		Texture2D * OutputFrameTexture(StorageFormat format, float resolutionRate);
	};
}

#endif