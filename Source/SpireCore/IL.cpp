#include "IL.h"
#include "../CoreLib/LibIO.h"
#include "Syntax.h"
#include "CompiledProgram.h"

namespace Spire
{
	namespace Compiler
	{
		using namespace CoreLib::IO;

		ILBaseType ILBaseTypeFromString(String str)
		{
			if (str == L"int")
				return ILBaseType::Int;
			else if (str == L"uint")
				return ILBaseType::UInt;
			if (str == L"float")
				return ILBaseType::Float;
			if (str == L"vec2")
				return ILBaseType::Float2;
			if (str == L"vec3")
				return ILBaseType::Float3;
			if (str == L"vec4")
				return ILBaseType::Float4;
			if (str == L"ivec2")
				return ILBaseType::Int2;
			if (str == L"mat3")
				return ILBaseType::Float3x3;
			if (str == L"mat4")
				return ILBaseType::Float4x4;
			if (str == L"ivec3")
				return ILBaseType::Int3;
			if (str == L"ivec4")
				return ILBaseType::Int4;
			if (str == L"sampler2D")
				return ILBaseType::Texture2D;
			if (str == L"sampler2DShadow")
				return ILBaseType::TextureShadow;
			if (str == L"samplerCube")
				return ILBaseType::TextureCube;
			if (str == L"samplerCubeShadow")
				return ILBaseType::TextureCubeShadow;
			return ILBaseType::Int;
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
			else
				return 0;
		}

		int AlignmentOfBaseType(ILBaseType type)
		{
			if (type == ILBaseType::Int)
				return 4;
			else if (type == ILBaseType::UInt)
				return 4;
			else if (type == ILBaseType::Int2)
				return 8;
			else if (type == ILBaseType::Int3)
				return 16;
			else if (type == ILBaseType::Int4)
				return 16;
			else if (type == ILBaseType::Float)
				return 4;
			else if (type == ILBaseType::Float2)
				return 8;
			else if (type == ILBaseType::Float3)
				return 16;
			else if (type == ILBaseType::Float4)
				return 16;
			else if (type == ILBaseType::Float3x3)
				return 16;
			else if (type == ILBaseType::Float4x4)
				return 16;
			else if (type == ILBaseType::Texture2D)
				return 8;
			else if (type == ILBaseType::TextureCube)
				return 8;
			else if (type == ILBaseType::TextureCubeShadow)
				return 8;
			else if (type == ILBaseType::TextureShadow)
				return 8;
			else
				return 0;
		}

		String ILBaseTypeToString(ILBaseType type)
		{
			if (type == ILBaseType::Int)
				return L"int";
			else if (type == ILBaseType::UInt)
				return L"uint";
			else if (type == ILBaseType::Int2)
				return L"ivec2";
			else if (type == ILBaseType::Int3)
				return L"ivec3";
			else if (type == ILBaseType::Int4)
				return L"ivec4";
			else if (type == ILBaseType::Float)
				return L"float";
			else if (type == ILBaseType::Float2)
				return L"vec2";
			else if (type == ILBaseType::Float3)
				return L"vec3";
			else if (type == ILBaseType::Float4)
				return L"vec4";
			else if (type == ILBaseType::Float3x3)
				return L"mat3";
			else if (type == ILBaseType::Float4x4)
				return L"mat4";
			else if (type == ILBaseType::Texture2D)
				return L"sampler2D";
			else if (type == ILBaseType::TextureCube)
				return L"samplerCube";
			else if (type == ILBaseType::TextureCubeShadow)
				return L"samplerCubeShadow";
			else if (type == ILBaseType::TextureShadow)
				return L"sampler2DShadow";
			else
				return L"?unkown";
		}

		bool ILType::IsInt()
		{
			auto basicType = dynamic_cast<ILBasicType*>(this);
			if (basicType)
				return basicType->Type == ILBaseType::Int;
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
					return 2;
				case ILBaseType::Int3:
				case ILBaseType::Float3:
					return 3;
				case ILBaseType::Int4:
				case ILBaseType::Float4:
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
			if (!Spire::Compiler::Is<AllocVarInstruction>(dest) && !Spire::Compiler::Is<GLeaInstruction>(dest) && !Spire::Compiler::Is<FetchArgInstruction>(dest))
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
		void GLeaInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitGLeaInstruction(this);
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
			bool done = false;
			do
			{
				done = true;
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
					done = false;
				}
				if (curInstr->GetNext() == nullptr)
					break;
			} while (!done);

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
			rs << Name << L" = import<" << ImportOperator->Name.Content << ">[" << ComponentName << L"@" << SourceWorld->WorldName << L"](";
			for (auto & arg : Arguments)
			{
				rs << arg->ToString() << L", ";
			}
			rs << L")";
			return rs.ProduceString();
		}
		String ImportInstruction::GetOperatorString()
		{
			return L"import<" + ImportOperator->Name.Content + L">";
		}
		void ImportInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitImportInstruction(this);
		}
		void ExportInstruction::Accept(InstructionVisitor * visitor)
		{
			visitor->VisitExportInstruction(this);
		}
}
}