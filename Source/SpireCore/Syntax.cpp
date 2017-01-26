#include "Syntax.h"
#include "SyntaxVisitors.h"
#include "SymbolTable.h"

#include <assert.h>

namespace Spire
{
	namespace Compiler
	{
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

		// DerefExpr

		RefPtr<SyntaxNode> DerefExpr::Accept(SyntaxVisitor * visitor)
		{
			// throw "unimplemented";
			return this;
		}

		DerefExpr * DerefExpr::Clone(CloneContext & ctx)
		{
			throw "unimplemented";
		}

		//

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

		// OverloadedExpr

		RefPtr<SyntaxNode> OverloadedExpr::Accept(SyntaxVisitor * visitor)
		{
//			throw "unimplemented";
			return this;
		}

		OverloadedExpr * OverloadedExpr::Clone(CloneContext & ctx)
		{
			throw "unimplemented";
		}

		//

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

		bool ExpressionType::EqualsVal(Val* val)
		{
			if (auto type = dynamic_cast<ExpressionType*>(val))
				return const_cast<ExpressionType*>(this)->Equals(type);
			return false;
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

		RefPtr<Val> ExpressionType::SubstituteImpl(Substitutions* subst, int* ioDiff)
		{
			int diff = 0;
			auto canSubst = GetCanonicalType()->SubstituteImpl(subst, &diff);

			// If nothing changed, then don't drop any sugar that is applied
			if (!diff)
				return this;

			// If the canonical type changed, then we return a canonical type,
			// rather than try to re-construct any amount of sugar
			(*ioDiff)++;
			return canSubst;
		}


        ExpressionType* ExpressionType::GetCanonicalType() const
        {
			if (!this) return nullptr;
            ExpressionType* et = const_cast<ExpressionType*>(this);
            if (!et->canonicalType)
            {
                // TODO(tfoley): worry about thread safety here?
                et->canonicalType = et->CreateCanonicalType();
				assert(et->canonicalType);
            }
            return et->canonicalType;
        }

		BindableResourceType ExpressionType::GetBindableResourceType() const
		{
			if (auto textureType = As<TextureType>())
				return BindableResourceType::Texture;
			else if (auto samplerType = As<SamplerStateType>())
				return BindableResourceType::Sampler;
			else if(auto storageBufferType = As<StorageBufferType>())
			{
				return BindableResourceType::StorageBuffer;
			}
			else if(auto uniformBufferType = As<UniformBufferType>())
			{
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
			auto structDeclRef = declRefType->declRef.As<StructDeclRef>();
			if (!structDeclRef) return false;
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
		RefPtr<ExpressionType> ExpressionType::Overloaded;
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
			Overloaded = new OverloadGroupType();
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

		// DeclRefType

		String DeclRefType::ToString() const
		{
			return declRef.GetName();
		}

		bool DeclRefType::EqualsImpl(const ExpressionType * type) const
		{
			if (auto declRefType = type->AsDeclRefType())
			{
				return declRef.Equals(declRefType->declRef);
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

		RefPtr<Val> DeclRefType::SubstituteImpl(Substitutions* subst, int* ioDiff)
		{
			if (!subst) return this;

			// the case we especially care about is when this type references a declaration
			// of a generic parameter, since that is what we might be substituting...
			if (auto genericTypeParamDecl = dynamic_cast<GenericTypeParamDecl*>(declRef.GetDecl()))
			{
				// search for a substitution that might apply to us
				for (auto s = subst; s; s = s->outer.Ptr())
				{
					// the generic decl associated with the substitution list must be
					// the generic decl that declared this parameter
					auto genericDecl = s->genericDecl;
					if (genericDecl != genericTypeParamDecl->ParentDecl)
						continue;

					int index = 0;
					for (auto m : genericDecl->Members)
					{
						if (m.Ptr() == genericTypeParamDecl)
						{
							// We've found it, so return the corresponding specialization argument
							(*ioDiff)++;
							return s->args[index];
						}
						else if(auto typeParam = m.As<GenericTypeParamDecl>())
						{
							index++;
						}
						else if(auto valParam = m.As<GenericValueParamDecl>())
						{
							index++;
						}
						else
						{
						}
					}

				}
			}


			int diff = 0;
			DeclRef substDeclRef = declRef.SubstituteImpl(subst, &diff);

			if (!diff)
				return this;

			// Re-construct the type in case we are using a specialized sub-class
			return DeclRefType::Create(substDeclRef);
		}

		static RefPtr<ExpressionType> ExtractGenericArgType(RefPtr<Val> val)
		{
			auto type = val.As<ExpressionType>();
			assert(type.Ptr());
			return type;
		}

		static int ExtractGenericArgInteger(RefPtr<Val> val)
		{
			auto intVal = val.As<IntVal>();
			assert(intVal.Ptr());
			return intVal->value;
		}

		// TODO: need to figure out how to unify this with the logic
		// in the generic case...
		DeclRefType* DeclRefType::Create(DeclRef declRef)
		{
			if (auto builtinMod = declRef.GetDecl()->FindModifier<BuiltinTypeModifier>())
			{
				auto type = new BasicExpressionType(builtinMod->tag);
				type->declRef = declRef;
				return type;
			}
			else if (auto magicMod = declRef.GetDecl()->FindModifier<MagicTypeModifier>())
			{
				Substitutions* subst = declRef.substitutions.Ptr();

				if (magicMod->name == "SamplerState")
				{
					auto type = new SamplerStateType();
					type->declRef = declRef;
					type->flavor = SamplerStateType::Flavor(magicMod->tag);
					return type;
				}
				else if (magicMod->name == "Vector")
				{
					assert(subst && subst->args.Count() == 2);
					auto vecType = new VectorExpressionType(
						ExtractGenericArgType(subst->args[0]),
						ExtractGenericArgInteger(subst->args[1]));
					vecType->declRef = declRef;
					return vecType;
				}
				else if (magicMod->name == "Matrix")
				{
					assert(subst && subst->args.Count() == 3);
					auto matType = new MatrixExpressionType(
						ExtractGenericArgType(subst->args[0]),
						ExtractGenericArgInteger(subst->args[1]),
						ExtractGenericArgInteger(subst->args[2]));
					matType->declRef = declRef;
					return matType;
				}
				else if (magicMod->name == "Texture")
				{
					assert(subst && subst->args.Count() >= 1);
					auto textureType = new TextureType(
						magicMod->tag,
						ExtractGenericArgType(subst->args[0]));
					textureType->declRef = declRef;
					return textureType;
				}

				#define CASE(n,T)													\
					else if(magicMod->name == #n) {									\
						assert(subst && subst->args.Count() == 1);					\
						auto type = new T();										\
						type->elementType = ExtractGenericArgType(subst->args[0]);	\
						type->declRef = declRef;									\
						return type;												\
					}

				CASE(ConstantBuffer, ConstantBufferType)
				CASE(TextureBuffer, TextureBufferType)

				CASE(PackedBuffer, PackedBufferType)
				CASE(StructuredBuffer, StructuredBufferType)
				CASE(RWStructuredBuffer, RWStructuredBufferType)
				CASE(Uniform, UniformBufferType)
				CASE(Patch, PatchType)

				else
				{
					throw "unimplemented";
				}
			}
			else
			{
				return new DeclRefType(declRef);
			}
		}

		// OverloadGroupType

		String OverloadGroupType::ToString() const
		{
			return "overload group";
		}

		bool OverloadGroupType::EqualsImpl(const ExpressionType * type) const
		{
			return false;
		}

		ExpressionType* OverloadGroupType::CreateCanonicalType()
		{
			return  this;
		}

        // NamedExpressionType

        String NamedExpressionType::ToString() const
        {
			return declRef.GetName();
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
            return declRef.GetType()->GetCanonicalType();
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

		// GenericDeclRefType

		String GenericDeclRefType::ToString() const
		{
			// TODO: what is appropriate here?
			return "<GenericDeclRef>";
		}

		bool GenericDeclRefType::EqualsImpl(const ExpressionType * type) const
		{
			if (auto genericDeclRefType = type->As<GenericDeclRefType>())
			{
				return declRef.Equals(genericDeclRefType->declRef);
			}
			return false;
		}

		ExpressionType* GenericDeclRefType::CreateCanonicalType()
		{
			return this;
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
			canType->declRef = declRef;
			sCanonicalTypes.Add(canType);
			return canType;
		}

		RefPtr<Val> VectorExpressionType::SubstituteImpl(Substitutions* subst, int* ioDiff)
		{
			int diff = 0;
			auto substDeclRef = declRef.SubstituteImpl(subst, &diff);
			auto substElementType = elementType->SubstituteImpl(subst, &diff).As<ExpressionType>();

			if (!diff)
				return this;

			(*ioDiff)++;
			auto substType = new VectorExpressionType(substElementType, elementCount);
			substType->declRef = substDeclRef;
			return substType;
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
			canType->declRef = declRef;
			sCanonicalTypes.Add(canType);
			return canType;
		}


		RefPtr<Val> MatrixExpressionType::SubstituteImpl(Substitutions* subst, int* ioDiff)
		{
			int diff = 0;
			auto substDeclRef = declRef.SubstituteImpl(subst, &diff);
			auto substElementType = elementType->SubstituteImpl(subst, &diff).As<ExpressionType>();

			if (!diff)
				return this;

			(*ioDiff)++;
			auto substType = new MatrixExpressionType(substElementType, rowCount, colCount);
			substType->declRef = substDeclRef;
			return substType;
		}

		// TextureType

		RefPtr<Val> TextureType::SubstituteImpl(Substitutions* subst, int* ioDiff)
		{
			int diff = 0;
			auto substDeclRef = declRef.SubstituteImpl(subst, &diff);
			auto substElementType = elementType->SubstituteImpl(subst, &diff).As<ExpressionType>();

			if (!diff)
				return this;

			(*ioDiff)++;
			auto substType = new TextureType(flavor, substElementType);
			substType->declRef = substDeclRef;
			return substType;
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

		RefPtr<SyntaxNode> GenericDecl::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitGenericDecl(this);
		}

		GenericDecl * GenericDecl::Clone(CloneContext & ctx) {
			throw "unimplemented";
		}

		// GenericTypeParamDecl

		RefPtr<SyntaxNode> GenericTypeParamDecl::Accept(SyntaxVisitor * visitor) {
			//throw "unimplemented";
			return this;
		}

		GenericTypeParamDecl * GenericTypeParamDecl::Clone(CloneContext & ctx) {
			throw "unimplemented";
		}

		// GenericValueParamDecl

		RefPtr<SyntaxNode> GenericValueParamDecl::Accept(SyntaxVisitor * visitor) {
			//throw "unimplemented";
			return this;
		}

		GenericValueParamDecl * GenericValueParamDecl::Clone(CloneContext & ctx) {
			throw "unimplemented";
		}

		// ExtensionDecl

		RefPtr<SyntaxNode> ExtensionDecl::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitExtensionDecl(this);
			return this;
		}

		ExtensionDecl* ExtensionDecl::Clone(CloneContext & ctx)
		{
			throw "unimplemented";
		}

		// ConstructorDecl

		RefPtr<SyntaxNode> ConstructorDecl::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitConstructorDecl(this);
			return this;
		}

		ConstructorDecl* ConstructorDecl::Clone(CloneContext & ctx)
		{
			throw "unimplemented";
		}

		// Substitutions

		RefPtr<Substitutions> Substitutions::SubstituteImpl(Substitutions* subst, int* ioDiff)
		{
			if (!this) return nullptr;

			int diff = 0;
			auto outerSubst = outer->SubstituteImpl(subst, &diff);

			List<RefPtr<Val>> substArgs;
			for (auto a : args)
			{
				substArgs.Add(a->SubstituteImpl(subst, &diff));
			}

			if (!diff) return this;

			auto substSubst = new Substitutions();
			substSubst->genericDecl = genericDecl;
			substSubst->args = substArgs;
			return substSubst;
		}

		bool Substitutions::Equals(Substitutions* subst)
		{
			// both must be NULL, or non-NULL
			if (!this || !subst)
				return !this && !subst;

			if (genericDecl != subst->genericDecl)
				return false;

			int argCount = args.Count();
			assert(args.Count() == subst->args.Count());
			for (int aa = 0; aa < argCount; ++aa)
			{
				if (!args[aa]->EqualsVal(subst->args[aa].Ptr()))
					return false;
			}

			if (!outer->Equals(subst->outer.Ptr()))
				return false;

			return true;
		}


		// DeclRef

		RefPtr<ExpressionType> DeclRef::Substitute(RefPtr<ExpressionType> type) const
		{
			// No substitutions? Easy.
			if (!substitutions)
				return type;

			// Otherwise we need to recurse on the type structure
			// and apply substitutions where it makes sense

			return type->Substitute(substitutions.Ptr()).As<ExpressionType>();
		}

		DeclRef DeclRef::SubstituteImpl(Substitutions* subst, int* ioDiff)
		{
			if (!substitutions) return *this;

			int diff = 0;
			RefPtr<Substitutions> substSubst = substitutions->SubstituteImpl(subst, &diff);

			if (!diff)
				return *this;

			DeclRef substDeclRef;
			substDeclRef.decl = decl;
			substDeclRef.substitutions = substSubst;
			return substDeclRef;
		}


		// Check if this is an equivalent declaration reference to another
		bool DeclRef::Equals(DeclRef const& declRef) const
		{
			if (decl != declRef.decl)
				return false;

			if (!substitutions->Equals(declRef.substitutions.Ptr()))
				return false;

			return true;
		}

		// Convenience accessors for common properties of declarations
		String const& DeclRef::GetName() const
		{
			return decl->Name.Content;
		}

		DeclRef DeclRef::GetParent() const
		{
			auto parentDecl = decl->ParentDecl;
			if (auto parentGeneric = dynamic_cast<GenericDecl*>(parentDecl))
			{
				// We need to strip away one layer of specialization
				assert(substitutions);
				return DeclRef(parentGeneric, substitutions->outer);
			}
			else
			{
				// If the parent isn't a generic, then it must
				// use the same specializations as this declaration
				return DeclRef(parentDecl, substitutions);
			}

		}

		// Val

		RefPtr<Val> Val::Substitute(Substitutions* subst)
		{
			if (!this) return nullptr;
			if (!subst) return this;
			int diff = 0;
			return SubstituteImpl(subst, &diff);
		}

		RefPtr<Val> Val::SubstituteImpl(Substitutions* subst, int* ioDiff)
		{
			// Default behavior is to not substitute at all
			return this;
		}

		// Intval

		bool IntVal::EqualsVal(Val* val)
		{
			if (auto intVal = dynamic_cast<IntVal*>(val))
				return value == intVal->value;
			return false;
		}
	}
}