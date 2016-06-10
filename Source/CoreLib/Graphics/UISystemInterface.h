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
		Color(COLORREF c) { R = GetRValue(c); G = GetGValue(c); B = GetBValue(c); A = 255; };
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
		virtual void Draw(int x, int y) = 0;
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
		virtual Rect MeasureString(const CoreLib::String & text, int width) = 0;
		virtual IBakedText * BakeString(const CoreLib::String & text, int width) = 0;
	};

	enum class DefaultFontType
	{
		Content, Title, Symbol
	};

	class ISystemInterface : public CoreLib::Object
	{
	public:
		virtual void SetRenderTransform(int dx, int dy) = 0;
		virtual void SetClipRect(const Rect & rect) = 0;
		virtual void DrawLine(const Pen & pen, float x0, float y0, float x1, float y1) = 0;
		virtual void FillRectangle(const Color & color, const Rect & rect) = 0;
		virtual void FillPolygon(const Color & color, CoreLib::ArrayView<VectorMath::Vec2> points) = 0;
		virtual void DrawRectangleShadow(const Color & color, float x, float y, float w, float h, float offsetX, float offsetY, float shadowSize) = 0;
		virtual void DrawBakedText(IBakedText * text, const Color & color, int x, int y) = 0;
		virtual void AdvanceTime(float seconds) = 0;
		virtual IFont * LoadDefaultFont(DefaultFontType dt = DefaultFontType::Content) = 0;
	};
}

#endif
