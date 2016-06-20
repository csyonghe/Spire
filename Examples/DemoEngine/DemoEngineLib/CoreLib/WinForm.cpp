/***********************************************************************

CoreLib - The MIT License (MIT)
Copyright (c) 2016, Yong He

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.

***********************************************************************/

/***********************************************************************
WARNING: This is an automatically generated file.
***********************************************************************/
#include "WinForm.h"

/***********************************************************************
ACCEL.CPP
***********************************************************************/
namespace CoreLib
{
	namespace WinForm
	{
		Accelerator::Accelerator()
		{
			memset(&accel, 0, sizeof(ACCEL));
		}

		Accelerator::Accelerator(AccelAuxKey auxKey, Word mainKey)
		{
			accel.fVirt = auxKey +  FVIRTKEY;
			accel.key = mainKey;
		}

		AccelTable::AccelTable()
		{
			handle = 0;
		}

		AccelTable::~AccelTable()
		{
			if (handle)
				DestroyAcceleratorTable(handle);
		}

		HACCEL AccelTable::GetHandle()
		{
			return handle;
		}

		
		void AccelTable::UpdateAccel(Accelerator acc, MenuItem * item)
		{
			int fid = -1;
			for (int i=0; i<accels.Count(); i++)
			{
				if (accels[i].fVirt == acc.accel.fVirt && accels[i].key == acc.accel.key)
				{
					fid = i;
					break;
				}
			}
			acc.accel.cmd = (WORD)item->GetIdentifier();
			if (fid != -1)
			{
				accels[fid] = acc.accel;
			}
			else
			{
				accels.Add(acc.accel);
			}
		}

		void AccelTable::RegisterAccel(Accelerator acc, MenuItem * item)
		{
			bool find = false;
			for (int i=0; i<accels.Count(); i++)
			{
				if (accels[i].fVirt == acc.accel.fVirt && accels[i].key == acc.accel.key)
				{
					find = true;
					break;
				}
			}
			if (find)
			{
				throw L"Accelerator confliction.";
			}
			acc.accel.cmd = (WORD)item->GetIdentifier();
			accels.Add(acc.accel);
		}

		void AccelTable::Update()
		{
			if (handle)
				DestroyAcceleratorTable(handle);
			handle = CreateAcceleratorTable(accels.Buffer(), accels.Count());
		}
	}
}

/***********************************************************************
APP.CPP
***********************************************************************/
#define _CRTDBG_MAP_ALLOC 
#include <crtdbg.h>

#include <commctrl.h>

#pragma comment(lib,"comctl32.lib")
#pragma comment(linker,"/manifestdependency:\""		\
	"type='win32' "									\
	"name='Microsoft.Windows.Common-Controls' "		\
	"version='6.0.0.0' "							\
	"processorArchitecture='*' "					\
	"publicKeyToken='6595b64144ccf1df' "			\
	"language='*'\""								\
)

using namespace CoreLib::Basic;
using namespace CoreLib::IO;

namespace CoreLib
{
	namespace WinForm
	{
		const wchar_t * Application::ControlClassName = L"GxWinControl";
		const wchar_t * Application::FormClassName = L"GxWinForm";
		const wchar_t * Application::GLFormClassName = L"GxGLWinForm";
		int Application::globalUID = 0;
		BaseForm * Application::mainForm = 0;
		HINSTANCE Application::instance = 0;
		HINSTANCE Application::prevInstance = 0;
		Dictionary<HWND, Component*> * Application::components;
		Dictionary<UINT_PTR, Object*> * Application::objMap;
		EnumerableHashSet<BaseForm*> Application::forms;
		String * Application::cmdLine;
		HWND Application::mainFormHandle = 0;
		void * Application::gdiToken = 0;
		int Application::cmdShow = 0;
		AccelTable * Application::accelTable = 0;
		LOGFONT Application::SysFont;
		bool Application::terminate = false;
		NotifyEvent * Application::onMainLoop = 0;
		DWORD Application::uiThreadId = 0;

		bool Application::IsUIThread()
		{
			return GetCurrentThreadId() == uiThreadId;
		}

		void Application::RegisterForm(BaseForm * form)
		{
			forms.Add(form);
		}

		void Application::UnregisterForm(BaseForm * form)
		{
			forms.Remove(form);

		}

		int Application::GenerateGUID()
		{
			return globalUID++;
		}

		void Application::Init()
		{
			Application::Init(GetModuleHandle(0), 0, ::GetCommandLineW());
		}

		void Application::Init(HINSTANCE hIns, HINSTANCE hPrevIns, LPTSTR cmdline, int show)
		{
			instance = hIns;
			components = new Dictionary<HWND, Component*>();
			Application::cmdLine = new String();
			Application::objMap = new Dictionary<UINT_PTR, Object*>();
			prevInstance = hPrevIns;
			*Application::cmdLine = cmdline;
			cmdShow = show;
			GdiplusStartupInput input;
			GdiplusStartup(&gdiToken, &input, 0);

			RegisterControlClass();
			RegisterFormClass();
			RegisterGLFormClass();

			INITCOMMONCONTROLSEX picce;
			picce.dwSize = sizeof(INITCOMMONCONTROLSEX);
			picce.dwICC = ICC_ANIMATE_CLASS | ICC_BAR_CLASSES | ICC_COOL_CLASSES | ICC_DATE_CLASSES
				| ICC_HOTKEY_CLASS | ICC_INTERNET_CLASSES | ICC_LINK_CLASS | ICC_LISTVIEW_CLASSES
				| ICC_NATIVEFNTCTL_CLASS | ICC_PAGESCROLLER_CLASS | ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES
				| ICC_TAB_CLASSES | ICC_TREEVIEW_CLASSES | ICC_UPDOWN_CLASS | ICC_USEREX_CLASSES | 
				ICC_WIN95_CLASSES;
			InitCommonControlsEx(&picce);

		}

		void Application::SetMainLoopEventHandler(NotifyEvent * mainLoop)
		{
			onMainLoop = mainLoop;
		}

		void Application::Terminate()
		{
			PostQuitMessage(0);
			terminate = true;
		}

		LOGFONT Application::GetSysFont()
		{
			NONCLIENTMETRICS NonClientMetrics;
			NonClientMetrics.cbSize=sizeof(NONCLIENTMETRICS) - sizeof(NonClientMetrics.iPaddedBorderWidth);
			SystemParametersInfo(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICS),&NonClientMetrics,0);
			SysFont = NonClientMetrics.lfMessageFont;
			return SysFont;
		}

		void Application::RegisterObjectHandle(UINT_PTR handle, Object * obj)
		{
			objMap->AddIfNotExists(handle, obj);
		}

		Object * Application::GetObjectFromHandle(UINT_PTR handle)
		{
			Object * rs = 0;
			objMap->TryGetValue(handle, rs);
			return rs;
		}

		void Application::RegisterControlClass()
		{
			WNDCLASSEX wcex;

			wcex.cbSize = sizeof(WNDCLASSEX);

			wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS ;
			wcex.lpfnWndProc	= WndProc;
			wcex.cbClsExtra		= 0;
			wcex.cbWndExtra		= 0;
			wcex.hInstance		= Application::GetHandle();
			wcex.hIcon			= 0;
			wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
			wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
			wcex.lpszMenuName	= 0;
			wcex.lpszClassName	= ControlClassName;
			wcex.hIconSm		= 0;

			RegisterClassEx(&wcex);
		}

		void Application::RegisterFormClass()
		{
			WNDCLASSEX wcex;

			wcex.cbSize = sizeof(WNDCLASSEX);

			wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS ;
			wcex.lpfnWndProc	= WndProc;
			wcex.cbClsExtra		= 0;
			wcex.cbWndExtra		= 0;
			wcex.hInstance		= Application::GetHandle();
			wcex.hIcon			= 0;
			wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
			wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
			wcex.lpszMenuName	= 0;
			wcex.lpszClassName	= FormClassName;
			wcex.hIconSm		= 0;

			RegisterClassEx(&wcex);
		}

		void Application::RegisterGLFormClass()
		{
			WNDCLASSEX wcex;

			wcex.cbSize = sizeof(WNDCLASSEX);

			wcex.style			= CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
			wcex.lpfnWndProc	= WndProc;
			wcex.cbClsExtra		= 0;
			wcex.cbWndExtra		= 0;
			wcex.hInstance		= Application::GetHandle();
			wcex.hIcon			= 0;
			wcex.hCursor		= NULL;
			wcex.hbrBackground	= NULL;
			wcex.lpszMenuName	= 0;
			wcex.lpszClassName	= GLFormClassName;
			wcex.hIconSm		= 0;

			RegisterClassEx(&wcex);
		}

		HINSTANCE Application::GetHandle()
		{
			return instance;
		}

		void Application::ProcessMessage(MSG & msg)
		{
			HWND hwnd = GetActiveWindow();
	
			bool processed = false;
			for (auto & form : forms)
				if (!form->GetWantChars() && IsDialogMessage(form->GetHandle(), &msg))
				{
					processed = true;
					break;
				}
			if (!accelTable || !TranslateAccelerator(hwnd,
				accelTable->GetHandle(), &msg))
			{
				if (!processed)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}

		void Application::Run(const BaseForm * MainForm, bool NonBlocking)
		{
			Application::uiThreadId = GetCurrentThreadId();
			MSG msg;
			mainForm = const_cast<BaseForm*>(MainForm);
			mainFormHandle = MainForm->GetHandle();
			ShowWindow(mainFormHandle, SW_SHOW);
			UpdateWindow(mainFormHandle);
			while (!terminate)
			{
				int HasMsg = (NonBlocking?PeekMessage(&msg, NULL,0,0,TRUE):GetMessage(&msg, NULL, 0,0));
				if (HasMsg)
				{
					ProcessMessage(msg);
				}
				if (msg.message == WM_QUIT)
					terminate = true;
				if (onMainLoop)
				{
					onMainLoop->Invoke((Object*)MainForm, EventArgs());
				}
			}
		}

		BaseForm * Application::GetMainForm()
		{
			return mainForm;
		}

		void Application::SetAccel(AccelTable * acc)
		{
			accelTable = acc;
		}

		void Application::DoEvents()
		{
			MSG msg;
			while (PeekMessage(&msg, NULL, 0,0,PM_REMOVE))
			{
				ProcessMessage(msg);
			}
		}

		void Application::RegisterComponent(Component * com)
		{
			components->Add(com->GetHandle(), com);
		}

		void Application::UnRegisterComponent(Component * com)
		{
			components->Remove(com->GetHandle());
		}

		int Application::RegisterHandle(Object * obj)
		{
			static int id = 0;
			id++;
			objMap->Add(id, obj);
			return id;
		}

		Object * Application::GetObject(UINT_PTR handle)
		{
			Object * rs = 0;
			objMap->TryGetValue(handle, rs);
			return rs;
		}

		void Application::UnRegisterHandle(UINT_PTR handle)
		{
			objMap->Remove(handle);
		}

		HWND Application::GetMainFormHandle()
		{
			return mainFormHandle;
		}

		Component * Application::GetComponent(const HWND handle)
		{
			Component * rs = 0;
			components->TryGetValue(handle, rs);
			return rs;
		}

		void Application::Dispose()
		{
			{
				for (auto & c : *components)
				{
					if (c.Value)
					{
						BaseForm * ctrl = dynamic_cast<BaseForm*>(c.Value);
						if (ctrl == mainForm)
							delete mainForm;
					}
				}
				delete components;
				delete cmdLine;
				delete objMap;
				delete onMainLoop;
			}
			forms = EnumerableHashSet<BaseForm*>();
			GdiplusShutdown(gdiToken);
			CoreLib::Text::Parser::DisposeTextLexer();
			_CrtDumpMemoryLeaks();
		}

		String Application::GetCommandLine()
		{
			return *cmdLine;
		}

		String Application::GetExePath()
		{
			wchar_t fileName[500];
			::GetModuleFileName(NULL, fileName, 500);
			return Path::GetDirectoryName(fileName);
		}

		CommandLineParser::CommandLineParser(const String & cmdLine)
		{
			String profile = 
				L"String = {\"[^\"]*\"}\n"\
				L"Value = {\\d+(.\\d*)?}\n"\
				L"Token = {[^ \\t\\n\"\']+}\n"\
				L"#WhiteSpace = {\\s+}\n";
			MetaLexer lexer(profile);
			lexer.Parse(cmdLine, stream);
		}
	
		String CommandLineParser::GetFileName()
		{
			if (stream.Count() && stream.FirstNode()->Value.TypeID != 1)
				return stream.FirstNode()->Value.Str;
			else
				return L"";
		}

		bool CommandLineParser::OptionExists(const String & opt)
		{
			for (auto * node = stream.FirstNode(); node; node=node->GetNext())
			{
				if (node->Value.Str.Equals(opt, false))
				{
					return true;
				}
			}
			return false;
		}

		String CommandLineParser::GetOptionValue(const String & opt)
		{
			for (auto * node = stream.FirstNode(); node; node=node->GetNext())
			{
				if (node->Value.Str.Equals(opt, false))
				{
					node = node->GetNext();
					if (node)
						return node->Value.Str;
					else
						return L"";
				}
			}
			return L"";
		}
	
		String CommandLineParser::GetToken(int id)
		{
			auto node = stream.FirstNode();
			for (int i = 0; i<id; i++)
			{
				node = node->GetNext();
			}
			return node->Value.Str;
		}
	
		int CommandLineParser::GetTokenCount()
		{
			return stream.Count();
		}

		int SystemMetrics::GetScreenWidth()
		{
			return GetSystemMetrics(SM_CXSCREEN);
		}

		int SystemMetrics::GetScreenHeight()
		{
			return GetSystemMetrics(SM_CYSCREEN);
		}
	}
}

/***********************************************************************
BUTTONS.CPP
***********************************************************************/

namespace CoreLib
{
	namespace WinForm
	{
		void ClearDefaultButtonAction::Apply(Component * comp)
		{
			Button * btn = dynamic_cast<Button*>(comp);
			if (btn)
				btn->SetDefault(false);
		}

		CustomButton::CustomButton(Component * parent)
			: Control()
		{
			Init(parent);
		}
		CustomButton::CustomButton()
		{
		}

		Button::Button(Component * comp)
			: CustomButton(comp)
		{
		}

		void Button::SetDefault(bool def)
		{
			bool ldef = GetStyle(GWL_STYLE, BS_DEFPUSHBUTTON);
			if (ldef != def)
			{
				if (GetStyle(GWL_STYLE, BS_DEFPUSHBUTTON) || GetStyle(GWL_STYLE, BS_PUSHBUTTON))
				{
					SetStyle(GWL_STYLE, BS_DEFPUSHBUTTON, def);
					SetStyle(GWL_STYLE, BS_PUSHBUTTON, !def);
				}
				UpdateWindow(handle);
			}
		
		}

		int Button::ProcessMessage(WinMessage & msg)
		{
			int rs = -1;
			switch (msg.message)
			{
			case gxMsgKeyBroadcast:
				{
					if (msg.wParam == VK_RETURN && GetStyle(GWL_STYLE, BS_DEFPUSHBUTTON))
					{
						_Click();
					}
					else if (msg.wParam == VK_ESCAPE)
					{
						BaseForm * f = GetOwnerForm();
						if (f && f->GetCancelButton() == this)
						{
							_Click();
						}
					}
				}
			}
			if (rs == -1)
				return CustomButton::ProcessMessage(msg);
			return rs;
		}

		int Button::ProcessNotification(WinNotification note)
		{
			int rs = -1;
			switch (note.code) 
			{
			case BN_SETFOCUS:
				{
					if (GetOwnerForm())
					{
						ClearDefaultButtonAction act;
						GetOwnerForm()->ApplyActionToComponentAndChildren(&act);
					}
					SetDefault(true);
					UpdateWindow(handle);
				}
				break;
			case BN_KILLFOCUS:
				{
					BaseForm * form = GetOwnerForm();
					if (form)
					{
						form->SetDefaultButton(form->GetDefaultButton());
					}
					UpdateWindow(handle);
				}
				break;
			default:
				break;
			}
			if (rs == -1)
				return CustomButton::ProcessNotification(note);
			else
				return rs;
		}

		void CustomButton::CreateWnd(Component * parent)
		{
			handle = CreateWindowEx
			(
				0,
				L"BUTTON",
				L"Button",
				WS_GROUP|WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_TEXT | BS_NOTIFY ,
				10,
				10,
				100,
				100,
				parent->GetHandle(),
				NULL,
				Application::GetHandle(), 
				NULL
			);
			captureClick = false;
			if (!handle)
			{
				throw "Failed to create window.";
			}
		}

		void CustomButton::SetText(const String & text)
		{
			SetWindowTextW(handle, text.Buffer());
		}

		int CustomButton::ProcessNotification(WinNotification note)
		{
			switch (note.code)
			{
			case BN_CLICKED:
				{
					_Click();
				}
				return -1;
			default:
				return Control::ProcessNotification(note);
			}
		}

		CheckBox::CheckBox(Component * parent)
			: CustomButton()
		{
			Init(parent);
		}

		void CheckBox::CreateWnd(Component * parent)
		{
			handle = CreateWindowEx
			(
				WS_EX_TRANSPARENT,
				L"BUTTON",
				L"CheckBox",
				WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_CHECKBOX | BS_TEXT | BS_NOTIFY ,
				10,
				10,
				100,
				100,
				parent->GetHandle(),
				NULL,
				Application::GetHandle(), 
				NULL
			); 
			captureClick = false;
			if (!handle)
			{
				throw "Failed to create window.";
			}
		}

		CheckBox::CheckBox()
		{
		}

		void CheckBox::SetChecked(bool chk)
		{
			SendMessage(handle, BM_SETCHECK, chk?BST_CHECKED:BST_UNCHECKED, 0);
		}

		bool CheckBox::GetChecked()
		{
			LRESULT rs = SendMessage(handle, BM_GETCHECK, 0,0);
			if (rs == BST_CHECKED)
				return true;
			else
				return false;
		}

		int CheckBox::ProcessNotification(WinNotification note)
		{
			switch (note.code)
			{
			case BN_CLICKED:
				{
					SetChecked(!GetChecked());
				}
				break;
			}
			return CustomButton::ProcessNotification(note);
		}

		RadioBox::RadioBox(Component * parent)
			: CheckBox()
		{
			Init(parent);
		}

		void RadioBox::CreateWnd(Component * parent)
		{
			handle = CreateWindowEx
			(
				WS_EX_TRANSPARENT,
				L"BUTTON",
				L"RadioBox",
				WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_RADIOBUTTON | BS_TEXT | BS_NOTIFY ,
				10,
				10,
				100,
				100,
				parent->GetHandle(),
				NULL,
				Application::GetHandle(), 
				NULL
			); 
			captureClick = false;
			if (!handle)
			{
				throw "Failed to create window.";
			}
		}

		int RadioBox::ProcessNotification(WinNotification note)
		{
			switch (note.code)
			{
			case BN_CLICKED:
				{
					Component * comp = Application::GetComponent(::GetParent(handle));
					if (comp)
					{
						for (int i=0; i<comp->GetChildrenCount(); i++)
						{
							RadioBox * rd = dynamic_cast<RadioBox*>(comp->GetChildren(i));
							if (rd)
							{
								rd->SetChecked(false);
							}
						}
					}
					SetChecked(true);
				}
				break;
			}
			return CustomButton::ProcessNotification(note);
		}

		GroupBox::GroupBox(Component * parent)
			: CustomButton()
		{
			Init(parent);
		}

		void GroupBox::CreateWnd(Component * parent)
		{
			handle = CreateWindowEx
			(
				0,
				L"BUTTON",
				L"GroupBox",
				WS_GROUP | WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_TEXT | BS_NOTIFY ,
				10,
				10,
				100,
				100,
				parent->GetHandle(),
				NULL,
				Application::GetHandle(), 
				NULL
			); 
			captureClick = false;
			if (!handle)
			{
				throw "Failed to create window.";
			}
		}
	}
}

/***********************************************************************
COMMONDLG.CPP
***********************************************************************/

using namespace CoreLib::IO;

namespace CoreLib
{
	namespace WinForm
	{
		UINT_PTR CALLBACK OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM /*wParam*/, LPARAM /*lParam*/)
		{
			if (uiMsg == WM_INITDIALOG)
			{
				RECT r, rs;
				HWND handle = GetParent(hdlg);
				HWND hp = GetParent(handle);
				GetWindowRect(hp , &r);
				GetWindowRect(handle, &rs);
				int w = rs.right - rs.left;
				int h = rs.bottom - rs.top;
				MoveWindow(handle, (r.left + r.right-w)/2, (r.top+r.bottom-h)/2, w,h, TRUE);
				return 1;
			}
			return 0;
		}

		FileDialog::FileDialog(const Component * _owner)
		{
			owner = const_cast<Component*>(_owner);
			memset(&fn, 0, sizeof(OPENFILENAME));
			memset(fileBuf,0,sizeof(fileBuf));
			memset(filterBuf,0,sizeof(filterBuf));
			fn.lStructSize = sizeof(OPENFILENAME);
			fn.hInstance = 0;
			fn.Flags = OFN_EXPLORER|OFN_ENABLEHOOK;
			fn.hwndOwner = owner->GetHandle();
			fn.lpstrCustomFilter = 0;
			fn.lpfnHook = OFNHookProc;
			fn.nMaxCustFilter = 0;
			fn.lpstrFile = fileBuf;
			fn.nMaxFile = FileBufferSize;
			MultiSelect = false;
			CreatePrompt = true;
			FileMustExist = false;
			HideReadOnly = false;
			OverwritePrompt = true;
			PathMustExist = false;
		}

		FileDialog::~FileDialog()
		{

		}

		void FileDialog::PrepareDialog()
		{	
			memset(fileBuf,0,sizeof(fileBuf));
			fn.nFileOffset = 0;
			if (Filter.Length())
			{
				memset(filterBuf,0,sizeof(wchar_t)*FilterBufferSize);
				if (Filter.Length()<FilterBufferSize)
				{
					for (int i=0; i<Filter.Length(); i++)
					{
						if (Filter[i]==L'|')
							filterBuf[i] = L'\0';
						else
							filterBuf[i] = Filter[i];
					}
				}
				else
					throw "Filter too long.";
				fn.lpstrFilter = filterBuf;
			}
			if (DefaultEXT.Length())
			{
				fn.lpstrDefExt = DefaultEXT.Buffer();
			}
			else
				fn.lpstrDefExt = 0;
			initDir = Path::GetDirectoryName(FileName);
			fn.lpstrInitialDir = initDir.Buffer();
			
			fn.Flags = OFN_EXPLORER|OFN_ENABLEHOOK;
			if (MultiSelect)
				fn.Flags = (fn.Flags | OFN_ALLOWMULTISELECT);
			if (CreatePrompt)
				fn.Flags = (fn.Flags | OFN_CREATEPROMPT);
			if (FileMustExist)
				fn.Flags = (fn.Flags | OFN_FILEMUSTEXIST);
			if (HideReadOnly)
				fn.Flags = (fn.Flags | OFN_HIDEREADONLY);
			if (OverwritePrompt)
				fn.Flags = (fn.Flags | OFN_OVERWRITEPROMPT);
			if (PathMustExist)
				fn.Flags = (fn.Flags | OFN_PATHMUSTEXIST);
		}

		bool FileDialog::ShowOpen()
		{
			PrepareDialog();
			bool succ = (GetOpenFileName(&fn)!=0);
			PostDialogShow();
			return succ;
		}

		bool FileDialog::ShowSave()
		{
			PrepareDialog();
			bool succ = GetSaveFileName(&fn)!=0;
			PostDialogShow();
			return succ;
		}

		void FileDialog::PostDialogShow()
		{
			StringBuilder cFileName;
			bool zero = false;
			FileNames.Clear();
			for (int i=0; i<FileBufferSize; i++)
			{
				if (fn.lpstrFile[i] != L'\0')
				{
					cFileName<<fn.lpstrFile[i];
					zero = false;
				}
				else
				{
					if (!zero)
					{
						FileNames.Add(cFileName.ToString());
						cFileName.Clear();
						zero = true;
					}
					else
						break;
				}
			}
			if (FileNames.Count())
				FileName = FileNames[0];
		}

		UINT_PTR CALLBACK ColorHookProc(HWND hdlg, UINT uiMsg, WPARAM /*wParam*/, LPARAM /*lParam*/)
		{
			if (uiMsg == WM_INITDIALOG)
			{
				RECT r, rs;
				HWND handle = hdlg;
				HWND hp = GetParent(handle);
				GetWindowRect(hp , &r);
				GetWindowRect(hdlg, &rs);
				int w = rs.right - rs.left;
				int h = rs.bottom - rs.top;
				MoveWindow(handle, (r.left + r.right-w)/2, (r.top+r.bottom-h)/2, w,h, TRUE);
				return 1;
			}
			return 0;
		}

		ColorDialog::ColorDialog(Component * _owner)
		{
			owner = _owner;
			FullOpen = true;
			PreventFullOpen = false;
			memset(&cs, 0, sizeof(CHOOSECOLOR));
			cs.hInstance = 0;
			cs.lpfnHook = ColorHookProc;
			cs.hwndOwner = owner->GetHandle();
			cs.lpCustColors = cr;
			cs.lStructSize = sizeof(CHOOSECOLOR);
		}

		bool ColorDialog::ShowColor()
		{
			cs.Flags = CC_ENABLEHOOK|CC_RGBINIT|CC_ANYCOLOR;
			if (FullOpen)
				cs.Flags = (cs.Flags | CC_FULLOPEN);
			if (PreventFullOpen)
				cs.Flags = (cs.Flags | CC_PREVENTFULLOPEN);
			bool succ = (ChooseColor(&cs)!=0);
			Color = cs.rgbResult;
			return succ;
		}
	}
}

/***********************************************************************
CTRLS.CPP
***********************************************************************/
#include <windowsx.h>
namespace CoreLib
{
	namespace WinForm
	{
		LRESULT CALLBACK SubClassWndProc(HWND hWnd,UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR /*uIdSubclass*/,
			DWORD_PTR /*dwRefData*/)
		{
			int rs = -1;
			Component * comp = 0;
			if (message == WM_COMMAND)
			{
				if (lParam != 0)
				{
					comp = Application::GetComponent((HWND)lParam);
					WinNotification note;
					note.code = HIWORD(wParam);
					note.hWnd = (HWND)lParam;
					if (comp)
						rs = comp->ProcessNotification(note)?0:1;
				}
			}
			else if (message == WM_NOTIFY)
			{
				WinNotification note;
				note.code = ((LPNMHDR)lParam)->code;
				note.hWnd = ((LPNMHDR)lParam)->hwndFrom;
				comp = Application::GetComponent(note.hWnd);
				if (comp)
					rs = comp->ProcessNotification(note)?0:1;
			}
			WinMessage msg(hWnd, message, wParam, lParam);
			comp = Application::GetComponent(hWnd);
			if (comp)
				rs = comp->ProcessMessage(msg);
			// Call Original WndProc
			if (rs != -1)
				return rs;
			//if (msg.message == WM_CHAR && msg.wParam == 0)
			//	return true;
			return DefSubclassProc(hWnd, msg.message, msg.wParam, msg.lParam);
		}

		Component::Component()
		{
			handle = 0;
		}

		BaseForm* Component::GetOwnerForm()
		{
			HWND chandle = handle;
			while (chandle)
			{
				Component * comp = Application::GetComponent(chandle);
				if (dynamic_cast<BaseForm*>(comp))
					return (BaseForm*)comp;
				else
					chandle = ::GetParent(chandle);
			}
			return 0;
		}

		void Component::BroadcastMessage(WinMessage & msg)
		{
			ProcessMessage(msg);
			for (int i=0; i<children.Count(); i++)
				children[i]->BroadcastMessage(msg);
		}

		void Component::AddChild(Component * comp)
		{
			children.Add(comp);
			comp->SetParent(this);
		}

		void Component::RemoveChild(Component * comp)
		{
			int id = 0;
			for (auto & child : children)
			{
				if (child.Ptr() == comp)
				{
					child = nullptr;
					break;
				}
				id++;
			}
			if (id < children.Count())
				children.RemoveAt(id);
		}

		void Component::SubClass()
		{
			SetWindowSubclass(handle, SubClassWndProc, 0, (DWORD_PTR)this);
		}

		void Component::UnSubClass()
		{
			RemoveWindowSubclass(handle, SubClassWndProc, 0);
		}

		void Component::Destroy()
		{

		}

		int Component::ProcessNotification(WinNotification /*note*/)
		{
			return -1;
		}

		int Component::ProcessMessage(WinMessage & msg)
		{
			UserMessageArgs args;
			if (msg.message >= WM_USER)
			{
				args.Message = msg.message;
				args.LParam = msg.lParam;
				args.WParam = msg.wParam;
				OnUserEvent.Invoke(this, args);
				return 0;
			}
			return -1;
		}

		HWND Component::GetHandle() const
		{
			return handle;
		}

		void Component::ApplyActionToComponentAndChildren(Action * act)
		{
			act->Apply(this);
			for (int i=0; i<children.Count(); i++)
			{
				children[i]->ApplyActionToComponentAndChildren(act);
			}
		}

		void Component::SetParent(const Component * Parent)
		{
			if (::GetParent(handle) != Parent->GetHandle())
				::SetParent(handle, Parent->GetHandle());
		}

		Component * Component::GetParent()
		{
			HWND p = ::GetParent(handle);
			if (p)
			{
				return Application::GetComponent(p);
			}
			else
				return 0;
		}

		int Component::GetChildrenCount()
		{
			return children.Count();
		}

		Component * Component::GetChildren(int id)
		{
			return children[id].operator->();
		}

		int BaseControl::ProcessMessage(WinMessage & msg)
		{
			int rs = -1; 
			switch (msg.message)
			{
			case WM_LBUTTONUP:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					OnMouseUp.Invoke(this, MouseEventArgs(mbLeft, mx, my,0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
					RECT rect;
					GetWindowRect(handle, &rect);
					if (captureClick && mouseDown && mx >= 0 && mx <= rect.right-rect.left && my >= 0 && my <= rect.bottom - rect.top)
					{
						OnClick.Invoke(this, EventArgs());
					}
					mouseDown = false;
				}
				break;
			case WM_MBUTTONUP:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					OnMouseUp.Invoke(this, MouseEventArgs(mbMiddle, mx, my,0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
					RECT rect;
					GetWindowRect(handle, &rect);
					if (captureClick && mouseDown && mx >= 0 && mx <= rect.right-rect.left && my >= 0 && my <= rect.bottom - rect.top)
					{
						OnClick.Invoke(this, EventArgs());
					}
					mouseDown = false;
				}
				break;
			case WM_RBUTTONUP:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					OnMouseUp.Invoke(this, MouseEventArgs(mbRight, mx, my,0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
					RECT rect;
					GetWindowRect(handle, &rect);
					if (captureClick && mouseDown && mx >= 0 && mx <= rect.right-rect.left && my >= 0 && my <= rect.bottom - rect.top)
					{
						OnClick.Invoke(this, EventArgs());
					}
					mouseDown = false;
				}
				break;
			case WM_LBUTTONDOWN:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					mouseDown = true;
					OnMouseDown.Invoke(this, MouseEventArgs(mbLeft, mx, my, 0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
				}
				break;
			case WM_MBUTTONDOWN:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					mouseDown = true;
					OnMouseDown.Invoke(this, MouseEventArgs(mbMiddle, mx, my, 0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
				}
				break;
			case WM_RBUTTONDOWN:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					mouseDown = true;
					OnMouseDown.Invoke(this, MouseEventArgs(mbRight, mx, my, 0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
				}
				break;
			case WM_MOUSEMOVE:
				{
					int mx = GET_X_LPARAM(msg.lParam);
					int my = GET_Y_LPARAM(msg.lParam);
					bool processed = false;

					OnMouseMove.Invoke(this, MouseEventArgs(mbNone, mx, my, 0, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
				}
				break;
			case WM_MOUSEWHEEL:
				{
					int delta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
					bool processed = false;
					OnMouseWheel.Invoke(this, MouseEventArgs(mbNone, 0,0, delta, GET_KEYSTATE_WPARAM(msg.wParam), &processed));
					if (processed)
						rs = 0;
				}
				break;
			case WM_CHAR:
				{ 
					OnKeyPressed.Invoke(this, KeyEventArgs((wchar_t)(msg.wParam), (int*)&msg.wParam, false, false, false));
					if (msg.wParam == 0)
						rs = 0;
				}
				break;
			case WM_KEYDOWN:
				{
					OnKeyDown.Invoke(this, KeyEventArgs(0, (int*)&msg.wParam, false, false, false));
					BaseControl * ctrl = dynamic_cast<BaseControl *>(Application::GetComponent(msg.hWnd));
					if (ctrl)
					{
						WinMessage nmsg(msg.hWnd, gxMsgKeyBroadcast, msg.wParam, msg.lParam);
						BaseForm * f = ctrl->GetOwnerForm();
						if (f)
							f->BroadcastMessage(nmsg);
						else
							ctrl->BroadcastMessage(nmsg);
					}
				}
				break;
			case WM_KEYUP:
				{
					OnKeyUp.Invoke(this, KeyEventArgs(0, (int*)&msg.wParam, false, false, false));
				}
				break;
			case WM_SETFOCUS:
				{
					OnFocus.Invoke(this, EventArgs());
				}
				break;
			case WM_KILLFOCUS:
				{
					OnLostFocus.Invoke(this, EventArgs());
				}
				break;
			case WM_MOUSEHOVER:
				{
					OnMouseEnter.Invoke(this, EventArgs());
				}
				break;
			case WM_MOUSELEAVE:
				{
					OnMouseLeave.Invoke(this, EventArgs());
				}
				break;
			case WM_SIZING:
				{
					ResizingEventArgs args;
					RECT * rect = (RECT *)msg.lParam;
					args.Width = rect->right - rect->left;
					args.Height = rect->bottom - rect->top;
					args.Left = rect->left;
					args.Top = rect->top;
					OnResizing.Invoke(this, args);
					rect->left = args.Left;
					rect->top = args.Top;
					rect->bottom = args.Top + args.Height;
					rect->right = args.Left + args.Width;
				}
				break;
			case WM_SIZE:
				{
					_OnResize();
					OnResized.Invoke(this, EventArgs());
				}
				break;
			case WM_PAINT:
				{
					if (ownerDraw)
					{
						PaintEventArgs args;
						RECT rect;
						int NeedDraw = GetUpdateRect(handle, &rect, true);
						if (NeedDraw)
						{
							PAINTSTRUCT s;
							args.Graphics = new Graphics(BeginPaint(handle, &s));
							args.UpdateRect.Left = rect.left;
							args.UpdateRect.Right = rect.right;
							args.UpdateRect.Bottom = rect.bottom;
							args.UpdateRect.Top = rect.top;
							OnPaint.Invoke(this, args);
							args.Graphics = 0;
							EndPaint(handle, &s);
							ReleaseDC(handle,s.hdc);
						}
						rs = 0;
					}
				}
				break;
			default:
				break;
			}
			if (rs != -1)
				return rs;
			return Component::ProcessMessage(msg);
		}

		int BaseControl::ProcessNotification(WinNotification note)
		{
			switch (note.code)
			{
			case NM_SETFOCUS:
				_Focused();
				break;
			case NM_KILLFOCUS:
				_LostFocus();
				break;
			}
			return -1;
		}

		void BaseControl::_Focused()
		{
			OnFocus.Invoke(this, EventArgs());
		}

		void BaseControl::_LostFocus()
		{
			OnLostFocus.Invoke(this, EventArgs());
		}

		void BaseControl::_OnResize()
		{
		}

		void BaseControl::Destroy()
		{
			if (nativeFont)
				DeleteObject(nativeFont);
			DestroyWindow(handle);
		}

		void BaseControl::SetWidth(int w)
		{
			RECT r;
			GetWindowRect(handle, &r);
			POINT point, point1;
			point.x = r.left;
			point.y = r.top;
			point1.x = r.right;
			point1.y = r.bottom;
			ScreenToClient(::GetParent(handle), &point);
			ScreenToClient(::GetParent(handle), &point1);

			MoveWindow(handle, point.x, point.y, w, point1.y-point.y, true);
		}

		void BaseControl::SetHeight(int h)
		{
			RECT r;
			GetWindowRect(handle, &r);
			POINT point, point1;
			point.x = r.left;
			point.y = r.top;
			point1.x = r.right;
			point1.y = r.bottom;
			ScreenToClient(::GetParent(handle), &point);
			ScreenToClient(::GetParent(handle), &point1);

			MoveWindow(handle, point.x, point.y, point1.x-point.x, h, true);
		}

		void BaseControl::SetLeft(int l)
		{
			RECT r;
			GetWindowRect(handle, &r);
			POINT point, point1;
			point.x = r.left;
			point.y = r.top;
			point1.x = r.right;
			point1.y = r.bottom;
			ScreenToClient(::GetParent(handle), &point);
			ScreenToClient(::GetParent(handle), &point1);

			MoveWindow(handle, l, point.y, point1.x - point.x, point1.y - point.y, true);
		}

		void BaseControl::SetTop(int t)
		{
			RECT r;
			GetWindowRect(handle, &r);
			POINT point, point1;
			point.x = r.left;
			point.y = r.top;
			point1.x = r.right;
			point1.y = r.bottom;
			ScreenToClient(::GetParent(handle), &point);
			ScreenToClient(::GetParent(handle), &point1);

			MoveWindow(handle, point.x, t, point1.x - point.x, point1.y - point.y, true);
		}

		void BaseControl::EnableRedraw()
		{
			SendMessage(handle, WM_SETREDRAW, TRUE, 0);
		}
		void BaseControl::DisableRedraw()
		{
			SendMessage(handle, WM_SETREDRAW, FALSE, 0);
		}
		void BaseControl::Refresh()
		{
			RECT rect;
			GetClientRect(handle, &rect);
			RedrawWindow(handle, &rect, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
		}
		void BaseControl::Invalidate()
		{
			RECT rect;
			GetClientRect(handle, &rect);
			InvalidateRect(handle, &rect, TRUE);
		}
		void BaseControl::BringToTop()
		{
			BringWindowToTop(handle);
		}
		int BaseControl::GetClientWidth()
		{
			RECT r;
			GetClientRect(handle, &r);
			return r.right - r.left;
		}

		int BaseControl::GetClientHeight()
		{
			RECT r;
			GetClientRect(handle, &r);
			return r.bottom - r.top;
		}

		int BaseControl::GetWidth()
		{
			RECT r;
			GetWindowRect(handle, &r);
			return r.right-r.left;
		}

		int BaseControl::GetHeight()
		{
			RECT r;
			GetWindowRect(handle, &r);
			return r.bottom-r.top;
		}

		int BaseControl::GetLeft()
		{
			RECT r;
			GetWindowRect(handle, &r);
			return r.left;
		}

		int BaseControl::GetTop()
		{
			RECT r;
			GetWindowRect(handle, &r);
			return r.top;
		}

		void BaseControl::SetStyle(int StyleType, unsigned int Style, bool val)
		{
			auto style = GetWindowLongPtr(handle, StyleType);
			if (val)
				style = style | Style;
			else
				style = style & (~(LONG_PTR)Style);
			SetWindowLongPtr(handle, StyleType, style);
		}

		bool BaseControl::GetStyle(int StyleType, unsigned int Style)
		{
			auto style = GetWindowLongPtr(handle, StyleType);
			return ((style & Style) == Style);
		}

		bool BaseControl::GetEnabled()
		{
			return IsWindowEnabled(handle)==TRUE; 
		}
		void BaseControl::SetEnabled(bool val)
		{
			EnableWindow(handle, val);
		}
		bool BaseControl::GetVisible()
		{
			return IsWindowVisible(handle)==TRUE;
		}
		void BaseControl::SetVisible(bool val)
		{
			ShowWindow(handle, val?SW_SHOW:SW_HIDE);
		}
		bool BaseControl::Focused()
		{
			return (GetFocus() == handle);
		}
		void BaseControl::SetFocus()
		{
			::SetFocus(handle);
		}
		void BaseControl::SetFont(Font * f)
		{
			font = f;
			LOGFONT fnt;
			Graphics g(handle);
			f->GetLogFontW(&g, &fnt);
			if (nativeFont)
				DeleteObject(nativeFont);
			nativeFont = CreateFontIndirect(&fnt);
			SendMessage(handle,WM_SETFONT,(WPARAM)nativeFont,TRUE);
		}

		void BaseControl::SetPosition(int x, int y, int w, int h)
		{
			MoveWindow(handle, x,y,w,h, TRUE);
		}

		BaseControl::BaseControl()
			: ownerDraw(false), captureClick(true)
		{
			nativeFont = 0;
		}


		void BaseControl::SetTabStop(bool ts)
		{
			SetStyle(GWL_STYLE, WS_TABSTOP, ts);
		}

		bool BaseControl::GetTabStop()
		{
			return GetStyle(GWL_STYLE, WS_TABSTOP);
		}

		void BaseControl::InitFont()
		{
			auto sysFont = Application::GetSysFont();
			Font * f = new Font(Graphics(handle).GetHDC(), &sysFont);
			SetFont(f);
		}

		void BaseControl::_Click()
		{
			OnClick.Invoke(this, EventArgs());
		}

		void Control::CreateWnd(Component * parent)
		{
			handle = CreateWindowEx(WS_EX_CONTROLPARENT, Application::ControlClassName, 0, WS_CHILD | WS_GROUP | WS_TABSTOP,
				0, 0, 50, 50, parent->GetHandle(), NULL, Application::GetHandle(), NULL);
			ownerDraw = true;
			if (!handle)
			{
				throw "Failed to create window.";
			}
		}

		void Control::Init(Component * parent)
		{
			CreateWnd(parent);
			InitFont();
			Application::RegisterComponent(this);
			SubClass();
			ShowWindow(handle, SW_SHOW); 
			UpdateWindow(handle); 
		}

		int Control::ProcessNotification(WinNotification note)
		{
			return BaseControl::ProcessNotification(note);
		}

		void ScrollPanel::VScroll(int delta)
		{
			RECT crect;
			GetClientRect(handle, &crect);
			if (lastScrollY - delta < 0)
				delta = lastScrollY;
			if (lastScrollY - delta > maxYScroll)
				delta = lastScrollY - maxYScroll;
			if (delta != 0)
			{
				ScrollWindow(handle, 0, delta, NULL, &crect);
				lastScrollY = lastScrollY - delta;
				SCROLLINFO sinfo;
				sinfo.cbSize = sizeof(SCROLLINFO);
				sinfo.fMask = SIF_POS;
				sinfo.nPos = sinfo.nTrackPos = lastScrollY;
				SetScrollInfo(handle, SB_VERT, &sinfo, true);
			}
		}

		void ScrollPanel::HScroll(int delta)
		{
			RECT crect;
			GetClientRect(handle, &crect);
			if (lastScrollX - delta < 0)
				delta = lastScrollX;
			if (lastScrollX - delta > maxXScroll)
				delta = lastScrollX - maxXScroll;
			if (delta != 0)
			{
				ScrollWindow(handle, delta, 0, NULL, &crect);
				lastScrollX = lastScrollX - delta;
				SCROLLINFO sinfo;
				sinfo.cbSize = sizeof(SCROLLINFO);
				sinfo.fMask = SIF_POS;
				sinfo.nPos = sinfo.nTrackPos = lastScrollX;
				SetScrollInfo(handle, SB_HORZ, &sinfo, true);
			}
		}

		int ScrollPanel::ProcessMessage(WinMessage & msg)
		{
			int rs = -1;
			switch (msg.message)
			{
			case WM_VSCROLL:
			{
				RECT crect;
				GetClientRect(handle, &crect);
				int delta = 0;
				if (LOWORD(msg.wParam) == SB_THUMBTRACK || LOWORD(msg.wParam) == SB_THUMBPOSITION)
				{
					delta = lastScrollY-HIWORD(msg.wParam);
				}
				else if (LOWORD(msg.wParam) == SB_LINEDOWN)
				{
					delta = -30;
				}
				else if (LOWORD(msg.wParam) == SB_LINEUP)
				{
					delta = 30;
				}
				else if (LOWORD(msg.wParam) == SB_PAGEDOWN)
					delta = -(crect.bottom - crect.top);
				else if (LOWORD(msg.wParam) == SB_PAGEUP)
					delta = (crect.bottom - crect.top);
				VScroll(delta);
				rs = 0;
				break;
			}
			case WM_HSCROLL:
			{
				RECT crect;
				GetClientRect(handle, &crect);
				int delta = 0;
				if (LOWORD(msg.wParam) == SB_THUMBTRACK || LOWORD(msg.wParam) == SB_THUMBPOSITION)
				{
					delta = lastScrollX - HIWORD(msg.wParam);
				}
				else if (LOWORD(msg.wParam) == SB_LINELEFT)
				{
					delta = -30;
				}
				else if (LOWORD(msg.wParam) == SB_LINERIGHT)
				{
					delta = 30;
				}
				else if (LOWORD(msg.wParam) == SB_PAGELEFT)
					delta = -(crect.right - crect.left);
				else if (LOWORD(msg.wParam) == SB_PAGERIGHT)
					delta = (crect.right - crect.left);
				HScroll(delta);
				break;
			}
			case WM_MOUSEWHEEL:
			{
				auto zDelta = GET_WHEEL_DELTA_WPARAM(msg.wParam);
				UINT lines;
				SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
				if (zDelta < 0)
					SetVerticalScrollPosition(GetVerticalScrollPosition() + lines * 25);
				else
					SetVerticalScrollPosition(GetVerticalScrollPosition() - lines * 25);
				break;
			}
			}
			if (rs == -1)
				return BaseControl::ProcessMessage(msg);
			else
				return rs;
		}

		void ScrollPanel::SetVerticalScrollPosition(int val)
		{
			SCROLLINFO sinfo;
			sinfo.cbSize = sizeof(SCROLLINFO);
			sinfo.fMask = SIF_POS | SIF_RANGE;
			GetScrollInfo(handle, SB_VERT, &sinfo);
			sinfo.nPos = Math::Clamp(val, sinfo.nMin, sinfo.nMax);
			VScroll(lastScrollY - sinfo.nPos);
		}

		int ScrollPanel::GetVerticalScrollPosition()
		{
			SCROLLINFO sinfo;
			sinfo.cbSize = sizeof(SCROLLINFO);
			sinfo.fMask = SIF_POS | SIF_RANGE;
			GetScrollInfo(handle, SB_VERT, &sinfo);
			return sinfo.nPos;
		}

		void ScrollPanel::_OnResize()
		{
			int bboxMaxY = -1<<30;
			int bboxMaxX = -1 << 30;
			int bboxMinY = hiddenCtrl ? hiddenCtrl->GetTop() : 0;
			int bboxMinX = hiddenCtrl ? hiddenCtrl->GetLeft() : 0;
			for (auto & child : children)
			{
				if (auto ctrl = dynamic_cast<Control*>(child.Ptr()))
				{
					bboxMaxY = Math::Max(bboxMaxY, ctrl->GetTop() + ctrl->GetHeight());
					bboxMaxX = Math::Max(bboxMaxX, ctrl->GetLeft() + ctrl->GetWidth());
				}
			}

			RECT crect;
			GetClientRect(handle, &crect);
			if (scrollBars == ScrollBars::Both || scrollBars == ScrollBars::Vertical)
			{
				SCROLLINFO sinfo;
				sinfo.cbSize = sizeof(SCROLLINFO);
				sinfo.fMask = SIF_PAGE | SIF_RANGE;
				sinfo.nMin = 0;
				int clientHeight = crect.bottom - crect.top;
				maxYScroll = Math::Max(0, bboxMaxY - bboxMinY);
				sinfo.nMax = maxYScroll;
				sinfo.nPage = clientHeight;
				SetScrollInfo(handle, SB_VERT, &sinfo, true);
			}

			if (scrollBars == ScrollBars::Both || scrollBars == ScrollBars::Horizontal)
			{
				SCROLLINFO sinfo;
				sinfo.cbSize = sizeof(SCROLLINFO);
				sinfo.fMask = SIF_PAGE | SIF_RANGE;
				sinfo.nMin = 0;
				int clientWidth = crect.right - crect.left;
				maxXScroll = Math::Max(0, bboxMaxX - bboxMinX);
				sinfo.nMax = maxXScroll;
				sinfo.nPage = clientWidth;
				SetScrollInfo(handle, SB_HORZ, &sinfo, true);
			}
		}

		ScrollPanel::ScrollPanel()
		{
		}

		ScrollPanel::ScrollPanel(Component * parent)
		{
			Init(parent);
			hiddenCtrl = new Control(this);
			hiddenCtrl->SetPosition(0, 0, 0, 0);
		}

		void Control::Destroy()
		{
			UnSubClass();
			Application::UnRegisterComponent(this);
			BaseControl::Destroy();
		}

		void ScrollPanel::SetScrollBar(ScrollBars scroll)
		{
			scrollBars = scroll;
			if (scroll == ScrollBars::None)
				ShowScrollBar(handle, (int)ScrollBars::Both, false);
			else
				ShowScrollBar(handle, (int)scroll, true);
		}

		void ScrollPanel::UpdateScrollBar()
		{
			_OnResize();
		}

		Control::Control(Component * parent)
		{
			Init(parent);
		}

		Control::Control()
		{
		}

		Control::~Control()
		{
			Destroy();
		}
	}
}

/***********************************************************************
FORM.CPP
***********************************************************************/

namespace CoreLib
{
	const int MSG_INVOKE = WM_USER + 0x5001;

	namespace WinForm
	{
		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
		{
			if (message == WM_DESTROY && hWnd == Application::GetMainFormHandle())
			{
				Application::Terminate();
				PostQuitMessage(0);
				return 0;
			}
			else
				return DefWindowProc(hWnd, message, wParam, lParam);
			//else
			//{
			//	Component * comp;
			//	if (message != WM_DESTROY && (comp = Application::GetComponent(hWnd)))
			//		processed = comp->ProcessMessage(WinMessage(hWnd, message, wParam, lParam));
			//	if (!processed)
			//	{
			//		return DefWindowProc(hWnd, message, wParam, lParam);
			//	}
			//	else
			//		return 0;
			//}
		}

		BaseForm::BaseForm()
		{
			menu = 0;
			defButton = cancelButton = 0;
			ownerDraw = true;
			closed = true;
			firstShown = true;
			dlgResult = Cancel;
			accelTable = new AccelTable();
			dlgOwner = 0;
			Application::RegisterForm(this);
		}

		BaseForm::~BaseForm()
		{
			Application::UnregisterForm(this);

			UnSubClass();
			if (menu)
			{
				SetMenu(handle, 0);
				delete menu;
				menu = 0;
			}
			Destroy();
			Application::UnRegisterComponent(this);
		}

		BaseForm::DialogResult BaseForm::ShowModal(BaseForm * owner)
		{
			MSG msg;
			dlgOwner = owner;
			dlgOwner->SetEnabled(false);
			dlgResult = BaseForm::Cancel;
			if (firstShown)
			{
				int l = (owner->GetWidth()-GetWidth())/2;
				int t = (owner->GetHeight()-GetHeight())/2;
				SetLeft(l+owner->GetLeft());
				SetTop(t+owner->GetTop());
				firstShown = false;
			}
			Show();

			while (GetMessage(&msg, NULL, 0,0)>0)
			{
				HWND hwnd = GetActiveWindow();
				if (!accelTable || !TranslateAccelerator(hwnd, 
					accelTable->GetHandle(), &msg))
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
				if (closed)
					break;
			}
			dlgOwner->SetEnabled(true);
			//SetActiveWindow(dlgOwner->GetHandle());
			dlgOwner = NULL;
			return dlgResult;
		}

		void BaseForm::SetDialogResult(DialogResult result)
		{
			dlgResult = result;
			Close();
			closed = true;
		}

		void BaseForm::SetDefaultButton(Button * btn)
		{
			ClearDefaultButtonAction act;
			ApplyActionToComponentAndChildren(&act);
			defButton = btn;
			if (btn)
				btn->SetDefault(true);
		}

		Button * BaseForm::GetDefaultButton()
		{
			return defButton;
		}

		bool BaseForm::GetWantTab()
		{
			return wantTab;
		}

		void BaseForm::SetWantChars(bool val)
		{
			wantChars = val;
		}

		bool BaseForm::GetWantChars()
		{
			return wantChars;
		}

		void BaseForm::SetMainMenu(MainMenu * _m)
		{
			menu = _m;
			SetMenu(handle, _m->GetHandle());
		}

		void BaseForm::Close()
		{
			WindowCloseEventArgs args;
			args.Cancel = false;
			OnClose.Invoke(this, args);
			if (!args.Cancel)
			{
				closed = true;
				if (dlgOwner)
				{
					dlgOwner->SetEnabled(true);
				}
				SetVisible(false);
				if (Application::GetMainFormHandle() == handle)
					Application::Terminate();
			}
		}

		void BaseForm::Show()
		{
			ShowWindow(handle, SW_SHOW);
			UpdateWindow(handle);
			closed = false;
		}

		void BaseForm::BringToFront()
		{
			::BringWindowToTop(handle);
		}

		int BaseForm::MessageBox(const String & msg, const String & title, unsigned int style)
		{
			return ::MessageBoxW(handle, msg.Buffer(), title.Buffer(), style);
		}

		void BaseForm::SetText(String text)
		{
			SetWindowTextW(handle, text.Buffer());
		}

		String BaseForm::GetText()
		{
			const int buflen = 1024;
			wchar_t buf[buflen];
			memset(buf,0,sizeof(wchar_t) * buflen);
			GetWindowTextW(handle, buf, buflen);
			String rs = buf;
			return rs;
		}

		void BaseForm::SetBorder(FormBorder border)
		{
			switch (border)
			{
			case fbNone:
				{
					SetStyle(GWL_STYLE, WS_BORDER, false);
					SetStyle(GWL_STYLE, WS_CAPTION, false);
					SetStyle(GWL_STYLE, WS_SIZEBOX, false);
					SetStyle(GWL_EXSTYLE, WS_EX_TOOLWINDOW, false);
				}
				break;
			case fbSingle:
				{
					SetStyle(GWL_STYLE, WS_BORDER, true);
					SetStyle(GWL_STYLE, WS_CAPTION, true);
					SetStyle(GWL_STYLE, WS_SIZEBOX, false);
					SetStyle(GWL_EXSTYLE, WS_EX_TOOLWINDOW, false);
				}
				break;
			case fbSizeable:
				{
					SetStyle(GWL_STYLE, WS_BORDER, true);
					SetStyle(GWL_STYLE, WS_CAPTION, true);
					SetStyle(GWL_STYLE, WS_SIZEBOX, true);
					SetStyle(GWL_EXSTYLE, WS_EX_TOOLWINDOW, false);
				}
				break;
			case fbFixedDialog:
				{
					SetStyle(GWL_STYLE, WS_BORDER, false);
					SetStyle(GWL_STYLE, WS_CAPTION, true);
					SetStyle(GWL_STYLE, WS_SIZEBOX, false);
					SetStyle(GWL_EXSTYLE, WS_EX_TOOLWINDOW, false);
				}
				break;
			case fbToolWindow:
				{
					SetStyle(GWL_STYLE, WS_BORDER, true);
					SetStyle(GWL_STYLE, WS_CAPTION, true);
					SetStyle(GWL_STYLE, WS_SIZEBOX, true);
					SetStyle(GWL_EXSTYLE, WS_EX_TOOLWINDOW, true);
				}
				break;
			case fbFixedToolWindow:
				{
					SetStyle(GWL_STYLE, WS_BORDER, true);
					SetStyle(GWL_STYLE, WS_CAPTION, true);
					SetStyle(GWL_STYLE, WS_SIZEBOX, false);
					SetStyle(GWL_EXSTYLE, WS_EX_TOOLWINDOW, true);
				}
				break;
			}
			RECT rect;
			GetWindowRect(handle ,&rect);
			MoveWindow(handle,rect.left,rect.top,rect.right-rect.left+1 ,rect.bottom-rect.top,true); 
			MoveWindow(handle,rect.left,rect.top,rect.right-rect.left ,rect.bottom-rect.top,true);
		}

		FormBorder BaseForm::GetBorder()
		{
			LONG_PTR style = GetWindowLongPtr(handle, GWL_STYLE);
			if (style == 0)
				return fbNone;
			else if (((style & (WS_SIZEBOX | WS_CAPTION))) == 
				(WS_SIZEBOX | WS_CAPTION) )
				return fbSizeable;
			else if ((style & (WS_BORDER | WS_CAPTION)) == (WS_BORDER | WS_CAPTION))
				return fbSingle;
			else if ((style & WS_POPUP) == WS_POPUP)
				return fbFixedDialog;
			else
			{
				LONG_PTR exstyle = GetWindowLongPtr(handle, GWL_EXSTYLE);
				if ((exstyle & WS_EX_TOOLWINDOW) == WS_EX_TOOLWINDOW)
				{
					if ((style & WS_SIZEBOX) == WS_SIZEBOX)
						return fbToolWindow;
					else
						return fbFixedToolWindow;
				}
			}
			return fbNone;
		}

		void BaseForm::InvokeAsync(const Event<> & func)
		{
			if (Application::IsUIThread())
			{
				func.Invoke();
			}
			else
			{
				PostMessage(handle, MSG_INVOKE, (WPARAM)&func, 0);
			}
		}

		void BaseForm::Invoke(const Event<> & func)
		{
			if (Application::IsUIThread())
			{
				func.Invoke();
			}
			else
			{
				SendMessage(handle, MSG_INVOKE, (WPARAM)&func, 0);
			}
		}

		int BaseForm::ProcessMessage(WinMessage & msg)
		{
			int rs = -1;
			
			switch (msg.message)
			{
			case WM_CLOSE:
				{
					WindowCloseEventArgs args;
					args.Cancel = false;
					OnClose.Invoke(this, args);
					if (!args.Cancel)
					{
						closed = true;
						SetVisible(false);
						if (Application::GetMainFormHandle() == handle)
						{
							Application::Terminate();
						}
					}
				}
				break;
			case WM_KEYDOWN:
				{
					if (msg.wParam == VK_RETURN)
					{
						if (GetDefaultButton())
						{
							GetDefaultButton()->_Click();
						}
					}
					else if (msg.wParam == VK_ESCAPE)
					{
						if (GetCancelButton())
						{
							GetCancelButton()->_Click();
						}
					}
				}
				break;
			case WM_SHOWWINDOW:
				{
					EventArgs e;
					if (msg.wParam == TRUE)
					{
						OnShow.Invoke(this, e);
						UpdateAccel();
					}
				}
				break;
			case WM_COMMAND:
				{
					Object * obj = Application::GetObject(LOWORD(msg.wParam));
					MenuItem * mn = dynamic_cast<MenuItem *>(obj);
					if (mn)
					{
						mn->Clicked();
						return true;
					}
				}
				break;
			case WM_INITMENUPOPUP:
				{
					Object * obj = Application::GetComponent((HWND)msg.wParam);
					CustomMenu * mn = dynamic_cast<CustomMenu *>(obj);
					if (mn)
					{
						mn->OnPopup();
					}
				}
				break;
			case WM_ACTIVATE:
				UpdateAccel();
				break;
			case WM_GETDLGCODE:
			{			
				rs = 0;
				if (wantChars)
					rs |= DLGC_WANTCHARS;
				if (wantTab)
					rs |= DLGC_WANTTAB;
				break;
			}
			case MSG_INVOKE:
				((Event<>*)msg.wParam)->Invoke();
				rs = 0;
				break;
			}
			if (rs == -1)
				return BaseControl::ProcessMessage(msg);
			return rs;
		}

		void BaseForm::UpdateAccel()
		{
			accelTable->Update();
			Application::SetAccel(accelTable.operator ->());
		}

		void BaseForm::SetShowInTaskBar(bool val)
		{
			SetStyle(GWL_EXSTYLE, WS_EX_APPWINDOW, val);
		}

		bool BaseForm::GetShowInTaskBar()
		{
			return GetStyle(GWL_EXSTYLE, WS_EX_APPWINDOW);
		}

		bool BaseForm::GetControlBox()
		{
			return GetStyle(GWL_STYLE, WS_SYSMENU);
		}
		void BaseForm::SetControlBox(bool val)
		{
			SetStyle(GWL_STYLE,WS_SYSMENU, val);
		}
		bool BaseForm::GetTopMost()
		{
			return GetStyle(GWL_EXSTYLE, WS_EX_TOPMOST);
		}
		void BaseForm::SetTopMost(bool val)
		{
			SetStyle(GWL_EXSTYLE, WS_EX_TOPMOST, val);
		}
		bool BaseForm::GetMinimizeBox()
		{
			return GetStyle(GWL_STYLE, WS_MINIMIZEBOX);
		}
		void BaseForm::SetMinimizeBox(bool val)
		{
			SetStyle(GWL_STYLE, WS_MINIMIZEBOX, val);
		}
		bool BaseForm::GetMaximizeBox()
		{
			return GetStyle(GWL_STYLE, WS_MAXIMIZEBOX);
		}
		void BaseForm::SetMaximizeBox(bool val)
		{
			SetStyle(GWL_STYLE, WS_MAXIMIZEBOX, val);
		}
		void BaseForm::SetClientHeight(int h)
		{
			RECT cr;
			GetClientRect(handle, &cr);
			cr.bottom = cr.top + h;
			RECT wr;
			GetWindowRect(handle, &wr);
			AdjustWindowRect(&cr, (DWORD)GetWindowLongPtr(handle, GWL_STYLE), this->menu!=0);
			MoveWindow(handle, wr.left, wr.top, cr.right-cr.left, cr.bottom-cr.top, 1);
		}
		void BaseForm::UpdateAccel(Accelerator acc, MenuItem * item)
		{
			accelTable->UpdateAccel(acc, item);
		}
		void BaseForm::RegisterAccel(Accelerator acc, MenuItem * item)
		{
			accelTable->RegisterAccel(acc, item);
		}
		void BaseForm::SetClientWidth(int w)
		{
			RECT cr;
			GetClientRect(handle, &cr);
			cr.right = cr.left + w;
			RECT wr;
			GetWindowRect(handle, &wr);
			AdjustWindowRect(&cr, (DWORD)GetWindowLongPtr(handle, GWL_STYLE), this->menu!=0);
			MoveWindow(handle, wr.left, wr.top, cr.right-cr.left, cr.bottom-cr.top, 1);
		}
		int BaseForm::GetClientHeight()
		{
			RECT cr;
			GetClientRect(handle, &cr);
			return cr.bottom - cr.top;
		}
		void BaseForm::SetWantTab(bool val)
		{
			wantTab = val;
		}
		int BaseForm::GetClientWidth()
		{
			RECT cr;
			GetClientRect(handle, &cr);
			return cr.right - cr.left;
		}

		void BaseForm::SetCancelButton(Button * btn)
		{
			cancelButton = btn;
		}

		Button * BaseForm::GetCancelButton()
		{
			return cancelButton;
		}

		Form::Form()
		{
			Create();
		
		}

		Form::~Form()
		{
			Application::UnRegisterComponent(this);
		}

		void Form::Create()
		{
			handle = CreateWindowW(Application::FormClassName, 0, WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, Application::GetHandle(), NULL);
			if (!handle)
			{
				throw "Failed to create window.";
			}
			Application::RegisterComponent(this);
			SubClass();
		}
	}
}

/***********************************************************************
LISTBOX.CPP
***********************************************************************/

namespace CoreLib
{
	namespace WinForm
	{
		void ListBox::CreateWnd(Component * parent)
		{
			handle = CreateWindowEx
				(
					0,
					L"ListBox",
					L"ListBox",
					WS_TABSTOP | WS_VISIBLE | WS_CHILD | LBS_NOTIFY,
					10,
					10,
					100,
					100,
					parent->GetHandle(),
					NULL,
					Application::GetHandle(),
					NULL
					);
			captureClick = false;
			if (!handle)
			{
				throw "Failed to create window.";
			}
		}
		int ListBox::ProcessNotification(WinNotification note)
		{
			switch (note.code)
			{
			case LBN_SELCHANGE:
			{
				_SelChange();
			}
			break;
			}
			return Control::ProcessNotification(note);
		}
		ListBox::ListBox()
		{
		}
		void ListBox::_SelChange()
		{
			SelectionChanged(this, EventArgs());
		}
		ListBox::ListBox(Component * parent)
		{
			Init(parent);
		}
		void ListBox::Clear()
		{
			SendMessage(handle, LB_RESETCONTENT, 0, 0);
		}
		void ListBox::AddItem(const String & text)
		{
			SendMessage(handle, LB_ADDSTRING, 0, (LPARAM)text.Buffer());
		}
		void ListBox::InsertItem(int id, const String & text)
		{
			SendMessage(handle, LB_INSERTSTRING, id, (LPARAM)text.Buffer());
		}
		void ListBox::RemoveItem(int id)
		{
			SendMessage(handle, LB_DELETESTRING, id, 0);
		}
		int ListBox::Count()
		{
			return (int)SendMessage(handle, LB_GETCOUNT, 0, 0);
		}
		String ListBox::GetItem(int id)
		{
			int len = (int)SendMessage(handle, LB_GETTEXTLEN, id, 0);
			List<wchar_t> buffer;
			buffer.SetSize(len + 1);
			SendMessage(handle, LB_GETTEXT, id, (LPARAM)buffer.Buffer());
			if (buffer.Count() > 0)
				buffer[buffer.Count() - 1] = 0;
			return String(buffer.Buffer());
		}
		int ListBox::GetSelectionIndex()
		{
			return (int)SendMessage(handle, LB_GETCURSEL, 0, 0);
		}
		void ListBox::SetSelectionIndex(int idx)
		{
			SendMessage(handle, LB_SETCURSEL, idx, 0);
		}

		void ComboBox::CreateWnd(Component * pParent)
		{
			parent = pParent;
			handle = CreateWindowEx
				(
					0,
					L"ComboBox",
					L"ComboBox",
					WS_GROUP | WS_TABSTOP | WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
					10,
					10,
					100,
					100,
					parent->GetHandle(),
					NULL,
					Application::GetHandle(),
					NULL
					);
			captureClick = false;
			if (!handle)
			{
				throw "Failed to create window.";
			}
		}
		int ComboBox::ProcessNotification(WinNotification note)
		{
			switch (note.code)
			{
			case CBN_SELCHANGE:
			{
				_SelChange();
			}
			break;
			case CBN_EDITCHANGE:
			{
				_TextChange();
			}
			break;
			}
			return Control::ProcessNotification(note);
		}
		ComboBox::ComboBox()
		{
		}
		void ComboBox::_SelChange()
		{
			SelectionChanged(this, EventArgs());
		}
		void ComboBox::_TextChange()
		{
			TextChanged(this, EventArgs());
		}
		ComboBox::ComboBox(Component * parent)
		{
			Init(parent);
		}
		void ComboBox::Clear()
		{
			SendMessage(handle, CB_RESETCONTENT, 0, 0);
		}
		int ComboBox::Count()
		{
			return (int)SendMessage(handle, CB_GETCOUNT, 0, 0);
		}
		void ComboBox::SetStyle(ComboBoxStyle style)
		{
			RECT rect;
			GetWindowRect(handle, &rect);
			SetWindowLongPtr(handle, GWL_STYLE, WS_TABSTOP | WS_VISIBLE | WS_CHILD | (int)style);
			/*DestroyWindow(handle);
			handle = CreateWindowEx
				(
					0,
					L"COMBOBOX",
					L"ComboBox",
					WS_TABSTOP | WS_VISIBLE | WS_CHILD | (int)style,
					rect.left,
					rect.top,
					rect.right-rect.left,
					rect.bottom - rect.top,
					parent->GetHandle(),
					NULL,
					Application::GetHandle(),
					NULL
					);*/
		}
		void ComboBox::AddItem(const String & text)
		{
			SendMessage(handle, CB_ADDSTRING, 0, (LPARAM)text.Buffer());
		}
		void ComboBox::InsertItem(int id, const String & text)
		{
			SendMessage(handle, CB_INSERTSTRING, id, (LPARAM)text.Buffer());
		}
		void ComboBox::RemoveItem(int id)
		{
			SendMessage(handle, CB_DELETESTRING, id, 0);
		}
		String ComboBox::GetItem(int id)
		{
			int len = (int)SendMessage(handle, CB_GETLBTEXTLEN, id, 0);
			List<wchar_t> buffer;
			buffer.SetSize(len + 1);
			SendMessage(handle, CB_GETLBTEXT, id, (LPARAM)buffer.Buffer());
			if (buffer.Count() > 0)
				buffer[buffer.Count() - 1] = 0;
			return String(buffer.Buffer());
		}
		int ComboBox::GetSelectionIndex()
		{
			return (int)SendMessage(handle, CB_GETCURSEL, 0, 0);
		}
		void ComboBox::SetSelectionIndex(int idx)
		{
			SendMessage(handle, CB_SETCURSEL, idx, 0);
		}
	}
}

/***********************************************************************
MENU.CPP
***********************************************************************/
namespace CoreLib
{
	namespace WinForm
	{

		MainMenu::MainMenu()
		{
			handle = (HWND)CreateMenu();
			Application::RegisterComponent(this);
		}

		PopupMenu::PopupMenu(Form * form)
		{
			ownerForm = form;
			handle = (HWND)CreatePopupMenu();
			Application::RegisterComponent(this);
		}

		CustomMenu::CustomMenu()
		{
			_owner = 0;
			ident = Application::RegisterHandle(this);
		}

		CustomMenu::~CustomMenu()
		{
			Application::UnRegisterComponent(this);
			Application::UnRegisterHandle(ident);
			DestroyMenu((HMENU)handle);
		}

		void MenuItem::Init(int pos)
		{
			ident = Application::RegisterHandle(this);
			popup = new PopupMenu();
			if (pos == -1)
				parent->AddItem(this);
			else
				parent->InsertItem(this, pos);
			popup->_owner = this;
		}

		MenuItem::MenuItem(CustomMenu * menu, int pos)
			: parent(menu), type(mitCommand)
		{
			Init(pos);
		}

		MenuItem::MenuItem(MenuItem * menu, int pos)
			: parent(menu->popup.operator ->()), type(mitCommand)	
		{
			Init(pos);
			menu->SetType(mitDropDown);
		}

		MenuItem::~MenuItem()
		{
			popup = 0;
			Application::UnRegisterHandle(ident);
		}

		PopupMenu * MenuItem::GetSubMenus()
		{
			return popup.operator ->();
		}

		int MenuItem::GetIdentifier()
		{
			return ident;
		}

		void MenuItem::Clicked()
		{
			OnClick.Invoke(this, EventArgs());
		}

		void MenuItem::GetState(bool &_enabled, bool &_checked)
		{
			MENUITEMINFO info;
			memset(&info, 0, sizeof(MENUITEMINFO));
			info.cbSize = sizeof(MENUITEMINFO);
			info.fMask = MIIM_STATE;
			GetMenuItemInfo(parent->GetHandle(), (UINT)ident, FALSE, &info);
			_enabled = (info.fState & MFS_ENABLED) == MFS_ENABLED;
			_checked = (info.fState & MFS_CHECKED) == MFS_CHECKED;
		}

		void MenuItem::SetState(bool _enabled, bool _checked)
		{
			MENUITEMINFO info;
			memset(&info, 0, sizeof(MENUITEMINFO));
			info.cbSize = sizeof(MENUITEMINFO);
			info.fMask = MIIM_STATE;
			info.fState = (_enabled?MFS_ENABLED:MFS_DISABLED) | (_checked?MFS_CHECKED:MFS_UNCHECKED);
			SetMenuItemInfo(parent->GetHandle(), (UINT)ident, FALSE, &info);
		}

		void MenuItem::SetEnabled(bool _enabled)
		{
			bool e,c;
			GetState(e,c);
			SetState(_enabled, c);
		}

		void MenuItem::SetChecked(bool _checked)
		{
			bool e,c;
			GetState(e,c);
			SetState(e,_checked);
		}

		bool MenuItem::GetChecked()
		{
			bool e,c;
			GetState(e,c);
			return c;
		}

		bool MenuItem::GetEnabled()
		{
			bool e,c;
			GetState(e,c);
			return e;
		}

		void MenuItem::SetType(MenuItemType ntype)
		{
			MENUITEMINFO info;
			info.cbSize = sizeof(MENUITEMINFO);
		
			if (ntype == mitCommand)
			{
				info.fMask = MIIM_SUBMENU;
				info.hSubMenu = 0;
			}
			else if (ntype == mitDropDown)
			{
				info.fMask = MIIM_SUBMENU;
				info.hSubMenu = popup->GetHandle();
			}
			else if (ntype == mitSeperator)
			{
				info.fMask = MIIM_FTYPE;
				info.fType = MFT_SEPARATOR;
			}

			SetMenuItemInfo(parent->GetHandle(), (UINT)ident, FALSE, &info);
		}

		void MenuItem::SetText(const String & _text, const String & _shortcut)
		{
			text = _text;
			shortcutText = _shortcut;
			UpdateText();
		}

		void MenuItem::SetText(const String & _text)
		{
			text = _text;
			UpdateText();
		}

		void MenuItem::SetShortcutText(const String & _shortcut)
		{
			shortcutText = _shortcut;
			UpdateText();
		}

		String MenuItem::GetText()
		{
			return text;
		}

		String MenuItem::GetShortcutText()
		{
			return shortcutText;
		}

		String MenuItem::GetCompleteText()
		{
			return text+L"\t"+shortcutText;
		}

		void MenuItem::UpdateText()
		{
			if (parent)
			{
				MENUITEMINFO info;
				info.cbSize = sizeof(MENUITEMINFO);
				info.fMask = MIIM_STRING;
				String txt = text + L"\t" + shortcutText;
				info.dwTypeData = (LPWSTR)txt.Buffer();
				SetMenuItemInfo(parent->GetHandle(), (UINT)ident, FALSE, &info);
			}
		}

		void MenuItem::UpdateState()
		{
			SetType(type);
			UpdateText();
		}

		void CustomMenu::OnPopup()
		{
			if (_owner)
				_owner->OnPopup.Invoke(_owner, EventArgs());
		}

		HMENU CustomMenu::GetHandle()
		{
			return (HMENU)handle;
		}

		int CustomMenu::GetCount()
		{
			return items.Count();
		}

		MenuItem * CustomMenu::GetItem(int id)
		{
			return items[id].operator->();
		}

		MenuItem * CustomMenu::operator [](int id)
		{
			return GetItem(id);
		}

		void CustomMenu::InsertItem(MenuItem * item, int pos)
		{
			items.Insert(pos, item);
			InsertMenu((HMENU)handle, pos, MF_BYPOSITION, (UINT_PTR)item->GetIdentifier(), 
				(LPWSTR)item->GetCompleteText().Buffer());
			item->UpdateState();
		}

		void CustomMenu::Clear()
		{
			for (auto & item : items)
				RemoveMenu((HMENU)handle, item->GetIdentifier(), FALSE);
			for (int i = 0; i<items.Count(); i++)
			{
				items[i] = 0;
			}
			items.Clear();
		}

		void CustomMenu::RemoveItem(MenuItem * item)
		{
			int fid = items.IndexOf(item);
			if (fid != -1)
			{
				RemoveMenu((HMENU)handle, item->GetIdentifier(), FALSE);
				items[fid] = nullptr;
				items.RemoveAt(fid);
				items.Compress();
			}
		}

		void CustomMenu::AddItem(MenuItem * item)
		{
			items.Add(item);
			AppendMenu((HMENU)handle, MF_STRING, (UINT_PTR)item->GetIdentifier(), 
				(LPWSTR)item->GetCompleteText().Buffer());
			item->UpdateState();
		}

		void PopupMenu::Popup(int x, int y)
		{
			int dropalign = GetSystemMetrics(SM_MENUDROPALIGNMENT);
			if (ownerForm)
				TrackPopupMenuEx((HMENU)handle, dropalign, x, y, ownerForm->GetHandle(), 0);
		}

	}
}

/***********************************************************************
MESSAGE.CPP
***********************************************************************/
namespace CoreLib
{
	namespace WinForm
	{
		WinMessage::WinMessage(HWND _hWnd, UINT _message, WPARAM _wParam, LPARAM _lParam)
		{
			hWnd = _hWnd;
			message = _message;
			wParam = _wParam;
			lParam = _lParam;
		}

		MouseEventArgs::MouseEventArgs()
			:Button(mbNone), X(0), Y(0), Delta(0)
		{
		}
	
		MouseEventArgs::MouseEventArgs(WinForm::MouseButton btn, int _x, int _y, int _delta, unsigned int buttons, bool * processed)
			:Button(btn), X(_x), Y(_y), Delta(_delta)
		{
			BaseControl = (buttons & MK_CONTROL) == MK_CONTROL;
			Shift = (buttons & MK_SHIFT) == MK_SHIFT;
			LButton = mbLeft?true:false;
			RButton = mbRight?true:false;
			MButton = mbMiddle?true:false;
			Processed = processed;
		}

		KeyEventArgs::KeyEventArgs(wchar_t key, int * keycode, bool ctrl, bool shift, bool alt)
			:Key(key), KeyCodeRef(keycode), KeyCode(*keycode), BaseControl(ctrl), Shift(shift), Alt(alt)
		{
		}
	}
}

/***********************************************************************
TEXTBOX.CPP
***********************************************************************/

namespace CoreLib
{
	namespace WinForm
	{
		TextBox::TextBox(Component * parent)
			: Control()
		{
			Init(parent);
		}
		TextBox::TextBox()
		{
		}

		void TextBox::CreateWnd(Component * parent)
		{
			handle = CreateWindowEx
			(
				WS_EX_CLIENTEDGE,
				L"EDIT",
				L"Edit",
				WS_TABSTOP | WS_VISIBLE | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL ,
				10,
				10,
				100,
				100,
				parent->GetHandle(),
				NULL,
				(HINSTANCE)(CoreLib::PtrInt)GetWindowLong(parent->GetHandle(), -6),
				NULL
			); 
			captureClick = false;
			if (!handle)
			{
				throw "Failed to create window.";
			}
		}

		void TextBox::SetText(const String & text)
		{
			SendMessage(handle, WM_SETTEXT, 0, (LPARAM)text.Buffer());
		}

		String TextBox::GetText()
		{
			int len = GetWindowTextLengthW(handle);
			List<wchar_t> buffer;
			buffer.SetSize(len+1);
			buffer.Last() = 0;
			GetWindowTextW(handle, buffer.Buffer(), len+1);
			return String(buffer.Buffer());
		}

		int TextBox::ProcessNotification(WinNotification note)
		{
			if (note.code == EN_CHANGE) 
			{
				OnChanged.Invoke(this, EventArgs());
				return -1;
			}
			else
				return Control::ProcessNotification(note);
		}


		void TextBox::SetReadOnly(bool val)
		{
			SetStyle(GWL_STYLE, ES_READONLY, val);
		}
		bool TextBox::GetReadOnly()
		{
			return GetStyle(GWL_STYLE, ES_READONLY);
		}
		void TextBox::SetMultiLine(bool val)
		{
			RECT rect;
			GetClientRect(handle, &rect);
			Application::UnRegisterComponent(this);
			auto parent = ::GetParent(handle);
			DestroyWindow(handle);
			if (val)
			{
				handle = CreateWindowEx
					(
					WS_EX_CLIENTEDGE,
					L"EDIT",
					L"Edit",
					WS_TABSTOP | WS_VISIBLE | WS_CHILD | ES_LEFT | ES_AUTOVSCROLL | ES_MULTILINE | ES_WANTRETURN | WS_VSCROLL,
					10,
					10,
					100,
					100,
					parent,
					NULL,
					(HINSTANCE)(CoreLib::PtrInt)GetWindowLong(parent, -6),
					NULL
					);
			}
			else
			{
				handle = CreateWindowEx
					(
					WS_EX_CLIENTEDGE,
					L"EDIT",
					L"Edit",
					WS_TABSTOP | WS_VISIBLE | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL,
					10,
					10,
					100,
					100,
					parent,
					NULL,
					(HINSTANCE)(CoreLib::PtrInt)GetWindowLong(parent, -6),
					NULL
					);
			}
			Application::RegisterComponent(this);
			MoveWindow(handle, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, TRUE);
			InitFont();
		}
		bool TextBox::GetMultiLine()
		{
			return GetStyle(GWL_STYLE, ES_MULTILINE);
		}
		void TextBox::SetPassword(bool val)
		{
			SetStyle(GWL_STYLE, ES_PASSWORD, val);
		}
		bool TextBox::GetPassword()
		{
			return GetStyle(GWL_STYLE, ES_PASSWORD);
		}
		void TextBox::SetAcceptNumber(bool val)
		{
			SetStyle(GWL_STYLE, ES_NUMBER, val);
		}
		bool TextBox::GetAcceptNumber()
		{
			return GetStyle(GWL_STYLE, ES_NUMBER);
		}


		Label::Label(Component * parent)
			: Control()
		{
			Init(parent);
		}
		Label::Label()
		{
		}

		int TextBox::ProcessMessage(WinMessage & msg)
		{
			if (msg.message == WM_GETDLGCODE)
				return DLGC_WANTALLKEYS;
			return Control::ProcessMessage(msg);
		}

		void Label::CreateWnd(Component * parent)
		{
			handle = CreateWindowEx
			(
				0,
				L"STATIC",
				L"Label",
				WS_VISIBLE | WS_CHILD,
				10,
				10,
				100,
				100,
				parent->GetHandle(),
				NULL,
				Application::GetHandle(), 
				NULL
			); 
			captureClick = false;
			if (!handle)
			{
				throw "Failed to create window.";
			}
		}

		void Label::SetText(const String & text)
		{
			SetWindowTextW(handle, text.Buffer());
		}

		String Label::GetText()
		{
			int len = GetWindowTextLengthW(handle);
			List<wchar_t> buffer;
			buffer.SetSize(len+1);
			buffer.Last() = 0;
			GetWindowTextW(handle, buffer.Buffer(), len);
			return String(buffer.Buffer());
		}
	}
}

/***********************************************************************
TIMER.CPP
***********************************************************************/
namespace CoreLib
{
	namespace WinForm
	{
		void CALLBACK TimerFunc(HWND /*hwnd*/, UINT /*uMsg*/, UINT_PTR idEvent, DWORD /*dwTime*/)
		{
			Timer * timer = ((Timer *)Application::GetObjectFromHandle((int)idEvent));
			if (timer)
				timer->OnTick.Invoke(timer, EventArgs());
		}

		Timer::Timer()
		{
			Interval = 100;
			timerHandle = (UINT_PTR)this;
		}

		Timer::~Timer()
		{
			KillTimer(0, timerHandle);
			Application::UnRegisterHandle(timerHandle);
		}

		void Timer::StartTimer()
		{
			timerHandle = SetTimer(0, timerHandle, Interval, TimerFunc);
			Application::RegisterObjectHandle(timerHandle, this);
		}
	
		void Timer::StopTimer()
		{
		
			KillTimer(0, timerHandle);
		}
	}
}
