#include "LibString.h"
#include "MD5.h"

namespace CoreLib
{
	namespace Basic
	{
		_EndLine EndLine;
		String StringConcat(const wchar_t * lhs, int leftLen, const wchar_t * rhs, int rightLen)
		{
			String res;
			res.length = leftLen + rightLen;
			res.buffer = new wchar_t[res.length + 1];
			wcscpy_s(res.buffer.Ptr(), res.length + 1, lhs);
			wcscpy_s(res.buffer + leftLen, res.length + 1 - leftLen, rhs);
			return res;
		}
		String operator+(const wchar_t * op1, const String & op2)
		{
			if(!op2.buffer)
				return String(op1);

			return StringConcat(op1, (int)wcslen(op1), op2.buffer.Ptr(), op2.length);
		}

		String operator+(const String & op1, const wchar_t*op2)
		{
			if(!op1.buffer)
				return String(op2);

			return StringConcat(op1.buffer.Ptr(), op1.length, op2, (int)wcslen(op2));
		}

		String operator+(const String & op1, const String & op2)
		{
			if(!op1.buffer && !op2.buffer)
				return String();
			else if(!op1.buffer)
				return String(op2);
			else if(!op2.buffer)
				return String(op1);

			return StringConcat(op1.buffer.Ptr(), op1.length, op2.buffer.Ptr(), op2.length);
		}

		int StringToInt(const String & str, int radix)
		{
			return (int)wcstol(str.Buffer(), NULL, radix);
			//return (int)_wcstoi64(str.Buffer(), NULL, 10);
		}
		unsigned int StringToUInt(const String & str, int radix)
		{
			return (unsigned int)wcstoul(str.Buffer(), NULL, radix);
		}
		double StringToDouble(const String & str)
		{
			return (double)wcstod(str.Buffer(), NULL);
		}

		String String::ReplaceAll(String src, String dst) const
		{
			String rs = *this;
			int index = 0;
			int srcLen = src.length;
			int len = rs.length;
			while ((index = rs.IndexOf(src, index)) != -1)
			{
				rs = rs.SubString(0, index) + dst + rs.SubString(index + srcLen, len - index - srcLen);
				len = rs.length;
			}
			return rs;
		}

		String String::MD5() const
		{
			unsigned char result[16];
			MD5_CTX ctx;
			MD5_Init(&ctx);
			MD5_Update(&ctx, buffer.Ptr(), length * sizeof(wchar_t));
			MD5_Final(result, &ctx);
			StringBuilder strResult;
			for (int i = 0; i < 16; i++)
			{
				auto ch = String((int)result[i], 16);
				if (ch.length == 1)
					strResult << L'0';
				else
					strResult << ch;
			}
			return strResult.ProduceString();
		}

		String String::PadLeft(wchar_t ch, int pLen)
		{
			StringBuilder sb;
			for (int i = 0; i < pLen - this->length; i++)
				sb << ch;
			for (int i = 0; i < this->length; i++)
				sb << buffer[i];
			return sb.ProduceString();
		}

		String String::PadRight(wchar_t ch, int pLen)
		{
			StringBuilder sb;
			for (int i = 0; i < this->length; i++)
				sb << buffer[i];
			for (int i = 0; i < pLen - this->length; i++)
				sb << ch;
			return sb.ProduceString();
		}
	}
}
