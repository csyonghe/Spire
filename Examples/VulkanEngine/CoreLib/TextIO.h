#ifndef CORE_LIB_TEXT_IO_H
#define CORE_LIB_TEXT_IO_H

#include "SecureCRT.h"
#include "Stream.h"
#include "WideChar.h"

namespace CoreLib
{
	namespace IO
	{
		using CoreLib::Basic::List;
		using CoreLib::Basic::_EndLine;

		class TextReader : public CoreLib::Basic::Object
		{
		private:
			wchar_t lookAhead = 0;
			bool hasLookAhead = false;
		protected:
			virtual wchar_t ReadChar() = 0;
		public:
			~TextReader()
			{
				Close();
			}
			virtual void Close(){}
			virtual String ReadLine()=0;
			virtual String ReadToEnd()=0;
			virtual bool IsEnd() = 0;
			int Read(wchar_t * buffer, int count);
			wchar_t Read()
			{
				if (!hasLookAhead)
					return ReadChar();
				else
				{
					hasLookAhead = false;
					return lookAhead;
				}
			}
			wchar_t Peak()
			{
				if (hasLookAhead)
					return lookAhead;
				lookAhead = Read();
				hasLookAhead = true;
				return lookAhead;
			}
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
			static Encoding * UTF8, * Ansi, * UTF16, *UTF16Reversed;
			virtual void GetBytes(List<char> & buffer, const String & str)=0;
			virtual ~Encoding()
			{}
		};

		class StreamWriter : public TextWriter
		{
		private:
			List<char> encodingBuffer;
			RefPtr<Stream> stream;
			Encoding * encoding;
		public:
			StreamWriter(const String & path, Encoding * encoding = Encoding::UTF8);
			StreamWriter(RefPtr<Stream> stream, Encoding * encoding = Encoding::UTF8);
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
			wchar_t lowSurrogate = 0;
			bool hasLowSurrogate = false;
			RefPtr<Stream> stream;
			List<char> buffer;
			Encoding * encoding;
			int ptr;
			char ReadBufferChar();
			void ReadBuffer();
			template<typename GetFunc>
			wchar_t GetChar(GetFunc get)
			{
				wchar_t decoded = 0;
				if (encoding == Encoding::UTF8)
				{
					if (hasLowSurrogate)
					{
						hasLowSurrogate = false;
						return lowSurrogate;
					}
					int codePoint = 0;
					int leading = get(0);
					int mask = 0x80;
					int count = 0;
					while (leading & mask)
					{
						count++;
						mask >>= 1;
					}
					codePoint = (leading & (mask - 1));
					for (int i = 1; i <= count - 1; i++)
					{
						codePoint <<= 6;
						codePoint += (get(i) & 0x3F);
					}
#ifdef _WIN32
					if (codePoint <= 0xD7FF || (codePoint >= 0xE000 && codePoint <= 0xFFFF))
						return (wchar_t)codePoint;
					else
					{
						int sub = codePoint - 0x10000;
						int high = (sub >> 10) + 0xD800;
						int low = (sub & 0x3FF) + 0xDC00;
						hasLowSurrogate = true;
						lowSurrogate = (wchar_t)low;
						return (wchar_t)high;
					}
#else
					return (wchar_t)codePoint; // linux platforms use UTF32
#endif
				}
				else if (encoding == Encoding::UTF16)
				{
					decoded = get(0) + (get(1) << 8);
#ifndef _WIN32
					if (decoded >= 0xD800 && decoded <= 0xDBFF) // high surrogate detected
					{
						unsigned short lowSurrogate = get(2) + (get(3) << 8);
						decoded = ((decoded - 0xD800) << 10) + (lowSurrogate - 0xDC00);
					}
#endif
					return decoded;
				}
				else if (encoding == Encoding::UTF16Reversed)
				{
					decoded = (get(0) << 8) + get(1);
#ifndef _WIN32
					if (decoded >= 0xD800 && decoded <= 0xDBFF) // high surrogate detected
					{
						unsigned short lowSurrogate = (get(2) << 8) + get(3);
						decoded = ((decoded - 0xD800) << 10) + (lowSurrogate - 0xDC00);
					}
#endif
					return decoded;
				}
				else
				{
					return get(0);
				}
			}
			Encoding * DetermineEncoding();
		protected:
			virtual wchar_t ReadChar()
			{
				return GetChar([&](int) {return ReadBufferChar(); });
			}
		public:
			StreamReader(const String & path);
			StreamReader(RefPtr<Stream> stream, Encoding * encoding = nullptr);
			virtual String ReadLine();
			virtual String ReadToEnd();
			virtual bool IsEnd()
			{
				return ptr == buffer.Count() && stream->IsEnd();
			}
			virtual void Close()
			{
				stream->Close();
			}
		};

	}
}

#endif
