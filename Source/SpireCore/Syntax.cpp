#include "Syntax.h"
#include "SyntaxVisitors.h"
#include "SymbolTable.h"

#include <assert.h>

namespace Spire
{
	namespace Compiler
	{
        // Scope

        Decl* Scope::LookUp(String const& name)
        {
            Scope* scope = this;
            while (scope)
            {
                for (auto m : scope->containerDecl->Members)
                {
                    if (m->Name.Content == name)
                        return m.Ptr();
                }

                scope = scope->Parent.Ptr();
            }
            return nullptr;
        }

        // Decl

        bool Decl::FindSimpleAttribute(String const& key, Token& outValue)
        {
            for (auto attr : GetLayoutAttributes())
            {
                if (attr->Key == key)
                {
                    outValue = attr->Value;
                    return true;
                }
            }
            return false;
        }
        bool Decl::FindSimpleAttribute(String const& key, String& outValue)
        {
            for (auto attr : GetLayoutAttributes())
            {
                if (attr->Key == key)
                {
                    outValue = attr->Value.Content;
                    return true;
                }
            }
            return false;
        }
        bool Decl::HasSimpleAttribute(String const& key)
        {
            for (auto attr : GetLayoutAttributes())
            {
                if (attr->Key == key)
                {
                    return true;
                }
            }
            return false;
        }

		SpecializeModifier * Decl::FindSpecializeModifier()
		{
			auto list = GetModifiersOfType<SpecializeModifier>();
			if (list.begin() != list.end())
				return *list.begin();
			return nullptr;
		}

        //

		bool BasicExpressionType::EqualsImpl(const ExpressionType * type) const
		{
			auto basicType = dynamic_cast<const BasicExpressionType*>(type);
			if (basicType == nullptr)
				return false;
			return (basicType->BaseType == BaseType &&
				basicType->Func == Func &&
				basicType->Shader == Shader &&
				basicType->structDecl == structDecl &&
				basicType->RecordTypeName == RecordTypeName);
		}

        ExpressionType* BasicExpressionType::CreateCanonicalType()
        {
            // A basic type is already canonical, in our setup
            return this;
        }

		BindableResourceType BasicExpressionType::GetBindableResourceType() const
		{
			switch (BaseType)
			{
			case Compiler::BaseType::Texture2DArray:
			case Compiler::BaseType::Texture2DArrayShadow:
			case Compiler::BaseType::Texture2D:
			case Compiler::BaseType::Texture2DShadow:
			case Compiler::BaseType::Texture3D:
			case Compiler::BaseType::TextureCube:
			case Compiler::BaseType::TextureCubeShadow:
			case Compiler::BaseType::TextureCubeArray:
			case Compiler::BaseType::TextureCubeShadowArray:
				return BindableResourceType::Texture;
			case Compiler::BaseType::SamplerState:
			case Compiler::BaseType::SamplerComparisonState:
				return BindableResourceType::Sampler;
			}
			return BindableResourceType::NonBindable;
		}

		bool BasicExpressionType::IsVectorTypeImpl() const
		{
			return IsVector(BaseType);
		}

		CoreLib::Basic::String BasicExpressionType::ToString() const
		{
			CoreLib::Basic::StringBuilder res;

			switch (BaseType)
			{
			case Compiler::BaseType::Int:
				res.Append("int");
				break;
			case Compiler::BaseType::UInt:
				res.Append("uint");
				break;
			case Compiler::BaseType::Bool:
				res.Append("bool");
				break;
			case Compiler::BaseType::Float:
				res.Append("float");
				break;
			case Compiler::BaseType::Int2:
				res.Append("ivec2");
				break;
			case Compiler::BaseType::UInt2:
				res.Append("uvec2");
				break;
			case Compiler::BaseType::Float2:
				res.Append("vec2");
				break;
			case Compiler::BaseType::Int3:
				res.Append("ivec3");
				break;
			case Compiler::BaseType::UInt3:
				res.Append("uvec3");
				break;
			case Compiler::BaseType::Float3:
				res.Append("vec3");
				break;
			case Compiler::BaseType::Int4:
				res.Append("ivec4");
				break;
			case Compiler::BaseType::UInt4:
				res.Append("uvec4");
				break;
			case Compiler::BaseType::Float4:
				res.Append("vec4");
				break;
			case Compiler::BaseType::Float3x3:
				res.Append("mat3");
				break;
			case Compiler::BaseType::Float4x4:
				res.Append("mat4");
				break;
			case Compiler::BaseType::Texture2DArray:
				res.Append("sampler2DArray");
				break;
			case Compiler::BaseType::Texture2DArrayShadow:
				res.Append("sampler2DArrayShadow");
				break;
			case Compiler::BaseType::Texture2D:
				res.Append("sampler2D");
				break;
			case Compiler::BaseType::Texture2DShadow:
				res.Append("sampler2DShadow");
				break;
			case Compiler::BaseType::Texture3D:
				res.Append("sampler3D");
				break;
			case Compiler::BaseType::TextureCube:
				res.Append("samplerCube");
				break;
			case Compiler::BaseType::TextureCubeShadow:
				res.Append("samplerCubeShadow");
				break;
			case Compiler::BaseType::Function:
				res.Append(Func->SyntaxNode->InternalName);
				break;
			case Compiler::BaseType::Shader:
				res.Append(Shader->SyntaxNode->Name.Content);
				break;
			case Compiler::BaseType::Void:
				res.Append("void");
				break;
			case Compiler::BaseType::Struct:
				res.Append(structDecl->Name.Content);
				break;
			case Compiler::BaseType::Record:
				res.Append(RecordTypeName);
				break;
			case Compiler::BaseType::SamplerState:
				res.Append("SamplerState");
				break;
			case Compiler::BaseType::Error:
				res.Append("<errtype>");
				break;
			default:
				break;
			}
			return res.ProduceString();
		}

		ExpressionType * BasicExpressionType::Clone()
		{
			BasicExpressionType * rs = new BasicExpressionType(*this);
			return rs;
		}


		RefPtr<SyntaxNode> ProgramSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitProgram(this);
		}
		ProgramSyntaxNode * ProgramSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ProgramSyntaxNode(*this), ctx);
			rs->Members.Clear();
			for (auto & m : Members)
				rs->Members.Add(m->Clone(ctx));
			return rs;
		}
		RefPtr<SyntaxNode> FunctionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitFunction(this);
		}
		FunctionSyntaxNode * FunctionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new FunctionSyntaxNode(*this), ctx);
			for (auto & member : rs->Members)
			{
				member = member->Clone(ctx);
			}
			rs->ReturnTypeNode = ReturnTypeNode->Clone(ctx);
			rs->Body = Body->Clone(ctx);
			return rs;
		}

        //

        RefPtr<SyntaxNode> ScopeDecl::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitScopeDecl(this);
        }

        ScopeDecl* ScopeDecl::Clone(CloneContext & ctx)
        {
            auto rs = CloneSyntaxNodeFields(new ScopeDecl(*this), ctx);
            for (auto & member : rs->Members)
            {
                member = member->Clone(ctx);
            }
            return rs;
        }


        //

		RefPtr<SyntaxNode> BlockStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitBlockStatement(this);
		}
		BlockStatementSyntaxNode * BlockStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new BlockStatementSyntaxNode(*this), ctx);
			rs->Statements.Clear();
			for (auto & stmt : Statements)
			{
				rs->Statements.Add(stmt->Clone(ctx));
			}
			return rs;
		}
		RefPtr<SyntaxNode> BreakStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitBreakStatement(this);
		}
		BreakStatementSyntaxNode * BreakStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new BreakStatementSyntaxNode(*this), ctx);
		}
		RefPtr<SyntaxNode> ContinueStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitContinueStatement(this);
		}
		ContinueStatementSyntaxNode * ContinueStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new ContinueStatementSyntaxNode(*this), ctx);
		}
		RefPtr<SyntaxNode> DoWhileStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitDoWhileStatement(this);
		}
		DoWhileStatementSyntaxNode * DoWhileStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new DoWhileStatementSyntaxNode(*this), ctx);
			if (Predicate)
				rs->Predicate = Predicate->Clone(ctx);
			if (Statement)
				rs->Statement = Statement->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> EmptyStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitEmptyStatement(this);
		}
		EmptyStatementSyntaxNode * EmptyStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new EmptyStatementSyntaxNode(*this), ctx);
		}
		RefPtr<SyntaxNode> ForStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitForStatement(this);
		}
		ForStatementSyntaxNode * ForStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ForStatementSyntaxNode(*this), ctx);
			if (InitialStatement)
				rs->InitialStatement = InitialStatement->Clone(ctx);
			if (SideEffectExpression)
				rs->SideEffectExpression = SideEffectExpression->Clone(ctx);
			if (PredicateExpression)
				rs->PredicateExpression = PredicateExpression->Clone(ctx);
			if (Statement)
				rs->Statement = Statement->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> IfStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitIfStatement(this);
		}
		IfStatementSyntaxNode * IfStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new IfStatementSyntaxNode(*this), ctx);
			if (Predicate)
				rs->Predicate = Predicate->Clone(ctx);
			if (PositiveStatement)
				rs->PositiveStatement = PositiveStatement->Clone(ctx);
			if (NegativeStatement)
				rs->NegativeStatement = NegativeStatement->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> ReturnStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitReturnStatement(this);
		}
		ReturnStatementSyntaxNode * ReturnStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ReturnStatementSyntaxNode(*this), ctx);
			if (Expression)
				rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> VarDeclrStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitVarDeclrStatement(this);
		}
		VarDeclrStatementSyntaxNode * VarDeclrStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new VarDeclrStatementSyntaxNode(*this), ctx);
            rs->decl = rs->decl->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> Variable::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitDeclrVariable(this);
		}
		Variable * Variable::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new Variable(*this), ctx);
			if (Expr)
				rs->Expr = Expr->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> WhileStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitWhileStatement(this);
		}
		WhileStatementSyntaxNode * WhileStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new WhileStatementSyntaxNode(*this), ctx);
			if (Predicate)
				rs->Predicate = Predicate->Clone(ctx);
			if (Statement)
				rs->Statement = Statement->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> ExpressionStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitExpressionStatement(this);
		}
		ExpressionStatementSyntaxNode * ExpressionStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ExpressionStatementSyntaxNode(*this), ctx);
			if (Expression)
				rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> BinaryExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitBinaryExpression(this);
		}
		BinaryExpressionSyntaxNode * BinaryExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new BinaryExpressionSyntaxNode(*this), ctx);
			rs->LeftExpression = LeftExpression->Clone(ctx);
			rs->RightExpression = RightExpression->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> ConstantExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitConstantExpression(this);
		}
		ConstantExpressionSyntaxNode * ConstantExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new ConstantExpressionSyntaxNode(*this), ctx);
		}
		IndexExpressionSyntaxNode * IndexExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new IndexExpressionSyntaxNode(*this), ctx);
			rs->BaseExpression = BaseExpression->Clone(ctx);
			rs->IndexExpression = IndexExpression->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> IndexExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitIndexExpression(this);
		}
		RefPtr<SyntaxNode> MemberExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitMemberExpression(this);
		}
		MemberExpressionSyntaxNode * MemberExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new MemberExpressionSyntaxNode(*this), ctx);
			rs->BaseExpression = BaseExpression->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> InvokeExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitInvokeExpression(this);
		}
		InvokeExpressionSyntaxNode * InvokeExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new InvokeExpressionSyntaxNode(*this), ctx);
			rs->FunctionExpr = FunctionExpr->Clone(ctx);
			rs->Arguments.Clear();
			for (auto & arg : Arguments)
			{
				rs->Arguments.Add(arg->Clone(ctx));
			}
			return rs;
		}
		RefPtr<SyntaxNode> TypeCastExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitTypeCastExpression(this);
		}
		TypeCastExpressionSyntaxNode * TypeCastExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new TypeCastExpressionSyntaxNode(*this), ctx);
			rs->TargetType = TargetType->Clone(ctx);
			rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> SelectExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitSelectExpression(this);
		}
		SelectExpressionSyntaxNode * SelectExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new SelectExpressionSyntaxNode(*this), ctx);
			rs->SelectorExpr = SelectorExpr->Clone(ctx);
			rs->Expr0 = Expr0->Clone(ctx);
			rs->Expr1 = Expr1->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> UnaryExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitUnaryExpression(this);
		}
		UnaryExpressionSyntaxNode * UnaryExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new UnaryExpressionSyntaxNode(*this), ctx);
			rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> VarExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitVarExpression(this);
		}
		VarExpressionSyntaxNode * VarExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new VarExpressionSyntaxNode(*this), ctx);
		}
		RefPtr<SyntaxNode> ParameterSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitParameter(this);
		}
		ParameterSyntaxNode * ParameterSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ParameterSyntaxNode(*this), ctx);
			rs->TypeNode = TypeNode->Clone(ctx);
			rs->Expr = Expr->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> BasicTypeSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitBasicType(this);
		}
		RefPtr<SyntaxNode> ComponentSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitComponent(this);
		}
		ComponentSyntaxNode * ComponentSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ComponentSyntaxNode(*this), ctx);
			rs->TypeNode = TypeNode->Clone(ctx);
			if (Rate)
				rs->Rate = Rate->Clone(ctx);
			if (BlockStatement)
				rs->BlockStatement = BlockStatement->Clone(ctx);
			if (Expression)
				rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> ShaderSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitShader(this);
		}
		ShaderSyntaxNode * ShaderSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ShaderSyntaxNode(*this), ctx);
			rs->Members.Clear();
			for (auto & comp : Members)
				rs->Members.Add(comp->Clone(ctx));
			return rs;
		}

        // UsingFileDecl

        RefPtr<SyntaxNode> UsingFileDecl::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitUsingFileDecl(this);
        }

        UsingFileDecl* UsingFileDecl::Clone(CloneContext & ctx)
        {
            return CloneSyntaxNodeFields(new UsingFileDecl(*this), ctx);
        }

        //

		RateSyntaxNode * RateSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new RateSyntaxNode(*this), ctx);
		}
		WorldSyntaxNode * WorldSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new WorldSyntaxNode(*this), ctx);
		}
		RefPtr<SyntaxNode> ImportOperatorDefSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitImportOperatorDef(this); 
		}
		ImportOperatorDefSyntaxNode * ImportOperatorDefSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new ImportOperatorDefSyntaxNode(*this), ctx);
		}
		PipelineSyntaxNode * PipelineSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new PipelineSyntaxNode(*this), ctx);
			rs->Members.Clear();
			for (auto & m : Members)
				rs->Members.Add(m->Clone(ctx));
			return rs;
		}
		ChoiceValueSyntaxNode * ChoiceValueSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new ChoiceValueSyntaxNode(*this), ctx);
		}
		RefPtr<SyntaxNode> ImportSyntaxNode::Accept(SyntaxVisitor * v)
		{
			return v->VisitImport(this);
		}
		ImportSyntaxNode * ImportSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ImportSyntaxNode(*this), ctx);
			rs->Arguments.Clear();
			for (auto & arg : Arguments)
				rs->Arguments.Add(arg->Clone(ctx));
			return rs;
		}
		RefPtr<SyntaxNode> ImportArgumentSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitImportArgument(this);
		}
		ImportArgumentSyntaxNode * ImportArgumentSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ImportArgumentSyntaxNode(*this), ctx);
			rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> ImportStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitImportStatement(this);
		}
		ImportStatementSyntaxNode * ImportStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ImportStatementSyntaxNode(*this), ctx);
			rs->Import = Import->Clone(ctx);
			return rs;
		}

		RefPtr<SyntaxNode> StructField::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitStructField(this);
		}
		RefPtr<SyntaxNode> StructSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitStruct(this);
		}

        RefPtr<SyntaxNode> TypeDefDecl::Accept(SyntaxVisitor * visitor)
        {
            return visitor->VisitTypeDefDecl(this);
        }
        TypeDefDecl* TypeDefDecl::Clone(CloneContext & ctx)
        {
            auto result = CloneSyntaxNodeFields(new TypeDefDecl(*this), ctx);
            return result;
        }

		RefPtr<SyntaxNode> DiscardStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitDiscardStatement(this);
		}
		DiscardStatementSyntaxNode * DiscardStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new DiscardStatementSyntaxNode(*this), ctx);
			return rs;
		}
		bool BasicExpressionType::IsIntegralImpl() const
		{
			return (BaseType == Compiler::BaseType::Int || BaseType == Compiler::BaseType::UInt || BaseType == Compiler::BaseType::Bool);
		}

        bool ExpressionType::IsIntegral() const
        {
            return GetCanonicalType()->IsIntegralImpl();
        }

        bool ExpressionType::Equals(const ExpressionType * type) const
        {
            return GetCanonicalType()->EqualsImpl(type->GetCanonicalType());
        }

		bool ExpressionType::Equals(RefPtr<ExpressionType> type) const
		{
			return Equals(type.Ptr());
		}

        bool ExpressionType::IsVectorType() const
        {
            return GetCanonicalType()->IsVectorTypeImpl();
        }

        bool ExpressionType::IsArray() const
        {
            return GetCanonicalType()->IsArrayImpl();
        }

        bool ExpressionType::IsGenericType(String typeName) const
        {
            return GetCanonicalType()->IsGenericTypeImpl(typeName);
        }

        BasicExpressionType * ExpressionType::AsBasicType() const
        {
            return GetCanonicalType()->AsBasicTypeImpl();
        }

        ArrayExpressionType * ExpressionType::AsArrayType() const
        {
            return GetCanonicalType()->AsArrayTypeImpl();
        }

        GenericExpressionType * ExpressionType::AsGenericType() const
        {
            return GetCanonicalType()->AsGenericTypeImpl();
        }

        NamedExpressionType* ExpressionType::AsNamedType() const
        {
            return AsNamedTypeImpl();
        }

        ExpressionType* ExpressionType::GetCanonicalType() const
        {
            ExpressionType* et = const_cast<ExpressionType*>(this);
            if (!et->canonicalType)
            {
                // TODO(tfoley): worry about thread safety here?
                et->canonicalType = et->CreateCanonicalType();
            }
            return et->canonicalType;
        }

		bool ExpressionType::IsTexture() const
		{
			auto basicType = AsBasicType();
			if (basicType)
				return basicType->BaseType == BaseType::Texture2D ||
				basicType->BaseType == BaseType::TextureCube ||
				basicType->BaseType == BaseType::Texture2DArray ||
				basicType->BaseType == BaseType::Texture2DShadow ||
				basicType->BaseType == BaseType::TextureCubeShadow ||
				basicType->BaseType == BaseType::Texture2DArrayShadow ||
				basicType->BaseType == BaseType::TextureCubeArray ||
				basicType->BaseType == BaseType::TextureCubeShadowArray ||
				basicType->BaseType == BaseType::Texture3D;
			return false;
		}
		bool ExpressionType::IsTextureOrSampler() const
		{
			auto basicType = AsBasicType();
			if (basicType)
				return basicType->BaseType == BaseType::Texture2D ||
					basicType->BaseType == BaseType::TextureCube ||
					basicType->BaseType == BaseType::Texture2DArray ||
					basicType->BaseType == BaseType::Texture2DShadow ||
					basicType->BaseType == BaseType::TextureCubeShadow ||
					basicType->BaseType == BaseType::Texture2DArrayShadow ||
					basicType->BaseType == BaseType::Texture3D ||
					basicType->BaseType == BaseType::TextureCubeArray ||
					basicType->BaseType == BaseType::TextureCubeShadowArray ||
					basicType->BaseType == BaseType::SamplerState;
			return false;
		}
		bool ExpressionType::IsStruct() const
		{
			auto basicType = AsBasicType();
			if (basicType)
				return basicType->structDecl != nullptr;
			return false;
		}
		bool ExpressionType::IsShader() const
		{
			auto basicType = AsBasicType();
			if (basicType)
				return basicType->Shader != nullptr;
			return false;
		}

		RefPtr<ExpressionType> ExpressionType::Bool;
		RefPtr<ExpressionType> ExpressionType::UInt;
		RefPtr<ExpressionType> ExpressionType::UInt2;
		RefPtr<ExpressionType> ExpressionType::UInt3;
		RefPtr<ExpressionType> ExpressionType::UInt4;
		RefPtr<ExpressionType> ExpressionType::Int;
		RefPtr<ExpressionType> ExpressionType::Int2;
		RefPtr<ExpressionType> ExpressionType::Int3;
		RefPtr<ExpressionType> ExpressionType::Int4;
		RefPtr<ExpressionType> ExpressionType::Float;
		RefPtr<ExpressionType> ExpressionType::Float2;
		RefPtr<ExpressionType> ExpressionType::Float3;
		RefPtr<ExpressionType> ExpressionType::Float4;
		RefPtr<ExpressionType> ExpressionType::Void;
		RefPtr<ExpressionType> ExpressionType::Error;
        List<RefPtr<ExpressionType>> ExpressionType::sCanonicalTypes;

		void ExpressionType::Init()
		{
			Bool = new BasicExpressionType(BaseType::Bool);
			UInt = new BasicExpressionType(BaseType::UInt);
			UInt2 = new BasicExpressionType(BaseType::UInt2);
			UInt3 = new BasicExpressionType(BaseType::UInt3);
			UInt4 = new BasicExpressionType(BaseType::UInt4);
			Int = new BasicExpressionType(BaseType::Int);
			Int2 = new BasicExpressionType(BaseType::Int2);
			Int3 = new BasicExpressionType(BaseType::Int3);
			Int4 = new BasicExpressionType(BaseType::Int4);
			Float = new BasicExpressionType(BaseType::Float);
			Float2 = new BasicExpressionType(BaseType::Float2);
			Float3 = new BasicExpressionType(BaseType::Float3);
			Float4 = new BasicExpressionType(BaseType::Float4);
			Void = new BasicExpressionType(BaseType::Void);
			Error = new BasicExpressionType(BaseType::Error);
		}
		void ExpressionType::Finalize()
		{
			Bool = nullptr;
			UInt = nullptr;
			UInt2 = nullptr;
			UInt3 = nullptr;
			UInt4 = nullptr;
			Int = nullptr;
			Int2 = nullptr;
			Int3 = nullptr;
			Int4 = nullptr;
			Float = nullptr;
			Float2 = nullptr;
			Float3 = nullptr;
			Float4 = nullptr;
			Void = nullptr;
			Error = nullptr;
            // Note(tfoley): This seems to be just about the only way to clear out a List<T>
            sCanonicalTypes = List<RefPtr<ExpressionType>>();
		}
		bool ArrayExpressionType::IsArrayImpl() const
		{
			return true;
		}
		bool ArrayExpressionType::EqualsImpl(const ExpressionType * type) const
		{
			auto arrType = type->AsArrayType();
			if (!arrType)
				return false;
			return (ArrayLength == arrType->ArrayLength && BaseType->Equals(arrType->BaseType.Ptr()));
		}
        ExpressionType* ArrayExpressionType::CreateCanonicalType()
        {
            auto canonicalBaseType = BaseType->GetCanonicalType();
            auto canonicalArrayType = new ArrayExpressionType();
            sCanonicalTypes.Add(canonicalArrayType);
            canonicalArrayType->BaseType = canonicalBaseType;
            canonicalArrayType->ArrayLength = ArrayLength;
            return canonicalArrayType;
        }
		CoreLib::Basic::String ArrayExpressionType::ToString() const
		{
			if (ArrayLength > 0)
				return BaseType->ToString() + "[" + String(ArrayLength) + "]";
			else
				return BaseType->ToString() + "[]";
		}
		ExpressionType * ArrayExpressionType::Clone()
		{
			auto rs = new ArrayExpressionType(*this);
			rs->BaseType = BaseType->Clone();
			return rs;
		}
		RefPtr<SyntaxNode> ArrayTypeSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitArrayType(this);
		}
		RefPtr<SyntaxNode> GenericTypeSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitGenericType(this);
		}
		bool GenericExpressionType::EqualsImpl(const ExpressionType * type) const
		{
			if (auto gtype = type->AsGenericType())
				return GenericTypeName == gtype->GenericTypeName && gtype->BaseType->Equals(BaseType.Ptr());
			
			return false;
		}
        ExpressionType* GenericExpressionType::CreateCanonicalType()
        {
            auto canonicalBaseType = BaseType->GetCanonicalType();
            auto canonicalGenericType = new GenericExpressionType();
            sCanonicalTypes.Add(canonicalGenericType);
            canonicalGenericType->BaseType = canonicalBaseType;
            canonicalGenericType->GenericTypeName = GenericTypeName;
            return canonicalGenericType;
        }
		BindableResourceType GenericExpressionType::GetBindableResourceType() const
		{
			if (GenericTypeName == "StructuredBuffer" || GenericTypeName == "RWStructuredBuffer")
				return BindableResourceType::StorageBuffer;
			else if (GenericTypeName == "Uniform")
				return BindableResourceType::Buffer;
			return BindableResourceType::NonBindable;
		}
		CoreLib::Basic::String GenericExpressionType::ToString() const
		{
			return GenericTypeName + "<" + BaseType->ToString() + ">";
		}
		ExpressionType * GenericExpressionType::Clone()
		{
			auto rs = new GenericExpressionType(*this);
			rs->BaseType = BaseType->Clone();
			return rs;
		}

        // NamedExpressionType

        String NamedExpressionType::ToString() const
        {
            return decl->Name.Content;
        }

        ExpressionType * NamedExpressionType::Clone()
        {
            NamedExpressionType* result = new NamedExpressionType();
            result->decl = decl;
            return result;
        }

		BindableResourceType NamedExpressionType::GetBindableResourceType() const
		{
			return GetCanonicalType()->GetBindableResourceType();
		}

        bool NamedExpressionType::EqualsImpl(const ExpressionType * /*type*/) const
        {
            assert(!"unreachable");
            return false;
        }

        NamedExpressionType * NamedExpressionType::AsNamedTypeImpl() const
        {
            return const_cast<NamedExpressionType*>(this);
        }

        ExpressionType* NamedExpressionType::CreateCanonicalType()
        {
            return decl->Type->GetCanonicalType();
        }

		RefPtr<SyntaxNode> ImportExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitImportExpression(this);
		}
		ImportExpressionSyntaxNode * ImportExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			ImportExpressionSyntaxNode * result = new ImportExpressionSyntaxNode(*this);
			CloneSyntaxNodeFields(result, ctx);
			result->Component = Component->Clone(ctx);
			result->Arguments.Clear();
			for (auto & arg : Arguments)
				result->Arguments.Add(arg->Clone(ctx));
			return result;
		}
		StageSyntaxNode * StageSyntaxNode::Clone(CloneContext &)
		{
			return new StageSyntaxNode(*this);
		}
		RefPtr<ComponentSyntaxNode> SyntaxVisitor::VisitComponent(ComponentSyntaxNode * comp)
		{
			if (comp->TypeNode)
				comp->TypeNode = comp->TypeNode->Accept(this).As<TypeSyntaxNode>();
			if (comp->Expression)
				comp->Expression = comp->Expression->Accept(this).As<ExpressionSyntaxNode>();
			if (comp->BlockStatement)
				comp->BlockStatement = comp->BlockStatement->Accept(this).As<BlockStatementSyntaxNode>();
			return comp;
		}
		String GetOperatorFunctionName(Operator op)
		{
			switch (op)
			{
			case Operator::Add:
			case Operator::AddAssign:
				return "+";
			case Operator::Sub:
			case Operator::SubAssign:
				return "-";
			case Operator::Neg:
				return "-";
			case Operator::Not:
				return "!";
			case Operator::BitNot:
				return "~";
			case Operator::PreInc:
			case Operator::PostInc:
				return "++";
			case Operator::PreDec:
			case Operator::PostDec:
				return "--";
			case Operator::Mul:
			case Operator::MulAssign:
				return "*";
			case Operator::Div:
			case Operator::DivAssign:
				return "/";
			case Operator::Mod:
			case Operator::ModAssign:
				return "%";
			case Operator::Lsh:
			case Operator::LshAssign:
				return "<<";
			case Operator::Rsh:
			case Operator::RshAssign:
				return ">>";
			case Operator::Eql:
				return "==";
			case Operator::Neq:
				return "!=";
			case Operator::Greater:
				return ">";
			case Operator::Less:
				return "<";
			case Operator::Geq:
				return ">=";
			case Operator::Leq:
				return "<=";
			case Operator::BitAnd:
			case Operator::AndAssign:
				return "&";
			case Operator::BitXor:
			case Operator::XorAssign:
				return "^";
			case Operator::BitOr:
			case Operator::OrAssign:
				return "|";
			case Operator::And:
				return "&&";
			case Operator::Or:
				return "||";
			default:
				return "";
			}
		}
		RefPtr<SyntaxNode> ProjectExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitProject(this);
		}
		ProjectExpressionSyntaxNode * ProjectExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto * result = new ProjectExpressionSyntaxNode(*this);
			result->BaseExpression = BaseExpression->Clone(ctx);
			return result;
		}
		RefPtr<SyntaxNode> InterfaceSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitInterface(this);
		}
		InterfaceSyntaxNode * InterfaceSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new InterfaceSyntaxNode(*this), ctx);
			rs->Members.Clear();
			for (auto & comp : Members)
				rs->Members.Add(comp->Clone(ctx));
			return rs;
		}
		RefPtr<SyntaxNode> TemplateShaderSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitTemplateShader(this);
		}
		TemplateShaderSyntaxNode * TemplateShaderSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new TemplateShaderSyntaxNode(*this), ctx);
			rs->Parameters.Clear();
			for (auto & param : Parameters)
				rs->Parameters.Add(param->Clone(ctx));
			for (auto & member : Members)
				rs->Members.Add(member->Clone(ctx));
			return rs;
		}
		TemplateShaderParameterSyntaxNode * TemplateShaderParameterSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new TemplateShaderParameterSyntaxNode(*this), ctx);
			return rs;
		}
}
}