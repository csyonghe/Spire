#ifndef GX_WINMENU_H
#define GX_WINMENU_H

#include "../Basic.h"
#include "WinCtrls.h"
#include "WinMessage.h"

namespace CoreLib
{
	namespace WinForm
	{
		class CustomMenu;
		class PopupMenu;
		class Form;

		class MenuItem : public Object
		{
		public:
			enum MenuItemType
			{
				mitCommand, mitDropDown, mitSeperator
			};
		private:
			String text;
			String shortcutText;
			MenuItemType type;
			RefPtr<PopupMenu> popup;
			CustomMenu * parent;
			int ident;
			void UpdateText();
			void Init(int pos);
			void SetBitmap();
			void SetState(bool _enabled, bool _checked);
			void GetState(bool &_enabled, bool &_checked);
		public:
			NotifyEvent OnClick;
			NotifyEvent OnPopup;
			PopupMenu * GetSubMenus();
			void SetText(const String & _text, const String & _shortcut);
			void SetText(const String & _text);
			void SetShortcutText(const String & _shortcut);
			void SetType(MenuItemType ntype);
			void SetEnabled(bool _enabled);
			void SetChecked(bool _checked);
			bool GetEnabled();
			bool GetChecked();

			String GetText();
			String GetCompleteText();
			String GetShortcutText();
			void UpdateState();
			void Clicked();
			int GetIdentifier();
		public:
			MenuItem(CustomMenu * menu, int pos = -1);
			MenuItem(MenuItem * menu, int pos = -1);
			~MenuItem();
		};

		class CustomMenu : public Component
		{
			friend class MenuItem;
		protected:
			List<RefPtr<MenuItem>> items;
			int ident;
			MenuItem * _owner;
		public:
			void AddItem(MenuItem * item);
			void InsertItem(MenuItem * item, int pos);
			void RemoveItem(MenuItem * item);
			void Clear();
			HMENU GetHandle();
			int GetCount();
			MenuItem * GetItem(int id);
			MenuItem * operator [](int id);
			CustomMenu();
			void OnPopup();
			virtual ~CustomMenu();
		};

		class PopupMenu : public CustomMenu
		{
		private:
			Form * ownerForm;
		public:
			PopupMenu(Form * _owner = 0);
			void Popup(int x, int y);
		
		};

		class MainMenu : public CustomMenu
		{
		public:
			MainMenu();
		};
	}
}
#endif