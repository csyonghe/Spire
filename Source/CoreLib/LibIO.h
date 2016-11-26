#ifndef CORE_LIB_IO_H
#define CORE_LIB_IO_H

#include "LibString.h"
#include "Stream.h"
#include "TextIO.h"
#include "SecureCRT.h"

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
#ifdef _WIN32
			static const char PathDelimiter = '\\';
#else
			static const char PathDelimiter = '/';
#endif
			static String TruncateExt(const String & path);
			static String ReplaceExt(const String & path, const char * newExt);
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