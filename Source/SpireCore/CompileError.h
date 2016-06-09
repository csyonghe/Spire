#ifndef RASTER_RENDERER_COMPILE_ERROR_H
#define RASTER_RENDERER_COMPILE_ERROR_H

#include "../CoreLib/Basic.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;

		class CodePosition
		{
		public:
			int Line = -1, Col = -1;
			String FileName;
			String ToString()
			{
				StringBuilder sb(100);
				sb << FileName;
				if (Line != -1)
					sb << L"(" << Line << L")";
				return sb.ProduceString();
			}
			CodePosition() = default;
			CodePosition(int line, int col, String fileName)
			{
				Line = line;
				Col = col;
				this->FileName = fileName;
			}
			bool operator < (const CodePosition & pos) const
			{
				return FileName < pos.FileName || (FileName == pos.FileName && Line < pos.Line) ||
					(FileName == pos.FileName && Line == pos.Line && Col < pos.Col);
			}
			bool operator == (const CodePosition & pos) const
			{
				return FileName == pos.FileName && Line == pos.Line && Col == pos.Col;
			}
		};

		class CompileError
		{
		public:
			String Message;
			CodePosition Position;
			int ErrorID;

			CompileError()
			{
				ErrorID = -1;
			}
			CompileError(const String & msg, int id,
						const CodePosition & pos)
			{
				Message = msg;
				ErrorID = id;
				Position = pos;
			}
		};

		class ErrorWriter
		{
		private:
			List<CompileError> & errors;
			List<CompileError> & warnings;
		public:
			ErrorWriter(List<CompileError> & perrors, List<CompileError> & pwarnings)
				: errors(perrors), warnings(pwarnings)
			{}
			void Error(int id, const String & msg, const CodePosition & pos)
			{
				errors.Add(CompileError(msg, id, pos));
			}
			void Warning(int id, const String & msg, const CodePosition & pos)
			{
				warnings.Add(CompileError(msg, id, pos));
			}
			int GetErrorCount()
			{
				return errors.Count();
			}
		};
	}
}

#endif