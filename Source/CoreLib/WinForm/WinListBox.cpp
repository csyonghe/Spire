#include "WinListBox.h"
#include "WinApp.h"

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
