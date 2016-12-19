#include "IL.h"
#include "../CoreLib/LibIO.h"
#include "Syntax.h"
#include "CompiledProgram.h"
#include "../CoreLib/Tokenizer.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::IO;

		RefPtr<ILType> BaseTypeFromString(CoreLib::Text::TokenReader & parser)
		{
			if (parser.LookAhead("int"))
				return new ILBasicType(ILBaseType::Int);
			else if (parser.LookAhead("uint"))
				return new ILBasicType(ILBaseType::UInt);
			else if (parser.LookAhead("uvec2"))
				return new ILBasicType(ILBaseType::UInt2);
			else if (parser.LookAhead("uvec3"))
				return new ILBasicType(ILBaseType::UInt3);
			else if (parser.LookAhead("uvec4"))
				return new ILBasicType(ILBaseType::UInt4);
			if (parser.LookAhead("float"))
				return new ILBasicType(ILBaseType::Float);
			if (parser.LookAhead("vec2"))
				return new ILBasicType(ILBaseType::Float2);
			if (parser.LookAhead("vec3"))
				return new ILBasicType(ILBaseType::Float3);
			if (parser.LookAhead("vec4"))
				return new ILBasicType(ILBaseType::Float4);
			if (parser.LookAhead("ivec2"))
				return new ILBasicType(ILBaseType::Int2);
			if (parser.LookAhead("mat3"))
				return new ILBasicType(ILBaseType::Float3x3);
			if (parser.LookAhead("mat4"))
				return new ILBasicType(ILBaseType::Float4x4);
			if (parser.LookAhead("ivec3"))
				return new ILBasicType(ILBaseType::Int3);
			if (parser.LookAhead("ivec4"))
				return new ILBasicType(ILBaseType::Int4);
			if (parser.LookAhead("sampler2D") || parser.LookAhead("Texture2D"))
				return new ILBasicType(ILBaseType::Texture2D);
			if (parser.LookAhead("samplerCube") || parser.LookAhead("TextureCube"))
				return new ILBasicType(ILBaseType::TextureCube);
			if (parser.LookAhead("sampler2DArray") || parser.LookAhead("Texture2DArray"))
				return new ILBasicType(ILBaseType::Texture2DArray);
			if (parser.LookAhead("sampler2DShadow") || parser.LookAhead("Texture2DShadow"))
				return new ILBasicType(ILBaseType::Texture2DShadow);
			if (parser.LookAhead("samplerCubeShadow") || parser.LookAhead("TextureCubeShadow"))
				return new ILBasicType(ILBaseType::TextureCubeShadow);
			if (parser.LookAhead("sampler2DArrayShadow") || parser.LookAhead("Texture2DArrayShadow"))
				return new ILBasicType(ILBaseType::Texture2DArrayShadow);
			if (parser.LookAhead("sampler3D") || parser.LookAhead("Texture3D"))
				return new ILBasicType(ILBaseType::Texture3D);
			if (parser.LookAhead("bool"))
				return new ILBasicType(ILBaseType::Bool);
			return nullptr;
		}

		int RoundToAlignment(int offset, int alignment)
		{
			int remainder = offset % alignment;
			if (remainder == 0)
				return offset;
			else
				return offset + (alignment - remainder);
		}

		int GetMaxResourceBindings(BindableResourceType type)
		{
			switch (type)
			{
			case BindableResourceType::Texture:
				return 32;
			case BindableResourceType::Sampler:
				return 32;
			case BindableResourceType::Buffer:
				return 16;
			case BindableResourceType::StorageBuffer:
				return 16;
			}
			return 0;
		}

		int SizeofBaseType(ILBaseType type)
		{
			if (type == ILBaseType::Int)
				return 4;
			if (type == ILBaseType::UInt)
				return 4;
			if (type == ILBaseType::UInt2)
				return 8;
			if (type == ILBaseType::UInt3)
				return 12;
			if (type == ILBaseType::UInt4)
				return 16;
			else if (type == ILBaseType::Int2)
				return 8;
			else if (type == ILBaseType::Int3)
				return 12;
			else if (type == ILBaseType::Int4)
				return 16;
			else if (type == ILBaseType::Float)
				return 4;
			else if (type == ILBaseType::Float2)
				return 8;
			else if (type == ILBaseType::Float3)
				return 12;
			else if (type == ILBaseType::Float4)
				return 16;
			else if (type == ILBaseType::Float3x3)
				return 48;
			else if (type == ILBaseType::Float4x4)
				return 64;
			else if (type == ILBaseType::Texture2D)
				return 8;
			else if (type == ILBaseType::TextureCube)
				return 8;
			else if (type == ILBaseType::Texture2DArray)
				return 8;
			else if (type == ILBaseType::Texture2DShadow)
				return 8;
			else if (type == ILBaseType::TextureCubeShadow)
				return 8;
			else if (type == ILBaseType::Texture2DArrayShadow)
				return 8;
			else if (type == ILBaseType::Bool)
				return 4;
			else
				return 0;
		}

		bool ILType::IsBool()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Bool;
			else
				return false;
		}

		bool ILType::IsInt()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Int;
			else
				return false;
		}

		bool ILType::IsUInt()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::UInt;
			else
				return false;
		}

		bool ILType::IsIntegral()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Int || basicType->Type == ILBaseType::Int2 || basicType->Type == ILBaseType::Int3 || basicType->Type == ILBaseType::Int4
				|| basicType->Type == ILBaseType::UInt || basicType->Type == ILBaseType::UInt2 || basicType->Type == ILBaseType::UInt3 || basicType->Type == ILBaseType::UInt4 ||
				basicType->Type == ILBaseType::Bool;
			else
				return false;
		}

		bool ILType::IsVoid()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Void;
			else
				return false;
		}

		bool ILType::IsFloat()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Float;
			else
				return false;
		}

		bool ILType::IsBoolVector()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Bool2 || basicType->Type == ILBaseType::Bool3 || basicType->Type == ILBaseType::Bool4;
			else
				return false;
		}

		bool ILType::IsIntVector()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Int2 || basicType->Type == ILBaseType::Int3 || basicType->Type == ILBaseType::Int4;
			else
				return false;
		}

		bool ILType::IsUIntVector()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::UInt2 || basicType->Type == ILBaseType::UInt3 || basicType->Type == ILBaseType::UInt4;
			else
				return false;
		}

		bool ILType::IsFloatVector()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Float2 || basicType->Type == ILBaseType::Float3 || basicType->Type == ILBaseType::Float4 ||
				basicType->Type == ILBaseType::Float3x3 || basicType->Type == ILBaseType::Float4x4;
			else
				return false;
		}

		bool ILType::IsFloatMatrix()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Float3x3 || basicType->Type == ILBaseType::Float4x4;
			else
				return false;
		}

		bool ILType::IsNonShadowTexture()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Texture2D || basicType->Type == ILBaseType::TextureCube || basicType->Type == ILBaseType::Texture2DArray ||
				basicType->Type == ILBaseType::Texture3D;
			else
				return false;
		}

		bool ILType::IsTexture()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Texture2D || basicType->Type == ILBaseType::TextureCube || basicType->Type == ILBaseType::Texture2DArray ||
				basicType->Type == ILBaseType::Texture2DShadow || basicType->Type == ILBaseType::TextureCubeShadow || basicType->Type == ILBaseType::Texture2DArrayShadow ||
				basicType->Type == ILBaseType::Texture3D;
			else
				return false;
		}

		bool ILType::IsSamplerState()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::SamplerState;
			else
				return false;
		}

		int ILType::GetVectorSize()
		{
			if (auto basicType = dynamic_cast<ILBasicType*>(this))
			{
				switch (basicType->Type)
				{
				case ILBaseType::Int2:
				case ILBaseType::Float2:
				case ILBaseType::UInt2:
					return 2;
				case ILBaseType::Int3:
				case ILBaseType::Float3:
				case ILBaseType::UInt3:
					return 3;
				case ILBaseType::Int4:
				case ILBaseType::Float4:
				case ILBaseType::UInt4:
					return 4;
				case ILBaseType::Float3x3:
					return 9;
				case ILBaseType::Float4x4:
					return 16;
				default:
					return 1;
				}
			}
			return 1;
		}

		RefPtr<ILType> DeserializeBasicType(CoreLib::Text::TokenReader & reader)
		{
			reader.Read("basic");
			auto rs = BaseTypeFromString(reader);
			reader.ReadWord();
			return rs;
		}
		RefPtr<ILType> DeserializeStructType(CoreLib::Text::TokenReader & reader)
		{
			reader.Read("struct");
			RefPtr<ILStructType> rs = new ILStructType();
			rs->TypeName = reader.ReadToken().Content;
			reader.Read("(");
			while (reader.LookAhead(")"))
			{
				ILStructType::ILStructField field;
				field.FieldName = reader.ReadToken().Content;
				reader.Read(":");
				field.Type = ILType::Deserialize(reader);
				reader.Read(";");
			}
			reader.Read(")");
			return rs;
		}
		RefPtr<ILType> DeserializeArrayType(CoreLib::Text::TokenReader & reader)
		{
			reader.Read("array");
			reader.Read("(");
			RefPtr<ILArrayType> rs = new ILArrayType();
			rs->BaseType = ILType::Deserialize(reader);
			reader.Read(",");
			rs->ArrayLength = reader.ReadInt();
			reader.Read(")");
			return rs;
		}
		RefPtr<ILType> DeserializeGenericType(CoreLib::Text::TokenReader & reader)
		{
			reader.Read("generic");
			RefPtr<ILGenericType> rs = new ILGenericType();
			rs->GenericTypeName = reader.ReadWord();
			reader.Read("(");
			rs->BaseType = ILType::Deserialize(reader);
			reader.Read(")");
			return rs;
		}
		RefPtr<ILType> DeserializeRecordType(CoreLib::Text::TokenReader & reader)
		{
			reader.Read("record");
			RefPtr<ILRecordType> rs = new ILRecordType();
			rs->TypeName = reader.ReadWord();
			return rs;
		}
		RefPtr<ILType> ILType::Deserialize(CoreLib::Text::TokenReader & reader)
		{
			if (reader.LookAhead("basic"))
				return DeserializeBasicType(reader);
			else if (reader.LookAhead("struct"))
				return DeserializeStructType(reader);
			else if (reader.LookAhead("array"))
				return DeserializeArrayType(reader);
			else if (reader.LookAhead("generic"))
				return DeserializeGenericType(reader);
			else if (reader.LookAhead("record"))
				return DeserializeRecordType(reader);
			return nullptr;
		}

		bool CFGNode::HasPhiInstruction()
		{
			return headInstr && headInstr->GetNext() && headInstr->GetNext()->Is<PhiInstruction>();
		}

		ILInstruction * CFGNode::GetFirstNonPhiInstruction()
		{
			for (auto & instr : *this)
			{
				if (!instr.Is<PhiInstruction>())
					return &instr;
			}
			return tailInstr;
		}

		int NamingCounter = 0;

		void CFGNode::NameAllInstructions()
		{
			// name all operands
			StringBuilder numBuilder;
			for (auto & instr : GetAllInstructions())
			{
				numBuilder.Clear();
				for (auto & c : instr.Name)
				{
					if (c >= '0' && c <= '9')
						numBuilder.Append(c);
					else
						numBuilder.Clear();
				}
				auto num = numBuilder.ToString();
				if (num.Length())
				{
					int id = StringToInt(num);
					NamingCounter = Math::Max(NamingCounter, id + 1);
				}
			}
			HashSet<String> existingNames;
			for (auto & instr : GetAllInstructions())
			{
				if (instr.Name.Length() == 0)
					instr.Name = String("t") + String(NamingCounter++, 16);
				else
				{
					int counter = 1;
					String newName = instr.Name;
					while (existingNames.Contains(newName))
					{
						newName = instr.Name + String(counter);
						counter++;
					}
					instr.Name = newName;
				}
				existingNames.Add(instr.Name);
			}
		}

		void CFGNode::DebugPrint()
		{
			printf("===========\n");
			for (auto& instr : *this)
			{
				printf("%S\n", instr.ToString().ToWString());
			}
			printf("===========\n");
		}

		LoadInstruction::LoadInstruction(ILOperand * dest)
		{
			Deterministic = false;
			Operand = dest;
			Type = dest->Type->Clone();
			if (!Spire::Compiler::Is<AllocVarInstruction>(dest) && !Spire::Compiler::Is<FetchArgInstruction>(dest))
				throw "invalid address operand";
		}
		void MemberUpdateInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitMemberUpdateInstruction(this);
		}
		void SubInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitSubInstruction(this);
		}
		void MulInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitMulInstruction(this);
		}
		void DivInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitDivInstruction(this);
		}
		void ModInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitModInstruction(this);
		}
		void AndInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitAndInstruction(this);
		}
		void OrInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitOrInstruction(this);
		}
		void BitAndInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitBitAndInstruction(this);
		}
		void BitOrInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitBitOrInstruction(this);
		}
		void BitXorInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitBitXorInstruction(this);
		}
		void ShlInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitShlInstruction(this);
		}
		void ShrInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitShrInstruction(this);
		}
		void CmpgtInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCmpgtInstruction(this);
		}
		void CmpgeInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCmpgeInstruction(this);
		}
		void CmpltInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCmpltInstruction(this);
		}
		void CmpleInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCmpleInstruction(this);
		}
		void CmpeqlInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCmpeqlInstruction(this);
		}
		void CmpneqInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCmpneqInstruction(this);
		}
		void Float2IntInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitFloat2IntInstruction(this);
		}
		void Int2FloatInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitInt2FloatInstruction(this);
		}
		void CopyInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCopyInstruction(this);
		}
		void LoadInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitLoadInstruction(this);
		}
		void StoreInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitStoreInstruction(this);
		}
		void AllocVarInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitAllocVarInstruction(this);
		}
		void FetchArgInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitFetchArgInstruction(this);
		}
		void PhiInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitPhiInstruction(this);
		}
		void SelectInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitSelectInstruction(this);
		}
		void CallInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitCallInstruction(this);
		}
		void SwitchInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitSwitchInstruction(this);
		}
		void NotInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitNotInstruction(this);
		}
		void NegInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitNegInstruction(this);
		}
		void BitNotInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitBitNotInstruction(this);
		}
		void AddInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitAddInstruction(this);
		}
		void MemberLoadInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitMemberLoadInstruction(this);
		}
		AllInstructionsIterator & AllInstructionsIterator::operator++()
		{
			if (subBlockPtr < curInstr->GetSubBlockCount())
			{
				StackItem item;
				item.instr = curInstr;
				item.subBlockPtr = subBlockPtr + 1;
				stack.Add(item);
				curInstr = curInstr->GetSubBlock(subBlockPtr)->begin().Current;
				subBlockPtr = 0;
			}
			else
				curInstr = curInstr->GetNext();
			while (curInstr->GetNext() == nullptr && stack.Count() > 0)
			{
				auto item = stack.Last();
				stack.RemoveAt(stack.Count() - 1);
				curInstr = item.instr;
				subBlockPtr = item.subBlockPtr;
				if (subBlockPtr >= curInstr->GetSubBlockCount())
				{
					subBlockPtr = 0;
					curInstr = curInstr->GetNext();
				}
			}
			return *this;
		}
		AllInstructionsIterator AllInstructionsCollection::begin()
		{
			return AllInstructionsIterator(node->begin().Current);
		}
		AllInstructionsIterator AllInstructionsCollection::end()
		{
			return AllInstructionsIterator(node->end().Current);
		}
		String ImportInstruction::ToString()
		{
			StringBuilder rs;
			rs << Name << " = import [" << ComponentName << "](";
			for (auto & arg : Arguments)
			{
				rs << arg->ToString() << ", ";
			}
			rs << ")";
			rs << "\n{";
			rs << ImportOperator->ToString() << "}\n";
			return rs.ProduceString();
		}
		String ImportInstruction::GetOperatorString()
		{
			return "import";
		}
		void ImportInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitImportInstruction(this);
		}
		void ExportInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitExportInstruction(this);
		}
		ILType * ILStructType::Clone()
		{
			auto rs = new ILStructType(*this);
			rs->Members.Clear();
			for (auto & m : Members)
			{
				ILStructField f;
				f.FieldName = m.FieldName;
				f.Type = m.Type->Clone();
				rs->Members.Add(f);
			}
			return rs;
		}
		String ILStructType::ToString()
		{
			return TypeName;
		}
		bool ILStructType::Equals(ILType * type)
		{
			auto st = dynamic_cast<ILStructType*>(type);
			if (st && st->TypeName == this->TypeName)
				return true;
			return false;
		}
		ILType * ILRecordType::Clone()
		{
			auto rs = new ILRecordType(*this);
			rs->Members.Clear();
			for (auto & m : Members)
			{
				ILObjectDefinition f;
				f.Type = m.Value.Type->Clone();
				f.Name = m.Value.Name;
				f.Attributes = m.Value.Attributes;
				rs->Members.Add(m.Key, f);
			}
			return rs;
		}
		String ILRecordType::ToString()
		{
			return TypeName;
		}
		bool ILRecordType::Equals(ILType * type)
		{
			auto recType = dynamic_cast<ILRecordType*>(type);
			if (recType)
				return TypeName == recType->TypeName;
			else
				return false;
		}
		void DiscardInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitDiscardInstruction(this);
		}
		void LoadInputInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitLoadInputInstruction(this);
		}
		void SwizzleInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitSwizzleInstruction(this);
		}
		void ProjectInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitProjectInstruction(this);
		}
}
}