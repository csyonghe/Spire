#include "CLikeCodeGen.h"
#include "../CoreLib/Tokenizer.h"
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
					ctx.Body << "0";
					return;
				}
				CLikeCodeGen::PrintOp(ctx, op, forceExpression);
			}

			void PrintRasterPositionOutputWrite(CodeGenContext & ctx, ILOperand * operand) override
			{
				ctx.Body << "gl_Position = ";
				PrintOp(ctx, operand);
				ctx.Body << ";\n";
			}

			void PrintUniformBufferInputReference(StringBuilder& sb, String inputName, String componentName) override
			{
				if ((!currentImportInstr->Type->IsTexture() || useBindlessTexture) && !currentImportInstr->Type.As<ILGenericType>())
					sb << "blk" << inputName << "." << componentName;
				else
					sb << componentName;
			}

			void PrintStorageBufferInputReference(StringBuilder& sb, String inputName, String componentName) override
			{
				sb << "blk" << inputName << "." << componentName;
			}

			void PrintArrayBufferInputReference(StringBuilder& sb, String inputName, String componentName) override
			{
				sb << "blk" << inputName << ".content";
			}

			void PrintPackedBufferInputReference(StringBuilder& sb, String inputName, String componentName) override
			{
				sb << "blk" << inputName << ".content";
			}

			void PrintStandardInputReference(StringBuilder& sb, ILRecordType* recType, String inputName, String componentName) override
			{
				String declName = componentName;
				declName = AddWorldNameSuffix(declName, recType->ToString());
				sb << declName;
			}

			void PrintStandardArrayInputReference(StringBuilder& sb, ILRecordType* recType, String inputName, String componentName) override
			{
				PrintStandardInputReference(sb, recType, inputName, componentName);
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

			void PrintSystemVarReference(CodeGenContext & /*ctx*/, StringBuilder& sb, String inputName, ExternComponentCodeGenInfo::SystemVarType systemVar) override
			{
				switch(systemVar)
				{
				case ExternComponentCodeGenInfo::SystemVarType::FragCoord:
					sb << "gl_FragCoord";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::TessCoord:
					sb << "gl_TessCoord";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::InvocationId:
					sb << "gl_InvocationID";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::ThreadId:
					sb << "gl_GlobalInvocationID.x";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::PatchVertexCount:
					sb << "gl_PatchVerticesIn";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::PrimitiveId:
					sb << "gl_PrimitiveID";
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
					if (genType && genType->GenericTypeName == "PackedBuffer")
					{
						// load record type from packed buffer
						String conversionFunction;
						int size = 0;
						if (memberLoadInstr->Type->ToString() == "int")
						{
							conversionFunction = "floatBitsToInt";
							size = 1;
						}
						else if (memberLoadInstr->Type->ToString() == "ivec2")
						{
							conversionFunction = "floatBitsToInt";
							size = 2;
						}
						else if (memberLoadInstr->Type->ToString() == "ivec3")
						{
							conversionFunction = "floatBitsToInt";
							size = 3;
						}
						else if (memberLoadInstr->Type->ToString() == "ivec4")
						{
							conversionFunction = "floatBitsToInt";
							size = 4;
						}
						else if (memberLoadInstr->Type->ToString() == "uint")
						{
							conversionFunction = "floatBitsToUint";
							size = 1;
						}
						else if (memberLoadInstr->Type->ToString() == "uvec2")
						{
							conversionFunction = "floatBitsToUint";
							size = 2;
						}
						else if (memberLoadInstr->Type->ToString() == "uvec3")
						{
							conversionFunction = "floatBitsToUint";
							size = 3;
						}
						else if (memberLoadInstr->Type->ToString() == "uvec4")
						{
							conversionFunction = "floatBitsToUint";
							size = 4;
						}
						else if (memberLoadInstr->Type->ToString() == "float")
						{
							conversionFunction = "";
							size = 1;
						}
						else if (memberLoadInstr->Type->ToString() == "vec2")
						{
							conversionFunction = "";
							size = 2;
						}
						else if (memberLoadInstr->Type->ToString() == "vec3")
						{
							conversionFunction = "";
							size = 3;
						}
						else if (memberLoadInstr->Type->ToString() == "vec4")
						{
							conversionFunction = "";
							size = 4;
						}
						else if (memberLoadInstr->Type->ToString() == "mat3")
						{
							conversionFunction = "";
							size = 9;
						}
						else if (memberLoadInstr->Type->ToString() == "mat4")
						{
							conversionFunction = "";
							size = 16;
						}
						else
						{
							errWriter->diagnose(CodePosition(), Diagnostics::importingFromPackedBufferUnsupported, memberLoadInstr->Type);
						}
						ctx.Body << memberLoadInstr->Type->ToString() << "(";
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
							ctx.Body << conversionFunction << "(";
							PrintOp(ctx, memberLoadInstr->Operands[0].Ptr());
							ctx.Body << "[(";
							PrintOp(ctx, memberLoadInstr->Operands[1].Ptr());
							ctx.Body << ") * " << recTypeSize << " + " << memberOffsets[proj->ComponentName]() << "])";
							if (i != size - 1)
								ctx.Body << ", ";
						}
						ctx.Body << ")";
						overrideBaseMemberLoad = true;
					}
					if (!overrideBaseMemberLoad)
						PrintOp(ctx, memberLoadInstr, true);
					if (genType)
					{
						if ((genType->GenericTypeName == "StructuredBuffer" || genType->GenericTypeName == "RWStructuredBuffer")
							&& dynamic_cast<ILRecordType*>(genType->BaseType.Ptr()))
							ctx.Body << "." << proj->ComponentName;
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
					sb << "int";
				else
					sb << type->ToString();
			}

			void PrintTextureCall(CodeGenContext & ctx, CallInstruction * instr)
			{
				if (instr->Function == "Sample")
				{
					if (instr->Arguments.Count() == 4)
						ctx.Body << "textureOffset";
					else
						ctx.Body << "texture";
					ctx.Body << "(";
					for (int i = 0; i < instr->Arguments.Count(); i++)
					{
						if (i == 1) continue; // skip sampler_state parameter
						PrintOp(ctx, instr->Arguments[i].Ptr());
						if (i < instr->Arguments.Count() - 1)
							ctx.Body << ", ";
					}
					ctx.Body << ")";
				}
				else if (instr->Function == "SampleGrad")
				{
					if (instr->Arguments.Count() == 6)
						ctx.Body << "textureGradOffset";
					else
						ctx.Body << "textureGrad";
					ctx.Body << "(";
					for (int i = 0; i < instr->Arguments.Count(); i++)
					{
						if (i == 1) continue; // skip sampler_state parameter
						PrintOp(ctx, instr->Arguments[i].Ptr());
						if (i < instr->Arguments.Count() - 1)
							ctx.Body << ", ";
					}
					ctx.Body << ")";
				}
				else if (instr->Function == "SampleBias")
				{
					if (instr->Arguments.Count() == 5) // loc, bias, offset
					{
						ctx.Body << "textureOffset(";
						PrintOp(ctx, instr->Arguments[0].Ptr());
						ctx.Body << ", ";
						PrintOp(ctx, instr->Arguments[2].Ptr());
						ctx.Body << ", ";
						PrintOp(ctx, instr->Arguments[4].Ptr());
						ctx.Body << ", ";
						PrintOp(ctx, instr->Arguments[3].Ptr());
						ctx.Body << ")";
					}
					else
					{
						ctx.Body << "texture(";
						PrintOp(ctx, instr->Arguments[0].Ptr());
						ctx.Body << ", ";
						PrintOp(ctx, instr->Arguments[2].Ptr());
						ctx.Body << ", ";
						PrintOp(ctx, instr->Arguments[3].Ptr());
						ctx.Body << ")";
					}
				}
				else
					throw NotImplementedException("CodeGen for texture function '" + instr->Function + "' is not implemented.");
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

				sb.GlobalHeader << "layout(std140";
				if (info.Binding != -1)
					sb.GlobalHeader << ", binding = " << info.Binding;
				sb.GlobalHeader << ") uniform " << input.Name << "\n{\n";

				int index = 0;
				for (auto & field : recType->Members)
				{
					if (!useBindlessTexture && field.Value.Type->IsTexture())
						continue;
					if (field.Value.Type->IsSamplerState())
						continue;
					if (field.Value.Type.As<ILGenericType>()) // ArrayBuffer etc. goes to separate declaration outside the block
						continue;
					String declName = field.Key;
					PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), declName);
					itemsDeclaredInBlock++;
					if (info.IsArray)
					{
						sb.GlobalHeader << "[";
						if (info.ArrayLength)
							sb.GlobalHeader << String(info.ArrayLength);
						sb.GlobalHeader << "]";
					}
					sb.GlobalHeader << ";\n";

					index++;
				}
				if (itemsDeclaredInBlock == 0)
				{
					sb.GlobalHeader.Remove(declarationStart, sb.GlobalHeader.Length() - declarationStart);
				}
				else
				{
					sb.GlobalHeader << "} blk" << input.Name << ";\n";
				}
				if (!useBindlessTexture)
				{
					for (auto & field : recType->Members)
					{
						//if (field.Value.Type->IsSamplerState())
							//continue;
						if (field.Value.Type->IsTexture())
						{
							if (field.Value.Attributes.ContainsKey("Binding"))
								sb.GlobalHeader << "layout(binding = " << field.Value.Attributes["Binding"]() << ") ";
							else
							{
								sb.GlobalHeader << "layout(binding = " << sb.TextureBindingsAllocator << ") ";
								sb.TextureBindingsAllocator++;
							}
							sb.GlobalHeader << "uniform ";
							PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), field.Key);
							sb.GlobalHeader << ";\n";
						}
					}
				}
				for (auto & field : recType->Members)
				{
					auto genType = field.Value.Type.As<ILGenericType>();
					if (!genType)
						continue;
					if (genType->GenericTypeName == "StructuredBuffer" || genType->GenericTypeName == "RWStructuredBuffer")
					{
						if (field.Value.Attributes.ContainsKey("Binding"))
							sb.GlobalHeader << "layout(std430, binding = " << field.Value.Attributes["Binding"]() << ") ";
						else
							sb.GlobalHeader << "layout(std430) ";

						sb.GlobalHeader << "buffer buf" << field.Key << "\n{\n";
						PrintType(sb.GlobalHeader, genType->BaseType.Ptr());
						sb.GlobalHeader << " " << field.Key << "[];\n};\n";
					}
				}
			}

			void DeclareArrayBuffer(CodeGenContext & sb, const ILObjectDefinition & input, bool /*isVertexShader*/) override
			{
				auto info = ExtractExternComponentInfo(input);
				extCompInfo[input.Name] = info;
				auto recType = ExtractRecordType(input.Type.Ptr());
				assert(recType);
				assert(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::ArrayBuffer);

				int itemsDeclaredInBlock = 0;
				sb.GlobalHeader << "struct T" << input.Name << "\n{\n";
					
				int index = 0;
				for (auto & field : recType->Members)
				{
					if (field.Value.Type->IsSamplerState())
						continue;
					String declName = field.Key;
					PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), declName);
					itemsDeclaredInBlock++;
					sb.GlobalHeader << ";\n";
					index++;
				}

				sb.GlobalHeader << "};\nlayout(std430";
				if (info.Binding != -1)
					sb.GlobalHeader << ", binding = " << info.Binding;
				sb.GlobalHeader  << ") buffer " << input.Name << "\n{\nT" << input.Name << " content[];\n} blk" << input.Name << ";\n";
			}

			void DeclarePackedBuffer(CodeGenContext & sb, const ILObjectDefinition & input, bool /*isVertexShader*/) override
			{
				auto info = ExtractExternComponentInfo(input);
				extCompInfo[input.Name] = info;
				auto recType = ExtractRecordType(input.Type.Ptr());
				assert(recType);
				assert(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::PackedBuffer);

				sb.GlobalHeader << "layout(std430";
				if (info.Binding != -1)
					sb.GlobalHeader << ", binding = " << info.Binding;
				sb.GlobalHeader << ") uniform " << input.Name << "\n{\nfloat content[];\n} blk" << input.Name << ";\n";
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
						sb.GlobalHeader << "layout(binding = " << sb.TextureBindingsAllocator << ") uniform sampler2D " << field.Key << ";\n";
						sb.TextureBindingsAllocator++;
					}
					else
					{
						errWriter->diagnose(field.Value.Position, Diagnostics::typeCannotBePlacedInATexture, field.Value.Type);
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
					if (input.Attributes.ContainsKey("VertexInput"))
						sb.GlobalHeader << "layout(location = " << index << ") ";
					if (!isVertexShader && (input.Attributes.ContainsKey("Flat") || field.Value.Type->IsIntegral()))
						sb.GlobalHeader << "flat ";
					sb.GlobalHeader << "in ";

					String declName = field.Key;
					declName = AddWorldNameSuffix(declName, recType->ToString());

					PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), declName);
					itemsDeclaredInBlock++;
					if (info.IsArray)
					{
						sb.GlobalHeader << "[";
						if (info.ArrayLength)
							sb.GlobalHeader << String(info.ArrayLength);
						sb.GlobalHeader << "]";
					}
					sb.GlobalHeader << ";\n";

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
					if (!isVertexShader && (input.Attributes.ContainsKey("Flat")))
						sb.GlobalHeader << "flat ";
					sb.GlobalHeader << "patch in ";

					String declName = field.Key;
					declName = AddWorldNameSuffix(declName, recType->ToString());

					PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), declName);
					itemsDeclaredInBlock++;
					if (info.IsArray)
					{
						sb.GlobalHeader << "[";
						if (info.ArrayLength)
							sb.GlobalHeader << String(info.ArrayLength);
						sb.GlobalHeader << "]";
					}
					sb.GlobalHeader << ";\n";

					index++;
				}
			}

			void GenerateHeader(StringBuilder & sb, ILStage * stage)
			{
				sb << "#version 440\n";
				if (stage->Attributes.ContainsKey("BindlessTexture"))
					sb << "#extension GL_ARB_bindless_texture: require\n#extension GL_NV_gpu_shader5 : require\n";
				if (stage->Attributes.ContainsKey("NV_CommandList"))
					sb << "#extension GL_NV_command_list: require\n";
			}

			void GenerateDomainShaderProlog(CodeGenContext & ctx, ILStage * stage)
			{
				ctx.GlobalHeader << "layout(";
				StageAttribute val;
				if (stage->Attributes.TryGetValue("Domain", val))
					ctx.GlobalHeader << ((val.Value == "quads") ? "quads" : "triangles");
				else
					ctx.GlobalHeader << "triangles";
                if (val.Value != "triangles" && val.Value != "quads")
                    getSink()->diagnose(val.Position, Diagnostics::invalidTessellationDomain);
				if (stage->Attributes.TryGetValue("Winding", val))
				{
					if (val.Value == "cw")
						ctx.GlobalHeader << ", cw";
					else
						ctx.GlobalHeader << ", ccw";
				}
				if (stage->Attributes.TryGetValue("EqualSpacing", val))
				{
					if (val.Value == "1" || val.Value == "true")
						ctx.GlobalHeader << ", equal_spacing";
				}
				ctx.GlobalHeader << ") in;\n";
			}
			StageSource GenerateSingleWorldShader(ILProgram * program, ILShader * shader, ILStage * stage) override
			{
				useBindlessTexture = stage->Attributes.ContainsKey("BindlessTexture");
				StageSource rs;
				CodeGenContext ctx;
				GenerateHeader(ctx.GlobalHeader, stage);
				if (stage->StageType == "DomainShader")
					GenerateDomainShaderProlog(ctx, stage);

				GenerateStructs(ctx.GlobalHeader, program);
				StageAttribute worldName;
				RefPtr<ILWorld> world = nullptr;
				if (stage->Attributes.TryGetValue("World", worldName))
				{
					if (!shader->Worlds.TryGetValue(worldName.Value, world))
						errWriter->diagnose(worldName.Position, Diagnostics::worldIsNotDefined, worldName.Value);
				}
				else
					errWriter->diagnose(stage->Position, Diagnostics::stageShouldProvideWorldAttribute, stage->StageType);
				if (!world)
					return rs;
				GenerateReferencedFunctions(ctx.GlobalHeader, program, MakeArrayView(world.Ptr()));
				extCompInfo.Clear();
				for (auto & input : world->Inputs)
				{
					DeclareInput(ctx, input, stage->StageType == "VertexShader");
				}
		
				outputStrategy->DeclareOutput(ctx, stage);
				ctx.codeGen = this;
				world->Code->NameAllInstructions();
				GenerateCode(ctx, world->Code.Ptr());
				if (stage->StageType == "VertexShader" || stage->StageType == "DomainShader")
					GenerateVertexShaderEpilog(ctx, world.Ptr(), stage);

				StringBuilder sb;
				sb << ctx.GlobalHeader.ProduceString();
				sb << "void main()\n{\n";
				sb << ctx.Header.ProduceString() << ctx.Body.ProduceString();
				sb << "}";
				rs.MainCode = sb.ProduceString();
				return rs;
			}

			StageSource GenerateHullShader(ILProgram * program, ILShader * shader, ILStage * stage) override
			{
				useBindlessTexture = stage->Attributes.ContainsKey("BindlessTexture");

				StageSource rs;
				StageAttribute patchWorldName, controlPointWorldName, cornerPointWorldName, domain, innerLevel, outerLevel, numControlPoints;
				RefPtr<ILWorld> patchWorld, controlPointWorld, cornerPointWorld;
				if (!stage->Attributes.TryGetValue("PatchWorld", patchWorldName))
				{
					errWriter->diagnose(stage->Position, Diagnostics::hullShaderRequiresPatchWorld);
					return rs;
				}
				if (!shader->Worlds.TryGetValue(patchWorldName.Value, patchWorld))
					errWriter->diagnose(patchWorldName.Position, Diagnostics::worldIsNotDefined, patchWorldName.Value);
				if (!stage->Attributes.TryGetValue("ControlPointWorld", controlPointWorldName))
				{
					errWriter->diagnose(stage->Position, Diagnostics::hullShaderRequiresControlPointWorld); 
					return rs;
				}
				if (!shader->Worlds.TryGetValue(controlPointWorldName.Value, controlPointWorld))
					errWriter->diagnose(controlPointWorldName.Position, Diagnostics::worldIsNotDefined, controlPointWorldName.Value);
				if (!stage->Attributes.TryGetValue("CornerPointWorld", cornerPointWorldName))
				{
					errWriter->diagnose(stage->Position, Diagnostics::hullShaderRequiresCornerPointWorld);
					return rs;
				}
				if (!shader->Worlds.TryGetValue(cornerPointWorldName.Value, cornerPointWorld))
					errWriter->diagnose(cornerPointWorldName.Position, Diagnostics::worldIsNotDefined, cornerPointWorldName.Value);
				if (!stage->Attributes.TryGetValue("Domain", domain))
				{
					errWriter->diagnose(stage->Position, Diagnostics::hullShaderRequiresDomain);
					return rs;
				}
				if (domain.Value != "triangles" && domain.Value != "quads")
				{
					errWriter->diagnose(domain.Position, Diagnostics::invalidTessellationDomain);
					return rs;
				}
				if (!stage->Attributes.TryGetValue("TessLevelOuter", outerLevel))
				{
					errWriter->diagnose(stage->Position, Diagnostics::hullShaderRequiresTessLevelOuter);
					return rs;
				}
				if (!stage->Attributes.TryGetValue("TessLevelInner", innerLevel))
				{
					errWriter->diagnose(stage->Position, Diagnostics::hullShaderRequiresTessLevelInner);
					return rs;
				}
				if (!stage->Attributes.TryGetValue("ControlPointCount", numControlPoints))
				{
					errWriter->diagnose(stage->Position, Diagnostics::hullShaderRequiresControlPointCount);
					return rs;
				}
				CodeGenContext ctx;
				ctx.codeGen = this;
				List<ILWorld*> worlds;
				worlds.Add(patchWorld.Ptr());
				worlds.Add(controlPointWorld.Ptr());
				worlds.Add(cornerPointWorld.Ptr());
				GenerateHeader(ctx.GlobalHeader, stage);
				ctx.GlobalHeader << "layout(vertices = " << numControlPoints.Value << ") out;\n";
				GenerateStructs(ctx.GlobalHeader, program);
				GenerateReferencedFunctions(ctx.GlobalHeader, program, worlds.GetArrayView());
				extCompInfo.Clear();

				HashSet<String> declaredInputs;

				patchWorld->Code->NameAllInstructions();
				outputStrategy = CreateStandardOutputStrategy(patchWorld.Ptr(), "patch");
				for (auto & input : patchWorld->Inputs)
				{
					if (declaredInputs.Add(input.Name))
						DeclareInput(ctx, input, false);
				}
				outputStrategy->DeclareOutput(ctx, stage);
				GenerateCode(ctx, patchWorld->Code.Ptr());

				controlPointWorld->Code->NameAllInstructions();
				outputStrategy = CreateArrayOutputStrategy(controlPointWorld.Ptr(), false, 0, "gl_InvocationID");
				for (auto & input : controlPointWorld->Inputs)
				{
					if (declaredInputs.Add(input.Name))
						DeclareInput(ctx, input, false);
				}
				outputStrategy->DeclareOutput(ctx, stage);
				GenerateCode(ctx, controlPointWorld->Code.Ptr());

				cornerPointWorld->Code->NameAllInstructions();
				outputStrategy = CreateArrayOutputStrategy(cornerPointWorld.Ptr(), true, (domain.Value == "triangles" ? 3 : 4), "sysLocalIterator");
				for (auto & input : cornerPointWorld->Inputs)
				{
					if (declaredInputs.Add(input.Name))
						DeclareInput(ctx, input, false);
				}
				outputStrategy->DeclareOutput(ctx, stage);
				ctx.Body << "for (int sysLocalIterator = 0; sysLocalIterator < gl_PatchVerticesIn; sysLocalIterator++)\n{\n";
				GenerateCode(ctx, cornerPointWorld->Code.Ptr());
				auto debugStr = cornerPointWorld->Code->ToString();
				ctx.Body << "}\n";

				// generate epilog
				bool found = false;
				for (auto & world : worlds)
				{
					ILOperand * operand;
					if (world->Components.TryGetValue(innerLevel.Value, operand))
					{
						for (int i = 0; i < 2; i++)
						{
							ctx.Body << "gl_TessLevelInner[" << i << "] = ";
							PrintOp(ctx, operand);
							ctx.Body << "[" << i << "];\n";
						}
						found = true;
						break;
					}
				}
				if (!found)
					errWriter->diagnose(innerLevel.Position, Diagnostics::componentNotDefined, innerLevel.Value);

				found = false;
				for (auto & world : worlds)
				{
					ILOperand * operand;
					if (world->Components.TryGetValue(outerLevel.Value, operand))
					{
						for (int i = 0; i < 4; i++)
						{
							ctx.Body << "gl_TessLevelOuter[" << i << "] = ";
							PrintOp(ctx, operand);
							ctx.Body << "[" << i << "];\n";
						}
						found = true;
						break;
					}

				}
				if (!found)
					errWriter->diagnose(outerLevel.Position, Diagnostics::componentNotDefined, outerLevel.Value);

				StringBuilder sb;
				sb << ctx.GlobalHeader.ProduceString();
				sb << "void main()\n{\n" << ctx.Header.ProduceString() << ctx.Body.ProduceString() << "}";
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
						ctx.GlobalHeader << declPrefix << " ";
					if (field.Value.Type->IsIntegral())
						ctx.GlobalHeader << "flat ";
					ctx.GlobalHeader << "out ";
					String declName = field.Key;
					codeGen->PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), AddWorldNameSuffix(declName, world->OutputType->TypeName));
					ctx.GlobalHeader << ";\n";
				}
			}
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) override
			{
				ctx.Body << AddWorldNameSuffix(instr->ComponentName, world->OutputType->TypeName) << " = ";
				codeGen->PrintOp(ctx, instr->Operand.Ptr());
				ctx.Body << ";\n";
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
						ctx.GlobalHeader << "patch ";
					ctx.GlobalHeader << "out ";
					codeGen->PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), AddWorldNameSuffix(field.Key, world->Name));
					ctx.GlobalHeader << "[";
					if (arraySize != 0)
						ctx.GlobalHeader << arraySize;
					ctx.GlobalHeader<<"]; \n";
				}
			}
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) override
			{
				ctx.Body << AddWorldNameSuffix(instr->ComponentName, world->Name) << "[" << outputIndex << "] = ";
				codeGen->PrintOp(ctx, instr->Operand.Ptr());
				ctx.Body << ";\n";
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
					ctx.GlobalHeader << "out ";
					codeGen->PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), field.Key);
					ctx.GlobalHeader << ";\n";
				}
			}
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * exportInstr) override
			{
				String conversionFunction;
				int size = 0;
				String typeName = exportInstr->Type->ToString();
				if (typeName == "int")
				{
					conversionFunction = "intBitsToFloat";
					size = 1;
				}
				else if (typeName == "ivec2")
				{
					conversionFunction = "intBitsToFloat";
					size = 2;
				}
				else if (typeName == "ivec3")
				{
					conversionFunction = "intBitsToFloat";
					size = 3;
				}
				else if (typeName == "ivec4")
				{
					conversionFunction = "intBitsToFloat";
					size = 4;
				}
				else if (typeName == "uint")
				{
					conversionFunction = "uintBitsToFloat";
					size = 1;
				}
				else if (typeName == "uvec2")
				{
					conversionFunction = "uintBitsToFloat";
					size = 2;
				}
				else if (typeName == "uvec3")
				{
					conversionFunction = "uintBitsToFloat";
					size = 3;
				}
				else if (typeName == "uvec4")
				{
					conversionFunction = "uintBitsToFloat";
					size = 4;
				}
				else if (typeName == "float")
				{
					conversionFunction = "";
					size = 1;
				}
				else if (typeName == "vec2")
				{
					conversionFunction = "";
					size = 2;
				}
				else if (typeName == "vec3")
				{
					conversionFunction = "";
					size = 3;
				}
				else if (typeName == "vec4")
				{
					conversionFunction = "";
					size = 4;
				}
				else if (typeName == "mat3")
				{
					conversionFunction = "";
					size = 9;
				}
				else if (typeName == "mat4")
				{
					conversionFunction = "";
					size = 16;
				}
				else
				{
                    codeGen->getSink()->diagnose(CodePosition(), Diagnostics::importingFromPackedBufferUnsupported, typeName);
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
					ctx.Body << "sysOutputBuffer.content[gl_InvocationId.x * " << recTypeSize << " + " + memberOffsets[exportInstr->ComponentName]()
						<< "] = " << conversionFunction << "(";
					codeGen->PrintOp(ctx, exportInstr->Operand.Ptr());
					if (size <= 4)
						ctx.Body << "[" << i << "]";
					else
					{
						int width = size == 9 ? 3 : 4;
						ctx.Body << "[" << i / width << "][" << i % width << "]";
					}
					ctx.Body << ");\n";
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