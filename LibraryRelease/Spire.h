/***********************************************************************

Spire - The MIT License (MIT)
Copyright (c) 2016, Yong He

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.

***********************************************************************/

/***********************************************************************
WARNING: This is an automatically generated file.
***********************************************************************/

/***********************************************************************
CORELIB\COMMON.H
***********************************************************************/
#ifndef CORE_LIB_COMMON_H
#define CORE_LIB_COMMON_H

#include <cstdint>

#ifdef __GNUC__
#define CORE_LIB_ALIGN_16(x) x __attribute__((aligned(16)))
#else
#define CORE_LIB_ALIGN_16(x) __declspec(align(16)) x
#endif

#define VARIADIC_TEMPLATE

namespace CoreLib
{
	typedef int64_t Int64;
	typedef unsigned short Word;
#ifdef _M_X64
	typedef int64_t PtrInt;
#else
	typedef int PtrInt;
#endif
	namespace Basic
	{
		class Object
		{
		public:
			virtual ~Object()
			{}
		};

		template <typename T>
		inline T&& _Move(T & obj)
		{
			return static_cast<T&&>(obj);
		}

		template <typename T>
		inline void Swap(T & v0, T & v1)
		{
			T tmp = _Move(v0);
			v0 = _Move(v1);
			v1 = _Move(tmp);
		}
	}
}

#endif

/***********************************************************************
CORELIB\WIDECHAR.H
***********************************************************************/
#ifndef WIDE_CHAR_H
#define WIDE_CHAR_H

void MByteToWideChar(wchar_t * buffer, int bufferSize, const char * str, int length);
char * WideCharToMByte(const wchar_t * buffer, int length);
wchar_t * MByteToWideChar(const char * buffer, int length);

#endif

/***********************************************************************
CORELIB\SMARTPOINTER.H
***********************************************************************/
#ifndef FUNDAMENTAL_LIB_SMART_POINTER_H
#define FUNDAMENTAL_LIB_SMART_POINTER_H

namespace CoreLib
{
	namespace Basic
	{
		class RefPtrDefaultDestructor
		{
		public:
			template<typename T>
			void operator ()(T * ptr)
			{
				delete ptr;
			}
		};

		class RefPtrArrayDestructor
		{
		public:
			template<typename T>
			void operator() (T * ptr)
			{
				delete [] ptr;
			}
		};

		template<typename T, typename Destructor = RefPtrDefaultDestructor>
		class RefPtr
		{
			template<typename T1, typename Destructor1>
			friend class RefPtr;
		private:
			T * pointer;
			int * refCount;
			
		public:
			RefPtr()
			{
				pointer = 0;
				refCount = 0;
			}
			RefPtr(T * ptr)
				: pointer(0), refCount(0)
			{
				this->operator=(ptr);
			}
			template<typename T1>
			RefPtr(T1 * ptr)
				: pointer(0), refCount(0)
			{
				this->operator=(ptr);
			}
			RefPtr(const RefPtr<T, Destructor> & ptr)
				: pointer(0), refCount(0)
			{
				this->operator=(ptr);
			}
			RefPtr(RefPtr<T, Destructor> && str)
				: pointer(0), refCount(0)
			{
				this->operator=(static_cast<RefPtr<T, Destructor> &&>(str));
			}
			RefPtr<T,Destructor>& operator=(T * ptr)
			{
				Dereferance();

				pointer = ptr;
				if(ptr)
				{
					refCount = new int;
					(*refCount) = 1;
				}
				else
					refCount = 0;
				return *this;
			}
			template<typename T1>
			RefPtr<T,Destructor>& operator=(T1 * ptr)
			{
				Dereferance();

				pointer = dynamic_cast<T*>(ptr);
				if(ptr)
				{
					refCount = new int;
					(*refCount) = 1;
				}
				else
					refCount = 0;
				return *this;
			}
			RefPtr<T,Destructor>& operator=(const RefPtr<T, Destructor> & ptr)
			{
				if(ptr.pointer != pointer)
				{
					Dereferance();
					pointer = ptr.pointer;
					refCount = ptr.refCount;
					if (refCount)
						(*refCount)++;
				}
				return *this;
			}

			template<typename T1>
			RefPtr(const RefPtr<T1> & ptr)
				: pointer(0), refCount(0)
			{
				this->operator=(ptr);
			}
			template<typename T1>
			RefPtr<T,Destructor> & operator = (const RefPtr<T1, Destructor> & ptr)
			{
				if(ptr.pointer != pointer)
				{
					Dereferance();
					pointer = dynamic_cast<T*>(ptr.pointer);
					if (ptr.pointer && !pointer)
						throw L"RefPtr assignment: type cast failed.";
					refCount = ptr.refCount;
					(*refCount)++;
				}
				return *this;
			}
			bool operator == (const T * ptr) const
			{
				return pointer == ptr;
			}
			bool operator != (const T * ptr) const
			{
				return pointer != ptr;
			}
			bool operator == (const RefPtr<T, Destructor> & ptr) const
			{
				return pointer == ptr.pointer;
			}
			bool operator != (const RefPtr<T, Destructor> & ptr) const
			{
				return pointer != ptr.pointer;
			}

			T* operator +(int offset) const
			{
				return pointer+offset;
			}
			T& operator [](int idx) const
			{
				return *(pointer + idx);
			}
			RefPtr<T,Destructor>& operator=(RefPtr<T, Destructor> && ptr)
			{
				if(ptr.pointer != pointer)
				{
					Dereferance();
					pointer = ptr.pointer;
					refCount = ptr.refCount;
					ptr.pointer = 0;
					ptr.refCount = 0;
				}
				return *this;
			}
			T* Release()
			{
				if(pointer)
				{
					if((*refCount) > 1)
					{
						(*refCount)--;
					}
					else
					{
						delete refCount;
					}
				}
				auto rs = pointer;
				refCount = 0;
				pointer = 0;
				return rs;
			}
			~RefPtr()
			{
				Dereferance();
			}

			void Dereferance()
			{
				if(pointer)
				{
					if((*refCount) > 1)
					{
						(*refCount)--;
					}
					else
					{
						Destructor destructor;
						destructor(pointer);
						delete refCount;
					}
				}
			}
			T & operator *() const
			{
				return *pointer;
			}
			T * operator->() const
			{
				return pointer;
			}
			T * Ptr() const
			{
				return pointer;
			}
		public:
			explicit operator bool() const 
			{
				if (pointer)
					return true;
				else
					return false;
			}
		};
	}
}

#endif

/***********************************************************************
CORELIB\SECURECRT.H
***********************************************************************/
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
	memset(buffer, 0, sizeInCharacters);
	memcpy(buffer, str.c_str(), str.length());
	return 0;
}

inline int _i64tow_s(long long value, wchar_t * buffer, size_t sizeInCharacters, int radix)
{
	std::wstringstream s;
	s<<value;
	auto str = s.str();
	memset(buffer, 0, sizeInCharacters);
	memcpy(buffer, str.c_str(), str.length());
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

inline int swprintf_s(wchar_t * buffer, size_t sizeOfBuffer, const wchar_t * format, ...)
{
	va_list argptr;
	va_start(argptr, format);
	int rs = swprintf(buffer, sizeOfBuffer, format, argptr);
	va_end(argptr);
	return rs;
}

inline void wcscpy_s(wchar_t * strDestination, size_t numberOfElements, const wchar_t * strSource)
{
	wcscpy(strDestination, strSource);
}

inline void wcsncpy_s(wchar_t * strDestination, size_t numberOfElements, const wchar_t * strSource, size_t count)
{
	wcsncpy(strDestination, strSource, count);
	//wcsncpy(strDestination, strSource, count);
}

#endif
#endif

/***********************************************************************
CORELIB\LIBSTRING.H
***********************************************************************/
#ifndef FUNDAMENTAL_LIB_STRING_H
#define FUNDAMENTAL_LIB_STRING_H
#include <string.h>
#include <cstdlib>
#include <stdio.h>

namespace CoreLib
{
	namespace Basic
	{
		class _EndLine
		{};
		extern _EndLine EndLine;
		class String
		{
			friend class StringBuilder;
		private:
			RefPtr<wchar_t, RefPtrArrayDestructor> buffer;
			char * multiByteBuffer;
			int length;
			void Free()
			{
				if (buffer)
					buffer = 0;
				if (multiByteBuffer)
					delete [] multiByteBuffer;
				buffer = 0;
				multiByteBuffer = 0;
				length = 0;
			}
		public:
			static String FromBuffer(RefPtr<wchar_t, RefPtrArrayDestructor> buffer, int len)
			{
				String rs;
				rs.buffer = buffer;
				rs.length = len;
				return rs;
			}
			String()
				:buffer(0), multiByteBuffer(0), length(0)
			{
			}
			String(const wchar_t * str) :buffer(0), multiByteBuffer(0), length(0)
			{
				this->operator=(str);
			}
			String(const wchar_t ch)
				:buffer(0), multiByteBuffer(0), length(0)
			{
				wchar_t arr[] = {ch, 0};
				*this = String(arr);
			}
			const wchar_t * begin()
			{
				return buffer.Ptr();
			}
			const wchar_t * end()
			{
				return buffer.Ptr() + length;
			}
			String(int val, int radix = 10)
				:buffer(0), multiByteBuffer(0), length(0)
			{
				buffer = new wchar_t[33];
				_itow_s(val, buffer.Ptr(), 33, radix);
				length = (int)wcsnlen_s(buffer.Ptr(), 33);
			}
			String(long long val, int radix = 10)
				:buffer(0), multiByteBuffer(0), length(0)
			{
				buffer = new wchar_t[65];
				_i64tow_s(val, buffer.Ptr(), 65, radix);
				length = (int)wcsnlen_s(buffer.Ptr(), 65);
			}
			String(float val, const wchar_t * format = L"%g")
				:buffer(0), multiByteBuffer(0), length(0)
			{
				buffer = new wchar_t[128];
				swprintf_s(buffer.Ptr(), 128, format, val);
				length = (int)wcsnlen_s(buffer.Ptr(), 128);
			}
			String(double val, const wchar_t * format = L"%g")
				:buffer(0), multiByteBuffer(0), length(0)
			{
				buffer = new wchar_t[128];
				swprintf_s(buffer.Ptr(), 128, format, val);
				length = (int)wcsnlen_s(buffer.Ptr(), 128);
			}
			String(const char * str)
				:buffer(0), multiByteBuffer(0), length(0)
			{
				if (str)
				{
					buffer = MByteToWideChar(str, (int)strlen(str));
					if (buffer)
						length = (int)wcslen(buffer.Ptr());
					else
						length = 0;
				}
			}
			String(const String & str)
				:buffer(0), multiByteBuffer(0), length(0)
			{				
				this->operator=(str);
			}
			String(String&& other)
				:buffer(0), multiByteBuffer(0), length(0)
			{
				this->operator=(static_cast<String&&>(other));
			}
			~String()
			{
				Free();
			}
			String & operator=(const wchar_t * str)
			{
				Free();
				if (str)
				{
					length = (int)wcslen(str);
					buffer = new wchar_t[length + 1];
					wcscpy_s(buffer.Ptr(), length + 1, str);
				}
				return *this;
			}
			String & operator=(const String & str)
			{
				if (str.buffer == buffer)
					return *this;
				Free();
				if (str.buffer)
				{
					length = str.length;
					buffer = str.buffer;
					multiByteBuffer = 0;
				}
				return *this;
			}
			String & operator=(String&& other)
			{
				if (this != &other)
				{
					Free();
					buffer = _Move(other.buffer);
					length = other.length;
					multiByteBuffer = other.multiByteBuffer;
					other.buffer = 0;
					other.length = 0;
					other.multiByteBuffer = 0;
				}
				return *this;
			}
			wchar_t operator[](int id) const
			{
#if _DEBUG
				if (id < 0 || id >= length)
					throw "Operator[]: index out of range.";
#endif
				return buffer.Ptr()[id];
			}

			friend String StringConcat(const wchar_t * lhs, int leftLen, const wchar_t * rhs, int rightLen);
			friend String operator+(const wchar_t*op1, const String & op2);
			friend String operator+(const String & op1, const wchar_t * op2);
			friend String operator+(const String & op1, const String & op2);

			String TrimStart() const
			{
				if(!buffer)
					return *this;
				int startIndex = 0;
				while (startIndex < length && 
					(buffer[startIndex] == L' ' || buffer[startIndex] == L'\t' || buffer[startIndex] == L'\r' || buffer[startIndex] == L'\n'))
						startIndex++;
				return String(buffer + startIndex);
			}

			String TrimEnd() const
			{
				if(!buffer)
					return *this;

				int endIndex = length - 1;
				while (endIndex >= 0 &&
					(buffer[endIndex] == L' ' || buffer[endIndex] == L'\t' || buffer[endIndex] == L'\r' || buffer[endIndex] == L'\n'))
					endIndex--;
				String res;
				res.length = endIndex + 1;
				res.buffer = new wchar_t[endIndex + 2];
				wcsncpy_s(res.buffer.Ptr(), endIndex + 2, buffer.Ptr(), endIndex + 1);
				return res;
			}

			String Trim() const
			{
				if(!buffer)
					return *this;

				int startIndex = 0;
				while (startIndex < length && 
					(buffer[startIndex] == L' ' || buffer[startIndex] == L'\t'))
						startIndex++;
				int endIndex = length - 1;
				while (endIndex >= startIndex &&
					(buffer[endIndex] == L' ' || buffer[endIndex] == L'\t'))
					endIndex--;

				String res;
				res.length = endIndex - startIndex + 1;
				res.buffer = new wchar_t[res.length + 1];
				memcpy(res.buffer.Ptr(), buffer + startIndex, sizeof(wchar_t) * res.length);
				res.buffer[res.length] = L'\0';
				return res;
			}

			String SubString(int id, int len) const
			{
				if (len == 0)
					return L"";
				if (id + len > length)
					len = length - id;
#if _DEBUG
				if (id < 0 || id >= length || (id + len) > length)
					throw "SubString: index out of range.";
				if (len < 0)
					throw "SubString: length less than zero.";
#endif
				String res;
				res.buffer = new wchar_t[len + 1];
				res.length = len;
				wcsncpy_s(res.buffer.Ptr(), len + 1, buffer + id, len);
				res.buffer[len] = 0;
				return res;
			}

			wchar_t * Buffer() const
			{
				if (buffer)
					return buffer.Ptr();
				else
					return (wchar_t*)L"";
			}

			char * ToMultiByteString(int * len = 0) const
			{
				if (!buffer)
					return (char*)"";
				else
				{
					if (multiByteBuffer)
						return multiByteBuffer;
					((String*)this)->multiByteBuffer = WideCharToMByte(buffer.Ptr(), length);
					if (len)
						*len = (int)strnlen_s(multiByteBuffer, length*2);
					return multiByteBuffer;
					/*if (multiByteBuffer)
						return multiByteBuffer;
					size_t requiredBufferSize;
					requiredBufferSize = WideCharToMultiByte(CP_OEMCP, NULL, buffer.Ptr(), length, 0, 0, NULL, NULL)+1;
					if (len)
						*len = requiredBufferSize-1;
					if (requiredBufferSize)
					{
						multiByteBuffer = new char[requiredBufferSize];
						WideCharToMultiByte(CP_OEMCP, NULL, buffer.Ptr(), length, multiByteBuffer, requiredBufferSize, NULL, NULL);
						multiByteBuffer[requiredBufferSize-1] = 0;
						return multiByteBuffer;
					}
					else
						return "";*/
				}
			}

			bool Equals(const String & str, bool caseSensitive = true)
			{
				if (!buffer)
					return (str.buffer == 0);
				if (caseSensitive)
					return (wcscmp(buffer.Ptr(), str.buffer.Ptr()) == 0);
				else
				{
#ifdef _MSC_VER
					return (_wcsicmp(buffer.Ptr(), str.buffer.Ptr()) == 0);
#else
					return (wcscasecmp(buffer.Ptr(), str.buffer.Ptr()) == 0);
#endif
				}
			}

			bool operator==(const String & str) const
			{
				if (!buffer)
					return (str.buffer == 0 || wcscmp(str.buffer.Ptr(), L"")==0);
				if (!str.buffer)
					return buffer == nullptr || wcscmp(buffer.Ptr(), L"") == 0;
				return (wcscmp(buffer.Ptr(), str.buffer.Ptr()) == 0);
			}
			bool operator!=(const String & str) const
			{
				if (!buffer)
					return (str.buffer != 0 && wcscmp(str.buffer.Ptr(), L"") != 0);
				if (str.buffer.Ptr() == 0)
					return length != 0;
				return (wcscmp(buffer.Ptr(), str.buffer.Ptr()) != 0);
			}
			bool operator>(const String & str) const
			{
				if (!buffer)
					return false;
				if (!str.buffer)
					return buffer.Ptr() != nullptr && length != 0;
				return (wcscmp(buffer.Ptr(), str.buffer.Ptr()) > 0);
			}
			bool operator<(const String & str) const
			{
				if (!buffer)
					return (str.buffer != 0);
				if (!str.buffer)
					return false;
				return (wcscmp(buffer.Ptr(), str.buffer.Ptr()) < 0);
			}
			bool operator>=(const String & str) const
			{
				if (!buffer)
					return (str.buffer == 0);
				if (!str.buffer)
					return length == 0;
				int res = wcscmp(buffer.Ptr(), str.buffer.Ptr());
				return (res > 0 || res == 0);
			}
			bool operator<=(const String & str) const
			{
				if (!buffer)
					return true;
				if (!str.buffer)
					return length > 0;
				int res = wcscmp(buffer.Ptr(), str.buffer.Ptr());
				return (res < 0 || res == 0);
			}

			String ToUpper() const
			{
				if(!buffer)
					return *this;
				String res;
				res.length = length;
				res.buffer = new wchar_t[length + 1];
				for (int i = 0; i <= length; i++)
					res.buffer[i] = (buffer[i] >= L'a' && buffer[i] <= L'z')? 
									(buffer[i] - L'a' + L'A') : buffer[i];
				return res;
			}

			String ToLower() const
			{
				if(!buffer)
					return *this;
				String res;
				res.length = length;
				res.buffer = new wchar_t[length + 1];
				for (int i = 0; i <= length; i++)
					res.buffer[i] = (buffer[i] >= L'A' && buffer[i] <= L'Z')? 
									(buffer[i] - L'A' + L'a') : buffer[i];
				return res;
			}
			
			int Length() const
			{
				return length;
			}

			int IndexOf(wchar_t * str, int id) const // String str
			{
#if _DEBUG
				if (id < 0 || id >= length)
					throw "SubString: index out of range.";
#endif
				if(!buffer)
					return -1;
				auto findRs = wcsstr(buffer + id, str);
				int res = findRs ? (int)(findRs - buffer.Ptr()) : -1;
				if (res >= 0)
					return res;
				else
					 return -1;
			}
			
			int IndexOf(const String & str, int id) const
			{
				return IndexOf(str.buffer.Ptr(), id);
			}

			int IndexOf(wchar_t * str) const
			{
				return IndexOf(str, 0);
			}

			int IndexOf(const String & str) const
			{
				return IndexOf(str.buffer.Ptr(), 0);
			}

			int IndexOf(wchar_t ch, int id) const
			{
#if _DEBUG
				if (id < 0 || id >= length)
					throw "SubString: index out of range.";
#endif
				if(!buffer)
					return -1;
				for (int i = id; i < length; i++)
					if (buffer[i] == ch)
						return i;
				return -1;
			}

			int IndexOf(wchar_t ch) const
			{
				return IndexOf(ch, 0);
			}

			int LastIndexOf(wchar_t ch) const
			{
				for (int i = length-1; i>=0; i--)
					if (buffer[i] == ch)
						return i;
				return -1;
			}

			bool StartsWith(const wchar_t * str) const // String str
			{
				if(!buffer)
					return false;
				int strLen =(int) wcslen(str);
				if (strLen > length)
					return false;
				for (int i = 0; i < strLen; i++)
					if (str[i] != buffer[i])
						return false;
				return true;
			}

			bool StartsWith(const String & str) const
			{
				return StartsWith((const wchar_t*)str.buffer.Ptr());
			}

			bool EndsWith(wchar_t * str)  const // String str
			{
				if(!buffer)
					return false;
				int strLen = (int)wcslen(str);
				if (strLen > length)
					return false;
				for (int i = strLen - 1; i >= 0; i--)
					if (str[i] != buffer[length - strLen + i])
						return false;
				return true;
			}

			bool EndsWith(const String & str) const
			{
				return EndsWith(str.buffer.Ptr());
			}

			bool Contains(wchar_t * str) const // String str
			{
				if(!buffer)
					return false;
				return (IndexOf(str) >= 0)? true : false;
			}

			bool Contains(const String & str) const
			{
				return Contains(str.buffer.Ptr());
			}

			int GetHashCode() const
			{
				if (!buffer)
					return 0;
				int hash = 0;
				int c;
				wchar_t * str = buffer.Ptr();
				c = *str++;
				while (c)
				{
					hash = c + (hash << 6) + (hash << 16) - hash;
					c = *str++;
				}
				return hash;
			}
			String PadLeft(wchar_t ch, int length);
			String PadRight(wchar_t ch, int length);
			String MD5() const;
		};

		class StringBuilder
		{
		private:
			wchar_t * buffer;
			int length;
			int bufferSize;
			static const int InitialSize = 512;
		public:
			StringBuilder(int bufferSize = 1024)
				:buffer(0), length(0), bufferSize(0)
			{
				buffer = new wchar_t[InitialSize]; // new a larger buffer 
				buffer[0] = L'\0';
				length = 0;
				bufferSize = InitialSize;
			}
			~StringBuilder()
			{
				if(buffer)
					delete [] buffer;
			}
			void EnsureCapacity(int size)
			{
				if(bufferSize < size)
				{
					wchar_t * newBuffer = new wchar_t[size + 1];
					if(buffer)
					{
						wcscpy_s(newBuffer, size + 1, buffer);
						delete [] buffer;
					}
					buffer = newBuffer;
					bufferSize = size;
				}
			}

			//void Append(wchar_t * str)
			//{
			//	length += wcslen(str);
			//	if(bufferSize < length + 1)
			//	{
			//		int newBufferSize = InitialSize;
			//		while(newBufferSize < length + 1)
			//			newBufferSize <<= 1;
			//		wchar_t * newBuffer = new wchar_t[newBufferSize];
			//		if (buffer)
			//		{
			//			wcscpy_s(newBuffer, newBufferSize, buffer);
			//			delete [] buffer;
			//		}
			//		wcscat_s(newBuffer, newBufferSize, str); // use memcpy, manually deal with zero terminator
			//		buffer = newBuffer;
			//		bufferSize = newBufferSize;
			//	}
			//	else
			//	{
			//		wcscat_s(buffer, bufferSize, str); // use memcpy, manually deal with zero terminator
			//	}
			//}
			StringBuilder & operator << (const wchar_t * str)
			{
				Append(str, (int)wcslen(str));
				return *this;
			}
			StringBuilder & operator << (const String & str)
			{
				Append(str);
				return *this;
			}
			StringBuilder & operator << (const _EndLine)
			{
				Append(L'\n');
				return *this;
			}
			void Append(wchar_t ch)
			{
				Append(&ch, 1);
			}
			void Append(int value, int radix = 10)
			{
				wchar_t vBuffer[33];
				_itow_s(value, vBuffer, 33, radix);
				Append(vBuffer);
			}
			void Append(const String & str)
			{
				Append(str.Buffer(), str.Length());
			}
			void Append(const wchar_t * str)
			{
				Append(str, (int)wcslen(str));
			}
			void Append(const wchar_t * str, int strLen)
			{
				int newLength = length + strLen;
				if(bufferSize < newLength + 1)
				{
					int newBufferSize = InitialSize;
					while(newBufferSize < newLength + 1)
						newBufferSize <<= 1;
					wchar_t * newBuffer = new wchar_t[newBufferSize];
					if (buffer)
					{
						//wcscpy_s(newBuffer, newBufferSize, buffer);
						memcpy(newBuffer, buffer, sizeof(wchar_t) * length);
						delete [] buffer;
					}
					//wcscat_s(newBuffer, newBufferSize, str);
					memcpy(newBuffer + length, str, sizeof(wchar_t) * strLen);
					newBuffer[newLength] = L'\0';
					buffer = newBuffer;
					bufferSize = newBufferSize;
				}
				else
				{
					memcpy(buffer + length, str, sizeof(wchar_t) * strLen);
					buffer[newLength] = L'\0';
					//wcscat_s(buffer, bufferSize, str); // use memcpy, manually deal with zero terminator
				}
				length = newLength;
			}

			int Capacity()
			{
				return bufferSize;
			}

			wchar_t * Buffer()
			{
				return buffer;
			}

			int Length()
			{
				return length;
			}

			String ToString()
			{
				return String(buffer);
			}

			String ProduceString()
			{
				String rs;
				rs.buffer = buffer;
				rs.length = length;
				buffer = 0;
				length = 0;
				return rs;

			}

			String GetSubString(int start, int count)
			{
				String rs;
				rs.buffer = new wchar_t[count+1];
				rs.length = count;
				wcsncpy_s(rs.buffer.Ptr(), count+1, buffer+start, count);
				rs.buffer[count] = 0;
				return rs;
			}

			void Remove(int id, int len)
			{
#if _DEBUG
				if (id >= length || id < 0)
					throw "Remove: Index out of range.";
				if(len < 0)
					throw "Remove: remove length smaller than zero.";
#endif
				int actualDelLength = ((id + len) >= length)? (length - id) : len;
				for (int i = id + actualDelLength; i <= length; i++)
					buffer[i - actualDelLength] = buffer[i];
				length -= actualDelLength;
			}

			void Clear()
			{
				length = 0;
				if (buffer)
					buffer[0] = 0;
			}
		};

		int StringToInt(const String & str);
		double StringToDouble(const String & str);

		
	}
}

#endif

/***********************************************************************
CORELIB\EXCEPTION.H
***********************************************************************/
#ifndef CORE_LIB_EXCEPTION_H
#define CORE_LIB_EXCEPTION_H


namespace CoreLib
{
	namespace Basic
	{
		class Exception : public Object
		{
		public:
			String Message;
			Exception()
			{}
			Exception(const String & message)
				: Message(message)
			{
			}
		};

		class IndexOutofRangeException : public Exception
		{
		public:
			IndexOutofRangeException()
			{}
			IndexOutofRangeException(const String & message)
				: Exception(message)
			{
			}

		};

		class InvalidOperationException : public Exception
		{
		public:
			InvalidOperationException()
			{}
			InvalidOperationException(const String & message)
				: Exception(message)
			{
			}

		};
		
		class ArgumentException : public Exception
		{
		public:
			ArgumentException()
			{}
			ArgumentException(const String & message)
				: Exception(message)
			{
			}

		};

		class KeyNotFoundException : public Exception
		{
		public:
			KeyNotFoundException()
			{}
			KeyNotFoundException(const String & message)
				: Exception(message)
			{
			}
		};
		class KeyExistsException : public Exception
		{
		public:
			KeyExistsException()
			{}
			KeyExistsException(const String & message)
				: Exception(message)
			{
			}
		};

		class NotSupportedException : public Exception
		{
		public:
			NotSupportedException()
			{}
			NotSupportedException(const String & message)
				: Exception(message)
			{
			}
		};

		class NotImplementedException : public Exception
		{
		public:
			NotImplementedException()
			{}
			NotImplementedException(const String & message)
				: Exception(message)
			{
			}
		};

		class InvalidProgramException : public Exception
		{
		public:
			InvalidProgramException()
			{}
			InvalidProgramException(const String & message)
				: Exception(message)
			{
			}
		};
	}
}

#endif

/***********************************************************************
CORELIB\ARRAYVIEW.H
***********************************************************************/
#ifndef CORE_LIB_ARRAY_VIEW_H
#define CORE_LIB_ARRAY_VIEW_H


namespace CoreLib
{
	namespace Basic
	{
		template<typename T>
		class ArrayView
		{
		private:
			T * _buffer;
			int _count;
			int stride;
		public:
			T* begin() const
			{
				return _buffer;
			}
			T* end() const
			{
				return (T*)((char*)_buffer + _count*stride);
			}
		public:
			ArrayView()
			{
				_buffer = 0;
				_count = 0;
			}
			ArrayView(const T & singleObj)
			{
				SetData((T*)&singleObj, 1, sizeof(T));
			}
			ArrayView(T * buffer, int count)
			{
				SetData(buffer, count, sizeof(T));
			}
			ArrayView(void * buffer, int count, int _stride)
			{
				SetData(buffer, count, _stride);
			}
			void SetData(void * buffer, int count, int _stride)
			{
				this->_buffer = (T*)buffer;
				this->_count = count;
				this->stride = _stride;
			}
			inline int GetCapacity() const
			{
				return size;
			}
			inline int Count() const
			{
				return _count;
			}

			inline T & operator [](int id) const
			{
#if _DEBUG
				if (id >= _count || id < 0)
					throw IndexOutofRangeException(L"Operator[]: Index out of Range.");
#endif
				return *(T*)((char*)_buffer+id*stride);
			}

			inline T* Buffer() const
			{
				return _buffer;
			}

			template<typename T2>
			int IndexOf(const T2 & val) const
			{
				for (int i = 0; i < _count; i++)
				{
					if (*(T*)((char*)_buffer + id*stride) == val)
						return i;
				}
				return -1;
			}

			template<typename T2>
			int LastIndexOf(const T2 & val) const
			{
				for (int i = _count - 1; i >= 0; i--)
				{
					if (*(T*)((char*)_buffer + id*stride) == val)
						return i;
				}
				return -1;
			}

			template<typename Func>
			int FindFirst(const Func & predicate) const
			{
				for (int i = 0; i < _count; i++)
				{
					if (predicate(buffer[i]))
						return i;
				}
				return -1;
			}

			template<typename Func>
			int FindLast(const Func & predicate) const
			{
				for (int i = _count - 1; i >= 0; i--)
				{
					if (predicate(buffer[i]))
						return i;
				}
				return -1;
			}
		};

		template<typename T>
		ArrayView<T> MakeArrayView(const T & obj)
		{
			return ArrayView<T>(obj);
		}
		template<typename T>
		ArrayView<T> MakeArrayView(T * buffer, int count)
		{
			return ArrayView<T>(buffer, count);
		}
	}
}
#endif

/***********************************************************************
CORELIB\LIBMATH.H
***********************************************************************/
#ifndef CORE_LIB_MATH_H
#define CORE_LIB_MATH_H

#include <math.h>

namespace CoreLib
{
	namespace Basic
	{
		class Math
		{
		public:
			static const float Pi;
			template<typename T>
			static T Min(const T& v1, const T&v2)
			{
				return v1<v2?v1:v2;
			}
			template<typename T>
			static T Max(const T& v1, const T&v2)
			{
				return v1>v2?v1:v2;
			}
			template<typename T>
			static T Min(const T& v1, const T&v2, const T&v3)
			{
				return Min(v1, Min(v2, v3));
			}
			template<typename T>
			static T Max(const T& v1, const T&v2, const T&v3)
			{
				return Max(v1, Max(v2, v3));
			}
			template<typename T>
			static T Clamp(const T& val, const T& vmin, const T&vmax)
			{
				if (val < vmin) return vmin;
				else if (val > vmax) return vmax;
				else return val;
			}

			static inline int FastFloor(float x)
			{
				int i = (int)x;
				return i - (i > x);
			}

			static inline int FastFloor(double x)
			{
				int i = (int)x;
				return i - (i > x);
			}

			static inline int IsNaN(float x)
			{
#ifdef _M_X64
				return _isnanf(x);
#else
				return isnan(x);
#endif
			}

			static inline int IsInf(float x)
			{
				return isinf(x);
			}

			static inline unsigned int Ones32(register unsigned int x)
			{
				/* 32-bit recursive reduction using SWAR...
					but first step is mapping 2-bit values
					into sum of 2 1-bit values in sneaky way
				*/
				x -= ((x >> 1) & 0x55555555);
				x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
				x = (((x >> 4) + x) & 0x0f0f0f0f);
				x += (x >> 8);
				x += (x >> 16);
				return(x & 0x0000003f);
			}

			static inline unsigned int Log2Floor(register unsigned int x)
			{
				x |= (x >> 1);
				x |= (x >> 2);
				x |= (x >> 4);
				x |= (x >> 8);
				x |= (x >> 16);
				return(Ones32(x >> 1));
			}

			static inline unsigned int Log2Ceil(register unsigned int x)
			{
				register int y = (x & (x - 1));
				y |= -y;
				y >>= (32 - 1);
				x |= (x >> 1);
				x |= (x >> 2);
				x |= (x >> 4);
				x |= (x >> 8);
				x |= (x >> 16);
				return(Ones32(x >> 1) - y);
			}
			/*
			static inline int Log2(float x)
			{
				unsigned int ix = (unsigned int&)x;
				unsigned int exp = (ix >> 23) & 0xFF;
				int log2 = (unsigned int)(exp) - 127;

				return log2;
			}
			*/
		};
		inline int FloatAsInt(float val)
		{
			union InterCast
			{
				float fvalue;
				int ivalue;
			} cast;
			cast.fvalue = val;
			return cast.ivalue;
		}
		inline float IntAsFloat(int val)
		{
			union InterCast
			{
				float fvalue;
				int ivalue;
			} cast;
			cast.ivalue = val;
			return cast.fvalue;
		}

		inline unsigned short FloatToHalf(float val)
		{
			int x = *(int*)&val;
			unsigned short bits = (x >> 16) & 0x8000;
			unsigned short m = (x >> 12) & 0x07ff;
			unsigned int e = (x >> 23) & 0xff;
			if (e < 103)
				return bits;
			if (e > 142)
			{
				bits |= 0x7c00u;
				bits |= e == 255 && (x & 0x007fffffu);
				return bits;
			}
			if (e < 113)
			{
				m |= 0x0800u;
				bits |= (m >> (114 - e)) + ((m >> (113 - e)) & 1);
				return bits;
			}
			bits |= ((e - 112) << 10) | (m >> 1);
			bits += m & 1;
			return bits;
		}

		inline float HalfToFloat(unsigned short input)
		{
			union InterCast
			{
				float fvalue;
				int ivalue;
				InterCast() = default;
				InterCast(int ival)
				{
					ivalue = ival;
				}
			};
			static const InterCast magic = InterCast((127 + (127 - 15)) << 23);
			static const InterCast was_infnan = InterCast((127 + 16) << 23);
			InterCast o;
			o.ivalue = (input & 0x7fff) << 13;     // exponent/mantissa bits
			o.fvalue *= magic.fvalue;                 // exponent adjust
			if (o.fvalue >= was_infnan.fvalue)        // make sure Inf/NaN survive
				o.ivalue |= 255 << 23;
			o.ivalue |= (input & 0x8000) << 16;    // sign bit
			return o.fvalue;
		}

		class Random
		{
		private:
			unsigned int seed;
		public:
			Random(int seed)
			{
				this->seed = seed;
			}
			int Next() // random between 0 and RandMax (currently 0x7fff)
			{
				return (((seed = seed * 214013L + 2531011L) >> 16) & 0x7fff);
			}
			int Next(int min, int max) // inclusive min, exclusive max
			{
				unsigned int a = ((seed = seed * 214013L + 2531011L) & 0xFFFF0000);
				unsigned int b = ((seed = seed * 214013L + 2531011L) >> 16);
				unsigned int r = a + b;
				return min + r % (max - min);
			}
			float NextFloat()
			{
				return ((Next() << 15) + Next()) / ((float)(1 << 30));
			}
			float NextFloat(float valMin, float valMax)
			{
				return valMin + (valMax - valMin) * NextFloat();
			}
			static int RandMax()
			{
				return 0x7fff;
			}
		};
	}
}

#endif 

/***********************************************************************
CORELIB\ARRAY.H
***********************************************************************/
#ifndef CORE_LIB_ARRAY_H
#define CORE_LIB_ARRAY_H


namespace CoreLib
{
	namespace Basic
	{
		template<typename T, int size>
		class Array
		{
		private:
			T _buffer[size];
			int _count;
		public:
			T* begin() const
			{
				return (T*)_buffer;
			}
			T* end() const
			{
				return (T*)_buffer+_count;
			}
		public:
			Array()
			{
				_count = 0;
			}
			inline int GetCapacity() const
			{
				return size;
			}
			inline int Count() const
			{
				return _count;
			}
			inline void SetSize(int newSize)
			{
#ifdef _DEBUG
				if (newSize > size)
					throw IndexOutofRangeException(L"size too large.");
#endif
				_count = newSize;
			}
			inline void Add(const T & item)
			{
#ifdef _DEBUG
				if (_count == size)
					throw IndexOutofRangeException(L"out of range access to static array.");
#endif
				_buffer[_count++] = item;
			}
			inline void Add(T && item)
			{
#ifdef _DEBUG
				if (_count == size)
					throw IndexOutofRangeException(L"out of range access to static array.");
#endif
				_buffer[_count++] = _Move(item);
			}

			inline T & operator [](int id) const
			{
#if _DEBUG
				if(id >= _count || id < 0)
					throw IndexOutofRangeException(L"Operator[]: Index out of Range.");
#endif
				return ((T*)_buffer)[id];
			}

			inline T* Buffer() const
			{
				return (T*)_buffer;
			}

			inline void Clear()
			{
				_count = 0;
			}

			template<typename T2>
			int IndexOf(const T2 & val) const
			{
				for (int i = 0; i < _count; i++)
				{
					if (_buffer[i] == val)
						return i;
				}
				return -1;
			}

			template<typename T2>
			int LastIndexOf(const T2 & val) const
			{
				for (int i = _count - 1; i >= 0; i--)
				{
					if(_buffer[i] == val)
						return i;
				}
				return -1;
			}

			inline ArrayView<T> GetArrayView() const
			{
				return ArrayView<T>((T*)_buffer, _count);
			}
			inline ArrayView<T> GetArrayView(int start, int count) const
			{
				return ArrayView<T>((T*)_buffer + start, count);
			}
		};
	}
}

#endif

/***********************************************************************
CORELIB\ALLOCATOR.H
***********************************************************************/
#ifndef CORE_LIB_ALLOCATOR_H
#define CORE_LIB_ALLOCATOR_H


namespace CoreLib
{
	namespace Basic
	{
		inline void * AlignedAlloc(size_t size, size_t alignment)
		{
#ifdef _MSC_VER
			return _aligned_malloc(size, alignment);
#else
			void * rs = 0;
			int succ = posix_memalign(&rs, alignment, size);
			if (succ!=0)
				rs = 0;
			return rs;
#endif
		}

		inline void AlignedFree(void * ptr)
		{
#ifdef _MSC_VER
			_aligned_free(ptr);
#else
			free(ptr);
#endif
		}

		class StandardAllocator
		{
		public:
			// not really called
			void * Alloc(size_t size)
			{
				return malloc(size);
			}
			void Free(void * ptr)
			{
				return free(ptr);
			}
		};

		template<int alignment>
		class AlignedAllocator
		{
		public:
			void * Alloc(size_t size)
			{
				return AlignedAlloc(size, alignment);
			}
			void Free(void * ptr)
			{
				return AlignedFree(ptr);
			}
		};
	}
}

#endif

/***********************************************************************
CORELIB\LIST.H
***********************************************************************/
#ifndef FUNDAMENTAL_LIB_LIST_H
#define FUNDAMENTAL_LIB_LIST_H

#include <type_traits>
#include <new>
#include <algorithm>

const int MIN_QSORT_SIZE = 32;

namespace CoreLib
{
	namespace Basic
	{
		template<typename T, int isPOD>
		class Initializer
		{

		};

		template<typename T>
		class Initializer<T, 0>
		{
		public:
			static void Initialize(T * buffer, int size)
			{
				for (int i = 0; i<size; i++)
					new (buffer + i) T();
			}
		};

		template<typename T, typename TAllocator>
		class AllocateMethod
		{
		public:
			static inline T* Alloc(int size)
			{
				TAllocator allocator;
				T * rs = (T*)allocator.Alloc(size*sizeof(T));
				Initializer<T, std::is_pod<T>::value>::Initialize(rs, size);
				return rs;
			}
			static inline void Free(T * ptr, int bufferSize)
			{
				TAllocator allocator;
				if (!std::has_trivial_destructor<T>::value)
				{
					for (int i = 0; i<bufferSize; i++)
						ptr[i].~T();
				}
				allocator.Free(ptr);
			}
		};

		template<typename T>
		class AllocateMethod<T, StandardAllocator>
		{
		public:
			static inline T* Alloc(int size)
			{
				return new T[size];
			}
			static inline void Free(T* ptr, int /*bufferSize*/)
			{
				delete [] ptr;
			}
		};

		template<typename T>
		class Initializer<T, 1>
		{
		public:
			static void Initialize(T * buffer, int size)
			{
				for (int i = 0; i<size; i++)
					new (buffer + i) T;
			}
		};

		template<typename T, typename TAllocator = StandardAllocator>
		class List
		{
		private:

			inline T * Allocate(int size)
			{
				return AllocateMethod<T, TAllocator>::Alloc(size);
				
			}
		private:
			static const int InitialSize = 16;
			TAllocator allocator;
		private:
			T * buffer;
			int _count;
			int bufferSize;
			void FreeBuffer()
			{
				AllocateMethod<T, TAllocator>::Free(buffer, bufferSize);
				buffer = 0;
			}
			void Free()
			{
				if (buffer)
				{
					FreeBuffer();
				}
				buffer = 0;
				_count = bufferSize = 0;
			}
		public:
			T* begin() const
			{
				return buffer;
			}
			T* end() const
			{
				return buffer+_count;
			}
		public:
			List()
				: buffer(0), _count(0), bufferSize(0)
			{
			}
			List(const List<T> & list)
				: buffer(0), _count(0), bufferSize(0)
			{
				this->operator=(list);
			}
			List(List<T> && list)
				: buffer(0), _count(0), bufferSize(0)
			{
				//int t = static_cast<int>(1.0f); reinterpret_cast<double*>(&t), dynamic_cast<> 
				this->operator=(static_cast<List<T>&&>(list));
			}
			~List()
			{
				Free();
			}
			List<T> & operator=(const List<T> & list)
			{
				Free();
				AddRange(list);

				return *this;
			}

			List<T> & operator=(List<T> && list)
			{
				Free();
				_count = list._count;
				bufferSize = list.bufferSize;
				buffer = list.buffer;

				list.buffer = 0;
				list._count = 0;
				list.bufferSize = 0;
				return *this;
			}

			T & First() const
			{
#ifdef _DEBUG
				if (_count == 0)
					throw "Index out of range.";
#endif
				return buffer[0];
			}

			T & Last() const
			{
#ifdef _DEBUG
				if (_count == 0)
					throw "Index out of range.";
#endif
				return buffer[_count-1];
			}

			inline void SwapWith(List<T, TAllocator> & other)
			{
				T* tmpBuffer = this->buffer;
				this->buffer = other.buffer;
				other.buffer = tmpBuffer;
				int tmpBufferSize = this->bufferSize;
				this->bufferSize = other.bufferSize;
				other.bufferSize = tmpBufferSize;
				int tmpCount = this->_count;
				this->_count = other._count;
				other._count = tmpCount;
				TAllocator tmpAlloc = _Move(this->allocator);
				this->allocator = _Move(other.allocator);
				other.allocator = _Move(tmpAlloc);
			}

			inline ArrayView<T> GetArrayView() const
			{
				return ArrayView<T>(buffer, _count);
			}

			inline ArrayView<T> GetArrayView(int start, int count) const
			{
#ifdef _DEBUG
				if (start + count > _count || start < 0 || count < 0)
					throw "Index out of range.";
#endif
				return ArrayView<T>(buffer + start, count);
			}

			void Add(T && obj)
			{
				if (bufferSize < _count + 1)
				{
					int newBufferSize = InitialSize;
					if (bufferSize)
						newBufferSize = (bufferSize << 1);

					Reserve(newBufferSize);
				}
				buffer[_count++] = static_cast<T&&>(obj);
			}

			void Add(const T & obj)
			{
				if (bufferSize < _count + 1)
				{
					int newBufferSize = InitialSize;
					if (bufferSize)
						newBufferSize = (bufferSize << 1);

					Reserve(newBufferSize);
				}
				buffer[_count++] = obj;

			}

			int Count() const
			{
				return _count;
			}

			T * Buffer() const
			{
				return buffer;
			}

			int Capacity() const
			{
				return bufferSize;
			}

			void Insert(int id, const T & val)
			{
				InsertRange(id, &val, 1);
			}

			void InsertRange(int id, const T * vals, int n)
			{
				if (bufferSize < _count + n)
				{
					int newBufferSize = InitialSize;
					while (newBufferSize < _count + n)
						newBufferSize = newBufferSize << 1;

					T * newBuffer = Allocate(newBufferSize);
					if (bufferSize)
					{
						/*if (std::has_trivial_copy_assign<T>::value && std::has_trivial_destructor<T>::value)
						{
							memcpy(newBuffer, buffer, sizeof(T) * id);
							memcpy(newBuffer + id + n, buffer + id, sizeof(T) * (_count - id));
						}
						else*/
						{
							for (int i = 0; i < id; i++)
								newBuffer[i] = buffer[i];
							for (int i = id; i < _count; i++)
								newBuffer[i + n] = T(static_cast<T&&>(buffer[i]));
						}
						FreeBuffer();
					}
					buffer = newBuffer;
					bufferSize = newBufferSize;
				}
				else
				{
					/*if (std::has_trivial_copy_assign<T>::value && std::has_trivial_destructor<T>::value)
						memmove(buffer + id + n, buffer + id, sizeof(T) * (_count - id));
					else*/
					{
						for (int i = _count - 1; i >= id; i--)
							buffer[i + n] = static_cast<T&&>(buffer[i]);
					}
				}
				/*if (std::has_trivial_copy_assign<T>::value && std::has_trivial_destructor<T>::value)
					memcpy(buffer + id, vals, sizeof(T) * n);
				else*/
					for (int i = 0; i < n; i++)
						buffer[id + i] = vals[i];

				_count += n;
			}

			//slower than original edition
			//void Add(const T & val)
			//{
			//	InsertRange(_count, &val, 1);
			//}

			void InsertRange(int id, const List<T> & list)
			{
				InsertRange(id, list.buffer, list._count);
			}

			void AddRange(const T * vals, int n)
			{
				InsertRange(_count, vals, n);
			}

			void AddRange(const List<T> & list)
			{
				InsertRange(_count, list.buffer, list._count);
			}

			void RemoveRange(int id, int deleteCount)
			{
#if _DEBUG
				if (id >= _count || id < 0)
					throw "Remove: Index out of range.";
				if(deleteCount < 0)
					throw "Remove: deleteCount smaller than zero.";
#endif
				int actualDeleteCount = ((id + deleteCount) >= _count)? (_count - id) : deleteCount;
				for (int i = id + actualDeleteCount; i < _count; i++)
					buffer[i - actualDeleteCount] = static_cast<T&&>(buffer[i]);
				_count -= actualDeleteCount;
			}

			void RemoveAt(int id)
			{
				RemoveRange(id, 1);
			}

			void Remove(const T & val)
			{
				int idx = IndexOf(val);
				if (idx != -1)
					RemoveAt(idx);
			}

			void Reverse()
			{
				for (int i = 0; i < (_count >> 1); i++)
				{
					Swap(buffer, i, _count - i - 1);
				}
			}

			void FastRemove(const T & val)
			{
				int idx = IndexOf(val);
				if (idx != -1 && _count-1 != idx)
				{
					buffer[idx] = _Move(buffer[_count-1]);
				}
				_count--;
			}

			void Clear()
			{
				_count = 0;
			}

			void Reserve(int size)
			{
				if(size > bufferSize)
				{
					T * newBuffer = Allocate(size);
					if (bufferSize)
					{
						/*if (std::has_trivial_copy_assign<T>::value && std::has_trivial_destructor<T>::value)
							memcpy(newBuffer, buffer, _count * sizeof(T));
						else*/
						{
							for (int i = 0; i < _count; i++)
								newBuffer[i] = static_cast<T&&>(buffer[i]);
						}
						FreeBuffer();
					}
					buffer = newBuffer;
					bufferSize = size;
				}
			}

			void GrowToSize(int size)
			{
				int newBufferSize = 1<<Math::Log2Ceil(size);
				if (bufferSize < newBufferSize)
				{
					Reserve(newBufferSize);
				}
				this->_count = size;
			}

			void SetSize(int size)
			{
				Reserve(size);
				_count = size;
			}

			void UnsafeShrinkToSize(int size)
			{
				_count = size;
			}

			void Compress()
			{
				if (bufferSize > _count && _count > 0)
				{
					T * newBuffer = Allocate(_count);
					for (int i = 0; i < _count; i++)
						newBuffer[i] = static_cast<T&&>(buffer[i]);
					FreeBuffer();
					buffer = newBuffer;
					bufferSize = _count;
				}
			}

#ifndef FORCE_INLINE
#ifdef _MSC_VER
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE inline
#endif
#endif

			FORCE_INLINE T & operator [](int id) const
			{
#if _DEBUG
				if(id >= _count || id < 0)
					throw IndexOutofRangeException(L"Operator[]: Index out of Range.");
#endif
				return buffer[id];
			}

			template<typename Func>
			int FindFirst(const Func & predicate) const
			{
				for (int i = 0; i < _count; i++)
				{
					if (predicate(buffer[i]))
						return i;
				}
				return -1;
			}

			template<typename Func>
			int FindLast(const Func & predicate) const
			{
				for (int i = _count - 1; i >= 0; i--)
				{
					if (predicate(buffer[i]))
						return i;
				}
				return -1;
			}

			template<typename T2>
			int IndexOf(const T2 & val) const
			{
				for (int i = 0; i < _count; i++)
				{
					if (buffer[i] == val)
						return i;
				}
				return -1;
			}

			template<typename T2>
			int LastIndexOf(const T2 & val) const
			{
				for (int i = _count - 1; i >= 0; i--)
				{
					if(buffer[i] == val)
						return i;
				}
				return -1;
			}

			void Sort()
			{
				Sort([](T& t1, T& t2){return t1<t2;});
			}

			bool Contains(const T & val)
			{
				for (int i = 0; i<_count; i++)
					if (buffer[i] == val)
						return true;
				return false;
			}

			template<typename Comparer>
			void Sort(Comparer compare)
			{
				//InsertionSort(buffer, 0, _count - 1);
				//QuickSort(buffer, 0, _count - 1, compare);
				std::sort(buffer, buffer + _count, compare);
			}

			template <typename IterateFunc>
			void ForEach(IterateFunc f) const
			{
				for (int i = 0; i<_count; i++)
					f(buffer[i]);
			}

			template<typename Comparer>
			void QuickSort(T * vals, int startIndex, int endIndex, Comparer comparer)
			{
				if(startIndex < endIndex)
				{
					if (endIndex - startIndex < MIN_QSORT_SIZE)
						InsertionSort(vals, startIndex, endIndex, comparer);
					else
					{
						int pivotIndex = (startIndex + endIndex) >> 1;
						int pivotNewIndex = Partition(vals, startIndex, endIndex, pivotIndex, comparer);
						QuickSort(vals, startIndex, pivotNewIndex - 1, comparer);
						QuickSort(vals, pivotNewIndex + 1, endIndex, comparer);
					}
				}

			}
			template<typename Comparer>
			int Partition(T * vals, int left, int right, int pivotIndex, Comparer comparer)
			{
				T pivotValue = vals[pivotIndex];
				Swap(vals, right, pivotIndex);
				int storeIndex = left;
				for (int i = left; i < right; i++)
				{
					if (comparer(vals[i], pivotValue))
					{
						Swap(vals, i, storeIndex);
						storeIndex++;
					}
				}
				Swap(vals, storeIndex, right);
				return storeIndex;
			}
			template<typename Comparer>
			void InsertionSort(T * vals, int startIndex, int endIndex, Comparer comparer)
			{
				for (int i = startIndex  + 1; i <= endIndex; i++)
				{
					T insertValue = static_cast<T&&>(vals[i]);
					int insertIndex = i - 1;
					while (insertIndex >= startIndex && comparer(insertValue, vals[insertIndex]))
					{
						vals[insertIndex + 1] = static_cast<T&&>(vals[insertIndex]);
						insertIndex--;
					}
					vals[insertIndex + 1] = static_cast<T&&>(insertValue);
				}
			}

			inline void Swap(T * vals, int index1, int index2)
			{
				if (index1 != index2)
				{
					T tmp = static_cast<T&&>(vals[index1]);
					vals[index1] = static_cast<T&&>(vals[index2]);
					vals[index2] = static_cast<T&&>(tmp);
				}
			}

			template<typename T2, typename Comparer>
			int BinarySearch(const T2 & obj, Comparer comparer)
			{
				int imin = 0, imax = _count - 1;
				while (imax >= imin)
				{
					int imid = (imin + imax) >> 1;
					int compareResult = comparer(buffer[imid], obj);
					if (compareResult == 0)
						return imid;
					else if (compareResult < 0)
						imin = imid + 1;
					else
						imax = imid - 1;
				}
				return -1;
			}

			template<typename T2>
			int BinarySearch(const T2 & obj)
			{
				return BinarySearch(obj, 
					[](T & curObj, const T2 & thatObj)->int
					{
						if (curObj < thatObj)
							return -1;
						else if (curObj == thatObj)
							return 0;
						else
							return 1;
					});
			}
		};

		template<typename T>
		T Min(const List<T> & list)
		{
			T minVal = list.First();
			for (int i = 1; i<list.Count(); i++)
				if (list[i] < minVal)
					minVal = list[i];
			return minVal;
		}

		template<typename T>
		T Max(const List<T> & list)
		{
			T maxVal = list.First();
			for (int i = 1; i<list.Count(); i++)
				if (list[i] > maxVal)
					maxVal = list[i];
			return maxVal;
		}
	}
}

#endif

/***********************************************************************
CORELIB\LINK.H
***********************************************************************/
#ifndef CORE_LIB_LINK_H
#define CORE_LIB_LINK_H


namespace CoreLib
{
	namespace Basic
	{
		template<typename T>
		class LinkedList;

		template<typename T>
		class LinkedNode
		{
			template<typename T1>
			friend class LinkedList;
		private:
			LinkedNode<T> *pPrev, *pNext;
			LinkedList<T> * FLink;
		public:
			T Value;
			LinkedNode (LinkedList<T> * lnk):FLink(lnk)
			{
				pPrev = pNext = 0;
			};
			LinkedNode<T> * GetPrevious()
			{
				return pPrev;
			};
			LinkedNode<T> * GetNext()
			{
				return pNext;
			};
			LinkedNode<T> * InsertAfter(const T & nData)
			{
				LinkedNode<T> * n = new LinkedNode<T>(FLink);
				n->Value = nData;
				n->pPrev = this;
				n->pNext = this->pNext;
				LinkedNode<T> *npp = n->pNext;
				if (npp)
				{
					npp->pPrev = n;
				}
				pNext = n;
				if (!n->pNext)
					FLink->FTail = n;
				FLink->FCount ++;
				return n;
			};
			LinkedNode<T> * InsertBefore(const T & nData)
			{
				LinkedNode<T> * n = new LinkedNode<T>(FLink);
				n->Value = nData;
				n->pPrev = pPrev;
				n->pNext = this;
				pPrev = n;
				LinkedNode<T> *npp = n->pPrev;
				if (npp)
					npp->pNext = n;
				if (!n->pPrev)
					FLink->FHead = n;
				FLink->FCount ++;
				return n;
			};
			void Delete()
			{
				if (pPrev)
					pPrev->pNext = pNext;
				if (pNext)
					pNext->pPrev = pPrev;
				FLink->FCount --;
				if (FLink->FHead == this)
				{
					FLink->FHead = pNext;
				}
				if (FLink->FTail == this)
				{
					FLink->FTail = pPrev;
				}
				delete this;
			}
		};
		template<typename T>
		class LinkedList
		{
			template<typename T1>
			friend class LinkedNode;
		private:
			LinkedNode<T> * FHead, *FTail;
			int FCount;
		public:
			class Iterator
			{
			public:
				LinkedNode<T> * Current, *Next;
				void SetCurrent(LinkedNode<T> * cur)
				{
					Current = cur;
					if (Current)
						Next = Current->GetNext();
					else
						Next = 0;
				}
				Iterator()
				{
					Current = Next = 0;
				}
				Iterator(LinkedNode<T> * cur)
				{
					SetCurrent(cur);
				}
				T & operator *() const
				{
					return Current->Value;
				}
				Iterator& operator ++()
				{
					SetCurrent(Next);
					return *this;
				}
				Iterator operator ++(int)
				{
					Iterator rs = *this;
					SetCurrent(Next);
					return rs;
				}
				bool operator != (const Iterator & iter) const
				{
					return Current != iter.Current;
				}
				bool operator == (const Iterator & iter) const
				{
					return Current == iter.Current;
				}
			};
			Iterator begin() const
			{
				return Iterator(FHead);
			}
			Iterator end() const
			{
				return Iterator(0);
			}
		public:
			LinkedList() : FHead(0), FTail(0), FCount(0)
			{
			}
			~LinkedList()
			{
				Clear();
			}
			LinkedList(const LinkedList<T> & link) : FHead(0), FTail(0), FCount(0)
			{
				this->operator=(link);
			}
			LinkedList(LinkedList<T> && link) : FHead(0), FTail(0), FCount(0)
			{
				this->operator=(_Move(link));
			}
			LinkedList<T> & operator = (LinkedList<T> && link)
			{
				if (FHead != 0)
					Clear();
				FHead = link.FHead;
				FTail = link.FTail;
				FCount = link.FCount;
				link.FHead = 0;
				link.FTail = 0;
				link.FCount = 0;
				for (auto node = FHead; node; node = node->GetNext())
					node->FLink = this;
				return *this;
			}
			LinkedList<T> & operator = (const LinkedList<T> & link)
			{
				if (FHead != 0)
					Clear();
				auto p = link.FHead;
				while (p)
				{
					AddLast(p->Value);
					p = p->GetNext();
				}
				return *this;
			}
			template<typename IteratorFunc>
			void ForEach(const IteratorFunc & f)
			{
				auto p = FHead;
				while (p)
				{
					f(p->Value);
					p = p->GetNext();
				}
			}
			LinkedNode<T> * GetNode(int x)
			{
				LinkedNode<T> *pCur = FHead;
				for (int i=0;i<x;i++)
				{
					if (pCur)
						pCur = pCur->pNext;
					else
						throw "Index out of range";
				}
				return pCur;
			};
			LinkedNode<T> * Find(const T& fData)
			{
				for (LinkedNode<T> * pCur = FHead; pCur; pCur = pCur->pNext)
				{
					if (pCur->Value == fData)
						return pCur;
				}
				return 0;
			};
			LinkedNode<T> * FirstNode()
			{
				return FHead;
			};
			T & First()
			{
				if (!FHead)
					throw IndexOutofRangeException("LinkedList: index out of range.");
				return FHead->Value;
			}
			T & Last()
			{
				if (!FTail)
					throw IndexOutofRangeException("LinkedList: index out of range.");
				return FTail->Value;
			}
			LinkedNode<T> * LastNode()
			{
				return FTail;
			};
			LinkedNode<T> * AddLast(const T & nData)
			{
				LinkedNode<T> * n = new LinkedNode<T>(this);
				n->Value = nData;
				n->pPrev = FTail;
				if (FTail)
					FTail->pNext = n;
				n->pNext = 0;
				FTail = n;
				if (!FHead)
					FHead = n;
				FCount ++;
				return n;
			};
			// Insert a blank node
			LinkedNode<T> * AddLast()
			{
				LinkedNode<T> * n = new LinkedNode<T>(this);
				n->pPrev = FTail;
				if (FTail)
					FTail->pNext = n;
				n->pNext = 0;
				FTail = n;
				if (!FHead)
					FHead = n;
				FCount ++;
				return n;
			};
			LinkedNode<T> * AddFirst(const T& nData)
			{
				LinkedNode<T> *n = new LinkedNode<T>(this);
				n->Value = nData;
				n->pPrev = 0;
				n->pNext = FHead;
				if (FHead)
					FHead->pPrev = n;
				FHead = n;
				if (!FTail)
					FTail = n;
				FCount ++;
				return n;
			};
			void Delete(LinkedNode<T>*n, int Count = 1)
			{
				LinkedNode<T> *n1,*n2 = 0, *tn;
				n1 = n->pPrev;
				tn = n;
				int numDeleted = 0;
				for (int i=0; i<Count; i++)
				{
					n2 = tn->pNext;
					delete tn;
					tn = n2;
					numDeleted++;
					if (tn == 0)
						break;
				}
				if (n1)
					n1->pNext = n2;
				else
					FHead = n2;
				if (n2)
					n2->pPrev = n1;
				else
					FTail = n1;
				FCount -= numDeleted;
			}
			void Clear()
			{
				for (LinkedNode<T> *n = FHead; n; )
				{
					LinkedNode<T> * tmp = n->pNext;
					delete n;
					n = tmp;
				}
				FHead = 0;
				FTail = 0;
				FCount = 0;
			}
			List<T> ToList() const
			{
				List<T> rs;
				rs.Reserve(FCount);
				for (auto & item : *this)
				{
					rs.Add(item);
				}
				return rs;
			}
			int Count()
			{
				return FCount;
			}
		};
	}
}
#endif

/***********************************************************************
CORELIB\INTSET.H
***********************************************************************/
#ifndef BIT_VECTOR_INT_SET_H
#define BIT_VECTOR_INT_SET_H

#include <memory.h>

namespace CoreLib
{
	namespace Basic
	{
		class IntSet
		{
		private:
			List<int> buffer;
		public:
			IntSet()
			{}
			IntSet(const IntSet & other)
			{
				buffer = other.buffer;
			}
			IntSet(IntSet && other)
			{
				*this = (_Move(other));
			}
			IntSet & operator = (IntSet && other)
			{
				buffer = _Move(other.buffer);
				return *this;
			}
			IntSet & operator = (const IntSet & other)
			{
				buffer = other.buffer;
				return *this;
			}
			int GetHashCode()
			{
				int rs = 0;
				for (auto val : buffer)
					rs ^= val;
				return rs;
			}
			IntSet(int maxVal)
			{
				SetMax(maxVal);
			}
			int Size() const
			{
				return buffer.Count()*32;
			}
			void SetMax(int val)
			{
				Resize(val);
				Clear();
			}
			void SetAll()
			{
				for (int i = 0; i<buffer.Count(); i++)
					buffer[i] = 0xFFFFFFFF;
			}
			void Resize(int size)
			{
				int oldBufferSize = buffer.Count();
				buffer.SetSize((size+31)>>5);
				if (buffer.Count() > oldBufferSize)
					memset(buffer.Buffer()+oldBufferSize, 0, (buffer.Count()-oldBufferSize) * sizeof(int));
			}
			void Clear()
			{
				for (int i = 0; i<buffer.Count(); i++)
					buffer[i] = 0;
			}
			void Add(int val)
			{
				int id = val>>5;
				if (id < buffer.Count())
					buffer[id] |= (1<<(val&31));
				else
				{
					int oldSize = buffer.Count();
					buffer.SetSize(id+1);
					memset(buffer.Buffer() + oldSize, 0, (buffer.Count()-oldSize)*sizeof(int));
					buffer[id] |= (1<<(val&31));
				}
			}
			void Remove(int val)
			{
				if ((val>>5) < buffer.Count())
					buffer[(val>>5)] &= ~(1<<(val&31));
			}
			bool Contains(int val) const
			{
				if ((val>>5) >= buffer.Count())
					return false;
				return (buffer[(val>>5)] & (1<<(val&31))) != 0;
			}
			void UnionWith(const IntSet & set)
			{
				for (int i = 0; i<Math::Min(set.buffer.Count(), buffer.Count()); i++)
				{
					buffer[i] |= set.buffer[i];
				}
				if (set.buffer.Count() > buffer.Count())
					buffer.AddRange(set.buffer.Buffer()+buffer.Count(), set.buffer.Count()-buffer.Count());
			}
			bool operator == (const IntSet & set)
			{
				if (buffer.Count() != set.buffer.Count())
					return false;
				for (int i = 0; i<buffer.Count(); i++)
					if (buffer[i] != set.buffer[i])
						return false;
				return true;
			}
			bool operator != (const IntSet & set)
			{
				return !(*this == set);
			}
			void IntersectWith(const IntSet & set)
			{
				if (set.buffer.Count() < buffer.Count())
					memset(buffer.Buffer() + set.buffer.Count(), 0, (buffer.Count()-set.buffer.Count())*sizeof(int));
				for (int i = 0; i<Math::Min(set.buffer.Count(), buffer.Count()); i++)
				{
					buffer[i] &= set.buffer[i];
				}
			}
			static void Union(IntSet & rs, const IntSet & set1, const IntSet & set2)
			{
				rs.buffer.SetSize(Math::Max(set1.buffer.Count(), set2.buffer.Count()));
				rs.Clear();
				for (int i = 0; i<set1.buffer.Count(); i++)
					rs.buffer[i] |= set1.buffer[i];
				for (int i = 0; i<set2.buffer.Count(); i++)
					rs.buffer[i] |= set2.buffer[i];
			}
			static void Intersect(IntSet & rs, const IntSet & set1, const IntSet & set2)
			{
				rs.buffer.SetSize(Math::Min(set1.buffer.Count(), set2.buffer.Count()));
				for (int i = 0; i<rs.buffer.Count(); i++)
					rs.buffer[i] = set1.buffer[i] & set2.buffer[i];
			}
			static void Subtract(IntSet & rs, const IntSet & set1, const IntSet & set2)
			{
				rs.buffer.SetSize(set1.buffer.Count());
				for (int i = 0; i<Math::Min(set1.buffer.Count(), set2.buffer.Count()); i++)
					rs.buffer[i] = set1.buffer[i] & (~set2.buffer[i]);
			}
			static bool HasIntersection(const IntSet & set1, const IntSet & set2)
			{
				for (int i = 0; i<Math::Min(set1.buffer.Count(), set2.buffer.Count()); i++)
				{
					if (set1.buffer[i] & set2.buffer[i])
						return true;
				}
				return false;
			}
		};
	}
}

#endif

/***********************************************************************
CORELIB\DICTIONARY.H
***********************************************************************/
#ifndef CORE_LIB_DICTIONARY_H
#define CORE_LIB_DICTIONARY_H
namespace CoreLib
{
	namespace Basic
	{
		template<int IsInt>
		class Hash
		{
		public:
		};
		template<>
		class Hash<1>
		{
		public:
			template<typename TKey>
			static int GetHashCode(TKey & key)
			{
				return (int)key;
			}
		};
		template<>
		class Hash<0>
		{
		public:
			template<typename TKey>
			static int GetHashCode(TKey & key)
			{
				return key.GetHashCode();
			}
		};
		template<int IsPointer>
		class PointerHash
		{};
		template<>
		class PointerHash<1>
		{
		public:
			template<typename TKey>
			static int GetHashCode(TKey & key)
			{
				return (int)((CoreLib::PtrInt)key)/sizeof(typename std::remove_pointer<TKey>::type);
			}
		};
		template<>
		class PointerHash<0>
		{
		public:
			template<typename TKey>
			static int GetHashCode(TKey & key)
			{
				return Hash<std::is_integral<TKey>::value || std::is_enum<TKey>::value>::GetHashCode(key);
			}
		};

		template<typename TKey>
		int GetHashCode(const TKey & key)
		{
			return PointerHash<std::is_pointer<TKey>::value>::GetHashCode(key);
		}

		template<typename TKey>
		int GetHashCode(TKey & key)
		{
			return PointerHash<std::is_pointer<TKey>::value>::GetHashCode(key);
		}
		
		
		inline int GetHashCode(double key)
		{
			return FloatAsInt((float)key);
		}
		inline int GetHashCode(float key)
		{
			return FloatAsInt(key);
		}

		template<typename TKey, typename TValue>
		class KeyValuePair
		{
		public:
			TKey Key;
			TValue Value;
			KeyValuePair()
			{}
			KeyValuePair(const TKey & key, const TValue & value)
			{
				Key = key;
				Value = value;
			}
			KeyValuePair(TKey && key, TValue && value)
			{
				Key = _Move(key);
				Value = _Move(value);
			}
			KeyValuePair(TKey && key, const TValue & value)
			{
				Key = _Move(key);
				Value = value;
			}
			KeyValuePair(const KeyValuePair<TKey, TValue> & _that)
			{
				Key = _that.Key;
				Value = _that.Value;
			}
			KeyValuePair(KeyValuePair<TKey, TValue> && _that)
			{
				operator=(_Move(_that));
			}
			KeyValuePair & operator=(KeyValuePair<TKey, TValue> && that)
			{
				Key = _Move(that.Key);
				Value = _Move(that.Value);
				return *this;
			}
			KeyValuePair & operator=(const KeyValuePair<TKey, TValue> & that)
			{
				Key = that.Key;
				Value = that.Value;
				return *this;
			}
			int GetHashCode()
			{
				return GetHashCode(Key);
			}
		};

		const float MaxLoadFactor = 0.7f;

		template<typename TKey, typename TValue>
		class Dictionary
		{
			friend class Iterator;
			friend class ItemProxy;
		private:
			inline int GetProbeOffset(int /*probeId*/) const
			{
				// quadratic probing
				return 1;
			}
		private:
			int bucketSizeMinusOne, shiftBits;
			int _count;
			IntSet marks;
			KeyValuePair<TKey, TValue>* hashMap;
			void Free()
			{
				if (hashMap)
					delete [] hashMap;
				hashMap = 0;
			}
			inline bool IsDeleted(int pos) const
			{
				return marks.Contains((pos<<1) + 1);
			}
			inline bool IsEmpty(int pos) const
			{
				return !marks.Contains((pos<<1));
			}
			inline void SetDeleted(int pos, bool val)
			{
				if (val)
					marks.Add((pos<<1)+1);
				else
					marks.Remove((pos<<1)+1);
			}
			inline void SetEmpty(int pos, bool val)
			{
				if (val)
					marks.Remove((pos<<1));
				else
					marks.Add((pos<<1));
			}
			struct FindPositionResult
			{
				int ObjectPosition;
				int InsertionPosition;
				FindPositionResult()
				{
					ObjectPosition = -1;
					InsertionPosition = -1;
				}
				FindPositionResult(int objPos, int insertPos)
				{
					ObjectPosition = objPos;
					InsertionPosition = insertPos;
				}

			};
			inline int GetHashPos(TKey & key) const
			{
				return (GetHashCode(key)*2654435761) >> shiftBits;
			}
			FindPositionResult FindPosition(const TKey & key) const
			{
				int hashPos = GetHashPos((TKey&)key);
				int insertPos = -1;
				int numProbes = 0;
				while (numProbes <= bucketSizeMinusOne)
				{
					if (IsEmpty(hashPos))
					{
						if (insertPos == -1)
							return FindPositionResult(-1, hashPos);
						else
							return FindPositionResult(-1, insertPos);
					}
					else if (IsDeleted(hashPos))
					{
						if (insertPos == -1)
							insertPos = hashPos;
					}
					else if (hashMap[hashPos].Key == key)
					{
						return FindPositionResult(hashPos, -1);
					}
					numProbes++;
					hashPos = (hashPos+GetProbeOffset(numProbes)) & bucketSizeMinusOne;
				}
				if (insertPos != -1)
					return FindPositionResult(-1, insertPos);
				throw InvalidOperationException(L"Hash map is full. This indicates an error in Key::Equal or Key::GetHashCode.");
			}
			TValue & _Insert(KeyValuePair<TKey, TValue> && kvPair, int pos)
			{
				hashMap[pos] = _Move(kvPair);
				SetEmpty(pos, false);
				SetDeleted(pos, false);
				return hashMap[pos].Value;
			}
			void Rehash()
			{
				if (bucketSizeMinusOne == -1 || _count/(float)bucketSizeMinusOne >= MaxLoadFactor)
				{
					int newSize = (bucketSizeMinusOne+1) * 2;
					int newShiftBits = shiftBits - 1;
					if (newSize == 0)
					{
						newSize = 16;
						newShiftBits = 28;
					}
					Dictionary<TKey, TValue> newDict;
					newDict.shiftBits = newShiftBits;
					newDict.bucketSizeMinusOne = newSize - 1;
					newDict.hashMap = new KeyValuePair<TKey, TValue>[newSize];
					newDict.marks.SetMax(newSize*2);
					if (hashMap)
					{
						for (auto & kvPair : *this)
						{
							newDict.Add(_Move(kvPair));
						}
					}
					*this = _Move(newDict);
				}
			}
			
			bool AddIfNotExists(KeyValuePair<TKey, TValue> && kvPair)
			{
				Rehash();
				auto pos = FindPosition(kvPair.Key);
				if (pos.ObjectPosition != -1)
					return false;
				else if (pos.InsertionPosition != -1)
				{
					_count++;
					_Insert(_Move(kvPair), pos.InsertionPosition);
					return true;
				}
				else
					throw InvalidOperationException(L"Inconsistent find result returned. This is a bug in Dictionary implementation.");
			}
			void Add(KeyValuePair<TKey, TValue> && kvPair)
			{
				if (!AddIfNotExists(_Move(kvPair)))
					throw KeyExistsException(L"The key already exists in Dictionary.");
			}
			TValue & Set(KeyValuePair<TKey, TValue> && kvPair)
			{
				Rehash();
				auto pos = FindPosition(kvPair.Key);
				if (pos.ObjectPosition != -1)
					return _Insert(_Move(kvPair), pos.ObjectPosition);
				else if (pos.InsertionPosition != -1)
				{
					_count++;
					return _Insert(_Move(kvPair), pos.InsertionPosition);
				}
				else
					throw InvalidOperationException(L"Inconsistent find result returned. This is a bug in Dictionary implementation.");
			}
		public:
			class Iterator
			{
			private:
				const Dictionary<TKey, TValue> * dict;
				int pos;
			public:
				KeyValuePair<TKey, TValue> & operator *() const
				{
					return dict->hashMap[pos];
				}
				KeyValuePair<TKey, TValue> * operator ->() const
				{
					return dict->hashMap + pos;
				}
				Iterator & operator ++()
				{
					if (pos > dict->bucketSizeMinusOne)
						return *this;
					pos++;
					while (pos <= dict->bucketSizeMinusOne && (dict->IsDeleted(pos) || dict->IsEmpty(pos)))
					{
						pos++;
					}
					return *this;
				}
				Iterator operator ++(int)
				{
					Iterator rs = * this;
					operator++( );
					return rs;
				}
				bool operator != (const Iterator & _that) const
				{
					return pos != _that.pos || dict != _that.dict;
				}
				bool operator == (const Iterator & _that) const
				{
					return pos == _that.pos && dict == _that.dict;
				}
				Iterator(const Dictionary<TKey, TValue> * _dict, int _pos)
				{
					this->dict = _dict;
					this->pos = _pos;
				}
				Iterator()
				{
					this->dict = 0;
					this->pos = 0;
				}
			};

			Iterator begin() const
			{
				int pos = 0;
				while (pos < bucketSizeMinusOne + 1)
				{
					if (IsEmpty(pos) || IsDeleted(pos))
						pos++;
					else
						break;
				}
				return Iterator(this, pos);
			}
			Iterator end() const
			{
				return Iterator(this, bucketSizeMinusOne + 1);
			}
		public:
			void Add(const TKey & key, const TValue & value)
			{
				Add(KeyValuePair<TKey, TValue>(key, value));
			}
			void Add(TKey && key, TValue && value)
			{
				Add(KeyValuePair<TKey, TValue>(_Move(key), _Move(value)));
			}
			bool AddIfNotExists(const TKey & key, const TValue & value)
			{
				return AddIfNotExists(KeyValuePair<TKey, TValue>(key, value));
			}
			bool AddIfNotExists(TKey && key, TValue && value)
			{
				return AddIfNotExists(KeyValuePair<TKey, TValue>(_Move(key), _Move(value)));
			}
			void Remove(const TKey & key)
			{
				auto pos = FindPosition(key);
				if (pos.ObjectPosition != -1)
				{
					SetDeleted(pos.ObjectPosition, true);
					_count--;
				}
			}
			void Clear()
			{
				_count = 0;

				marks.Clear();
			}
			bool ContainsKey(const TKey & key) const
			{
				if (bucketSizeMinusOne == -1)
					return false;
				auto pos = FindPosition(key);
				return pos.ObjectPosition != -1;
			}
			bool TryGetValue(const TKey & key, TValue & value) const
			{
				if (bucketSizeMinusOne == -1)
					return false;
				auto pos = FindPosition(key);
				if (pos.ObjectPosition != -1)
				{
					value = hashMap[pos.ObjectPosition].Value;
					return true;
				}
				return false;
			}
			TValue * TryGetValue(const TKey & key) const
			{
				if (bucketSizeMinusOne == -1)
					return nullptr;
				auto pos = FindPosition(key);
				if (pos.ObjectPosition != -1)
				{
					return &hashMap[pos.ObjectPosition].Value;
				}
				return nullptr;
			}
			class ItemProxy
			{
			private:
				const Dictionary<TKey, TValue> * dict;
				TKey key;
			public:
				ItemProxy(const TKey & _key, const Dictionary<TKey, TValue> * _dict)
				{
					this->dict = _dict;
					this->key = _key;
				}
				ItemProxy(TKey && _key, const Dictionary<TKey, TValue> * _dict)
				{
					this->dict = _dict;
					this->key = _Move(_key);
				}
				TValue & GetValue() const
				{
					auto pos = dict->FindPosition(key);
					if (pos.ObjectPosition != -1)
					{
						return dict->hashMap[pos.ObjectPosition].Value;
					}
					else
						throw KeyNotFoundException(L"The key does not exists in dictionary.");
				}
				inline TValue & operator()() const
				{
					return GetValue();
				}
				operator TValue&() const
				{
					return GetValue();
				}
				TValue & operator = (const TValue & val) const
				{
					return ((Dictionary<TKey,TValue>*)dict)->Set(KeyValuePair<TKey, TValue>(_Move(key), val));
				}
				TValue & operator = (TValue && val) const
				{
					return ((Dictionary<TKey,TValue>*)dict)->Set(KeyValuePair<TKey, TValue>(_Move(key), _Move(val)));
				}
			};
			ItemProxy operator [](const TKey & key) const
			{
				return ItemProxy(key, this);
			}
			ItemProxy operator [](TKey && key) const
			{
				return ItemProxy(_Move(key), this);
			}
			int Count() const
			{
				return _count;
			}
		public:
			Dictionary()
			{
				bucketSizeMinusOne = -1;
				shiftBits = 32;
				_count = 0;
				hashMap = 0;
			}
			Dictionary(const Dictionary<TKey, TValue> & other)
				: hashMap(0), _count(0), bucketSizeMinusOne(-1)
			{
				*this = other;
			}
			Dictionary(Dictionary<TKey, TValue> && other)
				: hashMap(0), _count(0), bucketSizeMinusOne(-1)
			{
				*this = (_Move(other));
			}
			Dictionary<TKey, TValue> & operator = (const Dictionary<TKey, TValue> & other)
			{
				Free();
				bucketSizeMinusOne = other.bucketSizeMinusOne;
				_count = other._count;
				shiftBits = other.shiftBits;
				hashMap = new KeyValuePair<TKey, TValue>[other.bucketSizeMinusOne+1];
				marks = other.marks;
				for (int i = 0; i<= bucketSizeMinusOne; i++)
					hashMap[i] = other.hashMap[i];
				return *this;
			}
			Dictionary<TKey, TValue> & operator = (Dictionary<TKey, TValue> && other)
			{
				Free();
				bucketSizeMinusOne = other.bucketSizeMinusOne;
				_count = other._count;
				hashMap = other.hashMap;
				shiftBits = other.shiftBits;
				marks = _Move(other.marks);
				other.hashMap = 0;
				other._count = 0;
				other.bucketSizeMinusOne = -1;
				return *this;
			}
			~Dictionary()
			{
				Free();
			}
		};

		template<typename TKey, typename TValue>
		class EnumerableDictionary
		{
			friend class Iterator;
			friend class ItemProxy;
		private:
			inline int GetProbeOffset(int /*probeIdx*/) const
			{
				// quadratic probing
				return 1;
			}
		private:
			int bucketSizeMinusOne, shiftBits;
			int _count;
			IntSet marks;

			// debug op
			struct Op
			{
				TKey key;
				int opType;
				Op()
				{}
				Op(const TKey & key, int t)
				{
					this->key = key;
					opType = t;
				}
			};
			LinkedList<KeyValuePair<TKey, TValue>> kvPairs;
			LinkedNode<KeyValuePair<TKey, TValue>>** hashMap;
			void Free()
			{
				if (hashMap)
					delete[] hashMap;
				hashMap = 0;
				kvPairs.Clear();
			}
			inline bool IsDeleted(int pos) const
			{
				return marks.Contains((pos << 1) + 1);
			}
			inline bool IsEmpty(int pos) const
			{
				return !marks.Contains((pos << 1));
			}
			inline void SetDeleted(int pos, bool val)
			{
				if (val)
					marks.Add((pos << 1) + 1);
				else
					marks.Remove((pos << 1) + 1);
			}
			inline void SetEmpty(int pos, bool val)
			{
				if (val)
					marks.Remove((pos << 1));
				else
					marks.Add((pos << 1));
			}
			struct FindPositionResult
			{
				int ObjectPosition;
				int InsertionPosition;
				FindPositionResult()
				{
					ObjectPosition = -1;
					InsertionPosition = -1;
				}
				FindPositionResult(int objPos, int insertPos)
				{
					ObjectPosition = objPos;
					InsertionPosition = insertPos;
				}

			};
			inline int GetHashPos(TKey & key) const
			{
				return (GetHashCode(key) * 2654435761) >> shiftBits;
			}
			FindPositionResult FindPosition(const TKey & key) const
			{
				int hashPos = GetHashPos((TKey&)key);
				int insertPos = -1;
				int numProbes = 0;
				while (numProbes <= bucketSizeMinusOne)
				{
					if (IsEmpty(hashPos))
					{
						if (insertPos == -1)
							return FindPositionResult(-1, hashPos);
						else
							return FindPositionResult(-1, insertPos);
					}
					else if (IsDeleted(hashPos))
					{
						if (insertPos == -1)
							insertPos = hashPos;
					}
					else if (hashMap[hashPos]->Value.Key == key)
					{
						return FindPositionResult(hashPos, -1);
					}
					numProbes++;
					hashPos = (hashPos + GetProbeOffset(numProbes)) & bucketSizeMinusOne;
				}
				if (insertPos != -1)
					return FindPositionResult(-1, insertPos);
				throw InvalidOperationException(L"Hash map is full. This indicates an error in Key::Equal or Key::GetHashCode.");
			}
			TValue & _Insert(KeyValuePair<TKey, TValue> && kvPair, int pos)
			{
				auto node = kvPairs.AddLast();
				node->Value = _Move(kvPair);
				hashMap[pos] = node;
				SetEmpty(pos, false);
				SetDeleted(pos, false);
				return node->Value.Value;
			}
			void Rehash()
			{
				if (bucketSizeMinusOne == -1 || _count / (float)bucketSizeMinusOne >= MaxLoadFactor)
				{
					int newSize = (bucketSizeMinusOne + 1) * 2;
					int newShiftBits = shiftBits - 1;
					if (newSize == 0)
					{
						newSize = 16;
						newShiftBits = 28;
					}
					EnumerableDictionary<TKey, TValue> newDict;
					newDict.shiftBits = newShiftBits;
					newDict.bucketSizeMinusOne = newSize - 1;
					newDict.hashMap = new LinkedNode<KeyValuePair<TKey, TValue>>*[newSize];
					newDict.marks.SetMax(newSize * 2);
					if (hashMap)
					{
						for (auto & kvPair : *this)
						{
							newDict.Add(_Move(kvPair));
						}
					}
					*this = _Move(newDict);
				}
			}

			bool AddIfNotExists(KeyValuePair<TKey, TValue> && kvPair)
			{
				Rehash();
				auto pos = FindPosition(kvPair.Key);
				if (pos.ObjectPosition != -1)
					return false;
				else if (pos.InsertionPosition != -1)
				{
					_count++;
					_Insert(_Move(kvPair), pos.InsertionPosition);
					return true;
				}
				else
					throw InvalidOperationException(L"Inconsistent find result returned. This is a bug in Dictionary implementation.");
			}
			void Add(KeyValuePair<TKey, TValue> && kvPair)
			{
				if (!AddIfNotExists(_Move(kvPair)))
					throw KeyExistsException(L"The key already exists in Dictionary.");
			}
			TValue & Set(KeyValuePair<TKey, TValue> && kvPair)
			{
				Rehash();
				auto pos = FindPosition(kvPair.Key);
				if (pos.ObjectPosition != -1)
				{
					hashMap[pos.ObjectPosition]->Delete();
					return _Insert(_Move(kvPair), pos.ObjectPosition);
				}
				else if (pos.InsertionPosition != -1)
				{
					_count++;
					return _Insert(_Move(kvPair), pos.InsertionPosition);
				}
				else
					throw InvalidOperationException(L"Inconsistent find result returned. This is a bug in Dictionary implementation.");
			}
		public:
			typedef typename LinkedList<KeyValuePair<TKey, TValue>>::Iterator Iterator;

			typename LinkedList<KeyValuePair<TKey, TValue>>::Iterator begin() const
			{
				return kvPairs.begin();
			}
			typename LinkedList<KeyValuePair<TKey, TValue>>::Iterator end() const
			{
				return kvPairs.end();
			}
		public:
			void Add(const TKey & key, const TValue & value)
			{
				Add(KeyValuePair<TKey, TValue>(key, value));
			}
			void Add(TKey && key, TValue && value)
			{
				Add(KeyValuePair<TKey, TValue>(_Move(key), _Move(value)));
			}
			bool AddIfNotExists(const TKey & key, const TValue & value)
			{
				return AddIfNotExists(KeyValuePair<TKey, TValue>(key, value));
			}
			bool AddIfNotExists(TKey && key, TValue && value)
			{
				return AddIfNotExists(KeyValuePair<TKey, TValue>(_Move(key), _Move(value)));
			}
			void Remove(const TKey & key)
			{
				if (_count > 0)
				{
					auto pos = FindPosition(key);
					if (pos.ObjectPosition != -1)
					{
						kvPairs.Delete(hashMap[pos.ObjectPosition]);
						hashMap[pos.ObjectPosition] = 0;
						SetDeleted(pos.ObjectPosition, true);
						_count--;
					}
				}
			}
			void Clear()
			{
				_count = 0;
				kvPairs.Clear();
				marks.Clear();
			}
			bool ContainsKey(const TKey & key) const
			{
				if (bucketSizeMinusOne == -1)
					return false;
				auto pos = FindPosition(key);
				return pos.ObjectPosition != -1;
			}
			TValue * TryGetValue(const TKey & key) const
			{
				if (bucketSizeMinusOne == -1)
					return nullptr;
				auto pos = FindPosition(key);
				if (pos.ObjectPosition != -1)
				{
					return &(hashMap[pos.ObjectPosition]->Value.Value);
				}
				return nullptr;
			}
			bool TryGetValue(const TKey & key, TValue & value) const
			{
				if (bucketSizeMinusOne == -1)
					return false;
				auto pos = FindPosition(key);
				if (pos.ObjectPosition != -1)
				{
					value = hashMap[pos.ObjectPosition]->Value.Value;
					return true;
				}
				return false;
			}
			class ItemProxy
			{
			private:
				const EnumerableDictionary<TKey, TValue> * dict;
				TKey key;
			public:
				ItemProxy(const TKey & _key, const EnumerableDictionary<TKey, TValue> * _dict)
				{
					this->dict = _dict;
					this->key = _key;
				}
				ItemProxy(TKey && _key, const EnumerableDictionary<TKey, TValue> * _dict)
				{
					this->dict = _dict;
					this->key = _Move(_key);
				}
				TValue & GetValue() const
				{
					auto pos = dict->FindPosition(key);
					if (pos.ObjectPosition != -1)
					{
						return dict->hashMap[pos.ObjectPosition]->Value.Value;
					}
					else
					{
						throw KeyNotFoundException(L"The key does not exists in dictionary.");
					}
				}
				inline TValue & operator()() const
				{
					return GetValue();
				}
				operator TValue&() const
				{
					return GetValue();
				}
				TValue & operator = (const TValue & val)
				{
					return ((EnumerableDictionary<TKey, TValue>*)dict)->Set(KeyValuePair<TKey, TValue>(_Move(key), val));
				}
				TValue & operator = (TValue && val)
				{
					return ((EnumerableDictionary<TKey, TValue>*)dict)->Set(KeyValuePair<TKey, TValue>(_Move(key), _Move(val)));
				}
			};
			ItemProxy operator [](const TKey & key) const
			{
				return ItemProxy(key, this);
			}
			ItemProxy operator [](TKey && key) const
			{
				return ItemProxy(_Move(key), this);
			}
			int Count() const
			{
				return _count;
			}
		public:
			EnumerableDictionary()
			{
				bucketSizeMinusOne = -1;
				shiftBits = 32;
				_count = 0;
				hashMap = 0;
			}
			EnumerableDictionary(const EnumerableDictionary<TKey, TValue> & other)
				: hashMap(0), _count(0), bucketSizeMinusOne(-1)
			{
				*this = other;
			}
			EnumerableDictionary(EnumerableDictionary<TKey, TValue> && other)
				: hashMap(0), _count(0), bucketSizeMinusOne(-1)
			{
				*this = (_Move(other));
			}
			EnumerableDictionary<TKey, TValue> & operator = (const EnumerableDictionary<TKey, TValue> & other)
			{
				Clear();
				for (auto & item : other)
					Add(item.Key, item.Value);
				return *this;
			}
			EnumerableDictionary<TKey, TValue> & operator = (EnumerableDictionary<TKey, TValue> && other)
			{
				Free();
				bucketSizeMinusOne = other.bucketSizeMinusOne;
				_count = other._count;
				hashMap = other.hashMap;
				shiftBits = other.shiftBits;
				marks = _Move(other.marks);
				other.hashMap = 0;
				other._count = 0;
				other.bucketSizeMinusOne = -1;
				kvPairs = _Move(other.kvPairs);
				return *this;
			}
			~EnumerableDictionary()
			{
				Free();
			}
		};
		
		class _DummyClass
		{};

		template<typename T, typename DictionaryType>
		class HashSetBase
		{
		private:
			DictionaryType dict;
		public:
			HashSetBase()
			{}
			HashSetBase(const HashSetBase & set)
			{
				operator=(set);
			}
			HashSetBase(HashSetBase && set)
			{
				operator=(_Move(set));
			}
			HashSetBase & operator = (const HashSetBase & set)
			{
				dict = set.dict;
				return *this;
			}
			HashSetBase & operator = (HashSetBase && set)
			{
				dict = _Move(set.dict);
				return *this;
			}
		public:
			class Iterator
			{
			private:
				typename DictionaryType::Iterator iter;
			public:
				Iterator() = default;
				T & operator *() const
				{
					return (*iter).Key;
				}
				T * operator ->() const
				{
					return &(*iter).Key;
				}
				Iterator & operator ++()
				{
					++iter;
					return *this;
				}
				Iterator operator ++(int)
				{
					Iterator rs = * this;
					operator++( );
					return rs;
				}
				bool operator != (const Iterator & _that) const
				{
					return iter != _that.iter;
				}
				bool operator == (const Iterator & _that) const
				{
					return iter == _that.iter;
				}
				Iterator(const typename DictionaryType::Iterator & _iter)
				{
					this->iter = _iter;
				}
			};
			Iterator begin() const
			{
				return Iterator(dict.begin());
			}
			Iterator end() const
			{
				return Iterator(dict.end());
			}
		public:
			T & First() const
			{
				return *begin();
			}
			T & Last() const
			{
				return *end();
			}
			int Count() const
			{
				return dict.Count();
			}
			void Clear()
			{
				dict.Clear();
			}
			bool Add(const T& obj)
			{
				return dict.AddIfNotExists(obj, _DummyClass());
			}
			bool Add(T && obj)
			{
				return dict.AddIfNotExists(_Move(obj), _DummyClass());
			}
			void Remove(const T & obj)
			{
				dict.Remove(obj);
			}
			bool Contains(const T & obj) const
			{
				return dict.ContainsKey(obj);
			}
		};
		template <typename T>
		class HashSet : public HashSetBase<T, Dictionary<T, _DummyClass>>
		{};

		template <typename T>
		class EnumerableHashSet : public HashSetBase<T, EnumerableDictionary<T, _DummyClass>>
		{};
	}
}

#endif

/***********************************************************************
CORELIB\FUNC.H
***********************************************************************/
#ifndef CORELIB_FUNC_H
#define CORELIB_FUNC_H


namespace CoreLib
{
	namespace Basic
	{
		template<typename TResult, typename... Arguments>
		class FuncPtr
		{
		public:
			virtual TResult operator()(Arguments...) = 0;
			virtual bool operator == (const FuncPtr *)
			{
				return false;
			}
		};

		template<typename TResult, typename... Arguments>
		class CdeclFuncPtr : public FuncPtr<TResult, Arguments...>
		{
		public:
			typedef TResult (*FuncType)(Arguments...);
		private:
			FuncType funcPtr;
		public:
			CdeclFuncPtr(FuncType func)
				:funcPtr(func)
			{
			}

			virtual TResult operator()(Arguments... params) override
			{
				return funcPtr(params...);
			}

			virtual bool operator == (const FuncPtr<TResult, Arguments...> * ptr) override
			{
				auto cptr = dynamic_cast<const CdeclFuncPtr<TResult, Arguments...>*>(ptr);
				if (cptr)
					return funcPtr == cptr->funcPtr;
				else
					return false;
			}
		};

		template<typename Class, typename TResult, typename... Arguments>
		class MemberFuncPtr : public FuncPtr<TResult, Arguments...>
		{
		public:
			typedef TResult (Class::*FuncType)(Arguments...);
		private:
			FuncType funcPtr;
			Class * object;
		public:
			MemberFuncPtr(Class * obj, FuncType func)
				:object(obj), funcPtr(func)
			{
			}

			virtual TResult operator()(Arguments... params) override
			{
				return (object->*funcPtr)(params...);
			}

			virtual bool operator == (const FuncPtr * ptr) override
			{
				auto cptr = dynamic_cast<const MemberFuncPtr<Class, TResult, Arguments...>*>(ptr);
				if (cptr)
					return funcPtr == cptr->funcPtr && object == cptr->object;
				else
					return false;
			}
		};

		template<typename F, typename TResult, typename... Arguments>
		class LambdaFuncPtr : public FuncPtr<TResult, Arguments...>
		{
		private:
			F func;
		public:
			LambdaFuncPtr(const F & _func)
				: func(_func)
			{}
			virtual TResult operator()(Arguments... params) override
			{
				return func(params...);
			}
			virtual bool operator == (const FuncPtr * /*ptr*/) override
			{
				return false;
			}
		};

		template<typename TResult, typename... Arguments>
		class Func
		{
		private:
			RefPtr<FuncPtr<TResult, Arguments...>> funcPtr;
		public:
			Func(){}
			Func(typename CdeclFuncPtr<TResult, Arguments...>::FuncType func)
			{
				funcPtr = new CdeclFuncPtr<TResult, Arguments...>(func);
			}
			template<typename Class>
			Func(Class * object, typename MemberFuncPtr<Class, TResult, Arguments...>::FuncType func)
			{
				funcPtr = new MemberFuncPtr<Class, TResult, Arguments...>(object, func);
			}
			template<typename TFuncObj>
			Func(const TFuncObj & func)
			{
				funcPtr = new LambdaFuncPtr<TFuncObj, TResult, Arguments...>(func);
			}
			Func & operator = (typename CdeclFuncPtr<TResult, Arguments...>::FuncType func)
			{
				funcPtr = new CdeclFuncPtr<TResult, Arguments...>(func);
				return *this;
			}
			template<typename Class>
			Func & operator = (const MemberFuncPtr<Class, TResult, Arguments...> & func)
			{
				funcPtr = new MemberFuncPtr<Class, TResult, Arguments...>(func);
				return *this;
			}
			template<typename TFuncObj>
			Func & operator = (const TFuncObj & func)
			{
				funcPtr = new LambdaFuncPtr<TFuncObj, TResult, Arguments>(func);
				return *this;
			}
			TResult operator()(Arguments... params)
			{
				return (*funcPtr)(params...);
			}
		};

		// template<typename... Arguments>
		// using Procedure = Func<void, Arguments...>;

		template<typename... Arguments>
		class Procedure : public Func<void, Arguments...>
		{
		private:
			RefPtr<FuncPtr<void, Arguments...>> funcPtr;
		public:
			Procedure(){}
			Procedure(const Procedure & proc)
			{
				funcPtr = proc;
			}
			Procedure(typename CdeclFuncPtr<void, Arguments...>::FuncType func)
			{
				funcPtr = new CdeclFuncPtr<void, Arguments...>(func);
			}
			template<typename Class>
			Procedure(Class * object, typename MemberFuncPtr<Class, void, Arguments...>::FuncType func)
			{
				funcPtr = new MemberFuncPtr<Class, void, Arguments...>(object, func);
			}
			template<typename TFuncObj>
			Procedure(const TFuncObj & func)
			{
				funcPtr = new LambdaFuncPtr<TFuncObj, void, Arguments...>(func);
			}
			Procedure & operator = (typename CdeclFuncPtr<void, Arguments...>::FuncType func)
			{
				funcPtr = new CdeclFuncPtr<void, Arguments...>(func);
				return *this;
			}
			template<typename Class>
			Procedure & operator = (const MemberFuncPtr<Class, void, Arguments...> & func)
			{
				funcPtr = new MemberFuncPtr<Class, void, Arguments...>(func);
				return *this;
			}
			template<typename TFuncObj>
			Procedure & operator = (const TFuncObj & func)
			{
				funcPtr = new LambdaFuncPtr<TFuncObj, void, Arguments...>(func);
				return *this;
			}
			Procedure & operator = (const Procedure & proc)
			{
				funcPtr = proc.funcPtr;
			}
			void Clear()
			{
				funcPtr = nullptr;
			}
			void operator()(Arguments... params)
			{
				if (funcPtr)
					(*funcPtr)(params...);
			}
		};
	}
}

#endif

/***********************************************************************
CORELIB\LINQ.H
***********************************************************************/
#ifndef FUNDAMENTAL_LIB_LINQ_H
#define FUNDAMENTAL_LIB_LINQ_H


namespace CoreLib
{
	namespace Basic
	{

		template <typename T>
		T ConstructT();

		template <typename T>
		class RemoveReference
		{
		public:
			typedef T Type;
		};

		template <typename T>
		class RemoveReference<T&>
		{
		public:
			typedef T Type;
		};

		template <typename T>
		class RemoveReference<T&&>
		{
		public:
			typedef T Type;
		};

		template<typename T>
		struct RemovePointer
		{
			typedef T Type;
		};

		template<typename T>
		struct RemovePointer<T*>
		{
			typedef T Type;
		};

		template <typename TQueryable, typename TEnumerator, typename T, typename TFunc>
		class WhereQuery
		{
		private:
			TQueryable items;
			TFunc func;
		public:
			WhereQuery(const TQueryable & queryable, const TFunc & f)
				: items(queryable), func(f)
			{}
			class Enumerator
			{
			private:
				TEnumerator ptr;
				TEnumerator end;
				TFunc *func;
			public:
				Enumerator(const Enumerator &) = default;
				Enumerator(TEnumerator ptr, TEnumerator end, TFunc & f)
					: ptr(ptr), end(end), func(&f)
				{}
				T operator *() const
				{
					return *(ptr);
				}
				Enumerator& operator ++()
				{
					++ptr;
					while (ptr != end)
					{
						if ((*func)(*ptr))
							break;
						else
							++ptr;
					}
					return *this;
				}
				Enumerator operator ++(int)
				{
					Enumerator rs = *this;
					while (rs.ptr != end)
					{
						if ((*func)(*rs.ptr))
							break;
						++rs.ptr;
					}
					return rs;
				}
				bool operator != (const Enumerator & iter) const
				{
					return ptr != iter.ptr;
				}
				bool operator == (const Enumerator & iter) const
				{
					return ptr == iter.ptr;
				}
			};
			Enumerator begin()
			{
				auto ptr = items.begin();
				auto end = items.end();
				while (ptr != end)
				{
					if (func(*ptr))
						break;
					++ptr;
				}
				return Enumerator(ptr, end, func);
			}
			Enumerator end()
			{
				return Enumerator(items.end(), items.end(), func);
			}
		};

		template <typename TQueryable, typename TEnumerator, typename T, typename TFunc>
		class SelectQuery
		{
		private:
			TQueryable items;
			TFunc func;
		public:
			SelectQuery(const TQueryable & queryable, const TFunc & f)
				: items(queryable), func(f)
			{}
			class Enumerator
			{
			private:
				TEnumerator ptr;
				TEnumerator end;
				TFunc *func;
			public:
				Enumerator(const Enumerator &) = default;
				Enumerator(TEnumerator ptr, TEnumerator end, TFunc & f)
					: ptr(ptr), end(end), func(&f)
				{}
				auto operator *() const -> decltype((*func)(*ptr))
				{
					return (*func)(*ptr);
				}
				Enumerator& operator ++()
				{
					++ptr;
					return *this;
				}
				Enumerator operator ++(int)
				{
					Enumerator rs = *this;
					++rs;
					return rs;
				}
				bool operator != (const Enumerator & iter) const
				{
					return !(ptr == iter.ptr);
				}
				bool operator == (const Enumerator & iter) const
				{
					return ptr == iter.ptr;
				}
			};
			Enumerator begin()
			{
				return Enumerator(items.begin(), items.end(), func);
			}
			Enumerator end()
			{
				return Enumerator(items.end(), items.end(), func);
			}
		};

		template <typename TQueryable, typename TEnumerator, typename T, typename TFunc>
		class SelectManyQuery
		{
		private:
			TQueryable items;
			TFunc func;
			SelectManyQuery()
			{}
		public:
			SelectManyQuery(const TQueryable & queryable, const TFunc & f)
				: items(queryable), func(f)
			{}
			template<typename TItems, typename TItemPtr>
			class Enumerator
			{
			private:
				TEnumerator ptr;
				TEnumerator end;
				TFunc &func;
				TItems items;
				TItemPtr subPtr;
			public:
				Enumerator(const Enumerator &) = default;
				Enumerator(TEnumerator ptr, TEnumerator end, TFunc & f)
					: ptr(ptr), end(end), func(f)
				{
					if (ptr != end)
					{
						items = f(*ptr);
						subPtr = items.begin();
					}
				}
				auto operator *() const -> decltype(*subPtr)
				{
					return *subPtr;
				}
				Enumerator& operator ++()
				{
					++subPtr;
					while (subPtr == items.end() && ptr != end)
					{
						++ptr;
						if (ptr != end)
						{
							items = func(*ptr);
							subPtr = items.begin();
						}
						else
							break;
					}
					
					return *this;
				}
				Enumerator operator ++(int)
				{
					Enumerator rs = *this;
					++rs;
					return rs;
				}
				bool operator != (const Enumerator & iter) const
				{
					return !operator==(iter);
				}
				bool operator == (const Enumerator & iter) const
				{
					if (ptr == iter.ptr)
					{
						if (ptr == end)
							return true;
						else
							return subPtr == iter.subPtr;
					}
					else
						return false;
				}
			};
			auto begin()->Enumerator<decltype(func(ConstructT<T>())), decltype(func(ConstructT<T>()).begin())>
			{
				return Enumerator<decltype(func(ConstructT<T>())), decltype(func(ConstructT<T>()).begin())>(items.begin(), items.end(), func);
			}
			auto end()->Enumerator<decltype(func(ConstructT<T>())), decltype(func(ConstructT<T>()).begin())>
			{
				return Enumerator<decltype(func(ConstructT<T>())), decltype(func(ConstructT<T>()).begin())>(items.end(), items.end(), func);
			}
		};

		template <typename T>
		struct EnumeratorType
		{
			typedef decltype(ConstructT<T>().begin()) Type;
		};

		template <typename TFunc, typename TArg>
		class ExtractReturnType
		{
		public:
			static TFunc * f;
			static TArg ConstructArg(){};
			typedef decltype((*f)(ConstructArg())) ReturnType;
		};

		template <typename T>
		class ExtractItemType
		{
		public:
			typedef typename RemovePointer<decltype(ConstructT<T>().begin())>::Type Type;
		};

		template <typename TQueryable, typename TEnumerator, typename T>
		class Queryable
		{
		private:
			TQueryable items;
		public:
			auto begin() -> decltype(items.begin())
			{
				return items.begin();
			}
			auto end() -> decltype(items.end())
			{
				return items.end();
			}
		public:
			Queryable(const TQueryable & items)
				: items(items)
			{}

			template<typename TFunc>
			Queryable<WhereQuery<TQueryable, TEnumerator, T, TFunc>, typename WhereQuery<TQueryable, TEnumerator, T, TFunc>::Enumerator, T> Where(const TFunc & f)
			{
				return Queryable<WhereQuery<TQueryable, TEnumerator, T, TFunc>, typename WhereQuery<TQueryable, TEnumerator, T, TFunc>::Enumerator, T>(WhereQuery<TQueryable, TEnumerator, T, TFunc>(items, f));
			}

			template<typename TFunc>
			Queryable<SelectQuery<TQueryable, TEnumerator, T, TFunc>, typename SelectQuery<TQueryable, TEnumerator, T, TFunc>::Enumerator, typename RemoveReference<typename ExtractReturnType<TFunc, T>::ReturnType>::Type> Select(const TFunc & f)
			{
				return Queryable<SelectQuery<TQueryable, TEnumerator, T, TFunc>, typename SelectQuery<TQueryable, TEnumerator, T, TFunc>::Enumerator, typename RemoveReference<typename ExtractReturnType<TFunc, T>::ReturnType>::Type>(SelectQuery<TQueryable, TEnumerator, T, TFunc>(items, f));
			}

			template<typename TFunc>
			auto SelectMany(const TFunc & f) ->Queryable<SelectManyQuery<TQueryable, TEnumerator, T, TFunc>, typename EnumeratorType<SelectManyQuery<TQueryable, TEnumerator, T, TFunc>>::Type, typename ExtractItemType<decltype(f(ConstructT<T>()))>::Type>
			{
				return Queryable<SelectManyQuery<TQueryable, TEnumerator, T, TFunc>, typename EnumeratorType<SelectManyQuery<TQueryable, TEnumerator, T, TFunc>>::Type, typename ExtractItemType<decltype(f(ConstructT<T>()))>::Type>(SelectManyQuery<TQueryable, TEnumerator, T, TFunc>(items, f));
			}

			template<typename TAggregateResult, typename TFunc>
			auto Aggregate(const TAggregateResult & initial, const TFunc & f) -> decltype(f(initial, *items.begin()))
			{
				TAggregateResult rs = initial;
				for (auto && x : items)
					rs = f(rs, x);
				return rs;
			}

			template<typename TFunc>
			T & First(const TFunc & condition)
			{
				for (auto && x : items)
					if (condition(x))
						return x;
			}

			template <typename TFunc>
			T Max(const TFunc & selector)
			{
				return Aggregate(*items.begin(), [&](const T & v0, const T & v1)
				{
					return selector(v0) > selector(v1) ? v0 : v1; 
				});
			}

			template <typename TFunc>
			T Min(const TFunc & selector)
			{
				return Aggregate(*items.begin(), [&](const T & v0, const T & v1)
				{
					return selector(v0) < selector(v1) ? v0 : v1;
				});
			}

			template <typename TFunc>
			auto Sum(const TFunc & selector) -> decltype(selector(ConstructT<T>()))
			{
				decltype(selector(ConstructT<T>())) rs(0);
				for (auto && x : items)
					rs = rs + selector(x);
				return rs;
			}

			T Max()
			{
				return Aggregate(*items.begin(), [](const T & v0, const T & v1) {return v0 > v1 ? v0 : v1; });
			}

			T Min()
			{
				return Aggregate(*items.begin(), [](const T & v0, const T & v1) {return v0 < v1 ? v0 : v1; });
			}

			T Sum()
			{
				T rs = T(0);
				for (auto && x : items)
					rs = rs + x;
				return rs;
			}

			T Avg()
			{
				T rs = T(0);
				int count = 0;
				for (auto && x : items)
				{
					rs = rs + x;
					count++;
				}
				return rs / count;
			}

			int Count()
			{
				int rs = 0;
				for (auto && x : items)
					rs++;
				return rs;
			}

			List<T> ToList()
			{
				List<T> rs;
				for (auto && val : items)
					rs.Add(val);
				return rs;
			}
		};


		template<typename T, typename TAllocator>
		inline Queryable<ArrayView<T>, T*, T> AsQueryable(const List<T, TAllocator> & list)
		{
			return Queryable<ArrayView<T>, T*, T>(list.GetArrayView());
		}

		template<typename T>
		inline Queryable<ArrayView<T>, T*, T> AsQueryable(const ArrayView<T> & list)
		{
			return Queryable<ArrayView<T>, T*, T>(list);
		}

		
		template<typename T>
		struct LinkedListView
		{
			typename LinkedList<T>::Iterator start, last;
			typename LinkedList<T>::Iterator begin()
			{
				return start;
			}
			typename LinkedList<T>::Iterator end()
			{
				return last;
			}
		};

		template<typename T>
		inline Queryable<LinkedListView<T>, LinkedNode<T>, T> AsQueryable(const LinkedList<T> & list)
		{
			LinkedListView<T> view;
			view.start = list.begin();
			view.last = list.end();
			return Queryable<LinkedListView<T>, LinkedNode<T>, T>(view);
		}

		template<typename TKey, typename TValue>
		struct EnumerableDictView
		{
			typename EnumerableDictionary<TKey, TValue>::Iterator start, last;
			typename EnumerableDictionary<TKey, TValue>::Iterator begin()
			{
				return start;
			}
			typename EnumerableDictionary<TKey, TValue>::Iterator end()
			{
				return last;
			}
		};

		template<typename TKey, typename TValue>
		inline Queryable<EnumerableDictView<TKey, TValue>, typename EnumerableDictionary<TKey, TValue>::Iterator, KeyValuePair<TKey, TValue>> AsQueryable(const EnumerableDictionary<TKey, TValue> & dict)
		{
			EnumerableDictView<TKey, TValue> view;
			view.start = dict.begin();
			view.last = dict.end();
			return Queryable<EnumerableDictView<TKey, TValue>, typename EnumerableDictionary<TKey, TValue>::Iterator, KeyValuePair<TKey, TValue>>(view);
		}

		template<typename TKey>
		struct EnumerableHashSetView
		{
			typename HashSetBase<TKey, EnumerableDictionary<TKey, _DummyClass>>::Iterator start, last;
			typename EnumerableHashSet<TKey>::Iterator begin()
			{
				return start;
			}
			typename EnumerableHashSet<TKey>::Iterator end()
			{
				return last;
			}
		};

		template<typename TKey>
		inline Queryable<EnumerableHashSetView<TKey>, typename HashSetBase<TKey, EnumerableDictionary<TKey, _DummyClass>>::Iterator, TKey> AsQueryable(const EnumerableHashSet<TKey> & dict)
		{
			EnumerableHashSetView<TKey> view;
			view.start = dict.begin();
			view.last = dict.end();
			return Queryable<EnumerableHashSetView<TKey>, typename HashSetBase<TKey, EnumerableDictionary<TKey, _DummyClass>>::Iterator, TKey>(view);
		}
	}
}

#endif

/***********************************************************************
CORELIB\BASIC.H
***********************************************************************/
#ifndef CORE_LIB_BASIC_H
#define CORE_LIB_BASIC_H


namespace CoreLib
{
	using namespace Basic;
}

#endif

/***********************************************************************
CORELIB\STREAM.H
***********************************************************************/
#ifndef CORE_LIB_STREAM_H
#define CORE_LIB_STREAM_H


namespace CoreLib
{
	namespace IO
	{
		using CoreLib::Basic::Exception;
		using CoreLib::Basic::String;
		using CoreLib::Basic::RefPtr;

		class IOException : public Exception
		{
		public:
			IOException()
			{}
			IOException(const String & message)
				: CoreLib::Basic::Exception(message)
			{
			}
		};

		class EndOfStreamException : public IOException
		{
		public:
			EndOfStreamException()
			{}
			EndOfStreamException(const String & message)
				: IOException(message)
			{
			}
		};

		enum class SeekOrigin
		{
			Start, End, Current
		};

		class Stream : public CoreLib::Basic::Object
		{
		public:
			virtual Int64 GetPosition()=0;
			virtual void Seek(SeekOrigin origin, Int64 offset)=0;
			virtual Int64 Read(void * buffer, Int64 length) = 0;
			virtual Int64 Write(const void * buffer, Int64 length) = 0;
			virtual bool CanRead()=0;
			virtual bool CanWrite()=0;
			virtual void Close()=0;
		};

		class BinaryReader
		{
		private:
			RefPtr<Stream> stream;
		public:
			BinaryReader(RefPtr<Stream> stream)
			{
				this->stream = stream;
			}
			Stream * GetStream()
			{
				return stream.Ptr();
			}
			void ReleaseStream()
			{
				stream.Release();
			}
			template<typename T>
			void Read(T * buffer, int count)
			{
				stream->Read(buffer, sizeof(T)*(Int64)count);
			}
			int ReadInt32()
			{
				int rs;
				stream->Read(&rs, sizeof(int));
				return rs;
			}
			short ReadInt16()
			{
				short rs;
				stream->Read(&rs, sizeof(short));
				return rs;
			}
			Int64 ReadInt64()
			{
				Int64 rs;
				stream->Read(&rs, sizeof(Int64));
				return rs;
			}
			float ReadFloat()
			{
				float rs;
				stream->Read(&rs, sizeof(float));
				return rs;
			}
			double ReadDouble()
			{
				double rs;
				stream->Read(&rs, sizeof(double));
				return rs;
			}
			char ReadChar()
			{
				char rs;
				stream->Read(&rs, sizeof(char));
				return rs;
			}
			String ReadString()
			{
				int len = ReadInt32();
				wchar_t * buffer = new wchar_t[len+1];
				try
				{
					stream->Read(buffer, sizeof(wchar_t)*len);
				}
				catch(IOException & e)
				{
					delete [] buffer;
					throw e;
				}
				buffer[len] = 0;
				return String::FromBuffer(buffer, len);
			}
		};

		class BinaryWriter
		{
		private:
			RefPtr<Stream> stream;
		public:
			BinaryWriter(RefPtr<Stream> stream)
			{
				this->stream = stream;
			}
			Stream * GetStream()
			{
				return stream.Ptr();
			}
			template<typename T>
			void Write(const T& val)
			{
				stream->Write(&val, sizeof(T));
			}
			template<typename T>
			void Write(T * buffer, int count)
			{
				stream->Write(buffer, sizeof(T)*(Int64)count);
			}
			void Write(const String & str)
			{
				Write(str.Length());
				Write(str.Buffer(), str.Length());
			}
			void ReleaseStream()
			{
				stream.Release();
			}
			void Close()
			{
				stream->Close();
			}
		};

		enum class FileMode
		{
			Create, Open, CreateNew, Append
		};

		enum class FileAccess
		{
			Read = 1, Write = 2, ReadWrite = 3
		};

		enum class FileShare
		{
			None, ReadOnly, WriteOnly, ReadWrite
		};

		class FileStream : public Stream
		{
		private:
			FILE * handle;
			FileAccess fileAccess;
			void Init(const CoreLib::Basic::String & fileName, FileMode fileMode, FileAccess access, FileShare share);
		public:
			FileStream(const CoreLib::Basic::String & fileName, FileMode fileMode = FileMode::Open);
			FileStream(const CoreLib::Basic::String & fileName, FileMode fileMode, FileAccess access, FileShare share);
			~FileStream();
		public:
			virtual Int64 GetPosition();
			virtual void Seek(SeekOrigin origin, Int64 offset);
			virtual Int64 Read(void * buffer, Int64 length);
			virtual Int64 Write(const void * buffer, Int64 length);
			virtual bool CanRead();
			virtual bool CanWrite();
			virtual void Close();
		};
	}
}

#endif

/***********************************************************************
CORELIB\TEXTIO.H
***********************************************************************/
#ifndef CORE_LIB_TEXT_IO_H
#define CORE_LIB_TEXT_IO_H

#ifdef _MSC_VER
#include <mbstring.h>
#endif

namespace CoreLib
{
	namespace IO
	{
		using CoreLib::Basic::List;
		using CoreLib::Basic::_EndLine;

		class TextReader : public CoreLib::Basic::Object
		{
		public:
			~TextReader()
			{
				Close();
			}
			virtual void Close(){}
			virtual wchar_t Read()=0;
			virtual wchar_t Peak()=0;
			virtual int Read(wchar_t * buffer, int count)=0;
			virtual String ReadLine()=0;
			virtual String ReadToEnd()=0;
		};

		class TextWriter : public CoreLib::Basic::Object
		{
		public:
			~TextWriter()
			{
				Close();
			}
			virtual void Write(const String & str)=0;
			virtual void Write(const wchar_t * str, int length=0)=0;
			virtual void Write(const char * str, int length=0)=0;
			virtual void Close(){}
			template<typename T>
			TextWriter & operator << (const T& val)
			{
				Write(val.ToString());
				return *this;
			}
			TextWriter & operator << (wchar_t value)
			{
				Write(String(value));
				return *this;
			}
			TextWriter & operator << (int value)
			{
				Write(String(value));
				return *this;
			}
			TextWriter & operator << (float value)
			{
				Write(String(value));
				return *this;
			}
			TextWriter & operator << (double value)
			{
				Write(String(value));
				return *this;
			}
			TextWriter & operator << (const char* value)
			{
				Write(value, (int)strlen(value));
				return *this;
			}
			TextWriter & operator << (const wchar_t * const val)
			{
				Write(val, (int)wcslen(val));
				return *this;
			}
			TextWriter & operator << (wchar_t * const val)
			{
				Write(val, (int)wcslen(val));
				return *this;
			}

			TextWriter & operator << (const String & val)
			{
				Write(val);
				return *this;
			}
			TextWriter & operator << (const _EndLine &)
			{
#ifdef WINDOWS_PLATFORM
				Write(L"\r\n", 2);
#else
				Write(L"\n", 1);
#endif
				return *this;
			}
		};

		class Encoding
		{
		public:
			static Encoding * Unicode, * Ansi;
			virtual List<char> GetBytes(const String & str)=0;
			virtual String GetString(char * buffer, int length)=0;
			virtual ~Encoding()
			{}
			String GetString(const List<char> & buffer)
			{
				return GetString(buffer.Buffer(), buffer.Count());
			}
		};

		class StreamWriter : public TextWriter
		{
		private:
			RefPtr<Stream> stream;
			Encoding * encoding;
		public:
			StreamWriter(const String & path, Encoding * encoding = Encoding::Unicode);
			StreamWriter(RefPtr<Stream> stream, Encoding * encoding = Encoding::Unicode);
			virtual void Write(const String & str);
			virtual void Write(const wchar_t * str, int length=0);
			virtual void Write(const char * str, int length=0);
			virtual void Close()
			{
				stream->Close();
			}
		};

		class StreamReader : public TextReader
		{
		private:
			RefPtr<Stream> stream;
			List<char> buffer;
			Encoding * encoding;
			int ptr;
			char ReadBufferChar();
			char PeakBufferChar(int offset);
			void ReadBuffer();
			template<typename GetFunc>
			wchar_t GetChar(GetFunc get)
			{
				wchar_t rs = 0;
				if (encoding == Encoding::Unicode)
				{
					((char*)&rs)[0] = get(0);
					((char*)&rs)[1] = get(1);
				}
				else
				{
					char mb[2];
					mb[0] = get(0);
					if (_ismbblead(mb[0]))
					{
						mb[1] = get(1);
						MByteToWideChar(&rs, 1, mb, 2);
					}
					else
					{
						rs = mb[0];
					}
					return rs;
				}
				return rs;
			}
			Encoding * DetermineEncoding();
		public:
			StreamReader(const String & path);
			StreamReader(RefPtr<Stream> stream, Encoding * encoding = Encoding::Ansi);
			
			virtual wchar_t Read()
			{
				return GetChar([&](int){return ReadBufferChar();});
			}
			virtual wchar_t Peak()
			{
				return GetChar([&](int offset){return PeakBufferChar(offset); });
			}
			virtual int Read(wchar_t * buffer, int count);
			virtual String ReadLine();
			virtual String ReadToEnd();

			virtual void Close()
			{
				stream->Close();
			}
		};
	}
}

#endif

/***********************************************************************
CORELIB\REGEX\REGEXTREE.H
***********************************************************************/
#ifndef GX_REGEX_PARSER_H
#define GX_REGEX_PARSER_H


namespace CoreLib
{
	namespace Text
	{
		using namespace CoreLib::Basic;

		class RegexCharSetNode;
		class RegexRepeatNode;
		class RegexConnectionNode;
		class RegexSelectionNode;

		class RegexNodeVisitor : public Object
		{
		public:
			virtual void VisitCharSetNode(RegexCharSetNode * node);
			virtual void VisitRepeatNode(RegexRepeatNode * node);
			virtual void VisitConnectionNode(RegexConnectionNode * node);
			virtual void VisitSelectionNode(RegexSelectionNode * node);
		};

		class RegexNode : public Object
		{
		public:
			virtual String Reinterpret() = 0;
			virtual void Accept(RegexNodeVisitor * visitor) = 0;
		};

		class RegexCharSet : public Object
		{
		private:
			List<RegexCharSet *> OriSet;
			void CopyCtor(const RegexCharSet & set);
		public:
			bool Neg;
			struct RegexCharRange
			{
				wchar_t Begin,End;
				bool operator == (const RegexCharRange & r);
			};
			List<RegexCharRange> Ranges;
			List<unsigned short> Elements; 
		
		public:
			RegexCharSet()
			{
				Neg = false;
			}
			RegexCharSet(const RegexCharSet & set);
			String Reinterpret();
			void Normalize();
			void Sort();
			void AddRange(RegexCharRange r);
			void SubtractRange(RegexCharRange r);
			bool Contains(RegexCharRange r);
			bool operator ==(const RegexCharSet & set);
			RegexCharSet & operator = (const RegexCharSet & set);
			static void InsertElement(List<RefPtr<RegexCharSet>> &L, RefPtr<RegexCharSet> & elem);
			static void RangeMinus(RegexCharRange r1, RegexCharRange r2, RegexCharSet & rs);
			static void CharSetMinus(RegexCharSet & s1, RegexCharSet & s2);
			static void RangeIntersection(RegexCharRange r1, RegexCharRange r2, RegexCharSet &rs);
			static void CalcCharElementFromPair(RegexCharSet * c1, RegexCharSet * c2, RegexCharSet & AmB, RegexCharSet & BmA, RegexCharSet & AnB);
			static void CalcCharElements(List<RegexCharSet *> & sets, List<RegexCharRange> & elements);
		};

		class RegexCharSetNode : public RegexNode
		{
		public:
			RefPtr<RegexCharSet> CharSet;
		public:
			String Reinterpret();
			virtual void Accept(RegexNodeVisitor * visitor);
			RegexCharSetNode();
		};

		class RegexRepeatNode : public RegexNode
		{
		public:
			enum _RepeatType
			{
				rtOptional, rtArbitary, rtMoreThanOnce, rtSpecified
			};
			_RepeatType RepeatType;
			int MinRepeat, MaxRepeat;
			RefPtr<RegexNode> Child;
		public:
			String Reinterpret();
			virtual void Accept(RegexNodeVisitor * visitor);
		};

		class RegexConnectionNode : public RegexNode
		{
		public:
			RefPtr<RegexNode> LeftChild, RightChild;
		public:
			String Reinterpret();
			virtual void Accept(RegexNodeVisitor * visitor);
		};

		class RegexSelectionNode : public RegexNode
		{
		public:
			RefPtr<RegexNode> LeftChild, RightChild;
		public:
			String Reinterpret();
			virtual void Accept(RegexNodeVisitor * visitor);
		};

		class RegexParser : public Object
		{
		private:
			String src;
			int ptr;
			RegexNode * ParseSelectionNode();
			RegexNode * ParseConnectionNode();
			RegexNode * ParseRepeatNode();
			RegexCharSet * ParseCharSet();
			wchar_t ReadNextCharInCharSet();
			int ParseInteger();
			bool IsOperator();
		public:
			struct SyntaxError
			{
				int Position;
				String Text;
			};
			List<SyntaxError> Errors;
			RefPtr<RegexNode> Parse(const String & regex); 
		};
	}
}

#endif

/***********************************************************************
CORELIB\REGEX\REGEXNFA.H
***********************************************************************/
#ifndef REGEX_NFA_H
#define REGEX_NFA_H


namespace CoreLib
{
	namespace Text
	{
		using namespace CoreLib::Basic;
	
		typedef unsigned short Word;

		class NFA_Node;

		class NFA_Translation : public Object
		{
		public:
			RefPtr<RegexCharSet> CharSet;
			NFA_Node * NodeSrc, * NodeDest;
			NFA_Translation();
			NFA_Translation(NFA_Node * src, NFA_Node * dest, RefPtr<RegexCharSet> charSet);
			NFA_Translation(NFA_Node * src, NFA_Node * dest);
		};

		class NFA_Node : public Object
		{
		private:
			static int HandleCount;
		public:
			int ID;
			bool Flag;
			bool IsFinal;
			int TerminalIdentifier;
			List<NFA_Translation *> Translations;
			List<NFA_Translation *> PrevTranslations;
			void RemoveTranslation(NFA_Translation * trans);
			void RemovePrevTranslation(NFA_Translation * trans);
			NFA_Node();
		};

		class NFA_Graph : public RegexNodeVisitor
		{
			friend class DFA_Graph;
		private:
			NFA_Node * start, * end;
			struct NFA_StatePair
			{
				NFA_Node * start, * end;
			};
			List<NFA_StatePair> stateStack;
			NFA_StatePair PopState();
			void PushState(NFA_StatePair s);
		private:
			List<RefPtr<NFA_Node>> nodes;
			List<RefPtr<NFA_Translation>> translations;
			void ClearNodes();
			void ClearNodeFlags();
			void GetValidStates(List<NFA_Node *> & states);
			void GetEpsilonClosure(NFA_Node * node, List<NFA_Node *> & states);
			void EliminateEpsilon();
		public:
			NFA_Node * CreateNode();
			NFA_Translation * CreateTranslation();
			String Interpret();
			void GenerateFromRegexTree(RegexNode * tree, bool elimEpsilon = true);
			void PostGenerationProcess();
			void CombineNFA(NFA_Graph * graph);
			NFA_Node * GetStartNode();
			void SetStartNode(NFA_Node * node);
			void SetTerminalIdentifier(int id);
			virtual void VisitCharSetNode(RegexCharSetNode * node);
			virtual void VisitRepeatNode(RegexRepeatNode * node);
			virtual void VisitConnectionNode(RegexConnectionNode * node);
			virtual void VisitSelectionNode(RegexSelectionNode * node);
		};
	}
}


#endif

/***********************************************************************
CORELIB\REGEX\REGEXDFA.H
***********************************************************************/
#ifndef REGEX_DFA_H
#define REGEX_DFA_H


namespace CoreLib
{
	namespace Text
	{
		using namespace CoreLib::Basic;

		typedef List<Word> RegexCharTable;
	
		class CharTableGenerator : public Object
		{
		private:
			List<String> sets;
			RegexCharTable * table;
			int AddSet(String set);
		public:
			List<RegexCharSet::RegexCharRange> elements;
			CharTableGenerator(RegexCharTable * _table);
			int Generate(List<RegexCharSet *> & charSets);
		};

		class DFA_Table_Tag
		{
		public:
			bool IsFinal;
			List<int> TerminalIdentifiers; // sorted
			DFA_Table_Tag();
		};

		class DFA_Table : public Object
		{
		public:
			int StateCount;
			int AlphabetSize;
			int ** DFA;
			List<RefPtr<DFA_Table_Tag>> Tags;
			int StartState;
			RefPtr<RegexCharTable> CharTable;
			DFA_Table();
			~DFA_Table();
		};

		class DFA_Node : public Object
		{
		public:
			int ID;
			bool IsFinal;
			List<int> TerminalIdentifiers; // sorted
			List<NFA_Node*> Nodes;  // sorted
			List<DFA_Node *> Translations;
			DFA_Node(int elements);
			bool operator == (const DFA_Node & node);
		};

		class DFA_Graph : public Object
		{
		private:
			List<RegexCharSet::RegexCharRange> CharElements;
			RefPtr<RegexCharTable> table;
			DFA_Node * startNode;
			List<RefPtr<DFA_Node>> nodes;
			void CombineCharElements(NFA_Node * node, List<Word> & elem);
		public:
			void Generate(NFA_Graph * nfa);
			String Interpret();
			void ToDfaTable(DFA_Table * dfa);
		};
	}
}
#endif

/***********************************************************************
CORELIB\REGEX\REGEX.H
***********************************************************************/
#ifndef GX_REGEX_H
#define GX_REGEX_H


namespace CoreLib
{
	namespace Text
	{
		class IllegalRegexException : public Exception
		{
		};

		class RegexMatcher : public Object
		{
		private:
			DFA_Table * dfa;
		public:
			RegexMatcher(DFA_Table * table);
			int Match(const String & str, int startPos = 0);
		};

		class PureRegex : public Object
		{
		private:
			RefPtr<DFA_Table> dfaTable;
		public:
			struct RegexMatchResult
			{
				int Start;
				int Length;
			};
			PureRegex(const String & regex);
			bool IsMatch(const String & str); // Match Whole Word
			RegexMatchResult Search(const String & str, int startPos = 0);
			DFA_Table * GetDFA();
		};
	}
}

#endif

/***********************************************************************
CORELIB\REGEX\METALEXER.H
***********************************************************************/
#ifndef GX_META_LEXER_H
#define GX_META_LEXER_H


namespace CoreLib
{
	namespace Text
	{
		class LexToken
		{
		public:
			String Str;
			int TypeID;
			int Position;
		};

		class LazyLexToken
		{
		public:
			int TypeID;
			int Position;
			int Length;
		};

		typedef LinkedList<LexToken> LexStream;
	
		class LexerError
		{
		public:
			String Text;
			int Position;
		};

		struct LexProfileToken
		{
			String str;
			enum LexProfileTokenType
			{
				Identifier,
				Equal,
				Regex
			} type;
		};

		typedef LinkedList<LexProfileToken> LexProfileTokenStream;
		typedef LinkedNode<LexProfileToken> LexProfileTokenNode;

		class LexicalParseException : public Exception
		{
		public:
			int Position;
			LexicalParseException(String str, int pos) : Exception(str), Position(pos)
			{}
		};

		class LazyLexStream
		{
		private:
			RefPtr<DFA_Table> dfa; 
			List<bool> *ignoreSet;
		public:
			String InputText;
			LazyLexStream() = default;
			LazyLexStream(String text, const RefPtr<DFA_Table> & dfa, List<bool> *ignoreSet)
				: InputText(text), dfa(dfa), ignoreSet(ignoreSet)
			{}
			inline DFA_Table * GetDFA()
			{
				return dfa.Ptr();
			}
			inline List<bool> & GetIgnoreSet()
			{
				return *ignoreSet;
			}
			class Iterator
			{
			public:
				int ptr, state, lastTokenPtr;
				LazyLexStream * stream;
				LazyLexToken currentToken;
				bool operator != (const Iterator & iter) const
				{
					return lastTokenPtr != iter.lastTokenPtr;
				}
				bool operator == (const Iterator & iter) const
				{
					return lastTokenPtr == iter.lastTokenPtr;
				}
				LazyLexToken * operator ->()
				{
					return &currentToken;
				}
				LazyLexToken operator *()
				{
					return currentToken;
				}
				Iterator & operator ++();
				Iterator operator ++(int)
				{
					Iterator rs = *this;
					this->operator++();
					return rs;
				}
			};
			Iterator begin()
			{
				Iterator iter;
				iter.ptr = 0;
				iter.lastTokenPtr = 0;
				iter.state = dfa->StartState;
				iter.stream = this;
				++iter;
				return iter;
			}
			Iterator end()
			{
				Iterator iter;
				iter.ptr = InputText.Length();
				iter.lastTokenPtr = -1;
				iter.state = dfa->StartState;
				iter.stream = this;
				return iter;
			}
		};

		class MetaLexer : public Object
		{
		private:
			RefPtr<DFA_Table> dfa;
			List<String> Regex;
			List<String> TokenNames;
			List<bool> Ignore;
			String ReadProfileToken(LexProfileTokenNode*n, LexProfileToken::LexProfileTokenType type);
			bool ParseLexProfile(const String &lex);
			void ConstructDFA();
		public:
			int TokensParsed;
			List<LexerError> Errors;
			MetaLexer(String LexProfile);
			MetaLexer();
			DFA_Table * GetDFA();
			String GetTokenName(int id);
			int GetRuleCount();
			void SetLexProfile(String lex);
			bool Parse(String str, LexStream & stream);
			LazyLexStream Parse(String str)
			{
				return LazyLexStream(str, dfa, &Ignore);
			}
		};
	}
}

#endif

/***********************************************************************
CORELIB\EVENTS.H
***********************************************************************/
#ifndef GX_EVENTS_H
#define GX_EVENTS_H

namespace CoreLib
{
	namespace Basic
	{
	/***************************************************************************

	Events.h

	Usage:

		class A
		{
		public:
			void EventHandler(int a)
			{
				cout<<endl<<"function of object handler invoked. a*a = ";
				cout<<a*a<<endl;
			}
		};

		class B
		{
		public:
			typedef gxEvent1<int> gxOnEvent;
		public:
			gxOnEvent OnEvent;
			void DoSomething()
			{
				OnEvent.Invoke(4);
			}
		};

		void FuncHandler()
		{
			cout<<"Function invoked."<<endl;
		}

		void main()
		{
			A a;
			B b;
			b.OnEvent.Bind(&a,&A::EventHandler);	
			b.OnEvent.Bind(FuncHandler);			
			b.DoSomething();
			b.OnEvent.Unbind(FuncHandler);			
			b.OnEvent.Unbind(&a,&A::EventHandler);
			b.DoSomething();                       
		}

	***************************************************************************/
		template <typename... Arguments>
		class Event
		{
		private:
			List<RefPtr<FuncPtr<void, Arguments... >>> Handlers;
			void Bind(FuncPtr<void, Arguments...> * fobj)
			{
				Handlers.Add(fobj);
			}
			void Unbind(FuncPtr<void, Arguments...> * fobj)
			{
				int id = -1;
				for (int i = 0; i < Handlers.Count(); i++)
				{
					if ((*Handlers[i]) == fobj)
					{
						id = i;
						break;
					}
				}
				if (id != -1)
				{
					Handlers[id] = 0;
					Handlers.Delete(id);				
				}
			}
		public:
			Event()
			{
			}
			Event(const Event & e)
			{
				operator=(e);
			}
			Event & operator = (const Event & e)
			{
				for (int i = 0; i < e.Handlers.Count(); i++)
					Handlers.Add(e.Handlers[i]->Clone());
				return *this;
			}
			template <typename Class>
			Event(Class * Owner, typename MemberFuncPtr<Class, void, Arguments...>::FuncType handler)
			{
				Bind(Owner, handler);
			}
			Event(typename CdeclFuncPtr<void, Arguments...>::FuncType f)
			{
				Bind(f);
			}
			template <typename TFunctor>
			Event(const TFunctor & func)
			{
				Bind(func);
			}
			template <typename Class>
			void Bind(Class * Owner, typename MemberFuncPtr<Class, void, Arguments...>::FuncType handler)
			{
				Handlers.Add(new MemberFuncPtr<Class, void, Arguments...>(Owner, handler));
			}
			template <typename Class>
			void Unbind(Class * Owner, typename MemberFuncPtr<Class, void, Arguments...>::FuncType handler)
			{
				MemberFuncPtr<Class, void, Arguments...> h(Owner, handler);
				Unbind(&h);
			}
			void Bind(typename CdeclFuncPtr<void, Arguments...>::FuncType f)
			{
				Bind(new CdeclFuncPtr<void, Arguments...>(f));
			}
			void Unbind(typename CdeclFuncPtr<void, Arguments...>::FuncType f)
			{
				CdeclFuncPtr<void, Arguments...> h(f);
				Unbind(&h);
			}
			template <typename TFunctor>
			void Bind(const TFunctor & func)
			{
				Handlers.Add(new LambdaFuncPtr<TFunctor, void, Arguments...>(func));
			}
			template <typename TFunctor>
			void Unbind(const TFunctor & func)
			{
				LambdaFuncPtr<TFunctor, void, Arguments...> h(func);
				Unbind(&h);
			}
			Event & operator += (typename CdeclFuncPtr<void, Arguments...>::FuncType f)
			{
				Bind(f);
				return *this;
			}
			Event & operator -= (typename CdeclFuncPtr<void, Arguments...>::FuncType f)
			{
				Unbind(f);
				return *this;
			}
			template <typename TFunctor>
			Event & operator += (const TFunctor & f)
			{
				Bind(f);
				return *this;
			}
			template <typename TFunctor>
			Event & operator -= (const TFunctor & f)
			{
				Unbind(f);
				return *this;
			}
			void Invoke(Arguments... params) const
			{
				for (int i = 0; i < Handlers.Count(); i++)
					Handlers[i]->operator()(params...);
			}
			void operator ()(Arguments... params) const
			{
				Invoke(params...);
			}
		};
	}
}

#endif

/***********************************************************************
CORELIB\VECTORMATH.H
***********************************************************************/
#ifndef VECTOR_MATH_H
#define VECTOR_MATH_H
#include <random>
#include <cmath>
#include <xmmintrin.h>
#ifdef _M_X64
#define NO_SIMD_ASM
#endif
#ifndef _MSC_VER
#define NO_SIMD_ASM
#endif

namespace VectorMath
{
	using namespace CoreLib::Basic;
	const float PI = 3.1415926535f;
	const float Epsilon = 1e-4f;
	const int DefaultFloatUlps = 1024;
	inline float Clamp(float val, float vmin, float vmax)
	{
		return val>vmax?vmax:val<vmin?vmin:val;
	}
	inline bool FloatEquals(float A, float B, int maxUlps = DefaultFloatUlps)
	{
		int aInt = *(int*)&A;
		// Make aInt lexicographically ordered as a twos-complement int
		if (aInt < 0)
			aInt = 0x80000000 - aInt;
		// Make bInt lexicographically ordered as a twos-complement int
		int bInt = *(int*)&B;
		if (bInt < 0)
			bInt = 0x80000000 - bInt;
		int intDiff = abs(aInt - bInt);
		if (intDiff <= maxUlps)
			return true;
		return false;
	}
	inline bool FloatLarger(float A, float B, int maxUlps = DefaultFloatUlps)
	{
		return A>B && !FloatEquals(A,B,maxUlps);
	}
	inline bool FloatSmaller(float A, float B, int maxUlps = DefaultFloatUlps)
	{
		return A<B && !FloatEquals(A,B,maxUlps);
	}
	inline bool FloatSmallerOrEquals(float A, float B, int maxUlps = DefaultFloatUlps)
	{
		return A<B || FloatEquals(A, B, maxUlps);
	}
	inline bool FloatLargerOrEquals(float A, float B, int maxUlps = DefaultFloatUlps)
	{
		return A>B || FloatEquals(A, B, maxUlps);
	}

	template<typename T>
	inline T Max(T v1, T v2)
	{
		if (v1>v2) return v1; else return v2;
	}
	template<typename T>
	inline T Min(T v1, T v2)
	{
		if (v1<v2) return v1; else return v2;
	}

	class Vec4;
	class Vec2
	{
	public:
		float x, y;
#ifndef NO_VECTOR_CONSTRUCTORS
		Vec2() = default;
		Vec2(const Vec2 & v) = default;
		Vec2(float vx, float vy)
		{
			x = vx; y = vy;
		}
#endif
		static inline Vec2 Create(float f)
		{
			Vec2 rs;
			rs.x = rs.y = f;
			return rs;
		}
		static inline Vec2 Create(float vx, float vy)
		{
			Vec2 rs;
			rs.x = vx;	rs.y = vy;
			return rs;
		}
		inline void SetZero()
		{
			x = y = 0.0f;
		}
		static inline float Dot(const Vec2 & v0, const Vec2 & v1)
		{
			return v0.x * v1.x + v0.y * v1.y;
		}
		inline float & operator [] (int i)
		{
			return ((float*)this)[i];
		}
		inline Vec2 operator * (float s) const
		{
			Vec2 rs;
			rs.x = x * s;
			rs.y = y * s;
			return rs;
		}
		inline Vec2 operator * (const Vec2 &vin) const
		{
			Vec2 rs;
			rs.x = x * vin.x;
			rs.y = y * vin.y;
			return rs;
		}
		inline Vec2 operator + (const Vec2 &vin) const
		{
			Vec2 rs;
			rs.x = x + vin.x;
			rs.y = y + vin.y;
			return rs;
		}
		inline Vec2 operator - (const Vec2 &vin) const
		{
			Vec2 rs;
			rs.x = x - vin.x;
			rs.y = y - vin.y;
			return rs;
		}
		inline Vec2 & operator += (const Vec2 & vin)
		{
			x += vin.x;
			y += vin.y;
			return *this;
		}
		inline Vec2 & operator -= (const Vec2 & vin)
		{
			x -= vin.x;
			y -= vin.y;
			return *this;
		}
		Vec2 & operator = (float v)
		{
			x = y = v;
			return *this;
		}
		inline Vec2 & operator *= (float s)
		{
			x *= s;
			y *= s;
			return *this;
		}
		inline Vec2 & operator *= (const Vec2 & vin)
		{
			x *= vin.x;
			y *= vin.y;
			return *this;
		}
		inline Vec2 Normalize()
		{
			float len = sqrt(x*x + y*y);
			float invLen = 1.0f / len;
			Vec2 rs;
			rs.x = x * invLen;
			rs.y = y * invLen;
			return rs;
		}
		inline float Length()
		{
			return sqrt(x*x + y*y);
		}
	};

	struct Vec3_Struct
	{
		float x,y,z;
	};

	class Vec3 : public Vec3_Struct
	{
	public:
#ifndef NO_VECTOR_CONSTRUCTORS
		inline Vec3() = default;
		inline Vec3(float f)
		{
			x = y = z = f;
		}
		inline Vec3(float vx, float vy, float vz)
		{
			x = vx;	y = vy;	z = vz;
		}
		inline Vec3(const Vec3 & v) = default;
#endif
		static inline Vec3 Create(float f)
		{
			Vec3 rs;
			rs.x = rs.y = rs.z = f;
			return rs;
		}
		static inline Vec3 Create(float vx, float vy, float vz)
		{
			Vec3 rs;
			rs.x = vx;	rs.y = vy;	rs.z = vz;
			return rs;
		}
		static inline Vec3 FromHomogeneous(const Vec4 & v);
		inline void SetZero()
		{
			x = y = z = 0.0f;
		}
		inline float& operator [] (int i) const
		{
			return ((float*)this)[i];
		}
		inline Vec3 operator + (const Vec3 &vin) const
		{
			Vec3 rs;
			rs.x = x + vin.x;
			rs.y = y + vin.y;
			rs.z = z + vin.z;
			return rs;
		}
		inline Vec3 operator - (const Vec3 &vin) const
		{
			Vec3 rs;
			rs.x = x - vin.x;
			rs.y = y - vin.y;
			rs.z = z - vin.z;
			return rs;
		}
		inline Vec3 operator - () const
		{
			Vec3 rs;
			rs.x = -x;
			rs.y = -y;
			rs.z = -z;
			return rs;
		}
		inline Vec3 operator * (float scale) const
		{
			Vec3 rs;
			rs.x = x * scale;
			rs.y = y * scale;
			rs.z = z * scale;
			return rs;
		}
		inline Vec3 & operator += (const Vec3 & vin)
		{
			x += vin.x; y += vin.y; z += vin.z;
			return *this;
		}
		inline Vec3 & operator -= (const Vec3 & vin)
		{
			x -= vin.x; y -= vin.y; z -= vin.z; 
			return *this;
		}
		inline Vec3 & operator *= (const Vec3 & vin)
		{
			x *= vin.x; y *= vin.y; z *= vin.z;
			return *this;
		}
		inline Vec3 & operator *= (float s)
		{
			x *= s; y *= s; z *= s;
			return *this;
		}
		inline Vec3 & operator /= (const Vec3 & vin)
		{
			x /= vin.x; y /= vin.y; z /= vin.z;
			return *this;
		}
		inline Vec3 & operator /= (float s)
		{
			float inv = 1.0f/s;
			return (*this)*=inv;
		}
		inline bool operator == (const Vec3 & vin)
		{
			return x == vin.x && y == vin.y && z == vin.z;
		}
		inline bool operator != (const Vec3 & vin)
		{
			return x != vin.x || y != vin.y || z != vin.z;
		}
		inline int GetHashCode()
		{
			return FloatAsInt(x) ^ FloatAsInt(y) ^ FloatAsInt(z);
		}
		inline static float Dot(const Vec3 & v1, const Vec3 & v2)
		{
			return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
		}
		inline static void Cross(Vec3 & rs_d, const Vec3 & v1, const Vec3 & v2)
		{
			rs_d.x = v1.y*v2.z - v1.z * v2.y;
			rs_d.y = v1.z*v2.x - v1.x * v2.z;
			rs_d.z = v1.x*v2.y - v1.y * v2.x;
		}
		inline static Vec3 Cross(const Vec3 & v1, const Vec3 & v2)
		{
			Vec3 rs_d;
			rs_d.x = v1.y*v2.z - v1.z * v2.y;
			rs_d.y = v1.z*v2.x - v1.x * v2.z;
			rs_d.z = v1.x*v2.y - v1.y * v2.x;
			return rs_d;
		}
		inline static void Scale(Vec3 & rs, const Vec3 & v1, float s)
		{
			rs.x = v1.x*s;	rs.y = v1.y*s;	rs.z = v1.z*s;
		}
		inline static void Add(Vec3 & rs, const Vec3 & v1, const Vec3 & v2)
		{
			rs.x = v1.x + v2.x;
			rs.y = v1.y + v2.y;
			rs.z = v1.z + v2.z;
		}
		inline static void Subtract(Vec3 & rs, const Vec3 & v1, const Vec3 & v2)
		{
			rs.x = v1.x - v2.x;
			rs.y = v1.y - v2.y;
			rs.z = v1.z - v2.z;
		}
		inline static void Multiply(Vec3 & rs, const Vec3 & v1, const Vec3 & v2)
		{
			rs.x = v1.x * v2.x;
			rs.y = v1.y * v2.y;
			rs.z = v1.z * v2.z;
		}
		inline float LengthFPU() const
		{
			return sqrt(x*x + y*y + z*z);
		}
		inline float Length2() const
		{
			return x*x+y*y+z*z;
		}
		static inline void NormalizeFPU(Vec3 & rs, const Vec3 & vin)
		{
			float invLen = 1.0f/vin.LengthFPU();
			Scale(rs, vin, invLen);
		}
		inline float Length() const;
		static inline void Normalize(Vec3 & rs, const Vec3 & vin);
		inline Vec3 Normalize() const
		{
			Vec3 rs;
			Normalize(rs, *this);
			return rs;
		}
	};

	struct Vec4_Struct
	{
		float x,y,z,w;
	};

	class Vec4 : public Vec4_Struct
	{
	public:
#ifndef NO_VECTOR_CONSTRUCTORS
		inline Vec4() = default;
		inline Vec4(const Vec4_Struct & v)
		{
			x = v.x;
			y = v.y;
			z = v.z;
			w = v.w;
		}
		inline Vec4(float f)
		{
			x = y = z = w = f;
		}
		inline Vec4(float vx, float vy, float vz, float vw)
		{
			x = vx;	y = vy;	z = vz;	w = vw;
		}
		inline Vec4(const Vec3 & v)
		{
			x = v.x; y = v.y; z = v.z; w = 0.0f;
		}
		inline Vec4(const Vec3 & v, float vw)
		{
			x = v.x; y = v.y; z = v.z; w = vw;
		}
		inline Vec4(const Vec4 & v) = default;
#endif
		static inline Vec4 Create(float f)
		{
			Vec4 rs;
			rs.x = rs.y = rs.z = rs.w = f;
			return rs;
		}
		static inline Vec4 Create(float vx, float vy, float vz, float vw)
		{
			Vec4 rs;
			rs.x = vx;	rs.y = vy;	rs.z = vz; rs.w = vw;
			return rs;
		}
		static inline Vec4 Create(const Vec3 & v, float vw)
		{
			Vec4 rs;
			rs.x = v.x; rs.y = v.y; rs.z = v.z; rs.w = vw;
			return rs;
		}
		inline void SetZero()
		{
			x = y = z = w = 0.0f;
		}
		inline void xyz(Vec3 & v) const
		{
			v.x = x;
			v.y = y;
			v.z = z;
		}
		inline Vec3 xyz() const
		{
			Vec3 rs;
			rs.x = x;
			rs.y = y;
			rs.z = z;
			return rs;
		}
		inline float& operator [] (int i)
		{
			return ((float*)this)[i];
		}
		inline Vec4 operator + (const Vec4 &vin)
		{
			Vec4 rs;
			rs.x = x + vin.x;
			rs.y = y + vin.y;
			rs.z = z + vin.z;
			rs.w = w + vin.w;
			return rs;
		}
		inline Vec4 operator - (const Vec4 &vin)
		{
			Vec4 rs;
			rs.x = x - vin.x;
			rs.y = y - vin.y;
			rs.z = z - vin.z;
			rs.w = w - vin.w;
			return rs;
		}
		inline Vec4 operator - ()
		{
			Vec4 rs;
			rs.x = -x;
			rs.y = -y;
			rs.z = -z;
			rs.w = -w;
			return rs;
		}
		inline Vec4 operator * (float scale) const
		{
			Vec4 rs;
			rs.x = x * scale;
			rs.y = y * scale;
			rs.z = z * scale;
			rs.w = w * scale;
			return rs;
		}
		inline Vec4 & operator += (const Vec4 & vin)
		{
			x += vin.x; y += vin.y; z += vin.z; w += vin.w;
			return *this;
		}
		inline Vec4 & operator -= (const Vec4 & vin)
		{
			x -= vin.x; y -= vin.y; z -= vin.z; w -= vin.w;
			return *this;
		}
		inline Vec4 & operator *= (const Vec4 & vin)
		{
			x *= vin.x; y *= vin.y; z *= vin.z; w *= vin.w;
			return *this;
		}
		inline Vec4 & operator *= (float s)
		{
			x *= s; y *= s; z *= s; w *= s;
			return *this;
		}
		inline Vec4 & operator /= (const Vec4 & vin)
		{
			x /= vin.x; y /= vin.y; z /= vin.z; w /= vin.w;
			return *this;
		}
		inline Vec4 & operator /= (float s)
		{
			float inv = 1.0f/s;
			return (*this)*=inv;
		}
		inline bool operator == (const Vec4 & vin)
		{
			return vin.x == x && vin.y == y && vin.z == z && vin.w == w;
		}
		inline bool operator != (const Vec4 & vin)
		{
			return vin.x != x || vin.y != y || vin.z != z || vin.w != w;
		}
		inline int GetHashCode()
		{
			return FloatAsInt(x) ^ FloatAsInt(y) ^ FloatAsInt(z) ^ FloatAsInt(w);
		}
		static inline void Add(Vec4 & rs, const Vec4 & v1, const Vec4 & v2);
		static inline void Subtract(Vec4 & rs, const Vec4 & v1, const Vec4 & v2);
		static inline void Multiply(Vec4 & rs, const Vec4 & v1, const Vec4 & v2);
		static inline void MultiplyScale(Vec4 & rs, const Vec4 & v1, const Vec4 & v2);
		static inline void Scale(Vec4 & rs, const Vec4 & v1, float s);
		static inline float Dot(const Vec4 & v1, const Vec4 & v2);
		static inline void Cross(Vec4 & rs_d, const Vec4 & v1, const Vec4 & v2);
		inline float LengthFPU() const;
		inline float Length() const;
		static inline void NormalizeFPU(Vec4& vout, const Vec4& vin);
		static inline void Normalize(Vec4 &vout, const Vec4 &vin);
		inline Vec4 Normalize()
		{
			Vec4 rs;
			Normalize(rs, *this);
			return rs;
		}
	};

	class Vec4_M128
	{
	public:
		__m128 vec;
		Vec4_M128()
		{}
		Vec4_M128(__m128 v)
		{
			vec = v;
		}
		Vec4_M128(float a, float b, float c, float d)
		{
			vec = _mm_set_ps(a, b, c, d);
		}
		Vec4_M128(const Vec4 & v)
		{
			vec = _mm_load_ps((const float*)&v);
		}
		inline void Zero()
		{
			vec = _mm_setzero_ps();
		}
		inline void ToVec4(Vec4 & v) const
		{
			_mm_store_ps((float*)&v, vec);
		}
	};

	class Matrix3
	{
	public:
		union
		{
			float values[9];
			float m[3][3];
			struct
			{
				float _11, _12, _13,
				_21, _22, _23,
				_31, _32, _33;
			} mi;
		};
		inline Vec3 Transform(const Vec3& vIn) const
		{
			Vec3 rs;
			rs.x = m[0][0] * vIn.x + m[1][0] * vIn.y + m[2][0] * vIn.z;
			rs.y = m[0][1] * vIn.x + m[1][1] * vIn.y + m[2][1] * vIn.z;
			rs.z = m[0][2] * vIn.x + m[1][2] * vIn.y + m[2][2] * vIn.z;
			return rs;
		}
		static inline void Multiply(Matrix3 & rs, Matrix3 & m1, Matrix3 & m2)
		{
			for (int i = 0; i < 3; i++)
				for (int j = 0; j < 3; j++)
				{
					float dot = 0.0f;
					for (int k = 0; k < 3; k++)
						dot += m1.m[k][j] * m2.m[i][k];
					rs.m[i][j] = dot;
				}
		}
	};

	class Matrix4
	{
	public:
		union
		{
			float values[16];
			float m[4][4];
			struct
			{
				float _11,_12,_13,_14,
				  _21,_22,_23,_24,
				  _31,_32,_33,_34,
				  _41,_42,_43,_44;
			} mi;
			struct
			{
				float _11,_12,_13,_14,
				  _21,_22,_23,_24,
				  _31,_32,_33,_34,
				  _41,_42,_43,_44;
			} mr;
		};
		Matrix4()
		{}
		Matrix4(float v)
		{
			for (int i = 0; i<16; i++)
				values[i] = v;
		}
		Matrix4(const Vec4 & c1, const Vec4 & c2, const Vec4 & c3, const Vec4 &c4)
		{
			memcpy(m[0], &c1, sizeof(Vec4));
			memcpy(m[1], &c2, sizeof(Vec4));
			memcpy(m[2], &c3, sizeof(Vec4));
			memcpy(m[3], &c4, sizeof(Vec4));
		}
		inline Matrix4 & operator += (const Matrix4 & v)
		{
			for (int i = 0; i < 16; i++)
				values[i] += v.values[i];
			return *this;
		}
		inline Matrix3 GetMatrix3()
		{
			Matrix3 rs;
			for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
				rs.m[i][j] = m[i][j];
			return rs;
		}
		inline Matrix4 & operator *= (const float & val)
		{
			for (int i = 0; i < 16; i++)
				values[i] *= val;
			return *this;
		}
		inline Matrix4 & operator *= (const Matrix4 & matrix)
		{
			Multiply(*this, *this, matrix);
			return *this;
		}
		inline Matrix4 operator * (const Matrix4 & matrix)
		{
			Matrix4 rs;
			Multiply(rs, *this, matrix);
			return rs;
		}
		inline Matrix4 & LeftMultiply(const Matrix4 & matrix)
		{
			Multiply(*this, matrix, *this);
			return *this;
		}
		inline void Transpose()
		{
			float tmp;
			for (int i = 1; i<4; i++)
				for (int j = 0; j < i; j++)
				{
					tmp = m[i][j];
					m[i][j] = m[j][i];
					m[j][i] = tmp;
				}
		}
		static inline void CreateIdentityMatrix(Matrix4 & mOut);
		static inline void CreateRandomMatrix(Matrix4 & mOut);
		static inline void CreateOrthoMatrix(Matrix4 & mOut, float left, float right, float top, float bottom, float zNear, float zFar);
		static inline void CreatePerspectiveMatrixFromViewAngle(Matrix4 &mOut, float fovY, float aspect, float zNear, float zFar);
		static inline void CreatePerspectiveMatrixFromViewAngleTiled(Matrix4 &mOut, float fovY, float aspect, float zNear, float zFar, float x0, float y0, float x1, float y1);
		static inline void CreatePerspectiveMatrix(Matrix4 &mOut, float left, float right, float bottom, float top, float zNear, float zFar);
		static void LookAt(Matrix4 & rs, const Vec3 & pos, const Vec3 & center, const Vec3 & up);
		static inline void RotationX(Matrix4 & rs, float angle);
		static inline void RotationY(Matrix4 & rs, float angle);
		static inline void RotationZ(Matrix4 & rs, float angle);
		static void Rotation(Matrix4 & rs, const Vec3 & axis, float angle);
		static void Rotation(Matrix4 & rs, float yaw, float pitch, float roll);
		static inline void Scale(Matrix4 & rs, float sx, float sy, float sz);
		static inline void Translation(Matrix4 & rs, float tx, float ty, float tz);
		inline void Transform(Vec3 & rs_d, const Vec3& vIn) const;
		inline void Transform(Vec4 & rs_d, const Vec4& vIn) const;
		inline void TransformNormal(Vec3 & rs, const Vec3& vIn) const;
		inline void TransposeTransformNormal(Vec3 & rs, const Vec3 & vIn) const;
		inline void TransposeTransform(Vec3 & rs, const Vec3 & vIn) const;
		inline void TransposeTransform(Vec4 & rs_d, const Vec4& vIn) const;
		inline void TransformHomogeneous(Vec3 & rs, const Vec3 & vIn) const;
		inline void TransformHomogeneous2D(Vec2 & rs, const Vec3 & vIn) const;
		static inline void MultiplyFPU(Matrix4 &mOut, const Matrix4& M1, const Matrix4& M2);
		static inline void Multiply(Matrix4 &mOut, const Matrix4& M1, const Matrix4& M2);
		float Inverse3D(Matrix4 & mOut_d) const;
		float InverseFPU(Matrix4 &mOut_d) const;
		void GetNormalMatrix(Matrix4 & mOut) const;
		inline float Inverse(Matrix4 &mOut_d) const;
	};

	//__declspec(align(16))
	class Matrix4_M128
	{
	private:
		static const __m128 VecOne;
	public:
		__m128 C1,C2,C3,C4;
		Matrix4_M128()
		{}
		Matrix4_M128(const Matrix4 & m)
		{
			C1 = _mm_loadu_ps(m.values);
			C2 = _mm_loadu_ps(m.values+4);
			C3 = _mm_loadu_ps(m.values+8);
			C4 = _mm_loadu_ps(m.values+12);
		}
		inline void ToMatrix4(Matrix4 & mOut) const;
		inline void Transform(Vec4_M128 & rs, const Vec4& vIn) const;
		inline void Transform(Vec4 & rs, const Vec4& vIn) const;
		inline void Transform(Vec4_M128 & rs, const Vec3& vIn) const;
		inline void Transform(Vec3 & rs, const Vec3& vIn) const;
		inline void Transform(Vec4_M128 & rs, const Vec4_M128& vIn) const;
		inline void TransformNormal(Vec4_M128 & rs, const Vec4& vIn) const;
		inline void TransformNormal(Vec4 & rs, const Vec4& vIn) const;
		inline void TransformNormal(Vec4_M128 & rs, const Vec3& vIn) const;
		inline void TransformNormal(Vec3 & rs, const Vec3& vIn) const;
		inline void Multiply(Matrix4_M128 & rs, const Matrix4 & mB) const;
		inline void Multiply(Matrix4_M128 & rs, const Matrix4_M128 & mB) const;
		float Inverse(Matrix4_M128 &mOut) const;
	};

	//***********************************************************************
	/**************************** Implementation ****************************/
	//***********************************************************************
	//inline int FloatAsInt(float val)
	//{
	//	union InterCast
	//	{
	//		float fvalue;
	//		int ivalue;
	//	} cast;
	//	cast.fvalue = val;
	//	return cast.ivalue;
	//}
	//inline float IntAsFloat(int val)
	//{
	//	union InterCast
	//	{
	//		float fvalue;
	//		int ivalue;
	//	} cast;
	//	cast.ivalue = val;
	//	return cast.fvalue;
	//}
	inline Vec3 Vec3::FromHomogeneous(const Vec4 & v)
	{
		float invW = 1.0f/v.w;
		return v.xyz()*invW;
	}
	// Vec3
	inline float Vec3::Length() const
	{
		return Vec4::Create(*this, 0.0f).Length();
	}
	inline void Vec3::Normalize(Vec3 & rs, const Vec3 & vin)
	{
		Vec3::NormalizeFPU(rs, vin);
	}
	// Vec4
	inline void Vec4::Add(Vec4 & rs, const Vec4 & v1, const Vec4 & v2)
	{
		rs.x = v1.x + v2.x;
		rs.y = v1.y + v2.y;
		rs.z = v1.z + v2.z;
		rs.w = v1.w + v2.w;
	}
	inline void Vec4::Subtract(Vec4 & rs, const Vec4 & v1, const Vec4 & v2)
	{
		rs.x = v1.x - v2.x;
		rs.y = v1.y - v2.y;
		rs.z = v1.z - v2.z;
		rs.w = v1.w - v2.w;
	}
	inline void Vec4::Multiply(Vec4 & rs, const Vec4 & v1, const Vec4 & v2)
	{
		rs.x = v1.x * v2.x;
		rs.y = v1.y * v2.y;
		rs.z = v1.z * v2.z;
		rs.w = v1.w * v2.w;
	}
	inline void Vec4::MultiplyScale(Vec4 & rs, const Vec4 & v1, const Vec4 & v2)
	{
		rs.x = v1.x * v2.x;
		rs.y = v1.y * v2.y;
		rs.z = v1.z * v2.z;
		rs.w = v1.w * v2.w;
	}
	inline void Vec4::Scale(Vec4 & rs, const Vec4 & v1, float s)
	{
		rs.x = v1.x * s;
		rs.y = v1.y * s;
		rs.z = v1.z * s;
		rs.w = v1.w * s;
	}
	inline float Vec4::Dot(const Vec4 & v1, const Vec4 & v2)
	{
		return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w;
	}
	inline void Vec4::Cross(Vec4 & rs_d, const Vec4 & v1, const Vec4 & v2)
	{
		rs_d.x = v1.y*v2.z - v1.z * v2.y;
		rs_d.y = v1.z*v2.x - v1.x * v2.z;
		rs_d.z = v1.x*v2.y - v1.y * v2.x;
		rs_d.w = 0.0f;
	}
	inline float Vec4::LengthFPU() const
	{
		return sqrt(Dot(*this, *this));
	}
	inline float Vec4::Length() const
	{
#ifdef NO_SIMD_ASM
		return LengthFPU();
#else
		float f;
		_asm
		{
			lea	ecx, f;
			mov	eax, this;

			movups	xmm0, [eax];
			mulps	xmm0, xmm0;
			movaps	xmm1, xmm0;
			shufps	xmm1, xmm1, 4Eh;
			addps	xmm0, xmm1;
			movaps	xmm1, xmm0;
			shufps	xmm1, xmm1, 11h;
			addss	xmm0, xmm1;

			sqrtss	xmm0, xmm0;
			movss	dword ptr [ecx], xmm0;
		}
		return f;
#endif
	}
	inline void Vec4::NormalizeFPU(Vec4& vout, const Vec4& vin)
	{
		float len = 1.0f/vin.Length();
		Scale(vout, vin, len);
	}
	inline void Vec4::Normalize(Vec4 &vout, const Vec4 &vin)
	{
#ifdef NO_SIMD_ASM
		NormalizeFPU(vout, vin);
#else
		__m128 xmm0 = _mm_loadu_ps((float*)&vin);
		__m128 xmm2 = xmm0;
		xmm0 = _mm_mul_ps(xmm0, xmm0);
		__m128 xmm1 = xmm0;
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0x4E);
		xmm0 = _mm_add_ps(xmm0, xmm1);
		xmm1 = xmm0;
		xmm1 = _mm_shuffle_ps(xmm1, xmm1, 0x11);
		xmm0 = _mm_add_ps(xmm0, xmm1);
		xmm0 = _mm_rsqrt_ps(xmm0);
		xmm2 = _mm_mul_ps(xmm2, xmm0);
		_mm_storeu_ps((float*)&vout, xmm2);
#endif
	}

	// Matrix4
	inline void Matrix4::CreateIdentityMatrix(Matrix4 & mOut)
	{
		memset(&mOut, 0, sizeof(Matrix4));
		mOut.m[0][0] = mOut.m[1][1] = mOut.m[2][2] = mOut.m[3][3] = 1.0f;
	}

	inline void Matrix4::CreateOrthoMatrix(Matrix4 & mOut, float left, float right, float top, float bottom, float zNear, float zFar)
	{
		memset(&mOut, 0, sizeof(Matrix4));
		mOut.m[0][0] = 2.0f / (right - left);
		mOut.m[1][1] = 2.0f / (top - bottom);
		mOut.m[2][2] = -2.0f / (zFar - zNear);
		mOut.m[3][0] = -(right + left) / (right - left);
		mOut.m[3][1] = -(top + bottom) / (top - bottom);
		mOut.m[3][2] = -(zFar + zNear) / (zFar - zNear);
		mOut.m[3][3] = 1.0f;
	}

	inline void Matrix4::CreatePerspectiveMatrix(Matrix4 &mOut, float left, float right, float bottom, float top, float znear, float zfar)
	{
		memset(&mOut, 0, sizeof(Matrix4));
		mOut.m[0][0] = (znear*2.0f)/(right-left);
		mOut.m[1][1] = (2.0f*znear)/(top-bottom);
		mOut.m[2][0] = (right+left)/(right-left);
		mOut.m[2][1] = (top+bottom)/(top-bottom);
		mOut.m[2][2] = (zfar+znear)/(znear-zfar);
		mOut.m[2][3] = -1.0f;
		mOut.m[3][2] = 2.0f*zfar*znear/(znear-zfar);
	}

	inline void Matrix4::CreatePerspectiveMatrixFromViewAngle(Matrix4 &mOut, float fovY, float aspect, float zNear, float zFar)
	{
		float xmin, xmax, ymin, ymax;
		ymax = zNear * tan(fovY * CoreLib::Basic::Math::Pi / 360.0f);
		ymin = -ymax;
		xmin = ymin * aspect;
		xmax = ymax * aspect;
		Matrix4::CreatePerspectiveMatrix(mOut, xmin, xmax, ymin, ymax, zNear, zFar);
	}

	inline void Matrix4::CreatePerspectiveMatrixFromViewAngleTiled(Matrix4 &mOut, float fovY, float aspect, float zNear, float zFar, float x0, float y0, float x1, float y1)
	{
		float xmin, xmax, ymin, ymax;
		ymax = zNear * tan(fovY * CoreLib::Basic::Math::Pi / 360.0f);
		ymin = -ymax;
		xmin = ymin * aspect;
		xmax = ymax * aspect;
		x0 *= (xmax - xmin);  x0 += xmin;
		y0 *= (ymax - ymin); y0 += ymin;
		x1 *= (xmax - xmin);  x1 += xmin;
		y1 *= (ymax - ymin); y1 += ymin; 
		Matrix4::CreatePerspectiveMatrix(mOut, x0, x1, y0, y1, zNear, zFar);
	}

	inline void Matrix4::CreateRandomMatrix(Matrix4 & mOut)
	{
		for (int i = 0; i<16; i++)
		{
			mOut.values[i] = rand()/(float)RAND_MAX;
		}
	}
	inline void Matrix4::RotationX(Matrix4 & rs, float angle)
	{
		float c = cosf(angle);
		float s = sinf(angle);

		Matrix4::CreateIdentityMatrix(rs);
		rs.m[1][1] = c;
		rs.m[2][1] = s;
		rs.m[1][2] = -s;
		rs.m[2][2] = c;
	}
	inline void Matrix4::RotationY(Matrix4 & rs, float angle)
	{
		float c = cosf(angle);
		float s = sinf(angle);

		Matrix4::CreateIdentityMatrix(rs);
		rs.m[0][0] = c;
		rs.m[2][0] = s;
		rs.m[0][2] = -s;
		rs.m[2][2] = c;
	}
	inline void Matrix4::RotationZ(Matrix4 & rs, float angle)
	{
		float c = cosf(angle);
		float s = sinf(angle);

		Matrix4::CreateIdentityMatrix(rs);
		rs.m[0][0] = c;
		rs.m[1][0] = s;
		rs.m[0][1] = -s;
		rs.m[1][1] = c;
	}

	inline void Matrix4::Scale(Matrix4 & rs, float sx, float sy, float sz)
	{
		Matrix4::CreateIdentityMatrix(rs);
		rs.m[0][0] = sx;
		rs.m[1][1] = sy;
		rs.m[2][2] = sz;
	}
	inline void Matrix4::Translation(Matrix4 & rs, float tx, float ty, float tz)
	{
		Matrix4::CreateIdentityMatrix(rs);
		rs.values[12] = tx;
		rs.values[13] = ty;
		rs.values[14] = tz;
	}
	inline void Matrix4::TransposeTransformNormal(Vec3 & rs, const Vec3 & vIn) const
	{
		rs.x = m[0][0]*vIn.x + m[0][1]*vIn.y + m[0][2]*vIn.z;
		rs.y = m[1][0]*vIn.x + m[1][1]*vIn.y + m[1][2]*vIn.z;
		rs.z = m[2][0]*vIn.x + m[2][1]*vIn.y + m[2][2]*vIn.z;
	}
	inline void Matrix4::TransposeTransform(Vec3 & rs, const Vec3 & vIn) const
	{
		rs.x = m[0][0]*vIn.x + m[0][1]*vIn.y + m[0][2]*vIn.z + m[0][3];
		rs.y = m[1][0]*vIn.x + m[1][1]*vIn.y + m[1][2]*vIn.z + m[1][3];
		rs.z = m[2][0]*vIn.x + m[2][1]*vIn.y + m[2][2]*vIn.z + m[2][3];
	}
	inline void Matrix4::TransposeTransform(Vec4 & rs, const Vec4 & vIn) const
	{
		rs.x = m[0][0]*vIn.x + m[0][1]*vIn.y + m[0][2]*vIn.z + m[0][3]*vIn.w;
		rs.y = m[1][0]*vIn.x + m[1][1]*vIn.y + m[1][2]*vIn.z + m[1][3]*vIn.w;
		rs.z = m[2][0]*vIn.x + m[2][1]*vIn.y + m[2][2]*vIn.z + m[2][3]*vIn.w;
		rs.w = m[3][0]*vIn.x + m[3][1]*vIn.y + m[3][2]*vIn.z + m[3][3]*vIn.w;
	}
	inline void Matrix4::Transform(Vec3 & rs, const Vec3& vIn) const
	{
		rs.x = m[0][0]*vIn.x + m[1][0]*vIn.y + m[2][0]*vIn.z + m[3][0];
		rs.y = m[0][1]*vIn.x + m[1][1]*vIn.y + m[2][1]*vIn.z + m[3][1];
		rs.z = m[0][2]*vIn.x + m[1][2]*vIn.y + m[2][2]*vIn.z + m[3][2];
	}
	inline void Matrix4::TransformHomogeneous(Vec3 & rs, const Vec3 & vIn) const
	{
		rs.x = m[0][0]*vIn.x + m[1][0]*vIn.y + m[2][0]*vIn.z + m[3][0];
		rs.y = m[0][1]*vIn.x + m[1][1]*vIn.y + m[2][1]*vIn.z + m[3][1];
		rs.z = m[0][2]*vIn.x + m[1][2]*vIn.y + m[2][2]*vIn.z + m[3][2];
		float w = 1.0f/(m[0][3]*vIn.x + m[1][3]*vIn.y + m[2][3]*vIn.z + m[3][3]);
		rs.x *= w;
		rs.y *= w;
		rs.z *= w;
	}
	inline void Matrix4::TransformHomogeneous2D(Vec2 & rs, const Vec3 & vIn) const
	{
		rs.x = m[0][0]*vIn.x + m[1][0]*vIn.y + m[2][0]*vIn.z + m[3][0];
		rs.y = m[0][1]*vIn.x + m[1][1]*vIn.y + m[2][1]*vIn.z + m[3][1];
		float w = 1.0f/(m[0][3]*vIn.x + m[1][3]*vIn.y + m[2][3]*vIn.z + m[3][3]);
		rs.x *= w;
		rs.y *= w;
	}
	inline void Matrix4::TransformNormal(Vec3 & rs, const Vec3& vIn) const
	{
		rs.x = m[0][0]*vIn.x + m[1][0]*vIn.y + m[2][0]*vIn.z;
		rs.y = m[0][1]*vIn.x + m[1][1]*vIn.y + m[2][1]*vIn.z;
		rs.z = m[0][2]*vIn.x + m[1][2]*vIn.y + m[2][2]*vIn.z;
	}
	inline void Matrix4::Transform(Vec4 & rs, const Vec4& vIn) const
	{
		rs.x = m[0][0]*vIn.x + m[1][0]*vIn.y + m[2][0]*vIn.z + m[3][0]*vIn.w;
		rs.y = m[0][1]*vIn.x + m[1][1]*vIn.y + m[2][1]*vIn.z + m[3][1]*vIn.w;
		rs.z = m[0][2]*vIn.x + m[1][2]*vIn.y + m[2][2]*vIn.z + m[3][2]*vIn.w;
		rs.w = m[0][3]*vIn.x + m[1][3]*vIn.y + m[2][3]*vIn.z + m[3][3]*vIn.w;
	}
	inline void Matrix4::MultiplyFPU(Matrix4 &mOut, const Matrix4& M1, const Matrix4& M2)
	{
		Matrix4 TempMat;
		for (int i=0;i<4;i++) //col
		{
			for (int j=0;j<4;j++) // row
			{
				TempMat.m[i][j] = M1.m[0][j]*M2.m[i][0] + M1.m[1][j]*M2.m[i][1] + M1.m[2][j]*M2.m[i][2] + M1.m[3][j]*M2.m[i][3];
			}
		}
		memcpy(&mOut,&TempMat,sizeof(Matrix4));
	}

	inline void Matrix4::Multiply(Matrix4 &mOut, const Matrix4 &M1, const Matrix4 &M2)
	{
		Matrix4 rs;
		Matrix4_M128 TempMat;
		Matrix4_M128 mA(M1);
		mA.Multiply(TempMat, M2);
		TempMat.ToMatrix4(rs);
		mOut = rs;
	}
	inline float Matrix4::Inverse(Matrix4 &mOut_d) const
	{
		Matrix4 mat;
		Matrix4_M128 m_m(*this);
		Matrix4_M128 tmr;
		float rs = m_m.Inverse(tmr);
		tmr.ToMatrix4(mat);
		mOut_d = mat;
		return rs;
	}

	// Matrix4_M128

	inline void Matrix4_M128::ToMatrix4(Matrix4 & mOut) const
	{
		_mm_storeu_ps(mOut.values, C1);
		_mm_storeu_ps(mOut.values+4, C2);
		_mm_storeu_ps(mOut.values+8, C3);
		_mm_storeu_ps(mOut.values+12, C4);
	}
	inline void Matrix4_M128::Transform(Vec4_M128 & rs, const Vec4& vIn) const
	{
		__m128 r;
		r = _mm_mul_ps(C1, _mm_set_ps1(vIn.x));
		r = _mm_add_ps(r, _mm_mul_ps(C2, _mm_set_ps1(vIn.y)));
		r = _mm_add_ps(r, _mm_mul_ps(C3, _mm_set_ps1(vIn.z)));
		r = _mm_add_ps(r, _mm_mul_ps(C4, _mm_set_ps1(vIn.w)));
		rs.vec = r;
	}
	inline void Matrix4_M128::Transform(Vec4 & rs, const Vec4& vIn) const
	{
		Vec4_M128 r;
		Transform(r, vIn);
		_mm_store_ps((float*)&rs, r.vec);
	}
	inline void Matrix4_M128::Transform(Vec4_M128 & rs, const Vec3& vIn) const
	{
		__m128 r;
		r = _mm_mul_ps(C1, _mm_set_ps1(vIn.x));
		r = _mm_add_ps(r, _mm_mul_ps(C2, _mm_set_ps1(vIn.y)));
		r = _mm_add_ps(r, _mm_mul_ps(C3, _mm_set_ps1(vIn.z)));
		rs.vec = r;
	}
	inline void Matrix4_M128::Transform(Vec3 & rs, const Vec3& vIn) const
	{
		Vec4_M128 r;
		Transform(r, vIn);
		Vec4 tmp;
		_mm_store_ps((float*)&tmp, r.vec);
		rs.x = tmp.x;
		rs.y = tmp.y;
		rs.z = tmp.z;
	}
	inline void Matrix4_M128::Transform(Vec4_M128 & rs, const Vec4_M128& vIn) const
	{
		__m128 r;
		__m128 x,y,z,w;
		x = _mm_shuffle_ps(vIn.vec, vIn.vec, _MM_SHUFFLE(0, 0, 0, 0));
		r = _mm_mul_ps(C1, x);
		y = _mm_shuffle_ps(vIn.vec, vIn.vec, _MM_SHUFFLE(1, 1, 1, 1));
		r = _mm_add_ps(r, _mm_mul_ps(C2, y));
		z = _mm_shuffle_ps(vIn.vec, vIn.vec, _MM_SHUFFLE(2, 2, 2, 2));
		r = _mm_add_ps(r, _mm_mul_ps(C3, z));
		w = _mm_shuffle_ps(vIn.vec, vIn.vec, _MM_SHUFFLE(3, 3, 3, 3));
		r = _mm_add_ps(r, _mm_mul_ps(C4, w));
		rs.vec = r;
	}
	inline void Matrix4_M128::TransformNormal(Vec4_M128 & rs, const Vec4& vIn) const
	{
		__m128 r;
		r = _mm_mul_ps(C1, _mm_set_ps1(vIn.x));
		r = _mm_add_ps(r, _mm_mul_ps(C2, _mm_set_ps1(vIn.y)));
		r = _mm_add_ps(r, _mm_mul_ps(C3, _mm_set_ps1(vIn.z)));
		rs.vec = r;
	}
	inline void Matrix4_M128::TransformNormal(Vec4 & rs, const Vec4& vIn) const
	{
		Vec4_M128 r;
		TransformNormal(r, vIn);
		_mm_store_ps((float*)&rs, r.vec);
		rs.w = 0.0f;
	}
	inline void Matrix4_M128::TransformNormal(Vec4_M128 & rs, const Vec3& vIn) const
	{
		__m128 r;
		r = _mm_mul_ps(C1, _mm_set_ps1(vIn.x));
		r = _mm_add_ps(r, _mm_mul_ps(C2, _mm_set_ps1(vIn.y)));
		r = _mm_add_ps(r, _mm_mul_ps(C3, _mm_set_ps1(vIn.z)));
		rs.vec = r;
	}
	inline void Matrix4_M128::TransformNormal(Vec3 & rs, const Vec3& vIn) const
	{
		Vec4_M128 r;
		TransformNormal(r, vIn);
		Vec4 tmp;
		_mm_store_ps((float*)&tmp, r.vec);
		rs = tmp.xyz();
	}
	inline void Matrix4_M128::Multiply(Matrix4_M128 & rs, const Matrix4 & mB) const
	{
		register __m128 T0, T1, T2, T3, R0, R1, R2, R3;
		T0 = _mm_set_ps1(mB.values[0]);
		T1 = _mm_set_ps1(mB.values[1]);
		T2 = _mm_set_ps1(mB.values[2]);
		T3 = _mm_set_ps1(mB.values[3]);
		R0 = _mm_mul_ps(C1, T0);
		T0 = _mm_set_ps1(mB.values[4]);
		R1 = _mm_mul_ps(C2, T1);
		R1 = _mm_add_ps(R1, R0);
		R2 = _mm_mul_ps(C3, T2);
		T1 = _mm_set_ps1(mB.values[5]);
		R3 = _mm_mul_ps(C4, T3);
		R2 = _mm_add_ps(R2, R1);
		T2 = _mm_set_ps1(mB.values[6]);
		rs.C1 = _mm_add_ps(R3, R2);
		R0 = _mm_mul_ps(C1, T0);
		T3 = _mm_set_ps1(mB.values[7]);
		R1 = _mm_mul_ps(C2, T1);
		T0 = _mm_set_ps1(mB.values[8]);
		R2 = _mm_mul_ps(C3, T2);
		R1 = _mm_add_ps(R1, R0);
		T1 = _mm_set_ps1(mB.values[9]);
		R3 = _mm_mul_ps(C4, T3);
		R2 = _mm_add_ps(R2, R1);
		rs.C2 = _mm_add_ps(R3, R2);
		T2 = _mm_set_ps1(mB.values[10]);
		R0 = _mm_mul_ps(C1, T0);
		T3 = _mm_set_ps1(mB.values[11]);
		R1 = _mm_mul_ps(C2, T1);
		T0 = _mm_set_ps1(mB.values[12]);
		R2 = _mm_mul_ps(C3, T2);
		R1 = _mm_add_ps(R1, R0);
		T2 = _mm_set_ps1(mB.values[14]);
		R3 = _mm_mul_ps(C4, T3);
		R2 = _mm_add_ps(R2, R1);
		T1 = _mm_set_ps1(mB.values[13]);
		rs.C3 = _mm_add_ps(R3, R2);
		R0 = _mm_mul_ps(C1, T0);
		R1 = _mm_mul_ps(C2, T1);
		T3 = _mm_set_ps1(mB.values[15]);
		R2 = _mm_mul_ps(C3, T2);
		R1 = _mm_add_ps(R1, R0);
		R3 = _mm_mul_ps(C4, T3);
		R2 = _mm_add_ps(R2, R1);
		rs.C4 = _mm_add_ps(R3, R2);
	}
	inline void Matrix4_M128::Multiply(Matrix4_M128 & rs, const Matrix4_M128 & mB) const
	{
		register __m128 T0, T1, T2, T3, R0, R1, R2, R3;
		T0 = _mm_shuffle_ps(mB.C1, mB.C1, _MM_SHUFFLE(0,0,0,0));
		T1 = _mm_shuffle_ps(mB.C1, mB.C1, _MM_SHUFFLE(1,1,1,1));
		T2 = _mm_shuffle_ps(mB.C1, mB.C1, _MM_SHUFFLE(2,2,2,2));
		T3 = _mm_shuffle_ps(mB.C1, mB.C1, _MM_SHUFFLE(3,3,3,3));
		R0 = _mm_mul_ps(C1, T0);
		R1 = _mm_mul_ps(C2, T1);
		R2 = _mm_mul_ps(C3, T2);
		R3 = _mm_mul_ps(C4, T3);
		R1 = _mm_add_ps(R1, R0);
		R2 = _mm_add_ps(R2, R1);
		rs.C1 = _mm_add_ps(R3, R2);

		T0 = _mm_shuffle_ps(mB.C2, mB.C2, _MM_SHUFFLE(0,0,0,0));
		T1 = _mm_shuffle_ps(mB.C2, mB.C2, _MM_SHUFFLE(1,1,1,1));
		T2 = _mm_shuffle_ps(mB.C2, mB.C2, _MM_SHUFFLE(2,2,2,2));
		T3 = _mm_shuffle_ps(mB.C2, mB.C2, _MM_SHUFFLE(3,3,3,3));
		R0 = _mm_mul_ps(C1, T0);
		R1 = _mm_mul_ps(C2, T1);
		R2 = _mm_mul_ps(C3, T2);
		R3 = _mm_mul_ps(C4, T3);
		R1 = _mm_add_ps(R1, R0);
		R2 = _mm_add_ps(R2, R1);
		rs.C2 = _mm_add_ps(R3, R2);

		T0 = _mm_shuffle_ps(mB.C3, mB.C3, _MM_SHUFFLE(0,0,0,0));
		T1 = _mm_shuffle_ps(mB.C3, mB.C3, _MM_SHUFFLE(1,1,1,1));
		T2 = _mm_shuffle_ps(mB.C3, mB.C3, _MM_SHUFFLE(2,2,2,2));
		T3 = _mm_shuffle_ps(mB.C3, mB.C3, _MM_SHUFFLE(3,3,3,3));
		R0 = _mm_mul_ps(C1, T0);
		R1 = _mm_mul_ps(C2, T1);
		R2 = _mm_mul_ps(C3, T2);
		R3 = _mm_mul_ps(C4, T3);
		R1 = _mm_add_ps(R1, R0);
		R2 = _mm_add_ps(R2, R1);
		rs.C3 = _mm_add_ps(R3, R2);

		T0 = _mm_shuffle_ps(mB.C4, mB.C4, _MM_SHUFFLE(0,0,0,0));
		T1 = _mm_shuffle_ps(mB.C4, mB.C4, _MM_SHUFFLE(1,1,1,1));
		T2 = _mm_shuffle_ps(mB.C4, mB.C4, _MM_SHUFFLE(2,2,2,2));
		T3 = _mm_shuffle_ps(mB.C4, mB.C4, _MM_SHUFFLE(3,3,3,3));
		R0 = _mm_mul_ps(C1, T0);
		R1 = _mm_mul_ps(C2, T1);
		R2 = _mm_mul_ps(C3, T2);
		R3 = _mm_mul_ps(C4, T3);
		R1 = _mm_add_ps(R1, R0);
		R2 = _mm_add_ps(R2, R1);
		rs.C4 = _mm_add_ps(R3, R2);
	}

	inline void CartesianToSphere(const Vec3 & dir, float & u, float & v)
	{
		const float inv2Pi = 0.5f/PI;
		v = acos(dir.y);
		u = atan2(dir.z, dir.x);
		if (u<0.0f)
			u += PI * 2.0f;
		u *= inv2Pi;
		v *= 1.0f/PI;
	}

	inline void SphereToCartesian(Vec3 & dir, float u, float v)
	{
		dir.y = cos(v);
		float s = sin(v);
		dir.x = cos(u) * s;
		dir.z = sin(u) * s;
	}

	inline void GetOrthoVec(Vec3 & vout, const Vec3 & vin)
	{
		Vec3 absV = Vec3::Create(abs(vin.x), abs(vin.y), abs(vin.z));
		if (absV.x <= absV.y && absV.x <= absV.z)
			Vec3::Cross(vout, vin, Vec3::Create(1.0f, 0.0f, 0.0f));
		else if (absV.y <= absV.x && absV.y <= absV.z)
			Vec3::Cross(vout, vin, Vec3::Create(0.0f, 1.0f, 0.0f));
		else
			Vec3::Cross(vout, vin, Vec3::Create(0.0f, 0.0f, 1.0f));
	}

	template<typename T>
	inline T CatmullInterpolate(const T & p0, const T & p1, const T & p2, const T & p3, float t)
	{
		float t2 = t * t;
		float t3 = t2 * t;
		return (p1 * 2.0f + (-p0 + p2) * t +
			(p0 * 2.0f - p1 * 5.0f + p2 * 4.0f - p3) * t2 +
			(-p0 + p1 * 3.0f - p2 * 3.0f + p3) * t3) * 0.5f;
	}

#ifndef M128_OPERATOR_OVERLOADS
#define M128_OPERATOR_OVERLOADS
	inline __m128 & operator += (__m128 & v0, const __m128 &v1)
	{
		v0 = _mm_add_ps(v0, v1);
		return v0;
	}
	inline __m128 & operator -= (__m128 & v0, const __m128 &v1)
	{
		v0 = _mm_sub_ps(v0, v1);
		return v0;
	}
	inline __m128 & operator *= (__m128 & v0, const __m128 &v1)
	{
		v0 = _mm_mul_ps(v0, v1);
		return v0;
	}
	inline __m128 & operator /= (__m128 & v0, const __m128 &v1)
	{
		v0 = _mm_div_ps(v0, v1);
		return v0;
	}
	inline __m128 operator + (const __m128 & v0, const __m128 & v1)
	{
		return _mm_add_ps(v0, v1);
	}
	inline __m128 operator - (const __m128 & v0, const __m128 & v1)
	{
		return _mm_sub_ps(v0, v1);
	}
	inline __m128 operator * (const __m128 & v0, const __m128 & v1)
	{
		return _mm_mul_ps(v0, v1);
	}
	inline __m128 operator / (const __m128 & v0, const __m128 & v1)
	{
		return _mm_div_ps(v0, v1);
	}
	inline __m128 operator - (const __m128 & v0)
	{
		static const __m128 SIGNMASK = 
               _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
		return _mm_xor_ps(v0, SIGNMASK);
	}

	inline __m128i & operator += (__m128i & v0, const __m128i &v1)
	{
		v0 = _mm_add_epi32(v0, v1);
		return v0;
	}
	inline __m128i & operator -= (__m128i & v0, const __m128i &v1)
	{
		v0 = _mm_sub_epi32(v0, v1);
		return v0;
	}
	inline __m128i & operator *= (__m128i & v0, const __m128i &v1)
	{
		v0 = _mm_mul_epi32(v0, v1);
		return v0;
	}
	inline __m128i operator + (const __m128i & v0, const __m128i & v1)
	{
		return _mm_add_epi32(v0, v1);
	}
	inline __m128i operator - (const __m128i & v0, const __m128i & v1)
	{
		return _mm_sub_epi32(v0, v1);
	}
	inline __m128i operator * (const __m128i & v0, const __m128i & v1)
	{
		return _mm_mullo_epi32(v0, v1);
	}
	inline __m128i operator - (const __m128i & v0)
	{
		return _mm_xor_si128(v0, _mm_set1_epi32(0xFFFFFFFF));
	}

	_declspec(align(16))
	class SSEVec3
	{
	public:
		__m128 x,y,z;
		SSEVec3()
		{};
		SSEVec3(__m128 x, __m128 y, __m128 z)
			:x(x), y(y), z(z)
		{
		}
		SSEVec3(const Vec3 &v)
		{
			this->x = _mm_set_ps1(v.x);
			this->y = _mm_set_ps1(v.y);
			this->z = _mm_set_ps1(v.z);
		}
		SSEVec3(float x, float y, float z)
		{
			this->x = _mm_set_ps1(x);
			this->y = _mm_set_ps1(y);
			this->z = _mm_set_ps1(z);
		}
		inline __m128 Length()
		{
			return _mm_sqrt_ps(x*x + y*y + z*z);
		}
		inline void Normalize(__m128 one)
		{
			auto s = one / Length();
			x *= s;
			y *= s;
			z *= s;
		}
		inline SSEVec3 operator + (const SSEVec3 &vin)
		{
			SSEVec3 rs;
			rs.x = x + vin.x;
			rs.y = y + vin.y;
			rs.z = z + vin.z;
			return rs;
		}
		inline SSEVec3 operator - (const SSEVec3 &vin)
		{
			SSEVec3 rs;
			rs.x = x - vin.x;
			rs.y = y - vin.y;
			rs.z = z - vin.z;
			return rs;
		}
		inline SSEVec3 operator - ()
		{
			SSEVec3 rs;
			rs.x = -x;
			rs.y = -y;
			rs.z = -z;
			return rs;
		}
		inline SSEVec3 operator * (__m128 scale)
		{
			SSEVec3 rs;
			rs.x = x * scale;
			rs.y = y * scale;
			rs.z = z * scale;
			return rs;
		}
		inline SSEVec3 & operator += (const SSEVec3 & vin)
		{
			x += vin.x; y += vin.y; z += vin.z;
			return *this;
		}
		inline SSEVec3 & operator -= (const SSEVec3 & vin)
		{
			x -= vin.x; y -= vin.y; z -= vin.z; 
			return *this;
		}
		inline SSEVec3 & operator *= (const SSEVec3 & vin)
		{
			x *= vin.x; y *= vin.y; z *= vin.z;
			return *this;
		}
		inline SSEVec3 & operator *= (__m128 s)
		{
			x *= s; y *= s; z *= s;
			return *this;
		}
		inline SSEVec3 & operator /= (const SSEVec3 & vin)
		{
			x /= vin.x; y /= vin.y; z /= vin.z;
			return *this;
		}
		inline SSEVec3 & operator /= (float s)
		{
			float inv = 1.0f/s;
			return (*this)*=_mm_set_ps1(inv);
		}

		inline static __m128 Dot(const SSEVec3 & v1, const SSEVec3 & v2)
		{
			return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z;
		}
		inline static void Cross(SSEVec3 & rs_d, const SSEVec3 & v1, const SSEVec3 & v2)
		{
			rs_d.x = v1.y*v2.z - v1.z * v2.y;
			rs_d.y = v1.z*v2.x - v1.x * v2.z;
			rs_d.z = v1.x*v2.y - v1.y * v2.x;
		}
	};

	_declspec(align(16))
	class SSEVec4
	{
	public:
		__m128 x, y, z, w;
		SSEVec4()
		{};
		SSEVec4(const __m128 & x, const __m128 & y, const __m128 & z, const __m128 & w)
			:x(x), y(y), z(z), w(w)
		{
		}
		SSEVec4(const Vec4 &v)
		{
			this->x = _mm_set_ps1(v.x);
			this->y = _mm_set_ps1(v.y);
			this->z = _mm_set_ps1(v.z);
			this->w = _mm_set_ps1(v.w);
		}
		SSEVec4(float x, float y, float z, float w)
		{
			this->x = _mm_set_ps1(x);
			this->y = _mm_set_ps1(y);
			this->z = _mm_set_ps1(z);
			this->w = _mm_set_ps1(w);
		}
		inline __m128 Length()
		{
			return _mm_sqrt_ps(x*x + y*y + z*z + w*w);
		}
		inline void Normalize(__m128 one)
		{
			auto s = one / Length();
			x *= s;
			y *= s;
			z *= s;
			w *= s;
		}
		inline SSEVec4 operator + (const SSEVec4 &vin)
		{
			SSEVec4 rs;
			rs.x = x + vin.x;
			rs.y = y + vin.y;
			rs.z = z + vin.z;
			rs.w = w + vin.w;
			return rs;
		}
		inline SSEVec4 operator - (const SSEVec4 &vin)
		{
			SSEVec4 rs;
			rs.x = x - vin.x;
			rs.y = y - vin.y;
			rs.z = z - vin.z;
			rs.w = w - vin.w;
			return rs;
		}
		inline SSEVec4 operator - ()
		{
			SSEVec4 rs;
			rs.x = -x;
			rs.y = -y;
			rs.z = -z;
			rs.w = -w;
			return rs;
		}
		inline SSEVec4 operator * (__m128 scale)
		{
			SSEVec4 rs;
			rs.x = x * scale;
			rs.y = y * scale;
			rs.z = z * scale;
			rs.w = w * scale;
			return rs;
		}
		inline SSEVec4 & operator += (const SSEVec4 & vin)
		{
			x += vin.x; y += vin.y; z += vin.z; w += vin.w;
			return *this;
		}
		inline SSEVec4 & operator -= (const SSEVec4 & vin)
		{
			x -= vin.x; y -= vin.y; z -= vin.z; w -= vin.w;
			return *this;
		}
		inline SSEVec4 & operator *= (const SSEVec4 & vin)
		{
			x *= vin.x; y *= vin.y; z *= vin.z; w *= vin.w;
			return *this;
		}
		inline SSEVec4 & operator *= (__m128 s)
		{
			x *= s; y *= s; z *= s; w *= s;
			return *this;
		}
		inline SSEVec4 & operator /= (const SSEVec4 & vin)
		{
			x /= vin.x; y /= vin.y; z /= vin.z; w /= vin.w;
			return *this;
		}
		inline SSEVec4 & operator /= (float s)
		{
			float inv = 1.0f / s;
			return (*this) *= _mm_set_ps1(inv);
		}

		inline static __m128 Dot(const SSEVec4 & v1, const SSEVec4 & v2)
		{
			return v1.x*v2.x + v1.y*v2.y + v1.z*v2.z + v1.w*v2.w;
		}
	};

	_declspec(align(16))
	class SSEMatrix4
	{
	public:
		__m128 values[16];
		SSEMatrix4()
		{}
		SSEMatrix4(const Matrix4 & mat)
		{
			for (int i = 0; i<16; i++)
				values[i] = _mm_set_ps1(mat.values[i]);
		}
		inline SSEVec3 Transform(SSEVec3 & v)
		{
			SSEVec3 rs;
			rs.x = values[0]*v.x + values[4]*v.y + values[8]*v.z + values[12];
			rs.y = values[1]*v.x + values[5]*v.y + values[9]*v.z + values[13];
			rs.z = values[2]*v.x + values[6]*v.y + values[10]*v.z + values[14];
			auto w = values[3]*v.x + values[7]*v.y + values[11]*v.z + values[15];
			w = _mm_set_ps1(1.0f) / w;
			rs.x *= w;
			rs.y *= w;
			rs.z *= w;
			return rs;
		}
		inline SSEVec3 TransformNonPerspective(SSEVec3 & v)
		{
			SSEVec3 rs;
			rs.x = values[0]*v.x + values[4]*v.y + values[8]*v.z + values[12];
			rs.y = values[1]*v.x + values[5]*v.y + values[9]*v.z + values[13];
			rs.z = values[2]*v.x + values[6]*v.y + values[10]*v.z + values[14];
			return rs;
		}
	};
#endif
}

#endif

/***********************************************************************
CORELIB\LIBIO.H
***********************************************************************/
#ifndef CORE_LIB_IO_H
#define CORE_LIB_IO_H


namespace CoreLib
{
	namespace IO
	{
		class File
		{
		public:
			static bool Exists(const CoreLib::Basic::String & fileName);
			static CoreLib::Basic::String ReadAllText(const CoreLib::Basic::String & fileName);
			static void WriteAllText(const CoreLib::Basic::String & fileName, const CoreLib::Basic::String & text);
		};

		class Path
		{
		public:
#ifdef WIN32
			static const wchar_t PathDelimiter = L'\\';
#else
			static const wchar_t PathDelimiter = L'/';
#endif
			static String TruncateExt(const String & path);
			static String ReplaceExt(const String & path, const wchar_t * newExt);
			static String GetFileName(const String & path);
			static String GetFileNameWithoutEXT(const String & path);
			static String GetFileExt(const String & path);
			static String GetDirectoryName(const String & path);
			static String Combine(const String & path1, const String & path2);
			static String Combine(const String & path1, const String & path2, const String & path3);
#ifdef CreateDirectory
#undef CreateDirectory
#endif
			static bool CreateDirectory(const String & path);
		};
	}
}

#endif

/***********************************************************************
SPIRECORE\COMPILEERROR.H
***********************************************************************/
#ifndef RASTER_RENDERER_COMPILE_ERROR_H
#define RASTER_RENDERER_COMPILE_ERROR_H


namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;

		class CodePosition
		{
		public:
			int Line = -1, Col = -1;
			String FileName;
			String ToString()
			{
				StringBuilder sb(100);
				sb << FileName;
				if (Line != -1)
					sb << L"(" << Line << L")";
				return sb.ProduceString();
			}
			CodePosition() = default;
			CodePosition(int line, int col, String fileName)
			{
				Line = line;
				Col = col;
				this->FileName = fileName;
			}
			bool operator < (const CodePosition & pos) const
			{
				return FileName < pos.FileName || (FileName == pos.FileName && Line < pos.Line) ||
					(FileName == pos.FileName && Line == pos.Line && Col < pos.Col);
			}
			bool operator == (const CodePosition & pos) const
			{
				return FileName == pos.FileName && Line == pos.Line && Col == pos.Col;
			}
		};

		class CompileError
		{
		public:
			String Message;
			CodePosition Position;
			int ErrorID;

			CompileError()
			{
				ErrorID = -1;
			}
			CompileError(const String & msg, int id,
						const CodePosition & pos)
			{
				Message = msg;
				ErrorID = id;
				Position = pos;
			}
		};

		class ErrorWriter
		{
		private:
			List<CompileError> & errors;
			List<CompileError> & warnings;
		public:
			ErrorWriter(List<CompileError> & perrors, List<CompileError> & pwarnings)
				: errors(perrors), warnings(pwarnings)
			{}
			void Error(int id, const String & msg, const CodePosition & pos)
			{
				errors.Add(CompileError(msg, id, pos));
			}
			void Warning(int id, const String & msg, const CodePosition & pos)
			{
				warnings.Add(CompileError(msg, id, pos));
			}
			int GetErrorCount()
			{
				return errors.Count();
			}
		};
	}
}

#endif

/***********************************************************************
SPIRECORE\LEXER.H
***********************************************************************/
#ifndef RASTER_RENDERER_LEXER_H
#define RASTER_RENDERER_LEXER_H


namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;

		enum class TokenType
		{
			// illegal
			Unkown,
			// identifier
			Identifier,
			KeywordReturn, KeywordBreak, KeywordContinue,
			KeywordIf, KeywordElse, KeywordFor, KeywordWhile, KeywordDo,
			// constant
			IntLiterial, DoubleLiterial, StringLiterial, CharLiterial,
			// operators
			Semicolon, Comma, Dot, LBrace, RBrace, LBracket, RBracket, LParent, RParent,
			OpAssign, OpAdd, OpSub, OpMul, OpDiv, OpMod, OpNot, OpBitNot, OpLsh, OpRsh, 
			OpEql, OpNeq, OpGreater, OpLess, OpGeq, OpLeq,
			OpAnd, OpOr, OpBitXor, OpBitAnd, OpBitOr,
			OpInc, OpDec, OpAddAssign, OpSubAssign, OpMulAssign, OpDivAssign, OpModAssign,
			
			QuestionMark, Colon, RightArrow, At,
		};

		String TokenTypeToString(TokenType type);

		class Token
		{
		public:
			TokenType Type = TokenType::Unkown;
			String Content;
			CodePosition Position;
			Token() = default;
			Token(TokenType type, const String & content, int line, int col, String fileName)
			{
				Type = type;
				Content = content;
				Position = CodePosition(line, col, fileName);
			}
		};

		class Lexer
		{
		public:
			List<Token> Parse(const String & fileName, const String & str, List<CompileError> & errorList);
		};
	}
}

#endif

/***********************************************************************
SPIRECORE\SYNTAX.H
***********************************************************************/
#ifndef RASTER_RENDERER_SYNTAX_H
#define RASTER_RENDERER_SYNTAX_H


namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;
		class SyntaxVisitor;
		class FunctionSyntaxNode;

		enum class VariableModifier
		{
			None = 0,
			Uniform = 1,
			Out = 2,
			In = 4,
			Centroid = 128,
			Const = 16,
			Instance = 1024,
			Builtin = 256,
			Parameter = 513
		};

		enum class BaseType
		{
			Void = 0,
			Int = 16, Int2 = 17, Int3 = 18, Int4 = 19,
			Float = 32, Float2 = 33, Float3 = 34, Float4 = 35,
			Float3x3 = 40, Float4x4 = 47,
			Texture2D = 48,
			TextureShadow = 49,
			TextureCube = 50,
			TextureCubeShadow = 51,
			Function = 64,
			Bool = 128,
			Shader = 256,
			Error = 512
		};

		inline const wchar_t * BaseTypeToString(BaseType t)
		{
			switch (t)
			{
			case BaseType::Void:
				return L"void";
			case BaseType::Bool:
			case BaseType::Int:
				return L"int";
			case BaseType::Int2:
				return L"int2";
			case BaseType::Int3:
				return L"int3";
			case BaseType::Int4:
				return L"int4";
			case BaseType::Float:
				return L"float";
			case BaseType::Float2:
				return L"float2";
			case BaseType::Float3:
				return L"float3";
			case BaseType::Float4:
				return L"float4";
			case BaseType::Float3x3:
				return L"float3x3";
			case BaseType::Float4x4:
				return L"float4x4";
			case BaseType::Texture2D:
				return L"sampler2D";
			case BaseType::TextureCube:
				return L"samplerCube";
			case BaseType::TextureShadow:
				return L"sampler2DShadow";
			case BaseType::TextureCubeShadow:
				return L"samplerCubeShadow";
			default:
				return L"<err-type>";
			}
		}

		inline bool IsVector(BaseType type)
		{
			return (((int)type) & 15) != 0;
		}

		inline int GetVectorSize(BaseType type)
		{
			return (((int)type) & 15) + 1;
		}

		inline BaseType GetVectorBaseType(BaseType type)
		{
			return (BaseType)(((int)type) & (~15));
		}

		inline bool IsTextureType(BaseType type)
		{
			return type == BaseType::Texture2D || type == BaseType::TextureCube || type == BaseType::TextureCubeShadow || type == BaseType::TextureShadow;
		}
		class ShaderSymbol;
		class ShaderClosure;
		class ExpressionType
		{
		public:
			bool IsLeftValue;
			bool IsReference;
			BaseType BaseType;
			bool IsArray = false;
			int ArrayLength = 0;
			ShaderSymbol * Shader = nullptr;
			ShaderClosure * ShaderClosure = nullptr;
			FunctionSyntaxNode * Func = nullptr;
			ExpressionType GetBaseType()
			{
				ExpressionType rs;
				rs.IsLeftValue = IsLeftValue;
				rs.BaseType = BaseType;
				rs.IsArray = false;
				rs.IsReference = false;
				rs.ArrayLength = 0;
				rs.Func = Func;
				return rs;
			}
			ExpressionType()
			{
				BaseType = BaseType::Int;
				ArrayLength = 0;
				IsArray = false;
				Func = 0;
				IsLeftValue = false;
				IsReference = false;
			}
			bool IsTextureType()
			{
				return !IsArray && (BaseType == BaseType::Texture2D || BaseType == BaseType::TextureCube || BaseType == BaseType::TextureCubeShadow || BaseType == BaseType::TextureShadow);
			}
			int GetSize()
			{
				int baseSize = GetVectorSize(BaseType);
				if (BaseType == BaseType::Texture2D || BaseType == BaseType::TextureCube ||
					BaseType == BaseType::TextureCubeShadow || BaseType == BaseType::TextureShadow)
					baseSize = sizeof(void*) / sizeof(int);
				if (ArrayLength == 0)
					return baseSize;
				else
					return ArrayLength*baseSize;
			}
			ExpressionType(Spire::Compiler::BaseType baseType)
			{
				BaseType = baseType;
				ArrayLength = 0;
				IsArray = false;
				Func = 0;
				IsLeftValue = false;
				IsReference = false;
			}

			static ExpressionType Bool;
			static ExpressionType Int;
			static ExpressionType Int2;
			static ExpressionType Int3;
			static ExpressionType Int4;
			static ExpressionType Float;
			static ExpressionType Float2;
			static ExpressionType Float3;
			static ExpressionType Float4;
			static ExpressionType Void;
			static ExpressionType Error;

			bool operator == (const ExpressionType & type)
			{
				return (type.BaseType == BaseType &&
						type.IsArray == IsArray &&
						type.ArrayLength == ArrayLength &&
						type.Func == Func &&
						type.Shader == Shader);
			}

			bool operator != (const ExpressionType & type)
			{
				return !(this->operator==(type));
			}

			bool IsVectorType()
			{
				return (!IsArray) && (IsVector(BaseType));
			}

			CoreLib::Basic::String ToString();
		};
		
		class Type
		{
		public:
			ExpressionType DataType;
			// ContrainedWorlds: Implementation must be defined at at least one of of these worlds in order to satisfy global dependency
			// FeasibleWorlds: The component can be computed at any of these worlds
			EnumerableHashSet<String> ConstrainedWorlds, FeasibleWorlds;
			EnumerableHashSet<String> PinnedWorlds; 
		};


		class VariableEntry
		{
		public:
			String Name;
			Type Type;
			bool IsComponent = false;
		};

		class Scope
		{
		public:
			Scope * Parent;
			Dictionary<String, VariableEntry> Variables;
			bool FindVariable(const String & name, VariableEntry & variable);
			Scope()
				: Parent(0)
			{}
		};

		class CloneContext
		{
		public:
			Dictionary<Spire::Compiler::Scope*, RefPtr<Spire::Compiler::Scope>> ScopeTranslateTable;
		};

		class SyntaxNode : public Object
		{
		protected:
			template<typename T>
			T* CloneSyntaxNodeFields(T * target, CloneContext & ctx)
			{
				if (this->Scope)
				{
					RefPtr<Spire::Compiler::Scope> newScope;
					if (ctx.ScopeTranslateTable.TryGetValue(this->Scope.Ptr(), newScope))
						target->Scope = newScope;
					else
					{
						target->Scope = new Spire::Compiler::Scope(*this->Scope);
						ctx.ScopeTranslateTable[this->Scope.Ptr()] = target->Scope;
						RefPtr<Spire::Compiler::Scope> parentScope;
						if (ctx.ScopeTranslateTable.TryGetValue(target->Scope->Parent, parentScope))
							target->Scope->Parent = parentScope.Ptr();
					}
					
				}
				target->Position = this->Position;
				Tags = this->Tags;
				return target;
			}
		public:
			EnumerableDictionary<String, RefPtr<Object>> Tags;
			CodePosition Position;
			RefPtr<Scope> Scope;
			virtual void Accept(SyntaxVisitor * visitor) = 0;
			virtual SyntaxNode * Clone(CloneContext & ctx) = 0;
		};

		class TypeSyntaxNode : public SyntaxNode
		{
		public:
			bool IsArray;
			String TypeName;
			int ArrayLength;
			String GenericBaseType;
			virtual void Accept(SyntaxVisitor * visitor);
			TypeSyntaxNode()
			{
				ArrayLength = 0;
				IsArray = false;
			}
			
			static TypeSyntaxNode * FromExpressionType(ExpressionType t);

			ExpressionType ToExpressionType()
			{
				ExpressionType expType;
				if (TypeName == "int")
					expType.BaseType = BaseType::Int;
				else if (TypeName == "float")
					expType.BaseType = BaseType::Float;
				else if (TypeName == "ivec2")
					expType.BaseType = BaseType::Int2;
				else if (TypeName == "ivec3")
					expType.BaseType = BaseType::Int3;
				else if (TypeName == "ivec4")
					expType.BaseType = BaseType::Int4;
				else if (TypeName == "vec2")
					expType.BaseType = BaseType::Float2;
				else if (TypeName == "vec3")
					expType.BaseType = BaseType::Float3;
				else if (TypeName == "vec4")
					expType.BaseType = BaseType::Float4;
				else if (TypeName == "mat3")
					expType.BaseType = BaseType::Float3x3;
				else if (TypeName == "mat4")
					expType.BaseType = BaseType::Float4x4;
				else if (TypeName == L"sampler2D")
					expType.BaseType = BaseType::Texture2D;
				else if (TypeName == L"samplerCube")
					expType.BaseType = BaseType::TextureCube;
				else if (TypeName == L"sampler2DShadow")
					expType.BaseType = BaseType::TextureShadow;
				else if (TypeName == L"samplerCubeShadow")
					expType.BaseType = BaseType::TextureCubeShadow;
				else if (TypeName == "void")
					expType.BaseType = BaseType::Void;
				expType.ArrayLength = ArrayLength;
				expType.IsArray = IsArray;
				return expType;
			}
			virtual TypeSyntaxNode * Clone(CloneContext & ctx)
			{
				return CloneSyntaxNodeFields(new TypeSyntaxNode(*this), ctx);
			}
		};


		enum class ExpressionAccess
		{
			Read, Write
		};

		class ExpressionSyntaxNode : public SyntaxNode
		{
		public:
			ExpressionType Type;
			ExpressionAccess Access;
			ExpressionSyntaxNode()
			{
				Access = ExpressionAccess::Read;
			}
			ExpressionSyntaxNode(const ExpressionSyntaxNode & expr) = default;
		};

		class ParameterSyntaxNode : public SyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> Type;
			String Name;
			RefPtr<ExpressionSyntaxNode> Expr;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual ParameterSyntaxNode * Clone(CloneContext & ctx);
		};

		class ChoiceValueSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			String WorldName, AlternateName;
			virtual void Accept(SyntaxVisitor *) {}
			virtual ChoiceValueSyntaxNode * Clone(CloneContext & ctx);
		};

		class VarExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			String Variable;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual VarExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		class ConstantExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			enum class ConstantType
			{
				Int, Float
			};
			ConstantType ConstType;
			union
			{
				int IntValue;
				float FloatValue;
			};
			virtual void Accept(SyntaxVisitor * visitor);
			virtual ConstantExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		enum class Operator
		{
			Neg, Not, BitNot, PreInc, PreDec, PostInc, PostDec,
			Mul, Div, Mod,
			Add, Sub, 
			Lsh, Rsh,
			Eql, Neq, Greater, Less, Geq, Leq,
			BitAnd, BitXor, BitOr,
			And,
			Or,
			Assign, AddAssign, SubAssign, MulAssign, DivAssign, ModAssign
		};
		
		class UnaryExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			Operator Operator;
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual UnaryExpressionSyntaxNode * Clone(CloneContext & ctx);
		};
		
		class BinaryExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			Operator Operator;
			RefPtr<ExpressionSyntaxNode> LeftExpression;
			RefPtr<ExpressionSyntaxNode> RightExpression;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual BinaryExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		class IndexExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> BaseExpression;
			RefPtr<ExpressionSyntaxNode> IndexExpression;
			virtual IndexExpressionSyntaxNode * Clone(CloneContext & ctx);
			virtual void Accept(SyntaxVisitor * visitor);
		};

		class MemberExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> BaseExpression;
			String MemberName;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual MemberExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		class InvokeExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<VarExpressionSyntaxNode> FunctionExpr;
			List<RefPtr<ExpressionSyntaxNode>> Arguments;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual InvokeExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		class TypeCastExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> TargetType;
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual TypeCastExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		class SelectExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> SelectorExpr, Expr0, Expr1;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual SelectExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		class StatementSyntaxNode : public SyntaxNode
		{
		};

		class EmptyStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			virtual void Accept(SyntaxVisitor * visitor);
			virtual EmptyStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class BlockStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			List<RefPtr<StatementSyntaxNode>> Statements;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual BlockStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class VariableDeclr
		{
		public:
			ExpressionType Type;
			String Name;

			bool operator ==(const VariableDeclr & var)
			{
				return Name == var.Name;
			}
			bool operator ==(const String & name)
			{
				return name == Name;
			}
		};
		class FunctionSyntaxNode : public SyntaxNode
		{
		public:
			String Name, InternalName;
			RefPtr<TypeSyntaxNode> ReturnType;
			List<RefPtr<ParameterSyntaxNode>> Parameters;
			RefPtr<BlockStatementSyntaxNode> Body;
			List<VariableDeclr> Variables;
			bool IsInline;
			bool IsExtern;
			bool HasSideEffect;
			virtual void Accept(SyntaxVisitor * visitor);
			FunctionSyntaxNode()
			{
				IsInline = false;
				IsExtern = false;
				HasSideEffect = true;
			}

			virtual FunctionSyntaxNode * Clone(CloneContext & ctx);
		};

		struct Variable : public SyntaxNode
		{
			String Name;
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual Variable * Clone(CloneContext & ctx);
		};

		class VarDeclrStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> Type;
			String LayoutString;
			List<RefPtr<Variable>> Variables;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual VarDeclrStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class RateWorld
		{
		public:
			Token World;
			bool Pinned = false;
		};

		class RateSyntaxNode : public SyntaxNode
		{
		public:
			List<RateWorld> Worlds;
			virtual void Accept(SyntaxVisitor *) override {}
			virtual RateSyntaxNode * Clone(CloneContext & ctx);
		};

		class ShaderMemberNode : public SyntaxNode
		{};

		class ComponentSyntaxNode : public ShaderMemberNode
		{
		public:
			bool IsOutput = false, IsPublic = false, IsInline = false, IsParam = false;
			RefPtr<TypeSyntaxNode> Type;
			RefPtr<RateSyntaxNode> Rate;
			Token Name, AlternateName;
			EnumerableDictionary<String, String> LayoutAttributes;
			RefPtr<BlockStatementSyntaxNode> BlockStatement;
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual ComponentSyntaxNode * Clone(CloneContext & ctx);
		};

		class WorldSyntaxNode : public SyntaxNode
		{
		public:
			bool IsAbstract = false;
			Token Name;
			Token ExportOperator;
			String TargetMachine;
			List<Token> Usings;
			EnumerableDictionary<String, String> LayoutAttributes;
			virtual void Accept(SyntaxVisitor *) override {}
			virtual WorldSyntaxNode * Clone(CloneContext & ctx);
		};

		class ImportOperatorDefSyntaxNode : public SyntaxNode
		{
		public:
			Token Name;
			Token SourceWorld, DestWorld;
			List<Token> Usings;
			EnumerableDictionary<String, String> LayoutAttributes;
			EnumerableDictionary<String, String> Arguments;
			virtual void Accept(SyntaxVisitor *) override {}
			virtual ImportOperatorDefSyntaxNode * Clone(CloneContext & ctx);
		};
		
		class PipelineSyntaxNode : public SyntaxNode
		{
		public:
			Token Name;
			List<RefPtr<WorldSyntaxNode>> Worlds;
			List<RefPtr<ImportOperatorDefSyntaxNode>> ImportOperators;
			List<RefPtr<ComponentSyntaxNode>> AbstractComponents;
			virtual void Accept(SyntaxVisitor *) override {}
			virtual PipelineSyntaxNode * Clone(CloneContext & ctx);
		};

		class ImportArgumentSyntaxNode : public SyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Expression;
			Token ArgumentName;
			virtual void Accept(SyntaxVisitor *) override;
			virtual ImportArgumentSyntaxNode * Clone(CloneContext & ctx);
		};

		class ImportSyntaxNode : public ShaderMemberNode
		{
		public:
			bool IsInplace = false;
			bool IsPublic = false;
			Token ShaderName;
			Token ObjectName;
			List<RefPtr<ImportArgumentSyntaxNode>> Arguments;
			virtual void Accept(SyntaxVisitor *) override;
			virtual ImportSyntaxNode * Clone(CloneContext & ctx) override;

		};

		class ShaderSyntaxNode : public SyntaxNode
		{
		public:
			Token Name;
			Token Pipeline;
			List<RefPtr<ShaderMemberNode>> Members;
			bool IsModule = false;
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual ShaderSyntaxNode * Clone(CloneContext & ctx);
		};

		class ProgramSyntaxNode : public SyntaxNode
		{
		public:
			List<Token> Usings;
			List<RefPtr<FunctionSyntaxNode>> Functions;
			List<RefPtr<PipelineSyntaxNode>> Pipelines;
			List<RefPtr<ShaderSyntaxNode>> Shaders;
			void Include(ProgramSyntaxNode * other)
			{
				Functions.AddRange(other->Functions);
				Pipelines.AddRange(other->Pipelines);
				Shaders.AddRange(other->Shaders);
			}
			virtual void Accept(SyntaxVisitor * visitor);
			virtual ProgramSyntaxNode * Clone(CloneContext & ctx);
		};

		class ImportStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ImportSyntaxNode> Import;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual ImportStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class IfStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Predicate;
			RefPtr<StatementSyntaxNode> PositiveStatement;
			RefPtr<StatementSyntaxNode> NegativeStatement;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual IfStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class ForStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> TypeDef;
			Token IterationVariable;

			RefPtr<ExpressionSyntaxNode> InitialExpression, StepExpression, EndExpression;
			RefPtr<StatementSyntaxNode> Statement;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual ForStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class WhileStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Predicate;
			RefPtr<StatementSyntaxNode> Statement;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual WhileStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class DoWhileStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<StatementSyntaxNode> Statement;
			RefPtr<ExpressionSyntaxNode> Predicate;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual DoWhileStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class BreakStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			virtual void Accept(SyntaxVisitor * visitor);
			virtual BreakStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class ContinueStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			virtual void Accept(SyntaxVisitor * visitor);
			virtual ContinueStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class ReturnStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual ReturnStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class ExpressionStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual ExpressionStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class SyntaxVisitor : public Object
		{
		protected:
			ErrorWriter * err = nullptr;
			void Error(int id, const String & text, SyntaxNode * node)
			{
				err->Error(id, text, node->Position);
			}
			void Error(int id, const String & text, Token node)
			{
				err->Error(id, text, node.Position);
			}
			void Warning(int id, const String & text, SyntaxNode * node)
			{
				err->Warning(id, text, node->Position);
			}
			void Warning(int id, const String & text, Token node)
			{
				err->Warning(id, text, node.Position);
			}
		public:
			SyntaxVisitor(ErrorWriter * pErr)
				: err(pErr)
			{}
			virtual void VisitProgram(ProgramSyntaxNode * program)
			{
				program->Functions.ForEach([&](RefPtr<FunctionSyntaxNode> f){f->Accept(this);});
			}
			virtual void VisitShader(ShaderSyntaxNode * shader)
			{
				for (auto & comp : shader->Members)
					comp->Accept(this);
			}
			virtual void VisitComponent(ComponentSyntaxNode * comp)
			{
				if (comp->Expression)
					comp->Expression->Accept(this);
				if (comp->BlockStatement)
					comp->BlockStatement->Accept(this);
			}
			virtual void VisitFunction(FunctionSyntaxNode* func)
			{
				func->ReturnType->Accept(this);
				for (auto & param : func->Parameters)
					param->Accept(this);
				if (func->Body)
					func->Body->Accept(this);
			}
			virtual void VisitBlockStatement(BlockStatementSyntaxNode* stmt)
			{
				for (auto & s : stmt->Statements)
					s->Accept(this);
			}
			virtual void VisitBreakStatement(BreakStatementSyntaxNode*){}
			virtual void VisitContinueStatement(ContinueStatementSyntaxNode*){}

			virtual void VisitDoWhileStatement(DoWhileStatementSyntaxNode* stmt)
			{
				if (stmt->Predicate)
					stmt->Predicate->Accept(this);
				if (stmt->Statement)
					stmt->Statement->Accept(this);
			}
			virtual void VisitEmptyStatement(EmptyStatementSyntaxNode*){}
			virtual void VisitForStatement(ForStatementSyntaxNode* stmt)
			{
				if (stmt->InitialExpression)
					stmt->InitialExpression->Accept(this);
				if (stmt->StepExpression)
					stmt->StepExpression->Accept(this);
				if (stmt->EndExpression)
					stmt->EndExpression->Accept(this);
				if (stmt->Statement)
					stmt->Statement->Accept(this);
			}
			virtual void VisitIfStatement(IfStatementSyntaxNode* stmt)
			{
				if (stmt->Predicate)
					stmt->Predicate->Accept(this);
				if (stmt->PositiveStatement)
					stmt->PositiveStatement->Accept(this);
				if (stmt->NegativeStatement)
					stmt->NegativeStatement->Accept(this);
			}
			virtual void VisitReturnStatement(ReturnStatementSyntaxNode* stmt)
			{
				if (stmt->Expression)
					stmt->Expression->Accept(this);
			}
			virtual void VisitVarDeclrStatement(VarDeclrStatementSyntaxNode* stmt)
			{
				for (auto & var : stmt->Variables)
					var->Accept(this);
			}
			virtual void VisitWhileStatement(WhileStatementSyntaxNode* stmt)
			{
				if (stmt->Predicate)
					stmt->Predicate->Accept(this);
				if (stmt->Statement)
					stmt->Statement->Accept(this);
			}
			virtual void VisitExpressionStatement(ExpressionStatementSyntaxNode* stmt)
			{
				if (stmt->Expression)
					stmt->Expression->Accept(this);
			}

			virtual void VisitBinaryExpression(BinaryExpressionSyntaxNode* expr)
			{
				if (expr->LeftExpression)
					expr->LeftExpression->Accept(this);
				if (expr->RightExpression)
					expr->RightExpression->Accept(this);
			}
			virtual void VisitConstantExpression(ConstantExpressionSyntaxNode*) {}
			virtual void VisitIndexExpression(IndexExpressionSyntaxNode* expr)
			{
				if (expr->BaseExpression)
					expr->BaseExpression->Accept(this);
				if (expr->IndexExpression)
					expr->IndexExpression->Accept(this);
			}
			virtual void VisitMemberExpression(MemberExpressionSyntaxNode * stmt)
			{
				if (stmt->BaseExpression)
					stmt->BaseExpression->Accept(this);
			}
			virtual void VisitInvokeExpression(InvokeExpressionSyntaxNode* stmt)
			{
				for (auto & arg : stmt->Arguments)
					arg->Accept(this);
			}
			virtual void VisitTypeCastExpression(TypeCastExpressionSyntaxNode * stmt)
			{
				if (stmt->Expression)
					stmt->Expression->Accept(this);
			}
			virtual void VisitSelectExpression(SelectExpressionSyntaxNode * expr)
			{
				if (expr->SelectorExpr)
					expr->SelectorExpr->Accept(this);
				if (expr->Expr0)
					expr->Expr0->Accept(this);
				if (expr->Expr1)
					expr->Expr1->Accept(this);
			}
			virtual void VisitUnaryExpression(UnaryExpressionSyntaxNode* expr)
			{
				if (expr->Expression)
					expr->Expression->Accept(this);
			}
			virtual void VisitVarExpression(VarExpressionSyntaxNode*){}
			virtual void VisitParameter(ParameterSyntaxNode*){}
			virtual void VisitType(TypeSyntaxNode*){}
			virtual void VisitDeclrVariable(Variable* dclr)
			{
				if (dclr->Expression)
					dclr->Expression->Accept(this);
			}
			virtual void VisitImport(ImportSyntaxNode* imp)
			{
				for (auto & arg : imp->Arguments)
					if (arg->Expression)
						arg->Expression->Accept(this);
			}
			virtual void VisitImportStatement(ImportStatementSyntaxNode* stmt)
			{
				if (stmt->Import)
					stmt->Import->Accept(this);
			}
			virtual void VisitImportArgument(ImportArgumentSyntaxNode * arg)
			{
				if (arg->Expression)
					arg->Expression->Accept(this);
			}

		};
	}
}

#endif

/***********************************************************************
SPIRECORE\SYMBOLTABLE.H
***********************************************************************/
#ifndef RASTER_RENDERER_SYMBOL_TABLE_H
#define RASTER_RENDERER_SYMBOL_TABLE_H


namespace Spire
{
	namespace Compiler
	{
		
		class FunctionSymbol
		{
		public:
			FunctionSyntaxNode * SyntaxNode;
			EnumerableHashSet<String> ReferencedFunctions;
		};
		class ShaderComponentSymbol;
		class ShaderComponentImplSymbol : public Object
		{
		public:
			String AlternateName;
			EnumerableHashSet<String> Worlds, ExportWorlds, SrcPinnedWorlds;
			RefPtr<ComponentSyntaxNode> SyntaxNode;
			EnumerableHashSet<ShaderComponentSymbol *> DependentComponents;
			EnumerableDictionary<ShaderComponentSymbol *, CodePosition> ComponentReferencePositions;
			ShaderComponentImplSymbol() = default;
			ShaderComponentImplSymbol(const ShaderComponentImplSymbol & other)
			{
				AlternateName = other.AlternateName;
				Worlds = other.Worlds;
				ExportWorlds = other.ExportWorlds;
				SrcPinnedWorlds = other.SrcPinnedWorlds;
				CloneContext ctx;
				SyntaxNode = other.SyntaxNode->Clone(ctx);
			}
		};

		class ShaderComponentSymbol : public Object
		{
		public:
			bool IsDceEntryPoint = false;
			String Name, UniqueName, UniqueKey;
			List<String> ChoiceNames;
			EnumerableHashSet<ShaderComponentSymbol *> DependentComponents, UserComponents;
			List<RefPtr<ShaderComponentImplSymbol>> Implementations;
			RefPtr<Type> Type;
			bool IsParam()
			{
				for (auto & impl : Implementations)
					if (impl->SyntaxNode->IsParam)
						return true;
				return false;
			}
			ShaderComponentSymbol() = default;
			ShaderComponentSymbol(const ShaderComponentSymbol & other)
			{
				Type = new Spire::Compiler::Type(*other.Type);
				for (auto &impl : other.Implementations)
					this->Implementations.Add(new ShaderComponentImplSymbol(*impl));
				this->Name = other.Name;
			}
		};
		
		class WorldSymbol
		{
		public:
			bool IsAbstract = false;
			WorldSyntaxNode * SyntaxNode = nullptr;
		};

		class PipelineSymbol;

		class ComponentDefinitionIR
		{
		public:
			ShaderComponentSymbol * Component;
			ShaderComponentImplSymbol * Implementation;
			String World;
			bool IsEntryPoint = false;
			EnumerableHashSet<ComponentDefinitionIR*> Users, Dependency; // Bidirectional dependency;
		};
		
		class ShaderClosure;

		class ShaderIR
		{
		public:
			ShaderClosure * Shader;
			List<RefPtr<ComponentDefinitionIR>> Definitions;
			EnumerableDictionary<String, EnumerableDictionary<String, ComponentDefinitionIR*>> DefinitionsByComponent;
			void EliminateDeadCode(); // returns remaining definitions in reverse dependency order
			void ResolveComponentReference(); // resolve reference and build dependency map
			List<ShaderComponentSymbol*> GetComponentDependencyOrder();
			template<typename ShouldRemoveFunc>
			void RemoveDefinitions(const ShouldRemoveFunc &shouldRemove)
			{
				List<RefPtr<ComponentDefinitionIR>> newDefinitions;
				for (auto & def : Definitions)
				{
					if (!shouldRemove(def.Ptr()))
					{
						newDefinitions.Add(def);
					}
				}
				Definitions = _Move(newDefinitions);
				for (auto & kv : DefinitionsByComponent)
				{
					for (auto & def : kv.Value)
						if (shouldRemove(def.Value))
							kv.Value.Remove(def.Key);
				}
			}

		};
		
		class ShaderSymbol;

		class ShaderUsing
		{
		public:
			ShaderSymbol * Shader;
			bool IsPublic;
		};

		class ShaderSymbol
		{
		public:
			bool IsAbstract = false;
			ShaderSyntaxNode * SyntaxNode = nullptr;
			PipelineSymbol * Pipeline = nullptr;
			EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>> Components;
			List<ShaderComponentSymbol*> GetComponentDependencyOrder();
			EnumerableHashSet<ShaderSymbol*> DependentShaders;
			List<ShaderUsing> ShaderUsings;
			EnumerableDictionary<String, ShaderUsing> ShaderObjects;
			void SortComponents(List<ShaderComponentSymbol*> & comps);
			struct ComponentReference
			{
				ShaderComponentSymbol * Component;
				bool IsAccessible = false;
			};
			ComponentReference ResolveComponentReference(String compName, bool topLevel = true);
		};

		class ShaderClosure : public Object
		{
		public:
			ShaderClosure * Parent = nullptr;
			CodePosition Position;
			PipelineSymbol * Pipeline = nullptr;
			bool IsInPlace = false;
			bool IsPublic = false;
			String Name;
			CodePosition UsingPosition;
			Dictionary<String, RefPtr<ShaderComponentSymbol>> RefMap;
			EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>> Components;
			EnumerableDictionary<String, ShaderComponentSymbol *> AllComponents;
			EnumerableDictionary<String, RefPtr<ShaderClosure>> SubClosures;
			RefPtr<ShaderComponentSymbol> FindComponent(String name, bool findInPrivate = false);
			RefPtr<ShaderClosure> FindClosure(String name);
			List<ShaderComponentSymbol*> GetDependencyOrder();
			RefPtr<ShaderIR> IR;
		};

		class ImportPath
		{
		public:
			class Node
			{
			public:
				String TargetWorld;
				ImportOperatorDefSyntaxNode * ImportOperator;
				Node() = default;
				Node(String world, ImportOperatorDefSyntaxNode * imp)
					: TargetWorld(world), ImportOperator(imp)
				{}
			};
			List<Node> Nodes;
		};

		class PipelineSymbol
		{
		private:
			List<String> WorldTopologyOrder;
		public:
			PipelineSyntaxNode * SyntaxNode;
			EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>> Components;
			EnumerableDictionary<String, EnumerableHashSet<String>> ReachableWorlds;
			EnumerableDictionary<String, EnumerableHashSet<String>> WorldDependency;
			EnumerableDictionary<String, WorldSymbol> Worlds;
			bool IsAbstractWorld(String world);
			bool IsWorldReachable(EnumerableHashSet<String> & src, String targetWorld);
			bool IsWorldReachable(String src, String targetWorld);
			bool IsWorldDirectlyReachable(String src, String targetWorld);
			List<String> & GetWorldTopologyOrder();
			List<ImportPath> FindImportOperatorChain(String worldSrc, String worldDest);
			List<ImportOperatorDefSyntaxNode*> GetImportOperatorsFromSourceWorld(String worldSrc);
		};

		class CompileResult;

		class SymbolTable
		{
		public:
			EnumerableDictionary<String, RefPtr<FunctionSymbol>> Functions;
			EnumerableDictionary<String, RefPtr<ShaderSymbol>> Shaders;
			EnumerableDictionary<String, RefPtr<PipelineSymbol>> Pipelines;
			List<ShaderSymbol*> ShaderDependenceOrder;
			bool SortShaders(); // return true if success, return false if dependency is cyclic
			void EvalFunctionReferenceClosure();
		};

		class GUID
		{
		private:
			static int currentGUID;
		public:
			static void Clear();
			static int Next();
		};

		bool CheckComponentImplementationConsistency(ErrorWriter * err, ShaderComponentSymbol * comp, ShaderComponentImplSymbol * impl);

		template<typename T, typename GetDependencyFunc>
		void DependencySort(List<T> & list, const GetDependencyFunc & getDep)
		{
			HashSet<T> allSymbols, addedSymbols;
			for (auto & comp : list)
				allSymbols.Add(comp);
			List<T> sorted;
			bool changed = true;
			while (changed)
			{
				changed = false;
				for (auto & comp : list)
				{
					if (!addedSymbols.Contains(comp))
					{
						bool isFirst = true;
						auto && dependency = getDep(comp);
						for (auto & dep : dependency)
							if (allSymbols.Contains(dep) && !addedSymbols.Contains(dep))
							{
								isFirst = false;
								break;
							}
						if (isFirst)
						{
							addedSymbols.Add(comp);
							sorted.Add(comp);
							changed = true;
						}
					}
				}
			}
			list = _Move(sorted);
		}

	}
}
#endif

/***********************************************************************
SPIRECORE\IL.H
***********************************************************************/
#ifndef RASTER_RENDERER_IL_H
#define RASTER_RENDERER_IL_H

#include <crtdbg.h>

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;

		const int MaxSIMDSize = 8;

		enum ILBaseType
		{
			Int = 16, Int2 = 17, Int3 = 18, Int4 = 19,
			Float = 32, Float2 = 33, Float3 = 34, Float4 = 35,
			Float3x3 = 40, Float4x4 = 47,
			Texture2D = 48,
			TextureShadow = 49,
			TextureCube = 50,
			TextureCubeShadow = 51,
		};

		ILBaseType ILBaseTypeFromString(String str);
		String ILBaseTypeToString(ILBaseType str);
		int SizeofBaseType(ILBaseType type);
		int AlignmentOfBaseType(ILBaseType type);
		int RoundToAlignment(int offset, int alignment);
		extern int NamingCounter;
		class ILType : public Object
		{
		public:
			bool IsInt();
			bool IsFloat();
			bool IsIntVector();
			bool IsFloatVector();
			bool IsFloatMatrix();
			bool IsVector()
			{
				return IsIntVector() || IsFloatVector();
			}
			bool IsTexture();
			bool IsNonShadowTexture();
			int GetVectorSize();
			virtual ILType * Clone() = 0;
			virtual String ToString() = 0;
			virtual bool Equals(ILType* type) = 0;
		};

		class ILBasicType : public ILType
		{
		public:
			ILBaseType Type;
			ILBasicType()
			{
				Type = ILBaseType::Int;
			}
			ILBasicType(ILBaseType t)
			{
				Type = t;
			}
			virtual bool Equals(ILType* type) override
			{
				auto btype = dynamic_cast<ILBasicType*>(type);
				if (!btype)
					return false;
				return Type == btype->Type;
			}

			virtual ILType * Clone() override
			{
				auto rs = new ILBasicType();
				rs->Type = Type;
				return rs;
			}
			virtual String ToString() override
			{
				return ILBaseTypeToString(Type);
			}
		};

		


		class ILArrayType : public ILType
		{
		public:
			RefPtr<ILType> BaseType;
			int ArrayLength;
			virtual bool Equals(ILType* type) override
			{
				auto btype = dynamic_cast<ILArrayType*>(type);
				if (!btype)
					return false;
				return BaseType->Equals(btype->BaseType.Ptr());;
			}
			virtual ILType * Clone() override
			{
				auto rs = new ILArrayType();
				rs->BaseType = BaseType->Clone();
				rs->ArrayLength = ArrayLength;
				return rs;
			}
			virtual String ToString() override
			{
				return L"Array<" + BaseType->ToString() + L", " + String(ArrayLength) + L">";
			}
		};

		class ILOperand;

		class UserReferenceSet
		{
		private:
			EnumerableDictionary<ILOperand*, int> userRefCounts;
			int count;
		public:
			UserReferenceSet()
			{
				count = 0;
			}
			int Count()
			{
				return count;
			}
			int GetUseCount(ILOperand * op)
			{
				int rs = -1;
				userRefCounts.TryGetValue(op, rs);
				return rs;
			}
			void Add(ILOperand * user)
			{
				this->count++;
				int ncount = 0;
				if (userRefCounts.TryGetValue(user, ncount))
				{
					ncount++;
					userRefCounts[user] = ncount;
				}
				else
				{
					userRefCounts.Add(user, 1);
				}
			}
			void Remove(ILOperand * user)
			{
				int ncount = 0;
				if (userRefCounts.TryGetValue(user, ncount))
				{
					this->count--;
					ncount--;
					if (ncount)
						userRefCounts[user] = ncount;
					else
						userRefCounts.Remove(user);
				}
			}
			void RemoveAll(ILOperand * user)
			{
				int ncount = 0;
				if (userRefCounts.TryGetValue(user, ncount))
				{
					this->count -= ncount;
					userRefCounts.Remove(user);
				}
			}
			class UserIterator
			{
			private:
				EnumerableDictionary<ILOperand*, int>::Iterator iter;
			public:
				ILOperand * operator *()
				{
					return iter.Current->Value.Key;
				}
				ILOperand ** operator ->()
				{
					return &iter.Current->Value.Key;
				}
				UserIterator & operator ++()
				{
					iter++;
					return *this;
				}
				UserIterator operator ++(int)
				{
					UserIterator rs = *this;
					operator++();
					return rs;
				}
				bool operator != (const UserIterator & _that)
				{
					return iter != _that.iter;
				}
				bool operator == (const UserIterator & _that)
				{
					return iter == _that.iter;
				}
				UserIterator(const EnumerableDictionary<ILOperand*, int>::Iterator & iter)
				{
					this->iter = iter;
				}
				UserIterator()
				{
				}
			};
			UserIterator begin()
			{
				return UserIterator(userRefCounts.begin());
			}
			UserIterator end()
			{
				return UserIterator(userRefCounts.end());
			}
		};

		class ILOperand : public Object
		{
		public:
			String Name;
			RefPtr<ILType> Type;
			UserReferenceSet Users;
			String Attribute;
			void * Tag;
			union VMFields
			{
				void * VMData;
				struct Fields
				{
					int VMDataWords[2];
				} Fields;
			} VMFields;
			Procedure<ILOperand*> OnDelete;
			ILOperand()
			{
				Tag = nullptr;
			}
			ILOperand(const ILOperand & op)
			{
				Tag = op.Tag;
				Name = op.Name;
				Attribute = op.Attribute;
				if (op.Type)
					Type = op.Type->Clone();
				//Users = op.Users;
			}
			virtual ~ILOperand()
			{
				OnDelete(this);
			}
			virtual String ToString()
			{
				return L"<operand>";
			}
			virtual bool IsUndefined()
			{
				return false;
			}
		};

		class ILUndefinedOperand : public ILOperand
		{
		public:
			ILUndefinedOperand()
			{
				Name = L"<undef>";
			}
			virtual String ToString()
			{
				return L"<undef>";
			}
			virtual bool IsUndefined() override
			{
				return true;
			}
		};

		class UseReference
		{
		private:
			ILOperand * user;
			ILOperand * reference;
		public:
			UseReference()
				: user(0), reference(0)
			{}
			UseReference(const UseReference &)
			{
				user = 0;
				reference = 0;
			}
			UseReference(ILOperand * user)
				: user(user), reference(0)
			{}
			UseReference(ILOperand * user, ILOperand * ref)
			{
				this->user = user;
				this->reference = ref;
			}
			~UseReference()
			{
				if (reference)
					reference->Users.Remove(user);
			}
			void SetUser(ILOperand * _user)
			{
				this->user = _user;
			}
			void operator = (const UseReference & ref)
			{
				if (reference)
					reference->Users.Remove(user);
				reference = ref.Ptr();
				if (ref.Ptr())
				{
					if (!user)
						throw InvalidOperationException(L"user not initialized.");
					ref.Ptr()->Users.Add(user);
				}
			}
			void operator = (ILOperand * newRef)
			{
				if (reference)
					reference->Users.Remove(user);
				reference = newRef;
				if (newRef)
				{
					if (!user)
						throw InvalidOperationException(L"user not initialized.");
					newRef->Users.Add(user);
				}
			}
			bool operator != (const UseReference & _that)
			{
				return reference != _that.reference || user != _that.user;
			}
			bool operator == (const UseReference & _that)
			{
				return reference == _that.reference && user == _that.user;
			}
			ILOperand * Ptr() const
			{
				return reference;
			}
			ILOperand * operator->()
			{
				return reference;
			}
			ILOperand & operator*()
			{
				return *reference;
			}
			explicit operator bool()
			{
				return (reference != 0);
			}
			String ToString()
			{
				if (reference)
					return reference->Name;
				else
					return L"<null>";
			}
		};

		class OperandIterator
		{
		private:
			UseReference * use;
		public:
			OperandIterator()
			{
				use = 0;
			}
			OperandIterator(UseReference * use)
				: use(use)
			{}
			ILOperand & operator *()
			{
				return use->operator*();
			}
			ILOperand * operator ->()
			{
				return use->operator->();
			}
			void Set(ILOperand * user, ILOperand * op)
			{
				(*use).SetUser(user);
				(*use) = op;
			}
			void Set(ILOperand * op)
			{
				(*use) = op; 
			}
			OperandIterator & operator ++()
			{
				use++;
				return *this;
			}
			OperandIterator operator ++(int)
			{
				OperandIterator rs = *this;
				operator++();
				return rs;
			}
			bool operator != (const OperandIterator & _that)
			{
				return use != _that.use;
			}
			bool operator == (const OperandIterator & _that)
			{
				return use == _that.use;
			}
			bool operator == (const ILOperand * op)
			{
				return use->Ptr() == op;
			}
			bool operator != (const ILOperand * op)
			{
				return use->Ptr() != op;
			}
		};

		class ILConstOperand : public ILOperand
		{
		public:
			union
			{
				int IntValues[16];
				float FloatValues[16];
			};
			virtual String ToString() override
			{
				if (Type->IsFloat())
					return String(FloatValues[0]) + L"f";
				else if (Type->IsInt())
					return String(IntValues[0]);
				else if (auto baseType = dynamic_cast<ILBasicType*>(Type.Ptr()))
				{
					StringBuilder sb(256);
					if (baseType->Type == ILBaseType::Float2)
						sb << L"vec2(" << FloatValues[0] << L"f, " << FloatValues[1] << L"f)";
					else if (baseType->Type == ILBaseType::Float3)
						sb << L"vec3(" << FloatValues[0] << L"f, " << FloatValues[1] << L"f, " << FloatValues[2] << L"f)";
					else if (baseType->Type == ILBaseType::Float4)
						sb << L"vec4(" << FloatValues[0] << L"f, " << FloatValues[1] << L"f, " << FloatValues[2] << L"f, " << FloatValues[3] << L"f)";
					else if (baseType->Type == ILBaseType::Float3x3)
						sb << L"mat3(...)";
					else if (baseType->Type == ILBaseType::Float4x4)
						sb << L"mat4(...)";
					else if (baseType->Type == ILBaseType::Int2)
						sb << L"ivec2(" << IntValues[0] << L", " << IntValues[1] << L")";
					else if (baseType->Type == ILBaseType::Int3)
						sb << L"ivec3(" << IntValues[0] << L", " << IntValues[1] << L", " << IntValues[2] << L")";
					else if (baseType->Type == ILBaseType::Int4)
						sb << L"ivec4(" << IntValues[0] << L", " << IntValues[1] << L", " << IntValues[2] << L", " << IntValues[3] << L")";
					return sb.ToString();
				}
				else
					throw InvalidOperationException(L"Illegal constant.");
			}
		};

		class InstructionVisitor;

		class CFGNode;

		class ILInstruction : public ILOperand
		{
		private:
			ILInstruction *next, *prev;
		public:
			CFGNode * Parent;
			ILInstruction()
			{
				next = 0;
				prev = 0;
				Parent = 0;
			}
			ILInstruction(const ILInstruction & instr)
				: ILOperand(instr)
			{
				next = 0;
				prev = 0;
				Parent = 0;
			}
			~ILInstruction()
			{
				
			}
			virtual ILInstruction * Clone()
			{
				return new ILInstruction(*this);
			}

			virtual String GetOperatorString()
			{
				return L"<instruction>";
			}
			virtual bool HasSideEffect()
			{
				return false;
			}
			virtual bool IsDeterministic()
			{
				return true;
			}
			virtual void Accept(InstructionVisitor *)
			{
			}
			void InsertBefore(ILInstruction * instr)
			{
				instr->Parent = Parent;
				instr->prev = prev;
				instr->next = this;
				prev = instr;
				auto *npp = instr->prev;
				if (npp)
					npp->next = instr;
			}
			void InsertAfter(ILInstruction * instr)
			{
				instr->Parent = Parent;
				instr->prev = this;
				instr->next = this->next;
				next = instr;
				auto *npp = instr->next;
				if (npp)
					npp->prev = instr;
			}
			ILInstruction * GetNext()
			{
				return next;
			}
			ILInstruction * GetPrevious()
			{
				return prev;
			}
			void Remove()
			{
				if (prev)
					prev->next = next;
				if (next)
					next->prev = prev;
			}
			void Erase()
			{
				Remove();
				if (Users.Count())
				{
					throw InvalidOperationException(L"All uses must be removed before removing this instruction");
				}
				delete this;
			}
			virtual OperandIterator begin()
			{
				return OperandIterator();
			}
			virtual OperandIterator end()
			{
				return OperandIterator();
			}
			virtual int GetSubBlockCount()
			{
				return 0;
			}
			virtual CFGNode * GetSubBlock(int)
			{
				return nullptr;
			}
			template<typename T>
			T * As()
			{
				return dynamic_cast<T*>(this);
			}
			template<typename T>
			bool Is()
			{
				return dynamic_cast<T*>(this) != 0;
			}
		};

		template <typename T, typename TOperand>
		bool Is(TOperand * op)
		{
			auto ptr = dynamic_cast<T*>(op);
			if (ptr)
				return true;
			else
				return false;
		}

		class SwitchInstruction : public ILInstruction
		{
		public:
			List<UseReference> Candidates;
			virtual OperandIterator begin()
			{
				return Candidates.begin();
			}
			virtual OperandIterator end()
			{
				return Candidates.end();
			}
			virtual String ToString() override
			{
				StringBuilder sb(256);
				sb << Name;
				sb << L" = switch ";
				for (auto & op : Candidates)
				{
					sb << op.ToString();
					if (op != Candidates.Last())
						sb << L", ";
				}
				return sb.ProduceString();
			}
			virtual String GetOperatorString() override
			{
				return L"switch";
			}
			virtual bool HasSideEffect() override
			{
				return false;
			}
			SwitchInstruction(int argSize)
			{
				Candidates.SetSize(argSize);
				for (auto & use : Candidates)
					use.SetUser(this);
			}
			SwitchInstruction(const SwitchInstruction & other)
				: ILInstruction(other)
			{
				Candidates.SetSize(other.Candidates.Count());
				for (int i = 0; i < other.Candidates.Count(); i++)
				{
					Candidates[i].SetUser(this);
					Candidates[i] = other.Candidates[i].Ptr();
				}

			}
			virtual SwitchInstruction * Clone()
			{
				return new SwitchInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class LeaInstruction : public ILInstruction
		{};

		// retrieves pointer to a global entry
		class GLeaInstruction : public LeaInstruction
		{
		public:
			String VariableName;
			GLeaInstruction() = default;
			GLeaInstruction(const GLeaInstruction &) = default;
			GLeaInstruction(const String & varName)
				:VariableName(varName)
			{
			}
			virtual String ToString() override
			{
				return Name + L" = g_var [" + VariableName + L"]";
			}
			virtual String GetOperatorString() override
			{
				return L"glea " + VariableName;
			}
			virtual GLeaInstruction * Clone() override
			{
				return new GLeaInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class ImportOperatorDefSyntaxNode;
		class CompiledWorld;
		class ImportInstruction : public LeaInstruction
		{
		public:
			String ComponentName;
			ImportOperatorDefSyntaxNode * ImportOperator;
			CompiledWorld * SourceWorld;

			List<UseReference> Arguments;
			virtual OperandIterator begin()
			{
				return Arguments.begin();
			}
			virtual OperandIterator end()
			{
				return Arguments.end();
			}

			ImportInstruction(int argSize = 0)
				: LeaInstruction()
			{
				Arguments.SetSize(argSize);
				for (auto & use : Arguments)
					use.SetUser(this);
			}
			ImportInstruction(const ImportInstruction & other)
				: LeaInstruction(other)
			{
				Arguments.SetSize(other.Arguments.Count());
				for (int i = 0; i < other.Arguments.Count(); i++)
				{
					Arguments[i].SetUser(this);
					Arguments[i] = other.Arguments[i].Ptr();
				}
			}

			ImportInstruction(int argSize, String compName, ImportOperatorDefSyntaxNode * importOp, CompiledWorld * srcWorld, ILType * type)
				:ImportInstruction(argSize)
			{
				this->ComponentName = compName;
				this->ImportOperator = importOp;
				this->SourceWorld = srcWorld;
				this->Type = type;
			}
			virtual String ToString() override;
			virtual String GetOperatorString() override;
			virtual ImportInstruction * Clone() override
			{
				return new ImportInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class AllocVarInstruction : public LeaInstruction
		{
		public:
			UseReference Size;
			AllocVarInstruction(ILType * type, ILOperand * count)
				: Size(this)
			{
				this->Type = type;
				this->Size = count;
			}
			AllocVarInstruction(RefPtr<ILType> & type, ILOperand * count)
				: Size(this)
			{
				auto ptrType = type->Clone();
				if (!type)
					throw ArgumentException(L"type cannot be null.");
				this->Type = ptrType;
				this->Size = count;
			}
			AllocVarInstruction(const AllocVarInstruction & other)
				:Size(this), LeaInstruction(other)
			{
				Size = other.Size.Ptr();
			}
			virtual bool IsDeterministic() override
			{
				return false;
			}
			virtual String ToString() override
			{
				return Name + L" = VAR " + Type->ToString() + L", " + Size.ToString();
			}
			virtual OperandIterator begin() override
			{
				return &Size;
			}
			virtual OperandIterator end() override
			{
				return &Size + 1;
			}
			virtual String GetOperatorString() override
			{
				return L"avar";
			}
			virtual AllocVarInstruction * Clone() override
			{
				return new AllocVarInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class FetchArgInstruction : public LeaInstruction
		{
		public:
			int ArgId;
			FetchArgInstruction(ILType * type)
			{
				this->Type = type;
				ArgId = 0;
			}
			virtual String ToString() override
			{
				return Name + L" = ARG " + Type->ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"arg " + String(ArgId);
			}
			virtual bool IsDeterministic() override
			{
				return false;
			}
			virtual FetchArgInstruction * Clone() override
			{
				return new FetchArgInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class CFGNode;

		class AllInstructionsIterator
		{
		private:
			struct StackItem
			{
				ILInstruction* instr;
				int subBlockPtr;
			};
			List<StackItem> stack;
			ILInstruction * curInstr = nullptr;
			int subBlockPtr = 0;
		public:
			AllInstructionsIterator(ILInstruction * instr)
			{
				curInstr = instr;
			}
			AllInstructionsIterator & operator ++();
			
			AllInstructionsIterator operator ++(int)
			{
				AllInstructionsIterator rs = *this;
				operator++();
				return rs;
			}
			bool operator != (const AllInstructionsIterator & _that)
			{
				return curInstr != _that.curInstr || subBlockPtr != _that.subBlockPtr;
			}
			bool operator == (const AllInstructionsIterator & _that)
			{
				return curInstr == _that.curInstr && subBlockPtr == _that.subBlockPtr;
			}
			ILOperand & operator *()
			{
				return *curInstr;
			}
			ILOperand * operator ->()
			{
				return curInstr;
			}
		};

		class AllInstructionsCollection
		{
		private:
			CFGNode * node;
		public:
			AllInstructionsCollection(CFGNode * _node)
				: node(_node)
			{}
			AllInstructionsIterator begin();
			AllInstructionsIterator end();
		};

		class CFGNode : public Object
		{
		private:
			ILInstruction *headInstr, *tailInstr;
		public:
			class Iterator
			{
			public:
				ILInstruction * Current, *Next;
				void SetCurrent(ILInstruction * cur)
				{
					Current = cur;
					if (Current)
						Next = Current->GetNext();
					else
						Next = 0;
				}
				Iterator(ILInstruction * cur)
				{
					SetCurrent(cur);
				}
				ILInstruction & operator *() const
				{
					return *Current;
				}
				Iterator& operator ++()
				{
					SetCurrent(Next);
					return *this;
				}
				Iterator operator ++(int)
				{
					Iterator rs = *this;
					SetCurrent(Next);
					return rs;
				}
				bool operator != (const Iterator & iter) const
				{
					return Current != iter.Current;
				}
				bool operator == (const Iterator & iter) const
				{
					return Current == iter.Current;
				}
			};

			Iterator begin() const
			{
				return Iterator(headInstr->GetNext());
			}

			Iterator end() const
			{
				return Iterator(tailInstr);
			}

			AllInstructionsCollection GetAllInstructions()
			{
				return AllInstructionsCollection(this);
			}
			
			ILInstruction * GetFirstNonPhiInstruction();
			bool HasPhiInstruction();

			ILInstruction * GetLastInstruction()
			{
				return (tailInstr->GetPrevious());
			}

			String Name;

			CFGNode()
			{
				headInstr = new ILInstruction();
				tailInstr = new ILInstruction();
				headInstr->Parent = this;
				headInstr->InsertAfter(tailInstr);
			}
			~CFGNode()
			{
				ILInstruction * instr = headInstr;
				while (instr)
				{
					for (auto user : instr->Users)
					{
						auto userInstr = dynamic_cast<ILInstruction*>(user);
						if (userInstr)
						{
							for (auto iter = userInstr->begin(); iter != userInstr->end(); ++iter)
							if (iter == instr)
								iter.Set(0);
						}
					}
				
					auto next = instr->GetNext();
					delete instr;
					instr = next;
				}
			}
			void InsertHead(ILInstruction * instr)
			{
				headInstr->InsertAfter(instr);
			}
			void InsertTail(ILInstruction * instr)
			{
				tailInstr->InsertBefore(instr);
			}
			void NameAllInstructions();
			void DebugPrint();
		};

		template<typename T>
		struct ConstKey
		{
			Array<T, 16> Value;
			int Size;
			ConstKey()
			{
				Value.SetSize(Value.GetCapacity());
			}
			ConstKey(T value, int size)
			{
				if (size == 0)
					size = 1;
				Value.SetSize(Value.GetCapacity());
				for (int i = 0; i < size; i++)
					Value[i] = value;
				Size = size;
			}
			static ConstKey<T> FromValues(T value, T value1)
			{
				ConstKey<T> result;
				result.Value.SetSize(result.Value.GetCapacity());
				result.Size = 2;
				result.Value[0] = value;
				result.Value[1] = value1;
				return result;
			}
			static ConstKey<T> FromValues(T value, T value1, T value2)
			{
				ConstKey<T> result;
				result.Value.SetSize(result.Value.GetCapacity());
				result.Size = 3;
				result.Value[0] = value;
				result.Value[1] = value1;
				result.Value[2] = value2;
				return result;
			}
			static ConstKey<T> FromValues(T value, T value1, T value2, T value3)
			{
				ConstKey<T> result;
				result.Value.SetSize(result.Value.GetCapacity());
				result.Size = 4;
				result.Value[0] = value;
				result.Value[1] = value1;
				result.Value[2] = value2;
				result.Value[3] = value3;
				return result;
			}
			int GetHashCode()
			{
				int result = Size;
				for (int i = 0; i < Size; i++)
					result ^= ((*(int*)&Value) << 5);
				return result;
			}
			bool operator == (const ConstKey<T> & other)
			{
				if (Size != other.Size)
					return false;
				for (int i = 0; i < Size; i++)
					if (Value[i] != other.Value[i])
						return false;
				return true;
			}
		};

		class PhiInstruction : public ILInstruction
		{
		public:
			List<UseReference> Operands; // Use as fixed array, no insert or resize
		public:
			PhiInstruction(int opCount)
			{
				Operands.SetSize(opCount);
				for (int i = 0; i < opCount; i++)
					Operands[i].SetUser(this);
			}
			PhiInstruction(const PhiInstruction & other)
				: ILInstruction(other)
			{
				Operands.SetSize(other.Operands.Count());
				for (int i = 0; i < Operands.Count(); i++)
				{
					Operands[i].SetUser(this);
					Operands[i] = other.Operands[i].Ptr();
				}
			}
			virtual String GetOperatorString() override
			{
				return L"phi";
			}
			virtual OperandIterator begin() override
			{
				return Operands.begin();
			}
			virtual OperandIterator end() override
			{
				return Operands.end();
			}
			virtual String ToString() override
			{
				StringBuilder sb;
				sb << Name << L" = phi ";
				for (auto & op : Operands)
				{
					if (op)
					{
						sb << op.ToString();
					}
					else
						sb << L"<?>";
					sb << L", ";
				}
				return sb.ProduceString();
			}
			virtual PhiInstruction * Clone()
			{
				return new PhiInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class UnaryInstruction abstract : public ILInstruction
		{
		public:
			UseReference Operand;
			UnaryInstruction()
				: Operand(this)
			{}
			UnaryInstruction(const UnaryInstruction & other)
				: ILInstruction(other), Operand(this)
			{
				Operand = other.Operand.Ptr();
			}
			virtual OperandIterator begin()
			{
				return &Operand;
			}
			virtual OperandIterator end()
			{
				return &Operand + 1;
			}
		};

		class ExportInstruction : public UnaryInstruction
		{
		public:
			String ComponentName;
			String ExportOperator;
			CompiledWorld * World;

			ExportInstruction() = default;
			ExportInstruction(const ExportInstruction &) = default;

			ExportInstruction(String compName, String exportOp, CompiledWorld * srcWorld, ILOperand * value)
				: UnaryInstruction()
			{
				this->Operand = value;
				this->ComponentName = compName;
				this->ExportOperator = exportOp;
				this->World = srcWorld;
				this->Type = value->Type;
			}
			virtual String ToString() override
			{
				return L"export<" + ExportOperator + L">[" + ComponentName + L"], " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"export<" + ExportOperator + L">[" + ComponentName + L"]";
			}
			virtual ExportInstruction * Clone() override
			{
				return new ExportInstruction(*this);
			}
			virtual bool HasSideEffect() override
			{
				return true;
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class BinaryInstruction abstract : public ILInstruction
		{
		public:
			Array<UseReference, 2> Operands;
			BinaryInstruction()
			{
				Operands.SetSize(2);
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
			}
			BinaryInstruction(const BinaryInstruction & other)
				: ILInstruction(other)
			{
				Operands.SetSize(2);
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[0] = other.Operands[0].Ptr();
				Operands[1] = other.Operands[1].Ptr();
			}
			virtual OperandIterator begin()
			{
				return Operands.begin();
			}
			virtual OperandIterator end()
			{
				return Operands.end();
			}
		};

		class SelectInstruction : public ILInstruction
		{
		public:
			UseReference Operands[3];
			SelectInstruction()
			{
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[2].SetUser(this);
			}
			SelectInstruction(const SelectInstruction & other)
				: ILInstruction(other)
			{
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[2].SetUser(this);
				Operands[0] = other.Operands[0].Ptr();
				Operands[1] = other.Operands[1].Ptr();
				Operands[2] = other.Operands[2].Ptr();
			}
			SelectInstruction(ILOperand * mask, ILOperand * val0, ILOperand * val1)
			{
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[2].SetUser(this);
				Operands[0] = mask;
				Operands[1] = val0;
				Operands[2] = val1;
				Type = val0->Type->Clone();
			}
			virtual OperandIterator begin()
			{
				return Operands;
			}
			virtual OperandIterator end()
			{
				return Operands + 3;
			}

			virtual String ToString() override
			{
				return Name + L" = select " + Operands[0].ToString() + L": " + Operands[1].ToString() + L", " + Operands[2].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"select";
			}
			virtual SelectInstruction * Clone() override
			{
				return new SelectInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class CallInstruction : public ILInstruction
		{
		public:
			String Function;
			List<UseReference> Arguments;
			virtual OperandIterator begin()
			{
				return Arguments.begin();
			}
			virtual OperandIterator end()
			{
				return Arguments.end();
			}
			virtual String ToString() override
			{
				StringBuilder sb(256);
				sb << Name;
				sb << L" = call " << Function << L"(";
				for (auto & op : Arguments)
				{
					sb << op.ToString();
					if (op != Arguments.Last())
						sb << L", ";
				}
				sb << L")";
				return sb.ProduceString();
			}
			virtual String GetOperatorString() override
			{
				return L"call " + Function;
			}
			virtual bool HasSideEffect() override
			{
				return false;
			}
			CallInstruction(int argSize)
			{
				Arguments.SetSize(argSize);
				for (auto & use : Arguments)
					use.SetUser(this);
			}
			CallInstruction(const CallInstruction & other)
				: ILInstruction(other)
			{
				Function = other.Function;
				Arguments.SetSize(other.Arguments.Count());
				for (int i = 0; i < other.Arguments.Count(); i++)
				{
					Arguments[i].SetUser(this);
					Arguments[i] = other.Arguments[i].Ptr();
				}

			}
			virtual CallInstruction * Clone()
			{
				return new CallInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class NotInstruction : public UnaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return  Name + L" = not " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"not";
			}
			virtual NotInstruction * Clone()
			{
				return new NotInstruction(*this);
			}
			NotInstruction() = default;
			NotInstruction(const NotInstruction & other) = default;

			NotInstruction(ILOperand * op)
			{
				Operand = op;
				Type = op->Type->Clone();
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class NegInstruction : public UnaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return  Name + L" = neg " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"neg";
			}
			virtual NegInstruction * Clone()
			{
				return new NegInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class BitNotInstruction : public UnaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return  Name + L" = bnot " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"bnot";
			}
			virtual BitNotInstruction * Clone()
			{
				return new BitNotInstruction(*this);
			}
			BitNotInstruction() = default;
			BitNotInstruction(const BitNotInstruction & instr) = default;

			BitNotInstruction(ILOperand * op)
			{
				Operand = op;
				Type = op->Type->Clone();
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class AddInstruction : public BinaryInstruction
		{
		public:
			AddInstruction() = default;
			AddInstruction(const AddInstruction & instr) = default;
			AddInstruction(ILOperand * v0, ILOperand * v1)
			{
				Operands[0] = v0;
				Operands[1] = v1;
				Type = v0->Type->Clone();
			}
			virtual String ToString() override
			{
				return Name + L" = add " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"add";
			}
			virtual AddInstruction * Clone()
			{
				return new AddInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class MemberLoadInstruction : public BinaryInstruction
		{
		public:
			MemberLoadInstruction() = default;
			MemberLoadInstruction(const MemberLoadInstruction &) = default;
			MemberLoadInstruction(ILOperand * v0, ILOperand * v1)
			{
				Operands[0] = v0;
				Operands[1] = v1;
				if (auto arrType = dynamic_cast<ILArrayType *>(v0->Type.Ptr()))
				{
					Type = arrType->BaseType->Clone();
				}
				else if (auto baseType = dynamic_cast<ILBasicType *>(v0->Type.Ptr()))
				{
					switch (baseType->Type)
					{
					case ILBaseType::Float2:
					case ILBaseType::Float3:
					case ILBaseType::Float4:
						Type = new ILBasicType(ILBaseType::Float);
						break;
					case ILBaseType::Float3x3:
						Type = new ILBasicType(ILBaseType::Float3);
						break;
					case ILBaseType::Float4x4:
						Type = new ILBasicType(ILBaseType::Float4);
						break;
					case ILBaseType::Int2:
					case ILBaseType::Int3:
					case ILBaseType::Int4:
						Type = new ILBasicType(ILBaseType::Int);
						break;
					default:
						throw InvalidOperationException(L"Unsupported aggregate type.");
					}
				}
			}
			virtual String ToString() override
			{
				return Name + L" = retrieve " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"retrieve";
			}
			virtual MemberLoadInstruction * Clone()
			{
				return new MemberLoadInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class SubInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = sub " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"sub";
			}
			virtual SubInstruction * Clone()
			{
				return new SubInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class MulInstruction : public BinaryInstruction
		{
		public:
			MulInstruction(){}
			MulInstruction(ILOperand * v0, ILOperand * v1)
			{
				Operands[0] = v0;
				Operands[1] = v1;
				Type = v0->Type->Clone();
			}
			MulInstruction(const MulInstruction &) = default;

			virtual String ToString() override
			{
				return Name + L" = mul " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"mul";
			}
			virtual MulInstruction * Clone()
			{
				return new MulInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class DivInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = div " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"div";
			}
			virtual DivInstruction * Clone()
			{
				return new DivInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class ModInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = mod " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"mod";
			}
			virtual ModInstruction * Clone()
			{
				return new ModInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class AndInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = and " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"and";
			}
			virtual AndInstruction * Clone() override
			{
				return new AndInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class OrInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = or " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"or";
			}
			virtual OrInstruction * Clone() override
			{
				return new OrInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class BitAndInstruction : public BinaryInstruction
		{
		public:
			BitAndInstruction(){}
			BitAndInstruction(ILOperand * v0, ILOperand * v1)
			{
				Operands[0] = v0;
				Operands[1] = v1;
				Type = v0->Type->Clone();
			}
			BitAndInstruction(const BitAndInstruction &) = default;
			virtual String ToString() override
			{
				return Name + L" = band " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"band";
			}
			virtual BitAndInstruction * Clone() override
			{
				return new BitAndInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class BitOrInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = bor " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"bor";
			}
			virtual BitOrInstruction * Clone() override
			{
				return new BitOrInstruction(*this);
			}
			BitOrInstruction(){}
			BitOrInstruction(const BitOrInstruction &) = default;
			BitOrInstruction(ILOperand * v0, ILOperand * v1)
			{
				Operands[0] = v0;
				Operands[1] = v1;
				Type = v0->Type->Clone();
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class BitXorInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = bxor " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"bxor";
			}
			virtual BitXorInstruction * Clone() override
			{
				return new BitXorInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class ShlInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = shl " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"shl";
			}
			virtual ShlInstruction * Clone() override
			{
				return new ShlInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class ShrInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = shr " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"shr";
			}
			virtual ShrInstruction * Clone() override
			{
				return new ShrInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class CompareInstruction : public BinaryInstruction
		{};
		class CmpgtInstruction : public CompareInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = gt " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"gt";
			}
			virtual CmpgtInstruction * Clone() override
			{
				return new CmpgtInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class CmpgeInstruction : public CompareInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = ge " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"ge";
			}
			virtual CmpgeInstruction * Clone() override
			{
				return new CmpgeInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class CmpltInstruction : public CompareInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = lt " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"lt";
			}
			virtual CmpltInstruction * Clone() override
			{
				return new CmpltInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class CmpleInstruction : public CompareInstruction
		{
		public:
			CmpleInstruction() = default;
			CmpleInstruction(const CmpleInstruction &) = default;
			CmpleInstruction(ILOperand * v0, ILOperand * v1)
			{
				Operands[0] = v0;
				Operands[1] = v1;
				Type = v0->Type->Clone();
			}

			virtual String ToString() override
			{
				return Name + L" = le " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"le";
			}
			virtual CmpleInstruction * Clone() override
			{
				return new CmpleInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class CmpeqlInstruction : public CompareInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = eql " + Operands[0].ToString()
					+ L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"eql";
			}
			virtual CmpeqlInstruction * Clone() override
			{
				return new CmpeqlInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class CmpneqInstruction : public CompareInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + L" = neq " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"neq";
			}
			virtual CmpneqInstruction * Clone() override
			{
				return new CmpneqInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class CastInstruction : public UnaryInstruction
		{};

		class Float2IntInstruction : public CastInstruction
		{
		public:
			Float2IntInstruction(){}
			Float2IntInstruction(const Float2IntInstruction &) = default;

			Float2IntInstruction(ILOperand * op)
			{
				Operand = op;
				Type = new ILBasicType(ILBaseType::Int);
			}
		public:
			virtual String ToString() override
			{
				return Name + L" = f2i " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"f2i";
			}
			virtual Float2IntInstruction * Clone() override
			{
				return new Float2IntInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class Int2FloatInstruction : public CastInstruction
		{
		public:
			Int2FloatInstruction(){}
			Int2FloatInstruction(ILOperand * op)
			{
				Operand = op;
				Type = new ILBasicType(ILBaseType::Float);
			}
			Int2FloatInstruction(const Int2FloatInstruction &) = default;

		public:
			virtual String ToString() override
			{
				return Name + L" = i2f " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"i2f";
			}
			virtual Int2FloatInstruction * Clone() override
			{
				return new Int2FloatInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class CopyInstruction : public UnaryInstruction
		{
		public:
			CopyInstruction(){}
			CopyInstruction(const CopyInstruction &) = default;

			CopyInstruction(ILOperand * dest)
			{
				Operand = dest;
				Type = dest->Type->Clone();
			}
		public:
			virtual String ToString() override
			{
				return Name + L" = " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"copy";
			}
			virtual CopyInstruction * Clone() override
			{
				return new CopyInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		// load(src)
		class LoadInstruction : public UnaryInstruction
		{
		public:
			bool Deterministic;
			LoadInstruction()
			{
				Deterministic = false;
			}
			LoadInstruction(const LoadInstruction & other)
				: UnaryInstruction(other)
			{
				Deterministic = other.Deterministic;
			}
			LoadInstruction(ILOperand * dest);
		public:
			virtual String ToString() override
			{
				return Name + L" = load " + Operand.ToString();
			}
			virtual bool IsDeterministic()
			{
				return Deterministic;
			}
			virtual String GetOperatorString() override
			{
				return L"ld";
			}
			virtual LoadInstruction * Clone() override
			{
				auto rs = new LoadInstruction(*this);
				if (!rs->Type)
					printf("shit");
				return rs;
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		// store(dest, value)
		class StoreInstruction : public BinaryInstruction
		{
		public:
			StoreInstruction(){}
			StoreInstruction(const StoreInstruction &) = default;

			StoreInstruction(ILOperand * dest, ILOperand * value)
			{
				Operands.SetSize(2);
				Operands[0] = dest;
				Operands[1] = value;
			}
		public:
			virtual String ToString() override
			{
				return L"store " + Operands[0].ToString() + L", " +
					Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"st";
			}
			virtual bool HasSideEffect() override
			{
				return true;
			}
			virtual StoreInstruction * Clone() override
			{
				return new StoreInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class MemberUpdateInstruction : public ILInstruction
		{
		public:
			UseReference Operands[3];
			MemberUpdateInstruction()
			{
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[2].SetUser(this);
			}
			MemberUpdateInstruction(const MemberUpdateInstruction & other)
				: ILInstruction(other)
			{
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[2].SetUser(this);
				Operands[0] = other.Operands[0].Ptr();
				Operands[1] = other.Operands[1].Ptr();
				Operands[2] = other.Operands[2].Ptr();
			}
			MemberUpdateInstruction(ILOperand * var, ILOperand * offset, ILOperand * value)
			{
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[2].SetUser(this);
				Operands[0] = var;
				Operands[1] = offset;
				Operands[2] = value;
				Type = var->Type->Clone();
			}
			virtual OperandIterator begin()
			{
				return Operands;
			}
			virtual OperandIterator end()
			{
				return Operands + 3;
			}
			virtual String ToString() override
			{
				return Name + L" = update " + Operands[0].ToString() + L", " + Operands[1].ToString() + L"," + Operands[2].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"update";
			}
			virtual MemberUpdateInstruction * Clone()
			{
				return new MemberUpdateInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};


		class InstructionVisitor : public Object
		{
		public:
			virtual void VisitAddInstruction(AddInstruction *){}
			virtual void VisitSubInstruction(SubInstruction *){}
			virtual void VisitDivInstruction(DivInstruction *){}
			virtual void VisitMulInstruction(MulInstruction *){}
			virtual void VisitModInstruction(ModInstruction *){}
			virtual void VisitNegInstruction(NegInstruction *){}
			virtual void VisitAndInstruction(AndInstruction *){}
			virtual void VisitOrInstruction(OrInstruction *){}
			virtual void VisitBitAndInstruction(BitAndInstruction *){}
			virtual void VisitBitOrInstruction(BitOrInstruction *){}
			virtual void VisitBitXorInstruction(BitXorInstruction *){}
			virtual void VisitShlInstruction(ShlInstruction *){}
			virtual void VisitShrInstruction(ShrInstruction *){}

			virtual void VisitBitNotInstruction(BitNotInstruction *){}
			virtual void VisitNotInstruction(NotInstruction *){}
			virtual void VisitCmpeqlInstruction(CmpeqlInstruction *){}
			virtual void VisitCmpneqInstruction(CmpneqInstruction *){}
			virtual void VisitCmpltInstruction(CmpltInstruction *){}
			virtual void VisitCmpleInstruction(CmpleInstruction *){}
			virtual void VisitCmpgtInstruction(CmpgtInstruction *){}
			virtual void VisitCmpgeInstruction(CmpgeInstruction *){}

			virtual void VisitLoadInstruction(LoadInstruction *){}
			virtual void VisitStoreInstruction(StoreInstruction *){}
			virtual void VisitCopyInstruction(CopyInstruction *){}

			virtual void VisitAllocVarInstruction(AllocVarInstruction *){}
			virtual void VisitFetchArgInstruction(FetchArgInstruction *){}
			virtual void VisitGLeaInstruction(GLeaInstruction *){}
			virtual void VisitCastInstruction(CastInstruction *){}
			virtual void VisitInt2FloatInstruction(Int2FloatInstruction *){}
			virtual void VisitFloat2IntInstruction(Float2IntInstruction *){}
			virtual void VisitMemberLoadInstruction(MemberLoadInstruction *){}
			virtual void VisitMemberUpdateInstruction(MemberUpdateInstruction *) {}
			virtual void VisitImportInstruction(ImportInstruction*) {}
			virtual void VisitExportInstruction(ExportInstruction*) {}
			virtual void VisitSelectInstruction(SelectInstruction *){}
			virtual void VisitCallInstruction(CallInstruction *){}
			virtual void VisitSwitchInstruction(SwitchInstruction *){}

			virtual void VisitPhiInstruction(PhiInstruction *){}
		};

		class ForInstruction : public ILInstruction
		{
		public:
			RefPtr<CFGNode> ConditionCode, SideEffectCode, BodyCode;
			virtual int GetSubBlockCount()
			{
				return 3;
			}
			virtual CFGNode * GetSubBlock(int i)
			{
				if (i == 0)
					return ConditionCode.Ptr();
				else if (i == 1)
					return SideEffectCode.Ptr();
				else if (i == 2)
					return BodyCode.Ptr();
				return nullptr;
			}
		};
		class IfInstruction : public UnaryInstruction
		{
		public:
			RefPtr<CFGNode> TrueCode, FalseCode;
			virtual int GetSubBlockCount()
			{
				if (FalseCode)
					return 2;
				else
					return 1;
			}
			virtual CFGNode * GetSubBlock(int i)
			{
				if (i == 0)
					return TrueCode.Ptr();
				else if (i == 1)
					return FalseCode.Ptr();
				return nullptr;
			}
		};
		class WhileInstruction : public ILInstruction
		{
		public:
			RefPtr<CFGNode> ConditionCode, BodyCode;
			virtual int GetSubBlockCount()
			{
				return 2;
			}
			virtual CFGNode * GetSubBlock(int i)
			{
				if (i == 0)
					return ConditionCode.Ptr();
				else if (i == 1)
					return BodyCode.Ptr();
				return nullptr;
			}
		};
		class DoInstruction : public ILInstruction
		{
		public:
			RefPtr<CFGNode> ConditionCode, BodyCode;
			virtual int GetSubBlockCount()
			{
				return 2;
			}
			virtual CFGNode * GetSubBlock(int i)
			{
				if (i == 1)
					return ConditionCode.Ptr();
				else if (i == 0)
					return BodyCode.Ptr();
				return nullptr;
			}
		};
		class ReturnInstruction : public UnaryInstruction
		{
		public:
			ReturnInstruction(ILOperand * op)
				:UnaryInstruction()
			{
				Operand = op;
			}
		};
		class BreakInstruction : public ILInstruction
		{};
		class ContinueInstruction : public ILInstruction
		{};

		class KeyHoleNode
		{
		public:
			String NodeType;
			int CaptureId = -1;
			List<RefPtr<KeyHoleNode>> Children;
			bool Match(List<ILOperand*> & matchResult, ILOperand * instr);
			static RefPtr<KeyHoleNode> Parse(String format);
		};
	}
}

#endif

/***********************************************************************
SPIRECORE\COMPILEDPROGRAM.H
***********************************************************************/
#ifndef BAKER_SL_COMPILED_PROGRAM_H
#define BAKER_SL_COMPILED_PROGRAM_H


namespace Spire
{
	namespace Compiler
	{
		class ConstantPoolImpl;

		class ConstantPool
		{
		private:
			ConstantPoolImpl * impl;
		public:
			ILConstOperand * CreateConstant(ILConstOperand * c);
			ILConstOperand * CreateConstantIntVec(int val0, int val1);
			ILConstOperand * CreateConstantIntVec(int val0, int val1, int val2);
			ILConstOperand * CreateConstantIntVec(int val0, int val1, int val3, int val4);
			ILConstOperand * CreateConstant(int val, int vectorSize = 0);
			ILConstOperand * CreateConstant(float val, int vectorSize = 0);
			ILConstOperand * CreateConstant(float val, float val1);
			ILConstOperand * CreateConstant(float val, float val1, float val2);
			ILConstOperand * CreateConstant(float val, float val1, float val2, float val3);
			ILOperand * CreateDefaultValue(ILType * type);
			ILUndefinedOperand * GetUndefinedOperand();
			ConstantPool();
			~ConstantPool();
		};

		enum class InterfaceQualifier
		{
			Input, Output
		};

		

		class CompiledGlobalVar
		{
		public:
			String Name;
			String InputSourceWorld;
			String OrderingStr;
			ImportOperatorDefSyntaxNode ImportOperator;
			InterfaceQualifier Qualifier;
			RefPtr<ILType> Type;
			int OutputIndex = -1, InputIndex = -1;
			bool IsBuiltin = false;
			EnumerableDictionary<String, String> LayoutAttribs;
		};

		class ComponentDefinition
		{
		public:
			String Name;
			String OrderingStr;
			int Offset = 0;
			RefPtr<ILType> Type;
			EnumerableDictionary<String, String> LayoutAttribs;
		};

		class InterfaceBlock : public Object
		{
		public:
			String Name;
			String SourceWorld;
			int Size = 0;
			EnumerableDictionary<String, String> Attributes;
			EnumerableHashSet<String> UserWorlds;
			EnumerableDictionary<String, ComponentDefinition> Entries;
		};

		class InputInterface
		{
		public:
			InterfaceBlock * Block;
			ImportOperatorDefSyntaxNode ImportOperator;
		};

		class CompiledShader;

		class CompiledComponent
		{
		public:
			ILOperand * CodeOperand;
			EnumerableDictionary<String, String> Attributes;
		};

		class CompiledWorld
		{
		public:
			String TargetMachine;
			String ShaderName, WorldName;
			Token ExportOperator;
			EnumerableDictionary<String, String> BackendParameters;
			EnumerableDictionary<String, InputInterface> WorldInputs;
			InterfaceBlock * WorldOutput;

			bool IsAbstract = false;
			CodePosition WorldDefPosition;
			EnumerableDictionary<String, String> Attributes;
			EnumerableHashSet<String> ReferencedFunctions;
			EnumerableDictionary<String, CompiledComponent> LocalComponents;
			EnumerableDictionary<String, ImportInstruction*> ImportInstructions;
			RefPtr<CFGNode> Code = new CFGNode();
			CompiledShader * Shader;
		};

		class InterfaceMetaData
		{
		public:
			CoreLib::Basic::String Name;
			Spire::Compiler::ILBaseType Type;
			EnumerableDictionary<String, String> Attributes;

			int GetHashCode()
			{
				return Name.GetHashCode();
			}
			bool operator == (const InterfaceMetaData & other)
			{
				return Name == other.Name;
			}
		};

		class WorldMetaData
		{
		public:
			CoreLib::Basic::String Name;
			CoreLib::Basic::String TargetName;
			CoreLib::Basic::String OutputBlock;
			CoreLib::Basic::List<CoreLib::Basic::String> InputBlocks;
			CoreLib::Basic::List<CoreLib::Basic::String> Components;
		};

		class InterfaceBlockEntry : public InterfaceMetaData
		{
		public:
			int Offset = 0, Size = 0;
		};
		class InterfaceBlockMetaData
		{
		public:
			String Name;
			int Size = 0;
			EnumerableHashSet<InterfaceBlockEntry> Entries;
			EnumerableDictionary<String, String> Attributes;
			EnumerableHashSet<String> UserWorlds;
		};
		class ShaderMetaData
		{
		public:
			CoreLib::String ShaderName;
			CoreLib::EnumerableDictionary<CoreLib::String, WorldMetaData> Worlds;
			EnumerableDictionary<String, InterfaceBlockMetaData> InterfaceBlocks;
		};

		class CompiledShader
		{
		public:
			ShaderMetaData MetaData;
			EnumerableDictionary<String, RefPtr<InterfaceBlock>> InterfaceBlocks;
			EnumerableDictionary<String, RefPtr<CompiledWorld>> Worlds;
		};
		class CompiledFunction
		{
		public:
			EnumerableDictionary<String, RefPtr<ILType>> Parameters;
			RefPtr<ILType> ReturnType;
			RefPtr<CFGNode> Code;
			String Name;
		};
		class CompiledProgram
		{
		public:
			RefPtr<ConstantPool> ConstantPool = new Compiler::ConstantPool();
			List<RefPtr<CompiledShader>> Shaders;
			List<RefPtr<CompiledFunction>> Functions;
		};

		class ShaderChoiceValue
		{
		public:
			String WorldName, AlternateName;
			ShaderChoiceValue() = default;
			ShaderChoiceValue(String world, String alt)
			{
				WorldName = world;
				AlternateName = alt;
			}
			static ShaderChoiceValue Parse(String str);
			String ToString()
			{
				if (AlternateName.Length() == 0)
					return WorldName;
				else
					return WorldName + L":" + AlternateName;
			}
			bool operator == (const ShaderChoiceValue & val)
			{
				return WorldName == val.WorldName && AlternateName == val.AlternateName;
			}
			bool operator != (const ShaderChoiceValue & val)
			{
				return WorldName != val.WorldName || AlternateName != val.AlternateName;
			}
			int GetHashCode()
			{
				return WorldName.GetHashCode() ^ AlternateName.GetHashCode();
			}
		};

		class ShaderChoice
		{
		public:
			String ChoiceName;
			String DefaultValue;
			List<ShaderChoiceValue> Options;
		};

		class CompiledShaderSource
		{
		private:
			void PrintAdditionalCode(StringBuilder & sb, String userCode);
		public:
			String GlobalHeader;
			EnumerableDictionary<String, String> InputDeclarations; // indexed by world
			String OutputDeclarations;
			String GlobalDefinitions;
			String LocalDeclarations;
			String MainCode;
			CoreLib::Basic::EnumerableDictionary<CoreLib::String, CoreLib::String> ComponentAccessNames;
			String GetAllCodeGLSL(String additionalHeader, String additionalGlobalDeclaration, String preambleCode, String epilogCode);
			String GetAllCodeGLSL()
			{
				return GetAllCodeGLSL(L"", L"", L"", L"");
			}
			void ParseFromGLSL(String code);
		};

		void IndentString(StringBuilder & sb, String src);

		class CompileResult
		{
		private:
			ErrorWriter errWriter;
		public:
			bool Success;
			List<CompileError> ErrorList, WarningList;
			String ScheduleFile;
			RefPtr<CompiledProgram> Program;
			List<ShaderChoice> Choices;
			EnumerableDictionary<String, EnumerableDictionary<String, CompiledShaderSource>> CompiledSource; // file -> world -> code
			void PrintError(bool printWarning = false)
			{
				for (int i = 0; i < ErrorList.Count(); i++)
				{
					printf("%s(%d): error %d: %s\n", ErrorList[i].Position.FileName.ToMultiByteString(), ErrorList[i].Position.Line,
						ErrorList[i].ErrorID, ErrorList[i].Message.ToMultiByteString());
				}
				if (printWarning)
					for (int i = 0; i < WarningList.Count(); i++)
					{
						printf("%s(%d): warning %d: %s\n", WarningList[i].Position.FileName.ToMultiByteString(),
							WarningList[i].Position.Line, WarningList[i].ErrorID, WarningList[i].Message.ToMultiByteString());
					}
			}
			CompileResult()
				: errWriter(ErrorList, WarningList)
			{}
			ErrorWriter * GetErrorWriter()
			{
				return &errWriter;
			}
		};

	}
}

#endif

/***********************************************************************
SPIRECORE\CODEGENBACKEND.H
***********************************************************************/
#ifndef CODE_GEN_BACKEND_H
#define CODE_GEN_BACKEND_H


namespace Spire
{
	namespace Compiler
	{
		class ImportOperatorContext
		{
		public:
			EnumerableDictionary<String, String> & Arguments;
			EnumerableDictionary<String, String> & BackendArguments;
			CompiledWorld * SourceWorld, * DestWorld;
			CompileResult & Result;
			ImportOperatorContext(EnumerableDictionary<String, String> & args,
				EnumerableDictionary<String, String> & backendArgs,
				CompiledWorld * destWorld,
				CompileResult & result, CompiledWorld * srcWorld)
				: Arguments(args), BackendArguments(backendArgs), DestWorld(destWorld),
				Result(result), SourceWorld(srcWorld)
			{}
		};
		class ImportOperatorHandler
		{
		public:
			virtual String GetName() = 0;
			virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & context) = 0;
			virtual void GeneratePreamble(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & context) = 0;
			virtual void GenerateEpilogue(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & context) = 0;
			virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & context) = 0;
			virtual void GenerateSetInput(StringBuilder & sb, ComponentDefinition * gvar, const ImportOperatorContext & context) = 0;
		};

		class ExportOperatorHandler
		{
		public:
			virtual String GetName() = 0;
			virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block) = 0;
			virtual void GeneratePreamble(StringBuilder & sb, InterfaceBlock * block) = 0;
			virtual void GenerateEpilogue(StringBuilder & sb, InterfaceBlock * block) = 0;
			virtual void GenerateExport(StringBuilder & sb, InterfaceBlock * block, CompiledWorld * world, String compName, String valueVar) = 0;
		};

		class CodeGenBackend : public CoreLib::Basic::Object
		{
		public:
			virtual CompiledShaderSource GenerateShaderWorld(CompileResult & result, SymbolTable * symbols, CompiledWorld * shader,
				Dictionary<String, ImportOperatorHandler *> & opHandlers,
				Dictionary<String, ExportOperatorHandler *> & exportHandlers) = 0;
			virtual void SetParameters(EnumerableDictionary<String, String> & arguments) = 0;
		};

		CodeGenBackend * CreateGLSLCodeGen();
	}
}

#endif

/***********************************************************************
SPIRECORE\SHADERCOMPILER.H
***********************************************************************/
#ifndef RASTER_SHADER_COMPILER_H
#define RASTER_SHADER_COMPILER_H


namespace Spire
{
	namespace Compiler
	{
		class ILConstOperand;

		enum class CompilerMode
		{
			ProduceShader,
			GenerateChoice
		};

		class CompileOptions
		{
		public:
			CompilerMode Mode = CompilerMode::ProduceShader;
			String ScheduleSource, ScheduleFileName;
			String SymbolToCompile;
		};

		class CompileUnit
		{
		public:
			RefPtr<ProgramSyntaxNode> SyntaxNode;
		};

		class ShaderCompiler : public CoreLib::Basic::Object
		{
		public:
			virtual CompileUnit Parse(CompileResult & result, String source, String fileName) = 0;
			virtual void Compile(CompileResult & result, List<CompileUnit> & units, const CompileOptions & options) = 0;
			virtual void RegisterImportOperator(String backendName, ImportOperatorHandler * handler) = 0;
			virtual void RegisterExportOperator(String backendName, ExportOperatorHandler * handler) = 0;

		};
		ShaderCompiler * CreateShaderCompiler();
	}
}

#endif

/***********************************************************************
CORELIB\MD5.H
***********************************************************************/
/*
* This is an OpenSSL-compatible implementation of the RSA Data Security, Inc.
* MD5 Message-Digest Algorithm (RFC 1321).
*
* Homepage:
* http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
*
* Author:
* Alexander Peslyak, better known as Solar Designer <solar at openwall.com>
*
* This software was written by Alexander Peslyak in 2001.  No copyright is
* claimed, and the software is hereby placed in the public domain.
* In case this attempt to disclaim copyright and place the software in the
* public domain is deemed null and void, then the software is
* Copyright (c) 2001 Alexander Peslyak and it is hereby released to the
* general public under the following terms:
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted.
*
* There's ABSOLUTELY NO WARRANTY, express or implied.
*
* See md5.c for more information.
*/

#ifdef HAVE_OPENSSL
#include <openssl/md5.h>
#elif !defined(_MD5_H)
#define _MD5_H

/* Any 32-bit or wider unsigned integer data type will do */
typedef unsigned int MD5_u32plus;

typedef struct {
	MD5_u32plus lo, hi;
	MD5_u32plus a, b, c, d;
	unsigned char buffer[64];
	MD5_u32plus block[16];
} MD5_CTX;

extern void MD5_Init(MD5_CTX *ctx);
extern void MD5_Update(MD5_CTX *ctx, const void *data, unsigned long size);
extern void MD5_Final(unsigned char *result, MD5_CTX *ctx);

#endif

/***********************************************************************
CORELIB\PARSER.H
***********************************************************************/
#ifndef CORELIB_TEXT_PARSER_H
#define CORELIB_TEXT_PARSER_H


namespace CoreLib
{
	namespace Text
	{
		class TextFormatException : public Exception
		{
		public:
			TextFormatException(String message)
				: Exception(message)
			{}
		};

		const int TokenType_Identifier = 3;
		const int TokenType_Int = 4;
		const int TokenType_Float = 5;
		const int TokenType_StringLiteral = 6;
		const int TokenType_CharLiteral = 7;

		class Parser
		{
		private:
			static RefPtr<MetaLexer> metaLexer;
			LazyLexStream stream;
			bool legal;
			String text;
			List<LazyLexToken> tokens;
			int tokenPtr;
			LexToken MakeToken(LazyLexToken ltk)
			{
				LexToken tk;
				tk.Position = ltk.Position;
				tk.TypeID = ltk.TypeID;
				tk.Str = text.SubString(ltk.Position, ltk.Length);
				return tk;
			}
		public:
			static MetaLexer * GetTextLexer();
			static void DisposeTextLexer();
			static Basic::List<Basic::String> SplitString(Basic::String str, wchar_t ch);
			Parser(Basic::String text);
			int ReadInt()
			{
				auto token = ReadToken();
				bool neg = false;
				if (token.Str == L'-')
				{
					neg = true;
					token = ReadToken();
				}
				if (token.TypeID == TokenType_Int)
				{
					if (neg)
						return -StringToInt(token.Str);
					else
						return StringToInt(token.Str);
				}
				throw TextFormatException(L"Text parsing error: int expected.");
			}
			double ReadDouble()
			{
				auto token = ReadToken();
				bool neg = false;
				if (token.Str == L'-')
				{
					neg = true;
					token = ReadToken();
				}
				if (token.TypeID == TokenType_Float || token.TypeID == TokenType_Int)
				{
					if (neg)
						return -StringToDouble(token.Str);
					else
						return StringToDouble(token.Str);
				}
				throw TextFormatException(L"Text parsing error: floating point value expected.");
			}
			String ReadWord()
			{
				auto token = ReadToken();
				if (token.TypeID == TokenType_Identifier)
				{
					return token.Str;
				}
				throw TextFormatException(L"Text parsing error: identifier expected.");
			}
			String Read(const wchar_t * expectedStr)
			{
				auto token = ReadToken();
				if (token.Str == expectedStr)
				{
					return token.Str;
				}
				throw TextFormatException(L"Text parsing error: \'" + String(expectedStr) + L"\' expected.");
			}
			String Read(String expectedStr)
			{
				auto token = ReadToken();
				if (token.Str == expectedStr)
				{
					return token.Str;
				}
				throw TextFormatException(L"Text parsing error: \'" + expectedStr + L"\' expected.");
			}
			static String EscapeStringLiteral(String str)
			{
				StringBuilder sb;
				sb << L"\"";
				for (int i = 0; i < str.Length(); i++)
				{
					switch (str[i])
					{
					case L' ':
						sb << L"\\s";
						break;
					case L'\n':
						sb << L"\\n";
						break;
					case L'\r':
						sb << L"\\r";
						break;
					case L'\t':
						sb << L"\\t";
						break;
					case L'\v':
						sb << L"\\v";
						break;
					case L'\'':
						sb << L"\\\'";
						break;
					case L'\"':
						sb << L"\\\"";
						break;
					case L'\\':
						sb << L"\\\\";
						break;
					default:
						sb << str[i];
						break;
					}
				}
				sb << L"\"";
				return sb.ProduceString();
			}
			String UnescapeStringLiteral(String str)
			{
				StringBuilder sb;
				for (int i = 0; i < str.Length(); i++)
				{
					if (str[i] == L'\\' && i < str.Length() - 1)
					{
						switch (str[i + 1])
						{
						case L's':
							sb << L" ";
							break;
						case L't':
							sb << L'\t';
							break;
						case L'n':
							sb << L'\n';
							break;
						case L'r':
							sb << L'\r';
							break;
						case L'v':
							sb << L'\v';
							break;
						case L'\'':
							sb << L'\'';
							break;
						case L'\"':
							sb << L"\"";
							break;
						case L'\\':
							sb << L"\\";
							break;
						default:
							i = i - 1;
							sb << str[i];
						}
						i++;
					}
					else
						sb << str[i];
				}
				return sb.ProduceString();
			}
			String ReadStringLiteral()
			{
				auto token = ReadToken();
				if (token.TypeID == TokenType_StringLiteral)
				{
					return UnescapeStringLiteral(token.Str.SubString(1, token.Str.Length()-2));
				}
				throw TextFormatException(L"Text parsing error: string literal expected.");
			}
			void Back(int count)
			{
				tokenPtr -= count;
			}
			LexToken ReadToken()
			{
				if (tokenPtr < tokens.Count())
				{
					LexToken rs = MakeToken(tokens[tokenPtr]);
					tokenPtr++;
					return rs;
				}
				throw TextFormatException(L"Unexpected ending.");
			}
			LexToken NextToken()
			{
				if (tokenPtr < tokens.Count())
					return MakeToken(tokens[tokenPtr]);
				else
				{
					LexToken rs;
					rs.TypeID = -1;
					rs.Position = -1;
					return rs;
				}
			}
			bool LookAhead(String token)
			{
				if (tokenPtr < tokens.Count())
				{
					auto next = NextToken();
					return next.Str == token;
				}
				else
				{
					return false;
				}
			}
			bool IsEnd()
			{
				return tokenPtr == tokens.Count();
			}
		public:
			bool IsLegalText()
			{
				return legal;
			}
		};

		List<String> Split(String str, wchar_t c);
	}
}

#endif

/***********************************************************************
CORELIB\PERFORMANCECOUNTER.H
***********************************************************************/
#ifndef CORELIB_PERFORMANCE_COUNTER_H
#define CORELIB_PERFORMANCE_COUNTER_H

#include <chrono>

namespace CoreLib
{
	namespace Diagnostics
	{
		typedef std::chrono::high_resolution_clock::time_point TimePoint;
		typedef std::chrono::high_resolution_clock::duration Duration;
		class PerformanceCounter
		{
		public:
			static TimePoint Start();
			static Duration End(TimePoint counter);
			static float EndSeconds(TimePoint counter);
			static double ToSeconds(Duration duration);
		};
	}
}

#endif

/***********************************************************************
CORELIB\THREADING.H
***********************************************************************/
#ifndef CORE_LIB_THREADING_H
#define CORE_LIB_THREADING_H
#include <atomic>
#include <thread>
#include <mutex>

namespace CoreLib
{
	namespace Threading
	{
		class SpinLock
		{
		private:
			std::atomic_flag lck;
		public:
			SpinLock()
			{
				lck.clear();
			}
			inline bool TryLock()
			{
				return !lck.test_and_set(std::memory_order_acquire);
			}
			inline void Lock()
			{
				while (lck.test_and_set(std::memory_order_acquire))
				{
				}
			}
			inline void Unlock()
			{
				lck.clear(std::memory_order_release);
			}
			SpinLock & operator = (const SpinLock & /*other*/)
			{
				lck.clear();
				return *this;
			}
		};

		class ParallelSystemInfo
		{
		public:
			static int GetProcessorCount();
		};

		typedef CoreLib::Basic::Event<> ThreadProc;
		typedef CoreLib::Basic::Event<CoreLib::Basic::Object *> ThreadParameterizedProc;
		class Thread;

		class ThreadParam
		{
		public:
			Thread * thread;
			CoreLib::Basic::Object * threadParam;
		};

		enum class ThreadPriority
		{
			Normal,
			AboveNormal,
			Highest,
			Critical,
			BelowNormal,
			Lowest,
			Idle
		};

		class Thread : public CoreLib::Basic::Object
		{
			friend unsigned int __stdcall ThreadProcedure(ThreadParam& param);
		private:
			 ThreadParam internalParam;
		public:
			
		private:
			std::thread threadHandle;
			CoreLib::Basic::RefPtr<ThreadProc> threadProc;
			CoreLib::Basic::RefPtr<ThreadParameterizedProc> paramedThreadProc;
		public:
			Thread()
			{
				internalParam.threadParam = nullptr;
				internalParam.thread = this;
			}
			Thread(ThreadProc * p)
				: Thread()
			{
				Start(p);
			}
			Thread(ThreadParameterizedProc * p, CoreLib::Basic::Object * param)
				: Thread()
			{
				Start(p, param);
			}
			void Start(ThreadProc * p)
			{
				threadProc = p;
				threadHandle = std::thread(ThreadProcedure, internalParam);
			}
			void Start(ThreadParameterizedProc * p, CoreLib::Basic::Object * param)
			{
				paramedThreadProc = p;
				internalParam.thread = this;
				internalParam.threadParam = param;
				threadHandle = std::thread(ThreadProcedure, internalParam);
			}
			void Join()
			{
				if (threadHandle.joinable())
					threadHandle.join();
			}
			void Detach()
			{
				if (threadHandle.joinable())
					threadHandle.detach();
			}
			std::thread::id GetHandle()
			{
				return threadHandle.get_id();
			}
		};

		class Mutex : public CoreLib::Basic::Object
		{
		private:
			std::mutex handle;
		public:
			void Lock()
			{
				handle.lock();
			}
			bool TryLock()
			{
				return handle.try_lock();
			}
			void Unlock()
			{
				return handle.unlock();
			}
		};

		inline unsigned int __stdcall ThreadProcedure(ThreadParam& param)
		{
			if (param.thread->paramedThreadProc)
				param.thread->paramedThreadProc->Invoke(param.threadParam);
			else
				param.thread->threadProc->Invoke();
			return 0;
		}
	}
}

#endif

/***********************************************************************
SPIRELIB\SPIRELIB.H
***********************************************************************/
#ifndef LIB_BAKER_SL_H
#define LIB_BAKER_SL_H

#include <Windows.h>

namespace SpireLib
{
	class ShaderLibFile : public CoreLib::Basic::Object
	{
	public:
		CoreLib::Basic::String BinaryFileName, BinarySourceName, BinarySource;
		CoreLib::Basic::EnumerableDictionary<CoreLib::Basic::String, Spire::Compiler::CompiledShaderSource> Sources; // indexed by world
		Spire::Compiler::ShaderMetaData MetaData;
		void AddSource(CoreLib::Basic::String source, CoreLib::Text::Parser & parser);
		void SaveToFile(CoreLib::Basic::String fileName);
		ShaderLibFile() = default;
		void Clear();
		void Load(CoreLib::Basic::String fileName);
		bool ProduceBinary(CoreLib::Basic::String outputDir);
	};
	
	CoreLib::Basic::List<ShaderLibFile> CompileShaderSource(Spire::Compiler::CompileResult & result,
		CoreLib::Basic::String sourceFileName,
		Spire::Compiler::CompileOptions &options);

	class ShaderLib : public ShaderLibFile
	{
	public:
		Spire::Compiler::CompiledShaderSource GetWorldSource(CoreLib::Basic::String world);
		ShaderLib() = default;
		ShaderLib(CoreLib::Basic::String fileName);
		void Reload(CoreLib::Basic::String fileName);
		bool CompileFrom(CoreLib::Basic::String symbolName, CoreLib::Basic::String sourceFileName, CoreLib::Basic::String schedule, CoreLib::Basic::String outputDir);
	};
}

#endif

/***********************************************************************
SPIRECORE\CLOSURE.H
***********************************************************************/
#ifndef BAKERSL_SHADER_CLOSURE_H
#define BAKERSL_SHADER_CLOSURE_H

namespace Spire
{
	namespace Compiler
	{
		RefPtr<ShaderClosure> CreateShaderClosure(ErrorWriter * err, SymbolTable * symTable, ShaderSymbol * shader);
		void FlattenShaderClosure(ErrorWriter * err, ShaderClosure * shader);
	}
}

#endif

/***********************************************************************
SPIRECORE\SYNTAXVISITORS.H
***********************************************************************/
#ifndef RASTER_RENDERER_SYNTAX_PRINTER_H
#define RASTER_RENDERER_SYNTAX_PRINTER_H


namespace Spire
{
	namespace Compiler
	{
		class ShaderCompiler;
		class ShaderLinkInfo;
		class ShaderSymbol;

		class ICodeGenerator : public SyntaxVisitor
		{
		public:
			ICodeGenerator(ErrorWriter * perr)
				: SyntaxVisitor(perr)
			{}
			virtual void ProcessFunction(FunctionSyntaxNode * func) = 0;
			virtual void ProcessShader(ShaderClosure * shader) = 0;
		};

		SyntaxVisitor * CreateComponentDependencyVisitor(SymbolTable * symbols, ShaderSymbol * currentShader, 
			ShaderComponentSymbol * compSym,
			ErrorWriter * err,
			EnumerableHashSet<ShaderComponentSymbol *> & _dependentComponents,
			Dictionary<ShaderComponentSymbol*, SyntaxNode*> & _referenceNodes);
		SyntaxVisitor * CreateSemanticsVisitor(SymbolTable * symbols, ErrorWriter * err);
		ICodeGenerator * CreateCodeGenerator(ShaderCompiler * compiler, SymbolTable * symbols, CompileResult & result);
	}
}

#endif

/***********************************************************************
SPIRECORE\SCOPEDICTIONARY.H
***********************************************************************/
#ifndef RASTER_RENDERER_SCOPE_DICTIONARY_H
#define RASTER_RENDERER_SCOPE_DICTIONARY_H


using namespace CoreLib::Basic;

namespace Spire
{
	namespace Compiler
	{
		template <typename TKey, typename TValue>
		class ScopeDictionary
		{
		public:
			LinkedList<Dictionary<TKey, TValue>> dicts;
		public:
			void PushScope()
			{
				dicts.AddLast();
			}
			void PopScope()
			{
				dicts.Delete(dicts.LastNode());
			}
			bool TryGetValue(const TKey & key, TValue & value)
			{
				for (auto iter = dicts.LastNode(); iter; iter = iter->GetPrevious())
				{
					bool rs = iter->Value.TryGetValue(key, value);
					if (rs)
						return true;
				}
				return false;
			}
			bool TryGetValueInCurrentScope(const TKey & key, TValue & value)
			{
				return dicts.Last().TryGetValue(key, value);
			}
			void Add(const TKey & key, const TValue & value)
			{
				dicts.Last().Add(key, value);
			}
			void Set(const TKey & key, const TValue & value)
			{
				dicts.Last()[key] = value;
			}
		};
	}
}

#endif

/***********************************************************************
SPIRECORE\CODEWRITER.H
***********************************************************************/
#ifndef IL_CODE_WRITER_H
#define IL_CODE_WRITER_H


namespace Spire
{
	namespace Compiler
	{
		class CodeWriter
		{
		private:
			List<RefPtr<CFGNode>> cfgNode;
			ConstantPool * constantPool = nullptr;
		public:
			void SetConstantPool(ConstantPool * pool)
			{
				constantPool = pool;
			}
			CFGNode * GetCurrentNode()
			{
				return cfgNode.Last().Ptr();
			}
			void PushNode()
			{
				RefPtr<CFGNode> n = new CFGNode();
				cfgNode.Add(n);
			}
			RefPtr<CFGNode> PopNode()
			{
				auto rs = cfgNode.Last();
				cfgNode.SetSize(cfgNode.Count() - 1);
				return rs;
			}
			void Assign(ILType * type, ILOperand * dest, ILOperand * src) // handles base type and ILArrayType assignment
			{
				auto arrType = dynamic_cast<ILArrayType*>(type);
				if (arrType)
				{
					for (int i = 0; i < arrType->ArrayLength; i++)
					{
						auto srcAddr = Add(src, i);
						auto destAddr = Add(dest, i);
						Store(destAddr, Load(srcAddr));
					}
				}
				else
					Store(dest, Load(src));
			}
			void Select(ILOperand * cond, ILOperand * v0, ILOperand * v1)
			{
				cfgNode.Last()->InsertTail(new SelectInstruction(cond, v0, v1));
			}
			ILOperand * BitAnd(ILOperand * v0, ILOperand * v1)
			{
				auto instr = new BitAndInstruction(v0, v1);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			ILOperand * BitAnd(ILOperand * v0, int c)
			{
				auto instr = new BitAndInstruction(v0, constantPool->CreateConstant(c));
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			ILOperand * Add(ILOperand * v0, ILOperand * v1)
			{
				auto instr = new AddInstruction(v0, v1);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			ILOperand * Add(ILOperand * v0, int v1)
			{
				auto instr = new AddInstruction(v0, constantPool->CreateConstant(v1));
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			ILOperand * Mul(ILOperand * v0, ILOperand * v1)
			{
				auto instr = new MulInstruction(v0, v1);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			ILOperand * Copy(ILOperand * src)
			{
				auto rs = new CopyInstruction(src);
				cfgNode.Last()->InsertTail(rs);
				return rs;
			}
			ILOperand * Load(ILOperand * src, int offset)
			{
				if (offset == 0)
				{
					auto instr = new LoadInstruction(src);
					cfgNode.Last()->InsertTail(instr);
					return instr;
				}
				else
				{
					auto dest = new AddInstruction(src, constantPool->CreateConstant(offset));
					cfgNode.Last()->InsertTail(dest);
					auto instr = new LoadInstruction(dest);
					cfgNode.Last()->InsertTail(instr);
					return instr;
				}
			}
			ILOperand * Load(ILOperand * src)
			{
				auto instr = new LoadInstruction(src);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			ILOperand * Load(ILOperand * src, ILOperand * offset)
			{
				auto dest = new AddInstruction(src, offset);
				cfgNode.Last()->InsertTail(dest);
				return Load(dest);
			}
			StoreInstruction * Store(ILOperand * dest, ILOperand * value)
			{
				auto instr = new StoreInstruction(dest, value);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			MemberUpdateInstruction * Update(ILOperand * dest, ILOperand * offset, ILOperand * value)
			{
				auto instr = new MemberUpdateInstruction(dest, offset, value);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			MemberLoadInstruction * Retrieve(ILOperand * dest, ILOperand * offset)
			{
				auto instr = new MemberLoadInstruction(dest, offset);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			//AllocVarInstruction * AllocVar(ILType * type, ILOperand * size)
			//{
			//	auto arrType = dynamic_cast<ILArrayType*>(type);
			//	if (arrType)
			//	{
			//		// check: size must be constant 1. Do not support array of array in IL level.
			//		auto s = dynamic_cast<ILConstOperand*>(size);
			//		if (!s || s->IntValues[0] != 1)
			//			throw ArgumentException(L"AllocVar(arrayType, size): size must be constant 1.");
			//		auto instr = new AllocVarInstruction(arrType->BaseType, program.CreateConstant(arrType->ArrayLength));
			//		cfgNode->InsertTail(instr);
			//		return instr;
			//	}
			//	else
			//	{
			//		auto instr = new AllocVarInstruction(type, size);
			//		cfgNode->InsertTail(instr);
			//		return instr;
			//	}
			//}
			AllocVarInstruction * AllocVar(RefPtr<ILType> & type, ILOperand * size)
			{
				auto arrType = dynamic_cast<ILArrayType*>(type.Ptr());
				if (arrType)
				{
					// check: size must be constant 1. Do not support array of array in IL level.
					auto s = dynamic_cast<ILConstOperand*>(size);
					if (!s || s->IntValues[0] != 1)
						throw ArgumentException(L"AllocVar(arrayType, size): size must be constant 1.");
					auto instr = new AllocVarInstruction(arrType->BaseType, constantPool->CreateConstant(arrType->ArrayLength));
					cfgNode.Last()->InsertTail(instr);
					return instr;
				}
				else
				{
					auto instr = new AllocVarInstruction(type, size);
					cfgNode.Last()->InsertTail(instr);
					return instr;
				}
			}
			/*GLeaInstruction * GLea(ILType * type, const String & name)
			{
				auto arrType = dynamic_cast<ILArrayType*>(type);
				auto instr = new GLeaInstruction();
				if (arrType)
					instr->Type = new ILPointerType(arrType->BaseType);
				else
					instr->Type = new ILPointerType(type);
				instr->Name = name;
				instr->VariableName = name;
				cfgNode->InsertTail(instr);
				return instr;
			}*/
			GLeaInstruction * GLea(RefPtr<ILType> & type, const String & name)
			{
				auto instr = new GLeaInstruction();
				instr->Type = type;
				instr->Name = name;
				instr->VariableName = name;
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			FetchArgInstruction * FetchArg(ILType * type, int argId)
			{
				auto instr = new FetchArgInstruction(type);
				cfgNode.Last()->InsertTail(instr);
				instr->ArgId = argId;
				return instr;
			}
			
			void Insert(ILInstruction * instr)
			{
				cfgNode.Last()->InsertTail(instr);
			}
		};
	}
}

#endif

/***********************************************************************
SPIRECORE\CPPCODEGENINCLUDE.H
***********************************************************************/
#ifndef CPP_CODE_INCLUDE_H
#define CPP_CODE_INCLUDE_H
extern const char * CppCodeIncludeString1;
extern const char * CppCodeIncludeString2;
#endif

/***********************************************************************
SPIRECORE\PARSER.H
***********************************************************************/
#ifndef RASTER_RENDERER_PARSER_H
#define RASTER_RENDERER_PARSER_H


namespace Spire
{
	namespace Compiler
	{
		const int MaxExprLevel = 12;

		class Parser
		{
		private:
			int pos;
			List<RefPtr<Scope>> scopeStack;
			List<Token> & tokens;
			List<CompileError> & errors;
			String fileName;
			HashSet<String> typeNames;
			HashSet<String> classNames;
			void FillPosition(SyntaxNode * node)
			{
				int id = Math::Min(pos, tokens.Count() - 1);
				if (id >= 0)
				{
					node->Position = tokens[id].Position;
				}
				else
				{
					node->Position = CodePosition(0, 0, fileName);
				}
				node->Scope = scopeStack.Last();
			}
			void PushScope()
			{
				scopeStack.Add(new Scope());
				if (scopeStack.Count() > 1)
					scopeStack.Last()->Parent = scopeStack[scopeStack.Count() - 2].Ptr();
			}
			void PopScope()
			{
				scopeStack.Last() = 0;
				scopeStack.RemoveAt(scopeStack.Count() - 1);
			}
		public:
			Parser(List<Token> & _tokens, List<CompileError> & _errors, String _fileName)
				:tokens(_tokens), errors(_errors), pos(0), fileName(_fileName)
			{
				typeNames.Add(L"int");
				typeNames.Add(L"float");
				typeNames.Add(L"void");
				typeNames.Add(L"ivec2");
				typeNames.Add(L"ivec3");
				typeNames.Add(L"ivec4");
				typeNames.Add(L"vec2");
				typeNames.Add(L"vec3");
				typeNames.Add(L"vec4");
				typeNames.Add(L"mat3");
				typeNames.Add(L"mat4");
				typeNames.Add(L"sampler2D");
				typeNames.Add(L"sampler2DShadow");
				typeNames.Add(L"samplerCube");
				typeNames.Add(L"samplerCubeShadow");
			}
			RefPtr<ProgramSyntaxNode> Parse();
		private:
			Token & ReadToken(TokenType type);
			Token & ReadToken(const wchar_t * string);
			bool LookAheadToken(TokenType type, int offset = 0);
			bool LookAheadToken(const wchar_t * string, int offset = 0);
			Token & ReadTypeKeyword();
			VariableModifier ReadVariableModifier();
			bool IsTypeKeyword();
			EnumerableDictionary<String, String>	ParseAttribute();
			RefPtr<ProgramSyntaxNode>				ParseProgram();
			RefPtr<ShaderSyntaxNode>				ParseShader();
			RefPtr<PipelineSyntaxNode>				ParsePipeline();
			RefPtr<ComponentSyntaxNode>				ParseComponent();
			RefPtr<WorldSyntaxNode>					ParseWorld();
			RefPtr<RateSyntaxNode>					ParseRate();
			RefPtr<ImportSyntaxNode>				ParseImport();
			RefPtr<ImportStatementSyntaxNode>		ParseImportStatement();
			RefPtr<ImportOperatorDefSyntaxNode>		ParseImportOperator();
			RefPtr<FunctionSyntaxNode>				ParseFunction();
			RefPtr<StatementSyntaxNode>				ParseStatement();
			RefPtr<BlockStatementSyntaxNode>		ParseBlockStatement();
			RefPtr<VarDeclrStatementSyntaxNode>		ParseVarDeclrStatement();
			RefPtr<IfStatementSyntaxNode>			ParseIfStatement();
			RefPtr<ForStatementSyntaxNode>			ParseForStatement();
			RefPtr<WhileStatementSyntaxNode>		ParseWhileStatement();
			RefPtr<DoWhileStatementSyntaxNode>		ParseDoWhileStatement();
			RefPtr<BreakStatementSyntaxNode>		ParseBreakStatement();
			RefPtr<ContinueStatementSyntaxNode>		ParseContinueStatement();
			RefPtr<ReturnStatementSyntaxNode>		ParseReturnStatement();
			RefPtr<ExpressionStatementSyntaxNode>	ParseExpressionStatement();
			RefPtr<ExpressionSyntaxNode>			ParseExpression(int level = 0);
			RefPtr<ExpressionSyntaxNode>			ParseLeafExpression();
			RefPtr<ParameterSyntaxNode>				ParseParameter();
			RefPtr<TypeSyntaxNode>					ParseType();

			Parser & operator = (const Parser &) = delete;
		};
		
	}
}

#endif

/***********************************************************************
SPIRECORE\SCHEDULE.H
***********************************************************************/
#ifndef BAKER_SL_SCHEDULE_H
#define BAKER_SL_SCHEDULE_H


namespace Spire
{
	namespace Compiler
	{
		class Schedule
		{
		public:
			CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::List<RefPtr<ChoiceValueSyntaxNode>>> Choices;
			CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::String>> AddtionalAttributes;
			static Schedule Parse(CoreLib::String source, CoreLib::String fileName, CoreLib::List<CompileError> & errorList);
		};
	}
}

#endif

/***********************************************************************
SPIRECORE\STDINCLUDE.H
***********************************************************************/
#ifndef SHADER_COMPILER_STD_LIB_H
#define SHADER_COMPILER_STD_LIB_H

extern const wchar_t * LibIncludeString;
extern const wchar_t * VertexShaderIncludeString;
#endif

/***********************************************************************
SPIRELIB\IMPORTOPERATOR.H
***********************************************************************/
#ifndef IMPORT_OPERATOR_HANDLERS_H
#define IMPORT_OPERATOR_HANDLERS_H


void CreateCppImportOperatorHandlers(CoreLib::Basic::List<Spire::Compiler::ImportOperatorHandler *> & handlers);
void CreateGLSLImportOperatorHandlers(CoreLib::Basic::List<Spire::Compiler::ImportOperatorHandler *> & handlers);
void CreateGLSLExportOperatorHandlers(CoreLib::Basic::List<Spire::Compiler::ExportOperatorHandler *> & handlers);
void DestroyImportOperatorHanlders(CoreLib::Basic::List<Spire::Compiler::ImportOperatorHandler *> & handlers);
void DestroyExportOperatorHanlders(CoreLib::Basic::List<Spire::Compiler::ExportOperatorHandler *> & handlers);

#endif

/***********************************************************************
SPIRECORE\SHADERINTERFACE.H
***********************************************************************/
#ifndef SHADER_INTERFACE_H
#define SHADER_INTERFACE_H

class ITexture2D
{
public:
	virtual void GetValue(float * value, float u, float v, float dudx, float dudy, float dvdx, float dvdy) = 0;
};

class ITexture3D
{
public:
	virtual void GetValue(float * value, float u, float v, float w, float lod) = 0;
};


class IShader
{
public:
	virtual void SetUniformInput(const char * name, void * value) = 0;
	virtual void SetInput(const char * name, void * buffer) = 0;
	virtual void SetInputSize(int n) = 0;
	virtual bool GetOutput(const char * name, void * buffer, int & bufferSize) = 0;
	virtual void Run() = 0;
};

#endif
