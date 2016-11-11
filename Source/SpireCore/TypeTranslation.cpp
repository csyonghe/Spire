#include "TypeTranslation.h"
#include "SymbolTable.h"

namespace Spire
{
	namespace Compiler
	{
		RefPtr<ILType> TranslateExpressionType(ExpressionType * type, Dictionary<String, RefPtr<ILRecordType>> * recordTypes)
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
					if (recordTypes)
						return (*recordTypes)[basicType->RecordTypeName]();
					else
						throw InvalidProgramException(L"unexpected record type.");
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
				nArrType->BaseType = TranslateExpressionType(arrType->BaseType.Ptr(), recordTypes);
				nArrType->ArrayLength = arrType->ArrayLength;
				resultType = nArrType;
			}
			else if (auto genType = type->AsGenericType())
			{
				auto gType = new ILGenericType();
				gType->GenericTypeName = genType->GenericTypeName;
				gType->BaseType = TranslateExpressionType(genType->BaseType.Ptr(), recordTypes);
				resultType = gType;
			}
			return resultType;
		}

		RefPtr<ILType> TranslateExpressionType(const RefPtr<ExpressionType> & type, Dictionary<String, RefPtr<ILRecordType>> * recordTypes)
		{
			return TranslateExpressionType(type.Ptr(), recordTypes);
		}
	}
}