#ifndef PTEX_MIPMAP_BUILDER_H
#define PTEX_MIPMAP_BUILDER_H

#include "DeviceResourcePool.h"

namespace RealtimeEngine
{
	class PTexMipmapBuilder
	{
	private:
		DeviceResourcePool * engine;
		GL::FrameBuffer fbo;
		List<GL::RenderBuffer> renderBuffer;
		List<GL::Texture2D> tmpTex;
		GL::Program dilateProgram, downSampleProgram;
	public:
		void Init(DeviceResourcePool * pEngine);
		void Free();
		void BuildMimap(GL::Texture2D tex);
	};
}

#endif