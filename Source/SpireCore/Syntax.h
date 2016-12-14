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
			UInt = 512, UInt2 = 513, UInt3 = 514, UInt4 = 515,
			Bool = 128, Bool2 = 129, Bool3 = 130, Bool4 = 131,
			Float3x3 = 40, Float4x4 = 47,
			Texture2D = 48,
			TextureCube = 49,
			Texture2DArray = 50,
			Texture2DShadow = 51,
			TextureCubeShadow = 52,
			Texture2DArrayShadow = 53,
			Texture3D = 54,
			SamplerState = 4096,
			Function = 64,
			Shader = 256,
			Struct = 1024,
			Record = 2048,
			Generic = 8192,
			Error = 16384,
		};

		inline const char * BaseTypeToString(BaseType t)
		{
			switch (t)
			{
			case BaseType::Void:
				return "void";
			case BaseType::Bool:
			case BaseType::Int:
				return "int";
			case BaseType::Int2:
				return "int2";
			case BaseType::Int3:
				return "int3";
			case BaseType::Int4:
				return "int4";
			case BaseType::Float:
				return "float";
			case BaseType::Float2:
				return "float2";
			case BaseType::Float3:
				return "float3";
			case BaseType::Float4:
				return "float4";
			case BaseType::Float3x3:
				return "float3x3";
			case BaseType::Float4x4:
				return "float4x4";
			case BaseType::Texture2D:
				return "sampler2D";
			case BaseType::TextureCube:
				return "samplerCube";
			default:
				return "<err-type>";
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

		class SymbolTable;
		class ShaderSymbol;
		class StructSymbol;
		class ShaderClosure;
		class StructSyntaxNode;
		class ShaderComponentSymbol;
		class FunctionSymbol;
		class BasicExpressionType;
		class ArrayExpressionType;
		class GenericExpressionType;

		class ExpressionType : public Object
		{
		public:
			static RefPtr<ExpressionType> Bool;
			static RefPtr<ExpressionType> UInt;
			static RefPtr<ExpressionType> UInt2;
			static RefPtr<ExpressionType> UInt3;
			static RefPtr<ExpressionType> UInt4;
			static RefPtr<ExpressionType> Int;
			static RefPtr<ExpressionType> Int2;
			static RefPtr<ExpressionType> Int3;
			static RefPtr<ExpressionType> Int4;
			static RefPtr<ExpressionType> Float;
			static RefPtr<ExpressionType> Float2;
			static RefPtr<ExpressionType> Float3;
			static RefPtr<ExpressionType> Float4;
			static RefPtr<ExpressionType> Void;
			static RefPtr<ExpressionType> Error;
		public:
			virtual String ToString() const = 0;
			virtual bool IsIntegral() const = 0;
			virtual bool Equals(const ExpressionType * type) const = 0;
			virtual bool IsVectorType() const = 0;
			virtual bool IsArray() const = 0;
			virtual bool IsGenericType(String typeName) const = 0;
			virtual BasicExpressionType * AsBasicType() const = 0;
			virtual ArrayExpressionType * AsArrayType() const = 0;
			virtual GenericExpressionType * AsGenericType() const = 0;
			virtual ExpressionType * Clone() = 0;
			bool IsTextureOrSampler() const;
			bool IsTexture() const;
			bool IsStruct() const;
			bool IsShader() const;
			static void Init();
			static void Finalize();
		};

		class BasicExpressionType : public ExpressionType
		{
		public:
			bool IsLeftValue;
			bool IsReference;
			bool IsMaskedVector = false;
			BaseType BaseType;
			ShaderSymbol * Shader = nullptr;
			ShaderClosure * ShaderClosure = nullptr;
			FunctionSymbol * Func = nullptr;
			ShaderComponentSymbol * Component = nullptr;
			StructSymbol * Struct = nullptr;
			String RecordTypeName, GenericTypeVar;

			BasicExpressionType()
			{
				BaseType = Compiler::BaseType::Int;
				Func = 0;
				IsLeftValue = false;
				IsReference = false;
			}
			BasicExpressionType(Compiler::BaseType baseType)
			{
				BaseType = baseType;
				Func = 0;
				IsLeftValue = false;
				IsReference = false;
			}
			BasicExpressionType(ShaderSymbol * shaderSym, Compiler::ShaderClosure * closure)
			{
				this->BaseType = BaseType::Shader;
				this->ShaderClosure = closure;
				this->Shader = shaderSym;
			}
			virtual bool IsIntegral() const override;
			virtual bool Equals(const ExpressionType * type) const override;
			virtual bool IsVectorType() const override;
			virtual bool IsArray() const override;
			virtual CoreLib::Basic::String ToString() const override;
			virtual ExpressionType * Clone() override;
			virtual bool IsGenericType(String typeName) const override
			{
				return false;
			}
			virtual BasicExpressionType * AsBasicType() const override
			{
				return const_cast<BasicExpressionType*>(this);
			}
			virtual ArrayExpressionType * AsArrayType() const override
			{
				return nullptr;
			}
			virtual GenericExpressionType * AsGenericType() const override
			{
				return nullptr;
			}
		};

		class ArrayExpressionType : public ExpressionType
		{
		public:
			RefPtr<ExpressionType> BaseType;
			int ArrayLength = 0;
			virtual bool IsIntegral() const override;
			virtual bool IsArray() const override;

			virtual bool Equals(const ExpressionType * type) const override;
			virtual bool IsVectorType() const override;
			virtual CoreLib::Basic::String ToString() const override;
			virtual ExpressionType * Clone() override;
			virtual bool IsGenericType(String typeName) const override
			{
				return false;
			}
			virtual BasicExpressionType * AsBasicType() const override
			{
				return nullptr;
			}
			virtual ArrayExpressionType * AsArrayType() const override
			{
				return const_cast<ArrayExpressionType*>(this);
			}
			virtual GenericExpressionType * AsGenericType() const override
			{
				return nullptr;
			}
		};

		class GenericExpressionType : public ExpressionType
		{
		public:
			RefPtr<ExpressionType> BaseType;
			String GenericTypeName;
			virtual bool IsIntegral() const override;
			virtual bool IsArray() const override;

			virtual bool Equals(const ExpressionType * type) const override;
			virtual bool IsVectorType() const override;
			virtual CoreLib::Basic::String ToString() const override;
			virtual ExpressionType * Clone() override;
			virtual bool IsGenericType(String typeName) const override
			{
				return GenericTypeName == typeName;
			}
			virtual BasicExpressionType * AsBasicType() const override
			{
				return nullptr;
			}
			virtual ArrayExpressionType * AsArrayType() const override
			{
				return nullptr;
			}
			virtual GenericExpressionType * AsGenericType() const override
			{
				return const_cast<GenericExpressionType*>(this);
			}
		};
		
		class Type
		{
		public:
			RefPtr<ExpressionType> DataType;
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

		class SyntaxNode : public RefObject
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
				target->Tags = this->Tags;
				return target;
			}
		public:
			EnumerableDictionary<String, RefPtr<Object>> Tags;
			CodePosition Position;
			RefPtr<Scope> Scope;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) = 0;
			virtual SyntaxNode * Clone(CloneContext & ctx) = 0;
		};

		class TypeSyntaxNode : public SyntaxNode
		{
		public:
			virtual TypeSyntaxNode * Clone(CloneContext & ctx) = 0;
		};

		class BasicTypeSyntaxNode : public TypeSyntaxNode
		{
		public:
			String TypeName;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual BasicTypeSyntaxNode * Clone(CloneContext & ctx) override
			{
				return CloneSyntaxNodeFields(new BasicTypeSyntaxNode(*this), ctx);
			}
		};

		class ArrayTypeSyntaxNode : public TypeSyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> BaseType;
			int ArrayLength;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ArrayTypeSyntaxNode * Clone(CloneContext & ctx) override
			{
				auto rs = CloneSyntaxNodeFields(new ArrayTypeSyntaxNode(*this), ctx);
				rs->BaseType = BaseType->Clone(ctx);
				return rs;
			}
		};

		class GenericTypeSyntaxNode : public TypeSyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> BaseType;
			String GenericTypeName;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual GenericTypeSyntaxNode * Clone(CloneContext & ctx) override
			{
				auto rs = CloneSyntaxNodeFields(new GenericTypeSyntaxNode(*this), ctx);
				rs->BaseType = BaseType->Clone(ctx);
				return rs;
			}
		};

		class StructField : public SyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> TypeNode;
			RefPtr<ExpressionType> Type;
			Token Name;
			StructField()
			{}
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual StructField * Clone(CloneContext & ctx) override
			{
				auto rs = CloneSyntaxNodeFields(new StructField(*this), ctx);
				rs->TypeNode = TypeNode->Clone(ctx);
				return rs;
			}
		};

		class StructSyntaxNode : public SyntaxNode
		{
		public:
			List<RefPtr<StructField>> Fields;
			Token Name;
			bool IsIntrinsic = false;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			int FindField(String name)
			{
				for (int i = 0; i < Fields.Count(); i++)
				{
					if (Fields[i]->Name.Content == name)
						return i;
				}
				return -1;
			}
			virtual StructSyntaxNode * Clone(CloneContext & ctx) override
			{
				auto rs = CloneSyntaxNodeFields(new StructSyntaxNode(*this), ctx);
				rs->Fields.Clear();
				for (auto & f : Fields)
					rs->Fields.Add(f->Clone(ctx));
				return rs;
			}
		};

		enum class ExpressionAccess
		{
			Read, Write
		};

		class ExpressionSyntaxNode : public SyntaxNode
		{
		public:
			RefPtr<ExpressionType> Type;
			ExpressionAccess Access;
			ExpressionSyntaxNode()
			{
				Access = ExpressionAccess::Read;
			}
			ExpressionSyntaxNode(const ExpressionSyntaxNode & expr) = default;
			virtual ExpressionSyntaxNode* Clone(CloneContext & ctx) = 0;
		};

		class StatementSyntaxNode : public SyntaxNode
		{
		public:
			virtual StatementSyntaxNode* Clone(CloneContext & ctx) = 0;
		};

		class BlockStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			List<RefPtr<StatementSyntaxNode>> Statements;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual BlockStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		enum class ParameterQualifier
		{
			In, Out, InOut, Uniform
		};

		class ParameterSyntaxNode : public SyntaxNode
		{
		public:
			ParameterQualifier Qualifier = ParameterQualifier::In;
			RefPtr<TypeSyntaxNode> TypeNode;
			RefPtr<ExpressionType> Type;
			String Name;
			RefPtr<ExpressionSyntaxNode> Expr;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ParameterSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class FunctionSyntaxNode : public SyntaxNode
		{
		public:
			String Name, InternalName;
			RefPtr<ExpressionType> ReturnType;
			RefPtr<TypeSyntaxNode> ReturnTypeNode;
			List<RefPtr<ParameterSyntaxNode>> Parameters;
			RefPtr<BlockStatementSyntaxNode> Body;
			bool IsInline;
			bool IsExtern;
			bool HasSideEffect;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			FunctionSyntaxNode()
			{
				IsInline = false;
				IsExtern = false;
				HasSideEffect = true;
			}

			virtual FunctionSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ImportOperatorDefSyntaxNode : public SyntaxNode
		{
		public:
			Token Name;
			Token SourceWorld, DestWorld;
			List<RefPtr<ParameterSyntaxNode>> Parameters;
			RefPtr<BlockStatementSyntaxNode> Body;
			EnumerableDictionary<String, String> LayoutAttributes;
			Token TypeName;
			List<RefPtr<FunctionSyntaxNode>> Requirements;
			List<String> Usings;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ImportOperatorDefSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ChoiceValueSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			String WorldName, AlternateName;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) { return this; }
			virtual ChoiceValueSyntaxNode * Clone(CloneContext & ctx);
		};

		class VarExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			String Variable;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual VarExpressionSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ConstantExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			enum class ConstantType
			{
				Int, Bool, Float
			};
			ConstantType ConstType;
			union
			{
				int IntValue;
				float FloatValue;
			};
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ConstantExpressionSyntaxNode * Clone(CloneContext & ctx) override;
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
			Assign = 200, AddAssign, SubAssign, MulAssign, DivAssign, ModAssign,
			LshAssign, RshAssign, OrAssign, AndAssign, XorAssign
		};

		String GetOperatorFunctionName(Operator op);
		
		class ImportExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Component;
			String ComponentUniqueName; // filled by RsolveDependence
			RefPtr<ImportOperatorDefSyntaxNode> ImportOperatorDef; // filled by semantics
			List<RefPtr<ExpressionSyntaxNode>> Arguments;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ImportExpressionSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ProjectExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> BaseExpression;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ProjectExpressionSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class UnaryExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			Operator Operator;
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual UnaryExpressionSyntaxNode * Clone(CloneContext & ctx) override;
		};
		
		class BinaryExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			Operator Operator;
			RefPtr<ExpressionSyntaxNode> LeftExpression;
			RefPtr<ExpressionSyntaxNode> RightExpression;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual BinaryExpressionSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class IndexExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> BaseExpression;
			RefPtr<ExpressionSyntaxNode> IndexExpression;
			virtual IndexExpressionSyntaxNode * Clone(CloneContext & ctx) override;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
		}; 

		class MemberExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> BaseExpression;
			String MemberName;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual MemberExpressionSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class InvokeExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> FunctionExpr;
			List<RefPtr<ExpressionSyntaxNode>> Arguments;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual InvokeExpressionSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class TypeCastExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> TargetType;
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual TypeCastExpressionSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class SelectExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> SelectorExpr, Expr0, Expr1;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual SelectExpressionSyntaxNode * Clone(CloneContext & ctx) override;
		};


		class EmptyStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual EmptyStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class DiscardStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual DiscardStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class VariableDeclr
		{
		public:
			RefPtr<ExpressionType> Type;
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

		struct Variable : public SyntaxNode
		{
			String Name;
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual Variable * Clone(CloneContext & ctx) override;
		};

		class VarDeclrStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> TypeNode;
			RefPtr<ExpressionType> Type;
			String LayoutString;
			List<RefPtr<Variable>> Variables;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual VarDeclrStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class RateWorld
		{
		public:
			Token World;
			bool Pinned = false;
			RateWorld() {}
			RateWorld(String world)
			{
				World.Content = world;
				World.Type = TokenType::Identifier;
			}
		};

		class RateSyntaxNode : public SyntaxNode
		{
		public:
			List<RateWorld> Worlds;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) override
			{
				return this;
			}
			virtual RateSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ShaderMemberNode : public SyntaxNode
		{
		public:
			Token ParentModuleName;
			virtual ShaderMemberNode * Clone(CloneContext & ctx) = 0;
		};

		class ComponentSyntaxNode : public ShaderMemberNode
		{
		public:
			bool IsOutput = false, IsPublic = false, IsInline = false, IsParam = false, IsInput = false;
			RefPtr<TypeSyntaxNode> TypeNode;
			RefPtr<ExpressionType> Type;
			RefPtr<RateSyntaxNode> Rate;
			Token Name, AlternateName;
			EnumerableDictionary<String, String> LayoutAttributes;
			RefPtr<BlockStatementSyntaxNode> BlockStatement;
			RefPtr<ExpressionSyntaxNode> Expression;
			List<RefPtr<ParameterSyntaxNode>> Parameters;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ComponentSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class WorldSyntaxNode : public SyntaxNode
		{
		public:
			bool IsAbstract = false;
			Token Name;
			EnumerableDictionary<String, String> LayoutAttributes;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) override { return this; }
			virtual WorldSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class StageSyntaxNode : public SyntaxNode
		{
		public:
			Token Name;
			Token StageType;
			EnumerableDictionary<String, Token> Attributes;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) override { return this; }
			virtual StageSyntaxNode * Clone(CloneContext & ctx) override;
		};
		
		class PipelineSyntaxNode : public SyntaxNode
		{
		public:
			Token Name;
			Token ParentPipeline;
			List<RefPtr<WorldSyntaxNode>> Worlds;
			List<RefPtr<ImportOperatorDefSyntaxNode>> ImportOperators;
			List<RefPtr<StageSyntaxNode>> Stages;
			List<RefPtr<ComponentSyntaxNode>> AbstractComponents;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) override { return this; }
			virtual PipelineSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ImportArgumentSyntaxNode : public SyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Expression;
			Token ArgumentName;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) override;
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
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) override;
			virtual ImportSyntaxNode * Clone(CloneContext & ctx) override;

		};

		class ShaderSyntaxNode : public SyntaxNode
		{
		public:
			Token Name;
			Token Pipeline;
			List<RefPtr<ShaderMemberNode>> Members;
			bool IsModule = false;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ShaderSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ProgramSyntaxNode : public SyntaxNode
		{
		public:
			List<Token> Usings;
			List<RefPtr<FunctionSyntaxNode>> Functions;
			List<RefPtr<PipelineSyntaxNode>> Pipelines;
			List<RefPtr<ShaderSyntaxNode>> Shaders;
			List<RefPtr<StructSyntaxNode>> Structs;
			void Include(ProgramSyntaxNode * other)
			{
				Functions.AddRange(other->Functions);
				Pipelines.AddRange(other->Pipelines);
				Shaders.AddRange(other->Shaders);
				Structs.AddRange(other->Structs);
			}
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ProgramSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ImportStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ImportSyntaxNode> Import;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ImportStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class IfStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Predicate;
			RefPtr<StatementSyntaxNode> PositiveStatement;
			RefPtr<StatementSyntaxNode> NegativeStatement;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual IfStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ForStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<TypeSyntaxNode> TypeDef;
			RefPtr<ExpressionType> IterationVariableType;
			Token IterationVariable;

			RefPtr<ExpressionSyntaxNode> InitialExpression, SideEffectExpression, PredicateExpression;
			RefPtr<StatementSyntaxNode> Statement;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ForStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class WhileStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Predicate;
			RefPtr<StatementSyntaxNode> Statement;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual WhileStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class DoWhileStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<StatementSyntaxNode> Statement;
			RefPtr<ExpressionSyntaxNode> Predicate;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual DoWhileStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class BreakStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual BreakStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ContinueStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ContinueStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ReturnStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ReturnStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ExpressionStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<ExpressionSyntaxNode> Expression;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ExpressionStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class SyntaxVisitor : public Object
		{
		protected:
			DiagnosticSink * sink = nullptr;
            DiagnosticSink* getSink() { return sink; }
		public:
			SyntaxVisitor(DiagnosticSink * sink)
				: sink(sink)
			{}
			virtual RefPtr<ProgramSyntaxNode> VisitProgram(ProgramSyntaxNode* program)
			{
				for (auto & f : program->Functions)
					f = f->Accept(this).As<FunctionSyntaxNode>();
				for (auto & shader : program->Shaders)
					shader = shader->Accept(this).As<ShaderSyntaxNode>();
				return program;
			}
			virtual RefPtr<ShaderSyntaxNode> VisitShader(ShaderSyntaxNode * shader)
			{
				for (auto & comp : shader->Members)
					comp = comp->Accept(this).As<ShaderMemberNode>();
				return shader;
			}
			virtual RefPtr<ComponentSyntaxNode> VisitComponent(ComponentSyntaxNode * comp);
			virtual RefPtr<FunctionSyntaxNode> VisitFunction(FunctionSyntaxNode* func)
			{
				func->ReturnTypeNode = func->ReturnTypeNode->Accept(this).As<TypeSyntaxNode>();
				for (auto & param : func->Parameters)
					param = param->Accept(this).As<ParameterSyntaxNode>();
				if (func->Body)
					func->Body = func->Body->Accept(this).As<BlockStatementSyntaxNode>();
				return func;
			}
			virtual RefPtr<StructSyntaxNode> VisitStruct(StructSyntaxNode * s)
			{
				for (auto & f : s->Fields)
					f = f->Accept(this).As<StructField>();
				return s;
			}
			virtual RefPtr<StatementSyntaxNode> VisitDiscardStatement(DiscardStatementSyntaxNode * stmt)
			{
				return stmt;
			}
			virtual RefPtr<StructField> VisitStructField(StructField * f)
			{
				f->TypeNode = f->TypeNode->Accept(this).As<TypeSyntaxNode>();
				return f;
			}
			virtual RefPtr<StatementSyntaxNode> VisitBlockStatement(BlockStatementSyntaxNode* stmt)
			{
				for (auto & s : stmt->Statements)
					s = s->Accept(this).As<StatementSyntaxNode>();
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitBreakStatement(BreakStatementSyntaxNode* stmt)
			{
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitContinueStatement(ContinueStatementSyntaxNode* stmt)
			{
				return stmt;
			}

			virtual RefPtr<StatementSyntaxNode> VisitDoWhileStatement(DoWhileStatementSyntaxNode* stmt)
			{
				if (stmt->Predicate)
					stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
				if (stmt->Statement)
					stmt->Statement = stmt->Statement->Accept(this).As<StatementSyntaxNode>();
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitEmptyStatement(EmptyStatementSyntaxNode* stmt)
			{
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitForStatement(ForStatementSyntaxNode* stmt)
			{
				if (stmt->InitialExpression)
					stmt->InitialExpression = stmt->InitialExpression->Accept(this).As<ExpressionSyntaxNode>();
				if (stmt->PredicateExpression)
					stmt->PredicateExpression = stmt->PredicateExpression->Accept(this).As<ExpressionSyntaxNode>();
				if (stmt->SideEffectExpression)
					stmt->SideEffectExpression = stmt->SideEffectExpression->Accept(this).As<ExpressionSyntaxNode>();
				if (stmt->Statement)
					stmt->Statement = stmt->Statement->Accept(this).As<StatementSyntaxNode>();
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitIfStatement(IfStatementSyntaxNode* stmt)
			{
				if (stmt->Predicate)
					stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
				if (stmt->PositiveStatement)
					stmt->PositiveStatement = stmt->PositiveStatement->Accept(this).As<StatementSyntaxNode>();
				if (stmt->NegativeStatement)
					stmt->NegativeStatement = stmt->NegativeStatement->Accept(this).As<StatementSyntaxNode>();
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitReturnStatement(ReturnStatementSyntaxNode* stmt)
			{
				if (stmt->Expression)
					stmt->Expression = stmt->Expression->Accept(this).As<ExpressionSyntaxNode>();
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitVarDeclrStatement(VarDeclrStatementSyntaxNode* stmt)
			{
				for (auto & var : stmt->Variables)
					var = var->Accept(this).As<Variable>();
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitWhileStatement(WhileStatementSyntaxNode* stmt)
			{
				if (stmt->Predicate)
					stmt->Predicate = stmt->Predicate->Accept(this).As<ExpressionSyntaxNode>();
				if (stmt->Statement)
					stmt->Statement = stmt->Statement->Accept(this).As<StatementSyntaxNode>();
				return stmt;
			}
			virtual RefPtr<StatementSyntaxNode> VisitExpressionStatement(ExpressionStatementSyntaxNode* stmt)
			{
				if (stmt->Expression)
					stmt->Expression = stmt->Expression->Accept(this).As<ExpressionSyntaxNode>();
				return stmt;
			}

			virtual RefPtr<ExpressionSyntaxNode> VisitBinaryExpression(BinaryExpressionSyntaxNode* expr)
			{
				if (expr->LeftExpression)
					expr->LeftExpression = expr->LeftExpression->Accept(this).As<ExpressionSyntaxNode>();
				if (expr->RightExpression)
					expr->RightExpression = expr->RightExpression->Accept(this).As<ExpressionSyntaxNode>();
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitConstantExpression(ConstantExpressionSyntaxNode* expr)
			{
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitIndexExpression(IndexExpressionSyntaxNode* expr)
			{
				if (expr->BaseExpression)
					expr->BaseExpression = expr->BaseExpression->Accept(this).As<ExpressionSyntaxNode>();
				if (expr->IndexExpression)
					expr->IndexExpression = expr->IndexExpression->Accept(this).As<ExpressionSyntaxNode>();
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitMemberExpression(MemberExpressionSyntaxNode * stmt)
			{
				if (stmt->BaseExpression)
					stmt->BaseExpression = stmt->BaseExpression->Accept(this).As<ExpressionSyntaxNode>();
				return stmt;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitInvokeExpression(InvokeExpressionSyntaxNode* stmt)
			{
				stmt->FunctionExpr->Accept(this);
				for (auto & arg : stmt->Arguments)
					arg = arg->Accept(this).As<ExpressionSyntaxNode>();
				return stmt;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitImportExpression(ImportExpressionSyntaxNode * expr)
			{
				for (auto & arg : expr->Arguments)
					arg = arg->Accept(this).As<ExpressionSyntaxNode>();
				if (expr->ImportOperatorDef)
					expr->ImportOperatorDef->Accept(this);
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitTypeCastExpression(TypeCastExpressionSyntaxNode * stmt)
			{
				if (stmt->Expression)
					stmt->Expression = stmt->Expression->Accept(this).As<ExpressionSyntaxNode>();
				return stmt->Expression;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitSelectExpression(SelectExpressionSyntaxNode * expr)
			{
				if (expr->SelectorExpr)
					expr->SelectorExpr = expr->SelectorExpr->Accept(this).As<ExpressionSyntaxNode>();
				if (expr->Expr0)
					expr->Expr0 = expr->Expr0->Accept(this).As<ExpressionSyntaxNode>();
				if (expr->Expr1)
					expr->Expr1 = expr->Expr1->Accept(this).As<ExpressionSyntaxNode>();
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitUnaryExpression(UnaryExpressionSyntaxNode* expr)
			{
				if (expr->Expression)
					expr->Expression = expr->Expression->Accept(this).As<ExpressionSyntaxNode>();
				return expr;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitVarExpression(VarExpressionSyntaxNode* expr)
			{
				return expr;
			}
			virtual RefPtr<PipelineSyntaxNode> VisitPipeline(PipelineSyntaxNode * pipe)
			{
				for (auto & comp : pipe->AbstractComponents)
					comp = comp->Accept(this).As<ComponentSyntaxNode>();
				for (auto & imp : pipe->ImportOperators)
					imp = imp->Accept(this).As<ImportOperatorDefSyntaxNode>();
				return pipe;
			}
			virtual RefPtr<ImportOperatorDefSyntaxNode> VisitImportOperatorDef(ImportOperatorDefSyntaxNode * imp)
			{
				imp->Body = imp->Body->Accept(this).As<BlockStatementSyntaxNode>();
				return imp;
			}
			virtual RefPtr<ParameterSyntaxNode> VisitParameter(ParameterSyntaxNode* param)
			{
				return param;
			}
			virtual RefPtr<TypeSyntaxNode> VisitBasicType(BasicTypeSyntaxNode* type)
			{
				return type;
			}
			virtual RefPtr<TypeSyntaxNode> VisitArrayType(ArrayTypeSyntaxNode* type)
			{
				return type;
			}
			virtual RefPtr<TypeSyntaxNode> VisitGenericType(GenericTypeSyntaxNode* type)
			{
				return type;
			}

			virtual RefPtr<Variable> VisitDeclrVariable(Variable* dclr)
			{
				if (dclr->Expression)
					dclr->Expression = dclr->Expression->Accept(this).As<ExpressionSyntaxNode>();
				return dclr;
			}
			virtual RefPtr<ImportSyntaxNode> VisitImport(ImportSyntaxNode* imp)
			{
				for (auto & arg : imp->Arguments)
					if (arg->Expression)
						arg->Expression = arg->Expression->Accept(this).As<ExpressionSyntaxNode>();
				return imp;
			}
			virtual RefPtr<StatementSyntaxNode> VisitImportStatement(ImportStatementSyntaxNode* stmt)
			{
				if (stmt->Import)
					stmt->Import = stmt->Import->Accept(this).As<ImportSyntaxNode>();
				return stmt;
			}
			virtual RefPtr<ImportArgumentSyntaxNode> VisitImportArgument(ImportArgumentSyntaxNode * arg)
			{
				if (arg->Expression)
					arg->Expression = arg->Expression->Accept(this).As<ExpressionSyntaxNode>();
				return arg;
			}
			virtual RefPtr<ExpressionSyntaxNode> VisitProject(ProjectExpressionSyntaxNode * project)
			{
				if (project->BaseExpression)
					project->BaseExpression = project->BaseExpression->Accept(this).As<ExpressionSyntaxNode>();
				return project;
			}
		};
	}
}

#endif