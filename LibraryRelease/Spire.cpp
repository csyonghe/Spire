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
#include "Spire.h"

/***********************************************************************
CORELIB\LIBIO.CPP
***********************************************************************/
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
namespace CoreLib
{
	namespace IO
	{
		using namespace CoreLib::Basic;

		bool File::Exists(const String & fileName)
		{
			struct stat sts;
			return stat(((String)fileName).ToMultiByteString(), &sts) != -1;
		}

		String Path::TruncateExt(const String & path)
		{
			int dotPos = path.LastIndexOf(L'.');
			if (dotPos != -1)
				return path.SubString(0, dotPos);
			else
				return path;
		}
		String Path::ReplaceExt(const String & path, const wchar_t * newExt)
		{
			StringBuilder sb(path.Length()+10);
			int dotPos = path.LastIndexOf(L'.');
			if (dotPos == -1)
				dotPos = path.Length();
			sb.Append(path.Buffer(), dotPos);
			sb.Append(L'.');
			sb.Append(newExt);
			return sb.ProduceString();
		}
		String Path::GetFileName(const String & path)
		{
			int pos = path.LastIndexOf(L'/');
			pos = Math::Max(path.LastIndexOf(L'\\'), pos) + 1;
			return path.SubString(pos, path.Length()-pos);
		}
		String Path::GetFileNameWithoutEXT(const String & path)
		{
			int pos = path.LastIndexOf(L'/');
			pos = Math::Max(path.LastIndexOf(L'\\'), pos) + 1;
			int dotPos = path.LastIndexOf(L'.');
			if (dotPos <= pos)
				dotPos = path.Length();
			return path.SubString(pos, dotPos - pos);
		}
		String Path::GetFileExt(const String & path)
		{
			int dotPos = path.LastIndexOf(L'.');
			if (dotPos != -1)
				return path.SubString(dotPos+1, path.Length()-dotPos-1);
			else
				return L"";
		}
		String Path::GetDirectoryName(const String & path)
		{
			int pos = path.LastIndexOf(L'/');
			pos = Math::Max(path.LastIndexOf(L'\\'), pos);
			if (pos != -1)
				return path.SubString(0, pos);
			else
				return L"";
		}
		String Path::Combine(const String & path1, const String & path2)
		{
			if (path1.Length() == 0) return path2;
			StringBuilder sb(path1.Length()+path2.Length()+2);
			sb.Append(path1);
			if (!path1.EndsWith(L'\\') && !path1.EndsWith(L'/'))
				sb.Append(PathDelimiter);
			sb.Append(path2);
			return sb.ProduceString();
		}
		String Path::Combine(const String & path1, const String & path2, const String & path3)
		{
			StringBuilder sb(path1.Length()+path2.Length()+path3.Length()+3);
			sb.Append(path1);
			if (!path1.EndsWith(L'\\') && !path1.EndsWith(L'/'))
				sb.Append(PathDelimiter);
			sb.Append(path2);
			if (!path2.EndsWith(L'\\') && !path2.EndsWith(L'/'))
				sb.Append(PathDelimiter);
			sb.Append(path3);
			return sb.ProduceString();
		}
#ifdef CreateDirectory
#undef CreateDirectory
#endif
		bool Path::CreateDirectory(const String & path)
		{
#if defined(_WIN32)
			return _mkdir(path.ToMultiByteString()) == 0;
#else 
			return mkdir(path.ToMultiByteString(), 0777) == 0;
#endif
		}

		CoreLib::Basic::String File::ReadAllText(const CoreLib::Basic::String & fileName)
		{
			StreamReader reader(new FileStream(fileName, FileMode::Open, FileAccess::Read, FileShare::ReadWrite));
			return reader.ReadToEnd();
		}

		void File::WriteAllText(const CoreLib::Basic::String & fileName, const CoreLib::Basic::String & text)
		{
			StreamWriter writer(new FileStream(fileName, FileMode::Create));
			writer.Write(text);
		}
	}
}

/***********************************************************************
CORELIB\LIBMATH.CPP
***********************************************************************/

namespace CoreLib
{
	namespace Basic
	{
		const float Math::Pi = 3.141592654f;
	}
}

/***********************************************************************
CORELIB\LIBSTRING.CPP
***********************************************************************/

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

		int StringToInt(const String & str)
		{
			return (int)wcstol(str.Buffer(), NULL, 10);
			//return (int)_wcstoi64(str.Buffer(), NULL, 10);
		}
		double StringToDouble(const String & str)
		{
			return (double)wcstod(str.Buffer(), NULL);
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

/***********************************************************************
CORELIB\MD5.CPP
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
* (This is a heavily cut-down "BSD license".)
*
* This differs from Colin Plumb's older public domain implementation in that
* no exactly 32-bit integer data type is required (any 32-bit or wider
* unsigned integer data type will do), there's no compile-time endianness
* configuration, and the function prototypes match OpenSSL's.  No code from
* Colin Plumb's implementation has been reused; this comment merely compares
* the properties of the two independent implementations.
*
* The primary goals of this implementation are portability and ease of use.
* It is meant to be fast, but not as fast as possible.  Some known
* optimizations are not included to reduce source code size and avoid
* compile-time configuration.
*/

#ifndef HAVE_OPENSSL



/*
* The basic MD5 functions.
*
* F and G are optimized compared to their RFC 1321 definitions for
* architectures that lack an AND-NOT instruction, just like in Colin Plumb's
* implementation.
*/
#define F(x, y, z)			((z) ^ ((x) & ((y) ^ (z))))
#define G(x, y, z)			((y) ^ ((z) & ((x) ^ (y))))
#define H(x, y, z)			(((x) ^ (y)) ^ (z))
#define H2(x, y, z)			((x) ^ ((y) ^ (z)))
#define I(x, y, z)			((y) ^ ((x) | ~(z)))

/*
* The MD5 transformation for all four rounds.
*/
#define STEP(f, a, b, c, d, x, t, s) \
	(a) += f((b), (c), (d)) + (x)+(t); \
	(a) = (((a) << (s)) | (((a)& 0xffffffff) >> (32 - (s)))); \
	(a) += (b);

/*
* SET reads 4 input bytes in little-endian byte order and stores them
* in a properly aligned word in host byte order.
*
* The check for little-endian architectures that tolerate unaligned
* memory accesses is just an optimization.  Nothing will break if it
* doesn't work.
*/
#if defined(__i386__) || defined(__x86_64__) || defined(__vax__)
#define SET(n) \
	(*(MD5_u32plus *)&ptr[(n)* 4])
#define GET(n) \
	SET(n)
#else
#define SET(n) \
	(ctx->block[(n)] = \
	(MD5_u32plus)ptr[(n)* 4] | \
	((MD5_u32plus)ptr[(n)* 4 + 1] << 8) | \
	((MD5_u32plus)ptr[(n)* 4 + 2] << 16) | \
	((MD5_u32plus)ptr[(n)* 4 + 3] << 24))
#define GET(n) \
	(ctx->block[(n)])
#endif

/*
* This processes one or more 64-byte data blocks, but does NOT update
* the bit counters.  There are no alignment requirements.
*/
static const void *body(MD5_CTX *ctx, const void *data, unsigned long size)
{
	const unsigned char *ptr;
	MD5_u32plus a, b, c, d;
	MD5_u32plus saved_a, saved_b, saved_c, saved_d;

	ptr = (const unsigned char *)data;

	a = ctx->a;
	b = ctx->b;
	c = ctx->c;
	d = ctx->d;

	do {
		saved_a = a;
		saved_b = b;
		saved_c = c;
		saved_d = d;

		/* Round 1 */
		STEP(F, a, b, c, d, SET(0), 0xd76aa478, 7)
			STEP(F, d, a, b, c, SET(1), 0xe8c7b756, 12)
			STEP(F, c, d, a, b, SET(2), 0x242070db, 17)
			STEP(F, b, c, d, a, SET(3), 0xc1bdceee, 22)
			STEP(F, a, b, c, d, SET(4), 0xf57c0faf, 7)
			STEP(F, d, a, b, c, SET(5), 0x4787c62a, 12)
			STEP(F, c, d, a, b, SET(6), 0xa8304613, 17)
			STEP(F, b, c, d, a, SET(7), 0xfd469501, 22)
			STEP(F, a, b, c, d, SET(8), 0x698098d8, 7)
			STEP(F, d, a, b, c, SET(9), 0x8b44f7af, 12)
			STEP(F, c, d, a, b, SET(10), 0xffff5bb1, 17)
			STEP(F, b, c, d, a, SET(11), 0x895cd7be, 22)
			STEP(F, a, b, c, d, SET(12), 0x6b901122, 7)
			STEP(F, d, a, b, c, SET(13), 0xfd987193, 12)
			STEP(F, c, d, a, b, SET(14), 0xa679438e, 17)
			STEP(F, b, c, d, a, SET(15), 0x49b40821, 22)

			/* Round 2 */
			STEP(G, a, b, c, d, GET(1), 0xf61e2562, 5)
			STEP(G, d, a, b, c, GET(6), 0xc040b340, 9)
			STEP(G, c, d, a, b, GET(11), 0x265e5a51, 14)
			STEP(G, b, c, d, a, GET(0), 0xe9b6c7aa, 20)
			STEP(G, a, b, c, d, GET(5), 0xd62f105d, 5)
			STEP(G, d, a, b, c, GET(10), 0x02441453, 9)
			STEP(G, c, d, a, b, GET(15), 0xd8a1e681, 14)
			STEP(G, b, c, d, a, GET(4), 0xe7d3fbc8, 20)
			STEP(G, a, b, c, d, GET(9), 0x21e1cde6, 5)
			STEP(G, d, a, b, c, GET(14), 0xc33707d6, 9)
			STEP(G, c, d, a, b, GET(3), 0xf4d50d87, 14)
			STEP(G, b, c, d, a, GET(8), 0x455a14ed, 20)
			STEP(G, a, b, c, d, GET(13), 0xa9e3e905, 5)
			STEP(G, d, a, b, c, GET(2), 0xfcefa3f8, 9)
			STEP(G, c, d, a, b, GET(7), 0x676f02d9, 14)
			STEP(G, b, c, d, a, GET(12), 0x8d2a4c8a, 20)

			/* Round 3 */
			STEP(H, a, b, c, d, GET(5), 0xfffa3942, 4)
			STEP(H2, d, a, b, c, GET(8), 0x8771f681, 11)
			STEP(H, c, d, a, b, GET(11), 0x6d9d6122, 16)
			STEP(H2, b, c, d, a, GET(14), 0xfde5380c, 23)
			STEP(H, a, b, c, d, GET(1), 0xa4beea44, 4)
			STEP(H2, d, a, b, c, GET(4), 0x4bdecfa9, 11)
			STEP(H, c, d, a, b, GET(7), 0xf6bb4b60, 16)
			STEP(H2, b, c, d, a, GET(10), 0xbebfbc70, 23)
			STEP(H, a, b, c, d, GET(13), 0x289b7ec6, 4)
			STEP(H2, d, a, b, c, GET(0), 0xeaa127fa, 11)
			STEP(H, c, d, a, b, GET(3), 0xd4ef3085, 16)
			STEP(H2, b, c, d, a, GET(6), 0x04881d05, 23)
			STEP(H, a, b, c, d, GET(9), 0xd9d4d039, 4)
			STEP(H2, d, a, b, c, GET(12), 0xe6db99e5, 11)
			STEP(H, c, d, a, b, GET(15), 0x1fa27cf8, 16)
			STEP(H2, b, c, d, a, GET(2), 0xc4ac5665, 23)

			/* Round 4 */
			STEP(I, a, b, c, d, GET(0), 0xf4292244, 6)
			STEP(I, d, a, b, c, GET(7), 0x432aff97, 10)
			STEP(I, c, d, a, b, GET(14), 0xab9423a7, 15)
			STEP(I, b, c, d, a, GET(5), 0xfc93a039, 21)
			STEP(I, a, b, c, d, GET(12), 0x655b59c3, 6)
			STEP(I, d, a, b, c, GET(3), 0x8f0ccc92, 10)
			STEP(I, c, d, a, b, GET(10), 0xffeff47d, 15)
			STEP(I, b, c, d, a, GET(1), 0x85845dd1, 21)
			STEP(I, a, b, c, d, GET(8), 0x6fa87e4f, 6)
			STEP(I, d, a, b, c, GET(15), 0xfe2ce6e0, 10)
			STEP(I, c, d, a, b, GET(6), 0xa3014314, 15)
			STEP(I, b, c, d, a, GET(13), 0x4e0811a1, 21)
			STEP(I, a, b, c, d, GET(4), 0xf7537e82, 6)
			STEP(I, d, a, b, c, GET(11), 0xbd3af235, 10)
			STEP(I, c, d, a, b, GET(2), 0x2ad7d2bb, 15)
			STEP(I, b, c, d, a, GET(9), 0xeb86d391, 21)

			a += saved_a;
		b += saved_b;
		c += saved_c;
		d += saved_d;

		ptr += 64;
	} while (size -= 64);

	ctx->a = a;
	ctx->b = b;
	ctx->c = c;
	ctx->d = d;

	return ptr;
}

void MD5_Init(MD5_CTX *ctx)
{
	ctx->a = 0x67452301;
	ctx->b = 0xefcdab89;
	ctx->c = 0x98badcfe;
	ctx->d = 0x10325476;

	ctx->lo = 0;
	ctx->hi = 0;
}

void MD5_Update(MD5_CTX *ctx, const void *data, unsigned long size)
{
	MD5_u32plus saved_lo;
	unsigned long used, available;

	saved_lo = ctx->lo;
	if ((ctx->lo = (saved_lo + size) & 0x1fffffff) < saved_lo)
		ctx->hi++;
	ctx->hi += size >> 29;

	used = saved_lo & 0x3f;

	if (used) {
		available = 64 - used;

		if (size < available) {
			memcpy(&ctx->buffer[used], data, size);
			return;
		}

		memcpy(&ctx->buffer[used], data, available);
		data = (const unsigned char *)data + available;
		size -= available;
		body(ctx, ctx->buffer, 64);
	}

	if (size >= 64) {
		data = body(ctx, data, size & ~(unsigned long)0x3f);
		size &= 0x3f;
	}

	memcpy(ctx->buffer, data, size);
}

void MD5_Final(unsigned char *result, MD5_CTX *ctx)
{
	unsigned long used, available;

	used = ctx->lo & 0x3f;

	ctx->buffer[used++] = 0x80;

	available = 64 - used;

	if (available < 8) {
		memset(&ctx->buffer[used], 0, available);
		body(ctx, ctx->buffer, 64);
		used = 0;
		available = 64;
	}

	memset(&ctx->buffer[used], 0, available - 8);

	ctx->lo <<= 3;
	ctx->buffer[56] = (unsigned char)(ctx->lo);
	ctx->buffer[57] = (unsigned char)(ctx->lo >> 8);
	ctx->buffer[58] = (unsigned char)(ctx->lo >> 16);
	ctx->buffer[59] = (unsigned char)(ctx->lo >> 24);
	ctx->buffer[60] = (unsigned char)(ctx->hi);
	ctx->buffer[61] = (unsigned char)(ctx->hi >> 8);
	ctx->buffer[62] = (unsigned char)(ctx->hi >> 16);
	ctx->buffer[63] = (unsigned char)(ctx->hi >> 24);

	body(ctx, ctx->buffer, 64);

	result[0] = (unsigned char)(ctx->a);
	result[1] = (unsigned char)(ctx->a >> 8);
	result[2] = (unsigned char)(ctx->a >> 16);
	result[3] = (unsigned char)(ctx->a >> 24);
	result[4] = (unsigned char)(ctx->b);
	result[5] = (unsigned char)(ctx->b >> 8);
	result[6] = (unsigned char)(ctx->b >> 16);
	result[7] = (unsigned char)(ctx->b >> 24);
	result[8] = (unsigned char)(ctx->c);
	result[9] = (unsigned char)(ctx->c >> 8);
	result[10] = (unsigned char)(ctx->c >> 16);
	result[11] = (unsigned char)(ctx->c >> 24);
	result[12] = (unsigned char)(ctx->d);
	result[13] = (unsigned char)(ctx->d >> 8);
	result[14] = (unsigned char)(ctx->d >> 16);
	result[15] = (unsigned char)(ctx->d >> 24);

	memset(ctx, 0, sizeof(*ctx));
}

#endif

/***********************************************************************
CORELIB\PARSER.CPP
***********************************************************************/

using namespace CoreLib::Basic;

namespace CoreLib
{
	namespace Text
	{
		RefPtr<MetaLexer> Parser::metaLexer;
		MetaLexer * Parser::GetTextLexer()
		{
			if (!metaLexer)
			{
				metaLexer = new MetaLexer();
				metaLexer->SetLexProfile(
					L"#WhiteSpace = {\\s+}\n"\
					L"#SingleLineComment = {//[^\\n]*\\n}\n"\
					L"#MultiLineComment = {/\\*([^*]|\\*[^/])*\\*/}\n"\
					L"Identifier = {[a-zA-Z_]\\w*}\n"\
					L"IntConstant = {\\d+}\n"\
					L"FloatConstant = {\\d*.\\d+|\\d+(.\\d+)?(e(-)?\\d+)?}\n"\
					L"StringConstant = {\"([^\\\\\"]|\\\\\\.)*\"}\n"\
					L"CharConstant = {'[^\\n\\r]*'}\n"\
					L"LParent = {\\(}\n"\
					L"RParent = {\\)}\n"\
					L"LBrace = {{}\n"\
					L"RBrace = {}}\n"\
					L"LBracket = {\\[}\n"\
					L"RBracket = {\\]}\n"\
					L"Dot = {.}\n"\
					L"Semicolon = {;}\n"\
					L"Comma = {,}\n"\
					L"Colon = {:}\n"\
					L"OpAdd = {\\+}\n"\
					L"OpSub = {-}\n"\
					L"OpDiv = {/}\n"\
					L"OpMul = {\\*}\n"\
					L"OpMod = {%}\n"\
					L"OpExp = {^}\n"\
					L"OpGreater = {>}\n"\
					L"OpLess = {<}\n"\
					L"OpEqual = {==}\n"\
					L"OpGEqual = {>=}\n"\
					L"OpLEqual = {<=}\n"\
					L"OpNEqual = {!=}\n"\
					L"OpAnd = {&}\n"\
					L"OpOr = {\\|}\n"\
					L"OpNot = {!}\n"\
					L"OpAssign = {=}\n"\
					L"OpDollar = {$}\n"
					);
			}
			return metaLexer.Ptr();
		}
		void Parser::DisposeTextLexer()
		{
			metaLexer = nullptr;
		}
		Basic::List<Basic::String> Parser::SplitString(Basic::String str, wchar_t ch)
		{
			List<String> result;
			StringBuilder currentBuilder;
			for (int i = 0; i < str.Length(); i++)
			{
				if (str[i] == ch)
				{
					result.Add(currentBuilder.ToString());
					currentBuilder.Clear();
				}
				else
					currentBuilder.Append(str[i]);
			}
			result.Add(currentBuilder.ToString());
			return result;
		}
		Parser::Parser(String text)
		{
			this->text = text;
			
			stream = GetTextLexer()->Parse(text);
			for (auto token : stream)
			{
				if (token.TypeID != -1)
					tokens.Add(token);
			}
			tokenPtr = 0;
		}


		List<String> Split(String text, wchar_t c)
		{
			List<String> result;
			StringBuilder sb;
			for (int i = 0; i < text.Length(); i++)
			{
				if (text[i] == c)
				{
					auto str = sb.ToString();
					if (str.Length() != 0)
						result.Add(str);
					sb.Clear();
				}
				else
					sb << text[i];
			}
			auto lastStr = sb.ToString();
			if (lastStr.Length())
				result.Add(lastStr);
			return result;
		}

	}
}

/***********************************************************************
CORELIB\PERFORMANCECOUNTER.CPP
***********************************************************************/

using namespace std::chrono;

namespace CoreLib
{
	namespace Diagnostics
	{
		TimePoint PerformanceCounter::Start()
		{
			return high_resolution_clock::now();
		}

		Duration PerformanceCounter::End(TimePoint counter)
		{
			return high_resolution_clock::now()-counter;
		}

		float PerformanceCounter::EndSeconds(TimePoint counter)
		{
			return (float)ToSeconds(high_resolution_clock::now() - counter);
		}

		double PerformanceCounter::ToSeconds(Duration counter)
		{
			auto rs = duration_cast<duration<double>>(counter);
			return *(double*)&rs;
		}
	}
}

/***********************************************************************
CORELIB\STREAM.CPP
***********************************************************************/
#ifdef _WIN32
#include <share.h>
#endif

namespace CoreLib
{
	namespace IO
	{
		using namespace CoreLib::Basic;
		FileStream::FileStream(const CoreLib::Basic::String & fileName, FileMode fileMode)
		{
			Init(fileName, fileMode, fileMode==FileMode::Open?FileAccess::Read:FileAccess::Write, FileShare::None);
		}
		FileStream::FileStream(const CoreLib::Basic::String & fileName, FileMode fileMode, FileAccess access, FileShare share)
		{
			Init(fileName, fileMode, access, share);
		}
		void FileStream::Init(const CoreLib::Basic::String & fileName, FileMode fileMode, FileAccess access, FileShare share)
		{
			const wchar_t * mode = L"rt";
			const char* modeMBCS = "rt";
			switch (fileMode)
			{
			case CoreLib::IO::FileMode::Create:
				if (access == FileAccess::Read)
					throw ArgumentException(L"Read-only access is incompatible with Create mode.");
				else if (access == FileAccess::ReadWrite)
				{
					mode = L"w+b";
					modeMBCS = "w+b";
					this->fileAccess = FileAccess::ReadWrite;
				}
				else
				{
					mode = L"wb";
					modeMBCS = "wb";
					this->fileAccess = FileAccess::Write;
				}
				break;
			case CoreLib::IO::FileMode::Open:
				if (access == FileAccess::Read)
				{
					mode = L"rb";
					modeMBCS = "rb";
					this->fileAccess = FileAccess::Read;
				}
				else if (access == FileAccess::ReadWrite)
				{
					mode = L"r+b";
					modeMBCS = "r+b";
					this->fileAccess = FileAccess::ReadWrite;
				}
				else
				{
					mode = L"wb";
					modeMBCS = "wb";
					this->fileAccess = FileAccess::Write;
				}
				break;
			case CoreLib::IO::FileMode::CreateNew:
				if (File::Exists(fileName))
				{
					throw IOException(L"Failed opening '" + fileName + L"', file already exists.");
				}
				if (access == FileAccess::Read)
					throw ArgumentException(L"Read-only access is incompatible with Create mode.");
				else if (access == FileAccess::ReadWrite)
				{
					mode = L"w+b";
					this->fileAccess = FileAccess::ReadWrite;
				}
				else
				{
					mode = L"wb";
					this->fileAccess = FileAccess::Write;
				}
				break;
			case CoreLib::IO::FileMode::Append:
				if (access == FileAccess::Read)
					throw ArgumentException(L"Read-only access is incompatible with Append mode.");
				else if (access == FileAccess::ReadWrite)
				{
					mode = L"a+b";
					this->fileAccess = FileAccess::ReadWrite;
				}
				else
				{
					mode = L"ab";
					this->fileAccess = FileAccess::Write;
				}
				break;
			default:
				break;
			}
			int shFlag;
#ifdef WIN32
			switch (share)
			{
			case CoreLib::IO::FileShare::None:
				shFlag = _SH_DENYRW;
				break;
			case CoreLib::IO::FileShare::ReadOnly:
				shFlag = _SH_DENYWR;
				break;
			case CoreLib::IO::FileShare::WriteOnly:
				shFlag = _SH_DENYRD;
				break;
			case CoreLib::IO::FileShare::ReadWrite:
				shFlag = _SH_DENYNO;
				break;
			default:
				throw ArgumentException(L"Invalid file share mode.");
				break;
			}
			handle = _wfsopen(fileName.Buffer(), mode, shFlag);
#else
			handle = fopen(fileName.ToMultiByteString(), modeMBCS);
#endif
			if (!handle)
			{
				throw IOException(L"Cannot open file '" + fileName + L"'");
			}
		}
		FileStream::~FileStream()
		{
			Close();
		}
		Int64 FileStream::GetPosition()
		{
#ifdef WIN32
			fpos_t pos;
			fgetpos(handle, &pos);
			return pos;
#else
			fpos64_t pos;
			fgetpos64(handle, &pos);
			return *(Int64*)(&pos);
#endif
		}
		void FileStream::Seek(SeekOrigin origin, Int64 offset)
		{
			int _origin;
			switch (origin)
			{
			case CoreLib::IO::SeekOrigin::Start:
				_origin = SEEK_SET;
				endReached = false;
				break;
			case CoreLib::IO::SeekOrigin::End:
				_origin = SEEK_END;
				endReached = true;
				break;
			case CoreLib::IO::SeekOrigin::Current:
				_origin = SEEK_CUR;
				endReached = false;
				break;
			default:
				throw NotSupportedException(L"Unsupported seek origin.");
				break;
			}
#ifdef WIN32
			int rs = _fseeki64(handle, offset, _origin);
#else
			int rs = fseek(handle, (int)offset, _origin);
#endif
			if (rs != 0)
			{
				throw IOException(L"FileStream seek failed.");
			}
		}
		Int64 FileStream::Read(void * buffer, Int64 length)
		{
			auto bytes = fread_s(buffer, (size_t)length, 1, (size_t)length, handle);
			if (bytes == 0 && length > 0)
			{
				if (!feof(handle))
					throw IOException(L"FileStream read failed.");
				else if (endReached)
					throw EndOfStreamException(L"End of file is reached.");
				endReached = true;
			}
			return (int)bytes;
		}
		Int64 FileStream::Write(const void * buffer, Int64 length)
		{
			auto bytes = (Int64)fwrite(buffer, 1, (size_t)length, handle);
			if (bytes < length)
			{
				throw IOException(L"FileStream write failed.");
			}
			return bytes;
		}
		bool FileStream::CanRead()
		{
			return ((int)fileAccess & (int)FileAccess::Read) != 0;
		}
		bool FileStream::CanWrite()
		{
			return ((int)fileAccess & (int)FileAccess::Write) != 0;
		}
		void FileStream::Close()
		{
			if (handle)
			{
				fclose(handle);
				handle = 0;
			}
		}
		bool FileStream::IsEnd()
		{
			return endReached;
		}
	}
}

/***********************************************************************
CORELIB\TEXTIO.CPP
***********************************************************************/
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
						unsigned char byte = (unsigned char)(0b11000000 + (codePoint >> 6));
						result.Add((char)byte);
						byte = 0b10000000 + (codePoint & 0b111111);
						result.Add((char)byte);
					}
					else if (codePoint <= 0xFFFF)
					{
						unsigned char byte = (unsigned char)(0b11100000 + (codePoint >> 12));
						result.Add((char)byte);
						byte = (unsigned char)(0b10000000 + ((codePoint >> 6) & (0b111111)));
						result.Add((char)byte);
						byte = (unsigned char)(0b10000000 + (codePoint & 0b111111));
						result.Add((char)byte);
					}
					else
					{
						unsigned char byte = (unsigned char)(0b11110000 + (codePoint >> 18));
						result.Add((char)byte);
						byte = (unsigned char)(0b10000000 + ((codePoint >> 12) & 0b111111));
						result.Add((char)byte);
						byte = (unsigned char)(0b10000000 + ((codePoint >> 6) & 0b111111));
						result.Add((char)byte);
						byte = (unsigned char)(0b10000000 + (codePoint & 0b111111));
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
						unsigned char lastByte = ch & 0b1111'1111;
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


		const unsigned char Utf8Header[] = { 0xEF,0xBB,0xBF };

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
			encoding->GetBytes(encodingBuffer, str);
			stream->Write(encodingBuffer.Buffer(), encodingBuffer.Count());
		}
		void StreamWriter::Write(const wchar_t * str)
		{
			encodingBuffer.Clear();
			encoding->GetBytes(encodingBuffer, String(str));
			stream->Write(encodingBuffer.Buffer(), encodingBuffer.Count());
		}
		void StreamWriter::Write(const char * str)
		{
			encodingBuffer.Clear();
			encoding->GetBytes(encodingBuffer, String(str));
			stream->Write(encodingBuffer.Buffer(), encodingBuffer.Count());
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

/***********************************************************************
CORELIB\THREADING.CPP
***********************************************************************/

#ifdef _WIN32
#elif MACOS
#include <sys/param.h>
#include <sys/sysctl.h>
#else
#include <unistd.h>
#endif

namespace CoreLib
{
	namespace Threading
	{
		unsigned int __stdcall ThreadProcedure(const ThreadParam& param)
		{
			if (param.thread->paramedThreadProc)
				param.thread->paramedThreadProc->Invoke(param.threadParam);
			else
				param.thread->threadProc->Invoke();
			return 0;
		}

		int ParallelSystemInfo::GetProcessorCount()
		{
		#ifdef _WIN32
			SYSTEM_INFO sysinfo;
			GetSystemInfo(&sysinfo);
			return sysinfo.dwNumberOfProcessors;
		#elif MACOS
			int nm[2];
			size_t len = 4;
			uint32_t count;

			nm[0] = CTL_HW; nm[1] = HW_AVAILCPU;
			sysctl(nm, 2, &count, &len, NULL, 0);

			if(count < 1) {
				nm[1] = HW_NCPU;
				sysctl(nm, 2, &count, &len, NULL, 0);
				if(count < 1) { count = 1; }
			}
			return count;
		#else
			return sysconf(_SC_NPROCESSORS_ONLN);
		#endif
		}
	}
}

/***********************************************************************
CORELIB\VECTORMATH.CPP
***********************************************************************/

namespace VectorMath
{
	const __m128 Matrix4_M128::VecOne = _mm_set_ps1(1.0f);
	void Matrix4::Rotation(Matrix4 & rs, const Vec3 & axis, float angle)
	{
		float c = cosf(angle);
		float s = sinf(angle);
		float t = 1.0f - c;

		Vec3 nAxis;
		Vec3::Normalize(nAxis, axis);
		float x = nAxis.x;
		float y = nAxis.y;
		float z = nAxis.z;

		rs.m[0][0] = 1 + t*(x*x-1);
		rs.m[1][0] = z*s+t*x*y;
		rs.m[2][0] = -y*s+t*x*z;
		rs.m[3][0] = 0.0f;

		rs.m[0][1] = -z*s+t*x*y;
		rs.m[1][1] = 1+t*(y*y-1);
		rs.m[2][1] = x*s+t*y*z;
		rs.m[3][1] = 0.0f;

		rs.m[0][2] = y*s+t*x*z;
		rs.m[1][2] = -x*s+t*y*z;
		rs.m[2][2] = 1+t*(z*z-1);
		rs.m[3][2] = 0.0f;

		rs.m[0][3] = 0.0f;
		rs.m[1][3] = 0.0f;
		rs.m[2][3] = 0.0f;
		rs.m[3][3] = 1.0f;
	}
	void Matrix4::Rotation(Matrix4 & rs, float yaw, float pitch, float roll)
	{
		Matrix4 mat;
		Matrix4::RotationY(rs, yaw);
		Matrix4::RotationX(mat, pitch);
		Matrix4::Multiply(rs, rs, mat);
		Matrix4::RotationZ(mat, roll);
		Matrix4::Multiply(rs, rs, mat);
	}

	void Matrix4::GetNormalMatrix(Matrix4 & mOut) const
	{
		float fDet = (mi._11 * (mi._22 * mi._33 - mi._23 * mi._32) -
			mi._12 * (mi._21 * mi._33 - mi._23 * mi._31) +
			mi._13 * (mi._21 * mi._32 - mi._22 * mi._31));
		float fDetInv = 1.0f / fDet;

		mOut.mi._11 = fDetInv * (mi._22 * mi._33 - mi._23 * mi._32);
		mOut.mi._21 = -fDetInv * (mi._12 * mi._33 - mi._13 * mi._32);
		mOut.mi._31 = fDetInv * (mi._12 * mi._23 - mi._13 * mi._22);

		mOut.mi._12 = -fDetInv * (mi._21 * mi._33 - mi._23 * mi._31);
		mOut.mi._22 = fDetInv * (mi._11 * mi._33 - mi._13 * mi._31);
		mOut.mi._32 = -fDetInv * (mi._11 * mi._23 - mi._13 * mi._21);

		mOut.mi._13 = fDetInv * (mi._21 * mi._32 - mi._22 * mi._31);
		mOut.mi._23 = -fDetInv * (mi._11 * mi._32 - mi._12 * mi._31);
		mOut.mi._33 = fDetInv * (mi._11 * mi._22 - mi._12 * mi._21);

		mOut.mi._14 = 0.0f;
		mOut.mi._24 = 0.0f;
		mOut.mi._34 = 0.0f;
		mOut.mi._41 = 0.0f;
		mOut.mi._42 = 0.0f;
		mOut.mi._43 = 0.0f;
		mOut.mi._44 = 1.0f;
	}
	
	float Matrix4::Inverse3D(Matrix4 & mOut_d) const
	{
		if(fabs(mi._44 - 1.0f) > 0.001f)
			return 0.0f;
		if(fabs(mi._14)>0.001f || fabs(mi._24)>0.001f || fabs(mi._34)>0.001f)
			return 0.0f;

		float fDet = (mi._11 * (mi._22 * mi._33 - mi._23 * mi._32) - 
		mi._12 * (mi._21 * mi._33 - mi._23 * mi._31) +
		mi._13 * (mi._21 * mi._32 - mi._22 * mi._31));
		float fDetInv = 1.0f / fDet;

		mOut_d.mi._11 = fDetInv * (mi._22 * mi._33 - mi._23 * mi._32);
		mOut_d.mi._12 = -fDetInv * (mi._12 * mi._33 - mi._13 * mi._32);
		mOut_d.mi._13 = fDetInv * (mi._12 * mi._23 - mi._13 * mi._22);
		mOut_d.mi._14 = 0.0f;

		mOut_d.mi._21 = -fDetInv * (mi._21 * mi._33 - mi._23 * mi._31);
		mOut_d.mi._22 = fDetInv * (mi._11 * mi._33 - mi._13 * mi._31);
		mOut_d.mi._23 = -fDetInv * (mi._11 * mi._23 - mi._13 * mi._21);
		mOut_d.mi._24 = 0.0f;

		mOut_d.mi._31 = fDetInv * (mi._21 * mi._32 - mi._22 * mi._31);
		mOut_d.mi._32 = -fDetInv * (mi._11 * mi._32 - mi._12 * mi._31);
		mOut_d.mi._33 = fDetInv * (mi._11 * mi._22 - mi._12 * mi._21);
		mOut_d.mi._34 = 0.0f;

		mOut_d.mi._41 = -(mi._41 * mOut_d.mi._11 + mi._42 * mOut_d.mi._21 + mi._43 * mOut_d.mi._31);
		mOut_d.mi._42 = -(mi._41 * mOut_d.mi._12 + mi._42 * mOut_d.mi._22 + mi._43 * mOut_d.mi._32);
		mOut_d.mi._43 = -(mi._41 * mOut_d.mi._13 + mi._42 * mOut_d.mi._23 + mi._43 * mOut_d.mi._33);
		mOut_d.mi._44 = 1.0f;

		return fDet;
	}
		
	float Matrix4::InverseFPU(Matrix4 &mOut_d) const
	{
		float succ = Inverse3D(mOut_d);
		if (succ != 0.0f)
			return succ;
		double Result[4][4];
		double tmp[12];
		double src[16];
		double det;
		for (int i = 0; i < 4; i++)
		{
			src[i+0] = m[i][0];
			src[i+4] = m[i][1];
			src[i+8] = m[i][2];
			src[i+12] = m[i][3];
		}
		tmp[0] = src[10] * src[15];
		tmp[1] = src[11] * src[14];
		tmp[2] = src[9] * src[15];
		tmp[3] = src[11] * src[13];
		tmp[4] = src[9] * src[14];
		tmp[5] = src[10] * src[13];
		tmp[6] = src[8] * src[15];
		tmp[7] = src[11] * src[12];
		tmp[8] = src[8] * src[14];
		tmp[9] = src[10] * src[12];
		tmp[10] = src[8] * src[13];
		tmp[11] = src[9] * src[12];
		Result[0][0] = tmp[0]*src[5] + tmp[3]*src[6] + tmp[4]*src[7];
		Result[0][0] -= tmp[1]*src[5] + tmp[2]*src[6] + tmp[5]*src[7];
		Result[0][1] = tmp[1]*src[4] + tmp[6]*src[6] + tmp[9]*src[7];
		Result[0][1] -= tmp[0]*src[4] + tmp[7]*src[6] + tmp[8]*src[7];
		Result[0][2] = tmp[2]*src[4] + tmp[7]*src[5] + tmp[10]*src[7];
		Result[0][2] -= tmp[3]*src[4] + tmp[6]*src[5] + tmp[11]*src[7];
		Result[0][3] = tmp[5]*src[4] + tmp[8]*src[5] + tmp[11]*src[6];
		Result[0][3] -= tmp[4]*src[4] + tmp[9]*src[5] + tmp[10]*src[6];
		Result[1][0] = tmp[1]*src[1] + tmp[2]*src[2] + tmp[5]*src[3];
		Result[1][0] -= tmp[0]*src[1] + tmp[3]*src[2] + tmp[4]*src[3];
		Result[1][1] = tmp[0]*src[0] + tmp[7]*src[2] + tmp[8]*src[3];
		Result[1][1] -= tmp[1]*src[0] + tmp[6]*src[2] + tmp[9]*src[3];
		Result[1][2] = tmp[3]*src[0] + tmp[6]*src[1] + tmp[11]*src[3];
		Result[1][2] -= tmp[2]*src[0] + tmp[7]*src[1] + tmp[10]*src[3];
		Result[1][3] = tmp[4]*src[0] + tmp[9]*src[1] + tmp[10]*src[2];
		Result[1][3] -= tmp[5]*src[0] + tmp[8]*src[1] + tmp[11]*src[2];
		tmp[0] = src[2]*src[7];
		tmp[1] = src[3]*src[6];
		tmp[2] = src[1]*src[7];
		tmp[3] = src[3]*src[5];
		tmp[4] = src[1]*src[6];
		tmp[5] = src[2]*src[5];
		tmp[6] = src[0]*src[7];
		tmp[7] = src[3]*src[4];
		tmp[8] = src[0]*src[6];
		tmp[9] = src[2]*src[4];
		tmp[10] = src[0]*src[5];
		tmp[11] = src[1]*src[4];
		Result[2][0] = tmp[0]*src[13] + tmp[3]*src[14] + tmp[4]*src[15];
		Result[2][0] -= tmp[1]*src[13] + tmp[2]*src[14] + tmp[5]*src[15];
		Result[2][1] = tmp[1]*src[12] + tmp[6]*src[14] + tmp[9]*src[15];
		Result[2][1] -= tmp[0]*src[12] + tmp[7]*src[14] + tmp[8]*src[15];
		Result[2][2] = tmp[2]*src[12] + tmp[7]*src[13] + tmp[10]*src[15];
		Result[2][2] -= tmp[3]*src[12] + tmp[6]*src[13] + tmp[11]*src[15];
		Result[2][3] = tmp[5]*src[12] + tmp[8]*src[13] + tmp[11]*src[14];
		Result[2][3] -= tmp[4]*src[12] + tmp[9]*src[13] + tmp[10]*src[14];
		Result[3][0] = tmp[2]*src[10] + tmp[5]*src[11] + tmp[1]*src[9];
		Result[3][0] -= tmp[4]*src[11] + tmp[0]*src[9] + tmp[3]*src[10];
		Result[3][1] = tmp[8]*src[11] + tmp[0]*src[8] + tmp[7]*src[10];
		Result[3][1] -= tmp[6]*src[10] + tmp[9]*src[11] + tmp[1]*src[8];
		Result[3][2] = tmp[6]*src[9] + tmp[11]*src[11] + tmp[3]*src[8];
		Result[3][2] -= tmp[10]*src[11] + tmp[2]*src[8] + tmp[7]*src[9];
		Result[3][3] = tmp[10]*src[10] + tmp[4]*src[8] + tmp[9]*src[9];
		Result[3][3] -= tmp[8]*src[9] + tmp[11]*src[10] + tmp[5]*src[8];
		det=src[0]*Result[0][0]+src[1]*Result[0][1]+src[2]*Result[0][2]+src[3]*Result[0][3];
		det = 1.0f / det;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				mOut_d.m[i][j] = (float)(Result[i][j] * det);
			}
		}
		return (float)det;
	}
	
	void Matrix4::LookAt(Matrix4 & rs, const Vec3 & pos, const Vec3 & center, const Vec3 & up)
	{
		Vec3 xAxis, yAxis, zAxis;
		Vec3::Subtract(zAxis, pos, center);
		Vec3::Normalize(zAxis, zAxis);
		Vec3::Cross(xAxis, up, zAxis);
		Vec3::Normalize(xAxis, xAxis);
		Vec3::Cross(yAxis, zAxis, xAxis);
		Vec3::Normalize(yAxis, yAxis);

		rs.m[0][0] = xAxis.x;
		rs.m[0][1] = yAxis.x;
		rs.m[0][2] = zAxis.x;
		rs.m[0][3] = 0.0f;

		rs.m[1][0] = xAxis.y;
		rs.m[1][1] = yAxis.y;
		rs.m[1][2] = zAxis.y;
		rs.m[1][3] = 0.0f;

		rs.m[2][0] = xAxis.z;
		rs.m[2][1] = yAxis.z;
		rs.m[2][2] = zAxis.z;
		rs.m[2][3] = 0.0f;

		rs.m[3][0] = -Vec3::Dot(xAxis, pos);
		rs.m[3][1] = -Vec3::Dot(yAxis, pos);
		rs.m[3][2] = -Vec3::Dot(zAxis, pos);
		rs.m[3][3] = 1.0f;
	}
	
	float Matrix4_M128::Inverse(Matrix4_M128 &mOut) const
	{
		__m128 Fac0;
		{
			__m128 Swp0a = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(3, 3, 3, 3));
			__m128 Swp0b = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(2, 2, 2, 2));

			__m128 Swp00 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(3, 3, 3, 3));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac0 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac1;
		{
			__m128 Swp0a = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(3, 3, 3, 3));
			__m128 Swp0b = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(1, 1, 1, 1));

			__m128 Swp00 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(3, 3, 3, 3));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac1 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac2;
		{
			__m128 Swp0a = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 Swp0b = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(1, 1, 1, 1));

			__m128 Swp00 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(2, 2, 2, 2));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac2 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac3;
		{
			__m128 Swp0a = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(3, 3, 3, 3));
			__m128 Swp0b = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 Swp00 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(3, 3, 3, 3));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac3 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac4;
		{
			__m128 Swp0a = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(2, 2, 2, 2));
			__m128 Swp0b = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 Swp00 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(2, 2, 2, 2));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac4 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 Fac5;
		{
			__m128 Swp0a = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(1, 1, 1, 1));
			__m128 Swp0b = _mm_shuffle_ps(C4, C3, _MM_SHUFFLE(0, 0, 0, 0));

			__m128 Swp00 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(0, 0, 0, 0));
			__m128 Swp01 = _mm_shuffle_ps(Swp0a, Swp0a, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp02 = _mm_shuffle_ps(Swp0b, Swp0b, _MM_SHUFFLE(2, 0, 0, 0));
			__m128 Swp03 = _mm_shuffle_ps(C3, C2, _MM_SHUFFLE(1, 1, 1, 1));

			__m128 Mul00 = _mm_mul_ps(Swp00, Swp01);
			__m128 Mul01 = _mm_mul_ps(Swp02, Swp03);
			Fac5 = _mm_sub_ps(Mul00, Mul01);
		}

		__m128 SignA = _mm_set_ps( 1.0f,-1.0f, 1.0f,-1.0f);
		__m128 SignB = _mm_set_ps(-1.0f, 1.0f,-1.0f, 1.0f);

		__m128 Temp0 = _mm_shuffle_ps(C2, C1, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 Vec0 = _mm_shuffle_ps(Temp0, Temp0, _MM_SHUFFLE(2, 2, 2, 0));

		__m128 Temp1 = _mm_shuffle_ps(C2, C1, _MM_SHUFFLE(1, 1, 1, 1));
		__m128 Vec1 = _mm_shuffle_ps(Temp1, Temp1, _MM_SHUFFLE(2, 2, 2, 0));

		__m128 Temp2 = _mm_shuffle_ps(C2, C1, _MM_SHUFFLE(2, 2, 2, 2));
		__m128 Vec2 = _mm_shuffle_ps(Temp2, Temp2, _MM_SHUFFLE(2, 2, 2, 0));

		__m128 Temp3 = _mm_shuffle_ps(C2, C1, _MM_SHUFFLE(3, 3, 3, 3));
		__m128 Vec3 = _mm_shuffle_ps(Temp3, Temp3, _MM_SHUFFLE(2, 2, 2, 0));

		__m128 Mul00 = _mm_mul_ps(Vec1, Fac0);
		__m128 Mul01 = _mm_mul_ps(Vec2, Fac1);
		__m128 Mul02 = _mm_mul_ps(Vec3, Fac2);
		__m128 Sub00 = _mm_sub_ps(Mul00, Mul01);
		__m128 Add00 = _mm_add_ps(Sub00, Mul02);
		__m128 Inv0 = _mm_mul_ps(SignB, Add00);

		__m128 Mul03 = _mm_mul_ps(Vec0, Fac0);
		__m128 Mul04 = _mm_mul_ps(Vec2, Fac3);
		__m128 Mul05 = _mm_mul_ps(Vec3, Fac4);
		__m128 Sub01 = _mm_sub_ps(Mul03, Mul04);
		__m128 Add01 = _mm_add_ps(Sub01, Mul05);
		__m128 Inv1 = _mm_mul_ps(SignA, Add01);

		__m128 Mul06 = _mm_mul_ps(Vec0, Fac1);
		__m128 Mul07 = _mm_mul_ps(Vec1, Fac3);
		__m128 Mul08 = _mm_mul_ps(Vec3, Fac5);
		__m128 Sub02 = _mm_sub_ps(Mul06, Mul07);
		__m128 Add02 = _mm_add_ps(Sub02, Mul08);
		__m128 Inv2 = _mm_mul_ps(SignB, Add02);

		__m128 Mul09 = _mm_mul_ps(Vec0, Fac2);
		__m128 Mul10 = _mm_mul_ps(Vec1, Fac4);
		__m128 Mul11 = _mm_mul_ps(Vec2, Fac5);
		__m128 Sub03 = _mm_sub_ps(Mul09, Mul10);
		__m128 Add03 = _mm_add_ps(Sub03, Mul11);
		__m128 Inv3 = _mm_mul_ps(SignA, Add03);

		__m128 Row0 = _mm_shuffle_ps(Inv0, Inv1, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 Row1 = _mm_shuffle_ps(Inv2, Inv3, _MM_SHUFFLE(0, 0, 0, 0));
		__m128 Row2 = _mm_shuffle_ps(Row0, Row1, _MM_SHUFFLE(2, 0, 2, 0));

		// Det0 = dot(C1, Row2)
		__m128 mul0 = _mm_mul_ps(C1, Row2);
		__m128 swp0 = _mm_shuffle_ps(mul0, mul0, _MM_SHUFFLE(2, 3, 0, 1));
		__m128 add0 = _mm_add_ps(mul0, swp0);
		__m128 swp1 = _mm_shuffle_ps(add0, add0, _MM_SHUFFLE(0, 1, 2, 3));
		__m128 Det0 = _mm_add_ps(add0, swp1);

		__m128 Rcp0 = _mm_div_ps(VecOne, Det0);

		mOut.C1 = _mm_mul_ps(Inv0, Rcp0);
		mOut.C2 = _mm_mul_ps(Inv1, Rcp0);
		mOut.C3 = _mm_mul_ps(Inv2, Rcp0);
		mOut.C4 = _mm_mul_ps(Inv3, Rcp0);

		float retVal;
		_mm_store_ss(&retVal, Det0);
		return retVal;
	}

}

/***********************************************************************
CORELIB\WIDECHAR.CPP
***********************************************************************/
#include <locale.h>

#define _CRT_SECUIRE_NO_WARNINGS

class DefaultLocaleSetter
{
public:
	DefaultLocaleSetter()
	{
		setlocale(LC_ALL, ""); 
	};
};


char * WideCharToMByte(const wchar_t * buffer, int length)
{
	size_t requiredBufferSize;
#ifdef _MSC_VER
	wcstombs_s(&requiredBufferSize, nullptr, 0, buffer, length);
#else
	requiredBufferSize = std::wcstombs(nullptr, buffer, 0);
#endif
	if (requiredBufferSize > 0)
	{
		char * multiByteBuffer = new char[requiredBufferSize + 1];
#ifdef _MSC_VER
		wcstombs_s(&requiredBufferSize, multiByteBuffer, requiredBufferSize, buffer, length);
		auto pos = requiredBufferSize;
#else
		auto pos = std::wcstombs(multiByteBuffer, buffer, requiredBufferSize + 1);
#endif
		if (pos <= requiredBufferSize && pos >= 0)
			multiByteBuffer[pos] = 0;
		return multiByteBuffer;
	}
	else
		return 0;
}

wchar_t * MByteToWideChar(const char * buffer, int length)
{
	// regard as ansi
#ifdef _MSC_VER
	size_t bufferSize;
	mbstowcs_s((size_t*)&bufferSize, nullptr, 0, buffer, length);
#else
	size_t bufferSize = std::mbstowcs(nullptr, buffer, 0);
#endif
	if (bufferSize > 0)
	{
		wchar_t * rbuffer = new wchar_t[bufferSize +1];
		size_t pos;
#ifdef _MSC_VER
		mbstowcs_s(&pos, rbuffer, bufferSize, buffer, length);
#else
		pos = std::mbstowcs(rbuffer, buffer, bufferSize + 1);
#endif
		if (pos <= bufferSize && pos >= 0)
			rbuffer[pos] = 0;
		return rbuffer;
	}
	else
		return 0;
}

void MByteToWideChar(wchar_t * buffer, int bufferSize, const char * str, int length)
{
#ifdef _MSC_VER
	size_t pos;
	mbstowcs_s(&pos, buffer, bufferSize, str, length);
#else
	std::mbstowcs(buffer, str, bufferSize);
#endif
}

/***********************************************************************
CORELIB\REGEX\METALEXER.CPP
***********************************************************************/


namespace CoreLib
{
namespace Text
{
	MetaLexer::MetaLexer()
	{
	}

	MetaLexer::MetaLexer(String profile)
	{
		SetLexProfile(profile);
	}

	String MetaLexer::GetTokenName(int id)
	{
		return TokenNames[id];
	}

	int MetaLexer::GetRuleCount()
	{
		return TokenNames.Count();
	}

	void MetaLexer::SetLexProfile(String lex)
	{
		Errors.Clear();
		ParseLexProfile(lex);
		if (!Errors.Count())
			ConstructDFA();
		else
			dfa = 0;
	}

	bool IsWhiteSpace(wchar_t ch)
	{
		return (ch == L' ' || ch == L'\t' || ch == L'\n' || ch == L'\r' || ch == L'\v');
	}

	bool IsIdent(wchar_t ch)
	{
		return (ch >=L'A' && ch <= L'Z' || ch >= L'a' && ch<=L'z' || ch>=L'0' && ch<=L'9'
			|| ch == L'_' || ch==L'#');
	}

	bool IsLetter(wchar_t ch)
	{
		return (ch >=L'A' && ch <= L'Z' || ch >= L'a' && ch<=L'z' || ch == L'_' || ch==L'#');
	}

	bool MetaLexer::ParseLexProfile(const CoreLib::String & lex)
	{
		LinkedList<LexProfileToken> tokens;
		int ptr = 0;
		int state = 0;
		StringBuilder curToken;
		while (ptr < lex.Length())
		{
			wchar_t curChar = lex[ptr];
			wchar_t nextChar = 0;
			if (ptr+1<lex.Length())
				nextChar = lex[ptr+1];
			switch (state)
			{
			case 0:
				{
					if (IsLetter(curChar))
						state = 1;
					else if (IsWhiteSpace(curChar))
						ptr ++;
					else if (curChar == L'{')
					{
						state = 2;
						ptr ++;
					}
					else if (curChar == L'=')
						state = 3;
					else if (curChar == L'/' && nextChar == L'/')
						state = 4;
					else
					{
						LexerError err;
						err.Position = ptr;
						err.Text = String(L"[Profile Error] Illegal character \'") + curChar + L"\'";
						Errors.Add(err);
						ptr ++;
					}
					curToken.Clear();
				}
				break;
			case 1:
				{
					if (IsIdent(curChar))
					{
						curToken.Append(curChar);
						ptr ++;
					}
					else
					{
						LexProfileToken tk;
						tk.str = curToken.ToString();
						tk.type = LexProfileToken::Identifier;
						tokens.AddLast(tk);
						state = 0;
					}
				}
				break;
			case 2:
				{
					if (curChar == L'}' && (nextChar == L'\r' || nextChar == L'\n' || nextChar == 0) )
					{
						LexProfileToken tk;
						tk.str = curToken.ToString();
						tk.type = LexProfileToken::Regex;
						tokens.AddLast(tk);
						ptr ++;
						state = 0;
					}
					else
					{
						curToken.Append(curChar);
						ptr ++;
					}
				}
				break;
			case 3:
				{
					LexProfileToken tk;
					tk.str = curChar;
					tk.type = LexProfileToken::Equal;
					tokens.AddLast(tk);
					ptr ++;
					state = 0;
				}
				break;
			case 4:
				{
					if (curChar == L'\n')
						state = 0;
					else
						ptr ++;
				}
			}
		}

		// Parse tokens
		LinkedNode<LexProfileToken> * l = tokens.FirstNode();
		state = 0;
		String curName, curRegex;
		try
		{
			TokenNames.Clear();
			Regex.Clear();
			while (l)
			{
				curName = ReadProfileToken(l, LexProfileToken::Identifier);
				l = l->GetNext();
				ReadProfileToken(l, LexProfileToken::Equal);
				l = l->GetNext();
				curRegex = ReadProfileToken(l, LexProfileToken::Regex);
				l = l->GetNext();
				TokenNames.Add(curName);
				Regex.Add(curRegex);
				if (curName[0] == L'#')
					Ignore.Add(true);
				else
					Ignore.Add(false);
			}
		}
		catch(int)
		{
			return false;
		}
		return true;
	}

	String MetaLexer::ReadProfileToken(LexProfileTokenNode*n, LexProfileToken::LexProfileTokenType type)
	{
		if (n && n->Value.type == type)
		{
			return n->Value.str;
		}
		else
		{
			String name = L"[Profile Error] ";
			switch (type)
			{
			case LexProfileToken::Equal:
				name = L"\'=\'";
				break;
			case LexProfileToken::Identifier:
				name = L"Token identifier";
				break;
			case LexProfileToken::Regex:
				name = L"Regular expression";
				break;
			}
			name = name + L" expected.";
			LexerError err;
			err.Text = name;
			err.Position = 0;
			Errors.Add(err);
			throw 0;
		}
	}

	DFA_Table * MetaLexer::GetDFA()
	{
		return dfa.operator->();
	}

	void MetaLexer::ConstructDFA()
	{
		RegexParser parser;
		NFA_Graph nfa;
		NFA_Node * node = nfa.CreateNode();
		nfa.SetStartNode(node);
		for (int i=0; i<Regex.Count(); i++)
		{
			RefPtr<RegexNode> tree = parser.Parse(Regex[i]);
			if (tree)
			{
				NFA_Graph cNfa;
				cNfa.GenerateFromRegexTree(tree.operator->(), true);
				cNfa.SetTerminalIdentifier(i);
				nfa.CombineNFA(&cNfa);
				NFA_Translation * trans = nfa.CreateTranslation();
				trans->NodeDest = cNfa.GetStartNode();
				trans->NodeSrc = node;
				trans->NodeDest->PrevTranslations.Add(trans);
				trans->NodeSrc->Translations.Add(trans);
			}
			else
			{
				LexerError err;
				err.Position = 0;
				err.Text = L"Illegal regex for \"" + String(TokenNames[i]) + L"\"";
				Errors.Add(err);
				return;
			}
		}
		nfa.PostGenerationProcess();
		DFA_Graph dfaGraph;
		dfaGraph.Generate(&nfa);
		dfa = new DFA_Table();
		dfaGraph.ToDfaTable(dfa.operator ->());
	}

	LazyLexStream::Iterator & LazyLexStream::Iterator::operator ++()
	{
		auto &str = stream->InputText;
		auto sDfa = stream->GetDFA();
		auto & ignore = stream->GetIgnoreSet();
		if (lastTokenPtr == str.Length())
		{
			lastTokenPtr = -1;
			return *this;
		}

		int lastAcceptState = -1;
		int lastAcceptPtr = -1;
		while (ptr < str.Length())
		{
			if (sDfa->Tags[state]->IsFinal)
			{
				lastAcceptState = state;
				lastAcceptPtr = ptr;
			}
			Word charClass = (*sDfa->CharTable)[str[ptr]];
			if (charClass == 0xFFFF)
			{
				ptr++;
				continue;
			}
			int nextState = sDfa->DFA[state][charClass];
			if (nextState >= 0)
			{
				state = nextState;
				ptr++;
			}
			else
			{
				if (lastAcceptState != -1)
				{
					state = lastAcceptState;
					ptr = lastAcceptPtr;
					
					
					if (!ignore[sDfa->Tags[state]->TerminalIdentifiers[0]])
					{
						currentToken.Length = ptr - lastTokenPtr;
						currentToken.TypeID = sDfa->Tags[state]->TerminalIdentifiers[0];
						currentToken.Position = lastTokenPtr;
						state = sDfa->StartState;
						lastTokenPtr = ptr;
						lastAcceptState = -1;
						lastAcceptPtr = -1;
						break;
					}
					state = sDfa->StartState;
					lastTokenPtr = ptr;
					lastAcceptState = -1;
					lastAcceptPtr = -1;
				}
				else
				{
					ptr++;
					lastAcceptState = lastAcceptPtr = -1;
					lastTokenPtr = ptr;
					state = sDfa->StartState;
					continue;
				}
			}
		}
		if (ptr == str.Length())
		{
			if (sDfa->Tags[state]->IsFinal &&
				!ignore[sDfa->Tags[state]->TerminalIdentifiers[0]])
			{
				currentToken.Length = ptr - lastTokenPtr;
				currentToken.TypeID = sDfa->Tags[state]->TerminalIdentifiers[0];
				currentToken.Position = lastTokenPtr;
			}
			else
			{
				currentToken.Length = 0;
				currentToken.TypeID = -1;
				currentToken.Position = lastTokenPtr;
			}
			lastTokenPtr = ptr;
		}
		
		return *this;
	}

	bool MetaLexer::Parse(String str, LexStream & stream)
	{
		TokensParsed = 0;
		if (!dfa)
			return false;
		int ptr = 0;
		int lastAcceptState = -1;
		int lastAcceptPtr = -1;
		int lastTokenPtr = 0;
		int state = dfa->StartState;
		while (ptr<str.Length())
		{
			if (dfa->Tags[state]->IsFinal)
			{
				lastAcceptState = state;
				lastAcceptPtr = ptr;
			}
			Word charClass = (*dfa->CharTable)[str[ptr]];
			if (charClass == 0xFFFF)
			{
				LexerError err;
				err.Text = String(L"Illegal character \'") + str[ptr] + L"\'";
				err.Position = ptr;
				Errors.Add(err);
				ptr++;
				continue;
			}
			int nextState = dfa->DFA[state][charClass];
			if (nextState >= 0)
			{
				state = nextState;
				ptr++;
			}
			else
			{
				if (lastAcceptState != -1)
				{
					state = lastAcceptState;
					ptr = lastAcceptPtr;
					if (!Ignore[dfa->Tags[state]->TerminalIdentifiers[0]])
					{
						LexToken tk;
						tk.Str = str.SubString(lastTokenPtr, ptr-lastTokenPtr);
						tk.TypeID = dfa->Tags[state]->TerminalIdentifiers[0];
						tk.Position = lastTokenPtr;
						stream.AddLast(tk);
					}
					TokensParsed ++;
					lastTokenPtr = ptr;
					state = dfa->StartState;
					lastAcceptState = -1;
					lastAcceptPtr = -1;
				}
				else
				{
					LexerError err;
					err.Text = L"Illegal token \'" +
						str.SubString(lastTokenPtr, ptr-lastTokenPtr) + L"\'";
					err.Position = ptr;
					Errors.Add(err);
					ptr++;
					lastAcceptState = lastAcceptPtr = -1;
					lastTokenPtr = ptr;
					state = dfa->StartState;
					continue;
				}
			}
		}

		if (dfa->Tags[state]->IsFinal &&
			!Ignore[dfa->Tags[state]->TerminalIdentifiers[0]])
		{
			LexToken tk;
			tk.Str = str.SubString(lastTokenPtr, ptr-lastTokenPtr);
			tk.TypeID = dfa->Tags[state]->TerminalIdentifiers[0];
			stream.AddLast(tk);
			TokensParsed ++;
		}
		return (Errors.Count() == 0);
	}
}
}

/***********************************************************************
CORELIB\REGEX\REGEX.CPP
***********************************************************************/

namespace CoreLib
{
namespace Text
{
	RegexMatcher::RegexMatcher(DFA_Table * table)
		:dfa(table)
	{
	}

	int RegexMatcher::Match(const String & str, int startPos)
	{
		int state = dfa->StartState;
		if (state == -1)
			return -1;
		for (int i=startPos; i<str.Length(); i++)
		{
			Word charClass = (*dfa->CharTable)[str[i]];
			if (charClass == 0xFFFF)
				return -1;
			int nextState = dfa->DFA[state][charClass];
			if (nextState == -1)
			{
				if (dfa->Tags[state]->IsFinal)
					return i-startPos;
				else
					return -1;
			}
			else
				state = nextState;
		}
		if (dfa->Tags[state]->IsFinal)
			return str.Length()-startPos;
		else
			return -1;
	}

	DFA_Table * PureRegex::GetDFA()
	{
		return dfaTable.operator->();
	}

	PureRegex::PureRegex(const String & regex)
	{
		RegexParser p;
		RefPtr<RegexNode> tree = p.Parse(regex);
		if (tree)
		{
			NFA_Graph nfa;
			nfa.GenerateFromRegexTree(tree.operator ->());
			DFA_Graph dfa;
			dfa.Generate(&nfa);
			dfaTable = new DFA_Table();
			dfa.ToDfaTable(dfaTable.operator->());
		}
		else
		{
			IllegalRegexException ex;
			if (p.Errors.Count())
				ex.Message = p.Errors[0].Text;
			throw ex;
		}
	}

	bool PureRegex::IsMatch(const String & str)
	{
		RegexMatcher matcher(dfaTable.operator->());
		return (matcher.Match(str, 0)==str.Length());
	}

	PureRegex::RegexMatchResult PureRegex::Search(const String & str, int startPos)
	{
		RegexMatcher matcher(dfaTable.operator ->());
		for (int i=startPos; i<str.Length(); i++)
		{
			int len = matcher.Match(str, i);
			if (len >= 0)
			{
				RegexMatchResult rs;
				rs.Start = i;
				rs.Length = len;
				return rs;
			}
		}
		RegexMatchResult rs;
		rs.Start = 0;
		rs.Length = -1;
		return rs;
	}
}
}

/***********************************************************************
CORELIB\REGEX\REGEXDFA.CPP
***********************************************************************/

namespace CoreLib
{
namespace Text
{
	CharTableGenerator::CharTableGenerator(RegexCharTable * _table)
		: table(_table)
	{
		table->SetSize(65536);
		memset(table->Buffer(),0,sizeof(Word)*table->Count());
	}

	DFA_Table_Tag::DFA_Table_Tag()
	{
		IsFinal = false;
	}

	int CharTableGenerator::AddSet(String set)
	{
		int fid = sets.IndexOf(set);
		if (fid != -1)
			return fid;
		else
		{
			sets.Add(set);
			return sets.Count()-1;
		}
	}

	int CharTableGenerator::Generate(List<RegexCharSet *> & charSets)
	{
		/*List<RegexCharSet *> cs;
		cs.SetCapacity(charSets.Count());
		String str;
		str.Alloc(1024);
		for (int i=1; i<65536; i++)
		{
			str = L"";
			cs.Clear();
			for (int j=0; j<charSets.Count(); j++)
			{
				if (charSets[j]->Contains(i))
				{
					str += (wchar_t)(j+1);
					cs.Add(charSets[j]);
				}
			}
			int lastCount = sets.Count();
			if (str.Length())
			{
				int id = AddSet(str);
				if (id == lastCount)
				{
					for (int j=0; j<cs.Count(); j++)
						cs[j]->Elements.Add(id);
				}
				(*table)[i] = id;
			}
			else
				(*table)[i] = 0xFFFF;
		}
		return sets.Count();*/
		
		RegexCharSet::CalcCharElements(charSets, elements);
		for (int i=0; i<table->Count(); i++)
			(*table)[i] = 0xFFFF;
		Word* buf = table->Buffer();
		for (int i=0; i<elements.Count(); i++)
		{
			for (int k=elements[i].Begin; k<=elements[i].End; k++)	
			{
#ifdef _DEBUG
				if ((*table)[k] != 0xFFFF)
				{
					throw L"Illegal subset generation."; // This indicates a bug.
				}
#endif
				buf[k] = (Word)i;
			}
		}
		return elements.Count();
	}

	DFA_Node::DFA_Node(int elements)
	{
		Translations.SetSize(elements);
		for (int i=0; i<elements; i++)
			Translations[i] = 0;
	}

	void DFA_Graph::CombineCharElements(NFA_Node * node, List<Word> & elem)
	{
		for (int i=0; i<node->Translations.Count(); i++)
		{
			for (int j=0; j<node->Translations[i]->CharSet->Elements.Count(); j++)
			{
				if (elem.IndexOf(node->Translations[i]->CharSet->Elements[j]) == -1)
					elem.Add(node->Translations[i]->CharSet->Elements[j]);
			}
		}
	}

	void DFA_Graph::Generate(NFA_Graph * nfa)
	{
		table = new RegexCharTable();
		List<RegexCharSet * > charSets;
		for (int i=0; i<nfa->translations.Count(); i++)
		{
			if (nfa->translations[i]->CharSet && nfa->translations[i]->CharSet->Ranges.Count())
				charSets.Add(nfa->translations[i]->CharSet.operator->());
		}
		CharTableGenerator gen(table.operator ->());
		int elements = gen.Generate(charSets);
		CharElements = gen.elements;
		List<DFA_Node *> L,D;
		startNode = new DFA_Node(elements);
		startNode->ID = 0;
		startNode->Nodes.Add(nfa->start);
		L.Add(startNode);
		nodes.Add(startNode);
		List<Word> charElem;
		do
		{
			DFA_Node * node = L.Last();
			L.RemoveAt(L.Count()-1);
			charElem.Clear();
			node->IsFinal = false;
			for (int i=0; i<node->Nodes.Count(); i++)
			{
				CombineCharElements(node->Nodes[i], charElem);
				if (node->Nodes[i]->IsFinal)
					node->IsFinal = true;
			}
			for (int i=0; i<charElem.Count(); i++)
			{
				DFA_Node * n = new DFA_Node(0);
				for (int j=0; j<node->Nodes.Count(); j++)
				{
					for (int k=0; k<node->Nodes[j]->Translations.Count(); k++)
					{
						NFA_Translation * trans = node->Nodes[j]->Translations[k];
						if (trans->CharSet->Elements.Contains(charElem[i]))
						{
							if (!n->Nodes.Contains(node->Nodes[j]->Translations[k]->NodeDest))
								n->Nodes.Add(node->Nodes[j]->Translations[k]->NodeDest);
						}
					}
				}
				int fid = -1;
				for (int j=0; j<nodes.Count(); j++)
				{
					if ((*nodes[j]) == *n)
					{
						fid = j;
						break;
					}
				}
				if (fid == -1)
				{
					n->Translations.SetSize(elements);
					for (int m=0; m<elements; m++)
						n->Translations[m] = 0;
					n->ID = nodes.Count();
					L.Add(n);
					nodes.Add(n);
					fid = nodes.Count()-1;
				}
				else
					delete n;
				n = nodes[fid].operator ->();
				node->Translations[charElem[i]] = n;
			}
		}
		while (L.Count());

		// Set Terminal Identifiers
		HashSet<int> terminalIdentifiers;
		for (int i=0; i<nodes.Count(); i++)
		{
			terminalIdentifiers.Clear();
			for (int j=0; j<nodes[i]->Nodes.Count(); j++)
			{
				if (nodes[i]->Nodes[j]->IsFinal && 
					!terminalIdentifiers.Contains(nodes[i]->Nodes[j]->TerminalIdentifier))
				{
					nodes[i]->IsFinal = true;
					terminalIdentifiers.Add(nodes[i]->Nodes[j]->TerminalIdentifier);
					nodes[i]->TerminalIdentifiers.Add(nodes[i]->Nodes[j]->TerminalIdentifier);
				}
			}
			nodes[i]->TerminalIdentifiers.Sort();
		}
	}

	bool DFA_Node::operator == (const DFA_Node & node)
	{
		if (Nodes.Count() != node.Nodes.Count())
			return false;
		for (int i=0; i<node.Nodes.Count(); i++)
		{
			if (node.Nodes[i] != Nodes[i])
				return false;
		}
		return true;
	}

	String DFA_Graph::Interpret()
	{
		StringBuilder sb(4096000);
		for (int i=0; i<nodes.Count(); i++)
		{
			if (nodes[i]->IsFinal)
				sb.Append(L'#');
			else if (nodes[i] == startNode)
				sb.Append(L'*');
			sb.Append(String(nodes[i]->ID));
			sb.Append(L'(');
			for (int j=0; j<nodes[i]->Nodes.Count(); j++)
			{
				sb.Append(String(nodes[i]->Nodes[j]->ID));
				sb.Append(L" ");
			}
			sb.Append(L")\n");
			for (int j=0; j<nodes[i]->Translations.Count(); j++)
			{
				if (nodes[i]->Translations[j])
				{
					sb.Append(L"\tOn ");
					sb.Append(String(j));
					sb.Append(L": ");
					sb.Append(String(nodes[i]->Translations[j]->ID));
					sb.Append(L'\n');
				}
			}
		}

		sb.Append(L"\n\n==================\n");
		sb.Append(L"Char Set Table:\n");
		for (int i=0; i<CharElements.Count(); i++)
		{
			sb.Append(L"Class ");
			sb.Append(String(i));
			sb.Append(L": ");
			RegexCharSet s;
			s.Ranges.Add(CharElements[i]);
			sb.Append(s.Reinterpret());
			sb.Append(L"\n");
		}
		return sb.ProduceString();
	}

	void DFA_Graph::ToDfaTable(DFA_Table * dfa)
	{
		dfa->CharTable = table;
		dfa->DFA = new int*[nodes.Count()];
		dfa->Tags.SetSize(nodes.Count());
		for (int i=0; i<nodes.Count(); i++)
			dfa->Tags[i] = new DFA_Table_Tag();
		dfa->StateCount = nodes.Count();
		dfa->AlphabetSize = CharElements.Count();
		for (int i=0; i<nodes.Count(); i++)
		{
			dfa->DFA[i] = new int[table->Count()];
			for (int j=0; j<nodes[i]->Translations.Count(); j++)
			{
				if (nodes[i]->Translations[j])
					dfa->DFA[i][j] = nodes[i]->Translations[j]->ID;
				else
					dfa->DFA[i][j] = -1;
			}
			if (nodes[i] == startNode)
				dfa->StartState = i;
			if (nodes[i]->IsFinal)
			{
				dfa->Tags[i]->IsFinal = true;
				dfa->Tags[i]->TerminalIdentifiers = nodes[i]->TerminalIdentifiers;
			}
		}
	}

	DFA_Table::DFA_Table()
	{
		DFA = 0;
		StateCount = 0;
		AlphabetSize = 0;
		StartState = -1;
	}
	
	DFA_Table::~DFA_Table()
	{
		if (DFA)
		{
			for (int i=0; i<StateCount; i++)
				delete [] DFA[i];
			delete [] DFA;
		}
	}
}
}

/***********************************************************************
CORELIB\REGEX\REGEXNFA.CPP
***********************************************************************/

namespace CoreLib
{
namespace Text
{
	int NFA_Node::HandleCount = 0;

	NFA_Translation::NFA_Translation(NFA_Node * src, NFA_Node * dest, RefPtr<RegexCharSet> charSet)
		: NodeSrc(src), NodeDest(dest), CharSet(charSet)
	{}

	NFA_Translation::NFA_Translation()
	{
		NodeSrc = NodeDest = 0;
	}

	NFA_Translation::NFA_Translation(NFA_Node * src, NFA_Node * dest)
		: NodeSrc(src), NodeDest(dest)
	{
	}

	NFA_Node::NFA_Node()
		: Flag(false), IsFinal(false), TerminalIdentifier(0)
	{
		HandleCount ++;
		ID = HandleCount;
	}

	void NFA_Node::RemoveTranslation(NFA_Translation * trans)
	{
		int fid = Translations.IndexOf(trans);
		if (fid != -1)
			Translations.RemoveAt(fid);
	}

	void NFA_Node::RemovePrevTranslation(NFA_Translation * trans)
	{
		int fid = PrevTranslations.IndexOf(trans);
		if (fid != -1)
			PrevTranslations.RemoveAt(fid);
	}

	NFA_Node * NFA_Graph::CreateNode()
	{
		NFA_Node * nNode = new NFA_Node();
		nodes.Add(nNode);
		return nNode;
	}

	NFA_Translation * NFA_Graph::CreateTranslation()
	{
		NFA_Translation * trans = new NFA_Translation();
		translations.Add(trans);
		return trans;
	}

	void NFA_Graph::ClearNodes()
	{
		for (int i=0; i<nodes.Count(); i++)
			nodes[i] = 0;
		for (int i=0; i<translations.Count(); i++)
			translations[i] = 0;
		nodes.Clear();
		translations.Clear();
	}

	void NFA_Graph::GenerateFromRegexTree(RegexNode * tree, bool elimEpsilon)
	{
		NFA_StatePair s;
		tree->Accept(this);
		s = PopState();
		start = s.start;
		end = s.end;
		end->IsFinal = true;

		if (elimEpsilon)
		{
			PostGenerationProcess();
		}

	}

	void NFA_Graph::PostGenerationProcess()
	{
		EliminateEpsilon();
		for (int i=0; i<translations.Count(); i++)
		{
			if (translations[i]->CharSet)
				translations[i]->CharSet->Normalize();
			else
			{
				translations[i] = 0;
				translations.RemoveAt(i);
				i--;
			}
		}
	}

	NFA_Node * NFA_Graph::GetStartNode()
	{
		return start;
	}

	void NFA_Graph::PushState(NFA_StatePair s)
	{
		stateStack.Add(s);
	}

	NFA_Graph::NFA_StatePair NFA_Graph::PopState()
	{
		NFA_StatePair s = stateStack.Last();
		stateStack.RemoveAt(stateStack.Count()-1);
		return s;
	}

	void NFA_Graph::VisitCharSetNode(RegexCharSetNode * node)
	{
		NFA_StatePair s;
		s.start = CreateNode();
		s.end = CreateNode();
		NFA_Translation * trans = CreateTranslation();
		trans->CharSet = node->CharSet;
		trans->NodeSrc = s.start;
		trans->NodeDest = s.end;
		s.start->Translations.Add(trans);
		s.end->PrevTranslations.Add(trans);
		PushState(s);
	}

	void NFA_Graph::VisitRepeatNode(RegexRepeatNode * node)
	{
		NFA_StatePair sr;
		sr.start = sr.end = nullptr;
		node->Child->Accept(this);
		NFA_StatePair s = PopState();
		if (node->RepeatType == RegexRepeatNode::rtArbitary)
		{
			sr.start = CreateNode();
			sr.end = CreateNode();

			NFA_Translation * trans = CreateTranslation();
			trans->NodeSrc = sr.start;
			trans->NodeDest = sr.end;
			sr.start->Translations.Add(trans);
			sr.end->PrevTranslations.Add(trans);
			
			NFA_Translation * trans1 = CreateTranslation();
			trans1->NodeSrc = sr.end;
			trans1->NodeDest = s.start;
			sr.end->Translations.Add(trans1);
			s.start->PrevTranslations.Add(trans1);

			NFA_Translation * trans2 = CreateTranslation();
			trans2->NodeSrc = s.end;
			trans2->NodeDest = sr.end;
			s.end->Translations.Add(trans2);
			sr.end->PrevTranslations.Add(trans2);
		}
		else if (node->RepeatType == RegexRepeatNode::rtOptional)
		{
			sr = s;

			NFA_Translation * trans = CreateTranslation();
			trans->NodeSrc = sr.start;
			trans->NodeDest = sr.end;
			sr.start->Translations.Add(trans);
			sr.end->PrevTranslations.Add(trans);
		}
		else if (node->RepeatType == RegexRepeatNode::rtMoreThanOnce)
		{
			sr = s;

			NFA_Translation * trans = CreateTranslation();
			trans->NodeSrc = sr.end;
			trans->NodeDest = sr.start;
			sr.start->PrevTranslations.Add(trans);
			sr.end->Translations.Add(trans);
		}
		else if (node->RepeatType == RegexRepeatNode::rtSpecified)
		{
			if (node->MinRepeat == 0)
			{
				if (node->MaxRepeat > 0)
				{
					for (int i=1; i<node->MaxRepeat; i++)
					{
						node->Child->Accept(this);
						NFA_StatePair s1 = PopState();
						NFA_Translation * trans = CreateTranslation();
						trans->NodeDest = s1.start;
						trans->NodeSrc = s.end;
						trans->NodeDest->PrevTranslations.Add(trans);
						trans->NodeSrc->Translations.Add(trans);

						trans = CreateTranslation();
						trans->NodeDest = s1.start;
						trans->NodeSrc = s.start;
						trans->NodeDest->PrevTranslations.Add(trans);
						trans->NodeSrc->Translations.Add(trans);

						s.end = s1.end;
					}
					NFA_Translation * trans = CreateTranslation();
					trans->NodeDest = s.end;
					trans->NodeSrc = s.start;
					trans->NodeDest->PrevTranslations.Add(trans);
					trans->NodeSrc->Translations.Add(trans);
					sr = s;
				}
				else if (node->MaxRepeat == 0)
				{
					sr.start = CreateNode();
					sr.end = CreateNode();
					NFA_Translation * trans = CreateTranslation();
					trans->NodeDest = sr.end;
					trans->NodeSrc = sr.start;
					trans->NodeDest->PrevTranslations.Add(trans);
					trans->NodeSrc->Translations.Add(trans);
				}
				else
				{
					// Arbitary repeat
					sr.start = CreateNode();
					sr.end = CreateNode();

					NFA_Translation * trans = CreateTranslation();
					trans->NodeSrc = sr.start;
					trans->NodeDest = sr.end;
					sr.start->Translations.Add(trans);
					sr.end->PrevTranslations.Add(trans);
					
					NFA_Translation * trans1 = CreateTranslation();
					trans1->NodeSrc = sr.end;
					trans1->NodeDest = s.start;
					sr.end->Translations.Add(trans1);
					s.start->PrevTranslations.Add(trans1);

					NFA_Translation * trans2 = CreateTranslation();
					trans2->NodeSrc = s.end;
					trans2->NodeDest = sr.end;
					s.end->Translations.Add(trans2);
					sr.end->PrevTranslations.Add(trans2);
				}
			}
			else
			{
				NFA_Node * lastBegin = s.start;
				for (int i=1; i<node->MinRepeat; i++)
				{
					node->Child->Accept(this);
					NFA_StatePair s1 = PopState();
					NFA_Translation * trans = CreateTranslation();
					trans->NodeDest = s1.start;
					trans->NodeSrc = s.end;
					trans->NodeDest->PrevTranslations.Add(trans);
					trans->NodeSrc->Translations.Add(trans);
					s.end = s1.end;
					lastBegin = s1.start;
				}
				if (node->MaxRepeat == -1)
				{
					NFA_Translation * trans = CreateTranslation();
					trans->NodeDest = lastBegin;
					trans->NodeSrc = s.end;
					trans->NodeDest->PrevTranslations.Add(trans);
					trans->NodeSrc->Translations.Add(trans);
				}
				else if (node->MaxRepeat > node->MinRepeat)
				{
					lastBegin = s.end;
					for (int i=node->MinRepeat; i<node->MaxRepeat; i++)
					{
						node->Child->Accept(this);
						NFA_StatePair s1 = PopState();
						NFA_Translation * trans = CreateTranslation();
						trans->NodeDest = s1.start;
						trans->NodeSrc = s.end;
						trans->NodeDest->PrevTranslations.Add(trans);
						trans->NodeSrc->Translations.Add(trans);

						trans = CreateTranslation();
						trans->NodeDest = s1.start;
						trans->NodeSrc = lastBegin;
						trans->NodeDest->PrevTranslations.Add(trans);
						trans->NodeSrc->Translations.Add(trans);

						s.end = s1.end;
					}

					NFA_Translation * trans = CreateTranslation();
					trans->NodeDest = s.end;
					trans->NodeSrc = lastBegin;
					trans->NodeDest->PrevTranslations.Add(trans);
					trans->NodeSrc->Translations.Add(trans);
				}

				sr = s;
			}
			
		}
		PushState(sr);
	}

	void NFA_Graph::VisitSelectionNode(RegexSelectionNode * node)
	{
		NFA_StatePair s, s1, sr;
		sr.start = CreateNode();
		sr.end = CreateNode();
		s.start = sr.start;
		s.end = sr.end;
		s1.start = sr.start;
		s1.end = sr.end;
		if (node->LeftChild)
		{
			node->LeftChild->Accept(this);
			s = PopState();
		}
		if (node->RightChild)
		{
			node->RightChild->Accept(this);
			s1 = PopState();
		}
		
		NFA_Translation * trans;
		trans = CreateTranslation();
		trans->NodeSrc = sr.start;
		trans->NodeDest = s.start;
		sr.start->Translations.Add(trans);
		s.start->PrevTranslations.Add(trans);

		trans = CreateTranslation();
		trans->NodeSrc = sr.start;
		trans->NodeDest = s1.start;
		sr.start->Translations.Add(trans);
		s1.start->PrevTranslations.Add(trans);

		trans = CreateTranslation();
		trans->NodeSrc = s.end;
		trans->NodeDest = sr.end;
		s.end->Translations.Add(trans);
		sr.end->PrevTranslations.Add(trans);

		trans = CreateTranslation();
		trans->NodeSrc = s1.end;
		trans->NodeDest = sr.end;
		s1.end->Translations.Add(trans);
		sr.end->PrevTranslations.Add(trans);

		PushState(sr);
	}

	void NFA_Graph::VisitConnectionNode(RegexConnectionNode * node)
	{
		NFA_StatePair s, s1;
		node->LeftChild->Accept(this);
		s = PopState();
		node->RightChild->Accept(this);
		s1 = PopState();
		NFA_Translation * trans = CreateTranslation();
		trans->NodeDest = s1.start;
		trans->NodeSrc = s.end;
		s.end->Translations.Add(trans);
		s1.start->PrevTranslations.Add(trans);
		s.end = s1.end;
		PushState(s);
		
	}

	void NFA_Graph::ClearNodeFlags()
	{
		for (int i=0; i<nodes.Count(); i++)
			nodes[i]->Flag = false;
	}

	void NFA_Graph::GetValidStates(List<NFA_Node *> & states)
	{
		RefPtr<List<NFA_Node *>> list1 = new List<NFA_Node *>();
		RefPtr<List<NFA_Node *>> list2 = new List<NFA_Node *>();
		list1->Add(start);
		states.Add(start);
		ClearNodeFlags();
		while (list1->Count())
		{
			list2->Clear();
			for (int i=0; i<list1->Count(); i++)
			{
				bool isValid = false;
				NFA_Node * curNode = (*list1)[i];
				curNode->Flag = true;
				for (int j=0; j<curNode->PrevTranslations.Count(); j++)
				{
					if (curNode->PrevTranslations[j]->CharSet)
					{
						isValid = true;
						break;
					}
					
				}
				if (isValid)
					states.Add(curNode);
				for (int j=0; j<curNode->Translations.Count(); j++)
				{
					if (!curNode->Translations[j]->NodeDest->Flag)
					{
						list2->Add(curNode->Translations[j]->NodeDest);
					}
				}
			}
			RefPtr<List<NFA_Node *>> tmp = list1;
			list1 = list2;
			list2 = tmp;
		}
	}

	void NFA_Graph::GetEpsilonClosure(NFA_Node * node, List<NFA_Node *> & states)
	{
		RefPtr<List<NFA_Node *>> list1 = new List<NFA_Node *>();
		RefPtr<List<NFA_Node *>> list2 = new List<NFA_Node *>();
		list1->Add(node);
		ClearNodeFlags();
		while (list1->Count())
		{
			list2->Clear();
			for (int m=0; m<list1->Count(); m++)
			{
				NFA_Node * curNode = (*list1)[m];
				for (int i=0; i<curNode->Translations.Count(); i++)
				{
					
					if (!curNode->Translations[i]->CharSet)
					{
						if (!curNode->Translations[i]->NodeDest->Flag)
						{
							states.Add(curNode->Translations[i]->NodeDest);
							list2->Add(curNode->Translations[i]->NodeDest);
							curNode->Translations[i]->NodeDest->Flag = true;
						}
					}
				}
			}
			RefPtr<List<NFA_Node *>> tmp = list1;
			list1 = list2;
			list2 = tmp;
		}
	}

	void NFA_Graph::EliminateEpsilon()
	{
		List<NFA_Node *> validStates;
		GetValidStates(validStates);
		for (int i=0; i<validStates.Count(); i++)
		{
			NFA_Node * curState = validStates[i];
			List<NFA_Node *> closure;
			GetEpsilonClosure(curState, closure);
			// Add translations from epsilon closures
			for (int j=0; j<closure.Count(); j++)
			{
				NFA_Node * curNode = closure[j];
				for (int k=0; k<curNode->Translations.Count(); k++)
				{
					if (curNode->Translations[k]->CharSet)
					{
						// Generate a translation from curState to curNode->Dest[k]
						NFA_Translation * trans = CreateTranslation();
						trans->CharSet = curNode->Translations[k]->CharSet;
						trans->NodeSrc = curState;
						trans->NodeDest = curNode->Translations[k]->NodeDest;
						curState->Translations.Add(trans);
						trans->NodeDest->PrevTranslations.Add(trans);
					}
				}
				if (curNode == end)
				{
					curState->IsFinal = true;
					curState->TerminalIdentifier = end->TerminalIdentifier;
				}
			}
		}
		// Remove epsilon-translations and invalid states
		ClearNodeFlags();
		for (int i=0; i<validStates.Count(); i++)
		{
			validStates[i]->Flag = true;
		}
		for (int i=0; i<nodes.Count(); i++)
		{
			if (!nodes[i]->Flag)
			{
				// Remove invalid state
				for (int j=0; j<nodes[i]->PrevTranslations.Count(); j++)
				{
					NFA_Translation * trans = nodes[i]->PrevTranslations[j];
					trans->NodeSrc->RemoveTranslation(trans);
					int fid = translations.IndexOf(trans);
					if (fid != -1)
					{
						translations[fid] = 0;
						translations.RemoveAt(fid);
					}
				}
				for (int j=0; j<nodes[i]->Translations.Count(); j++)
				{
					NFA_Translation * trans = nodes[i]->Translations[j];
					trans->NodeDest->RemovePrevTranslation(trans);
					int fid = translations.IndexOf(trans);
					if (fid != -1)
					{
						translations[fid] = 0;
						translations.RemoveAt(fid);
					}
				}
			}
		}

		for (int i=0; i<validStates.Count(); i++)
		{
			for (int j=0; j<validStates[i]->Translations.Count(); j++)
			{
				NFA_Translation * trans = validStates[i]->Translations[j];
				if (!trans->CharSet)
				{
					validStates[i]->RemoveTranslation(trans);
					trans->NodeDest->RemovePrevTranslation(trans);
					int fid = translations.IndexOf(trans);
					if (fid != -1)
					{
						translations[fid] = 0;
						translations.RemoveAt(fid);
					}
				}
			}
		}

		int ptr = 0;
		while (ptr < nodes.Count())
		{
			if (!nodes[ptr]->Flag)
			{
				nodes[ptr] = 0;
				nodes.RemoveAt(ptr);
			}
			else
				ptr ++;
		}
	}

	String NFA_Graph::Interpret()
	{
		StringBuilder sb(4096);
		for (int i=0; i<nodes.Count(); i++)
		{
			sb.Append(L"State: ");
			if (nodes[i]->IsFinal)
				sb.Append(L"[");
			if (nodes[i] == start)
				sb.Append(L"*");
			sb.Append(String(nodes[i]->ID));
			if (nodes[i]->IsFinal)
				sb.Append(L"]");
			sb.Append(L'\n');
			for (int j=0; j<nodes[i]->Translations.Count(); j++)
			{
				sb.Append(L"\t");
				if (nodes[i]->Translations[j]->CharSet)
					sb.Append(nodes[i]->Translations[j]->CharSet->Reinterpret());
				else
					sb.Append(L"<epsilon>");
				sb.Append(L":");
				sb.Append(String(nodes[i]->Translations[j]->NodeDest->ID));
				sb.Append(L"\n");
			}
		}
		return sb.ProduceString();
		
	}

	void NFA_Graph::SetStartNode(NFA_Node *node)
	{
		start = node;
	}

	void NFA_Graph::CombineNFA(NFA_Graph * graph)
	{
		for (int i=0; i<graph->nodes.Count(); i++)
		{
			nodes.Add(graph->nodes[i]);
		}
		for (int i=0; i<graph->translations.Count(); i++)
		{
			translations.Add(graph->translations[i]);
		}

	}

	void NFA_Graph::SetTerminalIdentifier(int id)
	{
		for (int i=0; i<nodes.Count(); i++)
		{
			if (nodes[i]->IsFinal)
			{
				nodes[i]->TerminalIdentifier = id;
			}
		}
	}
}
}

/***********************************************************************
CORELIB\REGEX\REGEXPARSER.CPP
***********************************************************************/

namespace CoreLib
{
namespace Text
{
	RegexCharSetNode::RegexCharSetNode()
	{
		CharSet = new RegexCharSet();
	}

	RefPtr<RegexNode> RegexParser::Parse(const String &regex)
	{
		src = regex;
		ptr = 0;
		try
		{
			return ParseSelectionNode();
		}
		catch (...)
		{
			return 0;
		}
	}

	RegexNode * RegexParser::ParseSelectionNode()
	{
		if (ptr >= src.Length())
			return 0;
		RefPtr<RegexNode> left = ParseConnectionNode();
		while (ptr < src.Length() && src[ptr] == L'|')
		{
			ptr ++;
			RefPtr<RegexNode> right = ParseConnectionNode();
			RegexSelectionNode * rs = new RegexSelectionNode();
			rs->LeftChild = left;
			rs->RightChild = right;
			left = rs;
		}
		return left.Release();
	}

	RegexNode * RegexParser::ParseConnectionNode()
	{
		if (ptr >= src.Length())
		{
			return 0;
		}
		RefPtr<RegexNode> left = ParseRepeatNode();
		while (ptr < src.Length() && src[ptr] != L'|' && src[ptr] != L')')
		{
			RefPtr<RegexNode> right = ParseRepeatNode();
			if (right)
			{
				RegexConnectionNode * reg = new RegexConnectionNode();
				reg->LeftChild = left;
				reg->RightChild = right;
				left = reg;
			}
			else
				break;
		}
		return left.Release();
	}

	RegexNode * RegexParser::ParseRepeatNode()
	{
		if (ptr >= src.Length() || src[ptr] == L')' || src[ptr] == L'|')
			return 0;
		
		RefPtr<RegexNode> content;
		if (src[ptr] == L'(')
		{
			RefPtr<RegexNode> reg;
			ptr ++;
			reg = ParseSelectionNode();
			if (src[ptr] != L')')
			{
				SyntaxError err;
				err.Position = ptr;
				err.Text = L"\')\' expected.";
				Errors.Add(err);
				throw 0;
			}
			ptr ++;
			content = reg.Release();
		}
		else
		{
			RefPtr<RegexCharSetNode> reg;
			if (src[ptr] == L'[')
			{
				reg = new RegexCharSetNode();
				ptr ++;
				reg->CharSet = ParseCharSet();
				
				if (src[ptr] != L']')
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = L"\']\' expected.";
					Errors.Add(err);
					throw 0;
				}
				ptr ++;
			}
			else if (src[ptr] == L'\\')
			{
				ptr ++;
				reg = new RegexCharSetNode();
				reg->CharSet = new RegexCharSet();
				switch (src[ptr])
				{
				case L'.':
					{
						reg->CharSet->Neg = true;
						break;
					}
				case L'w':
				case L'W':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'a';
						range.End = L'z';
						reg->CharSet->Ranges.Add(range);
						range.Begin = L'A';
						range.End = L'Z';
						reg->CharSet->Ranges.Add(range);
						range.Begin = L'_';
						range.End = L'_';
						reg->CharSet->Ranges.Add(range);
						range.Begin = L'0';
						range.End = L'9';
						reg->CharSet->Ranges.Add(range);
						if (src[ptr] == L'W')
							reg->CharSet->Neg = true;
						break;
					}
				case L's':
				case 'S':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L' ';
						range.End = L' ';
						reg->CharSet->Ranges.Add(range);
						range.Begin = L'\t';
						range.End = L'\t';
						reg->CharSet->Ranges.Add(range);
						range.Begin = L'\r';
						range.End = L'\r';
						reg->CharSet->Ranges.Add(range);
						range.Begin = L'\n';
						range.End = L'\n';
						reg->CharSet->Ranges.Add(range);
						if (src[ptr] == L'S')
							reg->CharSet->Neg = true;
						break;
					}
				case L'd':
				case L'D':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'0';
						range.End = L'9';
						reg->CharSet->Ranges.Add(range);
						if (src[ptr] == L'D')
							reg->CharSet->Neg = true;
						break;
					}
				case L'n':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'\n';
						range.End = L'\n';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case L't':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'\t';
						range.End = L'\t';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case L'r':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'\r';
						range.End = L'\r';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case L'v':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'\v';
						range.End = L'\v';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case L'f':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = L'\f';
						range.End = L'\f';
						reg->CharSet->Ranges.Add(range);
						break;
					}
				case L'*':
				case L'|':
				case L'(':
				case L')':
				case L'?':
				case L'+':
				case L'[':
				case L']':
				case L'\\':
					{
						RegexCharSet::RegexCharRange range;
						reg->CharSet->Neg = false;
						range.Begin = src[ptr];
						range.End = range.Begin;
						reg->CharSet->Ranges.Add(range);
						break;
					}
				default:
					{
						SyntaxError err;
						err.Position = ptr;
						err.Text = String(L"Illegal escape sequence \'\\") + src[ptr] + L"\'";
						Errors.Add(err);
						throw 0;
					}
				}
				ptr ++;
			}
			else if (!IsOperator())
			{
				RegexCharSet::RegexCharRange range;
				reg = new RegexCharSetNode();
				reg->CharSet->Neg = false;
				range.Begin = src[ptr];
				range.End = range.Begin;
				ptr ++;
				reg->CharSet->Ranges.Add(range);
			}
			else
			{
				SyntaxError err;
				err.Position = ptr;
				err.Text = String(L"Unexpected \'") + src[ptr] + L'\'';
				Errors.Add(err);
				throw 0;
			}
			content = reg.Release();
		}
		if (ptr < src.Length())
		{
			if (src[ptr] == L'*')
			{
				RefPtr<RegexRepeatNode> node = new RegexRepeatNode();
				node->Child = content;
				node->RepeatType = RegexRepeatNode::rtArbitary;
				ptr ++;
				return node.Release();
			}
			else if (src[ptr] == L'?')
			{
				RefPtr<RegexRepeatNode> node = new RegexRepeatNode();
				node->Child = content;
				node->RepeatType = RegexRepeatNode::rtOptional;
				ptr ++;
				return node.Release();
			}
			else if (src[ptr] == L'+')
			{
				RefPtr<RegexRepeatNode> node = new RegexRepeatNode();
				node->Child = content;
				node->RepeatType = RegexRepeatNode::rtMoreThanOnce;
				ptr ++;
				return node.Release();
			}
			else if (src[ptr] == L'{')
			{
				ptr++;
				RefPtr<RegexRepeatNode> node = new RegexRepeatNode();
				node->Child = content;
				node->RepeatType = RegexRepeatNode::rtSpecified;
				node->MinRepeat = ParseInteger();
				if (src[ptr] == L',')
				{
					ptr ++;
					node->MaxRepeat = ParseInteger();
				}
				else
					node->MaxRepeat = node->MinRepeat;
				if (src[ptr] == L'}')
					ptr++;
				else
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = L"\'}\' expected.";
					Errors.Add(err);
					throw 0;
				}
				if (node->MinRepeat < 0)
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = L"Minimun repeat cannot be less than 0.";
					Errors.Add(err);
					throw 0;
				}
				if (node->MaxRepeat != -1 && node->MaxRepeat < node->MinRepeat)
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = L"Max repeat cannot be less than min repeat.";
					Errors.Add(err);
					throw 0;
				}
				return node.Release();
			}
		}
		return content.Release();
	}

	bool IsDigit(wchar_t ch)
	{
		return ch>=L'0'&& ch <=L'9';
	}

	int RegexParser::ParseInteger()
	{
		StringBuilder number;
		while (IsDigit(src[ptr]))
		{
			number.Append(src[ptr]);
			ptr ++;
		}
		if (number.Length() == 0)
			return -1;
		else
			return StringToInt(number.ProduceString());
	}

	bool RegexParser::IsOperator()
	{
		return (src[ptr] == L'|' || src[ptr] == L'*' || src[ptr] == L'(' || src[ptr] == L')'
				|| src[ptr] == L'?' || src[ptr] == L'+');
	}

	wchar_t RegexParser::ReadNextCharInCharSet()
	{
		if (ptr < src.Length() && src[ptr] != L']')
		{
			if (src[ptr] == L'\\')
			{
				ptr ++;
				if (ptr >= src.Length())
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = String(L"Unexpected end of char-set when looking for escape sequence.");
					Errors.Add(err);
					throw 0;
				}
				wchar_t rs = 0;
				if (src[ptr] == L'\\')
					rs = L'\\';
				else if (src[ptr] == L'^')
					rs = L'^';
				else if (src[ptr] == L'-')
					rs = L'-';
				else if (src[ptr] == L']')
					rs = L']';
				else if (src[ptr] == L'n')
					rs = L'\n';
				else if (src[ptr] == L't')
					rs = L'\t';
				else if (src[ptr] == L'r')
					rs = L'\r';
				else if (src[ptr] == L'v')
					rs = L'\v';
				else if (src[ptr] == L'f')
					rs = L'\f';
				else
				{
					SyntaxError err;
					err.Position = ptr;
					err.Text = String(L"Illegal escape sequence inside charset definition \'\\") + src[ptr] + L"\'";
					Errors.Add(err);
					throw 0;
				}
				ptr ++;
				return rs;
			}
			else
				return src[ptr++];
		}
		else
		{
			SyntaxError err;
			err.Position = ptr;
			err.Text = String(L"Unexpected end of char-set.");
			Errors.Add(err);
			throw 0;
		}
	}

	RegexCharSet * RegexParser::ParseCharSet()
	{
		RefPtr<RegexCharSet> rs = new RegexCharSet();
		if (src[ptr] == L'^')
		{
			rs->Neg = true;
			ptr ++;
		}
		else
			rs->Neg = false;
		RegexCharSet::RegexCharRange range;
		while (ptr < src.Length() && src[ptr] != L']')
		{
			range.Begin = ReadNextCharInCharSet();
			//ptr ++;
			
			if (ptr >= src.Length())
			{
				break;
			}
			if (src[ptr] == L'-')
			{
				ptr ++;
				range.End = ReadNextCharInCharSet();	
			}
			else
			{
				range.End = range.Begin;
			}
			rs->Ranges.Add(range);
		
		}
		if (ptr >=src.Length() || src[ptr] != L']')
		{
			SyntaxError err;
			err.Position = ptr;
			err.Text = String(L"Unexpected end of char-set.");
			Errors.Add(err);
			throw 0;
		}
		return rs.Release();
	}
}
}

/***********************************************************************
CORELIB\REGEX\REGEXTREE.CPP
***********************************************************************/

namespace CoreLib
{
namespace Text
{
	void RegexNodeVisitor::VisitCharSetNode(RegexCharSetNode * )
	{
	}

	void RegexNodeVisitor::VisitRepeatNode(RegexRepeatNode * )
	{

	}

	void RegexNodeVisitor::VisitSelectionNode(RegexSelectionNode * )
	{
	}

	void RegexNodeVisitor::VisitConnectionNode(RegexConnectionNode * )
	{

	}

	void RegexCharSetNode::Accept(RegexNodeVisitor * visitor)
	{
		visitor->VisitCharSetNode(this);
	}

	void RegexSelectionNode::Accept(RegexNodeVisitor * visitor)
	{
		visitor->VisitSelectionNode(this);
	}

	void RegexConnectionNode::Accept(RegexNodeVisitor * visitor)
	{
		visitor->VisitConnectionNode(this);
	}

	void RegexRepeatNode::Accept(RegexNodeVisitor *visitor)
	{
		visitor->VisitRepeatNode(this);
	}

	String RegexConnectionNode::Reinterpret()
	{
		return LeftChild->Reinterpret() + RightChild->Reinterpret();
	}

	String RegexSelectionNode::Reinterpret()
	{
		return LeftChild->Reinterpret() + L"|" + RightChild->Reinterpret();
	}

	String RegexRepeatNode::Reinterpret()
	{
		wchar_t t;
		if (RepeatType == RegexRepeatNode::rtArbitary)
			t = L'*';
		else if (RepeatType == rtOptional)
			t = L'?';
		else
			t = L'+';
		return String(L"(") + Child->Reinterpret() + L")" + t;
	}

	String RegexCharSet::Reinterpret()
	{
		if (Ranges.Count()== 1 && Ranges[0].Begin == Ranges[0].End &&
			!Neg)
		{
			return (Ranges[0].Begin>=28 && Ranges[0].Begin <127)? String((wchar_t)Ranges[0].Begin):
				String(L"<") + String((int)Ranges[0].Begin) + String(L">");
		}
		else
		{
			StringBuilder rs;
			rs.Append(L"[");
			if (Neg)
				rs.Append(L'^');
			for (int i=0; i<Ranges.Count(); i++)
			{
				if (Ranges[i].Begin == Ranges[i].End)
					rs.Append(Ranges[i].Begin);
				else
				{
					rs.Append(Ranges[i].Begin>=28 && Ranges[i].Begin<128?Ranges[i].Begin:
						String(L"<") + String((int)Ranges[i].Begin) + L">");
					rs.Append(L'-');
					rs.Append(Ranges[i].End>=28 && Ranges[i].End<128?Ranges[i].End:
						String(L"<") + String((int)Ranges[i].End)+ L">");
				}
			}
			rs.Append(L']');
			return rs.ProduceString();
		}
	}

	String RegexCharSetNode::Reinterpret()
	{
		return CharSet->Reinterpret();
	}

	void RegexCharSet::Sort()
	{
		for (int i=0; i<Ranges.Count()-1; i++)
		{
			for (int j=i+1; j<Ranges.Count(); j++)
			{
				RegexCharRange ri,rj;
				ri = Ranges[i];
				rj = Ranges[j];
				if (Ranges[i].Begin > Ranges[j].Begin)
				{
					RegexCharRange range = Ranges[i];
					Ranges[i] = Ranges[j];
					Ranges[j] = range;
				}
			}
		}
	}

	void RegexCharSet::Normalize()
	{
		for (int i=0; i<Ranges.Count()-1; i++)
		{
			for (int j=i+1; j<Ranges.Count(); j++)
			{
				if ((Ranges[i].Begin >= Ranges[j].Begin && Ranges[i].Begin <= Ranges[j].End) ||
					(Ranges[j].Begin >= Ranges[i].Begin && Ranges[j].Begin <= Ranges[i].End) )
				{
					Ranges[i].Begin = Math::Min(Ranges[i].Begin, Ranges[j].Begin);
					Ranges[i].End = Math::Max(Ranges[i].End, Ranges[j].End);
					Ranges.RemoveAt(j);
					j--;
				}
			}
		}
		Sort();
		if (Neg)
		{
			List<RegexCharRange> nranges;
			nranges.AddRange(Ranges);
			Ranges.Clear();
			RegexCharRange range;
			range.Begin = 1;
			for (int i=0; i<nranges.Count(); i++)
			{
				range.End = nranges[i].Begin-1;
				Ranges.Add(range);
				range.Begin = nranges[i].End+1;
			}
			range.End = 65530;
			Ranges.Add(range);
			Neg = false;
		}
	}

	bool RegexCharSet::Contains(RegexCharRange r)
	{
		for (int i=0; i<Ranges.Count(); i++)
		{
			if (r.Begin >= Ranges[i].Begin && r.End <= Ranges[i].End)
				return true;
		}
		return false;
	}

	void RegexCharSet::RangeIntersection(RegexCharRange r1, RegexCharRange r2, RegexCharSet & rs)
	{
		RegexCharRange r;
		r.Begin = Math::Max(r1.Begin,r2.Begin);
		r.End = Math::Min(r1.End, r2.End);
		if (r.Begin <= r.End)
			rs.Ranges.Add(r);
	}
	
	void RegexCharSet::RangeMinus(RegexCharRange r1, RegexCharRange r2, RegexCharSet & rs)
	{
		if (r2.Begin <= r1.Begin && r2.End>= r1.Begin && r2.End <= r1.End)
		{
			RegexCharRange r;
			r.Begin = ((int)r2.End + 1)>0xFFFF?0xFFFF:r2.End+1;
			r.End = r1.End;
			if (r.Begin <= r.End && !(r.Begin == r.End && r.Begin == 65530))
				rs.Ranges.Add(r);
		}
		else if (r2.Begin >= r1.Begin && r2.Begin <= r1.End && r2.End >= r1.End)
		{
			RegexCharRange r;
			r.Begin = r1.Begin;
			r.End = r2.Begin == 1? 1: r2.Begin - 1;
			if (r.Begin <= r.End && !(r.Begin == r.End == 1))
				rs.Ranges.Add(r);
		}
		else if (r2.Begin >= r1.Begin && r2.End <= r1.End)
		{
			RegexCharRange r;
			r.Begin = r1.Begin;
			r.End = r2.Begin == 1? 1: r2.Begin - 1;
			if (r.Begin <= r.End && !(r.Begin == r.End && r.Begin  == 1))
				rs.Ranges.Add(r);
			r.Begin = r2.End == 0xFFFF? r2.End : r2.End + 1;
			r.End = r1.End;
			if (r.Begin <= r.End && !(r.Begin == r.End && r.Begin  == 65530))
				rs.Ranges.Add(r);
		}
		else if (r2.End<r1.Begin || r1.End < r2.Begin)
		{
			rs.Ranges.Add(r1);
		}
	}

	void RegexCharSet::CharSetMinus(RegexCharSet & s1, RegexCharSet & s2)
	{
		RegexCharSet s;
		for (int i=0; i<s1.Ranges.Count(); i++)
		{
			for (int j=0; j<s2.Ranges.Count(); j++)
			{
				if (i>=s1.Ranges.Count() || i<0)
					return;
				s.Ranges.Clear();
				RangeMinus(s1.Ranges[i], s2.Ranges[j], s);
				if (s.Ranges.Count() == 1)
					s1.Ranges[i] = s.Ranges[0];
				else if (s.Ranges.Count() == 2)
				{
					s1.Ranges[i] = s.Ranges[0];
					s1.Ranges.Add(s.Ranges[1]);
				}
				else
				{
					s1.Ranges.RemoveAt(i);
					i--;
				}
			}
		}
	}

	RegexCharSet & RegexCharSet::operator = (const RegexCharSet & set)
	{
		CopyCtor(set);
		return *this;
	}

	bool RegexCharSet::RegexCharRange::operator == (const RegexCharRange & r)
	{
		return r.Begin == Begin && r.End == End;
	}

	void RegexCharSet::AddRange(RegexCharRange newR)
	{
		//RegexCharSet set;
		//set.Ranges.Add(r);
		//for (int i=0; i<Ranges.Count(); i++)
		//{
		//	if (Ranges[i].Begin < r.Begin && Ranges[i].End > r.Begin)
		//	{
		//		RegexCharRange nrange;
		//		nrange.Begin = r.Begin;
		//		nrange.End = Ranges[i].End;
		//		Ranges[i].End = r.Begin == 1? 1:r.Begin-1;
		//		if (!Ranges.Contains(nrange))
		//			Ranges.Add(nrange);
		//	}
		//	if (r.End > Ranges[i].Begin && r.End < Ranges[i].End)
		//	{
		//		RegexCharRange nrange;
		//		nrange.Begin = r.End == 0xFFFF ? 0xFFFF : r.End+1;
		//		nrange.End = Ranges[i].End;
		//		Ranges[i].End = r.End;
		//		if (!Ranges.Contains(nrange))
		//			Ranges.Add(nrange);
		//	}
		//	if (r.Begin == Ranges[i].Begin && r.End == Ranges[i].End)
		//		return;
		//}
		//for (int i=0; i<Ranges.Count(); i++)
		//	set.SubtractRange(Ranges[i]);
		//for (int i=0; i<set.Ranges.Count(); i++)
		//{
		//	for (int j=0; j<Ranges.Count(); j++)
		//		if (Ranges[j].Begin == set.Ranges[i].Begin ||
		//			Ranges[j].Begin == set.Ranges[i].End ||
		//			Ranges[j].End == set.Ranges[i].End||
		//			Ranges[j].End == set.Ranges[i].Begin)
		//		{
		//			RegexCharRange sr = set.Ranges[i];
		//			RegexCharRange r = Ranges[j];
		//			throw 0;
		//		}
		//	if (!Ranges.Contains(set.Ranges[i]))
		//		Ranges.Add(set.Ranges[i]);
		//}
		//Normalize();
		if (newR.Begin > newR.End)
			return;
		Sort();
		int rangeCount = Ranges.Count();
		for (int i=0; i<rangeCount; i++)
		{
			RegexCharRange & oriR = Ranges[i];
			if (newR.Begin > oriR.Begin)
			{
				if (newR.Begin > oriR.End)
				{

				}
				else if (newR.End > oriR.End)
				{
					RegexCharRange nRange;
					nRange.Begin = newR.Begin;
					nRange.End = oriR.End;
					wchar_t newR_begin = newR.Begin;
					newR.Begin = oriR.End + 1;
					oriR.End = newR_begin-1;
					Ranges.Add(nRange);
				}
				else if (newR.End == oriR.End)
				{
					oriR.End = newR.Begin - 1;
					Ranges.Add(newR);
					return;
				}
				else if (newR.End < oriR.End)
				{
					RegexCharRange nRange;
					nRange.Begin = newR.End + 1;
					nRange.End = oriR.End;
					oriR.End = newR.Begin - 1;
					Ranges.Add(newR);
					Ranges.Add(nRange);
					return;
				}
			}
			else if (newR.Begin == oriR.Begin)
			{
				if (newR.End > oriR.End)
				{
					newR.Begin = oriR.End + 1;
				}
				else if (newR.End == oriR.End)
				{
					return;
				}
				else
				{
					wchar_t oriR_end = oriR.End;
					oriR.End = newR.End;
					newR.End = oriR_end;
					newR.Begin = oriR.End + 1;
					Ranges.Add(newR);
					return;
				}
			}
			else if (newR.Begin < oriR.Begin)
			{
				if (newR.End > oriR.End)
				{
					RegexCharRange nRange;
					nRange.Begin = newR.Begin;
					nRange.End = oriR.Begin-1;
					Ranges.Add(nRange);
					newR.Begin = oriR.Begin;
					i--;
				}
				else if (newR.End == oriR.End)
				{
					RegexCharRange nRange;
					nRange.Begin = newR.Begin;
					nRange.End = oriR.Begin-1;
					Ranges.Add(nRange);
					return;
				}
				else if (newR.End < oriR.End && newR.End >= oriR.Begin)
				{
					RegexCharRange nRange;
					nRange.Begin = newR.Begin;
					nRange.End = oriR.Begin-1;
					Ranges.Add(nRange);
					nRange.Begin = newR.End+1;
					nRange.End = oriR.End;
					Ranges.Add(nRange);
					oriR.End = newR.End;
					return;
				}
				else
					break;
			}
		}
		Ranges.Add(newR);
		
	}

	void RegexCharSet::SubtractRange(RegexCharRange r)
	{
		
		int rc = Ranges.Count();
		for (int i=0; i<rc; i++)
		{
			RegexCharSet rs;
			RangeMinus(Ranges[i], r, rs);
			if (rs.Ranges.Count() == 1)
				Ranges[i] = rs.Ranges[0];
			else if (rs.Ranges.Count() == 0)
			{
				Ranges.RemoveAt(i);
				i--;
				rc--;
			}
			else
			{
				Ranges[i] = rs.Ranges[0];
				Ranges.Add(rs.Ranges[1]);
			}
		}
		Normalize();
	}

	void RegexCharSet::CalcCharElementFromPair(RegexCharSet * c1, RegexCharSet * c2, RegexCharSet & AmB, RegexCharSet & BmA, RegexCharSet & AnB)
	{
		AmB = *c1;
		BmA = *c2;
		CharSetMinus(AmB, *c2);
		CharSetMinus(BmA, *c1);
		for (int i=0; i<c1->Ranges.Count(); i++)
		{
			if (c2->Ranges.Count())
			{
				for (int j=0; j<c2->Ranges.Count(); j++)
				{
					RangeIntersection(c1->Ranges[i], c2->Ranges[j], AnB);
				}
			}
		}
		AmB.Normalize();
		BmA.Normalize();
		AnB.Normalize();
	}

	bool RegexCharSet::operator ==(const RegexCharSet & set)
	{
		if (Ranges.Count() != set.Ranges.Count())
			return false;
		for (int i=0; i<Ranges.Count(); i++)
		{
			if (Ranges[i].Begin != set.Ranges[i].Begin ||
				Ranges[i].End != set.Ranges[i].End)
				return false;
		}
		return true;
	}

	void RegexCharSet::InsertElement(List<RefPtr<RegexCharSet>> &L, RefPtr<RegexCharSet> & elem)
	{
		bool find = false;
		for (int i=0; i<L.Count(); i++)
		{
			if ((*L[i]) == *elem)
			{
				for (int k=0; k<elem->OriSet.Count(); k++)
				{
					if (!L[i]->OriSet.Contains(elem->OriSet[k]))
						L[i]->OriSet.Add(elem->OriSet[k]);
				}
				find = true;
				break;
			}
		}
		if (!find)
			L.Add(elem);
	}

	void RegexCharSet::CalcCharElements(List<RegexCharSet *> &sets, List<RegexCharRange> & elements)
	{
		RegexCharSet set;
		for (int i=0; i<sets.Count(); i++)
			for (int j=0; j<sets[i]->Ranges.Count(); j++)
				set.AddRange(sets[i]->Ranges[j]);
		for (int j=0; j<set.Ranges.Count(); j++)
		{
			for (int i=0; i<sets.Count(); i++)
			{
				if (sets[i]->Contains(set.Ranges[j]))
					sets[i]->Elements.Add((unsigned short)j);
			}
			elements.Add(set.Ranges[j]);
		}
		/*
		List<RefPtr<RegexCharSet>> L;
		if (!sets.Count())
			return;
		int lastSetCount = sets.Count();
		for (int i=0; i<sets.Count(); i++)
			sets[i]->OriSet.Add(sets[i]);
		L.Add(new RegexCharSet(*(sets[0])));
		for (int i=1; i<sets.Count(); i++)
		{
			RefPtr<RegexCharSet> bma = new RegexCharSet(*sets[i]);
			bma->OriSet = sets[i]->OriSet;
			for (int j=L.Count()-1; j>=0; j--)
			{
				RefPtr<RegexCharSet> bma2 = new RegexCharSet();
				RefPtr<RegexCharSet> amb = new RegexCharSet();
				RefPtr<RegexCharSet> anb = new RegexCharSet();
				CalcCharElementFromPair(L[j].operator ->(), sets[i], *amb, *bma2, *anb);
				CharSetMinus(*bma, *L[j]);
				L[j]->Normalize();
				amb->OriSet = L[j]->OriSet;
				anb->OriSet = amb->OriSet;
				for (int k=0; k<bma->OriSet.Count(); k++)
				{
					if (!anb->OriSet.Contains(bma->OriSet[k]))
						anb->OriSet.Add(bma->OriSet[k]);
				}
				if (amb->Ranges.Count())
				{
					L[j] = amb;
				}
				else
				{
					L[j] = 0;
					L.RemoveAt(j);
				}
				if (anb->Ranges.Count())
				{
					InsertElement(L,anb);
				}
			}
			if (bma->Ranges.Count())
			{
				InsertElement(L,bma);
			}

		}
		for (int i=0; i<L.Count(); i++)
		{
			for (int j=0; j<L[i]->OriSet.Count(); j++)
			{
				L[i]->OriSet[j]->Elements.Add(i);
			}
			elements.Add(L[i].Release());
		}
		for (int i=lastSetCount; i<sets.Count(); i++)
			RemoveAt sets[i];
		sets.SetSize(lastSetCount);

		*/
	}

	void RegexCharSet::CopyCtor(const RegexCharSet & set)
	{
		Ranges = set.Ranges;
		Elements = set.Elements;
		Neg = set.Neg;
		OriSet = set.OriSet;
	}

	RegexCharSet::RegexCharSet(const RegexCharSet & set)
	{
		CopyCtor(set);
	}


}
}

/***********************************************************************
SPIRECORE\CLOSURE.CPP
***********************************************************************/

namespace Spire
{
	namespace Compiler
	{
		void CheckComponentRedefinition(ErrorWriter * err, ShaderClosure * parent, ShaderClosure * child)
		{
			for (auto & comp : child->Components)
			{
				RefPtr<ShaderComponentSymbol> ccomp;
				RefPtr<ShaderClosure> su;
				if ((comp.Value->Implementations.First()->SyntaxNode->IsPublic ||
					comp.Value->Implementations.First()->SyntaxNode->IsOutput))
				{
					if (parent->Components.TryGetValue(comp.Key, ccomp))
						err->Error(33022, L"\'" + comp.Key + L"\' is already defined in current scope.\nsee previous definition at " + ccomp->Implementations.First()->SyntaxNode->Position.ToString(),
							comp.Value->Implementations.First()->SyntaxNode->Position);
					else if (parent->SubClosures.TryGetValue(comp.Key, su))
						err->Error(33022, L"\'" + comp.Key + L"\' is already defined in current scope.\nsee previous definition at " + su->UsingPosition.ToString(),
							comp.Value->Implementations.First()->SyntaxNode->Position);
				}
			}
			for (auto & c : child->SubClosures)
			{
				if (c.Value->IsInPlace)
				{
					RefPtr<ShaderComponentSymbol> ccomp;
					RefPtr<ShaderClosure> su;
					if (parent->Components.TryGetValue(c.Key, ccomp))
						err->Error(33022, L"\'" + c.Key + L"\' is already defined in current scope.\nsee previous definition at " + ccomp->Implementations.First()->SyntaxNode->Position.ToString(),
							c.Value->UsingPosition);
					else if (parent->SubClosures.TryGetValue(c.Key, su))
						err->Error(33022, L"\'" + c.Key + L"\' is already defined in current scope.\nsee previous definition at " + su->UsingPosition.ToString(),
							c.Value->UsingPosition);
					for (auto & sc : c.Value->SubClosures)
						if (sc.Value->IsInPlace)
							CheckComponentRedefinition(err, parent, sc.Value.Ptr());
				}
			}
		}
		RefPtr<ShaderClosure> CreateShaderClosure(ErrorWriter * err, SymbolTable * symTable, ShaderSymbol * shader, CodePosition usingPos, const Dictionary<String, RefPtr<ShaderComponentSymbol>>& pRefMap)
		{
			RefPtr<ShaderClosure> rs = new ShaderClosure();
			rs->Name = shader->SyntaxNode->Name.Content;
			rs->RefMap = pRefMap;
			rs->Pipeline = shader->Pipeline;
			rs->UsingPosition = usingPos;
			rs->Position = shader->SyntaxNode->Position;
			for (auto & mbr : shader->SyntaxNode->Members)
			{
				if (auto import = dynamic_cast<ImportSyntaxNode*>(mbr.Ptr()))
				{
					// create component for each argument
					Dictionary<String, RefPtr<ShaderComponentSymbol>> refMap;
					for (auto & arg : import->Arguments)
					{
						RefPtr<ShaderComponentSymbol> ccomp = new ShaderComponentSymbol();
						auto compName = L"arg" + String(rs->Components.Count()) + L"_" + 
							(import->ObjectName.Content.Length()==0?import->ShaderName.Content:import->ObjectName.Content) + arg->ArgumentName.Content;
						auto impl = new ShaderComponentImplSymbol();
						auto compSyntax = new ComponentSyntaxNode();
						compSyntax->Position = arg->Expression->Position;
						compSyntax->Name.Content = compName;
						CloneContext cloneCtx;
						compSyntax->Expression = arg->Expression->Clone(cloneCtx);
						compSyntax->Type = TypeSyntaxNode::FromExpressionType(arg->Expression->Type);
						compSyntax->Type->Position = compSyntax->Position;
						impl->SyntaxNode = compSyntax;
						ccomp->Name = compName;
						ccomp->Type = new Type();
						ccomp->Type->DataType = arg->Expression->Type;
						ccomp->Implementations.Add(impl);
						rs->Components[compName] = ccomp;
						refMap[arg->ArgumentName.Content] = ccomp;
					}
					RefPtr<ShaderSymbol> shaderSym;
					if (symTable->Shaders.TryGetValue(import->ShaderName.Content, shaderSym))
					{
						// fill in automatic arguments
						for (auto & param : shaderSym->Components)
						{
							if (param.Value->IsParam() && !refMap.ContainsKey(param.Key))
							{
								auto arg = rs->FindComponent(param.Key);
								if (arg && arg->Type->DataType == param.Value->Type->DataType)
								{
									refMap[param.Key] = arg;
								}
							}
						}
						auto refClosure = CreateShaderClosure(err, symTable, shaderSym.Ptr(), import->Position, refMap);
						refClosure->IsPublic = import->IsPublic;
						refClosure->Parent = rs.Ptr();
						if (import->IsInplace)
						{
							refClosure->IsInPlace = true;
							CheckComponentRedefinition(err, rs.Ptr(), refClosure.Ptr());
							rs->SubClosures[L"annonymousObj" + String(GUID::Next())] = refClosure;
						}
						else
						{
							rs->SubClosures[import->ObjectName.Content] = refClosure;
						}
					}
				}
				else if (auto compt = dynamic_cast<ComponentSyntaxNode*>(mbr.Ptr()))
				{
					RefPtr<ShaderComponentSymbol> comp;
					if (shader->Components.TryGetValue(compt->Name.Content, comp) &&
						!rs->Components.ContainsKey(compt->Name.Content))
					{
						RefPtr<ShaderComponentSymbol> ccomp = new ShaderComponentSymbol(*comp);
						rs->Components.Add(comp->Name, ccomp);
					}
				}
			}
			// check for unassigned arguments
			for (auto & comp : shader->Components)
			{
				if (comp.Value->Implementations.First()->SyntaxNode->IsParam &&
					!pRefMap.ContainsKey(comp.Key))
				{
					StringBuilder errMsg;
					errMsg << L"argument '" + comp.Key + L"' is unassigned.";
					// try to provide more info on why it is unassigned
					if (auto arg = rs->FindComponent(comp.Key, true))
						errMsg << L" automatic argument filling failed because the component of the same name is not accessible from '" << shader->SyntaxNode->Name.Content << L"'.";
					else
						errMsg << L" automatic argument filling failed because shader '" << shader->SyntaxNode->Name.Content << L"' does not define component '" + comp.Key + L"'.";
					err->Error(33023,errMsg.ProduceString(), rs->UsingPosition);
				}
			}
			return rs;
		}

		RefPtr<ShaderClosure> CreateShaderClosure(ErrorWriter * err, SymbolTable * symTable, ShaderSymbol * shader)
		{
			return CreateShaderClosure(err, symTable, shader, shader->SyntaxNode->Position, Dictionary<String, RefPtr<ShaderComponentSymbol>>());
		}


		class ResolveDependencyVisitor : public SyntaxVisitor
		{
		private:
			ShaderClosure * shaderClosure = nullptr;
			ShaderComponentSymbol * currentComponent = nullptr;
			void AddReference(ShaderComponentSymbol * referee, CodePosition pos)
			{
				referee->UserComponents.Add(currentComponent);
				currentComponent->DependentComponents.Add(referee);
				currentImpl->DependentComponents.Add(referee);
				currentImpl->ComponentReferencePositions[referee] = pos;
			}
		public:
			ShaderComponentImplSymbol * currentImpl = nullptr;
			ResolveDependencyVisitor(ErrorWriter * err, ShaderClosure * closure, ShaderComponentSymbol * comp)
				: SyntaxVisitor(err), shaderClosure(closure), currentComponent(comp)
			{}

			void VisitVarExpression(VarExpressionSyntaxNode * var) override
			{
				VariableEntry varEntry;
				if (!var->Scope->FindVariable(var->Variable, varEntry))
				{
					if (auto comp = shaderClosure->FindComponent(var->Variable))
					{
						if (comp->Implementations.First()->SyntaxNode->IsParam)
							shaderClosure->RefMap.TryGetValue(var->Variable, comp);
						var->Tags[L"ComponentReference"] = comp;
						AddReference(comp.Ptr(), var->Position);
					}
					else if (auto closure = shaderClosure->FindClosure(var->Variable))
					{
						var->Type.ShaderClosure = closure.Ptr();
					}
				}
			}

			void VisitMemberExpression(MemberExpressionSyntaxNode * member) override
			{
				member->BaseExpression->Accept(this);
				if (member->BaseExpression->Type.ShaderClosure)
				{
					if (auto comp = member->BaseExpression->Type.ShaderClosure->FindComponent(member->MemberName))
					{
						member->Tags[L"ComponentReference"] = comp;
						AddReference(comp.Ptr(), member->Position);
					}
					else if (auto shader = member->BaseExpression->Type.ShaderClosure->FindClosure(member->MemberName))
						member->Type.ShaderClosure = shader.Ptr();
				}
			}
		};

		void ResolveReference(ErrorWriter * err, ShaderClosure* shader)
		{
			for (auto & comp : shader->Components)
			{
				ResolveDependencyVisitor depVisitor(err, shader, comp.Value.Ptr());
				for (auto & impl : comp.Value->Implementations)
				{
					depVisitor.currentImpl = impl.Ptr();
					impl->SyntaxNode->Accept(&depVisitor);
				}
			}
			for (auto & subClosure : shader->SubClosures)
				ResolveReference(err, subClosure.Value.Ptr());
		}

		String GetUniqueCodeName(String name)
		{
			StringBuilder sb;
			for (auto ch : name)
			{
				if (ch == L'.')
					sb << L"I_I";
				else
					sb << ch;
			}
			return sb.ProduceString();
		}

		bool IsInAbstractWorld(PipelineSymbol * pipeline, ShaderComponentSymbol* comp)
		{
			return comp->Implementations.First()->Worlds.Count() &&
				pipeline->IsAbstractWorld(comp->Implementations.First()->Worlds.First());
		}

		void AssignUniqueNames(ShaderClosure * shader, String namePrefix, String publicNamePrefix)
		{
			for (auto & comp : shader->Components)
			{
				if (IsInAbstractWorld(shader->Pipeline, comp.Value.Ptr()))
				{
					comp.Value->UniqueName = comp.Value->Name;
				}
				else
				{
					String uniqueChoiceName;
					if (comp.Value->Implementations.First()->SyntaxNode->IsPublic)
						uniqueChoiceName = publicNamePrefix + comp.Key;
					else
						uniqueChoiceName = namePrefix + comp.Key;
					comp.Value->ChoiceNames.Add(uniqueChoiceName);
					comp.Value->UniqueKey = uniqueChoiceName;
					comp.Value->UniqueName = GetUniqueCodeName(uniqueChoiceName);
				}
			}
			for (auto & subClosure : shader->SubClosures)
			{
				if (subClosure.Value->IsInPlace)
					AssignUniqueNames(subClosure.Value.Ptr(), namePrefix + subClosure.Value->Name + L".", publicNamePrefix);
				else
					AssignUniqueNames(subClosure.Value.Ptr(), namePrefix + subClosure.Key + L".", publicNamePrefix + subClosure.Key + L".");
			}
		}

		bool IsConsistentGlobalComponentDefinition(ShaderComponentSymbol * comp0, ShaderComponentSymbol * comp1)
		{
			if (comp0->Type->DataType != comp1->Type->DataType)
				return false;
			if (comp0->Implementations.First()->Worlds.Count() != comp1->Implementations.First()->Worlds.Count())
				return false;
			for (auto w : comp0->Implementations.First()->Worlds)
				if (!comp1->Implementations.First()->Worlds.Contains(w))
					return false;
			return true;
		}

		void GatherComponents(ErrorWriter * err, ShaderClosure * closure, ShaderClosure * subClosure)
		{
			for (auto & comp : subClosure->Components)
			{
				ShaderComponentSymbol* existingComp = nullptr;
				if (closure->AllComponents.TryGetValue(comp.Value->UniqueName, existingComp))
				{
					if (IsInAbstractWorld(closure->Pipeline, comp.Value.Ptr()) &&
						IsInAbstractWorld(closure->Pipeline, existingComp))
					{
						// silently ignore consistently defined global components (components in abstract worlds)
						if (!IsConsistentGlobalComponentDefinition(comp.Value.Ptr(), existingComp))
						{
							err->Error(34025, L"'" + existingComp->Name + L"': global component conflicts with previous declaration.see previous declaration at " + existingComp->Implementations.First()->SyntaxNode->Position.ToString(),
								comp.Value->Implementations.First()->SyntaxNode->Position);
						}
					}
					else
					{
						StringBuilder errBuilder;
						errBuilder << L"component named '" << comp.Value->UniqueKey << L"\' is already defined when compiling '" << closure->Name << L"'.";
						auto currentClosure = subClosure;
						while (currentClosure != nullptr && currentClosure != closure)
						{
							errBuilder << L"\nsee inclusion of '" << currentClosure->Name << L"' at " << currentClosure->UsingPosition.ToString() << L".";
							currentClosure = currentClosure->Parent;
						}
						err->Error(34024, errBuilder.ProduceString(), comp.Value->Implementations.First()->SyntaxNode->Position);
					}
				}
				closure->AllComponents.AddIfNotExists(comp.Value->UniqueName, comp.Value.Ptr());
			}
			for (auto & sc : subClosure->SubClosures)
				GatherComponents(err, closure, sc.Value.Ptr());
		}

		bool IsWorldFeasible(PipelineSymbol * pipeline, ShaderComponentImplSymbol * impl, String world, ShaderComponentSymbol*& unaccessibleComp)
		{
			bool isWFeasible = true;
			for (auto & dcomp : impl->DependentComponents)
			{
				bool reachable = false;
				for (auto & dw : dcomp->Type->FeasibleWorlds)
				{
					if (pipeline->IsWorldReachable(dw, world))
					{
						reachable = true;
						break;
					}
				}
				if (!reachable)
				{
					unaccessibleComp = dcomp;
					isWFeasible = false;
					break;
				}
			}
			return isWFeasible;
		}

		void SolveWorldConstraints(ErrorWriter * err, ShaderClosure * shader)
		{
			EnumerableHashSet<String> allWorlds;
			for (auto w : shader->Pipeline->Worlds)
				if (!shader->Pipeline->IsAbstractWorld(w.Key))
					allWorlds.Add(w.Key);
			auto depOrder = shader->GetDependencyOrder();
			for (auto & comp : depOrder)
			{
				Dictionary<String, EnumerableHashSet<String>> autoWorlds;
				comp->Type->FeasibleWorlds.Clear();
				for (auto & impl : comp->Implementations)
				{
					if (!autoWorlds.ContainsKey(impl->AlternateName))
						autoWorlds[impl->AlternateName] = allWorlds;
					auto & autoWorld = autoWorlds[impl->AlternateName]();
					for (auto & w : impl->Worlds)
					{
						ShaderComponentSymbol* unaccessibleComp = nullptr;
						if (!IsWorldFeasible(shader->Pipeline, impl.Ptr(), w, unaccessibleComp))
						{
							err->Error(33100, L"'" + comp->Name + L"' cannot be computed at '" + w + L"' because the dependent component '" + unaccessibleComp->Name + L"' is not accessible.\nsee definition of '"
								+ unaccessibleComp->Name + L"' at " + unaccessibleComp->Implementations.First()->SyntaxNode->Position.ToString(),
								impl->ComponentReferencePositions[unaccessibleComp]());
						}
						autoWorld.Remove(w);
					}
				}
				for (auto & impl : comp->Implementations)
				{
					if (impl->Worlds.Count() == 0)
					{
						EnumerableHashSet<String> deducedWorlds = autoWorlds[impl->AlternateName]();
						EnumerableHashSet<String> feasibleWorlds;
						for (auto & w : deducedWorlds)
						{
							ShaderComponentSymbol* unaccessibleComp = nullptr;
							bool isWFeasible = IsWorldFeasible(shader->Pipeline, impl.Ptr(), w, unaccessibleComp);
							if (isWFeasible)
								feasibleWorlds.Add(w);
						}
						impl->Worlds = feasibleWorlds;
					}
					for (auto & w : impl->Worlds)
						comp->Type->FeasibleWorlds.Add(w);
				}
			}
			for (auto & comp : depOrder)
			{
				comp->Type->ConstrainedWorlds = comp->Type->FeasibleWorlds;
			}
			auto useInWorld = [&](String comp, String world)
			{
				// comp is used in world, restrict comp.ContainedWorlds to guarantee
				// all candidate definitions can reach world
				RefPtr<ShaderComponentSymbol> compSym;
				if (shader->Components.TryGetValue(comp, compSym))
				{
					EnumerableHashSet<String> newWorlds;
					for (auto & w : compSym->Type->ConstrainedWorlds)
						if (shader->Pipeline->IsWorldReachable(w, world))
							newWorlds.Add(w);
					compSym->Type->ConstrainedWorlds = _Move(newWorlds);
				}
			};
			for (auto impOp : shader->Pipeline->SyntaxNode->ImportOperators)
			{
				for (auto comp : impOp->Usings)
				{
					useInWorld(comp.Content, impOp->DestWorld.Content);
				}
			}
			for (auto & userWorld : shader->Pipeline->Worlds)
			{
				for (auto comp : userWorld.Value.SyntaxNode->Usings)
				{
					useInWorld(comp.Content, userWorld.Key);
				}
			}
		}

		bool CheckCircularReference(ErrorWriter * err, ShaderClosure * shader)
		{
			bool rs = false;
			for (auto & comp : shader->Components)
			{
				for (auto & impl : comp.Value->Implementations)
				{
					// check circular references
					HashSet<ShaderComponentSymbol*> set;
					List<ShaderComponentSymbol*> referredComponents;
					referredComponents.Add(comp.Value.Ptr());
					for (int i = 0; i < referredComponents.Count(); i++)
					{
						auto xcomp = referredComponents[i];
						for (auto & xcompImpl : xcomp->Implementations)
						{
							for (auto & rcomp : xcompImpl->DependentComponents)
							{
								if (set.Add(rcomp))
								{
									referredComponents.Add(rcomp);
								}
								if (rcomp == comp.Value.Ptr())
								{
									err->Error(32013, L"circular reference is not allowed.", impl->SyntaxNode->Position);
									rs = true;
								}
							}
						}
					}
				}
			}
			return rs;
		}

		void PropagateArgumentConstraints(ShaderComponentSymbol * requirement, ShaderComponentSymbol * arg)
		{
			for (auto w : requirement->Implementations.First()->ExportWorlds)
			{
				for (auto impl : arg->Implementations)
				{
					if (impl->Worlds.Contains(w))
						impl->ExportWorlds.Add(w);
				}
			}
			for (auto w : requirement->Implementations.First()->SrcPinnedWorlds)
			{
				for (auto impl : arg->Implementations)
				{
					if (impl->Worlds.Contains(w))
						impl->SrcPinnedWorlds.Add(w);
				}
			}
		}

		void VerifyAndPropagateArgumentConstraints(ErrorWriter * err, ShaderClosure * shader)
		{
			for (auto & map : shader->RefMap)
			{
				auto & arg = map.Value;
				RefPtr<ShaderComponentSymbol> requirement;
				if (shader->Components.TryGetValue(map.Key, requirement) && requirement->IsParam())
				{
					if (requirement->Implementations.First()->SyntaxNode->Rate)
					{
						for (auto w : requirement->Implementations.First()->Worlds)
						{
							if (!shader->Pipeline->IsWorldReachable(arg->Type->FeasibleWorlds, w))
							{
								err->Error(32015, L"argument '" + arg->Name + L"' is not available in world '" + w + L"' as required by '" + shader->Name
									+ L"'.\nsee requirement declaration at " +
									requirement->Implementations.First()->SyntaxNode->Position.ToString(), arg->Implementations.First()->SyntaxNode->Position);
							}
						}
						PropagateArgumentConstraints(requirement.Ptr(), arg.Ptr());
					}
				}
			}
			for (auto & subClosure : shader->SubClosures)
				VerifyAndPropagateArgumentConstraints(err, subClosure.Value.Ptr());
		}

		void FlattenShaderClosure(ErrorWriter * err, ShaderClosure * shader)
		{
			ResolveReference(err, shader);
			// assign choice names
			AssignUniqueNames(shader, L"", L"");
			// traverse closures to get component list
			GatherComponents(err, shader, shader);
			// propagate world constraints
			if (CheckCircularReference(err, shader))
				return;
			SolveWorldConstraints(err, shader);
			// check pipeline constraints
			for (auto & requirement : shader->Pipeline->Components)
			{
				auto comp = shader->FindComponent(requirement.Key);
				if (!comp)
				{
					err->Error(32014, L"shader '" + shader->Name + L"' does not provide '" + requirement.Key + L"' as required by '" + shader->Pipeline->SyntaxNode->Name.Content
						+ L"'.\nsee requirement declaration at " +
						requirement.Value->Implementations.First()->SyntaxNode->Position.ToString(), shader->Position);
				}
				else
				{
					for (auto & impl : requirement.Value->Implementations)
					{
						for (auto w : impl->Worlds)
						{
							if (!shader->Pipeline->IsWorldReachable(comp->Type->FeasibleWorlds, w))
							{
								err->Error(32015, L"component '" + comp->Name + L"' is not available in world '" + w + L"' as required by '" + shader->Pipeline->SyntaxNode->Name.Content
									+ L"'.\nsee requirement declaration at " +
									requirement.Value->Implementations.First()->SyntaxNode->Position.ToString(), comp->Implementations.First()->SyntaxNode->Position);
							}
						}
					}
					PropagateArgumentConstraints(requirement.Value.Ptr(), comp.Ptr());
				}
			}
			// check argument constraints
			VerifyAndPropagateArgumentConstraints(err, shader);
		}
	}
}

/***********************************************************************
SPIRECORE\CODEGENERATOR.CPP
***********************************************************************/
#include <assert.h>

namespace Spire
{
	namespace Compiler
	{
		class CodeGenerator : public ICodeGenerator
		{
		private:
			ShaderCompiler * compiler;
			SymbolTable * symTable;
			CompiledWorld * currentWorld = nullptr;
			ShaderComponentSymbol * currentComponent = nullptr;
			ShaderComponentImplSymbol * currentComponentImpl = nullptr;
			ShaderClosure * currentShader = nullptr;
			CompileResult & result;
			List<ILOperand*> exprStack;
			CodeWriter codeWriter;
			ScopeDictionary<String, ILOperand*> variables;
			ILType * TranslateExpressionType(const ExpressionType & type)
			{
				ILType * resultType = 0;
				auto base = new ILBasicType();
				base->Type = (ILBaseType)type.BaseType;
				resultType = base;
				if (type.BaseType == BaseType::Bool)
				{
					base->Type = ILBaseType::Int;
				}

				if (type.IsArray)
				{
					ILArrayType * arrType = dynamic_cast<ILArrayType*>(resultType);
					if (resultType)
					{
						arrType->ArrayLength *= type.ArrayLength;
					}
					else
					{
						auto nArrType = new ILArrayType();
						nArrType->BaseType = resultType;
						nArrType->ArrayLength = type.ArrayLength;
						resultType = nArrType;
					}
				}
				return resultType;
			}
			void PushStack(ILOperand * op)
			{
				exprStack.Add(op);
			}
			ILOperand * PopStack()
			{
				auto rs = exprStack.Last();
				exprStack.SetSize(exprStack.Count() - 1);
				return rs;
			}
			AllocVarInstruction * AllocVar(const ExpressionType & etype)
			{
				AllocVarInstruction * varOp = 0;
				RefPtr<ILType> type = TranslateExpressionType(etype);
				auto arrType = dynamic_cast<ILArrayType*>(type.Ptr());

				if (arrType)
				{
					varOp = codeWriter.AllocVar(arrType->BaseType, result.Program->ConstantPool->CreateConstant(arrType->ArrayLength));
				}
				else
				{
					assert(type);
					varOp = codeWriter.AllocVar(type, result.Program->ConstantPool->CreateConstant(0));
				}
				return varOp;
			}
			FetchArgInstruction * FetchArg(const ExpressionType & etype, int argId)
			{
				auto type = TranslateExpressionType(etype);
				auto arrType = dynamic_cast<ILArrayType*>(type);
				FetchArgInstruction * varOp = 0;
				if (arrType)
				{
					auto baseType = arrType->BaseType.Release();
					varOp = codeWriter.FetchArg(baseType, argId);
					delete type;
				}
				else
				{
					varOp = codeWriter.FetchArg(type, argId);
				}
				return varOp;
			}
			void SortInterfaceBlock(InterfaceBlock * block)
			{
				List<KeyValuePair<String, ComponentDefinition>> entries;
				for (auto & kv : block->Entries)
					entries.Add(kv);
				entries.Sort([](const auto & v0, const auto & v1) {return v0.Value.OrderingStr < v1.Value.OrderingStr; });
				block->Entries.Clear();
				for (auto & kv : entries)
					block->Entries.Add(kv.Key, kv.Value);
			}
		public:
			virtual void VisitProgram(ProgramSyntaxNode *) override
			{
			}
			virtual void ProcessFunction(FunctionSyntaxNode * func) override
			{
				VisitFunction(func);
			}
			virtual void ProcessShader(ShaderClosure * shader) override
			{
				currentShader = shader;
				RefPtr<CompiledShader> compiledShader = new CompiledShader();
				compiledShader->MetaData.ShaderName = shader->Name;
				result.Program->Shaders.Add(compiledShader);
				for (auto & world : shader->Pipeline->Worlds)
				{
					auto w = new CompiledWorld();
					w->ExportOperator = world.Value.SyntaxNode->ExportOperator;
					auto outputBlock = new InterfaceBlock();
					outputBlock->Name = world.Key;
					world.Value.SyntaxNode->LayoutAttributes.TryGetValue(L"InterfaceBlock", outputBlock->Name);
					if (outputBlock->Name.Contains(L":"))
					{
						CoreLib::Text::Parser parser(outputBlock->Name);
						auto blockName = parser.ReadWord();
						parser.Read(L":");
						auto indx = parser.ReadInt();
						outputBlock->Name = blockName;
						outputBlock->Attributes[L"Index"] = String(indx);
					}
					String strIdx;
					if (world.Value.SyntaxNode->LayoutAttributes.TryGetValue(L"InterfaceBlockIndex", strIdx))
						outputBlock->Attributes[L"Index"] = strIdx;
					if (world.Value.SyntaxNode->LayoutAttributes.ContainsKey(L"Packed"))
						outputBlock->Attributes[L"Packed"] = L"1";
					w->WorldOutput = outputBlock;
					compiledShader->InterfaceBlocks[outputBlock->Name] = outputBlock;
					w->Attributes = world.Value.SyntaxNode->LayoutAttributes;
					w->Shader = compiledShader.Ptr();
					w->ShaderName = shader->Name;
					w->WorldName = world.Key;
					w->IsAbstract = world.Value.IsAbstract;
					auto impOps = shader->Pipeline->GetImportOperatorsFromSourceWorld(world.Key);

					w->TargetMachine = world.Value.SyntaxNode->TargetMachine;
					CoreLib::Text::Parser parser(w->TargetMachine);
					try
					{
						if (!parser.IsEnd())
						{
							w->TargetMachine = parser.ReadWord();
							if (parser.LookAhead(L"("))
							{
								parser.Read(L"(");
								while (!parser.LookAhead(L")"))
								{
									auto param = parser.ReadWord();
									if (parser.LookAhead(L":"))
									{
										parser.Read(L":");
										auto value = parser.ReadWord();
										w->BackendParameters[param] = value;
									}
									else
										w->BackendParameters[param] = L"";
									if (parser.LookAhead(L";"))
									{
										parser.Read(L";");
									}
									else
										break;
								}
								parser.Read(L")");
							}
						}
					}
					catch (const CoreLib::Text::TextFormatException & ex)
					{
						result.GetErrorWriter()->Error(34031, L"invalid target machine syntax. \n" + ex.Message, world.Value.SyntaxNode->Position);
					}
					w->WorldDefPosition = world.Value.SyntaxNode->Position;
					compiledShader->Worlds[world.Key] = w;
				}

				Dictionary<String, List<ComponentDefinitionIR*>> worldComps;
				struct ThroughVar
				{
					bool Export = false;
					String InputName, OutputName;
					String InputWorldName;
					ImportOperatorDefSyntaxNode * ImportOperator;
					ComponentDefinitionIR * Component;
					int GetHashCode()
					{
						return PointerHash<1>().GetHashCode(Component);
					}
					bool operator == (const ThroughVar & other)
					{
						return Component == other.Component;
					}
				};
				Dictionary<String, EnumerableHashSet<ThroughVar>> worldThroughVars;
				for (auto & world : shader->Pipeline->Worlds)
				{
					List<ComponentDefinitionIR*> components;
					for (auto & compDef : shader->IR->Definitions)
						if (compDef->World == world.Key)
							components.Add(compDef.Ptr());

					auto & compiledWorld = compiledShader->Worlds[world.Key].GetValue();
					DependencySort(components, [](ComponentDefinitionIR * def)
					{
						return def->Dependency;
					});
					worldComps[world.Key] = components;
					worldThroughVars[world.Key] = EnumerableHashSet<ThroughVar>();

					if (world.Value.SyntaxNode->LayoutAttributes.ContainsKey(L"Pinned"))
					{
						for (auto & comp : components)
						{
							ComponentDefinition compDef;
							compDef.LayoutAttribs = comp->Implementation->SyntaxNode->LayoutAttributes;
							compDef.Name = comp->Component->UniqueName;
							compDef.Type = TranslateExpressionType(comp->Component->Type->DataType);
							compDef.OrderingStr = comp->Implementation->SyntaxNode->Position.FileName +
								String(comp->Implementation->SyntaxNode->Position.Line).PadLeft(L' ', 8) + compDef.Name;
							compiledWorld->WorldOutput->Entries.AddIfNotExists(compDef.Name, compDef);
						}
					}
				}
				for (auto & world : shader->Pipeline->Worlds)
				{
					auto compiledWorld = compiledShader->Worlds[world.Key].GetValue();
					for (auto & impOp : shader->Pipeline->SyntaxNode->ImportOperators)
					{
						if (impOp->DestWorld.Content == world.Key)
						{
							InputInterface input;
							input.Block = compiledShader->Worlds[impOp->SourceWorld.Content].GetValue()->WorldOutput;
							input.ImportOperator = *impOp;
							compiledWorld->WorldInputs[impOp->SourceWorld.Content] = input;
						}
					}
					auto & components = worldComps[world.Key].GetValue();
					
					for (auto & comp : components)
					{
						auto srcWorld = world.Key;
						EnumerableHashSet<String> outputWorlds;
						if (comp->Implementation->ExportWorlds.Contains(srcWorld))
							outputWorlds.Add(srcWorld);
						struct ImportTechnique
						{
							String SourceWorld;
							ImportOperatorDefSyntaxNode * ImportOperator;
						};
						EnumerableDictionary<String, ImportTechnique> inputSourceWorlds;
						auto useInWorld = [&](String userWorld)
						{
							if (userWorld == srcWorld)
								return;
							auto path = currentShader->Pipeline->FindImportOperatorChain(srcWorld, userWorld);
							if (path.Count() == 0)
								throw InvalidProgramException(L"no import exists, this should have been checked by semantics analyzer.");
							for (int i = 0; i < path[0].Nodes.Count(); i++)
							{
								auto & node = path[0].Nodes[i];
								if (node.TargetWorld != userWorld)
								{
									// should define output in node.TargetWorld
									outputWorlds.Add(node.TargetWorld);
								}
								if (node.TargetWorld != srcWorld)
								{
									// should define input in node.TargetWorld
									ImportTechnique tech;
									tech.SourceWorld = path[0].Nodes[i - 1].TargetWorld;
									tech.ImportOperator = path[0].Nodes[i].ImportOperator;
									inputSourceWorlds[node.TargetWorld] = tech;
								}
							}
						};
						for (auto user : comp->Users)
						{
							if (user->World != srcWorld)
							{
								useInWorld(user->World);
							}
						}

						ShaderComponentImplSymbol * compImpl = comp->Implementation;
						
						// define outputs in all involved worlds
						for (auto & outputWorld : outputWorlds)
						{
							auto & w = compiledShader->Worlds[outputWorld].GetValue();
							
							ComponentDefinition compDef;
							compDef.Name = comp->Component->UniqueName;
							compDef.OrderingStr = compImpl->SyntaxNode->Position.FileName +
								String(compImpl->SyntaxNode->Position.Line).PadLeft(L' ', 8) + compDef.Name;
							compDef.Type = TranslateExpressionType(comp->Component->Type->DataType);
							compDef.LayoutAttribs = compImpl->SyntaxNode->LayoutAttributes;
							w->WorldOutput->Entries.AddIfNotExists(compDef.Name, compDef);
							// if an input is also defined in this world (i.e. this is a through world), insert assignment
							ImportTechnique importTech;
							if (inputSourceWorlds.TryGetValue(outputWorld, importTech))
							{
								ThroughVar tvar;
								tvar.ImportOperator = importTech.ImportOperator;
								tvar.Export = true;
								tvar.InputName = comp->Component->UniqueName;
								tvar.Component = comp;
								tvar.OutputName = w->WorldOutput->Name + L"." + comp->Component->UniqueName;
								tvar.InputWorldName = importTech.SourceWorld;
								worldThroughVars[outputWorld].GetValue().Add(tvar);
							}
						}
						// define inputs
						for (auto & input : inputSourceWorlds)
						{
							ThroughVar tvar;
							tvar.ImportOperator = input.Value.ImportOperator;
							tvar.Export = false;
							tvar.InputName = comp->Component->UniqueName;
							tvar.Component = comp;
							tvar.InputWorldName = input.Value.SourceWorld;
							worldThroughVars[input.Key].GetValue().Add(tvar);
						}
					}
				}
				for (auto & world : shader->Pipeline->Worlds)
				{
					if (world.Value.IsAbstract)
						continue;
					NamingCounter = 0;

					auto & components = worldComps[world.Key].GetValue();
					auto compiledWorld = compiledShader->Worlds[world.Key].GetValue().Ptr();
					currentWorld = compiledWorld;
					codeWriter.PushNode();
					variables.PushScope();
					HashSet<String> localComponents;
					for (auto & comp : components)
						localComponents.Add(comp->Component->UniqueName);

					DependencySort(components, [](ComponentDefinitionIR * def)
					{
						return def->Dependency;
					});

					auto generateImportInstr = [&](ComponentDefinitionIR * comp, ComponentDefinitionIR * user)
					{
						ImportInstruction * instr = nullptr;
						if (!compiledWorld->ImportInstructions.TryGetValue(comp->Component->UniqueName, instr))
						{
							auto path = shader->Pipeline->FindImportOperatorChain(comp->World, world.Key);
							auto importOp = path[0].Nodes.Last().ImportOperator;
							auto sourceWorld = compiledShader->Worlds[path[0].Nodes[path[0].Nodes.Count() - 2].TargetWorld].GetValue().Ptr();
							instr = new ImportInstruction(importOp->Usings.Count(), comp->Component->UniqueName, importOp,
								sourceWorld, TranslateExpressionType(comp->Component->Type->DataType));
							for (int i = 0; i < importOp->Usings.Count(); i++)
							{
								// resolve import operator arguments
								ILOperand * val = nullptr;
								if (!variables.TryGetValue(importOp->Usings[i].Content, val))
								{
									ImportInstruction * impInstr = nullptr;
									compiledWorld->ImportInstructions.TryGetValue(importOp->Usings[i].Content, impInstr);
									val = impInstr;
								}
								String userCompName;
								if (user)
									userCompName = user->Component->Name;
								else
									userCompName = comp->Component->Name;
								if (!val)
									result.GetErrorWriter()->Error(50010, L"\'" + importOp->Usings[i].Content + L"\': implicit import operator argument is not accessible when attempting to import \'"
										+ comp->Component->Name + L"\' from world \'" + sourceWorld->WorldName + L"\' when compiling \'" + userCompName + L"\' for world \'" + compiledWorld->WorldName + L"\'.\nsee import operator declaration at " +
										importOp->Position.ToString() + L".", comp->Implementation->SyntaxNode->Position);
								else
									instr->Arguments[i] = val;
							}
							sourceWorld->WorldOutput->UserWorlds.Add(world.Key);
							instr->Name = L"_vout" + comp->Component->UniqueName;
							codeWriter.Insert(instr);
							compiledWorld->ImportInstructions[comp->Component->UniqueName] = instr;
							CompiledComponent ccomp;
							ccomp.CodeOperand = instr;
							ccomp.Attributes = comp->Implementation->SyntaxNode->LayoutAttributes;
							compiledWorld->LocalComponents[comp->Component->UniqueName] = ccomp;
						}
						return instr;
					};
					HashSet<String> thisWorldComponents;
					for (auto & comp : components)
					{
						thisWorldComponents.Add(comp->Component->UniqueName);
					}
					auto & throughVars = worldThroughVars[world.Key].GetValue();
					auto genInputVar = [&]()
					{
						for (auto & throughVar : throughVars)
						{
							bool shouldSkip = false;
							for (auto & depComp : throughVar.ImportOperator->Usings)
							{
								if (thisWorldComponents.Contains(depComp.Content))
								{
									shouldSkip = true;
									break;
								}
							}
							if (shouldSkip)
								continue;
							auto srcInstr = generateImportInstr(throughVar.Component, nullptr);
							if (throughVar.Export)
							{
								auto exp = new ExportInstruction(throughVar.Component->Component->UniqueName, compiledWorld->ExportOperator.Content, compiledWorld, srcInstr);
								codeWriter.Insert(exp);
								throughVars.Remove(throughVar);
							}
						}
					};
					genInputVar();
					for (auto & comp : components)
					{
						genInputVar();
						for (auto & dep : comp->Dependency)
						{
							if (dep->World != world.Key)
							{
								generateImportInstr(dep, comp);
							}
						}
						thisWorldComponents.Remove(comp->Component->UniqueName);
						VisitComponent(comp);
					}
					
					variables.PopScope();
					compiledWorld->Code = codeWriter.PopNode();
					EvalReferencedFunctionClosure(compiledWorld);
					currentWorld = nullptr;

					// fill in meta data
					WorldMetaData wdata;
					for (auto & comp : components)
						wdata.Components.Add(comp->Component->UniqueName);
					wdata.Name = compiledWorld->WorldName;
					wdata.TargetName = compiledWorld->TargetMachine;
					wdata.OutputBlock = compiledWorld->WorldOutput->Name;
					for (auto & inputBlock : compiledWorld->WorldInputs)
						wdata.InputBlocks.Add(inputBlock.Value.Block->Name);
					compiledWorld->Shader->MetaData.Worlds.Add(wdata.Name, wdata);
				}
				for (auto & block : compiledShader->InterfaceBlocks)
				{
					SortInterfaceBlock(block.Value.Ptr());
					InterfaceBlockMetaData blockMeta;
					blockMeta.Name = block.Value->Name;
					blockMeta.Attributes = block.Value->Attributes;
					int offset = 0;
					bool pack = block.Value->Attributes.ContainsKey(L"Packed");
					for (auto & entry : block.Value->Entries)
					{
						InterfaceBlockEntry ment;
						ment.Type = dynamic_cast<ILBasicType*>(entry.Value.Type.Ptr())->Type;
						ment.Name = entry.Key;
						ment.Attributes = entry.Value.LayoutAttribs;
						if (!pack)
							offset = RoundToAlignment(offset, AlignmentOfBaseType(ment.Type));
						ment.Offset = offset;
						entry.Value.Offset = offset;
						ment.Size = SizeofBaseType(ment.Type);
						offset += ment.Size;
						blockMeta.Entries.Add(ment);
					}
					block.Value->Size = offset;
					if (!pack && block.Value->Entries.Count() > 0)
						block.Value->Size = RoundToAlignment(offset, AlignmentOfBaseType(dynamic_cast<ILBasicType*>((*(block.Value->Entries.begin())).Value.Type.Ptr())->Type));
					blockMeta.Size = block.Value->Size;
					compiledShader->MetaData.InterfaceBlocks[blockMeta.Name] = blockMeta;
				}
				currentShader = nullptr;
			}
			void EvalReferencedFunctionClosure(CompiledWorld * world)
			{
				List<String> workList;
				for (auto & rfunc : world->ReferencedFunctions)
					workList.Add(rfunc);
				for (int i = 0; i < workList.Count(); i++)
				{
					auto rfunc = workList[i];
					RefPtr<FunctionSymbol> funcSym;
					if (symTable->Functions.TryGetValue(rfunc, funcSym))
					{
						for (auto & rrfunc : funcSym->ReferencedFunctions)
						{
							world->ReferencedFunctions.Add(rrfunc);
							workList.Add(rrfunc);
						}
					}
				}
			}
			virtual void VisitComponent(ComponentSyntaxNode *)
			{
				throw NotImplementedException();
			}
			virtual void VisitComponent(ComponentDefinitionIR * comp)
			{
				currentComponent = comp->Component;
				currentComponentImpl = comp->Implementation;
				String varName = L"_vcmp" + currentComponent->UniqueName;

				RefPtr<ILType> type = TranslateExpressionType(currentComponent->Type->DataType);
				auto allocVar = codeWriter.AllocVar(type, result.Program->ConstantPool->CreateConstant(1));
				allocVar->Name = varName;
				variables.Add(currentComponent->UniqueName, allocVar);
				CompiledComponent ccomp;
				ccomp.CodeOperand = allocVar;
				ccomp.Attributes = comp->Implementation->SyntaxNode->LayoutAttributes;
				currentWorld->LocalComponents[currentComponent->UniqueName] = ccomp;
				if (currentComponentImpl->SyntaxNode->Expression)
				{
					currentComponentImpl->SyntaxNode->Expression->Accept(this);
					Assign(currentComponentImpl->SyntaxNode->Type->ToExpressionType(), allocVar, exprStack.Last());
					if (currentWorld->WorldOutput->Entries.ContainsKey(currentComponent->UniqueName))
					{
						auto exp = new ExportInstruction(currentComponent->UniqueName, currentWorld->ExportOperator.Content, currentWorld,
							allocVar);
						codeWriter.Insert(exp);
					}
					exprStack.Clear();
				}
				else if (currentComponentImpl->SyntaxNode->BlockStatement)
				{
					currentComponentImpl->SyntaxNode->BlockStatement->Accept(this);
				}
				currentComponentImpl = nullptr;
				currentComponent = nullptr;
			}
			virtual void VisitFunction(FunctionSyntaxNode* function)
			{
				if (function->IsExtern)
					return;
				RefPtr<CompiledFunction> func = new CompiledFunction();
				result.Program->Functions.Add(func);
				func->Name = function->InternalName;
				func->ReturnType = TranslateExpressionType(function->ReturnType->ToExpressionType());
				variables.PushScope();
				codeWriter.PushNode();
				int id = 0;
				for (auto &param : function->Parameters)
				{
					func->Parameters.Add(param->Name, TranslateExpressionType(param->Type->ToExpressionType()));
					auto op = FetchArg(param->Type->ToExpressionType(), ++id);
					op->Name = String(L"p_") + param->Name;
					variables.Add(param->Name, op);
				}
				function->Body->Accept(this);
				func->Code = codeWriter.PopNode();
				variables.PopScope();
			}
			virtual void VisitBlockStatement(BlockStatementSyntaxNode* stmt)
			{
				variables.PushScope();
				for (auto & subStmt : stmt->Statements)
					subStmt->Accept(this);
				variables.PopScope();
			}
			virtual void VisitEmptyStatement(EmptyStatementSyntaxNode*){}
			virtual void VisitWhileStatement(WhileStatementSyntaxNode* stmt)
			{
				RefPtr<WhileInstruction> instr = new WhileInstruction();
				variables.PushScope();
				codeWriter.PushNode();
				stmt->Predicate->Accept(this);
				codeWriter.Insert(new ReturnInstruction(PopStack()));
				instr->ConditionCode = codeWriter.PopNode();
				codeWriter.PushNode();
				stmt->Statement->Accept(this);
				instr->BodyCode = codeWriter.PopNode();
				codeWriter.Insert(instr.Release());
				variables.PopScope();
			}
			virtual void VisitDoWhileStatement(DoWhileStatementSyntaxNode* stmt)
			{
				RefPtr<WhileInstruction> instr = new DoInstruction();
				variables.PushScope();
				codeWriter.PushNode();
				stmt->Predicate->Accept(this);
				codeWriter.Insert(new ReturnInstruction(PopStack()));
				instr->ConditionCode = codeWriter.PopNode();
				codeWriter.PushNode();
				stmt->Statement->Accept(this);
				instr->BodyCode = codeWriter.PopNode();
				codeWriter.Insert(instr.Release());
				variables.PopScope();
			}
			virtual void VisitForStatement(ForStatementSyntaxNode* stmt)
			{
				RefPtr<ForInstruction> instr = new ForInstruction();
				variables.PushScope();
				if (stmt->TypeDef)
				{
					AllocVarInstruction * varOp = AllocVar(stmt->TypeDef->ToExpressionType());
					varOp->Name = L"v_" + stmt->IterationVariable.Content;
					variables.Add(stmt->IterationVariable.Content, varOp);
				}
				ILOperand * iterVar = nullptr;
				if (!variables.TryGetValue(stmt->IterationVariable.Content, iterVar))
					throw InvalidProgramException(L"Iteration variable not found in variables dictionary. This should have been checked by semantics analyzer.");
				stmt->InitialExpression->Accept(this);
				Assign(stmt->TypeDef->ToExpressionType(), iterVar, PopStack());

				codeWriter.PushNode();
				stmt->EndExpression->Accept(this);
				auto val = PopStack();
				codeWriter.Insert(new CmpleInstruction(new LoadInstruction(iterVar), val));
				instr->ConditionCode = codeWriter.PopNode();

				codeWriter.PushNode();
				ILOperand * stepVal = nullptr;
				if (stmt->StepExpression)
				{
					stmt->StepExpression->Accept(this);
					stepVal = PopStack();
				}
				else
				{
					if (iterVar->Type->IsFloat())
						stepVal = result.Program->ConstantPool->CreateConstant(1.0f);
					else
						stepVal = result.Program->ConstantPool->CreateConstant(1);
				}
				auto afterVal = new AddInstruction(new LoadInstruction(iterVar), stepVal);
				codeWriter.Insert(afterVal);
				Assign(stmt->TypeDef->ToExpressionType(), iterVar, afterVal);
				instr->SideEffectCode = codeWriter.PopNode();

				codeWriter.PushNode();
				stmt->Statement->Accept(this);
				instr->BodyCode = codeWriter.PopNode();
				codeWriter.Insert(instr.Release());
				variables.PopScope();
			}
			virtual void VisitIfStatement(IfStatementSyntaxNode* stmt)
			{
				RefPtr<IfInstruction> instr = new IfInstruction();
				variables.PushScope();
				stmt->Predicate->Accept(this);
				instr->Operand = PopStack();
				codeWriter.PushNode();
				stmt->PositiveStatement->Accept(this);
				instr->TrueCode = codeWriter.PopNode();
				if (stmt->NegativeStatement)
				{
					codeWriter.PushNode();
					stmt->NegativeStatement->Accept(this);
					instr->FalseCode = codeWriter.PopNode();
				}
				codeWriter.Insert(instr.Release());
				variables.PopScope();
			}
			virtual void VisitReturnStatement(ReturnStatementSyntaxNode* stmt)
			{
				if (currentComponentImpl != nullptr)
				{
					if (stmt->Expression)
					{
						stmt->Expression->Accept(this);
						ILOperand *op = nullptr;
						variables.TryGetValue(currentComponent->UniqueName, op);
						auto val = PopStack();
						codeWriter.Store(op, val);
						if (currentWorld->WorldOutput->Entries.ContainsKey(currentComponent->UniqueName))
						{
							auto exp = new ExportInstruction(currentComponent->UniqueName, currentWorld->ExportOperator.Content, currentWorld, op);
							codeWriter.Insert(exp);
						}
					}
				}
				else
				{
					if (stmt->Expression)
						stmt->Expression->Accept(this);
					codeWriter.Insert(new ReturnInstruction(PopStack()));
				}
			}
			virtual void VisitBreakStatement(BreakStatementSyntaxNode*)
			{
				codeWriter.Insert(new BreakInstruction());
			}
			virtual void VisitContinueStatement(ContinueStatementSyntaxNode*)
			{
				codeWriter.Insert(new ContinueInstruction());
			}
			virtual void VisitSelectExpression(SelectExpressionSyntaxNode * expr)
			{
				expr->SelectorExpr->Accept(this);
				auto predOp = PopStack();
				expr->Expr0->Accept(this);
				auto v0 = PopStack();
				expr->Expr1->Accept(this);
				auto v1 = PopStack();
				codeWriter.Select(predOp, v0, v1);
			}
			ILOperand * EnsureBoolType(ILOperand * op, ExpressionType type)
			{
				if (type != ExpressionType::Bool)
				{
					auto cmpeq = new CmpneqInstruction();
					cmpeq->Operands[0] = op;
					cmpeq->Operands[1] = result.Program->ConstantPool->CreateConstant(0);
					cmpeq->Type = new ILBasicType(ILBaseType::Int);
					codeWriter.Insert(cmpeq);
					return cmpeq;
				}
				else
					return op;
			}
			
			virtual void VisitVarDeclrStatement(VarDeclrStatementSyntaxNode* stmt)
			{
				for (auto & v : stmt->Variables)
				{
					AllocVarInstruction * varOp = AllocVar(stmt->Type->ToExpressionType());
					varOp->Name = L"v" + String(NamingCounter++) + L"_" + v->Name;
					variables.Add(v->Name, varOp);
					if (v->Expression)
					{
						v->Expression->Accept(this);
						Assign(stmt->Type->ToExpressionType(), varOp, PopStack());
					}
				}
			}
			virtual void VisitExpressionStatement(ExpressionStatementSyntaxNode* stmt)
			{
				stmt->Expression->Accept(this);
				PopStack();
			}
			void Assign(const ExpressionType & /*type*/, ILOperand * left, ILOperand * right)
			{
				if (auto add = dynamic_cast<AddInstruction*>(left))
				{
					auto baseOp = add->Operands[0].Ptr();
					codeWriter.Store(add->Operands[0].Ptr(), codeWriter.Update(codeWriter.Load(baseOp), add->Operands[1].Ptr(), right));
					add->Erase();
				}
				else
					codeWriter.Store(left, right);
			}
			virtual void VisitBinaryExpression(BinaryExpressionSyntaxNode* expr)
			{
				expr->RightExpression->Accept(this);
				auto right = PopStack();
				if (expr->Operator == Operator::Assign)
				{
					expr->LeftExpression->Access = ExpressionAccess::Write;
					expr->LeftExpression->Accept(this);
					auto left = PopStack();
					Assign(expr->LeftExpression->Type, left, right);
					PushStack(left);
				}
				else
				{
					expr->LeftExpression->Access = ExpressionAccess::Read;
					expr->LeftExpression->Accept(this);
					auto left = PopStack();
					BinaryInstruction * rs = 0;
					switch (expr->Operator)
					{
					case Operator::Add:
					case Operator::AddAssign:
						rs = new AddInstruction();
						break;
					case Operator::Sub:
					case Operator::SubAssign:
						rs = new SubInstruction();
						break;
					case Operator::Mul:
					case Operator::MulAssign:
						rs = new MulInstruction();
						break;
					case Operator::Mod:
					case Operator::ModAssign:
						rs = new ModInstruction();
						break;
					case Operator::Div:
					case Operator::DivAssign:
						rs = new DivInstruction();
						break;
					case Operator::And:
						rs = new AndInstruction();
						break;
					case Operator::Or:
						rs = new OrInstruction();
						break;
					case Operator::BitAnd:
						rs = new BitAndInstruction();
						break;
					case Operator::BitOr:
						rs = new BitOrInstruction();
						break;
					case Operator::BitXor:
						rs = new BitXorInstruction();
						break;
					case Operator::Lsh:
						rs = new ShlInstruction();
						break;
					case Operator::Rsh:
						rs = new ShrInstruction();
						break;
					case Operator::Eql:
						rs = new CmpeqlInstruction();
						break;
					case Operator::Neq:
						rs = new CmpneqInstruction();
						break;
					case Operator::Greater:
						rs = new CmpgtInstruction();
						break;
					case Operator::Geq:
						rs = new CmpgeInstruction();
						break;
					case Operator::Leq:
						rs = new CmpleInstruction();
						break;
					case Operator::Less:
						rs = new CmpltInstruction();
						break;
					default:
						throw NotImplementedException(L"Code gen not implemented for this operator.");
					}
					rs->Operands.SetSize(2);
					rs->Operands[0] = left;
					rs->Operands[1] = right;
					rs->Type = TranslateExpressionType(expr->Type);
					codeWriter.Insert(rs);
					switch (expr->Operator)
					{
					case Operator::AddAssign:
					case Operator::SubAssign:
					case Operator::MulAssign:
					case Operator::DivAssign:
					case Operator::ModAssign:
					{
						expr->LeftExpression->Access = ExpressionAccess::Write;
						expr->LeftExpression->Accept(this);
						auto target = PopStack();
						Assign(expr->Type, target, rs);
						break;
					}
					default:
						break;
					}
					PushStack(rs);
				}
			}
			virtual void VisitConstantExpression(ConstantExpressionSyntaxNode* expr)
			{
				ILConstOperand * op;
				if (expr->ConstType == ConstantExpressionSyntaxNode::ConstantType::Float)
				{
					op = result.Program->ConstantPool->CreateConstant(expr->FloatValue);
				}
				else
				{
					op = result.Program->ConstantPool->CreateConstant(expr->IntValue);
				}
				PushStack(op);
			}
			void GenerateIndexExpression(ILOperand * base, ILOperand * idx, bool read)
			{
				if (read)
				{
					auto ldInstr = codeWriter.Retrieve(base, idx);
					ldInstr->Attribute = base->Attribute;
					PushStack(ldInstr);
				}
				else
				{
					PushStack(codeWriter.Add(base, idx));
				}
			}
			virtual void VisitIndexExpression(IndexExpressionSyntaxNode* expr)
			{
				expr->BaseExpression->Access = expr->Access;
				expr->BaseExpression->Accept(this);
				auto base = PopStack();
				expr->IndexExpression->Access = ExpressionAccess::Read;
				expr->IndexExpression->Accept(this);
				auto idx = PopStack();
				GenerateIndexExpression(base, idx,
					expr->Access == ExpressionAccess::Read);
			}
			virtual void VisitMemberExpression(MemberExpressionSyntaxNode * expr)
			{
				RefPtr<Object> refObj;
				if (expr->Tags.TryGetValue(L"ComponentReference", refObj))
				{
					if (auto refComp = dynamic_cast<ShaderComponentSymbol*>(refObj.Ptr()))
					{
						ILOperand * op;
						if (variables.TryGetValue(refComp->UniqueName, op))
							PushStack(op);
						else
							PushStack(currentWorld->ImportInstructions[refComp->UniqueName]());
					}
				}
				else
				{
					expr->BaseExpression->Access = expr->Access;
					expr->BaseExpression->Accept(this);
					auto base = PopStack();
					auto generateSingleMember = [&](wchar_t memberName)
					{
						int idx = 0;
						if (memberName == L'y' || memberName == L'g')
							idx = 1;
						else if (memberName == L'z' || memberName == L'b')
							idx = 2;
						else if (memberName == L'w' || memberName == L'a')
							idx = 3;

						GenerateIndexExpression(base, result.Program->ConstantPool->CreateConstant(idx),
							expr->Access == ExpressionAccess::Read);
					};
					if (expr->BaseExpression->Type.IsVectorType())
					{
						if (expr->MemberName.Length() == 1)
						{
							generateSingleMember(expr->MemberName[0]);
						}
						else
						{
							if (expr->Access != ExpressionAccess::Read)
								throw InvalidOperationException(L"temporary vector (vec.xyz) is read-only.");
							String funcName = BaseTypeToString(expr->Type.BaseType);
							auto rs = AllocVar(expr->Type);
							ILOperand* tmp = codeWriter.Load(rs);
							for (int i = 0; i < expr->MemberName.Length(); i++)
							{
								generateSingleMember(expr->MemberName[i]);
								tmp = codeWriter.Update(tmp, result.Program->ConstantPool->CreateConstant(i), PopStack());
							}
							codeWriter.Store(rs, tmp);
							PushStack(codeWriter.Load(rs));
						}
					}
					else
						throw NotImplementedException(L"member expression codegen");
				}
			}
			virtual void VisitInvokeExpression(InvokeExpressionSyntaxNode* expr)
			{
				List<ILOperand*> args;
				if (currentWorld)
					currentWorld->ReferencedFunctions.Add(expr->FunctionExpr->Variable);
				for (auto arg : expr->Arguments)
				{
					arg->Accept(this);
					args.Add(PopStack());
				}
				auto instr = new CallInstruction(args.Count());
				instr->Function = expr->FunctionExpr->Variable;
				for (int i = 0; i < args.Count(); i++)
					instr->Arguments[i] = args[i];
				instr->Type = TranslateExpressionType(expr->Type);
				codeWriter.Insert(instr);
				PushStack(instr);
			}
			virtual void VisitTypeCastExpression(TypeCastExpressionSyntaxNode * expr)
			{
				expr->Expression->Accept(this);
				auto base = PopStack();
				if (expr->Expression->Type == expr->Type)
				{
					PushStack(base);
				}
				else if (expr->Expression->Type == ExpressionType::Float &&
					expr->Type == ExpressionType::Int)
				{
					auto instr = new Float2IntInstruction(base);
					codeWriter.Insert(instr);
					PushStack(instr);
				}
				else if (expr->Expression->Type == ExpressionType::Int &&
					expr->Type == ExpressionType::Float)
				{
					auto instr = new Int2FloatInstruction(base);
					codeWriter.Insert(instr);
					PushStack(instr);
				}
				else
				{
					Error(40001, L"Invalid type cast: \"" + expr->Expression->Type.ToString() + L"\" to \"" +
						expr->Type.ToString() + L"\"", expr);
				}
			}
			virtual void VisitUnaryExpression(UnaryExpressionSyntaxNode* expr)
			{
				if (expr->Operator == Operator::PostDec || expr->Operator == Operator::PostInc
					|| expr->Operator == Operator::PreDec || expr->Operator == Operator::PreInc)
				{
					expr->Expression->Access = ExpressionAccess::Read;
					expr->Expression->Accept(this);
					auto base = PopStack();
					BinaryInstruction * instr;
					if (expr->Operator == Operator::PostDec)
						instr = new SubInstruction();
					else
						instr = new AddInstruction();
					instr->Operands.SetSize(2);
					instr->Operands[0] = base;
					if (expr->Type == ExpressionType::Float)
						instr->Operands[1] = result.Program->ConstantPool->CreateConstant(1.0f);
					else
						instr->Operands[1] = result.Program->ConstantPool->CreateConstant(1);
					instr->Type = TranslateExpressionType(expr->Type);
					codeWriter.Insert(instr);

					expr->Expression->Access = ExpressionAccess::Write;
					expr->Expression->Accept(this);
					auto dest = PopStack();
					auto store = new StoreInstruction(dest, instr);
					codeWriter.Insert(store);
					PushStack(base);
				}
				else if (expr->Operator == Operator::PreDec || expr->Operator == Operator::PreInc)
				{
					expr->Expression->Access = ExpressionAccess::Read;
					expr->Expression->Accept(this);
					auto base = PopStack();
					BinaryInstruction * instr;
					if (expr->Operator == Operator::PostDec)
						instr = new SubInstruction();
					else
						instr = new AddInstruction();
					instr->Operands.SetSize(2);
					instr->Operands[0] = base;
					if (expr->Type == ExpressionType::Float)
						instr->Operands[1] = result.Program->ConstantPool->CreateConstant(1.0f);
					else
						instr->Operands[1] = result.Program->ConstantPool->CreateConstant(1);
					instr->Type = TranslateExpressionType(expr->Type);
					codeWriter.Insert(instr);

					expr->Expression->Access = ExpressionAccess::Write;
					expr->Expression->Accept(this);
					auto dest = PopStack();
					auto store = new StoreInstruction(dest, instr);
					codeWriter.Insert(store);
					PushStack(instr);
				}
				else
				{
					expr->Expression->Accept(this);
					auto base = PopStack();
					auto genUnaryInstr = [&](ILOperand * input)
					{
						UnaryInstruction * rs = 0;
						switch (expr->Operator)
						{
						case Operator::Not:
							input = EnsureBoolType(input, expr->Expression->Type);
							rs = new NotInstruction();
							break;
						case Operator::Neg:
							rs = new NegInstruction();
							break;
						case Operator::BitNot:
							rs = new BitNotInstruction();
							break;
						default:
							throw NotImplementedException(L"Code gen is not implemented for this operator.");
						}
						rs->Operand = input;
						rs->Type = input->Type;
						codeWriter.Insert(rs);
						return rs;
					};
					PushStack(genUnaryInstr(base));
				}
			}
			bool GenerateVarRef(String name, ExpressionType & type, ExpressionAccess access)
			{
				ILOperand * var = 0;
				String srcName = name;
				if (!variables.TryGetValue(srcName, var))
				{
					return false;
				}
				if (access == ExpressionAccess::Read)
				{
					auto instr = new LoadInstruction();
					instr->Name = L"local_" + name;
					instr->Operand = var;
					instr->Type = TranslateExpressionType(type);
					codeWriter.Insert(instr);
					instr->Attribute = var->Attribute;
					if (!Is<LeaInstruction>(var))
						throw L"error";
					PushStack(instr);
				}
				else
				{
					PushStack(var);
				}
				return true;
			}
			virtual void VisitVarExpression(VarExpressionSyntaxNode* expr)
			{
				RefPtr<Object> refObj;
				if (expr->Tags.TryGetValue(L"ComponentReference", refObj))
				{
					if (auto refComp = dynamic_cast<ShaderComponentSymbol*>(refObj.Ptr()))
					{
						ILOperand * op;
						if (variables.TryGetValue(refComp->UniqueName, op))
							PushStack(op);
						else
							PushStack(currentWorld->ImportInstructions[refComp->UniqueName]());
					}
				}
				else if (!GenerateVarRef(expr->Variable, expr->Type, expr->Access))
				{
						throw InvalidProgramException(L"identifier is neither a variable nor a regnoized component.");
				}
			}
			virtual void VisitParameter(ParameterSyntaxNode*){}
			virtual void VisitType(TypeSyntaxNode*){}
			virtual void VisitDeclrVariable(Variable*){}
		private:
			CodeGenerator & operator = (const CodeGenerator & other) = delete;
		public:
			CodeGenerator(ShaderCompiler * compiler, SymbolTable * symbols, ErrorWriter * pErr, CompileResult & _result)
				: ICodeGenerator(pErr), compiler(compiler), symTable(symbols), result(_result)
			{
				result.Program = new CompiledProgram();
				codeWriter.SetConstantPool(result.Program->ConstantPool.Ptr());
			}
		};

		ICodeGenerator * CreateCodeGenerator(ShaderCompiler * compiler, SymbolTable * symbols, CompileResult & result)
		{
			return new CodeGenerator(compiler, symbols, result.GetErrorWriter(), result);
		}
	}
}

/***********************************************************************
SPIRECORE\COMPILEDPROGRAM.CPP
***********************************************************************/
namespace Spire
{
	namespace Compiler
	{
		void CompiledShaderSource::PrintAdditionalCode(StringBuilder & sb, String userCode)
		{
			auto lines = CoreLib::Text::Parser::SplitString(userCode, L'\n');
			for (auto & line : lines)
			{
				CoreLib::Text::Parser parser(line);
				while (!parser.IsEnd())
				{
					auto word = parser.ReadToken().Str;
					if (word == L"$")
					{
						auto compName = parser.ReadToken().Str;
						String accessName;
						if (ComponentAccessNames.TryGetValue(compName, accessName))
						{
							sb << accessName;
						}
						else
							throw InvalidOperationException(L"cannot resolve symbol \'" + compName + L"\'.");
					}
					else
					{
						sb << word;
						if (word != L"#")
							sb << L" ";
					}
				}
				sb << EndLine;
			}
		}
		String CompiledShaderSource::GetAllCodeGLSL(String additionalHeader, String additionalGlobalDeclaration, String preambleCode, String epilogCode)
		{
			StringBuilder sb;
			sb << GlobalHeader << EndLine;
			sb << additionalHeader << EndLine;
			for (auto & compAccess : ComponentAccessNames)
			{
				sb << L"//$" << compAccess.Key << L"$" << compAccess.Value << EndLine;
			}
			for (auto &input : InputDeclarations)
			{
				sb << L"//! input from " << input.Key << EndLine;
				sb << input.Value;
			}
			sb << EndLine << L"//! output declarations" << EndLine << OutputDeclarations;
			sb << EndLine << L"//! global declarations" << EndLine;
			sb << GlobalDefinitions << EndLine;
			sb << additionalGlobalDeclaration;
			sb << EndLine << L"//! end declarations";
			sb << EndLine << L"void main()\n{";
			sb << EndLine << L"//! local declarations" << EndLine;
			sb << LocalDeclarations; 
			sb << EndLine << L"//! main code" << EndLine;
			PrintAdditionalCode(sb, preambleCode);
			sb << EndLine << MainCode << EndLine;
			PrintAdditionalCode(sb, epilogCode);
			sb << EndLine << L"//! end code" << EndLine;
			sb << L"}\n";
			StringBuilder sbIndent;
			IndentString(sbIndent, sb.ProduceString());
			return sbIndent.ProduceString();
		}

		void CompiledShaderSource::ParseFromGLSL(String code)
		{
			List<String> lines = CoreLib::Text::Parser::SplitString(code, L'\n');
			List<String> chunks;
			List<String> headers;
			StringBuilder currentBuilder;
			for (auto & line : lines)
			{
				auto trimLine = line.TrimStart();
				if (trimLine.StartsWith(L"//!"))
				{
					chunks.Add(currentBuilder.ToString());
					headers.Add(trimLine);
					currentBuilder.Clear();
				}
				else if (trimLine.StartsWith(L"//$"))
				{
					auto words = CoreLib::Text::Parser::SplitString(trimLine.SubString(3, trimLine.Length() - 3), L'$');
					if (words.Count() == 2)
						ComponentAccessNames[words[0]] = words[1];
				}
				else if (trimLine.Length() > 0)
					currentBuilder << trimLine << L'\n';
			}
			chunks.Add(currentBuilder.ToString());
			if (chunks.Count() == headers.Count() + 1 && chunks.Count() != 0)
			{
				GlobalHeader = chunks[0];
				for (int i = 0; i < headers.Count(); i++)
				{
					auto & header = headers[i];
					auto & chunk = chunks[i + 1];
					if (header.StartsWith(L"//! input from "))
					{
						String world = header.SubString(15, header.Length() - 15);
						InputDeclarations[world] = chunk;
					}
					else if (header.StartsWith(L"//! output declarations"))
						OutputDeclarations = chunk;
					else if (header.StartsWith(L"//! global declarations"))
						GlobalDefinitions = chunk;
					else if (header.StartsWith(L"//! local declarations"))
						LocalDeclarations = chunk;
					else if (header.StartsWith(L"//! main code"))
						MainCode = chunk;
				}
			}
		}

		void IndentString(StringBuilder & sb, String src)
		{
			int indent = 0;
			bool beginTrim = true;
			for (int c = 0; c < src.Length(); c++)
			{
				auto ch = src[c];
				if (ch == L'\n')
				{
					sb << L"\n";

					beginTrim = true;
				}
				else
				{
					if (beginTrim)
					{
						while (c < src.Length() - 1 && (src[c] == L'\t' || src[c] == L'\n' || src[c] == L'\r' || src[c] == L' '))
						{
							c++;
							ch = src[c];
						}
						for (int i = 0; i < indent - 1; i++)
							sb << L'\t';
						if (ch != '}' && indent > 0)
							sb << L'\t';
						beginTrim = false;
					}

					if (ch == L'{')
						indent++;
					else if (ch == L'}')
						indent--;
					if (indent < 0)
						indent = 0;

					sb << ch;
				}
			}
		}
		ShaderChoiceValue ShaderChoiceValue::Parse(String str)
		{
			ShaderChoiceValue result;
			int idx = str.IndexOf(L':');
			if (idx == -1)
				return ShaderChoiceValue(str, L"");
			return ShaderChoiceValue(str.SubString(0, idx), str.SubString(idx + 1, str.Length() - idx - 1));
		}
	}
}

/***********************************************************************
SPIRECORE\CONSTANTPOOL.CPP
***********************************************************************/
#ifndef CONSTANT_POOL_H
#define CONSTANT_POOL_H


namespace Spire
{
	namespace Compiler
	{
		class ConstantPoolImpl
		{
		private:
			ILUndefinedOperand undefOperand;
			Dictionary<ConstKey<int>, ILConstOperand*> intConsts;
			Dictionary<ConstKey<float>, ILConstOperand*> floatConsts;
			List<RefPtr<ILConstOperand>> constants;
		public:
			ILUndefinedOperand * GetUndefinedOperand()
			{
				return &undefOperand;
			}
			ILOperand * CreateDefaultValue(ILType * type)
			{
				ILOperand * value = 0;
				if (type->IsFloat())
					value = CreateConstant(0.0f);
				else if (type->IsInt())
					value = CreateConstant(0);
				else if (auto baseType = dynamic_cast<ILBasicType*>(type))
				{
					if (baseType->Type == ILBaseType::Int2)
					{
						value = CreateConstant(0, 2);
					}
					else if (baseType->Type == ILBaseType::Int3)
					{
						value = CreateConstant(0, 3);
					}
					else if (baseType->Type == ILBaseType::Int4)
					{
						value = CreateConstant(0, 4);
					}
					else if (baseType->Type == ILBaseType::Float2)
					{
						value = CreateConstant(0.0f, 2);
					}
					else if (baseType->Type == ILBaseType::Float3)
					{
						value = CreateConstant(0.0f, 3);
					}
					else if (baseType->Type == ILBaseType::Float4)
					{
						value = CreateConstant(0.0f, 4);
					}
					else if (baseType->Type == ILBaseType::Float3x3)
					{
						value = CreateConstant(0.0f, 9);
					}
					else if (baseType->Type == ILBaseType::Float4x4)
					{
						value = CreateConstant(0.0f, 16);
					}
					else
						throw NotImplementedException(L"default value for this type is not implemented.");
				}
				else
					throw NotImplementedException(L"default value for this type is not implemented.");
				return value;
			}
			ILConstOperand * CreateConstantIntVec(int val, int val2)
			{
				ILConstOperand * rs = 0;
				auto key = ConstKey<int>::FromValues(val, val2);
				if (intConsts.TryGetValue(key, rs))
					return rs;
				rs = new ILConstOperand();
				rs->Type = new ILBasicType(ILBaseType::Int2);
				rs->IntValues[0] = val;
				rs->IntValues[1] = val2;
				intConsts[key] = rs;
				rs->Name = rs->ToString();
				constants.Add(rs);
				return rs;
			}

			ILConstOperand * CreateConstantIntVec(int val, int val2, int val3)
			{
				ILConstOperand * rs = 0;
				auto key = ConstKey<int>::FromValues(val, val2, val3);
				if (intConsts.TryGetValue(key, rs))
					return rs;
				rs = new ILConstOperand();
				rs->Type = new ILBasicType(ILBaseType::Int3);
				rs->IntValues[0] = val;
				rs->IntValues[1] = val2;
				rs->IntValues[2] = val3;

				intConsts[key] = rs;
				rs->Name = rs->ToString();
				constants.Add(rs);

				return rs;
			}
			ILConstOperand * CreateConstantIntVec(int val, int val2, int val3, int val4)
			{
				ILConstOperand * rs = 0;
				auto key = ConstKey<int>::FromValues(val, val2, val3, val4);
				if (intConsts.TryGetValue(key, rs))
					return rs;
				rs = new ILConstOperand();
				rs->Type = new ILBasicType(ILBaseType::Int4);
				rs->IntValues[0] = val;
				rs->IntValues[1] = val2;
				rs->IntValues[2] = val3;
				rs->IntValues[3] = val4;
				intConsts[key] = rs;
				rs->Name = rs->ToString();
				constants.Add(rs);

				return rs;
			}

			ILConstOperand * CreateConstant(ILConstOperand * c)
			{
				auto baseType = dynamic_cast<ILBasicType*>(c->Type.Ptr())->Type;
				switch (baseType)
				{
				case ILBaseType::Float:
					return CreateConstant(c->FloatValues[0]);
				case ILBaseType::Float2:
					return CreateConstant(c->FloatValues[0], c->FloatValues[1]);
				case ILBaseType::Float3:
					return CreateConstant(c->FloatValues[0], c->FloatValues[1], c->FloatValues[2]);
				case ILBaseType::Float4:
					return CreateConstant(c->FloatValues[0], c->FloatValues[1], c->FloatValues[2], c->FloatValues[3]);
				case ILBaseType::Int:
					return CreateConstant(c->IntValues[0]);
				case ILBaseType::Int2:
					return CreateConstantIntVec(c->IntValues[0], c->IntValues[1]);
				case ILBaseType::Int3:
					return CreateConstantIntVec(c->IntValues[0], c->IntValues[1], c->IntValues[2]);
				case ILBaseType::Int4:
					return CreateConstantIntVec(c->IntValues[0], c->IntValues[1], c->IntValues[2], c->IntValues[3]);
				default:
					if (constants.IndexOf(c) != -1)
						return c;
					else
					{
						auto rs = new ILConstOperand(*c);
						constants.Add(rs);
						return rs;
					}
				}
			}

			ILConstOperand * CreateConstant(int val, int size = 0)
			{
				ILConstOperand * rs = 0;
				if (intConsts.TryGetValue(ConstKey<int>(val, size), rs))
					return rs;
				rs = new ILConstOperand();
				ILBaseType baseType;
				switch (size)
				{
				case 0:
				case 1:
					baseType = ILBaseType::Int;
					break;
				case 2:
					baseType = ILBaseType::Int2;
					break;
				case 3:
					baseType = ILBaseType::Int3;
					break;
				case 4:
					baseType = ILBaseType::Int4;
					break;
				default:
					throw InvalidOperationException(L"Invalid vector size.");
				}
				rs->Type = new ILBasicType(baseType);
				rs->IntValues[0] = val;
				intConsts[ConstKey<int>(val, size)] = rs;
				rs->Name = rs->ToString();
				constants.Add(rs);

				return rs;
			}

			ILConstOperand * CreateConstant(float val, int size = 0)
			{
				ILConstOperand * rs = 0;
				if (floatConsts.TryGetValue(ConstKey<float>(val, size), rs))
					return rs;
				if (Math::IsNaN(val) || Math::IsInf(val))
				{
					throw InvalidOperationException(L"Attempting to create NAN constant.");
				}
				rs = new ILConstOperand();
				ILBaseType baseType;
				switch (size)
				{
				case 0:
				case 1:
					baseType = ILBaseType::Float;
					break;
				case 2:
					baseType = ILBaseType::Float2;
					break;
				case 3:
					baseType = ILBaseType::Float3;
					break;
				case 4:
					baseType = ILBaseType::Float4;
					break;
				case 9:
					baseType = ILBaseType::Float3x3;
					break;
				case 16:
					baseType = ILBaseType::Float4x4;
					break;
				default:
					throw InvalidOperationException(L"Invalid vector size.");
				}
				rs->Type = new ILBasicType(baseType);
				for (int i = 0; i < 16; i++)
					rs->FloatValues[i] = val;
				floatConsts[ConstKey<float>(val, size)] = rs;
				rs->Name = rs->ToString();
				constants.Add(rs);

				return rs;
			}

			ILConstOperand * CreateConstant(float val, float val2)
			{
				ILConstOperand * rs = 0;
				if (Math::IsNaN(val) || Math::IsInf(val) || Math::IsNaN(val2) || Math::IsInf(val2))
				{
					throw InvalidOperationException(L"Attempting to create NAN constant.");
				}
				auto key = ConstKey<float>::FromValues(val, val2);
				if (floatConsts.TryGetValue(key, rs))
					return rs;
				rs = new ILConstOperand();
				rs->Type = new ILBasicType(ILBaseType::Float2);
				rs->FloatValues[0] = val;
				rs->FloatValues[1] = val2;
				floatConsts[key] = rs;
				rs->Name = rs->ToString();
				constants.Add(rs);

				return rs;
			}

			ILConstOperand * CreateConstant(float val, float val2, float val3)
			{
				ILConstOperand * rs = 0;
				if (Math::IsNaN(val) || Math::IsInf(val) || Math::IsNaN(val2) || Math::IsInf(val2) || Math::IsNaN(val3) || Math::IsInf(val3))
				{
					throw InvalidOperationException(L"Attempting to create NAN constant.");
				}
				auto key = ConstKey<float>::FromValues(val, val2, val3);
				if (floatConsts.TryGetValue(key, rs))
					return rs;
				rs = new ILConstOperand();
				rs->Type = new ILBasicType(ILBaseType::Float3);
				rs->FloatValues[0] = val;
				rs->FloatValues[1] = val2;
				rs->FloatValues[2] = val3;

				floatConsts[key] = rs;
				rs->Name = rs->ToString();
				constants.Add(rs);

				return rs;
			}

			ILConstOperand * CreateConstant(float val, float val2, float val3, float val4)
			{
				if (Math::IsNaN(val) || Math::IsInf(val) || Math::IsNaN(val2) || Math::IsInf(val2) || Math::IsNaN(val3) || Math::IsInf(val3) || Math::IsNaN(val4) || Math::IsInf(val4))
				{
					throw InvalidOperationException(L"Attempting to create NAN constant.");
				}
				ILConstOperand * rs = 0;
				auto key = ConstKey<float>::FromValues(val, val2, val3, val4);
				if (floatConsts.TryGetValue(key, rs))
					return rs;
				rs = new ILConstOperand();
				rs->Type = new ILBasicType(ILBaseType::Float4);
				rs->FloatValues[0] = val;
				rs->FloatValues[1] = val2;
				rs->FloatValues[2] = val3;
				rs->FloatValues[3] = val4;

				floatConsts[key] = rs;
				rs->Name = rs->ToString();
				constants.Add(rs);

				return rs;
			}
		};

		ConstantPool::ConstantPool()
		{
			impl = new ConstantPoolImpl();
		}
		ConstantPool::~ConstantPool()
		{
			delete impl;
		}
		ILUndefinedOperand * ConstantPool::GetUndefinedOperand()
		{
			return impl->GetUndefinedOperand();
		}
		ILConstOperand * ConstantPool::CreateConstant(ILConstOperand * c)
		{
			return impl->CreateConstant(c);
		}
		ILConstOperand * ConstantPool::CreateConstantIntVec(int val0, int val1)
		{
			return impl->CreateConstantIntVec(val0, val1);

		}
		ILConstOperand * ConstantPool::CreateConstantIntVec(int val0, int val1, int val2)
		{
			return impl->CreateConstantIntVec(val0, val1, val2);
		}
		ILConstOperand * ConstantPool::CreateConstantIntVec(int val0, int val1, int val3, int val4)
		{
			return impl->CreateConstantIntVec(val0, val1, val3, val4);
		}
		ILConstOperand * ConstantPool::CreateConstant(int val, int vectorSize)
		{
			return impl->CreateConstant(val, vectorSize);
		}
		ILConstOperand * ConstantPool::CreateConstant(float val, int vectorSize)
		{
			return impl->CreateConstant(val, vectorSize);
		}
		ILConstOperand * ConstantPool::CreateConstant(float val, float val1)
		{
			return impl->CreateConstant(val, val1);
		}
		ILConstOperand * ConstantPool::CreateConstant(float val, float val1, float val2)
		{
			return impl->CreateConstant(val, val1, val2);
		}
		ILConstOperand * ConstantPool::CreateConstant(float val, float val1, float val2, float val3)
		{
			return impl->CreateConstant(val, val1, val2, val3);
		}
		ILOperand * ConstantPool::CreateDefaultValue(ILType * type)
		{
			return impl->CreateDefaultValue(type);
		}
	}
}

#endif

/***********************************************************************
SPIRECORE\CPPCODEGENINCLUDE.CPP
***********************************************************************/

const char * CppCodeIncludeString1 = R"(
#ifndef SHADER_RUNTIME_H
#define SHADER_RUNTIME_H
#include <vector>
template<typename T, int dim>
class Vec
{
public:
	T vals[dim];
	Vec() = default;
	Vec(const Vec<T, dim> & other) = default;
	inline Vec(T val)
	{
		for (int i = 0; i < dim; i++)
			vals[i] = val;
	}
	inline Vec(T x, T y)
	{
		vals[0] = x;
		vals[1] = y;
	}
	inline Vec(T x, T y, T z)
	{
		vals[0] = x;
		vals[1] = y;
		vals[2] = z;
	}
	inline Vec(T x, T y, T z, T w)
	{
		vals[0] = x;
		vals[1] = y;
		vals[2] = z;
		vals[3] = w;
	}
	inline T & x()
	{
		return vals[0];
	}
	inline T & y()
	{
		return vals[1];
	}
	inline T & z()
	{
		return vals[2];
	}
	inline T & w()
	{
		return vals[3];
	}
	inline T x() const
	{
		return vals[0];
	}
	inline T y() const
	{
		return vals[1];
	}
	inline T z() const
	{
		return vals[2];
	}
	inline T w() const
	{
		return vals[3];
	}
	inline Vec<T, 2> xy() const
	{
		return Vec<T, 2>(vals[0], vals[1]);
	}
	inline Vec<T, 3> xyz() const
	{
		return Vec<T, 3>(vals[0], vals[1], vals[2]);
	}
	inline Vec<T, dim> operator +(const Vec<T, dim> & other) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] + other.vals[i];
		return rs;
	}
	inline Vec<T, dim> operator -() const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = -vals[i];
		return rs;
	}
	inline Vec<int, dim> operator !() const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] != 0;
		return rs;
	}
	inline Vec<T, dim> operator -(const Vec<T, dim> & other) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] - other.vals[i];
		return rs;
	}
	inline Vec<T, dim> operator *(T s) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] * s;
		return rs;
	}
	inline Vec<T, dim> operator *(const Vec<T, dim> & other) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] * other.vals[i];
		return rs;
	}
	inline Vec<T, dim> operator /(const Vec<T, dim> & other) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] / other.vals[i];
		return rs;
	}
	inline Vec<T, dim> operator %(const Vec<T, dim> & other) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] % other.vals[i];
		return rs;
	}
	inline Vec<int, dim> operator >(const Vec<T, dim> & other) const
	{
		Vec<int, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] > other.vals[i]>1:0;
		return rs;
	}
	inline Vec<int, dim> operator <(const Vec<T, dim> & other) const
	{
		Vec<int, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] < other.vals[i]>1:0;
		return rs;
	}
	inline Vec<int, dim> operator >=(const Vec<T, dim> & other) const
	{
		Vec<int, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] >= other.vals[i]>1:0;
		return rs;
	}
	inline Vec<int, dim> operator <=(const Vec<T, dim> & other) const
	{
		Vec<int, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] <= other.vals[i]>1:0;
		return rs;
	}
	inline Vec<int, dim> operator ==(const Vec<T, dim> & other) const
	{
		Vec<int, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] == other.vals[i]>1:0;
		return rs;
	}
	inline Vec<int, dim> operator !=(const Vec<T, dim> & other) const
	{
		Vec<int, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] != other.vals[i]>1:0;
		return rs;
	}
	inline const T & operator [](int i) const
	{
		return vals[i];
	}
	inline T & operator [](int i)
	{
		return vals[i];
	}
	template<typename Func>
	inline Vec<T, dim> map(const Func & f) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = f(rs.vals[i]);
		return rs;
	}
};

typedef Vec<float, 2> vec2;
typedef Vec<float, 3> vec3;
typedef Vec<float, 4> vec4;
typedef Vec<int, 2> ivec2;
typedef Vec<int, 3> ivec3;
typedef Vec<int, 4> ivec4;

inline vec4 make_vec4(float v)
{
	vec4 rs;
	rs[0] = v;
	return rs;
}

inline vec4 make_vec4(vec2 v)
{
	vec4 rs;
	rs[0] = v[0];
	rs[1] = v[1];
	return rs;
}

inline vec4 make_vec4(vec3 v)
{
	vec4 rs;
	rs[0] = v[0];
	rs[1] = v[1];
	rs[2] = v[2];
	return rs;
}

inline vec4 make_vec4(vec4 v)
{
	return v;
}

class mat3 : public Vec<float, 9>
{
public:
	mat3() = default;
	mat3(vec3 x, vec3 y, vec3 z)
	{
		vals[0] = x.vals[0];
		vals[1] = x.vals[1];
		vals[2] = x.vals[2];
		vals[3] = y.vals[0];
		vals[4] = y.vals[1];
		vals[5] = y.vals[2];
		vals[6] = z.vals[0];
		vals[7] = z.vals[1];
		vals[8] = z.vals[2];
	}
	mat3(float v0, float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8)
	{
		vals[0] = v0;
		vals[1] = v1;
		vals[2] = v2;
		vals[3] = v3;
		vals[4] = v4;
		vals[5] = v5;
		vals[6] = v6;
		vals[7] = v7;
		vals[8] = v8;
	}
	inline mat3 operator * (const mat3 & other) const
	{
		mat3 result;
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
			{
				float dot = 0.0f;
				for (int k = 0; k < 3; k++)
					dot += vals[k * 3 + j] * other.vals[i * 3 + k];
				result.vals[i * 3 + j] = dot;
			}
		return result;
	}
	inline vec3 operator * (const vec3 & other) const
	{
		vec3 result;
		for (int j = 0; j < 3; j++)
		{
			float dot = 0.0f;
			for (int k = 0; k < 3; k++)
				dot += vals[k * 3 + j] * other.vals[k];
			result.vals[j] = dot;
		}
		return result;
	}
	inline vec3 & operator [](int i)
	{
		return *((vec3*)(void*)this + i);
	}
	inline vec3 operator [](int i) const
	{
		return *((vec3*)(void*)this + i);
	}
	inline vec3 x() const
	{
		return this->operator[](0);
	}
	inline vec3 y() const
	{
		return this->operator[](1);
	}
	inline vec3 z() const
	{
		return this->operator[](2);
	}
	inline vec3 & x()
	{
		return this->operator[](0);
	}
	inline vec3 & y()
	{
		return this->operator[](1);
	}
	inline vec3 & z()
	{
		return this->operator[](2);
	}
};

class mat4 : public Vec<float, 16>
{
public:
	mat4() = default;
	mat4(vec4 x, vec4 y, vec4 z, vec4 w)
	{
		vals[0] = x.vals[0];
		vals[1] = x.vals[1];
		vals[2] = x.vals[2];
		vals[3] = x.vals[3];

		vals[4] = y.vals[0];
		vals[5] = y.vals[1];
		vals[6] = y.vals[2];
		vals[7] = y.vals[3];

		vals[8] = z.vals[0];
		vals[9] = z.vals[1];
		vals[10] = z.vals[2];
		vals[11] = z.vals[3];

		vals[12] = w.vals[0];
		vals[13] = w.vals[1];
		vals[14] = w.vals[2];
		vals[15] = w.vals[3];
	}
	mat4(float v0, float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8, float v9, float v10, float v11, float v12, float v13, float v14, float v15)
	{
		vals[0] = v0;
		vals[1] = v1;
		vals[2] = v2;
		vals[3] = v3;
		vals[4] = v4;
		vals[5] = v5;
		vals[6] = v6;
		vals[7] = v7;
		vals[8] = v8;
		vals[9] = v9;
		vals[10] = v10;
		vals[11] = v11;
		vals[12] = v12;
		vals[13] = v13;
		vals[14] = v14;
		vals[15] = v15;
	}
	inline vec4 operator [](int i) const
	{
		return *((vec4*)(void*)this + i);
	}
	inline vec4 & operator [](int i)
	{
		return *((vec4*)(void*)this + i);
	}
	inline vec4 x() const
	{
		return this->operator[](0);
	}
	inline vec4 y() const
	{
		return this->operator[](1);
	}
	inline vec4 z() const
	{
		return this->operator[](2);
	}
	inline vec4 w() const
	{
		return this->operator[](3);
	}
	inline vec4 & x()
	{
		return this->operator[](0);
	}
	inline vec4 & y()
	{
		return this->operator[](1);
	}
	inline vec4 & z()
	{
		return this->operator[](2);
	}
	inline vec4 & w()
	{
		return this->operator[](32);
	}
	inline mat4 operator * (const mat4 & other) const
	{
		mat4 result;
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
			{
				float dot = 0.0f;
				for (int k = 0; k < 4; k++)
					dot += vals[k * 4 + j] * other.vals[i * 4 + k];
				result.vals[i * 4 + j] = dot;
			}
		return result;
	}
	inline vec4 operator * (const vec4 & other) const
	{
		vec4 result;
		for (int j = 0; j < 4; j++)
		{
			float dot = 0.0f;
			for (int k = 0; k < 4; k++)
				dot += vals[k * 4 + j] * other.vals[k];
			result.vals[j] = dot;
		}
		return result;
	}
};

inline mat3 transpose(mat3 m)
{
	mat3 rs;
	rs.vals[0] = m.vals[0];
	rs.vals[1] = m.vals[3];
	rs.vals[2] = m.vals[6];

	rs.vals[3] = m.vals[1];
	rs.vals[4] = m.vals[4];
	rs.vals[5] = m.vals[7];

	rs.vals[6] = m.vals[2];
	rs.vals[7] = m.vals[5];
	rs.vals[8] = m.vals[8];
	return rs;
}

inline mat4 transpose(mat4 m)
{
	mat4 rs;
	rs.vals[0] = m.vals[0];
	rs.vals[1] = m.vals[4];
	rs.vals[2] = m.vals[8];
	rs.vals[3] = m.vals[12];

	rs.vals[4] = m.vals[1];
	rs.vals[5] = m.vals[5];
	rs.vals[6] = m.vals[9];
	rs.vals[7] = m.vals[13];

	rs.vals[8] = m.vals[2];
	rs.vals[9] = m.vals[6];
	rs.vals[10] = m.vals[10];
	rs.vals[11] = m.vals[14];

	rs.vals[12] = m.vals[3];
	rs.vals[13] = m.vals[7];
	rs.vals[14] = m.vals[11];
	rs.vals[15] = m.vals[15];
	return rs;
}

inline mat4 inverse(mat4 m)
{
	mat4 rs;
	double Result[4][4];
	double tmp[12];
	double src[16];
	double det;
	for (int i = 0; i < 4; i++)
	{
		src[i + 0] = m[i][0];
		src[i + 4] = m[i][1];
		src[i + 8] = m[i][2];
		src[i + 12] = m[i][3];
	}
	tmp[0] = src[10] * src[15];
	tmp[1] = src[11] * src[14];
	tmp[2] = src[9] * src[15];
	tmp[3] = src[11] * src[13];
	tmp[4] = src[9] * src[14];
	tmp[5] = src[10] * src[13];
	tmp[6] = src[8] * src[15];
	tmp[7] = src[11] * src[12];
	tmp[8] = src[8] * src[14];
	tmp[9] = src[10] * src[12];
	tmp[10] = src[8] * src[13];
	tmp[11] = src[9] * src[12];
	Result[0][0] = tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7];
	Result[0][0] -= tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7];
	Result[0][1] = tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7];
	Result[0][1] -= tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7];
	Result[0][2] = tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7];
	Result[0][2] -= tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7];
	Result[0][3] = tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6];
	Result[0][3] -= tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6];
	Result[1][0] = tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3];
	Result[1][0] -= tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3];
	Result[1][1] = tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3];
	Result[1][1] -= tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3];
	Result[1][2] = tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3];
	Result[1][2] -= tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3];
	Result[1][3] = tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2];
	Result[1][3] -= tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2];
	tmp[0] = src[2] * src[7];
	tmp[1] = src[3] * src[6];
	tmp[2] = src[1] * src[7];
	tmp[3] = src[3] * src[5];
	tmp[4] = src[1] * src[6];
	tmp[5] = src[2] * src[5];
	tmp[6] = src[0] * src[7];
	tmp[7] = src[3] * src[4];
	tmp[8] = src[0] * src[6];
	tmp[9] = src[2] * src[4];
	tmp[10] = src[0] * src[5];
	tmp[11] = src[1] * src[4];
	Result[2][0] = tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15];
	Result[2][0] -= tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15];
	Result[2][1] = tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15];
	Result[2][1] -= tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15];
	Result[2][2] = tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15];
	Result[2][2] -= tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15];
	Result[2][3] = tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14];
	Result[2][3] -= tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14];
	Result[3][0] = tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9];
	Result[3][0] -= tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10];
	Result[3][1] = tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10];
	Result[3][1] -= tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8];
	Result[3][2] = tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8];
	Result[3][2] -= tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9];
	Result[3][3] = tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9];
	Result[3][3] -= tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8];
	det = src[0] * Result[0][0] + src[1] * Result[0][1] + src[2] * Result[0][2] + src[3] * Result[0][3];
	det = 1.0f / det;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			rs.vals[i * 4 + j] = (float)(Result[i][j] * det);
		}
	}
	return rs;
}
)";
const char * CppCodeIncludeString2 = R"(
template<typename T, int dim>
inline T dot(Vec<T, dim> v0, Vec<T, dim> v1)
{
	T rs = 0;
	for (int i = 0; i < dim; i++)
	{
		rs += v0.vals[i] * v1.vals[i];
	}
	return rs;
}

template<int dim>
inline Vec<float, dim> normalize(Vec<float, dim> v0)
{
	Vec<float, dim> rs;
	float length = 0.0f;
	for (int i = 0; i < dim; i++)
	{
		length += v0.vals[i] * v0.vals[i];
	}
	length = 1.0f / sqrt(length);
	for (int i = 0; i < dim; i++)
		rs.vals[i] *= length;
	return rs;
}

template<typename T, int dim>
inline float length(const Vec<T, dim> & v0)
{
	float length = 0.0f;
	for (int i = 0; i < dim; i++)
	{
		length += v0.vals[i] * v0.vals[i];
	}
	return length;
}

template<typename T, int dim>
inline float distance(const Vec<T, dim> & v0, const Vec<T, dim> & v1)
{
	float length = 0.0f;
	for (int i = 0; i < dim; i++)
	{
		float diff = (v0.vals[i] - v1.vals[i]);
		length += diff * diff;
	}
	return length;
}

template<int dim>
inline Vec<float, dim> sin(const Vec<float, dim> & x)
{
	return x.map(sinf);
}
template<int dim>
Vec<float, dim> sinh(const Vec<float, dim> & x)
{
	return x.map(sinhf);
}
template<int dim>
inline Vec<float, dim> cos(const Vec<float, dim> & x)
{
	return x.map(cosf);
}
template<int dim>
inline Vec<float, dim> sqrt(const Vec<float, dim> & x)
{
	return x.map(sqrtf);
}
template<int dim>
inline Vec<float, dim> log(const Vec<float, dim> & x)
{
	return x.map(logf);
}
template<int dim>
inline Vec<float, dim> log2(const Vec<float, dim> & x)
{
	return x.map(log2f);
}
template<int dim>
inline Vec<float, dim> exp(const Vec<float, dim> & x)
{
	return x.map(expf);
}
template<int dim>
inline Vec<float, dim> tan(const Vec<float, dim> & x)
{
	return x.map(tanf);
}
template<int dim>
inline Vec<float, dim> tanh(const Vec<float, dim> & x)
{
	return x.map(tanhf);
}
template<int dim>
Vec<float, dim> asin(const Vec<float, dim> & x)
{
	return x.map(asinf);
}
template<int dim>
Vec<float, dim> acos(const Vec<float, dim> & x)
{
	return x.map(acosf);
}
template<int dim>
Vec<float, dim> atan(const Vec<float, dim> & x)
{
	return x.map(atanf);
}
template<int dim>
Vec<float, dim> floor(const Vec<float, dim> & x)
{
	return x.map(floorf);
}
template<int dim>
Vec<float, dim> ceil(const Vec<float, dim> & x)
{
	return x.map(ceilf);
}
template<int dim>
Vec<float, dim> round(const Vec<float, dim> & x)
{
	return x.map(roundf);
}
template<int dim>
Vec<float, dim> abs(const Vec<float, dim> & x)
{
	return x.map(fabsf);
}
template<int dim>
Vec<float, dim> fract(const Vec<float, dim> & x)
{
	return x - floor(x);
}
template<int dim>
Vec<float, dim> exp2(const Vec<float, dim> & x)
{
	return x.map(exp2f);
}
template<int dim>
Vec<float, dim> pow(const Vec<float, dim> & x, const Vec<float, dim> & y)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = powf(x.vals[i], y.vals[i]);
	return rs;
}
template<int dim>
Vec<float, dim> atan2(const Vec<float, dim> & x, const Vec<float, dim> & y)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = atan2(x.vals[i], y.vals[i]);
	return rs;
}

template<int dim>
Vec<float, dim> min(const Vec<float, dim> & x, const Vec<float, dim> & y)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = x.vals[i] < y.vals[i] ? x.vals[i] : y.vals[i];
	return rs;
}

template<int dim>
Vec<float, dim> max(const Vec<float, dim> & x, const Vec<float, dim> & y)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = x.vals[i] > y.vals[i] ? x.vals[i] : y.vals[i];
	return rs;
}

inline float min(float a, float b)
{
	return a < b ? a : b;
}

inline float max(float a, float b)
{
	return a > b ? a : b;
}

inline float clamp(float a, float v0, float v1)
{
	if (a < v0) return v0; else if (a>v1) return v1; else return a;
}

template<int dim>
inline Vec<float, dim> clamp(const Vec<float, dim> & a, const Vec<float, dim> & v0, const Vec<float, dim> & v1)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = clamp(a.vals[i], v0.vals[i], v1.vals[i]);
	return rs;
}
inline float radians(float degrees)
{
	return (3.141592653f / 180.0f) * degrees;
}

inline float mix(float a, float b, float t)
{
	return a*(1.0f - t) + b*t;
}

template<int dim>
inline Vec<float, dim> mix(const Vec<float, dim> & a, const Vec<float, dim> &  b, float t)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = mix(a.vals[i], b.vals[i], t);
	return rs;
}

template<int dim>
inline Vec<float, dim> mix(const Vec<float, dim> & a, const Vec<float, dim> &  b, const Vec<float, dim> & t)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = mix(a.vals[i], b.vals[i], t.vals[i]);
	return rs;
}

template<typename T, int dim>
inline Vec<T, dim> reflect(const Vec<T, dim> & I, const Vec<T, dim> & N)
{
	return I - N * (2 * dot(I, N));
}

template<int dim>
inline Vec<float, dim> reflect(const Vec<float, dim> & I, const Vec<float, dim> & N, float eta)
{
	float d = dot(N, I);
	float k = 1.0f - eta*eta *(1.0f - d*d);
	if (k < 0.0)
		return Vec<float, dim>(0.0f);
	else
		return I * eta - N * (eta * d + sqrt(k));
}

inline float sign(float x)
{
	if (x < 0.0f)
		return -1.0f;
	else
		return 1.0f;
}

template<int dim>
Vec<float, dim> sign(const Vec<float, dim> & x)
{
	return x.map([](float v) {return sign(v); });
}

inline float step(float edge, float x)
{
	if (x < edge)
		return 0.0f;
	else
		return 1.0f;
}


template<int dim>
inline Vec<float, dim> step(float edge, const Vec<float, dim> & x)
{
	return x.map([](float v) {return step(edge, v); });
}

template<int dim>
inline Vec<float, dim> step(const Vec<float, dim> & edge, const Vec<float, dim> & x)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = step(edge.vals[i], x.vals[i]);
	return rs;
}

inline float smoothstep(float edge0, float edge1, float x)
{
	float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return (float)(t*t*(3.0f - 2.0*t));
}

template<int dim>
Vec<float, dim> smoothstep(const Vec<float, dim> & edge0, const Vec<float, dim> & edge1, const Vec<float, dim> & x)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = smoothstep(edge0.vals[i], edge1.vals[i], x.vals[i]);
	return rs;
}

template<int dim>
Vec<float, dim> smoothstep(float edge0, float edge1, const Vec<float, dim> & x)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = smoothstep(edge0, edge1, x.vals[i]);
	return rs;
}

class ITexture2D
{
public:
	virtual vec4 GetValue(vec2 uv, vec2 ddx, vec2 ddy) = 0;
};

typedef ITexture2D* sampler2D;
typedef ITexture2D* sampler2DShadow;

inline vec4 texture(sampler2D sampler, vec2 uv)
{
	return sampler->GetValue(uv, vec2(0.0f, 0.0f), vec2(0.0f, 0.0f));
}

inline vec4 texture(sampler2D sampler, vec2 uv, vec2 ddx, vec2 ddy)
{
	return sampler->GetValue(uv, ddx, ddy);
}

class ITexture3D
{
public:
	virtual vec4 GetValue(vec3 uvw, vec3 ddx, vec3 ddy) = 0;
};

typedef ITexture3D * samplerCube;
typedef ITexture3D * samplerCubeShadow;

inline vec4 texture(samplerCube sampler, vec3 uv)
{
	return sampler->GetValue(uv, vec3(0.0f), vec3(0.0f));
}

inline vec4 texture(samplerCube sampler, vec3 uv, vec3 ddx, vec3 ddy)
{
	return sampler->GetValue(uv, ddx, ddy);
}


class IShader
{
public:
	virtual bool SetInput(const char * name, void * buffer) = 0;
	virtual void SetInputSize(int n) = 0;
	virtual bool GetOutput(const char * name, void * buffer, int & bufferSize) = 0;
	virtual void Run() = 0;
	virtual void Dispose() = 0;
};


#ifdef _MSC_VER
#pragma warning(disable:4100; disable:4189)
#endif

#define DLL_EXPORT extern "C" __declspec(dllexport) 

#endif
)";

/***********************************************************************
SPIRECORE\GLSLCODEGEN.CPP
***********************************************************************/

using namespace CoreLib::Basic;

namespace Spire
{
	namespace Compiler
	{
		void PrintBaseType(StringBuilder & sbCode, ILType* type)
		{
			if (auto baseType = dynamic_cast<ILBasicType*>(type))
			{
				switch (baseType->Type)
				{
				case ILBaseType::Int:
					sbCode << L"int";
					break;
				case ILBaseType::Int2:
					sbCode << L"ivec2";
					break;
				case ILBaseType::Int3:
					sbCode << L"ivec3";
					break;
				case ILBaseType::Int4:
					sbCode << L"ivec4";
					break;
				case ILBaseType::Float:
					sbCode << L"float";
					break;
				case ILBaseType::Float2:
					sbCode << L"vec2";
					break;
				case ILBaseType::Float3:
					sbCode << L"vec3";
					break;
				case ILBaseType::Float4:
					sbCode << L"vec4";
					break;
				case ILBaseType::Float3x3:
					sbCode << L"mat3";
					break;
				case ILBaseType::Float4x4:
					sbCode << L"mat4";
					break;
				case ILBaseType::Texture2D:
					sbCode << L"sampler2D";
					break;
				case ILBaseType::TextureCube:
					sbCode << L"samplerCube";
					break;
				case ILBaseType::TextureShadow:
					sbCode << L"sampler2DShadow";
					break;
				case ILBaseType::TextureCubeShadow:
					sbCode << L"samplerCubeShadow";
					break;
				default:
					throw NotImplementedException(L"unkown base type.");
				}
			}
			else
				throw NotImplementedException(L"unkown base type.");
		}

		void PrintDef(StringBuilder & sbCode, ILType* type, const String & name)
		{
			if (auto arrType = dynamic_cast<ILArrayType*>(type))
			{
				PrintDef(sbCode, arrType->BaseType.Ptr(), name + L"[" + arrType->ArrayLength + L"]");
			}
			else if (auto baseType = dynamic_cast<ILBasicType*>(type))
			{
				PrintBaseType(sbCode, type);
				sbCode << L" ";
				sbCode << name;
			}
		}

		class CodeGenContext
		{
		public:
			HashSet<String> GeneratedDefinitions;
			Dictionary<String, String> SubstituteNames;
			Dictionary<ILOperand*, String> VarName;
			Dictionary<String, ImportOperatorHandler *> ImportOperatorHandlers;
			Dictionary<String, ExportOperatorHandler *> ExportOperatorHandlers;

			CompileResult * Result = nullptr;
			HashSet<String> UsedVarNames;
			StringBuilder Body, Header, GlobalHeader;
			List<ILType*> Arguments;
			String ReturnVarName;
			String GenerateCodeName(String name, String prefix)
			{
				StringBuilder nameBuilder;
				int startPos = 0;
				if (name.StartsWith(L"_sys_"))
					startPos = name.IndexOf(L'_', 5) + 1;
				nameBuilder << prefix;
				for (int i = startPos; i < name.Length(); i++)
				{
					if (name[i] >= L'a' && name[i] <= L'z' || name[i] >= L'A' && name[i] <= L'Z' || name[i] == L'_' || name[i] >= L'0' && name[i] <= L'9')
					{
						nameBuilder << name[i];
					}
					else
						nameBuilder << L'_';
				}
				auto rs = nameBuilder.ToString();
				int i = 0;
				while (UsedVarNames.Contains(rs))
				{
					i++;
					rs = nameBuilder.ToString() + String(i);
				}
				UsedVarNames.Add(rs);

				return rs;
			}


			String DefineVariable(ILOperand * op)
			{
				String rs;
				if (op->Name == L"Tex")
					printf("break");
				if (VarName.TryGetValue(op, rs))
				{
					return rs;
				}
				else
				{
					auto name = GenerateCodeName(op->Name, L"");
					PrintDef(Header, op->Type.Ptr(), name);
					if (op->Type->IsInt())
					{
						Header << L" = 0;";
					}
					Header << L";\n";
					VarName.Add(op, name);
					op->Name = name;
					return op->Name;
				}
			}
		};

		class GLSLShaderState : public Object
		{
		public:
		};

		class GLSLCodeGen : public CodeGenBackend
		{
		private:
			String vertexOutputName;
			bool useNVCommandList = false;
			CompiledWorld * currentWorld = nullptr;
		private:
			String GetFunctionCallName(String name)
			{
				StringBuilder rs;
				for (int i = 0; i < name.Length(); i++)
				{
					if (name[i] >= L'a' && name[i] <= L'z' || name[i] >= L'A' && name[i] <= L'Z' || name[i] == L'_' || name[i] >= L'0' && name[i] <= L'9')
					{
						rs << name[i];
					}
					else if (i != name.Length() - 1)
						rs << L'_';
				}
				return rs.ProduceString();
			}

			String GetFuncOriginalName(const String & name)
			{
				String originalName;
				int splitPos = name.IndexOf(L'@');
				if (splitPos == 0)
					return name;
				if (splitPos != -1)
					originalName = name.SubString(0, splitPos);
				else
					originalName = name;
				return originalName;
			}

			void PrintOp(CodeGenContext & ctx, ILOperand * op, bool forceExpression = false)
			{
				auto makeFloat = [](float v)
				{
					String rs(v, L"%.12e");
					if (!rs.Contains(L'.') && !rs.Contains(L'e') && !rs.Contains(L'E'))
						rs = rs + L".0";
					if (rs.StartsWith(L"-"))
						rs = L"(" + rs + L")";
					return rs;
				};
				if (auto c = dynamic_cast<ILConstOperand*>(op))
				{
					auto type = c->Type.Ptr();
					if (type->IsFloat())
						ctx.Body << makeFloat(c->FloatValues[0]);
					else if (type->IsInt())
						ctx.Body << (c->IntValues[0]);
					else if (auto baseType = dynamic_cast<ILBasicType*>(type))
					{
						if (baseType->Type == ILBaseType::Float2)
							ctx.Body << L"vec2(" << makeFloat(c->FloatValues[0]) << L", " << makeFloat(c->FloatValues[1]) << L")";
						else if (baseType->Type == ILBaseType::Float3)
							ctx.Body << L"vec3(" << makeFloat(c->FloatValues[0]) << L", " << makeFloat(c->FloatValues[1]) << L", " << makeFloat(c->FloatValues[2]) << L")";
						else if (baseType->Type == ILBaseType::Float4)
							ctx.Body << L"vec4(" << makeFloat(c->FloatValues[0]) << L", " << makeFloat(c->FloatValues[1]) << L", " << makeFloat(c->FloatValues[2]) << L", " << makeFloat(c->FloatValues[3]) << L")";
						else if (baseType->Type == ILBaseType::Float3x3)
						{
							ctx.Body << L"mat3(";
							for (int i = 0; i < 9; i++)
							{
								ctx.Body << makeFloat(c->FloatValues[i]);
								if (i != 8)
									ctx.Body << L", ";
							}
							ctx.Body << L")";
						}
						else if (baseType->Type == ILBaseType::Float4x4)
						{
							ctx.Body << L"mat4(";
							for (int i = 0; i < 16; i++)
							{
								ctx.Body << makeFloat(c->FloatValues[i]);
								if (i != 15)
									ctx.Body << L", ";
							}
							ctx.Body << L")";
						}
						else if (baseType->Type == ILBaseType::Int2)
							ctx.Body << L"ivec2(" << c->IntValues[0] << L", " << c->IntValues[1] << L")";
						else if (baseType->Type == ILBaseType::Int3)
							ctx.Body << L"ivec3(" << c->IntValues[0] << L", " << c->IntValues[1] << L", " << c->IntValues[2] << L")";
						else if (baseType->Type == ILBaseType::Int4)
							ctx.Body << L"ivec4(" << c->IntValues[0] << L", " << c->IntValues[1] << L", " << c->IntValues[2] << L", " << c->IntValues[3] << L")";
					}
					else
						throw InvalidOperationException(L"Illegal constant.");
				}
				else if (auto instr = dynamic_cast<ILInstruction*>(op))
				{
					if (AppearAsExpression(*instr, forceExpression))
					{
						PrintInstrExpr(ctx, *instr);
					}
					else
					{
						if (forceExpression)
							throw InvalidProgramException(L"cannot generate code block as an expression.");
						String substituteName;
						if (ctx.SubstituteNames.TryGetValue(instr->Name, substituteName))
							ctx.Body << substituteName;
						else
							ctx.Body << instr->Name;
					}
				}
				else
					throw InvalidOperationException(L"Unsupported operand type.");
			}

			void PrintBinaryInstrExpr(CodeGenContext & ctx, BinaryInstruction * instr)
			{
				if (instr->Is<StoreInstruction>())
				{
					auto op0 = instr->Operands[0].Ptr();
					auto op1 = instr->Operands[1].Ptr();
					ctx.Body << L"(";
					PrintOp(ctx, op0);
					ctx.Body << L" = ";
					PrintOp(ctx, op1);
					ctx.Body << L")";
					return;
				}
				auto op0 = instr->Operands[0].Ptr();
				auto op1 = instr->Operands[1].Ptr();
				if (instr->Is<StoreInstruction>())
				{
					throw InvalidOperationException(L"store instruction cannot appear as expression.");
				}
				if (auto load = instr->As<MemberLoadInstruction>())
				{
					PrintOp(ctx, op0);
					bool printDefault = true;
					if (op0->Type->IsVector())
					{
						if (auto c = dynamic_cast<ILConstOperand*>(op1))
						{
							switch (c->IntValues[0])
							{
							case 0:
								ctx.Body << L".x";
								break;
							case 1:
								ctx.Body << L".y";
								break;
							case 2:
								ctx.Body << L".z";
								break;
							case 3:
								ctx.Body << L".w";
								break;
							default:
								throw InvalidOperationException(L"Invalid member access.");
							}
							printDefault = false;
						}
					}
					if (printDefault)
					{
						ctx.Body << L"[";
						PrintOp(ctx, op1);
						ctx.Body << L"]";
					}
					return;
				}
				const wchar_t * op = L"";
				if (instr->Is<AddInstruction>())
				{
					op = L"+";
				}
				else if (instr->Is<SubInstruction>())
				{
					op = L"-";
				}
				else if (instr->Is<MulInstruction>())
				{
					op = L"*";
				}
				else if (instr->Is<DivInstruction>())
				{
					op = L"/";
				}
				else if (instr->Is<ModInstruction>())
				{
					op = L"%";
				}
				else if (instr->Is<ShlInstruction>())
				{
					op = L"<<";
				}
				else if (instr->Is<ShrInstruction>())
				{
					op = L">>";
				}
				else if (instr->Is<CmpeqlInstruction>())
				{
					op = L"==";
					ctx.Body << L"int";
				}
				else if (instr->Is<CmpgeInstruction>())
				{
					op = L">=";
					ctx.Body << L"int";
				}
				else if (instr->Is<CmpgtInstruction>())
				{
					op = L">";
					ctx.Body << L"int";
				}
				else if (instr->Is<CmpleInstruction>())
				{
					op = L"<=";
					ctx.Body << L"int";
				}
				else if (instr->Is<CmpltInstruction>())
				{
					op = L"<";
					ctx.Body << L"int";
				}
				else if (instr->Is<CmpneqInstruction>())
				{
					op = L"!=";
					ctx.Body << L"int";
				}
				else if (instr->Is<AndInstruction>())
				{
					op = L"&&";
				}
				else if (instr->Is<OrInstruction>())
				{
					op = L"||";
				}
				else if (instr->Is<BitXorInstruction>())
				{
					op = L"^";
				}
				else if (instr->Is<BitAndInstruction>())
				{
					op = L"&";
				}
				else if (instr->Is<BitOrInstruction>())
				{
					op = L"|";
				}
				else
					throw InvalidProgramException(L"unsupported binary instruction.");
				ctx.Body << L"(";
				PrintOp(ctx, op0);
				ctx.Body << L" " << op << L" ";
				PrintOp(ctx, op1);
				ctx.Body << L")";
			}

			void PrintBinaryInstr(CodeGenContext & ctx, BinaryInstruction * instr)
			{
				auto op0 = instr->Operands[0].Ptr();
				auto op1 = instr->Operands[1].Ptr();
				if (instr->Is<StoreInstruction>())
				{
					PrintOp(ctx, op0);
					ctx.Body << L" = ";
					PrintOp(ctx, op1);
					ctx.Body << L";\n";
					return;
				}
				auto varName = ctx.DefineVariable(instr);
				if (auto load = instr->As<MemberLoadInstruction>())
				{
					ctx.Body << varName << L" = ";
					PrintBinaryInstrExpr(ctx, instr);
					ctx.Body << L";\n";
					return;
				}
				ctx.Body << varName << L" = ";
				PrintBinaryInstrExpr(ctx, instr);
				ctx.Body << L";\n";
			}

			void PrintUnaryInstrExpr(CodeGenContext & ctx, UnaryInstruction * instr)
			{
				auto op0 = instr->Operand.Ptr();
				if (instr->Is<LoadInstruction>())
				{
					PrintOp(ctx, op0);
					return;
				}
				const wchar_t * op = L"";
				if (instr->Is<BitNotInstruction>())
					op = L"~";
				else if (instr->Is<Float2IntInstruction>())
					op = L"(int)";
				else if (instr->Is<Int2FloatInstruction>())
					op = L"(float)";
				else if (instr->Is<CopyInstruction>())
					op = L"";
				else if (instr->Is<NegInstruction>())
					op = L"-";
				else if (instr->Is<NotInstruction>())
					op = L"!";
				else
					throw InvalidProgramException(L"unsupported unary instruction.");
				ctx.Body << L"(" << op;
				PrintOp(ctx, op0);
				ctx.Body << L")";
			}

			void PrintUnaryInstr(CodeGenContext & ctx, UnaryInstruction * instr)
			{
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName << L" = ";
				PrintUnaryInstrExpr(ctx, instr);
				ctx.Body << L";\n";
			}

			void PrintGleaInstrExpr(CodeGenContext & ctx, GLeaInstruction * instr)
			{
				RefPtr<CompiledGlobalVar> gvar;
				ctx.Body << instr->VariableName;
			}

			void PrintGleaInstr(CodeGenContext & /*ctx*/, GLeaInstruction * /*instr*/)
			{

			}

			void PrintAllocVarInstrExpr(CodeGenContext & ctx, AllocVarInstruction * instr)
			{
				ctx.Body << instr->Name;
			}

			void PrintAllocVarInstr(CodeGenContext & ctx, AllocVarInstruction * instr)
			{
				if (auto size = dynamic_cast<ILConstOperand*>(instr->Size.Ptr()))
				{
					PrintDef(ctx.Header, instr->Type.Ptr(), instr->Name);
					ctx.Header << L";\n";
				}
				else
					throw InvalidProgramException(L"size operand of allocVar instr is not an intermediate.");
			}

			void PrintFetchArgInstrExpr(CodeGenContext & ctx, FetchArgInstruction * instr)
			{
				ctx.Body << instr->Name;
			}

			void PrintFetchArgInstr(CodeGenContext & ctx, FetchArgInstruction * instr)
			{
				if (instr->ArgId == 0)
				{
					ctx.ReturnVarName = ctx.DefineVariable(instr);
				}
			}

			void PrintSelectInstrExpr(CodeGenContext & ctx, SelectInstruction * instr)
			{
				ctx.Body << L"(";
				PrintOp(ctx, instr->Operands[0].Ptr());
				ctx.Body << L"?";
				PrintOp(ctx, instr->Operands[1].Ptr());
				ctx.Body << L":";
				PrintOp(ctx, instr->Operands[2].Ptr());
				ctx.Body << L")";
			}

			void PrintSelectInstr(CodeGenContext & ctx, SelectInstruction * instr)
			{
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName << L" = ";
				PrintSelectInstrExpr(ctx, instr);
				ctx.Body << L";\n";
			}

			void PrintCallInstrExpr(CodeGenContext & ctx, CallInstruction * instr)
			{
				String callName;
				callName = GetFuncOriginalName(instr->Function);
				ctx.Body << callName;
				ctx.Body << L"(";
				int id = 0;
				for (auto & arg : instr->Arguments)
				{
					PrintOp(ctx, arg.Ptr());
					if (id != instr->Arguments.Count() - 1)
						ctx.Body << L", ";
					id++;
				}
				ctx.Body << L")";
			}

			void PrintCallInstr(CodeGenContext & ctx, CallInstruction * instr)
			{
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName;
				ctx.Body << L" = ";
				PrintCallInstrExpr(ctx, instr);
				ctx.Body << L";\n";
			}

			void PrintCastF2IInstrExpr(CodeGenContext & ctx, Float2IntInstruction * instr)
			{
				ctx.Body << L"((int)(";
				PrintOp(ctx, instr->Operand.Ptr());
				ctx.Body << L"))";
			}
			void PrintCastF2IInstr(CodeGenContext & ctx, Float2IntInstruction * instr)
			{
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName;
				ctx.Body << L" = ";
				PrintCastF2IInstrExpr(ctx, instr);
				ctx.Body << L";\n";
			}
			void PrintCastI2FInstrExpr(CodeGenContext & ctx, Int2FloatInstruction * instr)
			{
				ctx.Body << L"((float)(";
				PrintOp(ctx, instr->Operand.Ptr());
				ctx.Body << L"))";
			}
			void PrintCastI2FInstr(CodeGenContext & ctx, Int2FloatInstruction * instr)
			{
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName;
				ctx.Body << L" = ";
				PrintCastI2FInstrExpr(ctx, instr);
				ctx.Body << L";\n";
			}

			bool AppearAsExpression(ILInstruction & instr, bool force)
			{
				if (auto arg = instr.As<FetchArgInstruction>())
				{
					if (arg->ArgId == 0)
						return false;
				}
				for (auto &&usr : instr.Users)
				{
					if (auto update = dynamic_cast<MemberUpdateInstruction*>(usr))
					{
						if (&instr == update->Operands[0].Ptr())
							return false;
					}
					else if (auto import = dynamic_cast<ImportInstruction*>(usr))
						return false;
				}
				if (instr.Is<StoreInstruction>() && force)
					return true;
				return (instr.Users.Count() <= 1 && !instr.HasSideEffect() && !instr.Is<MemberUpdateInstruction>()
					&& !instr.Is<AllocVarInstruction>() && !instr.Is<ImportInstruction>())
					|| instr.Is<GLeaInstruction>() || instr.Is<FetchArgInstruction>() ;
			}

			void PrintImportInstr(CodeGenContext &ctx, ImportInstruction * import)
			{
				ImportOperatorHandler * handler = nullptr;
				if (ctx.ImportOperatorHandlers.TryGetValue(import->ImportOperator->Name.Content, handler))
				{
					handler->GenerateInterfaceLocalDefinition(ctx.Body, import, ImportOperatorContext(
						import->ImportOperator->Arguments, backendArguments, currentWorld, *ctx.Result,
						import->SourceWorld));
				}
			}

			void PrintExportInstr(CodeGenContext &ctx, ExportInstruction * exportInstr)
			{
				ExportOperatorHandler * handler = nullptr;
				if (ctx.ExportOperatorHandlers.TryGetValue(exportInstr->ExportOperator, handler))
				{
					handler->GenerateExport(ctx.Body, currentWorld->WorldOutput, currentWorld, exportInstr->ComponentName, exportInstr->Operand->Name);
				}
			}

			void PrintUpdateInstr(CodeGenContext & ctx, MemberUpdateInstruction * instr)
			{
				if (auto srcInstr = dynamic_cast<ILInstruction*>(instr->Operands[0].Ptr()))
				{
					if (srcInstr->Users.Count() == 1)
					{
						auto srcName = srcInstr->Name;
						while (ctx.SubstituteNames.TryGetValue(srcName, srcName));
						ctx.Body << srcName << L"[";
						PrintOp(ctx, instr->Operands[1].Ptr());
						ctx.Body << L"] = ";
						PrintOp(ctx, instr->Operands[2].Ptr());
						ctx.Body << L";\n";
						ctx.SubstituteNames[instr->Name] = srcName;
						return;
					}
				}
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName << L" = ";
				PrintOp(ctx, instr->Operands[0].Ptr());
				ctx.Body << L";\n";
				ctx.Body << varName << L"[";
				PrintOp(ctx, instr->Operands[1].Ptr());
				ctx.Body << L"] = ";
				PrintOp(ctx, instr->Operands[2].Ptr());
				ctx.Body << L";\n";
			}

			void PrintInstrExpr(CodeGenContext & ctx, ILInstruction & instr)
			{
				if (auto binInstr = instr.As<BinaryInstruction>())
					PrintBinaryInstrExpr(ctx, binInstr);
				else if (auto unaryInstr = instr.As<UnaryInstruction>())
					PrintUnaryInstrExpr(ctx, unaryInstr);
				else if (auto gleaInstr = instr.As<GLeaInstruction>())
					PrintGleaInstrExpr(ctx, gleaInstr);
				else if (auto allocVar = instr.As<AllocVarInstruction>())
					PrintAllocVarInstrExpr(ctx, allocVar);
				else if (auto fetchArg = instr.As<FetchArgInstruction>())
					PrintFetchArgInstrExpr(ctx, fetchArg);
				else if (auto select = instr.As<SelectInstruction>())
					PrintSelectInstrExpr(ctx, select);
				else if (auto call = instr.As<CallInstruction>())
					PrintCallInstrExpr(ctx, call);
				else if (auto castf2i = instr.As<Float2IntInstruction>())
					PrintCastF2IInstrExpr(ctx, castf2i);
				else if (auto casti2f = instr.As<Int2FloatInstruction>())
					PrintCastI2FInstrExpr(ctx, casti2f);
				else if (auto update = instr.As<MemberUpdateInstruction>())
					throw InvalidOperationException(L"member update instruction cannot appear as expression.");
			}

			void PrintInstr(CodeGenContext & ctx, ILInstruction & instr)
			{
				// ctx.Body << L"// " << instr.ToString() << L";\n";
				if (!AppearAsExpression(instr, false))
				{
					if (auto binInstr = instr.As<BinaryInstruction>())
						PrintBinaryInstr(ctx, binInstr);
					else if (auto exportInstr = instr.As<ExportInstruction>())
						PrintExportInstr(ctx, exportInstr);
					else if (auto unaryInstr = instr.As<UnaryInstruction>())
						PrintUnaryInstr(ctx, unaryInstr);
					else if (auto gleaInstr = instr.As<GLeaInstruction>())
						PrintGleaInstr(ctx, gleaInstr);
					else if (auto allocVar = instr.As<AllocVarInstruction>())
						PrintAllocVarInstr(ctx, allocVar);
					else if (auto fetchArg = instr.As<FetchArgInstruction>())
						PrintFetchArgInstr(ctx, fetchArg);
					else if (auto select = instr.As<SelectInstruction>())
						PrintSelectInstr(ctx, select);
					else if (auto call = instr.As<CallInstruction>())
						PrintCallInstr(ctx, call);
					else if (auto castf2i = instr.As<Float2IntInstruction>())
						PrintCastF2IInstr(ctx, castf2i);
					else if (auto casti2f = instr.As<Int2FloatInstruction>())
						PrintCastI2FInstr(ctx, casti2f);
					else if (auto update = instr.As<MemberUpdateInstruction>())
						PrintUpdateInstr(ctx, update);
					else if (auto import = instr.As<ImportInstruction>())
						PrintImportInstr(ctx, import);
				}
			}

			void GenerateCode(CodeGenContext & context, CFGNode * code)
			{
				for (auto & instr : *code)
				{
					if (auto ifInstr = instr.As<IfInstruction>())
					{
						context.Body << L"if (bool(";
						PrintOp(context, ifInstr->Operand.Ptr(), true);
						context.Body << L"))\n{\n";
						GenerateCode(context, ifInstr->TrueCode.Ptr());
						context.Body << L"}\n";
						if (ifInstr->FalseCode)
						{
							context.Body << L"else\n{\n";
							GenerateCode(context, ifInstr->FalseCode.Ptr());
							context.Body << L"}\n";
						}
					}
					else if (auto forInstr = instr.As<ForInstruction>())
					{
						context.Body << L"for (;bool(";
						PrintOp(context, forInstr->ConditionCode->GetLastInstruction(), true);
						context.Body << L"); ";
						PrintOp(context, forInstr->SideEffectCode->GetLastInstruction(), true);
						context.Body << L")\n{\n";
						GenerateCode(context, forInstr->BodyCode.Ptr());
						context.Body << L"}\n";
					}
					else if (auto doInstr = instr.As<DoInstruction>())
					{
						context.Body << L"do\n{\n";
						GenerateCode(context, forInstr->BodyCode.Ptr());
						context.Body << L"} while (bool(";
						PrintOp(context, forInstr->ConditionCode->GetLastInstruction()->As<ReturnInstruction>()->Operand.Ptr(), true);
						context.Body << L"));\n";
					}
					else if (auto whileInstr = instr.As<WhileInstruction>())
					{
						context.Body << L"while (bool(";
						PrintOp(context, whileInstr->ConditionCode->GetLastInstruction()->As<ReturnInstruction>()->Operand.Ptr(), true);
						context.Body << L"))\n{\n";
						GenerateCode(context, whileInstr->BodyCode.Ptr());
						context.Body << L"}\n";
					}
					else if (auto ret = instr.As<ReturnInstruction>())
					{
						context.Body << L"return ";
						PrintOp(context, ret->Operand.Ptr());
						context.Body << L";\n";
					}
					else if (auto brk = instr.As<BreakInstruction>())
					{
						context.Body << L"break;\n";
					}
					else if (auto ctn = instr.As<ContinueInstruction>())
					{
						context.Body << L"continue;\n";
					}
					else
						PrintInstr(context, instr);
				}
			}
		public:
			virtual CompiledShaderSource GenerateShaderWorld(CompileResult & result, SymbolTable *, CompiledWorld * shaderWorld,
				Dictionary<String, ImportOperatorHandler *> & opHandlers,
				Dictionary<String, ExportOperatorHandler *> & exportHandlers) override
			{
				CompiledShaderSource rs;
				CodeGenContext context;
				context.Result = &result;
				context.GlobalHeader << L"#version 450\n#extension GL_ARB_bindless_texture: require\n#extension GL_NV_gpu_shader5 : require\n";
				if (useNVCommandList)
					context.GlobalHeader << L"#extension GL_NV_command_list: require\n";
				context.ImportOperatorHandlers = opHandlers;
				context.ExportOperatorHandlers = exportHandlers;
				StringBuilder prologBuilder, epilogBuilder;
				for (auto & inputBlock : shaderWorld->WorldInputs)
				{
					if (!inputBlock.Value.Block->UserWorlds.Contains(shaderWorld->WorldName))
						continue;
					String impOpName = inputBlock.Value.ImportOperator.Name.Content;
					ImportOperatorHandler * handler = nullptr;
					if (!opHandlers.TryGetValue(impOpName, handler))
						result.GetErrorWriter()->Error(40003, L"import operator handler for '" + impOpName
							+ L"' is not registered.", inputBlock.Value.ImportOperator.Position);
					else
					{
						StringBuilder inputDefSB;
						ImportOperatorContext opCtx(inputBlock.Value.ImportOperator.Arguments, backendArguments, shaderWorld, result,
							shaderWorld->Shader->Worlds[inputBlock.Value.ImportOperator.SourceWorld.Content].GetValue().Ptr());
						handler->GenerateInterfaceDefinition(inputDefSB, inputBlock.Value.Block, opCtx);
						handler->GeneratePreamble(prologBuilder, inputBlock.Value.Block, opCtx);
						handler->GenerateEpilogue(epilogBuilder, inputBlock.Value.Block, opCtx);
						rs.InputDeclarations[inputBlock.Key] = inputDefSB.ProduceString();
					}
				}
				ExportOperatorHandler * expHandler = nullptr;
				if (!exportHandlers.TryGetValue(shaderWorld->ExportOperator.Content, expHandler))
				{
					result.GetErrorWriter()->Error(40004, L"export operator handler for '" + shaderWorld->ExportOperator.Content
						+ L"' is not registered.", shaderWorld->ExportOperator.Position);
				}
				else
				{
					StringBuilder outputDefSB;
					expHandler->GenerateInterfaceDefinition(outputDefSB, shaderWorld->WorldOutput);
					expHandler->GeneratePreamble(prologBuilder, shaderWorld->WorldOutput);
					expHandler->GenerateEpilogue(epilogBuilder, shaderWorld->WorldOutput);
					rs.OutputDeclarations = outputDefSB.ProduceString();
				}
				currentWorld = shaderWorld;
				NamingCounter = 0;
				shaderWorld->Code->NameAllInstructions();
				GenerateCode(context, shaderWorld->Code.Ptr());
				rs.GlobalHeader = context.GlobalHeader.ProduceString();

				StringBuilder funcSB;
				for (auto funcName : shaderWorld->ReferencedFunctions)
				{
					for (auto &func : result.Program->Functions)
					{
						if (func->Name == funcName)
						{
							GenerateFunctionDeclaration(funcSB, func.Ptr());
							funcSB << L";\n";
						}
					}
				}
				for (auto funcName : shaderWorld->ReferencedFunctions)
				{
					for (auto &func : result.Program->Functions)
					{
						if (func->Name == funcName)
						{
							funcSB << GenerateFunction(func.Ptr());
						}
					}
				}
				rs.GlobalDefinitions = funcSB.ProduceString();
				rs.LocalDeclarations = prologBuilder.ProduceString() + context.Header.ProduceString();

				if (vertexOutputName.Length())
				{
					CompiledComponent ccomp;
					if (currentWorld->LocalComponents.TryGetValue(vertexOutputName, ccomp))
					{
						epilogBuilder << L"gl_Position = " << ccomp.CodeOperand->Name << L";\n";
					}
					else
					{
						result.GetErrorWriter()->Error(40005, L"cannot resolve '" + vertexOutputName
							+ L"' when generating code for world '" + currentWorld->WorldName + L"\'.", currentWorld->WorldDefPosition);
					}
				}

				for (auto & localComp : currentWorld->LocalComponents)
				{
					CodeGenContext nctx;
					PrintOp(nctx, localComp.Value.CodeOperand);
					rs.ComponentAccessNames[localComp.Key] = nctx.Body.ProduceString();
				}
				rs.MainCode = context.Body.ProduceString() + epilogBuilder.ProduceString();

				currentWorld = nullptr;
				return rs;
			}
			void GenerateFunctionDeclaration(StringBuilder & sbCode, CompiledFunction * function)
			{
				auto retType = function->ReturnType.Ptr();
				if (retType)
					PrintBaseType(sbCode, retType);
				else
					sbCode << L"void";
				sbCode << L" " << GetFuncOriginalName(function->Name) << L"(";
				int id = 0;
				for (auto & instr : *function->Code)
				{
					if (auto arg = instr.As<FetchArgInstruction>())
					{
						if (arg->ArgId != 0)
						{
							if (id > 0)
							{
								sbCode << L", ";
							}
							PrintDef(sbCode, arg->Type.Ptr(), arg->Name);
							id++;
						}
					}
				}
				sbCode << L")";
			}
			String GenerateFunction(CompiledFunction * function)
			{
				StringBuilder sbCode;
				CodeGenContext ctx;
				ctx.UsedVarNames.Clear();
				ctx.Body.Clear();
				ctx.Header.Clear();
				ctx.Arguments.Clear();
				ctx.ReturnVarName = L"";
				ctx.VarName.Clear();
				
				function->Code->NameAllInstructions();
				GenerateFunctionDeclaration(sbCode, function);
				sbCode << L"\n{\n";
				GenerateCode(ctx, function->Code.Ptr());
				sbCode << ctx.Header.ToString() << ctx.Body.ToString();
				if (ctx.ReturnVarName.Length())
					sbCode << L"return " << ctx.ReturnVarName << L";\n";
				sbCode << L"}\n";
				return sbCode.ProduceString();
			}
			EnumerableDictionary<String, String> backendArguments;
			virtual void SetParameters(EnumerableDictionary<String, String> & args)
			{
				backendArguments = args;
				if (!args.TryGetValue(L"vertex", vertexOutputName))
					vertexOutputName = L"";
				useNVCommandList = args.ContainsKey(L"command_list");
			}
		};

		CodeGenBackend * CreateGLSLCodeGen()
		{
			return new GLSLCodeGen();
		}
	}
}

/***********************************************************************
SPIRECORE\IL.CPP
***********************************************************************/

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::IO;

		ILBaseType ILBaseTypeFromString(String str)
		{
			if (str == L"int")
				return ILBaseType::Int;
			if (str == L"float")
				return ILBaseType::Float;
			if (str == L"vec2")
				return ILBaseType::Float2;
			if (str == L"vec3")
				return ILBaseType::Float3;
			if (str == L"vec4")
				return ILBaseType::Float4;
			if (str == L"ivec2")
				return ILBaseType::Int2;
			if (str == L"mat3")
				return ILBaseType::Float3x3;
			if (str == L"mat4")
				return ILBaseType::Float4x4;
			if (str == L"ivec3")
				return ILBaseType::Int3;
			if (str == L"ivec4")
				return ILBaseType::Int4;
			if (str == L"sampler2D")
				return ILBaseType::Texture2D;
			if (str == L"sampler2DShadow")
				return ILBaseType::TextureShadow;
			if (str == L"samplerCube")
				return ILBaseType::TextureCube;
			if (str == L"samplerCubeShadow")
				return ILBaseType::TextureCubeShadow;
			return ILBaseType::Int;
		}

		int RoundToAlignment(int offset, int alignment)
		{
			int remainder = offset % alignment;
			if (remainder == 0)
				return offset;
			else
				return offset + (alignment - remainder);
		}

		int SizeofBaseType(ILBaseType type)
		{
			if (type == ILBaseType::Int)
				return 4;
			else if (type == ILBaseType::Int2)
				return 8;
			else if (type == ILBaseType::Int3)
				return 12;
			else if (type == ILBaseType::Int4)
				return 16;
			else if (type == ILBaseType::Float)
				return 4;
			else if (type == ILBaseType::Float2)
				return 8;
			else if (type == ILBaseType::Float3)
				return 12;
			else if (type == ILBaseType::Float4)
				return 16;
			else if (type == ILBaseType::Float3x3)
				return 48;
			else if (type == ILBaseType::Float4x4)
				return 64;
			else if (type == ILBaseType::Texture2D)
				return 8;
			else if (type == ILBaseType::TextureCube)
				return 8;
			else if (type == ILBaseType::TextureCubeShadow)
				return 8;
			else if (type == ILBaseType::TextureShadow)
				return 8;
			else
				return 0;
		}

		int AlignmentOfBaseType(ILBaseType type)
		{
			if (type == ILBaseType::Int)
				return 4;
			else if (type == ILBaseType::Int2)
				return 8;
			else if (type == ILBaseType::Int3)
				return 16;
			else if (type == ILBaseType::Int4)
				return 16;
			else if (type == ILBaseType::Float)
				return 4;
			else if (type == ILBaseType::Float2)
				return 8;
			else if (type == ILBaseType::Float3)
				return 16;
			else if (type == ILBaseType::Float4)
				return 16;
			else if (type == ILBaseType::Float3x3)
				return 16;
			else if (type == ILBaseType::Float4x4)
				return 16;
			else if (type == ILBaseType::Texture2D)
				return 8;
			else if (type == ILBaseType::TextureCube)
				return 8;
			else if (type == ILBaseType::TextureCubeShadow)
				return 8;
			else if (type == ILBaseType::TextureShadow)
				return 8;
			else
				return 0;
		}

		String ILBaseTypeToString(ILBaseType type)
		{
			if (type == ILBaseType::Int)
				return L"int";
			else if (type == ILBaseType::Int2)
				return L"ivec2";
			else if (type == ILBaseType::Int3)
				return L"ivec3";
			else if (type == ILBaseType::Int4)
				return L"ivec4";
			else if (type == ILBaseType::Float)
				return L"float";
			else if (type == ILBaseType::Float2)
				return L"vec2";
			else if (type == ILBaseType::Float3)
				return L"vec3";
			else if (type == ILBaseType::Float4)
				return L"vec4";
			else if (type == ILBaseType::Float3x3)
				return L"mat3";
			else if (type == ILBaseType::Float4x4)
				return L"mat4";
			else if (type == ILBaseType::Texture2D)
				return L"sampler2D";
			else if (type == ILBaseType::TextureCube)
				return L"samplerCube";
			else if (type == ILBaseType::TextureCubeShadow)
				return L"samplerCubeShadow";
			else if (type == ILBaseType::TextureShadow)
				return L"sampler2DShadow";
			else
				return L"?unkown";
		}

		bool ILType::IsInt()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Int;
			else
				return false;
		}

		bool ILType::IsFloat()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Float;
			else
				return false;
		}

		bool ILType::IsIntVector()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Int2 || basicType->Type == ILBaseType::Int3 || basicType->Type == ILBaseType::Int4;
			else
				return false;
		}

		bool ILType::IsFloatVector()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Float2 || basicType->Type == ILBaseType::Float3 || basicType->Type == ILBaseType::Float4 ||
					basicType->Type == ILBaseType::Float3x3 || basicType->Type == ILBaseType::Float4x4;
			else
				return false;
		}

		bool ILType::IsFloatMatrix()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Float3x3 || basicType->Type == ILBaseType::Float4x4;
			else
				return false;
		}

		bool ILType::IsNonShadowTexture()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Texture2D || basicType->Type == ILBaseType::TextureCube;
			else
				return false;
		}

		bool ILType::IsTexture()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Texture2D || basicType->Type == ILBaseType::TextureCube || basicType->Type == ILBaseType::TextureCubeShadow ||
				basicType->Type == ILBaseType::TextureShadow;
			else
				return false;
		}

		int ILType::GetVectorSize()
		{
			if (auto basicType = dynamic_cast<ILBasicType*>(this))
			{
				switch (basicType->Type)
				{
				case ILBaseType::Int2:
				case ILBaseType::Float2:
					return 2;
				case ILBaseType::Int3:
				case ILBaseType::Float3:
					return 3;
				case ILBaseType::Int4:
				case ILBaseType::Float4:
					return 4;
				case ILBaseType::Float3x3:
					return 9;
				case ILBaseType::Float4x4:
					return 16;
				default:
					return 1;
				}
			}
			return 1;
		}

		bool CFGNode::HasPhiInstruction()
		{
			return headInstr && headInstr->GetNext() && headInstr->GetNext()->Is<PhiInstruction>();
		}

		ILInstruction * CFGNode::GetFirstNonPhiInstruction()
		{
			for (auto & instr : *this)
			{
				if (!instr.Is<PhiInstruction>())
					return &instr;
			}
			return tailInstr;
		}

		int NamingCounter = 0;

		void CFGNode::NameAllInstructions()
		{
			// name all operands
			StringBuilder numBuilder;
			for (auto & instr : GetAllInstructions())
			{
				numBuilder.Clear();
				for (auto & c : instr.Name)
				{
					if (c >= L'0' && c <= '9')
						numBuilder.Append(c);
					else
						numBuilder.Clear();
				}
				auto num = numBuilder.ToString();
				if (num.Length())
				{
					int id = StringToInt(num);
					NamingCounter = Math::Max(NamingCounter, id + 1);
				}
			}
			for (auto & instr : GetAllInstructions())
			{
				if (instr.Name.Length() == 0)
					instr.Name = String(L"t") + String(NamingCounter++, 16);
			}
		}

		void CFGNode::DebugPrint()
		{
			printf("===========\n");
			for (auto& instr : *this)
			{
				printf("%s\n", instr.ToString().ToMultiByteString());
			}
			printf("===========\n");
		}

		LoadInstruction::LoadInstruction(ILOperand * dest)
		{
			Deterministic = false;
			Operand = dest;
			Type = dest->Type->Clone();
			if (!Spire::Compiler::Is<AllocVarInstruction>(dest) && !Spire::Compiler::Is<GLeaInstruction>(dest) && !Spire::Compiler::Is<FetchArgInstruction>(dest))
				throw L"invalid address operand";
		}
		void MemberUpdateInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitMemberUpdateInstruction(this);
		}
		void SubInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitSubInstruction(this);
		}
		void MulInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitMulInstruction(this);
		}
		void DivInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitDivInstruction(this);
		}
		void ModInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitModInstruction(this);
		}
		void AndInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitAndInstruction(this);
		}
		void OrInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitOrInstruction(this);
		}
		void BitAndInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitBitAndInstruction(this);
		}
		void BitOrInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitBitOrInstruction(this);
		}
		void BitXorInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitBitXorInstruction(this);
		}
		void ShlInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitShlInstruction(this);
		}
		void ShrInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitShrInstruction(this);
		}
		void CmpgtInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCmpgtInstruction(this);
		}
		void CmpgeInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCmpgeInstruction(this);
		}
		void CmpltInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCmpltInstruction(this);
		}
		void CmpleInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCmpleInstruction(this);
		}
		void CmpeqlInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCmpeqlInstruction(this);
		}
		void CmpneqInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCmpneqInstruction(this);
		}
		void Float2IntInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitFloat2IntInstruction(this);
		}
		void Int2FloatInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitInt2FloatInstruction(this);
		}
		void CopyInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCopyInstruction(this);
		}
		void LoadInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitLoadInstruction(this);
		}
		void StoreInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitStoreInstruction(this);
		}
		void GLeaInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitGLeaInstruction(this);
		}
		void AllocVarInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitAllocVarInstruction(this);
		}
		void FetchArgInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitFetchArgInstruction(this);
		}
		void PhiInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitPhiInstruction(this);
		}
		void SelectInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitSelectInstruction(this);
		}
		void CallInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCallInstruction(this);
		}
		void SwitchInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitSwitchInstruction(this);
		}
		void NotInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitNotInstruction(this);
		}
		void NegInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitNegInstruction(this);
		}
		void BitNotInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitBitNotInstruction(this);
		}
		void AddInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitAddInstruction(this);
		}
		void MemberLoadInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitMemberLoadInstruction(this);
		}
		AllInstructionsIterator & AllInstructionsIterator::operator++()
		{
			bool done = false;
			do
			{
				done = true;
				if (subBlockPtr < curInstr->GetSubBlockCount())
				{
					StackItem item;
					item.instr = curInstr;
					item.subBlockPtr = subBlockPtr + 1;
					stack.Add(item);
					curInstr = curInstr->GetSubBlock(subBlockPtr)->begin().Current;
					subBlockPtr = 0;
				}
				else
					curInstr = curInstr->GetNext();
				while (curInstr->GetNext() == nullptr && stack.Count() > 0)
				{
					auto item = stack.Last();
					stack.RemoveAt(stack.Count() - 1);
					curInstr = item.instr;
					subBlockPtr = item.subBlockPtr;
					if (subBlockPtr >= curInstr->GetSubBlockCount())
					{
						subBlockPtr = 0;
						curInstr = curInstr->GetNext();
					}
					done = false;
				}
				if (curInstr->GetNext() == nullptr)
					break;
			} while (!done);

			return *this;
		}
		AllInstructionsIterator AllInstructionsCollection::begin()
		{
			return AllInstructionsIterator(node->begin().Current);
		}
		AllInstructionsIterator AllInstructionsCollection::end()
		{
			return AllInstructionsIterator(node->end().Current);
		}
		String ImportInstruction::ToString()
		{
			StringBuilder rs;
			rs << Name << L" = import<" << ImportOperator->Name.Content << ">[" << ComponentName << L"@" << SourceWorld->WorldName << L"](";
			for (auto & arg : Arguments)
			{
				rs << arg->ToString() << L", ";
			}
			rs << L")";
			return rs.ProduceString();
		}
		String ImportInstruction::GetOperatorString()
		{
			return L"import<" + ImportOperator->Name.Content + L">";
		}
		void ImportInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitImportInstruction(this);
		}
		void ExportInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitExportInstruction(this);
		}
}
}

/***********************************************************************
SPIRECORE\KEYHOLEMATCHING.CPP
***********************************************************************/

namespace Spire
{
	namespace Compiler
	{
		RefPtr<KeyHoleNode> ParseInternal(CoreLib::Text::Parser & parser)
		{
			RefPtr<KeyHoleNode> result = new KeyHoleNode();
			result->NodeType = parser.ReadWord();
			if (parser.LookAhead(L"<"))
			{
				parser.ReadToken();
				result->CaptureId = parser.ReadInt();
				parser.ReadToken();
			}
			if (parser.LookAhead(L"("))
			{
				while (!parser.LookAhead(L")"))
				{
					result->Children.Add(ParseInternal(parser));
					if (parser.LookAhead(L","))
						parser.ReadToken();
					else
					{
						break;
					}
				}
				parser.Read(L")");
			}
			return result;
		}

		RefPtr<KeyHoleNode> KeyHoleNode::Parse(String format)
		{
			CoreLib::Text::Parser parser(format);
			return ParseInternal(parser);
		}

		bool KeyHoleNode::Match(List<ILOperand*> & matchResult, ILOperand * instr)
		{
			bool matches = false;
			if (NodeType == L"store")
				matches = dynamic_cast<StoreInstruction*>(instr) != nullptr;
			else if (NodeType == L"op")
				matches = true;
			else if (NodeType == L"load")
				matches = dynamic_cast<LoadInstruction*>(instr) != nullptr;
			else if (NodeType == L"glea")
				matches = dynamic_cast<GLeaInstruction*>(instr) != nullptr;
			else if (NodeType == L"add")
				matches = dynamic_cast<AddInstruction*>(instr) != nullptr;
			else if (NodeType == L"mul")
				matches = dynamic_cast<MulInstruction*>(instr) != nullptr;
			else if (NodeType == L"sub")
				matches = dynamic_cast<SubInstruction*>(instr) != nullptr;
			else if (NodeType == L"call")
				matches = dynamic_cast<CallInstruction*>(instr) != nullptr;
			else if (NodeType == L"switch")
				matches = dynamic_cast<SwitchInstruction*>(instr) != nullptr;
			if (matches)
			{
				if (Children.Count() > 0)
				{
					ILInstruction * cinstr = dynamic_cast<ILInstruction*>(instr);
					if (cinstr != nullptr)
					{
						int opId = 0;
						for (auto & op : *cinstr)
						{
							if (opId >= Children.Count())
							{
								matches = false;
								break;
							}
							matches = matches && Children[opId]->Match(matchResult, &op);
							opId++;
						}
						if (opId != Children.Count())
							matches = false;
					}
					else
						matches = false;
				}
			}
			if (matches && CaptureId != -1)
			{
				matchResult.SetSize(Math::Max(matchResult.Count(), CaptureId + 1));
				matchResult[CaptureId] = instr;
			}
			return matches;
		}
	}
}

/***********************************************************************
SPIRECORE\LEXER.CPP
***********************************************************************/

namespace Spire
{
	namespace Compiler
	{
		enum class State
		{
			Start, Identifier, Operator, Int, Fixed, Double, Char, String, MultiComment, SingleComment
		};

		bool IsLetter(wchar_t ch)
		{
			return (ch >= L'a' && ch <= L'z'||
				ch >= L'A' && ch <= L'Z' || ch == L'_' || ch == L'#');
		}

		bool IsDigit(wchar_t ch)
		{
			return ch >= L'0' && ch <= L'9';
		}

		bool IsPunctuation(wchar_t ch)
		{
			return  ch == L'+' || ch == L'-' || ch == L'*' || ch == L'/' || ch == L'%' ||
					ch == L'!' || ch == L'^' || ch == L'&' || ch == L'(' || ch == L')' ||
					ch == L'=' || ch == L'{' || ch == L'}' || ch == L'[' || ch == L']' ||
					ch == L'|' || ch == L';' || ch == L',' || ch == L'.' || ch == L'<' ||
					ch == L'>' || ch == L'~' || ch == L'@' || ch == L':';
		}

		TokenType GetKeywordTokenType(const String & str)
		{
			if (str == L"return")
				return TokenType::KeywordReturn;
			else if (str == L"break")
				return TokenType::KeywordBreak;
			else if (str == L"continue")
				return TokenType::KeywordContinue;
			else if (str == L"if")
				return TokenType::KeywordIf;
			else if (str == L"else")
				return TokenType::KeywordElse;
			else if (str == L"for")
				return TokenType::KeywordFor;
			else if (str == L"while")
				return TokenType::KeywordWhile;
			else if (str == L"do")
				return TokenType::KeywordDo;
			else
				return TokenType::Identifier;
		}

		void ParseOperators(const String & str, List<Token> & tokens, int line, int col, String fileName)
		{
			int pos = 0;
			while (pos < str.Length())
			{
				wchar_t curChar = str[pos];
				wchar_t nextChar = (pos < str.Length()-1)? str[pos + 1] : L'\0';
				auto InsertToken = [&](TokenType type, const String & ct)
				{
					tokens.Add(Token(type, ct, line, col + pos, fileName));
				};
				switch(curChar)
				{
				case L'+':
					if (nextChar == L'+')
					{
						InsertToken(TokenType::OpInc, L"++");
						pos += 2;
					}
					else if (nextChar == L'=')
					{
						InsertToken(TokenType::OpAddAssign, L"+=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpAdd, L"+");
						pos++;
					}
					break;
				case L'-':
					if (nextChar == L'-')
					{
						InsertToken(TokenType::OpDec, L"--");
						pos += 2;
					}
					else if (nextChar == L'=')
					{
						InsertToken(TokenType::OpSubAssign, L"-=");
						pos += 2;
					}
					else if (nextChar == L'>')
					{
						InsertToken(TokenType::RightArrow, L"->");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpSub, L"-");
						pos++;
					}
					break;
				case L'*':
					if (nextChar == L'=')
					{
						InsertToken(TokenType::OpMulAssign, L"*=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpMul, L"*");
						pos++;
					}
					break;
				case L'/':
					if (nextChar == L'=')
					{
						InsertToken(TokenType::OpDivAssign, L"/=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpDiv, L"/");
						pos++;
					}
					break;
				case L'%':
					if (nextChar == L'=')
					{
						InsertToken(TokenType::OpModAssign, L"%=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpMod, L"%");
						pos++;
					}
					break;
				case L'|':
					if (nextChar == L'|')
					{
						InsertToken(TokenType::OpOr, L"||");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpBitOr, L"|");
						pos++;
					}
					break;
				case L'&':
					if (nextChar == L'&')
					{
						InsertToken(TokenType::OpAnd, L"&&");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpBitAnd, L"&");
						pos++;
					}
					break;
				case L'>':
					if (nextChar == L'>')
					{
						InsertToken(TokenType::OpRsh, L">>");
						pos += 2;
					}
					else if (nextChar == L'=')
					{
						InsertToken(TokenType::OpGeq, L">=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpGreater, L">");
						pos++;
					}
					break;
				case L'<':
					if (nextChar == L'<')
					{
						InsertToken(TokenType::OpLsh, L"<<");
						pos += 2;
					}
					else if (nextChar == L'=')
					{
						InsertToken(TokenType::OpLeq, L"<=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpLess, L"<");
						pos++;
					}
					break;
				case L'=':
					if (nextChar == L'=')
					{
						InsertToken(TokenType::OpEql, L"==");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpAssign, L"=");
						pos++;
					}
					break;
				case L'!':
					if (nextChar == L'=')
					{
						InsertToken(TokenType::OpNeq, L"!=");
						pos += 2;
					}
					else
					{
						InsertToken(TokenType::OpNot, L"!");
						pos++;
					}
					break;
				case L'?':
					InsertToken(TokenType::QuestionMark, L"?");
					pos++;
					break;
				case L'@':
					InsertToken(TokenType::At, L"@");
					pos++;
					break;
				case L':':
					InsertToken(TokenType::Colon, L":");
					pos++;
					break;
				case L'~':
					InsertToken(TokenType::OpBitNot, L"~");
					pos++;
					break;
				case L';':
					InsertToken(TokenType::Semicolon, L";");
					pos++;
					break;
				case L',':
					InsertToken(TokenType::Comma, L","); 
					pos++;
					break;
				case L'.':
					InsertToken(TokenType::Dot, L".");
					pos++;
					break;
				case L'{':
					InsertToken(TokenType::LBrace, L"{"); 
					pos++;
					break;
				case L'}':
					InsertToken(TokenType::RBrace, L"}"); 
					pos++;
					break;
				case L'[':
					InsertToken(TokenType::LBracket, L"["); 
					pos++;
					break;
				case L']':
					InsertToken(TokenType::RBracket, L"]"); 
					pos++;
					break;
				case L'(':
					InsertToken(TokenType::LParent, L"("); 
					pos++;
					break;
				case L')':
					InsertToken(TokenType::RParent, L")"); 
					pos++;
					break;
				}
			}
		}

		List<Token> Lexer::Parse(const String & fileName, const String & str, List<CompileError> & errorList)
		{
			int lastPos = 0, pos = 0;
			int line = 1, col = 0;
			State state = State::Start;
			StringBuilder tokenBuilder;
			int tokenLine, tokenCol;
			List<Token> tokenList;
			auto InsertToken = [&](TokenType type)
			{
				tokenList.Add(Token(type, tokenBuilder.ToString(), tokenLine, tokenCol, fileName));
				tokenBuilder.Clear();
			};
			auto ProcessTransferChar = [&](wchar_t nextChar)
			{
				switch(nextChar)
				{
				case L'\\':
				case L'\"':
				case L'\'':
					tokenBuilder.Append(nextChar);
					break;
				case L't':
					tokenBuilder.Append('\t');
					break;
				case L's':
					tokenBuilder.Append(' ');
					break;
				case L'n':
					tokenBuilder.Append('\n');
					break;
				case L'r':
					tokenBuilder.Append('\r');
					break;
				case L'b':
					tokenBuilder.Append('\b');
					break;
				}
			};
			while (pos <= str.Length())
			{
				wchar_t curChar = (pos < str.Length()?str[pos]:L' ');
				wchar_t nextChar = (pos < str.Length()-1)? str[pos + 1] : L'\0';
				if (lastPos != pos)
				{
					if (curChar == L'\n')
					{
						line++;
						col = 0;
					}
					else
						col++;
					lastPos = pos;
				}

				switch (state)
				{
				case State::Start:
					if (IsLetter(curChar))
					{
						state = State::Identifier;
						tokenLine = line;
						tokenCol = col;
					}
					else if (IsDigit(curChar))
					{
						state = State::Int;
						tokenLine = line;
						tokenCol = col;
					}
					else if (curChar == L'\'')
					{
						state = State::Char;
						pos++;
						tokenLine = line;
						tokenCol = col;
					}
					else if (curChar == L'"')
					{
						state = State::String;
						pos++;
						tokenLine = line;
						tokenCol = col;
					}
					else if (curChar == L' ' || curChar == L'\t' || curChar == L'\r' || curChar == L'\n')
						pos++;
					else if (curChar == L'/' && nextChar == L'/')
					{
						state = State::SingleComment;
						pos += 2;
					}
					else if (curChar == L'/' && nextChar == L'*')
					{
						pos += 2;
						state = State::MultiComment;
					}
					else if (IsPunctuation(curChar))
					{
						state = State::Operator;
						tokenLine = line;
						tokenCol = col;
					}
					else
					{
						errorList.Add(CompileError(L"Illegal character '" + String(curChar) + L"'", 10000, CodePosition(line, col, fileName)));
						pos++;
					}
					break;
				case State::Identifier:
					if (IsLetter(curChar) || IsDigit(curChar))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else
					{
						auto tokenStr = tokenBuilder.ToString();
						if (tokenStr == L"#line_reset#")
						{
							line = 0;
							col = 0;
							tokenBuilder.Clear();
						}
						else
							InsertToken(GetKeywordTokenType(tokenStr));
						state = State::Start;
					}
					break;
				case State::Operator:
					if (IsPunctuation(curChar) && !(curChar == L'/' && nextChar == L'/' || curChar == L'/' && nextChar == L'*'))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else
					{
						//do token analyze
						ParseOperators(tokenBuilder.ToString(), tokenList, tokenLine, tokenCol, fileName);
						tokenBuilder.Clear();
						state = State::Start;
					}
					break;
				case State::Int:
					if (IsDigit(curChar))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else if (curChar == L'.')
					{
						state = State::Fixed;
						tokenBuilder.Append(curChar);
						pos++;
					}
					else if (curChar == L'e' || curChar == L'E')
					{
						state = State::Double;
						tokenBuilder.Append(curChar);
						if (nextChar == L'-' || nextChar == L'+')
						{
							tokenBuilder.Append(nextChar);
							pos++;
						}
						pos++;
					}
					else
					{
						InsertToken(TokenType::IntLiterial);
						state = State::Start;
					}
					break;
				case State::Fixed:
					if (IsDigit(curChar))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else if (curChar == L'e' || curChar == L'E')
					{
						state = State::Double;
						tokenBuilder.Append(curChar);
						if (nextChar == L'-' || nextChar == L'+')
						{
							tokenBuilder.Append(nextChar);
							pos++;
						}
						pos++;
					}
					else
					{
						if (curChar == L'f')
							pos++;
						InsertToken(TokenType::DoubleLiterial);
						state = State::Start;
					}
					break;
				case State::Double:
					if (IsDigit(curChar))
					{
						tokenBuilder.Append(curChar);
						pos++;
					}
					else
					{
						if (curChar == L'f')
							pos++;
						InsertToken(TokenType::DoubleLiterial);
						state = State::Start;
					}
					break;
				case State::String:
					if (curChar != L'"')
					{
						if (curChar == L'\\')
						{
							ProcessTransferChar(nextChar);
							pos++;
						}
						else
							tokenBuilder.Append(curChar);
					}
					else
					{
						InsertToken(TokenType::StringLiterial);
						state = State::Start;
					}
					pos++;
					break;
				case State::Char:
					if (curChar != L'\'')
					{
						if (curChar == L'\\')
						{
							ProcessTransferChar(nextChar);
							pos++;
						}
						else
							tokenBuilder.Append(curChar);
					}
					else
					{
						if (tokenBuilder.Length() > 1)
							errorList.Add(CompileError(L"Illegal character literial.", 10001, CodePosition(line, col-tokenBuilder.Length(), fileName)));
						InsertToken(TokenType::CharLiterial);
						state = State::Start;
					}
					pos++;
					break;
				case State::SingleComment:
					if (curChar == L'\n')
						state = State::Start;
					pos++;
					break;
				case State::MultiComment:
					if (curChar == L'*' && nextChar == '/')
					{
						state = State::Start;
						pos += 2;
					}
					else
						pos++;
					break;
				}
			}
			return tokenList;
		}

		String TokenTypeToString(TokenType type)
		{
			switch (type)
			{
			case TokenType::Unkown:
				return L"UnknownToken";
			case TokenType::Identifier:
				return L"Identifier";

			case TokenType::KeywordReturn:
				return L"\"return\"";
			case TokenType::KeywordBreak:
				return L"\"break\"";
			case TokenType::KeywordContinue:
				return L"\"continue\"";
			case TokenType::KeywordIf:
				return L"\"if\"";
			case TokenType::KeywordElse:
				return L"\"else\"";
			case TokenType::KeywordFor:
				return L"\"for\"";
			case TokenType::KeywordWhile:
				return L"\"while\"";
			case TokenType::KeywordDo:
				return L"\"do\"";
			case TokenType::IntLiterial:
				return L"Int Literial";
			case TokenType::DoubleLiterial:
				return L"Double Literial";
			case TokenType::StringLiterial:
				return L"String Literial";
			case TokenType::CharLiterial:
				return L"CharLiterial";
			case TokenType::QuestionMark:
				return L"'?'";
			case TokenType::Colon:
				return L"':'";
			case TokenType::Semicolon:
				return L"';'";
			case TokenType::Comma:
				return L"','";
			case TokenType::LBrace:
				return L"'{'";
			case TokenType::RBrace:
				return L"'}'";
			case TokenType::LBracket:
				return L"'['";
			case TokenType::RBracket:
				return L"']'";
			case TokenType::LParent:
				return L"'('";
			case TokenType::RParent:
				return L"')'";
			case TokenType::At:
				return L"'@'";
			case TokenType::OpAssign:
				return L"'='";
			case TokenType::OpAdd:
				return L"'+'";
			case TokenType::OpSub:
				return L"'-'";
			case TokenType::OpMul:
				return L"'*'";
			case TokenType::OpDiv:
				return L"'/'";
			case TokenType::OpMod:
				return L"'%'";
			case TokenType::OpNot:
				return L"'!'";
			case TokenType::OpLsh:
				return L"'<<'";
			case TokenType::OpRsh:
				return L"'>>'";
			case TokenType::OpAddAssign:
				return L"'+='";
			case TokenType::OpSubAssign:
				return L"'-='";
			case TokenType::OpMulAssign:
				return L"'*='";
			case TokenType::OpDivAssign:
				return L"'/='";
			case TokenType::OpModAssign:
				return L"'%='";
			case TokenType::OpEql:
				return L"'=='";
			case TokenType::OpNeq:
				return L"'!='";
			case TokenType::OpGreater:
				return L"'>'";
			case TokenType::OpLess:
				return L"'<'";
			case TokenType::OpGeq:
				return L"'>='";
			case TokenType::OpLeq:
				return L"'<='";
			case TokenType::OpAnd:
				return L"'&&'";
			case TokenType::OpOr:
				return L"'||'";
			case TokenType::OpBitXor:
				return L"'^'";
			case TokenType::OpBitAnd:
				return L"'&'";
			case TokenType::OpBitOr:
				return L"'|'";
			case TokenType::OpInc:
				return L"'++'";
			case TokenType::OpDec:
				return L"'--'";
			default:
				return L"";
			}
		}
		
	}
}

/***********************************************************************
SPIRECORE\PARSER.CPP
***********************************************************************/

namespace Spire
{
	namespace Compiler
	{
		Token & Parser::ReadToken(const wchar_t * string)
		{
			if (pos >= tokens.Count())
			{
				errors.Add(CompileError(String(L"\"") + string + String(L"\" expected but end of file encountered."), 20001, CodePosition(0, 0, fileName)));
				throw 0;
			}
			else if (tokens[pos].Content != string)
			{
				errors.Add(CompileError(String(L"\"") + string + String(L"\" expected"), 20001, tokens[pos].Position));
				throw 20001;
			}
			return tokens[pos++];
		}

		Token & Parser::ReadToken(TokenType type)
		{
			if (pos >= tokens.Count())
			{
				errors.Add(CompileError(TokenTypeToString(type) + String(L" expected but end of file encountered."), 20001, CodePosition(0, 0, fileName)));
				throw 0;
			}
			else if(tokens[pos].Type != type)
			{
				errors.Add(CompileError(TokenTypeToString(type) + String(L" expected"), 20001, tokens[pos].Position));
				throw 20001;
			}
			return tokens[pos++];
		}

		bool Parser::LookAheadToken(const wchar_t * string, int offset)
		{
			if (pos + offset >= tokens.Count())
			{
				errors.Add(CompileError(String(L"\'") + string + String(L"\' expected but end of file encountered."), 20001, CodePosition(0, 0, fileName)));
				return false;
			}
			else
			{
				if (tokens[pos + offset].Content == string)
					return true;
				else
					return false;
			}
		}

		bool Parser::LookAheadToken(TokenType type, int offset)
		{
			if (pos + offset >= tokens.Count())
			{
				errors.Add(CompileError(TokenTypeToString(type) + String(L" expected but end of file encountered."), 20001, CodePosition(0, 0, fileName)));
				return false;
			}
			else
			{
				if(tokens[pos + offset].Type == type)
					return true;
				else
					return false;
			}
		}

		Token & Parser::ReadTypeKeyword()
		{
			if (pos >= tokens.Count())
			{
				errors.Add(CompileError(String(L"type name expected but end of file encountered."), 20001, CodePosition(0, 0, fileName)));
				throw 0;
			}
			if(!IsTypeKeyword())
			{
				errors.Add(CompileError(String(L"type name expected but '" + tokens[pos].Content + L"' encountered."), 20001, tokens[pos].Position));
				throw 20001;
			}
			return tokens[pos++];
		}

		bool Parser::IsTypeKeyword()
		{
			if (pos >= tokens.Count())
			{
				errors.Add(CompileError(String(L"Unexpected end of file."), 20001, tokens[pos].Position));
				throw 0;
			}

			return typeNames.Contains(tokens[pos].Content);
		}

		RefPtr<ProgramSyntaxNode> Parser::Parse()
		{
			return ParseProgram();
		}

		EnumerableDictionary<String, String> Parser::ParseAttribute()
		{
			EnumerableDictionary<String, String> rs;
			while (LookAheadToken(TokenType::LBracket))
			{
				ReadToken(TokenType::LBracket);
				auto name = ReadToken(TokenType::Identifier).Content;
				String value;
				if (LookAheadToken(L":"))
				{
					ReadToken(L":");
					value = ReadToken(TokenType::StringLiterial).Content;
				}
				rs[name] = value;
				ReadToken(TokenType::RBracket);
			}
			return rs;
		}

		RefPtr<ProgramSyntaxNode> Parser::ParseProgram()
		{
			scopeStack.Add(new Scope());
			RefPtr<ProgramSyntaxNode> program = new ProgramSyntaxNode();
			program->Position = CodePosition(0, 0, fileName);
			program->Scope = scopeStack.Last();
			try
			{
				int lastPosBeforeError = 0;
				while (pos < tokens.Count())
				{
					try
					{
						if (LookAheadToken(L"shader") || LookAheadToken(L"module"))
							program->Shaders.Add(ParseShader());
						else if (LookAheadToken(L"pipeline"))
							program->Pipelines.Add(ParsePipeline());
						else if (LookAheadToken(L"using"))
						{
							ReadToken(L"using");
							program->Usings.Add(ReadToken(TokenType::StringLiterial));
							ReadToken(TokenType::Semicolon);
						}
						else if (IsTypeKeyword() || LookAheadToken(L"inline") || LookAheadToken(L"extern")
							|| LookAheadToken(L"__intrinsic"))
							program->Functions.Add(ParseFunction());
						else if (LookAheadToken(TokenType::Semicolon))
							ReadToken(TokenType::Semicolon);
						else
						{
							if (lastPosBeforeError == 0 && pos < tokens.Count())
								errors.Add(CompileError(L"unexpected token \'" + tokens[pos].Content + L"\'.", 20003, tokens[pos].Position));
							throw 0;
						}
					}
					catch (int)
					{
						if (pos == lastPosBeforeError)
							pos++;
						lastPosBeforeError = pos;
					}
				}
			}
			catch(int)
			{}
			scopeStack.Clear();
			return program;
		}

		RefPtr<ShaderSyntaxNode> Parser::ParseShader()
		{
			RefPtr<ShaderSyntaxNode> shader = new ShaderSyntaxNode();
			if (LookAheadToken(L"module"))
			{
				shader->IsModule = true;
				ReadToken(L"module");
			}
			else
				ReadToken(L"shader");
			PushScope();
			FillPosition(shader.Ptr());
			shader->Name = ReadToken(TokenType::Identifier);
			try
			{
				if (LookAheadToken(L":"))
				{
					ReadToken(L":");
					shader->Pipeline = ReadToken(TokenType::Identifier);
				}
			}
			catch (int)
			{
			}
			
			ReadToken(TokenType::LBrace);
			int lastErrorPos = 0;
			while (!LookAheadToken(TokenType::RBrace))
			{
				try
				{
					if (LookAheadToken(L"inline") || (LookAheadToken(L"public") && !LookAheadToken(L"using", 1)) ||
						LookAheadToken(L"out") || LookAheadToken(L"@") || IsTypeKeyword()
						|| LookAheadToken(L"[") || LookAheadToken(L"require"))
						shader->Members.Add(ParseComponent());
					else if (LookAheadToken(L"using") || LookAheadToken(L"public") && LookAheadToken(L"using", 1))
					{
						shader->Members.Add(ParseImport());
					}
					else
					{
						if (lastErrorPos == 0 && pos < tokens.Count())
							errors.Add(CompileError(L"unexpected token \'" + tokens[pos].Content + L"\', only component definitions are allowed in a shader scope.", 
								20004, tokens[pos].Position));
						throw 0;
					}
				}
				catch (int)
				{
					if (pos == lastErrorPos)
						pos++;
					lastErrorPos = pos;
				}
			}
			ReadToken(TokenType::RBrace);
			
			PopScope();
			return shader;
		}

		RefPtr<PipelineSyntaxNode> Parser::ParsePipeline()
		{
			RefPtr<PipelineSyntaxNode> pipeline = new PipelineSyntaxNode();
			ReadToken(L"pipeline");
			FillPosition(pipeline.Ptr());
			pipeline->Name = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::LBrace);
			while (!LookAheadToken(TokenType::RBrace))
			{
				auto attribs = ParseAttribute();
				if (LookAheadToken(L"abstract"))
				{
					pos++;
					bool isWorld = LookAheadToken(L"world");
					pos--;
					if (isWorld)
					{
						auto w = ParseWorld();
						w->LayoutAttributes = attribs;
						pipeline->Worlds.Add(w);
					}
					else
					{
						auto comp = ParseComponent();
						comp->LayoutAttributes = attribs;
						pipeline->AbstractComponents.Add(comp);
					}
				}
				else if (LookAheadToken(L"world"))
				{
					auto w = ParseWorld();
					w->LayoutAttributes = attribs;
					pipeline->Worlds.Add(w);
				}
				else if (LookAheadToken(L"import"))
				{
					auto op = ParseImportOperator();
					op->LayoutAttributes = attribs;
					pipeline->ImportOperators.Add(op);
				}
				else
				{
					auto comp = ParseComponent();
					comp->LayoutAttributes = attribs;
					pipeline->AbstractComponents.Add(comp);
				}
			}
			ReadToken(TokenType::RBrace);
			return pipeline;
		}

		RefPtr<ComponentSyntaxNode> Parser::ParseComponent()
		{
			RefPtr<ComponentSyntaxNode> component = new ComponentSyntaxNode();
			component->LayoutAttributes = ParseAttribute();
			while (LookAheadToken(L"inline") || LookAheadToken(L"out") || LookAheadToken(L"require") || LookAheadToken(L"public"))
			{
				if (LookAheadToken(L"inline"))
				{
					component->IsInline = true;
					ReadToken(L"inline");
				}
				else if (LookAheadToken(L"out"))
				{
					component->IsOutput = true;
					ReadToken(L"out");
				}
				else if (LookAheadToken(L"public"))
				{
					component->IsPublic = true;
					ReadToken(L"public");
				}
				else if (LookAheadToken(L"require"))
				{
					component->IsParam = true;
					ReadToken(L"require");
				}
				else
					break;
			}
			if (LookAheadToken(L"@"))
				component->Rate = ParseRate();
			component->Type = ParseType();
			FillPosition(component.Ptr());
			component->Name = ReadToken(TokenType::Identifier);
			if (LookAheadToken(L":"))
			{
				ReadToken(L":");
				component->AlternateName = ReadToken(TokenType::Identifier);
			}
			if (!component->IsParam && LookAheadToken(TokenType::OpAssign))
			{
				ReadToken(TokenType::OpAssign);
				component->Expression = ParseExpression();
				ReadToken(TokenType::Semicolon);
			}
			else if (!component->IsParam && LookAheadToken(TokenType::LBrace))
			{
				component->BlockStatement = ParseBlockStatement();
			}
			else
				ReadToken(TokenType::Semicolon);
			return component;
		}

		RefPtr<WorldSyntaxNode> Parser::ParseWorld()
		{
			RefPtr<WorldSyntaxNode> world = new WorldSyntaxNode();
			world->LayoutAttributes = ParseAttribute();
			world->IsAbstract = LookAheadToken(L"abstract");
			if (world->IsAbstract)
				ReadToken(L"abstract");
			ReadToken(L"world");
			FillPosition(world.Ptr());
			world->Name = ReadToken(TokenType::Identifier);
			if (LookAheadToken(TokenType::Colon))
			{
				ReadToken(TokenType::Colon);
				world->TargetMachine = ReadToken(TokenType::StringLiterial).Content;
			}
			if (LookAheadToken(L"using"))
			{
				ReadToken(L"using");
				while (LookAheadToken(TokenType::Identifier))
				{
					world->Usings.Add(ReadToken(TokenType::Identifier));
					if (LookAheadToken(TokenType::Comma))
						ReadToken(TokenType::Comma);
					else
						break;
				}
			}
			if (!world->IsAbstract)
			{
				ReadToken(L"export");
				world->ExportOperator = ReadToken(TokenType::Identifier);
			}
			ReadToken(TokenType::Semicolon);
			return world;
		}

		RefPtr<RateSyntaxNode> Parser::ParseRate()
		{
			RefPtr<RateSyntaxNode> rate = new RateSyntaxNode();
			FillPosition(rate.Ptr());
			ReadToken(TokenType::At);
			auto readWorldRate = [this]()
			{
				RateWorld rw;
				rw.World = ReadToken(TokenType::Identifier);
				if (LookAheadToken(TokenType::OpMul))
				{
					ReadToken(TokenType::OpMul);
					rw.Pinned = true;
				}
				return rw;
			};
			if (LookAheadToken(TokenType::LParent))
			{
				ReadToken(TokenType::LParent);
				while (!LookAheadToken(TokenType::RParent))
				{
					RateWorld rw = readWorldRate();
					rate->Worlds.Add(rw);
					if (LookAheadToken(TokenType::Comma))
					{
						ReadToken(TokenType::Comma);
					}
					else
						break;
				}
				ReadToken(TokenType::RParent);
			}
			else
				rate->Worlds.Add(readWorldRate());
			return rate;
		}

		RefPtr<ImportSyntaxNode> Parser::ParseImport()
		{
			RefPtr<ImportSyntaxNode> rs = new ImportSyntaxNode();
			if (LookAheadToken(L"public"))
			{
				rs->IsPublic = true;
				ReadToken(L"public");
			}
			ReadToken(L"using");
			rs->IsInplace = !LookAheadToken(TokenType::OpAssign, 1);
			if (!rs->IsInplace)
			{
				rs->ObjectName = ReadToken(TokenType::Identifier);
				ReadToken(TokenType::OpAssign);
			}
			FillPosition(rs.Ptr());
			rs->ShaderName = ReadToken(TokenType::Identifier);
			if (LookAheadToken(TokenType::Semicolon))
				ReadToken(TokenType::Semicolon);
			else
			{
				ReadToken(TokenType::LParent);
				while (!LookAheadToken(TokenType::RParent))
				{
					RefPtr<ImportArgumentSyntaxNode> arg = new ImportArgumentSyntaxNode();
					FillPosition(arg.Ptr());
					auto expr = ParseExpression();
					if (LookAheadToken(L":"))
					{
						if (auto varExpr = dynamic_cast<VarExpressionSyntaxNode*>(expr.Ptr()))
						{
							arg->ArgumentName.Content = varExpr->Variable;
							arg->ArgumentName.Position = varExpr->Position;
						}
						else
							errors.Add(CompileError(L"unexpected ':'.", 20011, pos < tokens.Count() ? tokens[pos].Position : CodePosition(0, 0, fileName)));
						ReadToken(L":");
						arg->Expression = ParseExpression();
					}
					else
						arg->Expression = expr;
					rs->Arguments.Add(arg);
					if (LookAheadToken(TokenType::Comma))
						ReadToken(TokenType::Comma);
					else
						break;
				}
				ReadToken(TokenType::RParent);
				ReadToken(TokenType::Semicolon);
			}
			return rs;
		}

		RefPtr<ImportStatementSyntaxNode> Parser::ParseImportStatement()
		{
			RefPtr<ImportStatementSyntaxNode> rs = new ImportStatementSyntaxNode();
			FillPosition(rs.Ptr());
			rs->Import = ParseImport();
			return rs;
		}

		RefPtr<ImportOperatorDefSyntaxNode> Parser::ParseImportOperator()
		{
			RefPtr<ImportOperatorDefSyntaxNode> op = new ImportOperatorDefSyntaxNode();
			ReadToken(L"import");
			FillPosition(op.Ptr());
			op->Name = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::LParent);
			op->SourceWorld = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::RightArrow);
			op->DestWorld = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::RParent);
			if (LookAheadToken(L"using"))
			{
				ReadToken(L"using");
				do
				{
					op->Usings.Add(ReadToken(TokenType::Identifier));
					if (LookAheadToken(TokenType::Comma))
						ReadToken(TokenType::Comma);
					else
						break;
				} while (LookAheadToken(TokenType::Identifier));
			}
			if (LookAheadToken(TokenType::LBrace))
			{
				ReadToken(TokenType::LBrace);
				while (!LookAheadToken(TokenType::RBrace))
				{
					auto name = ReadToken(TokenType::Identifier);
					ReadToken(TokenType::Colon);
					auto value = ReadToken(TokenType::StringLiterial);
					ReadToken(TokenType::Semicolon);
					op->Arguments[name.Content] = value.Content;
					if (LookAheadToken(TokenType::Comma))
						ReadToken(TokenType::Comma);
					else
						break;

				}
				ReadToken(TokenType::RBrace);
				if (LookAheadToken(TokenType::Semicolon))
					ReadToken(TokenType::Semicolon);
			}
			else 
				ReadToken(TokenType::Semicolon);
			return op;
		}

		RefPtr<FunctionSyntaxNode> Parser::ParseFunction()
		{
			RefPtr<FunctionSyntaxNode> function = new FunctionSyntaxNode();
			if (LookAheadToken(L"__intrinsic"))
			{
				function->HasSideEffect = false;
				function->IsExtern = true;
				pos++;
			}
			else if (LookAheadToken(L"extern"))
			{
				function->IsExtern = true;
				pos++;
			}
			else
				function->IsExtern = false;
			function->IsInline = true;
			if (LookAheadToken(L"inline"))
			{
				function->IsInline = true;
				pos++;
			}
			
			PushScope();
			function->ReturnType = ParseType();
			try
			{
				FillPosition(function.Ptr());
				Token & name = ReadToken(TokenType::Identifier);
				function->Name = name.Content;
				ReadToken(TokenType::LParent);
				while(pos < tokens.Count() && tokens[pos].Type != TokenType::RParent)
				{
					function->Parameters.Add(ParseParameter());
					if (LookAheadToken(TokenType::Comma))
						ReadToken(TokenType::Comma);
					else
						break;
				}
				ReadToken(TokenType::RParent);
			}
			catch(int e)
			{
				if (e == 0)
					return function;
				while (pos < tokens.Count() && tokens[pos].Type != TokenType::LBrace)
				{
					pos++;
				}
			}
			if (!function->IsExtern)
				function->Body = ParseBlockStatement();
			else
				ReadToken(TokenType::Semicolon);
			PopScope();
			return function;
		}

		RefPtr<StatementSyntaxNode> Parser::ParseStatement()
		{
			RefPtr<StatementSyntaxNode> statement;
			if (LookAheadToken(TokenType::LBrace))
				statement = ParseBlockStatement();
			else if (IsTypeKeyword() || LookAheadToken(L"const"))
				statement = ParseVarDeclrStatement();
			else if (LookAheadToken(TokenType::KeywordIf))
				statement = ParseIfStatement();
			else if (LookAheadToken(TokenType::KeywordFor))
				statement = ParseForStatement();
			else if (LookAheadToken(TokenType::KeywordWhile))
				statement = ParseWhileStatement();
			else if (LookAheadToken(TokenType::KeywordDo))
				statement = ParseDoWhileStatement();
			else if (LookAheadToken(TokenType::KeywordBreak))
				statement = ParseBreakStatement();
			else if (LookAheadToken(TokenType::KeywordContinue))
				statement = ParseContinueStatement();
			else if (LookAheadToken(TokenType::KeywordReturn))
				statement = ParseReturnStatement();
			else if (LookAheadToken(TokenType::Identifier) && LookAheadToken(TokenType::Identifier, 1) || LookAheadToken(L"using"))
				statement = ParseImportStatement();
			else if (LookAheadToken(TokenType::Identifier))
				statement = ParseExpressionStatement();
			else if (LookAheadToken(TokenType::Semicolon))
			{
				statement = new EmptyStatementSyntaxNode();
				FillPosition(statement.Ptr());
				ReadToken(TokenType::Semicolon);
			}
			else
			{
				errors.Add(CompileError(String(L"syntax error."), 20002, tokens[pos].Position));
				throw 20002;
			}
			return statement;
		}

		RefPtr<BlockStatementSyntaxNode> Parser::ParseBlockStatement()
		{
			RefPtr<BlockStatementSyntaxNode> blockStatement = new BlockStatementSyntaxNode();
			PushScope();
			ReadToken(TokenType::LBrace);
			if(pos < tokens.Count())
			{
				FillPosition(blockStatement.Ptr());
			}
			int lastErrorPos = 0;
			while (pos < tokens.Count() && !LookAheadToken(TokenType::RBrace))
			{
				try
				{
					blockStatement->Statements.Add(ParseStatement());
				}
				catch (int)
				{
					if (pos == lastErrorPos)
						pos++;
					lastErrorPos = pos;
				}
			}
			ReadToken(TokenType::RBrace);
			PopScope();
			return blockStatement;
		}

		VariableModifier Parser::ReadVariableModifier()
		{
			auto & token = ReadToken(TokenType::Identifier);
			if (token.Content == L"in")
				return VariableModifier::In;
			else if (token.Content == L"out")
				return VariableModifier::Out;
			else if (token.Content == L"uniform")
				return VariableModifier::Uniform;
			else if (token.Content == L"parameter")
				return VariableModifier::Parameter;
			else if (token.Content == L"const")
				return VariableModifier::Const;
			else if (token.Content == L"centroid")
				return VariableModifier::Centroid;
			else if (token.Content == L"instance")
				return VariableModifier::Instance;
			else if (token.Content == L"__builtin")
				return VariableModifier::Builtin;
			return VariableModifier::None; 
		}

		RefPtr<VarDeclrStatementSyntaxNode> Parser::ParseVarDeclrStatement()
		{
			RefPtr<VarDeclrStatementSyntaxNode>varDeclrStatement = new VarDeclrStatementSyntaxNode();
		
			if (pos < tokens.Count())
				FillPosition(varDeclrStatement.Ptr());
			while (pos < tokens.Count())
			{
				if (LookAheadToken(L"layout"))
				{
					ReadToken(L"layout");
					ReadToken(TokenType::LParent);
					StringBuilder layoutSB;
					while (!LookAheadToken(TokenType::RParent))
					{
						layoutSB.Append(ReadToken(TokenType::Identifier).Content);
						if (LookAheadToken(TokenType::OpAssign))
						{
							layoutSB.Append(ReadToken(TokenType::OpAssign).Content);
							layoutSB.Append(ReadToken(TokenType::IntLiterial).Content);
						}
						if (!LookAheadToken(TokenType::Comma))
							break;
						else
							layoutSB.Append(L", ");
					}
					ReadToken(TokenType::RParent);
					varDeclrStatement->LayoutString = layoutSB.ProduceString();
				}
				else
					break;
			}
			varDeclrStatement->Type = ParseType();
			while (pos < tokens.Count())
			{
				RefPtr<Variable> var = new Variable();
				FillPosition(var.Ptr());
				Token & name = ReadToken(TokenType::Identifier);
				var->Name = name.Content;
				if (LookAheadToken(TokenType::OpAssign))
				{
					ReadToken(TokenType::OpAssign);
					var->Expression = ParseExpression();
				}

				varDeclrStatement->Variables.Add(var);
				if (LookAheadToken(TokenType::Comma))
					ReadToken(TokenType::Comma);
				else
					break;
			}
			ReadToken(TokenType::Semicolon);
			
			return varDeclrStatement;
		}

		RefPtr<IfStatementSyntaxNode> Parser::ParseIfStatement()
		{
			RefPtr<IfStatementSyntaxNode> ifStatement = new IfStatementSyntaxNode();
			FillPosition(ifStatement.Ptr());
			ReadToken(TokenType::KeywordIf);
			ReadToken(TokenType::LParent);
			ifStatement->Predicate = ParseExpression();
			ReadToken(TokenType::RParent);
			ifStatement->PositiveStatement = ParseStatement();
			if (LookAheadToken(TokenType::KeywordElse))
			{
				ReadToken(TokenType::KeywordElse);
				ifStatement->NegativeStatement = ParseStatement();
			}
			return ifStatement;
		}

		RefPtr<ForStatementSyntaxNode> Parser::ParseForStatement()
		{
			RefPtr<ForStatementSyntaxNode> stmt = new ForStatementSyntaxNode();
			PushScope();
			FillPosition(stmt.Ptr());
			ReadToken(TokenType::KeywordFor);
			ReadToken(TokenType::LParent);
			if (IsTypeKeyword())
				stmt->TypeDef = ParseType();
			stmt->IterationVariable = ReadToken(TokenType::Identifier);
			ReadToken(TokenType::OpAssign);
			stmt->InitialExpression = ParseExpression();
			ReadToken(TokenType::Colon);
			stmt->EndExpression = ParseExpression();
			if (LookAheadToken(TokenType::Colon))
			{
				stmt->StepExpression = stmt->EndExpression;
				ReadToken(TokenType::Colon);
				stmt->EndExpression = ParseExpression();
			}
			ReadToken(TokenType::RParent);
			stmt->Statement = ParseStatement();
			PopScope();
			return stmt;
		}

		RefPtr<WhileStatementSyntaxNode> Parser::ParseWhileStatement()
		{
			RefPtr<WhileStatementSyntaxNode> whileStatement = new WhileStatementSyntaxNode();
			PushScope();
			FillPosition(whileStatement.Ptr());
			ReadToken(TokenType::KeywordWhile);
			ReadToken(TokenType::LParent);
			whileStatement->Predicate = ParseExpression();
			ReadToken(TokenType::RParent);
			whileStatement->Statement = ParseStatement();
			PopScope();
			return whileStatement;
		}

		RefPtr<DoWhileStatementSyntaxNode> Parser::ParseDoWhileStatement()
		{
			RefPtr<DoWhileStatementSyntaxNode> doWhileStatement = new DoWhileStatementSyntaxNode();
			PushScope();
			FillPosition(doWhileStatement.Ptr());
			ReadToken(TokenType::KeywordDo);
			doWhileStatement->Statement = ParseStatement();
			ReadToken(TokenType::KeywordWhile);
			ReadToken(TokenType::LParent);
			doWhileStatement->Predicate = ParseExpression();
			ReadToken(TokenType::RParent);
			ReadToken(TokenType::Semicolon);
			PopScope();
			return doWhileStatement;
		}

		RefPtr<BreakStatementSyntaxNode> Parser::ParseBreakStatement()
		{
			RefPtr<BreakStatementSyntaxNode> breakStatement = new BreakStatementSyntaxNode();
			FillPosition(breakStatement.Ptr());
			ReadToken(TokenType::KeywordBreak);
			ReadToken(TokenType::Semicolon);
			return breakStatement;
		}

		RefPtr<ContinueStatementSyntaxNode>	Parser::ParseContinueStatement()
		{
			RefPtr<ContinueStatementSyntaxNode> continueStatement = new ContinueStatementSyntaxNode();
			FillPosition(continueStatement.Ptr());
			ReadToken(TokenType::KeywordContinue);
			ReadToken(TokenType::Semicolon);
			return continueStatement;
		}

		RefPtr<ReturnStatementSyntaxNode> Parser::ParseReturnStatement()
		{
			RefPtr<ReturnStatementSyntaxNode> returnStatement = new ReturnStatementSyntaxNode();
			FillPosition(returnStatement.Ptr());
			ReadToken(TokenType::KeywordReturn);
			if (!LookAheadToken(TokenType::Semicolon))
				returnStatement->Expression = ParseExpression();
			ReadToken(TokenType::Semicolon);
			return returnStatement;
		}

		RefPtr<ExpressionStatementSyntaxNode> Parser::ParseExpressionStatement()
		{
			RefPtr<ExpressionStatementSyntaxNode> statement = new ExpressionStatementSyntaxNode();
			
			FillPosition(statement.Ptr());
			statement->Expression = ParseExpression();
			
			ReadToken(TokenType::Semicolon);
			return statement;
		}

		RefPtr<ParameterSyntaxNode> Parser::ParseParameter()
		{
			RefPtr<ParameterSyntaxNode> parameter = new ParameterSyntaxNode();
			
			parameter->Type = ParseType();
			Token & name = ReadToken(TokenType::Identifier);
			parameter->Name = name.Content;
			FillPosition(parameter.Ptr());
			
			return parameter;
		}

		RefPtr<TypeSyntaxNode> Parser::ParseType()
		{
			RefPtr<TypeSyntaxNode> type = new TypeSyntaxNode();
		
			FillPosition(type.Ptr());
			type->TypeName = ReadTypeKeyword().Content;
	
			if (LookAheadToken(TokenType::OpLess))
			{
				ReadToken(TokenType::OpLess);
				type->GenericBaseType = ReadToken(TokenType::Identifier).Content;
				ReadToken(TokenType::OpGreater);
			}
			if(LookAheadToken(TokenType::LBracket))
			{
				ReadToken(TokenType::LBracket);
				type->IsArray = true;
				type->ArrayLength = atoi(ReadToken(TokenType::IntLiterial).Content.ToMultiByteString());
				ReadToken(TokenType::RBracket);
			}
			return type;
		}

		enum class Associativity
		{
			Left, Right
		};

		Associativity GetAssociativityFromLevel(int level)
		{
			if (level == 0)
				return Associativity::Right;
			else
				return Associativity::Left;
		}

		int GetOpLevel(TokenType type)
		{
			switch(type)
			{
			case TokenType::OpAssign:
			case TokenType::OpMulAssign:
			case TokenType::OpDivAssign:
			case TokenType::OpAddAssign:
			case TokenType::OpSubAssign:
			case TokenType::OpModAssign:
				return 0;
			case TokenType::OpOr:
				return 2;
			case TokenType::OpAnd:
				return 3;
			case TokenType::OpBitOr:
				return 4;
			case TokenType::OpBitXor:
				return 5;
			case TokenType::OpBitAnd:
				return 6;
			case TokenType::OpEql:
			case TokenType::OpNeq:
				return 7;
			case TokenType::OpGeq:
			case TokenType::OpLeq:
			case TokenType::OpGreater:
			case TokenType::OpLess:
				return 8;
			case TokenType::OpLsh:
			case TokenType::OpRsh:
				return 9;
			case TokenType::OpAdd:
			case TokenType::OpSub:
				return 10;
			case TokenType::OpMul:
			case TokenType::OpDiv:
			case TokenType::OpMod:
				return 11;
			default:
				return -1;
			}
		}

		Operator GetOpFromToken(Token & token)
		{
			switch(token.Type)
			{
			case TokenType::OpAssign:
				return Operator::Assign;
			case TokenType::OpAddAssign:
				return Operator::AddAssign;
			case TokenType::OpSubAssign:
				return Operator::SubAssign;
			case TokenType::OpMulAssign:
				return Operator::MulAssign;
			case TokenType::OpDivAssign:
				return Operator::DivAssign;
			case TokenType::OpModAssign:
				return Operator::ModAssign;
			case TokenType::OpOr:
				return Operator::Or;
			case TokenType::OpAnd:
				return Operator::And;
			case TokenType::OpBitOr:
				return Operator::BitOr;
			case TokenType::OpBitXor:
				return Operator::BitXor;
			case TokenType::OpBitAnd:
				return Operator::BitAnd;
			case TokenType::OpEql:
				return Operator::Eql;
			case TokenType::OpNeq:
				return Operator::Neq;
			case TokenType::OpGeq:
				return Operator::Geq;
			case TokenType::OpLeq:
				return Operator::Leq;
			case TokenType::OpGreater:
				return Operator::Greater;
			case TokenType::OpLess:
				return Operator::Less;
			case TokenType::OpLsh:
				return Operator::Lsh;
			case TokenType::OpRsh:
				return Operator::Rsh;
			case TokenType::OpAdd:
				return Operator::Add;
			case TokenType::OpSub:
				return Operator::Sub;
			case TokenType::OpMul:
				return Operator::Mul;
			case TokenType::OpDiv:
				return Operator::Div;
			case TokenType::OpMod:
				return Operator::Mod;
			case TokenType::OpInc:
				return Operator::PostInc;
			case TokenType::OpDec:
				return Operator::PostDec;
			case TokenType::OpNot:
				return Operator::Not;
			case TokenType::OpBitNot:
				return Operator::BitNot;
			default:
				throw L"Illegal TokenType.";
			}
		}

		RefPtr<ExpressionSyntaxNode> Parser::ParseExpression(int level)
		{
			if (level == MaxExprLevel)
				return ParseLeafExpression();
			if (level == 1)
			{
				// parse select clause
				auto condition = ParseExpression(level + 1);
				if (LookAheadToken(TokenType::QuestionMark))
				{
					RefPtr<SelectExpressionSyntaxNode> select = new SelectExpressionSyntaxNode();
					FillPosition(select.Ptr());
					ReadToken(TokenType::QuestionMark);
					select->SelectorExpr = condition;
					select->Expr0 = ParseExpression(level);
					ReadToken(TokenType::Colon);
					select->Expr1 = ParseExpression(level);
					return select;
				}
				else
					return condition;
			}
			else
			{
				if (GetAssociativityFromLevel(level) == Associativity::Left)
				{
					auto left = ParseExpression(level + 1);
					while (pos < tokens.Count() && GetOpLevel(tokens[pos].Type) == level)
					{
						RefPtr<BinaryExpressionSyntaxNode> tmp = new BinaryExpressionSyntaxNode();
						tmp->LeftExpression = left;
						FillPosition(tmp.Ptr());
						Token & opToken = ReadToken(tokens[pos].Type);
						tmp->Operator = GetOpFromToken(opToken);
						tmp->RightExpression = ParseExpression(level + 1);
						left = tmp;
					}
					return left;
				}
				else
				{
					auto left = ParseExpression(level + 1);
					if (pos < tokens.Count() && GetOpLevel(tokens[pos].Type) == level)
					{
						RefPtr<BinaryExpressionSyntaxNode> tmp = new BinaryExpressionSyntaxNode();
						tmp->LeftExpression = left;
						FillPosition(tmp.Ptr());
						Token & opToken = ReadToken(tokens[pos].Type);
						tmp->Operator = GetOpFromToken(opToken);
						tmp->RightExpression = ParseExpression(level);
						left = tmp;
					}
					return left;
				}
			}
		}

		RefPtr<ExpressionSyntaxNode> Parser::ParseLeafExpression()
		{
			RefPtr<ExpressionSyntaxNode> rs;

			if (LookAheadToken(TokenType::OpInc) ||
				LookAheadToken(TokenType::OpDec) ||
				LookAheadToken(TokenType::OpNot) ||
				LookAheadToken(TokenType::OpBitNot) ||
				LookAheadToken(TokenType::OpSub))
			{
				RefPtr<UnaryExpressionSyntaxNode> unaryExpr = new UnaryExpressionSyntaxNode();
				Token & token = tokens[pos++];
				FillPosition(unaryExpr.Ptr());
				unaryExpr->Operator = GetOpFromToken(token);
				if (unaryExpr->Operator == Operator::PostInc)
					unaryExpr->Operator = Operator::PreInc;
				else if (unaryExpr->Operator == Operator::PostDec)
					unaryExpr->Operator = Operator::PreDec;
				else if (unaryExpr->Operator == Operator::Sub)
					unaryExpr->Operator = Operator::Neg;

				unaryExpr->Expression = ParseLeafExpression();
				rs = unaryExpr;
				return rs;
			}

			if (LookAheadToken(TokenType::LParent))
			{
				ReadToken(TokenType::LParent);
				RefPtr<ExpressionSyntaxNode> expr;
				if (IsTypeKeyword() && pos + 1 < tokens.Count() && tokens[pos+1].Type == TokenType::RParent)
				{
					RefPtr<TypeCastExpressionSyntaxNode> tcexpr = new TypeCastExpressionSyntaxNode();
					FillPosition(tcexpr.Ptr());
					tcexpr->TargetType = ParseType();
					ReadToken(TokenType::RParent);
					tcexpr->Expression = ParseExpression();
					expr = tcexpr;
				}
				else
				{
					expr = ParseExpression();
					ReadToken(TokenType::RParent);
				}
				rs = expr;
			}
			else if (LookAheadToken(TokenType::IntLiterial) ||
				LookAheadToken(TokenType::DoubleLiterial))
			{
				RefPtr<ConstantExpressionSyntaxNode> constExpr = new ConstantExpressionSyntaxNode();
				auto token = tokens[pos++];
				FillPosition(constExpr.Ptr());
				if (token.Type == TokenType::IntLiterial)
				{
					constExpr->ConstType = ConstantExpressionSyntaxNode::ConstantType::Int;
					constExpr->IntValue = StringToInt(token.Content);
				}
				else if (token.Type == TokenType::DoubleLiterial)
				{
					constExpr->ConstType = ConstantExpressionSyntaxNode::ConstantType::Float;
					constExpr->FloatValue = (float)StringToDouble(token.Content);
				}
				rs = constExpr;
			}
			else if (LookAheadToken(TokenType::Identifier))
			{
				RefPtr<VarExpressionSyntaxNode> varExpr = new VarExpressionSyntaxNode();
				FillPosition(varExpr.Ptr());
				auto & token = ReadToken(TokenType::Identifier);
				varExpr->Variable = token.Content;
				rs = varExpr;
			}

			while (pos < tokens.Count() &&
				(LookAheadToken(TokenType::OpInc) ||
				LookAheadToken(TokenType::OpDec) ||
				LookAheadToken(TokenType::Dot) ||
				LookAheadToken(TokenType::LBracket) ||
				LookAheadToken(TokenType::LParent)))
			{
				if (LookAheadToken(TokenType::OpInc))
				{
					RefPtr<UnaryExpressionSyntaxNode> unaryExpr = new UnaryExpressionSyntaxNode();
					FillPosition(unaryExpr.Ptr());
					ReadToken(TokenType::OpInc);
					unaryExpr->Operator = Operator::PostInc;
					unaryExpr->Expression = rs;
					rs = unaryExpr;
				}
				else if (LookAheadToken(TokenType::OpDec))
				{
					RefPtr<UnaryExpressionSyntaxNode> unaryExpr = new UnaryExpressionSyntaxNode();
					FillPosition(unaryExpr.Ptr());
					ReadToken(TokenType::OpDec);
					unaryExpr->Operator = Operator::PostDec;
					unaryExpr->Expression = rs;
					rs = unaryExpr;
				}
				else if (LookAheadToken(TokenType::LBracket))
				{
					RefPtr<IndexExpressionSyntaxNode> indexExpr = new IndexExpressionSyntaxNode();
					indexExpr->BaseExpression = rs;
					FillPosition(indexExpr.Ptr());
					ReadToken(TokenType::LBracket);
					indexExpr->IndexExpression = ParseExpression();
					ReadToken(TokenType::RBracket);
					rs = indexExpr;
				}
				else if (LookAheadToken(TokenType::LParent))
				{
					RefPtr<InvokeExpressionSyntaxNode> invokeExpr = new InvokeExpressionSyntaxNode();
					invokeExpr->FunctionExpr = rs;
					if (!invokeExpr->FunctionExpr)
					{
						errors.Add(CompileError(L"syntax error.", 20002, tokens[pos].Position));
					}
					FillPosition(invokeExpr.Ptr());
					ReadToken(TokenType::LParent);
					while (pos < tokens.Count())
					{
						if (!LookAheadToken(TokenType::RParent))
							invokeExpr->Arguments.Add(ParseExpression());
						else
						{
							break;
						}
						
						if (!LookAheadToken(TokenType::Comma))
							break;
						ReadToken(TokenType::Comma);
					}
					ReadToken(TokenType::RParent);
					rs = invokeExpr;
				}
				else if (LookAheadToken(TokenType::Dot))
				{
					RefPtr<MemberExpressionSyntaxNode> memberExpr = new MemberExpressionSyntaxNode();
					FillPosition(memberExpr.Ptr());
					memberExpr->BaseExpression = rs;
					ReadToken(TokenType::Dot); 
					memberExpr->MemberName = ReadToken(TokenType::Identifier).Content;
					rs = memberExpr;
				}
			}
			if (!rs)
			{
				CodePosition codePos;
				if (pos < tokens.Count())
				{
					codePos = tokens[pos].Position;
				}
				errors.Add(CompileError(String(L"syntax error."), 20002, codePos));
				throw 20005;
			}
			return rs;
		}
	}
}

/***********************************************************************
SPIRECORE\SCHEDULE.CPP
***********************************************************************/
using namespace CoreLib::Basic;

namespace Spire
{
	namespace Compiler
	{
		class ScheduleParser
		{
		private:
			List<CompileError>& errors;
			List<Token> tokens;
			int pos;
			String fileName;
			Token & ReadToken(const wchar_t * string)
			{
				if (pos >= tokens.Count())
				{
					errors.Add(CompileError(String(L"\"") + string + String(L"\" expected but end of file encountered."), 0, CodePosition(0, 0, fileName)));
					throw 0;
				}
				else if (tokens[pos].Content != string)
				{
					errors.Add(CompileError(String(L"\"") + string + String(L"\" expected"), 0, tokens[pos].Position));
					throw 20001;
				}
				return tokens[pos++];
			}

			Token & ReadToken(TokenType type)
			{
				if (pos >= tokens.Count())
				{
					errors.Add(CompileError(TokenTypeToString(type) + String(L" expected but end of file encountered."), 0, CodePosition(0, 0, fileName)));
					throw 0;
				}
				else if (tokens[pos].Type != type)
				{
					errors.Add(CompileError(TokenTypeToString(type) + String(L" expected"), 20001, tokens[pos].Position));
					throw 20001;
				}
				return tokens[pos++];
			}

			bool LookAheadToken(const wchar_t * string)
			{
				if (pos >= tokens.Count())
				{
					errors.Add(CompileError(String(L"\'") + string + String(L"\' expected but end of file encountered."), 0, CodePosition(0, 0, fileName)));
					return false;
				}
				else
				{
					if (tokens[pos].Content == string)
						return true;
					else
						return false;
				}
			}
		public:
			ScheduleParser(List<CompileError>& _errorList)
				: errors(_errorList)
			{}
			Schedule Parse(String source, String _fileName)
			{
				this->fileName = _fileName;
				Schedule schedule;
				Lexer lex;
				tokens = lex.Parse(fileName, source, errors);
				pos = 0;
				try
				{
					while (pos < tokens.Count())
					{
						if (LookAheadToken(L"attrib"))
						{
							EnumerableDictionary<String, String> additionalAttributes;
							ReadToken(L"attrib");
							String choiceName = ReadToken(TokenType::Identifier).Content;
							while (LookAheadToken(L"."))
							{
								choiceName = choiceName + L".";
								ReadToken(TokenType::Dot);
								choiceName = choiceName + ReadToken(TokenType::Identifier).Content;
							}
							ReadToken(TokenType::OpAssign);

							while (pos < tokens.Count())
							{
								auto name = ReadToken(TokenType::Identifier).Content;
								String value;
								if (LookAheadToken(L":"))
								{
									ReadToken(L":");
									value = ReadToken(TokenType::StringLiterial).Content;
								}
								additionalAttributes[name] = value;
								if (LookAheadToken(L","))
									ReadToken(TokenType::Comma);
								else
									break;
							}
							schedule.AddtionalAttributes[choiceName] = additionalAttributes;
						}
						else
						{
							String choiceName = ReadToken(TokenType::Identifier).Content;
							while (LookAheadToken(L"."))
							{
								choiceName = choiceName + L".";
								ReadToken(TokenType::Dot);
								choiceName = choiceName + ReadToken(TokenType::Identifier).Content;
							}
							ReadToken(TokenType::OpAssign);
							List<RefPtr<ChoiceValueSyntaxNode>> worlds;
							while (pos < tokens.Count())
							{
								auto & token = ReadToken(TokenType::StringLiterial);
								RefPtr<ChoiceValueSyntaxNode> choiceValue = new ChoiceValueSyntaxNode();
								choiceValue->Position = token.Position;
								int splitterPos = token.Content.IndexOf(L':');
								if (splitterPos != -1)
								{
									choiceValue->WorldName = token.Content.SubString(0, splitterPos);
									choiceValue->AlternateName = token.Content.SubString(splitterPos + 1, token.Content.Length() - splitterPos - 1);
								}
								else
								{
									choiceValue->WorldName = token.Content;
								}
								worlds.Add(choiceValue);
								if (LookAheadToken(L","))
									ReadToken(TokenType::Comma);
								else
									break;
							}
							schedule.Choices[choiceName] = worlds;
						}
						ReadToken(TokenType::Semicolon);
					}
				}
				catch (...)
				{
				}
				return schedule;
			}
		};
	
		Schedule Schedule::Parse(String source, String fileName, List<CompileError>& errorList)
		{
			return ScheduleParser(errorList).Parse(source, fileName);
		}
	}
}

/***********************************************************************
SPIRECORE\SEMANTICSVISITOR.CPP
***********************************************************************/

namespace Spire
{
	namespace Compiler
	{
		class SemanticsVisitor : public SyntaxVisitor
		{
			ProgramSyntaxNode * program = nullptr;
			FunctionSyntaxNode * function = nullptr;
			FunctionSymbol * currentFunc = nullptr;
			ShaderSymbol * currentShader = nullptr;
			ShaderComponentSymbol * currentComp = nullptr;
			ComponentSyntaxNode * currentCompNode = nullptr;
			List<SyntaxNode *> loops;
			SymbolTable * symbolTable;
		public:
			SemanticsVisitor(SymbolTable * symbols, ErrorWriter * pErr)
				:symbolTable(symbols), SyntaxVisitor(pErr)
			{
			}
			// return true if world0 depends on world1 (there exists a series of import operators that converts world1 variables to world0)
			bool IsWorldDependent(PipelineSymbol * pipeline, String world0, String world1)
			{
				HashSet<String> depWorldsSet;
				List<String> depWorlds;
				depWorlds.Add(world0);
				for (int i = 0; i < depWorlds.Count(); i++)
				{
					auto & dep = pipeline->WorldDependency[world0].GetValue();
					if (dep.Contains(world1))
						return true;
					else
					{
						for (auto w : dep)
							if (depWorldsSet.Add(w))
								depWorlds.Add(w);
					}
				}
				return false;
			}

			void VisitPipeline(PipelineSyntaxNode * pipeline)
			{
				RefPtr<PipelineSymbol> psymbol = new PipelineSymbol();
				psymbol->SyntaxNode = pipeline;
				symbolTable->Pipelines.Add(pipeline->Name.Content, psymbol);
				for (auto world : pipeline->Worlds)
				{
					WorldSymbol worldSym;
					worldSym.IsAbstract = world->IsAbstract;
					worldSym.SyntaxNode = world.Ptr();
					if (!psymbol->Worlds.ContainsKey(world->Name.Content))
					{
						psymbol->Worlds.Add(world->Name.Content, worldSym);
						psymbol->WorldDependency.Add(world->Name.Content, EnumerableHashSet<String>());
						psymbol->ReachableWorlds.Add(world->Name.Content, EnumerableHashSet<String>());
					}
					else
					{
						Error(33001, L"world \'" + world->Name.Content + L"\' is already defined.", world.Ptr());
					}
				}
				for (auto comp : pipeline->AbstractComponents)
				{
					if (comp->IsParam || comp->Rate && comp->Rate->Worlds.Count() == 1
						&& psymbol->IsAbstractWorld(comp->Rate->Worlds.First().World.Content))
						AddNewComponentSymbol(psymbol->Components, comp);
					else
						Error(33003, L"cannot define components in a pipeline.",
							comp.Ptr());
				}
				for (auto world : pipeline->Worlds)
				{
					for (auto & varUsing : world->Usings)
					{
						if (!psymbol->Components.ContainsKey(varUsing.Content))
							Error(33043, L"'using': unknown component '" + varUsing.Content + L"\'.", varUsing);
					}
				}
				// add initial world dependency edges
				for (auto op : pipeline->ImportOperators)
				{
					if (!psymbol->WorldDependency.ContainsKey(op->DestWorld.Content))
						Error(33004, L"undefined world name '" + op->DestWorld.Content + L"'.", op->DestWorld);
					else
					{
						if (psymbol->Worlds[op->DestWorld.Content].GetValue().IsAbstract)
							Error(33005, L"abstract world cannot appear as target as an import operator.", op->DestWorld);
						else if (!psymbol->WorldDependency.ContainsKey(op->SourceWorld.Content))
							Error(33006, L"undefined world name '" + op->SourceWorld.Content + L"'.", op->SourceWorld);
						else
						{
							if (IsWorldDependent(psymbol.Ptr(), op->SourceWorld.Content, op->DestWorld.Content))
							{
								Error(33007, L"import operator '" + op->Name.Content + L"' creates a circular dependency between world '" + op->SourceWorld.Content + L"' and '" + op->DestWorld.Content + L"'",
									op->Name);
							}
							else
								psymbol->WorldDependency[op->DestWorld.Content].GetValue().Add(op->SourceWorld.Content);
						}
					}
				
				}
				// propagate world dependency graph
				bool changed = true;
				while (changed)
				{
					changed = false;
					for (auto world : pipeline->Worlds)
					{
						EnumerableHashSet<String> & dependentWorlds = psymbol->WorldDependency[world->Name.Content].GetValue();
						List<String> loopRange;
						for (auto w : dependentWorlds)
							loopRange.Add(w);
						for (auto w : loopRange)
						{
							EnumerableHashSet<String> & ddw = psymbol->WorldDependency[w].GetValue();
							for (auto ww : ddw)
							{
								if (!dependentWorlds.Contains(ww))
								{
									dependentWorlds.Add(ww);
									changed = true;
								}
							}
						}
					}
				}
				// fill in reachable worlds
				for (auto world : psymbol->Worlds)
				{
					if (auto depWorlds = psymbol->WorldDependency.TryGetValue(world.Key))
					{
						for (auto & dep : *depWorlds)
						{
							psymbol->ReachableWorlds[dep].GetValue().Add(world.Key);
						}
					}
				}

				for (auto & op : pipeline->ImportOperators)
				{
					for (auto & dep : op->Usings)
					{
						RefPtr<ShaderComponentSymbol> refComp;
						if (psymbol->Components.TryGetValue(dep.Content, refComp))
						{
							bool invalid = true;
							for (auto & depImpl : refComp->Implementations)
							{
								for (auto & w : depImpl->Worlds)
								{
									if (psymbol->IsWorldReachable(w, op->DestWorld.Content))
									{
										invalid = false;
									}
								}
							}
							if (invalid)
							{
								Error(30039, L"import operator '" + op->Name.Content + L"': none of the definitions for '" + dep.Content + L"' is available to destination world '" + op->DestWorld.Content + L"'.", dep);
							}
						}
						else
							Error(30034, L"import operator reference '" + dep.Content + L"' is not a defined shader component.", dep);

					}
				}
			}

			virtual void VisitImport(ImportSyntaxNode * import) override
			{
				RefPtr<ShaderSymbol> refShader;
				symbolTable->Shaders.TryGetValue(import->ShaderName.Content, refShader);
				if (refShader)
				{
					// type check
					List<ShaderComponentSymbol*> paramList;
					for (auto & comp : refShader->Components)
						if (comp.Value->IsParam())
							paramList.Add(comp.Value.Ptr());
					int position = 0;
					bool namedArgumentAppeared = false;
					for (auto & arg : import->Arguments)
					{
						if (arg->ArgumentName.Content.Length())
							namedArgumentAppeared = true;
						else
						{
							if (namedArgumentAppeared)
							{
								Error(33030, L"positional argument cannot appear after a named argument.", arg->Expression.Ptr());
								break;
							}
							if (position >= paramList.Count())
							{
								Error(33031, L"too many arguments.", arg->Expression.Ptr());
								break;
							}
							arg->ArgumentName.Content = paramList[position]->Name;
							arg->ArgumentName.Position = arg->Position;
						}
						position++;
						arg->Accept(this);
						RefPtr<ShaderComponentSymbol> refComp;
						if (refShader->Components.TryGetValue(arg->ArgumentName.Content, refComp))
						{
							if (refComp->Type->DataType != arg->Expression->Type)
							{
								Error(33027, L"argument type (" + arg->Expression->Type.ToString() + L") does not match parameter type (" + refComp->Type->DataType.ToString() + L")", arg->Expression.Ptr());
							}
							if (!refComp->IsParam())
								Error(33028, L"'" + arg->ArgumentName.Content + L"' is not a parameter of '" + import->ShaderName.Content + L"'.", arg->ArgumentName);
						}
						else
							Error(33028, L"'" + arg->ArgumentName.Content + L"' is not a parameter of '" + import->ShaderName.Content + L"'.", arg->ArgumentName);
					}
				}
			}

			class ShaderImportVisitor : public SyntaxVisitor
			{
			private:
				SymbolTable * symbolTable = nullptr;
				ShaderSymbol * currentShader = nullptr;
				ShaderComponentSymbol * currentComp = nullptr;
				SemanticsVisitor * typeChecker = nullptr;
			public:
				ShaderImportVisitor(ErrorWriter * writer, SymbolTable * symTable, SemanticsVisitor * pTypeChecker)
					: SyntaxVisitor(writer), symbolTable(symTable), typeChecker(pTypeChecker)
				{}
				virtual void VisitShader(ShaderSyntaxNode * shader) override
				{
					currentShader = symbolTable->Shaders[shader->Name.Content].GetValue().Ptr();
					SyntaxVisitor::VisitShader(shader);
					currentShader = nullptr;
				}
				virtual void VisitComponent(ComponentSyntaxNode * comp) override
				{
					RefPtr<ShaderComponentSymbol> compSym;
					currentShader->Components.TryGetValue(comp->Name.Content, compSym);
					currentComp = compSym.Ptr();
					SyntaxVisitor::VisitComponent(comp);
					currentComp = nullptr;
				}
				virtual void VisitImport(ImportSyntaxNode * import) override
				{
					RefPtr<ShaderSymbol> refShader;
					symbolTable->Shaders.TryGetValue(import->ShaderName.Content, refShader);
					if (!refShader)
						Error(33015, L"undefined identifier \'" + import->ShaderName.Content + L"\'.", import->ShaderName);
					currentShader->DependentShaders.Add(refShader.Ptr());
					if (!currentComp)
					{
						ShaderUsing su;
						su.Shader = refShader.Ptr();
						su.IsPublic = import->IsPublic;
						if (import->IsInplace)
						{
							currentShader->ShaderUsings.Add(su);
						}
						else
						{
							if (currentShader->ShaderObjects.ContainsKey(import->ObjectName.Content) ||
								currentShader->Components.ContainsKey(import->ObjectName.Content))
							{
								Error(33018, L"\'" + import->ShaderName.Content + L"\' is already defined.", import->ShaderName);
							}
							currentShader->ShaderObjects[import->ObjectName.Content] = su;
						}
					}
					if (currentComp)
						Error(33016, L"'using': importing not allowed in component definition.", import->ShaderName);
				}
			};

			// pass 1: fill components in shader symbol table
			void VisitShaderPass1(ShaderSyntaxNode * shader)
			{
				HashSet<String> inheritanceSet;
				auto curShader = shader;
				inheritanceSet.Add(curShader->Name.Content);
				auto & shaderSymbol = symbolTable->Shaders[curShader->Name.Content].GetValue();
				this->currentShader = shaderSymbol.Ptr();
				if (shader->Pipeline.Content.Length() == 0) // implicit pipeline
				{
					if (program->Pipelines.Count() == 1)
					{
						shader->Pipeline = shader->Name; // get line and col from shader name
						shader->Pipeline.Content = program->Pipelines.First()->Name.Content;
					}
					else
					{
						// current compilation context has more than one pipeline defined,
						// in which case we do not allow implicit pipeline specification
						Error(33002, L"explicit pipeline specification required for shader '" +
							shader->Name.Content + L"' because multiple pipelines are defined in current context.", curShader->Name);
					}
				}
				
				auto pipelineName = shader->Pipeline.Content;
				auto pipeline = symbolTable->Pipelines.TryGetValue(pipelineName);
				if (pipeline)
					shaderSymbol->Pipeline = pipeline->Ptr();
				else
				{
					Error(33010, L"pipeline \'" + pipelineName + L"' is not defined.", shader->Pipeline);
					throw 0;
				}
				if (shader->IsModule)
					shaderSymbol->IsAbstract = true;
				// add components to symbol table
				for (auto & mbr : shader->Members)
				{
					if (auto comp = dynamic_cast<ComponentSyntaxNode*>(mbr.Ptr()))
					{
						if (comp->IsParam)
						{
							shaderSymbol->IsAbstract = true;
							if (!shaderSymbol->SyntaxNode->IsModule)
							{
								Error(33009, L"parameters can only be defined in modules.", shaderSymbol->SyntaxNode);
							}
						}
						AddNewComponentSymbol(shaderSymbol->Components, mbr);
					}
				}
				// add shader objects to symbol table
				ShaderImportVisitor importVisitor(err, symbolTable, this);
				shader->Accept(&importVisitor);

				for (auto & comp : shaderSymbol->Components)
				{
					for (auto & impl : comp.Value->Implementations)
					{
						bool inAbstractWorld = false;
						if (impl->SyntaxNode->Rate)
						{
							auto & userSpecifiedWorlds = impl->SyntaxNode->Rate->Worlds;
							for (auto & world : userSpecifiedWorlds)
							{
								if (!shaderSymbol->Pipeline->WorldDependency.ContainsKey(world.World.Content))
									Error(33012, L"\'" + world.World.Content + L"' is not a defined world in '" +
										pipelineName + L"'.", world.World);
								WorldSymbol worldSym;

								if (shaderSymbol->Pipeline->Worlds.TryGetValue(world.World.Content, worldSym))
								{
									if (worldSym.IsAbstract)
									{
										inAbstractWorld = true;
										if (userSpecifiedWorlds.Count() > 1)
										{
											Error(33013, L"abstract world cannot appear with other worlds.",
												world.World);
										}
									}
								}
							}
						}
						if (!inAbstractWorld && !impl->SyntaxNode->IsParam
							&& !impl->SyntaxNode->Expression && !impl->SyntaxNode->BlockStatement)
						{
							Error(33014, L"non-abstract component must have an implementation.",
								impl->SyntaxNode.Ptr());
						}
					}
				}
				this->currentShader = nullptr;
			}
			// pass 2: type checking component definitions
			void VisitShaderPass2(ShaderSyntaxNode * shaderNode)
			{
				RefPtr<ShaderSymbol> shaderSym;
				if (!symbolTable->Shaders.TryGetValue(shaderNode->Name.Content, shaderSym))
					return;
				this->currentShader = shaderSym.Ptr();
				for (auto & comp : shaderNode->Members)
				{
					comp->Accept(this);
				}
				this->currentShader = nullptr;
			}
			virtual void VisitComponent(ComponentSyntaxNode * comp) override
			{
				this->currentCompNode = comp;
				RefPtr<ShaderComponentSymbol> compSym;
				currentShader->Components.TryGetValue(comp->Name.Content, compSym);
				this->currentComp = compSym.Ptr();
				if (comp->Expression)
				{
					comp->Expression->Accept(this);
					if (comp->Expression->Type != compSym->Type->DataType && comp->Expression->Type != ExpressionType::Error)
						Error(30019, L"type mismatch \'" + comp->Expression->Type.ToString() + L"\' and \'" +
							currentComp->Type->DataType.ToString() + L"\'", comp->Name);
				}
				if (comp->BlockStatement)
					comp->BlockStatement->Accept(this);
				this->currentComp = nullptr;
				this->currentCompNode = nullptr;
			}
			virtual void VisitImportStatement(ImportStatementSyntaxNode * importStmt) override
			{
				importStmt->Import->Accept(this);
			}
			void AddNewComponentSymbol(EnumerableDictionary<String, RefPtr<ShaderComponentSymbol>> & components, RefPtr<ComponentSyntaxNode> comp)
			{
				RefPtr<ShaderComponentSymbol> compSym;
				RefPtr<ShaderComponentImplSymbol> compImpl = new ShaderComponentImplSymbol();
				if (comp->Rate)
					for (auto w : comp->Rate->Worlds)
						compImpl->Worlds.Add(w.World.Content);
				compImpl->SyntaxNode = comp;
				if (compImpl->SyntaxNode->Rate)
				{
					for (auto & w : compImpl->SyntaxNode->Rate->Worlds)
						if (w.Pinned)
							compImpl->SrcPinnedWorlds.Add(w.World.Content);
				}
				if (compImpl->SyntaxNode->AlternateName.Type == TokenType::Identifier)
				{
					compImpl->AlternateName = compImpl->SyntaxNode->AlternateName.Content;
				}
				if (compImpl->SyntaxNode->IsOutput)
				{
					if (compImpl->SyntaxNode->Rate)
					{
						for (auto & w : compImpl->SyntaxNode->Rate->Worlds)
							compImpl->ExportWorlds.Add(w.World.Content);
					}
					else
					{
						Error(33019, L"component \'" + compImpl->SyntaxNode->Name.Content + L"\': definition marked as 'export' must have an explicitly specified world.",
							compImpl->SyntaxNode.Ptr());
					}
				}
				if (!components.TryGetValue(comp->Name.Content, compSym))
				{
					compSym = new ShaderComponentSymbol();
					compSym->Type = new Type();
					compSym->Name = comp->Name.Content;
					compSym->Type->DataType = comp->Type->ToExpressionType();
					components.Add(comp->Name.Content, compSym);
				}
				else
				{
					if (comp->IsParam)
						Error(33029, L"\'" + compImpl->SyntaxNode->Name.Content + L"\': requirement clash with previous definition.",
							compImpl->SyntaxNode.Ptr());
					CheckComponentImplementationConsistency(err, compSym.Ptr(), compImpl.Ptr());
				}
				compSym->Implementations.Add(compImpl);
			}
			virtual void VisitProgram(ProgramSyntaxNode * programNode)
			{
				HashSet<String> funcNames;
				this->program = programNode;
				this->function = nullptr;
				for (auto & pipeline : program->Pipelines)
				{
					VisitPipeline(pipeline.Ptr());
				}
				for (auto & func : program->Functions)
				{
					VisitFunctionDeclaration(func.Ptr());
					if (funcNames.Contains(func->InternalName))
					{
						StringBuilder argList;
						argList << L"(";
						for (auto & param : func->Parameters)
						{
							argList << param->Type->ToExpressionType().ToString();
							if (param != func->Parameters.Last())
								argList << L", ";
						}
						argList << L")";
						Error(30001, L"function \'" + func->Name + argList.ProduceString() + L"\' redefinition.", func.Ptr());
					}
					else
						funcNames.Add(func->InternalName);
				}
				for (auto & func : program->Functions)
				{
					func->Accept(this);
				}
				// build initial symbol table for shaders
				for (auto & shader : program->Shaders)
				{
					RefPtr<ShaderSymbol> shaderSym = new ShaderSymbol();
					shaderSym->SyntaxNode = shader.Ptr();
					if (symbolTable->Shaders.ContainsKey(shader->Name.Content))
					{
						Error(33018, L"shader '" + shader->Name.Content + "' has already been defined.", shader->Name);
					}
					symbolTable->Shaders[shader->Name.Content] = shaderSym;
				}
				HashSet<ShaderSyntaxNode*> validShaders;
				for (auto & shader : program->Shaders)
				{
					int lastErrorCount = err->GetErrorCount();
					VisitShaderPass1(shader.Ptr());
					if (err->GetErrorCount() == lastErrorCount)
						validShaders.Add(shader.Ptr());
				}
				if (err->GetErrorCount() != 0)
					return;
				// shader dependency is discovered in pass 1, we can now sort the shaders
				if (!symbolTable->SortShaders())
				{
					HashSet<ShaderSymbol*> sortedShaders;
					for (auto & shader : symbolTable->ShaderDependenceOrder)
						sortedShaders.Add(shader);
					for (auto & shader : symbolTable->Shaders)
						if (!sortedShaders.Contains(shader.Value.Ptr()))
						{
							Error(33011, L"shader '" + shader.Key + L"' involves circular reference.", shader.Value->SyntaxNode->Name);
						}
				}

				for (auto & shader : symbolTable->ShaderDependenceOrder)
				{
					if (!validShaders.Contains(shader->SyntaxNode))
						continue;
					int lastErrorCount = err->GetErrorCount();
					VisitShaderPass2(shader->SyntaxNode);
					if (err->GetErrorCount() != lastErrorCount)
						validShaders.Remove(shader->SyntaxNode);
				}
				// update symbol table with only valid shaders
				EnumerableDictionary<String, RefPtr<ShaderSymbol>> newShaderSymbols;
				for (auto & shader : symbolTable->Shaders)
				{
					if (validShaders.Contains(shader.Value->SyntaxNode))
						newShaderSymbols.AddIfNotExists(shader.Key, shader.Value);
				}
				symbolTable->Shaders = _Move(newShaderSymbols);
			}

			virtual void VisitFunction(FunctionSyntaxNode *functionNode) override
			{
				if (!functionNode->IsExtern)
				{
					currentFunc = symbolTable->Functions.TryGetValue(functionNode->InternalName)->Ptr();
					this->function = functionNode;
					functionNode->Body->Accept(this);
					this->function = NULL;
					currentFunc = nullptr;
				}
				
			}
			void VisitFunctionDeclaration(FunctionSyntaxNode *functionNode)
			{
				this->function = functionNode;
				auto returnType = functionNode->ReturnType->ToExpressionType();
				if(returnType.BaseType == BaseType::Void && returnType.IsArray)
					Error(30024, L"function return type can not be 'void' array.", functionNode->ReturnType.Ptr());
				StringBuilder internalName;
				internalName << functionNode->Name;
				HashSet<String> paraNames;
				for (auto & para : functionNode->Parameters)
				{
					if (paraNames.Contains(para->Name))
						Error(30002, L"parameter \'" + para->Name + L"\' already defined.", para.Ptr());
					else
						paraNames.Add(para->Name);
					VariableEntry varEntry;
					varEntry.Name = para->Name;
					varEntry.Type.DataType = para->Type->ToExpressionType();
					functionNode->Scope->Variables.AddIfNotExists(varEntry.Name, varEntry);
					if (varEntry.Type.DataType.BaseType == BaseType::Void)
						Error(30016, L"'void' can not be parameter type.", para.Ptr());
					internalName << L"@" << varEntry.Type.DataType.ToString();
				}
				functionNode->InternalName = internalName.ProduceString();	
				RefPtr<FunctionSymbol> symbol = new FunctionSymbol();
				symbol->SyntaxNode = functionNode;
				symbolTable->Functions[functionNode->InternalName] = symbol;
				this->function = NULL;
			}
			
			virtual void VisitBlockStatement(BlockStatementSyntaxNode *stmt) override
			{
				for (auto & node : stmt->Statements)
				{
					node->Accept(this);
				}
			}
			virtual void VisitBreakStatement(BreakStatementSyntaxNode *stmt) override
			{
				if (!loops.Count())
					Error(30003, L"'break' must appear inside loop constructs.", stmt);
			}
			virtual void VisitContinueStatement(ContinueStatementSyntaxNode *stmt) override
			{
				if (!loops.Count())
					Error(30004, L"'continue' must appear inside loop constructs.", stmt);
			}
			virtual void VisitDoWhileStatement(DoWhileStatementSyntaxNode *stmt) override
			{
				loops.Add(stmt);
				if (stmt->Predicate != NULL)
					stmt->Predicate->Accept(this);
				if (stmt->Predicate->Type != ExpressionType::Error && stmt->Predicate->Type != ExpressionType::Int && stmt->Predicate->Type != ExpressionType::Bool)
					Error(30005, L"'while': expression must evaluate to int.", stmt);
				stmt->Statement->Accept(this);

				loops.RemoveAt(loops.Count() - 1);
			}
			virtual void VisitEmptyStatement(EmptyStatementSyntaxNode *){}
			virtual void VisitForStatement(ForStatementSyntaxNode *stmt) override
			{
				loops.Add(stmt);
				VariableEntry iterVar;
				if (stmt->TypeDef != nullptr)
				{
					VariableEntry varEntry;
					varEntry.IsComponent = false;
					varEntry.Name = stmt->IterationVariable.Content;
					varEntry.Type.DataType = stmt->TypeDef->ToExpressionType();
					stmt->Scope->Variables.AddIfNotExists(stmt->IterationVariable.Content, varEntry);
				}
				if (!stmt->Scope->FindVariable(stmt->IterationVariable.Content, iterVar))
					Error(30015, L"undefined identifier \'" + stmt->IterationVariable.Content + L"\'", stmt->IterationVariable);
				else
				{
					if (iterVar.Type.DataType != ExpressionType::Float && iterVar.Type.DataType != ExpressionType::Int)
						Error(30035, L"iteration variable \'" + stmt->IterationVariable.Content + L"\' can only be a int or float", stmt->IterationVariable);
					stmt->InitialExpression->Accept(this);
					if (stmt->InitialExpression->Type != iterVar.Type.DataType)
						Error(30019, L"type mismatch \'" + stmt->InitialExpression->Type.ToString() + L"\' and \'" +
							iterVar.Type.DataType.ToString() + L"\'", stmt->InitialExpression.Ptr());
					stmt->EndExpression->Accept(this);
					if (stmt->EndExpression->Type != iterVar.Type.DataType)
						Error(30019, L"type mismatch \'" + stmt->EndExpression->Type.ToString() + L"\' and \'" +
							iterVar.Type.DataType.ToString() + L"\'", stmt->EndExpression.Ptr());
					if (stmt->StepExpression != nullptr)
					{
						stmt->StepExpression->Accept(this);
						if (stmt->StepExpression->Type != iterVar.Type.DataType)
							Error(30019, L"type mismatch \'" + stmt->StepExpression->Type.ToString() + L"\' and \'" +
								iterVar.Type.DataType.ToString() + L"\'", stmt->StepExpression.Ptr());
					}
				}

				stmt->Statement->Accept(this);

				loops.RemoveAt(loops.Count() - 1);
			}
			virtual void VisitIfStatement(IfStatementSyntaxNode *stmt) override
			{
				if (stmt->Predicate != NULL)
					stmt->Predicate->Accept(this);
				if (stmt->Predicate->Type != ExpressionType::Error && (stmt->Predicate->Type != ExpressionType::Int && stmt->Predicate->Type != ExpressionType::Bool))
					Error(30006, L"'if': expression must evaluate to int.", stmt);

				if (stmt->PositiveStatement != NULL)
					stmt->PositiveStatement->Accept(this);
				
				if (stmt->NegativeStatement != NULL)
					stmt->NegativeStatement->Accept(this);
			}
			virtual void VisitReturnStatement(ReturnStatementSyntaxNode *stmt) override
			{
				if (currentCompNode && currentCompNode->BlockStatement->Statements.Count() &&
					stmt != currentCompNode->BlockStatement->Statements.Last().Ptr())
				{
					Error(30026, L"'return' can only appear as the last statement in component definition.", stmt);
				}
				if (!stmt->Expression)
				{
					if (function && function->ReturnType->ToExpressionType() != ExpressionType::Void)
						Error(30006, L"'return' should have an expression.", stmt);
				}
				else
				{
					stmt->Expression->Accept(this);
					if (stmt->Expression->Type != ExpressionType::Error)
					{
						if (function && stmt->Expression->Type != function->ReturnType->ToExpressionType())
							Error(30007, L"expression type '" + stmt->Expression->Type.ToString()
								+ L"' does not match function's return type '"
								+ function->ReturnType->ToExpressionType().ToString() + L"'", stmt);
						if (currentComp && stmt->Expression->Type != currentComp->Type->DataType)
						{
							Error(30007, L"expression type '" + stmt->Expression->Type.ToString()
								+ L"' does not match component's type '"
								+ currentComp->Type->DataType.ToString() + L"'", stmt);
						}
					}
				}
			}
			virtual void VisitVarDeclrStatement(VarDeclrStatementSyntaxNode *stmt) override
			{
				if (stmt->Type->ToExpressionType().IsTextureType())
				{
					Error(30033, L"cannot declare a local variable of 'texture' type.", stmt);
				}
				for (auto & para : stmt->Variables)
				{
					VariableEntry varDeclr;
					varDeclr.Name = para->Name;
					if (stmt->Scope->Variables.ContainsKey(para->Name))
						Error(30008, L"variable " + para->Name + L" already defined.", para.Ptr());

					varDeclr.Type.DataType = stmt->Type->ToExpressionType();
					if (varDeclr.Type.DataType.BaseType == BaseType::Void)
						Error(30009, L"invalid type 'void'.", stmt);
					if (varDeclr.Type.DataType.IsArray && varDeclr.Type.DataType.ArrayLength <= 0)
						Error(30025, L"array size must be larger than zero.", stmt);

					stmt->Scope->Variables.AddIfNotExists(para->Name, varDeclr);
					if (para->Expression != NULL)
					{
						para->Expression->Accept(this);
						if (para->Expression->Type != varDeclr.Type.DataType && para->Expression->Type != ExpressionType::Error)
						{
							Error(30019, L"type mismatch \'" + para->Expression->Type.ToString() + L"\' and \'" +
								varDeclr.Type.DataType.ToString() + L"\'", para.Ptr());
						}
					}
				}
			}
			virtual void VisitWhileStatement(WhileStatementSyntaxNode *stmt)
			{
				loops.Add(stmt);
				stmt->Predicate->Accept(this);
				if (stmt->Predicate->Type != ExpressionType::Error && stmt->Predicate->Type != ExpressionType::Int && stmt->Predicate->Type != ExpressionType::Bool)
					Error(30010, L"'while': expression must evaluate to int.", stmt);

				stmt->Statement->Accept(this);
				loops.RemoveAt(loops.Count() - 1);
			}
			virtual void VisitExpressionStatement(ExpressionStatementSyntaxNode *stmt)
			{
				stmt->Expression->Accept(this);
			}
			virtual void VisitBinaryExpression(BinaryExpressionSyntaxNode *expr)
			{
				expr->LeftExpression->Accept(this);
				expr->RightExpression->Accept(this);
				auto & leftType = expr->LeftExpression->Type;
				auto & rightType = expr->RightExpression->Type;
				switch (expr->Operator)
				{
				case Operator::Add:
				case Operator::Sub:
				case Operator::Div:
					if (leftType == rightType && !leftType.IsArray && !leftType.IsTextureType() && leftType.BaseType != BaseType::Shader)
						expr->Type = leftType;
					else if (leftType.IsVectorType() && rightType == GetVectorBaseType(leftType.BaseType))
						expr->Type = leftType;
					else if (rightType.IsVectorType() && leftType == GetVectorBaseType(rightType.BaseType))
						expr->Type = rightType;
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Mul:
					if (!leftType.IsArray && leftType.BaseType != BaseType::Shader)
					{
						if (leftType == rightType && !leftType.IsTextureType())
							expr->Type = leftType;
						else if (leftType.BaseType == BaseType::Float3x3 && rightType == ExpressionType::Float3 ||
							leftType.BaseType == BaseType::Float3 && rightType.BaseType == BaseType::Float3x3)
							expr->Type = ExpressionType::Float3;
						else if (leftType.BaseType == BaseType::Float4x4 && rightType == ExpressionType::Float4 ||
							leftType.BaseType == BaseType::Float4 && rightType.BaseType == BaseType::Float4x4)
							expr->Type = ExpressionType::Float4;
						else if (leftType.IsVectorType() && rightType == GetVectorBaseType(leftType.BaseType))
							expr->Type = leftType;
						else if (rightType.IsVectorType() && leftType == GetVectorBaseType(rightType.BaseType))
							expr->Type = rightType;
						else
							expr->Type = ExpressionType::Error;
					}
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Mod:
				case Operator::Rsh:
				case Operator::Lsh:
				case Operator::BitAnd:
				case Operator::BitOr:
				case Operator::BitXor:
				case Operator::And:
				case Operator::Or:
					if (leftType == rightType && !leftType.IsArray && !IsTextureType(GetVectorBaseType(leftType.BaseType))
						&& leftType.BaseType != BaseType::Shader &&
						GetVectorBaseType(leftType.BaseType) != BaseType::Float)
						expr->Type = (expr->Operator == Operator::And || expr->Operator == Operator::Or ? ExpressionType::Bool : leftType);
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Neq:
				case Operator::Eql:
					if (leftType == rightType && !leftType.IsArray && !leftType.IsTextureType() && leftType.BaseType != BaseType::Shader)
						expr->Type = ExpressionType::Bool;
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Greater:
				case Operator::Geq:
				case Operator::Less:
				case Operator::Leq:
					if (leftType == ExpressionType::Int && rightType == ExpressionType::Int)
						expr->Type = ExpressionType::Bool;
					else if (leftType == ExpressionType::Float && rightType == ExpressionType::Float)
						expr->Type = ExpressionType::Bool;
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Assign:
				case Operator::AddAssign:
				case Operator::MulAssign:
				case Operator::DivAssign:
				case Operator::SubAssign:
				case Operator::ModAssign:
					if (!leftType.IsLeftValue && leftType != ExpressionType::Error)
						Error(30011, L"left of '=' is not an l-value.", expr->LeftExpression.Ptr());
					expr->LeftExpression->Access = ExpressionAccess::Write;
					if (leftType == rightType)
						expr->Type = ExpressionType::Void;
					else
						expr->Type = ExpressionType::Error;
					break;
				default:
						expr->Type = ExpressionType::Error;
					break;
				}
				
				if (expr->Type == ExpressionType::Error &&
					leftType != ExpressionType::Error && rightType != ExpressionType::Error)
					Error(30012, L"no overload found for operator " + OperatorToString(expr->Operator)  + L" (" + leftType.ToString() + L", " + rightType.ToString() + L").", expr);
			}
			virtual void VisitConstantExpression(ConstantExpressionSyntaxNode *expr)
			{
				switch (expr->ConstType)
				{
				case ConstantExpressionSyntaxNode::ConstantType::Int:
					expr->Type = ExpressionType::Int;
					break;
				case ConstantExpressionSyntaxNode::ConstantType::Float:
					expr->Type = ExpressionType::Float;
					break;
				default:
					expr->Type = ExpressionType::Error;
					throw "Invalid constant type.";
					break;
				}
			}
			virtual void VisitIndexExpression(IndexExpressionSyntaxNode *expr)
			{
				expr->BaseExpression->Accept(this);
				expr->IndexExpression->Accept(this);
				if (expr->BaseExpression->Type == ExpressionType::Error)
					expr->Type = ExpressionType::Error;
				else
				{
					if (expr->BaseExpression->Type.IsArray &&
						GetVectorSize(expr->BaseExpression->Type.BaseType) == 0)
					{
						Error(30013, L"'[]' can only index on arrays and strings.", expr);
						expr->Type = ExpressionType::Error;
					}
					if (expr->IndexExpression->Type != ExpressionType::Int)
					{
						Error(30014, L"index expression must evaluate to int.", expr);
						expr->Type = ExpressionType::Error;
					}
				}
				if (expr->BaseExpression->Type.IsArray)
					expr->Type.BaseType = expr->BaseExpression->Type.BaseType;
				else
				{
					if (expr->BaseExpression->Type.BaseType == BaseType::Float3x3)
						expr->Type.BaseType = BaseType::Float3;
					else if (expr->BaseExpression->Type.BaseType == BaseType::Float4x4)
						expr->Type.BaseType = BaseType::Float4;
					else
						expr->Type.BaseType = GetVectorBaseType(expr->BaseExpression->Type.BaseType);
				}
				expr->Type.IsLeftValue = true;
				expr->Type.IsReference = true;
			}
			bool MatchArguments(FunctionSyntaxNode * functionNode, List < RefPtr < ExpressionSyntaxNode >> &args)
			{
				if (functionNode->Parameters.Count() != args.Count())
					return false;
				for (int i = 0; i < functionNode->Parameters.Count(); i++)
				{
					if (functionNode->Parameters[i]->Type->ToExpressionType() != args[i]->Type)
						return false;
				}
				return true;
			}
			virtual void VisitInvokeExpression(InvokeExpressionSyntaxNode *expr)
			{
				StringBuilder internalName;
				StringBuilder argList;
				internalName << expr->FunctionExpr->Variable;
				argList << L"(";
				for (int i = 0; i < expr->Arguments.Count(); i++)
				{
					expr->Arguments[i]->Accept(this);
					internalName << L"@" << expr->Arguments[i]->Type.ToString();
					argList << expr->Arguments[i]->Type.ToString();
					if (i != expr->Arguments.Count() - 1)
					{
						argList << L", ";
					}
					if (expr->Arguments[i]->Type == ExpressionType::Error)
					{
						expr->Type = ExpressionType::Error;
						return;
					}
				}
				argList << L")";
				String funcName = internalName.ProduceString();
				RefPtr<FunctionSymbol> func;
				bool found = symbolTable->Functions.TryGetValue(funcName, func);
				if (!found)
				{
					// find function overload with explicit conversions from int -> float
					auto namePrefix = expr->FunctionExpr->Variable + L"@";
					for (auto & f : symbolTable->Functions)
					{
						if (f.Key.StartsWith(namePrefix))
						{
							if (f.Value->SyntaxNode->Parameters.Count() == expr->Arguments.Count())
							{
								bool match = true;
								for (int i = 0; i < expr->Arguments.Count(); i++)
								{
									auto argType = expr->Arguments[i]->Type;
									auto paramType = f.Value->SyntaxNode->Parameters[i]->Type->ToExpressionType();
									if (argType == paramType)
										continue;
									else if (argType.ArrayLength == paramType.ArrayLength
										&& GetVectorBaseType(argType.BaseType) == BaseType::Int && GetVectorBaseType(paramType.BaseType) == BaseType::Float &&
										GetVectorSize(argType.BaseType) == GetVectorSize(argType.BaseType))
										continue;
									else
									{
										match = false;
										break;
									}
								}
								if (match)
								{
									func = f.Value;
									funcName = f.Key;
									found = true;
								}
							}
						}
					}
				}

				if (!found)
				{
					expr->Type = ExpressionType::Error;
					Error(30021, expr->FunctionExpr->Variable + L": no overload takes arguments " + argList.ProduceString(), expr);
				}
				else
				{
					if (!func->SyntaxNode->IsExtern)
					{
						expr->FunctionExpr->Variable = funcName;
						if (currentFunc)
							currentFunc->ReferencedFunctions.Add(funcName);
					}
					expr->Type = func->SyntaxNode->ReturnType->ToExpressionType();
				}
			}

			String OperatorToString(Operator op)
			{
				switch (op)
				{
				case Spire::Compiler::Operator::Neg:
					return L"-";
				case Spire::Compiler::Operator::Not:
					return L"!";
				case Spire::Compiler::Operator::PreInc:
					return L"++";
				case Spire::Compiler::Operator::PreDec:
					return L"--";
				case Spire::Compiler::Operator::PostInc:
					return L"++";
				case Spire::Compiler::Operator::PostDec:
					return L"--";
				case Spire::Compiler::Operator::Mul:
					return L"*";
				case Spire::Compiler::Operator::Div:
					return L"/";
				case Spire::Compiler::Operator::Mod:
					return L"%";
				case Spire::Compiler::Operator::Add:
					return L"+";
				case Spire::Compiler::Operator::Sub:
					return L"-";
				case Spire::Compiler::Operator::Lsh:
					return L"<<";
				case Spire::Compiler::Operator::Rsh:
					return L">>";
				case Spire::Compiler::Operator::Eql:
					return L"==";
				case Spire::Compiler::Operator::Neq:
					return L"!=";
				case Spire::Compiler::Operator::Greater:
					return L">";
				case Spire::Compiler::Operator::Less:
					return L"<";
				case Spire::Compiler::Operator::Geq:
					return L">=";
				case Spire::Compiler::Operator::Leq:
					return L"<=";
				case Spire::Compiler::Operator::BitAnd:
					return L"&";
				case Spire::Compiler::Operator::BitXor:
					return L"^";
				case Spire::Compiler::Operator::BitOr:
					return L"|";
				case Spire::Compiler::Operator::And:
					return L"&&";
				case Spire::Compiler::Operator::Or:
					return L"||";
				case Spire::Compiler::Operator::Assign:
					return L"=";
				default:
					return L"ERROR";
				}
			}
			virtual void VisitUnaryExpression(UnaryExpressionSyntaxNode *expr)
			{
				expr->Expression->Accept(this);
				
				switch (expr->Operator)
				{
				case Operator::Neg:
					if (expr->Expression->Type == ExpressionType::Int ||
						expr->Expression->Type == ExpressionType::Bool ||
						expr->Expression->Type == ExpressionType::Float ||
						expr->Expression->Type.IsVectorType())
						expr->Type = expr->Expression->Type;
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::Not:
				case Operator::BitNot:
					if (expr->Expression->Type == ExpressionType::Int || expr->Expression->Type == ExpressionType::Bool ||
						expr->Expression->Type == ExpressionType::Int2
						|| expr->Expression->Type == ExpressionType::Int3 || expr->Expression->Type == ExpressionType::Int4)
						expr->Type = (expr->Operator == Operator::Not ? ExpressionType::Bool : expr->Expression->Type);
					else
						expr->Type = ExpressionType::Error;
					break;
				case Operator::PostDec:
				case Operator::PostInc:
				case Operator::PreDec:
				case Operator::PreInc:
					if (expr->Expression->Type == ExpressionType::Int)
						expr->Type = ExpressionType::Int;
					else
						expr->Type = ExpressionType::Error;
					break;
				default:
					expr->Type = ExpressionType::Error;
					break;
				}

				if(expr->Type == ExpressionType::Error && expr->Expression->Type != ExpressionType::Error)
					Error(30020, L"operator " + OperatorToString(expr->Operator) + L" can not be applied to " + expr->Expression->Type.ToString(), expr);
			}
			virtual void VisitVarExpression(VarExpressionSyntaxNode *expr)
			{
				VariableEntry variable;
				ShaderUsing shaderObj;
				if (expr->Scope->FindVariable(expr->Variable, variable))
				{
					expr->Type = variable.Type.DataType;
					expr->Type.IsLeftValue = !variable.IsComponent;
				}
				else if (currentShader && currentShader->ShaderObjects.TryGetValue(expr->Variable, shaderObj))
				{
					expr->Type.BaseType = BaseType::Shader;
					expr->Type.Shader = shaderObj.Shader;
					expr->Type.IsLeftValue = false;
				}
				else
				{
					if (currentShader)
					{
						auto compRef = currentShader->ResolveComponentReference(expr->Variable);
						if (compRef.IsAccessible)
						{
							expr->Type = compRef.Component->Type->DataType;
							expr->Type.IsLeftValue = false;
						}
						else if (compRef.Component)
						{
							Error(30017, L"component \'" + expr->Variable + L"\' is not accessible from shader '" + currentShader->SyntaxNode->Name.Content + L"'.", expr);
						}
						else
							Error(30015, L"undefined identifier \'" + expr->Variable + L"\'", expr);
					}
					else
						Error(30015, L"undefined identifier \'" + expr->Variable + L"\'", expr);
				}
			}
			virtual void VisitTypeCastExpression(TypeCastExpressionSyntaxNode * expr) override
			{
				expr->Expression->Accept(this);
				auto targetType = expr->TargetType->ToExpressionType();
				
				if (expr->Expression->Type != ExpressionType::Error)
				{
					if (expr->Expression->Type.IsArray)
						expr->Type = ExpressionType::Error;
					else if (GetVectorBaseType(expr->Expression->Type.BaseType) != BaseType::Int && GetVectorBaseType(expr->Expression->Type.BaseType) != BaseType::Float ||
						GetVectorBaseType(targetType.BaseType) != BaseType::Int && GetVectorBaseType(targetType.BaseType) != BaseType::Float)
						expr->Type = ExpressionType::Error;
					else if (targetType.BaseType == BaseType::Void || expr->Expression->Type.BaseType == BaseType::Void)
						expr->Type = ExpressionType::Error;
					else
						expr->Type = targetType;
				}
				else
					expr->Type = ExpressionType::Error;
				if (expr->Type == ExpressionType::Error && expr->Expression->Type != ExpressionType::Error)
				{
					Error(30022, L"invalid type cast between \"" + expr->Expression->Type.ToString() + L"\" and \"" +
						targetType.ToString() + L"\".", expr);
				}
			}
			virtual void VisitSelectExpression(SelectExpressionSyntaxNode * expr) override
			{
				expr->SelectorExpr->Accept(this);
				if ((expr->SelectorExpr->Type != ExpressionType::Int && expr->SelectorExpr->Type != ExpressionType::Bool) && expr->SelectorExpr->Type != ExpressionType::Error)
				{
					expr->Type = ExpressionType::Error;
					Error(30079, L"selector must evaluate to int.", expr);
				}
				expr->Expr0->Accept(this);
				expr->Expr1->Accept(this);
				if (expr->Expr0->Type != expr->Expr1->Type)
				{
					Error(30080, L"the two value expressions in a select clause must evaluate to same type.", expr);
				}
				expr->Type = expr->Expr0->Type;
			}
			virtual void VisitMemberExpression(MemberExpressionSyntaxNode * expr) override
			{
				expr->BaseExpression->Accept(this);
				auto & baseType = expr->BaseExpression->Type;
				if (baseType.IsArray)
					expr->Type = ExpressionType::Error;
				else if (IsVector(baseType.BaseType))
				{
					Array<int, 4> children;
					if (expr->MemberName.Length() > 4)
						expr->Type = ExpressionType::Error;
					else
					{
						bool error = false;

						for (int i = 0; i < expr->MemberName.Length(); i++)
						{
							auto ch = expr->MemberName[i];
							switch (ch)
							{
							case L'x':
							case L'r':
								children.Add(0);
								break;
							case L'y':
							case L'g':
								children.Add(1);
								break;
							case L'z':
							case L'b':
								children.Add(2);
								break;
							case L'w':
							case L'a':
								children.Add(3);
								break;
							default:
								error = true;
								expr->Type = ExpressionType::Error;
								break;
							}
						}
						int vecLen = GetVectorSize(baseType.BaseType);
						for (auto m : children)
						{
							if (m >= vecLen)
							{
								error = true;
								expr->Type = ExpressionType::Error;
								break;
							}
						}
						if ((vecLen == 9 || vecLen == 16) && children.Count() > 1)
						{
							error = true;
							expr->Type = ExpressionType::Error;
						}
						if (!error)
						{
							expr->Type = baseType;
							if (vecLen == 9)
								expr->Type.BaseType = (BaseType)((int)GetVectorBaseType(baseType.BaseType) + 2);
							else if (vecLen == 16)
								expr->Type.BaseType = (BaseType)((int)GetVectorBaseType(baseType.BaseType) + 15);
							else
							{
								expr->Type.BaseType = (BaseType)((int)GetVectorBaseType(baseType.BaseType) + children.Count() - 1);
							}
						}
						expr->Type.IsLeftValue = true;
					}
				}
				else if (baseType.BaseType == BaseType::Shader)
				{
					ShaderUsing shaderObj;
					auto refComp = baseType.Shader->ResolveComponentReference(expr->MemberName);
					if (refComp.IsAccessible)
						expr->Type = refComp.Component->Type->DataType;
					else if (baseType.Shader->ShaderObjects.TryGetValue(expr->MemberName, shaderObj))
					{
						if (shaderObj.IsPublic)
						{
							expr->Type.BaseType = BaseType::Shader;
							expr->Type.Shader = shaderObj.Shader;
						}
						else
							expr->Type = ExpressionType::Error;
					}
					else
						expr->Type = ExpressionType::Error;
				}
				else
					expr->Type = ExpressionType::Error;
				if (baseType != ExpressionType::Error &&
					expr->Type == ExpressionType::Error)
				{
					Error(30023, L"\"" + baseType.ToString() + L"\" does not have public member \"" +
						expr->MemberName + L"\".", expr);
				}
			}
			virtual void VisitParameter(ParameterSyntaxNode *){}
			virtual void VisitType(TypeSyntaxNode *){}
			virtual void VisitDeclrVariable(Variable *){}
			SemanticsVisitor & operator = (const SemanticsVisitor &) = delete;
		};

		SyntaxVisitor * CreateSemanticsVisitor(SymbolTable * symbols, ErrorWriter * err)
		{
			return new SemanticsVisitor(symbols, err);
		}
		
	}
}

/***********************************************************************
SPIRECORE\SHADERCOMPILER.CPP
***********************************************************************/
// Compiler.cpp : Defines the entry point for the console application.
//

#ifdef CreateDirectory
#undef CreateDirectory
#endif

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace Spire::Compiler;

namespace Spire
{
	namespace Compiler
	{
		class ShaderCompilerImpl : public ShaderCompiler
		{
		private:
			Dictionary<String, RefPtr<CodeGenBackend>> backends;
			Dictionary<String, Dictionary<String, ImportOperatorHandler *>> opHandlers;
			Dictionary<String, Dictionary<String, ExportOperatorHandler *>> exportHandlers;

			void ResolveAttributes(SymbolTable * symTable)
			{
				for (auto & shader : symTable->ShaderDependenceOrder)
				{
					auto comps = shader->GetComponentDependencyOrder();
					for (auto & comp : comps)
					{
						for (auto & impl : comp->Implementations)
							for (auto & attrib : impl->SyntaxNode->LayoutAttributes)
							{
								try
								{
									if (attrib.Value.StartsWith(L"%"))
									{
										CoreLib::Text::Parser parser(attrib.Value.SubString(1, attrib.Value.Length() - 1));
										auto compName = parser.ReadWord();
										parser.Read(L".");
										auto compAttrib = parser.ReadWord();
										RefPtr<ShaderComponentSymbol> compSym;
										if (shader->Components.TryGetValue(compName, compSym))
										{
											for (auto & timpl : compSym->Implementations)
											{
												String attribValue;
												if (timpl->SyntaxNode->LayoutAttributes.TryGetValue(compAttrib, attribValue))
													attrib.Value = attribValue;
											}
										}
									}
								}
								catch (Exception)
								{
								}
							}
					}
				}
			}

			/* Generate a shader variant by applying mechanic choice rules and the choice file.
			   The choice file provides "preferred" definitions, as represented in ShaderComponentSymbol::Type::PinnedWorlds
		       The process resolves the component references by picking a pinned definition if one is available, or a definition
			   with the preferred import path as defined by import operator ordering.
			   After all references are resolved, all unreferenced definitions (dead code) are eliminated, 
			   resulting a shader variant ready for code generation.
			*/
			RefPtr<ShaderIR> GenerateShaderVariantIR(CompileResult & cresult, ShaderClosure * shader, Schedule & schedule)
			{
				RefPtr<ShaderIR> result = new ShaderIR();
				result->Shader = shader;
				// mark pinned worlds
				for (auto & comp : shader->Components)
				{
					for (auto & impl : comp.Value->Implementations)
					{
						for (auto & w : impl->Worlds)
						{
							if (impl->SrcPinnedWorlds.Contains(w) || impl->SyntaxNode->IsInline || impl->ExportWorlds.Contains(w))
							{
								comp.Value->Type->PinnedWorlds.Add(w);
							}
						}
					}
				}
				// apply choices
				Dictionary<String, ShaderComponentSymbol*> choiceComps;
				for (auto & comp : shader->AllComponents)
				{
					for (auto & choiceName : comp.Value->ChoiceNames)
						choiceComps[choiceName] = comp.Value;
				}
				HashSet<ShaderComponentImplSymbol*> pinnedImpl;
				for (auto & choice : schedule.Choices)
				{
					ShaderComponentSymbol * comp = nullptr;
					if (choiceComps.TryGetValue(choice.Key, comp))
					{
						comp->Type->PinnedWorlds.Clear();
						for (auto & selectedDef : choice.Value)
						{
							if (comp->Type->ConstrainedWorlds.Contains(selectedDef->WorldName))
							{
								comp->Type->PinnedWorlds.Add(selectedDef->WorldName);
								// find specified impl
								for (auto & impl : comp->Implementations)
								{
									if (impl->AlternateName == selectedDef->AlternateName && impl->Worlds.Contains(selectedDef->WorldName))
										pinnedImpl.Add(impl.Ptr());
								}
							}
							else
							{
								cresult.GetErrorWriter()->Warning(33101, L"'" + selectedDef->WorldName + L"' is not a valid choice for '" + choice.Key
									+ L"'.", selectedDef.Ptr()->Position);
							}
						}
					}
				}
				for (auto & attribs : schedule.AddtionalAttributes)
				{
					ShaderComponentSymbol * comp = nullptr;
					if (choiceComps.TryGetValue(attribs.Key, comp))
					{
						// apply attributes
						for (auto & impl : comp->Implementations)
						{
							for (auto & attrib : attribs.Value)
								impl->SyntaxNode->LayoutAttributes[attrib.Key] = attrib.Value;
						}
					}
				}
				// generate definitions
				for (auto & comp : shader->AllComponents)
				{
					EnumerableDictionary<String, ComponentDefinitionIR*> defs;
					for (auto & impl : comp.Value->Implementations)
					{
						for (auto & w : impl->Worlds)
						{
							RefPtr<ComponentDefinitionIR> def = new ComponentDefinitionIR();
							def->Component = comp.Value;
							def->Implementation = impl.Ptr();
							def->World = w;
							result->Definitions.Add(def);
							bool existingDefIsPinned = false;
							if (auto existingDef = defs.TryGetValue(w))
								existingDefIsPinned = pinnedImpl.Contains((*existingDef)->Implementation);
							if (!existingDefIsPinned)
								defs[w] = def.Ptr();
						}
					}
					result->DefinitionsByComponent[comp.Key] = defs;
				}
				bool changed = true;
				while (changed)
				{
					changed = false;
					result->ResolveComponentReference();
					result->EliminateDeadCode();
					// check circular references
					for (auto & def : result->Definitions)
					{
						if (def->Dependency.Contains(def.Ptr()))
						{
							cresult.GetErrorWriter()->Error(33102, L"component definition \'" + def->Component->Name + L"\' involves circular reference.",
								def->Implementation->SyntaxNode->Position);
							return nullptr;
						}
					}
					/*
					// eliminate redundant (downstream) definitions, one at a time
					auto comps = result->GetComponentDependencyOrder();
					for (int i = comps.Count() - 1; i >= 0; i--)
					{
						auto comp = comps[i];
						auto & defs = result->DefinitionsByComponent[comp->UniqueName]();
						EnumerableHashSet<ComponentDefinitionIR*> removedDefs;
						for (auto & def : defs)
							if (!def.Value->IsEntryPoint && !comp->Type->PinnedWorlds.Contains(def.Value->World))
							{
								for (auto & otherDef : defs)
								{
									if (otherDef.Value != def.Value && !removedDefs.Contains(otherDef.Value)
										&& shader->Pipeline->IsWorldReachable(otherDef.Value->World, def.Value->World))
									{
										removedDefs.Add(def.Value);
										break;
									}
								}
							}
						if (removedDefs.Count())
						{
							result->RemoveDefinitions([&](ComponentDefinitionIR* def) {return removedDefs.Contains(def); });
							changed = true;
						}
					}
					*/
				}
				return result;
			}
		public:
			virtual CompileUnit Parse(CompileResult & result, String source, String fileName) override
			{
				result.Success = false;
				Lexer lexer;
				auto tokens = lexer.Parse(fileName, source, result.ErrorList);
				Parser parser(tokens, result.ErrorList, fileName);
				CompileUnit rs;
				rs.SyntaxNode = parser.Parse();
				return rs;
			}
			virtual void Compile(CompileResult & result, List<CompileUnit> & units, const CompileOptions & options) override
			{
				result.Success = false;
				RefPtr<ProgramSyntaxNode> programSyntaxNode = new ProgramSyntaxNode();
				for (auto & unit : units)
				{
					programSyntaxNode->Include(unit.SyntaxNode.Ptr());
				}

				SymbolTable symTable;
				RefPtr<SyntaxVisitor> visitor = CreateSemanticsVisitor(&symTable, result.GetErrorWriter());
				try
				{
					programSyntaxNode->Accept(visitor.Ptr());
					visitor = nullptr;
					if (result.ErrorList.Count() > 0)
						return;
					symTable.EvalFunctionReferenceClosure();
					if (result.ErrorList.Count() > 0)
						return;
					List<RefPtr<ShaderClosure>> shaderClosures;

					for (auto & shader : symTable.ShaderDependenceOrder)
					{
						if (shader->IsAbstract)
							continue;
						auto shaderClosure = CreateShaderClosure(result.GetErrorWriter(), &symTable, shader);
						FlattenShaderClosure(result.GetErrorWriter(), shaderClosure.Ptr());
						shaderClosures.Add(shaderClosure);
					}
					
					ResolveAttributes(&symTable);

					if (result.ErrorList.Count() > 0)
						return;

					Schedule schedule;
					if (options.ScheduleSource != L"")
					{
						schedule = Schedule::Parse(options.ScheduleSource, options.ScheduleFileName, result.ErrorList);
					}
					for (auto shader : shaderClosures)
					{
						// generate shader variant from schedule file, and also apply mechanic deduction rules
						shader->IR = GenerateShaderVariantIR(result, shader.Ptr(), schedule);
					}
					if (options.Mode == CompilerMode::ProduceShader)
					{
						if (result.ErrorList.Count() > 0)
							return;
						// generate IL code
						RefPtr<ICodeGenerator> codeGen = CreateCodeGenerator(this, &symTable, result);
						for (auto & func : programSyntaxNode->Functions)
							codeGen->ProcessFunction(func.Ptr());
						for (auto & shader : shaderClosures)
							codeGen->ProcessShader(shader.Ptr());
						if (result.ErrorList.Count() > 0)
							return;
						// emit target code
						EnumerableHashSet<String> symbolsToGen;
						for (auto & shader : units[0].SyntaxNode->Shaders)
							if (!shader->IsModule)
								symbolsToGen.Add(shader->Name.Content);
						for (auto & func : units[0].SyntaxNode->Functions)
							symbolsToGen.Add(func->Name);
						auto IsSymbolToGen = [&](String & shaderName)
						{
							if (symbolsToGen.Contains(shaderName))
								return true;
							for (auto & symbol : symbolsToGen)
								if (shaderName.StartsWith(symbol))
									return true;
							return false;
						};
						for (auto & shader : result.Program->Shaders)
						{
							if (options.SymbolToCompile.Length() == 0 && IsSymbolToGen(shader->MetaData.ShaderName)
								|| options.SymbolToCompile == shader->MetaData.ShaderName)
							{
								StringBuilder glslBuilder;
								StringBuilder cppBuilder;
								cppBuilder << CppCodeIncludeString1 << CppCodeIncludeString2;
								Dictionary<String, String> targetCode;
								result.CompiledSource[shader->MetaData.ShaderName + L".glsl"] = EnumerableDictionary<String, CompiledShaderSource>();
								auto & worldSources = result.CompiledSource[shader->MetaData.ShaderName + L".glsl"]();
								for (auto & world : shader->Worlds)
								{
									if (world.Value->IsAbstract)
										continue;
									RefPtr<CodeGenBackend> backend;
									if (!backends.TryGetValue(world.Value->TargetMachine, backend))
									{
										result.GetErrorWriter()->Error(40000, L"backend '" + world.Value->TargetMachine + L"' is not supported.",
											world.Value->WorldDefPosition);
									}
									else
									{
										backend->SetParameters(world.Value->BackendParameters);
										Dictionary<String, ImportOperatorHandler*> importHandlers;
										Dictionary<String, ExportOperatorHandler*> beExportHandlers;

										opHandlers.TryGetValue(world.Value->TargetMachine, importHandlers);
										exportHandlers.TryGetValue(world.Value->TargetMachine, beExportHandlers);

										worldSources[world.Key] = backend->GenerateShaderWorld(result, &symTable, world.Value.Ptr(), importHandlers, beExportHandlers);
									}
								}
							}
						}
						result.Success = result.ErrorList.Count() == 0;
					}
					else if (options.Mode == CompilerMode::GenerateChoice)
					{
						for (auto shader : shaderClosures)
						{
							if (options.SymbolToCompile.Length() == 0 || shader->Name == options.SymbolToCompile)
							{
								auto &worldOrder = shader->Pipeline->GetWorldTopologyOrder();
								for (auto & comp : shader->AllComponents)
								{
									ShaderChoice choice;
									if (comp.Value->ChoiceNames.Count() == 0)
										continue;
									if (comp.Value->IsParam())
										continue;
									choice.ChoiceName = comp.Value->ChoiceNames.First();
									for (auto & impl : comp.Value->Implementations)
									{
										for (auto w : impl->Worlds)
											if (comp.Value->Type->ConstrainedWorlds.Contains(w))
												choice.Options.Add(ShaderChoiceValue(w, impl->AlternateName));
									}
									if (auto defs = shader->IR->DefinitionsByComponent.TryGetValue(comp.Key))
									{
										int latestWorldOrder = -1;
										for (auto & def : *defs)
										{
											int order = worldOrder.IndexOf(def.Key);
											if (latestWorldOrder < order)
											{
												choice.DefaultValue = def.Key;
												latestWorldOrder = order;
											}
										}
									}
									result.Choices.Add(choice);
								}
							}
						}
					}
					else
					{
						result.GetErrorWriter()->Error(2, L"unsupported compiler mode.", CodePosition());
						return;
					}
					result.Success = true;
				}
				catch (int)
				{
				}
				catch (...)
				{
					throw;
				}
				return;
			}

			virtual void RegisterImportOperator(String backendName, ImportOperatorHandler * handler) override
			{
				if (!opHandlers.ContainsKey(backendName))
					opHandlers[backendName] = Dictionary<String, ImportOperatorHandler*>();
				opHandlers[backendName]().Add(handler->GetName(), handler);
			}

			virtual void RegisterExportOperator(String backendName, ExportOperatorHandler * handler) override
			{
				if (!exportHandlers.ContainsKey(backendName))
					exportHandlers[backendName] = Dictionary<String, ExportOperatorHandler*>();
				exportHandlers[backendName]().Add(handler->GetName(), handler);
			}

			ShaderCompilerImpl()
			{
				backends.Add(L"glsl", CreateGLSLCodeGen());
			}
		};

		ShaderCompiler * CreateShaderCompiler()
		{
			return new ShaderCompilerImpl();
		}

	}
}

/***********************************************************************
SPIRECORE\STDINCLUDE.CPP
***********************************************************************/

const wchar_t * VertexShaderIncludeString = LR"(
__builtin out vec4 gl_Position;
)";

const wchar_t * LibIncludeString = LR"(
__intrinsic float dFdx(float v);
__intrinsic float dFdy(float v);
__intrinsic float fwidth(float v);
__intrinsic vec2 dFdx(vec2 v);
__intrinsic vec2 dFdy(vec2 v);
__intrinsic vec2 fwidth(vec2 v);
__intrinsic vec3 dFdx(vec3 v);
__intrinsic vec3 dFdy(vec3 v);
__intrinsic vec3 fwidth(vec3 v);
__intrinsic vec4 dFdx(vec4 v);
__intrinsic vec4 dFdy(vec4 v);
__intrinsic vec4 fwidth(vec4 v);

__intrinsic vec3 normalize(vec3 v);
__intrinsic float dot(vec2 v0, vec2 v1);
__intrinsic float dot(vec3 v0, vec3 v1);
__intrinsic float dot(vec4 v0, vec4 v1);
__intrinsic float sin(float v);
__intrinsic float cos(float v);
__intrinsic float tan(float v);
__intrinsic float sqrt(float v);
__intrinsic vec2 sin(vec2 v);
__intrinsic vec2 cos(vec2 v);
__intrinsic vec2 tan(vec2 v);
__intrinsic vec2 sqrt(vec2 v);
__intrinsic vec3 sin(vec3 v);
__intrinsic vec3 cos(vec3 v);
__intrinsic vec3 tan(vec3 v);
__intrinsic vec3 sqrt(vec3 v);
__intrinsic vec4 sin(vec4 v);
__intrinsic vec4 cos(vec4 v);
__intrinsic vec4 tan(vec4 v);
__intrinsic vec4 sqrt(vec4 v);
__intrinsic float abs(float v);
__intrinsic vec2 abs(vec2 v);
__intrinsic vec3 abs(vec3 v);
__intrinsic vec4 abs(vec4 v);
__intrinsic float exp(float v);
__intrinsic float log(float v);
__intrinsic float exp2(float v);
__intrinsic float log2(float v);
__intrinsic float asin(float v);
__intrinsic float acos(float v);
__intrinsic float atan(float v);
__intrinsic float sign(float x);
__intrinsic float pow(float base, float e);
__intrinsic vec2 pow(vec2 base, vec2 e);
__intrinsic vec3 pow(vec3 base, vec3 e);
__intrinsic vec4 pow(vec4 base, vec4 e);
__intrinsic float atan2(float x, float y);
__intrinsic float floor(float v);
__intrinsic vec2 floor(vec2 v);
__intrinsic vec3 floor(vec3 v);
__intrinsic vec4 floor(vec4 v);
__intrinsic float fract(float v);
__intrinsic vec2 fract(vec2 v);
__intrinsic vec3 fract(vec3 v);
__intrinsic vec4 fract(vec4 v);
__intrinsic float ceil(float v);
__intrinsic vec2 ceil(vec2 v);
__intrinsic vec3 ceil(vec3 v);
__intrinsic vec4 ceil(vec4 v);
__intrinsic float step(float v, float y);
__intrinsic vec2 step(vec2 v, vec2 v1);
__intrinsic vec3 step(vec3 v, vec3 v1);
__intrinsic vec4 step(vec4 v, vec4 v1);
__intrinsic float smoothstep(float e0, float e1, float v);
__intrinsic vec2 smoothstep(vec2 e0, vec2 e1, vec2 v);
__intrinsic vec3 smoothstep(vec3 e0, vec3 e1, vec3 v);
__intrinsic vec4 smoothstep(vec4 e0, vec4 e1, vec4 v);
__intrinsic vec4 texture(sampler2D tex, vec2 coord);
__intrinsic vec4 texture(samplerCube tex, vec3 coord);
__intrinsic vec4 texture(sampler2D tex, vec2 coord, vec2 dPdx, vec2 dPdy);
__intrinsic vec4 textureGrad(sampler2D tex, vec2 coord, vec2 dPdx, vec2 dPdy);
__intrinsic vec4 textureGrad(samplerCube tex, vec3 coord, vec3 dPdx, vec3 dPdy);
__intrinsic vec4 texture(samplerCube tex, vec3 coord, float bias);
__intrinsic float texture(sampler2DShadow tex, vec3 coord);
__intrinsic float texture(samplerCubeShadow tex, vec4 coord);
__intrinsic float textureProj(sampler2DShadow tex, vec4 coord);
__intrinsic float textureProj(samplerCubeShadow tex, vec4 coord);
__intrinsic float diff(float v);
__intrinsic float mod(float x, float y);
__intrinsic float max(float v);
__intrinsic float min(float v);
__intrinsic float max(float v, float v1);
__intrinsic float min(float v, float v1);
__intrinsic vec2 max(vec2 v, vec2 v1);
__intrinsic vec2 min(vec2 v, vec2 v1);
__intrinsic vec3 max(vec3 v, vec3 v1);
__intrinsic vec3 min(vec3 v, vec3 v1);
__intrinsic vec4 max(vec4 v, vec4 v1);
__intrinsic vec4 min(vec4 v, vec4 v1);
__intrinsic float clamp(float v, float v1, float v2);
__intrinsic vec2 clamp(vec2 v, vec2 v1, vec2 v2);
__intrinsic vec3 clamp(vec3 v, vec3 v1, vec3 v2);
__intrinsic vec4 clamp(vec4 v, vec4 v1, vec4 v2);

__intrinsic vec3 reflect(vec3 I, vec3 N);
__intrinsic vec3 reflect(vec3 I, vec3 N, float eta);

__intrinsic float length(vec2 v);
__intrinsic float length(vec3 v);
__intrinsic float length(vec4 v);

__intrinsic void alphaTest(float alpha, float threshold);
__intrinsic vec3 mix(vec3 v0, vec3 v1, float t);
__intrinsic vec4 mix(vec4 v0, vec4 v1, float t);
__intrinsic vec2 mix(vec2 v0, vec2 v1, float t);
__intrinsic float mix(float v0, float v1, float t);
__intrinsic vec3 mix(vec3 v0, vec3 v1, vec3 t);
__intrinsic vec4 mix(vec4 v0, vec4 v1, vec4 t);
__intrinsic vec2 mix(vec2 v0, vec2 v1, vec2 t);
__intrinsic mat3 mat3(vec3 a, vec3 b, vec3 c);
__intrinsic mat3 mat3(float a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
__intrinsic vec3 cross(vec3 v1, vec3 v2);
__intrinsic vec2 vec2(float v);
__intrinsic vec3 vec3(float v);
__intrinsic vec4 vec4(float v);
__intrinsic vec2 vec2(float x, float y);
__intrinsic vec3 vec3(float x, float y, float z);
__intrinsic vec3 vec3(vec2 v, float z);
__intrinsic vec4 vec4(float x, float y, float z, float w);
__intrinsic vec4 vec4(vec3 v, float w);
__intrinsic vec4 vec4(vec2 v, float z, float w);
__intrinsic vec4 vec4(vec2 v, vec2 w);
__intrinsic ivec2 ivec2(int x, int y);
__intrinsic ivec3 ivec3(int x, int y, int z);
__intrinsic ivec3 ivec3(ivec2 v, int z);
__intrinsic ivec4 ivec4(int x, int y, int z, int w);
__intrinsic ivec4 ivec4(ivec3 v, int w);
__intrinsic ivec4 ivec4(ivec2 v, int z, int w);
__intrinsic ivec4 ivec4(ivec2 v, ivec2 w);
__intrinsic mat3 transpose(mat3 in);
__intrinsic mat4 transpose(mat4 in);
#line_reset#
)";

/***********************************************************************
SPIRECORE\SYMBOLTABLE.CPP
***********************************************************************/

namespace Spire
{
	namespace Compiler
	{
		bool SymbolTable::SortShaders()
		{
			HashSet<ShaderSymbol*> shaderSet;
			ShaderDependenceOrder.Clear();
			List<ShaderSymbol *> nextShaders, currentShaders;
			// sort shaders in dependency order
			for (auto & shader : Shaders)
			{
				if (shader.Value->DependentShaders.Count() == 0)
				{
					ShaderDependenceOrder.Add(shader.Value.Ptr());
					shaderSet.Add(shader.Value.Ptr());
				}
				else
					currentShaders.Add(shader.Value.Ptr());
			}
			while (currentShaders.Count())
			{
				nextShaders.Clear();
				for (auto & shader : currentShaders)
				{
					bool pass = true;
					for (auto & dshader : shader->DependentShaders)
						if (!shaderSet.Contains(dshader))
						{
							pass = false;
							break;
						}
					if (pass)
					{
						ShaderDependenceOrder.Add(shader);
						shaderSet.Add(shader);
					}
					else
						nextShaders.Add(shader);
				}
				currentShaders.SwapWith(nextShaders);
			}
			return (ShaderDependenceOrder.Count() == Shaders.Count());
		}
		void SymbolTable::EvalFunctionReferenceClosure()
		{
			for (auto & func : Functions)
			{
				List<String> funcList;
				EnumerableHashSet<String> funcSet;
				for (auto & ref : func.Value->ReferencedFunctions)
				{
					funcList.Add(ref);
					funcSet.Add(ref);
				}
				for (int i = 0; i < funcList.Count(); i++)
				{
					RefPtr<FunctionSymbol> funcSym;
					if (Functions.TryGetValue(funcList[i], funcSym))
					{
						for (auto rfunc : funcSym->ReferencedFunctions)
						{
							if (funcSet.Add(rfunc))
								funcList.Add(rfunc);
						}
					}
				}
				func.Value->ReferencedFunctions = _Move(funcSet);
			}
		}

		bool PipelineSymbol::IsAbstractWorld(String world)
		{
			WorldSymbol ws;
			if (Worlds.TryGetValue(world, ws))
				return ws.IsAbstract;
			return false;
		}

		bool PipelineSymbol::IsWorldReachable(String src, String targetWorld)
		{
			if (src == targetWorld)
				return true;
			if (ReachableWorlds.ContainsKey(src))
				if (ReachableWorlds[src]().Contains(targetWorld))
					return true;
			return false;
		}

		bool PipelineSymbol::IsWorldDirectlyReachable(String src, String targetWorld)
		{
			if (src == targetWorld)
				return true;
			for (auto & op : SyntaxNode->ImportOperators)
				if (op->SourceWorld.Content == src && op->DestWorld.Content == targetWorld)
					return true;
			return false;
		}

		List<String>& PipelineSymbol::GetWorldTopologyOrder()
		{
			if (WorldTopologyOrder.Count() != 0)
				return WorldTopologyOrder;
			List<String> rs;
			HashSet<String> rsSet;
			bool changed = true;
			while (changed)
			{
				changed = false;
				for (auto & w : WorldDependency)
				{
					if (!rsSet.Contains(w.Key))
					{
						bool canAdd = true;
						for (auto & dw : w.Value)
							if (!rsSet.Contains(dw))
							{
								canAdd = false;
								break;
							}
						if (canAdd)
						{
							rsSet.Add(w.Key);
							rs.Add(w.Key);
							changed = true;
						}
					}
				}
			}
			WorldTopologyOrder = _Move(rs);
			return WorldTopologyOrder;
		}
		
		bool PipelineSymbol::IsWorldReachable(EnumerableHashSet<String>& src, String targetWorld)
		{
			for (auto srcW : src)
			{
				if (srcW == targetWorld)
					return true;
				if (ReachableWorlds.ContainsKey(srcW))
					if (ReachableWorlds[srcW]().Contains(targetWorld))
						return true;
			}
			return false;
		}
		
		List<ImportPath> PipelineSymbol::FindImportOperatorChain(String worldSrc, String worldDest)
		{
			List<ImportPath> resultPathes;
			if (worldSrc == worldDest)
				return resultPathes;
			List<ImportPath> pathes, pathes2;
			pathes.Add(ImportPath());
			pathes[0].Nodes.Add(ImportPath::Node(worldSrc, nullptr));
			while (pathes.Count())
			{
				pathes2.Clear();
				for (auto & p : pathes)
				{
					String world0 = p.Nodes.Last().TargetWorld;
					for (auto op : SyntaxNode->ImportOperators)
					{
						if (op->SourceWorld.Content == world0)
						{
							ImportPath np = p;
							np.Nodes.Add(ImportPath::Node(op->DestWorld.Content, op.Ptr()));
							if (op->DestWorld.Content == worldDest)
								resultPathes.Add(np);
							else
								pathes2.Add(np);
						}
					}
				}
				pathes.SwapWith(pathes2);
			}
			return resultPathes;
		}
		List<ImportOperatorDefSyntaxNode*> PipelineSymbol::GetImportOperatorsFromSourceWorld(String worldSrc)
		{
			List<ImportOperatorDefSyntaxNode*> rs;
			for (auto & op : this->SyntaxNode->ImportOperators)
			{
				if (op->SourceWorld.Content == worldSrc)
					rs.Add(op.Ptr());
			}
			return rs;
		}
		List<ShaderComponentSymbol*> ShaderSymbol::GetComponentDependencyOrder()
		{
			List<ShaderComponentSymbol*> components;

			for (auto & comp : Components)
			{
				components.Add(comp.Value.Ptr());
			}
			SortComponents(components);
			return components;
		}
		void ShaderSymbol::SortComponents(List<ShaderComponentSymbol*>& comps)
		{
			comps.Sort([&](ShaderComponentSymbol*c0, ShaderComponentSymbol*c1)
			{
				return c0->Implementations.First()->SyntaxNode->Position < c1->Implementations.First()->SyntaxNode->Position;
			});
			HashSet<ShaderComponentSymbol*> allSymbols, addedSymbols;
			for (auto & comp : comps)
				allSymbols.Add(comp);
			List<ShaderComponentSymbol*> sorted;
			bool changed = true;
			while (changed)
			{
				changed = false;
				for (auto & comp : comps)
				{
					if (!addedSymbols.Contains(comp))
					{
						bool isFirst = true;
						for (auto & impl : comp->Implementations)
							for (auto & dep : impl->DependentComponents)
								if (allSymbols.Contains(dep) && !addedSymbols.Contains(dep))
								{
									isFirst = false;
									goto loopEnd;
								}
					loopEnd:;
						if (isFirst)
						{
							addedSymbols.Add(comp);
							sorted.Add(comp);
							changed = true;
						}
					}
				}
			}
			comps = _Move(sorted);
		}

		ShaderSymbol::ComponentReference ShaderSymbol::ResolveComponentReference(String compName, bool topLevel)
		{
			ComponentReference result;
			result.IsAccessible = true;
			RefPtr<ShaderComponentSymbol> refComp, privateRefComp;
			if (Components.TryGetValue(compName, refComp))
			{
				result.Component = refComp.Ptr();
				return result;
			}
			for (auto & shaderUsing : ShaderUsings)
			{
				if (shaderUsing.Shader->Components.TryGetValue(compName, refComp))
				{
					if (refComp->Implementations.First()->SyntaxNode->IsPublic)
					{
						result.Component = refComp.Ptr();
						result.IsAccessible = true;
						return result;
					}
					else
					{
						result.Component = refComp.Ptr();
						result.IsAccessible = false;
					}
				}
				else if (shaderUsing.IsPublic || topLevel)
				{
					auto rresult = shaderUsing.Shader->ResolveComponentReference(compName, false);
					if (rresult.IsAccessible)
						return rresult;
					else
						result = rresult;
				}
			}
			result.IsAccessible = false;
			return result;
		}

		void ShaderIR::EliminateDeadCode()
		{
			// mark entry points
			auto MarkUsing = [&](String compName, String userWorld)
			{
				if (auto defs = DefinitionsByComponent.TryGetValue(compName))
				{
					if (auto def = defs->TryGetValue(userWorld))
						(*def)->IsEntryPoint = true;
					else
					{
						for (auto & world : Shader->Pipeline->WorldDependency[userWorld]())
						{
							if (auto def2 = defs->TryGetValue(world))
							{
								(*def2)->IsEntryPoint = true;
								break;
							}
						}
					}
				}
			};
			for (auto & impOp : Shader->Pipeline->SyntaxNode->ImportOperators)
				for (auto & ref : impOp->Usings)
					MarkUsing(ref.Content, impOp->DestWorld.Content);
			for (auto & w : Shader->Pipeline->SyntaxNode->Worlds)
				for (auto & ref : w->Usings)
					MarkUsing(ref.Content, w->Name.Content);
			for (auto & comp : Definitions)
				if (comp->Implementation->ExportWorlds.Contains(comp->World) ||
					Shader->Pipeline->IsAbstractWorld(comp->World) &&
					(comp->Implementation->SyntaxNode->LayoutAttributes.ContainsKey(L"Pinned") || Shader->Pipeline->Worlds[comp->World]().SyntaxNode->LayoutAttributes.ContainsKey(L"Pinned")))
				{
					comp->IsEntryPoint = true;
				}

			List<ComponentDefinitionIR*> workList;
			HashSet<ComponentDefinitionIR*> referencedDefs;
			for (auto & def : Definitions)
			{
				if (def->IsEntryPoint)
				{
					if (referencedDefs.Add(def.Ptr()))
						workList.Add(def.Ptr());
				}
			}
			for (int i = 0; i < workList.Count(); i++)
			{
				auto def = workList[i];
				for (auto & dep : def->Dependency)
				{
					if (referencedDefs.Add(dep))
						workList.Add(dep);
				}
			}
			List<RefPtr<ComponentDefinitionIR>> newDefinitions;
			for (auto & def : Definitions)
			{
				if (referencedDefs.Contains(def.Ptr()))
				{
					newDefinitions.Add(def);
					EnumerableHashSet<ComponentDefinitionIR*> newSet;
					for (auto & comp : def->Users)
						if (referencedDefs.Contains(comp))
						{
							newSet.Add(comp);
						}
					def->Users = newSet;
					newSet.Clear();
					for (auto & comp : def->Dependency)
						if (referencedDefs.Contains(comp))
						{
							newSet.Add(comp);
						}
					def->Dependency = newSet;
				}
			}
			Definitions = _Move(newDefinitions);
			for (auto & kv : DefinitionsByComponent)
			{
				for (auto & def : kv.Value)
					if (!referencedDefs.Contains(def.Value))
						kv.Value.Remove(def.Key);
			}
		}
		void ShaderIR::ResolveComponentReference()
		{
			// build bidirectional dependency map of component definitions
			for (auto & comp : Definitions)
			{
				comp->Dependency.Clear();
				comp->Users.Clear();
			}
			for (auto & comp : Definitions)
			{
				List<ShaderComponentSymbol *> workList;
				for (auto & dep : comp->Implementation->DependentComponents)
					workList.Add(dep);
				HashSet<ShaderComponentSymbol*> proceseedDefCompss;
				for (int i = 0; i < workList.Count(); i++)
				{
					auto dep = workList[i];
					if (!proceseedDefCompss.Add(dep))
						continue;
					auto & depDefs = DefinitionsByComponent[dep->UniqueName]();
					// select the best overload according to import operator ordering,
					// prefer user-pinned definitions (as provided in the choice file)
					List<String> depWorlds;
					depWorlds.Add(comp->World);
					for (auto & w : Shader->Pipeline->WorldDependency[comp->World]())
						depWorlds.Add(w);
					for (int pass = 0; pass < 2; pass++)
					{
						// in the first pass, examine the pinned definitions only
						// in the second pass, examine all the rest definitions
						for (auto & depWorld : depWorlds)
						{
							bool isPinned = false;
							for (auto existingDef : dep->Type->PinnedWorlds)
								if (existingDef.StartsWith(depWorld))
								{
									isPinned = true;
									break;
								}
							if (pass == 0 && !isPinned || pass == 1 && isPinned) continue;
							ComponentDefinitionIR * depDef;
							if (depDefs.TryGetValue(depWorld, depDef))
							{
								comp->Dependency.Add(depDef);
								depDef->Users.Add(comp.Ptr());
								// add additional dependencies due to import operators
								if (depWorld != comp->World)
								{
									auto importPath = Shader->Pipeline->FindImportOperatorChain(depWorld, comp->World);
									if (importPath.Count() == 0)
										throw InvalidProgramException(L"no import path found.");
									auto & usings = importPath.First().Nodes.Last().ImportOperator->Usings;
									for (auto & importUsing : usings)
									{
										ShaderComponentSymbol* refComp;
										if (!Shader->AllComponents.TryGetValue(importUsing.Content, refComp))
											throw InvalidProgramException(L"import operator dependency not exists.");
										workList.Add(refComp);
									}
								}
								goto selectionEnd; // first preferred overload is found, terminate searching
							}
						}
					}
					selectionEnd:;
				}
			}
		}
		List<ShaderComponentSymbol*> ShaderIR::GetComponentDependencyOrder()
		{
			List<ShaderComponentSymbol*> result, workList;
			HashSet<String> set;
			for (auto & comp : DefinitionsByComponent)
			{
				bool emptyDependency = true;
				for (auto & def : comp.Value)
					if (def.Value->Dependency.Count())
					{
						emptyDependency = false;
						break;
					}
				if (emptyDependency)
				{
					workList.Add(Shader->AllComponents[comp.Key]());
				}
			}
			for (int i = 0; i < workList.Count(); i++)
			{
				auto comp = workList[i];
				if (!set.Contains(comp->UniqueName))
				{
					bool insertable = true;
					for (auto & def : DefinitionsByComponent[comp->UniqueName]())
					{
						for (auto & dep : def.Value->Dependency)
							if (!set.Contains(dep->Component->UniqueName))
							{
								insertable = false;
								goto breakLoc;
							}
					}
				breakLoc:;
					if (insertable)
					{
						if (set.Add(comp->UniqueName))
						{
							result.Add(comp);
							for (auto & def : DefinitionsByComponent[comp->UniqueName]())
								for (auto & user : def.Value->Users)
									workList.Add(user->Component);
						}
					}
				}
			}
			return result;
		}
		bool CheckComponentImplementationConsistency(ErrorWriter * err, ShaderComponentSymbol * comp, ShaderComponentImplSymbol * impl)
		{
			bool rs = true;
			if (impl->SyntaxNode->Rate)
			{
				for (auto & cimpl : comp->Implementations)
				{
					for (auto & w : cimpl->Worlds)
						if (impl->Worlds.Contains(w) && impl->AlternateName == cimpl->AlternateName)
						{
							err->Error(33020, L"\'" + comp->Name + L"\' is already defined at '" + w + L"\'.", impl->SyntaxNode->Position);
							rs = false;
						}
				}
			}
			else
			{
				for (auto & cimpl : comp->Implementations)
				{
					if (cimpl->Worlds.Count() == 0 && impl->Worlds.Count() == 0 && impl->AlternateName == cimpl->AlternateName)
					{
						err->Error(33020, L"\'" + comp->Name + L"\' is already defined.", impl->SyntaxNode->Position);
						rs = false;
					}
				}
			}
			for (auto & cimpl : comp->Implementations)
			{
				if (impl->SyntaxNode->IsOutput != cimpl->SyntaxNode->IsOutput)
				{
					err->Error(33021, L"\'" + comp->Name + L"\': inconsistent signature.\nsee previous definition at " + cimpl->SyntaxNode->Position.ToString(), impl->SyntaxNode->Position);
					rs = false;
					break;
				}
				if (impl->SyntaxNode->IsParam != cimpl->SyntaxNode->IsParam)
				{
					err->Error(33021, L"\'" + comp->Name + L"\': inconsistent signature.\nsee previous definition at " + cimpl->SyntaxNode->Position.ToString(), impl->SyntaxNode->Position);
					rs = false;
					break;
				}
				if (impl->SyntaxNode->IsPublic != cimpl->SyntaxNode->IsPublic)
				{
					err->Error(33021, L"\'" + comp->Name + L"\': inconsistent signature.\nsee previous definition at " + cimpl->SyntaxNode->Position.ToString(), impl->SyntaxNode->Position);
					rs = false;
					break;
				}
				if (impl->SyntaxNode->Type->ToExpressionType() != cimpl->SyntaxNode->Type->ToExpressionType())
				{
					err->Error(33021, L"\'" + comp->Name + L"\': inconsistent signature.\nsee previous definition at " + cimpl->SyntaxNode->Position.ToString(), impl->SyntaxNode->Position);
					rs = false;
					break;
				}
			}
			if (impl->SyntaxNode->IsParam && comp->Implementations.Count() != 0)
			{
				err->Error(33022, L"\'" + comp->Name + L"\': parameter name conflicts with existing definition.", impl->SyntaxNode->Position);
				rs = false;
			}
			return rs;
		}

		int GUID::currentGUID = 0;
		void GUID::Clear()
		{
			currentGUID = 0;
		}
		int GUID::Next()
		{
			return currentGUID++;
		}
		RefPtr<ShaderComponentSymbol> ShaderClosure::FindComponent(String name, bool findInPrivate)
		{
			RefPtr<ShaderComponentSymbol> rs;
			if (RefMap.TryGetValue(name, rs))
				return rs;
			if (Components.TryGetValue(name, rs))
				return rs;
			for (auto & subClosure : SubClosures)
			{
				if (subClosure.Value->IsInPlace)
				{
					rs = subClosure.Value->FindComponent(name);
					if (rs && (findInPrivate || rs->Implementations.First()->SyntaxNode->IsPublic))
						return rs;
					else
						rs = nullptr;
				}
			}
			return rs;
		}
		RefPtr<ShaderClosure> ShaderClosure::FindClosure(String name)
		{
			RefPtr<ShaderClosure> rs;
			if (SubClosures.TryGetValue(name, rs))
				return rs;
			for (auto & subClosure : SubClosures)
			{
				if (subClosure.Value->IsInPlace)
				{
					rs = subClosure.Value->FindClosure(name);
					if (rs && rs->IsPublic)
						return rs;
					else
						rs = nullptr;
				}
			}
			return rs;
		}
		List<ShaderComponentSymbol*> ShaderClosure::GetDependencyOrder()
		{
			List<ShaderComponentSymbol*> comps;
			for (auto & comp : AllComponents)
				comps.Add(comp.Value);
			comps.Sort([&](ShaderComponentSymbol*c0, ShaderComponentSymbol*c1)
			{
				return c0->Implementations.First()->SyntaxNode->Position < c1->Implementations.First()->SyntaxNode->Position;
			});
			HashSet<ShaderComponentSymbol*> allSymbols, addedSymbols;
			for (auto & comp : comps)
				allSymbols.Add(comp);
			List<ShaderComponentSymbol*> sorted;
			bool changed = true;
			while (changed)
			{
				changed = false;
				for (auto & comp : comps)
				{
					if (!addedSymbols.Contains(comp))
					{
						bool isFirst = true;
						for (auto & impl : comp->Implementations)
							for (auto & dep : impl->DependentComponents)
								if (allSymbols.Contains(dep) && !addedSymbols.Contains(dep))
								{
									isFirst = false;
									goto loopEnd;
								}
					loopEnd:;
						if (isFirst)
						{
							addedSymbols.Add(comp);
							sorted.Add(comp);
							changed = true;
						}
					}
				}
			}
			return sorted;
		}
	}
}

/***********************************************************************
SPIRECORE\SYNTAX.CPP
***********************************************************************/

namespace Spire
{
	namespace Compiler
	{
		ExpressionType ExpressionType::Bool(Compiler::BaseType::Bool);
		ExpressionType ExpressionType::Int(Compiler::BaseType::Int);
		ExpressionType ExpressionType::Float(Compiler::BaseType::Float);
		ExpressionType ExpressionType::Int2(Compiler::BaseType::Int2);
		ExpressionType ExpressionType::Float2(Compiler::BaseType::Float2);
		ExpressionType ExpressionType::Int3(Compiler::BaseType::Int3);
		ExpressionType ExpressionType::Float3(Compiler::BaseType::Float3);
		ExpressionType ExpressionType::Int4(Compiler::BaseType::Int4);
		ExpressionType ExpressionType::Float4(Compiler::BaseType::Float4);
		ExpressionType ExpressionType::Void(Compiler::BaseType::Void);
		ExpressionType ExpressionType::Error(Compiler::BaseType::Error);

		bool Scope::FindVariable(const String & name, VariableEntry & variable)
		{
			if (Variables.TryGetValue(name, variable))
				return true;
			if (Parent)
				return Parent->FindVariable(name, variable);
			return false;
		}

		CoreLib::Basic::String ExpressionType::ToString()
		{
			CoreLib::Basic::StringBuilder res;

			switch (BaseType)
			{
			case Compiler::BaseType::Int:
				res.Append(L"int");
				break;
			case Compiler::BaseType::Float:
				res.Append(L"float");
				break;
			case Compiler::BaseType::Int2:
				res.Append(L"ivec2");
				break;
			case Compiler::BaseType::Float2:
				res.Append(L"vec2");
				break;
			case Compiler::BaseType::Int3:
				res.Append(L"ivec3");
				break;
			case Compiler::BaseType::Float3:
				res.Append(L"vec3");
				break;
			case Compiler::BaseType::Int4:
				res.Append(L"ivec4");
				break;
			case Compiler::BaseType::Float4:
				res.Append(L"vec4");
				break;
			case Compiler::BaseType::Float3x3:
				res.Append(L"mat3");
				break;
			case Compiler::BaseType::Float4x4:
				res.Append(L"mat4");
				break;
			case Compiler::BaseType::Texture2D:
				res.Append(L"sampler2D");
				break;
			case Compiler::BaseType::TextureCube:
				res.Append(L"samplerCube");
				break;
			case Compiler::BaseType::TextureShadow:
				res.Append(L"samplerShadow");
				break;
			case Compiler::BaseType::TextureCubeShadow:
				res.Append(L"samplerCubeShadow");
				break;
			case Compiler::BaseType::Function:
				res.Append(L"(");
				for (int i = 0; i < Func->Parameters.Count(); i++)
				{
					if (i > 0)
						res.Append(L",");
					res.Append(Func->Parameters[i]->Type->ToExpressionType().ToString());
				}
				res.Append(L") => ");
				res.Append(Func->ReturnType->ToExpressionType().ToString());
				break;
			case Compiler::BaseType::Shader:
				res.Append(Shader->SyntaxNode->Name.Content);
				break;
			case Compiler::BaseType::Void:
				res.Append("void");
				break;
			default:
				break;
			}
			if (ArrayLength != 0)
			{
				res.Append(L"[");
				res.Append(CoreLib::Basic::String(ArrayLength));
				res.Append(L"]");
			}
			return res.ToString();
		}


		void ProgramSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitProgram(this);
		}
		ProgramSyntaxNode * ProgramSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ProgramSyntaxNode(*this), ctx);
			rs->Functions.Clear();
			for (auto & x : Functions)
				rs->Functions.Add(x->Clone(ctx));
			rs->Pipelines.Clear();
			for (auto & x : Pipelines)
				rs->Pipelines.Add(x->Clone(ctx));
			rs->Shaders.Clear();
			for (auto & x : Shaders)
				rs->Shaders.Add(x->Clone(ctx));
			return rs;
		}
		void FunctionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitFunction(this);
		}
		FunctionSyntaxNode * FunctionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new FunctionSyntaxNode(*this), ctx);
			rs->Parameters.Clear();
			for (auto & param : Parameters)
			{
				rs->Parameters.Add(param->Clone(ctx));
			}
			rs->ReturnType = ReturnType->Clone(ctx);
			rs->Body = Body->Clone(ctx);
			return rs;
		}
		void BlockStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitBlockStatement(this);
		}
		BlockStatementSyntaxNode * BlockStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new BlockStatementSyntaxNode(*this), ctx);
			rs->Statements.Clear();
			for (auto & stmt : Statements)
			{
				rs->Statements.Add(stmt->Clone(ctx));
			}
			return rs;
		}
		void BreakStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitBreakStatement(this);
		}
		BreakStatementSyntaxNode * BreakStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new BreakStatementSyntaxNode(*this), ctx);
		}
		void ContinueStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitContinueStatement(this);
		}
		ContinueStatementSyntaxNode * ContinueStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new ContinueStatementSyntaxNode(*this), ctx);
		}
		void DoWhileStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitDoWhileStatement(this);
		}
		DoWhileStatementSyntaxNode * DoWhileStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new DoWhileStatementSyntaxNode(*this), ctx);
			if (Predicate)
				rs->Predicate = Predicate->Clone(ctx);
			if (Statement)
				rs->Statement = Statement->Clone(ctx);
			return rs;
		}
		void EmptyStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitEmptyStatement(this);
		}
		EmptyStatementSyntaxNode * EmptyStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new EmptyStatementSyntaxNode(*this), ctx);
		}
		void ForStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitForStatement(this);
		}
		ForStatementSyntaxNode * ForStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ForStatementSyntaxNode(*this), ctx);
			if (InitialExpression)
				rs->InitialExpression = InitialExpression->Clone(ctx);
			if (StepExpression)
				rs->StepExpression = StepExpression->Clone(ctx);
			if (EndExpression)
				rs->EndExpression = EndExpression->Clone(ctx);
			if (Statement)
				rs->Statement = Statement->Clone(ctx);
			rs->TypeDef = TypeDef->Clone(ctx);
			return rs;
		}
		void IfStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitIfStatement(this);
		}
		IfStatementSyntaxNode * IfStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new IfStatementSyntaxNode(*this), ctx);
			if (Predicate)
				rs->Predicate = Predicate->Clone(ctx);
			if (PositiveStatement)
				rs->PositiveStatement = PositiveStatement->Clone(ctx);
			if (NegativeStatement)
				rs->NegativeStatement = NegativeStatement->Clone(ctx);
			return rs;
		}
		void ReturnStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitReturnStatement(this);
		}
		ReturnStatementSyntaxNode * ReturnStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ReturnStatementSyntaxNode(*this), ctx);
			if (Expression)
				rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void VarDeclrStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitVarDeclrStatement(this);
		}
		VarDeclrStatementSyntaxNode * VarDeclrStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new VarDeclrStatementSyntaxNode(*this), ctx);
			rs->Type = Type->Clone(ctx);
			rs->Variables.Clear();
			for (auto & var : Variables)
				rs->Variables.Add(var->Clone(ctx));
			return rs;
		}
		void Variable::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitDeclrVariable(this);
		}
		Variable * Variable::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new Variable(*this), ctx);
			if (Expression)
				rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void WhileStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitWhileStatement(this);
		}
		WhileStatementSyntaxNode * WhileStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new WhileStatementSyntaxNode(*this), ctx);
			if (Predicate)
				rs->Predicate = Predicate->Clone(ctx);
			if (Statement)
				rs->Statement = Statement->Clone(ctx);
			return rs;
		}
		void ExpressionStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitExpressionStatement(this);
		}
		ExpressionStatementSyntaxNode * ExpressionStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ExpressionStatementSyntaxNode(*this), ctx);
			if (Expression)
				rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void BinaryExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitBinaryExpression(this);
		}
		BinaryExpressionSyntaxNode * BinaryExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new BinaryExpressionSyntaxNode(*this), ctx);
			rs->LeftExpression = LeftExpression->Clone(ctx);
			rs->RightExpression = RightExpression->Clone(ctx);
			return rs;
		}
		void ConstantExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitConstantExpression(this);
		}
		ConstantExpressionSyntaxNode * ConstantExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new ConstantExpressionSyntaxNode(*this), ctx);
		}
		IndexExpressionSyntaxNode * IndexExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new IndexExpressionSyntaxNode(*this), ctx);
			rs->BaseExpression = BaseExpression->Clone(ctx);
			rs->IndexExpression = IndexExpression->Clone(ctx);
			return rs;
		}
		void IndexExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitIndexExpression(this);
		}
		void MemberExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitMemberExpression(this);
		}
		MemberExpressionSyntaxNode * MemberExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new MemberExpressionSyntaxNode(*this), ctx);
			rs->BaseExpression = BaseExpression->Clone(ctx);
			return rs;
		}
		void InvokeExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitInvokeExpression(this);
		}
		InvokeExpressionSyntaxNode * InvokeExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new InvokeExpressionSyntaxNode(*this), ctx);
			rs->FunctionExpr = FunctionExpr->Clone(ctx);
			rs->Arguments.Clear();
			for (auto & arg : Arguments)
			{
				rs->Arguments.Add(arg->Clone(ctx));
			}
			return rs;
		}
		void TypeCastExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitTypeCastExpression(this);
		}
		TypeCastExpressionSyntaxNode * TypeCastExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new TypeCastExpressionSyntaxNode(*this), ctx);
			rs->TargetType = TargetType->Clone(ctx);
			rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void SelectExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitSelectExpression(this);
		}
		SelectExpressionSyntaxNode * SelectExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new SelectExpressionSyntaxNode(*this), ctx);
			rs->SelectorExpr = SelectorExpr->Clone(ctx);
			rs->Expr0 = Expr0->Clone(ctx);
			rs->Expr1 = Expr1->Clone(ctx);
			return rs;
		}
		void UnaryExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitUnaryExpression(this);
		}
		UnaryExpressionSyntaxNode * UnaryExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new UnaryExpressionSyntaxNode(*this), ctx);
			rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void VarExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitVarExpression(this);
		}
		VarExpressionSyntaxNode * VarExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new VarExpressionSyntaxNode(*this), ctx);
		}
		void ParameterSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitParameter(this);
		}
		ParameterSyntaxNode * ParameterSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ParameterSyntaxNode(*this), ctx);
			rs->Type = Type->Clone(ctx);
			rs->Expr = Expr->Clone(ctx);
			return rs;
		}
		void TypeSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitType(this);
		}
		TypeSyntaxNode * TypeSyntaxNode::FromExpressionType(ExpressionType t)
		{
			TypeSyntaxNode * rs = new TypeSyntaxNode();
			ExpressionType expType;
			if (t.BaseType == BaseType::Int)
				rs->TypeName = L"int";
			else if (t.BaseType == BaseType::Float)
				rs->TypeName = L"float";
			else if (t.BaseType == BaseType::Int2)
				rs->TypeName = L"ivec2";
			else if (t.BaseType == BaseType::Int3)
				rs->TypeName = L"ivec3";
			else if (t.BaseType == BaseType::Int4)
				rs->TypeName = L"ivec4";
			else if (t.BaseType == BaseType::Float2)
				rs->TypeName = L"vec2";
			else if (t.BaseType == BaseType::Float3)
				rs->TypeName = L"vec3";
			else if (t.BaseType == BaseType::Float4)
				rs->TypeName = L"vec4";
			else if (t.BaseType == BaseType::Float3x3)
				rs->TypeName = L"mat3";
			else if (t.BaseType == BaseType::Float4x4)
				rs->TypeName = L"mat4";
			else if (t.BaseType == BaseType::Texture2D)
				rs->TypeName = L"sampler2D";
			else if (t.BaseType == BaseType::TextureCube)
				rs->TypeName = L"samplerCube";
			else if (t.BaseType == BaseType::TextureShadow)
				rs->TypeName = L"samplerShadow";
			else if (t.BaseType == BaseType::TextureCubeShadow)
				rs->TypeName = L"samplerCubeShadow";
			rs->ArrayLength = 0;
			rs->IsArray = false;		
			return rs;
		}
		void ComponentSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitComponent(this);
		}
		ComponentSyntaxNode * ComponentSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ComponentSyntaxNode(*this), ctx);
			rs->Type = Type->Clone(ctx);
			if (Rate)
				rs->Rate = Rate->Clone(ctx);
			if (BlockStatement)
				rs->BlockStatement = BlockStatement->Clone(ctx);
			if (Expression)
				rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void ShaderSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitShader(this);
		}
		ShaderSyntaxNode * ShaderSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ShaderSyntaxNode(*this), ctx);
			rs->Members.Clear();
			for (auto & comp : Members)
				rs->Members.Add(comp->Clone(ctx));
			return rs;
		}
		RateSyntaxNode * RateSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new RateSyntaxNode(*this), ctx);
		}
		WorldSyntaxNode * WorldSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new WorldSyntaxNode(*this), ctx);
		}
		ImportOperatorDefSyntaxNode * ImportOperatorDefSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new ImportOperatorDefSyntaxNode(*this), ctx);
		}
		PipelineSyntaxNode * PipelineSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new PipelineSyntaxNode(*this), ctx);
			rs->Worlds.Clear();
			for (auto & w : Worlds)
				rs->Worlds.Add(w->Clone(ctx));
			rs->ImportOperators.Clear();
			for (auto & imp : ImportOperators)
				rs->ImportOperators.Add(imp->Clone(ctx));
			rs->AbstractComponents.Clear();
			for (auto & comp : AbstractComponents)
				rs->AbstractComponents.Add(comp->Clone(ctx));
			return rs;
		}
		ChoiceValueSyntaxNode * ChoiceValueSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new ChoiceValueSyntaxNode(*this), ctx);
		}
		void ImportSyntaxNode::Accept(SyntaxVisitor * v)
		{
			v->VisitImport(this);
		}
		ImportSyntaxNode * ImportSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ImportSyntaxNode(*this), ctx);
			rs->Arguments.Clear();
			for (auto & arg : Arguments)
				rs->Arguments.Add(arg->Clone(ctx));
			return rs;
		}
		void ImportArgumentSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitImportArgument(this);
		}
		ImportArgumentSyntaxNode * ImportArgumentSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ImportArgumentSyntaxNode(*this), ctx);
			rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void ImportStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitImportStatement(this);
		}
		ImportStatementSyntaxNode * ImportStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ImportStatementSyntaxNode(*this), ctx);
			rs->Import = Import->Clone(ctx);
			return rs;
		}
	}
}

/***********************************************************************
SPIRELIB\CPPIMPORTOPERATORHANDLERS.CPP
***********************************************************************/
using namespace Spire::Compiler;
using namespace CoreLib::Basic;

class CppImportOperatorHandler : public ImportOperatorHandler
{
public:
	virtual void GenerateSetInput(StringBuilder &, ComponentDefinition *, const ImportOperatorContext &) override
	{}
	virtual void GeneratePreamble(StringBuilder &, InterfaceBlock *, const ImportOperatorContext &) override
	{}
	virtual void GenerateEpilogue(StringBuilder &, InterfaceBlock *, const ImportOperatorContext &) override
	{}
};

class StandardCppImportOperatorHandler : public CppImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"standardImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext &) override
	{
		sb << L"in " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			sb << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		auto block = ctx.SourceWorld->WorldOutput;
		instr->Name = L"blk" + block->Name + L"." + instr->ComponentName;
	}
};

class VertexCppImportOperatorHandler : public CppImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"vertexImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & ctx) override
	{
		int location = 0;
		for (auto & ent : block->Entries)
		{
			sb << L"layout(location = " << location << L") ";
			sb << L"in " << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
			location++;
		}
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder &sb, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		instr->Name = instr->ComponentName;
	}
};

class UniformCppImportOperatorHandler : public CppImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"uniformImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & ctx) override
	{
		sb << L"layout(std140";
		String strIndex;
		if (block->Attributes.TryGetValue(L"Index", strIndex))
			sb << L", binding = " << strIndex;
		if (ctx.BackendArguments.ContainsKey(L"command_list"))
			sb << L", commandBindableNV";
		sb << L") ";
		sb << L"uniform " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			sb << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		auto block = ctx.SourceWorld->WorldOutput;
		instr->Name = L"blk" + block->Name + L"." + instr->ComponentName;
	}
};


class TextureCppImportOperatorHandler : public CppImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"textureImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & ctx) override
	{
		sb << L"layout(std140";
		String strIndex;
		if (block->Attributes.TryGetValue(L"Index", strIndex))
			sb << L", binding = " << strIndex;
		if (ctx.BackendArguments.ContainsKey(L"command_list"))
			sb << L", commandBindableNV";
		sb << L") ";
		sb << L"uniform " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			sb << L"sampler2D " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & context) override
	{
		if (instr->Arguments.Count() != 1)
		{
			context.Result.GetErrorWriter()->Error(50001, L"missing import operator argument.", instr->ImportOperator->Position);
			return;
		}
		sb << instr->Type->ToString() << L" " << instr->Name << L" = " << instr->Type->ToString() << "(texture(blk" << context.SourceWorld->WorldOutput->Name
			<< L"." << instr->ComponentName << L", " << instr->Arguments[0]->Name << L")";
		int vecSize = instr->Type->GetVectorSize();
		if (vecSize <= 1)
			sb << L".x";
		else if (vecSize == 2)
			sb << L".xy";
		else if (vecSize == 3)
			sb << L".xyz";
		sb << L");\n";
	}
};

class StandardExportOperatorHandler : public ExportOperatorHandler
{
	virtual String GetName() override
	{
		return L"standardExport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block) override
	{
		sb << L"out " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			sb << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateExport(StringBuilder & sb, InterfaceBlock * block, CompiledWorld *, String componentName, String valueVar) override
	{
		sb << L"blk" << block->Name << L"." << componentName << L" = " << valueVar << L";\n";
	}
};


void CreateCppImportOperatorHandlers(CoreLib::Basic::List<Spire::Compiler::ImportOperatorHandler *> & handlers)
{
	handlers.Add(new StandardCppImportOperatorHandler());
	handlers.Add(new UniformCppImportOperatorHandler());
	handlers.Add(new TextureCppImportOperatorHandler());
}

/***********************************************************************
SPIRELIB\GLSLIMPORTOPERATORHANDLERS.CPP
***********************************************************************/
using namespace Spire::Compiler;
using namespace CoreLib::Basic;

class GLSLImportOperatorHandler : public ImportOperatorHandler
{
public:
	virtual void GenerateSetInput(StringBuilder &, ComponentDefinition *, const ImportOperatorContext &) override
	{}
	virtual void GeneratePreamble(StringBuilder &, InterfaceBlock *, const ImportOperatorContext &) override
	{}
	virtual void GenerateEpilogue(StringBuilder &, InterfaceBlock *, const ImportOperatorContext &) override
	{}
};

class StandardGLSLImportOperatorHandler : public GLSLImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"standardImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext &) override
	{
		if (block->Entries.Count() == 0)
			return;
		sb << L"in " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			sb << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		auto block = ctx.SourceWorld->WorldOutput;
		instr->Name = L"blk" + block->Name + L"." + instr->ComponentName;		
	}
};

class VertexGLSLImportOperatorHandler : public GLSLImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"vertexImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & ctx) override
	{
		int location = 0;
		for (auto & ent : block->Entries)
		{
			sb << L"layout(location = " << location << L") ";
			sb << L"in " << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
			location++;
		}
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder &sb, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		instr->Name = instr->ComponentName;
	}
};

void GenerateBufferInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block)
{
	String strIdx;
	if (block->Attributes.TryGetValue(L"Index", strIdx))
		sb << L"layout(location = " << strIdx << L") ";
	sb << L"uniform float * " << block->Name << L";" << EndLine;
}

class BufferGLSLImportOperatorHandler : public GLSLImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"bufferImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext &) override
	{
		GenerateBufferInterfaceDefinition(sb, block);
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		auto block = ctx.SourceWorld->WorldOutput;
		sb << instr->Type->ToString() << L" " << instr->Name << L";\n";
		if (instr->Type->IsTexture())
		{
			sb << instr->Name << L" = *(" << instr->Type->ToString() << L"*)(" << block->Name <<
				L" + " << String(block->Entries[instr->ComponentName].GetValue().Offset / 4) << L" + gl_GlobalInvocationID.x * "
				<< String(block->Size / 4) << L");" << EndLine;
		}
		else
		{
			int vecSize = instr->Type->GetVectorSize();
			for (int i = 0; i < vecSize; i++)
			{
				sb << instr->Name;
				if (vecSize > 1)
				{
					if (vecSize == 9)
						sb << L"[" << i/3 << L"][" << i%3 << L"]";
					else if (vecSize == 16)
						sb << L"[" << i / 4 << L"][" << i % 4 << L"]";
					else
						sb << L"[" << i << L"]";
				}
				sb << L" = *";
				if (instr->Type->IsIntVector() || instr->Type->IsInt())
					sb << L"(int*)";
				sb<< L"(" << block->Name <<
					L" + " << String(block->Entries[instr->ComponentName].GetValue().Offset / 4 + i) << L" + gl_GlobalInvocationID.x * "
					<< String(block->Size / 4) << L");" << EndLine;
			}
		}
	}

	virtual void GeneratePreamble(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & context) override
	{
		sb << L"if (gl_GlobalInvocationID.x >= sys_thread_count) return;" << EndLine;
	}
	virtual void GenerateEpilogue(StringBuilder &, InterfaceBlock *, const ImportOperatorContext &) override
	{}
};

class BufferGLSLExportOperatorHandler : public ExportOperatorHandler
{
	virtual String GetName() override
	{
		return L"bufferExport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block) override
	{
		GenerateBufferInterfaceDefinition(sb, block);
		sb << L"uniform uint sys_thread_count;" << EndLine;
		sb << L"layout(local_size_x = 256) in;" << EndLine;
	}
	virtual void GenerateExport(StringBuilder & sb, InterfaceBlock * block, CompiledWorld *, String componentName, String valueVar) override
	{
		auto & comp = block->Entries[componentName].GetValue();
		if (comp.Type->IsTexture() || comp.Type->GetVectorSize() > 4)
		{
			throw NotImplementedException(L"exporting sampler2D or matrices is not supported.");
		}
		else
		{
			for (int i = 0; i < comp.Type->GetVectorSize(); i++)
			{
				sb << L"*";
				if (comp.Type->IsIntVector() || comp.Type->IsInt())
					sb << L"(int*)";

				sb << L"(" << block->Name <<
					L" + " << String(block->Entries[componentName].GetValue().Offset / 4 + i) << L" + gl_GlobalInvocationID.x * "
					<< String(block->Size / 4) << L") = " << valueVar;
				if (comp.Type->GetVectorSize() > 1)
					sb << L"[" << i << L"]";
				sb << L";" << EndLine;
			}
		}
	}
	virtual void GeneratePreamble(StringBuilder & sb, InterfaceBlock *) override
	{
		sb << L"if (gl_GlobalInvocationID.x >= sys_thread_count) return;" << EndLine;
	}
	virtual void GenerateEpilogue(StringBuilder &, InterfaceBlock *) override
	{}
};

class UniformGLSLImportOperatorHandler : public GLSLImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"uniformImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & ctx) override
	{
		if (block->Entries.Count() == 0)
			return;
		sb << L"layout(std140";
		String strIndex;
		if (block->Attributes.TryGetValue(L"Index", strIndex))
			sb << L", binding = " << strIndex;
		if (ctx.BackendArguments.ContainsKey(L"command_list"))
			sb << L", commandBindableNV";
		sb << L") ";
		sb << L"uniform " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			sb << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & ctx) override
	{
		auto block = ctx.SourceWorld->WorldOutput;
		instr->Name = L"blk" + block->Name + L"." + instr->ComponentName;
	}
};


class TextureGLSLImportOperatorHandler : public GLSLImportOperatorHandler
{
	virtual String GetName() override
	{
		return L"textureImport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block, const ImportOperatorContext & ctx) override
	{
		if (block->Entries.Count() == 0)
			return;
		sb << L"layout(std140";
		String strIndex;
		if (block->Attributes.TryGetValue(L"Index", strIndex))
			sb << L", binding = " << strIndex;
		if (ctx.BackendArguments.ContainsKey(L"command_list"))
			sb << L", commandBindableNV";
		sb << L") ";
		sb << L"uniform " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			sb << L"sampler2D " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateInterfaceLocalDefinition(StringBuilder & sb, ImportInstruction * instr, const ImportOperatorContext & context) override
	{
		if (instr->Arguments.Count() != 1)
		{
			context.Result.GetErrorWriter()->Error(50001, L"missing import operator argument.", instr->ImportOperator->Position);
			return;
		}
		sb << instr->Type->ToString() << L" " << instr->Name << L" = " << instr->Type->ToString() << "(texture(blk" << context.SourceWorld->WorldOutput->Name
			<< L"."<< instr->ComponentName << L", " << instr->Arguments[0]->Name << L")";
		int vecSize = instr->Type->GetVectorSize();
		if (vecSize <= 1)
			sb << L".x";
		else if (vecSize == 2)
			sb << L".xy";
		else if (vecSize == 3)
			sb << L".xyz";
		CompiledComponent ccomp;
		if (context.SourceWorld->LocalComponents.TryGetValue(instr->ComponentName, ccomp))
		{
			if (ccomp.Attributes.ContainsKey(L"Normal"))
			{
				sb << L" * 2.0 - 1.0";
			}
		}
		sb << L");\n";
	}
};

class StandardGLSLExportOperatorHandler : public ExportOperatorHandler
{
	virtual String GetName() override
	{
		return L"standardExport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block) override
	{
		if (block->Entries.Count() == 0)
			return;
		sb << L"out " << block->Name << L"\n{\n";
		for (auto & ent : block->Entries)
		{
			sb << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
		}
		sb << L"} blk" << block->Name << L";\n";
	}
	virtual void GenerateExport(StringBuilder & sb, InterfaceBlock * block, CompiledWorld *, String componentName, String valueVar) override
	{
		sb << L"blk" << block->Name << L"." << componentName << L" = " << valueVar << L";\n";
	}
	virtual void GeneratePreamble(StringBuilder &, InterfaceBlock *) override
	{}
	virtual void GenerateEpilogue(StringBuilder &, InterfaceBlock *) override
	{}
};

class FragmentGLSLExportOperatorHandler : public ExportOperatorHandler
{
	virtual String GetName() override
	{
		return L"fragmentExport";
	}
	virtual void GenerateInterfaceDefinition(StringBuilder & sb, InterfaceBlock * block) override
	{
		if (block->Entries.Count() == 0)
			return;
		int idx = 0;
		for (auto & ent : block->Entries)
		{
			if (!ent.Value.LayoutAttribs.ContainsKey(L"DepthOutput"))
			{
				sb << L"layout(location = " << idx << L") out " << ent.Value.Type->ToString() << L" " << ent.Key << L";\n";
				idx++;
			}
		}
	}
	virtual void GenerateExport(StringBuilder & sb, InterfaceBlock * block, CompiledWorld * world, String componentName, String valueVar) override
	{
		CompiledComponent ccomp;
		bool isNormal = false;
		bool isDepthOutput = false;
		if (world->LocalComponents.TryGetValue(componentName, ccomp))
		{
			if (ccomp.Attributes.ContainsKey(L"Normal"))
				isNormal = true;
			if (ccomp.Attributes.ContainsKey(L"DepthOutput"))
				isDepthOutput = true;
		}
		if (isDepthOutput)
			sb << L"gl_FragDepth";
		else
			sb << componentName;
		sb << L" = ";
		if (isNormal)
			sb << valueVar << L" * 0.5 + 0.5";
		else
			sb << valueVar;
		sb << L";\n";
	}	
	virtual void GeneratePreamble(StringBuilder &, InterfaceBlock *) override
	{}
	virtual void GenerateEpilogue(StringBuilder &, InterfaceBlock *) override
	{}
};


void CreateGLSLImportOperatorHandlers(CoreLib::Basic::List<Spire::Compiler::ImportOperatorHandler *> & handlers)
{
	handlers.Add(new StandardGLSLImportOperatorHandler());
	handlers.Add(new UniformGLSLImportOperatorHandler());
	handlers.Add(new TextureGLSLImportOperatorHandler());
	handlers.Add(new VertexGLSLImportOperatorHandler());
	handlers.Add(new BufferGLSLImportOperatorHandler());
}

void CreateGLSLExportOperatorHandlers(CoreLib::Basic::List<Spire::Compiler::ExportOperatorHandler *> & handlers)
{
	handlers.Add(new StandardGLSLExportOperatorHandler());
	handlers.Add(new FragmentGLSLExportOperatorHandler());
	handlers.Add(new BufferGLSLExportOperatorHandler());
}


/***********************************************************************
SPIRELIB\IMPORTOPERATOR.CPP
***********************************************************************/
using namespace Spire::Compiler;
using namespace CoreLib::Basic;

void DestroyImportOperatorHanlders(CoreLib::Basic::List<Spire::Compiler::ImportOperatorHandler *> & handlers)
{
	for (auto handler : handlers)
		delete handler;
	handlers.Clear();
}

void DestroyExportOperatorHanlders(CoreLib::Basic::List<Spire::Compiler::ExportOperatorHandler *> & handlers)
{
	for (auto handler : handlers)
		delete handler;
	handlers.Clear();
}

/***********************************************************************
SPIRELIB\SPIRELIB.CPP
***********************************************************************/

using namespace CoreLib::Basic;
using namespace CoreLib::IO;
using namespace CoreLib::Text;
using namespace Spire::Compiler;

namespace SpireLib
{
	void ReadSource(EnumerableDictionary<CoreLib::Basic::String, CompiledShaderSource> & sources, CoreLib::Text::Parser & parser, String src)
	{
		auto getShaderSource = [&]()
		{
			auto token = parser.ReadToken();
			int endPos = token.Position + 1;
			int brace = 0;
			while (endPos < src.Length() && !(src[endPos] == L'}' && brace == 0))
			{
				if (src[endPos] == L'{')
					brace++;
				else if (src[endPos] == L'}')
					brace--;
				endPos++;
			}
			while (!parser.IsEnd() && parser.NextToken().Position != endPos)
				parser.ReadToken();
			parser.ReadToken();
			return src.SubString(token.Position + 1, endPos - token.Position - 1);
		};
		while (!parser.IsEnd() && !parser.LookAhead(L"}"))
		{
			auto worldName = parser.ReadWord();
			CompiledShaderSource src;
			src.ParseFromGLSL(getShaderSource());
			sources[worldName] = src;
		}
	}
	CompiledShaderSource ShaderLib::GetWorldSource(String world)
	{
		CompiledShaderSource rs;
		Sources.TryGetValue(world, rs);
		return rs;
	}
	ShaderLib::ShaderLib(CoreLib::Basic::String fileName)
	{
		Reload(fileName);
	}
	void ShaderLib::Reload(CoreLib::Basic::String fileName)
	{
		Load(fileName);
	}
	bool ShaderLib::CompileFrom(String symbolName, String sourceFileName, String schedule)
	{
		CompileResult result;
		CompileOptions options;
		options.ScheduleSource = schedule;
		options.SymbolToCompile = symbolName;
		options.Mode = CompilerMode::ProduceShader;
		auto shaderLibs = CompileShaderSource(result, sourceFileName, options);
		if (result.Success)
		{
			for (auto & lib : shaderLibs)
			{
				if (lib.MetaData.ShaderName == symbolName)
				{
					FromString(shaderLibs[0].ToString());
					return true;
				}
			}
		}
		result.PrintError(true);
		return false;
	}

	List<ShaderLibFile> CompileUnits(Spire::Compiler::CompileResult & compileResult,
		ShaderCompiler * compiler, List<CompileUnit> & units,
		Spire::Compiler::CompileOptions & options)
	{
		List<ShaderLibFile> resultFiles;
		List<ImportOperatorHandler*> importHandlers, cppImportHandlers;
		List<ExportOperatorHandler*> exportHandlers;
		CreateGLSLImportOperatorHandlers(importHandlers);
		CreateGLSLExportOperatorHandlers(exportHandlers);
		CreateCppImportOperatorHandlers(cppImportHandlers);
		for (auto handler : exportHandlers)
			compiler->RegisterExportOperator(L"glsl", handler);
		for (auto handler : importHandlers)
			compiler->RegisterImportOperator(L"glsl", handler);
		for (auto handler : cppImportHandlers)
			compiler->RegisterImportOperator(L"cpp", handler);
		try
		{
			if (compileResult.ErrorList.Count() == 0)
				compiler->Compile(compileResult, units, options);
			DestroyImportOperatorHanlders(importHandlers);
			DestroyExportOperatorHanlders(exportHandlers);
			DestroyImportOperatorHanlders(cppImportHandlers);
		}
		catch (...)
		{
			DestroyImportOperatorHanlders(importHandlers);
			DestroyExportOperatorHanlders(exportHandlers);
			DestroyImportOperatorHanlders(cppImportHandlers);
			throw;
		}
		if (compileResult.Success)
		{
			if (options.Mode == CompilerMode::ProduceShader)
			{
				EnumerableDictionary<String, ShaderLibFile> shaderLibs;
				for (auto file : compileResult.CompiledSource)
				{
					auto shaderName = Path::GetFileNameWithoutEXT(file.Key);
					ShaderLibFile * libFile = nullptr;
					if (!(libFile = shaderLibs.TryGetValue(shaderName)))
					{
						shaderLibs.Add(shaderName, ShaderLibFile());
						libFile = shaderLibs.TryGetValue(shaderName);
						libFile->MetaData.ShaderName = shaderName;
					}
					libFile->Sources = file.Value;
				}
				for (auto & libFile : shaderLibs)
				{
					for (auto & shader : compileResult.Program->Shaders)
					{
						if (shader->MetaData.ShaderName == libFile.Key)
						{
							// fill in meta data
							libFile.Value.MetaData = shader->MetaData;
						}
					}
					resultFiles.Add(libFile.Value);
				}
			}
		}
		return resultFiles;
	}

	List<ShaderLibFile> CompileShaderSource(Spire::Compiler::CompileResult & compileResult,
		const CoreLib::String & src, const CoreLib::Basic::String& sourceDir,
		Spire::Compiler::CompileOptions & options)
	{
		Spire::Compiler::NamingCounter = 0;
		RefPtr<ShaderCompiler> compiler = CreateShaderCompiler();
		List<CompileUnit> units;
		HashSet<String> processedUnits;
		List<String> unitsToInclude;
		unitsToInclude.Add(L"");
		processedUnits.Add(L"");
		auto predefUnit = compiler->Parse(compileResult, LibIncludeString, L"stdlib");
		for (int i = 0; i < unitsToInclude.Count(); i++)
		{
			auto inputFileName = unitsToInclude[i];
			try
			{
				String source = src;
				if (i > 0)
					source = File::ReadAllText(inputFileName);
				auto unit = compiler->Parse(compileResult, source, Path::GetFileName(inputFileName));
				units.Add(unit);
				if (unit.SyntaxNode)
				{
					for (auto inc : unit.SyntaxNode->Usings)
					{
						String includeFile = Path::Combine(Path::GetDirectoryName(inputFileName), inc.Content);
						if (processedUnits.Add(includeFile))
						{
							unitsToInclude.Add(includeFile);
						}
					}
				}
			}
			catch (IOException)
			{
				compileResult.GetErrorWriter()->Error(1, L"cannot open file '" + Path::GetFileName(inputFileName) + L"'.", CodePosition(0, 0, L""));
			}
		}
		units.Add(predefUnit);
		return CompileUnits(compileResult, compiler.Ptr(), units, options);
	}

	List<ShaderLibFile> CompileShaderSource(Spire::Compiler::CompileResult & compileResult, 
		CoreLib::Basic::String sourceFileName,
		Spire::Compiler::CompileOptions & options)
	{
		Spire::Compiler::NamingCounter = 0;
		RefPtr<ShaderCompiler> compiler = CreateShaderCompiler();
		List<CompileUnit> units;
		HashSet<String> processedUnits;
		List<String> unitsToInclude;
		unitsToInclude.Add(sourceFileName);
		processedUnits.Add(sourceFileName);
		auto predefUnit = compiler->Parse(compileResult, LibIncludeString, L"stdlib");
		for (int i = 0; i < unitsToInclude.Count(); i++)
		{
			auto inputFileName = unitsToInclude[i];
			try
			{
				String source = File::ReadAllText(inputFileName);
				auto unit = compiler->Parse(compileResult, source, Path::GetFileName(inputFileName));
				units.Add(unit);
				if (unit.SyntaxNode)
				{
					for (auto inc : unit.SyntaxNode->Usings)
					{
						String includeFile = Path::Combine(Path::GetDirectoryName(inputFileName), inc.Content);
						if (processedUnits.Add(includeFile))
						{
							unitsToInclude.Add(includeFile);
						}
					}
				}
			}
			catch (IOException)
			{
				compileResult.GetErrorWriter()->Error(1, L"cannot open file '" + Path::GetFileName(inputFileName) + L"'.", CodePosition(0, 0, sourceFileName));
			}
		}
		units.Add(predefUnit);
		return CompileUnits(compileResult, compiler.Ptr(), units, options);
	}
	void ShaderLibFile::AddSource(CoreLib::Basic::String source, CoreLib::Text::Parser & parser)
	{
		ReadSource(Sources, parser, source);
	}

	CoreLib::String ShaderLibFile::ToString()
	{
		StringBuilder writer;
		writer << L"name " << MetaData.ShaderName << EndLine;
		for (auto & world : MetaData.Worlds)
		{
			writer << L"world " << world.Key << EndLine << L"{" << EndLine;
			writer << L"target " << world.Value.TargetName << EndLine;
			for (auto & blk : world.Value.InputBlocks)
			{
				writer << L"in " << blk << L";\n";
			}
			writer << L"out " << world.Value.OutputBlock << L";\n";
			for (auto & comp : world.Value.Components)
				writer << L"comp " << comp << L";\n";
			writer << L"}" << EndLine;
		}
		for (auto & ublock : MetaData.InterfaceBlocks)
		{
			writer << L"interface " << ublock.Key << L" size " << ublock.Value.Size << L"\n{\n";
			for (auto & entry : ublock.Value.Entries)
			{
				writer << ILBaseTypeToString(entry.Type) << L" " << entry.Name << L" : " << entry.Offset << L"," << entry.Size;
				if (entry.Attributes.Count())
				{
					writer << L"\n{\n";
					for (auto & attrib : entry.Attributes)
					{
						writer << attrib.Key << L" : " << CoreLib::Text::Parser::EscapeStringLiteral(attrib.Value) << L";\n";
					}
					writer << L"}";
				}
				writer << L";\n";
			}
			writer << L"}\n";
		}
		writer << L"source" << EndLine << L"{" << EndLine;
		for (auto & src : Sources)
		{
			writer << src.Key << EndLine;
			writer << L"{" << EndLine;
			writer << src.Value.GetAllCodeGLSL() << EndLine;
			writer << L"}" << EndLine;
		}
		writer << L"}" << EndLine;
		StringBuilder formatSB;
		IndentString(formatSB, writer.ProduceString());
		return formatSB.ProduceString();
	}
	
	void ShaderLibFile::Clear()
	{
		Sources.Clear();
		MetaData.Worlds.Clear();
		Sources.Clear();
	}

	void ShaderLibFile::SaveToFile(CoreLib::Basic::String fileName)
	{
		StreamWriter fwriter(fileName);
		fwriter.Write(ToString());
	}

	void ShaderLibFile::FromString(const String & src)
	{
		Clear();
		CoreLib::Text::Parser parser(src);
		while (!parser.IsEnd())
		{
			auto fieldName = parser.ReadWord();
			if (fieldName == L"name")
			{
				MetaData.ShaderName = parser.ReadWord();
			}
			else if (fieldName == L"source")
			{
				parser.Read(L"{");
				ReadSource(Sources, parser, src);
				parser.Read(L"}");
			}
			else if (fieldName == L"binary")
			{
			}
			else if (fieldName == L"world")
			{
				WorldMetaData world;
				world.Name = parser.ReadWord();
				parser.Read(L"{");
				while (!parser.LookAhead(L"}"))
				{
					auto readAttribs = [&](InterfaceMetaData & comp)
					{
						parser.Read(L"{");
						while (!parser.LookAhead(L"}"))
						{
							auto name = parser.ReadWord();
							parser.Read(L":");
							auto value = parser.ReadStringLiteral();
							comp.Attributes[name] = parser.UnescapeStringLiteral(value);
						}
						parser.Read(L"}");
					};
					auto fieldName = parser.ReadWord();
					if (fieldName == L"target")
						world.TargetName = parser.ReadWord();
					else if (fieldName == L"in")
					{
						world.InputBlocks.Add(parser.ReadWord());
						parser.Read(L";");
					}
					else if (fieldName == L"out")
					{
						world.OutputBlock = parser.ReadWord();
						parser.Read(L";");
					}
					else if (fieldName == L"comp")
					{
						auto compName = parser.ReadWord();
						parser.Read(L";");
						world.Components.Add(compName);
					}
				}
				parser.Read(L"}");
				MetaData.Worlds[world.Name] = world;
			}
			else if (fieldName == L"interface")
			{
				InterfaceBlockMetaData block;
				if (!parser.LookAhead(L"{") && !parser.LookAhead(L"size"))
					block.Name = parser.ReadWord();
				if (parser.LookAhead(L"size"))
				{
					parser.ReadWord();
					block.Size = parser.ReadInt();
				}
				parser.Read(L"{");
				while (!parser.LookAhead(L"}") && !parser.IsEnd())
				{
					InterfaceBlockEntry entry;
					entry.Type = ILBaseTypeFromString(parser.ReadWord());
					entry.Name = parser.ReadWord();
					parser.Read(L":");
					entry.Offset = parser.ReadInt();
					parser.Read(L",");
					entry.Size = parser.ReadInt();
					if (parser.LookAhead(L"{"))
					{
						parser.Read(L"{");
						while (!parser.LookAhead(L"}") && !parser.IsEnd())
						{
							auto attribName = parser.ReadWord();
							parser.Read(L":");
							auto attribValue = parser.ReadStringLiteral();
							parser.Read(L";");
							entry.Attributes[attribName] = attribValue;
						}
						parser.Read(L"}");
					}
					parser.Read(L";");
					block.Entries.Add(entry);
				}
				parser.Read(L"}");
				MetaData.InterfaceBlocks[block.Name] = block;
			}
		}
	}

	void ShaderLibFile::Load(String fileName)
	{
		String src = File::ReadAllText(fileName);
		FromString(src);
	}
}
