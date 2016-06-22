#include "CodeGenBackend.h"
#include "../CoreLib/Parser.h"
#include "Syntax.h"

using namespace CoreLib::Basic;

namespace Spire
{
	namespace Compiler
	{
		void PrintBaseType(StringBuilder & sbCode, ILType* type)
		{
			if (auto baseType = dynamic_cast<ILBasicType*>(type))
			{
				switch (baseType->Type)
				{
				case ILBaseType::Int:
					sbCode << L"int";
					break;
				case ILBaseType::Int2:
					sbCode << L"ivec2";
					break;
				case ILBaseType::Int3:
					sbCode << L"ivec3";
					break;
				case ILBaseType::Int4:
					sbCode << L"ivec4";
					break;
				case ILBaseType::Float:
					sbCode << L"float";
					break;
				case ILBaseType::Float2:
					sbCode << L"vec2";
					break;
				case ILBaseType::Float3:
					sbCode << L"vec3";
					break;
				case ILBaseType::Float4:
					sbCode << L"vec4";
					break;
				case ILBaseType::Float3x3:
					sbCode << L"mat3";
					break;
				case ILBaseType::Float4x4:
					sbCode << L"mat4";
					break;
				case ILBaseType::Texture2D:
					sbCode << L"sampler2D";
					break;
				case ILBaseType::TextureCube:
					sbCode << L"samplerCube";
					break;
				case ILBaseType::TextureShadow:
					sbCode << L"sampler2DShadow";
					break;
				case ILBaseType::TextureCubeShadow:
					sbCode << L"samplerCubeShadow";
					break;
				default:
					throw NotImplementedException(L"unkown base type.");
				}
			}
			else
				throw NotImplementedException(L"unkown base type.");
		}

		void PrintDef(StringBuilder & sbCode, ILType* type, const String & name)
		{
			if (auto arrType = dynamic_cast<ILArrayType*>(type))
			{
				PrintDef(sbCode, arrType->BaseType.Ptr(), name + L"[" + arrType->ArrayLength + L"]");
			}
			else if (dynamic_cast<ILBasicType*>(type))
			{
				PrintBaseType(sbCode, type);
				sbCode << L" ";
				sbCode << name;
			}
		}

		class CodeGenContext
		{
		public:
			HashSet<String> GeneratedDefinitions;
			Dictionary<String, String> SubstituteNames;
			Dictionary<ILOperand*, String> VarName;
			Dictionary<String, ImportOperatorHandler *> ImportOperatorHandlers;
			Dictionary<String, ExportOperatorHandler *> ExportOperatorHandlers;

			CompileResult * Result = nullptr;
			HashSet<String> UsedVarNames;
			StringBuilder Body, Header, GlobalHeader;
			List<ILType*> Arguments;
			String ReturnVarName;
			String GenerateCodeName(String name, String prefix)
			{
				StringBuilder nameBuilder;
				int startPos = 0;
				if (name.StartsWith(L"_sys_"))
					startPos = name.IndexOf(L'_', 5) + 1;
				nameBuilder << prefix;
				for (int i = startPos; i < name.Length(); i++)
				{
					if ((name[i] >= L'a' && name[i] <= L'z') || 
						(name[i] >= L'A' && name[i] <= L'Z') ||
						name[i] == L'_' || 
						(name[i] >= L'0' && name[i] <= L'9'))
					{
						nameBuilder << name[i];
					}
					else
						nameBuilder << L'_';
				}
				auto rs = nameBuilder.ToString();
				int i = 0;
				while (UsedVarNames.Contains(rs))
				{
					i++;
					rs = nameBuilder.ToString() + String(i);
				}
				UsedVarNames.Add(rs);

				return rs;
			}


			String DefineVariable(ILOperand * op)
			{
				String rs;
				if (op->Name == L"Tex")
					printf("break");
				if (VarName.TryGetValue(op, rs))
				{
					return rs;
				}
				else
				{
					auto name = GenerateCodeName(op->Name, L"");
					PrintDef(Header, op->Type.Ptr(), name);
					if (op->Type->IsInt())
					{
						Header << L" = 0;";
					}
					Header << L";\n";
					VarName.Add(op, name);
					op->Name = name;
					return op->Name;
				}
			}
		};

		class GLSLShaderState : public Object
		{
		public:
		};

		class GLSLCodeGen : public CodeGenBackend
		{
		private:
			String vertexOutputName;
			bool useNVCommandList = false;
			CompiledWorld * currentWorld = nullptr;
		private:
			String GetFunctionCallName(String name)
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

			String GetFuncOriginalName(const String & name)
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

			void PrintOp(CodeGenContext & ctx, ILOperand * op, bool forceExpression = false)
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
					else if (auto baseType = dynamic_cast<ILBasicType*>(type))
					{
						if (baseType->Type == ILBaseType::Float2)
							ctx.Body << L"vec2(" << makeFloat(c->FloatValues[0]) << L", " << makeFloat(c->FloatValues[1]) << L")";
						else if (baseType->Type == ILBaseType::Float3)
							ctx.Body << L"vec3(" << makeFloat(c->FloatValues[0]) << L", " << makeFloat(c->FloatValues[1]) << L", " << makeFloat(c->FloatValues[2]) << L")";
						else if (baseType->Type == ILBaseType::Float4)
							ctx.Body << L"vec4(" << makeFloat(c->FloatValues[0]) << L", " << makeFloat(c->FloatValues[1]) << L", " << makeFloat(c->FloatValues[2]) << L", " << makeFloat(c->FloatValues[3]) << L")";
						else if (baseType->Type == ILBaseType::Float3x3)
						{
							ctx.Body << L"mat3(";
							for (int i = 0; i < 9; i++)
							{
								ctx.Body << makeFloat(c->FloatValues[i]);
								if (i != 8)
									ctx.Body << L", ";
							}
							ctx.Body << L")";
						}
						else if (baseType->Type == ILBaseType::Float4x4)
						{
							ctx.Body << L"mat4(";
							for (int i = 0; i < 16; i++)
							{
								ctx.Body << makeFloat(c->FloatValues[i]);
								if (i != 15)
									ctx.Body << L", ";
							}
							ctx.Body << L")";
						}
						else if (baseType->Type == ILBaseType::Int2)
							ctx.Body << L"ivec2(" << c->IntValues[0] << L", " << c->IntValues[1] << L")";
						else if (baseType->Type == ILBaseType::Int3)
							ctx.Body << L"ivec3(" << c->IntValues[0] << L", " << c->IntValues[1] << L", " << c->IntValues[2] << L")";
						else if (baseType->Type == ILBaseType::Int4)
							ctx.Body << L"ivec4(" << c->IntValues[0] << L", " << c->IntValues[1] << L", " << c->IntValues[2] << L", " << c->IntValues[3] << L")";
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

			void PrintBinaryInstrExpr(CodeGenContext & ctx, BinaryInstruction * instr)
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
					ctx.Body << L"int";
				}
				else if (instr->Is<CmpgeInstruction>())
				{
					op = L">=";
					ctx.Body << L"int";
				}
				else if (instr->Is<CmpgtInstruction>())
				{
					op = L">";
					ctx.Body << L"int";
				}
				else if (instr->Is<CmpleInstruction>())
				{
					op = L"<=";
					ctx.Body << L"int";
				}
				else if (instr->Is<CmpltInstruction>())
				{
					op = L"<";
					ctx.Body << L"int";
				}
				else if (instr->Is<CmpneqInstruction>())
				{
					op = L"!=";
					ctx.Body << L"int";
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

			void PrintBinaryInstr(CodeGenContext & ctx, BinaryInstruction * instr)
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

			void PrintUnaryInstrExpr(CodeGenContext & ctx, UnaryInstruction * instr)
			{
				auto op0 = instr->Operand.Ptr();
				if (instr->Is<LoadInstruction>())
				{
					PrintOp(ctx, op0);
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

			void PrintUnaryInstr(CodeGenContext & ctx, UnaryInstruction * instr)
			{
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName << L" = ";
				PrintUnaryInstrExpr(ctx, instr);
				ctx.Body << L";\n";
			}

			void PrintGleaInstrExpr(CodeGenContext & ctx, GLeaInstruction * instr)
			{
				RefPtr<CompiledGlobalVar> gvar;
				ctx.Body << instr->VariableName;
			}

			void PrintGleaInstr(CodeGenContext & /*ctx*/, GLeaInstruction * /*instr*/)
			{

			}

			void PrintAllocVarInstrExpr(CodeGenContext & ctx, AllocVarInstruction * instr)
			{
				ctx.Body << instr->Name;
			}

			void PrintAllocVarInstr(CodeGenContext & ctx, AllocVarInstruction * instr)
			{
				if (dynamic_cast<ILConstOperand*>(instr->Size.Ptr()))
				{
					PrintDef(ctx.Header, instr->Type.Ptr(), instr->Name);
					ctx.Header << L";\n";
				}
				else
					throw InvalidProgramException(L"size operand of allocVar instr is not an intermediate.");
			}

			void PrintFetchArgInstrExpr(CodeGenContext & ctx, FetchArgInstruction * instr)
			{
				ctx.Body << instr->Name;
			}

			void PrintFetchArgInstr(CodeGenContext & ctx, FetchArgInstruction * instr)
			{
				if (instr->ArgId == 0)
				{
					ctx.ReturnVarName = ctx.DefineVariable(instr);
				}
			}

			void PrintSelectInstrExpr(CodeGenContext & ctx, SelectInstruction * instr)
			{
				ctx.Body << L"(";
				PrintOp(ctx, instr->Operands[0].Ptr());
				ctx.Body << L"?";
				PrintOp(ctx, instr->Operands[1].Ptr());
				ctx.Body << L":";
				PrintOp(ctx, instr->Operands[2].Ptr());
				ctx.Body << L")";
			}

			void PrintSelectInstr(CodeGenContext & ctx, SelectInstruction * instr)
			{
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName << L" = ";
				PrintSelectInstrExpr(ctx, instr);
				ctx.Body << L";\n";
			}

			void PrintCallInstrExpr(CodeGenContext & ctx, CallInstruction * instr)
			{
				String callName;
				callName = GetFuncOriginalName(instr->Function);
				ctx.Body << callName;
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

			void PrintCallInstr(CodeGenContext & ctx, CallInstruction * instr)
			{
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName;
				ctx.Body << L" = ";
				PrintCallInstrExpr(ctx, instr);
				ctx.Body << L";\n";
			}

			void PrintCastF2IInstrExpr(CodeGenContext & ctx, Float2IntInstruction * instr)
			{
				ctx.Body << L"((int)(";
				PrintOp(ctx, instr->Operand.Ptr());
				ctx.Body << L"))";
			}
			void PrintCastF2IInstr(CodeGenContext & ctx, Float2IntInstruction * instr)
			{
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName;
				ctx.Body << L" = ";
				PrintCastF2IInstrExpr(ctx, instr);
				ctx.Body << L";\n";
			}
			void PrintCastI2FInstrExpr(CodeGenContext & ctx, Int2FloatInstruction * instr)
			{
				ctx.Body << L"((float)(";
				PrintOp(ctx, instr->Operand.Ptr());
				ctx.Body << L"))";
			}
			void PrintCastI2FInstr(CodeGenContext & ctx, Int2FloatInstruction * instr)
			{
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName;
				ctx.Body << L" = ";
				PrintCastI2FInstrExpr(ctx, instr);
				ctx.Body << L";\n";
			}

			bool AppearAsExpression(ILInstruction & instr, bool force)
			{
				if (auto arg = instr.As<FetchArgInstruction>())
				{
					if (arg->ArgId == 0)
						return false;
				}
				for (auto &&usr : instr.Users)
				{
					if (auto update = dynamic_cast<MemberUpdateInstruction*>(usr))
					{
						if (&instr == update->Operands[0].Ptr())
							return false;
					}
					else if (dynamic_cast<ImportInstruction*>(usr))
						return false;
				}
				if (instr.Is<StoreInstruction>() && force)
					return true;
				return (instr.Users.Count() <= 1 && !instr.HasSideEffect() && !instr.Is<MemberUpdateInstruction>()
					&& !instr.Is<AllocVarInstruction>() && !instr.Is<ImportInstruction>())
					|| instr.Is<GLeaInstruction>() || instr.Is<FetchArgInstruction>() ;
			}

			void PrintImportInstr(CodeGenContext &ctx, ImportInstruction * import)
			{
				ImportOperatorHandler * handler = nullptr;
				if (ctx.ImportOperatorHandlers.TryGetValue(import->ImportOperator->Name.Content, handler))
				{
					handler->GenerateInterfaceLocalDefinition(ctx.Body, import, ImportOperatorContext(
						import->ImportOperator->Arguments, backendArguments, currentWorld, *ctx.Result,
						import->SourceWorld));
				}
			}

			void PrintExportInstr(CodeGenContext &ctx, ExportInstruction * exportInstr)
			{
				ExportOperatorHandler * handler = nullptr;
				if (ctx.ExportOperatorHandlers.TryGetValue(exportInstr->ExportOperator, handler))
				{
					handler->GenerateExport(ctx.Body, currentWorld->WorldOutput, currentWorld, exportInstr->ComponentName, exportInstr->Operand->Name);
				}
			}

			void PrintUpdateInstr(CodeGenContext & ctx, MemberUpdateInstruction * instr)
			{
				if (auto srcInstr = dynamic_cast<ILInstruction*>(instr->Operands[0].Ptr()))
				{
					if (srcInstr->Users.Count() == 1)
					{
						auto srcName = srcInstr->Name;
						while (ctx.SubstituteNames.TryGetValue(srcName, srcName));
						ctx.Body << srcName << L"[";
						PrintOp(ctx, instr->Operands[1].Ptr());
						ctx.Body << L"] = ";
						PrintOp(ctx, instr->Operands[2].Ptr());
						ctx.Body << L";\n";
						ctx.SubstituteNames[instr->Name] = srcName;
						return;
					}
				}
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName << L" = ";
				PrintOp(ctx, instr->Operands[0].Ptr());
				ctx.Body << L";\n";
				ctx.Body << varName << L"[";
				PrintOp(ctx, instr->Operands[1].Ptr());
				ctx.Body << L"] = ";
				PrintOp(ctx, instr->Operands[2].Ptr());
				ctx.Body << L";\n";
			}

			void PrintInstrExpr(CodeGenContext & ctx, ILInstruction & instr)
			{
				if (auto binInstr = instr.As<BinaryInstruction>())
					PrintBinaryInstrExpr(ctx, binInstr);
				else if (auto unaryInstr = instr.As<UnaryInstruction>())
					PrintUnaryInstrExpr(ctx, unaryInstr);
				else if (auto gleaInstr = instr.As<GLeaInstruction>())
					PrintGleaInstrExpr(ctx, gleaInstr);
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
				else if (instr.As<MemberUpdateInstruction>())
					throw InvalidOperationException(L"member update instruction cannot appear as expression.");
			}

			void PrintInstr(CodeGenContext & ctx, ILInstruction & instr)
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
					else if (auto gleaInstr = instr.As<GLeaInstruction>())
						PrintGleaInstr(ctx, gleaInstr);
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
					else if (auto import = instr.As<ImportInstruction>())
						PrintImportInstr(ctx, import);
				}
			}

			void GenerateCode(CodeGenContext & context, CFGNode * code)
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
						context.Body << L"for (;bool(";
						PrintOp(context, forInstr->ConditionCode->GetLastInstruction(), true);
						context.Body << L"); ";
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
						context.Body << L"return ";
						PrintOp(context, ret->Operand.Ptr());
						context.Body << L";\n";
					}
					else if (instr.Is<BreakInstruction>())
					{
						context.Body << L"break;\n";
					}
					else if (instr.Is<ContinueInstruction>())
					{
						context.Body << L"continue;\n";
					}
					else
						PrintInstr(context, instr);
				}
			}
		public:
			virtual CompiledShaderSource GenerateShaderWorld(CompileResult & result, SymbolTable *, CompiledWorld * shaderWorld,
				Dictionary<String, ImportOperatorHandler *> & opHandlers,
				Dictionary<String, ExportOperatorHandler *> & exportHandlers) override
			{
				CompiledShaderSource rs;
				CodeGenContext context;
				context.Result = &result;
				context.GlobalHeader << L"#version 450\n#extension GL_ARB_bindless_texture: require\n#extension GL_NV_gpu_shader5 : require\n";
				if (useNVCommandList)
					context.GlobalHeader << L"#extension GL_NV_command_list: require\n";
				context.ImportOperatorHandlers = opHandlers;
				context.ExportOperatorHandlers = exportHandlers;
				StringBuilder prologBuilder, epilogBuilder;
				for (auto & inputBlock : shaderWorld->WorldInputs)
				{
					if (!inputBlock.Value.Block->UserWorlds.Contains(shaderWorld->WorldName))
						continue;
					String impOpName = inputBlock.Value.ImportOperator.Name.Content;
					ImportOperatorHandler * handler = nullptr;
					if (!opHandlers.TryGetValue(impOpName, handler))
						result.GetErrorWriter()->Error(40003, L"import operator handler for '" + impOpName
							+ L"' is not registered.", inputBlock.Value.ImportOperator.Position);
					else
					{
						StringBuilder inputDefSB;
						ImportOperatorContext opCtx(inputBlock.Value.ImportOperator.Arguments, backendArguments, shaderWorld, result,
							shaderWorld->Shader->Worlds[inputBlock.Value.ImportOperator.SourceWorld.Content].GetValue().Ptr());
						handler->GenerateInterfaceDefinition(inputDefSB, inputBlock.Value.Block, opCtx);
						handler->GeneratePreamble(prologBuilder, inputBlock.Value.Block, opCtx);
						handler->GenerateEpilogue(epilogBuilder, inputBlock.Value.Block, opCtx);
						rs.InputDeclarations[inputBlock.Key] = inputDefSB.ProduceString();
					}
				}
				ExportOperatorHandler * expHandler = nullptr;
				if (!exportHandlers.TryGetValue(shaderWorld->ExportOperator.Content, expHandler))
				{
					result.GetErrorWriter()->Error(40004, L"export operator handler for '" + shaderWorld->ExportOperator.Content
						+ L"' is not registered.", shaderWorld->ExportOperator.Position);
				}
				else
				{
					StringBuilder outputDefSB;
					expHandler->GenerateInterfaceDefinition(outputDefSB, shaderWorld->WorldOutput);
					expHandler->GeneratePreamble(prologBuilder, shaderWorld->WorldOutput);
					expHandler->GenerateEpilogue(epilogBuilder, shaderWorld->WorldOutput);
					rs.OutputDeclarations = outputDefSB.ProduceString();
				}
				currentWorld = shaderWorld;
				NamingCounter = 0;
				shaderWorld->Code->NameAllInstructions();
				GenerateCode(context, shaderWorld->Code.Ptr());
				rs.GlobalHeader = context.GlobalHeader.ProduceString();

				StringBuilder funcSB;
				for (auto funcName : shaderWorld->ReferencedFunctions)
				{
					for (auto &func : result.Program->Functions)
					{
						if (func->Name == funcName)
						{
							GenerateFunctionDeclaration(funcSB, func.Ptr());
							funcSB << L";\n";
						}
					}
				}
				for (auto funcName : shaderWorld->ReferencedFunctions)
				{
					for (auto &func : result.Program->Functions)
					{
						if (func->Name == funcName)
						{
							funcSB << GenerateFunction(func.Ptr());
						}
					}
				}
				rs.GlobalDefinitions = funcSB.ProduceString();
				rs.LocalDeclarations = prologBuilder.ProduceString() + context.Header.ProduceString();

				if (vertexOutputName.Length())
				{
					CompiledComponent ccomp;
					if (currentWorld->LocalComponents.TryGetValue(vertexOutputName, ccomp))
					{
						epilogBuilder << L"gl_Position = " << ccomp.CodeOperand->Name << L";\n";
					}
					else
					{
						result.GetErrorWriter()->Error(40005, L"cannot resolve '" + vertexOutputName
							+ L"' when generating code for world '" + currentWorld->WorldName + L"\'.", currentWorld->WorldDefPosition);
					}
				}

				for (auto & localComp : currentWorld->LocalComponents)
				{
					CodeGenContext nctx;
					PrintOp(nctx, localComp.Value.CodeOperand);
					rs.ComponentAccessNames[localComp.Key] = nctx.Body.ProduceString();
				}
				rs.MainCode = context.Body.ProduceString() + epilogBuilder.ProduceString();

				currentWorld = nullptr;
				return rs;
			}
			void GenerateFunctionDeclaration(StringBuilder & sbCode, CompiledFunction * function)
			{
				auto retType = function->ReturnType.Ptr();
				if (retType)
					PrintBaseType(sbCode, retType);
				else
					sbCode << L"void";
				sbCode << L" " << GetFuncOriginalName(function->Name) << L"(";
				int id = 0;
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
							PrintDef(sbCode, arg->Type.Ptr(), arg->Name);
							id++;
						}
					}
				}
				sbCode << L")";
			}
			String GenerateFunction(CompiledFunction * function)
			{
				StringBuilder sbCode;
				CodeGenContext ctx;
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
			EnumerableDictionary<String, String> backendArguments;
			virtual void SetParameters(EnumerableDictionary<String, String> & args) override
			{
				backendArguments = args;
				if (!args.TryGetValue(L"vertex", vertexOutputName))
					vertexOutputName = L"";
				useNVCommandList = args.ContainsKey(L"command_list");
			}
		};

		CodeGenBackend * CreateGLSLCodeGen()
		{
			return new GLSLCodeGen();
		}
	}
}