#include "ShaderInfoForm.h"

namespace SceneViewer
{
	const int columnWidth = 200;

	ShaderInfoForm::ShaderInfoForm()
	{
		this->SetText(L"Shader Componenets");
		this->OnPaint.Bind(this, &ShaderInfoForm::Form_Paint);
		this->OnMouseDown.Bind(this, &ShaderInfoForm::Form_MouseDown);
		this->OnMouseUp.Bind(this, &ShaderInfoForm::Form_MouseUp);
		this->OnMouseMove.Bind(this, &ShaderInfoForm::Form_MouseMove);
	}

	void ShaderInfoForm::Form_Paint(Object *, PaintEventArgs e)
	{
		int wid = 0;
		Gdiplus::Font titleFont(L"Segoe UI", 14.0f, Gdiplus::FontStyleBold);
		Gdiplus::Font compFont(L"Segoe UI", 11.0f);
		e.Graphics->Clear(Gdiplus::Color(255, 255, 255));
		Gdiplus::SolidBrush blackBrush(Gdiplus::Color(0, 0, 0));
		Gdiplus::Pen blackPen(&blackBrush);

		Gdiplus::SolidBrush blueBrush(Gdiplus::Color(0, 0, 255));
		Gdiplus::Pen bluePen(&blueBrush);
		int compFontHeight = (int)compFont.GetHeight(e.Graphics.Ptr());
		int lineHeight = compFontHeight + 16;
		Gdiplus::Matrix transformMat(1.0f, 0.0f, 0.0f, 1.0f, (float)offsetX, (float)offsetY);
		Gdiplus::Matrix lineTransformMat(1.0f, 0.0f, 0.0f, 1.0f, (float)offsetX, 0.0f);
		e.Graphics->SetTransform(&lineTransformMat);

		for (int i = 1; i < metaData.Worlds.Count(); i++)
		{
			e.Graphics->DrawLine(&blackPen, i * columnWidth + 10, 0, i * columnWidth + 10, this->GetClientHeight());
		}

		e.Graphics->SetTransform(&transformMat);
		maxY = 0;
		const int leftMargin = 20;
		for (auto & w : metaData.Worlds)
		{
			e.Graphics->DrawString(w.Key.Buffer(), w.Key.Length(), &titleFont, Gdiplus::PointF(wid * columnWidth + 20.0f, 10.0f), &blackBrush);
			int curRow = 0;
			int curX = leftMargin;
			for (auto & comp : w.Value.Components)
			{
				auto block = metaData.InterfaceBlocks.TryGetValue(w.Value.OutputBlock);
				bool isInInterface = false;
				if (block)
				{
					for (auto & entry : block->Entries)
						if (entry.Name == comp)
						{
							isInInterface = true;
							break;
						}
				}
				Gdiplus::RectF bbox;
				e.Graphics->MeasureString(comp.Buffer(), comp.Length(), &compFont, PointF(0.0f, 0.0f), &bbox);
				int width = (int)bbox.Width;
				int x, y;
				if (curX + width + 8 < columnWidth)
				{
					x = curX;
				}
				else
				{
					curRow++;
					x = curX = leftMargin;
				}
				y = curRow * lineHeight + 50;
				Pen * pen = isInInterface ? &bluePen : &blackPen;
				Brush * brush = isInInterface ? &blueBrush : &blackBrush;
				e.Graphics->DrawRectangle(pen, curX + wid * columnWidth, y, width + 8, (int)bbox.Height + 8);
				e.Graphics->DrawString(comp.Buffer(), comp.Length(), &compFont, Gdiplus::PointF((float)(curX + wid * columnWidth + 4), (float)(y + 4)), brush);
				curX += width + 12;
				maxY = Math::Max(y + lineHeight, maxY);
			}
			wid++;
		}
	}
	void ShaderInfoForm::Form_MouseDown(Object *, MouseEventArgs e)
	{
		SetCapture(this->GetHandle());
		isMouseDown = true;
		lastX = e.X;
		lastY = e.Y;
	}
	void ShaderInfoForm::Form_MouseUp(Object *, MouseEventArgs e)
	{
		ReleaseCapture();
		isMouseDown = false;
	}
	void ShaderInfoForm::Form_MouseMove(Object *, MouseEventArgs e)
	{
		if (isMouseDown)
		{
			int dx = e.X - lastX;
			int dy = e.Y - lastY;
			offsetX += dx;
			offsetY += dy;
			lastX = e.X;
			lastY = e.Y;
			int xBound = -Math::Max(0, (metaData.Worlds.Count() - 1)) * columnWidth;
			if (offsetX < xBound)
				offsetX = xBound;
			if (offsetY < -maxY + this->GetClientHeight())
				offsetY = -maxY + this->GetClientHeight();
			if (offsetX > 0) offsetX = 0;
			if (offsetY > 0) offsetY = 0;
			Refresh();
		}
	}
	void ShaderInfoForm::Update(const Spire::Compiler::ShaderMetaData & pMetaData)
	{
		this->metaData = pMetaData;
		Refresh();
	}
}
