#include "CodeGenBackend.h"
#include "../CoreLib/Parser.h"
#include "Syntax.h"

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

		class GLSLCodeGen;

		class CodeGenContext
		{
		public:
			GLSLCodeGen * codeGen;
			HashSet<String> GeneratedDefinitions;
			Dictionary<String, String> SubstituteNames;
			Dictionary<ILOperand*, String> VarName;
			CompileResult * Result = nullptr;
			HashSet<String> UsedVarNames;
			int TextureBindingsAllocator = 0;
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


			String DefineVariable(ILOperand * op);
		};

		class ExternComponentCodeGenInfo
		{
		public:
			enum class DataStructureType
			{
				StandardInput, UniformBuffer, ArrayBuffer, PackedBuffer, StorageBuffer, Texture, Patch
			};
			enum class SystemVarType
			{
				None, TessCoord, InvocationId, ThreadId, FragCoord, PatchVertexCount, PrimitiveId
			};
			DataStructureType DataStructure = DataStructureType::StandardInput;
			RefPtr<ILType> Type;
			SystemVarType SystemVar = SystemVarType::None;
			bool IsArray = false;
			int ArrayLength = 0;
			int Binding = -1;
		};

		class OutputStrategy : public Object
		{
		protected:
			GLSLCodeGen * codeGen = nullptr;
			ILWorld * world = nullptr;
		public:
			OutputStrategy(GLSLCodeGen * pCodeGen, ILWorld * pWorld)
			{
				codeGen = pCodeGen;
				world = pWorld;
			}

			virtual void DeclareOutput(CodeGenContext & ctx, ILStage * stage) = 0;
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) = 0;
		};

		OutputStrategy * CreateStandardOutputStrategy(GLSLCodeGen * codeGen, ILWorld * world, String prefix);
		OutputStrategy * CreatePackedBufferOutputStrategy(GLSLCodeGen * codeGen, ILWorld * world);
		OutputStrategy * CreateArrayOutputStrategy(GLSLCodeGen * codeGen, ILWorld * world, bool pIsPatch, int pArraySize, String arrayIndex);


		class GLSLCodeGen : public CodeGenBackend
		{
		private:
			//ILWorld * currentWorld = nullptr;
			//ILRecordType * currentRecordType = nullptr;
			//bool exportWriteToPackedBuffer = false;
			RefPtr<OutputStrategy> outputStrategy;
			Dictionary<String, ExternComponentCodeGenInfo> extCompInfo;
			ImportInstruction * currentImportInstr = nullptr;
			bool useBindlessTexture = false;
			ErrorWriter * errWriter;
		public:
			void Error(int errId, String msg, CodePosition pos)
			{
				errWriter->Error(errId, msg, pos);
			}
			void PrintType(StringBuilder & sbCode, ILType* type)
			{
				if (dynamic_cast<ILRecordType*>(type))
					PrintType(sbCode, currentImportInstr->Type.Ptr());
				else
					sbCode << type->ToString();
			}

			void PrintDef(StringBuilder & sbCode, ILType* type, const String & name)
			{
				PrintType(sbCode, type);
				sbCode << L" ";
				sbCode << name;
				if (name.Length() == 0)
					throw InvalidProgramException(L"unnamed instruction.");
			}

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
					else if (type->IsBool())
						ctx.Body << ((c->IntValues[0] != 0) ? L"true" : L"false");
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
					auto genType = dynamic_cast<ILGenericType*>(op0->Type.Ptr());
					if (genType && genType->GenericTypeName == L"PackedBuffer")
					{
						// load record type from packed buffer
						String conversionFunction;
						int size = 0;
						if (instr->Type->ToString() == L"int")
						{
							conversionFunction = L"floatBitsToInt";
							size = 1;
						}
						else if (instr->Type->ToString() == L"ivec2")
						{
							conversionFunction = L"floatBitsToInt";
							size = 2;
						}
						else if (instr->Type->ToString() == L"ivec3")
						{
							conversionFunction = L"floatBitsToInt";
							size = 3;
						}
						else if (instr->Type->ToString() == L"ivec4")
						{
							conversionFunction = L"floatBitsToInt";
							size = 4;
						}
						else if (instr->Type->ToString() == L"uint")
						{
							conversionFunction = L"floatBitsToUint";
							size = 1;
						}
						else if (instr->Type->ToString() == L"uvec2")
						{
							conversionFunction = L"floatBitsToUint";
							size = 2;
						}
						else if (instr->Type->ToString() == L"uvec3")
						{
							conversionFunction = L"floatBitsToUint";
							size = 3;
						}
						else if (instr->Type->ToString() == L"uvec4")
						{
							conversionFunction = L"floatBitsToUint";
							size = 4;
						}
						else if (instr->Type->ToString() == L"float")
						{
							conversionFunction = L"";
							size = 1;
						}
						else if (instr->Type->ToString() == L"vec2")
						{
							conversionFunction = L"";
							size = 2;
						}
						else if (instr->Type->ToString() == L"vec3")
						{
							conversionFunction = L"";
							size = 3;
						}
						else if (instr->Type->ToString() == L"vec4")
						{
							conversionFunction = L"";
							size = 4;
						}
						else if (instr->Type->ToString() == L"mat3")
						{
							conversionFunction = L"";
							size = 9;
						}
						else if (instr->Type->ToString() == L"mat4")
						{
							conversionFunction = L"";
							size = 16;
						}
						else
						{
							errWriter->Error(50082, L"importing type '" + instr->Type->ToString() + L"' from PackedBuffer is not supported by the GLSL backend.",
								CodePosition());
						}
						ctx.Body << instr->Type->ToString() << L"(";
						auto recType = dynamic_cast<ILRecordType*>(genType->BaseType.Ptr());
						int recTypeSize = 0;
						EnumerableDictionary<String, int> memberOffsets;
						for (auto & member : recType->Members)
						{
							memberOffsets[member.Key] = recTypeSize;
							recTypeSize += member.Value.Type->GetVectorSize();
						}
						for (int i = 0; i < size; i++)
						{
							ctx.Body << conversionFunction << L"(";
							PrintOp(ctx, op0);
							ctx.Body << L"[(";
							PrintOp(ctx, op1);
							ctx.Body << L") * " << recTypeSize << L" + " << memberOffsets[currentImportInstr->ComponentName]() << L"])";
							if (i != size - 1)
								ctx.Body << L", ";
						}
						ctx.Body << L")";
					}
					else
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
						if (genType)
						{
							if (genType->GenericTypeName == L"Buffer" && dynamic_cast<ILRecordType*>(genType->BaseType.Ptr()))
								ctx.Body << L"." << currentImportInstr->ComponentName;
						}
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

			void PrintUnaryInstr(CodeGenContext & ctx, UnaryInstruction * instr)
			{
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName << L" = ";
				PrintUnaryInstrExpr(ctx, instr);
				ctx.Body << L";\n";
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
				if (instr.Is<LoadInputInstruction>())
					return true;
				if (instr.Is<SwizzleInstruction>())
					return true;
				if (auto arg = instr.As<FetchArgInstruction>())
				{
					if (arg->ArgId == 0)
						return false;
				}
				if (auto import = instr.As<ImportInstruction>())
				{
					if (!useBindlessTexture && import->Type->IsTexture() || import->Type.As<ILArrayType>())
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
					|| instr.Is<FetchArgInstruction>() ;
			}

			void PrintExportInstr(CodeGenContext &ctx, ExportInstruction * exportInstr)
			{
				outputStrategy->ProcessExportInstruction(ctx, exportInstr);
			}

			void PrintUpdateInstr(CodeGenContext & ctx, MemberUpdateInstruction * instr)
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
				auto varName = ctx.DefineVariable(instr);
				ctx.Body << varName << L" = ";
				PrintOp(ctx, instr->Operands[0].Ptr());
				ctx.Body << L";\n";
				genCode(varName, instr->Operands[0]->Type.Ptr(), instr->Operands[1].Ptr(), instr->Operands[2].Ptr());
			}

			void PrintSwizzleInstrExpr(CodeGenContext & ctx, SwizzleInstruction * swizzle)
			{
				PrintOp(ctx, swizzle->Operand.Ptr());
				ctx.Body << L"." << swizzle->SwizzleString;
			}

			void PrintImportInstr(CodeGenContext & ctx, ImportInstruction * importInstr)
			{
				currentImportInstr = importInstr;
				
				PrintDef(ctx.Header, importInstr->Type.Ptr(), importInstr->Name);
				ctx.Header << L";\n";
				GenerateCode(ctx, importInstr->ImportOperator.Ptr());
				
				currentImportInstr = nullptr;
			}

			void PrintImportInstrExpr(CodeGenContext & ctx, ImportInstruction * importInstr)
			{
				currentImportInstr = importInstr;
				PrintOp(ctx, importInstr->ImportOperator->GetLastInstruction()->As<ReturnInstruction>()->Operand.Ptr());
				currentImportInstr = nullptr;
			}

			void PrintInstrExpr(CodeGenContext & ctx, ILInstruction & instr)
			{
				if (auto binInstr = instr.As<BinaryInstruction>())
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

			void PrintLoadInputInstrExpr(CodeGenContext & ctx, LoadInputInstruction * instr)
			{
				PrintInputReference(ctx.Body, instr->InputName);
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
		public:
			virtual CompiledShaderSource GenerateShader(CompileResult & result, SymbolTable *, ILShader * shader, ErrorWriter * err) override
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

			void GenerateStructs(StringBuilder & sb, ILProgram * program)
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

			void GenerateHeader(StringBuilder & sb, ILStage * stage)
			{
				sb << L"#version 440\n";
				if (stage->Attributes.ContainsKey(L"BindlessTexture"))
					sb << L"#extension GL_ARB_bindless_texture: require\n#extension GL_NV_gpu_shader5 : require\n";
				if (stage->Attributes.ContainsKey(L"NV_CommandList"))
					sb << L"#extension GL_NV_command_list: require\n";
			}

			void GenerateReferencedFunctions(StringBuilder & sb, ILProgram * program, ArrayView<ILWorld*> worlds)
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

			ExternComponentCodeGenInfo ExtractExternComponentInfo(const ILObjectDefinition & input)
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
						else if (genType->GenericTypeName == L"ArrayBuffer")
							info.DataStructure = ExternComponentCodeGenInfo::DataStructureType::ArrayBuffer;
						else if (genType->GenericTypeName == L"StorageBuffer")
							info.DataStructure = ExternComponentCodeGenInfo::DataStructureType::StorageBuffer;
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

			void PrintInputReference(StringBuilder & sb, String input)
			{
				auto info = extCompInfo[input]();
				
				if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::UniformBuffer ||
					info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StorageBuffer)
				{
					if (!currentImportInstr->Type->IsTexture() || useBindlessTexture)
						sb << L"blk" << input << L"." << currentImportInstr->ComponentName;
					else
						sb << currentImportInstr->ComponentName;
				}
				else if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::ArrayBuffer)
				{
					sb << L"blk" << input << L".content";
				}
				else if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::PackedBuffer)
				{
					sb << L"blk" << input << L".content";
				}
				else if (auto recType = ExtractRecordType(info.Type.Ptr()))
				{
					sb << currentImportInstr->ComponentName;
					if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StandardInput ||
						info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::Patch)
						sb << L"_at" << recType->ToString();
				}
				else
				{
					if (info.SystemVar == ExternComponentCodeGenInfo::SystemVarType::FragCoord)
						sb << L"gl_FragCoord";
					else if (info.SystemVar == ExternComponentCodeGenInfo::SystemVarType::TessCoord)
						sb << L"gl_TessCoord";
					else if (info.SystemVar == ExternComponentCodeGenInfo::SystemVarType::InvocationId)
						sb << L"gl_InvocationID";
					else if (info.SystemVar == ExternComponentCodeGenInfo::SystemVarType::ThreadId)
						sb << L"gl_GlobalInvocationID.x";
					else if (info.SystemVar == ExternComponentCodeGenInfo::SystemVarType::PatchVertexCount)
						sb << L"gl_PatchVerticesIn";
					else if (info.SystemVar == ExternComponentCodeGenInfo::SystemVarType::PrimitiveId)
						sb << L"gl_PrimitiveID";
					else
						sb << input;
				}
			}

			void DeclareInput(CodeGenContext & sb, const ILObjectDefinition & input, bool isVertexShader)
			{
				auto info = ExtractExternComponentInfo(input);
				extCompInfo[input.Name] = info;
				auto recType = ExtractRecordType(input.Type.Ptr());
				if (recType)
				{
					int declarationStart = sb.GlobalHeader.Length();
					int itemsDeclaredInBlock = 0;
					if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::UniformBuffer)
					{
						sb.GlobalHeader << L"layout(std140";
						if (info.Binding != -1)
							sb.GlobalHeader << L", binding = " << info.Binding;
						sb.GlobalHeader << L") uniform " << input.Name << L"\n{\n";
					}
					else if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StorageBuffer)
					{
						sb.GlobalHeader << L"layout(std430";
						if (info.Binding != -1)
							sb.GlobalHeader << L", binding = " << info.Binding;
						sb.GlobalHeader << L") buffer " << input.Name << L"\n{\n";
					}
					else if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::ArrayBuffer)
					{
						sb.GlobalHeader << L"struct T" << input.Name << L"\n{\n";
					}
					
					if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::PackedBuffer)
					{
						sb.GlobalHeader << L"layout(std430";
						if (info.Binding != -1)
							sb.GlobalHeader << L", binding = " << info.Binding;
						sb.GlobalHeader << L") uniform " << input.Name << L"\n{\nfloat content[];\n} blk" << input.Name << L";\n";
					}
					else
					{
						int index = 0;
						for (auto & field : recType->Members)
						{
							if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::Texture)
							{
								if (field.Value.Type->IsFloat() || field.Value.Type->IsFloatVector() && !field.Value.Type->IsFloatMatrix())
								{
									sb.GlobalHeader << L"layout(binding = " << sb.TextureBindingsAllocator << L") uniform sampler2D " << field.Key << L";\n";
									sb.TextureBindingsAllocator++;
								}
								else
									errWriter->Error(51091, L"type '" + field.Value.Type->ToString() + L"' cannot be placed in a texture.",
										field.Value.Position);
							}
							else
							{
								if (!useBindlessTexture && info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::UniformBuffer &&
									field.Value.Type->IsTexture())
									continue;
								if (input.Attributes.ContainsKey(L"VertexInput"))
									sb.GlobalHeader << L"layout(location = " << index << L") ";
								if (!isVertexShader && (input.Attributes.ContainsKey(L"Flat") ||
									(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StandardInput &&
									 field.Value.Type->IsIntegral())))
									sb.GlobalHeader << L"flat ";
								if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StandardInput)
								{
									sb.GlobalHeader << L"in ";
								}
								else if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::Patch)
									sb.GlobalHeader << L"patch in ";
								String defPostFix;
								if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StandardInput ||
									info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::Patch)
									defPostFix = L"_at" + recType->ToString();
								PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), field.Key + defPostFix);
								itemsDeclaredInBlock++;
								if (info.IsArray)
								{
									sb.GlobalHeader << L"[";
									if (info.ArrayLength)
										sb.GlobalHeader << String(info.ArrayLength);
									sb.GlobalHeader << L"]";
								}
								sb.GlobalHeader << L";\n";
							}
							index++;
						}
					}

					auto removeEmptyBlock = [&]()
					{
						if (itemsDeclaredInBlock == 0)
							sb.GlobalHeader.Remove(declarationStart, sb.GlobalHeader.Length() - declarationStart);
					};
					if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::UniformBuffer ||
						info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StorageBuffer)
					{
						sb.GlobalHeader << L"} blk" << input.Name << L";\n";
						removeEmptyBlock();
					}
					else if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::ArrayBuffer)
					{
						sb.GlobalHeader << L"};\nlayout(std430";
						if (info.Binding != -1)
							sb.GlobalHeader << L", binding = " << info.Binding;
						sb.GlobalHeader  << ") buffer " << input.Name << L"\n{\nT" << input.Name << L"content[];\n} blk" << input.Name << L";\n";
					}
					if (!useBindlessTexture && info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::UniformBuffer)
					{
						for (auto & field : recType->Members)
						{
							if (field.Value.Type->IsTexture())
							{
								if (field.Value.Attributes.ContainsKey(L"Binding"))
									sb.GlobalHeader << L"layout(binding = " << field.Value.Attributes[L"Binding"]() << L") ";
								else
								{
									sb.GlobalHeader << L"layout(binding = " << sb.TextureBindingsAllocator << L") ";
									sb.TextureBindingsAllocator++;
								}
								sb.GlobalHeader << L"uniform ";
								PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), field.Key);
								sb.GlobalHeader << L";\n";
							}
						}
					}
				}
			}

			void GenerateVertexShaderEpilog(CodeGenContext & ctx, ILWorld * world, ILStage * stage)
			{
				StageAttribute positionVar;
				if (stage->Attributes.TryGetValue(L"Position", positionVar))
				{
					ILOperand * operand;
					if (world->Components.TryGetValue(positionVar.Value, operand))
					{
						if (operand->Type->IsFloatVector() && operand->Type->GetVectorSize() == 4)
							ctx.Body << L"gl_Position = " << operand->Name << L";\n";
						else
							errWriter->Error(50040, L"'" + positionVar.Value + L"': component used as 'Position' output must be of vec4 type.",
								positionVar.Position);
					}
					else
						errWriter->Error(50041, L"'" + positionVar.Value + L"': component not defined.",
							positionVar.Position);
				}
			}

			void GenerateDomainShaderProlog(CodeGenContext & ctx, ILStage * stage)
			{
				ctx.GlobalHeader << L"layout(";
				StageAttribute val;
				if (stage->Attributes.TryGetValue(L"Domain", val))
					ctx.GlobalHeader << ((val.Value == L"quads") ? L"quads" : L"triangles");
				else
					ctx.GlobalHeader << L"triangles";
				if (val.Value != L"triangles" && val.Value != L"quads")
					Error(50093, L"'Domain' should be either 'triangles' or 'quads'.", val.Position);
				if (stage->Attributes.TryGetValue(L"Winding", val))
				{
					if (val.Value == L"cw")
						ctx.GlobalHeader << L", cw";
					else
						ctx.GlobalHeader << L", ccw";
				}
				if (stage->Attributes.TryGetValue(L"EqualSpacing", val))
				{
					if (val.Value == L"1" || val.Value == L"true")
						ctx.GlobalHeader << L", equal_spacing";
				}
				ctx.GlobalHeader << L") in;\n";
			}

			StageSource GenerateSingleWorldShader(ILProgram * program, ILShader * shader, ILStage * stage)
			{
				useBindlessTexture = stage->Attributes.ContainsKey(L"BindlessTexture");
				StageSource rs;
				CodeGenContext ctx;
				GenerateHeader(ctx.GlobalHeader, stage);
				if (stage->StageType == L"DomainShader")
					GenerateDomainShaderProlog(ctx, stage);

				GenerateStructs(ctx.GlobalHeader, program);
				StageAttribute worldName;
				RefPtr<ILWorld> world = nullptr;
				if (stage->Attributes.TryGetValue(L"World", worldName))
				{
					if (!shader->Worlds.TryGetValue(worldName.Value, world))
						errWriter->Error(50022, L"world '" + worldName.Value + L"' is not defined.", worldName.Position);
				}
				else
					errWriter->Error(50023, L"'" + stage->StageType + L"' should provide 'World' attribute.", stage->Position);
				if (!world)
					return rs;
				GenerateReferencedFunctions(ctx.GlobalHeader, program, MakeArrayView(world.Ptr()));
				extCompInfo.Clear();
				for (auto & input : world->Inputs)
				{
					DeclareInput(ctx, input, stage->StageType == L"VertexShader");
				}
		
				outputStrategy->DeclareOutput(ctx, stage);
				ctx.codeGen = this;
				world->Code->NameAllInstructions();
				GenerateCode(ctx, world->Code.Ptr());
				if (stage->StageType == L"VertexShader" || stage->StageType == L"DomainShader")
					GenerateVertexShaderEpilog(ctx, world.Ptr(), stage);

				StringBuilder sb;
				sb << ctx.GlobalHeader.ProduceString();
				sb << L"void main()\n{\n";
				sb << ctx.Header.ProduceString() << ctx.Body.ProduceString();
				sb << L"}";
				rs.MainCode = sb.ProduceString();
				return rs;
			}

			StageSource GenerateVertexFragmentDomainShader(ILProgram * program, ILShader * shader, ILStage * stage)
			{
				RefPtr<ILWorld> world = nullptr;
				StageAttribute worldName;
				if (stage->Attributes.TryGetValue(L"World", worldName))
				{
					if (!shader->Worlds.TryGetValue(worldName.Value, world))
						errWriter->Error(50022, L"world '" + worldName.Value + L"' is not defined.", worldName.Position);
				}
				outputStrategy = CreateStandardOutputStrategy(this, world.Ptr(), L"");
				return GenerateSingleWorldShader(program, shader, stage);
			}

			StageSource GenerateComputeShader(ILProgram * program, ILShader * shader, ILStage * stage)
			{
				RefPtr<ILWorld> world = nullptr;
				StageAttribute worldName;
				if (stage->Attributes.TryGetValue(L"World", worldName))
				{
					if (!shader->Worlds.TryGetValue(worldName.Value, world))
						errWriter->Error(50022, L"world '" + worldName.Value + L"' is not defined.", worldName.Position);
				}
				outputStrategy = CreatePackedBufferOutputStrategy(this, world.Ptr());
				return GenerateSingleWorldShader(program, shader, stage);
			}

			StageSource GenerateHullShader(ILProgram * program, ILShader * shader, ILStage * stage)
			{
				useBindlessTexture = stage->Attributes.ContainsKey(L"BindlessTexture");

				StageSource rs;
				StageAttribute patchWorldName, controlPointWorldName, cornerPointWorldName, domain, innerLevel, outerLevel, numControlPoints;
				RefPtr<ILWorld> patchWorld, controlPointWorld, cornerPointWorld;
				if (!stage->Attributes.TryGetValue(L"PatchWorld", patchWorldName))
				{
					errWriter->Error(50052, L"'HullShader' requires attribute 'PatchWorld'.", stage->Position);
					return rs;
				}
				if (!shader->Worlds.TryGetValue(patchWorldName.Value, patchWorld))
					errWriter->Error(50022, L"world '" + patchWorldName.Value + L"' is not defined.", patchWorldName.Position);
				if (!stage->Attributes.TryGetValue(L"ControlPointWorld", controlPointWorldName))
				{
					errWriter->Error(50052, L"'HullShader' requires attribute 'ControlPointWorld'.", stage->Position); 
					return rs;
				}
				if (!shader->Worlds.TryGetValue(controlPointWorldName.Value, controlPointWorld))
					errWriter->Error(50022, L"world '" + controlPointWorldName.Value + L"' is not defined.", controlPointWorldName.Position);
				if (!stage->Attributes.TryGetValue(L"CornerPointWorld", cornerPointWorldName))
				{
					errWriter->Error(50052, L"'HullShader' requires attribute 'CornerPointWorld'.", stage->Position);
					return rs;
				}
				if (!shader->Worlds.TryGetValue(cornerPointWorldName.Value, cornerPointWorld))
					errWriter->Error(50022, L"world '" + cornerPointWorldName.Value + L"' is not defined.", cornerPointWorldName.Position);
				if (!stage->Attributes.TryGetValue(L"Domain", domain))
				{
					errWriter->Error(50052, L"'HullShader' requires attribute 'Domain'.", stage->Position);
					return rs;
				}
				if (domain.Value != L"triangles" && domain.Value != L"quads")
				{
					errWriter->Error(50053, L"'Domain' should be either 'triangles' or 'quads'.", domain.Position);
					return rs;
				}
				if (!stage->Attributes.TryGetValue(L"TessLevelOuter", outerLevel))
				{
					errWriter->Error(50052, L"'HullShader' requires attribute 'TessLevelOuter'.", stage->Position);
					return rs;
				}
				if (!stage->Attributes.TryGetValue(L"TessLevelInner", innerLevel))
				{
					errWriter->Error(50052, L"'HullShader' requires attribute 'TessLevelInner'.", stage->Position);
					return rs;
				}
				if (!stage->Attributes.TryGetValue(L"ControlPointCount", numControlPoints))
				{
					errWriter->Error(50052, L"'HullShader' requires attribute 'ControlPointCount'.", stage->Position);
					return rs;
				}
				CodeGenContext ctx;
				ctx.codeGen = this;
				List<ILWorld*> worlds;
				worlds.Add(patchWorld.Ptr());
				worlds.Add(controlPointWorld.Ptr());
				worlds.Add(cornerPointWorld.Ptr());
				GenerateHeader(ctx.GlobalHeader, stage);
				ctx.GlobalHeader << L"layout(vertices = " << numControlPoints.Value << L") out;\n";
				GenerateStructs(ctx.GlobalHeader, program);
				GenerateReferencedFunctions(ctx.GlobalHeader, program, worlds.GetArrayView());
				extCompInfo.Clear();

				HashSet<String> declaredInputs;

				patchWorld->Code->NameAllInstructions();
				outputStrategy = CreateStandardOutputStrategy(this, patchWorld.Ptr(), L"patch");
				for (auto & input : patchWorld->Inputs)
				{
					if (declaredInputs.Add(input.Name))
						DeclareInput(ctx, input, false);
				}
				outputStrategy->DeclareOutput(ctx, stage);
				GenerateCode(ctx, patchWorld->Code.Ptr());

				controlPointWorld->Code->NameAllInstructions();
				outputStrategy = CreateArrayOutputStrategy(this, controlPointWorld.Ptr(), false, 0, L"gl_InvocationID");
				for (auto & input : controlPointWorld->Inputs)
				{
					if (declaredInputs.Add(input.Name))
						DeclareInput(ctx, input, false);
				}
				outputStrategy->DeclareOutput(ctx, stage);
				GenerateCode(ctx, controlPointWorld->Code.Ptr());

				cornerPointWorld->Code->NameAllInstructions();
				outputStrategy = CreateArrayOutputStrategy(this, cornerPointWorld.Ptr(), true, (domain.Value == L"triangles" ? 3 : 4), L"sysLocalIterator");
				for (auto & input : cornerPointWorld->Inputs)
				{
					if (declaredInputs.Add(input.Name))
						DeclareInput(ctx, input, false);
				}
				outputStrategy->DeclareOutput(ctx, stage);
				ctx.Body << L"for (int sysLocalIterator = 0; sysLocalIterator < gl_PatchVerticesIn; sysLocalIterator++)\n{\n";
				GenerateCode(ctx, cornerPointWorld->Code.Ptr());
				auto debugStr = cornerPointWorld->Code->ToString();
				ctx.Body << L"}\n";

				// generate epilog
				bool found = false;
				for (auto & world : worlds)
				{
					ILOperand * operand;
					if (world->Components.TryGetValue(innerLevel.Value, operand))
					{
						for (int i = 0; i < 2; i++)
						{
							ctx.Body << L"gl_TessLevelInner[" << i << L"] = ";
							PrintOp(ctx, operand);
							ctx.Body << L"[" << i << L"];\n";
						}
						found = true;
						break;
					}
				}
				if (!found)
					errWriter->Error(50041, L"'" + innerLevel.Value + L"': component not defined.",
						innerLevel.Position);

				found = false;
				for (auto & world : worlds)
				{
					ILOperand * operand;
					if (world->Components.TryGetValue(outerLevel.Value, operand))
					{
						for (int i = 0; i < 4; i++)
						{
							ctx.Body << L"gl_TessLevelOuter[" << i << L"] = ";
							PrintOp(ctx, operand);
							ctx.Body << L"[" << i << L"];\n";
						}
						found = true;
						break;
					}

				}
				if (!found)
					errWriter->Error(50041, L"'" + outerLevel.Value + L"': component not defined.",
						outerLevel.Position);

				StringBuilder sb;
				sb << ctx.GlobalHeader.ProduceString();
				sb << L"void main()\n{\n" << ctx.Header.ProduceString() << ctx.Body.ProduceString() << L"}";
				rs.MainCode = sb.ProduceString();
				return rs;
			}

			void GenerateFunctionDeclaration(StringBuilder & sbCode, ILFunction * function)
			{
				function->Code->NameAllInstructions();
				auto retType = function->ReturnType.Ptr();
				if (retType)
					PrintType(sbCode, retType);
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
			String GenerateFunction(ILFunction * function)
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
		};


		class StandardOutputStrategy : public OutputStrategy
		{
		private:
			String declPrefix;
		public:
			StandardOutputStrategy(GLSLCodeGen * pCodeGen, ILWorld * world, String prefix)
				: OutputStrategy(pCodeGen, world), declPrefix(prefix)
			{}
			virtual void DeclareOutput(CodeGenContext & ctx, ILStage *) override
			{
				for (auto & field : world->OutputType->Members)
				{
					if (declPrefix.Length())
						ctx.GlobalHeader << declPrefix << L" ";
					if (field.Value.Type->IsIntegral())
						ctx.GlobalHeader << L"flat ";
					ctx.GlobalHeader << L"out ";
					codeGen->PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), field.Key + L"_at" + world->OutputType->TypeName);
					ctx.GlobalHeader << L";\n";
				}
			}
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) override
			{
				ctx.Body << instr->ComponentName << L"_at" << world->OutputType->TypeName << L" = ";
				codeGen->PrintOp(ctx, instr->Operand.Ptr());
				ctx.Body << L";\n";
			}
		};

		class ArrayOutputStrategy : public OutputStrategy
		{
		protected:
			bool isPatch = false;
			int arraySize = 0;
		public:
			String outputIndex;
			ArrayOutputStrategy(GLSLCodeGen * pCodeGen, ILWorld * world, bool pIsPatch, int pArraySize, String pOutputIndex)
				: OutputStrategy(pCodeGen, world)
			{
				isPatch = pIsPatch;
				arraySize = pArraySize;
				outputIndex = pOutputIndex;
			}
			virtual void DeclareOutput(CodeGenContext & ctx, ILStage *) override
			{
				for (auto & field : world->OutputType->Members)
				{
					if (isPatch)
						ctx.GlobalHeader << L"patch ";
					ctx.GlobalHeader << L"out ";
					codeGen->PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), field.Key + L"_at" + world->Name);
					ctx.GlobalHeader << L"[";
					if (arraySize != 0)
						ctx.GlobalHeader << arraySize;
					ctx.GlobalHeader<<L"]; \n";
				}
			}
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) override
			{
				ctx.Body << instr->ComponentName << L"_at" << world->Name << L"[" << outputIndex << L"] = ";
				codeGen->PrintOp(ctx, instr->Operand.Ptr());
				ctx.Body << L";\n";
			}
		};

		class PackedBufferOutputStrategy : public OutputStrategy
		{
		public:
			PackedBufferOutputStrategy(GLSLCodeGen * pCodeGen, ILWorld * world)
				: OutputStrategy(pCodeGen, world)
			{}
			virtual void DeclareOutput(CodeGenContext & ctx, ILStage *) override
			{
				for (auto & field : world->OutputType->Members)
				{
					ctx.GlobalHeader << L"out ";
					codeGen->PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), field.Key);
					ctx.GlobalHeader << L";\n";
				}
			}
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * exportInstr) override
			{
				String conversionFunction;
				int size = 0;
				String typeName = exportInstr->Type->ToString();
				if (typeName == L"int")
				{
					conversionFunction = L"intBitsToFloat";
					size = 1;
				}
				else if (typeName == L"ivec2")
				{
					conversionFunction = L"intBitsToFloat";
					size = 2;
				}
				else if (typeName == L"ivec3")
				{
					conversionFunction = L"intBitsToFloat";
					size = 3;
				}
				else if (typeName == L"ivec4")
				{
					conversionFunction = L"intBitsToFloat";
					size = 4;
				}
				else if (typeName == L"uint")
				{
					conversionFunction = L"uintBitsToFloat";
					size = 1;
				}
				else if (typeName == L"uvec2")
				{
					conversionFunction = L"uintBitsToFloat";
					size = 2;
				}
				else if (typeName == L"uvec3")
				{
					conversionFunction = L"uintBitsToFloat";
					size = 3;
				}
				else if (typeName == L"uvec4")
				{
					conversionFunction = L"uintBitsToFloat";
					size = 4;
				}
				else if (typeName == L"float")
				{
					conversionFunction = L"";
					size = 1;
				}
				else if (typeName == L"vec2")
				{
					conversionFunction = L"";
					size = 2;
				}
				else if (typeName == L"vec3")
				{
					conversionFunction = L"";
					size = 3;
				}
				else if (typeName == L"vec4")
				{
					conversionFunction = L"";
					size = 4;
				}
				else if (typeName == L"mat3")
				{
					conversionFunction = L"";
					size = 9;
				}
				else if (typeName == L"mat4")
				{
					conversionFunction = L"";
					size = 16;
				}
				else
				{
					codeGen->Error(50082, L"importing type '" + typeName + L"' from PackedBuffer is not supported by the GLSL backend.",
						CodePosition());
				}
				auto recType = world->OutputType.Ptr();
				int recTypeSize = 0;
				EnumerableDictionary<String, int> memberOffsets;
				for (auto & member : recType->Members)
				{
					memberOffsets[member.Key] = recTypeSize;
					recTypeSize += member.Value.Type->GetVectorSize();
				}
				for (int i = 0; i < size; i++)
				{
					ctx.Body << L"sysOutputBuffer.content[gl_InvocationId.x * " << recTypeSize << L" + " + memberOffsets[exportInstr->ComponentName]()
						<< L"] = " << conversionFunction << L"(";
					codeGen->PrintOp(ctx, exportInstr->Operand.Ptr());
					if (size <= 4)
						ctx.Body << L"[" << i << L"]";
					else
					{
						int width = size == 9 ? 3 : 4;
						ctx.Body << L"[" << i / width << L"][" << i % width << L"]";
					}
					ctx.Body << L");\n";
				}
			}
		};

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
					Header << L" = 0;";
				}
				Header << L";\n";
				VarName.Add(op, name);
				op->Name = name;
				return op->Name;
			}
		}

		OutputStrategy * CreateStandardOutputStrategy(GLSLCodeGen * codeGen, ILWorld * world, String layoutPrefix)
		{
			return new StandardOutputStrategy(codeGen, world, layoutPrefix);
		}
		OutputStrategy * CreatePackedBufferOutputStrategy(GLSLCodeGen * codeGen, ILWorld * world)
		{
			return new PackedBufferOutputStrategy(codeGen, world);
		}
		OutputStrategy * CreateArrayOutputStrategy(GLSLCodeGen * codeGen, ILWorld * world, bool pIsPatch, int pArraySize, String arrayIndex)
		{
			return new ArrayOutputStrategy(codeGen, world, pIsPatch, pArraySize, arrayIndex);
		}

		CodeGenBackend * CreateGLSLCodeGen()
		{
			return new GLSLCodeGen();
		}
	}
}