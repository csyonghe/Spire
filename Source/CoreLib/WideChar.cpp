#include "WideChar.h"
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <cstdlib>
#include "SecureCRT.h"

#define _CRT_SECUIRE_NO_WARNINGS

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
	size_t requiredBufferSize;
#ifdef _MSC_VER
	wcstombs_s(&requiredBufferSize, nullptr, 0, buffer, length);
#else
	requiredBufferSize = std::wcstombs(nullptr, buffer, 0);
#endif
	if (requiredBufferSize > 0)
	{
		char * multiByteBuffer = new char[requiredBufferSize + 1];
#ifdef _MSC_VER
		wcstombs_s(&requiredBufferSize, multiByteBuffer, requiredBufferSize, buffer, length);
		int pos = (int)requiredBufferSize;
#else
		int pos = (int)std::wcstombs(multiByteBuffer, buffer, requiredBufferSize + 1);
#endif
		if (pos <= (int)requiredBufferSize && pos >= 0)
			multiByteBuffer[pos] = 0;
		return multiByteBuffer;
	}
	else
		return 0;
}

wchar_t * MByteToWideChar(const char * buffer, int length)
{
	// regard as ansi
#ifdef _MSC_VER
	int bufferSize;
	mbstowcs_s((size_t*)&bufferSize, nullptr, 0, buffer, length);
#else
	int bufferSize = (int)std::mbstowcs(nullptr, buffer, 0);
#endif
	if (bufferSize > 0)
	{
		wchar_t * rbuffer = new wchar_t[bufferSize +1];
		int pos;
#ifdef _MSC_VER
		mbstowcs_s((size_t*)&pos, rbuffer, bufferSize, buffer, length);
#else
		pos = (int)std::mbstowcs(rbuffer, buffer, bufferSize + 1);
#endif
		if (pos <= (int)bufferSize && pos >= 0)
			rbuffer[pos] = 0;
		return rbuffer;
	}
	else
		return 0;
}

void MByteToWideChar(wchar_t * buffer, int bufferSize, const char * str, int length)
{
#ifdef _MSC_VER
	int pos;
	mbstowcs_s((size_t*)&pos, buffer, bufferSize, str, length);
#else
	std::mbstowcs(buffer, str, bufferSize);
#endif
}
