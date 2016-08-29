#ifndef GX_WINLISTBOX_H
#define GX_WINLISTBOX_H

#include "WinCtrls.h"

namespace CoreLib
{
	namespace WinForm
	{
		class ListBox : public Control
		{
		protected:
			virtual void CreateWnd(Component * parent) override;
			virtual int ProcessNotification(WinNotification note) override;
			ListBox();
			void _SelChange();
		public:
			NotifyEvent SelectionChanged;
			ListBox(Component * parent);
			void Clear();
			int Count();
			void AddItem(const String & text);
			void InsertItem(int id, const String & text);
			void RemoveItem(int id);
			String GetItem(int id);
			int GetSelectionIndex();
			void SetSelectionIndex(int idx);
		};

		enum class ComboBoxStyle
		{
			Simple = CBS_SIMPLE, DropDown = CBS_DROPDOWN, DropDownList = CBS_DROPDOWNLIST
		};

		class ComboBox : public Control
		{
		protected:
			Component * parent;
			virtual void CreateWnd(Component * pParent) override;
			virtual int ProcessNotification(WinNotification note) override;
			ComboBox();
			void _SelChange();
			void _TextChange();

		public:
			NotifyEvent SelectionChanged;
			NotifyEvent TextChanged;

			ComboBox(Component * parent);
			void Clear();
			int Count();
			void SetStyle(ComboBoxStyle style);
			void AddItem(const String & text);
			void InsertItem(int id, const String & text);
			void RemoveItem(int id);
			String GetItem(int id);
			int GetSelectionIndex();
			void SetSelectionIndex(int idx);
		};
	}
}
#endif
