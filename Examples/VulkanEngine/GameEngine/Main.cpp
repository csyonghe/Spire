#include "CoreLib/Basic.h"
#include "CoreLib/PerformanceCounter.h"
#include "CoreLib/WinForm/WinForm.h"
#include "CoreLib/WinForm/WinApp.h"
#include "Engine.h"

namespace GameEngine
{
	using namespace CoreLib;
	using namespace CoreLib::WinForm;

	class MainForm : public CoreLib::WinForm::Form
	{
	protected:
		int ProcessMessage(WinMessage & msg) override
		{
			auto engine = Engine::Instance();
			int rs = -1;
			if (engine)
			{
				rs = engine->HandleWindowsMessage(msg.hWnd, msg.message, msg.wParam, msg.lParam);
			}
			if (rs == -1)
				return BaseForm::ProcessMessage(msg);
			return rs;
		}
	public:
		MainForm()
		{
			wantChars = true;
			SetText(L"Game Engine");
			OnResized.Bind(this, &MainForm::FormResized);
			OnFocus.Bind([](auto, auto) {Engine::Instance()->EnableInput(true); });
			OnLostFocus.Bind([](auto, auto) {Engine::Instance()->EnableInput(false); });
		}
		
		void FormResized(Object *, EventArgs)
		{
			Engine::Instance()->Resize(GetClientWidth(), GetClientHeight());
		}
		void MainLoop(Object *, EventArgs)
		{
			auto instance = Engine::Instance();
			if (instance)
				instance->Tick();
		}
	};

}

using namespace GameEngine;
using namespace CoreLib::WinForm;

#define COMMAND true
#define WINDOWED !COMMAND

String RemoveQuote(String dir)
{
	if (dir.StartsWith(L"\""))
		return dir.SubString(1, dir.Length() - 2);
	return dir;
}

#if COMMAND
int wmain(int /*argc*/, const wchar_t ** /*argv*/)
#elif WINDOWED
int wWinMain(
	_In_ HINSTANCE /*hInstance*/,
	_In_ HINSTANCE /*hPrevInstance*/,
	_In_ LPWSTR     /*lpCmdLine*/,
	_In_ int       /*nCmdShow*/
)
#endif
{
	Application::Init();
	{
		auto form = new MainForm();
		Application::SetMainLoopEventHandler(new CoreLib::WinForm::NotifyEvent(form, &MainForm::MainLoop));
		EngineInitArguments args;
		args.API = RenderAPI::OpenGL;
		args.Width = form->GetClientWidth();
		args.Height = form->GetClientHeight();
		args.Window = (WindowHandle)form->GetHandle();
		args.GpuId = 0;

		CoreLib::String wat = Application::GetCommandLine();
		CommandLineParser parser(Application::GetCommandLine());
		if(parser.OptionExists(L"-vk"))
			args.API = RenderAPI::Vulkan;
		if (parser.OptionExists(L"-dir"))
			args.GameDirectory = RemoveQuote(parser.GetOptionValue(L"-dir"));
		if (parser.OptionExists(L"-enginedir"))
			args.EngineDirectory = RemoveQuote(parser.GetOptionValue(L"-enginedir"));
		if (parser.OptionExists(L"-gpu"))
			args.GpuId = StringToInt(parser.GetOptionValue(L"-gpu"));

		Engine::Init(args);
		
		Application::Run(form, true);
		
		Engine::Destroy();
	}
	Application::Dispose();
}