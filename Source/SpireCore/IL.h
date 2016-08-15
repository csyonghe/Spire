#ifndef RASTER_RENDERER_IL_H
#define RASTER_RENDERER_IL_H

#include "../CoreLib/Basic.h"
#include "../CoreLib/Parser.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::Basic;

		const int MaxSIMDSize = 8;

		enum ILBaseType
		{
			Int = 16, Int2 = 17, Int3 = 18, Int4 = 19,
			Float = 32, Float2 = 33, Float3 = 34, Float4 = 35,
			Float3x3 = 40, Float4x4 = 47,
			Texture2D = 48,
			TextureShadow = 49,
			TextureCube = 50,
			TextureCubeShadow = 51,
			UInt = 512, UInt2 = 513, UInt3 = 514, UInt4 = 515,
		};
		int SizeofBaseType(ILBaseType type);
		int RoundToAlignment(int offset, int alignment);
		extern int NamingCounter;
		class ILType : public Object
		{
		public:
			bool IsInt();
			bool IsIntegral();
			bool IsFloat();
			bool IsIntVector();
			bool IsFloatVector();
			bool IsFloatMatrix();
			bool IsVector()
			{
				return IsIntVector() || IsFloatVector();
			}
			bool IsTexture();
			bool IsNonShadowTexture();
			int GetVectorSize();
			virtual ILType * Clone() = 0;
			virtual String ToString() = 0;
			virtual bool Equals(ILType* type) = 0;
			virtual int GetSize() = 0;
			virtual int GetAlignment() = 0;
		};

		RefPtr<ILType> TypeFromString(CoreLib::Text::Parser & parser);

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

			virtual ILType * Clone() override
			{
				auto rs = new ILBasicType();
				rs->Type = Type;
				return rs;
			}
			virtual String ToString() override
			{
				if (Type == ILBaseType::Int)
					return L"int";
				else if (Type == ILBaseType::UInt)
					return L"uint";
				else if (Type == ILBaseType::UInt2)
					return L"uvec2";
				else if (Type == ILBaseType::UInt3)
					return L"uvec3";
				else if (Type == ILBaseType::UInt4)
					return L"uvec4";
				else if (Type == ILBaseType::Int2)
					return L"ivec2";
				else if (Type == ILBaseType::Int3)
					return L"ivec3";
				else if (Type == ILBaseType::Int4)
					return L"ivec4";
				else if (Type == ILBaseType::Float)
					return L"float";
				else if (Type == ILBaseType::Float2)
					return L"vec2";
				else if (Type == ILBaseType::Float3)
					return L"vec3";
				else if (Type == ILBaseType::Float4)
					return L"vec4";
				else if (Type == ILBaseType::Float3x3)
					return L"mat3";
				else if (Type == ILBaseType::Float4x4)
					return L"mat4";
				else if (Type == ILBaseType::Texture2D)
					return L"sampler2D";
				else if (Type == ILBaseType::TextureCube)
					return L"samplerCube";
				else if (Type == ILBaseType::TextureCubeShadow)
					return L"samplerCubeShadow";
				else if (Type == ILBaseType::TextureShadow)
					return L"sampler2DShadow";
				else
					return L"?unkown";
			}
			virtual int GetAlignment() override
			{
				switch (Type)
				{
				case ILBaseType::Int:
					return 4;
				case ILBaseType::UInt:
					return 4;
				case ILBaseType::Int2:
				case ILBaseType::UInt2:
					return 8;
				case ILBaseType::Int3:
				case ILBaseType::UInt3:
					return 16;
				case ILBaseType::Int4:
				case ILBaseType::UInt4:
					return 16;
				case ILBaseType::Float:
					return 4;
				case ILBaseType::Float2:
					return 8;
				case  ILBaseType::Float3:
					return 16;
				case ILBaseType::Float4:
					return 16;
				case ILBaseType::Float3x3:
					return 16;
				case  ILBaseType::Float4x4:
					return 16;
				case ILBaseType::Texture2D:
					return 8;
				case ILBaseType::TextureCube:
					return 8;
				case ILBaseType::TextureCubeShadow:
					return 8;
				case ILBaseType::TextureShadow:
					return 8;
				default:
					return 0;
				}
			}
			virtual int GetSize() override
			{
				switch (Type)
				{
				case ILBaseType::Float:
				case ILBaseType::Int:
				case ILBaseType::UInt:
					return 4;
				case ILBaseType::Float2:
				case ILBaseType::Int2:
				case ILBaseType::UInt2:
					return 8;
				case ILBaseType::Int3:
				case ILBaseType::Float3:
				case ILBaseType::UInt3:
					return 12;
				case ILBaseType::Int4:
				case ILBaseType::Float4:
				case ILBaseType::UInt4:
					return 16;
				case ILBaseType::Float3x3:
					return 48;
				case ILBaseType::Float4x4:
					return 64;
				case ILBaseType::Texture2D:
				case ILBaseType::TextureCube:
				case ILBaseType::TextureCubeShadow:
				case ILBaseType::TextureShadow:
					return 8;
				default:
					return 0;
				}
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
			virtual String ToString() override
			{
				if (ArrayLength > 0)
					return BaseType->ToString() + L"[" + String(ArrayLength) + L"]";
				else
					return BaseType->ToString() + L"[]";
			}
			virtual int GetSize() override
			{
				return BaseType->GetSize() * ArrayLength;
			}
			virtual int GetAlignment() override
			{
				return BaseType->GetAlignment();
			}
		};

		class ILStructType : public ILType
		{
		public:
			String TypeName;
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
			virtual int GetSize() override;
			virtual int GetAlignment() override;
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
				return L"<operand>";
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
				Name = L"<undef>";
			}
			virtual String ToString() override
			{
				return L"<undef>";
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
						throw InvalidOperationException(L"user not initialized.");
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
						throw InvalidOperationException(L"user not initialized.");
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
					return L"<null>";
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
					return String(FloatValues[0]) + L"f";
				else if (Type->IsInt())
					return String(IntValues[0]);
				else if (auto baseType = dynamic_cast<ILBasicType*>(Type.Ptr()))
				{
					StringBuilder sb(256);
					if (baseType->Type == ILBaseType::Float2)
						sb << L"vec2(" << FloatValues[0] << L"f, " << FloatValues[1] << L"f)";
					else if (baseType->Type == ILBaseType::Float3)
						sb << L"vec3(" << FloatValues[0] << L"f, " << FloatValues[1] << L"f, " << FloatValues[2] << L"f)";
					else if (baseType->Type == ILBaseType::Float4)
						sb << L"vec4(" << FloatValues[0] << L"f, " << FloatValues[1] << L"f, " << FloatValues[2] << L"f, " << FloatValues[3] << L"f)";
					else if (baseType->Type == ILBaseType::Float3x3)
						sb << L"mat3(...)";
					else if (baseType->Type == ILBaseType::Float4x4)
						sb << L"mat4(...)";
					else if (baseType->Type == ILBaseType::Int2)
						sb << L"ivec2(" << IntValues[0] << L", " << IntValues[1] << L")";
					else if (baseType->Type == ILBaseType::Int3)
						sb << L"ivec3(" << IntValues[0] << L", " << IntValues[1] << L", " << IntValues[2] << L")";
					else if (baseType->Type == ILBaseType::Int4)
						sb << L"ivec4(" << IntValues[0] << L", " << IntValues[1] << L", " << IntValues[2] << L", " << IntValues[3] << L")";
					else if (baseType->Type == ILBaseType::UInt2)
						sb << L"uvec2(" << IntValues[0] << L", " << IntValues[1] << L")";
					else if (baseType->Type == ILBaseType::UInt3)
						sb << L"uvec3(" << IntValues[0] << L", " << IntValues[1] << L", " << IntValues[2] << L")";
					else if (baseType->Type == ILBaseType::UInt4)
						sb << L"uvec4(" << IntValues[0] << L", " << IntValues[1] << L", " << IntValues[2] << L", " << IntValues[3] << L")";
					return sb.ToString();
				}
				else
					throw InvalidOperationException(L"Illegal constant.");
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
				return L"<instruction>";
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
					throw InvalidOperationException(L"All uses must be removed before removing this instruction");
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
				sb << L" = switch ";
				for (auto & op : Candidates)
				{
					sb << op.ToString();
					if (op != Candidates.Last())
						sb << L", ";
				}
				return sb.ProduceString();
			}
			virtual String GetOperatorString() override
			{
				return L"switch";
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

		// retrieves pointer to a global entry
		class GLeaInstruction : public LeaInstruction
		{
		public:
			String VariableName;
			GLeaInstruction() = default;
			GLeaInstruction(const GLeaInstruction &) = default;
			GLeaInstruction(const String & varName)
				:VariableName(varName)
			{
			}
			virtual String ToString() override
			{
				return Name + L" = g_var [" + VariableName + L"]";
			}
			virtual String GetOperatorString() override
			{
				return L"glea " + VariableName;
			}
			virtual GLeaInstruction * Clone() override
			{
				return new GLeaInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};
		class ImportOperatorDefSyntaxNode;
		class CompiledWorld;
		class ImportInstruction : public LeaInstruction
		{
		public:
			String ComponentName;
			ImportOperatorDefSyntaxNode * ImportOperator;
			CompiledWorld * SourceWorld;

			List<UseReference> Arguments;
			virtual OperandIterator begin() override
			{
				return Arguments.begin();
			}
			virtual OperandIterator end() override
			{
				return Arguments.end();
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

			ImportInstruction(int argSize, String compName, ImportOperatorDefSyntaxNode * importOp, CompiledWorld * srcWorld, RefPtr<ILType> type)
				:ImportInstruction(argSize)
			{
				this->ComponentName = compName;
				this->ImportOperator = importOp;
				this->SourceWorld = srcWorld;
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
					throw ArgumentException(L"type cannot be null.");
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
				return Name + L" = VAR " + Type->ToString() + L", " + Size.ToString();
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
				return L"avar";
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
				return Name + L" = ARG " + Type->ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"arg " + String(ArgId);
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
				return L"phi";
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
				sb << Name << L" = phi ";
				for (auto & op : Operands)
				{
					if (op)
					{
						sb << op.ToString();
					}
					else
						sb << L"<?>";
					sb << L", ";
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

		class ExportInstruction : public UnaryInstruction
		{
		public:
			String ComponentName;
			String ExportOperator;
			CompiledWorld * World;

			ExportInstruction() = default;
			ExportInstruction(const ExportInstruction &) = default;

			ExportInstruction(String compName, String exportOp, CompiledWorld * srcWorld, ILOperand * value)
				: UnaryInstruction()
			{
				this->Operand = value;
				this->ComponentName = compName;
				this->ExportOperator = exportOp;
				this->World = srcWorld;
				this->Type = value->Type;
			}
			virtual String ToString() override
			{
				return L"export<" + ExportOperator + L">[" + ComponentName + L"], " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"export<" + ExportOperator + L">[" + ComponentName + L"]";
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
				return Name + L" = select " + Operands[0].ToString() + L": " + Operands[1].ToString() + L", " + Operands[2].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"select";
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
				sb << L" = call " << Function << L"(";
				for (auto & op : Arguments)
				{
					sb << op.ToString();
					if (op != Arguments.Last())
						sb << L", ";
				}
				sb << L")";
				return sb.ProduceString();
			}
			virtual String GetOperatorString() override
			{
				return L"call " + Function;
			}
			virtual bool HasSideEffect() override
			{
				return false;
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
				return  Name + L" = not " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"not";
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
				return  Name + L" = neg " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"neg";
			}
			virtual NegInstruction * Clone() override
			{
				return new NegInstruction(*this);
			}
			virtual void Accept(InstructionVisitor * visitor) override;
		};

		class BitNotInstruction : public UnaryInstruction
		{
		public:
			virtual String ToString() override
			{
				return  Name + L" = bnot " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"bnot";
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
				return Name + L" = add " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"add";
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
						throw InvalidOperationException(L"Unsupported aggregate type.");
					}
				}
				else if (auto structType = dynamic_cast<ILStructType*>(v0->Type.Ptr()))
				{
					auto cv1 = dynamic_cast<ILConstOperand*>(v1);
					if (!cv1)
						throw InvalidProgramException(L"member field access offset is not constant.");
					if (cv1->IntValues[0] < 0 || cv1->IntValues[0] >= structType->Members.Count())
						throw InvalidProgramException(L"member field access offset out of bounds.");
					Type = structType->Members[cv1->IntValues[0]].Type;
				}
			}
			virtual String ToString() override
			{
				return Name + L" = retrieve " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"retrieve";
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
				return Name + L" = sub " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"sub";
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
				return Name + L" = mul " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"mul";
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
				return Name + L" = div " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"div";
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
				return Name + L" = mod " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"mod";
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
				return Name + L" = and " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"and";
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
				return Name + L" = or " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"or";
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
				return Name + L" = band " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"band";
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
				return Name + L" = bor " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"bor";
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
				return Name + L" = bxor " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"bxor";
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
				return Name + L" = shl " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"shl";
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
				return Name + L" = shr " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"shr";
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
				return Name + L" = gt " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"gt";
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
				return Name + L" = ge " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"ge";
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
				return Name + L" = lt " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"lt";
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
				return Name + L" = le " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"le";
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
				return Name + L" = eql " + Operands[0].ToString()
					+ L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"eql";
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
				return Name + L" = neq " + Operands[0].ToString() + L", " + Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"neq";
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
				return Name + L" = f2i " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"f2i";
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
				return Name + L" = i2f " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"i2f";
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
				return Name + L" = " + Operand.ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"copy";
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
				return Name + L" = load " + Operand.ToString();
			}
			virtual bool IsDeterministic() override
			{
				return Deterministic;
			}
			virtual String GetOperatorString() override
			{
				return L"ld";
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
				return  L"discard";
			}
			virtual String GetOperatorString() override
			{
				return L"discard";
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
				return L"store " + Operands[0].ToString() + L", " +
					Operands[1].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"st";
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
				return Name + L" = update " + Operands[0].ToString() + L", " + Operands[1].ToString() + L"," + Operands[2].ToString();
			}
			virtual String GetOperatorString() override
			{
				return L"update";
			}
			virtual MemberUpdateInstruction * Clone() override
			{
				return new MemberUpdateInstruction(*this);
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
			virtual void VisitGLeaInstruction(GLeaInstruction *){}
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

			virtual void VisitPhiInstruction(PhiInstruction *){}
		};

		class ForInstruction : public ILInstruction
		{
		public:
			RefPtr<CFGNode> ConditionCode, SideEffectCode, BodyCode;
			virtual int GetSubBlockCount()
			{
				return 3;
			}
			virtual CFGNode * GetSubBlock(int i)
			{
				if (i == 0)
					return ConditionCode.Ptr();
				else if (i == 1)
					return SideEffectCode.Ptr();
				else if (i == 2)
					return BodyCode.Ptr();
				return nullptr;
			}
		};
		class IfInstruction : public UnaryInstruction
		{
		public:
			RefPtr<CFGNode> TrueCode, FalseCode;
			virtual int GetSubBlockCount()
			{
				if (FalseCode)
					return 2;
				else
					return 1;
			}
			virtual CFGNode * GetSubBlock(int i)
			{
				if (i == 0)
					return TrueCode.Ptr();
				else if (i == 1)
					return FalseCode.Ptr();
				return nullptr;
			}
		};
		class WhileInstruction : public ILInstruction
		{
		public:
			RefPtr<CFGNode> ConditionCode, BodyCode;
			virtual int GetSubBlockCount()
			{
				return 2;
			}
			virtual CFGNode * GetSubBlock(int i)
			{
				if (i == 0)
					return ConditionCode.Ptr();
				else if (i == 1)
					return BodyCode.Ptr();
				return nullptr;
			}
		};
		class DoInstruction : public ILInstruction
		{
		public:
			RefPtr<CFGNode> ConditionCode, BodyCode;
			virtual int GetSubBlockCount()
			{
				return 2;
			}
			virtual CFGNode * GetSubBlock(int i)
			{
				if (i == 1)
					return ConditionCode.Ptr();
				else if (i == 0)
					return BodyCode.Ptr();
				return nullptr;
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