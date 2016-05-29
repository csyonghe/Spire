#ifndef REALTIME_ENGINE_WORLD_H
#define REALTIME_ENGINE_WORLD_H

#include "CoreLib/Basic.h"
#include "CoreLib/Graphics/BBox.h"
#include "CoreLib/VectorMath.h"
#include "DeviceResourcePool.h"
#include "CoreLib/Graphics/Camera.h"
#include "CoreLib/Graphics/ViewFrustum.h"

#include "CoreLib/Parser.h"
#include "CoreLib/LibMath.h"

namespace RealtimeEngine
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