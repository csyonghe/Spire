#include "LibUI.h"

namespace GraphicsUI
{
	using namespace CoreLib;
	using namespace VectorMath;

	GraphicsUI::ColorTable Global::Colors = CreateDefaultColorTable();
	int Global::HoverTimeThreshold = 200;
	int Global::EventGUID = 0;
	int Global::CursorPosX = 0;
	int Global::CursorPosY = 0;
	int Global::DeviceLineHeight = 18;
	int Global::SCROLLBAR_BUTTON_SIZE = 17;
	int Global::SCROLLBAR_MIN_PAGESIZE = 8;

	Control * Global::PointedComponent = nullptr;
	Control * Global::MouseCaptureControl = nullptr;
	Control * Global::MouseDownControl = nullptr;


	Control * lastFocusedCtrl = 0;

	//Message Type defination
	const int MSG_UI_CLICK = 1;
	const int MSG_UI_DBLCLICK = 2;
	const int MSG_UI_MOUSEDOWN = 3;
	const int MSG_UI_MOUSEUP = 4;
	const int MSG_UI_MOUSEMOVE = 5;
	const int MSG_UI_MOUSEENTER = 6;
	const int MSG_UI_MOUSELEAVE = 7;
	const int MSG_UI_MOUSEHOVER = 19;
	const int MSG_UI_KEYDOWN = 8;
	const int MSG_UI_KEYUP = 9;
	const int MSG_UI_KEYPRESS = 10;
	const int MSG_UI_CHANGED = 11;
	const int MSG_UI_RESIZE = 12;

	// this message(TopLayer Draw) is sent by Entry to notify controls to do some drawing above any other controls if necessary.
	const int MSG_UI_TOPLAYER_DRAW = 13;
	const int MSG_UI_MOUSEWHEEL = 14;
	// Form Messages
	const int MSG_UI_FORM_ACTIVATE = 15;
	const int MSG_UI_FORM_DEACTIVATE = 16;

	Control * FindNextFocus(Control * ctrl);

	void Graphics::DrawArc(int x, int y, int rad, float theta, float theta2)
	{
		float lastX = x + rad*cos(theta);
		float lastY = y - rad*sin(theta);
		int segs = rad;
		float deltaPhi = (theta2-theta)/segs;
		theta += deltaPhi;
		for (int i=1; i < segs + 1; i++)
		{	
			float nx = x + rad*cos(theta);
			float ny = y - rad*sin(theta);
			DrawLine(lastX, lastY, nx, ny);
			theta += deltaPhi;
			lastX = nx;
			lastY = ny;
		}
	}

	void Graphics::FillEllipse(float x1, float y1, float x2, float y2)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::Ellipse;
		cmd.SolidColorParams.color = SolidBrushColor;
		cmd.x0 = x1 + dx; cmd.y0 = y1 + dy;
		cmd.x1 = x2 + dx; cmd.y1 = y2 + dy;
		commandBuffer.Add(cmd);
	}

	void Graphics::FillTriangle(int x0, int y0, int x1, int y1, int x2, int y2)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::Triangle;
		cmd.x0 = (float)x0 + dx;
		cmd.x1 = (float)x1 + dx;
		cmd.y0 = (float)y0 + dy;
		cmd.y1 = (float)y1 + dy;
		cmd.TriangleParams.x2 = (float)x2 + dx;
		cmd.TriangleParams.y2 = (float)y2 + dy;
		cmd.TriangleParams.color = SolidBrushColor;
		commandBuffer.Add(cmd);
	}

	void Graphics::DrawRectangle(int x1, int y1, int x2, int y2)
	{
		DrawLine((float)x1 + 0.5f, (float)y1 + 0.5f, (float)x2, (float)y1 + 0.5f);
		DrawLine((float)x1 + 0.5f, (float)y1 + 1.5f, (float)x1 + 0.5f, (float)y2);
		DrawLine((float)x2 + 0.5f, (float)y1 + 0.5f, (float)x2 + 0.5f, (float)y2);
		DrawLine((float)x2 + 0.5f, (float)y2 + 0.5f, (float)x1 + 0.5f, (float)y2 + 0.5f);
	}

	void Graphics::FillRectangle(int x1, int y1, int x2, int y2)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::SolidQuad;
		cmd.SolidColorParams.color = SolidBrushColor;
		cmd.x0 = (float)x1 + dx; cmd.y0 = (float)y1 + dy;
		cmd.x1 = (float)x2 + dx; cmd.y1 = (float)y2 + dy;
		commandBuffer.Add(cmd);
	}

	void Graphics::DrawLine(float x1, float y1, float x2, float y2)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::Line;
		cmd.x0 = x1 + dx; cmd.y0 = y1 + dy;
		cmd.x1 = x2 + dx; cmd.y1 = y2 + dy;
		cmd.SolidColorParams.color = PenColor;
		commandBuffer.Add(cmd);
	}

	void Graphics::SetClipRect(int x, int y, int w, int h)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::ClipQuad;
		cmd.x0 = (float)x + dx;
		cmd.x1 = (float)x + w + dx;
		cmd.y0 = (float)y + dy;
		cmd.y1 = (float)y + h + dy;
		commandBuffer.Add(cmd);
	}

	void Graphics::DrawShadowRect(Color shadowColor, int x0, int y0, int w, int h, int offsetX, int offsetY, float size)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::ShadowQuad;
		cmd.ShadowParams.x = (short)(x0 + dx);
		cmd.ShadowParams.y = (short)(y0 + dy);
		cmd.ShadowParams.w = (short)w;
		cmd.ShadowParams.h = (short)h;
		cmd.ShadowParams.offsetX = (unsigned char)offsetX;
		cmd.ShadowParams.offsetY = (unsigned char)offsetY;
		cmd.ShadowParams.color = shadowColor;
		cmd.ShadowParams.shadowSize = (unsigned char)size;
		float shadowSize = size * 1.5f;
		cmd.x0 = x0 + dx + offsetX - shadowSize; cmd.y0 = y0 + dy + offsetY - shadowSize;
		cmd.x1 = cmd.x0 + w + shadowSize * 2.0f; cmd.y1 = cmd.y0 + h + shadowSize * 2.0f;
		commandBuffer.Add(cmd);
	}

	void Graphics::DrawTextQuad(IBakedText * txt, int x, int y)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::TextQuad;
		cmd.x0 = (float)x + dx;
		cmd.y0 = (float)y + dy;
		cmd.x1 = cmd.x0 + (float)txt->GetWidth();
		cmd.y1 = cmd.y0 + (float)txt->GetHeight();
		cmd.TextParams.color = SolidBrushColor;
		cmd.TextParams.text = txt;
		commandBuffer.Add(cmd);
	}

	void Graphics::DrawImage(IImage * img, int x, int y)
	{
		DrawCommand cmd;
		cmd.Name = DrawCommandName::TextQuad;
		cmd.x0 = (float)x + dx;
		cmd.y0 = (float)y + dy;
		cmd.x1 = cmd.x0 + (float)img->GetWidth();
		cmd.y1 = cmd.y0 + (float)img->GetHeight();
		cmd.TextureParams.image = img;
		commandBuffer.Add(cmd);
	}

	ColorTable CreateDarkColorTable()
	{
		ColorTable tbl;

		tbl.ShadowColor = Color(0, 0, 0, 255);
		tbl.ControlBackColor = Color(0, 0, 0, 0);
		tbl.ControlBorderColor = Color(140, 140, 140, 255);
		tbl.ControlFontColor = Color(255, 255, 255, 255);
		tbl.EditableAreaBackColor = Color(50, 50, 50, 170);

		tbl.MemuIconBackColor = Color(127, 127, 127, 255);
		tbl.MenuBackColor = Color(80, 80, 80, 255);
		tbl.MenuBorderColor = Color(127, 127, 127, 255);
		tbl.MenuSeperatorColor = Color(130, 130, 130, 255);
		tbl.MenuItemForeColor = Color(255, 255, 255, 255);
		tbl.MenuItemDisabledForeColor = Color(180, 180, 180, 255);
		tbl.MenuItemHighlightForeColor = tbl.MenuItemForeColor;
	
		tbl.TabPageBorderColor = tbl.ControlBorderColor;
		tbl.TabPageItemSelectedBackColor1 = Color(140, 140, 140, 255);
		tbl.TabPageItemSelectedBackColor2 = tbl.TabPageItemSelectedBackColor1;

		tbl.TabPageItemHighlightBackColor1 = Color(70, 70, 70, 255);
		tbl.TabPageItemHighlightBackColor2 = tbl.TabPageItemHighlightBackColor1;

		tbl.TabPageItemBackColor1 = tbl.ControlBackColor;
		tbl.TabPageItemBackColor2 = tbl.TabPageBorderColor;

		tbl.ButtonBackColorChecked = Color(40, 40, 40, 255);

		tbl.DefaultFormStyle.ShowIcon = true;
		tbl.DefaultFormStyle.CtrlButtonBorderStyle = BS_RAISED;
		tbl.DefaultFormStyle.TitleBarColors[0] = Color(85, 85, 85, 255);
		tbl.DefaultFormStyle.TitleBarColors[1] = tbl.DefaultFormStyle.TitleBarColors[0];
		tbl.DefaultFormStyle.TitleBarColors[2] = Color(166, 202, 240, 255);
		tbl.DefaultFormStyle.TitleBarColors[3] = tbl.DefaultFormStyle.TitleBarColors[2];
		tbl.DefaultFormStyle.TitleBarDeactiveColors[0] = Color(128, 128, 128, 255);
		tbl.DefaultFormStyle.TitleBarDeactiveColors[1] = tbl.DefaultFormStyle.TitleBarDeactiveColors[0];
		tbl.DefaultFormStyle.TitleBarDeactiveColors[2] = Color(192, 192, 192, 255);
		tbl.DefaultFormStyle.TitleBarDeactiveColors[3] = tbl.DefaultFormStyle.TitleBarDeactiveColors[2];

		tbl.DefaultFormStyle.TitleBarFontColor = Color(255, 255, 255, 255);
		tbl.DefaultFormStyle.TopMost = false;
		tbl.DefaultFormStyle.BackColor = Color(0, 0, 0, 180);
		tbl.DefaultFormStyle.BorderColor = tbl.ControlBorderColor;

		tbl.SelectionColor = Color(224, 135, 0, 255);
		tbl.UnfocusedSelectionColor = Color(100, 100, 100, 127);
		tbl.HighlightColor = Color(100, 100, 100, 127);
		tbl.HighlightForeColor = Color(255, 255, 255, 255);
		tbl.SelectionForeColor = Color(255, 255, 255, 255);
		tbl.FocusRectColor = Color(120, 120, 120, 220);

		tbl.ToolButtonBackColor1 = tbl.ControlBackColor;
		tbl.ToolButtonBackColor2 = Color(55, 55, 55, 220);
		tbl.ToolButtonBackColorHighlight1 = tbl.SelectionColor;
		tbl.ToolButtonBackColorHighlight2 = tbl.SelectionColor;
		tbl.ToolButtonBackColorPressed1 = Color(184, 75, 0, 255);
		tbl.ToolButtonBackColorPressed2 = Color(184, 75, 0, 255);
		tbl.ToolButtonBorderHighLight = Color(254, 193, 92, 0);
		tbl.ToolButtonBorderSelected = Color(254, 193, 92, 0);
		tbl.ToolButtonSeperatorColor = Color(130, 130, 130, 255);
		tbl.ToolButtonBackColorChecked1 = Color(204, 105, 0, 255);
		tbl.ToolButtonBackColorChecked2 = tbl.ToolButtonBackColorChecked1;
		tbl.StatusStripBackColor1 = tbl.StatusStripBackColor2 = tbl.ToolButtonBackColor2;
		tbl.StatusStripBackColor3 = tbl.StatusStripBackColor4 = tbl.ToolButtonBackColor2;

		tbl.ScrollBarBackColor = tbl.EditableAreaBackColor;
		tbl.ScrollBarBackColor.R += 30;
		tbl.ScrollBarBackColor.G += 30;
		tbl.ScrollBarBackColor.B += 30;

		tbl.ScrollBarForeColor = Color(180, 180, 180, 255);
		tbl.ScrollBarHighlightColor = Color(140, 140, 140, 255);
		tbl.ScrollBarPressedColor = Color(100, 100, 100, 255);
		tbl.ScrollBarSliderColor = Color(110, 110, 110, 255);
		return tbl;
	}

	int emToPixel(float em)
	{
		return (int)(em * Global::DeviceLineHeight);
	}

	ColorTable CreateDefaultColorTable()
	{
		ColorTable tbl;
		tbl.ShadowColor = Color(0, 0, 0, 120);
		tbl.ControlBackColor = Color(235,238,241,255);
		tbl.ControlBorderColor = Color(160, 160, 160, 255);
		tbl.ControlFontColor = Color(0, 0, 0, 255);
		tbl.EditableAreaBackColor = Color(255, 255, 255, 255);
		tbl.ScrollBarBackColor = Color(255, 255, 255, 127);
		tbl.MemuIconBackColor = Color(232,232,225,255);
		tbl.MenuBackColor = Color(242,242,238,255);
		tbl.MenuBorderColor = Color(150,150,150,255);
		tbl.MenuSeperatorColor = Color(180,180,180,255);
		tbl.MenuItemForeColor = Color(0,0,0,255);
		tbl.MenuItemDisabledForeColor = Color(180,180,180,255);
		tbl.MenuItemHighlightForeColor = tbl.MenuItemForeColor;
		tbl.ToolButtonBackColor1 = tbl.ControlBackColor;
		tbl.ToolButtonBackColor2 = Color(215,226,228,255);
		tbl.ToolButtonBackColorHighlight1 = Color(255,250,210,255);
		tbl.ToolButtonBackColorHighlight2 = Color(253,236,168,255);
		tbl.ToolButtonBackColorPressed1 = Color(249,217,132,255);
		tbl.ToolButtonBackColorPressed2 = Color(252,236,194,255);
		tbl.ToolButtonBorderHighLight = Color(254,193,92,255);
		tbl.ToolButtonBorderSelected = Color(254,193,92,255);
		tbl.ToolButtonSeperatorColor = Color(170,170,160,255);
		tbl.ToolButtonBackColorChecked1 = Color(253, 247, 182, 255);
		tbl.ToolButtonBackColorChecked2 = tbl.ToolButtonBackColorChecked1;
		tbl.StatusStripBackColor1 = tbl.StatusStripBackColor2 = tbl.ToolButtonBackColor2;
		tbl.StatusStripBackColor3 = tbl.StatusStripBackColor4 = tbl.ToolButtonBackColor2;

		tbl.TabPageBorderColor = Color(127, 127, 127, 255);
		tbl.TabPageItemSelectedBackColor1 = Color(210, 227, 255, 255);
		tbl.TabPageItemSelectedBackColor2 = tbl.ControlBackColor;

		tbl.TabPageItemHighlightBackColor1 = Color(220, 244, 255, 255);
		tbl.TabPageItemHighlightBackColor2 = Color(220, 244, 255, 255);

		tbl.TabPageItemBackColor1 = tbl.ControlBackColor;
		tbl.TabPageItemBackColor2 = tbl.TabPageBorderColor;

		tbl.ButtonBackColorChecked = Color(254,216,152,255);

		tbl.SelectionColor = Color(10, 36, 106, 255);
		tbl.HighlightColor = Color(200, 200, 200, 255);
		tbl.HighlightForeColor = Color(0, 0, 0, 255);
		tbl.SelectionForeColor = Color(255, 255, 255, 255);
		tbl.UnfocusedSelectionColor = Color(200, 200, 200, 255);
		tbl.DefaultFormStyle.ShowIcon = true;
		tbl.DefaultFormStyle.CtrlButtonBorderStyle = BS_RAISED;
		tbl.DefaultFormStyle.TitleBarColors[0] = Color(10, 36, 106, 255);
		tbl.DefaultFormStyle.TitleBarColors[1] = tbl.DefaultFormStyle.TitleBarColors[0];
		tbl.DefaultFormStyle.TitleBarColors[2] = Color(166, 202, 240, 255);
		tbl.DefaultFormStyle.TitleBarColors[3] = tbl.DefaultFormStyle.TitleBarColors[2];
		tbl.DefaultFormStyle.TitleBarDeactiveColors[0] = Color(128, 128, 128, 255);
		tbl.DefaultFormStyle.TitleBarDeactiveColors[1] = tbl.DefaultFormStyle.TitleBarDeactiveColors[0];
		tbl.DefaultFormStyle.TitleBarDeactiveColors[2] = Color(192, 192, 192, 255);
		tbl.DefaultFormStyle.TitleBarDeactiveColors[3] = tbl.DefaultFormStyle.TitleBarDeactiveColors[2];

		tbl.DefaultFormStyle.TitleBarFontColor = Color(255, 255, 255, 255);
		tbl.DefaultFormStyle.TopMost = false;
		tbl.DefaultFormStyle.BackColor = tbl.ControlBackColor;
		tbl.DefaultFormStyle.BorderColor = tbl.ControlBorderColor;

		tbl.UnfocusedSelectionColor = Color(127, 127, 127, 255);
		tbl.FocusRectColor = Color(120, 120, 120, 220);

		tbl.ScrollBarBackColor = tbl.EditableAreaBackColor;
		tbl.ScrollBarBackColor.R -= 15;
		tbl.ScrollBarBackColor.G -= 15;
		tbl.ScrollBarBackColor.B -= 15;

		tbl.ScrollBarForeColor = Color(80, 80, 80, 255);
		tbl.ScrollBarSliderColor = Color(tbl.ScrollBarBackColor.R - 30, tbl.ScrollBarBackColor.R - 30, tbl.ScrollBarBackColor.R - 30, 255);
		tbl.ScrollBarHighlightColor = Color(tbl.ScrollBarSliderColor.R - 60, tbl.ScrollBarSliderColor.G - 60, tbl.ScrollBarSliderColor.B - 60, 255);
		tbl.ScrollBarPressedColor = Color(tbl.ScrollBarHighlightColor.R - 20, tbl.ScrollBarHighlightColor.G - 20, tbl.ScrollBarHighlightColor.B - 20, 255);

		return tbl;
	}

	int ClampInt(int val, int min, int max)
	{
		if (val < min)
			return min;
		else if (val > max)
			return max;
		else
			return val;
	}

	ClipRectStack::ClipRectStack(Graphics * g)
	{
		StackSize = 0;
		graphics = g;
	}

	void ClipRectStack::PushRect(Rect nRect)
	{
		Buffer[StackSize] = nRect;
		StackSize ++;
		graphics->SetClipRect(nRect.x, nRect.y, nRect.w, nRect.h);
	}

	Rect ClipRectStack::PopRect()
	{
		if (StackSize)
			StackSize--;
		if (StackSize)
		{
			auto r = Buffer[StackSize - 1];
			graphics->SetClipRect(r.x, r.y, r.w, r.h);
			return Buffer[StackSize-1];
		}
		else
		{
			auto rect = Rect(0,0,WindowWidth,WindowHeight);
			graphics->SetClipRect(rect.x, rect.y, rect.w, rect.h);
			return rect;
		}
	}

	Rect ClipRectStack::GetTop()
	{
		return Buffer[StackSize-1];
	}

	void ClipRectStack::Clear()
	{
		StackSize = 0;
	}

	void ClipRectStack::AddRect(Rect nRect)
	{
		Rect cRect;
		if (StackSize)
		{
			int nx1,nx2,ny1,ny2;
			nx1 = nRect.x + nRect.w;
			nx2 = Buffer[StackSize-1].x + Buffer[StackSize-1].w;
			ny1 = nRect.y + nRect.h;
			ny2 = Buffer[StackSize-1].y + Buffer[StackSize-1].h;
			cRect.x = Math::Max(nRect.x,Buffer[StackSize-1].x);
			cRect.y = Math::Max(nRect.y,Buffer[StackSize-1].y);
			cRect.w = Math::Min(nx1,nx2)-cRect.x;
			cRect.h = Math::Min(ny1,ny2)-cRect.y;
		}
		else
		{
			cRect = nRect;
		}
		PushRect(cRect);
	}

	void UI_Base::HandleMessage(const UI_MsgArgs *)
	{
	}

	Control::Control(Container * parent, bool addToParent)
	{
		ID = 0;
		EventID = -1;
		Cursor = CursorType::Arrow;
		Width = Height = Left = Top = 0;
		Name = "unnamed";
		Enabled = true;
		Visible = true;
		TopMost = false;
		LastInClient = false;
		BackgroundShadow = false;
		FontColor = Color(0, 0, 0, 255);
		Parent = parent;
		if (parent)
		{
			font = parent->GetFont();
			if (addToParent)
				parent->AddChild(this);
		}
		TabStop = false;
		BorderStyle = BS_RAISED;
		Type = CT_CONTROL;
		AbsolutePosX = AbsolutePosY = 0;
		BackColor = Global::Colors.ControlBackColor;
		BorderColor = Global::Colors.ControlBorderColor;
		DockStyle = dsNone;
	}

	Control::Control(Container * parent)
		: Control(parent, true)
	{
	}

	Control::~Control()
	{
		auto entry = GetEntry();
		if (Global::PointedComponent == this)
			Global::PointedComponent = Parent;
		if (Global::MouseCaptureControl == this)
			Global::MouseCaptureControl = nullptr;
		if (entry && entry->FocusedControl == this)
			entry->FocusedControl = nullptr;
	}

	bool Control::DoClosePopup()
	{
		return false;
	}

	VectorMath::Vec2i Control::GetRelativePos(Container * parent)
	{
		VectorMath::Vec2i result;
		result.x = Left;
		result.y = Top;
		auto current = this;
		auto curParent = this->Parent;
		while (curParent != parent)
		{
			result.x += curParent->Left;
			result.y += curParent->Top;
			if (current->DockStyle == dsFill || current->DockStyle == dsNone)
			{
				result.x += curParent->ClientRect().x;
				result.y += curParent->ClientRect().y;
			}
			current = curParent;
			curParent = curParent->Parent;
		}
		return result;
	}

	void Control::LocalPosToAbsolutePos(int x, int y, int & ax, int & ay)
	{
		auto relPos = GetRelativePos(nullptr);
		ax = relPos.x + x;
		ay = relPos.y + y;
	}

	bool Control::IsFocused()
	{
		auto focus = GetEntry()->FocusedControl;
		while (focus)
		{
			if (focus == this)
				return true;
			focus = focus->Parent;
		}
		return false;
	}

	void Control::BroadcastMessage(const UI_MsgArgs *Args)
	{
		switch (Args->Type)
		{
		case MSG_UI_CLICK:
			OnClick.Invoke(this);
			return;
		case MSG_UI_DBLCLICK:
			OnDblClick.Invoke(this);
			return;
		case MSG_UI_CHANGED:
			OnChanged.Invoke(this);
			return;
		case MSG_UI_RESIZE:
			OnResize.Invoke(this);
			return;
		case MSG_UI_MOUSEENTER:
			OnMouseEnter.Invoke(this);
			return;
		case MSG_UI_MOUSELEAVE:
			OnMouseLeave.Invoke(this);
			return;
		case MSG_UI_MOUSEMOVE:
			OnMouseMove.Invoke(this, *((UIMouseEventArgs*)Args->Data));
			return;
		case MSG_UI_MOUSEDOWN:
			OnMouseDown.Invoke(this, *((UIMouseEventArgs*)Args->Data));
			return;
		case MSG_UI_MOUSEUP:
			OnMouseUp.Invoke(this, *((UIMouseEventArgs*)Args->Data));
			return;
		case MSG_UI_MOUSEWHEEL:
			OnMouseWheel.Invoke(this,*((UIMouseEventArgs*)Args->Data));
			return;
		case MSG_UI_MOUSEHOVER:
			OnMouseHover.Invoke(this);
			return;
		case MSG_UI_KEYDOWN:
			OnKeyDown.Invoke(this, *((UIKeyEventArgs*)Args->Data));
			return;
		case MSG_UI_KEYUP:
			OnKeyUp.Invoke(this, *((UIKeyEventArgs*)Args->Data));
			return;
		case MSG_UI_KEYPRESS:
			OnKeyPress.Invoke(this, *((UIKeyEventArgs*)Args->Data));
			return;
		}
	}

	int Control::GetWidth()
	{
		return Width;
	}

	int Control::GetHeight()
	{
		return Height;
	}

	void Control::Posit(int ALeft, int ATop, int AWidth, int AHeight)
	{
		Left = ALeft;
		Top = ATop;
		Height = AHeight;
		Width = AWidth;
		SizeChanged();
	}
		
	Rect Control::ClientRect()
	{
		return clientRect;
	}

	bool Control::IsPointInClient(int X, int Y)
	{
		bool rs = (X>0 && Y>0 && X<Width && Y<Height);
		return rs;
	}

	Control * Control::FindControlAtPosition(int x, int y)
	{
		bool rs = (x>0 && y>0 && x<Width && y<Height);
		if (rs && Visible)
			return this;
		return nullptr;
	}

	bool Control::IsChildOf(Container * ctrl)
	{
		auto parent = Parent;
		while (parent && parent != ctrl)
			parent = parent->Parent;
		if (parent)
			return true;
		return false;
	}

	void Control::ReleaseMouse()
	{
		if (Global::MouseCaptureControl == this)
			Global::MouseCaptureControl = nullptr;
	}

	void Control::SetHeight(int val)
	{
		Height = val;
		SizeChanged();
	}

	void Control::SetWidth(int val)
	{
		Width = val;
		SizeChanged();
	}

	void Control::SizeChanged()
	{
		clientRect = Rect(0,0,Width,Height);
		UI_MsgArgs Arg;
		Arg.Sender = this;
		Arg.Type = MSG_UI_RESIZE;
		BroadcastMessage(&Arg);
		OnResize.Invoke(this);
	}

	bool Control::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		if (IsPointInClient(X,Y))
		{
			GetEntry()->System->SwitchCursor(Cursor);
			UI_MsgArgs Args;UIMouseEventArgs Data;
			Args.Sender = this;	Args.Type = MSG_UI_MOUSEDOWN;
			Data.Shift = Shift;	Data.X = X;	Data.Y = Y;
			Args.Data = &Data;
			BroadcastMessage(&Args);
			if (Parent)
				SetFocus();
		}
		return false;
	}

	bool Control::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		UI_MsgArgs Args;UIMouseEventArgs Data;
		Args.Sender = this;	Args.Type = MSG_UI_MOUSEUP;
		Data.Shift = Shift;	Data.X = X;	Data.Y = Y;
		Args.Data = &Data;
		BroadcastMessage(&Args);
		if (this == Global::MouseDownControl)
		{
			Args.Sender = this; Args.Type = MSG_UI_CLICK;
			Args.Data = nullptr;
			BroadcastMessage(&Args);
		}
		return false;
	}

	bool Control::DoMouseMove(int X, int Y)
	{
		if (!Enabled || !Visible)
			return false;
		UI_MsgArgs Args;
		UIMouseEventArgs Data;
		Args.Sender = this;	
		Data.Shift = 0;	Data.X = X-Left;	Data.Y = Y-Top;
		Args.Data = &Data;
		GetEntry()->System->SwitchCursor(Cursor);
		Args.Type = MSG_UI_MOUSEMOVE;
		BroadcastMessage(&Args);
		return false;
	}

	bool Control::DoMouseEnter()
	{
		UI_MsgArgs Args;
		UIMouseEventArgs Data;
		Args.Sender = this;
		Data.Shift = 0;	Data.X = 0;	Data.Y = 0;
		Args.Data = &Data;
		Args.Type = MSG_UI_MOUSEENTER;
		BroadcastMessage(&Args);
		//GetEntry()->System->SwitchCursor(Cursor);
		return false;
	}

	bool Control::DoMouseLeave()
	{
		UI_MsgArgs Args;
		UIMouseEventArgs Data;
		Args.Sender = this;	
		Data.Shift = 0;	Data.X = 0;	Data.Y = 0;
		Args.Data = &Data;
		Args.Type = MSG_UI_MOUSELEAVE;
		BroadcastMessage(&Args);
		//GetEntry()->System->SwitchCursor(CursorType::Arrow);
		return false;
	}

	bool Control::DoMouseHover()
	{
		OnMouseHover.Invoke(this);
		return false;
	}

	bool Control::DoKeyDown(unsigned short Key, SHIFTSTATE Shift) 
	{
		if (!Enabled || !Visible)
			return false;
		UI_MsgArgs Args;UIKeyEventArgs Data;
		Args.Sender = this;	Args.Type = MSG_UI_KEYDOWN;
		Data.Key = Key;Data.Shift = Shift;
		Args.Data = &Data;
		BroadcastMessage(&Args);
		return false;
	}

	bool Control::DoKeyUp(unsigned short Key, SHIFTSTATE Shift) 
	{
		if (!Enabled || !Visible)
			return false;
		UI_MsgArgs Args;UIKeyEventArgs Data;
		Args.Sender = this;	Args.Type = MSG_UI_KEYUP;
		Data.Key = Key;Data.Shift = Shift;
		Args.Data = &Data;
		BroadcastMessage(&Args);
		return false;
	}

	bool Control::DoKeyPress(unsigned short Key, SHIFTSTATE Shift) 
	{
		if (!Enabled || !Visible)
			return false;
		UI_MsgArgs Args;UIKeyEventArgs Data;
		Args.Sender = this;	Args.Type = MSG_UI_KEYPRESS;
		Data.Key = Key;Data.Shift = Shift;
		Args.Data = &Data;
		BroadcastMessage(&Args);
		return false;
	}

	bool Control::DoClick() 
	{
		if (!Enabled || !Visible )
			return false;
		UI_MsgArgs Args;
		Args.Sender = this;
		Args.Type = MSG_UI_CLICK;
		BroadcastMessage(&Args);
		return false;
	}

	bool Control::DoDblClick()
	{
		if (!Enabled || !Visible)
			return false;
		UI_MsgArgs Args;
		Args.Sender = this;
		Args.Type = MSG_UI_DBLCLICK;
		BroadcastMessage(&Args);
		return false;
	}

	void Control::LostFocus(Control * /*newFocus*/)
	{
		OnLostFocus.Invoke(this);
	}

	void Control::SetName(String AName)
	{
		Name = AName;
	}

	void Control::Draw(int absX, int absY)
	{
		absX = absX + Left;
		absY = absY + Top;
		AbsolutePosX = absX;  AbsolutePosY = absY;
		auto &clipRects = GetEntry()->ClipRects;
		auto entry = GetEntry();
		if (BackgroundShadow)
		{
			// Draw background shadow
			Rect R = clipRects->PopRect();
			{
				Color shadowColor = Global::Colors.ShadowColor;
				shadowColor.A = ShadowOpacity;
				entry->DrawCommands.DrawShadowRect(shadowColor, absX, absY, Width, Height, ShadowOffset, ShadowOffset, ShadowSize);
			}
			clipRects->PushRect(R);
		}
		//Draw Background
		if (BackColor.A)
		{
			entry->DrawCommands.SolidBrushColor = BackColor;
			entry->DrawCommands.FillRectangle(absX + 1, absY + 1, absX + Width - 1, absY + Height - 1);
		}
		//Draw Border
		Color LightColor, DarkColor;
		LightColor.R = (unsigned char)ClampInt(BorderColor.R + COLOR_LIGHTEN,0,255);
		LightColor.G = (unsigned char)ClampInt(BorderColor.G + COLOR_LIGHTEN,0,255);
		LightColor.B = (unsigned char)ClampInt(BorderColor.B + COLOR_LIGHTEN,0,255);
		LightColor.A = BorderColor.A;
		DarkColor.R = (unsigned char)ClampInt(BorderColor.R - COLOR_LIGHTEN, 0, 255);
		DarkColor.G = (unsigned char)ClampInt(BorderColor.G - COLOR_LIGHTEN, 0, 255);
		DarkColor.B = (unsigned char)ClampInt(BorderColor.B - COLOR_LIGHTEN, 0, 255);
		DarkColor.A = BorderColor.A;
		if (BorderStyle == BS_RAISED)
		{
			entry->DrawCommands.PenColor = LightColor;
			entry->DrawCommands.DrawLine(absX, absY, absX + Width - 1, absY);
			entry->DrawCommands.DrawLine(absX, absY, absX, absY + Height - 1);

			entry->DrawCommands.PenColor = DarkColor;
			entry->DrawCommands.DrawLine(absX + Width - 1, absY, absX + Width - 1, absY + Height - 1);
			entry->DrawCommands.DrawLine(absX + Width - 1, absY + Height - 1, absX, absY + Height - 1);
		}
		else if (BorderStyle == BS_LOWERED)
		{
			entry->DrawCommands.PenColor = DarkColor;
			entry->DrawCommands.DrawLine(absX, absY, absX + Width - 1, absY);
			entry->DrawCommands.DrawLine(absX, absY, absX, absY + Height - 1);

			entry->DrawCommands.PenColor = LightColor;
			entry->DrawCommands.DrawLine(absX + Width - 1, absY, absX + Width - 1, absY + Height - 1);
			entry->DrawCommands.DrawLine(absX + Width - 1, absY + Height - 1, absX, absY + Height - 1);
		}
		else if (BorderStyle == BS_FLAT_)
		{
			entry->DrawCommands.PenColor = BorderColor;
			entry->DrawCommands.DrawRectangle(absX, absY, absX + Width - 1, absY + Height - 1);
		}
	}

	void Control::SetFont(IFont * AFont)
	{
		this->font = AFont;
	}

	void Control::KillFocus()
	{
		OnLostFocus(this);
		if (GetEntry()->FocusedControl == this)
			GetEntry()->FocusedControl = nullptr;
	}

	void Control::SetFocus()
	{			
		GetEntry()->SetFocusedControl(this);		
	}

	Label::Label(Container * parent)
		: Container(parent)
	{
		BorderStyle = BS_NONE;
		BackColor.A = 0;
		BackColor.R = 255;  BackColor.G =255; BackColor.B = 255;
		FontColor = Global::Colors.ControlFontColor;
		FChanged = true;
		Type = CT_LABEL;
		AutoSize = true;
		DropShadow = false;
		this->font = parent->GetFont();
	}

	Label::~Label()
	{
	}

	String Label::GetText()
	{
		return FCaption;
	}

	void Label::SetText(const String & pText)
	{
		bool diff = FCaption != pText;
		FChanged = FChanged || diff;
		if (FCaption != pText)
		{
			FCaption = pText;
			UpdateText();
		}
	}

	void Label::SetFont(IFont * pFont)
	{
		if (pFont != font)
		{
			Control::SetFont(pFont);
			FChanged = true;
		}
	}

	void Label::SizeChanged()
	{
	}

	void Label::DoDpiChanged()
	{
		UpdateText();
	}

	void Label::UpdateText()
	{
		if (text)
			text = nullptr;
		auto size = font->MeasureString(FCaption);
		TextWidth = size.w;
		TextHeight = size.h;
		FChanged = false;
		if (AutoSize)
		{
			SetWidth(TextWidth + Padding.Horizontal());
			SetHeight(TextHeight + Padding.Vertical());
		}
	}

	void Label::Draw(int absX, int absY)
	{
		Control::Draw(absX, absY);
		absX = absX + Left;
		auto entry = GetEntry();
		if (font == nullptr)
		{
			font = entry->System->LoadDefaultFont();
			FChanged = true;
			UpdateText();
		}
		if (FChanged || !text)
		{
			text = font->BakeString(FCaption);
			FChanged = false;
		}
		if (VertAlignment == VerticalAlignment::Top)
			absY = absY + Top + Padding.Top;
		else if (VertAlignment == VerticalAlignment::Center)
			absY = absY + Top + (Height - TextHeight) / 2;
		else
			absY = absY + Top + Height - Padding.Bottom - TextHeight;
		if (DropShadow)
		{
			entry->DrawCommands.SolidBrushColor = ShadowColor;
			entry->DrawCommands.DrawTextQuad(text.Ptr(), absX + 1, absY + 1);
		}
		entry->DrawCommands.SolidBrushColor = FontColor;
		entry->DrawCommands.DrawTextQuad(text.Ptr(), absX, absY);
	}

	Button::Button(Container * parent)
		: Label(parent)
	{
		IsMouseDown = false;
		TabStop = true;
		Type = CT_BUTTON;
		BorderStyle = BS_RAISED;
		BackColor = Global::Colors.ControlBackColor;
		FontColor = Global::Colors.ControlFontColor;
		Checked = false;
		Padding = GetEntry()->GetLineHeight() / 2;
		Padding.Top = Padding.Bottom = Padding.Left / 2;
	}

	Button::Button(Container * parent, const CoreLib::String & text)
		:Button(parent)
	{
		SetText(text);
	}

	void Button::Draw(int absX, int absY)
	{
		if (!Visible)
			return;
		int lastBorder = BorderStyle;
		Color backColor = BackColor;
		if (Checked)
		{
			BackColor = Global::Colors.ButtonBackColorChecked;
			BorderStyle = BS_LOWERED;
		}
		Control::Draw(absX,absY);
		BorderStyle = lastBorder;
		BackColor = backColor;
		absX = absX + Left;
		absY = absY + Top;
		auto entry = GetEntry();
		if (font == nullptr)
			font = entry->System->LoadDefaultFont();
		if (FChanged || !text)
		{
			text = font->BakeString(FCaption);
		}
		int tx,ty;
		tx = (Width - TextWidth)/2;
		ty = (Height - TextHeight)/2;
		if (BorderStyle == BS_LOWERED)
		{
			tx += 1;
			ty += 1;
		}
		auto & graphics = entry->DrawCommands;
		if (Enabled)
		{
			graphics.SolidBrushColor = FontColor;
			graphics.DrawTextQuad(text.Ptr(), absX+tx,absY+ty);
		}
		else
		{
			graphics.SolidBrushColor = Color(255, 255, 255, FontColor.A);
			graphics.DrawTextQuad(text.Ptr(), absX + tx + 1, absY + ty + 1);
			graphics.SolidBrushColor = Color((unsigned char)ClampInt(FontColor.R + COLOR_LIGHTEN, 0, 255),
				(unsigned char)ClampInt(FontColor.R + COLOR_LIGHTEN, 0, 255),
				(unsigned char)ClampInt(FontColor.R + COLOR_LIGHTEN, 0, 255),
				FontColor.A);
			graphics.DrawTextQuad(text.Ptr(), absX + tx, absY + ty);
		}
		
		// Draw Focus Rect
		if (IsFocused())
		{
			graphics.PenColor = Global::Colors.FocusRectColor;
			graphics.DrawRectangle(absX + 3, absY + 3, absX + Width - 3, absY + Height - 3);
		}
	}

	bool Button::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Label::DoMouseDown(X,Y,Shift); 
		if (!Enabled || !Visible)
			return false;
		if (Shift == SS_BUTTONLEFT)
		{
			IsMouseDown = true;
			BorderStyle = BS_LOWERED;
		}
		return true;
	}

	bool Button::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Label::DoMouseUp(X,Y,Shift);
		IsMouseDown = false;
		BorderStyle = BS_RAISED;
		return true;
	}

	bool Button::DoMouseLeave()
	{
		IsMouseDown = false;
		BorderStyle = BS_RAISED;
		return true;
	}

	bool Button::DoDblClick()
	{
		return DoMouseDown(1, 1, SS_BUTTONLEFT);
	}

	bool Button::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Label::DoKeyDown(Key,Shift);
		if (!Enabled || !Visible)
			return false;
		if (Key == 0x20) // VK_SPACE
		{
			IsMouseDown = true;
			BorderStyle = BS_LOWERED;
		}
		else if (Key == 0x0D) // VK_RETURN
		{
			Control::DoClick();
		}
		return false;
	}

	bool Button::DoKeyUp(unsigned short Key, SHIFTSTATE Shift)
	{
		Label::DoKeyUp(Key,Shift);
		if (!Enabled || !Visible)
			return false;
		if (Key == 0x20) // VK_SPACE
		{
			IsMouseDown = false;
			BorderStyle = BS_RAISED;
			Control::DoClick();
		}
		return false;
	}

	void Button::DoDpiChanged()
	{
		Padding = GetEntry()->GetLineHeight() / 2;
		Padding.Top = Padding.Bottom = Padding.Left / 2;
		Label::DoDpiChanged();
	}

	Control * Container::FindControlAtPosition(int x, int y)
	{
		if (Visible && IsPointInClient(x, y))
		{
			if (x <= Padding.Left || y <= Padding.Top || x >= Width - Padding.Right || y >= Height - Padding.Bottom)
				return this;
			for (int i = controls.Count() - 1; i >= 0; i--)
			{
				if (controls[i]->EventID != Global::EventGUID)
				{
					int dx = 0;
					int dy = 0;
					if (controls[i]->DockStyle == dsNone || controls[i]->DockStyle == dsFill)
					{
						dx = clientRect.x;
						dy = clientRect.y;
					}
					int nx = x - dx;
					int ny = y - dy;
					if (auto child = controls[i]->FindControlAtPosition(nx - controls[i]->Left, ny - controls[i]->Top))
						return child;
				}
			}
			return this;
		}
		return nullptr;
	}

	Container::Container(Container * parent, bool addToParent)
		: Control(parent, addToParent)
	{
		Type = CT_CONTAINER;
		TabStop = false;
		Padding = 0;
		BorderStyle = BS_NONE;
	}

	Container::Container(Container * parent)
		: Container(parent, true)
	{
	}

	Container::Container(Container * parent, ContainerLayoutType pLayout)
		: Container(parent, true)
	{
		layout = pLayout;
	}

	bool Container::DoClosePopup()
	{
		for (int i=0;i<controls.Count(); i++)
			controls[i]->DoClosePopup();
		return false;
	}

	void Container::KillFocus()
	{
		for (int i = 0; i<controls.Count(); i++)
		{
			controls[i]->KillFocus();
		}
		Control::KillFocus();
	}

	void Container::SetLayout(ContainerLayoutType pLayout)
	{
		layout = pLayout;
	}

	void Container::DoDpiChanged()
	{
		if (layout == ContainerLayoutType::None)
		{
			float dpiScale = GetEntry()->GetDpiScale();
			for (auto & child : controls)
			{
				child->Posit((int)(child->Left*dpiScale),
					(int)(child->Top * dpiScale),
					(int)(child->GetWidth()*dpiScale),
					(int)(child->GetHeight() * dpiScale));
			}
		}
		for (auto & child : controls)
			child->DoDpiChanged();
		
		SizeChanged();
	}

	void Container::AddChild(Control *nControl)
	{
		controls.Add(nControl);
		nControl->Parent = this;
	}

	void Container::RemoveChild(Control *AControl)
	{
		for (int i=0; i<controls.Count(); i++)
		{
			if (controls[i] == AControl)
			{
				controls[i] = nullptr;
				controls.RemoveAt(i);
				break;
			}
		}
	}

	void Container::DrawChildren(int absX, int absY)
	{
		auto entry = GetEntry();
		entry->ClipRects->AddRect(Rect(absX + Padding.Left, absY + Padding.Top, Width - Padding.Horizontal(), Height - Padding.Vertical()));
		for (int i = 0; i<controls.Count(); i++)
		{
			if (controls[i]->Visible)
			{
				Control *ctrl = controls[i].Ptr();
				if (ctrl->Visible)
				{
					int dx = 0;
					int dy = 0;
					if (ctrl->DockStyle == dsNone || ctrl->DockStyle == dsFill)
					{
						dx = clientRect.x;
						dy = clientRect.y;
					}
					entry->ClipRects->AddRect(Rect(ctrl->Left + absX + dx, ctrl->Top + absY + dy, ctrl->GetWidth(), ctrl->GetHeight()));
					auto clipRect = entry->ClipRects->GetTop();
					if (ctrl->Visible && clipRect.Intersects(Rect(absX + dx + ctrl->Left, absY + dy + ctrl->Top, ctrl->GetWidth(), ctrl->GetHeight())))
						ctrl->Draw(absX + dx, absY + dy);
					entry->ClipRects->PopRect();
				}
			}
		}
		entry->ClipRects->PopRect();
	}

	void Container::Draw(int absX, int absY)
	{
		Control::Draw(absX, absY);
		absX+=Left; absY+=Top;
		if (drawChildren)
			DrawChildren(absX, absY);
	}

	void Container::ArrangeControls(Rect initalClientRect)
	{
		clientRect = initalClientRect;
		clientRect.x = initalClientRect.x + Padding.Left;
		clientRect.y = initalClientRect.y + Padding.Top;
		clientRect.w -= Padding.Horizontal();
		clientRect.h -= Padding.Vertical();
		for (int i=0; i < controls.Count(); i++)
		{
			if (!controls[i]->Visible)
				continue;
			switch (controls[i]->DockStyle)
			{
			case dsTop:
				controls[i]->Posit(clientRect.x, clientRect.y, clientRect.w, controls[i]->GetHeight());
				clientRect.y += controls[i]->GetHeight();
				clientRect.h -= controls[i]->GetHeight();
				break;
			case dsBottom:
				controls[i]->Posit(clientRect.x, clientRect.y + clientRect.h - controls[i]->GetHeight(), clientRect.w,
					controls[i]->GetHeight());
				clientRect.h -= controls[i]->GetHeight();
				break;
			case dsLeft:
				controls[i]->Posit(clientRect.x, clientRect.y, controls[i]->GetWidth(), clientRect.h);
				clientRect.x += controls[i]->GetWidth();
				clientRect.w -= controls[i]->GetWidth();
				break;
			case dsRight:
				controls[i]->Posit(clientRect.x + clientRect.w - controls[i]->GetWidth(), clientRect.y,
					controls[i]->GetWidth(), clientRect.h);
				clientRect.w -= controls[i]->GetWidth();
				break;
			default:
				break;
			}
		}
		int layoutX = 0;
		int layoutY = 0;
		int maxHeight = 0;
		for (int i = 0; i < controls.Count(); i++)
		{
			if (controls[i]->DockStyle == dsFill)
			{
				controls[i]->Posit(0, 0, clientRect.w, clientRect.h);
			}
		}
		if (layout == ContainerLayoutType::Flow || layout == ContainerLayoutType::Stack)
		{
			for (int i = 0; i < controls.Count(); i++)
			{
				if (controls[i]->DockStyle == dsNone)
				{
					if (layout == ContainerLayoutType::Stack ||
						(layoutX > 0 && layoutX + controls[i]->GetWidth() + controls[i]->Margin.Left > clientRect.w)) // new line
					{
						layoutY += maxHeight;
						layoutX = 0;
						maxHeight = 0;
					}
					controls[i]->Left = layoutX + controls[i]->Margin.Left;
					controls[i]->Top = layoutY + controls[i]->Margin.Top;
					if (layout == ContainerLayoutType::Stack)
					{
						controls[i]->Posit(controls[i]->Left, controls[i]->Top, Width - Padding.Horizontal(), controls[i]->GetHeight());
					}
					layoutX += controls[i]->GetWidth() + controls[i]->Margin.Horizontal();
					maxHeight = Math::Max(maxHeight, controls[i]->GetHeight() + controls[i]->Margin.Vertical());
				}
			}
		}
		if (AutoWidth || AutoHeight)
		{
			int nWidth = 0;
			int nHeight = 0;
			for (int i = 0; i < controls.Count(); i++)
			{
				int cw = controls[i]->GetWidth() + controls[i]->Left + controls[i]->Margin.Right;
				int ch = controls[i]->GetHeight() + controls[i]->Top + controls[i]->Margin.Bottom;
				if (controls[i]->DockStyle == dsLeft || controls[i]->DockStyle == dsRight)
					ch -= clientRect.y;
				if (controls[i]->DockStyle == dsTop || controls[i]->DockStyle == dsBottom)
					cw -= clientRect.x;
				if (cw > nWidth)
					nWidth = cw;
				if (ch > nHeight)
					nHeight = ch;
			}
			nWidth += Padding.Horizontal();
			nHeight += Padding.Vertical();
			if (AutoWidth) Width = nWidth;
			if (AutoHeight) Height = nHeight;
		}
	}

	void Container::SizeChanged()
	{
		Control::SizeChanged();
		ArrangeControls(Rect(0, 0, Width, Height));
	}

	bool Container::DoMouseLeave()
	{
		Control::DoMouseLeave();
		if (!Enabled || !Visible)
			return false;
		return false;
	}

	bool Container::DoDblClick()
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoDblClick();
		return false;
	}
	
	bool Container::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoKeyDown(Key,Shift);
		return false;
	}

	bool Container::DoKeyUp(unsigned short Key, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoKeyUp(Key,Shift);
		return false;
	}

	bool Container::DoKeyPress(unsigned short Key, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoKeyPress(Key,Shift);
		return false;
	}

	void Container::DoFocusChange()
	{
		if (Parent)
			Parent->DoFocusChange();
	}

	void Container::InternalBroadcastMessage(UI_MsgArgs *Args)
	{
		this->HandleMessage(Args);
		for (int i = controls.Count() - 1; i>=0; i--)
		{
			Container * ctn = dynamic_cast<Container *>(controls[i].Ptr());
			if (ctn)
				ctn->InternalBroadcastMessage(Args);
			else
				controls[i]->HandleMessage(Args);
		}
			
	}

	Form::Form(UIEntry * parent)
		: Container(parent)
	{
		Type = CT_FORM;
		Activated = false;
		ButtonClose = true;
		DownInTitleBar = false;
		DownInButton = false;
		BackgroundShadow = true;
		ShadowOffset = 0;
		ShadowSize = 25.0f;
		DownPosX = DownPosY = 0;
		Text = L"Form";
		parent->Forms.Add(this);
		this->content = nullptr;
		btnClose = new Control(this);
		lblTitle = new Label(this);
		lblClose = new Label(this);
		wchar_t CloseSymbol[2] = {114,0}; 
		lblClose->SetText(CloseSymbol);
		lblClose->SetFont(GetEntry()->System->LoadDefaultFont(GraphicsUI::DefaultFontType::Symbol));
		btnClose->Visible = false;
		lblTitle->Visible = false;
		lblClose->Visible = false;
		btnClose->BorderStyle = BS_NONE;
		btnClose->BackColor.A = 0;
		formStyle = Global::Colors.DefaultFormStyle;
		formStyle.TitleFont = parent->GetEntry()->System->LoadDefaultFont(GraphicsUI::DefaultFontType::Title);
		content = new Container(this);
		content->DockStyle = dsFill;
		content->BackColor.A = 0;
		content->BorderStyle = BS_NONE;
		FormStyleChanged();
		SetText(Text);
		Padding = 5;
		Posit(20, 20, 200, 200);
	}

	void Form::SetText(String AText)
	{
		Text = AText;
		lblTitle->SetText(Text);
	}

	String Form::GetText()
	{
		return Text;
	}

	void Form::AddChild(Control * ctrl)
	{
		if (!content)
			Container::AddChild(ctrl);
		else
			content->AddChild(ctrl);
	}

	ContainerLayoutType Form::GetLayout()
	{
		return content->GetLayout();
	}

	void Form::SetLayout(ContainerLayoutType pLayout)
	{
		content->SetLayout(pLayout);
	}

	CoreLib::List<CoreLib::RefPtr<Control>>& Form::GetChildren()
	{
		return content->GetChildren();
	}

	Control * Form::FindControlAtPosition(int x, int y)
	{
		auto ctrl = Container::FindControlAtPosition(x, y);
		if (ctrl)
			return ctrl;
		else
		{
			int additionalMargin = GetEntry()->GetLineHeight() / 2;
			if (x <= -additionalMargin || x - Width >= additionalMargin ||
				y <= -additionalMargin || y - Height >= additionalMargin)
				return nullptr;
			else
				return this;
		}
	}

	int Form::GetClientHeight()
	{
		return Height - Padding.Vertical() - GetTitleBarHeight();
	}

	int Form::GetClientWidth()
	{
		return Width - Padding.Horizontal();
	}

	ResizeMode Form::GetResizeHandleType(int x, int y)
	{
		int handleSize = 4;
		ResizeMode rs = ResizeMode::None;
		if (formStyle.Sizeable)
		{
			if (x <= handleSize)
				rs = (ResizeMode)((int)rs | (int)ResizeMode::Left);
			if (x >= Width - handleSize)
				rs = (ResizeMode)((int)rs | (int)ResizeMode::Right);
			if (y <= handleSize)
				rs = (ResizeMode)((int)rs | (int)ResizeMode::Top);
			if (y >= Height - handleSize)
				rs = (ResizeMode)((int)rs | (int)ResizeMode::Bottom);
		}
		return rs;
	}

	void Form::FormStyleChanged()
	{
		int titleHeight = GetTitleBarHeight();
		lblTitle->SetFont(formStyle.TitleFont);
		lblTitle->FontColor = formStyle.TitleBarFontColor;
		lblClose->FontColor = formStyle.TitleBarFontColor;
		btnClose->Posit(0, Padding.Top, titleHeight - Padding.Right, titleHeight - Padding.Right);
		BackColor = formStyle.BackColor;
		BorderColor = formStyle.BorderColor;
		BorderStyle = BS_FLAT_;
		btnClose->BackColor = formStyle.CtrlButtonBackColor;
		SizeChanged();
	}

	int Form::GetTitleBarHeight()
	{
		return (int)(GetEntry()->GetLineHeight() * formStyle.emTitleBarHeight);
	}

	void Form::SizeChanged()
	{
		int titleHeight = GetTitleBarHeight();
		btnClose->Posit(Width- titleHeight, 3, titleHeight - 4, titleHeight - 4);
		lblClose->Posit(Width- titleHeight + 2, 3, titleHeight - 4, titleHeight - 4);
		Control::SizeChanged();
		OnResize.Invoke(this);
		ArrangeControls(Rect(1, 1 + titleHeight, Width - 2, Height - 2 - titleHeight));
		
	}

	void Form::Draw(int absX,int absY)
	{
		if (!Enabled ||!Visible)
			return;
		int ox=absX, oy=absY;
		absX+=Left; absY+=Top;
		drawChildren = false;
		if (Activated)
		{
			ShadowOpacity = 180;
			ShadowSize = 30.0f;
		}
		else
		{
			ShadowOpacity = 90;
			ShadowSize = 10.0f;
		}
		Container::Draw(ox,oy);
		auto entry = GetEntry();
		//Title bar
		Color *Color = Activated?formStyle.TitleBarColors :formStyle.TitleBarDeactiveColors; 
		auto & graphics = entry->DrawCommands;
		graphics.SolidBrushColor = Color[0];
		int titleHeight = GetTitleBarHeight();
		graphics.FillRectangle(absX + 1, absY + 1, absX + Width - 1, absY + 1 + titleHeight);
		entry->ClipRects->AddRect(Rect(absX,  absY, lblClose->Left - 24, titleHeight));
		lblTitle->Draw(absX+8,absY+1+(titleHeight - lblTitle->GetHeight())/2);
		entry->ClipRects->PopRect();
		//Draw close Button
		if (ButtonClose)
		{
			btnClose->Draw(absX,absY);
			lblClose->Draw(absX,absY);
		}

		//Draw Controls
		entry->ClipRects->AddRect(Rect(absX + Padding.Left, absY + Padding.Top + titleHeight, Width - Padding.Horizontal(), Height - Padding.Vertical() - titleHeight));
		DrawChildren(absX, absY);
		entry->ClipRects->PopRect();
	}

	void Form::SetFormStyle(const FormStyle &AFormStyle)
	{
		formStyle = AFormStyle;
		BackColor = formStyle.BackColor;
		BorderColor = formStyle.BorderColor;
		FormStyleChanged();
	}

	void Form::HandleMessage(const UI_MsgArgs * msg)
	{
		if (msg->Type == MSG_UI_FORM_ACTIVATE)
		{
			auto nxt = FindNextFocus(this);
			if (nxt && nxt->IsChildOf(this))
				nxt->SetFocus();
		}
	}

	bool Form::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled ||!Visible)
			return false;
		Container::DoMouseUp(X-1,Y-1,Shift);
		DownInTitleBar = false;
		resizeMode = ResizeMode::None;
		this->ReleaseMouse();
		if (DownInButton)
		{
			int titleHeight = GetTitleBarHeight();
			if (X > Width - titleHeight && X < Width && Y > 0 && Y < titleHeight + 1)
			{
				GetEntry()->CloseWindow(this);
			}
		}
		if (Left < 0) Left = 0;
		if (Top < 0) Top = 0;
		if (Left > Parent->GetWidth() - 50)
			Left = Parent->GetWidth() - 50;
		if (Top > Parent->GetHeight() - 50)
			Top = Parent->GetHeight() - 50;
		DownInButton = false;
		return true;
	}

	CursorType GetResizeCursor(ResizeMode rm)
	{
		switch (rm)
		{
		case ResizeMode::None:
			return CursorType::Arrow;
		case ResizeMode::Left:
		case ResizeMode::Right:
			return CursorType::SizeWE;
		case ResizeMode::Top:
		case ResizeMode::Bottom:
			return CursorType::SizeNS;
		case ResizeMode::TopLeft:
		case ResizeMode::BottomRight:
			return CursorType::SizeNWSE;
		default:
			return CursorType::SizeNESW;
		}
	}

	bool Form::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled ||!Visible)
			return false;
		Container::DoMouseDown(X-1,Y-1,Shift);
		DownInButton=false;
		DownPosX = X; DownPosY = Y;
		resizeMode = GetResizeHandleType(X, Y);
		if (resizeMode == ResizeMode::None)
		{
			int titleHeight = GetTitleBarHeight();
			if (X > 3 && X < Width - titleHeight && Y > 0 && Y < titleHeight + 1)
			{
				DownInTitleBar = true;
				Global::MouseCaptureControl = this;
			}
			else
			{
				DownInTitleBar = false;
				if (X > Width - titleHeight && X < Width - 2 && Y > 0 && Y < titleHeight + 1)
				{
					DownInButton = true;
					Global::MouseCaptureControl = this;
				}
			}
		}
		else
		{
			GetEntry()->System->SwitchCursor(GetResizeCursor(resizeMode));
			Global::MouseCaptureControl = this;
		}
		return true;
	}

	bool Form::DoMouseMove(int X, int Y)
	{
		const int MinWidth = 120;
		const int MinHeight = GetTitleBarHeight() * 2;

		if (!Enabled ||!Visible)
			return false;
		Container::DoMouseMove(X-1,Y-1);		
		if (resizeMode != ResizeMode::None)
		{
			GetEntry()->System->SwitchCursor(GetResizeCursor(resizeMode));

			if ((int)resizeMode & (int)ResizeMode::Left)
			{
				int dwidth = DownPosX - X;
				if (Width + dwidth < MinWidth)
					dwidth = MinWidth - Width;
				Left -= dwidth;
				Width += dwidth;
			}
			if ((int)resizeMode & (int)ResizeMode::Right)
			{
				int dwidth = X - DownPosX;
				if (Width + dwidth < MinWidth)
					dwidth = MinWidth - Width;
				else
					DownPosX = X;
				Width += dwidth;
			}
			if ((int)resizeMode & (int)ResizeMode::Top)
			{
				int dHeight = DownPosY - Y;
				if (Height + dHeight < MinHeight)
					dHeight = MinHeight - Height;
				Top -= dHeight;
				Height += dHeight;
			}
			if ((int)resizeMode & (int)ResizeMode::Bottom)
			{
				int dHeight = Y - DownPosY;
				if (Height + dHeight < MinHeight)
					dHeight = MinHeight - Height;
				else
					DownPosY = Y;
				Height += dHeight;
			}
			SizeChanged();
		}
		else
		{
			auto rm = GetResizeHandleType(X, Y);
			GetEntry()->System->SwitchCursor(GetResizeCursor(rm));

			if (DownInTitleBar)
			{
				int dx, dy;
				dx = X - DownPosX; dy = Y - DownPosY;
				Left += dx; Top += dy;
			}
		}
		return true;
	}

	bool Form::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		if (!Enabled ||!Visible)
			return false;
		Container::DoKeyDown(Key,Shift);
		return false;
	}

	UIEntry::UIEntry(int WndWidth, int WndHeight, ISystemInterface * pSystem)
		: Container(nullptr)
	{
		this->System = pSystem;
		this->font = pSystem->LoadDefaultFont();
		ClipRects = new ClipRectStack(&DrawCommands);
		Left = Top =0;
		Global::EventGUID = 0;
		Height = WndHeight;
		Width = WndWidth;
		BorderStyle = BS_NONE;
		Type = CT_ENTRY;
		FocusedControl = NULL;
		ClipRects->WindowHeight = WndHeight;
		ClipRects->WindowWidth = WndWidth;
		ActiveForm = 0;
		CheckmarkLabel = new Label(this);
		CheckmarkLabel->AutoSize = true;
		CheckmarkLabel->Visible = false;
		CheckmarkLabel->SetFont(pSystem->LoadDefaultFont(DefaultFontType::Symbol));
		CheckmarkLabel->SetText(L"a");
		
		ImeMessageHandler.Init(this);
		ImeMessageHandler.ImeWindow->Visible = false;
		ImeMessageHandler.ImeWindow->WindowWidth = WndWidth;
		ImeMessageHandler.ImeWindow->WindowHeight = WndHeight;
		DoDpiChanged();
	}

	void UIEntry::InternalBroadcastMessage(UI_MsgArgs *Args)
	{
		//Broadcast to the activated form only.
		if (ActiveForm)
		{
			ActiveForm->InternalBroadcastMessage(Args);
		}
		for (auto & ctrl : controls)
			if (dynamic_cast<Form*>(ctrl.Ptr()) == nullptr)
			{
				if (auto ctn = dynamic_cast<Container*>(ctrl.Ptr()))
					ctn->InternalBroadcastMessage(Args);
				else
					ctrl->HandleMessage(Args);
			}
	}

	void UIEntry::SizeChanged()
	{
		Container::SizeChanged();
		ImeMessageHandler.ImeWindow->WindowWidth = Width;
		ImeMessageHandler.ImeWindow->WindowHeight = Height;
		ClipRects->WindowHeight = Height;
		ClipRects->WindowWidth = Width;
		for (auto & form : Forms)
		{
			if (form->Left + form->GetWidth() > Width - 1)
				form->Left = Width - form->GetWidth() - 1;
			if (form->Top + form->GetHeight() > Height - 1)
				form->Top = Height - form->GetHeight() - 1;
			if (form->Left < 0)
				form->Left = 0;
			if (form->Top < 0)
				form->Top = 0;
		}
	}

	void UIEntry::RemoveForm(Form *Form)
	{
		for (int i=0; i<Forms.Count(); i++)
		{
			if (Forms[i] == Form)
			{
				Forms[i] = 0;
				Forms.RemoveAt(i);
				break;
			}
		}
		this->RemoveChild(Form);
	}

	List<DrawCommand> & UIEntry::DrawUI()
	{
		DrawCommands.ClearCommands();
		Draw(0,0);
		return DrawCommands.Buffer();
	}

	bool UIEntry::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		KeyInputConsumed = true;
		if (Key == 0x09)  // VK_TAB
		{
			if (Shift & SS_CONTROL)
			{
				if (Forms.Count())
				{
					for (int i = 0; i < Forms.Count(); i++)
					{
						if (Forms[i]->Visible && Forms[i]->Enabled)
						{
							ShowWindow(Forms[i]);
							break;
						}
					}
				}
			}
			else
			{
				if (FocusedControl && FocusedControl->WantsTab)
					goto noTabProcess;
				if (Shift == SS_SHIFT)
					MoveFocusBackward();
				else
					MoveFocusForward();
			}
			return true;
		}
		else if (Key == 0x12) // VK_MENU
		{
			Menu * menu = nullptr;
			if (ActiveForm && ActiveForm->MainMenu)
				menu = ActiveForm->MainMenu;
			else if (this->MainMenu)
				menu = this->MainMenu;
			if (menu)
			{
				menu->SetFocus();
				if (menu->Count())
					menu->GetItem(0)->Selected = true;
				return true;
			}
		}
	noTabProcess:;
		auto ctrl = FocusedControl;
		while (ctrl && ctrl != this)
		{
			if (ctrl->DoKeyDown(Key, Shift))
				return true;
			ctrl = ctrl->Parent;
		}
		Control::DoKeyDown(Key, Shift);
		KeyInputConsumed = false;
		return false;
	}

	bool UIEntry::DoKeyUp(unsigned short Key, SHIFTSTATE Shift)
	{
		auto ctrl = FocusedControl;
		while (ctrl && ctrl != this)
		{
			if (ctrl->DoKeyUp(Key, Shift))
				return true;
			ctrl = ctrl->Parent;
		}
		Control::DoKeyUp(Key, Shift);

		return false;
	}

	bool UIEntry::DoKeyPress(unsigned short Key, SHIFTSTATE Shift)
	{
		bool rs = false;
		auto ctrl = FocusedControl;
		while (ctrl && ctrl != this)
		{
			if (ctrl->DoKeyPress(Key, Shift))
				return true;
			ctrl = ctrl->Parent;
		}
		if (ImeMessageHandler.ImeWindow->Visible &&
			(Key == 9 || (Key >= 32 && Key <= 127)))
		{
			ImeMessageHandler.StringInputed(String((wchar_t)(Key)));
			return true;
		}
		Control::DoKeyPress(Key, Shift);

		return rs;
	}

	template<typename Func>
	void BroadcastMouseMessage(List<MouseMessageStack> & stack, int X, int Y, const Func & f)
	{ 
		// mouse messages send to pointed components only
		auto ctrlToBroadcast = Global::MouseCaptureControl ? Global::MouseCaptureControl : Global::PointedComponent;
		if (ctrlToBroadcast)
		{
			stack.Clear();
			while (ctrlToBroadcast)
			{
				MouseMessageStack item;
				item.Ctrl = ctrlToBroadcast;
				stack.Add(item);
				ctrlToBroadcast = ctrlToBroadcast->Parent;
			}
			auto parent = stack.Last().Ctrl;
			int cx = X;
			int cy = Y;
			
			for (int i = stack.Count() - 2; i >= 0; i--)
			{
				auto ctrl = stack[i].Ctrl;
				cx -= ctrl->Left;
				cy -= ctrl->Top;
				if (ctrl->DockStyle == Control::dsNone || ctrl->DockStyle == Control::dsFill)
				{
					cx -= parent->ClientRect().x;
					cy -= parent->ClientRect().y;
				}
				stack[i].X = cx;
				stack[i].Y = cy;
				parent = ctrl;
			}
			for (int i = 0; i < stack.Count() - 1; i++)
				if (f(stack[i].Ctrl, stack[i].X, stack[i].Y))
					break;
		}
	}
	bool UIEntry::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		MouseInputConsumed = true;
		// Detect new active Form.
		Form *nForm=0;
		Global::PointedComponent = FindControlAtPosition(X, Y);
		if (Global::MouseCaptureControl == nullptr)
		{
			DeactivateAllForms();
			for (int i = Forms.Count() - 1; i >= 0; i--)
			{
				Form *curForm = Forms[i];
				if (curForm->Visible && curForm->Enabled && (curForm == Global::PointedComponent || 
					(Global::PointedComponent && Global::PointedComponent->IsChildOf(curForm))))
				{
					ShowWindow(curForm);
					nForm = curForm;
					break;
				}
			}
			if (nForm == 0)
			{
				if (ActiveForm)
				{
					SetFocusedControl(nullptr);
				}
				ActiveForm = 0;
			}
		}
		Global::MouseDownControl = Global::MouseCaptureControl ? Global::MouseCaptureControl : Global::PointedComponent;
		Global::EventGUID ++;
		bool processed = false;
		BroadcastMouseMessage(controlStack, X, Y, [&](Control* ctrl, int x, int y)
		{
			bool rs = ctrl->DoMouseDown(x, y, Shift);
			processed = processed || rs;
			return rs;
		});
		if (!processed && !Global::MouseCaptureControl && Global::PointedComponent == this)
		{
			UIMouseEventArgs e;
			e.Delta = 0;
			e.Shift = Shift;
			e.X = X;
			e.Y = Y;
			Global::MouseCaptureControl = this;
			OnMouseDown.Invoke(this, e);
		}
		MouseInputConsumed = false;
		return false;
	}

	bool UIEntry::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Global::PointedComponent = FindControlAtPosition(X, Y);
		Global::EventGUID++;
		MouseInputConsumed = true;
		bool processed = false;
		BroadcastMouseMessage(controlStack, X, Y, [&](Control* ctrl, int x, int y)
		{
			bool rs = ctrl->DoMouseUp(x, y, Shift);
			processed = processed || rs;
			return rs;
		});
		if (Global::MouseCaptureControl == this || !processed)
		{
			UIMouseEventArgs e;
			e.Delta = 0;
			e.Shift = Shift;
			e.X = X;
			e.Y = Y;
			OnMouseUp.Invoke(this, e);
			ReleaseMouse();
		}
		MouseInputConsumed = false;
		return false;
	}

	bool UIEntry::DoMouseMove(int X, int Y)
	{
		MouseInputConsumed = true;
		auto pointedComp = FindControlAtPosition(X, Y);
		if (pointedComp != Global::PointedComponent)
		{
			if (!Global::MouseCaptureControl)
			{
				auto cur = Global::PointedComponent;
				while (cur && !pointedComp->IsChildOf((Container*)cur))
				{
					cur->DoMouseLeave();
					cur = cur->Parent;
				}
				auto cur2 = pointedComp;
				while (cur2 != cur)
				{
					cur2->DoMouseEnter();
					cur2 = cur2->Parent;
				}
				Global::PointedComponent = pointedComp;
			}
		}
		Global::CursorPosX = X;
		Global::CursorPosY = Y;
		Global::EventGUID++;
		bool processed = false;
		BroadcastMouseMessage(controlStack, X, Y, [&](Control* ctrl, int x, int y)
		{
			bool rs = ctrl->DoMouseMove(x, y);
			processed = processed || rs;
			return rs;
		});
		if (Global::MouseCaptureControl == this || !processed)
		{
			Control::DoMouseMove(X, Y);
			UIMouseEventArgs e;
			e.Delta = 0;
			e.Shift = 0;
			e.X = X;
			e.Y = Y;
			OnMouseMove.Invoke(this, e);
		}
		MouseInputConsumed = false;
		return processed;
	}

	bool GraphicsUI::UIEntry::DoMouseWheel(int delta)
	{
		auto ctrlToBroadcast = Global::MouseCaptureControl ? Global::MouseCaptureControl : Global::PointedComponent;
		while (ctrlToBroadcast && ctrlToBroadcast != this)
		{
			if (ctrlToBroadcast->DoMouseWheel(delta))
				return true;
			ctrlToBroadcast = ctrlToBroadcast->Parent;
		}
		UIMouseEventArgs e;
		e.Delta = delta;
		e.Shift = 0;
		e.X = Global::CursorPosX;
		e.Y = Global::CursorPosY;
		OnMouseWheel.Invoke(this, e);
		return false;
	}

	bool UIEntry::DoMouseHover()
	{
		auto ctrlToBroadcast = Global::MouseCaptureControl ? Global::MouseCaptureControl : Global::PointedComponent;
		while (ctrlToBroadcast && ctrlToBroadcast != this)
		{
			if (ctrlToBroadcast->DoMouseHover())
				return true;
			ctrlToBroadcast = ctrlToBroadcast->Parent;
		}
		OnMouseHover.Invoke(this);
		return false;
	}

	bool UIEntry::DoDblClick()
	{
		auto ctrlToBroadcast = Global::MouseCaptureControl ? Global::MouseCaptureControl : Global::PointedComponent;
		while (ctrlToBroadcast && ctrlToBroadcast != this)
		{
			if (ctrlToBroadcast->DoDblClick())
				return true;
			ctrlToBroadcast = ctrlToBroadcast->Parent;
		}
		OnDblClick.Invoke(this);
		return true;
	}

	bool UIEntry::DoTick()
	{
		for (auto & ctrl : tickEventSubscribers)
			ctrl->DoTick();
		return true;
	}

	void UIEntry::DeactivateAllForms()
	{
		for (int i=0; i<Forms.Count(); i++)
		{
			Forms[i]->Activated = false;
		}
	}

	void UIEntry::ShowWindow(Form * Form)
	{
		if (Form == ActiveForm)
		{
			Form->Activated = true;
			return;
		}
		GraphicsUI::Form* form;
		for (int i=0; i<Forms.Count(); i++)
		{
			if (Forms[i] == Form)
			{
				form = Forms[i];
				Forms.RemoveAt(i);
				break;
			}
		}
		Forms.Add(form);
		if (!Form->Visible)
			Form->OnShow.Invoke(Form);
		Form->Visible = true;
		DeactivateAllForms();
		if (ActiveForm != Form)
		{
			UI_MsgArgs Args;
			Args.Sender = this;
			Args.Type = MSG_UI_FORM_DEACTIVATE;
			if (ActiveForm)
				ActiveForm->HandleMessage(&Args);
			else
				HandleMessage(&Args);
			ActiveForm = Form;
			Args.Type = MSG_UI_FORM_ACTIVATE;
			Form->HandleMessage(&Args);
		}
		ActiveForm = Form;
		Form->Activated = true;
	}

	void UIEntry::CloseWindow(Form *Form)
	{
		if (Form->Visible)
			Form->OnClose.Invoke(Form);
		Form->Visible = false;
			
		FocusedControl = 0;
		ActiveForm = 0;
		for (int i=Forms.Count()-1; i>=0; i--)
		{
			GraphicsUI::Form *curForm;
			curForm = Forms[i];
			if (curForm->Visible)
			{
				ShowWindow(curForm);
				break;
			}
		};
	}

	void UIEntry::HandleMessage(const UI_MsgArgs *Args)
	{
		if (Args->Type == MSG_UI_FORM_DEACTIVATE)
		{
			KillFocus();
			SetFocusedControl(0);
		}
	}

	void UIEntry::DoDpiChanged()
	{
		int nLineHeight = font->MeasureString(L"M").h;
		if (lineHeight != 0)
			dpiScale = nLineHeight / (float)lineHeight;
		Global::DeviceLineHeight = lineHeight = nLineHeight;
		Global::SCROLLBAR_BUTTON_SIZE = (int)(GetLineHeight());
		CheckmarkLabel->DoDpiChanged();
		Container::DoDpiChanged();
	}

	Control * FindNextFocus(Control * ctrl)
	{
		if (auto ctn = dynamic_cast<Container*>(ctrl))
		{
			for (auto & child : ctn->GetChildren())
			{
				if (child->Enabled && child->Visible)
				{
					if (child->TabStop)
						return child.Ptr();
					else if (auto tmpRs = FindNextFocus(child.Ptr()))
						return tmpRs;
				}
			}
				
		}
		auto parent = ctrl->Parent;
		while (parent->GetChildren().Last() == ctrl)
		{
			ctrl = parent;
			parent = ctrl->Parent;
			if (!parent)
				break;
		}
		if (parent)
		{
			int idx = parent->GetChildren().IndexOf(ctrl);
			if (idx == -1)
				return nullptr;
			for (int i = idx + 1; i < parent->GetChildren().Count(); i++)
			{
				if (parent->GetChildren()[i]->Enabled && parent->GetChildren()[i]->Visible)
				{
					if (parent->GetChildren()[i]->TabStop)
						return parent->GetChildren()[i].Ptr();
					else
						return FindNextFocus(parent->GetChildren()[i].Ptr());
				}
			}
		}
		return nullptr;
	}

	Control * GetLastLeaf(Container * ctn)
	{
		if (ctn->GetChildren().Count() == 0)
			return ctn;
		for (int i = ctn->GetChildren().Count() - 1; i >= 0; i--)
		{
			auto ctrl = ctn->GetChildren()[i].Ptr();
			if (ctrl->Visible && ctrl->Enabled)
			{
				if ((ctrl->Type & CT_CONTAINER) != 0)
				{
					return GetLastLeaf(dynamic_cast<Container*>(ctrl));
				}
				else
					return ctrl;
			}
		}
		return ctn;
	}

	Control * FindPreviousFocus(Control * ctrl)
	{
		auto parent = ctrl->Parent;
		while (parent && parent->GetChildren().First() == ctrl)
		{
			ctrl = parent;
			parent = ctrl->Parent;
			if (!parent)
				break;
		}
		if (parent)
		{
			int idx = parent->GetChildren().IndexOf(ctrl);
			for (int i = idx - 1; i >= 0; i--)
			{
				if (parent->GetChildren()[i]->Enabled && parent->GetChildren()[i]->Visible)
				{
					if (parent->GetChildren()[i]->TabStop)
						return parent->GetChildren()[i].Ptr();
					else if (auto ctn = dynamic_cast<Container*>(parent->GetChildren()[i].Ptr()))
					{
						auto last = GetLastLeaf(ctn);
						if (last->Visible && last->Enabled && last->TabStop)
							return last;
						else
							return FindPreviousFocus(GetLastLeaf(ctn));
					}
				}
			}
			return FindPreviousFocus(parent);
		}
		return nullptr;
	}


	void UIEntry::MoveFocusBackward()
	{
		if (FocusedControl)
		{
			auto nxt = FindPreviousFocus(FocusedControl);
			if (!nxt)
			{
				nxt = GetLastLeaf(this);
				if (!nxt->TabStop || !nxt->Enabled || !nxt->Visible)
					nxt = FindPreviousFocus(nxt);
			}
			if (nxt && nxt != FocusedControl)
			{
				FocusedControl->LostFocus(nxt);
				FocusedControl->KillFocus();
				SetFocusedControl(nxt);
			}
		}
	}

	void UIEntry::MoveFocusForward()
	{
		if (FocusedControl)
		{
			auto nxt = FindNextFocus(FocusedControl);
			if (!nxt)
				nxt = FindNextFocus(this);
			if (nxt && nxt != FocusedControl)
			{
				FocusedControl->LostFocus(nxt);
				FocusedControl->KillFocus();
				SetFocusedControl(nxt);
			}
		}
	}

	VectorMath::Vec2i UIEntry::GetCaretScreenPos()
	{
		if (FocusedControl && (FocusedControl->Type & CT_IME_RECEIVER))
		{
			auto receiver = dynamic_cast<ImeCharReceiver*>(FocusedControl);
			if (receiver)
			{
				return receiver->GetCaretScreenPos();
			}
		}
		return VectorMath::Vec2i::Create(0, 0);
	}

	void UIEntry::Draw(int absX,int absY)
	{
		drawChildren = false;
		Container::Draw(absX,absY);
		//Draw Forms
		for (auto children : controls)
		{
			if (children->Visible && children->Type != CT_FORM)
			{
				int dx = 0;
				int dy = 0;
				if (children->DockStyle == dsNone || children->DockStyle == dsFill)
				{
					dx = clientRect.x;
					dy = clientRect.y;
				}
				ClipRects->AddRect(Rect(children->Left + absX + dx, children->Top + absY + dy, children->GetWidth() + 1, children->GetHeight() + 1));
				children->Draw(absX + dx, absY + dy);
				ClipRects->PopRect();
			}
		}
		for (int i=0; i<Forms.Count(); i++)
		{
			Forms[i]->Draw(absX + clientRect.x,absY + clientRect.y);
		}
		//Draw Top Layer Menus
		UI_MsgArgs Arg;
		Arg.Sender = this;
		Arg.Type = MSG_UI_TOPLAYER_DRAW;
		InternalBroadcastMessage(&Arg);

		//Draw IME 
		if (FocusedControl && (FocusedControl->Type & CT_IME_RECEIVER))
		{
			if (ImeMessageHandler.ImeWindow->Visible)
			{
				auto screenPos = ImeMessageHandler.TextBox->GetCaretScreenPos();
				ImeMessageHandler.ImeWindow->Draw(screenPos.x, screenPos.y);
			}
		}
			
	}

	void UIEntry::SetFocusedControl(Control *Target)
	{
		while (Target && !Target->AcceptsFocus)
			Target = Target->Parent;
		if (FocusedControl == Target)
			return;
		if (FocusedControl)
		{
			FocusedControl->LostFocus(Target);
			KillFocus();
		}
		if (!FocusedControl)
			KillFocus();
		auto parent = Target;
		bool formFound = false;
		while (parent)
		{
			if (parent->Type == CT_FORM)
			{
				this->ShowWindow((Form*)(parent));
				formFound = true;
				break;
			}
			parent = parent->Parent;
		}
	
		if (!formFound)
		{
			this->DeactivateAllForms();
			this->ActiveForm = nullptr;
		}
		FocusedControl = Target;
		if (Target && Target->Parent)
			Target->Parent->DoFocusChange();
		if (Target && (Target->Type & CT_IME_RECEIVER) != 0)
		{
			if (Target->Type == CT_IMETEXTBOX)
				ImeMessageHandler.TextBox = (TextBox*)(Target);
			else
				ImeMessageHandler.TextBox = (MultiLineTextBox*)(Target);
		}
		else
		{
			ImeMessageHandler.TextBox = nullptr;
		}
	}

	Control * UIEntry::FindControlAtPosition(int x, int y)
	{
		if (Visible)
		{
			auto checkCtrl = [&](Control * ctrl) -> Control*
			{
				if (!ctrl->Visible)
					return nullptr;
				int dx = 0;
				int dy = 0;
				if (ctrl->DockStyle == dsNone || ctrl->DockStyle == dsFill)
				{
					dx = clientRect.x;
					dy = clientRect.y;
				}
				int nx = x - dx;
				int ny = y - dy;
				if (auto child = ctrl->FindControlAtPosition(nx - ctrl->Left, ny - ctrl->Top))
					return child;
				return nullptr;
			};
			for (int i = Forms.Count() - 1; i >= 0; i--)
			{
				if (auto rs = checkCtrl(Forms[i]))
					return rs;
			}
			for (int i = controls.Count() - 1; i >= 0; i--)
			{
				if (auto rs = checkCtrl(controls[i].Ptr()))
					return rs;
			}
			return this;
		}
		return nullptr;
	}

	CheckBox::CheckBox(Container * parent)
		: Label(parent)
	{
		FontColor = Global::Colors.MenuItemForeColor;
		BackColor = Global::Colors.EditableAreaBackColor;
		TabStop = true;
		Type = CT_CHECKBOX;
		BorderStyle = BS_FLAT_;
		BackColor.A = 0;
		Checked = false;
	}

	CheckBox::CheckBox(Container * parent, const CoreLib::String & text, bool checked)
		: CheckBox(parent)
	{
		SetText(text);
		Checked = checked;
	}


	void CheckBox::ComputeAutoSize()
	{
		if (AutoSize)
		{
			this->Width = TextWidth + (int)(GetEntry()->CheckmarkLabel->TextWidth * 1.5f) + 2;
			this->Height = TextHeight + 1;
		}
	}

	void CheckBox::DoDpiChanged()
	{
		Label::DoDpiChanged();
		ComputeAutoSize();
	}

	void CheckBox::SetText(const CoreLib::String & pText)
	{
		Label::SetText(pText);
		ComputeAutoSize();
	}

	void CheckBox::Draw(int absX, int absY)
	{
		auto oldBorderStyle = BorderStyle;
		BorderStyle = BS_NONE;
		Control::Draw(absX, absY);
		BorderStyle = oldBorderStyle;
		absX = absX + Left;
		absY = absY + Top;
		auto entry = GetEntry();
		int checkBoxSize = GetEntry()->CheckmarkLabel->TextWidth;
		int checkBoxTop = (Height - checkBoxSize) >> 1;
		auto & graphics = entry->DrawCommands;
		graphics.SolidBrushColor = Global::Colors.EditableAreaBackColor;
		graphics.FillRectangle(absX + 1, absY + checkBoxTop + 1, absX + checkBoxSize, absY + checkBoxTop + checkBoxSize);
		//Draw Check Box
		Color lightColor, darkColor;
		if (BorderStyle == BS_LOWERED)
		{
			lightColor.R = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.R + COLOR_LIGHTEN, 0, 255);
			lightColor.G = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.G + COLOR_LIGHTEN, 0, 255);
			lightColor.B = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.B + COLOR_LIGHTEN, 0, 255);
			lightColor.A = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.A + COLOR_LIGHTEN, 0, 255);
			darkColor.R = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.R - COLOR_LIGHTEN, 0, 255);
			darkColor.G = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.G - COLOR_LIGHTEN, 0, 255);
			darkColor.B = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.B - COLOR_LIGHTEN, 0, 255);
			darkColor.A = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.A + COLOR_LIGHTEN, 0, 255);
		}
		else
		{
			lightColor = darkColor = Global::Colors.ControlBorderColor;
		}
		graphics.PenColor = darkColor;
		graphics.DrawLine(absX, absY + checkBoxTop, absX + checkBoxSize, absY + checkBoxTop);
		graphics.DrawLine(absX, absY + checkBoxTop + 1, absX, absY + checkBoxSize + checkBoxTop);
		graphics.PenColor = lightColor;
		graphics.DrawLine(absX + checkBoxSize, absY + checkBoxTop, absX + checkBoxSize, absY + checkBoxSize + checkBoxTop);
		graphics.DrawLine(absX + checkBoxSize - 1, absY + checkBoxSize + checkBoxTop, absX + 1, absY + checkBoxSize + checkBoxTop);
		// Draw check mark
		if (Checked)
		{
			auto checkMark = entry->CheckmarkLabel;
			checkMark->FontColor = FontColor;
			checkMark->Draw(absX + (checkBoxSize - checkMark->TextWidth) / 2 + 1,absY + checkBoxTop + (checkBoxSize - checkMark->TextHeight) / 2 - 1);
		}
		//Draw Caption
		int textStart = checkBoxSize + checkBoxSize / 4;
		BorderStyle = BS_NONE;
		Label::Draw(absX + textStart - Left, absY - Top);
		BorderStyle = oldBorderStyle;
		// Draw Focus Rect
		if (IsFocused())
		{
			graphics.PenColor = Global::Colors.FocusRectColor;
			graphics.DrawRectangle(absX + textStart, absY, absX + text->GetWidth() + textStart, absY + text->GetHeight());
		}
	}

	bool CheckBox::DoDblClick()
	{
		Control::DoDblClick();
		CheckBox::DoMouseDown(1, 1, 0);
		return true;
	}

	bool CheckBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Label::DoMouseDown(X,Y,Shift);
		if (!Enabled || !Visible)
			return false;
		Checked = !Checked;
		UI_MsgArgs Args;
		Args.Sender = this;
		Args.Type = MSG_UI_CHANGED;
		BroadcastMessage(&Args);
		return true;
	}

	bool CheckBox::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key,Shift);
		if (!Enabled || !Visible)
			return false;
		if (Key == 0x20 || Key == 0x0D) // VK_SPACE, VK_RETURN
		{
			Checked = !Checked; 
			UI_MsgArgs Args;
			Args.Sender = this;
			Args.Type = MSG_UI_CHANGED;
			BroadcastMessage(&Args);
		}
		return false;
	}

	RadioBox::RadioBox(Container * parent)
		: CheckBox(parent)
	{
		Type = CT_RADIOBOX;
	}

	bool RadioBox::GetValue()
	{
		return Checked;
	}

	void RadioBox::SetValue(bool AValue)
	{
		if (AValue)
		{
			if (Parent && (Parent->Type & CT_CONTAINER) != 0) // A type less then zero means the control is a container. 
			{
				for (int i=0; i<((Container * )Parent)->GetChildren().Count(); i++)
				{
					Control * curControl = ((Container * )Parent)->GetChildren()[i].Ptr();
					if (curControl->Type == CT_RADIOBOX)
					((RadioBox *)curControl)->Checked = false;
				}
				Checked = true;
			}
		}
	}

	void RadioBox::Draw(int absX, int absY)
	{
		auto oldBorderStyle = BorderStyle;
		BorderStyle = BS_NONE;
		Control::Draw(absX,absY);
		BorderStyle = oldBorderStyle;
		absX = absX + Left;
		absY = absY + Top;
		auto entry = GetEntry();
		int checkBoxSize = GetEntry()->CheckmarkLabel->TextWidth;
		int rad = checkBoxSize / 2 + 1;
		int dotX = absX + rad;
		int dotY = absY + (Height >> 1);
		auto & graphics = entry->DrawCommands;
		graphics.SolidBrushColor = Global::Colors.EditableAreaBackColor;
		graphics.FillEllipse((float)dotX - rad, (float)dotY - rad, (float)dotX + rad, (float)dotY + rad);

		if (BorderStyle == BS_LOWERED)
		{
			//Draw Check Box
			Color LightColor, DarkColor;
			LightColor.R = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.R + COLOR_LIGHTEN, 0, 255);
			LightColor.G = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.G + COLOR_LIGHTEN, 0, 255);
			LightColor.B = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.B + COLOR_LIGHTEN, 0, 255);
			LightColor.A = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.A + COLOR_LIGHTEN, 0, 255);
			DarkColor.R = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.R - COLOR_LIGHTEN, 0, 255);
			DarkColor.G = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.G - COLOR_LIGHTEN, 0, 255);
			DarkColor.B = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.B - COLOR_LIGHTEN, 0, 255);
			DarkColor.A = (unsigned char)ClampInt(Global::Colors.ControlBorderColor.A + COLOR_LIGHTEN, 0, 255);
			graphics.PenColor = DarkColor;
			graphics.DrawArc(dotX, dotY, rad, Math::Pi / 4, Math::Pi * 5 / 4);
			graphics.PenColor = LightColor;
			graphics.DrawArc(dotX, dotY, rad, PI * 5 / 4, PI * 9 / 4);
		}
		else
		{
			graphics.PenColor = Global::Colors.ControlBorderColor;
			graphics.DrawArc(dotX, dotY, rad, 0.0f, Math::Pi * 2.0f);
		}
		float dotRad = rad * 0.5f;
		if (Checked)
		{
			// Draw dot
			graphics.SolidBrushColor = Global::Colors.ControlFontColor;
			graphics.FillEllipse((dotX + 0.5f - dotRad), (dotY + 0.5f - dotRad), (dotX + dotRad), (dotY + dotRad));
		}
		//Draw Caption
		int textStart = checkBoxSize + checkBoxSize / 4;
		BorderStyle = BS_NONE;
		Label::Draw(absX + textStart - Left, absY - Top);
		BorderStyle = oldBorderStyle;
		// Draw Focus Rect
		if (IsFocused())
		{
			graphics.PenColor = Global::Colors.FocusRectColor;
			graphics.DrawRectangle(absX + textStart, absY, absX + text->GetWidth() + textStart, absY + text->GetHeight());
		}
	}

	bool RadioBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X,Y,Shift);
		if (!Enabled || !Visible)
			return false;
		SetValue(true);
		return true;
	}

	bool RadioBox::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key,Shift);
		if (!Enabled || !Visible)
			return false;
		if (Key == 0x20 || Key == 0x0D) // VK_SPACE, VK_RETURN
		{
			SetValue(true);
		}
		return true;
	}

	CustomTextBox::CustomTextBox(Container * parent)
		: Container(parent)
	{
		Cursor = CursorType::IBeam;
		Type = CT_TEXTBOX;
		FText= "";
		font = parent->GetFont();
		SelectMode = false;
		TabStop = true;
		Locked = false; Changed = true;
		SelStart = SelLength = SelOrigin = 0;
		SelectionColor = Global::Colors.SelectionColor;
		SelectedTextColor = Global::Colors.SelectionForeColor;
		BorderStyle = BS_FLAT_;
		BackColor = Global::Colors.EditableAreaBackColor;
		FontColor = Global::Colors.ControlFontColor;
		TextBorderX =2; TextBorderY = 4;
		LabelOffset = TextBorderX;
		menu = new Menu(this, Menu::msPopup);
		auto mnCut = new MenuItem(menu, L"Cut", L"Ctrl+X");
		auto mnCopy = new MenuItem(menu, L"Copy", L"Ctrl+C");
		auto mnPaste = new MenuItem(menu, L"Paste", L"Ctrl+V");
		auto mnSelAll = new MenuItem(menu, L"Select All", L"Ctrl+A");
		mnCut->OnClick.Bind([this](auto) 
		{
			CopyToClipBoard();
			DeleteSelectionText();
		});
		mnCopy->OnClick.Bind([this](auto)
		{
			CopyToClipBoard();
		});
		mnPaste->OnClick.Bind([this](auto)
		{
			PasteFromClipBoard();
		});
		mnSelAll->OnClick.Bind([this](auto)
		{
			SelectAll();
		});
		time = CoreLib::Diagnostics::PerformanceCounter::Start();
		CursorPos = 0;
		KeyDown = false;
		DoDpiChanged();
	}

	const String CustomTextBox::GetText()
	{
		return FText;
	}

	void CustomTextBox::SetText(const String & AText)
	{
		FText = AText;
		Changed = true;
		CursorPos = FText.Length();
		SelLength = 0;
		OnChanged.Invoke(this);
	}

	void CustomTextBox::SetFont(IFont * AFont)
	{
		this->font = AFont;
		Changed = true;
	}

	void CustomTextBox::CursorPosChanged()
	{
		//Calculate Text offset.
		int txtWidth = font->MeasureString(FText).w;
		if (txtWidth <= Width-TextBorderX*2)
		{
			LabelOffset = TextBorderX;
		}
		else
		{
			String ls;
			ls = FText.SubString(0, CursorPos);
			int px = font->MeasureString(ls).w+LabelOffset;
			if (px>Width-TextBorderX)
			{
				int delta = px-(Width-TextBorderX);
				LabelOffset-=delta;
			}
			else if (px<TextBorderX && LabelOffset<2)
			{
				LabelOffset += 40;
				if (LabelOffset>2)
					LabelOffset = 2;
			}
		}
	}

	int CustomTextBox::HitTest(int posX)
	{
		String curText;
		posX -= LabelOffset;
		curText = "";
		for (int i =0;i<FText.Length();i++)
		{
			curText = curText + FText[i];
			int tw = font->MeasureString(curText).w;
			if (tw>posX)
			{
				int cw = font->MeasureString(FText[i]).w;
				cw /=2;
				if (tw-cw>posX)
				{
					return i;
				}
				else
				{
					return i+1;
				}
			}
		}
		return FText.Length();
	}


	String DeleteString(String src, int start, int len)
	{
		return src.SubString(0, start) + src.SubString(start + len, src.Length() - start - len);
	}


	bool CustomTextBox::DoInput(const String & AInput)
	{
		if (AInput == L"\t")
			return false;
		if (Locked)
			return true;
		String nStr;
		nStr = AInput;
		if (SelLength !=0)
		{
			DeleteSelectionText();
		}
		if (CursorPos!=FText.Length())
			FText = FText.SubString(0, CursorPos) + nStr + FText.SubString(CursorPos, FText.Length() - CursorPos);
		else
			FText = FText + AInput;
		TextChanged();
		CursorPos += nStr.Length();
		SelStart = CursorPos;
		return true;
	}

	void CustomTextBox::CopyToClipBoard()
	{
		if( SelLength != 0)
		{
			GetEntry()->System->SetClipboardText(FText.SubString(SelStart, SelLength));
		}
	}
	void CustomTextBox::PasteFromClipBoard()
	{
		DeleteSelectionText();
		auto txt = GetEntry()->System->GetClipboardText();
		int fid = txt.IndexOf(L'\r');
		if (fid != -1)
			txt = txt.SubString(0, fid);
		fid = txt.IndexOf(L'\n');
		if (fid != -1)
			txt = txt.SubString(0, fid);
		DoInput(txt);
	}


	void CustomTextBox::DeleteSelectionText()
	{
		if (SelLength != 0 && !Locked)
		{
			if (SelStart + SelLength > FText.Length())
				SelLength = FText.Length() - SelStart;
			FText = DeleteString(FText, SelStart, SelLength);
			TextChanged();
			SelLength=0;
			CursorPos = SelStart;
		}
	}

	bool CustomTextBox::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key,Shift);
		if (Enabled && Visible)
		{
			KeyDown=true;
			if (Shift==SS_SHIFT)
			{
				int selEnd;
				selEnd = SelStart+SelLength;
				if (Key == 0x25) // VK_LEFT
				{
					if (CursorPos==0)
						return false;
					CursorPos --;
					if (CursorPos<SelStart)
					{
						SelStart = CursorPos;
						SelLength = selEnd-CursorPos;
					}
					else if (CursorPos>SelStart)
						SelLength = CursorPos-SelStart;
					else
					{
						SelStart=CursorPos;
						SelLength =0;
					}
					cursorPosChanged = true;
				}
				else if(Key == 0x27) // VK_RIGHT
				{
					if (CursorPos==FText.Length())
						return false;
					CursorPos ++;
					if (CursorPos<selEnd)
					{
						SelStart = CursorPos;
						SelLength = selEnd-CursorPos;
					}
					else if (CursorPos>selEnd)
						SelLength = CursorPos-SelStart;
					else
					{
						SelStart= CursorPos;
						SelLength = 0;
					}	
					cursorPosChanged = true;
				}
				return true;
			}
			else if (Shift == SS_CONTROL)
			{
				if (Key == L'C')
				{
					CopyToClipBoard();
				}
				else if (Key == L'V')
				{
					DeleteSelectionText();
					if (!Locked)
						PasteFromClipBoard();
				}
				else if (Key == L'X')
				{
					CopyToClipBoard();
					DeleteSelectionText();
				}
				else if (Key == L'A')
				{
					SelectAll();
				}
				return true;
			}
			else if (Shift == 0)
			{
				if (Key == 0x25) // VK_LEFT
				{
					if (SelLength == 0)
						CursorPos--;
					else
					{
						CursorPos = SelStart;
					}
					SelLength = 0;
					SelStart = CursorPos=ClampInt(CursorPos,0,FText.Length());
					cursorPosChanged = true;
					return true;
				}			
				else if (Key == 0x27) // VK_RIGHT
				{
					if (SelLength ==0)
						CursorPos++;
					else
						CursorPos = SelStart+SelLength;
					SelLength = 0;
					SelStart = CursorPos=ClampInt(CursorPos,0,FText.Length());
					cursorPosChanged = true;
					return true;
				}
				else if (Key == 0x2E && !Locked) // VK_DELETE
				{
					if (SelLength!=0)
					{
						FText = DeleteString(FText, SelStart, SelLength);
						TextChanged();
						SelLength=0;
						CursorPos = SelStart;
						cursorPosChanged = true;
					}
					else if (CursorPos<(int)FText.Length())
					{
						FText = DeleteString(FText, CursorPos, 1);
						TextChanged();
					}
					return true;
				}
				else if (Key == 0x08 && !Locked) // VK_BACK
				{
					if (SelLength != 0)
					{
						DeleteSelectionText();
						cursorPosChanged = true;
					}
					else if (CursorPos>0)
					{
						FText = DeleteString(FText, CursorPos-1, 1);
						CursorPos--;
						TextChanged();
					}
					return true;
				}
			}
		}
		return true;
	}

	bool CustomTextBox::DoKeyPress(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyPress(Key,Shift);
		if ((Shift & SS_CONTROL) == 0)
		{
			if (Key >= 32)
			{
				DoInput((wchar_t)Key);
				return true;
			}
		}
		return true;
	}

	void CustomTextBox::TextChanged()
	{
		cursorPosChanged = true;
		Changed = true;
		UI_MsgArgs Args;
		Args.Sender = this;
		Args.Type = MSG_UI_CHANGED;
		BroadcastMessage(&Args);
	}

	bool CustomTextBox::DoKeyUp(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyUp(Key,Shift);
		KeyDown = false;
		return true;
	}

	bool CustomTextBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X, Y, Shift);
		if (Enabled && Visible)
		{
			time = CoreLib::Diagnostics::PerformanceCounter::Start();
			SetFocus();
			if (Shift & SS_BUTTONLEFT)
			{
				SelLength = 0;
				SelStart = HitTest(X);
				CursorPos = SelStart;
				SelectMode = true;
				SelOrigin = CursorPos;
				cursorPosChanged = true;
				Global::MouseCaptureControl = this;
			}
			return true;
		}
		else
			SelectMode = false;
		return false;
	}

	bool CustomTextBox::DoMouseMove(int X , int Y)
	{
		Control::DoMouseMove(X,Y);
		if (Enabled && Visible)
		{
			if (SelectMode)
			{
				int cp = HitTest(X);
				if (cp < SelOrigin)
				{
					SelStart = cp;
					SelLength = SelOrigin - cp;
				}
				else if (cp >= SelOrigin)
				{
					SelStart = SelOrigin;
					SelLength = cp - SelOrigin;
				}
				CursorPos = cp;
				cursorPosChanged = true;
			}
			return true;
		}
		return false;
	}

	bool CustomTextBox::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X,Y,Shift);
		SelectMode = false;
		ReleaseMouse();
		if (Enabled && Visible)
		{
			if (Shift == SS_BUTTONRIGHT)
			{
				menu->Popup(X, Y);
			}
			return true;
		}
		return false;
	}

	void CustomTextBox::SelectAll()
	{
		SelStart = 0;
		SelLength = FText.Length();
	}

	void CustomTextBox::Posit(int pLeft, int pTop, int pWidth, int /*pHeight*/)
	{
		Control::Posit(pLeft, pTop, pWidth, Height);
	}

	void CustomTextBox::DoDpiChanged()
	{
		Changed = true;
		if (font)
			Height = (int)(font->MeasureString(L"M").h * 1.2f);
		Container::DoDpiChanged();
	}

	bool IsSeparatorChar(wchar_t ch)
	{
		bool isLetter = ((ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == L'_');
		return !isLetter;
	}

	bool CustomTextBox::DoDblClick()
	{
		if (CursorPos >= FText.Length())
			SelectAll();
		else
		{
			int begin = CursorPos;
			while (begin > 0 && !IsSeparatorChar(FText[begin - 1]))
				begin--;
			int end = CursorPos;
			while (end < FText.Length() && !IsSeparatorChar(FText[end]))
				end++;
			SelStart = begin;
			SelLength = end - begin;
			CursorPos = end;
			cursorPosChanged = true;
		}
		return true;
	}

	void CustomTextBox::Draw(int absX, int absY)
	{
		Control::Draw(absX,absY);
		absX+=Left; absY+=Top;	
		auto entry = GetEntry();
		if (font == nullptr)
		{
			font = entry->System->LoadDefaultFont();
			Changed = true;
		}
		if (cursorPosChanged)
		{
			cursorPosChanged = false;
			CursorPosChanged();
		}
		if (Changed)
		{
			text = font->BakeString(FText);
			Changed = false;
		}
		auto & graphics = entry->DrawCommands;
		//Draw Text
		Rect textRect;
		textRect.x = absX+TextBorderX; textRect.y = absY; textRect.w = Width-TextBorderX-TextBorderX; textRect.h = Height;
		entry->ClipRects->AddRect(textRect);
		graphics.SolidBrushColor = FontColor;
		graphics.DrawTextQuad(text.Ptr(), absX+LabelOffset,absY+TextBorderY);
		entry->ClipRects->PopRect();
		String ls;
		ls= FText.SubString(0, CursorPos);
		
		int spX=0,epX=0;
		//Draw Selection Rect
			
		if ((IsFocused() || menu->Visible) && SelLength!=0)
		{
			if (SelStart + SelLength > FText.Length())
				SelLength = FText.Length() - SelStart;
			ls = FText.SubString(0, SelStart);
			spX = font->MeasureString(ls).w;
			ls = FText.SubString(0, SelStart+SelLength);
			epX = font->MeasureString(ls).w;
			spX+=LabelOffset+absX; epX+=LabelOffset+absX;
			graphics.SolidBrushColor = SelectionColor;
			graphics.FillRectangle(spX, absY + TextBorderX, epX - 1, absY + Height - TextBorderX);
			entry->ClipRects->AddRect(Rect(spX, absY + TextBorderX, epX - 1 - spX, Height - TextBorderX));
			graphics.SolidBrushColor = Color(255, 255, 255, 255);
			graphics.DrawTextQuad(text.Ptr(), absX + LabelOffset, absY + TextBorderY);
			entry->ClipRects->PopRect();
		}
			
		//Draw Cursor
		float timePassed = CoreLib::Diagnostics::PerformanceCounter::EndSeconds(time);
		int tick = int(timePassed / CURSOR_FREQUENCY);
		if (IsFocused() && ((tick&1)==0 || KeyDown))
		{
			int csX = font->MeasureString(ls).w;
			csX += LabelOffset;
			AbsCursorPosX = absX+csX;
			AbsCursorPosY = absY+Height-TextBorderX;
			graphics.PenColor = Color(255 - BackColor.R, 255 - BackColor.G, 255 - BackColor.B, 255);
			graphics.DrawLine(AbsCursorPosX, absY + TextBorderX, AbsCursorPosX, AbsCursorPosY);
		}

	}

	void TextBox::ImeInputString(const String & txt)
	{
		DoInput(txt);
	}

	bool TextBox::DoKeyPress(unsigned short Key, SHIFTSTATE Shift)
	{
		CustomTextBox::DoKeyPress(Key,Shift);
		if (!IsFocused() || !Enabled ||!Visible)
			return false;
		return false;
	}

	VectorMath::Vec2i TextBox::GetCaretScreenPos()
	{
		return Vec2i::Create(AbsCursorPosX, AbsCursorPosY);
	}

	IMEWindow::IMEWindow(Container * parent)
		: Container(parent)
	{
		lblCompStr = new Label(this);
		Panel = new Control(this);
		Panel->BorderStyle = BS_FLAT_;
	}

	IMEWindow::~IMEWindow()
	{
	}

	void IMEWindow::ChangeCompositionString(String AString)
	{
		lblCompStr->SetText(AString);
		strComp = AString;
	}

	void IMEWindow::Draw(int absX, int absY)
	{
		int maxW=0;
		int height;
		int cpx,cpy;
		const int panelMargin = 4;
		absX += panelMargin;
		if (strComp.Length())
		{
			if (lblCompStr->TextWidth+absX > WindowWidth)
				cpx = WindowWidth-lblCompStr->TextWidth;
			else
				cpx=absX;
			if (lblCompStr->TextHeight+absY>WindowHeight)
				cpy = absY - 40;
			else
				cpy = absY;
			height = lblCompStr->TextHeight;
			maxW = lblCompStr->TextWidth;
			Panel->Left = cpx - panelMargin;
			Panel->Top = cpy - panelMargin;
			Panel->SetWidth(maxW + panelMargin * 2);
			Panel->SetHeight(height + panelMargin * 2);
			Panel->Draw(0,0);
			lblCompStr->Draw(cpx,cpy);		
		}
	}
	
	void IMEHandler::Init(UIEntry * entry)
	{
		TextBox = NULL;
		ImeWindow = new GraphicsUI::IMEWindow(entry);
	}

	bool IMEHandler::DoImeStart()
	{
		ImeWindow->ChangeCompositionString(String(L""));
		ImeWindow->Visible = true;
		return true;
	}

	bool IMEHandler::DoImeEnd()
	{
		ImeWindow->Visible = false;
		return true;
	}

	bool IMEHandler::DoImeCompositeString(const CoreLib::String & str)
	{
		ImeWindow->ChangeCompositionString(str);
		return false;
	}

	bool IMEHandler::DoImeResultString(const CoreLib::String & str)
	{
		StringInputed(str);
		return true;
	}

	void IMEHandler::StringInputed(String AString)
	{
		if (TextBox)
			TextBox->ImeInputString(AString);
	}

	ScrollBar::ScrollBar(Container * parent, bool addToParent)
		: Container(parent, addToParent)
	{
		Type = CT_SCROLLBAR;
		BorderStyle = BS_NONE;
		BackColor = Global::Colors.ScrollBarBackColor;
		btnInc = new Button(this);
		btnDec = new Button(this);
		Slider = new Control(this);
		btnInc->TabStop = false;
		btnDec->TabStop = false;
		btnInc->BackColor.A = 0;
		btnDec->BackColor.A = 0;
		btnInc->SetFont(GetEntry()->System->LoadDefaultFont(DefaultFontType::Symbol));
		btnDec->SetFont(GetEntry()->System->LoadDefaultFont(DefaultFontType::Symbol));
		Min = 0; Max = 100; Position = 0;
		btnInc->OnMouseDown.Bind(this, &ScrollBar::BtnIncMouseDown);
		btnDec->OnMouseDown.Bind(this, &ScrollBar::BtnDecMouseDown);
		btnInc->OnMouseUp.Bind(this, &ScrollBar::BtnIncMouseUp);
		btnDec->OnMouseUp.Bind(this, &ScrollBar::BtnDecMouseUp);
		btnInc->BorderStyle = BS_NONE;
		btnDec->BorderStyle = BS_NONE;
		btnInc->BorderColor.A = 0;
		btnDec->BorderColor.A = 0;
		Slider->BorderStyle = BS_NONE;
		Slider->BackColor = Global::Colors.ScrollBarSliderColor;
		btnInc->FontColor = btnDec->FontColor = Global::Colors.ScrollBarForeColor;
		Slider->OnMouseEnter.Bind([this](auto) {Slider->BackColor = Global::Colors.ScrollBarHighlightColor; });
		Slider->OnMouseLeave.Bind([this](auto) {Slider->BackColor = Global::Colors.ScrollBarForeColor; });

		SetOrientation(SO_HORIZONTAL);
		SetValue(0,100,0,20);
		SmallChange = 1;
		LargeChange = 10;
		DownInSlider = false;
		
	}

	ScrollBar::ScrollBar(Container * parent)
		: ScrollBar(parent, true)
	{
	}

	ScrollBar::~ScrollBar()
	{
	}

	void ScrollBar::Draw(int absX, int absY)
	{
		if (!Visible) return;
		Control::Draw(absX,absY);
		absX+=Left; absY+=Top;
		if (DownInSlider)
			Slider->BackColor = Global::Colors.ScrollBarPressedColor;
		else
		{
			if (highlightSlider)
				Slider->BackColor = Global::Colors.ScrollBarHighlightColor;
			else
				Slider->BackColor = Global::Colors.ScrollBarSliderColor;
		}
		btnInc->Draw(absX,absY);
		btnDec->Draw(absX,absY);
		if (Slider->Visible)
			Slider->Draw(absX, absY);
	}

	void ScrollBar::SetOrientation(int NewOri)
	{
		Orientation = NewOri;
		Position = Min;
		SetValue(Min,Max,Position, PageSize);
		if (NewOri == SO_HORIZONTAL)
		{
			Height = Global::SCROLLBAR_BUTTON_SIZE;
			btnInc->SetText(L"4"); 
			btnDec->SetText(L"3");
		}
		else
		{
			Width = Global::SCROLLBAR_BUTTON_SIZE;
			btnInc->SetText(L"6"); 
			btnDec->SetText(L"5");
		}
		SizeChanged();
	}

	void ScrollBar::SizeChanged()
	{
		Control::SizeChanged();
		if (Orientation == SO_HORIZONTAL)
		{
			btnDec->Posit(0,0,Global::SCROLLBAR_BUTTON_SIZE,Height);
			btnInc->Posit(Width- Global::SCROLLBAR_BUTTON_SIZE,0, Global::SCROLLBAR_BUTTON_SIZE,Height);
			Slider->Posit(Global::SCROLLBAR_BUTTON_SIZE,0,PageSize,Height);
		}
		else
		{
			btnDec->Posit(0,0,Width, Global::SCROLLBAR_BUTTON_SIZE);
			btnInc->Posit(0,Height- Global::SCROLLBAR_BUTTON_SIZE,Width, Global::SCROLLBAR_BUTTON_SIZE);
			Slider->Posit(0, Global::SCROLLBAR_BUTTON_SIZE,Width,PageSize);
		}
		SetValue(Min,Max,Position, PageSize);
	}

	void ScrollBar::DoDpiChanged()
	{
		Container::DoDpiChanged();
		SizeChanged();
	}

	int ScrollBar::GetOrientation()
	{
		return Orientation;
	}

	int ScrollBar::GetMax()
	{
		return Max;
	}

	int ScrollBar::GetMin()
	{
		return Min;
	}

	int ScrollBar::GetPosition()
	{
		return Position;
	}
	int ScrollBar::GetPageSize()
	{
		return PageSize;
	}
	void ScrollBar::SetMax(int AMax)
	{
		SetValue(Min,AMax,Position, PageSize);
	}

	void ScrollBar::SetMin(int AMin)
	{
		SetValue(AMin,Max,Position, PageSize);
	}

	void ScrollBar::SetPosition(int APos)
	{
		SetValue(Min,Max,APos, PageSize);
	}

	void ScrollBar::SetValue(int AMin, int AMax, int APos, int pageSize)
	{
		int FreeSlide = (Orientation==SO_HORIZONTAL)?Width-(Global::SCROLLBAR_BUTTON_SIZE+1)*2:
														Height-(Global::SCROLLBAR_BUTTON_SIZE+1)*2;
		if (AMin>=0 && AMax>AMin && APos>=AMin && APos<=AMax)
		{
			bool Changed = (AMin != Min || AMax !=Max || APos !=Position);
			Min = AMin;
			Max = AMax;
			Position = APos;
			if (Changed)
			{
				UI_MsgArgs Args;
				Args.Sender = this;
				Args.Type = MSG_UI_CHANGED;
				BroadcastMessage(&Args);
				OnChanged.Invoke(this);
			}
				
			PageSize = pageSize;
			float p = PageSize/(float)(PageSize+AMax-AMin);
			int slideSize = Math::Max((int)(p*FreeSlide), GetEntry()->GetLineHeight());
			int spos = (int)((FreeSlide - slideSize)*(APos/(float)(AMax-AMin))) + Global::SCROLLBAR_BUTTON_SIZE + 1;
			if (Orientation == SO_HORIZONTAL)
			{	
				Slider->Left = spos;
				Slider->SetWidth(slideSize);
			}
			else
			{
				Slider->Top = spos;
				Slider->SetHeight(slideSize);
			}
			Slider->Visible = true;
		}
		else
		{
			Slider->Visible = false;
			Min = Max = Position = 0;
		}
	}

	bool ScrollBar::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoMouseDown(X,Y,Shift);
		DownInSlider = false;
		DownPosX = X; DownPosY=Y;
		if (PointInSlider(X,Y))
		{
			DownInSlider = true;
			OriPos = Position;
			Global::MouseCaptureControl = this;
		}
		else if (PointInFreeSpace(X,Y))
		{
			int nPos = Position;
			if (Orientation == SO_HORIZONTAL)
			{
				if (X>Slider->Left)
					nPos += LargeChange;
				else
					nPos -= LargeChange;
			}
			else
			{
				if (Y>Slider->Top)
					nPos += LargeChange;
				else
					nPos -= LargeChange;
			}
			nPos = Math::Clamp(nPos, Min, Max);
			SetPosition(nPos);
		}
		auto hitTest = Container::FindControlAtPosition(X, Y);
		if (hitTest == btnDec || hitTest == btnInc)
			hitTest->DoMouseDown(X - hitTest->Left, Y - hitTest->Top, Shift);
		return true;
	}

	bool ScrollBar::DoMouseMove(int X, int Y)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoMouseMove(X, Y);
		int Delta, FreeSpace, Range, APos;
		if (DownInSlider)
		{
			Range = Max-Min;
			int slideSize = (Orientation == SO_HORIZONTAL?Slider->GetWidth():Slider->GetHeight());
			FreeSpace = (Orientation == SO_HORIZONTAL?Width:Height)-(Global::SCROLLBAR_BUTTON_SIZE+1)*2-slideSize;
			if (Orientation == SO_HORIZONTAL)
			{
				Delta = X-DownPosX;
			}
			else
			{
				Delta = Y-DownPosY;
			}
			APos = OriPos + (int)(Delta*(float)Range/(float)FreeSpace);
			APos = Math::Max(Min, APos);
			APos = Math::Min(Max, APos);
			SetPosition(APos);
		}
		auto hitTest = Container::FindControlAtPosition(X, Y);
		if (hitTest == btnDec || hitTest == btnInc)
			hitTest->DoMouseMove(X - hitTest->Left, Y - hitTest->Top);
		else if (hitTest == Slider)
			highlightSlider = true;
		return true;
	}

	bool ScrollBar::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoMouseUp(X,Y,Shift);

		DownPosX=DownPosY=0;
		DownInSlider = false;
		auto hitTest = Container::FindControlAtPosition(X, Y);
		if (hitTest == btnDec || hitTest == btnInc)
			hitTest->DoMouseUp(X - hitTest->Left, Y - hitTest->Top, Shift);
		ReleaseMouse();
		return true;
	}

	bool ScrollBar::DoMouseLeave()
	{
		highlightSlider = false;
		return false;
	}

	bool ScrollBar::DoMouseHover()
	{
		if (tmrOrientation != -1)
			GetEntry()->SubscribeTickEvent(this);
		return false;
	}

	bool ScrollBar::DoTick()
	{
		if (tmrOrientation == 0)
		{
			if (Position - SmallChange >= Min)
			{
				SetPosition(Position - SmallChange);
			}
			else
				SetPosition(Min);
		}
		else
		{
			if (Position + SmallChange <= Max)
			{
				SetPosition(Position + SmallChange);
			}
			else
				SetPosition(Max);
		}
		return true;
	}

	void ScrollBar::BtnDecMouseDown(UI_Base *, UIMouseEventArgs &)
	{
		if (Position-SmallChange>=Min)
		{
			SetPosition(Position-SmallChange);
			tmrOrientation = 0;
		}
	}

	void ScrollBar::BtnIncMouseDown(UI_Base *, UIMouseEventArgs &)
	{
		if (Position+SmallChange<=Max)
		{
			SetPosition(Position+SmallChange);
			tmrOrientation = 1;
		}
	}

	void ScrollBar::BtnDecMouseUp(UI_Base *, UIMouseEventArgs &)
	{
		tmrOrientation = -1;
		GetEntry()->UnSubscribeTickEvent(this);
	}

	void ScrollBar::BtnIncMouseUp(UI_Base *, UIMouseEventArgs &)
	{
		tmrOrientation = -1;
		GetEntry()->UnSubscribeTickEvent(this);
	}

	void ScrollBar::HandleMessage(const UI_MsgArgs *Args)
	{
		Control::HandleMessage(Args);
	}

	bool ScrollBar::PointInSlider(int X, int Y)
	{
		return (X>=Slider->Left && X<=Slider->Left+Slider->GetWidth() && Y>=Slider->Top && Y<=Slider->Top+Slider->GetHeight());
	}

	bool ScrollBar::PointInFreeSpace(int X, int Y)
	{
		if (PointInSlider(X,Y))
			return false;
		if (Orientation == SO_HORIZONTAL)
		{
			return (Y>0 && Y<Height && X>Global::SCROLLBAR_BUTTON_SIZE && X<Width- Global::SCROLLBAR_BUTTON_SIZE);
		}
		else
		{
			return (X>0 && X<Width && Y>Global::SCROLLBAR_BUTTON_SIZE && Y<Height- Global::SCROLLBAR_BUTTON_SIZE);
		}
	}


	ListBox::ListBox(Container * parent)
		: Container(parent)
	{
		Type = CT_LISTBOX;
		TabStop = true;
		BorderStyle = BS_FLAT_;
		BackColor = Global::Colors.EditableAreaBackColor;
		HideSelection = false;
		MultiSelect = false;
		Selecting = false;
		DownInItem = false;
		HotTrack = false;
		SelectedIndex= -1;
		Padding = 1;
		SelectionColor = Global::Colors.SelectionColor;
		HighLightColor = Global::Colors.HighlightColor;
		HighLightForeColor = Global::Colors.HighlightForeColor;
		SelectionForeColor = Global::Colors.SelectionForeColor;
		FontColor = Global::Colors.ControlFontColor;
		UnfocusedSelectionColor = Global::Colors.UnfocusedSelectionColor;
		HighLightColor = Global::Colors.HighlightColor;
		ScrollBar = new GraphicsUI::ScrollBar(this);
		ScrollBar->SetOrientation(SO_VERTICAL);
		ScrollBar->Visible = false;
		BorderWidth = 2;
		DoDpiChanged();
	}

	bool ListBox::ItemInSelection(Control *Item)
	{
		for (int i=0; i<Selection.Count(); i++)
		{
			if (Selection[i] == Item)
				return true;
		}
		return false;
	}

	void ListBox::Draw(int absX, int absY)
	{
		Control::Draw(absX,absY);
		if (!Visible) return;
		absX+=Left;absY+=Top;
		int ShowCount = Height / ItemHeight +1;
		int bdr = (ScrollBar->Visible?ScrollBar->GetWidth():0);
		auto entry = GetEntry();
		entry->ClipRects->AddRect(Rect(absX+BorderWidth, absY + BorderWidth, Width-BorderWidth*2 - bdr, Height-BorderWidth*2));
		bool focused = IsFocused();
		auto & graphics = entry->DrawCommands;
		for (int i=ScrollBar->GetPosition();i<=ScrollBar->GetPosition()+ShowCount && i<Items.Count();i++)
		{
			Control *CurItem = Items[i];
			if (i==HighLightID)
			{
				CurItem->BackColor = HighLightColor;
				CurItem->FontColor = HighLightForeColor;
			}
			else if (SelectedIndex ==i || ItemInSelection(CurItem))
			{

				CurItem->BackColor = HideSelection && !focused ? BackColor : (focused ? SelectionColor : UnfocusedSelectionColor);
				CurItem->FontColor = SelectionForeColor;
			}
				
			else
			{
				CurItem->BackColor = BackColor;
				CurItem->FontColor = FontColor;
			}
			CurItem->Posit(BorderWidth,BorderWidth+(i-ScrollBar->GetPosition())*ItemHeight,Width-BorderWidth*2-bdr, ItemHeight);
			graphics.SolidBrushColor = CurItem->BackColor;
			graphics.FillRectangle(absX + BorderWidth, absY + CurItem->Top, absX + Width - BorderWidth, absY + CurItem->Top + CurItem->GetHeight());
			CurItem->Draw(absX,absY);
		}
		if (focused && AcceptsFocus)
		{
			int FID =SelectedIndex;
			if (FID==-1) FID =0;
			bdr = BorderWidth*2;
			if (ScrollBar->Visible)	bdr += ScrollBar->GetWidth()+1;
			int RectX1 = BorderWidth+absX;
			int RectX2 = RectX1 + Width - bdr;
			int RectY1 = (FID-ScrollBar->GetPosition())*ItemHeight+absY+BorderWidth-1;
			int RectY2 = RectY1+ItemHeight+1;
			graphics.PenColor = Global::Colors.FocusRectColor;
			graphics.DrawRectangle(RectX1, RectY1, RectX2, RectY2);
		}
		entry->ClipRects->PopRect();
		ScrollBar->Draw(absX,absY);
	}

	void ListBox::SizeChanged()
	{
		ScrollBar->Posit(Width - Global::SCROLLBAR_BUTTON_SIZE - BorderWidth, BorderWidth, Global::SCROLLBAR_BUTTON_SIZE, Height - BorderWidth*2);
		ListChanged();
	}

	bool ListBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		lastSelIdx = SelectedIndex;
		Control::DoMouseDown(X,Y,Shift);
		int bdr=0,ShowCount=Height/ItemHeight;
		if (!Enabled || !Visible)
			return false;
		Selecting = false;
		DownInItem = false;
		auto hitTest = Container::FindControlAtPosition(X, Y);
		for (int i=ScrollBar->GetPosition();i<=ScrollBar->GetPosition()+ShowCount && i<Items.Count();i++)
		{
			Control *CurItem = Items[i];
			if (hitTest == CurItem || (hitTest && hitTest->IsChildOf((Container*)CurItem)))
				CurItem->DoMouseDown(X-CurItem->Left, Y-CurItem->Top, Shift);
		}
		if (ScrollBar->Visible)
		{
			if (hitTest == ScrollBar)
				ScrollBar->DoMouseDown(X-hitTest->Left, Y-hitTest->Top, Shift);
			bdr = ScrollBar->GetWidth();
		}
		if (X < Width-bdr)
		{
			DownInItem = true;
			auto newSelIdx = HitTest(X,Y);
			SelectedIndex = newSelIdx;
			if (MultiSelect)
			{
				Selecting = true;
				SelOriX = X;
				SelOriY = Y+ScrollBar->GetPosition()*ItemHeight+BorderWidth;
			}
		}
		if (hitTest != ScrollBar)
			Global::MouseCaptureControl = this;
		return true;
	}

	bool ListBox::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key,Shift);
		if (!Enabled || !Visible)
			return false;
		int ShowCount=Height/ItemHeight;
		for (int i = ScrollBar->GetPosition();i <= ScrollBar->GetPosition()+ShowCount && i < Items.Count();i++)
		{
			Control *CurItem =Items[i];
			CurItem->DoKeyDown(Key,Shift);
		}
		if (Items.Count())
		{
			if (Key == 0x28) // VK_DOWN
			{
				SelectedIndex = ClampInt(SelectedIndex+1,0,Items.Count()-1);
				SelectionChanged();

			}
			else if (Key == 0x26) // VK_UP
			{
				SelectedIndex = ClampInt(SelectedIndex-1,0,Items.Count()-1);
				SelectionChanged();
					
			}
			int sy =(SelectedIndex-ScrollBar->GetPosition())*ItemHeight+BorderWidth-1;
			if (sy<=5)
			{
				ScrollBar->SetPosition(ClampInt(SelectedIndex,0,ScrollBar->GetMax()));
			}
			else if (sy>Height-ItemHeight-5)
			{
				ScrollBar->SetPosition(ClampInt(SelectedIndex-Height / ItemHeight +1,0,ScrollBar->GetMax()));
			}
		}
		return false;
	}

	bool ListBox::DoMouseLeave()
	{
		this->ScrollBar->DoMouseLeave();
		return false;
	}

	void ListBox::DoDpiChanged()
	{
		ItemHeight = 18;
		if (font)
			ItemHeight = (int)(font->MeasureString(L"M").h * 1.1f);
		Container::DoDpiChanged();
	}

	bool ListBox::DoMouseMove(int X, int Y)
	{
		Control::DoMouseMove(X,Y);
		if (!Enabled || !Visible)
			return false;
		auto hitTest = Container::FindControlAtPosition(X, Y);
		int bdr = ScrollBar->Visible?ScrollBar->GetWidth():0;
		if (ScrollBar->Visible && hitTest == ScrollBar)
			ScrollBar->DoMouseMove(X - hitTest->Left, Y - hitTest->Top);
		else
			ScrollBar->DoMouseLeave();
		int ShowCount=Height/ItemHeight;
		for (int i = ScrollBar->GetPosition(); i <= ScrollBar->GetPosition() + ShowCount && i<Items.Count(); i++)
		{
			Control *CurItem = Items[i];
			if (hitTest == CurItem || (hitTest && hitTest->IsChildOf((Container*)CurItem)))
				CurItem->DoMouseMove(X - CurItem->Left, Y - CurItem->Top);
		}
		if (Selecting)
		{
			Selection.Clear();
			int cX,cY;
			cX = X;
			cY = Y - BorderWidth + ScrollBar->GetPosition()*ItemHeight;
			if (SelOriY>cY)
			{
				int tmpY = cY;
				cY=SelOriY;SelOriY=tmpY;
			}
			int idBegin = SelOriY / ItemHeight;
			int idEnd = cY / ItemHeight;
			if (idBegin<Items.Count())
			{
				if (idEnd>=Items.Count()) idEnd = Items.Count()-1;
				SelectedIndex=idEnd;
				for (int i=idBegin;i<=idEnd;i++)
				{
					Selection.Add(Items[i]);
				}
				auto newSelIdx = idEnd;
				SelectedIndex = newSelIdx;
			}
		}
		else if (DownInItem)
		{
			auto newSelIdx = HitTest(X, Y);
			SelectedIndex = newSelIdx;
		}
		if (DownInItem && ScrollBar->Visible)
		{
			if (Y>=Height)
			{
				if (ScrollBar->GetPosition()<ScrollBar->GetMax())
					ScrollBar->SetPosition(ScrollBar->GetPosition()+1);
			}
			else if (Y<0)
			{
				if (ScrollBar->GetPosition()>ScrollBar->GetMin())
					ScrollBar->SetPosition(ScrollBar->GetPosition()-1);
			}
		}
		if (HotTrack && X>0 && X<Width - bdr &&Y>0 && Y<Height)
		{
			HighLightID = HitTest(X,Y);
		}
		else
		{
			HighLightID = -1;
		}
		return true;
	}

	bool ListBox::DoMouseWheel(int delta)
	{
		if (Visible && Enabled && ScrollBar->Visible)
		{
			ScrollBar->SetPosition(Math::Clamp(ScrollBar->GetPosition() + (delta > 0 ? -1 : 1) * 3, 0, ScrollBar->GetMax()));
			return true;
		}
		return false;
	}

	bool ListBox::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X,Y,Shift);
		if (!Enabled || !Visible)
			return false;
		int ShowCount=Height/ItemHeight;
		auto hitTest = Container::FindControlAtPosition(X, Y);
		for (int i = ScrollBar->GetPosition(); i <= ScrollBar->GetPosition() + ShowCount && i<Items.Count(); i++)
		{
			Control *CurItem = Items[i];
			if (hitTest == CurItem || (hitTest && hitTest->IsChildOf((Container*)CurItem)))
				CurItem->DoMouseUp(X - CurItem->Left, Y - CurItem->Top, Shift);
		}
		DownInItem = false;
		Selecting = false;
		if (ScrollBar->Visible && hitTest == ScrollBar)
			ScrollBar->DoMouseUp(X - hitTest->Left, Y - hitTest->Top,Shift);
		if (lastSelIdx != SelectedIndex || (Items.Count() && Items[0]->Type == CT_CHECKBOX))
		{
			SelectionChanged();
		}
		ReleaseMouse();
		return true;
	}

	void ListBox::SelectionChanged()
	{
		UI_MsgArgs a;
		a.Sender = this;
		a.Type = MSG_UI_CHANGED;
		BroadcastMessage(&a);
	}

	int ListBox::AddCheckBoxItem(String Text)
	{
		CheckBox *chkBox = new CheckBox(this);
		chkBox->SetFont(font);
		chkBox->SetText(Text);
		chkBox->SetHeight(chkBox->TextHeight);
		chkBox->BackColor = Color(255,255,255,0);
		return AddControlItem(chkBox);
	}

	int ListBox::AddControlItem(Control *Item)
	{
		Items.Add(Item);
		Item->BackColor = Color(255,255,255,0);
		if (Item->GetHeight()>ItemHeight)
			ItemHeight = Item->GetHeight();
		Item->AcceptsFocus = false;
		Item->TabStop = false;
		ListChanged();
		return Items.Count()-1;
	}

	int ListBox::AddTextItem(String Text)
	{
		Label *lbl = new Label(this);
		lbl->SetFont(font);
		lbl->SetText(Text);
		lbl->SetHeight(lbl->TextHeight);
		lbl->BackColor = Color(255,255,255,0);
		return AddControlItem(lbl);
	}
	
	void ListBox::Delete(Control *Item)
	{
		for (int i=0; i<Items.Count(); i++)
		{
			if (Items[i] == Item)
			{
				Items[i] = 0;
				Items.RemoveAt(i);
				ListChanged();
				break;
			}
		}
		this->RemoveChild(Item);
	}

	void ListBox::Delete(int Index)
	{
		this->RemoveChild(Items[Index]);
		Items[Index] = 0;
		Items.RemoveAt(Index);
		ListChanged();
	}

	void ListBox::Clear()
	{
		for (auto item : Items)
			RemoveChild(item);
		Items.Clear();
	}

	void ListBox::ListChanged()
	{
		int PageSize; //当前ListBox的大小能同时显示的项目个数
		PageSize = Height / ItemHeight;
		if (PageSize<1) PageSize=1;
		if (PageSize>=Items.Count())
		{
			//无需显示滚动条
			ScrollBar->Visible = false;
			ScrollBar->SetValue(0, 1, 0, 1);
		}
		else
		{
			//需要显示滚动条
			ScrollBar->Visible = true;
			ScrollBar->SetValue(0,Items.Count()-PageSize,(SelectedIndex==-1)?0:ClampInt(SelectedIndex,0,Items.Count()-PageSize), PageSize);
		}
	}

	CheckBox* ListBox::GetCheckBoxItem(int Index)
	{
		return (CheckBox *)(Items[Index]);
	}

	Label* ListBox::GetTextItem(int Index)
	{
		return (Label *)(Items[Index]);
	}

	Control* ListBox::GetItem(int Index)
	{
		return (Control *)(Items[Index]);
	}

	int ListBox::GetItemHeight()
	{
		return ItemHeight;
	}

	int ListBox::HitTest(int , int Y)
	{
		int rs = Y/ItemHeight + ScrollBar->GetPosition();
		if (rs>=Items.Count())
			rs = -1;
		return rs;
	}

	Control* ListBox::GetSelectedItem()
	{
		if (SelectedIndex!=-1)
			return (Control *)(Items[SelectedIndex]);
		else
			return NULL;
	}

	ComboBox::ComboBox(Container * parent)
		: ListBox(parent)
	{
		btnDrop = new Button(this);
		btnDrop->AcceptsFocus = false;
		btnDrop->TabStop = false;
		btnDrop->SetFont(GetEntry()->System->LoadDefaultFont(DefaultFontType::Symbol));
		btnDrop->SetText(L"6");
		btnDrop->BorderColor.A = 0;
		TextBox = new GraphicsUI::TextBox(this);
		BorderStyle = BS_FLAT_;
		TextBox->BorderStyle = BS_NONE;
		TextBox->BackColor.A = 0;
		TextBox->AcceptsFocus = false;
		TextBox->TabStop = false;
		ShowList = false;
		HotTrack = true;
		HighLightColor = SelectionColor;
		HighLightForeColor = SelectionForeColor;
		SelectionColor = BackColor;
		SelectionForeColor = FontColor;
		UnfocusedSelectionColor = BackColor;
		BorderWidth = 1;
		DoDpiChanged();
	}

	bool ComboBox::DoClosePopup()
	{
		ToggleList(false);
		return false;
	}


	void ComboBox::SetSelectedIndex(int id)
	{
		ChangeSelectedItem(id);
	}

	void ComboBox::DoDpiChanged()
	{
		ListBox::DoDpiChanged();
		Posit(Left, Top, Width, Global::SCROLLBAR_BUTTON_SIZE + BorderWidth * 4);
	}

	void ComboBox::Posit(int x, int y, int w, int)
	{
		ListBox::Posit(x, y, w, Global::SCROLLBAR_BUTTON_SIZE + BorderWidth * 4);
	}

	void ComboBox::SizeChanged()
	{
		TextBox->Posit(BorderWidth, 0, Width - Global::SCROLLBAR_BUTTON_SIZE - BorderWidth * 2, Height);
		btnDrop->Posit(Width - Global::SCROLLBAR_BUTTON_SIZE - BorderWidth, BorderWidth, Global::SCROLLBAR_BUTTON_SIZE, Height - BorderWidth * 2);
	}
	void ComboBox::Draw(int absX, int absY)
	{
		Control::Draw(absX, absY);
		absX += Left; absY += Top;
		if (!Visible)
			return;
		TextBox->Draw(absX, absY);
		btnDrop->Checked = ShowList;
		btnDrop->Draw(absX, absY);
		if (IsFocused())
		{
			auto & graphics = GetEntry()->DrawCommands;
			graphics.PenColor = Global::Colors.FocusRectColor;
			graphics.DrawRectangle(absX + 3, absY + 3, absX + btnDrop->Left - 2, absY + Height - 3);
		}
	}

	void ComboBox::ToggleList(bool sl)
	{
		auto entry = GetEntry();
		ShowList = sl;
		ListLeft = 0;
		ListTop = Height+1;
		ListHeight = ItemHeight * ClampInt(Items.Count(),1,COMBOBOX_LIST_SIZE);
		ListWidth = Width;
		if (AbsolutePosY + ListTop + ListHeight > entry->GetHeight())
		{
			ListTop -= Height + ListHeight;
		}
		int vlH,vlW,vlL,vlT;
		vlH = Height; vlW = Width;
		vlL = Left; vlT = Top;
		Left = 0; Top = 0;
		Height = ListHeight; Width = ListWidth; Left = ListLeft; Top = ListTop;
		ListBox::SizeChanged();
		Height = vlH; Width = vlW; Left = vlL; Top = vlT;
		if (ShowList)
		{
			Global::MouseCaptureControl = this;
		}
	}

	bool ComboBox::PosInList(int X, int Y)
	{
		if (ShowList)
		{
			return (X >=ListLeft && X <ListLeft+ListWidth && Y>=ListTop && Y<=ListTop+ListHeight);
		}
		else
			return false;
	}

	void ComboBox::ChangeSelectedItem(int id)
	{
		if (id != -1)
		{
			if (Items[id]->Type != CT_CHECKBOX)
				TextBox->SetText(((Label *)Items[id])->GetText());
		}
		else
			TextBox->SetText(L"");
		SelectedIndex = id;
	}

	void ComboBox::SetFocus()
	{
		Control::SetFocus();
	}

	void ComboBox::BeginListBoxFunctions()
	{
		lH = Height; lW = Width;
		lL = Left; lT = Top;
		Left = 0; Top = 0;
		ListBox::Posit(ListLeft,ListTop,ListWidth,ListHeight);
		Rect R;
		R.x = ListLeft+AbsolutePosX;
		R.y = ListTop + AbsolutePosY;
		R.w = ListWidth+1;
		R.h = ListHeight+2;
		GetEntry()->ClipRects->AddRect(R);
		btnDrop->Visible = false;
	}

	void ComboBox::EndListBoxFunctions()
	{
		GetEntry()->ClipRects->PopRect();
		ComboBox::Posit(lL,lT,lW,lH);
		btnDrop->Visible = true;
	}

	bool ComboBox::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X,Y,Shift);
		if (!Visible || !Enabled)
			return false;
		lastSelIdx = SelectedIndex;
		if (IsPointInClient(X, Y))
		{
			ToggleList(!ShowList);
			Global::MouseCaptureControl = this;
		}
		else
		{
			if (PosInList(X,Y))
			{
				BeginListBoxFunctions();
				ListBox::DoMouseDown(X - ListLeft, Y - ListTop, Shift);
				EndListBoxFunctions();
				Global::MouseCaptureControl = this;
			}
			else
			{
				ToggleList(false);
				ReleaseMouse();
			}
		}
		return true;
	}

	bool ComboBox::DoMouseMove(int X, int Y)
	{
		Control::DoMouseMove(X,Y);
		if (!Visible || !Enabled)
			return false;
		if (ShowList)
		{
			BeginListBoxFunctions();
			ListBox::DoMouseMove(X - ListLeft, Y - ListTop);
			EndListBoxFunctions();
		}
		return true;
	}

	bool ComboBox::DoMouseWheel(int delta)
	{
		if (Visible && Enabled)
		{
			if (ShowList)
				return ListBox::DoMouseWheel(delta);
			else
			{
				int nselId = SelectedIndex;
				if (delta > 0)
					nselId--;
				else
					nselId++;
				nselId = Math::Clamp(nselId, 0, Items.Count() - 1);
				if (nselId != SelectedIndex)
				{
					ChangeSelectedItem(nselId);
					OnChanged.Invoke(this);
				}
				return true;
			}
		}
		return false;
	}

	bool ComboBox::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X,Y,Shift);
		if (!Visible || !Enabled)
			return false;
		if (ShowList)
		{
			BeginListBoxFunctions();
			bool PosInItem;
			int bdr = ScrollBar->Visible ? ScrollBar->GetWidth() : 0;
			PosInItem = X<ListLeft + Width - bdr && X>ListLeft && Y > ListTop && Y < ListTop + ListHeight;
			if (PosInItem)
			{
				ToggleList(false);
				ChangeSelectedItem(SelectedIndex);
				ListBox::DoMouseUp(X - ListLeft, Y - ListTop, Shift);
				ReleaseMouse();
			}
			else
			{
				ListBox::DoMouseUp(X - ListLeft, Y - ListTop, Shift);
				Global::MouseCaptureControl = this;
			}
			EndListBoxFunctions();
		}
		else
			ReleaseMouse();
		return true;
	}

	bool ComboBox::DoKeyDown(unsigned short Key, SHIFTSTATE shift)
	{
		if (!Visible || !Enabled)
			return false;
		bool AltDown = (shift != 0);
		if (!AltDown && (Key == 0x26 || Key == 0x28))
		{
			if (Key == 0x26) // VK_UP
			{
				HighLightID = ClampInt(HighLightID - 1, 0, Items.Count() - 1);
			}
			else if (Key == 0x28) // VK_DOWN
			{
				HighLightID = ClampInt(HighLightID + 1, 0, Items.Count() - 1);
			}
			if (!ShowList)
			{
				if (HighLightID != SelectedIndex)
				{
					ChangeSelectedItem(HighLightID);
					SelectionChanged();
				}
			}
			else
			{
				int sy = (HighLightID - ScrollBar->GetPosition())*ItemHeight + BorderWidth - 1;
				if (sy < 0)
				{
					ScrollBar->SetPosition(ClampInt(HighLightID, 0, ScrollBar->GetMax()));
				}
				else if (sy > ListHeight - ItemHeight - 1)
				{
					ScrollBar->SetPosition(ClampInt(HighLightID - ListHeight / ItemHeight + 1, 0, ScrollBar->GetMax()));
				}
			}
		}
		if ((Key == 0x20 || Key == 0x0D)) // VK_SPACE VK_RETURN
		{
			if (ShowList)
			{
				if (HighLightID != SelectedIndex)
				{
					ChangeSelectedItem(HighLightID);
					SelectionChanged();
				}
			}
			ToggleList(!ShowList);
			return true;
		}
		else if (Key == 0x1B) // VK_ESCAPE
		{
			ToggleList(false);
			return true;
		}
		if (Key == 0x26 || Key == 0x28)
			return true;
		return false;
	}

	void ComboBox::HandleMessage(const UI_MsgArgs *Args)
	{
		if (ShowList)
			ListBox::HandleMessage(Args);
		if (Args->Type == MSG_UI_TOPLAYER_DRAW)
		{
			if (Visible && ShowList)
			{
				BeginListBoxFunctions();
				int lstB = BorderStyle;
				Color lstBC= BorderColor;
				BorderColor = Global::Colors.ControlFontColor;
				BorderStyle = BS_FLAT_;
				auto oldShadow = BackgroundShadow;
				BackgroundShadow = true;
				ListBox::Draw(AbsolutePosX,AbsolutePosY);
				BackgroundShadow = oldShadow;
				BorderStyle = lstB;
				BorderColor = lstBC;
				EndListBoxFunctions();
			}
		}
		if (Args->Type == MSG_UI_MOUSEWHEEL)
		{
			if (!ShowList && IsFocused())
			{
				if ((*(UIMouseEventArgs *)Args->Data).Delta<0)
				{
					SelectedIndex=ClampInt(SelectedIndex+1,0,Items.Count()-1);
					ChangeSelectedItem(SelectedIndex);
				}
				else
				{
					SelectedIndex=ClampInt(SelectedIndex-1,0,Items.Count()-1);
					ChangeSelectedItem(SelectedIndex);
				}
			}
		}
	}

	void ComboBox::LostFocus(Control * newFocus)
	{
		Control::LostFocus(newFocus);
		while (newFocus && newFocus != this)
			newFocus = newFocus->Parent;
		if (!newFocus)
			ToggleList(false);
	}

	ProgressBar::ProgressBar(Container * parent)
		: Control(parent)
	{
		BorderStyle = BS_LOWERED;
		Type = CT_PROGRESSBAR;
		Style = PROGRESSBAR_STYLE_NORMAL;
		Max = 100;
		Position = 0;
	}

	ProgressBar::~ProgressBar()
	{
			
	}

	void ProgressBar::SetMax(int AMax)
	{
		Max = AMax;
		if (Position>Max)
			Position = Max;
	}

	void ProgressBar::SetPosition(int APos)
	{
		Position = ClampInt(APos,0,Max);
	}

	int ProgressBar::GetMax()
	{
		return Max;
	}

	int ProgressBar::GetPosition()
	{
		return Position;
	}

	void ProgressBar::Draw(int absX, int absY)
	{
		Control::Draw(absX,absY);
		absX+=Left; absY+=Top;
		int PH,PW;
		PH = Height - 4;
		auto entry = GetEntry();
		auto & graphics = entry->DrawCommands;
		if (Style == 2) //Block Style
		{
			entry->ClipRects->AddRect(Rect(absX+2,absY+2,Width-6,Height-4));
			PW = int(PH *0.65);
			int bc = (int)(Position/(float)Max *ceil((Width - 2)/(float)PW));
			for (int i=0;i<bc;i++)
			{
				int cx = i*PW+3+absX;
				int cy = 2+absY;
				graphics.SolidBrushColor = Global::Colors.SelectionColor;
				graphics.FillRectangle(cx, cy, cx + PW - 2, cy + PH);
			}
			entry->ClipRects->PopRect();
		}
		else
		{
			int cx = absX+3, cy= absY+2;
			PW = (Width -4)*Position/Max;
			graphics.SolidBrushColor = Global::Colors.SelectionColor;
			graphics.FillRectangle(cx, cy, cx + PW, cy + PH);
		}
	}
		
	Menu::Menu(Container * parent, MenuStyle s)
		: Container(parent), style(s)
	{
		Type = CT_MENU;
		TabStop = s == msMainMenu;
		TopMost = true;
		Padding = 0;
		Height = Padding.Vertical();
		Width = Padding.Horizontal();
		BorderStyle = BS_NONE;
		BorderColor = Global::Colors.MenuBorderColor;
		BackColor = Global::Colors.MenuBackColor;
		curSubMenu = 0;
		parentItem = 0;
		if (style == msPopup)
			Visible = false;
		if (style == msPopup)
			BackgroundShadow = true;
		else
		{
			DockStyle = dsTop;
			BackColor = Global::Colors.ToolButtonBackColor1;
		}
		if (s == msMainMenu)
		{
			if (parent->Type == CT_ENTRY)
				((UIEntry*)parent)->MainMenu = this;
			else if (parent->Type == CT_FORM)
				((Form*)parent)->MainMenu = this;
		}
		else
			Padding = 2;
	}

	void Menu::SetFocus()
	{
		if (style == msMainMenu)
			lastFocusedCtrl = GetEntry()->FocusedControl;
		Container::SetFocus();	
	}

	void Menu::DoDpiChanged()
	{
		Container::DoDpiChanged();
		PositMenuItems();
	}

	void Menu::PopupSubMenu(Menu * subMenu, int x, int y)
	{
		if (!subMenu->Visible || subMenu != curSubMenu)
		{
			if (curSubMenu)
				CloseSubMenu();
			subMenu->Popup(x, y);
			curSubMenu = subMenu;
		}
	}

	void Menu::CloseSubMenu()
	{
		if (curSubMenu)
		{
			curSubMenu->CloseSubMenu();
			curSubMenu->CloseMenu();
			curSubMenu = 0;
			ReleaseMouse();
			if (this->style != msMainMenu)
				Global::MouseCaptureControl = this;
		}
	}

	bool Menu::DoClosePopup()
	{
		CloseSubMenu();
		if (style == msPopup)
			CloseMenu();
		return false;
	}

	void Menu::AddItem(MenuItem * item)
	{
		Items.Add(item);
		item->Parent = this;
		if (controls.IndexOf(item) == -1)
			controls.Add(item);
		PositMenuItems();
	}

	void Menu::RemoveItem(MenuItem * item)
	{
		int fid = -1;
		fid = Items.IndexOf(item);
		if (fid != -1)
		{
			Items[fid] = 0;
			Items.RemoveAt(fid);
		}
		fid = controls.IndexOf(item);
		if (fid != -1)
		{
			controls.RemoveAt(fid);
		}
		PositMenuItems();
	}

	void Menu::PositMenuItems()
	{
		if (style == msPopup)
		{
			int cHeight = Padding.Top;
			Width = 0;
			ItemHeight = (int)(GetEntry()->GetLineHeight() * 1.5f);
			for (int i=0; i<Items.Count(); i++)
			{
				if (!Items[i]->Visible)
					continue;
				int nWidth = Items[i]->MeasureWidth() + ItemHeight;
				if (nWidth + Padding.Horizontal() > Width)
					Width = nWidth + Padding.Horizontal();
				if (Items[i]->IsSeperator())
					Items[i]->SetHeight(ItemHeight>>2);
				else
					Items[i]->SetHeight(ItemHeight);
					
				Items[i]->Left = Padding.Left;
				Items[i]->Top = cHeight;

				cHeight += Items[i]->GetHeight();
			}
			Height = cHeight + Padding.Bottom;
			for (int i=0; i<Items.Count(); i++)
			{
				Items[i]->SetWidth(Width - Padding.Horizontal());
			}
		}
		else
		{
			Height = (int)(GetEntry()->GetLineHeight() * 1.25f);
			Width = 0;
			for (int i=0; i<Items.Count(); i++)
			{
				Items[i]->isButton = true;
				if (Items[i]->Visible && !Items[i]->IsSeperator())
				{
					Items[i]->Top = 0;
					Items[i]->SetWidth(Items[i]->MeasureWidth(true));
					Items[i]->SetHeight(Height);
					Items[i]->Left = Width;
					Width += Items[i]->GetWidth();
				}
				else
					Items[i]->Visible = false;
			}
		}
	}

	int Menu::Count()
	{
		return Items.Count();
	}

	MenuItem * Menu::GetItem(int id)
	{
		if (id < Items.Count())
		{
			return Items[id];
		}
		else
		{
			throw CoreLib::IndexOutofRangeException();
		}
	}

	void Menu::ItemSelected(MenuItem * item)
	{
		if (parentItem)
			parentItem->ItemSelected(item);
		if (style == msPopup)
			CloseMenu();
		else
		{
			for (int i=0; i<Items.Count(); i++)
				Items[i]->Selected = false;
		}
		if (style == msMainMenu)
		{
			if (lastFocusedCtrl)
				lastFocusedCtrl->SetFocus();
		}
	}

	void Menu::DrawPopup()
	{
		auto entry = GetEntry();
		int absX, absY;
		LocalPosToAbsolutePos(0, 0, absX, absY);
		Control::Draw(absX - Left, absY - Top);
		auto & graphics = entry->DrawCommands;
		graphics.SetRenderTransform(absX, absY);
		for (auto & item : Items)
			ItemHeight = Math::Max(ItemHeight, item->GetHeight());
		graphics.SolidBrushColor = Global::Colors.MemuIconBackColor;
		graphics.FillRectangle(Padding.Left, Padding.Top, ItemHeight + Padding.Left, Height - Padding.Bottom);
		graphics.PenColor = Global::Colors.MenuBorderColor;
		graphics.DrawRectangle(0, 0, Width - 1, Height - 1);
		graphics.DrawLine(ItemHeight+Padding.Left, Padding.Top, ItemHeight + Padding.Left, Height - Padding.Bottom - 1);
		int cposY = 0;
		for (int i =0; i<Items.Count(); i++)
		{
			int itemHeight = Items[i]->GetHeight();
			graphics.SetRenderTransform(absX + Padding.Left, absY + Padding.Top + cposY);
			Items[i]->DrawMenuItem(Width - Padding.Horizontal(), ItemHeight);
			cposY += itemHeight;
		}
		graphics.SetRenderTransform(0, 0);
	}


	void Menu::Popup(int x, int y)
	{
		if (!Visible)
		{
			auto entry = GetEntry();
			if (!parentItem)
				lastFocusedCtrl = entry->FocusedControl;
			OnPopup.Invoke(this);
			PositMenuItems();
			for (int i=0; i<Items.Count(); i++)
				Items[i]->Selected = false;
			Left = x;
			Top = y;
			int ax, ay;
			LocalPosToAbsolutePos(0, 0, ax, ay);
			if (ax + Width > entry->GetWidth())
				Left -= Width;
			if (ay + Height > entry->GetHeight())
				Top -= Height;
			Visible = true;
			SetFocus();
			Global::MouseCaptureControl = this;
		}
	}

	void Menu::CloseMenu()
	{
		if (Visible)
		{
			Visible = false;
			OnMenuClosed.Invoke(this);
			if ((!parentItem || parentItem->isButton) && lastFocusedCtrl)
				lastFocusedCtrl->SetFocus();
			if (parentItem && parentItem->isButton)
				parentItem->Selected = false;
			if (parentItem)
			{
				Control * parent = ((MenuItem*)parentItem)->Parent;
				if (parent)
					((Menu*)parent)->curSubMenu = 0;
			}
			enableMouseHover = false;
			curSubMenu = nullptr;
			ReleaseMouse();
			if (Global::MouseCaptureControl && Global::MouseCaptureControl->IsChildOf(this))
				Global::MouseCaptureControl = nullptr;
		}
	}

	void Menu::DrawMenuBar(int absX, int absY)
	{
		Control::Draw(absX, absY);
		auto entry = GetEntry();
		int ox = absX + Left + Padding.Left;
		int oy = absY + Top + Padding.Top;
		int cposY = 0;
		auto & graphics = entry->DrawCommands;
		for (int i = 0; i < Items.Count(); i++)
		{
			graphics.SetRenderTransform(ox + Items[i]->Left, oy + Items[i]->Top);
			int itemHeight = Items[i]->IsSeperator()?3:ItemHeight;
			Items[i]->DrawMenuButton(Items[i]->GetWidth(), Items[i]->GetHeight());
			cposY += itemHeight;
		}
		graphics.SetRenderTransform(0, 0);
	}

	void Menu::Draw(int absX, int absY)
	{
		if (style == msMainMenu)
			DrawMenuBar(absX, absY);
	}

	bool Menu::DoMouseHover()
	{
		if (!enableMouseHover)
			return false;
		enableMouseHover = false;
		for (auto & item : Items)
		{
			if (item->Selected)
			{
				item->DoMouseHover();
			}
		}
		return false;
	}


	bool Menu::DoMouseMove(int X, int Y)
	{
		Container::DoMouseMove(X,Y);
		if (!Visible || ! Enabled)
			return false;
		for (auto & item : Items)
		{
			if (X >= item->Left && X < item->Left + item->Width &&
				Y >= item->Top && Y < item->Top + item->Height)
			{
				item->Selected = true;
			}
			else
				item->Selected = false;
		}
		if (IsPointInClient(X, Y))
		{
			enableMouseHover = true;
			if (parentItem)
				parentItem->Selected = true;
		}
		else
		{
			enableMouseHover = false;
			if (!curSubMenu)
			{
				for (int i = 0; i < Items.Count(); i++)
					Items[i]->Selected = false;
			}
		}
		if (curSubMenu)
		{
			for (auto & item : Items)
			{
				if (item->Selected)
				{
					if (item->SubMenu && item->SubMenu && item->SubMenu->Count())
					{
						if (style == Menu::msMainMenu)
						{
							CloseSubMenu();
							PopupSubMenu(item->SubMenu, -item->Padding.Left, Height - item->Padding.Vertical());
						}
					}
				}
			}
		}
		if (!Parent || (Parent->Type != CT_MENU && Parent->Type != CT_MENU_ITEM))
			return true;
		return false;
	}
	bool Menu::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Container::DoMouseDown(X, Y, Shift);
		if (!IsPointInClient(X, Y))
		{
			if (style == msPopup)
				CloseMenu();
			else
			{
				for (int i=0; i<Items.Count(); i++)
					Items[i]->Selected = false;
			}
		}
		else
		{
			for (auto & item : Items)
				if (X >= item->Left && X < item->Left + item->Width &&
					Y >= item->Top && Y <= item->Top + item->Height)
					item->DoMouseDown(X - item->Left, Y - item->Top, Shift);
			return true;
		}
		
		return false;
	}
	bool Menu::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Container::DoMouseUp(X, Y, Shift);
		return true;
	}

	int Menu::GetSelectedItemID()
	{
		for (int i=0; i<Items.Count(); i++)
			if (Items[i]->Selected && Items[i]->Enabled && Items[i]->Visible && !Items[i]->IsSeperator())
				return i;
		return -1;
	}

	bool Menu::DoKeyDown(unsigned short Key, SHIFTSTATE /*Shift*/)
	{
		if (!Enabled || !Visible)
			return false;
		if ((Key >= L'A' && Key <= L'Z') || (Key >= L'0' && Key <= L'9'))
		{
			for (int i=0; i<Items.Count(); i++)
				Items[i]->Selected = false;
			for (int i=0; i<Items.Count(); i++)
				if (Items[i]->GetAccessKey() == Key)
				{
					Items[i]->Hit();
					Items[i]->Selected = true;
					return true;
				}
			return false;
		}
		int id = GetSelectedItemID();

		if (Key == 0x20 || Key == 0x0D) // VK_SPACE || VK_RETURN
		{
			if (id >= 0 && Items[id]->Selected && Items[id]->Enabled && !Items[id]->IsSeperator())
			{
				Items[id]->Hit();
			}
			return true;
		}
		if (style == msPopup)
		{
			Menu * parentMainMenu = nullptr;
			if (parentItem && parentItem->Parent && parentItem->Parent->Type == CT_MENU &&
				((Menu*)parentItem->Parent)->style == msMainMenu)
				parentMainMenu = ((Menu*)parentItem->Parent);
			if (Key == 0x26 || Key == 0x28) // VK_UP VK_DOWN
			{
				for (int i=0; i<Items.Count(); i++)
					Items[i]->Selected = false;
					
				if (Key == 0x28)
				{
					int nxt = id + 1;
					int tc = Items.Count();
					nxt %= Items.Count();
					while (nxt != id && tc)
					{
						if (!Items[nxt]->IsSeperator() && Items[nxt]->Visible && Items[nxt]->Enabled)
						{
							Items[nxt]->Selected = true;
							break;
						}
						nxt ++;
						nxt %= Items.Count();
						tc--;
					}
					if (nxt == id)
						Items[id]->Selected = true;
				}
				else if (Key == 0x26)
				{
					int nxt = id - 1;
					int tc = Items.Count();
					if (nxt < 0)
						nxt += Items.Count();
					nxt %= Items.Count();
					while (nxt != id && tc)
					{
						if (!Items[nxt]->IsSeperator() && Items[nxt]->Visible && Items[nxt]->Enabled)
						{
							Items[nxt]->Selected = true;
							break;
						}
						nxt --;
						if (nxt < 0)
							nxt += Items.Count();
						nxt %= Items.Count();
						tc--;
					}
					if (nxt == id && id != -1)
						Items[id]->Selected = true;
				}
				return true;
			}
			if (Key == 0x27)  // VK_RIGHT
			{
				if (id != -1 && Items[id]->SubMenu && Items[id]->SubMenu->Count())
				{
					PopupSubMenu(Items[id]->SubMenu, Items[id]->Width - 2, 0);
					for (int i=0; i<Items[id]->SubMenu->Count(); i++)
					{
						MenuItem * item = Items[id]->SubMenu->GetItem(i);
						if (!item->IsSeperator() && item->Enabled && item->Visible)
						{
							item->Selected = true;
							break;
						}
					}
				}
				else if (parentMainMenu && parentMainMenu->Items.Count())
				{
					int pid = -1;
					for (int i = 0; i < parentMainMenu->Items.Count(); i++)
						if (parentMainMenu->Items[i]->Selected)
						{
							pid = i;
							break;
						}
					int npId = (pid + 1) % parentMainMenu->Items.Count();
					int trials = 0;
					while (trials < parentMainMenu->Items.Count() && (parentMainMenu->Items[npId]->IsSeperator() || !parentMainMenu->Items[npId]->Enabled
						|| !parentMainMenu->Items[npId]->Visible))
						npId = (npId + 1) % parentMainMenu->Items.Count();
					for (auto item : parentMainMenu->Items)
						item->Selected = false;
					parentMainMenu->Items[npId]->Selected = true;

					parentMainMenu->Items[npId]->Hit();
				}
				return true;
			}
			else if (Key == 0x25)
			{
				if (parentMainMenu && parentMainMenu->Items.Count())
				{
					int pid = -1;
					for (int i = 0; i < parentMainMenu->Items.Count(); i++)
						if (parentMainMenu->Items[i]->Selected)
						{
							pid = i;
							break;
						}
					int npId = (pid - 1 + parentMainMenu->Items.Count()) % parentMainMenu->Items.Count();
					int trials = 0;
					while (trials < parentMainMenu->Items.Count() && (parentMainMenu->Items[npId]->IsSeperator() || !parentMainMenu->Items[npId]->Enabled
						|| !parentMainMenu->Items[npId]->Visible))
						npId = (npId - 1 + parentMainMenu->Items.Count()) % parentMainMenu->Items.Count();
					for (auto item : parentMainMenu->Items)
						item->Selected = false;
					parentMainMenu->Items[npId]->Selected = true;
					parentMainMenu->Items[npId]->Hit();
				}
				else if (parentItem && parentItem->Parent)
				{
					((Menu *)parentItem->Parent)->CloseSubMenu();
				}
				return true;
			}
			else if (Key == 0x1B) // VK_LEFT || VK_ESCAPE
			{
				if (parentItem && parentItem->Parent)
				{
					((Menu *)parentItem->Parent)->CloseSubMenu();
				}
				CloseMenu();
				if (parentMainMenu)
				{
					parentMainMenu->SetFocus();
					int pid = parentMainMenu->Items.IndexOf(parentItem);
					if (pid != -1)
						parentMainMenu->Items[pid]->Selected = true;
				}
				return true;
			}
		}
		else
		{
			if (Key == 0x25 || Key == 0x27) // VK_LEFT VK_RIGHT
			{
				for (int i=0; i<Items.Count(); i++)
					Items[i]->Selected = false;
					
				if (Key == 0x27)
				{
					int nxt = id + 1;
					int tc = Items.Count();
					nxt %= Items.Count();
					while (nxt != id && tc)
					{
						if (!Items[nxt]->IsSeperator() && Items[nxt]->Visible && Items[nxt]->Enabled)
						{
							Items[nxt]->Selected = true;
							break;
						}
						nxt ++;
						nxt %= Items.Count();
						tc--;
					}
					if (nxt == id)
						Items[id]->Selected = true;
				}
				else if (Key == 0x25)
				{
					int nxt = id - 1;
					int tc = Items.Count();
					if (nxt < 0)
						nxt += Items.Count();
					nxt %= Items.Count();
					while (nxt != id && tc)
					{
						if (!Items[nxt]->IsSeperator() && Items[nxt]->Visible && Items[nxt]->Enabled)
						{
							Items[nxt]->Selected = true;
							break;
						}
						nxt --;
						if (nxt < 0)
							nxt += Items.Count();
						nxt %= Items.Count();
						tc--;
					}
					if (nxt == id && id != -1)
						Items[id]->Selected = true;
				}
				return true;
			}
			else if (Key == 0x28) // VK_DOWN
 			{
				if (id != -1)
					Items[id]->Hit();
				if (curSubMenu)
				{
					for (int i=0; i<curSubMenu->Count(); i++)
					{
						MenuItem * item = curSubMenu->GetItem(i);
						if (!item->IsSeperator() && item->Enabled && item->Visible)
						{
							item->Selected = true;
							break;
						}
					}
				}
				return true;
			}
			else if (Key == 0x1B) // VK_ESCAPE
			{
				CloseSubMenu();
				for (auto item : Items)
					item->Selected = false;
				if (lastFocusedCtrl)
					lastFocusedCtrl->SetFocus();
				return true;
			}
		}

		return false;
	}

	void Menu::HandleMessage(const UI_MsgArgs * Args)
	{
		if (Args->Type == MSG_UI_TOPLAYER_DRAW)
		{
			if (Visible)
				if (style == msPopup)
					DrawPopup();
		}
	}

	MenuItem::MenuItem(Menu * parent)
		: Container(parent), accKey(0)
	{
		TabStop = false;
		isSeperator = true;
		parent->AddItem(this);
	}

	MenuItem::MenuItem(MenuItem * parent)
		: Container(parent->GetSubMenu()), accKey(0)
	{
		TabStop = false;
		isSeperator = true;
		parent->AddItem(this);
	}

	MenuItem::MenuItem(Menu * parent, const String & text, const String & shortcutText)
		: Container(parent)
	{
		TabStop = false;
		isSeperator = false;
		Init();
		SetText(text);
		lblShortcut->SetText(shortcutText);
		parent->AddItem(this);

	}

	MenuItem::MenuItem(MenuItem * parent, const String & text, const String & shortcutText)
		: Container(parent->GetSubMenu())
	{
		TabStop = false;
		isSeperator = false;
		Init();
		SetText(text);
		parent->AddItem(this);
		lblShortcut->SetText(shortcutText);
	}

	MenuItem::MenuItem(Menu * parent, const String & text)
		: MenuItem(parent, text, L"")
	{
	}

	MenuItem::MenuItem(MenuItem * parent, const String & text)
		: MenuItem(parent, text, L"")
	{
	}

	void MenuItem::SetText(const String & text)
	{
		StringBuilder unescape;
		accKey = 0;
		accKeyId = -1;
		for (int i = 0; i < text.Length(); i++)
		{
			if (text[i] != L'&')
				unescape << text[i];
			else if (i < text.Length() - 1)
			{
				if (text[i + 1] != L'&')
				{
					accKey = text[i + 1];
					accKeyId = unescape.Length();
				}
			}
		}
		if (accKey >= 97 && accKey <= 122)
			accKey = accKey + (-97 + 65);
		lblText->SetText(unescape.ProduceString());
	}

	String MenuItem::GetText()
	{
		return lblText->GetText();
	}

	void MenuItem::SetShortcutText(const String & text)
	{
		lblShortcut->SetText(text);
	}

	String MenuItem::GetShortcutText()
	{
		return lblShortcut->GetText();
	}

	void MenuItem::Init()
	{
		Type = CT_MENU_ITEM; 
		Selected = false;
		Checked = false;
		isButton = false;
		accKey = 0;
		DoDpiChanged();
		if (!isSeperator)
		{
			lblText = new Label(this);
			lblShortcut = new Label(this);
			lblText->AutoSize = true;
			lblShortcut->AutoSize = true;
		}
	}

	bool MenuItem::IsSeperator()
	{
		return isSeperator;
	}

	wchar_t MenuItem::GetAccessKey()
	{
		return accKey;
	}

	int MenuItem::MeasureWidth(bool pIsButton)
	{
		if (!pIsButton)
		{
			if (isSeperator)
			{
				return 20;
			}
			else
			{
				lblText->SetHeight(lblText->TextHeight);
				lblShortcut->SetHeight(lblText->TextHeight);
				int rm = 0;
				if (SubMenu && SubMenu->Count())
					rm = 8;
				return lblText->TextWidth + 16 +
					lblShortcut->TextWidth + separatorHeading + Padding.Horizontal() + rm;
			}
		}
		else
		{
			return lblText->TextWidth + separatorHeading + Padding.Horizontal();
		}
	}

	bool MenuItem::DoMouseEnter()
	{
		cursorInClient = true;
		Control::DoMouseEnter();
		Menu * mn = (Menu*)Parent;
		if (mn)
		{
			for (int i=0; i<mn->Count(); i++)
				mn->GetItem(i)->Selected = false;
		}
		if (Enabled && Visible && !isSeperator)
			Selected = true;
		return false;
	}

	bool MenuItem::DoMouseLeave()
	{
		cursorInClient = false;
		Control::DoMouseLeave();
		Menu * mn = (Menu*)Parent;
		if (SubMenu && mn && mn->curSubMenu == SubMenu)
			Selected = true;
		else
			Selected = false;
		return false;
	}

	bool MenuItem::DoMouseHover()
	{
		if (isButton)
			return false;
		if (Enabled && SubMenu && SubMenu->Count() != 0)
		{
			if (Parent)
			{
				((Menu *)Parent)->PopupSubMenu(SubMenu, Width - Padding.Left, -Padding.Top);
			}
		}
		else
		{
			if (Parent)
			{
				((Menu *)Parent)->CloseSubMenu();
			}
		}
		return false;
	}

	bool MenuItem::DoClick()
	{	
		if (!isSeperator && Enabled && Visible && Parent)
		{
			if (!SubMenu || SubMenu->Count() == 0)
				((Menu*)Parent)->ItemSelected(this);
		}
		Control::DoClick();
		return false;
	}

	void MenuItem::AddItem(MenuItem * item)
	{
		GetSubMenu();
		SubMenu->AddItem(item);
	}

	void MenuItem::RemoveItem(MenuItem * item)
	{
		if (SubMenu)
			SubMenu->RemoveItem(item);
	}

	Menu * MenuItem::GetSubMenu()
	{
		if (!SubMenu)
		{
			SubMenu = new Menu(this);
			SubMenu->parentItem = this;
		}
		return SubMenu;
	}

	int MenuItem::Count()
	{
		if (SubMenu)
			return SubMenu->Count();
		else
			return 0;
	}
	
	void MenuItem::ItemSelected(MenuItem * item)
	{
		if (Parent)
		{
			((Menu *)Parent)->ItemSelected(item);
		}
	}


	void MenuItem::DrawMenuButton(int width, int height)
	{
		if (!isSeperator && Visible)
		{
			auto entry = GetEntry();
			if (Selected || (SubMenu && SubMenu->Visible))
			{
				auto & graphics = entry->DrawCommands;
				if (SubMenu && SubMenu->Visible)
				{
					graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorPressed1;
					graphics.FillRectangle(0,0,width, height);
				}
				else
				{
					graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorHighlight1;
					graphics.FillRectangle(0,0,width, height);
				}
				graphics.PenColor = Global::Colors.ToolButtonBorderHighLight;
				graphics.DrawRectangle(0,0,width,height);
				lblText->FontColor = Global::Colors.MenuItemHighlightForeColor;
			}
			else
			{
				if (Enabled)
					lblText->FontColor = Global::Colors.MenuItemForeColor;
				else
					lblText->FontColor = Global::Colors.MenuItemDisabledForeColor;
			}
			lblText->Draw((width-lblText->GetWidth())/2, 
				(height-entry->GetLineHeight())/2);
			
		}
	}

	void MenuItem::DrawMenuItem(int width, int itemHeight)
	{
		auto entry = GetEntry();
		auto & graphics = entry->DrawCommands;
		if (isSeperator)
		{
			graphics.PenColor = Global::Colors.MenuItemDisabledForeColor;
			graphics.DrawLine(itemHeight + separatorHeading, Height >> 1, width, Height >> 1);
		}
		else
		{
			if (Selected || (SubMenu && SubMenu->Visible))
			{
				if (SubMenu && SubMenu->Visible)
					graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorPressed1;
				else
					graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorHighlight1;
				graphics.FillRectangle(0, 0, width, itemHeight);
			}
			int top = (itemHeight - lblText->GetHeight())/2;
			if (!Enabled)
			{
				lblText->FontColor = Global::Colors.MenuItemDisabledForeColor;
				lblShortcut->FontColor = Global::Colors.MenuItemDisabledForeColor;
			}
			else
			{
				if (Selected)
				{
					lblText->FontColor = Global::Colors.MenuItemHighlightForeColor;
					lblShortcut->FontColor = Global::Colors.MenuItemHighlightForeColor;
				}
				else
				{
					lblText->FontColor = Global::Colors.MenuItemForeColor;
					lblShortcut->FontColor = Global::Colors.MenuItemForeColor;
				}
			}
			lblText->Draw(itemHeight + separatorHeading, top);
			lblShortcut->Draw(width - Padding.Right - lblShortcut->GetWidth(), top);
			if (SubMenu && SubMenu->Count())
			{
				int size = GetEntry()->GetLineHeight() >> 1;
				int x1 = width - Padding.Right;
				int y1 = itemHeight / 2 - size / 2;
				graphics.SolidBrushColor = lblText->FontColor;
				graphics.FillTriangle(x1, y1, x1 + size / 2, itemHeight / 2, x1, y1 + size);
			}
			if (Checked)
			{
				// Draw Checkmark
				if (Selected)
					graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorPressed1;
				else
					graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorHighlight1;
				const int IconMargin = 2;
				graphics.FillRectangle(0, 0, itemHeight, itemHeight);
				if (!Selected)
				{
					graphics.PenColor = Global::Colors.ToolButtonBorderHighLight;
					graphics.DrawRectangle(IconMargin, IconMargin, Height - IconMargin, Height-IconMargin);
				}
				entry->CheckmarkLabel->FontColor = lblText->FontColor;
				entry->CheckmarkLabel->Draw((itemHeight - entry->CheckmarkLabel->GetHeight())/2 + 2,
					(itemHeight - entry->CheckmarkLabel->GetHeight())/2);
			}
			
		}
	}

	void MenuItem::HandleMessage(const UI_MsgArgs * Args)
	{
		Control::HandleMessage(Args);
	}

	void MenuItem::DoDpiChanged()
	{
		Container::DoDpiChanged();
		Padding.Left = Padding.Right = GetEntry()->GetLineHeight() / 2;
	}

	void MenuItem::Hit()
	{
		Menu * mn = (Menu*)Parent;

		if (Parent && SubMenu && SubMenu->Count())
		{
			if (isButton)
				mn->PopupSubMenu(SubMenu, -Padding.Left, Height - Padding.Vertical());
			else
				mn->PopupSubMenu(SubMenu, Width - Padding.Left, -Padding.Top);
		}
		else
		{
			while (mn)
			{
				if (mn->style == Menu::msPopup)
					mn->CloseMenu();
				else
					break;
				if (mn->Parent)
					mn = dynamic_cast<Menu*>(mn->Parent->Parent);
			}
			OnClick(this);
		}
	}

	bool MenuItem::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X,Y, Shift);
		if (IsPointInClient(X, Y))
		{
			Hit();
			return true;
		}
		return false;
	}
	bool MenuItem::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X, Y, Shift);

		return false;
	}
	bool MenuItem::DoKeyDown(unsigned short Key, SHIFTSTATE Shift)
	{
		Control::DoKeyDown(Key, Shift);
		if (SubMenu)
			SubMenu->DoKeyDown(Key,Shift);
		return false;
	}

	MenuItem * MenuItem::GetItem(int id)
	{
		if (SubMenu && SubMenu->Count() > id)
			return SubMenu->GetItem(id);
		else
			return 0;
	}

	ImageDisplay::ImageDisplay(Container * parent)
		: Container(parent)
	{
		BorderStyle = BS_LOWERED;
	}

	void ImageDisplay::SetImage(IImage *img)
	{
		image = img;
	}

	IImage * ImageDisplay::GetImage()
	{
		return image.operator->();
	}

	void ImageDisplay::Draw(int absX, int absY)
	{
		Control::Draw(absX, absY);
		absX += Left;
		absY += Top;
		if (image)
		{
			auto entry = GetEntry();
			entry->ClipRects->AddRect(Rect(absX, absY, Width-2, Height-2));
			GetEntry()->DrawCommands.DrawImage(image.Ptr(), absX, absY);
			entry->ClipRects->PopRect();
		}
	}

	ToolButton::ToolButton(ToolStrip * parent)
		: Container(parent)
	{
		ButtonStyle = bsNormal;
		Init();
	}

	ToolButton::ToolButton(ToolStrip * parent, const String & _text, _ButtonStyle bs, IImage * bmp)
		: Container(parent)
	{
		ButtonStyle = bs;
		Init();
		SetText(_text);
		SetImage(bmp);
	}

	void ToolButton::Init()
	{
		Type = CT_TOOL_BUTTON;
		Selected = false;
		Checked = false;
		Pressed = false;
		ShowText = false;
		lblText = new Label(this);
		bindButton = 0;
		Padding = imageLabelPadding = GetEntry()->GetLineHeight() / 4;
	}

	void ToolButton::SetImage(IImage * bmp)
	{
		if (bmp)
		{
			imageDisabled = image = bmp;
		}
	}


	String ToolButton::GetText()
	{
		return text;
	}

	void ToolButton::SetText(const String & _text)
	{
		text = _text;
		lblText->SetText(_text);
	}

	void ToolButton::BindButton(ToolButton * btn)
	{
		bindButton = btn;
		btn->bindButton = this;
	}

	bool ToolButton::DoMouseEnter()
	{
		Control::DoMouseEnter();
		if (Enabled && Visible)
			Selected = true;
		if (bindButton && bindButton->Enabled && bindButton->Visible)
			bindButton->Selected = true;
		return false;
	}

	bool ToolButton::DoMouseLeave()
	{
		Control::DoMouseLeave();
		Selected = false;
		if (bindButton)
			bindButton->Selected = false;
		return false;
	}

	int ToolButton::MeasureWidth()
	{
		int imgSize = image?image->GetWidth():0;
		int textWidth = imageLabelPadding + lblText->GetWidth();
		if (ButtonStyle == bsNormal)
			return imgSize + Padding.Horizontal() + (ShowText ? textWidth:0);
		else if (ButtonStyle == bsDropDown)
			return DropDownButtonWidth;
		else
			return Padding.Horizontal();
	}

	int ToolButton::MeasureHeight()
	{
		int imgSize = image?image->GetHeight():0;
		if (lblText->GetHeight() > imgSize)
			imgSize = lblText->GetHeight();
		return imgSize + Padding.Vertical();
	}

	bool ToolButton::DoMouseDown(int X, int Y, SHIFTSTATE shift)
	{
		Control::DoMouseDown(X, Y, shift);
		if (Enabled && Visible)
			Pressed = true;
		return false;
	}

	bool ToolButton::DoMouseUp(int X, int Y, SHIFTSTATE shift)
	{
		Control::DoMouseUp(X,Y,shift);
		Pressed = false;
		return false;
	}

	bool ToolButton::DoMouseMove(int X, int Y)
	{
		Control::DoMouseMove(X,Y);
		if (Enabled && Visible && IsPointInClient(X-Left, Y-Top))
		{
			Pressed = true;
			return true;
		}
		else
			Pressed = false;
		return false;
	}

	void ToolButton::Draw(int absX, int absY)
	{
		if (!Visible)
			return;
		absX += Left;
		absY += Top;
		auto entry = GetEntry();
		auto & graphics = entry->DrawCommands;
		if (ButtonStyle == bsSeperator)
		{
			graphics.PenColor = Global::Colors.ToolButtonSeperatorColor;
			graphics.DrawLine(absX+1,absY+1, absX+1, absY+Height-2);
			return;
		}
		bool drawbkg = true;
		if (Selected || Global::PointedComponent == this || Global::PointedComponent->IsChildOf(this))
		{
			if (Checked || Pressed)
			{
				graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorPressed1;
			}
			else
			{
				graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorHighlight1;
			}

		}
		else
		{
			if (Checked)
			{
				graphics.SolidBrushColor = Global::Colors.ToolButtonBackColorChecked1;
			}
			else
			{
				drawbkg = false;
			}
		}
		if (drawbkg)
			graphics.FillRectangle(absX,absY,absX+Width-1,absY+Height-1);
		if (Selected || Checked)
		{
			graphics.PenColor = Global::Colors.ToolButtonBorderHighLight;
			graphics.DrawRectangle(absX,absY,absX+Width-1,absY+Height-1);
		}
		if (ButtonStyle == bsNormal)
		{
			int imgX=absX, imgY=absY;
			if (!ShowText)
			{
				if (image)
				{
					imgX += (Width-image->GetWidth())/2;
					imgY += (Height-image->GetHeight())/2;
				}
			}
			else
			{
				if (image)
				{
					imgX += imageLabelPadding;
					imgY += (Height-image->GetHeight())/2;
				}
			}
			if (Enabled)
			{
				if (image)
				{
					graphics.DrawImage(image.Ptr(), imgX, imgY);
				}
			}
			else
			{
				if (imageDisabled)
				{
					graphics.DrawImage(imageDisabled.Ptr(), imgX, imgY);
				}
			}
			if (ShowText)
			{
				int imgw = (image?image->GetWidth():0);
				lblText->Draw(imgX + imgw + imageLabelPadding, absY + (Height-lblText->GetHeight())/2);
			}

		}
		else
		{
			Color color;
			if (Enabled)
				color = Color(0,0,0,255);
			else
				color = Global::Colors.ToolButtonSeperatorColor;
			Array<Vec2, 3> polygon;
			graphics.SolidBrushColor = color;
			graphics.FillTriangle(absX + 3, absY + 10, absX + 7, absY + 10, absX + 5, absY + 12);
		}
				
	}

	ToolStrip::ToolStrip(Container * parent)
		: Container(parent)
	{
		DockStyle = dsTop;
		MultiLine = false;
		FullLineFill = true;
		ShowText = false;
		SetOrientation(Horizontal);
	}

	ToolButton * ToolStrip::AddButton(const String &text, IImage *bmp)
	{
		ToolButton * btn = new ToolButton(this, text, ToolButton::bsNormal, bmp);
		buttons.Add(btn);
		btn->Parent = this;
		PositButtons();
		return btn;
	}

	void ToolStrip::AddSeperator()
	{
		ToolButton * btn = new ToolButton(this, L"", ToolButton::bsSeperator, 0);
		buttons.Add(btn);
		btn->Parent = this;
		PositButtons();
	}

	bool ToolStrip::DoMouseLeave()
	{
		Control::DoMouseLeave();
		for (int i=0; i<buttons.Count(); i++)
			buttons[i]->Selected = false;
		return false;
	}

	void ToolStrip::SetOrientation(ToolStripOrientation ori)
	{
		orientation = ori;
		Padding = 0;
		if (orientation == Horizontal)
		{
			Padding.Left = GetEntry()->GetLineHeight() / 2;
			Padding.Top = Padding.Bottom = Padding.Left / 2;
		}
		else
		{
			Padding.Top = GetEntry()->GetLineHeight() / 2;
			Padding.Left = Padding.Right = Padding.Top / 2;
		}
	}

	ToolButton * ToolStrip::GetButton(int id)
	{
		return buttons[id];
	}

	int ToolStrip::Count()
	{
		return buttons.Count();
	}

	void ToolStrip::SizeChanged()
	{
		Control::SizeChanged();
		PositButtons();
	}

	void ToolStrip::PositButtons()
	{
		int left = Padding.Left;
		if (orientation == Horizontal)
		{
			if (!MultiLine)
			{
				int maxW = 0, maxH = 0;
				for (int i=0; i<buttons.Count(); i++)
				{
					buttons[i]->ShowText = ShowText;
					if (!buttons[i]->Visible)
						continue;
					int w = buttons[i]->MeasureWidth();
					int h = buttons[i]->MeasureHeight();
					if (w>maxW)
						maxW = w;
					if (h>maxH)
						maxH = h;
				}
				for (int i=0; i<buttons.Count(); i++)
				{
					if (!buttons[i]->Visible)
						continue;
					buttons[i]->Posit(left, 0, buttons[i]->MeasureWidth(), maxH);
					left += buttons[i]->GetWidth();
				}
				Width = left + Padding.Right;
				Height = maxH + Padding.Vertical();
			}
		}
		else
		{
			int maxW = 0, maxH = 0;
			int top = Padding.Top;
			for (int i=0; i<buttons.Count(); i++)
			{
				buttons[i]->ShowText = ShowText;
				if (!buttons[i]->Visible)
					continue;
				int w = buttons[i]->MeasureWidth();
				int h = buttons[i]->MeasureHeight();
				if (w>maxW)
					maxW = w;
				if (h>maxH)
					maxH = h;
			}
			for (int i=0; i<buttons.Count(); i++)
			{
				if (!buttons[i]->Visible)
					continue;
				int w = (FullLineFill?Width:maxW);
				buttons[i]->Posit(0, top, w, buttons[i]->MeasureHeight());
				top += buttons[i]->GetHeight();
			}
			Height = top + Padding.Top;
		}
	}

	void ToolStrip::Draw(int absX, int absY)
	{
		auto & graphics = GetEntry()->DrawCommands;
		graphics.SolidBrushColor = Global::Colors.ToolButtonBackColor1;
		graphics.FillRectangle(absX+Left,absY+Top,absX+Left+Width-1,absY+Top+Height-1);
		for (int i=0; i<buttons.Count(); i++)
			buttons[i]->Draw(absX+Left, absY+Top);
	}

	bool ToolStrip::DoMouseDown(int X, int Y, GraphicsUI::SHIFTSTATE shift)
	{
		Control::DoMouseDown(X,Y,shift);
		if (!Visible || !Enabled)
			return false;
		for (int i=0; i<buttons.Count(); i++)
			buttons[i]->DoMouseDown(X-Left, Y-Top, shift);
		return false;
	}

	bool ToolStrip::DoMouseUp(int X, int Y, GraphicsUI::SHIFTSTATE shift)
	{
		Control::DoMouseUp(X,Y,shift);
		if (!Visible || !Enabled)
			return false;
		for (int i=0; i<buttons.Count(); i++)
			buttons[i]->DoMouseUp(X-Left, Y-Top, shift);
		return false;
	}

	bool ToolStrip::DoMouseMove(int X, int Y)
	{
		Control::DoMouseMove(X,Y);
		if (!Visible || !Enabled)
			return false;
		if (!IsPointInClient(X-Left, Y-Top))
			return false;
		for (int i=0; i<buttons.Count(); i++)
		{
			int nx = X-Left;
			int ny = Y-Top;
			bool inside = (nx>buttons[i]->Left && nx<buttons[i]->Left+buttons[i]->GetWidth() &&
				ny>buttons[i]->Top && ny<buttons[i]->Top+buttons[i]->GetHeight());
			if (!buttons[i]->LastInClient && inside)
				buttons[i]->DoMouseEnter();
			if (buttons[i]->LastInClient && !inside)
				buttons[i]->DoMouseLeave();
			buttons[i]->LastInClient = inside;
			buttons[i]->DoMouseMove(X-Left, Y-Top);
		}
		return false;
	}

	StatusPanel::StatusPanel(StatusStrip * parent)
		: Container(parent)
	{
		Init();
		parent->AddItem(this);
	}

	StatusPanel::StatusPanel(StatusStrip * parent, const String & _text, int width, _FillMode fm)
		: Container(parent)
	{
		Init();
		FillMode = fm;
		SetText(_text);
		Width = width;
		Height = (int)(GetEntry()->GetLineHeight() * 1.2f);
		parent->AddItem(this);
	}

	void StatusPanel::Init()
	{
		BackColor = Color(0,0,0,0);
		BorderStyle = BS_NONE;
		FillMode = Fixed;
		Width = 50;
		text = new Label(this);
	}

	void StatusPanel::SetText(const String & _text)
	{
		text->SetText(_text);
	}

	String StatusPanel::GetText()
	{
		return text->GetText();
	}

	int StatusPanel::MeasureWidth()
	{
		if (FillMode == Fixed)
			return Width;
		else if (FillMode == AutoSize)
			return text->GetWidth();
		else
			return -1;
	}

	void StatusPanel::Draw(int absX, int absY)
	{
		Control::Draw(absX, absY);
		auto entry = GetEntry();
		entry->ClipRects->AddRect(Rect(absX + Left, absY + Top, Width - text->TextHeight, Height));
		text->Draw(absX+Left, absY+Top + ((Height - text->TextHeight) >> 1));
		entry->ClipRects->PopRect();
	}
		
	StatusStrip::StatusStrip(Container * parent)
		: Container(parent)
	{
		DockStyle = dsBottom;
		DoDpiChanged();
	}

	void StatusStrip::AddItem(GraphicsUI::StatusPanel *panel)
	{
		panels.Add(panel);
		Height = Math::Max(Height, panel->GetHeight());
	}

	int StatusStrip::Count()
	{
		return panels.Count();
	}

	StatusPanel * StatusStrip::GetItem(int id)
	{
		return panels[id];
	}

	void StatusStrip::PositItems()
	{
		int fc = 0;
		int w = Width - Padding.Horizontal();
		for (int i=0; i<panels.Count(); i++)
		{
			int cw = panels[i]->MeasureWidth();
			if (cw!=-1)
				w -= cw;
			else
				fc ++;
		}
		if (fc == 0)
			fc = 1;
		int fw = w/fc;
		int h = Height - Padding.Vertical();
		int left = 0;
		for (int i=0; i<panels.Count(); i++)
		{
			int cw = panels[i]->MeasureWidth();
			if (cw != -1)
			{
				panels[i]->Posit(left, 0, cw, h);
				left += cw;
			}
			else
			{
				panels[i]->Posit(left, 0, fw, h);
				left += fw;
			}
		}
	}

	void StatusStrip::Draw(int absX, int absY)
	{
		absX += Left;
		absY += Top;
		PositItems();
		auto & graphics = GetEntry()->DrawCommands;
		graphics.SolidBrushColor = Global::Colors.StatusStripBackColor1;
		graphics.FillRectangle(absX, absY, absX+Width, absY+Height);
		for (int i=0; i<panels.Count(); i++)
		{
			panels[i]->Draw(absX, absY);
		}
	}

	bool StatusStrip::DoMouseMove(int X, int Y)
	{
		Control::DoMouseMove(X,Y);
		if (!Enabled || !Visible)
			return false;
		for (int i=0; i<panels.Count(); i++)
		{
			panels[i]->DoMouseMove(X-Left, Y-Top);
		}
		return false;
	}

	bool StatusStrip::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X,Y,Shift);
		if (!Enabled || !Visible)
			return false;
		for (int i=0; i<panels.Count(); i++)
		{
			panels[i]->DoMouseUp(X-Left, Y-Top, Shift);
		}
		return false;
	}

	void StatusStrip::DoDpiChanged()
	{
		Container::DoDpiChanged();
		Padding.Top = Padding.Bottom = 0;
		Padding.Left = Padding.Right = GetEntry()->GetLineHeight() / 2;
		Height = (int)(GetEntry()->GetLineHeight() * 1.2f);
		PositItems();
	}

	bool StatusStrip::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseDown(X,Y,Shift);
		if (!Enabled || !Visible)
			return false;
		for (int i=0; i<panels.Count(); i++)
		{
			panels[i]->DoMouseDown(X-Left, Y-Top, Shift);
		}
		return false;
	}

	TabPage::TabPage(TabControl * parent)
		: Container(parent)
	{
		text = new Label(this);
		text->Visible = false;
		BorderStyle = BS_NONE;
		parent->AddItem(this);
		imageTextPadding = GetEntry()->GetLineHeight() / 2;
		Padding = imageTextPadding;
	}

	TabPage::TabPage(TabControl * parent, CoreLib::String text)
		: TabPage(parent)
	{
		SetText(text);
	}

	void TabPage::SetText(const String &_text)
	{
		text->SetText(_text);
	}

	String TabPage::GetText()
	{
		return text->GetText();
	}

	void TabPage::SetImage(IImage *bitmap)
	{
		image = bitmap;
	}

	int TabPage::MeasureWidth(TabControl::_TabStyle style)
	{
		switch (style)
		{
		case TabControl::tsImage:
			return (image?image->GetWidth():0);
		case TabControl::tsText:
			return text->GetWidth();
		case TabControl::tsTextImage:
			return text->GetWidth() + (image?image->GetWidth() + imageTextPadding : 0);
		default:
			return 0;
		}
	}

	int TabPage::MeasureHeight(TabControl::_TabStyle style)
	{
		switch (style)
		{
		case TabControl::tsImage:
			return (image?image->GetHeight():0);
		case TabControl::tsText:
			return text->GetHeight() + Padding.Vertical();
		case TabControl::tsTextImage:
			return Math::Max(text->GetHeight(), (image?image->GetHeight():0));
		default:
			return 0;
		}
	}

	TabControl::TabControl(Container * parent)
		: Container(parent)
	{
		highlightItem = -1;
		SelectedIndex = -1;
		CanClose = false;
		CanMove = false;
		TabStyle = tsTextImage;
		TabPosition = tpTop;
		DoDpiChanged();
	}

	void TabControl::SetClient()
	{
		if (TabPosition == tpTop)
		{
			clientRect.x = 0;
			clientRect.y = headerHeight;
			clientRect.w = Width;
			clientRect.h = Height - headerHeight;
		}
		else
		{
			clientRect.x = 0;
			clientRect.y = 0;
			clientRect.w = Width;
			clientRect.h = Height - headerHeight;
		}
		for (int i=0; i<pages.Count(); i++)
		{
			pages[i]->Posit(0, 0, clientRect.w, clientRect.h);
		}
	}

	void TabControl::AddItem(TabPage *page)
	{
		page->Parent = this;
		page->Visible = false;
		pages.Add(page);
		headerHeight = MeasureHeight();
		SetClient();
		if (SelectedIndex == -1)
			SwitchPage(0);
	}

	void TabControl::RemoveItem(TabPage * page)
	{
		int fid = pages.IndexOf(page);
		if (fid != -1)
		{
			pages[fid] = 0;
			pages.RemoveAt(fid);
		}
		RemoveChild(page);
		if (SelectedIndex == fid)
			SwitchPage(SelectedIndex-1);
		headerHeight = MeasureHeight();
		SetClient();
	}

	void TabControl::SwitchPage(int id)
	{
		for (int i=0; i<pages.Count(); i++)
			pages[i]->Visible = false;
		pages[id]->Visible = true;
		SelectedIndex = id;
	}

	TabPage * TabControl::GetItem(int id)
	{
		return pages[id];
	}

	TabPage * TabControl::GetSelectedItem()
	{
		if (SelectedIndex != -1)
			return pages[SelectedIndex];
		else
			return 0;
	}

	void TabPage::DrawHeader(int x, int y, int h, const MarginValues & headerPadding, TabControl::_TabStyle style)
	{
		switch (style)
		{
		case TabControl::tsTextImage:
			{
				int cw = x + headerPadding.Left;
				if (image)
				{
					GetEntry()->DrawCommands.DrawImage(image.Ptr(), cw, y + headerPadding.Top);
					cw += image->GetWidth() + imageTextPadding;
				}
				text->Draw(cw, y + (h-text->GetHeight())/2);
			}
			break;
		case TabControl::tsText:
			{
				text->Draw(x + Padding.Left, y + Padding.Top);
			}
			break;
		case TabControl::tsImage:
			{
				if (image)
					GetEntry()->DrawCommands.DrawImage(image.Ptr(), x + headerPadding.Left, y + headerPadding.Top);
			}
			break;
		}
	}

	int TabControl::MeasureHeight()
	{
		int h = 0;
		for (int i=0; i<pages.Count(); i++)
		{
			int ch = pages[i]->MeasureHeight(TabStyle);
			if (ch>h)
				h = ch;
		}
		return h + HeaderPadding.Vertical();
	}

	int TabControl::HitTest(int X, int Y)
	{
		bool inHeader = false;
		if (TabPosition == tpTop)
		{
			inHeader = (Y < headerHeight && Y > 0);
		}
		else
			inHeader = (Y > Height-headerHeight && Y<Height);
		if (inHeader)
		{
			int cw = 0;
			for (int i=0; i<pages.Count(); i++)
			{
				int pw = pages[i]->MeasureWidth(TabStyle) + HeaderPadding.Horizontal();
				if (X>cw && X<=cw+pw)
				{
					return i;
				}
				cw += pw;
			}
			return -1;
		}
		else
			return -1;
	}

	void TabControl::SizeChanged()
	{
		Control::SizeChanged();
		SetClient();
	}

	bool TabControl::DoMouseMove(int X, int Y)
	{
		Container::DoMouseMove(X,Y);
		if (!Visible || !Enabled)
			return false;
		highlightItem = HitTest(X,Y);
		return false;
	}

	bool TabControl::DoMouseDown(int X, int Y, GraphicsUI::SHIFTSTATE Shift)
	{
		Container::DoMouseDown(X,Y,Shift);
		if (!Visible || !Enabled)
			return false;
		int citem = HitTest(X,Y);
		if (citem != -1)
			SwitchPage(citem);
		return false;
	}

	bool TabControl::DoMouseUp(int X, int Y, GraphicsUI::SHIFTSTATE Shift)
	{
		Container::DoMouseUp(X,Y,Shift);
		if (!Visible || !Enabled)
			return false;
		return false;
	}

	void TabControl::DoDpiChanged()
	{
		Container::DoDpiChanged();
		HeaderPadding.Left = HeaderPadding.Right = GetEntry()->GetLineHeight() / 2;
		HeaderPadding.Top = HeaderPadding.Bottom = HeaderPadding.Left / 4;
		headerHeight = MeasureHeight() + HeaderPadding.Vertical();
	}

	void TabControl::Draw(int absX, int absY)
	{
		SetClient();
		absX += Left;
		absY += Top;
		if (!Visible)
			return;
		int maxWidth = Width-16;
		if (CanClose)
			Width -= 16;
		int cw = 0;
		TabPage * page = GetSelectedItem();
		auto entry = GetEntry();
		if (page)
		{
			entry->ClipRects->AddRect(Rect(absX+clientRect.x, absY+clientRect.y, absX+clientRect.x+clientRect.w, absY+clientRect.y+clientRect.h));
			page->Draw(absX+clientRect.x, absY+clientRect.y);
			entry->ClipRects->PopRect();
		}
		auto & graphics = entry->DrawCommands;
		graphics.SetRenderTransform(absX, absY);
		int h0 = Height-headerHeight-1;
		for (int i=0; i<pages.Count(); i++)
		{
			int pw = pages[i]->MeasureWidth(TabStyle) + HeaderPadding.Horizontal();
			if (cw + pw > maxWidth)
				break;
			if (SelectedIndex != i && highlightItem != i)
			{
				graphics.SolidBrushColor = Global::Colors.TabPageItemBackColor1;
			}
			else if (SelectedIndex == i)
			{
				graphics.SolidBrushColor = Global::Colors.TabPageItemSelectedBackColor1;
			}
			else
			{
				graphics.SolidBrushColor = Global::Colors.TabPageItemHighlightBackColor1;
			}
			graphics.PenColor = Global::Colors.TabPageBorderColor;
			if (TabPosition == tpTop)
			{
				graphics.FillRectangle(cw, 0, cw+pw, headerHeight);
				graphics.DrawLine(cw,0,cw+pw,0);
				graphics.DrawLine(cw,0, cw, headerHeight - 2);
				graphics.DrawLine(cw+pw,0, cw+pw, headerHeight - 2);
				if (SelectedIndex != i)
				{
					graphics.DrawLine(cw, headerHeight - 1, cw+pw, headerHeight - 1);
				}
				pages[i]->DrawHeader(cw, 0, headerHeight, HeaderPadding, TabStyle);
			}
			else
			{
				graphics.FillRectangle(cw, h0+headerHeight, cw+pw, h0);
				graphics.DrawLine(cw,h0, cw, h0+headerHeight);
				graphics.DrawLine(cw+pw,h0, cw+pw, h0+headerHeight);
				graphics.DrawLine(cw, h0+headerHeight, cw+pw, h0+headerHeight);
				if (SelectedIndex != i)
				{
					graphics.DrawLine(cw,h0,cw+pw,h0);
				}
				pages[i]->DrawHeader(cw, h0, headerHeight, HeaderPadding, TabStyle);
			}
				
			cw += pw;
		}
			
		if (TabPosition == tpTop)
		{
			graphics.DrawLine(cw,headerHeight, Width, headerHeight);
			graphics.DrawLine(0,headerHeight,0,Height-1);
			graphics.DrawLine(Width-1,headerHeight,Width-1,Height-1);
			graphics.DrawLine(0,Height-1,Width,Height-1);
		}
		else
		{
			graphics.DrawLine(cw,h0, Width, h0);
			graphics.DrawLine(0,0,0,Height-headerHeight);
			graphics.DrawLine(Width-1,0,Width-1,Height-1-headerHeight);
			graphics.DrawLine(0,0,Width,0);
		}
		graphics.SetRenderTransform(0, 0);
	}

	class DeviceNotReadyException
	{};

	UpDown::UpDown(Container * parent, GraphicsUI::TextBox *txtBox, float _min, float _max, float minInc, float maxInc)
		: Container(parent)
	{
		Digits = 3;
		state = 0;
		text = txtBox;
		Min = _min;
		Max = _max;
		MinIncrement = minInc;
		MaxIncrement = maxInc;
		Left = text->Left + text->GetWidth();
		Height = text->GetHeight();
		Top = text->Top;
		Width = 16;
		btnUp = new Button(this);
		btnUp->SetHeight(Height/2);
		btnDown = new Button(this);
		btnDown->SetHeight(Height/2);
		btnUp->SetWidth(Width);
		btnDown->SetWidth(Width);
		auto symFont = GetEntry()->System->LoadDefaultFont(DefaultFontType::Symbol);
		btnUp->SetFont(symFont);
		btnDown->SetFont(symFont);
		btnUp->SetText(L"5");
		btnDown->SetText(L"6");
	}

	UpDown::~UpDown()
	{
		GetEntry()->UnSubscribeTickEvent(this);
	}

	void UpDown::Draw(int absX, int absY)
	{
		btnUp->BorderStyle = btnDown->BorderStyle = BS_RAISED;
		if (state == 1)
		{
			btnUp->BorderStyle = BS_LOWERED;
		}
		else if (state == 2)
		{
			btnDown->BorderStyle = BS_LOWERED;
		}
		absX += Left;
		absY += Top;
		btnUp->Draw(absX,absY);
		btnDown->Draw(absX,absY+btnUp->GetHeight());
	}

	bool UpDown::DoTick()
	{
		float val = (float)StringToDouble(text->GetText());
		if (state == 1)
			val += inc;
		else
			val -= inc;
		val = Math::Max(Min, val);
		val = Math::Min(Max, val);
		text->SetText(String(val, (L"%." + String(Digits) + L"f").Buffer()));
		return true;
	}

	bool UpDown::DoMouseDown(int X, int Y, SHIFTSTATE Shift)
	{
		if (!Enabled || !Visible)
			return false;
		Control::DoMouseDown(X,Y, Shift);
		ldY = Y;
		if (Y-Top<Height/2)
			state = 1;
		else
			state = 2;
		inc = MinIncrement;
		Global::MouseCaptureControl = this;
		return false;
	}

	bool UpDown::DoMouseUp(int X, int Y, SHIFTSTATE Shift)
	{
		Control::DoMouseUp(X,Y,Shift);
		state = 0;
		GetEntry()->UnSubscribeTickEvent(this);
		ReleaseMouse();
		return false;
	}

	bool UpDown::DoMouseHover()
	{
		if (state != 0)
		{
			GetEntry()->SubscribeTickEvent(this);
		}
		return true;
	}

	bool UpDown::DoMouseMove(int /*X*/, int Y)
	{
		if (state)
		{
			int dY = Y-ldY;
			float s = fabs(dY/100.0f);
			inc = MinIncrement * (1.0f-s) + MaxIncrement * s;
		}
		return false;
	}
	UIEntry * Control::GetEntry()
	{
		if (entryCache == nullptr)
		{
			Control * parent = Parent;
			if (parent)
				entryCache = parent->GetEntry();
		}
		return entryCache;
	}
	void VScrollPanel::ScrollBar_Changed(UI_Base * /*sender*/)
	{
		content->Top = -vscrollBar->GetPosition();
	}

	VScrollPanel::VScrollPanel(Container * parent)
		: Container(parent)
	{
		vscrollBar = new ScrollBar(this, false);
		content = new Container(this, false);
		Container::AddChild(vscrollBar);
		Container::AddChild(content);
		vscrollBar->SetOrientation(SO_VERTICAL);
		vscrollBar->Posit(0, 0, Global::SCROLLBAR_BUTTON_SIZE, 50);
		vscrollBar->DockStyle = dsRight;
		vscrollBar->SmallChange = 30;
		vscrollBar->OnChanged.Bind(this, &VScrollPanel::ScrollBar_Changed);
		content->AutoHeight = true;
		BorderStyle = content->BorderStyle = BS_NONE;
		BackColor.A = content->BackColor.A = 0;
	}
	void VScrollPanel::SizeChanged()
	{
		content->SizeChanged();
		//for (auto & ctrl : content->GetChildren())
		//	maxY = Math::Max(ctrl->Top + ctrl->GetHeight(), maxY);
		int maxY = content->GetHeight();
		auto entry = GetEntry();
		vscrollBar->LargeChange = Math::Max(Height - 30, 10);
		if (maxY > Height)
		{
			maxY += entry->GetLineHeight() * 3;
			if (!vscrollBar->Visible)
			{
				vscrollBar->Visible = true;
				SizeChanged();
				return;
			}
			int vmax = maxY - Height;
			vscrollBar->SetValue(0, vmax, Math::Clamp(vscrollBar->GetPosition(), 0, vmax), Height);
			vscrollBar->Visible = true;
		}
		else
		{
			vscrollBar->SetPosition(0);
			vscrollBar->Visible = false;
		}
		vscrollBar->Posit(0, 0, Global::SCROLLBAR_BUTTON_SIZE, Height - 2);
		content->Posit(0, -vscrollBar->GetPosition(), vscrollBar->Visible ? Width - vscrollBar->GetWidth() : Width, maxY);
		Container::SizeChanged();
	}
	void VScrollPanel::AddChild(Control * ctrl)
	{
		content->AddChild(ctrl);
		SizeChanged();
	}
	void VScrollPanel::RemoveChild(Control * ctrl)
	{
		content->RemoveChild(ctrl);
		SizeChanged();
	}
	bool VScrollPanel::DoMouseWheel(int delta)
	{
		if (vscrollBar->Visible)
		{
			int nPos = vscrollBar->GetPosition() + (delta < 0 ? 1 : -1) * GetEntry()->GetLineHeight() * 3;
			nPos = Math::Clamp(nPos, vscrollBar->GetMin(), vscrollBar->GetMax());
			vscrollBar->SetPosition(nPos);
			return true;
		}
		return false;
	}
	void VScrollPanel::DoFocusChange()
	{
		Container::DoFocusChange();
		auto focusedCtrl = GetEntry()->FocusedControl;
		if (focusedCtrl && focusedCtrl->IsChildOf(content))
		{
			auto pos = focusedCtrl->GetRelativePos(content);
			if (pos.y - vscrollBar->GetPosition() < 0)
			{
				vscrollBar->SetPosition(Math::Clamp(pos.y, vscrollBar->GetMin(), vscrollBar->GetMax()));
			}
			else if (pos.y - vscrollBar->GetPosition() + focusedCtrl->GetHeight() > Height)
			{
				vscrollBar->SetPosition(Math::Clamp(pos.y - Height + focusedCtrl->GetHeight(), vscrollBar->GetMin(), vscrollBar->GetMax()));
			}
		}
	}
	ContainerLayoutType VScrollPanel::GetLayout()
	{
		return content->GetLayout();
	}
	void VScrollPanel::SetLayout(ContainerLayoutType pLayout)
	{
		content->SetLayout(pLayout);
	}
	void VScrollPanel::ClearChildren()
	{
		for (auto & child : content->GetChildren())
			child = nullptr;
		content->GetChildren().Clear();
	}
	int VScrollPanel::GetClientWidth()
	{
		return content->GetWidth();
	}
	int VScrollPanel::GetClientHeight()
	{
		return content->GetHeight();
	}
	Line::Line(Container * owner)
		: Control(owner)
	{
	}
	void Line::Draw(int absX, int absY)
	{
		auto & graphics = GetEntry()->DrawCommands;
		graphics.PenColor = BorderColor;
		graphics.DrawLine(absX + Left, absY + Top, absX + Left + Width - 1, absY + Top + Height - 1);
	}

	CommandForm::CommandForm(UIEntry * parent)
		:Form(parent)
	{
		this->SetText(L"Command Prompt");
		txtCmd = new TextBox(this);
		txtCmd->SetHeight((int)(GetEntry()->GetLineHeight() * 1.2f));
		txtCmd->DockStyle = dsBottom;
		textBox = CreateMultiLineTextBox(this);
		textBox->DockStyle = dsFill;
		textBox->BorderStyle = BS_NONE;
		textBox->TabStop = false;
		textBox->SetReadOnly(true);
		txtCmd->OnKeyDown.Bind([=](UI_Base *, UIKeyEventArgs & e)
		{
			if (e.Key == Keys::Return)
			{
				auto cmdText = txtCmd->GetText();
				if (cmdText.Length())
				{
					commandHistories.Add(cmdText);
					cmdPtr = commandHistories.Count();
					txtCmd->SetText(L"");
					Write(L"> " + cmdText + L"\n");
					OnCommand(cmdText);

					auto pos = textBox->GetCaretPos();
					if (pos.Col > 0)
						textBox->InsertText(L"\n");
				}
			}
			else if (e.Key == Keys::Up)
			{
				cmdPtr--;
				if (cmdPtr < 0)
					cmdPtr = 0;
				if (cmdPtr < commandHistories.Count())
				{
					txtCmd->SetText(commandHistories[cmdPtr]);
				}
			}
			else if (e.Key == Keys::Down)
			{
				cmdPtr++;
				if (cmdPtr >= commandHistories.Count())
					cmdPtr = commandHistories.Count();
				if (cmdPtr < commandHistories.Count())
					txtCmd->SetText(commandHistories[cmdPtr]);
				else
					txtCmd->SetText(L"");
			}
		});
		this->Posit(10, 10, 500, 400);
	}
	void CommandForm::Write(const CoreLib::String & text)
	{
		textBox->MoveCaretToEnd();
		textBox->InsertText(text);
		while (textBox->GetLineCount() > 2048)
			textBox->DeleteLine(0);
	}
	bool CommandForm::DoMouseUp(int x, int y, SHIFTSTATE shift)
	{
		Form::DoMouseUp(x, y, shift);
		if (this->Visible)
			txtCmd->SetFocus();
		return true;
	}
	void UICommandLineWriter::Write(const String & text)
	{
		cmdForm->Write(text);
		OnWriteText(text);
	}
}