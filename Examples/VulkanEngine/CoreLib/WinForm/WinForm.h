#ifndef GX_WINFORM_H
#define GX_WINFORM_H

#include "WinCtrls.h"
#include "WinMenu.h"
#include "WinAccel.h"

namespace CoreLib
{
	namespace WinForm
	{
		class Button;

		enum FormBorder
		{
			fbNone,
			fbSingle,
			fbSizeable,
			fbFixedDialog,
			fbToolWindow,
			fbFixedToolWindow
		};

		class BaseForm : public BaseControl 
		{
		public:
			enum DialogResult
			{
				OK, Cancel, Yes, No, Retry, Ignore
			};
		protected:
			DialogResult dlgResult;
			bool closed, firstShown;
			bool wantTab = false, wantChars = false;
			BaseForm * dlgOwner;
			Button * defButton, * cancelButton;
			int ProcessMessage(WinMessage & msg);
			virtual void Create()=0;
		private:
			MainMenu * menu;
			RefPtr<AccelTable> accelTable;
		public:
			BaseForm();
			virtual ~BaseForm();
		public:
			void		Show();
			void		BringToFront();
			DialogResult ShowModal(BaseForm * owner);
			void		Close();
			void		SetDialogResult(DialogResult result);
			void		SetText(String text);
			String	GetText();
			void		SetBorder(FormBorder border);
			FormBorder	GetBorder();
			bool		GetShowInTaskBar();
			void		SetShowInTaskBar(bool val);
			bool		GetControlBox();
			void		SetControlBox(bool val);
			bool		GetTopMost();
			void		SetTopMost(bool val);
			bool		GetMinimizeBox();
			void		SetMinimizeBox(bool val);
			bool		GetMaximizeBox();
			void		SetMaximizeBox(bool val);
			void		SetClientWidth(int w);
			void		SetClientHeight(int h);
			int			GetClientWidth();
			int			GetClientHeight();
			void		SetWantTab(bool val);
			bool		GetWantTab();
			void		SetWantChars(bool val);
			bool		GetWantChars();
			void		SetMainMenu(MainMenu * _menu);
			void		SetDefaultButton(Button * btn);
			Button *	GetDefaultButton();
			void		SetCancelButton(Button * btn);
			Button *	GetCancelButton();
			int			MessageBox(const String & msg, const String & title, unsigned int style);
			void		Invoke(const Event<> & func);
			void		InvokeAsync(const Event<> & func);
			template <typename Func>
			void Invoke(const Func & f)
			{
				Invoke(Event<>(f));
			}
		public:
			NotifyEvent OnLoad;
			NotifyEvent OnShow;
			WindowCloseEvent OnClose;
			void RegisterAccel(Accelerator acc, MenuItem * item);
			void UpdateAccel(Accelerator acc, MenuItem * item);
			void UpdateAccel();
			void CenterScreen();
		};

		class Form : public BaseForm
		{
		protected:
			virtual void Create();
		public:
			Form();
			~Form();
		};
	}
}
#endif