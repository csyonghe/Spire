// Reflection.cpp
#include "Reflection.h"

#include "TypeLayout.h"

#include <assert.h>

namespace Spire {
namespace Compiler {

struct ReflectionGenerationContext
{
    List<uint8_t> data;
};

template<typename T>
struct NodePtr
{
    ReflectionGenerationContext*    context;
    size_t                          offset;

    NodePtr()
        : context(0), offset(0)
    {}

    NodePtr(
        ReflectionGenerationContext*    context,
        size_t                          offset)
        : context(context)
        , offset(offset)
    {}

    template<typename U>
    NodePtr(
        NodePtr<U> ptr,
        typename EnableIf<IsConvertible<T*, U*>::Value, void>::type * = 0)
        : context(ptr.context)
        , offset(ptr.offset)
    {}

    operator T*()
    {
        return (T*) (context->data.begin() + offset);
    }

    T* operator->()
    {
        return *this;
    }

    NodePtr<T> operator+(size_t index)
    {
        return NodePtr<T>(context, offset + index*sizeof(T));
    }

    template<typename U>
    NodePtr<U> Cast()
    {
        return NodePtr<U>(context, offset);
    }
};

template<typename T>
NodePtr<T> MakeNodePtr(ReflectionGenerationContext* context, T* rawPtr)
{
    NodePtr<T> nodePtr;
    nodePtr.context = context;
    nodePtr.offset = ((uint8_t*)rawPtr - context->data.begin());
    return nodePtr;
}

static size_t AllocateNodeData(
    ReflectionGenerationContext*    context,
    size_t                          size,
    size_t                          alignment,
    size_t                          count)
{
    size_t mask = alignment - 1;

    // Pad out the existing data to our alignment
    while( (context->data.Count() & mask) != 0 )
    {
        // TODO(tfoley): There's got to be something faster than this...
        context->data.Add(0);
    }

    // Starting offset for the new node data
    size_t result = context->data.Count();

    // Stride between elements (should always == size in C/C++)
    size_t stride = (size + alignment-1) & ~(alignment-1);

    // bytes to allocate
    size_t byteCount = stride * count;

    for( size_t bb = 0; bb < byteCount; ++bb )
    {
        // TODO(tfoley): There's got to be something faster than this...
        context->data.Add(0);
    }

    return result;
}

template<typename T>
static NodePtr<T> AllocateNode(
    ReflectionGenerationContext*    context)
{
    return NodePtr<T>(context, AllocateNodeData(context, sizeof(T), alignof(T), 1));
}

template<typename T>
static NodePtr<T> AllocateNodes(
    ReflectionGenerationContext*    context,
    size_t                          count = 1)
{
    return NodePtr<T>(context, AllocateNodeData(context, sizeof(T), alignof(T), count));
}

//

static NodePtr<ReflectionTypeNode> GenerateReflectionType(
    ReflectionGenerationContext*    context,
    RefPtr<ExpressionType>          type);

static NodePtr<ReflectionTypeLayoutNode> GenerateReflectionTypeLayout(
    ReflectionGenerationContext*    context,
    RefPtr<ExpressionType>          type,
    LayoutRule                      rule);

//

static NodePtr<char> GenerateReflectionName(
    ReflectionGenerationContext*    context,
    String const&                   text)
{
    // TODO(tfoley): It would be a good idea to only emit each unique string
    // once, to avoid wasting a bunch of space.

    size_t textSize = text.Length();

// Need enough space for given text plus NULL byte
NodePtr<char> ptr = AllocateNodes<char>(context, textSize + 1);

memcpy(ptr, text.begin(), textSize);
ptr[textSize] = 0;

return ptr;
}

static void GenerateReflectionVar(
    ReflectionGenerationContext*    context,
    VarDeclBaseRef                  declRef,
    NodePtr<ReflectionVariableNode> info)
{
    info->flavor = ReflectionNodeFlavor::Variable;
    info->name = GenerateReflectionName(context, declRef.GetName());
    info->type = GenerateReflectionType(context, declRef.GetType());
}

static NodePtr<ReflectionTypeNode> GenerateReflectionType(
    ReflectionGenerationContext*    context,
    RefPtr<ExpressionType>          type)
{
    // TODO(tfoley: Don't emit the same type more than once...

    if (auto basicType = type->As<BasicExpressionType>())
    {
        auto info = AllocateNode<ReflectionScalarTypeNode>(context);
        info->flavor = ReflectionNodeFlavor::Type;
        info->SetKind(SPIRE_TYPE_KIND_SCALAR);
        switch (basicType->BaseType)
        {
#define CASE(BASE, TAG) \
        case BaseType::BASE: info->SetScalarType(SPIRE_SCALAR_TYPE_##TAG); break

            CASE(Void, VOID);
            CASE(Int, INT32);
            CASE(Float, FLOAT32);
            CASE(UInt, UINT32);
            CASE(Bool, BOOL);
            CASE(UInt64, UINT64);

#undef CASE

        default:
            assert(!"unexpected");
            info->SetScalarType(SPIRE_SCALAR_TYPE_NONE);
            break;
        }
        return info;
    }
    else if (auto vectorType = type->As<VectorExpressionType>())
    {
        auto info = AllocateNode<ReflectionVectorTypeNode>(context);
        info->flavor = ReflectionNodeFlavor::Type;
        info->SetKind(SPIRE_TYPE_KIND_VECTOR);
        info->SetElementCount(GetIntVal(vectorType->elementCount));
        info->elementType = GenerateReflectionType(context, vectorType->elementType).Cast<ReflectionScalarTypeNode>();
        return info;
    }
    else if (auto matrixType = type->As<MatrixExpressionType>())
    {
        auto info = AllocateNode<ReflectionMatrixTypeNode>(context);
        info->flavor = ReflectionNodeFlavor::Type;
        info->SetKind(SPIRE_TYPE_KIND_MATRIX);
        info->SetRowCount(GetIntVal(matrixType->rowCount));
        info->SetColumnCount(GetIntVal(matrixType->colCount));
        info->elementType = GenerateReflectionType(context, matrixType->elementType).Cast<ReflectionScalarTypeNode>();
        return info;
    }
    else if (auto constantBufferType = type->As<ConstantBufferType>())
    {
        auto info = AllocateNode<ReflectionConstantBufferTypeNode>(context);
        info->flavor = ReflectionNodeFlavor::Type;
        info->SetKind(SPIRE_TYPE_KIND_CONSTANT_BUFFER);
        info->elementType = GenerateReflectionTypeLayout(
            context,
            constantBufferType->elementType,
            LayoutRule::Std140); // TODO(tfoley): pick up layout rule from declaration...
        return info;
    }
    else if (auto samplerStateType = type->As<SamplerStateType>())
    {
        auto info = AllocateNode<ReflectionSamplerStateTypeNode>(context);
        info->flavor = ReflectionNodeFlavor::Type;
        info->SetKind(SPIRE_TYPE_KIND_SAMPLER_STATE);
        return info;
    }
    else if (auto textureType = type->As<TextureType>())
    {
        auto info = AllocateNode<ReflectionTextureTypeNode>(context);
        info->flavor = ReflectionNodeFlavor::Type;
        info->SetKind(SPIRE_TYPE_KIND_TEXTURE);
        info->SetShape(textureType->flavor);
        info->elementType = GenerateReflectionType(context, textureType->elementType);
        return info;
    }
    else if (auto arrayType = type->As<ArrayExpressionType>())
    {
        auto info = AllocateNode<ReflectionArrayTypeNode>(context);
        info->flavor = ReflectionNodeFlavor::Type;
        info->SetKind(SPIRE_TYPE_KIND_ARRAY);
        info->elementType = GenerateReflectionType(context, arrayType->BaseType);
        info->elementCount = arrayType->ArrayLength ? GetIntVal(arrayType->ArrayLength) : 0;
        return info;
    }
    else if( auto declRefType = type->As<DeclRefType>() )
    {
        auto declRef = declRefType->declRef;
        if( auto structDeclRef = declRef.As<StructDeclRef>() )
        {
            auto info = AllocateNode<ReflectionStructTypeNode>(context);
            info->flavor = ReflectionNodeFlavor::Type;
            info->SetKind(SPIRE_TYPE_KIND_STRUCT);

            size_t fieldCount = structDeclRef.GetDecl()->GetFields().Count();
            info->SetFieldCount(fieldCount);

            auto fields = AllocateNodes<ReflectionVariableNode>(context, fieldCount);

            size_t fieldIndex = 0;
            for(auto ff : structDeclRef.GetFields())
            {
                auto field = fields + fieldIndex;

                // TODO: fill the damn thing in!!!!!
                GenerateReflectionVar(context, ff, field);

                fieldIndex++;
            }

            return info;
        }
    }

    assert(!"unexpected");
    return NodePtr<ReflectionTypeNode>();
}

static NodePtr<ReflectionTypeLayoutNode> GenerateReflectionTypeLayout(
    ReflectionGenerationContext*    context,
    NodePtr<ReflectionTypeNode>     typeNode,
    LayoutRulesImpl*                rules,
    LayoutInfo*                     outLayoutInfo)
{
    switch( typeNode->GetKind() )
    {
    case spire::TypeReflection::Kind::Scalar:
        {
            auto scalarTypeNode = typeNode.Cast<ReflectionScalarTypeNode>();
            auto scalarLayout = rules->GetScalarLayout(scalarTypeNode->GetScalarType());

            auto info = AllocateNode<ReflectionTypeLayoutNode>(context);
            info->flavor = ReflectionNodeFlavor::TypeLayout;
            info->type = typeNode;
            info->size = (ReflectionSize) scalarLayout.size;

            *outLayoutInfo = scalarLayout;
            return info;
        }

    case spire::TypeReflection::Kind::Vector:
        {
            auto vectorTypeNode = typeNode.Cast<ReflectionVectorTypeNode>();
            auto scalarLayout = rules->GetScalarLayout(vectorTypeNode->GetElementType()->GetScalarType());
            auto vectorLayout = rules->GetVectorLayout(scalarLayout, vectorTypeNode->GetElementCount());

            auto info = AllocateNode<ReflectionTypeLayoutNode>(context);
            info->flavor = ReflectionNodeFlavor::TypeLayout;
            info->type = typeNode;
            info->size = (ReflectionSize) vectorLayout.size;

            *outLayoutInfo = vectorLayout;
            return info;
        }

    case spire::TypeReflection::Kind::Matrix:
        {
            auto matrixTypeNode = typeNode.Cast<ReflectionMatrixTypeNode>();
            auto scalarLayout = rules->GetScalarLayout(matrixTypeNode->GetElementType()->GetScalarType());
            auto matrixLayout = rules->GetMatrixLayout(scalarLayout, matrixTypeNode->GetRowCount(), matrixTypeNode->GetColumnCount());

            auto info = AllocateNode<ReflectionTypeLayoutNode>(context);
            info->flavor = ReflectionNodeFlavor::TypeLayout;
            info->type = typeNode;
            info->size = (ReflectionSize) matrixLayout.size;

            *outLayoutInfo = matrixLayout;
            return info;
        }

    case SPIRE_TYPE_KIND_CONSTANT_BUFFER:
    case SPIRE_TYPE_KIND_SAMPLER_STATE:
    case SPIRE_TYPE_KIND_TEXTURE:
        {
            auto objectLayout = rules->GetObjectLayout(
                (spire::TypeReflection::Kind) typeNode->GetKind());

            auto info = AllocateNode<ReflectionTypeLayoutNode>(context);
            info->flavor = ReflectionNodeFlavor::TypeLayout;
            info->type = typeNode;
            info->size = (ReflectionSize)objectLayout.size;

            *outLayoutInfo = objectLayout;
            return info;
        }

    case spire::TypeReflection::Kind::Array:
        {
            auto info = AllocateNode<ReflectionArrayTypeLayoutNode>(context);

            auto arrayTypeNode = typeNode.Cast<ReflectionArrayTypeNode>();
            auto elementTypeNode = MakeNodePtr(context, arrayTypeNode->GetElementType());

            LayoutInfo elementLayout;
            auto elementTypeLayout = GenerateReflectionTypeLayout(
                context,
                elementTypeNode,
                rules,
                &elementLayout);

            auto arrayLayout = rules->GetArrayLayout(elementLayout, arrayTypeNode->GetElementCount());

            info->flavor = ReflectionNodeFlavor::TypeLayout;
            info->type = typeNode;
            info->size = (ReflectionSize) arrayLayout.size;
            info->elementTypeLayout = elementTypeLayout;
            info->elementStride = (ReflectionSize) arrayLayout.elementStride;

            *outLayoutInfo = arrayLayout;
            return info;
        }

    case spire::TypeReflection::Kind::Struct:
        {
            auto structTypeNode = typeNode.Cast<ReflectionStructTypeNode>();

            auto info = AllocateNode<ReflectionStructTypeLayoutNode>(context);
            info->flavor = ReflectionNodeFlavor::TypeLayout;
            info->type = typeNode;

            size_t fieldCount = structTypeNode->GetFieldCount();
            auto fieldLayoutNodes = AllocateNodes<ReflectionVariableLayoutNode>(context, fieldCount);

            LayoutInfo structLayout = rules->BeginStructLayout();

            for( size_t ff = 0; ff < fieldCount; ++ff )
            {
                auto fieldNode = structTypeNode->GetFieldByIndex(ff);
                auto fieldLayoutNode = fieldLayoutNodes + ff;

                LayoutInfo fieldTypeLayout;
                auto fieldTypeLayoutNode = GenerateReflectionTypeLayout(
                    context,
                    MakeNodePtr(context, fieldNode->GetType()),
                    rules,
                    &fieldTypeLayout);

                size_t fieldOffset = rules->AddStructField(&structLayout, fieldTypeLayout);

                fieldLayoutNode->flavor = ReflectionNodeFlavor::VariableLayout;
                fieldLayoutNode->variable = fieldNode;
                fieldLayoutNode->typeLayout = fieldTypeLayoutNode;
                fieldLayoutNode->offset = (ReflectionSize) fieldOffset;
            }

            rules->EndStructLayout(&structLayout);

            info->size = (ReflectionSize) structLayout.size;
            *outLayoutInfo = structLayout;
            return info;
        }
    }

    assert(!"unexpected");
    return NodePtr<ReflectionTypeLayoutNode>();
}

static NodePtr<ReflectionTypeLayoutNode> GenerateReflectionTypeLayout(
    ReflectionGenerationContext*    context,
    RefPtr<ExpressionType>          type,
    LayoutRule                      rule)
{
    auto typeNode = GenerateReflectionType(context, type);
    auto rules = GetLayoutRulesImpl(rule);

    LayoutInfo layoutInfo;
    return GenerateReflectionTypeLayout(context, typeNode, rules, &layoutInfo);
}

static bool IsReflectionParameter(
    ReflectionGenerationContext*        context,
    RefPtr<Decl>                        decl)
{
    if( auto varDecl = decl.As<VarDeclBase>() )
    {
        // We need to determine if this variable represents a shader
        // parameter, or just an ordinary global variable...
        if(varDecl->HasModifier<HLSLStaticModifier>())
            return false;

        // TODO(tfoley): there may be other cases that we need to handle here

        return true;
    }
    else
    {
        // Only variable declarations can represent parameters at global scope
        return false;
    }
}

static SpireParameterCategory ComputeRefelctionParameterCategory(
    ReflectionGenerationContext*        context,
    RefPtr<TypeLayout>                  typeLayout)
{
    // If we have any uniform data, then we are either uniform,
    // or mixed-type
    if (typeLayout->uniforms.size != 0)
    {
        // If we have any non-uniform data, then we are mixed.
        if (IsResourceKind(typeLayout->resources.kind))
        {
            return SPIRE_PARAMETER_CATEGORY_MIXED;
        }
        else
        {
            return SPIRE_PARAMETER_CATEGORY_UNIFORM;
        }
    }

    // Otherwise we only have resource fields.

    // If we have more then one kind of resource,
    // then the whole thing is mixed-type
    if (typeLayout->resources.next)
    {
        return SPIRE_PARAMETER_CATEGORY_MIXED;
    }
    
    // Otherwise look at the kind of resource
    switch (typeLayout->resources.kind)
    {
    case LayoutResourceKind::ConstantBuffer:
        return SPIRE_PARAMETER_CATEGORY_CONSTANT_BUFFER;
    case LayoutResourceKind::ShaderResource:
        return SPIRE_PARAMETER_CATEGORY_SHADER_RESOURCE;
    case LayoutResourceKind::UnorderedAccess:
        return SPIRE_PARAMETER_CATEGORY_UNORDERED_ACCESS;
    case LayoutResourceKind::SamplerState:
        return SPIRE_PARAMETER_CATEGORY_SAMPLER_STATE;

    default:
        return SPIRE_PARAMETER_CATEGORY_NONE;
    }
}

static void GenerateReflectionParameter(
    ReflectionGenerationContext*        context,
    RefPtr<VarLayout>                   paramLayout,
    NodePtr<ReflectionParameterNode>    parameter)
{
    auto varDecl = paramLayout->varDecl.GetDecl();

    // Just to confirm that we aren't applying this logic to something we shouldn't
    assert(!varDecl->HasModifier<HLSLStaticModifier>());

    // Figure out what kind of parameter we are looking at:
    auto category = ComputeRefelctionParameterCategory(context, paramLayout->typeLayout);

    parameter->name = GenerateReflectionName(context, varDecl->Name.Content);
    parameter->type = GenerateReflectionType(context, varDecl->Type);
    parameter->category = category;

    if (category == SPIRE_PARAMETER_CATEGORY_MIXED)
    {
        parameter->flavor = ReflectionNodeFlavor::MultiParameter;
        // More than one resource: need to handle this in a special way

        assert(!"unimplemented");
    }
    else if (category == SPIRE_PARAMETER_CATEGORY_UNIFORM)
    {
        // A uniform parameter inside of a parent buffer.
        parameter->flavor = ReflectionNodeFlavor::Parameter;
        parameter->bindingSpace = 0;
        parameter->bindingIndex = (ReflectionSize) paramLayout->uniformOffset;
    }
    else
    {
        // A resource parameter
        parameter->flavor = ReflectionNodeFlavor::Parameter;
        parameter->bindingSpace = (ReflectionSize) paramLayout->resources.space;
        parameter->bindingIndex = (ReflectionSize) paramLayout->resources.index;
    }
}


static ReflectionBlob* GenerateReflectionBlob(
    ReflectionGenerationContext*    context,
    RefPtr<ProgramSyntaxNode>       program)
{
    // We need the program to already have had parameter binding performed.

    auto programLayoutMod = program->FindModifier<ComputedLayoutModifier>();
    if (!programLayoutMod)
    {
        // TODO: error message
        return nullptr;
    }
    auto programLayout = programLayoutMod->typeLayout.As<ProgramLayout>();
    if (!programLayout)
    {
        // TODO: error message
        return nullptr;
    }

    // We need to walk the declarations in the program, and look for those that identify logical shader parameters
    NodePtr<ReflectionBlob> blob = AllocateNode<ReflectionBlob>(context);
    blob->flavor = ReflectionNodeFlavor::Blob;

    // First let's count how many parameters there are to consider
    size_t parameterCount = programLayout->fields.Count();
    blob->parameterCount = (ReflectionSize) parameterCount;

    // Now allocate space for the parameters (to go right after the blob itself) and fill them in
    NodePtr<ReflectionParameterNode> parameters = AllocateNodes<ReflectionParameterNode>(context, parameterCount);
    size_t parameterIndex = 0;
    for( auto paramLayout : programLayout->fields )
    {
        GenerateReflectionParameter(context, paramLayout, parameters + parameterIndex);
        parameterIndex++;
    }
    assert(parameterIndex == parameterCount);

    // We are done emitting things, so lets look at what we got


    // The total number of words emitted needs to fit in a word itself
    size_t dataSize = context->data.Count();

    // Need to update the blob info based on what we output
    blob->reflectionDataSize = (ReflectionSize) dataSize;

    // Make a raw memory allocation to hold the final data
    ReflectionBlob* result = (ReflectionBlob*) malloc(dataSize);
    memcpy(result, context->data.begin(), dataSize);

    return result;
}

ReflectionBlob* ReflectionBlob::Create(RefPtr<ProgramSyntaxNode> program)
{
    ReflectionGenerationContext context;
    return GenerateReflectionBlob(&context, program);
}

}}
