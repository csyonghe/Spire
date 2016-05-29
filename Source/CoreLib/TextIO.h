#ifndef CORE_LIB_TEXT_IO_H
#define CORE_LIB_TEXT_IO_H

#include "SecureCRT.h"
#include "Stream.h"
#include "WideChar.h"
#include <locale>
#include <codecvt>
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
			virtual void Write(const wchar_t * str)=0;
			virtual void Write(const char * str)=0;
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
				Write(value);
				return *this;
			}
			TextWriter & operator << (const wchar_t * const val)
			{
				Write(val);
				return *this;
			}
			TextWriter & operator << (wchar_t * const val)
			{
				Write(val);
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
				Write(L"\r\n");
#else
				Write(L"\n");
#endif
				return *this;
			}
		};

		class Encoding
		{
		public:
			static Encoding * Unicode, * Ansi, * UTF16;
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
			virtual void Write(const wchar_t * str);
			virtual void Write(const char * str);
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
				wchar_t decoded = 0;
				char charBuffer[5] = {0, 0, 0, 0, 0};
				if (encoding == Encoding::Unicode)
				{
					int leading = get(0);
					charBuffer[0] = (char)leading;
					if (leading < 0)
					{
						int c = get(1);
						charBuffer[1] = (char)c;
						if (leading & 0b00100000)
						{
							c = get(2);
							charBuffer[2] = (char)c;
							if (leading & 0b00010000)
								charBuffer[3] = (char)get(3);
								// ignore decoding beyond 0xFFFF
						}
					}
					std::wstring_convert<std::codecvt_utf8<wchar_t>> cvt;
					auto str = cvt.from_bytes(charBuffer);
					if (str.length() > 0)
						return str.at(0);
					else
						return 0;
				}
				else if (encoding == Encoding::UTF16)
				{
					((char*)&decoded)[0] = get(0);
					((char*)&decoded)[1] = get(1);
					return decoded;
				}
				else
				{
				
					char mb[2];
					mb[0] = get(0);
					if (_ismbblead(mb[0]))
					{
						mb[1] = get(1);
						MByteToWideChar(&decoded, 1, mb, 2);
					}
					else
					{
						decoded = mb[0];
					}
					return decoded;
				}
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
