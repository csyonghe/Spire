#include "LibIO.h"
#include "Exception.h"
#ifndef __STDC__
#define __STDC__ 1
#endif
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#endif
namespace CoreLib
{
	namespace IO
	{
		using namespace CoreLib::Basic;

		CommandLineWriter * currentCommandWriter = nullptr;

		void SetCommandLineWriter(CommandLineWriter * writer)
		{
			currentCommandWriter = writer;
		}

		bool File::Exists(const String & fileName)
		{
			struct _stat32 statVar;
			return ::_stat32(((String)fileName).ToMultiByteString(), &statVar) != -1;
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