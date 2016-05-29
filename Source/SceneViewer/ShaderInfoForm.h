#pragma once

#include "CoreLib/WinForm/WinForm.h"
#include "CoreLib/WinForm/WinButtons.h"
#include "CoreLib/WinForm/WinApp.h"
#include "CoreLib/WinForm/WinCommonDlg.h"
#include "CoreLib/WinForm/WinTextBox.h"
#include "CoreLib/WinForm/WinListBox.h"
#include "LibGL/OpenGLHardwareRenderer.h"
#include "ShaderCompiler.h"
#include "Schedule.h"

using namespace CoreLib::WinForm;

namespace SceneViewer
{
	class ShaderInfoForm : public Form
	{
	private:
		Spire::Compiler::ShaderMetaData metaData;
		bool isMouseDown = false;
		int lastX, lastY;
		int maxY = 0;
		void Form_Paint(Object*, PaintEventArgs e);
		void Form_MouseDown(Object *, MouseEventArgs e);
		void Form_MouseMove(Object *, MouseEventArgs e);
		void Form_MouseUp(Object *, MouseEventArgs e);

		int offsetX = 0, offsetY = 0;
	public:

		ShaderInfoForm();
		void Update(const Spire::Compiler::ShaderMetaData & pMetaData);

	};
}