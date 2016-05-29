#ifndef WIDE_CHAR_H
#define WIDE_CHAR_H

void MByteToWideChar(wchar_t * buffer, int bufferSize, const char * str, int length);
char * WideCharToMByte(const wchar_t * buffer, int length);
wchar_t * MByteToWideChar(const char * buffer, int length);

#endif