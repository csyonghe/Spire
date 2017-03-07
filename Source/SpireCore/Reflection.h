#ifndef SPIRE_REFLECTION_H
#define SPIRE_REFLECTION_H

#include "../CoreLib/Basic.h"
#include "Syntax.h"

#include "../../Spire.h"

namespace Spire {
namespace Compiler {

// The role of the reflection interface is to extract data from one or more program elements that
// can later be used to assit in binding shader parameters, filling in buffers, etc.
//
// The key goals for the design here are:
//
// - Reflection data can be represented as a single binary "blob" with no pointers, so that it can be saved/restored easily.
//   - As a corralary: reflection data is independent of the AST and other compiler data structures
// - Loading reflection data and performing queries should not require any additional heap allocation
//
// The approach taken here is that reflection data is encoded as "node" structures with fixed layouts.
// Every kind of node stores an initial "tag" field that identifies the flavor of node.
// Cross-references between nodes are stored as relative offsets, so that we can navigate from one node to another,
// without needing to know the base address of the entire reflection "blob."
//
// Many routines in this interface are "defensively coded" to try to handle the case where
// they get invoked on NULL pointers. This supports the fluent calls that are used in `SpireLib.cpp` to
// implement the public reflection API. E.g., `node->AsType()->AsStruct()->GetFieldCount()` is an expression
// we would like to have work even if the original `node` is NULL, or not a structure type.

// The different flavors of reflection nodes we support
enum class ReflectionNodeFlavor : uint8_t
{
    None,
    Blob,
    Type,
    TypeLayout,
    MultiTypeLayout,
    Parameter,
    MultiParameter,
    Variable,
    VariableLayout,
};

// Encoding for a relative pointer, to be used inside of nodes.
template<typename T>
struct ReflectionPtr
{
    // byte offset from `this` to the pointed-at data, or zero for a NULL pointer.
    int32_t raw;

    // Extract value as an ordinary pointer.
    T* Ptr() const
    {
        // If the raw value is zero, the pointer is null
        if(!raw) return nullptr;

        // Otherwise, add the approrpiate byte offset to `this`
        return (T*) (((char*) this) + (int32_t)raw);
    }

    // Allow implicit conversion to the appropriate pointer type
    operator T*() const
    {
        return Ptr();
    }
    T* operator->() const
    {
        return Ptr();
    }

    // Allow assignment from a raw pointer
    void operator=(T* ptr)
    {
        if( ptr )
        {
            // Encode a non-null pointer as the byte difference.
            ptrdiff_t diff = (char*)ptr - (char*)this;

            // Make sure the difference fits in 32 bits
            assert(diff == (int32_t)diff);
            raw = (int32_t) diff;
        }
        else
        {
            // A null pointer is just a zero value.
            raw = 0;
        }
    }
};

// In places where we really ought to use `size_t` we'll use
// ReflectionSize, knowing that the 99% case for shaders will
// not hit the limits for a 32-bit size/offset.
typedef uint32_t ReflectionSize;

// Forward-declare a few cases.
struct ReflectionArrayTypeLayoutNode;
struct ReflectionArrayTypeNode;
struct ReflectionParameterNode;
struct ReflectionStructTypeLayoutNode;
struct ReflectionStructTypeNode;
struct ReflectionTypeNode;
struct ReflectionTypeLayoutNode;
struct ReflectionVariableNode;
struct ReflectionVariableLayoutNode;



struct ReflectionNode
{
    // The first 32 bits of every node is laid out the same way,
    // but more specific flavors of nodes will use these bits
    // differently.
    union
    {
        struct
        {
            // We always use the first 8 bits to identify the flavor
            // of the node.
            ReflectionNodeFlavor flavor;

            // 8 bits of flavor-specific data
            uint8_t extraA;

            // another 16 bits of flavor-specific data
            uint16_t extraB;

        };

        // Used for type nodes
        struct
        {
            ReflectionNodeFlavor    flavor;
            uint8_t                 kind;

            // Stuff used by specific kinds of types
            union
            {
                struct
                {
                    uint16_t        fieldCount;
                } structType;
                struct
                {
                    uint8_t         scalarType;
                } scalar;
                struct
                {
                    uint8_t         elementCount;
                } vector;
                struct
                {
                    uint8_t         rowCount;
                    uint8_t         columnCount;
                } matrix;
                struct
                {
                    uint16_t        shape;
                } resource;
            };
        } type;

        // We put a 32-bit value in this `union` to force 4-byte alignment on the structure
        uint32_t headerWord;
    };

    ReflectionNodeFlavor GetFlavor() const { return flavor; }


    // We add a few convenience functions for down-casting a node
    // to more specialized flavors, as needed.

    ReflectionTypeNode* AsType();

    ReflectionTypeLayoutNode* AsTypeLayout()
    {
        if(!this) return nullptr;
        switch( GetFlavor() )
        {
        case ReflectionNodeFlavor::TypeLayout:
            return (ReflectionTypeLayoutNode*) this;

        default:
            return nullptr;
        }
    }

    ReflectionVariableNode* AsVariable();

    ReflectionVariableLayoutNode* AsVariableLayout()
    {
        if(!this) return nullptr;
        switch( GetFlavor() )
        {
        case ReflectionNodeFlavor::VariableLayout:
            return (ReflectionVariableLayoutNode*) this;

        default:
            return nullptr;
        }
    }

    ReflectionParameterNode* AsParameter()
    {
        if(!this) return nullptr;
        switch( GetFlavor() )
        {
        case ReflectionNodeFlavor::Parameter:
            return (ReflectionParameterNode*) this;

        default:
            return nullptr;
        }
    }
};

// A type node represents a (canonical for now) type in the user's program,
// which can be one of several different *kinds* of types (arrays, scalars, structs, etc.)
struct ReflectionTypeNode : ReflectionNode
{
    // We store the kind of  type in one of the "extra" fields
    SpireTypeKind GetKind() const
    {
        if(!this) return SPIRE_TYPE_KIND_NONE;
        return (SpireTypeKind) type.kind;
    }

    void SetKind(SpireTypeKind kind)
    {
        type.kind = kind;
    }

    // We add some utility routines for down-casting to commonly-used node types
    ReflectionStructTypeNode* AsStruct() const
    {
        if(!this) return nullptr;
        switch( GetKind() )
        {
        case SPIRE_TYPE_KIND_STRUCT:
            return (ReflectionStructTypeNode*) this;

        default:
            return nullptr;
        }
    }

    ReflectionArrayTypeNode* AsArray() const
    {
        if(!this) return nullptr;
        switch( GetKind() )
        {
        case SPIRE_TYPE_KIND_ARRAY:
            return (ReflectionArrayTypeNode*) this;

        default:
            return nullptr;
        }
    }

    // This routine removes any outer array types until we end up
    // with a non-array type. E.g., given `int[][]` it returns `int`
    ReflectionTypeNode* UnwrapArrays();
};

// A type layout node extends a type node with information about how the type was laid
// out in memory, using particular layout rules (e.g., `std140`). For all types this
// includes the size of the type as laid out. More specialized node types can be used
// for specific kinds of types (e.g., a struct type layout will also record field offsets)
struct ReflectionTypeLayoutNode : ReflectionNode
{
    ReflectionPtr<ReflectionTypeNode> type;
    ReflectionSize size;

    SpireTypeKind GetKind() const { return type->GetKind(); }
    size_t GetSize() const { return size; }
    ReflectionTypeNode* GetType() const { return type; }

    ReflectionStructTypeLayoutNode* AsStruct() const
    {
        if(!this) return nullptr;
        switch( GetKind() )
        {
        case SPIRE_TYPE_KIND_STRUCT:
            return (ReflectionStructTypeLayoutNode*) this;

        default:
            return nullptr;
        }
    }

    ReflectionArrayTypeLayoutNode* AsArray() const
    {
        if(!this) return nullptr;
        switch( GetKind() )
        {
        case SPIRE_TYPE_KIND_ARRAY:
            return (ReflectionArrayTypeLayoutNode*) this;

        default:
            return nullptr;
        }
    }
};

// A variable node represents a shader parameter, struct field, value in a constant buffer, etc.
struct ReflectionVariableNode : ReflectionNode
{
    // The name of the variable
    ReflectionPtr<char> name;

    // The type
    ReflectionPtr<ReflectionTypeNode> type;

    char const* GetName() const { return name; }
    ReflectionTypeNode* GetType() const { return type; }
};

// A variable *layout* represents a variable/field/etc. that has been laid out
// according to a specific set of layout rules.
struct ReflectionVariableLayoutNode : ReflectionNode
{
    // The original variable that got laid out
    ReflectionPtr<ReflectionVariableNode> variable;

    // The layout generated for the variable's type
    ReflectionPtr<ReflectionTypeLayoutNode> typeLayout;

    // The offset of this variable within its logical parent.
    ReflectionSize offset;

    ReflectionVariableNode* GetVariable() const { return variable; }
    char const* GetName() const { return variable->GetName(); }
    ReflectionTypeNode* GetType() const { return variable->GetType(); }
    ReflectionTypeLayoutNode* GetTypeLayout() const { return typeLayout; }
    size_t GetOffset() const { return offset; }
};

// Scalar types don't need to store much additional information.
struct ReflectionScalarTypeNode : ReflectionTypeNode
{
    // Store the scalar type in one of the "extra" fields
    spire::TypeReflection::ScalarType GetScalarType() const { return (spire::TypeReflection::ScalarType) type.scalar.scalarType; }
    void SetScalarType(SpireScalarType scalarType) { type.scalar.scalarType = scalarType; }
};

// Structure types need to store a bunch of fields.
//
// We handle this by requiring that the fields of a struct type node
// are stores as `ReflectionVariableNode` instances tightly packed
// after the `ReflectionStructTypeNode`.
struct ReflectionStructTypeNode : ReflectionTypeNode
{
    // The number of fields is stored in one of the "extra" fields
    // Note: this limits us to 64k fields in a given `struct` type.
    uint32_t GetFieldCount() const
    {
        if(!this) return 0;
        return type.structType.fieldCount;
    }
    void SetFieldCount(size_t fieldCount)
    {
        type.structType.fieldCount = (uint16_t) fieldCount;

        // make sure it fit!
        assert(fieldCount == type.structType.fieldCount);
    }

    ReflectionVariableNode* GetFieldByIndex(size_t index) const
    {
        if(!this) return nullptr;

        // We know the fields are stored densely packed right after this node.
        auto fields = (ReflectionVariableNode*) (this + 1);
        return &fields[index];
    }
};

// A layout node for a `struct` type is stored similarly, but it stores
// variable layout nodes for the fields, instead of variable nodes.
struct ReflectionStructTypeLayoutNode : ReflectionTypeLayoutNode
{
    // Convenience routine to access the underlying type node as a struct type
    ReflectionStructTypeNode* GetType() const { return (ReflectionStructTypeNode*) type.Ptr(); }

    // Get field count from underlying type
    uint32_t GetFieldCount() const { return GetType()->GetFieldCount(); }

    // Indexing logic for fields in a struct type layout matches the type case
    ReflectionVariableLayoutNode* GetFieldByIndex(size_t index) const
    {
        if(!this) return nullptr;
        auto fields = (ReflectionVariableLayoutNode*) (this + 1);
        return &fields[index];
    }
};

// An array type node stores the element type and count.
struct ReflectionArrayTypeNode : ReflectionTypeNode
{
    // The element type
    ReflectionPtr<ReflectionTypeNode> elementType;

    // The number of elements.
    ReflectionSize elementCount;

    uint32_t GetElementCount() const { return elementCount; }
    ReflectionTypeNode* GetElementType() const { return elementType; }
};

// A layout for an array type needs to store the layout for the element
// type, and also the "stride" between array elements (which may not
// necessarily be the size of the element type)
struct ReflectionArrayTypeLayoutNode : ReflectionTypeLayoutNode
{
    // layout for the element type
    ReflectionPtr<ReflectionTypeLayoutNode> elementTypeLayout;

    // stride in bytes between array elements
    ReflectionSize elementStride;

    // Get the underlying array type.
    ReflectionArrayTypeNode* GetType() const { return (ReflectionArrayTypeNode*) type.Ptr(); }

    // Forward request for element count to the underlying type.
    uint32_t GetElementCount() const { return GetType()->GetElementCount(); }

    // Access the element stride
    uint32_t GetElementStride() const { return elementStride; }

    // Access the element type layout
    ReflectionTypeLayoutNode* GetElementTypeLayout() const { return elementTypeLayout; }
};

// The remaining cases of  types don't have specialized layout information,
// so they don't need both a type and a type-layout node.

// Vector types
struct ReflectionVectorTypeNode : ReflectionTypeNode
{
    ReflectionPtr<ReflectionScalarTypeNode> elementType;

    // Store the element count in one of the "extra" fields
    uint32_t GetElementCount() const { return type.vector.elementCount; }
    void SetElementCount(uint32_t count) { type.vector.elementCount = count; }

    ReflectionScalarTypeNode* GetElementType() const { return elementType; }
};

struct ReflectionMatrixTypeNode : ReflectionTypeNode
{
    ReflectionPtr<ReflectionScalarTypeNode> elementType;

    uint32_t GetRowCount() const { return type.matrix.rowCount; }
    void SetRowCount(uint32_t count) { type.matrix.rowCount = count; }
    
    uint32_t GetColumnCount() const { return type.matrix.columnCount; }
    void SetColumnCount(uint32_t count) { type.matrix.columnCount = count; }

    ReflectionScalarTypeNode* GetElementType() const { return elementType; }
};

struct ReflectionConstantBufferTypeNode : ReflectionTypeNode
{
    ReflectionPtr<ReflectionTypeLayoutNode> elementType;

    ReflectionTypeLayoutNode* GetElementType() const { return elementType; }
};

struct ReflectionTextureTypeNode : ReflectionTypeNode
{
    ReflectionPtr<ReflectionTypeNode> elementType;

    SpireTextureShape GetShape() const { return type.resource.shape; }
    void SetShape(SpireTextureShape shape) { type.resource.shape = shape; }

    ReflectionTypeNode* GetElementType() const { return elementType; }
};

struct ReflectionSamplerStateTypeNode : ReflectionTypeNode
{
    // TODO(tfoley): record whether it was a shadow one or not?
};



struct ReflectionParameterNode : ReflectionVariableNode
{
    ReflectionSize category;
    ReflectionSize bindingIndex;
    ReflectionSize bindingSpace;

    SpireParameterCategory GetCategory() const { return (SpireParameterCategory) category; }
    uint32_t GetBindingIndex() const { return bindingIndex; }
    uint32_t GetBindingSpace() const { return bindingSpace; }
};

struct ReflectionBlob : ReflectionNode
{
    // total size of the reflection data, in bytes
    size_t reflectionDataSize;
    ReflectionSize parameterCount;
    ReflectionSize pad;

    static ReflectionBlob* Create(RefPtr<ProgramSyntaxNode> program);

    uint32_t GetParameterCount() const { return parameterCount; }
    ReflectionParameterNode* GetParameterByIndex(uint32_t index) const
    {
        auto params = (ReflectionParameterNode*) (this + 1);
        return &params[index];
    }

    size_t GetReflectionDataSize() const
    {
        return reflectionDataSize;
    }
};



//

inline ReflectionTypeNode* ReflectionNode::AsType()
{
    if(!this) return nullptr;
    switch( GetFlavor() )
    {
    case ReflectionNodeFlavor::Type:
        return (ReflectionTypeNode*) this;

    case ReflectionNodeFlavor::TypeLayout:
        return ((ReflectionTypeLayoutNode*) this)->GetType();

    default:
        return nullptr;
    }
}

inline ReflectionVariableNode* ReflectionNode::AsVariable()
{
    if(!this) return nullptr;
    switch( GetFlavor() )
    {
    case ReflectionNodeFlavor::Variable:
    case ReflectionNodeFlavor::Parameter:
        return (ReflectionVariableNode*) this;

    case ReflectionNodeFlavor::VariableLayout:
        return ((ReflectionVariableLayoutNode*) this)->GetVariable();

    default:
        return nullptr;
    }
}


inline ReflectionTypeNode* ReflectionTypeNode::UnwrapArrays()
{
    ReflectionTypeNode* type = this;
    for( ;;)
    {
        auto arrayType = type->AsArray();
        if(!arrayType) return type;
        type = arrayType->GetElementType();
    }
}


}}

#endif // SPIRE_REFLECTION_H
