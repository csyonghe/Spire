#include "Syntax.h"
#include "SyntaxVisitors.h"
#include "SymbolTable.h"

namespace Spire
{
	namespace Compiler
	{
		ExpressionType ExpressionType::Bool(Compiler::BaseType::Bool);
		ExpressionType ExpressionType::Int(Compiler::BaseType::Int);
		ExpressionType ExpressionType::UInt(Compiler::BaseType::UInt);
		ExpressionType ExpressionType::Float(Compiler::BaseType::Float);
		ExpressionType ExpressionType::Int2(Compiler::BaseType::Int2);
		ExpressionType ExpressionType::Float2(Compiler::BaseType::Float2);
		ExpressionType ExpressionType::Int3(Compiler::BaseType::Int3);
		ExpressionType ExpressionType::Float3(Compiler::BaseType::Float3);
		ExpressionType ExpressionType::Int4(Compiler::BaseType::Int4);
		ExpressionType ExpressionType::Float4(Compiler::BaseType::Float4);
		ExpressionType ExpressionType::Void(Compiler::BaseType::Void);
		ExpressionType ExpressionType::Error(Compiler::BaseType::Error);

		bool Scope::FindVariable(const String & name, VariableEntry & variable)
		{
			if (Variables.TryGetValue(name, variable))
				return true;
			if (Parent)
				return Parent->FindVariable(name, variable);
			return false;
		}

		int ExpressionType::GetSize()
		{
			int baseSize = GetVectorSize(BaseType);
			if (BaseType == Compiler::BaseType::Texture2D || BaseType == Compiler::BaseType::TextureCube ||
				BaseType == Compiler::BaseType::TextureCubeShadow || BaseType == Compiler::BaseType::TextureShadow)
				baseSize = sizeof(void*) / sizeof(int);
			else if (BaseType == Compiler::BaseType::Struct)
				baseSize = Struct->Type->GetSize();
			if (ArrayLength == 0)
				return baseSize;
			else
				return ArrayLength*baseSize;
		}

		CoreLib::Basic::String ExpressionType::ToString()
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
				res.Append(L"(");
				for (int i = 0; i < Func->Parameters.Count(); i++)
				{
					if (i > 0)
						res.Append(L",");
					res.Append(Func->Parameters[i]->Type->ToString());
				}
				res.Append(L") => ");
				res.Append(Func->ReturnType->ToString());
				break;
			case Compiler::BaseType::Shader:
				res.Append(Shader->SyntaxNode->Name.Content);
				break;
			case Compiler::BaseType::Void:
				res.Append("void");
				break;
			default:
				break;
			}
			if (ArrayLength != 0)
			{
				res.Append(L"[");
				res.Append(CoreLib::Basic::String(ArrayLength));
				res.Append(L"]");
			}
			return res.ToString();
		}


		void ProgramSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitProgram(this);
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
		void FunctionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitFunction(this);
		}
		FunctionSyntaxNode * FunctionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new FunctionSyntaxNode(*this), ctx);
			rs->Parameters.Clear();
			for (auto & param : Parameters)
			{
				rs->Parameters.Add(param->Clone(ctx));
			}
			rs->ReturnType = ReturnType->Clone(ctx);
			rs->Body = Body->Clone(ctx);
			return rs;
		}
		void BlockStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitBlockStatement(this);
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
		void BreakStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitBreakStatement(this);
		}
		BreakStatementSyntaxNode * BreakStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new BreakStatementSyntaxNode(*this), ctx);
		}
		void ContinueStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitContinueStatement(this);
		}
		ContinueStatementSyntaxNode * ContinueStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new ContinueStatementSyntaxNode(*this), ctx);
		}
		void DoWhileStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitDoWhileStatement(this);
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
		void EmptyStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitEmptyStatement(this);
		}
		EmptyStatementSyntaxNode * EmptyStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new EmptyStatementSyntaxNode(*this), ctx);
		}
		void ForStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitForStatement(this);
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
		void IfStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitIfStatement(this);
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
		void ReturnStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitReturnStatement(this);
		}
		ReturnStatementSyntaxNode * ReturnStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ReturnStatementSyntaxNode(*this), ctx);
			if (Expression)
				rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void VarDeclrStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitVarDeclrStatement(this);
		}
		VarDeclrStatementSyntaxNode * VarDeclrStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new VarDeclrStatementSyntaxNode(*this), ctx);
			rs->Type = Type->Clone(ctx);
			rs->Variables.Clear();
			for (auto & var : Variables)
				rs->Variables.Add(var->Clone(ctx));
			return rs;
		}
		void Variable::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitDeclrVariable(this);
		}
		Variable * Variable::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new Variable(*this), ctx);
			if (Expression)
				rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void WhileStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitWhileStatement(this);
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
		void ExpressionStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitExpressionStatement(this);
		}
		ExpressionStatementSyntaxNode * ExpressionStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ExpressionStatementSyntaxNode(*this), ctx);
			if (Expression)
				rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void BinaryExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitBinaryExpression(this);
		}
		BinaryExpressionSyntaxNode * BinaryExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new BinaryExpressionSyntaxNode(*this), ctx);
			rs->LeftExpression = LeftExpression->Clone(ctx);
			rs->RightExpression = RightExpression->Clone(ctx);
			return rs;
		}
		void ConstantExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitConstantExpression(this);
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
		void IndexExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitIndexExpression(this);
		}
		void MemberExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitMemberExpression(this);
		}
		MemberExpressionSyntaxNode * MemberExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new MemberExpressionSyntaxNode(*this), ctx);
			rs->BaseExpression = BaseExpression->Clone(ctx);
			return rs;
		}
		void InvokeExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitInvokeExpression(this);
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
		void TypeCastExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitTypeCastExpression(this);
		}
		TypeCastExpressionSyntaxNode * TypeCastExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new TypeCastExpressionSyntaxNode(*this), ctx);
			rs->TargetType = TargetType->Clone(ctx);
			rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void SelectExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitSelectExpression(this);
		}
		SelectExpressionSyntaxNode * SelectExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new SelectExpressionSyntaxNode(*this), ctx);
			rs->SelectorExpr = SelectorExpr->Clone(ctx);
			rs->Expr0 = Expr0->Clone(ctx);
			rs->Expr1 = Expr1->Clone(ctx);
			return rs;
		}
		void UnaryExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitUnaryExpression(this);
		}
		UnaryExpressionSyntaxNode * UnaryExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new UnaryExpressionSyntaxNode(*this), ctx);
			rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void VarExpressionSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitVarExpression(this);
		}
		VarExpressionSyntaxNode * VarExpressionSyntaxNode::Clone(CloneContext & ctx)
		{
			return CloneSyntaxNodeFields(new VarExpressionSyntaxNode(*this), ctx);
		}
		void ParameterSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitParameter(this);
		}
		ParameterSyntaxNode * ParameterSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ParameterSyntaxNode(*this), ctx);
			rs->Type = Type->Clone(ctx);
			rs->Expr = Expr->Clone(ctx);
			return rs;
		}
		void TypeSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitType(this);
		}
		TypeSyntaxNode * TypeSyntaxNode::FromExpressionType(ExpressionType t)
		{
			TypeSyntaxNode * rs = new TypeSyntaxNode();
			ExpressionType expType;
			if (t.BaseType == BaseType::Int)
				rs->TypeName = L"int";
			else if (t.BaseType == BaseType::Float)
				rs->TypeName = L"float";
			else if (t.BaseType == BaseType::Int2)
				rs->TypeName = L"ivec2";
			else if (t.BaseType == BaseType::Int3)
				rs->TypeName = L"ivec3";
			else if (t.BaseType == BaseType::Int4)
				rs->TypeName = L"ivec4";
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
			rs->ArrayLength = 0;
			rs->IsArray = false;		
			return rs;
		}
		ExpressionType TypeSyntaxNode::ToExpressionType(SymbolTable * symTable, ErrorWriter * errWriter)
		{
			ExpressionType expType;
			if (TypeName == L"int")
				expType.BaseType = BaseType::Int;
			else if (TypeName == L"uint")
				expType.BaseType = BaseType::UInt;
			else if (TypeName == L"float")
				expType.BaseType = BaseType::Float;
			else if (TypeName == L"ivec2")
				expType.BaseType = BaseType::Int2;
			else if (TypeName == L"ivec3")
				expType.BaseType = BaseType::Int3;
			else if (TypeName == L"ivec4")
				expType.BaseType = BaseType::Int4;
			else if (TypeName == L"vec2")
				expType.BaseType = BaseType::Float2;
			else if (TypeName == L"vec3")
				expType.BaseType = BaseType::Float3;
			else if (TypeName == L"vec4")
				expType.BaseType = BaseType::Float4;
			else if (TypeName == L"mat3" || TypeName == L"mat3x3")
				expType.BaseType = BaseType::Float3x3;
			else if (TypeName == L"mat4" || TypeName == L"mat4x4")
				expType.BaseType = BaseType::Float4x4;
			else if (TypeName == L"sampler2D")
				expType.BaseType = BaseType::Texture2D;
			else if (TypeName == L"samplerCube")
				expType.BaseType = BaseType::TextureCube;
			else if (TypeName == L"sampler2DShadow")
				expType.BaseType = BaseType::TextureShadow;
			else if (TypeName == L"samplerCubeShadow")
				expType.BaseType = BaseType::TextureCubeShadow;
			else if (TypeName == L"void")
				expType.BaseType = BaseType::Void;
			else
			{
				expType.BaseType = BaseType::Struct;
				RefPtr<StructSymbol> ssym;
				if (symTable->Structs.TryGetValue(TypeName, ssym))
				{
					expType.Struct = ssym.Ptr();
				}
				else
				{
					if (errWriter)
					{
						errWriter->Error(31040, L"undefined type name: '" + TypeName + L"'.", Position);
					}
					return ExpressionType::Error;
				}
			}
			expType.ArrayLength = ArrayLength;
			expType.IsArray = IsArray;
			return expType;
		}
		void ComponentSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitComponent(this);
		}
		ComponentSyntaxNode * ComponentSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ComponentSyntaxNode(*this), ctx);
			rs->Type = Type->Clone(ctx);
			if (Rate)
				rs->Rate = Rate->Clone(ctx);
			if (BlockStatement)
				rs->BlockStatement = BlockStatement->Clone(ctx);
			if (Expression)
				rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void ShaderSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitShader(this);
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
		void ImportSyntaxNode::Accept(SyntaxVisitor * v)
		{
			v->VisitImport(this);
		}
		ImportSyntaxNode * ImportSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ImportSyntaxNode(*this), ctx);
			rs->Arguments.Clear();
			for (auto & arg : Arguments)
				rs->Arguments.Add(arg->Clone(ctx));
			return rs;
		}
		void ImportArgumentSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitImportArgument(this);
		}
		ImportArgumentSyntaxNode * ImportArgumentSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ImportArgumentSyntaxNode(*this), ctx);
			rs->Expression = Expression->Clone(ctx);
			return rs;
		}
		void ImportStatementSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitImportStatement(this);
		}
		ImportStatementSyntaxNode * ImportStatementSyntaxNode::Clone(CloneContext & ctx)
		{
			auto rs = CloneSyntaxNodeFields(new ImportStatementSyntaxNode(*this), ctx);
			rs->Import = Import->Clone(ctx);
			return rs;
		}
		void StructField::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitStructField(this);
		}
		void StructSyntaxNode::Accept(SyntaxVisitor * visitor)
		{
			visitor->VisitStruct(this);
		}
	}
}