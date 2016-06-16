#ifndef GX_GLTEXT_H
#define GX_GLTEXT_H

#include "../Basic.h"
#include "../Graphics/LibUI.h"
#include "../Imaging/Bitmap.h"
#include "../WinForm/WinTimer.h"
#include "../MemoryPool.h"
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
		int BufferSize;
		unsigned char * ImageData;
	};

	class WinGLSystemInterface;

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
		TextRasterizationResult RasterizeText(WinGLSystemInterface * system, const CoreLib::String & text, int w = 0);
		TextSize GetTextSize(const CoreLib::String & text);
	};


	class BakedText : public IBakedText
	{
	public:
		WinGLSystemInterface* system;
		unsigned char * textBuffer;
		int BufferSize;
		int Width, Height;
		virtual int GetWidth() override
		{
			return Width;
		}
		virtual int GetHeight() override
		{
			return Height;
		}
		~BakedText();
	};


	class WinGLFont : public IFont
	{
	private:
		TextRasterizer rasterizer;
		WinGLSystemInterface * system;
	public:
		WinGLFont(WinGLSystemInterface * ctx, const GraphicsUI::Font & font)
		{
			system = ctx;
			rasterizer.SetFont(font);
		}
		virtual Rect MeasureString(const CoreLib::String & text, int /*width*/) override;
		virtual IBakedText * BakeString(const CoreLib::String & text, int width) override;

	};

	class GLUIRenderer;

	class WinGLSystemInterface : public ISystemInterface
	{
	private:
		unsigned char * textBuffer = nullptr;
		GL::BufferObject textBufferObj;
		CoreLib::MemoryPool textBufferPool;
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
		unsigned char * AllocTextBuffer(int size)
		{
			return textBufferPool.Alloc(size);
		}
		void FreeTextBuffer(unsigned char * buffer, int size)
		{
			textBufferPool.Free(buffer, size);
		}
		int GetTextBufferRelativeAddress(unsigned char * buffer)
		{
			return (int)(buffer - textBuffer);
		}
		GL::BufferObject GetTextBufferObject()
		{
			return textBufferObj;
		}
		IFont * CreateFontObject(const Font & f);
		IImage * CreateImageObject(const CoreLib::Imaging::Bitmap & bmp);
		void SetResolution(int w, int h);
		void ExecuteDrawCommands(CoreLib::List<DrawCommand> & commands);
		void SetEntry(UIEntry * pentry);
		int HandleSystemMessage(HWND hWnd, UINT message, WPARAM &wParam, LPARAM &lParam);
	};
}

#endif