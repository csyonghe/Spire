#include "WinTextBox.h"
#include "WinApp.h"

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