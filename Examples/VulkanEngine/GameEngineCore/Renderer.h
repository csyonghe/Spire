#ifndef RENDERER_H
#define RENDERER_H

#include "Common.h"
#include "Level.h"

namespace GameEngine
{
	enum class RenderAPI
	{
		OpenGL, Vulkan
	};

	class Texture2D;
	class HardwareRenderer;

	class Renderer : public CoreLib::Object
	{
	public:
		virtual void InitializeLevel(Level * level) = 0;
		virtual void TakeSnapshot() = 0;
		virtual void RenderFrame() = 0;
		virtual void DestroyContext() = 0;
		virtual void Resize(int w, int h) = 0;
		virtual void Wait() = 0;
		virtual Texture2D * GetRenderedImage() = 0;
		virtual HardwareRenderer * GetHardwareRenderer() = 0;
		
	};

	Renderer* CreateRenderer(WindowHandle window, RenderAPI api);
}

#endif