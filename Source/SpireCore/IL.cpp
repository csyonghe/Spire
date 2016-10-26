#include "IL.h"
#include "../CoreLib/LibIO.h"
#include "Syntax.h"
#include "CompiledProgram.h"
#include "../CoreLib/Parser.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::IO;

		RefPtr<ILType> BaseTypeFromString(CoreLib::Text::Parser & parser)
		{
			if (parser.LookAhead(L"int"))
				return new ILBasicType(ILBaseType::Int);
			else if (parser.LookAhead(L"uint"))
				return new ILBasicType(ILBaseType::UInt);
			else if (parser.LookAhead(L"uvec2"))
				return new ILBasicType(ILBaseType::UInt2);
			else if (parser.LookAhead(L"uvec3"))
				return new ILBasicType(ILBaseType::UInt3);
			else if (parser.LookAhead(L"uvec4"))
				return new ILBasicType(ILBaseType::UInt4);
			if (parser.LookAhead(L"float"))
				return new ILBasicType(ILBaseType::Float);
			if (parser.LookAhead(L"vec2"))
				return new ILBasicType(ILBaseType::Float2);
			if (parser.LookAhead(L"vec3"))
				return new ILBasicType(ILBaseType::Float3);
			if (parser.LookAhead(L"vec4"))
				return new ILBasicType(ILBaseType::Float4);
			if (parser.LookAhead(L"ivec2"))
				return new ILBasicType(ILBaseType::Int2);
			if (parser.LookAhead(L"mat3"))
				return new ILBasicType(ILBaseType::Float3x3);
			if (parser.LookAhead(L"mat4"))
				return new ILBasicType(ILBaseType::Float4x4);
			if (parser.LookAhead(L"ivec3"))
				return new ILBasicType(ILBaseType::Int3);
			if (parser.LookAhead(L"ivec4"))
				return new ILBasicType(ILBaseType::Int4);
			if (parser.LookAhead(L"sampler2D"))
				return new ILBasicType(ILBaseType::Texture2D);
			if (parser.LookAhead(L"sampler2DShadow"))
				return new ILBasicType(ILBaseType::TextureShadow);
			if (parser.LookAhead(L"samplerCube"))
				return new ILBasicType(ILBaseType::TextureCube);
			if (parser.LookAhead(L"samplerCubeShadow"))
				return new ILBasicType(ILBaseType::TextureCubeShadow);
			if (parser.LookAhead(L"bool"))
				return new ILBasicType(ILBaseType::Bool);
			return nullptr;
		}

		RefPtr<ILType> TypeFromString(CoreLib::Text::Parser & parser)
		{
			auto result = BaseTypeFromString(parser);
			parser.ReadToken();
			while (parser.LookAhead(L"["))
			{
				parser.ReadToken();
				RefPtr<ILArrayType> newResult = new ILArrayType();
				newResult->BaseType = result;
				if (!parser.LookAhead(L"]"))
					newResult->ArrayLength = parser.ReadInt();
				result = newResult;
				parser.Read(L"]");
			}
			return result;
		}

		int RoundToAlignment(int offset, int alignment)
		{
			int remainder = offset % alignment;
			if (remainder == 0)
				return offset;
			else
				return offset + (alignment - remainder);
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
			else if (type == ILBaseType::TextureCubeShadow)
				return 8;
			else if (type == ILBaseType::TextureShadow)
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

		bool ILType::IsFloat()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Float;
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
				return basicType->Type == ILBaseType::Texture2D || basicType->Type == ILBaseType::TextureCube;
			else
				return false;
		}

		bool ILType::IsTexture()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Texture2D || basicType->Type == ILBaseType::TextureCube || basicType->Type == ILBaseType::TextureCubeShadow ||
				basicType->Type == ILBaseType::TextureShadow;
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
					if (c >= L'0' && c <= '9')
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
			for (auto & instr : GetAllInstructions())
			{
				if (instr.Name.Length() == 0)
					instr.Name = String(L"t") + String(NamingCounter++, 16);
			}
		}

		void CFGNode::DebugPrint()
		{
			printf("===========\n");
			for (auto& instr : *this)
			{
				printf("%s\n", instr.ToString().ToMultiByteString());
			}
			printf("===========\n");
		}

		LoadInstruction::LoadInstruction(ILOperand * dest)
		{
			Deterministic = false;
			Operand = dest;
			Type = dest->Type->Clone();
			if (!Spire::Compiler::Is<AllocVarInstruction>(dest) && !Spire::Compiler::Is<FetchArgInstruction>(dest))
				throw L"invalid address operand";
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
			rs << Name << L" = import [" << ComponentName << L"](";
			for (auto & arg : Arguments)
			{
				rs << arg->ToString() << L", ";
			}
			rs << L")";
			rs << L"\n{";
			rs << ImportOperator->ToString() << L"}\n";
			return rs.ProduceString();
		}
		String ImportInstruction::GetOperatorString()
		{
			return L"import";
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
		void Align(int & ptr, int alignment)
		{
			if (ptr % alignment != 0)
			{
				ptr = (ptr / alignment + 1) * alignment;
			}
		}
		int ILStructType::GetSize()
		{
			int rs = 0;
			for (auto & m : Members)
			{
				int size = m.Type->GetSize();
				int alignment = m.Type->GetAlignment();
				Align(rs, alignment);
				rs += size;
			}
			return rs;
		}
		int ILStructType::GetAlignment()
		{
			int rs = 1;
			for (auto & m : Members)
			{
				int alignment = m.Type->GetAlignment();
				rs = Math::Max(rs, alignment);
			}
			return rs;
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
		int ILRecordType::GetSize()
		{
			return 0;
		}
		int ILRecordType::GetAlignment()
		{
			return 0;
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
}
}