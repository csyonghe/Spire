#include "TextIO.h"
#include <ctype.h>
#include <codecvt>
#ifdef _WIN32
#include <Windows.h>
#define CONVERT_END_OF_LINE
#endif

namespace CoreLib
{
	namespace IO
	{
		using namespace CoreLib::Basic;

		class UnicodeEncoding : public Encoding //UTF8
		{
		public:
			virtual List<char> GetBytes(const String & str)
			{
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				const std::string rs = converter.to_bytes(str.Buffer(), str.Buffer() + str.Length());
				List<char> result;
				result.Reserve(rs.length());
				result.AddRange(rs.data(), rs.length());
				return result;
			}

			virtual String GetString(char * buffer, int length)
			{
				std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
				const std::wstring rs = converter.from_bytes(buffer, buffer + length);
				return String(rs.c_str());
			}
		};

		class Utf16Encoding : public Encoding //UTF16
		{
		public:
			virtual List<char> GetBytes(const String & str)
			{
				std::wstring_convert<std::codecvt_utf16<wchar_t>> converter;
				const std::string rs = converter.to_bytes(str.Buffer(), str.Buffer() + str.Length());
				List<char> result;
				result.Reserve(rs.length());
				result.AddRange(rs.data(), rs.length());
				return result;
			}

			virtual String GetString(char * buffer, int length)
			{
				std::wstring_convert<std::codecvt_utf16<wchar_t>> converter;
				const std::wstring rs = converter.from_bytes(buffer, buffer + length);
				return String(rs.c_str());
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
		Utf16Encoding __utf16Encoding;
		AnsiEncoding __ansiEncoding;

		Encoding * Encoding::Unicode = &__unicodeEncoding;
		Encoding * Encoding::UTF16 = &__utf16Encoding;

		Encoding * Encoding::Ansi = &__ansiEncoding;

		const unsigned char Utf16Header[] = { 0xFE,0xFF };

		const unsigned char Utf8Header[] = { 0xEF,0xBB,0xBF };

		StreamWriter::StreamWriter(const String & path, Encoding * encoding)
		{
			this->stream = new FileStream(path, FileMode::Create);
			this->encoding = encoding;
			if (encoding == Encoding::Unicode)
			{
				//this->stream->Write(Utf8Header, 3);
			}
			else if (encoding == Encoding::UTF16)
			{
				this->stream->Write(Utf16Header, 2);
			}
		}
		StreamWriter::StreamWriter(RefPtr<Stream> stream, Encoding * encoding)
		{
			this->stream = stream;
			this->encoding = encoding;
			if (encoding == Encoding::Unicode)
			{
				//this->stream->Write(Utf8Header, 3);
			}
			else if (encoding == Encoding::UTF16)
			{
				this->stream->Write(Utf16Header, 2);
			}
		}
		void StreamWriter::Write(const String & str)
		{
			auto bytes = encoding->GetBytes(str);
			stream->Write(bytes.Buffer(), bytes.Count());
		}
		void StreamWriter::Write(const wchar_t * str)
		{
			auto bytes = encoding->GetBytes(String(str));
			stream->Write(bytes.Buffer(), bytes.Count());
		}
		void StreamWriter::Write(const char * str)
		{
			auto bytes = encoding->GetBytes(String(str));
			stream->Write(bytes.Buffer(), bytes.Count());
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
			if ((unsigned char)(buffer[0]) == 0xEF && (unsigned char)(buffer[1]) == 0xBB)
			{
				ptr += 3;
				return Encoding::Unicode;
			}
			else if (*((unsigned short*)(buffer.Buffer())) == 0xFEFF || *((unsigned short*)(buffer.Buffer())) == 0xFFFE)
			{
				ptr += 2;
				return Encoding::UTF16;
			}
			else
			{
#ifdef _WIN32
				int flag = IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_REVERSE_SIGNATURE | IS_TEXT_UNICODE_STATISTICS;
				int rs = IsTextUnicode(buffer.Buffer(), buffer.Count(), &flag);
				if (flag & (IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_REVERSE_SIGNATURE | IS_TEXT_UNICODE_STATISTICS))
					return Encoding::Unicode;
				else if (rs)
					return Encoding::Ansi;
				else
					return Encoding::Unicode;
#else
				return Encoding::Unicode;
#endif
			}
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