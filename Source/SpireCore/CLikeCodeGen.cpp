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
				return EscapeDoubleUnderscore(name + L"_" + suffix);
		}


		void CLikeCodeGen::Error(int errId, String msg, CodePosition pos)
		{
			errWriter->Error(errId, msg, pos);
		}

		void CLikeCodeGen::PrintType(StringBuilder & sbCode, ILType* type)
		{
			PrintTypeName(sbCode, type);
		}

		void CLikeCodeGen::PrintDef(StringBuilder & sbCode, ILType* type, const String & name)
		{
			PrintType(sbCode, type);
			sbCode << L" ";
			sbCode << name;
			if (name.Length() == 0)
				throw InvalidProgramException(L"unnamed instruction.");
		}

		String CLikeCodeGen::GetFunctionCallName(String name)
		{
			StringBuilder rs;
			for (int i = 0; i < name.Length(); i++)
			{
				if ((name[i] >= L'a' && name[i] <= L'z') || (name[i] >= L'A' && name[i] <= L'Z') || 
					name[i] == L'_' || (name[i] >= L'0' && name[i] <= L'9'))
				{
					rs << name[i];
				}
				else if (i != name.Length() - 1)
					rs << L'_';
			}
			return rs.ProduceString();
		}

		String CLikeCodeGen::GetFuncOriginalName(const String & name)
		{
			String originalName;
			int splitPos = name.IndexOf(L'@');
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
				String rs(v, L"%.12e");
				if (!rs.Contains(L'.') && !rs.Contains(L'e') && !rs.Contains(L'E'))
					rs = rs + L".0";
				if (rs.StartsWith(L"-"))
					rs = L"(" + rs + L")";
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
					ctx.Body << ((c->IntValues[0] != 0) ? L"true" : L"false");
				else if (auto baseType = dynamic_cast<ILBasicType*>(type))
				{
					PrintType(ctx.Body, baseType);
					ctx.Body << "(";

					if (baseType->Type == ILBaseType::Float2)
						ctx.Body << makeFloat(c->FloatValues[0]) << L", " << makeFloat(c->FloatValues[1]);
					else if (baseType->Type == ILBaseType::Float3)
						ctx.Body << makeFloat(c->FloatValues[0]) << L", " << makeFloat(c->FloatValues[1]) << L", " << makeFloat(c->FloatValues[2]);
					else if (baseType->Type == ILBaseType::Float4)
						ctx.Body << makeFloat(c->FloatValues[0]) << L", " << makeFloat(c->FloatValues[1]) << L", " << makeFloat(c->FloatValues[2]) << L", " << makeFloat(c->FloatValues[3]);
					else if (baseType->Type == ILBaseType::Float3x3)
					{
						ctx.Body << L"mat3(";
						for (int i = 0; i < 9; i++)
						{
							ctx.Body << makeFloat(c->FloatValues[i]);
							if (i != 8)
								ctx.Body << L", ";
						}
						ctx.Body;
					}
					else if (baseType->Type == ILBaseType::Float4x4)
					{
						for (int i = 0; i < 16; i++)
						{
							ctx.Body << makeFloat(c->FloatValues[i]);
							if (i != 15)
								ctx.Body << L", ";
						}
					}
					else if (baseType->Type == ILBaseType::Int2)
						ctx.Body << c->IntValues[0] << L", " << c->IntValues[1];
					else if (baseType->Type == ILBaseType::Int3)
						ctx.Body << c->IntValues[0] << L", " << c->IntValues[1] << L", " << c->IntValues[2];
					else if (baseType->Type == ILBaseType::Int4)
						ctx.Body << c->IntValues[0] << L", " << c->IntValues[1] << L", " << c->IntValues[2] << L", " << c->IntValues[3];

					ctx.Body << ")";
				}
				else
					throw InvalidOperationException(L"Illegal constant.");
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
						throw InvalidProgramException(L"cannot generate code block as an expression.");
					String substituteName;
					if (ctx.SubstituteNames.TryGetValue(instr->Name, substituteName))
						ctx.Body << substituteName;
					else
						ctx.Body << instr->Name;
				}
			}
			else
				throw InvalidOperationException(L"Unsupported operand type.");
		}

		static bool IsMatrix(ILOperand* operand)
		{
			auto type = operand->Type;
			// TODO(tfoley): This needs to be expanded once other matrix types are supported
			return type->IsFloatMatrix();
		}

		void CLikeCodeGen::PrintMatrixMulInstrExpr(CodeGenContext & ctx, ILOperand* op0, ILOperand* op1)
		{
			ctx.Body << L"(";
			PrintOp(ctx, op0);
			ctx.Body << L" * ";
			PrintOp(ctx, op1);
			ctx.Body << L")";
		}

		void CLikeCodeGen::PrintBinaryInstrExpr(CodeGenContext & ctx, BinaryInstruction * instr)
		{
			if (instr->Is<StoreInstruction>())
			{
				auto op0 = instr->Operands[0].Ptr();
				auto op1 = instr->Operands[1].Ptr();
				ctx.Body << L"(";
				PrintOp(ctx, op0);
				ctx.Body << L" = ";
				PrintOp(ctx, op1);
				ctx.Body << L")";
				return;
			}
			auto op0 = instr->Operands[0].Ptr();
			auto op1 = instr->Operands[1].Ptr();
			if (instr->Is<StoreInstruction>())
			{
				throw InvalidOperationException(L"store instruction cannot appear as expression.");
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
							ctx.Body << L".x";
							break;
						case 1:
							ctx.Body << L".y";
							break;
						case 2:
							ctx.Body << L".z";
							break;
						case 3:
							ctx.Body << L".w";
							break;
						default:
							throw InvalidOperationException(L"Invalid member access.");
						}
						printDefault = false;
					}
				}
				else if (auto structType = dynamic_cast<ILStructType*>(op0->Type.Ptr()))
				{
					if (auto c = dynamic_cast<ILConstOperand*>(op1))
					{
						ctx.Body << L"." << structType->Members[c->IntValues[0]].FieldName;
					}
					printDefault = false;
				}
				if (printDefault)
				{
					ctx.Body << L"[";
					PrintOp(ctx, op1);
					ctx.Body << L"]";
				}
				
				
				return;
			}
			const wchar_t * op = L"";
			if (instr->Is<AddInstruction>())
			{
				op = L"+";
			}
			else if (instr->Is<SubInstruction>())
			{
				op = L"-";
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

				op = L"*";
			}
			else if (instr->Is<DivInstruction>())
			{
				op = L"/";
			}
			else if (instr->Is<ModInstruction>())
			{
				op = L"%";
			}
			else if (instr->Is<ShlInstruction>())
			{
				op = L"<<";
			}
			else if (instr->Is<ShrInstruction>())
			{
				op = L">>";
			}
			else if (instr->Is<CmpeqlInstruction>())
			{
				op = L"==";
				//ctx.Body << L"int";
			}
			else if (instr->Is<CmpgeInstruction>())
			{
				op = L">=";
				//ctx.Body << L"int";
			}
			else if (instr->Is<CmpgtInstruction>())
			{
				op = L">";
				//ctx.Body << L"int";
			}
			else if (instr->Is<CmpleInstruction>())
			{
				op = L"<=";
				//ctx.Body << L"int";
			}
			else if (instr->Is<CmpltInstruction>())
			{
				op = L"<";
				//ctx.Body << L"int";
			}
			else if (instr->Is<CmpneqInstruction>())
			{
				op = L"!=";
				//ctx.Body << L"int";
			}
			else if (instr->Is<AndInstruction>())
			{
				op = L"&&";
			}
			else if (instr->Is<OrInstruction>())
			{
				op = L"||";
			}
			else if (instr->Is<BitXorInstruction>())
			{
				op = L"^";
			}
			else if (instr->Is<BitAndInstruction>())
			{
				op = L"&";
			}
			else if (instr->Is<BitOrInstruction>())
			{
				op = L"|";
			}
			else
				throw InvalidProgramException(L"unsupported binary instruction.");
			ctx.Body << L"(";
			PrintOp(ctx, op0);
			ctx.Body << L" " << op << L" ";
			PrintOp(ctx, op1);
			ctx.Body << L")";
		}

		void CLikeCodeGen::PrintBinaryInstr(CodeGenContext & ctx, BinaryInstruction * instr)
		{
			auto op0 = instr->Operands[0].Ptr();
			auto op1 = instr->Operands[1].Ptr();
			if (instr->Is<StoreInstruction>())
			{
				PrintOp(ctx, op0);
				ctx.Body << L" = ";
				PrintOp(ctx, op1);
				ctx.Body << L";\n";
				return;
			}
			auto varName = ctx.DefineVariable(instr);
			if (instr->Is<MemberLoadInstruction>())
			{
				ctx.Body << varName << L" = ";
				PrintBinaryInstrExpr(ctx, instr);
				ctx.Body << L";\n";
				return;
			}
			ctx.Body << varName << L" = ";
			PrintBinaryInstrExpr(ctx, instr);
			ctx.Body << L";\n";
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
			const wchar_t * op = L"";
			if (instr->Is<BitNotInstruction>())
				op = L"~";
			else if (instr->Is<Float2IntInstruction>())
				op = L"(int)";
			else if (instr->Is<Int2FloatInstruction>())
				op = L"(float)";
			else if (instr->Is<CopyInstruction>())
				op = L"";
			else if (instr->Is<NegInstruction>())
				op = L"-";
			else if (instr->Is<NotInstruction>())
				op = L"!";
			else
				throw InvalidProgramException(L"unsupported unary instruction.");
			ctx.Body << L"(" << op;
			PrintOp(ctx, op0);
			ctx.Body << L")";
		}

		void CLikeCodeGen::PrintUnaryInstr(CodeGenContext & ctx, UnaryInstruction * instr)
		{
			auto varName = ctx.DefineVariable(instr);
			ctx.Body << varName << L" = ";
			PrintUnaryInstrExpr(ctx, instr);
			ctx.Body << L";\n";
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
				throw InvalidProgramException(L"size operand of allocVar instr is not an intermediate.");
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
			ctx.Body << L"(";
			PrintOp(ctx, instr->Operands[0].Ptr());
			ctx.Body << L"?";
			PrintOp(ctx, instr->Operands[1].Ptr());
			ctx.Body << L":";
			PrintOp(ctx, instr->Operands[2].Ptr());
			ctx.Body << L")";
		}

		void CLikeCodeGen::PrintSelectInstr(CodeGenContext & ctx, SelectInstruction * instr)
		{
			auto varName = ctx.DefineVariable(instr);
			ctx.Body << varName << L" = ";
			PrintSelectInstrExpr(ctx, instr);
			ctx.Body << L";\n";
		}

		void CLikeCodeGen::PrintCallInstrExprForTarget(CodeGenContext & ctx, CallInstruction * instr, String const& name)
		{
			PrintDefaultCallInstrExpr(ctx, instr, name);
		}

		void CLikeCodeGen::PrintDefaultCallInstrArgs(CodeGenContext & ctx, CallInstruction * instr)
		{
			ctx.Body << L"(";
			int id = 0;
			for (auto & arg : instr->Arguments)
			{
				PrintOp(ctx, arg.Ptr());
				if (id != instr->Arguments.Count() - 1)
					ctx.Body << L", ";
				id++;
			}
			ctx.Body << L")";
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
				ctx.Body << L" = ";
			}
			PrintCallInstrExpr(ctx, instr);
			ctx.Body << L";\n";
		}

		void CLikeCodeGen::PrintCastF2IInstrExpr(CodeGenContext & ctx, Float2IntInstruction * instr)
		{
			ctx.Body << L"((int)(";
			PrintOp(ctx, instr->Operand.Ptr());
			ctx.Body << L"))";
		}
		void CLikeCodeGen::PrintCastF2IInstr(CodeGenContext & ctx, Float2IntInstruction * instr)
		{
			auto varName = ctx.DefineVariable(instr);
			ctx.Body << varName;
			ctx.Body << L" = ";
			PrintCastF2IInstrExpr(ctx, instr);
			ctx.Body << L";\n";
		}
		void CLikeCodeGen::PrintCastI2FInstrExpr(CodeGenContext & ctx, Int2FloatInstruction * instr)
		{
			ctx.Body << L"((float)(";
			PrintOp(ctx, instr->Operand.Ptr());
			ctx.Body << L"))";
		}
		void CLikeCodeGen::PrintCastI2FInstr(CodeGenContext & ctx, Int2FloatInstruction * instr)
		{
			auto varName = ctx.DefineVariable(instr);
			ctx.Body << varName;
			ctx.Body << L" = ";
			PrintCastI2FInstrExpr(ctx, instr);
			ctx.Body << L";\n";
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
					ctx.Body << L".";
					ctx.Body << structType->Members[dynamic_cast<ILConstOperand*>(op1)->IntValues[0]].FieldName;
				}
				else
				{
					ctx.Body << L"[";
					PrintOp(ctx, op1);
					ctx.Body << L"]";
				}
				ctx.Body << L" = ";
				PrintOp(ctx, op2);
				ctx.Body << L";\n";
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
			ctx.Body << L"." << swizzle->SwizzleString;
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
				throw InvalidOperationException(L"member update instruction cannot appear as expression.");
		}

		void CLikeCodeGen::PrintInstr(CodeGenContext & ctx, ILInstruction & instr)
		{
			// ctx.Body << L"// " << instr.ToString() << L";\n";
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
					context.Body << L"if (bool(";
					PrintOp(context, ifInstr->Operand.Ptr(), true);
					context.Body << L"))\n{\n";
					GenerateCode(context, ifInstr->TrueCode.Ptr());
					context.Body << L"}\n";
					if (ifInstr->FalseCode)
					{
						context.Body << L"else\n{\n";
						GenerateCode(context, ifInstr->FalseCode.Ptr());
						context.Body << L"}\n";
					}
				}
				else if (auto forInstr = instr.As<ForInstruction>())
				{
					context.Body << L"for (";
					if (forInstr->InitialCode)
						PrintOp(context, forInstr->InitialCode->GetLastInstruction(), true);
					context.Body << L"; ";
					if (forInstr->ConditionCode)
						PrintOp(context, forInstr->ConditionCode->GetLastInstruction(), true);
					context.Body << L"; ";
					if (forInstr->SideEffectCode)
						PrintOp(context, forInstr->SideEffectCode->GetLastInstruction(), true);
					context.Body << L")\n{\n";
					GenerateCode(context, forInstr->BodyCode.Ptr());
					context.Body << L"}\n";
				}
				else if (auto doInstr = instr.As<DoInstruction>())
				{
					context.Body << L"do\n{\n";
					GenerateCode(context, doInstr->BodyCode.Ptr());
					context.Body << L"} while (bool(";
					PrintOp(context, doInstr->ConditionCode->GetLastInstruction()->As<ReturnInstruction>()->Operand.Ptr(), true);
					context.Body << L"));\n";
				}
				else if (auto whileInstr = instr.As<WhileInstruction>())
				{
					context.Body << L"while (bool(";
					PrintOp(context, whileInstr->ConditionCode->GetLastInstruction()->As<ReturnInstruction>()->Operand.Ptr(), true);
					context.Body << L"))\n{\n";
					GenerateCode(context, whileInstr->BodyCode.Ptr());
					context.Body << L"}\n";
				}
				else if (auto ret = instr.As<ReturnInstruction>())
				{
					if (currentImportInstr) 
					{
						context.Body << currentImportInstr->Name << L" = ";
						PrintOp(context, ret->Operand.Ptr());
						context.Body << L";\n";
					}
					else
					{
						context.Body << L"return ";
						PrintOp(context, ret->Operand.Ptr());
						context.Body << L";\n";
					}
				}
				else if (instr.Is<BreakInstruction>())
				{
					context.Body << L"break;\n";
				}
				else if (instr.Is<ContinueInstruction>())
				{
					context.Body << L"continue;\n";
				}
				else if (instr.Is<DiscardInstruction>())
				{
					context.Body << L"discard;\n";
				}
				else
					PrintInstr(context, instr);
			}
		}

		CLikeCodeGen::CLikeCodeGen()
		{
			intrinsicTextureFunctions.Add(L"Sample");
			intrinsicTextureFunctions.Add(L"SampleBias");
			intrinsicTextureFunctions.Add(L"SampleGrad");
		}

		CompiledShaderSource CLikeCodeGen::GenerateShader(CompileResult & result, SymbolTable *, ILShader * shader, ErrorWriter * err)
		{
			this->errWriter = err;

			CompiledShaderSource rs;

			for (auto & stage : shader->Stages)
			{
				StageSource src;
				if (stage.Value->StageType == L"VertexShader" || stage.Value->StageType == L"FragmentShader" || stage.Value->StageType == L"DomainShader")
					src = GenerateVertexFragmentDomainShader(result.Program.Ptr(), shader, stage.Value.Ptr());
				else if (stage.Value->StageType == L"ComputeShader")
					src = GenerateComputeShader(result.Program.Ptr(), shader, stage.Value.Ptr());
				else if (stage.Value->StageType == L"HullShader")
					src = GenerateHullShader(result.Program.Ptr(), shader, stage.Value.Ptr());
				else
					errWriter->Error(50020, L"Unknown stage type '" + stage.Value->StageType + L"'.", stage.Value->Position);
				rs.Stages[stage.Key] = src;
			}
				
			// TODO: fill metadatas
			rs.MetaData.ShaderName = shader->Name;
				
			return rs;
		}

		void CLikeCodeGen::GenerateStructs(StringBuilder & sb, ILProgram * program)
		{
			for (auto & st : program->Structs)
			{
				if (!st->IsIntrinsic)
				{
					sb << L"struct " << st->TypeName << L"\n{\n";
					for (auto & f : st->Members)
					{
						sb << f.Type->ToString();
						sb << " " << f.FieldName << L";\n";
					}
					sb << L"};\n";
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
					sb << L";\n";
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
			String bindingVal;
			if (input.Attributes.TryGetValue(L"Binding", bindingVal))
				info.Binding = StringToInt(bindingVal);
			if (recType)
			{
				if (auto genType = dynamic_cast<ILGenericType*>(type))
				{
					type = genType->BaseType.Ptr();
					if (genType->GenericTypeName == L"Uniform")
						info.DataStructure = ExternComponentCodeGenInfo::DataStructureType::UniformBuffer;
					else if (genType->GenericTypeName == L"Patch")
						info.DataStructure = ExternComponentCodeGenInfo::DataStructureType::Patch;
					else if (genType->GenericTypeName == L"Texture")
						info.DataStructure = ExternComponentCodeGenInfo::DataStructureType::Texture;
					else if (genType->GenericTypeName == L"PackedBuffer")
						info.DataStructure = ExternComponentCodeGenInfo::DataStructureType::PackedBuffer;
					else if (genType->GenericTypeName == L"StructuredBuffer" || genType->GenericTypeName == L"RWStructuredBuffer")
						info.DataStructure = ExternComponentCodeGenInfo::DataStructureType::ArrayBuffer;
				}
				if (auto arrType = dynamic_cast<ILArrayType*>(type))
				{
					if (info.DataStructure != ExternComponentCodeGenInfo::DataStructureType::StandardInput &&
						info.DataStructure != ExternComponentCodeGenInfo::DataStructureType::UniformBuffer &&
						info.DataStructure != ExternComponentCodeGenInfo::DataStructureType::Patch)
						errWriter->Error(51090, L"cannot generate code for extern component type '" + type->ToString() + L"'.",
							input.Position);
					type = arrType->BaseType.Ptr();
					info.IsArray = true;
					info.ArrayLength = arrType->ArrayLength;
				}
				if (type != recType)
				{
					errWriter->Error(51090, L"cannot generate code for extern component type '" + type->ToString() + L"'.",
						input.Position);
				}
			}
			else
			{
				// check for attributes 
				if (input.Attributes.ContainsKey(L"TessCoord"))
				{
					info.SystemVar = ExternComponentCodeGenInfo::SystemVarType::TessCoord;
					if (!(input.Type->IsFloatVector() && input.Type->GetVectorSize() <= 3))
						Error(50020, L"TessCoord must have vec2 or vec3 type.", input.Position);
				}
				else if (input.Attributes.ContainsKey(L"FragCoord"))
				{
					info.SystemVar = ExternComponentCodeGenInfo::SystemVarType::FragCoord;
					if (!(input.Type->IsFloatVector() && input.Type->GetVectorSize() == 4))
						Error(50020, L"FragCoord must be a vec4.", input.Position);
				}
				else if (input.Attributes.ContainsKey(L"InvocationId"))
				{
					info.SystemVar = ExternComponentCodeGenInfo::SystemVarType::InvocationId;
					if (!input.Type->IsInt())
						Error(50020, L"InvocationId must have int type.", input.Position);
				}
				else if (input.Attributes.ContainsKey(L"ThreadId"))
				{
					info.SystemVar = ExternComponentCodeGenInfo::SystemVarType::InvocationId;
					if (!input.Type->IsInt())
						Error(50020, L"ThreadId must have int type.", input.Position);
				}
				else if (input.Attributes.ContainsKey(L"PrimitiveId"))
				{
					info.SystemVar = ExternComponentCodeGenInfo::SystemVarType::PrimitiveId;
					if (!input.Type->IsInt())
						Error(50020, L"PrimitiveId must have int type.", input.Position);
				}
				else if (input.Attributes.ContainsKey(L"PatchVertexCount"))
				{
					info.SystemVar = ExternComponentCodeGenInfo::SystemVarType::PatchVertexCount;
					if (!input.Type->IsInt())
						Error(50020, L"PatchVertexCount must have int type.", input.Position);
				}
			}
			return info;
		}

		void CLikeCodeGen::PrintInputReference(CodeGenContext & ctx, StringBuilder & sb, String input)
		{
			auto info = extCompInfo[input]();

			// TODO(tfoley): Is there any reason why this isn't just a `switch`?
			if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::UniformBuffer)
			{
				PrintUniformBufferInputReference(sb, input, currentImportInstr->ComponentName);
			}
			else if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::ArrayBuffer)
			{
				PrintArrayBufferInputReference(sb, input, currentImportInstr->ComponentName);
			}
			else if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::PackedBuffer)
			{
				PrintPackedBufferInputReference(sb, input, currentImportInstr->ComponentName);
			}
			else if (auto recType = ExtractRecordType(info.Type.Ptr()))
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
				case ExternComponentCodeGenInfo::DataStructureType::UniformBuffer:
					DeclareUniformBuffer(sb, input, isVertexShader);
					return;

				case ExternComponentCodeGenInfo::DataStructureType::ArrayBuffer:
					DeclareArrayBuffer(sb, input, isVertexShader);
					return;

				case ExternComponentCodeGenInfo::DataStructureType::PackedBuffer:
					DeclarePackedBuffer(sb, input, isVertexShader);
					return;

				case ExternComponentCodeGenInfo::DataStructureType::Texture:
					DeclareTextureInputRecord(sb, input, isVertexShader);
					return;

				case ExternComponentCodeGenInfo::DataStructureType::StandardInput:
					DeclareStandardInputRecord(sb, input, isVertexShader);
					return;

				case ExternComponentCodeGenInfo::DataStructureType::Patch:
					DeclarePatchInputRecord(sb, input, isVertexShader);
					return;

				default:
					errWriter->Error(99999, L"internal error: unexpected data structure for record type",
							input.Position);
					break;
				}
			}
		}

		void CLikeCodeGen::GenerateVertexShaderEpilog(CodeGenContext & ctx, ILWorld * world, ILStage * stage)
		{
			StageAttribute positionVar;
			if (stage->Attributes.TryGetValue(L"Position", positionVar))
			{
				ILOperand * operand;
				if (world->Components.TryGetValue(positionVar.Value, operand))
				{
					if(operand->Type->IsFloatVector() && operand->Type->GetVectorSize() == 4)
					{
						PrintRasterPositionOutputWrite(ctx, operand);
					}
					else
						errWriter->Error(50040, L"'" + positionVar.Value + L"': component used as 'Position' output must be of vec4 type.",
							positionVar.Position);
				}
				else
					errWriter->Error(50041, L"'" + positionVar.Value + L"': component not defined.",
						positionVar.Position);
			}
		}

		StageSource CLikeCodeGen::GenerateVertexFragmentDomainShader(ILProgram * program, ILShader * shader, ILStage * stage)
		{
			RefPtr<ILWorld> world = nullptr;
			StageAttribute worldName;
			if (stage->Attributes.TryGetValue(L"World", worldName))
			{
				if (!shader->Worlds.TryGetValue(worldName.Value, world))
					errWriter->Error(50022, L"world '" + worldName.Value + L"' is not defined.", worldName.Position);
			}
			outputStrategy = CreateStandardOutputStrategy(world.Ptr(), L"");
			return GenerateSingleWorldShader(program, shader, stage);
		}

		StageSource CLikeCodeGen::GenerateComputeShader(ILProgram * program, ILShader * shader, ILStage * stage)
		{
			RefPtr<ILWorld> world = nullptr;
			StageAttribute worldName;
			if (stage->Attributes.TryGetValue(L"World", worldName))
			{
				if (!shader->Worlds.TryGetValue(worldName.Value, world))
					errWriter->Error(50022, L"world '" + worldName.Value + L"' is not defined.", worldName.Position);
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
				sbCode << L"void";
			sbCode << L" " << GetFuncOriginalName(function->Name) << L"(";
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
							sbCode << L", ";
						}
						auto qualifier = (*paramIter).Value.Qualifier;
						if (qualifier == ParameterQualifier::InOut)
							sbCode << L"inout ";
						else if (qualifier == ParameterQualifier::Out)
							sbCode << L"out ";
						else if (qualifier == ParameterQualifier::Uniform)
							sbCode << L"uniform ";
						PrintDef(sbCode, arg->Type.Ptr(), arg->Name);
						id++;
					}
					++paramIter;
				}
			}
			sbCode << L")";
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
			ctx.ReturnVarName = L"";
			ctx.VarName.Clear();
				
			function->Code->NameAllInstructions();
			GenerateFunctionDeclaration(sbCode, function);
			sbCode << L"\n{\n";
			GenerateCode(ctx, function->Code.Ptr());
			sbCode << ctx.Header.ToString() << ctx.Body.ToString();
			if (ctx.ReturnVarName.Length())
				sbCode << L"return " << ctx.ReturnVarName << L";\n";
			sbCode << L"}\n";
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
				auto name = GenerateCodeName(op->Name, L"");
				codeGen->PrintDef(Header, op->Type.Ptr(), name);
				if (op->Type->IsInt() || op->Type->IsUInt())
				{
					Header << L" = 0";
				}
				Header << L";\n";
				VarName.Add(op, name);
				op->Name = name;
				return op->Name;
			}
		}
	}
}