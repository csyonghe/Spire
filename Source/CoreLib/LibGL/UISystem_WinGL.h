#ifndef GX_GLTEXT_H
#define GX_GLTEXT_H

#include "../Basic.h"
#include "../Graphics/LibUI.h"
#include "../Imaging/Bitmap.h"
#include "../WinForm/WinTimer.h"
#include "OpenGLHardwareRenderer.h"

namespace GraphicsUI
{
	class Font;

	class DIBImage;

	struct TextSize
	{
		int x, y;
	};

	class TextRasterizationResult
	{
	public:
		TextSize Size;
		CoreLib::List<unsigned char> Image;
	};

	class TextRasterizer
	{
	private:
		unsigned int TexID;
		DIBImage *Bit;
	public:
		TextRasterizer();
		~TextRasterizer();
		bool MultiLine = false;
		void SetFont(const Font & Font);
		TextRasterizationResult RasterizeText(const CoreLib::String & text, int w = 0);
		TextSize GetTextSize(const CoreLib::String & text);
	};

	class BakedText : public IBakedText
	{
	public:
		GL::HardwareRenderer * glContext;
		GL::Texture2D texture;
		int Width, Height;
		virtual int GetWidth() override
		{
			return Width;
		}
		virtual int GetHeight() override
		{
			return Height;
		}
		~BakedText()
		{
			if (texture.Handle)
				glContext->DestroyTexture(texture);
		}
	};

	class WinGLFont : public IFont
	{
	private:
		TextRasterizer rasterizer;
		GL::HardwareRenderer * glContext;
	public:
		WinGLFont(GL::HardwareRenderer * ctx, const GraphicsUI::Font & font)
		{
			glContext = ctx;
			rasterizer.SetFont(font);
		}
		virtual Rect MeasureString(const CoreLib::String & text, int /*width*/) override;
		virtual IBakedText * BakeString(const CoreLib::String & text, int width) override;

	};

	class GLUIRenderer;

	class WinGLSystemInterface : public ISystemInterface
	{
	private:
		VectorMath::Vec4 ColorToVec(GraphicsUI::Color c);
		CoreLib::RefPtr<WinGLFont> defaultFont, titleFont, symbolFont;
		CoreLib::WinForm::Timer tmrHover, tmrTick;
		UIEntry * entry = nullptr;
		void TickTimerTick(CoreLib::Object *, CoreLib::WinForm::EventArgs e);
		void HoverTimerTick(CoreLib::Object *, CoreLib::WinForm::EventArgs e);
	public:
		GLUIRenderer * uiRenderer;
		GL::HardwareRenderer * glContext = nullptr;
		virtual void SetClipboardText(const CoreLib::String & text) override;
		virtual CoreLib::String GetClipboardText() override;
		virtual IFont * LoadDefaultFont(DefaultFontType dt = DefaultFontType::Content) override;
		virtual void SwitchCursor(CursorType c) override;
	public:
		WinGLSystemInterface(GL::HardwareRenderer * ctx);
		~WinGLSystemInterface();
		IFont * CreateFontObject(const Font & f);
		IImage * CreateImageObject(const CoreLib::Imaging::Bitmap & bmp);
		void SetResolution(int w, int h);
		void ExecuteDrawCommands(CoreLib::List<DrawCommand> & commands);
		void SetEntry(UIEntry * pentry);
		int HandleSystemMessage(HWND hWnd, UINT message, WPARAM &wParam, LPARAM &lParam);
	};
}

#endif