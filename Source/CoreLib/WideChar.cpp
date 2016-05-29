#include "WideChar.h"
#ifdef WINDOWS_PLATFORM
#include <Windows.h>
#endif
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "SecureCRT.h"

class DefaultLocaleSetter
{
public:
	DefaultLocaleSetter()
	{
		setlocale(LC_ALL, ""); 
	};
};

char * WideCharToMByte(const wchar_t * buffer, int length)
{
#ifdef WINDOWS_PLATFORM
	size_t requiredBufferSize;
	requiredBufferSize = WideCharToMultiByte(CP_OEMCP, NULL, buffer, length, 0, 0, NULL, NULL)+1;
	if (requiredBufferSize)
	{
		char * multiByteBuffer = new char[requiredBufferSize];
		WideCharToMultiByte(CP_OEMCP, NULL, buffer, length, multiByteBuffer, (int)requiredBufferSize, NULL, NULL);
		multiByteBuffer[requiredBufferSize-1] = 0;
		return multiByteBuffer;
	}
	else
		return 0;
	
#else
	static DefaultLocaleSetter setter;
	size_t ret;
	char * dest = new char[length*2 + 1];
	memset(dest, 0, sizeof(char)*(length*2+1));
	wcstombs_s(&ret, dest, length*2+1, buffer, _TRUNCATE);
	return dest;
#endif
}

wchar_t * MByteToWideChar(const char * buffer, int length)
{
#ifdef WINDOWS_PLATFORM
	// regard as ansi
	MultiByteToWideChar(CP_ACP, 0, buffer, length, NULL, 0);
	if (length < 0) length = 0;
	if (length != 0)
	{
		wchar_t * rbuffer = new wchar_t[length+1];
		MultiByteToWideChar(CP_ACP, NULL, buffer, length, rbuffer, length+1);
		rbuffer[length] = 0;
		return rbuffer;
	}
	else
		return 0;
#else
	size_t ret;
	static DefaultLocaleSetter setter;
	wchar_t * dest = new wchar_t[length+1];
	memset(dest, 0, sizeof(wchar_t)*(length+1));
	mbstowcs_s(&ret, dest, length+1, buffer, _TRUNCATE);
	return dest;
#endif
}

void MByteToWideChar(wchar_t * buffer, int bufferSize, const char * str, int length)
{
#ifdef WINDOWS_PLATFORM
	// regard as ansi
	MultiByteToWideChar(CP_ACP, NULL, str, length, buffer, bufferSize);
#else
	size_t ret;
	static DefaultLocaleSetter setter;
	mbstowcs_s(&ret, buffer, bufferSize, str, _TRUNCATE);
#endif
}
