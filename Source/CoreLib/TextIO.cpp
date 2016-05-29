#include "TextIO.h"
#include <ctype.h>
#ifdef WINDOWS_PLATFORM
#include <Windows.h>
#define CONVERT_END_OF_LINE
#endif

namespace CoreLib
{
	namespace IO
	{
		using namespace CoreLib::Basic;

		class UnicodeEncoding : public Encoding
		{
		public:
			virtual List<char> GetBytes(const String & str)
			{
				List<char> rs;
				rs.Reserve(str.Length());
				rs.AddRange((char*)str.Buffer(), str.Length()*2);
				return rs;
			}

			virtual String GetString(char * buffer, int length)
			{
#ifdef WINDOWS_PLATFORM
				int unicodeFlag;
				int startPos = 0;
				IsTextUnicode(buffer, length, &unicodeFlag);
				if (unicodeFlag & IS_TEXT_UNICODE_UNICODE_MASK)
				{
					if (unicodeFlag & IS_TEXT_UNICODE_SIGNATURE)
					{
						startPos += 2;
					}
					length = (int)wcsnlen_s((wchar_t*)buffer+startPos, length-startPos);
					wchar_t * rbuffer = new wchar_t[length+1];
					memcpy(rbuffer, buffer, length*sizeof(wchar_t));
					return String::FromBuffer(rbuffer, length);
				}
				else if (unicodeFlag & IS_TEXT_UNICODE_REVERSE_MASK)
				{
					if (unicodeFlag & IS_TEXT_UNICODE_REVERSE_SIGNATURE)
					{
						startPos += 2;
					}
					length = (int)wcsnlen_s((wchar_t*)buffer+startPos, length-startPos);
					wchar_t * rbuffer = new wchar_t[length+1];
					for (int i = 0; i<length; i++)
					{
						((char*)rbuffer)[i*2] = buffer[i*2+1];
						((char*)rbuffer)[i*2+1] = buffer[i*2];
					}
					rbuffer[length] = 0;
					return String::FromBuffer(rbuffer, length);
				}
				else
				{
					throw ArgumentException(L"Invalid unicode text format.");
				}
#else
				throw NotImplementedException(L"UnicodeEncoding::GetString() not implemented for current platform.");
#endif
			}
		};

		class AnsiEncoding : public Encoding
		{
		private:
			static char * WideCharToAnsi(wchar_t * buffer, int length)
			{
				return WideCharToMByte(buffer, length);
			}
		public:
			virtual List<char> GetBytes(const String & str)
			{
				List<char> rs;
				String cpy = str;
				int len;
				char * buffer = cpy.ToMultiByteString(&len);
				rs.AddRange(buffer, len);
				return rs;
			}

			virtual String GetString(char * buffer, int length)
			{
				auto rbuffer = MByteToWideChar(buffer, length);
				return String::FromBuffer(rbuffer, length);
			}
		};

		UnicodeEncoding __unicodeEncoding;
		AnsiEncoding __ansiEncoding;

		Encoding * Encoding::Unicode = &__unicodeEncoding;
		Encoding * Encoding::Ansi = &__ansiEncoding;

		const unsigned char UnicodeHeader[] = {0xFF, 0xFE};

		StreamWriter::StreamWriter(const String & path, Encoding * encoding)
		{
			this->stream = new FileStream(path, FileMode::Create);
			this->encoding = encoding;
			if (encoding == Encoding::Unicode)
			{
				this->stream->Write(UnicodeHeader, 2);
			}
		}
		StreamWriter::StreamWriter(RefPtr<Stream> stream, Encoding * encoding)
		{
			this->stream = stream;
			this->encoding = encoding;
			if (encoding == Encoding::Unicode)
			{
				this->stream->Write(UnicodeHeader, 2);
			}
		}
		void StreamWriter::Write(const String & str)
		{
			if (encoding == Encoding::Unicode)
			{
				Write(str.Buffer(), str.Length());
			}
			else
			{
				auto bytes = encoding->GetBytes(String(str));
				Write(bytes.Buffer(), bytes.Count());
			}
		}
		void StreamWriter::Write(const wchar_t * str, int length)
		{
			if (encoding == Encoding::Unicode)
			{
				if (length == 0)
					length = (int)wcslen(str);
#ifdef CONVERT_END_OF_LINE
				for (int i = 0; i < length; i++)
				{
					if (str[i] == L'\r' && (i == length - 1 || str[i + 1] != L'\n'))
						stream->Write(L"\r\n", sizeof(wchar_t)*2);
					else if (str[i] == L'\n' && (i == 0 || str[i - 1] != L'\r'))
						stream->Write(L"\r\n", sizeof(wchar_t) * 2);
					else
						stream->Write(str+i, sizeof(wchar_t));
				}
#else
				stream->Write(str, length*sizeof(wchar_t));
#endif
			}
			else
			{
				auto bytes = encoding->GetBytes(String(str));
				stream->Write(bytes.Buffer(), bytes.Count());
			}
		}
		void StreamWriter::Write(const char * str, int length)
		{
			if (encoding == Encoding::Ansi)
			{
				if (length == 0)
					length = (int)strlen(str);
#ifdef CONVERT_END_OF_LINE
				for (int i = 0; i < length; i++)
				{
					if (str[i] == '\r' && (i == length - 1 || str[i + 1] != '\n'))
						stream->Write("\r\n", sizeof(char) * 2);
					else if (str[i] == '\n' && (i == 0 || str[i - 1] != '\r'))
						stream->Write("\r\n", sizeof(char) * 2);
					else
						stream->Write(str + i, sizeof(char));
				}
#else
				stream->Write(str, length*sizeof(char));
#endif
			}
			else
			{
				String cpy(str);
				stream->Write(cpy.Buffer(), sizeof(wchar_t) * cpy.Length());
			}
		}

		StreamReader::StreamReader(const String & path)
		{
			stream = new FileStream(path, FileMode::Open);
			ReadBuffer();
			encoding = DetermineEncoding();
			if (encoding == 0)
				encoding = Encoding::Ansi;
		}
		StreamReader::StreamReader(RefPtr<Stream> stream, Encoding * encoding)
		{
			this->stream = stream;
			this->encoding = encoding;
			ReadBuffer();
			this->encoding = DetermineEncoding();
			if (this->encoding == 0)
				this->encoding = encoding;
		}

		Encoding * StreamReader::DetermineEncoding()
		{
			if ((unsigned char)(buffer[0]) == 0xFE && (unsigned char)(buffer[1]) == 0xFF ||
				(unsigned char)(buffer[1]) == 0xFE && (unsigned char)(buffer[0]) == 0xFF)
			{
				ptr += 2;
				return Encoding::Unicode;
			}
			int flag = IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_REVERSE_SIGNATURE | IS_TEXT_UNICODE_STATISTICS;
#ifdef WINDOWS_PLATFORM
			int rs = IsTextUnicode(buffer.Buffer(), buffer.Count(), &flag);
			if (flag & (IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_REVERSE_SIGNATURE | IS_TEXT_UNICODE_STATISTICS))
				return Encoding::Unicode;
			else if (rs)
				return Encoding::Ansi;
			else
				return 0;
#else
			return Encoding::Ansi;
#endif
		}
		
		void StreamReader::ReadBuffer()
		{
			buffer.SetSize(4096);
			auto len = stream->Read(buffer.Buffer(), buffer.Count());
			buffer.SetSize((int)len);
			ptr = 0;
		}

		char StreamReader::ReadBufferChar()
		{
			if (ptr<buffer.Count())
			{
				return buffer[ptr++];
			}
			ReadBuffer();
			if (ptr<buffer.Count())
			{
				return buffer[ptr++];
			}
			return 0;
		}

		char StreamReader::PeakBufferChar(int offset)
		{
			if (ptr + offset < buffer.Count())
			{
				return buffer[ptr + offset];
			}
			try
			{
				ReadBuffer();
			}
			catch (EndOfStreamException)
			{
				buffer.Clear();
				ptr = 0;
			}
			if (ptr + offset<buffer.Count())
			{
				return buffer[ptr + offset];
			}
			return 0;
		}
		int StreamReader::Read(wchar_t * destBuffer, int length)
		{
			int i = 0;
			for (i = 0; i<length; i++)
			{
				try
				{
					auto ch = Read();
					if (ch == L'\r')
					{
						if (Peak() == L'\n')
							Read();
						break;
					}
					else if (ch == L'\n')
					{
						break;
					}
					destBuffer[i] = ch;
				}
				catch (EndOfStreamException)
				{
					break;
				}
			}
			return i;
		}
		String StreamReader::ReadLine()
		{
			StringBuilder sb(256);
#pragma warning (suppress : 4127)
			while (true)
			{
				try
				{
					auto ch = Read();
					if (ch == L'\r')
					{
						if (Peak() == L'\n')
							Read();
						break;
					}
					else if (ch == L'\n')
					{
						break;
					}
					sb.Append(ch);
				}
				catch (EndOfStreamException)
				{
					break;
				}
			}
			return sb.ProduceString();
		}
		String StreamReader::ReadToEnd()
		{
			StringBuilder sb(16384);
#pragma warning (suppress : 4127)
			while (true)
			{
				try
				{
					auto ch = Read();
					if (ch == L'\r')
					{
						sb.Append(L'\n');
						if (Peak() == L'\n')
							Read();
					}
					else
						sb.Append(ch);
				}
				catch (EndOfStreamException)
				{
					break;
				}
			}
			return sb.ProduceString();
		}
	}
}