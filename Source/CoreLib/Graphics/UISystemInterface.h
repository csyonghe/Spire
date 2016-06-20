#ifndef GX_UI_SYS_INTERFACE_H
#define GX_UI_SYS_INTERFACE_H

#include "../VectorMath.h"
#include "../Basic.h"

namespace GraphicsUI
{
	struct Rect
	{
		int x, y, h, w;
		Rect()
		{
			x = y = h = w = 0;
		}
		Rect(int ax, int ay, int aw, int ah)
		{
			x = ax; y = ay; h = ah; w = aw;
		}
		bool Intersects(const Rect & other)
		{
			if (x > other.x + other.w)
				return false;
			if (x + w < other.x)
				return false;
			if (y > other.y + other.h)
				return false;
			if (y + h < other.y)
				return false;
			return true;
		}
	};


	struct Color
	{
		unsigned char R, G, B, A;
		Color() { R = G = B = 0; A = 0; };
		Color(unsigned char AR, unsigned char AG, unsigned char AB, unsigned char AA) { R = AR; G = AG; B = AB; A = AA; };
		Color(unsigned char AR, unsigned char AG, unsigned char AB) { R = AR; G = AG; B = AB; A = 255; };
	};

	class Pen
	{
	public:
		Color Color;
		float Width;
		int DashPattern = -1;
		Pen(const GraphicsUI::Color & c)
		{
			this->Color = c;
			Width = 1.0f;
		}
		Pen()
		{
			Color = GraphicsUI::Color(0, 0, 0);
			Width = 1.0f;
		}
	};

	class IImage : public CoreLib::Object
	{
	public:
		virtual int GetHeight() = 0;
		virtual int GetWidth() = 0;
	};

	class IBakedText
	{
	public:
		virtual int GetWidth() = 0;
		virtual int GetHeight() = 0;
		virtual ~IBakedText() {}
	};

	class IFont
	{
	public:
		virtual Rect MeasureString(const CoreLib::String & text) = 0;
		virtual IBakedText * BakeString(const CoreLib::String & tex) = 0;
	};

	enum class DefaultFontType
	{
		Content, Title, Symbol
	};

	enum class CursorType
	{
		Arrow, Cross, IBeam, Wait, SizeNS, SizeWE, SizeNESW, SizeNWSE, SizeAll
	};

	class DrawBufferVertex
	{
	public:
		float x, y, u, v;
		unsigned int ShaderType : 2;
		unsigned int ShaderInput : 30;
		unsigned int Color;
	};

	enum class DrawCommandName
	{
		Line, Ellipse, Triangle, SolidQuad, TextureQuad, ShadowQuad, TextQuad, ClipQuad
	};
	struct SolidColorCommand
	{
		Color color;
	};
	struct TextureCommand
	{
		IImage * image;
	};
	struct TextCommand
	{
		IBakedText * text;
		Color color;
	};
	struct ShadowCommand
	{
		short x, y, w, h;
		unsigned char offsetX, offsetY, shadowSize;
		Color color;
	};
	struct DrawTriangleCommand
	{
		float x2, y2;
		Color color;
	};
	class DrawCommand
	{
	public:
		DrawCommandName Name;
		float x0, y0, x1, y1;
		union
		{
			SolidColorCommand SolidColorParams;
			TextureCommand TextureParams;
			TextCommand TextParams;
			ShadowCommand ShadowParams;
			DrawTriangleCommand TriangleParams;
		};
		DrawCommand() {}
	};

	class ISystemInterface : public CoreLib::Object
	{
	public:
		virtual void SwitchCursor(GraphicsUI::CursorType cursor) = 0;
		virtual void SetClipboardText(const CoreLib::String & text) = 0;
		virtual CoreLib::String GetClipboardText() = 0;
		virtual IFont * LoadDefaultFont(DefaultFontType dt = DefaultFontType::Content) = 0;
	};
}

#endif
