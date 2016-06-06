#pragma once

#include "LibGL/GLForm.h"
#include "CoreLib/Graphics/LibUI.h"

#include "LibGL/OpenGLHardwareRenderer.h"
#include "ShaderCompiler.h"
#include "Schedule.h"

using namespace CoreLib::WinForm;

namespace SceneViewer
{
	class ShaderInfoForm : public GraphicsUI::Form
	{
	private:
		Spire::Compiler::ShaderMetaData metaData;
		bool isMouseDown = false;
		int lastX, lastY;
		int maxY = 0;
		GraphicsUI::Container * contentBox;
		int offsetX = 0, offsetY = 0;
	public:
		ShaderInfoForm(GraphicsUI::UIEntry * entry);
		void Update(const Spire::Compiler::ShaderMetaData & pMetaData);
		virtual bool DoMouseMove(int x, int y) override;
		virtual bool DoMouseDown(int x, int y, GraphicsUI::SHIFTSTATE shift) override;
		virtual bool DoMouseUp(int x, int y, GraphicsUI::SHIFTSTATE shift) override;
	};
}