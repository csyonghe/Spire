#ifndef FUNDAMENTAL_LIB_STRING_H
#define FUNDAMENTAL_LIB_STRING_H
#include <string.h>
#include <cstdlib>
#include <stdio.h>
#include "WideChar.h"
#include "SmartPointer.h"
#include "Common.h"
#include "SecureCRT.h"

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
			const wchar_t * begin() const
			{
				return buffer.Ptr();
			}
			const wchar_t * end() const
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

			int IndexOf(const wchar_t * str, int id) const // String str
			{
				if(!buffer)
					return -1;
				if (id < 0 || id >= length)
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

			int IndexOf(const wchar_t * str) const
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

			bool Contains(const wchar_t * str) const // String str
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
			String ReplaceAll(String src, String dst) const;
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

		int StringToInt(const String & str, int radix = 10);
		unsigned int StringToUInt(const String & str, int radix = 10);
		double StringToDouble(const String & str);

		
	}
}

#endif
