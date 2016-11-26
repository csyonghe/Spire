#ifndef IL_CODE_WRITER_H
#define IL_CODE_WRITER_H

#include "IL.h"
#include "ShaderCompiler.h"

namespace Spire
{
	namespace Compiler
	{
		class CodeWriter
		{
		private:
			List<RefPtr<CFGNode>> cfgNode;
			ConstantPool * constantPool = nullptr;
		public:
			void SetConstantPool(ConstantPool * pool)
			{
				constantPool = pool;
			}
			CFGNode * GetCurrentNode()
			{
				return cfgNode.Last().Ptr();
			}
			void PushNode()
			{
				RefPtr<CFGNode> n = new CFGNode();
				cfgNode.Add(n);
			}
			RefPtr<CFGNode> PopNode()
			{
				auto rs = cfgNode.Last();
				cfgNode.SetSize(cfgNode.Count() - 1);
				return rs;
			}
			void Assign(ILType * type, ILOperand * dest, ILOperand * src) // handles base type and ILArrayType assignment
			{
				auto arrType = dynamic_cast<ILArrayType*>(type);
				if (arrType)
				{
					for (int i = 0; i < arrType->ArrayLength; i++)
					{
						auto srcAddr = Add(src, i);
						auto destAddr = Add(dest, i);
						Store(destAddr, Load(srcAddr));
					}
				}
				else
					Store(dest, Load(src));
			}
			ILOperand * Select(ILOperand * cond, ILOperand * v0, ILOperand * v1)
			{
				auto rs = new SelectInstruction(cond, v0, v1);
				cfgNode.Last()->InsertTail(rs);
				return rs;
			}
			ILOperand * BitAnd(ILOperand * v0, ILOperand * v1)
			{
				auto instr = new BitAndInstruction(v0, v1);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			ILOperand * BitAnd(ILOperand * v0, int c)
			{
				auto instr = new BitAndInstruction(v0, constantPool->CreateConstant(c));
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			ILOperand * Add(ILOperand * v0, ILOperand * v1)
			{
				auto instr = new AddInstruction(v0, v1);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			ILOperand * Add(ILOperand * v0, int v1)
			{
				auto instr = new AddInstruction(v0, constantPool->CreateConstant(v1));
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			ILOperand * Mul(ILOperand * v0, ILOperand * v1)
			{
				auto instr = new MulInstruction(v0, v1);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			ILOperand * Copy(ILOperand * src)
			{
				auto rs = new CopyInstruction(src);
				cfgNode.Last()->InsertTail(rs);
				return rs;
			}
			ILOperand * Load(ILOperand * src, int offset)
			{
				if (offset == 0)
				{
					auto instr = new LoadInstruction(src);
					cfgNode.Last()->InsertTail(instr);
					return instr;
				}
				else
				{
					auto dest = new AddInstruction(src, constantPool->CreateConstant(offset));
					cfgNode.Last()->InsertTail(dest);
					auto instr = new LoadInstruction(dest);
					cfgNode.Last()->InsertTail(instr);
					return instr;
				}
			}
			ILOperand * Load(ILOperand * src)
			{
				auto instr = new LoadInstruction(src);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			ILOperand * Load(ILOperand * src, ILOperand * offset)
			{
				auto dest = new AddInstruction(src, offset);
				cfgNode.Last()->InsertTail(dest);
				return Load(dest);
			}
			StoreInstruction * Store(ILOperand * dest, ILOperand * value)
			{
				auto instr = new StoreInstruction(dest, value);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			DiscardInstruction * Discard()
			{
				auto instr = new DiscardInstruction();
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			MemberUpdateInstruction * Update(ILOperand * dest, ILOperand * offset, ILOperand * value)
			{
				auto instr = new MemberUpdateInstruction(dest, offset, value);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			MemberLoadInstruction * Retrieve(ILOperand * dest, ILOperand * offset)
			{
				auto instr = new MemberLoadInstruction(dest, offset);
				cfgNode.Last()->InsertTail(instr);
				return instr;
			}
			//AllocVarInstruction * AllocVar(ILType * type, ILOperand * size)
			//{
			//	auto arrType = dynamic_cast<ILArrayType*>(type);
			//	if (arrType)
			//	{
			//		// check: size must be constant 1. Do not support array of array in IL level.
			//		auto s = dynamic_cast<ILConstOperand*>(size);
			//		if (!s || s->IntValues[0] != 1)
			//			throw ArgumentException("AllocVar(arrayType, size): size must be constant 1.");
			//		auto instr = new AllocVarInstruction(arrType->BaseType, program.CreateConstant(arrType->ArrayLength));
			//		cfgNode->InsertTail(instr);
			//		return instr;
			//	}
			//	else
			//	{
			//		auto instr = new AllocVarInstruction(type, size);
			//		cfgNode->InsertTail(instr);
			//		return instr;
			//	}
			//}
			AllocVarInstruction * AllocVar(RefPtr<ILType> & type, ILOperand * size)
			{
				auto arrType = dynamic_cast<ILArrayType*>(type.Ptr());
				if (arrType)
				{
					// check: size must be constant 1. Do not support array of array in IL level.
					auto s = dynamic_cast<ILConstOperand*>(size);
					if (!s || s->IntValues[0] != 1)
						throw ArgumentException("AllocVar(arrayType, size): size must be constant 1.");
					auto instr = new AllocVarInstruction(arrType->BaseType, constantPool->CreateConstant(arrType->ArrayLength));
					cfgNode.Last()->InsertTail(instr);
					return instr;
				}
				else
				{
					auto instr = new AllocVarInstruction(type, size);
					cfgNode.Last()->InsertTail(instr);
					return instr;
				}
			}
			/*GLeaInstruction * GLea(ILType * type, const String & name)
			{
				auto arrType = dynamic_cast<ILArrayType*>(type);
				auto instr = new GLeaInstruction();
				if (arrType)
					instr->Type = new ILPointerType(arrType->BaseType);
				else
					instr->Type = new ILPointerType(type);
				instr->Name = name;
				instr->VariableName = name;
				cfgNode->InsertTail(instr);
				return instr;
			}*/
			FetchArgInstruction * FetchArg(RefPtr<ILType> type, int argId)
			{
				auto instr = new FetchArgInstruction(type);
				cfgNode.Last()->InsertTail(instr);
				instr->ArgId = argId;
				return instr;
			}
			
			void Insert(ILInstruction * instr)
			{
				cfgNode.Last()->InsertTail(instr);
			}
		};
	}
}

#endif