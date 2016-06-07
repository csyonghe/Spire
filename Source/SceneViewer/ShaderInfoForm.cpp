#include "ShaderInfoForm.h"

namespace SceneViewer
{
	const int columnWidth = 200;

	ShaderInfoForm::ShaderInfoForm(GraphicsUI::UIEntry * entry)
		: GraphicsUI::Form(entry)
	{
		this->SetText(L"Shader Components");
		contentBox = new Container(this);
		contentBox->Posit(0, 0, 100, 100);
		contentBox->BackColor.A = 0;
		contentBox->BorderStyle = GraphicsUI::BS_NONE;
		this->BackColor = GraphicsUI::Color(0, 0, 0, 180);
		this->BorderColor = GraphicsUI::Color(255, 255, 255, 120);
		this->BorderStyle = GraphicsUI::BS_FLAT_;
	}

	void ShaderInfoForm::Update(const Spire::Compiler::ShaderMetaData & pMetaData)
	{
		this->metaData = pMetaData;
		contentBox->Enabled = false;
		for (auto & child : contentBox->Controls)
			child = nullptr;
		contentBox->Controls.Clear();
		const int leftMargin = 20;
		auto titleFont = GetEntry()->System->LoadDefaultFont(GraphicsUI::DefaultFontType::Title);
		
		int wid = 0;
		int lineHeight = (int)(GetEntry()->GetLineHeight() * 1.2f);
		int maxX = 0;
		maxY = 0;
		for (auto & w : metaData.Worlds)
		{
			auto worldLbl = new GraphicsUI::Label(contentBox);
			worldLbl->AutoSize = true;
			worldLbl->Posit(wid * columnWidth + 20, 10, 100, 30);
			worldLbl->SetText(w.Key);
			worldLbl->SetWidth(worldLbl->GetWidth() + 8);
			worldLbl->FontColor = GraphicsUI::Color(255, 255, 255);
			worldLbl->SetFont(titleFont);
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
				auto compLbl = new GraphicsUI::Label(contentBox);
				compLbl->AutoSize = true;
				compLbl->BorderStyle = GraphicsUI::BS_NONE;
				compLbl->Posit(0, 0, 100, 30);
				compLbl->SetText(comp);
				int x, y;
				if (curX + compLbl->GetWidth() + 8 < columnWidth)
				{
					x = curX;
				}
				else
				{
					curRow++;
					x = curX = leftMargin;
				}
				y = curRow * lineHeight + 50;
				compLbl->BorderColor.A = 0;
				compLbl->BackColor = GraphicsUI::Color(90,90,90,160);
				compLbl->FontColor = isInInterface ? GraphicsUI::Color(255, 220, 128) : GraphicsUI::Color(255, 255, 255);
				compLbl->Left = curX + wid * columnWidth;
				compLbl->Top = y;
				
				compLbl->SetWidth(Math::Min(compLbl->GetWidth() + 4, (wid + 1) * columnWidth - 10 - compLbl->Left));
				compLbl->BorderStyle = GraphicsUI::BS_FLAT_;
				curX += compLbl->GetWidth() + 12;
				maxY = Math::Max(y + lineHeight, maxY);
				maxX = Math::Max(compLbl->Left + compLbl->GetWidth(), maxX);
			}
			wid++;
		}
		for (int i = 1; i < metaData.Worlds.Count(); i++)
		{
			auto line = new GraphicsUI::Line(contentBox);
			line->Posit(i * columnWidth + 10, 0, 0, maxY);
		}
		contentBox->SetWidth(maxX);
		contentBox->SetHeight(maxY);
	}
	bool ShaderInfoForm::DoMouseMove(int x, int y)
	{
		Form::DoMouseMove(x, y);
		if (isMouseDown)
		{
			int dx = x - lastX;
			int dy = y - lastY;
			offsetX += dx;
			offsetY += dy;
			lastX = x;
			lastY = y;
			int xBound = -Math::Max(0, (metaData.Worlds.Count() - 1)) * columnWidth;
			if (offsetX < xBound)
				offsetX = xBound;
			if (offsetY < -maxY + this->GetClientHeight())
				offsetY = -maxY + this->GetClientHeight();
			if (offsetX > 0) offsetX = 0;
			if (offsetY > 0) offsetY = 0;
			contentBox->Left = offsetX;
			contentBox->Top = offsetY;
		}
		return true;
	}
	bool ShaderInfoForm::DoMouseDown(int x, int y, GraphicsUI::SHIFTSTATE shift)
	{
		Form::DoMouseDown(x, y, shift);
		if (resizeMode == GraphicsUI::ResizeMode::None && !this->DownInTitleBar && !this->DownInButton)
		{
			GraphicsUI::Global::MouseCaptureControl = this;
			isMouseDown = true;
			lastX = x;
			lastY = y;
		}
		return true;
	}
	bool ShaderInfoForm::DoMouseUp(int x, int y, GraphicsUI::SHIFTSTATE shift)
	{
		Form::DoMouseUp(x, y, shift);
		if (isMouseDown)
		{
			ReleaseMouse();
			isMouseDown = false;
		}
		return true;
	}
}
