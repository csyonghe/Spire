#include "Syntax.h"
#include "SyntaxVisitors.h"
#include "SymbolTable.h"

namespace Spire
{
	namespace Compiler
	{
		bool Scope::FindVariable(const String & name, VariableEntry & variable)
		{
			if (Variables.TryGetValue(name, variable))
				return true;
			if (Parent)
				return Parent->FindVariable(name, variable);
			return false;
		}

		int BasicExpressionType::GetSize() const
		{
			int baseSize = GetVectorSize(BaseType);
			if (BaseType == Compiler::BaseType::Texture2D || BaseType == Compiler::BaseType::TextureCube ||
				BaseType == Compiler::BaseType::TextureCubeShadow || BaseType == Compiler::BaseType::TextureShadow)
				baseSize = sizeof(void*) / sizeof(int);
			else if (BaseType == Compiler::BaseType::Struct)
				baseSize = Struct->Type->GetSize();
			return baseSize;
		}

		bool BasicExpressionType::Equals(const ExpressionType * type) const
		{
			auto basicType = dynamic_cast<const BasicExpressionType*>(type);
			if (basicType == nullptr)
				return false;
			return (basicType->BaseType == BaseType &&
				basicType->Func == Func &&
				basicType->Shader == Shader &&
				basicType->Struct == Struct);
		}

		bool BasicExpressionType::IsVectorType() const
		{
			return IsVector(BaseType);
		}

		bool BasicExpressionType::IsArray() const
		{
			return false;
		}

		CoreLib::Basic::String BasicExpressionType::ToString() const
		{
			CoreLib::Basic::StringBuilder res;

			switch (BaseType)
			{
			case Compiler::BaseType::Int:
				res.Append(L"int");
				break;
			case Compiler::BaseType::UInt:
				res.Append(L"uint");
				break;
			case Compiler::BaseType::Bool:
				res.Append(L"bool");
				break;
			case Compiler::BaseType::Float:
				res.Append(L"float");
				break;
			case Compiler::BaseType::Int2:
				res.Append(L"ivec2");
				break;
			case Compiler::BaseType::Float2:
				res.Append(L"vec2");
				break;
			case Compiler::BaseType::Int3:
				res.Append(L"ivec3");
				break;
			case Compiler::BaseType::Float3:
				res.Append(L"vec3");
				break;
			case Compiler::BaseType::Int4:
				res.Append(L"ivec4");
				break;
			case Compiler::BaseType::Float4:
				res.Append(L"vec4");
				break;
			case Compiler::BaseType::Float3x3:
				res.Append(L"mat3");
				break;
			case Compiler::BaseType::Float4x4:
				res.Append(L"mat4");
				break;
			case Compiler::BaseType::Texture2D:
				res.Append(L"sampler2D");
				break;
			case Compiler::BaseType::TextureCube:
				res.Append(L"samplerCube");
				break;
			case Compiler::BaseType::TextureShadow:
				res.Append(L"samplerShadow");
				break;
			case Compiler::BaseType::TextureCubeShadow:
				res.Append(L"samplerCubeShadow");
				break;
			case Compiler::BaseType::Function:
				res.Append(Func->InternalName);
				break;
			case Compiler::BaseType::Shader:
				res.Append(Shader->SyntaxNode->Name.Content);
				break;
			case Compiler::BaseType::Void:
				res.Append("void");
				break;
			case Compiler::BaseType::Record:
				res.Append(RecordTypeName);
				break;
			case Compiler::BaseType::Error:
				res.Append(L"<errtype>");
				break;
			default:
				break;
			}
			return res.ToString();
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
			rs->Structs.Clear();
			for (auto & x : Structs)
				rs->Structs.Add(x->Clone(ctx));
			rs->Functions.Clear();
			for (auto & x : Functions)
				rs->Functions.Add(x->Clone(ctx));
			rs->Pipelines.Clear();
			for (auto & x : Pipelines)
				rs->Pipelines.Add(x->Clone(ctx));
			rs->Shaders.Clear();
			for (auto & x : Shaders)
				rs->Shaders.Add(x->Clone(ctx));
			return rs;
		}
		RefPtr<SyntaxNode> FunctionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitFunction(this);
		}
		FunctionSyntaxNode * FunctionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new FunctionSyntaxNode(*this), ctx);
			rs->Parameters.Clear();
			for (auto & param : Parameters)
			{
				rs->Parameters.Add(param->Clone(ctx));
			}
			rs->ReturnTypeNode = ReturnTypeNode->Clone(ctx);
			rs->Body = Body->Clone(ctx);
			return rs;
		}
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
			if (InitialExpression)
				rs->InitialExpression = InitialExpression->Clone(ctx);
			if (StepExpression)
				rs->StepExpression = StepExpression->Clone(ctx);
			if (EndExpression)
				rs->EndExpression = EndExpression->Clone(ctx);
			if (Statement)
				rs->Statement = Statement->Clone(ctx);
			rs->TypeDef = TypeDef->Clone(ctx);
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
			rs->TypeNode = TypeNode->Clone(ctx);
			rs->Variables.Clear();
			for (auto & var : Variables)
				rs->Variables.Add(var->Clone(ctx));
			return rs;
		}
		RefPtr<SyntaxNode> Variable::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitDeclrVariable(this);
		}
		Variable * Variable::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new Variable(*this), ctx);
			if (Expression)
				rs->Expression = Expression->Clone(ctx);
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
		RefPtr<TypeSyntaxNode> TypeSyntaxNode::FromExpressionType(ExpressionType * type)
		{
			if (auto basicType = dynamic_cast<BasicExpressionType*>(type))
			{
				RefPtr<BasicTypeSyntaxNode> rs = new BasicTypeSyntaxNode();
				auto & t = *basicType;
				if (basicType->BaseType == BaseType::Int)
					rs->TypeName = L"int";
				else if (t.BaseType == BaseType::Float)
					rs->TypeName = L"float";
				else if (t.BaseType == BaseType::Bool)
					rs->TypeName = L"bool";
				else if (t.BaseType == BaseType::Int2)
					rs->TypeName = L"ivec2";
				else if (t.BaseType == BaseType::Int3)
					rs->TypeName = L"ivec3";
				else if (t.BaseType == BaseType::Int4)
					rs->TypeName = L"ivec4";
				else if (t.BaseType == BaseType::UInt)
					rs->TypeName = L"uint";
				else if (t.BaseType == BaseType::UInt2)
					rs->TypeName = L"uint2";
				else if (t.BaseType == BaseType::UInt3)
					rs->TypeName = L"uint3";
				else if (t.BaseType == BaseType::UInt4)
					rs->TypeName = L"uint4";
				else if (t.BaseType == BaseType::Float2)
					rs->TypeName = L"vec2";
				else if (t.BaseType == BaseType::Float3)
					rs->TypeName = L"vec3";
				else if (t.BaseType == BaseType::Float4)
					rs->TypeName = L"vec4";
				else if (t.BaseType == BaseType::Float3x3)
					rs->TypeName = L"mat3";
				else if (t.BaseType == BaseType::Float4x4)
					rs->TypeName = L"mat4";
				else if (t.BaseType == BaseType::Texture2D)
					rs->TypeName = L"sampler2D";
				else if (t.BaseType == BaseType::TextureCube)
					rs->TypeName = L"samplerCube";
				else if (t.BaseType == BaseType::TextureShadow)
					rs->TypeName = L"samplerShadow";
				else if (t.BaseType == BaseType::TextureCubeShadow)
					rs->TypeName = L"samplerCubeShadow";
				return rs;
			}
			else if (auto arrayType = dynamic_cast<ArrayExpressionType*>(type))
			{
				RefPtr<ArrayTypeSyntaxNode> rs = new ArrayTypeSyntaxNode();
				rs->ArrayLength = arrayType->ArrayLength;
				rs->BaseType = FromExpressionType(arrayType->BaseType.Ptr());
				return rs;
			}
			throw NotImplementedException();
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
			rs->Worlds.Clear();
			for (auto & w : Worlds)
				rs->Worlds.Add(w->Clone(ctx));
			rs->ImportOperators.Clear();
			for (auto & imp : ImportOperators)
				rs->ImportOperators.Add(imp->Clone(ctx));
			rs->AbstractComponents.Clear();
			for (auto & comp : AbstractComponents)
				rs->AbstractComponents.Add(comp->Clone(ctx));
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
		RefPtr<SyntaxNode> DiscardStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			return visitor->VisitDiscardStatement(this);
		}
		DiscardStatementSyntaxNode * DiscardStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new DiscardStatementSyntaxNode(*this), ctx);
			return rs;
		}
		bool BasicExpressionType::IsIntegral() const
		{
			return (BaseType == Compiler::BaseType::Int || BaseType == Compiler::BaseType::UInt || BaseType == Compiler::BaseType::Bool);
		}
		bool ExpressionType::IsTexture() const
		{
			auto basicType = AsBasicType();
			if (basicType)
				return basicType->BaseType == BaseType::Texture2D ||
					basicType->BaseType == BaseType::TextureCube ||
					basicType->BaseType == BaseType::TextureCubeShadow ||
					basicType->BaseType == BaseType::TextureShadow;
			return false;
		}
		bool ExpressionType::IsStruct() const
		{
			auto basicType = AsBasicType();
			if (basicType)
				return basicType->Struct != nullptr;
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
		}
		bool ArrayExpressionType::IsIntegral() const
		{
			return false;
		}
		bool ArrayExpressionType::IsArray() const
		{
			return true;
		}
		int ArrayExpressionType::GetSize() const
		{
			return ArrayLength * BaseType->GetSize();
		}
		bool ArrayExpressionType::Equals(const ExpressionType * type) const
		{
			auto arrType = dynamic_cast<const ArrayExpressionType*>(type);
			if (!arrType)
				return false;
			return (ArrayLength == arrType->ArrayLength && BaseType->Equals(arrType->BaseType.Ptr()));
		}
		bool ArrayExpressionType::IsVectorType() const
		{
			return false;
		}
		CoreLib::Basic::String ArrayExpressionType::ToString() const
		{
			if (ArrayLength > 0)
				return BaseType->ToString() + L"[" + String(ArrayLength) + L"]";
			else
				return BaseType->ToString() + L"[]";
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
		bool GenericExpressionType::IsIntegral() const
		{
			return false;
		}
		int GenericExpressionType::GetSize() const
		{
			return 0;
		}
		bool GenericExpressionType::IsArray() const
		{
			return false;
		}
		bool GenericExpressionType::Equals(const ExpressionType * type) const
		{
			if (auto gtype = dynamic_cast<const GenericExpressionType*>(type))
				return GenericTypeName == gtype->GenericTypeName && gtype->BaseType->Equals(BaseType.Ptr());
			
			return false;
		}
		bool GenericExpressionType::IsVectorType() const
		{
			return false;
		}
		CoreLib::Basic::String GenericExpressionType::ToString() const
		{
			return GenericTypeName + L"<" + BaseType->ToString() + L">";
		}
		ExpressionType * GenericExpressionType::Clone()
		{
			auto rs = new GenericExpressionType(*this);
			rs->BaseType = BaseType->Clone();
			return rs;
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
}
}