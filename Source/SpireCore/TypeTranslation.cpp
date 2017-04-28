#include "TypeTranslation.h"
#include "SymbolTable.h"

namespace Spire
{
	namespace Compiler
	{
		RefPtr<ILType> TranslateExpressionType(ExpressionType * type, Dictionary<String, RefPtr<ILType>> * genericTypeMappings)
		{
			RefPtr<ILType> resultType = 0;
			if (auto basicType = type->AsBasicType())
			{
				if (basicType->BaseType == BaseType::Struct)
				{
					resultType = basicType->Struct->Type;
				}
				else if (basicType->BaseType == BaseType::Record)
				{
					if (genericTypeMappings)
						return (*genericTypeMappings)[basicType->RecordTypeName]();
					else
						throw InvalidProgramException("unexpected record type.");
				}
				else if (basicType->BaseType == BaseType::Generic)
				{
					if (genericTypeMappings)
						return (*genericTypeMappings)[basicType->GenericTypeVar]();
					else
						throw InvalidProgramException("unexpected generic type.");
				}
				else
				{
					auto base = new ILBasicType();
					base->Type = (ILBaseType)basicType->BaseType;
					resultType = base;
				}
			}
			else if (auto arrType = type->AsArrayType())
			{
				auto nArrType = new ILArrayType();
				nArrType->BaseType = TranslateExpressionType(arrType->BaseType.Ptr(), genericTypeMappings);
				nArrType->ArrayLength = arrType->ArrayLength;
				resultType = nArrType;
			}
			else if (auto genType = type->AsGenericType())
			{
				auto gType = new ILGenericType();
				gType->GenericTypeName = genType->GenericTypeName;
				gType->BaseType = TranslateExpressionType(genType->BaseType.Ptr(), genericTypeMappings);
				resultType = gType;
			}
			return resultType;
		}

		RefPtr<ILType> TranslateExpressionType(const RefPtr<ExpressionType> & type, Dictionary<String, RefPtr<ILType>> * genericTypeMappings)
		{
			return TranslateExpressionType(type.Ptr(), genericTypeMappings);
		}
	}
}