#ifndef CORE_LIB_GL_FORM_H
#define CORE_LIB_GL_FORM_H

#include "../WinForm/WinApp.h"
#include "../WinForm/WinForm.h"
#include "OpenGLHardwareRenderer.h"
#include "../Graphics/LibUI.h"
#include "UISystem_WinGL.h"
#pragma comment(lib, "opengl32.lib")
namespace CoreLib
{
	namespace WinForm
	{
		class GLForm : public BaseForm
		{
		protected:
			virtual void Create()
			{
				handle = ::CreateWindow(Application::GLFormClassName, 0, WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, Application::GetHandle(), NULL);
				if (!handle)
				{
					throw "Failed to create window.";
				}
				Application::RegisterComponent(this);
				SubClass();
				InitGL();
			}
		protected:
			RefPtr<GL::HardwareRenderer> glContext = nullptr;
			RefPtr<GraphicsUI::UIEntry> uiEntry;
			RefPtr<GraphicsUI::WinGLSystemInterface> uiSystemInterface;
			void InitGL()
			{
				glContext = new GL::HardwareRenderer();
				glContext->Initialize((GL::GUIHandle)this->GetHandle());
				uiSystemInterface = new GraphicsUI::WinGLSystemInterface(glContext.Ptr());
				uiEntry = new GraphicsUI::UIEntry(GetClientWidth(), GetClientHeight(), uiSystemInterface.Ptr());
				uiEntry->BackColor.A = 0;
				uiSystemInterface->SetEntry(uiEntry.Ptr());
			}
			int ProcessMessage(WinMessage & msg) override
			{
				int rs = uiSystemInterface->HandleSystemMessage(msg.hWnd, msg.message, msg.wParam, msg.lParam);
				if (rs == -1)
					return BaseForm::ProcessMessage(msg);
				return rs;
			}
			virtual void _OnResize() override
			{
				BaseForm::_OnResize();
				uiSystemInterface->SetResolution(GetClientWidth(), GetClientHeight());
			}
		public:
			GLForm()
			{
				Create();
				wantChars = true;
			}
			~GLForm()
			{
				uiEntry = nullptr;
				uiSystemInterface = nullptr;
				glContext->Destroy();
				glContext = nullptr;
				Application::UnRegisterComponent(this);
			}
			void DrawUIOverlay()
			{
				uiSystemInterface->ExecuteDrawCommands(uiEntry->DrawUI());
			}
		};
	}
}

#endif