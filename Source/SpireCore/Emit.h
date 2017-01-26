// Emit.h
#ifndef SPIRE_EMIT_H_INCLUDED
#define SPIRE_EMIT_H_INCLUDED

namespace Spire
{
	namespace Compiler
	{
		class ProgramSyntaxNode;

		void EmitProgram(ProgramSyntaxNode* program);
	}
}
#endif
