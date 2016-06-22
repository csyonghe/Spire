#ifndef RASTER_RENDERER_SYNTAX_H
#define RASTER_RENDERER_SYNTAX_H

#include "../CoreLib/Basic.h"
#include "Lexer.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;
		class SyntaxVisitor;
		class FunctionSyntaxNode;

		enum class VariableModifier
		{
			None = 0,
			Uniform = 1,
			Out = 2,
			In = 4,
			Centroid = 128,
			Const = 16,
			Instance = 1024,
			Builtin = 256,
			Parameter = 513
		};

		enum class BaseType
		{
			Void = 0,
			Int = 16, Int2 = 17, Int3 = 18, Int4 = 19,
			Float = 32, Float2 = 33, Float3 = 34, Float4 = 35,
			Float3x3 = 40, Float4x4 = 47,
			Texture2D = 48,
			TextureShadow = 49,
			TextureCube = 50,
			TextureCubeShadow = 51,
			Function = 64,
			Bool = 128,
			Shader = 256,
			Error = 512
		};

		inline const wchar_t * BaseTypeToString(BaseType t)
		{
			switch (t)
			{
			case BaseType::Void:
				return L"void";
			case BaseType::Bool:
			case BaseType::Int:
				return L"int";
			case BaseType::Int2:
				return L"int2";
			case BaseType::Int3:
				return L"int3";
			case BaseType::Int4:
				return L"int4";
			case BaseType::Float:
				return L"float";
			case BaseType::Float2:
				return L"float2";
			case BaseType::Float3:
				return L"float3";
			case BaseType::Float4:
				return L"float4";
			case BaseType::Float3x3:
				return L"float3x3";
			case BaseType::Float4x4:
				return L"float4x4";
			case BaseType::Texture2D:
				return L"sampler2D";
			case BaseType::TextureCube:
				return L"samplerCube";
			case BaseType::TextureShadow:
				return L"sampler2DShadow";
			case BaseType::TextureCubeShadow:
				return L"samplerCubeShadow";
			default:
				return L"<err-type>";
			}
		}

		inline bool IsVector(BaseType type)
		{
			return (((int)type) & 15) != 0;
		}

		inline int GetVectorSize(BaseType type)
		{
			return (((int)type) & 15) + 1;
		}

		inline BaseType GetVectorBaseType(BaseType type)
		{
			return (BaseType)(((int)type) & (~15));
		}

		inline bool IsTextureType(BaseType type)
		{
			return type == BaseType::Texture2D || type == BaseType::TextureCube || type == BaseType::TextureCubeShadow || type == BaseType::TextureShadow;
		}
		class ShaderSymbol;
		class ShaderClosure;
		class ExpressionType
		{
		public:
			bool IsLeftValue;
			bool IsReference;
			BaseType BaseType;
			bool IsArray = false;
			int ArrayLength = 0;
			ShaderSymbol * Shader = nullptr;
			ShaderClosure * ShaderClosure = nullptr;
			FunctionSyntaxNode * Func = nullptr;
			ExpressionType GetBaseType()
			{
				ExpressionType rs;
				rs.IsLeftValue = IsLeftValue;
				rs.BaseType = BaseType;
				rs.IsArray = false;
				rs.IsReference = false;
				rs.ArrayLength = 0;
				rs.Func = Func;
				return rs;
			}
			ExpressionType()
			{
				BaseType = Compiler::BaseType::Int;
				ArrayLength = 0;
				IsArray = false;
				Func = 0;
				IsLeftValue = false;
				IsReference = false;
			}
			bool IsTextureType()
			{
				return !IsArray && (BaseType == Compiler::BaseType::Texture2D || BaseType == Compiler::BaseType::TextureCube || BaseType == Compiler::BaseType::TextureCubeShadow || BaseType == Compiler::BaseType::TextureShadow);
			}
			int GetSize()
			{
				int baseSize = GetVectorSize(BaseType);
				if (BaseType == Compiler::BaseType::Texture2D || BaseType == Compiler::BaseType::TextureCube ||
					BaseType == Compiler::BaseType::TextureCubeShadow || BaseType == Compiler::BaseType::TextureShadow)
					baseSize = sizeof(void*) / sizeof(int);
				if (ArrayLength == 0)
					return baseSize;
				else
					return ArrayLength*baseSize;
			}
			ExpressionType(Spire::Compiler::BaseType baseType)
			{
				BaseType = baseType;
				ArrayLength = 0;
				IsArray = false;
				Func = 0;
				IsLeftValue = false;
				IsReference = false;
			}

			static ExpressionType Bool;
			static ExpressionType Int;
			static ExpressionType Int2;
			static ExpressionType Int3;
			static ExpressionType Int4;
			static ExpressionType Float;
			static ExpressionType Float2;
			static ExpressionType Float3;
			static ExpressionType Float4;
			static ExpressionType Void;
			static ExpressionType Error;

			bool operator == (const ExpressionType & type)
			{
				return (type.BaseType == BaseType &&
						type.IsArray == IsArray &&
						type.ArrayLength == ArrayLength &&
						type.Func == Func &&
						type.Shader == Shader);
			}

			bool operator != (const ExpressionType & type)
			{
				return !(this->operator==(type));
			}

			bool IsVectorType()
			{
				return (!IsArray) && (IsVector(BaseType));
			}

			CoreLib::Basic::String ToString();
		};
		
		class Type
		{
		public:
			ExpressionType DataType;
			// ContrainedWorlds: Implementation must be defined at at least one of of these worlds in order to satisfy global dependency
			// FeasibleWorlds: The component can be computed at any of these worlds
			EnumerableHashSet<String> ConstrainedWorlds, FeasibleWorlds;
			EnumerableHashSet<String> PinnedWorlds; 
		};


		class VariableEntry
		{
		public:
			String Name;
			Type Type;
			bool IsComponent = false;
		};

		class Scope
		{
		public:
			Scope * Parent;
			Dictionary<String, VariableEntry> Variables;
			bool FindVariable(const String & name, VariableEntry & variable);
			Scope()
				: Parent(0)
			{}
		};

		class CloneContext
		{
		public:
			Dictionary<Spire::Compiler::Scope*, RefPtr<Spire::Compiler::Scope>> ScopeTranslateTable;
		};

		class SyntaxNode : public Object
		{
		protected:
			template<typename T>
			T* CloneSyntaxNodeFields(T * target, CloneContext & ctx)
			{
				if (this->Scope)
				{
					RefPtr<Spire::Compiler::Scope> newScope;
					if (ctx.ScopeTranslateTable.TryGetValue(this->Scope.Ptr(), newScope))
						target->Scope = newScope;
					else
					{
						target->Scope = new Spire::Compiler::Scope(*this->Scope);
						ctx.ScopeTranslateTable[this->Scope.Ptr()] = target->Scope;
						RefPtr<Spire::Compiler::Scope> parentScope;
						if (ctx.ScopeTranslateTable.TryGetValue(target->Scope->Parent, parentScope))
							target->Scope->Parent = parentScope.Ptr();
					}
					
				}
				target->Position = this->Position;
				Tags = this->Tags;
				return target;
			}
		public:
			EnumerableDictionary<String, RefPtr<Object>> Tags;
			CodePosition Position;
			RefPtr<Scope> Scope;
			virtual void Accept(SyntaxVisitor * visitor) = 0;
			virtual SyntaxNode * Clone(CloneContext & ctx) = 0;
		};

		class TypeSyntaxNode : public SyntaxNode
		{
		public:
			bool IsArray;
			String TypeName;
			int ArrayLength;
			String GenericBaseType;
			virtual void Accept(SyntaxVisitor * visitor);
			TypeSyntaxNode()
			{
				ArrayLength = 0;
				IsArray = false;
			}
			
			static TypeSyntaxNode * FromExpressionType(ExpressionType t);

			ExpressionType ToExpressionType()
			{
				ExpressionType expType;
				if (TypeName == "int")
					expType.BaseType = BaseType::Int;
				else if (TypeName == "float")
					expType.BaseType = BaseType::Float;
				else if (TypeName == "ivec2")
					expType.BaseType = BaseType::Int2;
				else if (TypeName == "ivec3")
					expType.BaseType = BaseType::Int3;
				else if (TypeName == "ivec4")
					expType.BaseType = BaseType::Int4;
				else if (TypeName == "vec2")
					expType.BaseType = BaseType::Float2;
				else if (TypeName == "vec3")
					expType.BaseType = BaseType::Float3;
				else if (TypeName == "vec4")
					expType.BaseType = BaseType::Float4;
				else if (TypeName == "mat3" || TypeName == L"mat3x3")
					expType.BaseType = BaseType::Float3x3;
				else if (TypeName == "mat4" || TypeName == L"mat4x4")
					expType.BaseType = BaseType::Float4x4;
				else if (TypeName == L"sampler2D")
					expType.BaseType = BaseType::Texture2D;
				else if (TypeName == L"samplerCube")
					expType.BaseType = BaseType::TextureCube;
				else if (TypeName == L"sampler2DShadow")
					expType.BaseType = BaseType::TextureShadow;
				else if (TypeName == L"samplerCubeShadow")
					expType.BaseType = BaseType::TextureCubeShadow;
				else if (TypeName == "void")
					expType.BaseType = BaseType::Void;
				expType.ArrayLength = ArrayLength;
				expType.IsArray = IsArray;
				return expType;
			}
			virtual TypeSyntaxNode * Clone(CloneContext & ctx)
			{
				return CloneSyntaxNodeFields(new TypeSyntaxNode(*this), ctx);
			}
		};


		enum class ExpressionAccess
		{
			Read, Write
		};

		class ExpressionSyntaxNode : public SyntaxNode
		{
		public:
			ExpressionType Type;
			ExpressionAccess Access;
			ExpressionSyntaxNode()
			{
				Access = ExpressionAccess::Read;
			}
			ExpressionSyntaxNode(const ExpressionSyntaxNode & expr) = default;
		};

		class ParameterSyntaxNode : public SyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> Type;
			String Name;
			RefPtr<ExpressionSyntaxNode> Expr;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual ParameterSyntaxNode * Clone(CloneContext & ctx);
		};

		class ChoiceValueSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			String WorldName, AlternateName;
			virtual void Accept(SyntaxVisitor *) {}
			virtual ChoiceValueSyntaxNode * Clone(CloneContext & ctx);
		};

		class VarExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			String Variable;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual VarExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		class ConstantExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			enum class ConstantType
			{
				Int, Float
			};
			ConstantType ConstType;
			union
			{
				int IntValue;
				float FloatValue;
			};
			virtual void Accept(SyntaxVisitor * visitor);
			virtual ConstantExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		enum class Operator
		{
			Neg, Not, BitNot, PreInc, PreDec, PostInc, PostDec,
			Mul, Div, Mod,
			Add, Sub, 
			Lsh, Rsh,
			Eql, Neq, Greater, Less, Geq, Leq,
			BitAnd, BitXor, BitOr,
			And,
			Or,
			Assign, AddAssign, SubAssign, MulAssign, DivAssign, ModAssign
		};
		
		class UnaryExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			Operator Operator;
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual UnaryExpressionSyntaxNode * Clone(CloneContext & ctx);
		};
		
		class BinaryExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			Operator Operator;
			RefPtr<ExpressionSyntaxNode> LeftExpression;
			RefPtr<ExpressionSyntaxNode> RightExpression;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual BinaryExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		class IndexExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> BaseExpression;
			RefPtr<ExpressionSyntaxNode> IndexExpression;
			virtual IndexExpressionSyntaxNode * Clone(CloneContext & ctx);
			virtual void Accept(SyntaxVisitor * visitor);
		};

		class MemberExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> BaseExpression;
			String MemberName;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual MemberExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		class InvokeExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<VarExpressionSyntaxNode> FunctionExpr;
			List<RefPtr<ExpressionSyntaxNode>> Arguments;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual InvokeExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		class TypeCastExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> TargetType;
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual TypeCastExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		class SelectExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> SelectorExpr, Expr0, Expr1;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual SelectExpressionSyntaxNode * Clone(CloneContext & ctx);
		};

		class StatementSyntaxNode : public SyntaxNode
		{
		};

		class EmptyStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			virtual void Accept(SyntaxVisitor * visitor);
			virtual EmptyStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class BlockStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			List<RefPtr<StatementSyntaxNode>> Statements;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual BlockStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class VariableDeclr
		{
		public:
			ExpressionType Type;
			String Name;

			bool operator ==(const VariableDeclr & var)
			{
				return Name == var.Name;
			}
			bool operator ==(const String & name)
			{
				return name == Name;
			}
		};
		class FunctionSyntaxNode : public SyntaxNode
		{
		public:
			String Name, InternalName;
			RefPtr<TypeSyntaxNode> ReturnType;
			List<RefPtr<ParameterSyntaxNode>> Parameters;
			RefPtr<BlockStatementSyntaxNode> Body;
			List<VariableDeclr> Variables;
			bool IsInline;
			bool IsExtern;
			bool HasSideEffect;
			virtual void Accept(SyntaxVisitor * visitor);
			FunctionSyntaxNode()
			{
				IsInline = false;
				IsExtern = false;
				HasSideEffect = true;
			}

			virtual FunctionSyntaxNode * Clone(CloneContext & ctx);
		};

		struct Variable : public SyntaxNode
		{
			String Name;
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual Variable * Clone(CloneContext & ctx);
		};

		class VarDeclrStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> Type;
			String LayoutString;
			List<RefPtr<Variable>> Variables;
			virtual void Accept(SyntaxVisitor * visitor);
			virtual VarDeclrStatementSyntaxNode * Clone(CloneContext & ctx);
		};

		class RateWorld
		{
		public:
			Token World;
			bool Pinned = false;
		};

		class RateSyntaxNode : public SyntaxNode
		{
		public:
			List<RateWorld> Worlds;
			virtual void Accept(SyntaxVisitor *) override {}
			virtual RateSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ShaderMemberNode : public SyntaxNode
		{};

		class ComponentSyntaxNode : public ShaderMemberNode
		{
		public:
			bool IsOutput = false, IsPublic = false, IsInline = false, IsParam = false;
			RefPtr<TypeSyntaxNode> Type;
			RefPtr<RateSyntaxNode> Rate;
			Token Name, AlternateName;
			EnumerableDictionary<String, String> LayoutAttributes;
			RefPtr<BlockStatementSyntaxNode> BlockStatement;
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual ComponentSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class WorldSyntaxNode : public SyntaxNode
		{
		public:
			bool IsAbstract = false;
			Token Name;
			Token ExportOperator;
			String TargetMachine;
			List<Token> Usings;
			EnumerableDictionary<String, String> LayoutAttributes;
			virtual void Accept(SyntaxVisitor *) override {}
			virtual WorldSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ImportOperatorDefSyntaxNode : public SyntaxNode
		{
		public:
			Token Name;
			Token SourceWorld, DestWorld;
			List<Token> Usings;
			EnumerableDictionary<String, String> LayoutAttributes;
			EnumerableDictionary<String, String> Arguments;
			virtual void Accept(SyntaxVisitor *) override {}
			virtual ImportOperatorDefSyntaxNode * Clone(CloneContext & ctx) override;
		};
		
		class PipelineSyntaxNode : public SyntaxNode
		{
		public:
			Token Name;
			List<RefPtr<WorldSyntaxNode>> Worlds;
			List<RefPtr<ImportOperatorDefSyntaxNode>> ImportOperators;
			List<RefPtr<ComponentSyntaxNode>> AbstractComponents;
			virtual void Accept(SyntaxVisitor *) override {}
			virtual PipelineSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ImportArgumentSyntaxNode : public SyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Expression;
			Token ArgumentName;
			virtual void Accept(SyntaxVisitor *) override;
			virtual ImportArgumentSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ImportSyntaxNode : public ShaderMemberNode
		{
		public:
			bool IsInplace = false;
			bool IsPublic = false;
			Token ShaderName;
			Token ObjectName;
			List<RefPtr<ImportArgumentSyntaxNode>> Arguments;
			virtual void Accept(SyntaxVisitor *) override;
			virtual ImportSyntaxNode * Clone(CloneContext & ctx) override;

		};

		class ShaderSyntaxNode : public SyntaxNode
		{
		public:
			Token Name;
			Token Pipeline;
			List<RefPtr<ShaderMemberNode>> Members;
			bool IsModule = false;
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual ShaderSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ProgramSyntaxNode : public SyntaxNode
		{
		public:
			List<Token> Usings;
			List<RefPtr<FunctionSyntaxNode>> Functions;
			List<RefPtr<PipelineSyntaxNode>> Pipelines;
			List<RefPtr<ShaderSyntaxNode>> Shaders;
			void Include(ProgramSyntaxNode * other)
			{
				Functions.AddRange(other->Functions);
				Pipelines.AddRange(other->Pipelines);
				Shaders.AddRange(other->Shaders);
			}
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual ProgramSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ImportStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ImportSyntaxNode> Import;
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual ImportStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class IfStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Predicate;
			RefPtr<StatementSyntaxNode> PositiveStatement;
			RefPtr<StatementSyntaxNode> NegativeStatement;
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual IfStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ForStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> TypeDef;
			Token IterationVariable;

			RefPtr<ExpressionSyntaxNode> InitialExpression, StepExpression, EndExpression;
			RefPtr<StatementSyntaxNode> Statement;
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual ForStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class WhileStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Predicate;
			RefPtr<StatementSyntaxNode> Statement;
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual WhileStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class DoWhileStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<StatementSyntaxNode> Statement;
			RefPtr<ExpressionSyntaxNode> Predicate;
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual DoWhileStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class BreakStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual BreakStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ContinueStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual ContinueStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ReturnStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual ReturnStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ExpressionStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual void Accept(SyntaxVisitor * visitor) override;
			virtual ExpressionStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class SyntaxVisitor : public Object
		{
		protected:
			ErrorWriter * err = nullptr;
			void Error(int id, const String & text, SyntaxNode * node)
			{
				err->Error(id, text, node->Position);
			}
			void Error(int id, const String & text, Token node)
			{
				err->Error(id, text, node.Position);
			}
			void Warning(int id, const String & text, SyntaxNode * node)
			{
				err->Warning(id, text, node->Position);
			}
			void Warning(int id, const String & text, Token node)
			{
				err->Warning(id, text, node.Position);
			}
		public:
			SyntaxVisitor(ErrorWriter * pErr)
				: err(pErr)
			{}
			virtual void VisitProgram(ProgramSyntaxNode * program)
			{
				program->Functions.ForEach([&](RefPtr<FunctionSyntaxNode> f){f->Accept(this);});
			}
			virtual void VisitShader(ShaderSyntaxNode * shader)
			{
				for (auto & comp : shader->Members)
					comp->Accept(this);
			}
			virtual void VisitComponent(ComponentSyntaxNode * comp)
			{
				if (comp->Expression)
					comp->Expression->Accept(this);
				if (comp->BlockStatement)
					comp->BlockStatement->Accept(this);
			}
			virtual void VisitFunction(FunctionSyntaxNode* func)
			{
				func->ReturnType->Accept(this);
				for (auto & param : func->Parameters)
					param->Accept(this);
				if (func->Body)
					func->Body->Accept(this);
			}
			virtual void VisitBlockStatement(BlockStatementSyntaxNode* stmt)
			{
				for (auto & s : stmt->Statements)
					s->Accept(this);
			}
			virtual void VisitBreakStatement(BreakStatementSyntaxNode*){}
			virtual void VisitContinueStatement(ContinueStatementSyntaxNode*){}

			virtual void VisitDoWhileStatement(DoWhileStatementSyntaxNode* stmt)
			{
				if (stmt->Predicate)
					stmt->Predicate->Accept(this);
				if (stmt->Statement)
					stmt->Statement->Accept(this);
			}
			virtual void VisitEmptyStatement(EmptyStatementSyntaxNode*){}
			virtual void VisitForStatement(ForStatementSyntaxNode* stmt)
			{
				if (stmt->InitialExpression)
					stmt->InitialExpression->Accept(this);
				if (stmt->StepExpression)
					stmt->StepExpression->Accept(this);
				if (stmt->EndExpression)
					stmt->EndExpression->Accept(this);
				if (stmt->Statement)
					stmt->Statement->Accept(this);
			}
			virtual void VisitIfStatement(IfStatementSyntaxNode* stmt)
			{
				if (stmt->Predicate)
					stmt->Predicate->Accept(this);
				if (stmt->PositiveStatement)
					stmt->PositiveStatement->Accept(this);
				if (stmt->NegativeStatement)
					stmt->NegativeStatement->Accept(this);
			}
			virtual void VisitReturnStatement(ReturnStatementSyntaxNode* stmt)
			{
				if (stmt->Expression)
					stmt->Expression->Accept(this);
			}
			virtual void VisitVarDeclrStatement(VarDeclrStatementSyntaxNode* stmt)
			{
				for (auto & var : stmt->Variables)
					var->Accept(this);
			}
			virtual void VisitWhileStatement(WhileStatementSyntaxNode* stmt)
			{
				if (stmt->Predicate)
					stmt->Predicate->Accept(this);
				if (stmt->Statement)
					stmt->Statement->Accept(this);
			}
			virtual void VisitExpressionStatement(ExpressionStatementSyntaxNode* stmt)
			{
				if (stmt->Expression)
					stmt->Expression->Accept(this);
			}

			virtual void VisitBinaryExpression(BinaryExpressionSyntaxNode* expr)
			{
				if (expr->LeftExpression)
					expr->LeftExpression->Accept(this);
				if (expr->RightExpression)
					expr->RightExpression->Accept(this);
			}
			virtual void VisitConstantExpression(ConstantExpressionSyntaxNode*) {}
			virtual void VisitIndexExpression(IndexExpressionSyntaxNode* expr)
			{
				if (expr->BaseExpression)
					expr->BaseExpression->Accept(this);
				if (expr->IndexExpression)
					expr->IndexExpression->Accept(this);
			}
			virtual void VisitMemberExpression(MemberExpressionSyntaxNode * stmt)
			{
				if (stmt->BaseExpression)
					stmt->BaseExpression->Accept(this);
			}
			virtual void VisitInvokeExpression(InvokeExpressionSyntaxNode* stmt)
			{
				for (auto & arg : stmt->Arguments)
					arg->Accept(this);
			}
			virtual void VisitTypeCastExpression(TypeCastExpressionSyntaxNode * stmt)
			{
				if (stmt->Expression)
					stmt->Expression->Accept(this);
			}
			virtual void VisitSelectExpression(SelectExpressionSyntaxNode * expr)
			{
				if (expr->SelectorExpr)
					expr->SelectorExpr->Accept(this);
				if (expr->Expr0)
					expr->Expr0->Accept(this);
				if (expr->Expr1)
					expr->Expr1->Accept(this);
			}
			virtual void VisitUnaryExpression(UnaryExpressionSyntaxNode* expr)
			{
				if (expr->Expression)
					expr->Expression->Accept(this);
			}
			virtual void VisitVarExpression(VarExpressionSyntaxNode*){}
			virtual void VisitParameter(ParameterSyntaxNode*){}
			virtual void VisitType(TypeSyntaxNode*){}
			virtual void VisitDeclrVariable(Variable* dclr)
			{
				if (dclr->Expression)
					dclr->Expression->Accept(this);
			}
			virtual void VisitImport(ImportSyntaxNode* imp)
			{
				for (auto & arg : imp->Arguments)
					if (arg->Expression)
						arg->Expression->Accept(this);
			}
			virtual void VisitImportStatement(ImportStatementSyntaxNode* stmt)
			{
				if (stmt->Import)
					stmt->Import->Accept(this);
			}
			virtual void VisitImportArgument(ImportArgumentSyntaxNode * arg)
			{
				if (arg->Expression)
					arg->Expression->Accept(this);
			}

		};
	}
}

#endif