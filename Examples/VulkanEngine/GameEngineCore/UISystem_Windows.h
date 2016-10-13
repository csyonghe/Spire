#ifndef GX_GLTEXT_H
#define GX_GLTEXT_H

#include "CoreLib/Basic.h"
#include "CoreLib/LibUI/LibUI.h"
#include "CoreLib/Imaging/Bitmap.h"
#include "CoreLib/WinForm/WinTimer.h"
#include "CoreLib/MemoryPool.h"
#include "HardwareRenderer.h"

namespace GraphicsUI
{
	class Font
	{
	public:
		CoreLib::String FontName;
		int Size;
		bool Bold, Underline, Italic, StrikeOut;
		Font()
		{
			NONCLIENTMETRICS NonClientMetrics;
			NonClientMetrics.cbSize = sizeof(NONCLIENTMETRICS) - sizeof(NonClientMetrics.iPaddedBorderWidth);
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &NonClientMetrics, 0);
			FontName = NonClientMetrics.lfMessageFont.lfFaceName;
			Size = 9;
			Bold = false;
			Underline = false;
			Italic = false;
			StrikeOut = false;
		}
		Font(const CoreLib::String& sname, int ssize)
		{
			FontName = sname;
			Size = ssize;
			Bold = false;
			Underline = false;
			Italic = false;
			StrikeOut = false;
		}
		Font(const CoreLib::String & sname, int ssize, bool sBold, bool sItalic, bool sUnderline)
		{
			FontName = sname;
			Size = ssize;
			Bold = sBold;
			Underline = sUnderline;
			Italic = sItalic;
			StrikeOut = false;
		}
		CoreLib::String ToString() const
		{
			CoreLib::StringBuilder sb;
			sb << FontName << Size << Bold << Underline << Italic << StrikeOut;
			return sb.ProduceString();
		}
	};

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

	class UIWindowsSystemInterface;

	class TextRasterizer
	{
	private:
		unsigned int TexID;
		DIBImage *Bit;
	public:
		TextRasterizer();
		~TextRasterizer();
		bool MultiLine = false;
		void SetFont(const Font & Font, int dpi);
		TextRasterizationResult RasterizeText(UIWindowsSystemInterface * system, const CoreLib::String & text);
		TextSize GetTextSize(const CoreLib::String & text);
	};


	class BakedText : public IBakedText
	{
	public:
		UIWindowsSystemInterface* system;
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


	class WindowsFont : public IFont
	{
	private:
		CoreLib::RefPtr<TextRasterizer> rasterizer;
		UIWindowsSystemInterface * system;
		GraphicsUI::Font fontDesc;
	public:
		WindowsFont(UIWindowsSystemInterface * ctx, int dpi, const GraphicsUI::Font & font)
		{
			system = ctx;
			fontDesc = font;
			rasterizer = new TextRasterizer();
			UpdateFontContext(dpi);
		}
		void UpdateFontContext(int dpi)
		{
			rasterizer->SetFont(fontDesc, dpi);
		}
		virtual Rect MeasureString(const CoreLib::String & text) override;
		virtual IBakedText * BakeString(const CoreLib::String & text) override;

	};

	class GLUIRenderer;

	class UIWindowsSystemInterface : public ISystemInterface
	{
	private:
		unsigned char * textBuffer = nullptr;
		CoreLib::Dictionary<CoreLib::String, CoreLib::RefPtr<WindowsFont>> fonts;
		CoreLib::RefPtr<GameEngine::Buffer> textBufferObj;
		CoreLib::MemoryPool textBufferPool;
		VectorMath::Vec4 ColorToVec(GraphicsUI::Color c);
		CoreLib::RefPtr<WindowsFont> defaultFont, titleFont, symbolFont;
		CoreLib::WinForm::Timer tmrHover, tmrTick;
		UIEntry * entry = nullptr;
		int GetCurrentDpi();
		void TickTimerTick(CoreLib::Object *, CoreLib::WinForm::EventArgs e);
		void HoverTimerTick(CoreLib::Object *, CoreLib::WinForm::EventArgs e);
	public:
		GLUIRenderer * uiRenderer;
		GameEngine::HardwareRenderer * rendererApi = nullptr;
		virtual void SetClipboardText(const CoreLib::String & text) override;
		virtual CoreLib::String GetClipboardText() override;
		virtual IFont * LoadDefaultFont(DefaultFontType dt = DefaultFontType::Content) override;
		virtual void SwitchCursor(CursorType c) override;
		void UpdateCompositionWindowPos(HIMC hIMC, int x, int y);
	public:
		UIWindowsSystemInterface(GameEngine::HardwareRenderer * ctx);
		~UIWindowsSystemInterface();
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
		GameEngine::Texture2D * GetRenderedImage();
		GameEngine::Buffer * GetTextBufferObject()
		{
			return textBufferObj.Ptr();
		}
		IFont * LoadFont(const Font & f);
		IImage * CreateImageObject(const CoreLib::Imaging::Bitmap & bmp);
		void SetResolution(int w, int h);
		void ExecuteDrawCommands(GameEngine::Texture2D* baseTexture, CoreLib::List<DrawCommand> & commands);
		void SetEntry(UIEntry * pentry);
		int HandleSystemMessage(HWND hWnd, UINT message, WPARAM &wParam, LPARAM &lParam);
	};
}

#endif