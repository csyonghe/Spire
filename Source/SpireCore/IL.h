#ifndef RASTER_RENDERER_IL_H
#define RASTER_RENDERER_IL_H

#include "../CoreLib/Basic.h"
#include "../CoreLib/Tokenizer.h"

namespace Spire
{
	namespace Compiler
	{
		using CoreLib::Text::CodePosition;

		using namespace CoreLib::Basic;
		enum ILBaseType
		{
			Void = 0,
			Int = 16, Int2 = 17, Int3 = 18, Int4 = 19,
			Float = 32, Float2 = 33, Float3 = 34, Float4 = 35,
			Float3x3 = 40, Float4x4 = 47,
			Texture2D = 48,
			TextureCube = 49,
			Texture2DArray = 50,
			Texture2DShadow = 51,
			TextureCubeShadow = 52,
			Texture2DArrayShadow = 53,
			Texture3D = 54,
			Bool = 128, Bool2 = 129, Bool3 = 130, Bool4 = 131,
			UInt = 512, UInt2 = 513, UInt3 = 514, UInt4 = 515,
			SamplerState = 4096, SamplerComparisonState = 4097
		};
		int SizeofBaseType(ILBaseType type);
		int RoundToAlignment(int offset, int alignment);
		extern int NamingCounter;

		enum class BindableResourceType
		{
			NonBindable, Texture, Sampler, Buffer, StorageBuffer
		};
		int GetMaxResourceBindings(BindableResourceType type);

		class ILType : public RefObject
		{
		public:
			bool IsBool();
			bool IsInt();
			bool IsUInt();
			bool IsIntegral();
			bool IsFloat();
			bool IsVoid();
			bool IsScalar()
			{
				return IsInt() || IsUInt() || IsFloat() || IsBool();
			}
			bool IsBoolVector(); 
			bool IsIntVector();
			bool IsUIntVector();
			bool IsFloatVector();
			bool IsFloatMatrix();
			bool IsVector()
			{
				return IsIntVector() || IsUIntVector() || IsFloatVector() || IsBoolVector();
			}
			bool IsTexture();
			bool IsSamplerState();
			bool IsNonShadowTexture();
			int GetVectorSize();
			virtual BindableResourceType GetBindableResourceType() = 0;
			virtual ILType * Clone() = 0;
			virtual String ToString() = 0;
			virtual bool Equals(ILType* type) = 0;
			virtual void Serialize(StringBuilder & sb) = 0;
			static RefPtr<ILType> Deserialize(CoreLib::Text::TokenReader & reader);
		};

		class ILObjectDefinition
		{
		public:
			RefPtr<ILType> Type;
			String Name;
			EnumerableDictionary<String, CoreLib::Text::Token> Attributes;
			CodePosition Position;
			int Binding = -1;
		};

		class ILRecordType : public ILType
		{
		public:
			String TypeName;
			EnumerableDictionary<String, ILObjectDefinition> Members;
			virtual ILType * Clone() override;
			virtual String ToString() override;
			virtual bool Equals(ILType* type) override;
			virtual void Serialize(StringBuilder & sb) override
			{
				sb << "record " << TypeName;
			}
			virtual BindableResourceType GetBindableResourceType() override
			{
				return BindableResourceType::NonBindable;
			}
		};

		class ILBasicType : public ILType
		{
		public:
			ILBaseType Type;
			ILBasicType()
			{
				Type = ILBaseType::Int;
			}
			ILBasicType(ILBaseType t)
			{
				Type = t;
			}
			virtual bool Equals(ILType* type) override
			{
				auto btype = dynamic_cast<ILBasicType*>(type);
				if (!btype)
					return false;
				return Type == btype->Type;
			}

			virtual BindableResourceType GetBindableResourceType() override
			{
				switch (Type)
				{
				case ILBaseType::Texture2D:
				case ILBaseType::TextureCube:
				case ILBaseType::Texture2DArray:
				case ILBaseType::Texture2DShadow:
				case ILBaseType::TextureCubeShadow:
				case ILBaseType::Texture2DArrayShadow:
				case ILBaseType::Texture3D:
					return BindableResourceType::Texture;
				case ILBaseType::SamplerState:
				case ILBaseType::SamplerComparisonState:
					return BindableResourceType::Sampler;
				default:
					return BindableResourceType::NonBindable;
				}
			}

			virtual ILType * Clone() override
			{
				auto rs = new ILBasicType();
				rs->Type = Type;
				return rs;
			}
			virtual void Serialize(StringBuilder & sb) override
			{
				sb << "basic " << ToString();
			}
			virtual String ToString() override
			{
				if (Type == ILBaseType::Int)
					return "int";
				else if (Type == ILBaseType::UInt)
					return "uint";
				else if (Type == ILBaseType::UInt2)
					return "uvec2";
				else if (Type == ILBaseType::UInt3)
					return "uvec3";
				else if (Type == ILBaseType::UInt4)
					return "uvec4";
				else if (Type == ILBaseType::Int2)
					return "ivec2";
				else if (Type == ILBaseType::Int3)
					return "ivec3";
				else if (Type == ILBaseType::Int4)
					return "ivec4";
				else if (Type == ILBaseType::Float)
					return "float";
				else if (Type == ILBaseType::Float2)
					return "vec2";
				else if (Type == ILBaseType::Float3)
					return "vec3";
				else if (Type == ILBaseType::Float4)
					return "vec4";
				else if (Type == ILBaseType::Float3x3)
					return "mat3";
				else if (Type == ILBaseType::Float4x4)
					return "mat4";
				else if (Type == ILBaseType::Texture2D)
					return "sampler2D";
				else if (Type == ILBaseType::TextureCube)
					return "samplerCube";
				else if (Type == ILBaseType::Texture2DArray)
					return "sampler2DArray";
				else if (Type == ILBaseType::Texture2DShadow)
					return "sampler2DShadow";
				else if (Type == ILBaseType::TextureCubeShadow)
					return "samplerCubeShadow";
				else if (Type == ILBaseType::Texture2DArrayShadow)
					return "sampler2DArrayShadow";
				else if (Type == ILBaseType::Texture3D)
					return "sampler3D";
				else if (Type == ILBaseType::Bool)
					return "bool";
				else if (Type == ILBaseType::Bool2)
					return "bvec2";
				else if (Type == ILBaseType::Bool3)
					return "bvec3";
				else if (Type == ILBaseType::Bool4)
					return "bvec4";
				else if (Type == ILBaseType::SamplerState)
					return "SamplerState";
				else if (Type == ILBaseType::SamplerComparisonState)
					return "SamplerComparisonState";
				else if (Type == ILBaseType::Void)
					return "void";
				else
					return "?unknown";
			}
		};

		class ILArrayType : public ILType
		{
		public:
			RefPtr<ILType> BaseType;
			int ArrayLength;
			virtual bool Equals(ILType* type) override
			{
				auto btype = dynamic_cast<ILArrayType*>(type);
				if (!btype)
					return false;
				return BaseType->Equals(btype->BaseType.Ptr());;
			}
			virtual ILType * Clone() override
			{
				auto rs = new ILArrayType();
				rs->BaseType = BaseType->Clone();
				rs->ArrayLength = ArrayLength;
				return rs;
			}
			virtual void Serialize(StringBuilder & sb) override
			{
				sb << "array(";
				BaseType->Serialize(sb);
				sb << ", " << ArrayLength << ")";
			}
			virtual String ToString() override
			{
				if (ArrayLength > 0)
					return BaseType->ToString() + "[" + String(ArrayLength) + "]";
				else
					return BaseType->ToString() + "[]";
			}
			virtual BindableResourceType GetBindableResourceType() override
			{
				return BindableResourceType::NonBindable;
			}
		};

		class ILGenericType : public ILType
		{
		public:
			RefPtr<ILType> BaseType;
			String GenericTypeName;
			virtual bool Equals(ILType* type) override
			{
				auto btype = dynamic_cast<ILArrayType*>(type);
				if (!btype)
					return false;
				return BaseType->Equals(btype->BaseType.Ptr());;
			}
			virtual ILType * Clone() override
			{
				auto rs = new ILGenericType();
				rs->BaseType = BaseType->Clone();
				rs->GenericTypeName = GenericTypeName;
				return rs;
			}
			virtual String ToString() override
			{
				return GenericTypeName + "<" + BaseType->ToString() + ">";
			}
			virtual void Serialize(StringBuilder & sb) override
			{
				sb << "generic " << GenericTypeName << "(";
				BaseType->Serialize(sb);
				sb << ")";
			}
			virtual BindableResourceType GetBindableResourceType() override
			{
				if (GenericTypeName == "StructuredBuffer" || GenericTypeName == "RWStructuredBuffer")
					return BindableResourceType::StorageBuffer;
				else if (GenericTypeName == "Buffer" || GenericTypeName == "RWBuffer" || GenericTypeName == "ByteAddressBuffer" ||
					GenericTypeName == "RWByteAddressBuffer")
					return BindableResourceType::Buffer;
				return BindableResourceType::NonBindable;
			}
		};

		class ILStructType : public ILType
		{
		public:
			String TypeName;
			bool IsIntrinsic = false;
			class ILStructField
			{
			public:
				RefPtr<ILType> Type;
				String FieldName;
			};
			List<ILStructField> Members;
			virtual ILType * Clone() override;
			virtual String ToString() override;
			virtual bool Equals(ILType * type) override;
			virtual void Serialize(StringBuilder & sb) override
			{
				sb << "struct " << TypeName << "(";
				for (auto & member : Members)
				{
					sb << member.FieldName << ":";
					member.Type->Serialize(sb);
					sb << "; ";
				}
				sb << ")";
			}
			virtual BindableResourceType GetBindableResourceType() override
			{
				return BindableResourceType::NonBindable;
			}
		};

		class ILOperand;

		class UserReferenceSet
		{
		private:
			EnumerableDictionary<ILOperand*, int> userRefCounts;
			int count;
		public:
			UserReferenceSet()
			{
				count = 0;
			}
			int Count()
			{
				return count;
			}
			int GetUseCount(ILOperand * op)
			{
				int rs = -1;
				userRefCounts.TryGetValue(op, rs);
				return rs;
			}
			void Add(ILOperand * user)
			{
				this->count++;
				int ncount = 0;
				if (userRefCounts.TryGetValue(user, ncount))
				{
					ncount++;
					userRefCounts[user] = ncount;
				}
				else
				{
					userRefCounts.Add(user, 1);
				}
			}
			void Remove(ILOperand * user)
			{
				int ncount = 0;
				if (userRefCounts.TryGetValue(user, ncount))
				{
					this->count--;
					ncount--;
					if (ncount)
						userRefCounts[user] = ncount;
					else
						userRefCounts.Remove(user);
				}
			}
			void RemoveAll(ILOperand * user)
			{
				int ncount = 0;
				if (userRefCounts.TryGetValue(user, ncount))
				{
					this->count -= ncount;
					userRefCounts.Remove(user);
				}
			}
			class UserIterator
			{
			private:
				EnumerableDictionary<ILOperand*, int>::Iterator iter;
			public:
				ILOperand * operator *()
				{
					return iter.Current->Value.Key;
				}
				ILOperand ** operator ->()
				{
					return &iter.Current->Value.Key;
				}
				UserIterator & operator ++()
				{
					iter++;
					return *this;
				}
				UserIterator operator ++(int)
				{
					UserIterator rs = *this;
					operator++();
					return rs;
				}
				bool operator != (const UserIterator & _that)
				{
					return iter != _that.iter;
				}
				bool operator == (const UserIterator & _that)
				{
					return iter == _that.iter;
				}
				UserIterator(const EnumerableDictionary<ILOperand*, int>::Iterator & iter)
				{
					this->iter = iter;
				}
				UserIterator()
				{
				}
			};
			UserIterator begin()
			{
				return UserIterator(userRefCounts.begin());
			}
			UserIterator end()
			{
				return UserIterator(userRefCounts.end());
			}
		};

		class ILOperand : public Object
		{
		public:
			String Name;
			RefPtr<ILType> Type;
			UserReferenceSet Users;
			String Attribute;
			void * Tag;
			CodePosition Position;
			union VMFields
			{
				void * VMData;
				struct Fields
				{
					int VMDataWords[2];
				} Fields;
			} VMFields;
			Procedure<ILOperand*> OnDelete;
			ILOperand()
			{
				Tag = nullptr;
			}
			ILOperand(const ILOperand & op)
			{
				Tag = op.Tag;
				Name = op.Name;
				Attribute = op.Attribute;
				if (op.Type)
					Type = op.Type->Clone();
				//Users = op.Users;
			}
			virtual ~ILOperand()
			{
				OnDelete(this);
			}
			virtual String ToString()
			{
				return "<operand>";
			}
			virtual bool IsUndefined()
			{
				return false;
			}
		};

		class ILUndefinedOperand : public ILOperand
		{
		public:
			ILUndefinedOperand()
			{
				Name = "<undef>";
			}
			virtual String ToString() override
			{
				return "<undef>";
			}
			virtual bool IsUndefined() override
			{
				return true;
			}
		};

		class UseReference
		{
		private:
			ILOperand * user;
			ILOperand * reference;
		public:
			UseReference()
				: user(0), reference(0)
			{}
			UseReference(const UseReference &)
			{
				user = 0;
				reference = 0;
			}
			UseReference(ILOperand * user)
				: user(user), reference(0)
			{}
			UseReference(ILOperand * user, ILOperand * ref)
			{
				this->user = user;
				this->reference = ref;
			}
			~UseReference()
			{
				if (reference)
					reference->Users.Remove(user);
			}
			void SetUser(ILOperand * _user)
			{
				this->user = _user;
			}
			void operator = (const UseReference & ref)
			{
				if (reference)
					reference->Users.Remove(user);
				reference = ref.Ptr();
				if (ref.Ptr())
				{
					if (!user)
						throw InvalidOperationException("user not initialized.");
					ref.Ptr()->Users.Add(user);
				}
			}
			void operator = (ILOperand * newRef)
			{
				if (reference)
					reference->Users.Remove(user);
				reference = newRef;
				if (newRef)
				{
					if (!user)
						throw InvalidOperationException("user not initialized.");
					newRef->Users.Add(user);
				}
			}
			bool operator != (const UseReference & _that)
			{
				return reference != _that.reference || user != _that.user;
			}
			bool operator == (const UseReference & _that)
			{
				return reference == _that.reference && user == _that.user;
			}
			ILOperand * Ptr() const
			{
				return reference;
			}
			ILOperand * operator->()
			{
				return reference;
			}
			ILOperand & operator*()
			{
				return *reference;
			}
			explicit operator bool()
			{
				return (reference != 0);
			}
			String ToString()
			{
				if (reference)
					return reference->Name;
				else
					return "<null>";
			}
		};

		class OperandIterator
		{
		private:
			UseReference * use;
		public:
			OperandIterator()
			{
				use = 0;
			}
			OperandIterator(UseReference * use)
				: use(use)
			{}
			ILOperand & operator *()
			{
				return use->operator*();
			}
			ILOperand * operator ->()
			{
				return use->operator->();
			}
			void Set(ILOperand * user, ILOperand * op)
			{
				(*use).SetUser(user);
				(*use) = op;
			}
			void Set(ILOperand * op)
			{
				(*use) = op; 
			}
			OperandIterator & operator ++()
			{
				use++;
				return *this;
			}
			OperandIterator operator ++(int)
			{
				OperandIterator rs = *this;
				operator++();
				return rs;
			}
			bool operator != (const OperandIterator & _that)
			{
				return use != _that.use;
			}
			bool operator == (const OperandIterator & _that)
			{
				return use == _that.use;
			}
			bool operator == (const ILOperand * op)
			{
				return use->Ptr() == op;
			}
			bool operator != (const ILOperand * op)
			{
				return use->Ptr() != op;
			}
		};

		class ILConstOperand : public ILOperand
		{
		public:
			union
			{
				int IntValues[16];
				float FloatValues[16];
			};
			virtual String ToString() override
			{
				if (Type->IsFloat())
					return String(FloatValues[0]) + "f";
				else if (Type->IsInt())
					return String(IntValues[0]);
				else if (auto baseType = dynamic_cast<ILBasicType*>(Type.Ptr()))
				{
					StringBuilder sb(256);
					if (baseType->Type == ILBaseType::Float2)
						sb << "vec2(" << FloatValues[0] << "f, " << FloatValues[1] << "f)";
					else if (baseType->Type == ILBaseType::Float3)
						sb << "vec3(" << FloatValues[0] << "f, " << FloatValues[1] << "f, " << FloatValues[2] << "f)";
					else if (baseType->Type == ILBaseType::Float4)
						sb << "vec4(" << FloatValues[0] << "f, " << FloatValues[1] << "f, " << FloatValues[2] << "f, " << FloatValues[3] << "f)";
					else if (baseType->Type == ILBaseType::Float3x3)
						sb << "mat3(...)";
					else if (baseType->Type == ILBaseType::Float4x4)
						sb << "mat4(...)";
					else if (baseType->Type == ILBaseType::Int2)
						sb << "ivec2(" << IntValues[0] << ", " << IntValues[1] << ")";
					else if (baseType->Type == ILBaseType::Int3)
						sb << "ivec3(" << IntValues[0] << ", " << IntValues[1] << ", " << IntValues[2] << ")";
					else if (baseType->Type == ILBaseType::Int4)
						sb << "ivec4(" << IntValues[0] << ", " << IntValues[1] << ", " << IntValues[2] << ", " << IntValues[3] << ")";
					else if (baseType->Type == ILBaseType::UInt2)
						sb << "uvec2(" << IntValues[0] << ", " << IntValues[1] << ")";
					else if (baseType->Type == ILBaseType::UInt3)
						sb << "uvec3(" << IntValues[0] << ", " << IntValues[1] << ", " << IntValues[2] << ")";
					else if (baseType->Type == ILBaseType::UInt4)
						sb << "uvec4(" << IntValues[0] << ", " << IntValues[1] << ", " << IntValues[2] << ", " << IntValues[3] << ")";
					return sb.ToString();
				}
				else
					throw InvalidOperationException("Illegal constant.");
			}
		};

		class InstructionVisitor;

		class CFGNode;

		class ILInstruction : public ILOperand
		{
		private:
			ILInstruction *next, *prev;
		public:
			CFGNode * Parent;
			ILInstruction()
			{
				next = 0;
				prev = 0;
				Parent = 0;
			}
			ILInstruction(const ILInstruction & instr)
				: ILOperand(instr)
			{
				next = 0;
				prev = 0;
				Parent = 0;
			}
			~ILInstruction()
			{
				
			}
			virtual ILInstruction * Clone()
			{
				return new ILInstruction(*this);
			}

			virtual String GetOperatorString()
			{
				return "<instruction>";
			}
			virtual bool HasSideEffect()
			{
				return false;
			}
			virtual bool IsDeterministic()
			{
				return true;
			}
			virtual void Accept(InstructionVisitor *)
			{
			}
			void InsertBefore(ILInstruction * instr)
			{
				instr->Parent = Parent;
				instr->prev = prev;
				instr->next = this;
				prev = instr;
				auto *npp = instr->prev;
				if (npp)
					npp->next = instr;
			}
			void InsertAfter(ILInstruction * instr)
			{
				instr->Parent = Parent;
				instr->prev = this;
				instr->next = this->next;
				next = instr;
				auto *npp = instr->next;
				if (npp)
					npp->prev = instr;
			}
			ILInstruction * GetNext()
			{
				return next;
			}
			ILInstruction * GetPrevious()
			{
				return prev;
			}
			void Remove()
			{
				if (prev)
					prev->next = next;
				if (next)
					next->prev = prev;
			}
			void Erase()
			{
				Remove();
				if (Users.Count())
				{
					throw InvalidOperationException("All uses must be removed before removing this instruction");
				}
				delete this;
			}
			virtual OperandIterator begin()
			{
				return OperandIterator();
			}
			virtual OperandIterator end()
			{
				return OperandIterator();
			}
			virtual int GetSubBlockCount()
			{
				return 0;
			}
			virtual CFGNode * GetSubBlock(int)
			{
				return nullptr;
			}
			template<typename T>
			T * As()
			{
				return dynamic_cast<T*>(this);
			}
			template<typename T>
			bool Is()
			{
				return dynamic_cast<T*>(this) != 0;
			}
		};

		template <typename T, typename TOperand>
		bool Is(TOperand * op)
		{
			auto ptr = dynamic_cast<T*>(op);
			if (ptr)
				return true;
			else
				return false;
		}

		class SwitchInstruction : public ILInstruction
		{
		public:
			List<UseReference> Candidates;
			virtual OperandIterator begin() override
			{
				return Candidates.begin();
			}
			virtual OperandIterator end() override
			{
				return Candidates.end();
			}
			virtual String ToString() override
			{
				StringBuilder sb(256);
				sb << Name;
				sb << " = switch ";
				for (auto & op : Candidates)
				{
					sb << op.ToString();
					if (op != Candidates.Last())
						sb << ", ";
				}
				return sb.ProduceString();
			}
			virtual String GetOperatorString() override
			{
				return "switch";
			}
			virtual bool HasSideEffect() override
			{
				return false;
			}
			SwitchInstruction(int argSize)
			{
				Candidates.SetSize(argSize);
				for (auto & use : Candidates)
					use.SetUser(this);
			}
			SwitchInstruction(const SwitchInstruction & other)
				: ILInstruction(other)
			{
				Candidates.SetSize(other.Candidates.Count());
				for (int i = 0; i < other.Candidates.Count(); i++)
				{
					Candidates[i].SetUser(this);
					Candidates[i] = other.Candidates[i].Ptr();
				}

			}
			virtual SwitchInstruction * Clone() override
			{
				return new SwitchInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class LeaInstruction : public ILInstruction
		{};
		class ILWorld;
		class ImportInstruction : public LeaInstruction
		{
		public:
			String ComponentName;
			RefPtr<CFGNode> ImportOperator;

			List<UseReference> Arguments;
			virtual OperandIterator begin() override
			{
				return Arguments.begin();
			}
			virtual OperandIterator end() override
			{
				return Arguments.end();
			}
			virtual int GetSubBlockCount() override
			{
				return 1;
			}
			virtual CFGNode * GetSubBlock(int i) override
			{
				if (i == 0)
					return ImportOperator.Ptr();
				return nullptr;
			}
			ImportInstruction(int argSize = 0)
				: LeaInstruction()
			{
				Arguments.SetSize(argSize);
				for (auto & use : Arguments)
					use.SetUser(this);
			}
			ImportInstruction(const ImportInstruction & other)
				: LeaInstruction(other)
			{
				Arguments.SetSize(other.Arguments.Count());
				for (int i = 0; i < other.Arguments.Count(); i++)
				{
					Arguments[i].SetUser(this);
					Arguments[i] = other.Arguments[i].Ptr();
				}
			}

			ImportInstruction(int argSize, String compName, RefPtr<CFGNode> importOp, RefPtr<ILType> type)
				:ImportInstruction(argSize)
			{
				this->ComponentName = compName;
				this->ImportOperator = importOp;
				this->Type = type;
			}
			virtual String ToString() override;
			virtual String GetOperatorString() override;
			virtual ImportInstruction * Clone() override
			{
				return new ImportInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class LoadInputInstruction : public LeaInstruction
		{
		public:
			String InputName;
			LoadInputInstruction(RefPtr<ILType> type, String name)
				: InputName(name)
			{
				this->Type = type;
			}
			LoadInputInstruction(const LoadInputInstruction & other)
				:LeaInstruction(other), InputName(other.InputName)
			{
			}
			virtual bool IsDeterministic() override
			{
				return true;
			}
			virtual String ToString() override
			{
				return Name + " = INPUT " + InputName;
			}
			virtual String GetOperatorString() override
			{
				return "input";
			}
			virtual LoadInputInstruction * Clone() override
			{
				return new LoadInputInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class AllocVarInstruction : public LeaInstruction
		{
		public:
			UseReference Size;
			AllocVarInstruction(ILType * type, ILOperand * count)
				: Size(this)
			{
				this->Type = type;
				this->Size = count;
			}
			AllocVarInstruction(RefPtr<ILType> & type, ILOperand * count)
				: Size(this)
			{
				auto ptrType = type->Clone();
				if (!type)
					throw ArgumentException("type cannot be null.");
				this->Type = ptrType;
				this->Size = count;
			}
			AllocVarInstruction(const AllocVarInstruction & other)
				:LeaInstruction(other), Size(this)
			{
				Size = other.Size.Ptr();
			}
			virtual bool IsDeterministic() override
			{
				return false;
			}
			virtual String ToString() override
			{
				return Name + " = VAR " + Type->ToString() + ", " + Size.ToString();
			}
			virtual OperandIterator begin() override
			{
				return &Size;
			}
			virtual OperandIterator end() override
			{
				return &Size + 1;
			}
			virtual String GetOperatorString() override
			{
				return "avar";
			}
			virtual AllocVarInstruction * Clone() override
			{
				return new AllocVarInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class FetchArgInstruction : public LeaInstruction
		{
		public:
			int ArgId;
			FetchArgInstruction(RefPtr<ILType> type)
			{
				this->Type = type;
				ArgId = 0;
			}
			virtual String ToString() override
			{
				return Name + " = ARG " + Type->ToString();
			}
			virtual String GetOperatorString() override
			{
				return "arg " + String(ArgId);
			}
			virtual bool IsDeterministic() override
			{
				return false;
			}
			virtual FetchArgInstruction * Clone() override
			{
				return new FetchArgInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class CFGNode;

		class AllInstructionsIterator
		{
		private:
			struct StackItem
			{
				ILInstruction* instr;
				int subBlockPtr;
			};
			List<StackItem> stack;
			ILInstruction * curInstr = nullptr;
			int subBlockPtr = 0;
		public:
			AllInstructionsIterator(ILInstruction * instr)
			{
				curInstr = instr;
			}
			AllInstructionsIterator & operator ++();
			
			AllInstructionsIterator operator ++(int)
			{
				AllInstructionsIterator rs = *this;
				operator++();
				return rs;
			}
			bool operator != (const AllInstructionsIterator & _that)
			{
				return curInstr != _that.curInstr || subBlockPtr != _that.subBlockPtr;
			}
			bool operator == (const AllInstructionsIterator & _that)
			{
				return curInstr == _that.curInstr && subBlockPtr == _that.subBlockPtr;
			}
			ILOperand & operator *()
			{
				return *curInstr;
			}
			ILOperand * operator ->()
			{
				return curInstr;
			}
		};

		class AllInstructionsCollection
		{
		private:
			CFGNode * node;
		public:
			AllInstructionsCollection(CFGNode * _node)
				: node(_node)
			{}
			AllInstructionsIterator begin();
			AllInstructionsIterator end();
		};
		
		class CFGNode : public Object
		{
		private:
			ILInstruction *headInstr, *tailInstr;
		public:
			class Iterator
			{
			public:
				ILInstruction * Current, *Next;
				void SetCurrent(ILInstruction * cur)
				{
					Current = cur;
					if (Current)
						Next = Current->GetNext();
					else
						Next = 0;
				}
				Iterator(ILInstruction * cur)
				{
					SetCurrent(cur);
				}
				ILInstruction & operator *() const
				{
					return *Current;
				}
				Iterator& operator ++()
				{
					SetCurrent(Next);
					return *this;
				}
				Iterator operator ++(int)
				{
					Iterator rs = *this;
					SetCurrent(Next);
					return rs;
				}
				bool operator != (const Iterator & iter) const
				{
					return Current != iter.Current;
				}
				bool operator == (const Iterator & iter) const
				{
					return Current == iter.Current;
				}
			};

			String ToString() {
				StringBuilder sb;
				bool first = true;
				auto pintr = begin();
				while (pintr != end()) {
					if (!first)
						sb << EndLine;
					first = false;
					sb << pintr.Current->ToString();
					pintr++;
				}
				return sb.ToString();
			}

			Iterator begin() const
			{
				return Iterator(headInstr->GetNext());
			}

			Iterator end() const
			{
				return Iterator(tailInstr);
			}

			AllInstructionsCollection GetAllInstructions()
			{
				return AllInstructionsCollection(this);
			}
			
			ILInstruction * GetFirstNonPhiInstruction();
			bool HasPhiInstruction();

			ILInstruction * GetLastInstruction()
			{
				return (tailInstr->GetPrevious());
			}

			String Name;

			CFGNode()
			{
				headInstr = new ILInstruction();
				tailInstr = new ILInstruction();
				headInstr->Parent = this;
				headInstr->InsertAfter(tailInstr);
			}
			~CFGNode()
			{
				ILInstruction * instr = headInstr;
				while (instr)
				{
					for (auto user : instr->Users)
					{
						auto userInstr = dynamic_cast<ILInstruction*>(user);
						if (userInstr)
						{
							for (auto iter = userInstr->begin(); iter != userInstr->end(); ++iter)
							if (iter == instr)
								iter.Set(0);
						}
					}
				
					auto next = instr->GetNext();
					delete instr;
					instr = next;
				}
			}
			void InsertHead(ILInstruction * instr)
			{
				headInstr->InsertAfter(instr);
			}
			void InsertTail(ILInstruction * instr)
			{
				tailInstr->InsertBefore(instr);
			}
			void NameAllInstructions();
			void DebugPrint();
		};

		template<typename T>
		struct ConstKey
		{
			Array<T, 16> Value;
			int Size;
			ConstKey()
			{
				Value.SetSize(Value.GetCapacity());
			}
			ConstKey(T value, int size)
			{
				if (size == 0)
					size = 1;
				Value.SetSize(Value.GetCapacity());
				for (int i = 0; i < size; i++)
					Value[i] = value;
				Size = size;
			}
			static ConstKey<T> FromValues(T value, T value1)
			{
				ConstKey<T> result;
				result.Value.SetSize(result.Value.GetCapacity());
				result.Size = 2;
				result.Value[0] = value;
				result.Value[1] = value1;
				return result;
			}
			static ConstKey<T> FromValues(T value, T value1, T value2)
			{
				ConstKey<T> result;
				result.Value.SetSize(result.Value.GetCapacity());
				result.Size = 3;
				result.Value[0] = value;
				result.Value[1] = value1;
				result.Value[2] = value2;
				return result;
			}
			static ConstKey<T> FromValues(T value, T value1, T value2, T value3)
			{
				ConstKey<T> result;
				result.Value.SetSize(result.Value.GetCapacity());
				result.Size = 4;
				result.Value[0] = value;
				result.Value[1] = value1;
				result.Value[2] = value2;
				result.Value[3] = value3;
				return result;
			}
			int GetHashCode()
			{
				int result = Size;
				for (int i = 0; i < Size; i++)
					result ^= ((*(int*)&Value) << 5);
				return result;
			}
			bool operator == (const ConstKey<T> & other)
			{
				if (Size != other.Size)
					return false;
				for (int i = 0; i < Size; i++)
					if (Value[i] != other.Value[i])
						return false;
				return true;
			}
		};


		class PhiInstruction : public ILInstruction
		{
		public:
			List<UseReference> Operands; // Use as fixed array, no insert or resize
		public:
			PhiInstruction(int opCount)
			{
				Operands.SetSize(opCount);
				for (int i = 0; i < opCount; i++)
					Operands[i].SetUser(this);
			}
			PhiInstruction(const PhiInstruction & other)
				: ILInstruction(other)
			{
				Operands.SetSize(other.Operands.Count());
				for (int i = 0; i < Operands.Count(); i++)
				{
					Operands[i].SetUser(this);
					Operands[i] = other.Operands[i].Ptr();
				}
			}
			virtual String GetOperatorString() override
			{
				return "phi";
			}
			virtual OperandIterator begin() override
			{
				return Operands.begin();
			}
			virtual OperandIterator end() override
			{
				return Operands.end();
			}
			virtual String ToString() override
			{
				StringBuilder sb;
				sb << Name << " = phi ";
				for (auto & op : Operands)
				{
					if (op)
					{
						sb << op.ToString();
					}
					else
						sb << "<?>";
					sb << ", ";
				}
				return sb.ProduceString();
			}
			virtual PhiInstruction * Clone() override
			{
				return new PhiInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class UnaryInstruction : public ILInstruction
		{
		public:
			UseReference Operand;
			UnaryInstruction()
				: Operand(this)
			{}
			UnaryInstruction(const UnaryInstruction & other)
				: ILInstruction(other), Operand(this)
			{
				Operand = other.Operand.Ptr();
			}
			virtual OperandIterator begin() override
			{
				return &Operand;
			}
			virtual OperandIterator end() override
			{
				return &Operand + 1;
			}
		};

		class MakeRecordInstruction : public ILInstruction
		{
		public:
			RefPtr<ILRecordType> RecordType;
			List<UseReference> Arguments;
		};

		class ProjectInstruction : public UnaryInstruction
		{
		public:
			String ComponentName;
			virtual String ToString() override
			{
				StringBuilder sb;
				sb << Name << " = project ";
				sb << Operand.ToString();
				sb << ", " << ComponentName;
				return sb.ProduceString();
			}
			virtual ProjectInstruction * Clone() override
			{
				return new ProjectInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class ExportInstruction : public UnaryInstruction
		{
		public:
			String ComponentName;
			ILWorld * World;

			ExportInstruction() = default;
			ExportInstruction(const ExportInstruction &) = default;

			ExportInstruction(String compName, ILWorld * srcWorld, ILOperand * value)
				: UnaryInstruction()
			{
				this->Operand = value;
				this->ComponentName = compName;
				this->World = srcWorld;
				this->Type = value->Type;
			}
			virtual String ToString() override
			{
				return "export [" + ComponentName + "], " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return "export [" + ComponentName + "]";
			}
			virtual ExportInstruction * Clone() override
			{
				return new ExportInstruction(*this);
			}
			virtual bool HasSideEffect() override
			{
				return true;
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class BinaryInstruction : public ILInstruction
		{
		public:
			Array<UseReference, 2> Operands;
			BinaryInstruction()
			{
				Operands.SetSize(2);
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
			}
			BinaryInstruction(const BinaryInstruction & other)
				: ILInstruction(other)
			{
				Operands.SetSize(2);
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[0] = other.Operands[0].Ptr();
				Operands[1] = other.Operands[1].Ptr();
			}
			virtual OperandIterator begin() override
			{
				return Operands.begin();
			}
			virtual OperandIterator end() override
			{
				return Operands.end();
			}
		};

		class SelectInstruction : public ILInstruction
		{
		public:
			UseReference Operands[3];
			SelectInstruction()
			{
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[2].SetUser(this);
			}
			SelectInstruction(const SelectInstruction & other)
				: ILInstruction(other)
			{
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[2].SetUser(this);
				Operands[0] = other.Operands[0].Ptr();
				Operands[1] = other.Operands[1].Ptr();
				Operands[2] = other.Operands[2].Ptr();
			}
			SelectInstruction(ILOperand * mask, ILOperand * val0, ILOperand * val1)
			{
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[2].SetUser(this);
				Operands[0] = mask;
				Operands[1] = val0;
				Operands[2] = val1;
				Type = val0->Type->Clone();
			}
			virtual OperandIterator begin() override
			{
				return Operands;
			}
			virtual OperandIterator end() override
			{
				return Operands + 3;
			}

			virtual String ToString() override
			{
				return Name + " = select " + Operands[0].ToString() + ": " + Operands[1].ToString() + ", " + Operands[2].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "select";
			}
			virtual SelectInstruction * Clone() override
			{
				return new SelectInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class CallInstruction : public ILInstruction
		{
		public:
			String Function;
			List<UseReference> Arguments;
			bool SideEffect = false;
			virtual OperandIterator begin() override
			{
				return Arguments.begin();
			}
			virtual OperandIterator end() override
			{
				return Arguments.end();
			}
			virtual String ToString() override
			{
				StringBuilder sb(256);
				sb << Name;
				sb << " = call " << Function << "(";
				for (auto & op : Arguments)
				{
					sb << op.ToString();
					if (op != Arguments.Last())
						sb << ", ";
				}
				sb << ")";
				return sb.ProduceString();
			}
			virtual String GetOperatorString() override
			{
				return "call " + Function;
			}
			virtual bool HasSideEffect() override
			{
				return SideEffect;
			}
			CallInstruction(int argSize)
			{
				Arguments.SetSize(argSize);
				for (auto & use : Arguments)
					use.SetUser(this);
			}
			CallInstruction(const CallInstruction & other)
				: ILInstruction(other)
			{
				Function = other.Function;
				SideEffect = other.SideEffect;
				Arguments.SetSize(other.Arguments.Count());
				for (int i = 0; i < other.Arguments.Count(); i++)
				{
					Arguments[i].SetUser(this);
					Arguments[i] = other.Arguments[i].Ptr();
				}

			}
			virtual CallInstruction * Clone() override
			{
				return new CallInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class NotInstruction : public UnaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return  Name + " = not " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return "not";
			}
			virtual NotInstruction * Clone() override
			{
				return new NotInstruction(*this);
			}
			NotInstruction() = default;
			NotInstruction(const NotInstruction & other) = default;

			NotInstruction(ILOperand * op)
			{
				Operand = op;
				Type = op->Type->Clone();
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class NegInstruction : public UnaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return  Name + " = neg " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return "neg";
			}
			virtual NegInstruction * Clone() override
			{
				return new NegInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};


		class SwizzleInstruction : public UnaryInstruction
		{
		public:
			String SwizzleString;
			virtual String ToString() override
			{
				return  Name + " = " + Operand.ToString() + "." + SwizzleString;
			}
			virtual String GetOperatorString() override
			{
				return "swizzle";
			}
			virtual SwizzleInstruction * Clone() override
			{
				return new SwizzleInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class BitNotInstruction : public UnaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return  Name + " = bnot " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return "bnot";
			}
			virtual BitNotInstruction * Clone() override
			{
				return new BitNotInstruction(*this);
			}
			BitNotInstruction() = default;
			BitNotInstruction(const BitNotInstruction & instr) = default;

			BitNotInstruction(ILOperand * op)
			{
				Operand = op;
				Type = op->Type->Clone();
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class AddInstruction : public BinaryInstruction
		{
		public:
			AddInstruction() = default;
			AddInstruction(const AddInstruction & instr) = default;
			AddInstruction(ILOperand * v0, ILOperand * v1)
			{
				Operands[0] = v0;
				Operands[1] = v1;
				Type = v0->Type->Clone();
			}
			virtual String ToString() override
			{
				return Name + " = add " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "add";
			}
			virtual AddInstruction * Clone() override
			{
				return new AddInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class MemberLoadInstruction : public BinaryInstruction
		{
		public:
			MemberLoadInstruction() = default;
			MemberLoadInstruction(const MemberLoadInstruction &) = default;
			MemberLoadInstruction(ILOperand * v0, ILOperand * v1)
			{
				Operands[0] = v0;
				Operands[1] = v1;
				if (auto arrType = dynamic_cast<ILArrayType *>(v0->Type.Ptr()))
				{
					Type = arrType->BaseType->Clone();
				}
				else if (auto genType = dynamic_cast<ILGenericType*>(v0->Type.Ptr()))
				{
					Type = genType->BaseType->Clone();
				}
				else if (auto baseType = dynamic_cast<ILBasicType *>(v0->Type.Ptr()))
				{
					switch (baseType->Type)
					{
					case ILBaseType::Float2:
					case ILBaseType::Float3:
					case ILBaseType::Float4:
						Type = new ILBasicType(ILBaseType::Float);
						break;
					case ILBaseType::Float3x3:
						Type = new ILBasicType(ILBaseType::Float3);
						break;
					case ILBaseType::Float4x4:
						Type = new ILBasicType(ILBaseType::Float4);
						break;
					case ILBaseType::Int2:
					case ILBaseType::Int3:
					case ILBaseType::Int4:
						Type = new ILBasicType(ILBaseType::Int);
						break;
					case ILBaseType::UInt2:
					case ILBaseType::UInt3:
					case ILBaseType::UInt4:
						Type = new ILBasicType(ILBaseType::UInt);
						break;
					default:
						throw InvalidOperationException("Unsupported aggregate type.");
					}
				}
				else if (auto structType = dynamic_cast<ILStructType*>(v0->Type.Ptr()))
				{
					auto cv1 = dynamic_cast<ILConstOperand*>(v1);
					if (!cv1)
						throw InvalidProgramException("member field access offset is not constant.");
					if (cv1->IntValues[0] < 0 || cv1->IntValues[0] >= structType->Members.Count())
						throw InvalidProgramException("member field access offset out of bounds.");
					Type = structType->Members[cv1->IntValues[0]].Type;
				}
			}
			virtual String ToString() override
			{
				return Name + " = retrieve " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "retrieve";
			}
			virtual MemberLoadInstruction * Clone() override
			{
				return new MemberLoadInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class SubInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = sub " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "sub";
			}
			virtual SubInstruction * Clone() override
			{
				return new SubInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class MulInstruction : public BinaryInstruction
		{
		public:
			MulInstruction(){}
			MulInstruction(ILOperand * v0, ILOperand * v1)
			{
				Operands[0] = v0;
				Operands[1] = v1;
				Type = v0->Type->Clone();
			}
			MulInstruction(const MulInstruction &) = default;

			virtual String ToString() override
			{
				return Name + " = mul " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "mu";
			}
			virtual MulInstruction * Clone() override
			{
				return new MulInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class DivInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = div " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "div";
			}
			virtual DivInstruction * Clone() override
			{
				return new DivInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class ModInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = mod " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "mod";
			}
			virtual ModInstruction * Clone() override
			{
				return new ModInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class AndInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = and " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "and";
			}
			virtual AndInstruction * Clone() override
			{
				return new AndInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class OrInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = or " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "or";
			}
			virtual OrInstruction * Clone() override
			{
				return new OrInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class BitAndInstruction : public BinaryInstruction
		{
		public:
			BitAndInstruction(){}
			BitAndInstruction(ILOperand * v0, ILOperand * v1)
			{
				Operands[0] = v0;
				Operands[1] = v1;
				Type = v0->Type->Clone();
			}
			BitAndInstruction(const BitAndInstruction &) = default;
			virtual String ToString() override
			{
				return Name + " = band " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "band";
			}
			virtual BitAndInstruction * Clone() override
			{
				return new BitAndInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class BitOrInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = bor " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "bor";
			}
			virtual BitOrInstruction * Clone() override
			{
				return new BitOrInstruction(*this);
			}
			BitOrInstruction(){}
			BitOrInstruction(const BitOrInstruction &) = default;
			BitOrInstruction(ILOperand * v0, ILOperand * v1)
			{
				Operands[0] = v0;
				Operands[1] = v1;
				Type = v0->Type->Clone();
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class BitXorInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = bxor " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "bxor";
			}
			virtual BitXorInstruction * Clone() override
			{
				return new BitXorInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class ShlInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = shl " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "sh";
			}
			virtual ShlInstruction * Clone() override
			{
				return new ShlInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class ShrInstruction : public BinaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = shr " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "shr";
			}
			virtual ShrInstruction * Clone() override
			{
				return new ShrInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class CompareInstruction : public BinaryInstruction
		{};
		class CmpgtInstruction : public CompareInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = gt " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "gt";
			}
			virtual CmpgtInstruction * Clone() override
			{
				return new CmpgtInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class CmpgeInstruction : public CompareInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = ge " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "ge";
			}
			virtual CmpgeInstruction * Clone() override
			{
				return new CmpgeInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class CmpltInstruction : public CompareInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = lt " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "lt";
			}
			virtual CmpltInstruction * Clone() override
			{
				return new CmpltInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class CmpleInstruction : public CompareInstruction
		{
		public:
			CmpleInstruction() = default;
			CmpleInstruction(const CmpleInstruction &) = default;
			CmpleInstruction(ILOperand * v0, ILOperand * v1)
			{
				Operands[0] = v0;
				Operands[1] = v1;
				Type = v0->Type->Clone();
			}

			virtual String ToString() override
			{
				return Name + " = le " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "le";
			}
			virtual CmpleInstruction * Clone() override
			{
				return new CmpleInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class CmpeqlInstruction : public CompareInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = eql " + Operands[0].ToString()
					+ ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "eq";
			}
			virtual CmpeqlInstruction * Clone() override
			{
				return new CmpeqlInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class CmpneqInstruction : public CompareInstruction
		{
		public:
			virtual String ToString() override
			{
				return Name + " = neq " + Operands[0].ToString() + ", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "neq";
			}
			virtual CmpneqInstruction * Clone() override
			{
				return new CmpneqInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class CastInstruction : public UnaryInstruction
		{};

		class Float2IntInstruction : public CastInstruction
		{
		public:
			Float2IntInstruction(){}
			Float2IntInstruction(const Float2IntInstruction &) = default;

			Float2IntInstruction(ILOperand * op)
			{
				Operand = op;
				Type = new ILBasicType(ILBaseType::Int);
			}
		public:
			virtual String ToString() override
			{
				return Name + " = f2i " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return "f2i";
			}
			virtual Float2IntInstruction * Clone() override
			{
				return new Float2IntInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class Int2FloatInstruction : public CastInstruction
		{
		public:
			Int2FloatInstruction(){}
			Int2FloatInstruction(ILOperand * op)
			{
				Operand = op;
				Type = new ILBasicType(ILBaseType::Float);
			}
			Int2FloatInstruction(const Int2FloatInstruction &) = default;

		public:
			virtual String ToString() override
			{
				return Name + " = i2f " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return "i2f";
			}
			virtual Int2FloatInstruction * Clone() override
			{
				return new Int2FloatInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class CopyInstruction : public UnaryInstruction
		{
		public:
			CopyInstruction(){}
			CopyInstruction(const CopyInstruction &) = default;

			CopyInstruction(ILOperand * dest)
			{
				Operand = dest;
				Type = dest->Type->Clone();
			}
		public:
			virtual String ToString() override
			{
				return Name + " = " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return "copy";
			}
			virtual CopyInstruction * Clone() override
			{
				return new CopyInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		// load(src)
		class LoadInstruction : public UnaryInstruction
		{
		public:
			bool Deterministic;
			LoadInstruction()
			{
				Deterministic = false;
			}
			LoadInstruction(const LoadInstruction & other)
				: UnaryInstruction(other)
			{
				Deterministic = other.Deterministic;
			}
			LoadInstruction(ILOperand * dest);
		public:
			virtual String ToString() override
			{
				return Name + " = load " + Operand.ToString();
			}
			virtual bool IsDeterministic() override
			{
				return Deterministic;
			}
			virtual String GetOperatorString() override
			{
				return "ld";
			}
			virtual LoadInstruction * Clone() override
			{
				auto rs = new LoadInstruction(*this);
				if (!rs->Type)
					printf("shit");
				return rs;
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class DiscardInstruction : public ILInstruction
		{
		public:
			virtual bool IsDeterministic() override
			{
				return true;
			}
			virtual bool HasSideEffect() override
			{
				return true;
			}
			virtual String ToString() override
			{
				return  "discard";
			}
			virtual String GetOperatorString() override
			{
				return "discard";
			}
			virtual DiscardInstruction * Clone() override
			{
				return new DiscardInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		// store(dest, value)
		class StoreInstruction : public BinaryInstruction
		{
		public:
			StoreInstruction(){}
			StoreInstruction(const StoreInstruction &) = default;

			StoreInstruction(ILOperand * dest, ILOperand * value)
			{
				Operands.SetSize(2);
				Operands[0] = dest;
				Operands[1] = value;
			}
		public:
			virtual String ToString() override
			{
				return "store " + Operands[0].ToString() + ", " +
					Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "st";
			}
			virtual bool HasSideEffect() override
			{
				return true;
			}
			virtual StoreInstruction * Clone() override
			{
				return new StoreInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class MemberUpdateInstruction : public ILInstruction
		{
		public:
			UseReference Operands[3];
			MemberUpdateInstruction()
			{
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[2].SetUser(this);
			}
			MemberUpdateInstruction(const MemberUpdateInstruction & other)
				: ILInstruction(other)
			{
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[2].SetUser(this);
				Operands[0] = other.Operands[0].Ptr();
				Operands[1] = other.Operands[1].Ptr();
				Operands[2] = other.Operands[2].Ptr();
			}
			MemberUpdateInstruction(ILOperand * var, ILOperand * offset, ILOperand * value)
			{
				Operands[0].SetUser(this);
				Operands[1].SetUser(this);
				Operands[2].SetUser(this);
				Operands[0] = var;
				Operands[1] = offset;
				Operands[2] = value;
				Type = var->Type->Clone();
			}
			virtual OperandIterator begin() override
			{
				return Operands;
			}
			virtual OperandIterator end() override
			{
				return Operands + 3;
			}
			virtual String ToString() override
			{
				return Name + " = update " + Operands[0].ToString() + ", " + Operands[1].ToString() + "," + Operands[2].ToString();
			}
			virtual String GetOperatorString() override
			{
				return "update";
			}
			virtual MemberUpdateInstruction * Clone() override
			{
				return new MemberUpdateInstruction(*this);
			}
			virtual bool HasSideEffect() override
			{
				return true;
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};


		class InstructionVisitor : public Object
		{
		public:
			virtual void VisitAddInstruction(AddInstruction *){}
			virtual void VisitSubInstruction(SubInstruction *){}
			virtual void VisitDivInstruction(DivInstruction *){}
			virtual void VisitMulInstruction(MulInstruction *){}
			virtual void VisitModInstruction(ModInstruction *){}
			virtual void VisitNegInstruction(NegInstruction *){}
			virtual void VisitAndInstruction(AndInstruction *){}
			virtual void VisitOrInstruction(OrInstruction *){}
			virtual void VisitBitAndInstruction(BitAndInstruction *){}
			virtual void VisitBitOrInstruction(BitOrInstruction *){}
			virtual void VisitBitXorInstruction(BitXorInstruction *){}
			virtual void VisitShlInstruction(ShlInstruction *){}
			virtual void VisitShrInstruction(ShrInstruction *){}
			virtual void VisitBitNotInstruction(BitNotInstruction *){}
			virtual void VisitNotInstruction(NotInstruction *){}
			virtual void VisitCmpeqlInstruction(CmpeqlInstruction *){}
			virtual void VisitCmpneqInstruction(CmpneqInstruction *){}
			virtual void VisitCmpltInstruction(CmpltInstruction *){}
			virtual void VisitCmpleInstruction(CmpleInstruction *){}
			virtual void VisitCmpgtInstruction(CmpgtInstruction *){}
			virtual void VisitCmpgeInstruction(CmpgeInstruction *){}

			virtual void VisitLoadInstruction(LoadInstruction *){}
			virtual void VisitStoreInstruction(StoreInstruction *){}
			virtual void VisitCopyInstruction(CopyInstruction *){}

			virtual void VisitAllocVarInstruction(AllocVarInstruction *){}
			virtual void VisitFetchArgInstruction(FetchArgInstruction *){}
			virtual void VisitCastInstruction(CastInstruction *){}
			virtual void VisitInt2FloatInstruction(Int2FloatInstruction *){}
			virtual void VisitFloat2IntInstruction(Float2IntInstruction *){}
			virtual void VisitMemberLoadInstruction(MemberLoadInstruction *){}
			virtual void VisitMemberUpdateInstruction(MemberUpdateInstruction *) {}
			virtual void VisitImportInstruction(ImportInstruction*) {}
			virtual void VisitExportInstruction(ExportInstruction*) {}
			virtual void VisitSelectInstruction(SelectInstruction *){}
			virtual void VisitCallInstruction(CallInstruction *){}
			virtual void VisitSwitchInstruction(SwitchInstruction *){}
			virtual void VisitDiscardInstruction(DiscardInstruction *) {}
			virtual void VisitLoadInputInstruction(LoadInputInstruction *) {}
			virtual void VisitPhiInstruction(PhiInstruction *){}
			virtual void VisitSwizzleInstruction(SwizzleInstruction*) {}
			virtual void VisitProjectInstruction(ProjectInstruction*) {}
		};

		class ForInstruction : public ILInstruction
		{
		public:
			RefPtr<CFGNode> InitialCode, ConditionCode, SideEffectCode, BodyCode;
			virtual int GetSubBlockCount() override
			{
				int count = 0;
				if (InitialCode)
					count++;
				if (ConditionCode)
					count++;
				if (SideEffectCode)
					count++;
				if (BodyCode)
					count++;
				return count;
			}
			virtual CFGNode * GetSubBlock(int i) override
			{
				int id = 0;
				if (InitialCode)
				{
					if (id == i) return InitialCode.Ptr();
					id++;
				}
				if (ConditionCode)
				{
					if (id == i) return ConditionCode.Ptr();
					id++;
				}
				if (SideEffectCode)
				{
					if (id == i) return SideEffectCode.Ptr();
					id++;
				}
				if (BodyCode)
				{
					if (id == i) return BodyCode.Ptr();
				}
				return nullptr;
			}

			virtual String ToString() override
			{
				StringBuilder sb;
				sb << "for (" << InitialCode->ToString() << "; " << ConditionCode->ToString() << "; ";
				sb << SideEffectCode->ToString() << ")" << EndLine;
				sb << "{" << EndLine;
				sb << BodyCode->ToString() << EndLine;
				sb << "}" << EndLine;
				return sb.ProduceString();
			}
		};
		class IfInstruction : public UnaryInstruction
		{
		public:
			RefPtr<CFGNode> TrueCode, FalseCode;
			virtual int GetSubBlockCount() override
			{
				if (FalseCode)
					return 2;
				else
					return 1;
			}
			virtual CFGNode * GetSubBlock(int i) override
			{
				if (i == 0)
					return TrueCode.Ptr();
				else if (i == 1)
					return FalseCode.Ptr();
				return nullptr;
			}

			virtual String ToString() override
			{
				StringBuilder sb;
				sb << "if (" << Operand->ToString() << ")" << EndLine;
				sb << "{" << EndLine;
				sb << TrueCode->ToString() << EndLine;
				sb << "}" << EndLine;
				if (FalseCode)
				{
					sb << "else" << EndLine;
					sb << "{" << EndLine;
					sb << FalseCode->ToString() << EndLine;
					sb << "}" << EndLine;
				}
				return sb.ProduceString();
			}
		};
		class WhileInstruction : public ILInstruction
		{
		public:
			RefPtr<CFGNode> ConditionCode, BodyCode;
			virtual int GetSubBlockCount() override
			{
				return 2;
			}
			virtual CFGNode * GetSubBlock(int i) override
			{
				if (i == 0)
					return ConditionCode.Ptr();
				else if (i == 1)
					return BodyCode.Ptr();
				return nullptr;
			}

			virtual String ToString() override
			{
				StringBuilder sb;
				sb << "while (" << ConditionCode->ToString() << ")" << EndLine;
				sb << "{" << EndLine;
				sb << BodyCode->ToString();
				sb << "}" << EndLine;
				return sb.ProduceString();
			}
		};
		class DoInstruction : public ILInstruction
		{
		public:
			RefPtr<CFGNode> ConditionCode, BodyCode;
			virtual int GetSubBlockCount() override
			{
				return 2;
			}
			virtual CFGNode * GetSubBlock(int i) override
			{
				if (i == 1)
					return ConditionCode.Ptr();
				else if (i == 0)
					return BodyCode.Ptr();
				return nullptr;
			}

			virtual String ToString() override
			{
				StringBuilder sb;
				sb << "{" << EndLine;
				sb << BodyCode->ToString();
				sb << "}" << EndLine;
				sb << "while (" << ConditionCode->ToString() << ")" << EndLine;
				return sb.ProduceString();
			}
		};
		class ReturnInstruction : public UnaryInstruction
		{
		public:
			ReturnInstruction(ILOperand * op)
				:UnaryInstruction()
			{
				Operand = op;
			}

			virtual String ToString() override
			{
				return "return " + Operand->ToString() + ";";
			}
		};
		class BreakInstruction : public ILInstruction
		{};
		class ContinueInstruction : public ILInstruction
		{};

		class KeyHoleNode
		{
		public:
			String NodeType;
			int CaptureId = -1;
			List<RefPtr<KeyHoleNode>> Children;
			bool Match(List<ILOperand*> & matchResult, ILOperand * instr);
			static RefPtr<KeyHoleNode> Parse(String format);
		};
	}
}

#endif