// Emit.cpp
#include "Emit.h"

#include "Syntax.h"

#include <assert.h>

#ifdef _WIN32
#include <d3dcompiler.h>
#pragma warning(disable:4996)
#endif

namespace Spire { namespace Compiler {

struct EmitContext
{
	StringBuilder sb;
};

//

static void EmitDecl(EmitContext* context, RefPtr<Decl> decl);
static void EmitType(EmitContext* context, RefPtr<ExpressionType> type, String const& name);
static void EmitType(EmitContext* context, RefPtr<ExpressionType> type);
static void EmitExpr(EmitContext* context, RefPtr<ExpressionSyntaxNode> expr);
static void EmitStmt(EmitContext* context, RefPtr<StatementSyntaxNode> stmt);
static void EmitDeclRef(EmitContext* context, DeclRef declRef);

// Low-level emit logic

static void Emit(EmitContext* context, char const* textBegin, char const* textEnd)
{
	context->sb.Append(textBegin, int(textEnd - textBegin));
}

static void Emit(EmitContext* context, char const* text)
{
	Emit(context, text, text + strlen(text));
}

static void Emit(EmitContext* context, String const& text)
{
	Emit(context, text.begin(), text.end());
}

static void Emit(EmitContext* context, int value)
{
	char buffer[16];
	sprintf(buffer, "%d", value);
	Emit(context, buffer);
}

static void Emit(EmitContext* context, double value)
{
	// TODO(tfoley): need to print things in a way that can round-trip
	char buffer[128];
	sprintf(buffer, "%f", value);
	Emit(context, buffer);
}

// Expressions

// Determine if an expression should not be emitted when it is the base of
// a member reference expression.
static bool IsBaseExpressionImplicit(EmitContext* context, RefPtr<ExpressionSyntaxNode> expr)
{
	// HACK(tfoley): For now, anything with a constant-buffer type should be
	// left implicit.

	// Look through any dereferencing that took place
	RefPtr<ExpressionSyntaxNode> e = expr;
	while (auto derefExpr = e.As<DerefExpr>())
	{
		e = derefExpr->base;
	}
	// Is the expression referencing a constant buffer?
	if (auto cbufferType = e->Type->As<ConstantBufferType>())
	{
		return true;
	}

	return false;
}

static void EmitExpr(EmitContext* context, RefPtr<ExpressionSyntaxNode> expr)
{
	if (auto appExpr = expr.As<InvokeExpressionSyntaxNode>())
	{
		auto funcExpr = appExpr->FunctionExpr;
		if (auto funcDeclRefExpr = funcExpr.As<DeclRefExpr>())
		{
			auto declRef = funcDeclRefExpr->declRef;
			if (auto ctorDeclRef = declRef.As<ConstructorDeclRef>())
			{
				// We really want to emit a reference to the type begin constructed
				EmitType(context, expr->Type);
			}
			else
			{
				// default case: just emit the decl ref
				EmitExpr(context, funcExpr);
			}
		}
		else
		{
			// default case: just emit the expression
			EmitExpr(context, funcExpr);
		}

		Emit(context, "(");
		int argCount = appExpr->Arguments.Count();
		for (int aa = 0; aa < argCount; ++aa)
		{
			if (aa != 0) Emit(context, ", ");
			EmitExpr(context, appExpr->Arguments[aa]);
		}
		Emit(context, ")");
		return;
	}
	else if (auto memberExpr = expr.As<MemberExpressionSyntaxNode>())
	{
		// TODO(tfoley): figure out a good way to reference
		// declarations that might be generic and/or might
		// not be generated as lexically nested declarations...

		// TODO(tfoley): also, probably need to special case
		// this for places where we are using a built-in...

		auto base = memberExpr->BaseExpression;
		if (IsBaseExpressionImplicit(context, base))
		{
			// don't emit the base expression
		}
		else
		{
			EmitExpr(context, memberExpr->BaseExpression);
			Emit(context, ".");
		}

		Emit(context, memberExpr->declRef.GetName());
		return;
	}
	else if (auto swizExpr = expr.As<SwizzleExpr>())
	{
		EmitExpr(context, swizExpr->base);
		Emit(context, ".");
		static const char* kComponentNames[] = { "x", "y", "z", "w" };
		int elementCount = swizExpr->elementCount;
		for (int ee = 0; ee < elementCount; ++ee)
		{
			Emit(context, kComponentNames[swizExpr->elementIndices[ee]]);
		}
		return;
	}
	else if (auto varExpr = expr.As<VarExpressionSyntaxNode>())
	{
		EmitDeclRef(context, varExpr->declRef);
		return;
	}
	else if (auto derefExpr = expr.As<DerefExpr>())
	{
		// TODO(tfoley): dereference shouldn't always be implicit
		EmitExpr(context, derefExpr->base);
		return;
	}
	else if (auto litExpr = expr.As<ConstantExpressionSyntaxNode>())
	{
		switch (litExpr->ConstType)
		{
		case ConstantExpressionSyntaxNode::ConstantType::Int:
			Emit(context, litExpr->IntValue);
			break;
		case ConstantExpressionSyntaxNode::ConstantType::Float:
			Emit(context, litExpr->FloatValue);
			break;
		case ConstantExpressionSyntaxNode::ConstantType::Bool:
			Emit(context, litExpr->IntValue ? "true" : "false");
			break;
		default:
			assert(!"unreachable");
			break;
		}
		return;
	}
	else if (auto binExpr = expr.As<BinaryExpressionSyntaxNode>())
	{
		// TODO(tfoley): Need to deal with operator precedence

		EmitExpr(context, binExpr->LeftExpression);
		switch (binExpr->Operator)
		{
#define CASE(NAME, OP) case Operator::NAME: Emit(context, #OP); break
		CASE(Mul, *);
		CASE(Div, /);
		CASE(Mod, %);
		CASE(Add, +);
		CASE(Sub, -);
		CASE(Lsh, <<);
		CASE(Rsh, >>);
		CASE(Eql, ==);
		CASE(Neq, !=);
		CASE(Greater, >);
		CASE(Less, <);
		CASE(Geq, >=);
		CASE(Leq, <=);
		CASE(BitAnd, &=);
		CASE(BitXor, ^);
		CASE(BitOr, |);
		CASE(And, &&);
		CASE(Or, ||);
		CASE(Assign, =);
		CASE(AddAssign, +=);
		CASE(SubAssign, -=);
		CASE(MulAssign, *=);
		CASE(DivAssign, /=);
		CASE(ModAssign, %=);
		CASE(LshAssign, <<=);
		CASE(RshAssign, >>=);
		CASE(OrAssign, |=);
		CASE(AndAssign, &=);
		CASE(XorAssign, ^=);
#undef CASE
		default:
			assert(!"unreachable");
			break;
		}
		EmitExpr(context, binExpr->RightExpression);
		return;
	}
	else if (auto castExpr = expr.As<TypeCastExpressionSyntaxNode>())
	{
		Emit(context, "((");
		EmitType(context, castExpr->Type);
		Emit(context, ") ");
		EmitExpr(context, castExpr->Expression);
		Emit(context, ")");
		return;
	}
	throw "unimplemented";
}

// Types

// represents a declarator for use in emitting types
struct EDeclarator
{
	enum class Kind
	{
		Name,
	};
	Kind kind;
	EDeclarator* next = nullptr;

	// Used for `Kind::Name`
	String name;
};

static void EmitDeclarator(EmitContext* context, EDeclarator* declarator)
{
	if (!declarator) return;

	Emit(context, " ");

	switch (declarator->kind)
	{
	case EDeclarator::Kind::Name:
		Emit(context, declarator->name);
		break;

	default:
		assert(!"unreachable");
		break;
	}
}

static void EmitType(EmitContext* context, RefPtr<ExpressionType> type, EDeclarator* declarator)
{
	if (auto basicType = type->As<BasicExpressionType>())
	{
		switch (basicType->BaseType)
		{
		case BaseType::Void:	Emit(context, "void");		break;
		case BaseType::Int:		Emit(context, "int");		break;
		case BaseType::Float:	Emit(context, "float");		break;
		case BaseType::UInt:	Emit(context, "uint");		break;
		case BaseType::Bool:	Emit(context, "bool");		break;
		case BaseType::Error:	Emit(context, "<error>");	break;
		default:
			assert(!"unreachable");
			break;
		}

		EmitDeclarator(context, declarator);
		return;
	}
	else if (auto vecType = type->As<VectorExpressionType>())
	{
		// TODO(tfoley): should really emit these with sugar
		Emit(context, "vector<");
		EmitType(context, vecType->elementType);
		Emit(context, ",");
		Emit(context, vecType->elementCount);
		Emit(context, "> ");

		EmitDeclarator(context, declarator);
		return;
	}
	else if (auto matType = type->As<MatrixExpressionType>())
	{
		// TODO(tfoley): should really emit these with sugar
		Emit(context, "matrix<");
		EmitType(context, matType->elementType);
		Emit(context, ",");
		Emit(context, matType->rowCount);
		Emit(context, ",");
		Emit(context, matType->colCount);
		Emit(context, "> ");

		EmitDeclarator(context, declarator);
		return;
	}
	else if (auto texType = type->As<TextureType>())
	{
		switch (texType->GetBaseShape())
		{
		case TextureType::Shape1D:		Emit(context, "Texture1D");		break;
		case TextureType::Shape2D:		Emit(context, "Texture2D");		break;
		case TextureType::Shape3D:		Emit(context, "Texture3D");		break;
		case TextureType::ShapeCube:	Emit(context, "TextureCube");	break;
		default:
			assert(!"unreachable");
			break;
		}

		if (texType->isMultisample())
		{
			Emit(context, "MS");
		}
		if (texType->isArray())
		{
			Emit(context, "Array");
		}
		Emit(context, "<");
		EmitType(context, texType->elementType);
		Emit(context, "> ");

		EmitDeclarator(context, declarator);
		return;
	}
	else if (auto samplerStateType = type->As<SamplerStateType>())
	{
		switch (samplerStateType->flavor)
		{
		case SamplerStateType::Flavor::SamplerState:			Emit(context, "SamplerState");				break;
		case SamplerStateType::Flavor::SamplerComparisonState:	Emit(context, "SamplerComparisonState");	break;
		default:
			assert(!"unreachable");
			break;
		}

		EmitDeclarator(context, declarator);
		return;
	}
	else if (auto declRefType = type->As<DeclRefType>())
	{
		EmitDeclRef(context,  declRefType->declRef);

		EmitDeclarator(context, declarator);
		return;
	}

	throw "unimplemented";
}

static void EmitType(EmitContext* context, RefPtr<ExpressionType> type, String const& name)
{
	EDeclarator nameDeclarator;
	nameDeclarator.kind = EDeclarator::Kind::Name;
	nameDeclarator.name = name;
	EmitType(context, type, &nameDeclarator);
}

static void EmitType(EmitContext* context, RefPtr<ExpressionType> type)
{
	EmitType(context, type, nullptr);
}

// Statements

static void EmitBlockStmt(EmitContext* context, RefPtr<BlockStatementSyntaxNode> stmt)
{
	// TODO(tfoley): support indenting
	Emit(context, "{\n");
	for (auto s : stmt->Statements)
	{
		EmitStmt(context, s);
	}
	Emit(context, "}\n");
}

static void EmitStmt(EmitContext* context, RefPtr<StatementSyntaxNode> stmt)
{
	if (auto blockStmt = stmt.As<BlockStatementSyntaxNode>())
	{
		EmitBlockStmt(context, blockStmt);
		return;
	}
	else if (auto exprStmt = stmt.As<ExpressionStatementSyntaxNode>())
	{
		EmitExpr(context, exprStmt->Expression);
		Emit(context, ";\n");
		return;
	}
	else if (auto returnStmt = stmt.As<ReturnStatementSyntaxNode>())
	{
		Emit(context, "return");
		if (auto expr = returnStmt->Expression)
		{
			Emit(context, " ");
			EmitExpr(context, expr);
		}
		Emit(context, ";\n");
		return;
	}
	else if (auto declStmt = stmt.As<VarDeclrStatementSyntaxNode>())
	{
		EmitDecl(context, declStmt->decl);
		return;
	}

	throw "unimplemented";

}

// Declaration References

static void EmitVal(EmitContext* context, RefPtr<Val> val)
{
	if (auto type = val.As<ExpressionType>())
	{
		EmitType(context, type);
	}
	else if (auto intVal = val.As<IntVal>())
	{
		Emit(context, intVal->value);
	}
	else
	{
		assert(!"unimplemented");
	}
}

static void EmitDeclRef(EmitContext* context, DeclRef declRef)
{
	// TODO: need to qualify a declaration name based on parent scopes/declarations

	// Emit the name for the declaration itself
	Emit(context, declRef.GetName());

	// If the declaration is nested directly in a generic, then
	// we need to output the generic arguments here
	auto parentDeclRef = declRef.GetParent();
	if (auto genericDeclRef = parentDeclRef.As<GenericDeclRef>())
	{
		Substitutions* subst = declRef.substitutions.Ptr();
		Emit(context, "<");
		int argCount = subst->args.Count();
		for (int aa = 0; aa < argCount; ++aa)
		{
			if (aa != 0) Emit(context, ",");
			EmitVal(context, subst->args[aa]);
		}
		Emit(context, ">");
	}

}

// Declarations

static void EmitSemantic(EmitContext* context, RefPtr<HLSLSemantic> semantic)
{
	if (auto simple = semantic.As<HLSLSimpleSemantic>())
	{
		Emit(context, ": ");
		Emit(context, simple->name.Content);
	}
	else
	{
		assert(!"unimplemented");
	}
}


static void EmitSemantics(EmitContext* context, RefPtr<Decl> decl)
{
	for (auto mod = decl->modifiers.first; mod; mod = mod->next)
	{
		auto semantic = mod.As<HLSLSemantic>();
		if (!semantic)
			continue;

		EmitSemantic(context, semantic);
	}
}

static void EmitDeclsInContainer(EmitContext* context, RefPtr<ContainerDecl> container)
{
	for (auto member : container->Members)
	{
		EmitDecl(context, member);
	}
}

static void EmitTypeDefDecl(EmitContext* context, RefPtr<TypeDefDecl> decl)
{
	// TODO(tfoley): check if current compilation target even supports typedefs

	Emit(context, "typedef ");
	EmitType(context, decl->Type, decl->Name.Content);
	Emit(context, ";\n");
}

static void EmitStructDecl(EmitContext* context, RefPtr<StructSyntaxNode> decl)
{
	Emit(context, "struct ");
	Emit(context, decl->Name.Content);
	Emit(context, "\n{\n");

	// TODO(tfoley): Need to hoist members functions, etc. out to global scope
	EmitDeclsInContainer(context, decl);

	Emit(context, "};\n");
}

static void EmitConstantBufferDecl(
	EmitContext*				context,
	RefPtr<VarDeclBase>			varDecl,
	RefPtr<ConstantBufferType>	cbufferType)
{
	// The data type that describes where stuff in the constant buffer should go
	RefPtr<ExpressionType> dataType = cbufferType->elementType;

	// We expect/require the data type to be a user-defined `struct` type
	if (auto declRefType = dataType->As<DeclRefType>())
	{
		Emit(context, "cbuffer ");
		Emit(context, declRefType->declRef.GetName());
		// TODO: semantics
		Emit(context, "\n{\n");
		if (auto structRef = declRefType->declRef.As<StructDeclRef>())
		{
			for (auto field : structRef.GetMembersOfType<FieldDeclRef>())
			{
				EmitType(context, field.GetType(), field.GetName());
				// TODO: semantics
				Emit(context, ";\n");
			}
		}
		Emit(context, "}\n");
	}
	else
	{
		assert(!"unexpected");
	}
}

static void EmitVarDecl(EmitContext* context, RefPtr<VarDeclBase> decl)
{
	// As a special case, a variable using the `Constantbuffer<T>` type
	// should be translated into a `cbuffer` declaration if the target
	// requires it.
	//
	// TODO(tfoley): there might be a better way to detect this, e.g.,
	// with an attribute that gets attached to the variable declaration.
	if (auto cbufferType = decl->Type->As<ConstantBufferType>())
	{
		EmitConstantBufferDecl(context, decl, cbufferType);
		return;
	}


	EmitType(context, decl->Type, decl->Name.Content);

	if (auto initExpr = decl->Expr)
	{
		Emit(context, " = ");
		EmitExpr(context, initExpr);
	}
	Emit(context, ";\n");
}

static void EmitParamDecl(EmitContext* context, RefPtr<ParameterSyntaxNode> decl)
{
	if (decl->HasModifier(ModifierFlag::InOut))
	{
		Emit(context, "inout ");
	}
	else if (decl->HasModifier(ModifierFlag::Out))
	{
		Emit(context, "out ");
	}

	EmitType(context, decl->Type, decl->Name.Content);
	EmitSemantics(context, decl);

	// TODO(tfoley): handle case where parameter has a default value...
}

static void EmitFuncDecl(EmitContext* context, RefPtr<FunctionSyntaxNode> decl)
{
	// TODO: if a function returns an array type, or something similar that
	// isn't allowed by declarator syntax and/or language rules, we could
	// hypothetically wrap things in a `typedef` and work around it.

	EmitType(context, decl->ReturnType, decl->Name.Content);

	Emit(context, "(");
	bool first = true;
	for (auto paramDecl : decl->GetMembersOfType<ParameterSyntaxNode>())
	{
		if (!first) Emit(context, ", ");
		EmitParamDecl(context, paramDecl);
		first = false;
	}
	Emit(context, ")");

	EmitSemantics(context, decl);

	if (auto bodyStmt = decl->Body)
	{
		EmitBlockStmt(context, bodyStmt);
	}
	else
	{
		Emit(context, ";\n");
	}
}

static void EmitProgram(EmitContext* context, RefPtr<ProgramSyntaxNode> program)
{
	EmitDeclsInContainer(context, program);
}

static void EmitDecl(EmitContext* context, RefPtr<Decl> decl)
{
	// Don't emit code for declarations that came from the stdlib.
	//
	// TODO(tfoley): We probably need to relax this eventually,
	// since different targets might have different sets of builtins.
	if (decl->HasModifier(ModifierFlag::FromStdlib))
		return;

	if (auto typeDefDecl = decl.As<TypeDefDecl>())
	{
		EmitTypeDefDecl(context, typeDefDecl);
		return;
	}
	else if (auto structDecl = decl.As<StructSyntaxNode>())
	{
		EmitStructDecl(context, structDecl);
		return;
	}
	else if (auto varDecl = decl.As<VarDeclBase>())
	{
		EmitVarDecl(context, varDecl);
		return;
	}
	else if (auto funcDecl = decl.As<FunctionSyntaxNode>())
	{
		EmitFuncDecl(context, funcDecl);
		return;
	}
	else if (auto genericDecl = decl.As<GenericDecl>())
	{
		// Don't emit generic decls directly; we will only
		// ever emit particular instantiations of them.
		return;
	}

	throw "unimplemented";
}

String EmitProgram(ProgramSyntaxNode* program)
{
	EmitContext context;

	// TODO(tfoley): only emit symbols on-demand, as needed by a particular entry point

	EmitProgram(&context, program);

	String code = context.sb.ProduceString();

	return code;

#if 0
	// HACK(tfoley): Invoke the D3D HLSL compiler on the result, to validate it

#ifdef _WIN32
	{
		HMODULE d3dCompiler = LoadLibraryA("d3dcompiler_47");
		assert(d3dCompiler);

		pD3DCompile D3DCompile_ = (pD3DCompile)GetProcAddress(d3dCompiler, "D3DCompile");
		assert(D3DCompile_);

		ID3DBlob* codeBlob;
		ID3DBlob* diagnosticsBlob;
		HRESULT hr = D3DCompile_(
			code.begin(),
			code.Length(),
			"spire",
			nullptr,
			nullptr,
			"main",
			"ps_5_0",
			0,
			0,
			&codeBlob,
			&diagnosticsBlob);
		if (codeBlob) codeBlob->Release();
		if (diagnosticsBlob)
		{
			String diagnostics = (char const*) diagnosticsBlob->GetBufferPointer();
			fprintf(stderr, "%s", diagnostics.begin());
			OutputDebugStringA(diagnostics.begin());
			diagnosticsBlob->Release();
		}
		if (FAILED(hr))
		{
			int f = 9;
		}
	}

	#include <d3dcompiler.h>
#endif
#endif

}


}} // Spire::Compiler
