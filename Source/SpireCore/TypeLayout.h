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