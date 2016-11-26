#ifndef _MSC_VER
#ifndef CORE_LIB_SECURE_CRT_H
#define CORE_LIB_SECURE_CRT_H
#include <stdarg.h>
#include <stdlib.h>
#include <sstream>
#include <cstring>
#ifndef _ismbblead

inline bool _ismbblead(char c)
{
	return (0x80<= c) && (c < 0xa0 || 0xe0 <= c);
}

#endif

inline void memcpy_s(void *dest, size_t numberOfElements, const void * src, size_t count)
{
	memcpy(dest, src, count);
}

#define _TRUNCATE ((size_t)-1)
#define _stricmp strcasecmp

inline void fopen_s(FILE**f, const char * fileName, const char * mode)
{
	*f = fopen(fileName, mode);
}

inline void wcstombs_s(size_t * pReturnValue, char *mbstr, size_t sizeInWords, const wchar_t *wcstr, size_t count)
{
	if (count == _TRUNCATE)
		count = sizeInWords;
	*pReturnValue = wcstombs(mbstr, wcstr, count);
}

inline void mbstowcs_s(size_t * pReturnValue, wchar_t *wcstr, size_t sizeInWords, const char *mbstr, size_t count)
{
	if (count == _TRUNCATE)
		count = sizeInWords;
	*pReturnValue = mbstowcs(wcstr, mbstr, count);
}

inline size_t fread_s(void * buffer, size_t bufferSize, size_t elementSize, size_t count, FILE * stream)
{
	return fread(buffer, elementSize, count, stream);
}

inline int _itow_s(int value, wchar_t * buffer, size_t sizeInCharacters, int radix)
{
	std::wstringstream s;
	s<<value;
	auto str = s.str();
	memset(buffer, 0, sizeInCharacters * sizeof(wchar_t));
	memcpy(buffer, str.c_str(), str.length() * sizeof(wchar_t));
	return 0;
}

inline int _i64tow_s(long long value, wchar_t * buffer, size_t sizeInCharacters, int radix)
{
	std::wstringstream s;
	s<<value;
	auto str = s.str();
	memset(buffer, 0, sizeInCharacters * sizeof(wchar_t));
	memcpy(buffer, str.c_str(), str.length() * sizeof(wchar_t));
	return 0;
}

inline size_t wcsnlen_s(const wchar_t * str, size_t numberofElements)
{
	return wcsnlen(str, numberofElements);
}

inline size_t strnlen_s(const char * str, size_t numberofElements)
{
	return strnlen(str, numberofElements);
}

inline int sprintf_s(char * buffer, size_t sizeOfBuffer, const char * format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	int rs = sprintf(buffer, sizeOfBuffer, format, argptr);
	va_end(argptr);
	return rs;
}

inline int swprintf_s(wchar_t * buffer, size_t sizeOfBuffer, const wchar_t * format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	int rs = swprintf(buffer, sizeOfBuffer, format, argptr);
	va_end(argptr);
	return rs;
}

inline void wcscpy_s(wchar_t * strDestination, size_t /*numberOfElements*/, const wchar_t * strSource)
{
	wcscpy(strDestination, strSource);
}
inline void strcpy_s(char * strDestination, size_t /*numberOfElements*/, const char * strSource)
{
	strcpy(strDestination, strSource);
}

inline void wcsncpy_s(wchar_t * strDestination, size_t /*numberOfElements*/, const wchar_t * strSource, size_t count)
{
	wcsncpy(strDestination, strSource, count);
	//wcsncpy(strDestination, strSource, count);
}
inline void strncpy_s(char * strDestination, size_t /*numberOfElements*/, const char * strSource, size_t count)
{
	strncpy(strDestination, strSource, count);
	//wcsncpy(strDestination, strSource, count);
}
#endif
#endif
