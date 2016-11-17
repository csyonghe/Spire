#include "CLikeCodeGen.h"
#include "../CoreLib/Parser.h"
#include "Syntax.h"
#include "Naming.h"

#include <cassert>

using namespace CoreLib::Basic;

namespace Spire
{
	namespace Compiler
	{
		class GLSLCodeGen : public CLikeCodeGen
		{
		protected:
			OutputStrategy * CreateStandardOutputStrategy(ILWorld * world, String layoutPrefix) override;
			OutputStrategy * CreatePackedBufferOutputStrategy(ILWorld * world) override;
			OutputStrategy * CreateArrayOutputStrategy(ILWorld * world, bool pIsPatch, int pArraySize, String arrayIndex) override;

			void PrintOp(CodeGenContext & ctx, ILOperand * op, bool forceExpression = false) override
			{
				// GLSL does not have sampler type, print 0 as placeholder
				if (op->Type->IsSamplerState())
				{
					ctx.Body << L"0";
					return;
				}
				CLikeCodeGen::PrintOp(ctx, op, forceExpression);
			}

			void PrintRasterPositionOutputWrite(CodeGenContext & ctx, ILOperand * operand) override
			{
				ctx.Body << L"gl_Position = ";
				PrintOp(ctx, operand);
				ctx.Body << L";\n";
			}

			void PrintUniformBufferInputReference(StringBuilder& sb, String inputName, String componentName) override
			{
				if (!currentImportInstr->Type->IsTexture() || useBindlessTexture)
					sb << L"blk" << inputName << L"." << componentName;
				else
					sb << componentName;
			}

			void PrintStorageBufferInputReference(StringBuilder& sb, String inputName, String componentName) override
			{
				sb << L"blk" << inputName << L"." << componentName;
			}

			void PrintArrayBufferInputReference(StringBuilder& sb, String inputName, String componentName) override
			{
				sb << L"blk" << inputName << L".content";
			}

			void PrintPackedBufferInputReference(StringBuilder& sb, String inputName, String componentName) override
			{
				sb << L"blk" << inputName << L".content";
			}

			void PrintStandardInputReference(StringBuilder& sb, ILRecordType* recType, String inputName, String componentName) override
			{
				String declName = componentName;
				declName = AddWorldNameSuffix(declName, recType->ToString());
				sb << declName;
			}

			void PrintPatchInputReference(StringBuilder& sb, ILRecordType* recType, String inputName, String componentName) override
			{
				String declName = componentName;
				declName = AddWorldNameSuffix(declName, recType->ToString());
				sb << declName;
			}

			void PrintDefaultInputReference(StringBuilder& sb, ILRecordType* /*recType*/, String inputName, String componentName) override
			{
				String declName = componentName;
				sb << declName;
			}

			void PrintSystemVarReference(StringBuilder& sb, String inputName, ExternComponentCodeGenInfo::SystemVarType systemVar) override
			{
				switch(systemVar)
				{
				case ExternComponentCodeGenInfo::SystemVarType::FragCoord:
					sb << L"gl_FragCoord";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::TessCoord:
					sb << L"gl_TessCoord";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::InvocationId:
					sb << L"gl_InvocationID";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::ThreadId:
					sb << L"gl_GlobalInvocationID.x";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::PatchVertexCount:
					sb << L"gl_PatchVerticesIn";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::PrimitiveId:
					sb << L"gl_PrimitiveID";
					break;
				default:
					sb << inputName;
					break;
				}
			}

			void PrintProjectInstrExpr(CodeGenContext & ctx, ProjectInstruction * proj)
			{
				if (auto memberLoadInstr = dynamic_cast<MemberLoadInstruction*>(proj->Operand.Ptr()))
				{
					bool overrideBaseMemberLoad = false;
					auto genType = dynamic_cast<ILGenericType*>(memberLoadInstr->Operands[0]->Type.Ptr());
					if (genType && genType->GenericTypeName == L"PackedBuffer")
					{
						// load record type from packed buffer
						String conversionFunction;
						int size = 0;
						if (memberLoadInstr->Type->ToString() == L"int")
						{
							conversionFunction = L"floatBitsToInt";
							size = 1;
						}
						else if (memberLoadInstr->Type->ToString() == L"ivec2")
						{
							conversionFunction = L"floatBitsToInt";
							size = 2;
						}
						else if (memberLoadInstr->Type->ToString() == L"ivec3")
						{
							conversionFunction = L"floatBitsToInt";
							size = 3;
						}
						else if (memberLoadInstr->Type->ToString() == L"ivec4")
						{
							conversionFunction = L"floatBitsToInt";
							size = 4;
						}
						else if (memberLoadInstr->Type->ToString() == L"uint")
						{
							conversionFunction = L"floatBitsToUint";
							size = 1;
						}
						else if (memberLoadInstr->Type->ToString() == L"uvec2")
						{
							conversionFunction = L"floatBitsToUint";
							size = 2;
						}
						else if (memberLoadInstr->Type->ToString() == L"uvec3")
						{
							conversionFunction = L"floatBitsToUint";
							size = 3;
						}
						else if (memberLoadInstr->Type->ToString() == L"uvec4")
						{
							conversionFunction = L"floatBitsToUint";
							size = 4;
						}
						else if (memberLoadInstr->Type->ToString() == L"float")
						{
							conversionFunction = L"";
							size = 1;
						}
						else if (memberLoadInstr->Type->ToString() == L"vec2")
						{
							conversionFunction = L"";
							size = 2;
						}
						else if (memberLoadInstr->Type->ToString() == L"vec3")
						{
							conversionFunction = L"";
							size = 3;
						}
						else if (memberLoadInstr->Type->ToString() == L"vec4")
						{
							conversionFunction = L"";
							size = 4;
						}
						else if (memberLoadInstr->Type->ToString() == L"mat3")
						{
							conversionFunction = L"";
							size = 9;
						}
						else if (memberLoadInstr->Type->ToString() == L"mat4")
						{
							conversionFunction = L"";
							size = 16;
						}
						else
						{
							errWriter->Error(50082, L"importing type '" + memberLoadInstr->Type->ToString() + L"' from PackedBuffer is not supported by the GLSL backend.",
								CodePosition());
						}
						ctx.Body << memberLoadInstr->Type->ToString() << L"(";
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
							PrintOp(ctx, memberLoadInstr->Operands[0].Ptr());
							ctx.Body << L"[(";
							PrintOp(ctx, memberLoadInstr->Operands[1].Ptr());
							ctx.Body << L") * " << recTypeSize << L" + " << memberOffsets[proj->ComponentName]() << L"])";
							if (i != size - 1)
								ctx.Body << L", ";
						}
						ctx.Body << L")";
						overrideBaseMemberLoad = true;
					}
					if (!overrideBaseMemberLoad)
						PrintOp(ctx, memberLoadInstr, true);
					if (genType)
					{
						if ((genType->GenericTypeName == L"Buffer" ||
							genType->GenericTypeName == L"ArrayBuffer")
							&& dynamic_cast<ILRecordType*>(genType->BaseType.Ptr()))
							ctx.Body << L"." << proj->ComponentName;
					}
				}
				else
					PrintOp(ctx, proj->Operand.Ptr(), true);
			}

			void PrintTypeName(StringBuilder& sb, ILType* type) override
			{
				// Currently, all types are internally named based on their GLSL equivalent, so
				// outputting a type for GLSL is trivial.

				// GLSL does not have sampler type, use int as placeholder
				if (type->IsSamplerState())
					sb << L"int";
				else
					sb << type->ToString();
			}

			void PrintTextureCall(CodeGenContext & ctx, CallInstruction * instr)
			{
				if (instr->Function == L"Sample")
				{
					if (instr->Arguments.Count() == 4)
						ctx.Body << L"textureOffset";
					else
						ctx.Body << L"texture";
					ctx.Body << L"(";
					for (int i = 0; i < instr->Arguments.Count(); i++)
					{
						if (i == 1) continue; // skip sampler_state parameter
						PrintOp(ctx, instr->Arguments[i].Ptr());
						if (i < instr->Arguments.Count() - 1)
							ctx.Body << L", ";
					}
					ctx.Body << L")";
				}
				else if (instr->Function == L"SampleGrad")
				{
					if (instr->Arguments.Count() == 6)
						ctx.Body << L"textureGradOffset";
					else
						ctx.Body << L"textureGrad";
					ctx.Body << L"(";
					for (int i = 0; i < instr->Arguments.Count(); i++)
					{
						if (i == 1) continue; // skip sampler_state parameter
						PrintOp(ctx, instr->Arguments[i].Ptr());
						if (i < instr->Arguments.Count() - 1)
							ctx.Body << L", ";
					}
					ctx.Body << L")";
				}
				else if (instr->Function == L"SampleBias")
				{
					if (instr->Arguments.Count() == 5) // loc, bias, offset
					{
						ctx.Body << L"textureOffset(";
						PrintOp(ctx, instr->Arguments[0].Ptr());
						ctx.Body << L", ";
						PrintOp(ctx, instr->Arguments[2].Ptr());
						ctx.Body << L", ";
						PrintOp(ctx, instr->Arguments[4].Ptr());
						ctx.Body << L", ";
						PrintOp(ctx, instr->Arguments[3].Ptr());
						ctx.Body << L")";
					}
					else
					{
						ctx.Body << L"texture(";
						PrintOp(ctx, instr->Arguments[0].Ptr());
						ctx.Body << L", ";
						PrintOp(ctx, instr->Arguments[2].Ptr());
						ctx.Body << L", ";
						PrintOp(ctx, instr->Arguments[3].Ptr());
						ctx.Body << L")";
					}
				}
				else
					throw NotImplementedException(L"CodeGen for texture function '" + instr->Function + L"' is not implemented.");
			}

			void DeclareUniformBuffer(CodeGenContext & sb, const ILObjectDefinition & input, bool /*isVertexShader*/) override
			{
				auto info = ExtractExternComponentInfo(input);
				extCompInfo[input.Name] = info;
				auto recType = ExtractRecordType(input.Type.Ptr());
				assert(recType);
				assert(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::UniformBuffer);

				int declarationStart = sb.GlobalHeader.Length();
				int itemsDeclaredInBlock = 0;

				sb.GlobalHeader << L"layout(std140";
				if (info.Binding != -1)
					sb.GlobalHeader << L", binding = " << info.Binding;
				sb.GlobalHeader << L") uniform " << input.Name << L"\n{\n";

				int index = 0;
				for (auto & field : recType->Members)
				{
					if (!useBindlessTexture && field.Value.Type->IsTexture())
						continue;
					if (field.Value.Type->IsSamplerState())
						continue;
					String declName = field.Key;
					PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), declName);
					itemsDeclaredInBlock++;
					if (info.IsArray)
					{
						sb.GlobalHeader << L"[";
						if (info.ArrayLength)
							sb.GlobalHeader << String(info.ArrayLength);
						sb.GlobalHeader << L"]";
					}
					sb.GlobalHeader << L";\n";

					index++;
				}
				if (itemsDeclaredInBlock == 0)
				{
					sb.GlobalHeader.Remove(declarationStart, sb.GlobalHeader.Length() - declarationStart);
				}
				else
				{
					sb.GlobalHeader << L"} blk" << input.Name << L";\n";
				}
				if (!useBindlessTexture)
				{
					for (auto & field : recType->Members)
					{
						//if (field.Value.Type->IsSamplerState())
							//continue;
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

			void DeclareStorageBuffer(CodeGenContext & sb, const ILObjectDefinition & input, bool isVertexShader) override
			{
				auto info = ExtractExternComponentInfo(input);
				extCompInfo[input.Name] = info;
				auto recType = ExtractRecordType(input.Type.Ptr());
				assert(recType);
				assert(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StorageBuffer);

				int declarationStart = sb.GlobalHeader.Length();
				int itemsDeclaredInBlock = 0;

				sb.GlobalHeader << L"layout(std430";
				if (info.Binding != -1)
					sb.GlobalHeader << L", binding = " << info.Binding;
				sb.GlobalHeader << L") buffer " << input.Name << L"\n{\n";

				int index = 0;
				for (auto & field : recType->Members)
				{
					if (!useBindlessTexture && info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::UniformBuffer &&
						field.Value.Type->IsTexture())
						continue;
					if (field.Value.Type->IsSamplerState())
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
					String declName = field.Key;
					if (info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StandardInput ||
						info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::Patch)
						declName = AddWorldNameSuffix(declName, recType->ToString());
					PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), declName);
					itemsDeclaredInBlock++;
					if (info.IsArray)
					{
						sb.GlobalHeader << L"[";
						if (info.ArrayLength)
							sb.GlobalHeader << String(info.ArrayLength);
						sb.GlobalHeader << L"]";
					}
					sb.GlobalHeader << L";\n";

					index++;
				}
				if (itemsDeclaredInBlock == 0)
				{
					sb.GlobalHeader.Remove(declarationStart, sb.GlobalHeader.Length() - declarationStart);
					return;
				}

				sb.GlobalHeader << L"} blk" << input.Name << L";\n";
			}

			void DeclareArrayBuffer(CodeGenContext & sb, const ILObjectDefinition & input, bool isVertexShader) override
			{
				auto info = ExtractExternComponentInfo(input);
				extCompInfo[input.Name] = info;
				auto recType = ExtractRecordType(input.Type.Ptr());
				assert(recType);
				assert(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::ArrayBuffer);

				int itemsDeclaredInBlock = 0;
				sb.GlobalHeader << L"struct T" << input.Name << L"\n{\n";
					
				int index = 0;
				for (auto & field : recType->Members)
				{
					if (field.Value.Type->IsSamplerState())
						continue;
					if (input.Attributes.ContainsKey(L"VertexInput"))
						sb.GlobalHeader << L"layout(location = " << index << L") ";
					if (!isVertexShader && (input.Attributes.ContainsKey(L"Flat")))
						sb.GlobalHeader << L"flat ";
					String declName = field.Key;
					PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), declName);
					itemsDeclaredInBlock++;
					sb.GlobalHeader << L";\n";
					index++;
				}

				sb.GlobalHeader << L"};\nlayout(std430";
				if (info.Binding != -1)
					sb.GlobalHeader << L", binding = " << info.Binding;
				sb.GlobalHeader  << ") buffer " << input.Name << L"\n{\nT" << input.Name << L" content[];\n} blk" << input.Name << L";\n";
			}

			void DeclarePackedBuffer(CodeGenContext & sb, const ILObjectDefinition & input, bool /*isVertexShader*/) override
			{
				auto info = ExtractExternComponentInfo(input);
				extCompInfo[input.Name] = info;
				auto recType = ExtractRecordType(input.Type.Ptr());
				assert(recType);
				assert(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::PackedBuffer);

				sb.GlobalHeader << L"layout(std430";
				if (info.Binding != -1)
					sb.GlobalHeader << L", binding = " << info.Binding;
				sb.GlobalHeader << L") uniform " << input.Name << L"\n{\nfloat content[];\n} blk" << input.Name << L";\n";
			}

			void DeclareTextureInputRecord(CodeGenContext & sb, const ILObjectDefinition & input, bool /*isVertexShader*/) override
			{
				auto info = ExtractExternComponentInfo(input);
				auto recType = ExtractRecordType(input.Type.Ptr());
				assert(recType);
				assert(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::Texture);

				for(auto & field : recType->Members)
				{
					if(field.Value.Type->IsFloat() || field.Value.Type->IsFloatVector() && !field.Value.Type->IsFloatMatrix())
					{
						sb.GlobalHeader << L"layout(binding = " << sb.TextureBindingsAllocator << L") uniform sampler2D " << field.Key << L";\n";
						sb.TextureBindingsAllocator++;
					}
					else
					{
						errWriter->Error(51091, L"type '" + field.Value.Type->ToString() + L"' cannot be placed in a texture.",
							field.Value.Position);
					}
				}
			}

			void DeclareStandardInputRecord(CodeGenContext & sb, const ILObjectDefinition & input, bool isVertexShader) override
			{
				auto info = ExtractExternComponentInfo(input);
				auto recType = ExtractRecordType(input.Type.Ptr());
				assert(recType);
				assert(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StandardInput);

				int itemsDeclaredInBlock = 0;

				int index = 0;
				for (auto & field : recType->Members)
				{
					if (field.Value.Type->IsSamplerState())
						continue;
					if (input.Attributes.ContainsKey(L"VertexInput"))
						sb.GlobalHeader << L"layout(location = " << index << L") ";
					if (!isVertexShader && (input.Attributes.ContainsKey(L"Flat") || field.Value.Type->IsIntegral()))
						sb.GlobalHeader << L"flat ";
					sb.GlobalHeader << L"in ";

					String declName = field.Key;
					declName = AddWorldNameSuffix(declName, recType->ToString());

					PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), declName);
					itemsDeclaredInBlock++;
					if (info.IsArray)
					{
						sb.GlobalHeader << L"[";
						if (info.ArrayLength)
							sb.GlobalHeader << String(info.ArrayLength);
						sb.GlobalHeader << L"]";
					}
					sb.GlobalHeader << L";\n";

					index++;
				}
			}

			void DeclarePatchInputRecord(CodeGenContext & sb, const ILObjectDefinition & input, bool isVertexShader) override
			{
				auto info = ExtractExternComponentInfo(input);
				auto recType = ExtractRecordType(input.Type.Ptr());
				assert(recType);
				assert(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::Patch);


				int itemsDeclaredInBlock = 0;

				int index = 0;
				for (auto & field : recType->Members)
				{
					if (field.Value.Type->IsSamplerState())
						continue;
					if (!isVertexShader && (input.Attributes.ContainsKey(L"Flat")))
						sb.GlobalHeader << L"flat ";
					sb.GlobalHeader << L"patch in ";

					String declName = field.Key;
					declName = AddWorldNameSuffix(declName, recType->ToString());

					PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), declName);
					itemsDeclaredInBlock++;
					if (info.IsArray)
					{
						sb.GlobalHeader << L"[";
						if (info.ArrayLength)
							sb.GlobalHeader << String(info.ArrayLength);
						sb.GlobalHeader << L"]";
					}
					sb.GlobalHeader << L";\n";

					index++;
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

			StageSource GenerateSingleWorldShader(ILProgram * program, ILShader * shader, ILStage * stage) override
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

			StageSource GenerateHullShader(ILProgram * program, ILShader * shader, ILStage * stage) override
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
				outputStrategy = CreateStandardOutputStrategy(patchWorld.Ptr(), L"patch");
				for (auto & input : patchWorld->Inputs)
				{
					if (declaredInputs.Add(input.Name))
						DeclareInput(ctx, input, false);
				}
				outputStrategy->DeclareOutput(ctx, stage);
				GenerateCode(ctx, patchWorld->Code.Ptr());

				controlPointWorld->Code->NameAllInstructions();
				outputStrategy = CreateArrayOutputStrategy(controlPointWorld.Ptr(), false, 0, L"gl_InvocationID");
				for (auto & input : controlPointWorld->Inputs)
				{
					if (declaredInputs.Add(input.Name))
						DeclareInput(ctx, input, false);
				}
				outputStrategy->DeclareOutput(ctx, stage);
				GenerateCode(ctx, controlPointWorld->Code.Ptr());

				cornerPointWorld->Code->NameAllInstructions();
				outputStrategy = CreateArrayOutputStrategy(cornerPointWorld.Ptr(), true, (domain.Value == L"triangles" ? 3 : 4), L"sysLocalIterator");
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
					String declName = field.Key;
					codeGen->PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), AddWorldNameSuffix(declName, world->OutputType->TypeName));
					ctx.GlobalHeader << L";\n";
				}
			}
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) override
			{
				ctx.Body << AddWorldNameSuffix(instr->ComponentName, world->OutputType->TypeName) << L" = ";
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
					codeGen->PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), AddWorldNameSuffix(field.Key, world->Name));
					ctx.GlobalHeader << L"[";
					if (arraySize != 0)
						ctx.GlobalHeader << arraySize;
					ctx.GlobalHeader<<L"]; \n";
				}
			}
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) override
			{
				ctx.Body << AddWorldNameSuffix(instr->ComponentName, world->Name) << L"[" << outputIndex << L"] = ";
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

		OutputStrategy * GLSLCodeGen::CreateStandardOutputStrategy(ILWorld * world, String layoutPrefix)
		{
			return new StandardOutputStrategy(this, world, layoutPrefix);
		}
		OutputStrategy * GLSLCodeGen::CreatePackedBufferOutputStrategy(ILWorld * world)
		{
			return new PackedBufferOutputStrategy(this, world);
		}
		OutputStrategy * GLSLCodeGen::CreateArrayOutputStrategy(ILWorld * world, bool pIsPatch, int pArraySize, String arrayIndex)
		{
			return new ArrayOutputStrategy(this, world, pIsPatch, pArraySize, arrayIndex);
		}

		CodeGenBackend * CreateGLSLCodeGen()
		{
			return new GLSLCodeGen();
		}
	}
}