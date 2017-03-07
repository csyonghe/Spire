#ifndef SPIRE_PARAMETER_BINDING_H
#define SPIRE_PARAMETER_BINDING_H

#include "../CoreLib/Basic.h"
#include "Syntax.h"

#include "../../Spire.h"

namespace Spire {
namespace Compiler {

// The parameter-binding interface is responsible for assigning
// binding locations/registers to every parameter of a shader
// program. This can include both parameters declared on a
// particular entry point, as well as parameters declared at
// global scope.
//


// Generate binding information for the given program,
// and attach that information to the syntax nodes
// of the program.

void GenerateParameterBindings(
    ProgramSyntaxNode* program);

}}

#endif // SPIRE_REFLECTION_H
