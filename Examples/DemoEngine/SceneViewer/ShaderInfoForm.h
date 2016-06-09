#ifndef SHADER_INFO_FORM_H
#define SHADER_INFO_FORM_H

#include "CoreLib/Graphics.h"
#include "Spire.h"

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
#endif