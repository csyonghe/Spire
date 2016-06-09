#ifndef REALTIME_ENGINE_WORLD_H
#define REALTIME_ENGINE_WORLD_H

#include "CoreLib/Basic.h"
#include "CoreLib/Graphics.h"
#include "DeviceResourcePool.h"

namespace DemoEngine
{
	using namespace VectorMath;

	class Background
	{
	private:
		GL::Program program;
		GL::Texture2D texture;
	public:
		Background(DeviceResourcePool * engine, String fileName);
		void Draw(DeviceResourcePool * engine, CoreLib::Graphics::ViewFrustum & sysUniforms);

	};

}

#endif