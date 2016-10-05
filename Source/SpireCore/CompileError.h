#ifndef RASTER_RENDERER_COMPILE_ERROR_H
#define RASTER_RENDERER_COMPILE_ERROR_H

#include "../CoreLib/Basic.h"
#include "CodePosition.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;

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
			ErrorWriter & operator = (const ErrorWriter & other) = delete;
		};
	}
}

#endif