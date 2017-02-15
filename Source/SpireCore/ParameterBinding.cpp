// ParameterBinding.cpp
#include "ParameterBinding.h"

#include "TypeLayout.h"

#include "../../Spire.h"

namespace Spire {
namespace Compiler {

// Information on ranges of registers already claimed/used
struct UsedRange
{
    size_t begin;
    size_t end;
};
bool operator<(UsedRange left, UsedRange right)
{
    if (left.begin != right.begin)
        return left.begin < right.begin;
    if (left.end != right.end)
        return left.end < right.end;
    return false;
}

struct UsedRanges
{
    List<UsedRange> ranges;

    // Add a range to the set, either by extending
    // an existing range, or by adding a new one...
    void Add(UsedRange const& range)
    {
        for (auto& rr : ranges)
        {
            if (rr.begin == range.end)
            {
                rr.begin = range.begin;
                return;
            }
            else if (rr.end == range.begin)
            {
                rr.end = range.end;
                return;
            }
        }
        ranges.Add(range);
        ranges.Sort();
    }

    void Add(size_t begin, size_t end)
    {
        UsedRange range;
        range.begin = begin;
        range.end = end;
        Add(range);
    }


    // Try to find space for `count` entries
    size_t Allocate(size_t count)
    {
        size_t begin = 0;

        size_t rangeCount = ranges.Count();
        for (size_t rr = 0; rr < rangeCount; ++rr)
        {
            // try to fit in before this range...

            size_t end = ranges[rr].begin;

            // If there is enough space...
            if (end > begin + count)
            {
                // ... then claim it and be done
                Add(begin, end);
                return begin;
            }

            // ... otherwise, we need to look at the
            // space between this range and the next
            begin = ranges[rr].end;
        }

        // We've run out of ranges to check, so we
        // can safely go after the last one!
        Add(begin, begin + count);
        return begin;
    }
};

struct SharedParameterBindingContext
{
    LayoutRulesImpl* defaultLayoutRules;

    // Parameters that we've discovered and started to lay out...
    List<RefPtr<VarLayout>> varLayouts;
};

struct ParameterBindingContext
{
    SharedParameterBindingContext* shared;

    // TODO: information for implicit constant buffer!

    // The layout rules to use while computing usage...
    LayoutRulesImpl* layoutRules;

    UsedRanges usedResourceRanges[(int)LayoutResourceKind::Count];
};

// Check whether a global variable declaration represents
// a shader parameter, or is there for some other purpose.
static bool IsGlobalVariableAShaderParameter(
    ParameterBindingContext*    /*context*/,
    RefPtr<VarDeclBase>         varDecl)
{
    // We need to determine if this variable represents a shader
    // parameter, or just an ordinary global variable...
    if(varDecl->HasModifier<HLSLStaticModifier>())
        return false;

    // TODO(tfoley): there may be other cases that we need to handle here

    return true;
}

struct LayoutSemanticInfo
{
    LayoutResourceKind  kind; // the register kind
    size_t              space;
    size_t              index;

    // TODO: need to deal with component-granularity binding...
};

LayoutSemanticInfo ExtractLayoutSemanticInfo(
    ParameterBindingContext*    /*context*/,
    HLSLLayoutSemantic*         semantic)
{
    LayoutSemanticInfo info;
    info.space = 0;
    info.index = 0;
    info.kind = LayoutResourceKind::Invalid;

    auto registerName = semantic->registerName.Content;
    if (registerName.Length() == 0)
        return info;

    LayoutResourceKind kind = LayoutResourceKind::Invalid;
    switch (registerName[0])
    {
    case 'b':
        kind = LayoutResourceKind::ConstantBuffer;
        break;

    case 't':
        kind = LayoutResourceKind::ShaderResource;
        break;

    case 'u':
        kind = LayoutResourceKind::UnorderedAccess;
        break;

    case 's':
        kind = LayoutResourceKind::SamplerState;
        break;

    default:
        // TODO: issue an error here!
        return info;
    }

    // TODO: need to parse and handle `space` binding
    size_t space = 0;

    size_t index = 0;
    for (int ii = 1; ii < registerName.Length(); ++ii)
    {
        int c = registerName[ii];
        if (c >= '0' && c <= '9')
        {
            index = index * 10 + (c - '0');
        }
        else
        {
            // TODO: issue an error here!
            return info;
        }
    }

    // TODO: handle component mask part of things...

    info.kind = kind;
    info.index = index;
    info.space = space;
    return info;
}

// Generate the binding information for a shader parameter.
static void GenerateBindingsForParameter(
    ParameterBindingContext*    context,
    RefPtr<VarDeclBase>         varDecl)
{
    // Find out the layout for the type...
    RefPtr<TypeLayout> typeLayout = CreateTypeLayout(
        varDecl->Type.Ptr(),
        context->layoutRules);

    // Now create a variable layout that we can use
    RefPtr<VarLayout> varLayout = new VarLayout();
    varLayout->typeLayout = typeLayout;
    varLayout->varDecl = DeclRef(varDecl.Ptr(), nullptr).As<VarDeclBaseRef>();

    context->shared->varLayouts.Add(varLayout);

    // If the parameter has explicit binding modifiers, then
    // here is where we want to extract and apply them...

    // Look for HLSL `register` or `packoffset` semantics.
    for (auto semantic : varDecl->GetModifiersOfType<HLSLLayoutSemantic>())
    {
        // Need to extract the information encoded in the semantic
        LayoutSemanticInfo semanticInfo = ExtractLayoutSemanticInfo(context, semantic);
        if (semanticInfo.kind == LayoutResourceKind::Invalid)
            continue;

        // TODO: need to special-case when this is a `c` register binding...

        // Find the appropriate resource-binding information
        // inside the type, to see if we even use any resources
        // of the given kind.

        auto typeRes = typeLayout->FindResourceInfo(semanticInfo.kind);
        int count = 0;
        if (typeRes)
        {
            count = typeRes->count;
        }
        else
        {
            // TODO: warning here!
        }

        // Now we need to add the appropriate binding
        // to the variable layout
        auto varRes = varLayout->FindResourceInfo(semanticInfo.kind);
        if (varRes)
        {
            // TODO: error: we are trying to bind the same thing twice!?!?!

            // TODO(tfoley): `register` semantics can technically be
            // profile-specific (not sure if anybody uses that)...
        }
        varRes = varLayout->AddResourceInfo(semanticInfo.kind);
        varRes->space = semanticInfo.space;
        varRes->index = semanticInfo.index;

        // If things are bound in `space0` (the default), then we need
        // to lay claim to the register range used, so that automatic
        // assignment doesn't go and use the same registers.
        if (semanticInfo.space == 0)
        {
            context->usedResourceRanges[(int)semanticInfo.kind].Add(
                semanticInfo.index,
                count);
        }
    }
}

// Generate the binding information for a shader parameter.
static void CompleteBindingsForParameter(
    ParameterBindingContext*    context,
    RefPtr<VarLayout>           varLayout)
{
    // For any resource kind used by the variable,
    // we need to update its layout information
    // to include a binding for that resource kind.

    auto typeLayout = varLayout->typeLayout;

    for (auto typeRes = &typeLayout->resources;
        typeRes && IsResourceKind(typeRes->kind);
        typeRes = typeRes->next.Ptr())
    {
        // check if the variable already has a binding applied
        // for this resource kind...
        auto varRes = varLayout->FindResourceInfo(typeRes->kind);
        if (varRes)
        {
            // If things have already been bound, our work is done.
            continue;
        }

        // Otherwise, allocate a resource-info node, and then
        // allocate a range of registers to use when binding
        // this parameter.
        varRes = varLayout->AddResourceInfo(typeRes->kind);
        varRes->index = context->usedResourceRanges[(int)typeRes->kind].Allocate(typeRes->count);
    }
}

static void GenerateParameterBindings(
    ParameterBindingContext*    context,
    ProgramSyntaxNode*          program)
{
    // First enumerate parameters at global scope
    for( auto decl : program->Members )
    {
        // A shader parameter is always a variable,
        // so skip declarations that aren't variables.
        auto varDecl = decl.As<VarDeclBase>();
        if (!varDecl)
            continue;

        // Skip globals that don't look like parameters
        if (!IsGlobalVariableAShaderParameter(context, varDecl))
            continue;

        GenerateBindingsForParameter(context, varDecl);
    }

    // Next, we need to enumerate the parameters of
    // each entry point (which requires knowing what the
    // entry points *are*)

    // TODO(tfoley): Entry point functions should be identified
    // by looking for a generated modifier that is attached
    // to global-scope function declarations.
}

void GenerateParameterBindings(
    ProgramSyntaxNode* program)
{
    // Create a context to hold shared state during the process
    // of generating parameter bindings
    SharedParameterBindingContext sharedContext;
    sharedContext.defaultLayoutRules = GetLayoutRulesImpl(LayoutRule::HLSLConstantBuffer);

    // Create a sub-context to collect parameters that get
    // declared into the global scope
    ParameterBindingContext context;
    context.shared = &sharedContext;
    context.layoutRules = sharedContext.defaultLayoutRules;

    // Walk through global scope to create binding nodes for everything
    GenerateParameterBindings(&context, program);

    // Now walk through again to actually give everything
    // ranges of registers...
    for (auto varLayout : sharedContext.varLayouts)
    {
        CompleteBindingsForParameter(&context, varLayout);
    }

    // We now have a bunch of layout information, which we should
    // record into a suitable object that represents the program
    RefPtr<ProgramLayout> programLayout = new ProgramLayout;
    programLayout->rules = context.layoutRules;
    for (auto varLayout : sharedContext.varLayouts)
    {
        programLayout->fields.Add(varLayout);
    }

    // We'd like to attach this layout to the program now,
    // as a modifier node...

    RefPtr<ComputedLayoutModifier> modifier = new ComputedLayoutModifier();
    modifier->typeLayout = programLayout;
    modifier->next = program->modifiers.first;
    program->modifiers.first = modifier;
}

}}
