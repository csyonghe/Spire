#include "ShaderEditorForm.h"

namespace SceneViewer
{
	using namespace GraphicsUI;
	using namespace CoreLib::IO;

	ShaderEditorForm::ShaderEditorForm(UIEntry * parent, IFont * font)
		: Form(parent)
	{
		auto menuEditor = new GraphicsUI::Menu(this, GraphicsUI::Menu::msMainMenu);
		auto menuFile = new GraphicsUI::MenuItem(menuEditor, L"&File");
		auto menuSave = new GraphicsUI::MenuItem(menuFile, L"&Save and Apply", L"Ctrl+S");
		auto menuEdit = new GraphicsUI::MenuItem(menuEditor, L"&Edit");
		auto menuUndo = new GraphicsUI::MenuItem(menuEdit, L"&Undo", L"Ctrl+Z");
		auto menuRedo = new GraphicsUI::MenuItem(menuEdit, L"&Redo", L"Ctrl+Y");
		new GraphicsUI::MenuItem(menuEdit);
		auto menuCut = new GraphicsUI::MenuItem(menuEdit, L"&Cut", L"Ctrl+X");
		auto menuCopy = new GraphicsUI::MenuItem(menuEdit, L"&Copy", L"Ctrl+C");
		auto menuPaste = new GraphicsUI::MenuItem(menuEdit, L"&Paste", L"Ctrl+V");
		auto menuSelAll = new GraphicsUI::MenuItem(menuEdit, L"&Select All", L"Ctrl+A");
		new GraphicsUI::MenuItem(menuEdit);
		auto menuIndent = new GraphicsUI::MenuItem(menuEdit, L"&Increase Line Indent");
		auto menuUnIndent = new GraphicsUI::MenuItem(menuEdit, L"&Decrease Line Indent");
		auto menuView = new GraphicsUI::MenuItem(menuEditor, L"&View");
		auto menuWordWrap = new GraphicsUI::MenuItem(menuView, L"&Word Wrap");
		menuWordWrap->Checked = true;
		textBox = GraphicsUI::CreateMultiLineTextBox(this);
		textBox->DockStyle = GraphicsUI::Control::dsFill;
		textBox->SetFont(font);
		this->Posit(100, 100, 600, 500);
		textBox->OnKeyDown.Bind([=](UI_Base *, GraphicsUI::UIKeyEventArgs & e)
		{
			if (e.Key == L'S' && e.Shift == GraphicsUI::SS_CONTROL)
				SaveAndApply();
		});
		menuSave->OnClick.Bind([=](auto)
		{
			SaveAndApply();
		});

		menuIndent->OnClick.Bind([=](auto)
		{
			textBox->IncreaseIndent();
		});
		menuUnIndent->OnClick.Bind([=](auto)
		{
			textBox->DecreaseIndent();
		});
		menuWordWrap->OnClick.Bind([=](auto)
		{
			menuWordWrap->Checked = !menuWordWrap->Checked;
			textBox->SetWordWrap(menuWordWrap->Checked);
		});
		menuUndo->OnClick.Bind([=](auto)
		{
			textBox->Undo();
		});
		menuRedo->OnClick.Bind([=](auto)
		{
			textBox->Redo();
		});
		menuCut->OnClick.Bind([=](auto)
		{
			textBox->Cut();
		});
		menuCopy->OnClick.Bind([=](auto)
		{
			textBox->Copy();
		});
		menuCut->OnClick.Bind([=](auto)
		{
			textBox->Cut();
		});
		menuPaste->OnClick.Bind([=](auto)
		{
			textBox->Paste();
		});
		menuSelAll->OnClick.Bind([=](auto)
		{
			textBox->SelectAll();
		});
	}

	void ShaderEditorForm::SaveAndApply()
	{
		try
		{
			if (shaderFileName.Length())
			{
				File::WriteAllText(shaderFileName, textBox->GetText());
				OnShaderChange();
			}
		}
		catch (const Exception &)
		{
		}
	}

	void ShaderEditorForm::SetShaderFile(String pShaderName, String fileName)
	{
		shaderFileName = fileName;
		this->shaderName = pShaderName;
		this->SetText(pShaderName);
		textBox->SetText(File::ReadAllText(shaderFileName));
	}

}
