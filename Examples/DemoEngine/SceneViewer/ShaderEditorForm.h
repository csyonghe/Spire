#ifndef SHADER_EDITOR_FORM_H
#define SHADER_EDITOR_FORM_H

#include "CoreLib/WinForm.h"
#include "CoreLib/LibUI.h"
#include "Spire.h"

using namespace CoreLib::WinForm;

namespace SceneViewer
{
	class ShaderEditorForm : public GraphicsUI::Form
	{
	private:
		GraphicsUI::MultiLineTextBox * textBox;
	public:
		GraphicsUI::StatusPanel * pnlStatus;
		String shaderFileName, shaderName;
		ShaderEditorForm(GraphicsUI::UIEntry * parent, GraphicsUI::IFont * font);
		Event<> OnShaderChange;
		void SaveAndApply();
		void SetShaderFile(String shaderName, String fileName);
	};
}
#endif