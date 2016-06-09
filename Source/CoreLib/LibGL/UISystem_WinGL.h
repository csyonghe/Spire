#ifndef GX_GLTEXT_H
#define GX_GLTEXT_H

#include "../Basic.h"
#include "../Graphics/LibUI.h"
#include "../Imaging/Bitmap.h"
#include "OpenGLHardwareRenderer.h"

namespace GraphicsUI
{
	class Font;
	class UISystemInterface : public ISystemInterface
	{
	public:
		virtual IFont * CreateFont(const GraphicsUI::Font & f) = 0;
		virtual IImage * CreateImage(const CoreLib::Imaging::Bitmap & bmp) = 0;
		virtual void SetResolution(int w, int h) = 0;
		virtual void BeginUIDrawing() = 0;
		virtual void EndUIDrawing() = 0;
	};
	UISystemInterface * CreateWinGLInterface(GL::HardwareRenderer * glContext);
}

#endif