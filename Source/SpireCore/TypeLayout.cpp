// TypeLayout.cpp
#include "TypeLayout.h"

#include "Syntax.h"
#include "SymbolTable.h"

#include <assert.h>

namespace Spire {
namespace Compiler {

size_t RoundToAlignment(size_t offset, size_t alignment)
{
	size_t remainder = offset % alignment;
	if (remainder == 0)
		return offset;
	else
		return offset + (alignment - remainder);
}

static size_t RoundUpToPowerOfTwo( size_t value )
{
    // TODO(tfoley): I know this isn't a fast approach
    size_t result = 1;
    while (result < value)
        result *= 2;
    return result;
}

struct DefaultLayoutRulesImpl : LayoutRulesImpl
{
    // Get size and alignment for a single value of base type.
    LayoutInfo GetScalarLayout(BaseType baseType) override
    {
        switch (baseType)
        {
        case BaseType::Int:
        case BaseType::UInt:
        case BaseType::Float:
		case BaseType::Bool:
            return{ 4, 4 };

        default:
            assert(!"unimplemented");
            return{ 0, 1 };
        }
    }

    LayoutInfo GetScalarLayout(ILBaseType baseType) override
    {
        switch (baseType)
        {
        case ILBaseType::Int:
        case ILBaseType::UInt:
        case ILBaseType::Float:
            return{ 4, 4 };

        case ILBaseType::Int2:      return GetVectorLayout(GetScalarLayout(ILBaseType::Int),    2);
        case ILBaseType::UInt2:     return GetVectorLayout(GetScalarLayout(ILBaseType::UInt),   2);
        case ILBaseType::Float2:    return GetVectorLayout(GetScalarLayout(ILBaseType::Float),  2);
        case ILBaseType::Int3:      return GetVectorLayout(GetScalarLayout(ILBaseType::Int),    3);
        case ILBaseType::UInt3:     return GetVectorLayout(GetScalarLayout(ILBaseType::UInt),   3);
        case  ILBaseType::Float3:   return GetVectorLayout(GetScalarLayout(ILBaseType::Float),  3);
        case ILBaseType::Int4:      return GetVectorLayout(GetScalarLayout(ILBaseType::Int),    4);
        case ILBaseType::UInt4:     return GetVectorLayout(GetScalarLayout(ILBaseType::UInt),   4);
        case ILBaseType::Float4:    return GetVectorLayout(GetScalarLayout(ILBaseType::Float),  4);

        case ILBaseType::Float3x3:  return GetMatrixLayout(GetScalarLayout(ILBaseType::Float),  3, 3);
        case ILBaseType::Float4x4:  return GetMatrixLayout(GetScalarLayout(ILBaseType::Float),  4, 4);

        case ILBaseType::Texture2D:
        case ILBaseType::Texture2DShadow:
        case ILBaseType::TextureCube:
        case ILBaseType::TextureCubeShadow:
            return{ 8, 8 };

        default:
            assert(!"unimplemented");
            return{ 0, 1 };
        }
    }

    LayoutInfo GetArrayLayout(LayoutInfo elementInfo, size_t elementCount) override
    {
        LayoutInfo arrayInfo;
        arrayInfo.size = elementInfo.size * elementCount;
        arrayInfo.alignment = elementInfo.alignment;
        return arrayInfo;
    }

    LayoutInfo GetVectorLayout(LayoutInfo elementInfo, size_t elementCount) override
    {
        LayoutInfo vectorInfo;
        vectorInfo.size = elementInfo.size * elementCount;
        vectorInfo.alignment = RoundUpToPowerOfTwo(elementInfo.size * elementInfo.alignment);
        return vectorInfo;
    }

    LayoutInfo GetMatrixLayout(LayoutInfo elementInfo, size_t rowCount, size_t columnCount) override
    {
        return GetArrayLayout(
            GetVectorLayout(elementInfo, columnCount),
            rowCount);
    }

    LayoutInfo BeginStructLayout() override
    {
        LayoutInfo structInfo;
        structInfo.size = 0;
        structInfo.alignment = 1;
        return structInfo;
    }

    size_t AddStructField(LayoutInfo* ioStructInfo, LayoutInfo fieldInfo) override
    {
        ioStructInfo->alignment = std::max(ioStructInfo->alignment, fieldInfo.alignment);
        ioStructInfo->size = RoundToAlignment(ioStructInfo->size, fieldInfo.alignment);
        size_t fieldOffset = ioStructInfo->size;
        ioStructInfo->size += fieldInfo.size;
        return fieldOffset;
    }


    void EndStructLayout(LayoutInfo* ioStructInfo) override
    {
        ioStructInfo->size = RoundToAlignment(ioStructInfo->size, ioStructInfo->alignment);
    }
};

struct Std140LayoutRulesImpl : DefaultLayoutRulesImpl
{
    // The `std140` rules require that all array elements
    // be a multiple of 16 bytes.
    LayoutInfo GetArrayLayout(LayoutInfo elementInfo, size_t elementCount) override
    {
        if (elementInfo.alignment < 16)
            elementInfo.alignment = 16;
        elementInfo.size = RoundToAlignment(elementInfo.size, elementInfo.alignment);

        return DefaultLayoutRulesImpl::GetArrayLayout(elementInfo, elementCount);
    }

    // The `std140` rules require that a `struct` type be
    // alinged to at least 16.
    LayoutInfo BeginStructLayout() override
    {
        LayoutInfo structInfo;
        structInfo.size = 0;
        structInfo.alignment = 16;
        return structInfo;
    }
};

struct Std430LayoutRulesImpl : DefaultLayoutRulesImpl
{
};

struct PackedLayoutRulesImpl : DefaultLayoutRulesImpl
{
};

Std140LayoutRulesImpl kStd140LayoutRulesImpl;
Std430LayoutRulesImpl kStd430LayoutRulesImpl;
PackedLayoutRulesImpl kPackedLayoutRulesImpl;

LayoutRulesImpl* GetLayoutRulesImpl(LayoutRule rule)
{
    switch (rule)
    {
    case LayoutRule::Std140: return &kStd140LayoutRulesImpl;
    case LayoutRule::Std430: return &kStd430LayoutRulesImpl;
    case LayoutRule::Packed: return &kPackedLayoutRulesImpl;
    default:
        return nullptr;
    }
}

static int GetElementCount(RefPtr<IntVal> val)
{
	if (auto constantVal = val.As<ConstantIntVal>())
	{
		return constantVal->value;
	}
	assert(!"unexpected");
	return 0;
}

LayoutInfo GetLayout(ExpressionType* inType, LayoutRulesImpl* rules)
{
	RefPtr<ExpressionType> type = inType;
    if (auto basicType = type->As<BasicExpressionType>())
    {
        return rules->GetScalarLayout(basicType->BaseType);
    }
    else if (auto arrayType = type->As<ArrayExpressionType>())
    {
        return rules->GetArrayLayout(
            GetLayout(arrayType->BaseType.Ptr(), rules),
            GetElementCount(arrayType->ArrayLength));
    }
	else if (auto declRefType = type->As<DeclRefType>())
	{
		auto declRef = declRefType->declRef;
		if (auto structDeclRef = declRef.As<StructDeclRef>())
		{
            LayoutInfo info = rules->BeginStructLayout();

            for (auto field : structDeclRef.GetFields())
            {
                rules->AddStructField(&info,
                    GetLayout(field.GetType().Ptr(), rules));
            }

            rules->EndStructLayout(&info);
            return info;
		}
	}

	// catch-all case in case nothing matched
	assert(!"unimplemented");
	return{ 0, 1 };
}

LayoutInfo GetLayout(ILType* type, LayoutRulesImpl* rules)
{
    if (auto basicType = dynamic_cast<ILBasicType*>(type))
    {
        return rules->GetScalarLayout(basicType->Type);
    }
    else if (auto arrayType = dynamic_cast<ILArrayType*>(type))
    {
        return rules->GetArrayLayout(
            GetLayout(arrayType->BaseType.Ptr(), rules),
            arrayType->ArrayLength);
    }
    else if (auto structType = dynamic_cast<ILStructType*>(type))
    {
        LayoutInfo info = rules->BeginStructLayout();

        for (auto field : structType->Members)
        {
            rules->AddStructField(&info,
                GetLayout(field.Type.Ptr(), rules));
        }

        rules->EndStructLayout(&info);
        return info;
    }
    else if (auto recordType = dynamic_cast<ILRecordType*>(type))
    {
        // TODO: this need to be implemented
        LayoutInfo info = { 0, 1 };
        return info;
    }
    else if (auto genericType = dynamic_cast<ILGenericType*>(type))
    {
        return GetLayout(genericType->BaseType.Ptr(), rules);
    }
    else
    {
        assert(!"unimplemented");
        return{ 0, 1 };
    }
}

LayoutInfo GetLayout(ExpressionType* type, LayoutRule rule)
{
    LayoutRulesImpl* rulesImpl = GetLayoutRulesImpl(rule);
    return GetLayout(type, rulesImpl);
}

LayoutInfo GetLayout(ILType* type, LayoutRule rule)
{
    LayoutRulesImpl* rulesImpl = GetLayoutRulesImpl(rule);
    return GetLayout(type, rulesImpl);
}

}}
