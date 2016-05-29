#ifndef CORE_LIB_TEXT_IO_H
#define CORE_LIB_TEXT_IO_H

#include "SecureCRT.h"
#include "Stream.h"
#include "WideChar.h"
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
#ifdef _WIN32
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
