#include "CLikeCodeGen.h"
#include "../CoreLib/Tokenizer.h"
#include "Syntax.h"
#include "Naming.h"

using namespace CoreLib::Basic;

namespace Spire
{
	namespace Compiler
	{
		ILRecordType * ExtractRecordType(ILType * type)
		{
			if (auto recType = dynamic_cast<ILRecordType*>(type))
				return recType;
			else if (auto arrType = dynamic_cast<ILArrayType*>(type))
				return ExtractRecordType(arrType->BaseType.Ptr());
			else if (auto genType = dynamic_cast<ILGenericType*>(type))
				return ExtractRecordType(genType->BaseType.Ptr());
			else
				return nullptr;
		}

		String AddWorldNameSuffix(String name, String suffix)
		{
			if (name.EndsWith(suffix))
				return name;
			else
				return EscapeCodeName(name + "_" + suffix);
		}

		void CLikeCodeGen::PrintType(StringBuilder & sbCode, ILType* type)
		{
			PrintTypeName(sbCode, type);
		}

		void CLikeCodeGen::PrintDef(StringBuilder & sbCode, ILType* type, const String & name)
		{
			PrintType(sbCode, type);
			sbCode << " ";
			sbCode << name;
			if (name.Length() == 0)
				throw InvalidProgramException("unnamed instruction.");
		}
		
		String CLikeCodeGen::GetFuncOriginalName(const String & name)
		{
			String originalName;
			int splitPos = name.IndexOf('@');
			if (splitPos == 0)
				return name;
			if (splitPos != -1)
				originalName = name.SubString(0, splitPos);
			else
				originalName = name;
			return originalName;
		}

		void CLikeCodeGen::PrintOp(CodeGenContext & ctx, ILOperand * op, bool forceExpression)
		{
			auto makeFloat = [](float v)
			{
				String rs(v, "%.12e");
				if (!rs.Contains('.') && !rs.Contains('e') && !rs.Contains('E'))
					rs = rs + ".0";
				if (rs.StartsWith("-"))
					rs = "(" + rs + ")";
				return rs;
			};
			
			if (auto c = dynamic_cast<ILConstOperand*>(op))
			{
				auto type = c->Type.Ptr();
				if (type->IsFloat())
					ctx.Body << makeFloat(c->FloatValues[0]);
				else if (type->IsInt())
					ctx.Body << (c->IntValues[0]);
				else if (type->IsBool())
					ctx.Body << ((c->IntValues[0] != 0) ? "true" : "false");
				else if (auto baseType = dynamic_cast<ILBasicType*>(type))
				{
					PrintType(ctx.Body, baseType);
					ctx.Body << "(";

					if (baseType->Type == ILBaseType::Float2)
						ctx.Body << makeFloat(c->FloatValues[0]) << ", " << makeFloat(c->FloatValues[1]);
					else if (baseType->Type == ILBaseType::Float3)
						ctx.Body << makeFloat(c->FloatValues[0]) << ", " << makeFloat(c->FloatValues[1]) << ", " << makeFloat(c->FloatValues[2]);
					else if (baseType->Type == ILBaseType::Float4)
						ctx.Body << makeFloat(c->FloatValues[0]) << ", " << makeFloat(c->FloatValues[1]) << ", " << makeFloat(c->FloatValues[2]) << ", " << makeFloat(c->FloatValues[3]);
					else if (baseType->Type == ILBaseType::Float3x3)
					{
						ctx.Body << "mat3(";
						for (int i = 0; i < 9; i++)
						{
							ctx.Body << makeFloat(c->FloatValues[i]);
							if (i != 8)
								ctx.Body << ", ";
						}
						ctx.Body;
					}
					else if (baseType->Type == ILBaseType::Float4x4)
					{
						for (int i = 0; i < 16; i++)
						{
							ctx.Body << makeFloat(c->FloatValues[i]);
							if (i != 15)
								ctx.Body << ", ";
						}
					}
					else if (baseType->Type == ILBaseType::Int2)
						ctx.Body << c->IntValues[0] << ", " << c->IntValues[1];
					else if (baseType->Type == ILBaseType::Int3)
						ctx.Body << c->IntValues[0] << ", " << c->IntValues[1] << ", " << c->IntValues[2];
					else if (baseType->Type == ILBaseType::Int4)
						ctx.Body << c->IntValues[0] << ", " << c->IntValues[1] << ", " << c->IntValues[2] << ", " << c->IntValues[3];

					ctx.Body << ")";
				}
				else
					throw InvalidOperationException("Illegal constant.");
			}
			else if (auto instr = dynamic_cast<ILInstruction*>(op))
			{
				if (AppearAsExpression(*instr, forceExpression))
				{
					PrintInstrExpr(ctx, *instr);
				}
				else
				{
					if (forceExpression)
						throw InvalidProgramException("cannot generate code block as an expression.");
					String substituteName;
					if (ctx.SubstituteNames.TryGetValue(instr->Name, substituteName))
						ctx.Body << substituteName;
					else
						ctx.Body << instr->Name;
				}
			}
			else if (auto param = dynamic_cast<ILModuleParameterInstance*>(op))
			{
				PrintParameterReference(ctx.Body, param);
			}
			else
				throw InvalidOperationException("Unsupported operand type.");
		}

		static bool IsMatrix(ILOperand* operand)
		{
			auto type = operand->Type;
			// TODO(tfoley): This needs to be expanded once other matrix types are supported
			return type->IsFloatMatrix();
		}

		void CLikeCodeGen::PrintMatrixMulInstrExpr(CodeGenContext & ctx, ILOperand* op0, ILOperand* op1)
		{
			ctx.Body << "(";
			PrintOp(ctx, op0);
			ctx.Body << " * ";
			PrintOp(ctx, op1);
			ctx.Body << ")";
		}

		void CLikeCodeGen::PrintBinaryInstrExpr(CodeGenContext & ctx, BinaryInstruction * instr)
		{
			if (instr->Is<StoreInstruction>())
			{
				auto op0 = instr->Operands[0].Ptr();
				auto op1 = instr->Operands[1].Ptr();
				ctx.Body << "(";
				PrintOp(ctx, op0);
				ctx.Body << " = ";
				PrintOp(ctx, op1);
				ctx.Body << ")";
				return;
			}
			auto op0 = instr->Operands[0].Ptr();
			auto op1 = instr->Operands[1].Ptr();
			if (instr->Is<StoreInstruction>())
			{
				throw InvalidOperationException("store instruction cannot appear as expression.");
			}
			if (instr->Is<MemberLoadInstruction>())
			{
				PrintOp(ctx, op0);
				bool printDefault = true;
				if (op0->Type->IsVector())
				{
					if (auto c = dynamic_cast<ILConstOperand*>(op1))
					{
						switch (c->IntValues[0])
						{
						case 0:
							ctx.Body << ".x";
							break;
						case 1:
							ctx.Body << ".y";
							break;
						case 2:
							ctx.Body << ".z";
							break;
						case 3:
							ctx.Body << ".w";
							break;
						default:
							throw InvalidOperationException("Invalid member access.");
						}
						printDefault = false;
					}
				}
				else if (auto structType = dynamic_cast<ILStructType*>(op0->Type.Ptr()))
				{
					if (auto c = dynamic_cast<ILConstOperand*>(op1))
					{
						ctx.Body << "." << structType->Members[c->IntValues[0]].FieldName;
					}
					printDefault = false;
				}
				if (printDefault)
				{
					ctx.Body << "[";
					PrintOp(ctx, op1);
					ctx.Body << "]";
				}
				
				
				return;
			}
			const char * op = "";
			if (instr->Is<AddInstruction>())
			{
				op = "+";
			}
			else if (instr->Is<SubInstruction>())
			{
				op = "-";
			}
			else if (instr->Is<MulInstruction>())
			{
				// For matrix-matrix, matrix-vector, and vector-matrix `*`,
				// GLSL performs a linear-algebraic inner product, while HLSL
				// always does element-wise product. We need to give the
				// codegen backend a change to handle this case.
				if(IsMatrix(op0) || IsMatrix(op1))
				{
					PrintMatrixMulInstrExpr(ctx, op0, op1);
					return;
				}

				op = "*";
			}
			else if (instr->Is<DivInstruction>())
			{
				op = "/";
			}
			else if (instr->Is<ModInstruction>())
			{
				op = "%";
			}
			else if (instr->Is<ShlInstruction>())
			{
				op = "<<";
			}
			else if (instr->Is<ShrInstruction>())
			{
				op = ">>";
			}
			else if (instr->Is<CmpeqlInstruction>())
			{
				op = "==";
				//ctx.Body << "int";
			}
			else if (instr->Is<CmpgeInstruction>())
			{
				op = ">=";
				//ctx.Body << "int";
			}
			else if (instr->Is<CmpgtInstruction>())
			{
				op = ">";
				//ctx.Body << "int";
			}
			else if (instr->Is<CmpleInstruction>())
			{
				op = "<=";
				//ctx.Body << "int";
			}
			else if (instr->Is<CmpltInstruction>())
			{
				op = "<";
				//ctx.Body << "int";
			}
			else if (instr->Is<CmpneqInstruction>())
			{
				op = "!=";
				//ctx.Body << "int";
			}
			else if (instr->Is<AndInstruction>())
			{
				op = "&&";
			}
			else if (instr->Is<OrInstruction>())
			{
				op = "||";
			}
			else if (instr->Is<BitXorInstruction>())
			{
				op = "^";
			}
			else if (instr->Is<BitAndInstruction>())
			{
				op = "&";
			}
			else if (instr->Is<BitOrInstruction>())
			{
				op = "|";
			}
			else
				throw InvalidProgramException("unsupported binary instruction.");
			ctx.Body << "(";
			PrintOp(ctx, op0);
			ctx.Body << " " << op << " ";
			PrintOp(ctx, op1);
			ctx.Body << ")";
		}

		void CLikeCodeGen::PrintBinaryInstr(CodeGenContext & ctx, BinaryInstruction * instr)
		{
			auto op0 = instr->Operands[0].Ptr();
			auto op1 = instr->Operands[1].Ptr();
			if (instr->Is<StoreInstruction>())
			{
				PrintOp(ctx, op0);
				ctx.Body << " = ";
				PrintOp(ctx, op1);
				ctx.Body << ";\n";
				return;
			}
			auto varName = ctx.DefineVariable(instr);
			if (instr->Is<MemberLoadInstruction>())
			{
				ctx.Body << varName << " = ";
				PrintBinaryInstrExpr(ctx, instr);
				ctx.Body << ";\n";
				return;
			}
			ctx.Body << varName << " = ";
			PrintBinaryInstrExpr(ctx, instr);
			ctx.Body << ";\n";
		}

		void CLikeCodeGen::PrintUnaryInstrExpr(CodeGenContext & ctx, UnaryInstruction * instr)
		{
			auto op0 = instr->Operand.Ptr();
			if (instr->Is<LoadInstruction>())
			{
				PrintOp(ctx, op0);
				return;
			}
			else if (instr->Is<SwizzleInstruction>())
			{
				PrintSwizzleInstrExpr(ctx, instr->As<SwizzleInstruction>());
				return;
			}
			const char * op = "";
			if (instr->Is<BitNotInstruction>())
				op = "~";
			else if (instr->Is<Float2IntInstruction>())
				op = "(int)";
			else if (instr->Is<Int2FloatInstruction>())
				op = "(float)";
			else if (instr->Is<CopyInstruction>())
				op = "";
			else if (instr->Is<NegInstruction>())
				op = "-";
			else if (instr->Is<NotInstruction>())
				op = "!";
			else
				throw InvalidProgramException("unsupported unary instruction.");
			ctx.Body << "(" << op;
			PrintOp(ctx, op0);
			ctx.Body << ")";
		}

		void CLikeCodeGen::PrintUnaryInstr(CodeGenContext & ctx, UnaryInstruction * instr)
		{
			auto varName = ctx.DefineVariable(instr);
			ctx.Body << varName << " = ";
			PrintUnaryInstrExpr(ctx, instr);
			ctx.Body << ";\n";
		}

		void CLikeCodeGen::PrintAllocVarInstrExpr(CodeGenContext & ctx, AllocVarInstruction * instr)
		{
			ctx.Body << instr->Name;
		}

		void CLikeCodeGen::PrintAllocVarInstr(CodeGenContext & ctx, AllocVarInstruction * instr)
		{
			if (dynamic_cast<ILConstOperand*>(instr->Size.Ptr()))
			{
				ctx.DefineVariable(instr);
			}
			else
				throw InvalidProgramException("size operand of allocVar instr is not an intermediate.");
		}

		void CLikeCodeGen::PrintFetchArgInstrExpr(CodeGenContext & ctx, FetchArgInstruction * instr)
		{
			ctx.Body << instr->Name;
		}

		void CLikeCodeGen::PrintFetchArgInstr(CodeGenContext & ctx, FetchArgInstruction * instr)
		{
			if (instr->ArgId == 0)
			{
				ctx.ReturnVarName = ctx.DefineVariable(instr);
			}
		}

		void CLikeCodeGen::PrintSelectInstrExpr(CodeGenContext & ctx, SelectInstruction * instr)
		{
			ctx.Body << "(";
			PrintOp(ctx, instr->Operands[0].Ptr());
			ctx.Body << "?";
			PrintOp(ctx, instr->Operands[1].Ptr());
			ctx.Body << ":";
			PrintOp(ctx, instr->Operands[2].Ptr());
			ctx.Body << ")";
		}

		void CLikeCodeGen::PrintSelectInstr(CodeGenContext & ctx, SelectInstruction * instr)
		{
			auto varName = ctx.DefineVariable(instr);
			ctx.Body << varName << " = ";
			PrintSelectInstrExpr(ctx, instr);
			ctx.Body << ";\n";
		}

		void CLikeCodeGen::PrintCallInstrExprForTarget(CodeGenContext & ctx, CallInstruction * instr, String const& name)
		{
			PrintDefaultCallInstrExpr(ctx, instr, name);
		}

		void CLikeCodeGen::PrintDefaultCallInstrArgs(CodeGenContext & ctx, CallInstruction * instr)
		{
			ctx.Body << "(";
			int id = 0;
			for (auto & arg : instr->Arguments)
			{
				PrintOp(ctx, arg.Ptr());
				if (id != instr->Arguments.Count() - 1)
					ctx.Body << ", ";
				id++;
			}
			ctx.Body << ")";
		}


		void CLikeCodeGen::PrintDefaultCallInstrExpr(CodeGenContext & ctx, CallInstruction * instr, String const& callName)
		{
			ctx.Body << callName;
			PrintDefaultCallInstrArgs(ctx, instr);
		}

		void CLikeCodeGen::PrintCallInstrExpr(CodeGenContext & ctx, CallInstruction * instr)
		{
			if (instr->Arguments.Count() > 0 && instr->Arguments.First()->Type->IsTexture() && intrinsicTextureFunctions.Contains(instr->Function))
			{
				PrintTextureCall(ctx, instr);
				return;
			}
			String callName;
			callName = GetFuncOriginalName(instr->Function);
			PrintCallInstrExprForTarget(ctx, instr, callName);
		}

		void CLikeCodeGen::PrintCallInstr(CodeGenContext & ctx, CallInstruction * instr)
		{
			if (!instr->Type->IsVoid())
			{
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName;
				ctx.Body << " = ";
			}
			PrintCallInstrExpr(ctx, instr);
			ctx.Body << ";\n";
		}

		void CLikeCodeGen::PrintCastF2IInstrExpr(CodeGenContext & ctx, Float2IntInstruction * instr)
		{
			ctx.Body << "((int)(";
			PrintOp(ctx, instr->Operand.Ptr());
			ctx.Body << "))";
		}
		void CLikeCodeGen::PrintCastF2IInstr(CodeGenContext & ctx, Float2IntInstruction * instr)
		{
			auto varName = ctx.DefineVariable(instr);
			ctx.Body << varName;
			ctx.Body << " = ";
			PrintCastF2IInstrExpr(ctx, instr);
			ctx.Body << ";\n";
		}
		void CLikeCodeGen::PrintCastI2FInstrExpr(CodeGenContext & ctx, Int2FloatInstruction * instr)
		{
			ctx.Body << "((float)(";
			PrintOp(ctx, instr->Operand.Ptr());
			ctx.Body << "))";
		}
		void CLikeCodeGen::PrintCastI2FInstr(CodeGenContext & ctx, Int2FloatInstruction * instr)
		{
			auto varName = ctx.DefineVariable(instr);
			ctx.Body << varName;
			ctx.Body << " = ";
			PrintCastI2FInstrExpr(ctx, instr);
			ctx.Body << ";\n";
		}

		bool CLikeCodeGen::AppearAsExpression(ILInstruction & instr, bool force)
		{
			if (instr.Is<LoadInputInstruction>() || instr.Is<ProjectInstruction>())
				return true;
			if (auto arg = instr.As<FetchArgInstruction>())
			{
				if (arg->ArgId == 0)
					return false;
			}
			if (auto import = instr.As<ImportInstruction>())
			{
				if ((!useBindlessTexture && import->Type->IsTexture()) 
					|| import->Type.As<ILArrayType>() 
					|| import->Type->IsSamplerState()
					|| import->Type.As<ILGenericType>())
					return true;
			}
			for (auto &&usr : instr.Users)
			{
				if (auto update = dynamic_cast<MemberUpdateInstruction*>(usr))
				{
					if (&instr == update->Operands[0].Ptr())
						return false;
				}
				else if (dynamic_cast<MemberLoadInstruction*>(usr))
					return false;
				else if (dynamic_cast<ExportInstruction*>(usr))
					return false;
				else if (dynamic_cast<ImportInstruction*>(usr))
					return false;
			}
			if (instr.Is<StoreInstruction>() && force)
				return true;

			return (instr.Users.Count() <= 1 && !instr.HasSideEffect() && !instr.Is<MemberUpdateInstruction>()
				&& !instr.Is<AllocVarInstruction>() && !instr.Is<ImportInstruction>())
				|| instr.Is<FetchArgInstruction>();
		}

		void CLikeCodeGen::PrintExportInstr(CodeGenContext &ctx, ExportInstruction * exportInstr)
		{
			outputStrategy->ProcessExportInstruction(ctx, exportInstr);
		}

		void CLikeCodeGen::PrintUpdateInstr(CodeGenContext & ctx, MemberUpdateInstruction * instr)
		{
			auto genCode = [&](String varName, ILType * srcType, ILOperand * op1, ILOperand * op2)
			{
				ctx.Body << varName;
				if (auto structType = dynamic_cast<ILStructType*>(srcType))
				{
					ctx.Body << ".";
					ctx.Body << structType->Members[dynamic_cast<ILConstOperand*>(op1)->IntValues[0]].FieldName;
				}
				else
				{
					ctx.Body << "[";
					PrintOp(ctx, op1);
					ctx.Body << "]";
				}
				ctx.Body << " = ";
				PrintOp(ctx, op2);
				ctx.Body << ";\n";
			};
			if (auto srcInstr = dynamic_cast<ILInstruction*>(instr->Operands[0].Ptr()))
			{
				if (srcInstr->Users.Count() == 1)
				{
					auto srcName = srcInstr->Name;
					while (ctx.SubstituteNames.TryGetValue(srcName, srcName));
					genCode(srcName, srcInstr->Type.Ptr(), instr->Operands[1].Ptr(), instr->Operands[2].Ptr());
					ctx.SubstituteNames[instr->Name] = srcName;
					return;
				}
			}
			genCode(instr->Operands[0]->Name, instr->Operands[0]->Type.Ptr(), instr->Operands[1].Ptr(), instr->Operands[2].Ptr());
		}

		void CLikeCodeGen::PrintSwizzleInstrExpr(CodeGenContext & ctx, SwizzleInstruction * swizzle)
		{
			PrintOp(ctx, swizzle->Operand.Ptr());
			ctx.Body << "." << swizzle->SwizzleString;
		}

		void CLikeCodeGen::PrintImportInstr(CodeGenContext & ctx, ImportInstruction * importInstr)
		{
			currentImportInstr = importInstr;
				
			ctx.DefineVariable(importInstr);
			GenerateCode(ctx, importInstr->ImportOperator.Ptr());
				
			currentImportInstr = nullptr;
		}

		void CLikeCodeGen::PrintImportInstrExpr(CodeGenContext & ctx, ImportInstruction * importInstr)
		{
			currentImportInstr = importInstr;
			PrintOp(ctx, importInstr->ImportOperator->GetLastInstruction()->As<ReturnInstruction>()->Operand.Ptr());
			currentImportInstr = nullptr;
		}

		void CLikeCodeGen::PrintInstrExpr(CodeGenContext & ctx, ILInstruction & instr)
		{
			if (auto projInstr = instr.As<ProjectInstruction>())
				PrintProjectInstrExpr(ctx, projInstr);
			else if (auto binInstr = instr.As<BinaryInstruction>())
				PrintBinaryInstrExpr(ctx, binInstr);
			else if (auto unaryInstr = instr.As<UnaryInstruction>())
				PrintUnaryInstrExpr(ctx, unaryInstr);
			else if (auto allocVar = instr.As<AllocVarInstruction>())
				PrintAllocVarInstrExpr(ctx, allocVar);
			else if (auto fetchArg = instr.As<FetchArgInstruction>())
				PrintFetchArgInstrExpr(ctx, fetchArg);
			else if (auto select = instr.As<SelectInstruction>())
				PrintSelectInstrExpr(ctx, select);
			else if (auto call = instr.As<CallInstruction>())
				PrintCallInstrExpr(ctx, call);
			else if (auto castf2i = instr.As<Float2IntInstruction>())
				PrintCastF2IInstrExpr(ctx, castf2i);
			else if (auto casti2f = instr.As<Int2FloatInstruction>())
				PrintCastI2FInstrExpr(ctx, casti2f);
			else if (auto ldInput = instr.As<LoadInputInstruction>())
				PrintLoadInputInstrExpr(ctx, ldInput);
			else if (auto import = instr.As<ImportInstruction>())
				PrintImportInstrExpr(ctx, import);
			else if (instr.As<MemberUpdateInstruction>())
				throw InvalidOperationException("member update instruction cannot appear as expression.");
		}

		void CLikeCodeGen::PrintInstr(CodeGenContext & ctx, ILInstruction & instr)
		{
			// ctx.Body << "// " << instr.ToString() << ";\n";
			if (!AppearAsExpression(instr, false))
			{
				if (auto binInstr = instr.As<BinaryInstruction>())
					PrintBinaryInstr(ctx, binInstr);
				else if (auto exportInstr = instr.As<ExportInstruction>())
					PrintExportInstr(ctx, exportInstr);
				else if (auto unaryInstr = instr.As<UnaryInstruction>())
					PrintUnaryInstr(ctx, unaryInstr);
				else if (auto allocVar = instr.As<AllocVarInstruction>())
					PrintAllocVarInstr(ctx, allocVar);
				else if (auto fetchArg = instr.As<FetchArgInstruction>())
					PrintFetchArgInstr(ctx, fetchArg);
				else if (auto select = instr.As<SelectInstruction>())
					PrintSelectInstr(ctx, select);
				else if (auto call = instr.As<CallInstruction>())
					PrintCallInstr(ctx, call);
				else if (auto castf2i = instr.As<Float2IntInstruction>())
					PrintCastF2IInstr(ctx, castf2i);
				else if (auto casti2f = instr.As<Int2FloatInstruction>())
					PrintCastI2FInstr(ctx, casti2f);
				else if (auto update = instr.As<MemberUpdateInstruction>())
					PrintUpdateInstr(ctx, update);
				else if (auto importInstr = instr.As<ImportInstruction>())
					PrintImportInstr(ctx, importInstr);
					
			}
		}

		void CLikeCodeGen::PrintLoadInputInstrExpr(CodeGenContext & ctx, LoadInputInstruction * instr)
		{
			PrintInputReference(ctx, ctx.Body, instr->InputName);
		}

		void CLikeCodeGen::GenerateCode(CodeGenContext & context, CFGNode * code)
		{
			for (auto & instr : *code)
			{
				if (auto ifInstr = instr.As<IfInstruction>())
				{
					context.Body << "if (bool(";
					PrintOp(context, ifInstr->Operand.Ptr());
					context.Body << "))\n{\n";
					GenerateCode(context, ifInstr->TrueCode.Ptr());
					context.Body << "}\n";
					if (ifInstr->FalseCode)
					{
						context.Body << "else\n{\n";
						GenerateCode(context, ifInstr->FalseCode.Ptr());
						context.Body << "}\n";
					}
				}
				else if (auto forInstr = instr.As<ForInstruction>())
				{
					context.Body << "for (";
					if (forInstr->InitialCode)
						PrintOp(context, forInstr->InitialCode->GetLastInstruction(), true);
					context.Body << "; ";
					if (forInstr->ConditionCode)
						PrintOp(context, forInstr->ConditionCode->GetLastInstruction(), true);
					context.Body << "; ";
					if (forInstr->SideEffectCode)
						PrintOp(context, forInstr->SideEffectCode->GetLastInstruction(), true);
					context.Body << ")\n{\n";
					GenerateCode(context, forInstr->BodyCode.Ptr());
					context.Body << "}\n";
				}
				else if (auto doInstr = instr.As<DoInstruction>())
				{
					context.Body << "do\n{\n";
					GenerateCode(context, doInstr->BodyCode.Ptr());
					context.Body << "} while (bool(";
					PrintOp(context, doInstr->ConditionCode->GetLastInstruction()->As<ReturnInstruction>()->Operand.Ptr());
					context.Body << "));\n";
				}
				else if (auto whileInstr = instr.As<WhileInstruction>())
				{
					context.Body << "while (bool(";
					PrintOp(context, whileInstr->ConditionCode->GetLastInstruction()->As<ReturnInstruction>()->Operand.Ptr());
					context.Body << "))\n{\n";
					GenerateCode(context, whileInstr->BodyCode.Ptr());
					context.Body << "}\n";
				}
				else if (auto ret = instr.As<ReturnInstruction>())
				{
					if (currentImportInstr) 
					{
						context.Body << currentImportInstr->Name << " = ";
						PrintOp(context, ret->Operand.Ptr());
						context.Body << ";\n";
					}
					else
					{
						context.Body << "return ";
						PrintOp(context, ret->Operand.Ptr());
						context.Body << ";\n";
					}
				}
				else if (instr.Is<BreakInstruction>())
				{
					context.Body << "break;\n";
				}
				else if (instr.Is<ContinueInstruction>())
				{
					context.Body << "continue;\n";
				}
				else if (instr.Is<DiscardInstruction>())
				{
					context.Body << "discard;\n";
				}
				else
					PrintInstr(context, instr);
			}
		}

		CLikeCodeGen::CLikeCodeGen()
		{
			intrinsicTextureFunctions.Add("Sample");
			intrinsicTextureFunctions.Add("SampleBias");
			intrinsicTextureFunctions.Add("SampleGrad");
			intrinsicTextureFunctions.Add("SampleCmp");
		}

		void CLikeCodeGen::GenerateShaderMetaData(ShaderMetaData & result, ILProgram* /*program*/, ILShader * shader, DiagnosticSink * /*err*/)
		{
			result.ShaderName = shader->Name;
			result.ParameterSets = shader->ModuleParamSets;
		}

		CompiledShaderSource CLikeCodeGen::GenerateShader(CompileResult & result, SymbolTable *, ILShader * shader, DiagnosticSink * err)
		{
			this->errWriter = err;

			CompiledShaderSource rs;

			for (auto & stage : shader->Stages)
			{
				StageSource src;
				if (stage.Value->StageType == "VertexShader" || stage.Value->StageType == "FragmentShader" || stage.Value->StageType == "DomainShader")
					src = GenerateVertexFragmentDomainShader(result.Program.Ptr(), shader, stage.Value.Ptr());
				else if (stage.Value->StageType == "ComputeShader")
					src = GenerateComputeShader(result.Program.Ptr(), shader, stage.Value.Ptr());
				else if (stage.Value->StageType == "HullShader")
					src = GenerateHullShader(result.Program.Ptr(), shader, stage.Value.Ptr());
				else
                    errWriter->diagnose(stage.Value->Position, Diagnostics::unknownStageType, stage.Value->StageType);
				rs.Stages[stage.Key] = src;
			}
				
			GenerateShaderMetaData(rs.MetaData, result.Program.Ptr(), shader, err);
				
			return rs;
		}

		void CLikeCodeGen::GenerateStructs(StringBuilder & sb, ILProgram * program)
		{
			for (auto & st : program->Structs)
			{
				if (!st->IsIntrinsic)
				{
					sb << "struct " << st->TypeName << "\n{\n";
					for (auto & f : st->Members)
					{
						sb << f.Type->ToString();
						sb << " " << f.FieldName << ";\n";
					}
					sb << "};\n";
				}
			}
		}

		void CLikeCodeGen::GenerateReferencedFunctions(StringBuilder & sb, ILProgram * program, ArrayView<ILWorld*> worlds)
		{
			EnumerableHashSet<String> refFuncs;
			for (auto & world : worlds)
				for (auto & func : world->ReferencedFunctions)
					refFuncs.Add(func);
			for (auto & func : program->Functions)
			{
				if (refFuncs.Contains(func.Value->Name))
				{
					GenerateFunctionDeclaration(sb, func.Value.Ptr());
					sb << ";\n";
				}
			}
			for (auto & func : program->Functions)
			{
				if (refFuncs.Contains(func.Value->Name))
					sb << GenerateFunction(func.Value.Ptr());
			}
		}

		ExternComponentCodeGenInfo CLikeCodeGen::ExtractExternComponentInfo(const ILObjectDefinition & input)
		{
			auto type = input.Type.Ptr();
			auto recType = ExtractRecordType(type);
			ExternComponentCodeGenInfo info;
			info.Type = type;
			Token bindingVal;
			if (input.Attributes.TryGetValue("Binding", bindingVal))
				info.Binding = StringToInt(bindingVal.Content);
			if (recType)
			{
				if (auto genType = dynamic_cast<ILGenericType*>(type))
				{
					if (genType->GenericTypeName == "Patch")
					{
						type = genType->BaseType.Ptr();
						info.DataStructure = ExternComponentCodeGenInfo::DataStructureType::Patch;
					}
				}
				if (auto arrType = dynamic_cast<ILArrayType*>(type))
				{
                    if (info.DataStructure != ExternComponentCodeGenInfo::DataStructureType::StandardInput &&
                        info.DataStructure != ExternComponentCodeGenInfo::DataStructureType::Patch)
                    {
                        errWriter->diagnose(input.Position, Diagnostics::cannotGenerateCodeForExternComponentType, type);
                    }
					type = arrType->BaseType.Ptr();
					info.IsArray = true;
					info.ArrayLength = arrType->ArrayLength;
				}
				if (type != recType)
				{
                    errWriter->diagnose(input.Position, Diagnostics::cannotGenerateCodeForExternComponentType, type);
				}
			}
			else
			{
				// check for attributes 
				if (input.Attributes.ContainsKey("TessCoord"))
				{
					info.SystemVar = ExternComponentCodeGenInfo::SystemVarType::TessCoord;
					if (!(input.Type->IsFloatVector() && input.Type->GetVectorSize() <= 3))
                        getSink()->diagnose(input.Position, Diagnostics::invalidTessCoordType);
				}
				else if (input.Attributes.ContainsKey("FragCoord"))
				{
					info.SystemVar = ExternComponentCodeGenInfo::SystemVarType::FragCoord;
					if (!(input.Type->IsFloatVector() && input.Type->GetVectorSize() == 4))
                        getSink()->diagnose(input.Position, Diagnostics::invalidFragCoordType);
				}
				else if (input.Attributes.ContainsKey("InvocationId"))
				{
					info.SystemVar = ExternComponentCodeGenInfo::SystemVarType::InvocationId;
					if (!input.Type->IsInt())
                        getSink()->diagnose(input.Position, Diagnostics::invalidInvocationIdType);
				}
				else if (input.Attributes.ContainsKey("ThreadId"))
				{
					info.SystemVar = ExternComponentCodeGenInfo::SystemVarType::InvocationId;
					if (!input.Type->IsInt())
                        getSink()->diagnose(input.Position, Diagnostics::invalidThreadIdType);
				}
				else if (input.Attributes.ContainsKey("PrimitiveId"))
				{
					info.SystemVar = ExternComponentCodeGenInfo::SystemVarType::PrimitiveId;
					if (!input.Type->IsInt())
                        getSink()->diagnose(input.Position, Diagnostics::invalidPrimitiveIdType);
				}
				else if (input.Attributes.ContainsKey("PatchVertexCount"))
				{
					info.SystemVar = ExternComponentCodeGenInfo::SystemVarType::PatchVertexCount;
					if (!input.Type->IsInt())
                        getSink()->diagnose(input.Position, Diagnostics::invalidPatchVertexCountType);
				}
			}
			return info;
		}

		void CLikeCodeGen::PrintInputReference(CodeGenContext & ctx, StringBuilder & sb, String input)
		{
			auto info = extCompInfo[input]();

			// TODO(tfoley): Is there any reason why this isn't just a `switch`?
			if (auto recType = ExtractRecordType(info.Type.Ptr()))
			{
				// TODO(tfoley): hoist this logic up to the top-level if chain?
				if(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StandardInput)
				{
					if(info.IsArray)
					{
						PrintStandardArrayInputReference(sb, recType, input, currentImportInstr->ComponentName);
					}
					else
					{
						PrintStandardInputReference(sb, recType, input, currentImportInstr->ComponentName);
					}
				}
				else if(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::Patch)
				{
					PrintPatchInputReference(sb, recType, input, currentImportInstr->ComponentName);
				}
				else
				{
					// TODO(tfoley): Does this case ever actually trigger?
					PrintDefaultInputReference(sb, recType, input, currentImportInstr->ComponentName);
				}
			}
			else
			{
				PrintSystemVarReference(ctx, sb, input, info.SystemVar);
			}
		}

		void CLikeCodeGen::DeclareInput(CodeGenContext & sb, const ILObjectDefinition & input, bool isVertexShader)
		{
			auto info = ExtractExternComponentInfo(input);
			extCompInfo[input.Name] = info;
			auto recType = ExtractRecordType(input.Type.Ptr());
			if (recType)
			{
				switch(info.DataStructure)
				{
				case ExternComponentCodeGenInfo::DataStructureType::StandardInput:
					DeclareStandardInputRecord(sb, input, isVertexShader);
					return;

				case ExternComponentCodeGenInfo::DataStructureType::Patch:
					DeclarePatchInputRecord(sb, input, isVertexShader);
					return;

				default:
                    SPIRE_INTERNAL_ERROR(getSink(), input.Position);
					break;
				}
			}
		}

		void CLikeCodeGen::GenerateVertexShaderEpilog(CodeGenContext & ctx, ILWorld * world, ILStage * stage)
		{
			StageAttribute positionVar;
			if (stage->Attributes.TryGetValue("Position", positionVar))
			{
				ILOperand * operand;
				if (world->Components.TryGetValue(positionVar.Value, operand))
				{
                    if (operand->Type->IsFloatVector() && operand->Type->GetVectorSize() == 4)
                    {
                        PrintRasterPositionOutputWrite(ctx, operand);
                    }
                    else
                        errWriter->diagnose(positionVar.Position, Diagnostics::componentHasInvalidTypeForPositionOutput, positionVar.Value);
				}
				else
					errWriter->diagnose(positionVar.Position, Diagnostics::componentNotDefined, positionVar.Value);
			}
		}

		StageSource CLikeCodeGen::GenerateVertexFragmentDomainShader(ILProgram * program, ILShader * shader, ILStage * stage)
		{
			RefPtr<ILWorld> world = nullptr;
			StageAttribute worldName;
			if (stage->Attributes.TryGetValue("World", worldName))
			{
				if (!shader->Worlds.TryGetValue(worldName.Value, world))
					errWriter->diagnose(worldName.Position, Diagnostics::worldIsNotDefined, worldName.Value);
			}
			outputStrategy = CreateStandardOutputStrategy(world.Ptr(), "");
			return GenerateSingleWorldShader(program, shader, stage);
		}

		StageSource CLikeCodeGen::GenerateComputeShader(ILProgram * program, ILShader * shader, ILStage * stage)
		{
			RefPtr<ILWorld> world = nullptr;
			StageAttribute worldName;
			if (stage->Attributes.TryGetValue("World", worldName))
			{
				if (!shader->Worlds.TryGetValue(worldName.Value, world))
					errWriter->diagnose(worldName.Position, Diagnostics::worldIsNotDefined, worldName);
			}
			outputStrategy = CreatePackedBufferOutputStrategy(world.Ptr());
			return GenerateSingleWorldShader(program, shader, stage);
		}

		void CLikeCodeGen::GenerateFunctionDeclaration(StringBuilder & sbCode, ILFunction * function)
		{
			function->Code->NameAllInstructions();
			auto retType = function->ReturnType.Ptr();
			if (retType)
				PrintType(sbCode, retType);
			else
				sbCode << "void";
			sbCode << " " << GetFuncOriginalName(function->Name) << "(";
			int id = 0;
			auto paramIter = function->Parameters.begin();
			for (auto & instr : *function->Code)
			{
				if (auto arg = instr.As<FetchArgInstruction>())
				{
					if (arg->ArgId != 0)
					{
						if (id > 0)
						{
							sbCode << ", ";
						}
						auto qualifier = (*paramIter).Value.Qualifier;
						if (qualifier == ParameterQualifier::InOut)
							sbCode << "inout ";
						else if (qualifier == ParameterQualifier::Out)
							sbCode << "out ";
						else if (qualifier == ParameterQualifier::Uniform)
							sbCode << "uniform ";
						PrintDef(sbCode, arg->Type.Ptr(), arg->Name);
						id++;
					}
					++paramIter;
				}
			}
			sbCode << ")";
		}
		String CLikeCodeGen::GenerateFunction(ILFunction * function)
		{
			StringBuilder sbCode;
			CodeGenContext ctx;
			ctx.codeGen = this;
			ctx.UsedVarNames.Clear();
			ctx.Body.Clear();
			ctx.Header.Clear();
			ctx.Arguments.Clear();
			ctx.ReturnVarName = "";
			ctx.VarName.Clear();
				
			function->Code->NameAllInstructions();
			GenerateFunctionDeclaration(sbCode, function);
			sbCode << "\n{\n";
			GenerateCode(ctx, function->Code.Ptr());
			sbCode << ctx.Header.ToString() << ctx.Body.ToString();
			if (ctx.ReturnVarName.Length())
				sbCode << "return " << ctx.ReturnVarName << ";\n";
			sbCode << "}\n";
			return sbCode.ProduceString();
		}

		String CodeGenContext::DefineVariable(ILOperand * op)
		{
			String rs;
			if (VarName.TryGetValue(op, rs))
			{
				return rs;
			}
			else
			{
				auto name = GenerateCodeName(op->Name, "");
				codeGen->PrintDef(Header, op->Type.Ptr(), name);
				if (op->Type->IsInt() || op->Type->IsUInt())
				{
					Header << " = 0";
				}
				Header << ";\n";
				VarName.Add(op, name);
				op->Name = name;
				return op->Name;
			}
		}
	}
}