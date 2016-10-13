#ifndef GX_WIN_COMMON_DLG_H
#define GX_WIN_COMMON_DLG_H

#include "WinCtrls.h"

namespace CoreLib
{
	namespace WinForm
	{
		class FileDialog : public Object
		{
		private:
			static const int FilterBufferSize = 512;
			static const int FileBufferSize = 25600;
		private:
			Component * owner;
			String initDir;
		private:
			wchar_t fileBuf[FileBufferSize];
			wchar_t filterBuf[FilterBufferSize];
			OPENFILENAME fn;
			void PrepareDialog();
			void PostDialogShow();
		public:
			String Filter;
			String DefaultEXT;
			String FileName;
			List<String> FileNames;
			bool MultiSelect;
			bool CreatePrompt;
			bool FileMustExist;
			bool HideReadOnly;
			bool OverwritePrompt;
			bool PathMustExist;
			bool ShowOpen();
			bool ShowSave();
			FileDialog(const Component * _owner);
			~FileDialog();
		};

		class ColorDialog : public Object
		{
		private:
			Component * owner;
			COLORREF cr[16];
			CHOOSECOLOR cs;
		public:
			COLORREF Color;
			bool FullOpen;
			bool PreventFullOpen;
			bool ShowColor();
			ColorDialog(Component * _owner);	
		};
	}
}

#endif