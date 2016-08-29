#ifndef GX_WIN_APP_H
#define GX_WIN_APP_H

#include <Windows.h>
#include "../Basic.h"
#include "WinAccel.h"
#include "WinMessage.h"
#include "../Regex/MetaLexer.h"
#include "Debug.h"

namespace CoreLib
{
	namespace WinForm
	{
		using namespace CoreLib::Text;

		class Component;
		class BaseForm;
		class AccelTable;

		class Application : public Object
		{
		private:
			static DWORD uiThreadId;
			static int globalUID;
			static HINSTANCE instance, prevInstance;
			static HWND mainFormHandle;
			static BaseForm * mainForm;
			static String *cmdLine;
			static Dictionary<HWND, Component* > *components;
			static Dictionary<UINT_PTR, Object *> *objMap;
			static int cmdShow;
			static void * gdiToken;
			static AccelTable * accelTable;
			static LOGFONT SysFont;
			static bool terminate;
			static NotifyEvent * onMainLoop;
			static EnumerableHashSet<BaseForm*> forms;
			static void RegisterControlClass();
			static void RegisterFormClass();
			static void RegisterGLFormClass();
			static void ProcessMessage(MSG & msg);
		public:
			static const wchar_t * ControlClassName;
			static const wchar_t * FormClassName;
			static const wchar_t * GLFormClassName;
			static void Init();
			static void Init(HINSTANCE hIns, HINSTANCE hPrevIns, LPTSTR cmdLine, int show = 0);
			static void Dispose();
			static void RegisterComponent(Component * Ctrl);
			static void UnRegisterComponent(Component *  Ctrl);
			static Component * GetComponent(const HWND handle); 
			static HINSTANCE GetHandle();
			static void SetMainLoopEventHandler(NotifyEvent * mainLoop);
			static void Run(const BaseForm * MainForm, bool NonBlocking = false);
			static void Terminate();
			static HWND GetMainFormHandle();
			static BaseForm * GetMainForm();
			static void DoEvents();
			static int RegisterHandle(Object * obj);
			static void UnRegisterHandle(UINT_PTR _handle);
			static Object * GetObject(UINT_PTR _handle);
			static String GetCommandLine();
			static String GetExePath();
			static void SetAccel(AccelTable * acc);
			static void RegisterObjectHandle(UINT_PTR handle, Object * obj);
			static Object * GetObjectFromHandle(UINT_PTR handle);
			static LOGFONT GetSysFont();
			static int GenerateGUID();
			static bool IsUIThread();
			static void RegisterForm(BaseForm * form);
			static void UnregisterForm(BaseForm * form);

		};

		class SystemMetrics
		{
		public:
			static int GetScreenWidth();
			static int GetScreenHeight();
		};

		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	}
}

#endif