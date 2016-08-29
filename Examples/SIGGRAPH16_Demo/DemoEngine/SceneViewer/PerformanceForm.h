#ifndef PERFORMANCE_FORM_H
#define PERFORMANCE_FORM_H

#include "CoreLib/WinForm.h"
#include "CoreLib/LibUI.h"
#include "CoreLib/LibGL.h"
#include "Spire.h"

using namespace CoreLib::WinForm;

namespace SceneViewer
{
	class PerformanceForm : public GraphicsUI::Form
	{
	private:
		Spire::Compiler::ShaderMetaData metaData;
		GraphicsUI::Label * lblPerf;
	public:
		PerformanceForm(GraphicsUI::UIEntry * entry, GraphicsUI::WinGLSystemInterface * sys)
			: GraphicsUI::Form(entry)
		{
			lblPerf = new GraphicsUI::Label(this);
			lblPerf->AutoSize = true;
			lblPerf->Posit(5, 5, 100, GraphicsUI::emToPixel(5.0f));
			lblPerf->SetFont(sys->LoadFont(GraphicsUI::Font(L"Segoe UI", 26, true, false, false)));
			lblPerf->SetText(L"0 ms");
			SetText(L"Performance");
			this->Posit(entry->GetWidth() - GraphicsUI::emToPixel(10.0f), 10, GraphicsUI::emToPixel(9.0f), GraphicsUI::emToPixel(5.0f));
		}
		void Update(float time)
		{
			lblPerf->SetText(String(time, L"%.2g") + L"ms");
			//this->SetWidth(Math::Max(this->GetWidth(), lblPerf->GetWidth() + 10));
			//this->SetHeight(Math::Max((int)(lblPerf->GetHeight() * 1.5f), this->GetHeight()));
		}
	};
}
#endif