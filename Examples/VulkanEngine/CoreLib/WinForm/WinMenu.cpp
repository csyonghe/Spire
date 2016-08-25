#include "WinMenu.h"
#include "WinApp.h"
#include "WinForm.h"
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
			: type(mitCommand), parent(menu)
		{
			Init(pos);
		}

		MenuItem::MenuItem(MenuItem * menu, int pos)
			: type(mitCommand), parent(menu->popup.operator ->())
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