#ifndef SPIRE_TYPE_TRANSLATION_H
#define SPIRE_TYPE_TRANSLATION_H

#include "Syntax.h"
#include "IL.h"

namespace Spire
{
	namespace Compiler
	{
		RefPtr<ILType> TranslateExpressionType(ExpressionType * type, Dictionary<String, RefPtr<ILType>> * genericTypeMappings = nullptr);
		RefPtr<ILType> TranslateExpressionType(const RefPtr<ExpressionType> & type, Dictionary<String, RefPtr<ILType>> * genericTypeMappings = nullptr);
	}
}

#endif