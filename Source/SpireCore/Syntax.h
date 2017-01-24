#ifndef RASTER_RENDERER_SYNTAX_H
#define RASTER_RENDERER_SYNTAX_H

#include "../CoreLib/Basic.h"
#include "Lexer.h"
#include "IL.h"

#include <assert.h>

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;
		class SyntaxVisitor;
		class FunctionSyntaxNode;

		// We use a unified representation for modifiers on all declarations.
		// (Eventually this will also apply to statements that support attributes)
		//
		// The parser allows any set of modifiers on any declaration, and we leave
		// it to later phases of analysis to reject inappropriate uses.
		// TODO: implement rejection properly.
		//
		// Some common modifiers (thos represented by single keywords) are
		// specified via a simple set of flags.
		// TODO: consider using a real AST even for these, so we can give good
		// error messages on confliction modifiers.
		typedef unsigned int ModifierFlags;
		enum ModifierFlag : ModifierFlags
		{
			None = 0,
			Uniform = 1 << 0,
			Out = 1 << 1,
			In = 1 << 2,
			Const = 1 << 4,
			Instance = 1 << 5,
			Builtin = 1 << 6,

			Inline = 1 << 8,
			Public = 1 << 9,
			Require = 1 << 10,
			Param = (1 << 11) | ModifierFlag::Public,
			Extern = 1 << 12,
			Input = 1 << 13,
			Intrinsic = (1 << 14) | ModifierFlag::Extern,
			// TODO(tfoley): This should probably be its own flag
			InOut = ModifierFlag::In | ModifierFlag::Out,
		};
		//
		// Other modifiers may have more elaborate data, and so
		// are represented as heap-allocated objects, in a linked
		// list.
		//
		class Modifier : public RefObject
		{
		public:
			RefPtr<Modifier> next;
		};

		// A `layout` modifier
		class LayoutModifier : public Modifier
		{
		public:
			String LayoutString;
		};

		// An attribute of the form `[Name]` or `[Name: Value]`
		class SimpleAttribute : public Modifier
		{
		public:
			String Key;
			Token Value;

			String const& GetValue() const { return Value.Content; }
		};

		// An HLSL semantic
		class HLSLSemantic : public Modifier
		{
		public:
			Token name;
		};


		// An HLSL semantic that affects layout
		class HLSLLayoutSemantic : public HLSLSemantic
		{
		public:
			Token registerName;
			Token componentMask;
		};

		// An HLSL `register` semantic
		class HLSLRegisterSemantic : public HLSLLayoutSemantic
		{
		};

		// TODO(tfoley): `packoffset`
		class HLSLPackOffsetSemantic : public HLSLLayoutSemantic
		{
		};

		// An HLSL semantic that just associated a declaration with a semantic name
		class HLSLSimpleSemantic : public HLSLSemantic
		{
		};

		// A set of modifiers attached to a syntax node
		struct Modifiers
		{
			// The first modifier in the linked list of heap-allocated modifiers
			RefPtr<Modifier> first;

			// The bit-flags for the common modifiers
			ModifierFlags flags = ModifierFlag::None;
		};

		// Helper class for iterating over a list of heap-allocated modifiers
		struct ModifierList
		{
			struct Iterator
			{
				Modifier* current;

				Modifier* operator*()
				{
					return current;
				}

				void operator++()
				{
					current = current->next.Ptr();
				}

				bool operator!=(Iterator other)
				{
					return current != other.current;
				};

				Iterator()
					: current(nullptr)
				{}

				Iterator(Modifier* modifier)
					: current(modifier)
				{}
			};

			ModifierList()
				: modifiers(nullptr)
			{}

			ModifierList(Modifier* modifiers)
				: modifiers(modifiers)
			{}

			Iterator begin() { return Iterator(modifiers); }
			Iterator end() { return Iterator(nullptr); }

			Modifier* modifiers;
		};

		// Helper class for iterating over heap-allocated modifiers
		// of a specific type.
		template<typename T>
		struct FilteredModifierList
		{
			struct Iterator
			{
				Modifier* current;

				T* operator*()
				{
					return (T*)current;
				}

				void operator++()
				{
					current = Adjust(current->next.Ptr());
				}

				bool operator!=(Iterator other)
				{
					return current != other.current;
				};

				Iterator()
					: current(nullptr)
				{}

				Iterator(Modifier* modifier)
					: current(modifier)
				{}
			};

			FilteredModifierList()
				: modifiers(nullptr)
			{}

			FilteredModifierList(Modifier* modifiers)
				: modifiers(Adjust(modifiers))
			{}

			Iterator begin() { return Iterator(modifiers); }
			Iterator end() { return Iterator(nullptr); }

			static Modifier* Adjust(Modifier* modifier)
			{
				Modifier* m = modifier;
				for (;;)
				{
					if (!m) return m;
					if (dynamic_cast<T*>(m)) return m;
					m = m->next.Ptr();
				}
			}

			Modifier* modifiers;
		};

		enum class BaseType
		{
			Void = 0,
			Int = 16,
			Float = 32,
			UInt = 512,
			Bool = 128,
#if 0
			Texture2D = 48,
			TextureCube = 49,
			Texture2DArray = 50,
			Texture2DShadow = 51,
			TextureCubeShadow = 52,
			Texture2DArrayShadow = 53,
			Texture3D = 54,
			SamplerState = 4096, SamplerComparisonState = 4097,
#endif
			Error = 16384,
		};

		class Decl;
		class SymbolTable;
		class ShaderSymbol;
		class ShaderClosure;
		class StructSyntaxNode;
		class ShaderComponentSymbol;
		class FunctionSymbol;
		class BasicExpressionType;
		class ArrayExpressionType;
		class GenericExpressionType;
		class TypeDefDecl;
		class DeclRefType;
		class NamedExpressionType;
		class TypeExpressionType;
		class VectorExpressionType;
		class MatrixExpressionType;
		class ArithmeticExpressionType;

		class ExpressionType : public RefObject
		{
		public:
			static RefPtr<ExpressionType> Bool;
			static RefPtr<ExpressionType> UInt;
			static RefPtr<ExpressionType> Int;
			static RefPtr<ExpressionType> Float;
			static RefPtr<ExpressionType> Float2;
			static RefPtr<ExpressionType> Void;
			static RefPtr<ExpressionType> Error;
			// Note: just exists to make sure we can clean up
			// canonical types we create along the way
			static List<RefPtr<ExpressionType>> sCanonicalTypes;
		public:
			virtual String ToString() const = 0;

			bool IsIntegral() const;
			bool Equals(const ExpressionType * type) const;
			bool Equals(RefPtr<ExpressionType> type) const;

			bool IsVectorType() const;
			bool IsArray() const;
			bool IsGenericType(String typeName) const;

			template<typename T>
			T* As() const
			{
				return dynamic_cast<T*>(GetCanonicalType());
			}

			ArithmeticExpressionType * AsArithmeticType() const;
			BasicExpressionType * AsBasicType() const;
			VectorExpressionType * AsVectorType() const;
			MatrixExpressionType * AsMatrixType() const;
			ArrayExpressionType * AsArrayType() const;
			GenericExpressionType * AsGenericType() const;
			DeclRefType* AsDeclRefType() const;
			NamedExpressionType* AsNamedType() const;
			TypeExpressionType* AsTypeType() const;
			bool IsTextureOrSampler() const;
			bool IsTexture() const;
			bool IsSampler() const;
			bool IsStruct() const;
			bool IsShader() const;
			static void Init();
			static void Finalize();
			ExpressionType* GetCanonicalType() const;
			BindableResourceType GetBindableResourceType() const;
		protected:
			virtual bool IsIntegralImpl() const { return false; }
			virtual bool EqualsImpl(const ExpressionType * type) const = 0;
			virtual bool IsVectorTypeImpl() const { return false; }
			virtual bool IsArrayImpl() const { return false; }
			virtual bool IsGenericTypeImpl(String typeName) const { return nullptr; }
			virtual ArithmeticExpressionType * AsArithmeticTypeImpl() const { return nullptr; }
			virtual BasicExpressionType * AsBasicTypeImpl() const { return nullptr; }
			virtual VectorExpressionType * AsVectorTypeImpl() const { return nullptr; }
			virtual MatrixExpressionType * AsMatrixTypeImpl() const { return nullptr; }
			virtual ArrayExpressionType * AsArrayTypeImpl() const { return nullptr; }
			virtual GenericExpressionType * AsGenericTypeImpl() const { return nullptr; }
			virtual DeclRefType * AsDeclRefTypeImpl() const { return nullptr; }
			virtual NamedExpressionType * AsNamedTypeImpl() const { return nullptr; }
			virtual TypeExpressionType * AsTypeTypeImpl() const { return nullptr; }

			virtual ExpressionType* CreateCanonicalType() = 0;
			ExpressionType* canonicalType = nullptr;
		};

		// A type that takes the form of a reference to some declaration
		class DeclRefType : public ExpressionType
		{
		public:
			DeclRefType()
			{}
			DeclRefType(Decl* decl)
				: decl(decl)
			{}

			Decl* decl = nullptr;

			virtual String ToString() const override;

		protected:
			virtual bool EqualsImpl(const ExpressionType * type) const override;
			virtual DeclRefType * AsDeclRefTypeImpl() const override;
			virtual ExpressionType* CreateCanonicalType() override;
		};

		// Base class for types that can be used in arithmetic expressions
		class ArithmeticExpressionType : public DeclRefType
		{
		public:
			virtual BasicExpressionType* GetScalarType() const = 0;

		protected:
			virtual ArithmeticExpressionType * AsArithmeticTypeImpl() const override;
		};

		class FunctionDeclBase;

		// Function types are currently used for references to symbols that name
		// either ordinary functions, or "component functions."
		// We do not directly store a representation of the type, and instead
		// use a reference to the symbol to stand in for its logical type
		class FuncType : public ExpressionType
		{
		public:
			ShaderComponentSymbol * Component = nullptr;
			FunctionSymbol * Func = nullptr;
			FunctionDeclBase* decl = nullptr;

			virtual String ToString() const override;
		protected:
			virtual bool EqualsImpl(const ExpressionType * type) const override;
			virtual ExpressionType* CreateCanonicalType() override;
		};

		// The type of a shader symbol.
		class ShaderType : public ExpressionType
		{
		public:
			ShaderSymbol * Shader = nullptr;
			ShaderClosure * ShaderClosure = nullptr;

			ShaderType(ShaderSymbol * shaderSym, Compiler::ShaderClosure * closure)
			{
				this->ShaderClosure = closure;
				this->Shader = shaderSym;
			}

			virtual String ToString() const override;
		protected:
			virtual bool EqualsImpl(const ExpressionType * type) const override;
			virtual ExpressionType* CreateCanonicalType() override;
		};

		// A reference to the generic type parameter of an import operator
		class ImportOperatorGenericParamType : public ExpressionType
		{
		public:
			String GenericTypeVar;

			ImportOperatorGenericParamType(String genericTypeVar)
				: GenericTypeVar(genericTypeVar)
			{}

			virtual String ToString() const override;
		protected:
			virtual bool EqualsImpl(const ExpressionType * type) const override;
			virtual ExpressionType* CreateCanonicalType() override;
		};

		class BasicExpressionType : public ArithmeticExpressionType
		{
		public:
			BaseType BaseType;

			BasicExpressionType()
			{
				BaseType = Compiler::BaseType::Int;
			}
			BasicExpressionType(Compiler::BaseType baseType)
			{
				BaseType = baseType;
			}
			virtual CoreLib::Basic::String ToString() const override;
		protected:
			virtual BasicExpressionType* GetScalarType() const override;
			virtual bool IsIntegralImpl() const override;
			virtual bool EqualsImpl(const ExpressionType * type) const override;
			virtual BasicExpressionType * AsBasicTypeImpl() const override
			{
				return const_cast<BasicExpressionType*>(this);
			}
			virtual ExpressionType* CreateCanonicalType() override;
		};


		class TextureType : public DeclRefType
		{
		public:
			// The type that results from fetching an element from this texture
			RefPtr<ExpressionType> elementType;

			// Bits representing the kind of texture type we are looking at
			// (e.g., `Texture2DMS` vs. `TextureCubeArray`)
			uint16_t flavor;
			enum
			{
				// Mask for the overall "shape" of the texture
				ShapeMask		= 0x0F,

				// Flag for whether the shape has "array-ness"
				ArrayFlag		= 0x80,

				// Whether or not the texture stores multiple samples per pixel
				MultisampleFlag	= 0x10,

				// Whether or not this is a shadow texture
				//
				// TODO(tfoley): is this even meaningful/used?
				ShadowFlag		= 0x20, 
			};

			enum Shape : uint8_t
			{
				Shape1D			= 0x01,
				Shape2D			= 0x02,
				Shape3D			= 0x03,
				ShapeCube		= 0x04,

				Shape1DArray	= Shape1D | ArrayFlag,
				Shape2DArray	= Shape2D | ArrayFlag,
				// No Shape3DArray
				ShapeCubeArray	= ShapeCube | ArrayFlag,
			};

			

			Shape GetBaseShape() const { return Shape(flavor & ShapeMask); }
			bool isArray() const { return (flavor & ArrayFlag) != 0; }
			bool isMultisample() const { return (flavor & MultisampleFlag) != 0; }
			bool isShadow() const { return (flavor & ShadowFlag) != 0; }

			TextureType(
				uint16_t flavor,
				RefPtr<ExpressionType> elementType)
				: elementType(elementType)
				, flavor(flavor)
			{}
		};

		class SamplerStateType : public DeclRefType
		{
		public:
			// What flavor of sampler state is this
			enum class Flavor : uint8_t
			{
				SamplerState,
				SamplerComparisonState,
			};
			Flavor flavor;
		};

		class ArrayExpressionType : public ExpressionType
		{
		public:
			RefPtr<ExpressionType> BaseType;
			int ArrayLength = 0;
			virtual CoreLib::Basic::String ToString() const override;
		protected:
			virtual bool IsArrayImpl() const override;
			virtual bool EqualsImpl(const ExpressionType * type) const override;
			virtual ArrayExpressionType * AsArrayTypeImpl() const override
			{
				return const_cast<ArrayExpressionType*>(this);
			}
			virtual ExpressionType* CreateCanonicalType() override;
		};

		class GenericExpressionType : public ExpressionType
		{
		public:
			RefPtr<ExpressionType> BaseType;
			String GenericTypeName;
			virtual CoreLib::Basic::String ToString() const override;
		protected:
			virtual bool EqualsImpl(const ExpressionType * type) const override;
			virtual bool IsGenericTypeImpl(String typeName) const override
			{
				return GenericTypeName == typeName;
			}
			virtual GenericExpressionType * AsGenericTypeImpl() const override
			{
				return const_cast<GenericExpressionType*>(this);
			}
			virtual ExpressionType* CreateCanonicalType() override;
		};



		// A type alias of some kind (e.g., via `typedef`)
		class NamedExpressionType : public ExpressionType
		{
		public:
			NamedExpressionType(TypeDefDecl* decl)
				: decl(decl)
			{}

			TypeDefDecl* decl;

			virtual String ToString() const override;

		protected:
			virtual bool EqualsImpl(const ExpressionType * type) const override;
			virtual NamedExpressionType * AsNamedTypeImpl() const override;
			virtual ExpressionType* CreateCanonicalType() override;
		};

		// The "type" of an expression that resolves to a type.
		// For example, in the expression `float(2)` the sub-expression,
		// `float` would have the type `TypeType(float)`.
		class TypeExpressionType : public ExpressionType
		{
		public:
			TypeExpressionType(RefPtr<ExpressionType> type)
				: type(type)
			{}

			// The type that this is the type of...
			RefPtr<ExpressionType> type;


			virtual String ToString() const override;

		protected:
			virtual bool EqualsImpl(const ExpressionType * type) const override;
			virtual TypeExpressionType * AsTypeTypeImpl() const override;
			virtual ExpressionType* CreateCanonicalType() override;
		};

		class GenericDecl;

		// The "type" of an expression that names a generic declaration.
		class GenericDeclRefType : public ExpressionType
		{
		public:
			GenericDeclRefType(GenericDecl* decl)
				: decl(decl)
			{}

			GenericDecl* decl;

			virtual String ToString() const override;

		protected:
			virtual bool EqualsImpl(const ExpressionType * type) const override;
			virtual ExpressionType* CreateCanonicalType() override;
		};

		// A vector type, e.g., `vector<T,N>`
		class VectorExpressionType : public ArithmeticExpressionType
		{
		public:
			VectorExpressionType(
				RefPtr<ExpressionType>	elementType,
				int						elementCount)
				: elementType(elementType)
				, elementCount(elementCount)
			{}

			// The type of vector elements.
			// As an invariant, this should be a basic type or an alias.
			RefPtr<ExpressionType>	elementType;

			// The number of elements
			//
			// TODO(tfoley): If we start allowing generics, then this might
			// need to be a symbolic "constant" expression, and not just
			// a literal value.
			int						elementCount;

			virtual String ToString() const override;

		protected:
			virtual BasicExpressionType* GetScalarType() const override;
			virtual bool EqualsImpl(const ExpressionType * type) const override;
			virtual VectorExpressionType * AsVectorTypeImpl() const override;
			virtual ExpressionType* CreateCanonicalType() override;
		};

		// A matrix type, e.g., `matrix<T,R,C>`
		class MatrixExpressionType : public ArithmeticExpressionType
		{
		public:
			MatrixExpressionType(
				RefPtr<ExpressionType>	elementType,
				int						rowCount,
				int						colCount)
				: elementType(elementType)
				, rowCount(rowCount)
				, colCount(colCount)
			{}

			// The type of vector elements.
			// As an invariant, this should be a basic type or an alias.
			RefPtr<ExpressionType>	elementType;

			// The type of the matrix rows
			RefPtr<VectorExpressionType>	rowType;

			// The number of rows and columns
			int								rowCount;
			int								colCount;

			virtual String ToString() const override;

		protected:
			virtual BasicExpressionType* GetScalarType() const override;
			virtual bool EqualsImpl(const ExpressionType * type) const override;
			virtual MatrixExpressionType * AsMatrixTypeImpl() const override;
			virtual ExpressionType* CreateCanonicalType() override;

		};

		inline BaseType GetVectorBaseType(VectorExpressionType* vecType) {
			return vecType->elementType->AsBasicType()->BaseType;
		}

		inline int GetVectorSize(VectorExpressionType* vecType) {
			return vecType->elementCount;
		}

		class Type
		{
		public:
			RefPtr<ExpressionType> DataType;
			// ContrainedWorlds: Implementation must be defined at at least one of of these worlds in order to satisfy global dependency
			// FeasibleWorlds: The component can be computed at any of these worlds
			EnumerableHashSet<String> ConstrainedWorlds, FeasibleWorlds;
			EnumerableHashSet<String> PinnedWorlds;
		};

		class ContainerDecl;
		class Scope : public RefObject
		{
		public:
			RefPtr<Scope> Parent;
			ContainerDecl* containerDecl;
			Dictionary<String, Decl*> decls;
			Decl* LookUp(String const& name);
			Scope(RefPtr<Scope> parent, ContainerDecl* containerDecl)
				: Parent(parent)
				, containerDecl(containerDecl)
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
						if (ctx.ScopeTranslateTable.TryGetValue(target->Scope->Parent.Ptr(), parentScope))
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

		class ContainerDecl;
		class SpecializeModifier;

		// Represents how much checking has been applied to a declaration.
		enum class DeclCheckState : uint8_t
		{
			// The declaration has been parsed, but not checked
			Unchecked,

			// We are in the process of checking the declaration "header"
			// (those parts of the declaration needed in order to
			// reference it)
			CheckingHeader,

			// We are done checking the declaration header.
			CheckedHeader,

			// We have checked the declaration fully.
			Checked,
		};

		class Decl : public SyntaxNode
		{
		public:
			ContainerDecl*  ParentDecl;
			Token Name;
			Modifiers modifiers;
			DeclCheckState checkState = DeclCheckState::Unchecked;

			bool HasModifier(ModifierFlags flags) { return (modifiers.flags & flags) == flags; }

			template<typename T>
			FilteredModifierList<T> GetModifiersOfType() { return FilteredModifierList<T>(modifiers.first.Ptr()); }

			// Find the first modifier of a given type, or return `nullptr` if none is found.
			template<typename T>
			T* FindModifier()
			{
				for (auto m : GetModifiersOfType<T>())
					return m;
				return nullptr;
			}

			FilteredModifierList<SimpleAttribute> GetLayoutAttributes() { return GetModifiersOfType<SimpleAttribute>(); }

			bool FindSimpleAttribute(String const& key, Token& outValue);
			bool FindSimpleAttribute(String const& key, String& outValue);
			bool HasSimpleAttribute(String const& key);
			SpecializeModifier * FindSpecializeModifier();

			bool IsChecked(DeclCheckState state) { return checkState >= state; }
			void SetCheckState(DeclCheckState state)
			{
				assert(state >= checkState);
				checkState = state;
			}

			virtual Decl * Clone(CloneContext & ctx) = 0;
		};

		template<typename T>
		struct FilteredMemberList
		{
			typedef RefPtr<Decl> Element;

			FilteredMemberList()
				: mBegin(NULL)
				, mEnd(NULL)
			{}

			explicit FilteredMemberList(
				List<Element> const& list)
				: mBegin(Adjust(list.begin(), list.end()))
				, mEnd(list.end())
			{}

			struct Iterator
			{
				Element* mCursor;
				Element* mEnd;

				bool operator!=(Iterator const& other)
				{
					return mCursor != other.mCursor;
				}

				void operator++()
				{
					mCursor = Adjust(mCursor + 1, mEnd);
				}

				RefPtr<T>& operator*()
				{
					return *(RefPtr<T>*)mCursor;
				}
			};

			Iterator begin()
			{
				Iterator iter = { mBegin, mEnd };
				return iter;
			}

			Iterator end()
			{
				Iterator iter = { mEnd, mEnd };
				return iter;
			}

			static Element* Adjust(Element* cursor, Element* end)
			{
				while (cursor != end)
				{
					if ((*cursor).As<T>())
						return cursor;
					cursor++;
				}
				return cursor;
			}

			// TODO(tfoley): It is ugly to have these.
			// We should probably fix the call sites instead.
			RefPtr<T>& First() { return *begin(); }
			int Count()
			{
				int count = 0;
				for (auto iter : (*this))
				{
					(void)iter;
					count++;
				}
				return count;
			}

			List<RefPtr<T>> ToArray()
			{
				List<RefPtr<T>> result;
				for (auto element : (*this))
				{
					result.Add(element);
				}
				return result;
			}

			Element* mBegin;
			Element* mEnd;
		};

		// A "container" decl is a parent to other declarations
		class ContainerDecl : public Decl
		{
		public:
			List<RefPtr<Decl>> Members;

			template<typename T>
			FilteredMemberList<T> GetMembersOfType()
			{
				return FilteredMemberList<T>(Members);
			}
		};

		enum class ExpressionAccess
		{
			Read, Write
		};

		struct QualType
		{
			RefPtr<ExpressionType>	type;
			bool					IsLeftValue;

			QualType()
				: IsLeftValue(false)
			{}

			QualType(RefPtr<ExpressionType> type)
				: type(type)
				, IsLeftValue(false)
			{}

			QualType(ExpressionType* type)
				: type(type)
				, IsLeftValue(false)
			{}

			void operator=(RefPtr<ExpressionType> t)
			{
				*this = QualType(t);
			}

			void operator=(ExpressionType* t)
			{
				*this = QualType(t);
			}

			ExpressionType* Ptr() { return type.Ptr(); }

			operator RefPtr<ExpressionType>() { return type; }
			RefPtr<ExpressionType> operator->() { return type; }
		};

		class ExpressionSyntaxNode : public SyntaxNode
		{
		public:
			QualType Type;
			ExpressionAccess Access;
			ExpressionSyntaxNode()
			{
				Access = ExpressionAccess::Read;
			}
			ExpressionSyntaxNode(const ExpressionSyntaxNode & expr) = default;
			virtual ExpressionSyntaxNode* Clone(CloneContext & ctx) = 0;
		};

		// A 'specialize' modifier indicating the shader parameter should be specialized
		class SpecializeModifier : public Modifier
		{
		public:
			List<RefPtr<ExpressionSyntaxNode>> Values;

		};

		//
		// Type Expressions
		//

		// A "type expression" is a term that we expect to resolve to a type during checking.
		// We store both the original syntax and the resolved type here.
		struct TypeExp
		{
			TypeExp() {}
			TypeExp(TypeExp const& other)
				: exp(other.exp)
				, type(other.type)
			{}
			explicit TypeExp(RefPtr<ExpressionSyntaxNode> exp)
				: exp(exp)
			{}

			RefPtr<ExpressionSyntaxNode> exp;
			RefPtr<ExpressionType> type;

			bool Equals(ExpressionType* other) {
				return type->Equals(other);
			}
			bool Equals(RefPtr<ExpressionType> other) {
				return type->Equals(other.Ptr());
			}
			ExpressionType* Ptr() { return type.Ptr(); }
			operator RefPtr<ExpressionType>()
			{
				return type;
			}

			TypeExp Clone(CloneContext& context);
			TypeExp Accept(SyntaxVisitor* visitor);
		};


		//
		// Declarations
		//

		// Base class for all variable-like declarations
		class VarDeclBase : public Decl
		{
		public:
			// Type of the variable
			TypeExp Type;

			// Initializer expression (optional)
			RefPtr<ExpressionSyntaxNode> Expr;
		};

		// A field of a `struct` type
		class StructField : public VarDeclBase
		{
		public:
			StructField()
			{}
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual StructField * Clone(CloneContext & ctx) override
			{
				auto rs = CloneSyntaxNodeFields(new StructField(*this), ctx);
				rs->Type = Type.Clone(ctx);
				return rs;
			}
		};

		// Declaration of a type that represents some sort of aggregate
		class AggTypeDecl : public ContainerDecl
		{};

		class StructSyntaxNode : public AggTypeDecl
		{
		public:
			FilteredMemberList<StructField> GetFields()
			{
				return GetMembersOfType<StructField>();
			}
			bool IsIntrinsic = false;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			StructField* FindField(String name)
			{
				for (auto field : GetFields())
				{
					if (field->Name.Content == name)
						return field.Ptr();
				}
				return nullptr;
			}
			int FindFieldIndex(String name)
			{
				int index = 0;
				for (auto field : GetFields())
				{
					if (field->Name.Content == name)
						return index;
					index++;
				}
				return -1;
			}
			virtual StructSyntaxNode * Clone(CloneContext & ctx) override
			{
				auto rs = CloneSyntaxNodeFields(new StructSyntaxNode(*this), ctx);
				rs->Members.Clear();
				for (auto & m : Members)
					rs->Members.Add(m->Clone(ctx));
				return rs;
			}
		};

		// A declaration that represents a simple (non-aggregate) type
		class SimpleTypeDecl : public Decl
		{
		};

		// A `typedef` declaration
		class TypeDefDecl : public SimpleTypeDecl
		{
		public:
			TypeExp Type;

			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual TypeDefDecl * Clone(CloneContext & ctx) override;
		};

		class StatementSyntaxNode : public SyntaxNode
		{
		public:
			virtual StatementSyntaxNode* Clone(CloneContext & ctx) = 0;
		};

		// A scope for local declarations (e.g., as part of a statement)
		class ScopeDecl : public ContainerDecl
		{
		public:
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ScopeDecl * Clone(CloneContext & ctx) override;
		};

		class ScopeStmt : public StatementSyntaxNode
		{
		public:
			RefPtr<ScopeDecl> scopeDecl;
		};

		class BlockStatementSyntaxNode : public ScopeStmt
		{
		public:
			List<RefPtr<StatementSyntaxNode>> Statements;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual BlockStatementSyntaxNode * Clone(CloneContext & ctx) override;
		};

		// TODO(tfoley): Only used by IL at this point
		enum class ParameterQualifier
		{
			In, Out, InOut, Uniform
		};

		class ParameterSyntaxNode : public VarDeclBase
		{
		public:
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ParameterSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class FunctionDeclBase : public ContainerDecl
		{
		public:
			FilteredMemberList<ParameterSyntaxNode> GetParameters()
			{
				return GetMembersOfType<ParameterSyntaxNode>();
			}
			TypeExp ReturnType;
			RefPtr<BlockStatementSyntaxNode> Body;
		};

		class FunctionSyntaxNode : public FunctionDeclBase
		{
		public:
			String InternalName;
			bool IsInline() { return HasModifier(ModifierFlag::Inline); }
			bool IsExtern() { return HasModifier(ModifierFlag::Extern); }
			bool HasSideEffect() { return !HasModifier(ModifierFlag::Intrinsic); }
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			FunctionSyntaxNode()
			{
			}

			virtual FunctionSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ImportOperatorDefSyntaxNode : public FunctionDeclBase
		{
		public:
			Token SourceWorld, DestWorld;
			Token TypeName;
			List<RefPtr<FunctionSyntaxNode>> Requirements;
			List<String> Usings;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ImportOperatorDefSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ChoiceValueSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			String WorldName;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) { return this; }
			virtual ChoiceValueSyntaxNode * Clone(CloneContext & ctx);
		};

		class VarExpressionSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			// The name of the symbol being referenced
			String Variable;

			// The declaration of the symbol being referenced
			Decl* decl = nullptr;

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
			String ComponentUniqueName; // filled by ResolveDependence
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
			RefPtr<ExpressionSyntaxNode> TargetType;
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

		struct Variable : public VarDeclBase
		{
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual Variable * Clone(CloneContext & ctx) override;
		};

		class VarDeclrStatementSyntaxNode : public StatementSyntaxNode
		{
		public:
			RefPtr<Decl> decl;
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

		class ComponentSyntaxNode : public ContainerDecl
		{
		public:
			bool IsOutput() { return HasModifier(ModifierFlag::Out); }
			bool IsPublic() { return HasModifier(ModifierFlag::Public); }
			bool IsInline() { return HasModifier(ModifierFlag::Inline) || IsComponentFunction(); }
			bool IsRequire() { return HasModifier(ModifierFlag::Require); }
			bool IsInput() { return HasModifier(ModifierFlag::Extern); }
			bool IsParam() { return HasModifier(ModifierFlag::Param); }
			TypeExp Type;
			RefPtr<RateSyntaxNode> Rate;
			RefPtr<BlockStatementSyntaxNode> BlockStatement;
			RefPtr<ExpressionSyntaxNode> Expression;
			FilteredMemberList<ParameterSyntaxNode> GetParameters()
			{
				return GetMembersOfType<ParameterSyntaxNode>();
			}
			bool IsComponentFunction() { return GetParameters().Count() != 0; }
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ComponentSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class WorldSyntaxNode : public SimpleTypeDecl
		{
		public:
			bool IsAbstract() { return HasModifier(ModifierFlag::Input); }
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) override { return this; }
			virtual WorldSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class StageSyntaxNode : public Decl
		{
		public:
			Token StageType;
			EnumerableDictionary<String, Token> Attributes;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) override { return this; }
			virtual StageSyntaxNode * Clone(CloneContext & ctx) override;
		};

		// Shared base class for declarations of buffers with shader parameters
		//
		// TODO(tfoley): Should this actually be unified with the component
		// declaration syntax in some way?
		//
		class BufferDecl : public ContainerDecl
		{
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) override;
		};

		// An HLSL buffer declaration
		class HLSLBufferDecl : public BufferDecl {};

		// An HLSL `cbuffer` declaration
		class HLSLConstantBufferDecl : public HLSLBufferDecl
		{
		public:
			virtual HLSLConstantBufferDecl * Clone(CloneContext & ctx) override;
		};

		// An HLSL `tbuffer` declaration
		class HLSLTextureBufferDecl : public HLSLBufferDecl
		{
		public:
			virtual HLSLTextureBufferDecl * Clone(CloneContext & ctx) override;
		};

		// TODO(tfoley): equivalent cases for GLSL uniform buffer declarations

		// Shared functionality for "shader class"-like declarations
		class ShaderDeclBase : public AggTypeDecl
		{
		public:
			Token ParentPipelineName;
			List<Token> InterfaceNames;
		};

		class PipelineSyntaxNode : public ShaderDeclBase
		{
		public:
			// Access members of specific types
			FilteredMemberList<WorldSyntaxNode> GetWorlds()
			{
				return GetMembersOfType<WorldSyntaxNode>();
			}
			FilteredMemberList<ImportOperatorDefSyntaxNode> GetImportOperators()
			{
				return GetMembersOfType<ImportOperatorDefSyntaxNode>();
			}
			FilteredMemberList<StageSyntaxNode> GetStages()
			{
				return GetMembersOfType<StageSyntaxNode>();
			}
			FilteredMemberList<ComponentSyntaxNode> GetAbstractComponents()
			{
				return GetMembersOfType<ComponentSyntaxNode>();
			}
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

		class ImportSyntaxNode : public Decl
		{
		public:
			bool IsInplace = false;
			bool IsPublic() { return HasModifier(ModifierFlag::Public); }
			Token ShaderName;
			Token ObjectName;
			List<RefPtr<ImportArgumentSyntaxNode>> Arguments;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) override;
			virtual ImportSyntaxNode * Clone(CloneContext & ctx) override;

		};

		class TemplateShaderParameterSyntaxNode : public SyntaxNode
		{
		public:
			Token ModuleName, InterfaceName;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * /*visitor*/) { return this; }
			virtual TemplateShaderParameterSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class TemplateShaderSyntaxNode : public ShaderDeclBase
		{
		public:
			List<RefPtr<TemplateShaderParameterSyntaxNode>> Parameters;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual TemplateShaderSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class ShaderSyntaxNode : public ShaderDeclBase
		{
		public:
			bool IsModule = false;
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual ShaderSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class InterfaceSyntaxNode : public ShaderDeclBase
		{
		public:
			FilteredMemberList<ComponentSyntaxNode> GetComponents()
			{
				return GetMembersOfType<ComponentSyntaxNode>();
			}
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor *) override;
			virtual InterfaceSyntaxNode * Clone(CloneContext & ctx) override;
		};

		class UsingFileDecl : public Decl
		{
		public:
			Token fileName;

			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual UsingFileDecl * Clone(CloneContext & ctx) override;
		};

		class ProgramSyntaxNode : public ContainerDecl
		{
		public:
			// Access members of specific types
			FilteredMemberList<UsingFileDecl> GetUsings()
			{
				return GetMembersOfType<UsingFileDecl>();
			}
			FilteredMemberList<FunctionSyntaxNode> GetFunctions()
			{
				return GetMembersOfType<FunctionSyntaxNode>();
			}
			FilteredMemberList<PipelineSyntaxNode> GetPipelines()
			{
				return GetMembersOfType<PipelineSyntaxNode>();
			}
			FilteredMemberList<InterfaceSyntaxNode> GetInterfaces()
			{
				return GetMembersOfType<InterfaceSyntaxNode>();
			}
			FilteredMemberList<ShaderSyntaxNode> GetShaders()
			{
				return GetMembersOfType<ShaderSyntaxNode>();
			}
			FilteredMemberList<StructSyntaxNode> GetStructs()
			{
				return GetMembersOfType<StructSyntaxNode>();
			}
			FilteredMemberList<TypeDefDecl> GetTypeDefs()
			{
				return GetMembersOfType<TypeDefDecl>();
			}
			void Include(ProgramSyntaxNode * other)
			{
				Members.AddRange(other->Members);
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

		class ForStatementSyntaxNode : public ScopeStmt
		{
		public:
			RefPtr<StatementSyntaxNode> InitialStatement;
			RefPtr<ExpressionSyntaxNode> SideEffectExpression, PredicateExpression;
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

		// Note(tfoley): Moved this further down in the file because it depends on
		// `ExpressionSyntaxNode` and a forward reference just isn't good enough
		// for `RefPtr`.
		//
		class GenericTypeSyntaxNode : public ExpressionSyntaxNode
		{
		public:
			// The name of the generic to apply (e.g., `Buffer`)
			String GenericTypeName;

			// Additional expression arguments after the first (type) argument.
			List<RefPtr<ExpressionSyntaxNode>> Args;

			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual GenericTypeSyntaxNode * Clone(CloneContext & ctx) override
			{
				auto rs = CloneSyntaxNodeFields(new GenericTypeSyntaxNode(*this), ctx);
				for (auto& arg : rs->Args)
					arg = arg->Clone(ctx);
				return rs;
			}
		};

		// A modifier that indicates a built-in base type (e.g., `float`)
		class BuiltinTypeModifier : public Modifier
		{
		public:
			BaseType tag;
		};

		// A modifier that indicates a built-in type that isn't a base type (e.g., `vector`)
		//
		// TODO(tfoley): This deserves a better name than "magic"
		class MagicTypeModifier : public Modifier
		{
		public:
			String name;
			uint32_t tag;
		};

		//

		// A generic declaration, parameterized on types/values
		class GenericDecl : public ContainerDecl
		{
		public:
			// The decl that is genericized...
			RefPtr<Decl> inner;

			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual GenericDecl * Clone(CloneContext & ctx) override;
		};

		class GenericTypeParamDecl : public SimpleTypeDecl
		{
		public:
			// The "initializer" for the parameter represents a default value
			TypeExp initType;

			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual GenericTypeParamDecl * Clone(CloneContext & ctx) override;
		};

		class GenericValueParamDecl : public VarDeclBase
		{
		public:
			virtual RefPtr<SyntaxNode> Accept(SyntaxVisitor * visitor) override;
			virtual GenericValueParamDecl * Clone(CloneContext & ctx) override;
		};

		//

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
				for (auto & m : program->Members)
					m = m->Accept(this).As<Decl>();
				return program;
			}
			virtual RefPtr<ShaderSyntaxNode> VisitShader(ShaderSyntaxNode * shader)
			{
				for (auto & comp : shader->Members)
					comp = comp->Accept(this).As<Decl>();
				return shader;
			}

			virtual RefPtr<UsingFileDecl> VisitUsingFileDecl(UsingFileDecl * decl)
			{
				return decl;
			}

			virtual RefPtr<ComponentSyntaxNode> VisitComponent(ComponentSyntaxNode * comp);
			virtual RefPtr<FunctionSyntaxNode> VisitFunction(FunctionSyntaxNode* func)
			{
				func->ReturnType = func->ReturnType.Accept(this);
				for (auto & member : func->Members)
					member = member->Accept(this).As<Decl>();
				if (func->Body)
					func->Body = func->Body->Accept(this).As<BlockStatementSyntaxNode>();
				return func;
			}
			virtual RefPtr<ScopeDecl> VisitScopeDecl(ScopeDecl* decl)
			{
				// By default don't visit children, because they will always
				// be encountered in the ordinary flow of the corresponding statement.
				return decl;
			}
			virtual RefPtr<StructSyntaxNode> VisitStruct(StructSyntaxNode * s)
			{
				for (auto & f : s->Members)
					f = f->Accept(this).As<Decl>();
				return s;
			}
			virtual RefPtr<GenericDecl> VisitGenericDecl(GenericDecl * decl)
			{
				for (auto & m : decl->Members)
					m = m->Accept(this).As<Decl>();
				decl->inner = decl->inner->Accept(this).As<Decl>();
				return decl;
			}
			virtual RefPtr<TypeDefDecl> VisitTypeDefDecl(TypeDefDecl* decl)
			{
				decl->Type = decl->Type.Accept(this);
				return decl;
			}
			virtual RefPtr<StatementSyntaxNode> VisitDiscardStatement(DiscardStatementSyntaxNode * stmt)
			{
				return stmt;
			}
			virtual RefPtr<StructField> VisitStructField(StructField * f)
			{
				f->Type = f->Type.Accept(this);
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
				if (stmt->InitialStatement)
					stmt->InitialStatement = stmt->InitialStatement->Accept(this).As<StatementSyntaxNode>();
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
				stmt->decl = stmt->decl->Accept(this).As<Decl>();
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
				for (auto & comp : pipe->Members)
					comp = comp->Accept(this).As<Decl>();
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
			virtual RefPtr<ExpressionSyntaxNode> VisitGenericType(GenericTypeSyntaxNode* type)
			{
				return type;
			}

			virtual RefPtr<Variable> VisitDeclrVariable(Variable* dclr)
			{
				if (dclr->Expr)
					dclr->Expr = dclr->Expr->Accept(this).As<ExpressionSyntaxNode>();
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
			virtual RefPtr<InterfaceSyntaxNode> VisitInterface(InterfaceSyntaxNode * node)
			{
				for (auto & member : node->GetComponents())
				{
					member->Accept(this);
				}
				return node;
			}
			virtual RefPtr<SyntaxNode> VisitTemplateShader(TemplateShaderSyntaxNode * shader)
			{
				for (auto & param : shader->Parameters)
					param->Accept(this);
				for (auto & member : shader->Members)
					member->Accept(this);
				return shader;
			}

			virtual RefPtr<SyntaxNode> VisitBufferDecl(BufferDecl * decl)
			{
				for (auto & member : decl->Members)
					member->Accept(this);
				return decl;
			}

			virtual TypeExp VisitTypeExp(TypeExp const& typeExp)
			{
				TypeExp result = typeExp;
				result.exp = typeExp.exp->Accept(this).As<ExpressionSyntaxNode>();
				if (auto typeType = result.exp->Type.type.As<TypeExpressionType>())
				{
					result.type = typeType->type;
				}
				return result;
			}

		};
	}
}

#endif