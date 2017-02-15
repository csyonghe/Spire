#ifndef SPIRE_TYPE_LAYOUT_H
#define SPIRE_TYPE_LAYOUT_H

#include "../CoreLib/Basic.h"
#include "IL.h"
#include "Syntax.h"

#include "../../Spire.h"

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

    // TODO(tfoley): HLSL constant buffer rules don't actually
    // match `std140`, but we won't deal with the differences right now
    HLSLConstantBuffer = Std140,
};

// Layout appropriate to "just memory" scenarios,
// such as laying out the members of a constant buffer.
struct LayoutInfo
{
    size_t size;
    size_t alignment;
};

struct ArrayLayoutInfo : LayoutInfo
{
    size_t elementStride;
};

enum class LayoutResourceKind
{
    Uniform = -2,
    Invalid = -1,
    //
    ConstantBuffer,     // HLSL `b` register
    ShaderResource,     // HLSL `t` register
    UnorderedAccess,    // HLSL 'u` register
    SamplerState,       // HLSL `s` register
    //
    Count,
};
bool IsResourceKind(LayoutResourceKind kind);

// Layout information for an object/resource type
struct ObjectLayoutInfo
{
    // What kind of resource should we consume?
    LayoutResourceKind kind;

    // How many resources of that kind?
    size_t size;

    // only useful in the uniform case
    size_t alignment;

    ObjectLayoutInfo()
        : kind(LayoutResourceKind::Invalid)
        , size(0)
        , alignment(1)
    {}

    ObjectLayoutInfo(LayoutResourceKind kind)
        : kind(kind)
        , size(1)
        , alignment(1)
    {}

    ObjectLayoutInfo(LayoutInfo const& uniform)
        : kind(LayoutResourceKind::Uniform)
        , size(uniform.size)
        , alignment(uniform.alignment)
    {}

    operator LayoutInfo() const
    {
        LayoutInfo info = { 0, 1 };
        if (kind == LayoutResourceKind::Uniform)
        {
            info.size = size;
            info.alignment = alignment;
        }
        return info;
    }
};

struct LayoutRulesImpl;

// A reified reprsentation of a particular laid-out type
class TypeLayout : public RefObject
{
public:
    // The type that was laid out
    RefPtr<ExpressionType>  type;

    // The layout rules that were used to produce this type
    LayoutRulesImpl*        rules;

    // layout information for any uniforms in this type
    LayoutInfo              uniforms;

    // layout information for any reosurces in this type
    // (where "resources" means everything that isn't just
    // allocated as data in a uniform buffer)
    struct ResourceInfo
    {
        // What kind of register was it?
        LayoutResourceKind  kind;

        // How many registers of the above kind did we use?
        int                 count;

        // Further infor for other register kinds.
        // This field will only be used in the case
        // where a single type/parameter contains
        // data that belongs to multiple categories.
        RefPtr<ResourceInfo> next;
    };
    ResourceInfo            resources;

    ResourceInfo* FindResourceInfo(LayoutResourceKind kind)
    {
        auto rr = &resources;
        while (rr)
        {
            if (rr->kind == kind)
                return rr;
            rr = rr->next.Ptr();
        }
        return nullptr;
    }
};

// A reified layout for a particular variable, field, etc.
class VarLayout : public RefObject
{
public:
    // The variable we are laying out
    VarDeclBaseRef          varDecl;

    // The result of laying out the variable's type
    RefPtr<TypeLayout>      typeLayout;

    // The offset of any uniforms, inside the parent
    size_t                  uniformOffset;

    // The start register(s) for any resources
    struct ResourceInfo
    {
        // What kind of register was it?
        LayoutResourceKind  kind;

        // What binding space (HLSL) or set (Vulkan) are we placed in?
        int                 space;

        // What is our starting register in that space?
        int                 index;

        // Further infor for other register kinds.
        // This field will only be used in the case
        // where a single type/parameter contains
        // data that belongs to multiple categories.
        RefPtr<ResourceInfo>  next;
    };
    ResourceInfo            resources;

    ResourceInfo* FindResourceInfo(LayoutResourceKind kind)
    {
        auto rr = &resources;
        while (rr)
        {
            if (rr->kind == kind)
                return rr;
            rr = rr->next.Ptr();
        }
        return nullptr;
    }

    ResourceInfo* AddResourceInfo(LayoutResourceKind kind)
    {
        if (!IsResourceKind(resources.kind))
            return &resources;

        auto link = &resources.next;
        while (*link)
            link = &(*link)->next;

        auto result = new ResourceInfo();
        result->kind = kind;
        *link = result;
        return result;
    }
};

// Type layout for a variable that has a constant-buffer type
class ConstantBufferTypeLayout : public TypeLayout
{
public:
    RefPtr<TypeLayout> elementTypeLayout;
};

// Specific case of type layout for an array
class ArrayTypeLayout : public TypeLayout
{
public:
    // The layout used for the element type
    RefPtr<TypeLayout>  elementTypeLayout;

    // the stride between elements when used in
    // a uniform buffer
    size_t              uniformStride;
};

// Specific case of type layout for a struct
class StructTypeLayout : public TypeLayout
{
public:
    List<RefPtr<VarLayout>> fields;
};

// Layout information for the global scope of a program
class ProgramLayout : public StructTypeLayout
{
};

// Layout information for a particular shader entry point
class EntryPointLayout : public StructTypeLayout
{
};

// A modifier to be attached to syntax after we've computed layout
class ComputedLayoutModifier : public Modifier
{
public:
    RefPtr<TypeLayout> typeLayout;
};



struct LayoutRulesImpl
{
    // Get size and alignment for a single value of base type.
    virtual LayoutInfo GetScalarLayout(BaseType baseType) = 0;
    virtual LayoutInfo GetScalarLayout(ILBaseType baseType) = 0;
    virtual LayoutInfo GetScalarLayout(spire::TypeReflection::ScalarType scalarType) = 0;

    // Compute layout info for an object type
    virtual ObjectLayoutInfo GetObjectLayout(spire::TypeReflection::Kind kind) = 0;

    // Get size and alignment for an array of elements
    virtual ArrayLayoutInfo GetArrayLayout(LayoutInfo elementInfo, size_t elementCount) = 0;

#if 0
    ExtendedArrayLayoutInfo GetArrayLayoutExt(ExtendedLayoutInfo elementInfo, size_t elementCount)
    {
        auto uniformInfo = GetArrayLayout(
            elementInfo.uniforms,
            elementCount);

        ExtendedArrayLayoutInfo arrayInfo;
        arrayInfo.uniforms = uniformInfo;
        arrayInfo.uniformElementStride = uniformInfo.elementStride;

        for (int ii = 0; ii < (int)LayoutResourceKind::Count; ++ii)
        {
            arrayInfo.resourceCounts[ii] = elementCount*elementInfo.resourceCounts[ii];
        }
        return arrayInfo;
    }
#endif

    // Get layout for a vector or matrix type
    virtual LayoutInfo GetVectorLayout(LayoutInfo elementInfo, size_t elementCount) = 0;
    virtual LayoutInfo GetMatrixLayout(LayoutInfo elementInfo, size_t rowCount, size_t columnCount) = 0;

    // Begin doing layout on a `struct` type
    virtual LayoutInfo BeginStructLayout() = 0;

#if 0
    ExtendedLayoutInfo BeginStructLayoutExt()
    {
        return ExtendedLayoutInfo(BeginStructLayout());
    }
#endif

    // Add a field to a `struct` type, and return the offset for the field
    virtual size_t AddStructField(LayoutInfo* ioStructInfo, LayoutInfo fieldInfo) = 0;

#if 0
    ExtendedOffsetInfo AddStructFieldExt(ExtendedLayoutInfo* ioStructInfo, ExtendedLayoutInfo fieldInfo)
    {
        ExtendedOffsetInfo extOffset;

        // Skip fields with no uniform data, so that they don't
        // screw up layout computation for other fields
        if (fieldInfo.uniforms.size != 0)
        {
            extOffset.uniformOffset = AddStructField(
                &ioStructInfo->uniforms,
                fieldInfo.uniforms);
        }

        for (int ii = 0; ii < (int)LayoutResourceKind::Count; ++ii)
        {
            extOffset.resourceIndices[ii] = ioStructInfo->resourceCounts[ii];
            ioStructInfo->resourceCounts[ii] += fieldInfo.resourceCounts[ii];
        }

        return extOffset;
    }
#endif

    // End layout for a struct, and finalize its size/alignment.
    virtual void EndStructLayout(LayoutInfo* ioStructInfo) = 0;

#if 0
    void EndStructLayoutExt(ExtendedLayoutInfo* ioStructInfo)
    {
        EndStructLayout(&ioStructInfo->uniforms);
    }
#endif
};

LayoutRulesImpl* GetLayoutRulesImpl(LayoutRule rule);

LayoutInfo GetLayout(ExpressionType* type, LayoutRulesImpl* rules);
LayoutInfo GetLayout(ILType* type, LayoutRulesImpl* rules);

LayoutInfo GetLayout(ExpressionType* type, LayoutRule rule = LayoutRule::Std430);
LayoutInfo GetLayout(ILType* type, LayoutRule rule = LayoutRule::Std430);

RefPtr<TypeLayout> CreateTypeLayout(ExpressionType* type, LayoutRulesImpl* rules);

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