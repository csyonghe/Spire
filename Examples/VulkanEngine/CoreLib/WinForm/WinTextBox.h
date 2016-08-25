#ifndef GX_WINTEXTBOX_H
#define GX_WINTEXTBOX_H

#include "WinCtrls.h"

namespace CoreLib
{
	namespace WinForm
	{
		class TextBox : public Control
		{		
		protected:
			virtual void CreateWnd(Component * parent) override;
			virtual int ProcessMessage(WinMessage & msg) override;

			virtual int ProcessNotification(WinNotification note) override;
			TextBox();
		public:
			NotifyEvent OnChanged;
			TextBox(Component * parent);
			void SetText(const String & text);
			String GetText();
			void SetReadOnly(bool val);
			bool GetReadOnly();
			void SetMultiLine(bool val);
			bool GetMultiLine();
			void SetPassword(bool val);
			bool GetPassword();
			void SetAcceptNumber(bool val);
			bool GetAcceptNumber();
		};

		class Label : public Control
		{		
		protected:
			virtual void CreateWnd(Component * parent);
			Label();
		public:
			Label(Component * parent);
			void SetText(const String & text);
			String GetText();
		};
	}
}

#endif