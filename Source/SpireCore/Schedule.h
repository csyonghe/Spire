#ifndef BAKER_SL_SCHEDULE_H
#define BAKER_SL_SCHEDULE_H

#include "../CoreLib/Basic.h"
#include "CompileError.h"
#include "Syntax.h"

namespace Spire
{
	namespace Compiler
	{
		class Schedule
		{
		public:
			CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::List<RefPtr<ChoiceValueSyntaxNode>>> Choices;
			CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::String>> AddtionalAttributes;
			static Schedule Parse(CoreLib::String source, CoreLib::String fileName, CoreLib::List<CompileError> & errorList);
		};
	}
}

#endif