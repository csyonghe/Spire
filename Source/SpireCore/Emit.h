// Emit.h
#ifndef SPIRE_EMIT_H_INCLUDED
#define SPIRE_EMIT_H_INCLUDED

#include "../CoreLib/Basic.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;

		class ProgramSyntaxNode;

		String EmitProgram(ProgramSyntaxNode* program);
	}
}
#endif
