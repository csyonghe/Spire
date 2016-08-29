#include "TextIO.h"
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
			virtual void GetBytes(List<char> & result, const String & str) override
			{
				for (int i = 0; i < str.Length(); i++)
				{
					unsigned int codePoint = str[i];
					if (codePoint >= 0xD800 && codePoint <= 0xDBFF && i < str.Length() - 1) // surrogate
					{
						codePoint -= 0xD800;
						codePoint <<= 10;
						i++;
						codePoint += str[i] - 0xDC00;
						codePoint += 0x10000;
					}
					// encode codePoint as UTF8
					if (codePoint <= 0x7F)
						result.Add((char)codePoint);
					else if (codePoint <= 0x7FF)
					{
						unsigned char byte = (unsigned char)(0xC0 + (codePoint >> 6));
						result.Add((char)byte);
						byte = 0x80 + (codePoint & 0x3F);
						result.Add((char)byte);
					}
					else if (codePoint <= 0xFFFF)
					{
						unsigned char byte = (unsigned char)(0xE0 + (codePoint >> 12));
						result.Add((char)byte);
						byte = (unsigned char)(0x80 + ((codePoint >> 6) & (0x3F)));
						result.Add((char)byte);
						byte = (unsigned char)(0x80 + (codePoint & 0x3F));
						result.Add((char)byte);
					}
					else
					{
						unsigned char byte = (unsigned char)(0xF0 + (codePoint >> 18));
						result.Add((char)byte);
						byte = (unsigned char)(0x80 + ((codePoint >> 12) & 0x3F));
						result.Add((char)byte);
						byte = (unsigned char)(0x80 + ((codePoint >> 6) & 0x3F));
						result.Add((char)byte);
						byte = (unsigned char)(0x80 + (codePoint & 0x3F));
						result.Add((char)byte);
					}
				}
			}
		};

		class Utf16Encoding : public Encoding //UTF16
		{
		private:
			bool reverseOrder = false;
		public:
			Utf16Encoding(bool pReverseOrder)
				: reverseOrder(pReverseOrder)
			{}
			virtual void GetBytes(List<char> & result, const String & str) override
			{
				auto addChar = [&](unsigned short ch)
				{
					if (reverseOrder)
					{
						unsigned char firstByte = ch >> 8;
						unsigned char lastByte = ch & 0xFF;
						result.Add((char)firstByte);
						result.Add((char)lastByte);
					}
					else
						result.AddRange((char*)&ch, 2);
				};
#ifdef _WIN32
				if (reverseOrder)
				{
					for (int i = 0; i < str.Length(); i++)
					{
						unsigned short ch = (unsigned short)str[i];
						addChar(ch);
					}
				}
				else
					result.AddRange((char*)str.Buffer(), str.Length() * sizeof(wchar_t));
#else
				for (int i = 0; i < str.Length(); i++)
				{
					unsigned int codePoint = str[i];
					if (codePoint <= 0xD7FF || codePoint >= 0xE000 && codePoint <= 0xFFFF)
					{
						unsigned short toWrite = (unsigned short)codePoint;
						addChar(toWrite);
					}
					else
					{
						int sub = codePoint - 0x10000;
						unsigned short high = (unsigned short)((sub >> 10) + 0xD800);
						unsigned short low = (unsigned short)((sub & 0x3FF) + 0xDC00);
						addChar(high);
						addChar(low);
					}
				}
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
			virtual void GetBytes(List<char> & result, const String & str) override
			{
				String cpy = str;
				int len;
				char * buffer = cpy.ToMultiByteString(&len);
				result.AddRange(buffer, len);
			}
		};

		UnicodeEncoding __unicodeEncoding;
		Utf16Encoding __utf16Encoding(false);
		Utf16Encoding __utf16EncodingReversed(true);
		AnsiEncoding __ansiEncoding;

		Encoding * Encoding::UTF8 = &__unicodeEncoding;
		Encoding * Encoding::UTF16 = &__utf16Encoding;
		Encoding * Encoding::UTF16Reversed = &__utf16EncodingReversed;
		Encoding * Encoding::Ansi = &__ansiEncoding;

		const unsigned short Utf16Header = 0xFEFF;
		const unsigned short Utf16ReversedHeader = 0xFFFE;

		StreamWriter::StreamWriter(const String & path, Encoding * encoding)
		{
			this->stream = new FileStream(path, FileMode::Create);
			this->encoding = encoding;
			if (encoding == Encoding::UTF16)
			{
				this->stream->Write(&Utf16Header, 2);
			}
			else if (encoding == Encoding::UTF16Reversed)
			{
				this->stream->Write(&Utf16ReversedHeader, 2);
			}
		}
		StreamWriter::StreamWriter(RefPtr<Stream> stream, Encoding * encoding)
		{
			this->stream = stream;
			this->encoding = encoding;
			if (encoding == Encoding::UTF16)
			{
				this->stream->Write(&Utf16Header, 2);
			}
			else if (encoding == Encoding::UTF16Reversed)
			{
				this->stream->Write(&Utf16ReversedHeader, 2);
			}
		}
		void StreamWriter::Write(const String & str)
		{
			encodingBuffer.Clear();
			StringBuilder sb;
			String newLine;
#ifdef _WIN32
			newLine = L"\r\n";
#else
			newLine = L"\n";
#endif
			for (int i = 0; i < str.Length(); i++)
			{
				if (str[i] == L'\r')
					sb << newLine;
				else if (str[i] == L'\n')
				{
					if (i > 0 && str[i - 1] != L'\r')
						sb << newLine;
				}
				else
					sb << str[i];
			}
			encoding->GetBytes(encodingBuffer, sb.ProduceString());
			stream->Write(encodingBuffer.Buffer(), encodingBuffer.Count());
		}
		void StreamWriter::Write(const wchar_t * str)
		{
			Write(String(str));
		}
		void StreamWriter::Write(const char * str)
		{
			Write(String(str));
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
			auto determinedEncoding = DetermineEncoding();
			if (this->encoding == nullptr)
				this->encoding = determinedEncoding;
		}

		Encoding * StreamReader::DetermineEncoding()
		{
			if (buffer.Count() >= 3 && (unsigned char)(buffer[0]) == 0xEF && (unsigned char)(buffer[1]) == 0xBB && (unsigned char)(buffer[2]) == 0xBF)
			{
				ptr += 3;
				return Encoding::UTF8;
			}
			else if (*((unsigned short*)(buffer.Buffer())) == 0xFEFF)
			{
				ptr += 2;
				return Encoding::UTF16;
			}
			else if (*((unsigned short*)(buffer.Buffer())) == 0xFFFE)
			{
				ptr += 2;
				return Encoding::UTF16Reversed;
			}
			else
			{
#ifdef _WIN32
				int flag = IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_REVERSE_SIGNATURE | IS_TEXT_UNICODE_STATISTICS | IS_TEXT_UNICODE_ASCII16;
				int rs = IsTextUnicode(buffer.Buffer(), buffer.Count(), &flag);
				if (rs)
				{
					if (flag & (IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_STATISTICS))
						return Encoding::UTF16;
					else if (flag & (IS_TEXT_UNICODE_SIGNATURE | IS_TEXT_UNICODE_STATISTICS))
						return Encoding::UTF16Reversed;
					else if (flag & IS_TEXT_UNICODE_ASCII16)
						return Encoding::Ansi;
				}
#endif 
				return Encoding::UTF8;
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
			if (!stream->IsEnd())
				ReadBuffer();
			if (ptr<buffer.Count())
			{
				return buffer[ptr++];
			}
			return 0;
		}
		int TextReader::Read(wchar_t * destBuffer, int length)
		{
			int i = 0;
			for (i = 0; i<length; i++)
			{
				try
				{
					auto ch = Read();
					if (IsEnd())
						break;
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
			while (!IsEnd())
			{
				try
				{
					auto ch = Read();
					if (IsEnd())
						break;
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
			while (!IsEnd())
			{
				try
				{
					auto ch = Read();
					if (IsEnd())
						break;
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