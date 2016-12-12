#ifndef SPIRE_TYPE_LAYOUT_H
#define SPIRE_TYPE_LAYOUT_H

#include "../CoreLib/Basic.h"
#include "IL.h"

namespace Spire {
namespace Compiler {

// Forward declarations

enum class BaseType;
class ExpressionType;

//

enum class LayoutRule
{
    Std140,
    Std430,
    Packed,
};


struct LayoutInfo
{
    size_t size;
    size_t alignment;
};

struct LayoutRulesImpl
{
    // Get size and alignment for a single value of base type.
    virtual LayoutInfo GetScalarLayout(BaseType baseType) = 0;
    virtual LayoutInfo GetScalarLayout(ILBaseType baseType) = 0;

    // Get size and alignment for an array of elements
    virtual LayoutInfo GetArrayLayout(LayoutInfo elementInfo, size_t elementCount) = 0;

    // Get layout for a vector or matrix type
    virtual LayoutInfo GetVectorLayout(LayoutInfo elementInfo, size_t elementCount) = 0;
    virtual LayoutInfo GetMatrixLayout(LayoutInfo elementInfo, size_t rowCount, size_t columnCount) = 0;

    // Begin doing layout on a `struct` type
    virtual LayoutInfo BeginStructLayout() = 0;

    // Add a field to a `struct` type, and return the offset for the field
    virtual size_t AddStructField(LayoutInfo* ioStructInfo, LayoutInfo fieldInfo) = 0;

    // End layout for a struct, and finalize its size/alignment.
    virtual void EndStructLayout(LayoutInfo* ioStructInfo) = 0;
};

LayoutRulesImpl* GetLayoutRulesImpl(LayoutRule rule);

LayoutInfo GetLayout(ExpressionType* type, LayoutRulesImpl* rules);
LayoutInfo GetLayout(ILType* type, LayoutRulesImpl* rules);

LayoutInfo GetLayout(ExpressionType* type, LayoutRule rule = LayoutRule::Std430);
LayoutInfo GetLayout(ILType* type, LayoutRule rule = LayoutRule::Std430);

inline size_t GetTypeSize(ExpressionType* type, LayoutRule rule = LayoutRule::Std430)
{
    return GetLayout(type, rule).size;
}

inline size_t GetTypeSize(ILType* type, LayoutRule rule = LayoutRule::Std430)
{
    return GetLayout(type, rule).size;
}

inline size_t GetTypeAlignment(ExpressionType* type, LayoutRule rule = LayoutRule::Std430)
{
    return GetLayout(type, rule).alignment;
}

inline size_t GetTypeAlignment(ILType* type, LayoutRule rule = LayoutRule::Std430)
{
    return GetLayout(type, rule).alignment;
}

}}

#endif