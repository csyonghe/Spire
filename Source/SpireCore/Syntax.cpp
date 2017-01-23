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
			return basicType->BaseType == BaseType;
		}

        ExpressionType* BasicExpressionType::CreateCanonicalType()
        {
            // A basic type is already canonical, in our setup
            return this;
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
			case Compiler::BaseType::Void:
				res.Append("void");
				break;
			case Compiler::BaseType::Error:
				res.Append("<errtype>");
				break;
			default:
				break;
			}
			return res.ProduceString();
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
			rs->ReturnType = ReturnType.Clone(ctx);
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
			if(IndexExpression)
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
			rs->Type = Type.Clone(ctx);
			rs->Expr = Expr->Clone(ctx);
			return rs;
		}
		RefPtr<SyntaxNode> ComponentSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitComponent(this);
		}
		ComponentSyntaxNode * ComponentSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ComponentSyntaxNode(*this), ctx);
			rs->Type = Type.Clone(ctx);
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

		// BasicExpressionType

		BasicExpressionType* BasicExpressionType::GetScalarType() const
		{
			return const_cast<BasicExpressionType*>(this);
		}

		bool BasicExpressionType::IsIntegralImpl() const
		{
			return (BaseType == Compiler::BaseType::Int || BaseType == Compiler::BaseType::UInt || BaseType == Compiler::BaseType::Bool);
		}

		//

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

		ArithmeticExpressionType * ExpressionType::AsArithmeticType() const
		{
			return GetCanonicalType()->AsArithmeticTypeImpl();
		}

        BasicExpressionType * ExpressionType::AsBasicType() const
        {
            return GetCanonicalType()->AsBasicTypeImpl();
        }

		VectorExpressionType * ExpressionType::AsVectorType() const
		{
			return GetCanonicalType()->AsVectorTypeImpl();
		}

		MatrixExpressionType * ExpressionType::AsMatrixType() const
		{
			return GetCanonicalType()->AsMatrixTypeImpl();
		}



        ArrayExpressionType * ExpressionType::AsArrayType() const
        {
            return GetCanonicalType()->AsArrayTypeImpl();
        }

        GenericExpressionType * ExpressionType::AsGenericType() const
        {
            return GetCanonicalType()->AsGenericTypeImpl();
        }

		DeclRefType * ExpressionType::AsDeclRefType() const
		{
			return GetCanonicalType()->AsDeclRefTypeImpl();
		}

        NamedExpressionType* ExpressionType::AsNamedType() const
        {
            return AsNamedTypeImpl();
        }

		TypeExpressionType* ExpressionType::AsTypeType() const
		{
			return GetCanonicalType()->AsTypeTypeImpl();
		}

        ExpressionType* ExpressionType::GetCanonicalType() const
        {
			if (!this) return nullptr;
            ExpressionType* et = const_cast<ExpressionType*>(this);
            if (!et->canonicalType)
            {
                // TODO(tfoley): worry about thread safety here?
                et->canonicalType = et->CreateCanonicalType();
            }
            return et->canonicalType;
        }

		BindableResourceType ExpressionType::GetBindableResourceType() const
		{
			if (auto textureType = As<TextureType>())
				return BindableResourceType::Texture;
			else if (auto samplerType = As<SamplerStateType>())
				return BindableResourceType::Sampler;
			else if (auto genericType = As<GenericExpressionType>())
			{
				auto name = genericType->GenericTypeName;
				if (name == "StructuredBuffer" || name == "RWStructuredBuffer")
					return BindableResourceType::StorageBuffer;
				else if (name == "Uniform")
					return BindableResourceType::Buffer;
			}

			return BindableResourceType::NonBindable;
		}

		bool ExpressionType::IsTexture() const
		{
			return As<TextureType>() != nullptr;
		}
		bool ExpressionType::IsSampler() const
		{
			return As<SamplerStateType>() != nullptr;
		}
		bool ExpressionType::IsTextureOrSampler() const
		{
			return IsTexture() || IsSampler();
		}
		bool ExpressionType::IsStruct() const
		{
			auto declRefType = AsDeclRefType();
			if (!declRefType) return false;
			auto structDecl = dynamic_cast<StructSyntaxNode*>(declRefType->decl);
			if (!structDecl) return false;
			return true;
		}
		bool ExpressionType::IsShader() const
		{
			return this->As<ShaderType>() != nullptr;
		}

		RefPtr<ExpressionType> ExpressionType::Bool;
		RefPtr<ExpressionType> ExpressionType::UInt;
		RefPtr<ExpressionType> ExpressionType::Int;
		RefPtr<ExpressionType> ExpressionType::Float;
		RefPtr<ExpressionType> ExpressionType::Float2;
		RefPtr<ExpressionType> ExpressionType::Void;
		RefPtr<ExpressionType> ExpressionType::Error;
        List<RefPtr<ExpressionType>> ExpressionType::sCanonicalTypes;

		void ExpressionType::Init()
		{
			Bool = new BasicExpressionType(BaseType::Bool);
			UInt = new BasicExpressionType(BaseType::UInt);
			Int = new BasicExpressionType(BaseType::Int);

			RefPtr<BasicExpressionType> floatType = new BasicExpressionType(BaseType::Float);
			Float = floatType;
			Float2 = new VectorExpressionType(floatType, 2);
			Void = new BasicExpressionType(BaseType::Void);
			Error = new BasicExpressionType(BaseType::Error);
		}
		void ExpressionType::Finalize()
		{
			Bool = nullptr;
			UInt = nullptr;
			Int = nullptr;
			Float = nullptr;
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

		CoreLib::Basic::String GenericExpressionType::ToString() const
		{
			return GenericTypeName + "<" + BaseType->ToString() + ">";
		}

		// DeclRefType

		String DeclRefType::ToString() const
		{
			return decl->Name.Content;
		}

		bool DeclRefType::EqualsImpl(const ExpressionType * type) const
		{
			if (auto declRefType = type->AsDeclRefType())
			{
				return decl == declRefType->decl;
			}
			return false;
		}

		DeclRefType * DeclRefType::AsDeclRefTypeImpl() const
		{
			return const_cast<DeclRefType*>(this);
		}

		ExpressionType* DeclRefType::CreateCanonicalType()
		{
			// A declaration reference is already canonical
			return this;
		}

        // NamedExpressionType

        String NamedExpressionType::ToString() const
        {
            return decl->Name.Content;
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
            return decl->Type.type->GetCanonicalType();
        }

		// FuncType

		String FuncType::ToString() const
		{
			// TODO: a better approach than this
			if (Func)
				return Func->SyntaxNode->InternalName;
			else if (Component)
				return Component->Name;
			else
				return "/* unknown FuncType */";
		}

		bool FuncType::EqualsImpl(const ExpressionType * type) const
		{
			if (auto funcType = type->As<FuncType>())
			{
				return Func == funcType->Func
					&& Component == funcType->Component;
			}
			return false;
		}

		ExpressionType* FuncType::CreateCanonicalType()
		{
			return this;
		}

		// ShaderType

		String ShaderType::ToString() const
		{
			return Shader->SyntaxNode->Name.Content;
		}

		bool ShaderType::EqualsImpl(const ExpressionType * type) const
		{
			if (auto shaderType = type->As<ShaderType>())
			{
				// TODO(tfoley): This does not compare the shader closure,
				// because the original implementation in `BasicExpressionType`
				// didn't either. It isn't clear whether that would be right or wrong.
				return Shader == shaderType->Shader;
			}
			return false;
		}

		ExpressionType* ShaderType::CreateCanonicalType()
		{
			return this;
		}

		// ImportOperatorGenericParamType

		String ImportOperatorGenericParamType::ToString() const
		{
			return GenericTypeVar;
		}

		bool ImportOperatorGenericParamType::EqualsImpl(const ExpressionType * type) const
		{
			if (auto genericType = type->As<ImportOperatorGenericParamType>())
			{
				// TODO(tfoley): This does not compare the shader closure,
				// because the original implementation in `BasicExpressionType`
				// didn't either. It isn't clear whether that would be right or wrong.
				return GenericTypeVar == genericType->GenericTypeVar;
			}
			return false;
		}

		ExpressionType* ImportOperatorGenericParamType::CreateCanonicalType()
		{
			return this;
		}

		// TypeExpressionType

		String TypeExpressionType::ToString() const
		{
			StringBuilder sb;
			sb << "typeof(" << type->ToString() << ")";
			return sb.ProduceString();
		}

		bool TypeExpressionType::EqualsImpl(const ExpressionType * t) const
		{
			if (auto typeType = t->AsTypeType())
			{
				return t->Equals(typeType->type);
			}
			return false;
		}

		TypeExpressionType * TypeExpressionType::AsTypeTypeImpl() const
		{
			return const_cast<TypeExpressionType*>(this);
		}

		ExpressionType* TypeExpressionType::CreateCanonicalType()
		{
			auto canType = new TypeExpressionType(type->GetCanonicalType());
			sCanonicalTypes.Add(canType);
			return canType;
		}

		// ArithmeticExpressionType

		ArithmeticExpressionType * ArithmeticExpressionType::AsArithmeticTypeImpl() const
		{
			return const_cast<ArithmeticExpressionType*>(this);
		}


		// VectorExpressionType

		String VectorExpressionType::ToString() const
		{
			StringBuilder sb;
			sb << "vector<" << elementType->ToString() << "," << elementCount << ">";
			return sb.ProduceString();
		}

		BasicExpressionType* VectorExpressionType::GetScalarType() const
		{
			return elementType->AsBasicType();
		}

		bool VectorExpressionType::EqualsImpl(const ExpressionType * type) const
		{
			if (auto vecType = type->AsVectorType())
			{
				return elementType->Equals(vecType->elementType)
					&& elementCount == vecType->elementCount;
			}
			
			return false;
		}

		VectorExpressionType * VectorExpressionType::AsVectorTypeImpl() const
		{
			return const_cast<VectorExpressionType*>(this);
		}

		ExpressionType* VectorExpressionType::CreateCanonicalType()
		{
			auto canElementType = elementType->GetCanonicalType();
			auto canType = new VectorExpressionType(canElementType, elementCount);
			sCanonicalTypes.Add(canType);
			return canType;
		}

		// MatrixExpressionType

		String MatrixExpressionType::ToString() const
		{
			StringBuilder sb;
			sb << "matrix<" << elementType->ToString() << "," << rowCount << "," << colCount << ">";
			return sb.ProduceString();
		}

		BasicExpressionType* MatrixExpressionType::GetScalarType() const
		{
			return elementType->AsBasicType();
		}

		bool MatrixExpressionType::EqualsImpl(const ExpressionType * type) const
		{
			if (auto matType = type->AsMatrixType())
			{
				return elementType->Equals(matType->elementType)
					&& rowCount == matType->rowCount
					&& colCount == matType->colCount;
			}
			
			return false;
		}
		MatrixExpressionType * MatrixExpressionType::AsMatrixTypeImpl() const
		{
			return const_cast<MatrixExpressionType*>(this);
		}

		ExpressionType* MatrixExpressionType::CreateCanonicalType()
		{
			auto canElementType = elementType->GetCanonicalType();
			auto canType = new MatrixExpressionType(canElementType, rowCount, colCount);
			sCanonicalTypes.Add(canType);
			return canType;
		}

		//

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

		// BufferDecl

		RefPtr<SyntaxNode> BufferDecl::Accept(SyntaxVisitor *visitor)
		{
			return visitor->VisitBufferDecl(this);
		}

		// HLSLConstantBufferDecl

		HLSLConstantBufferDecl * HLSLConstantBufferDecl::Clone(CloneContext & ctx)
		{
			assert(!"unimplemented");
			return nullptr;
		}

		// HLSLTextureBufferDecl

		HLSLTextureBufferDecl * HLSLTextureBufferDecl::Clone(CloneContext & ctx)
		{
			assert(!"unimplemented");
			return nullptr;
		}

		//


		RefPtr<ComponentSyntaxNode> SyntaxVisitor::VisitComponent(ComponentSyntaxNode * comp)
		{
			comp->Type = comp->Type.Accept(this);
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

		// TypeExp

		TypeExp TypeExp::Clone(CloneContext& context)
		{
			TypeExp result;
			if (exp)
				result.exp = exp->Clone(context);
			return result;
		}

		TypeExp TypeExp::Accept(SyntaxVisitor* visitor)
		{
			return visitor->VisitTypeExp(*this);
		}

		// BuiltinTypeModifier

		// MagicTypeModifier

		// GenericDecl

		RefPtr<SyntaxNode> GenericDecl::Accept(SyntaxVisitor * visitor) {
			throw "unimplemented";
		}

		GenericDecl * GenericDecl::Clone(CloneContext & ctx) {
			throw "unimplemented";
		}

		// GenericTypeParamDecl

		RefPtr<SyntaxNode> GenericTypeParamDecl::Accept(SyntaxVisitor * visitor) {
			throw "unimplemented";
		}

		GenericTypeParamDecl * GenericTypeParamDecl::Clone(CloneContext & ctx) {
			throw "unimplemented";
		}

		// GenericValueParamDecl

		RefPtr<SyntaxNode> GenericValueParamDecl::Accept(SyntaxVisitor * visitor) {
			throw "unimplemented";
		}

		GenericValueParamDecl * GenericValueParamDecl::Clone(CloneContext & ctx) {
			throw "unimplemented";
		}

		// VectorTypeDecl

		RefPtr<SyntaxNode> VectorTypeDecl::Accept(SyntaxVisitor * visitor) {
			throw "unimplemented";
		}

		VectorTypeDecl * VectorTypeDecl::Clone(CloneContext & ctx) {
			throw "unimplemented";
		}

	}
}