#include "CLikeCodeGen.h"
#include "../CoreLib/Tokenizer.h"
#include "Syntax.h"
#include "Naming.h"
#include "SamplerUsageAnalysis.h"
#include <cassert>

using namespace CoreLib::Basic;

namespace Spire
{
	namespace Compiler
	{
		class GLSLCodeGen : public CLikeCodeGen
		{
		private:
			bool useVulkanBinding = false;
			bool useSingleDescSet = false;
		protected:
			OutputStrategy * CreateStandardOutputStrategy(ILWorld * world, String layoutPrefix) override;
			OutputStrategy * CreatePackedBufferOutputStrategy(ILWorld * world) override;
			OutputStrategy * CreateArrayOutputStrategy(ILWorld * world, bool pIsPatch, int pArraySize, String arrayIndex) override;

			void PrintOp(CodeGenContext & ctx, ILOperand * op, bool forceExpression = false) override
			{
				if (!useVulkanBinding && op->Type->IsSamplerState())
				{
					// GLSL does not have sampler type, print 0 as placeholder
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

			const char * GetTextureType(ILType * textureType)
			{
				auto baseType = dynamic_cast<ILBasicType*>(textureType)->Type;
				const char * textureName = nullptr;
				switch (baseType)
				{
				case ILBaseType::Texture2D:
					textureName = "texture2D";
					break;
				case ILBaseType::Texture2DArray:
					textureName = "texture2DArray";
					break;
				case ILBaseType::Texture2DArrayShadow:
					textureName = "texture2DArray";
					break;
				case ILBaseType::TextureCube:
					textureName = "textureCube";
					break;
				case ILBaseType::TextureCubeShadow:
					textureName = "textureCube";
					break;
				case ILBaseType::Texture3D:
					textureName = "texture3D";
					break;
				default:
					throw NotImplementedException();
				}
				return textureName;
			}

			const char * GetSamplerType(ILType * textureType)
			{
				auto baseType = dynamic_cast<ILBasicType*>(textureType)->Type;
				const char * samplerName = nullptr;
				switch (baseType)
				{
				case ILBaseType::Texture2D:
					samplerName = "sampler2D";
					break;
				case ILBaseType::Texture2DArray:
					samplerName = "sampler2DArray";
					break;
				case ILBaseType::Texture2DArrayShadow:
					samplerName = "sampler2DArrayShadow";
					break;
				case ILBaseType::TextureCube:
					samplerName = "samplerCube";
					break;
				case ILBaseType::TextureCubeShadow:
					samplerName = "samplerCubeShadow";
					break;
				case ILBaseType::Texture3D:
					samplerName = "sampler3D";
					break;
				default:
					throw NotImplementedException();
				}
				return samplerName;
			}

			void PrintTypeName(StringBuilder& sb, ILType* type) override
			{
				// Currently, all types are internally named based on their GLSL equivalent, so
				// outputting a type for GLSL is trivial.

				// GLSL does not have sampler type, use int as placeholder
				if (type->IsSamplerState())
				{
					if (useVulkanBinding)
					{
						if (dynamic_cast<ILBasicType*>(type)->Type == ILBaseType::SamplerComparisonState)
							sb << "samplerShadow";
						else
							sb << "sampler";
					}
					else
						sb << "int";
				}
				else if (type->IsTexture())
				{
					if (useVulkanBinding)
						sb << GetTextureType(type);
					else
						sb << GetSamplerType(type);
				}
				else
					sb << type->ToString();
			}

			void PrintTextureCall(CodeGenContext & ctx, CallInstruction * instr)
			{
				auto printSamplerArgument = [&](ILOperand * texture, ILOperand * sampler)
				{
					if (useVulkanBinding)
					{
						ctx.Body << GetSamplerType(texture->Type.Ptr()) << "(";
						PrintOp(ctx, texture);
						ctx.Body << ", ";
						PrintOp(ctx, sampler);
						ctx.Body << ")";
					}
					else
					{
						PrintOp(ctx, texture);
					}
					ctx.Body << ", ";
				};
				if (instr->Function == "Sample")
				{
					if (instr->Arguments.Count() == 4)
						ctx.Body << "textureOffset";
					else
						ctx.Body << "texture";
					ctx.Body << "(";
					printSamplerArgument(instr->Arguments[0].Ptr(), instr->Arguments[1].Ptr());
					for (int i = 2; i < instr->Arguments.Count(); i++)
					{
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
					printSamplerArgument(instr->Arguments[0].Ptr(), instr->Arguments[1].Ptr());
					for (int i = 2; i < instr->Arguments.Count(); i++)
					{
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
						printSamplerArgument(instr->Arguments[0].Ptr(), instr->Arguments[1].Ptr());
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
						printSamplerArgument(instr->Arguments[0].Ptr(), instr->Arguments[1].Ptr());
						PrintOp(ctx, instr->Arguments[2].Ptr());
						ctx.Body << ", ";
						PrintOp(ctx, instr->Arguments[3].Ptr());
						ctx.Body << ")";
					}
				}
				else if (instr->Function == "SampleCmp")
				{
					if (instr->Arguments.Count() == 5)
						ctx.Body << "textureOffset(";
					else
						ctx.Body << "texture(";
					printSamplerArgument(instr->Arguments[0].Ptr(), instr->Arguments[1].Ptr());
					auto baseType = dynamic_cast<ILBasicType*>(instr->Arguments[0]->Type.Ptr());
					if (baseType)
					{
						if (baseType->Type == ILBaseType::Texture2DShadow)
							ctx.Body << "vec3(";
						else if (baseType->Type == ILBaseType::TextureCubeShadow || baseType->Type == ILBaseType::Texture2DArrayShadow)
							ctx.Body << "vec4(";
						PrintOp(ctx, instr->Arguments[2].Ptr());
						ctx.Body << ", ";
						PrintOp(ctx, instr->Arguments[3].Ptr());
						ctx.Body << ")";
						if (instr->Arguments.Count() == 5)
						{
							ctx.Body << ", ";
							PrintOp(ctx, instr->Arguments[4].Ptr());
						}
					}
					ctx.Body << ")";
				}
				else
					throw NotImplementedException("CodeGen for texture function '" + instr->Function + "' is not implemented.");
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
					if (!useVulkanBinding && field.Value.Type->IsSamplerState())
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
					if (!useVulkanBinding && field.Value.Type->IsSamplerState())
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
			virtual void PrintParameterReference(StringBuilder& sb, ILModuleParameterInstance * param) override
			{
				if (param->Type->GetBindableResourceType() == BindableResourceType::NonBindable)
				{
					auto bufferName = EscapeCodeName(param->Module->BindingName);
					sb << bufferName << "." << param->Name;
				}
				else
				{
					sb << EscapeCodeName(param->Module->BindingName + "_" + param->Name);
				}
			}
			void GenerateShaderParameterDefinition(CodeGenContext & ctx, ILShader * shader)
			{
				int oneDescBindingLoc = 0;
				for (auto module : shader->ModuleParamSets)
				{
					// generate uniform buffer declaration
					auto bufferName = EscapeCodeName(module.Value->BindingName);
					bool containsOrdinaryParams = false;
					for (auto param : module.Value->Parameters)
						if (param.Value->BufferOffset != -1)
						{
							containsOrdinaryParams = true;
							break;
						}
					if (containsOrdinaryParams)
					{
						if (useVulkanBinding)
						{
							if (!useSingleDescSet)
							{
								ctx.GlobalHeader << "layout(std140, set = " << module.Value->DescriptorSetId << ", binding = 0) ";
							}
							else
							{
								ctx.GlobalHeader << "layout(std140, set = 0, binding =" << oneDescBindingLoc << ") ";
								module.Value->UniformBufferLegacyBindingPoint = oneDescBindingLoc;
								oneDescBindingLoc++;
							}
						}
						else
							ctx.GlobalHeader << "layout(binding = " << module.Value->DescriptorSetId << ", std140) ";
						ctx.GlobalHeader << "uniform buf" << bufferName << "\n{\n";
						for (auto param : module.Value->Parameters)
						{
							if (param.Value->BufferOffset != -1)
							{
								PrintType(ctx.GlobalHeader, param.Value->Type.Ptr());
								ctx.GlobalHeader << " " << param.Value->Name << ";\n";
							}
						}
						ctx.GlobalHeader << "} " << bufferName << ";\n";
					}
					int slotId = containsOrdinaryParams ? 1 : 0;
					for (auto param : module.Value->Parameters)
					{
						auto bindableType = param.Value->Type->GetBindableResourceType();
						if (bindableType != BindableResourceType::NonBindable)
						{
							switch (bindableType)
							{
							case BindableResourceType::StorageBuffer:
							{
								auto genType = param.Value->Type.As<ILGenericType>();
								if (!genType)
									continue;
								String bufName = EscapeCodeName(module.Value->BindingName + "_" + param.Value->Name);
								if (useVulkanBinding)
								{
									if (!useSingleDescSet)
										ctx.GlobalHeader << "layout(std430, set = " << module.Value->DescriptorSetId << ", binding = " << slotId << ") ";
									else
									{
										ctx.GlobalHeader << "layout(std430, set = 0, binding = " << oneDescBindingLoc << ") ";
										param.Value->BindingPoints.Clear();
										param.Value->BindingPoints.Add(oneDescBindingLoc);
										oneDescBindingLoc++;
									}
								}
								else
									ctx.GlobalHeader << "layout(std430, binding = " << param.Value->BindingPoints.First() << ") ";
								ctx.GlobalHeader << "buffer buf" << bufName << "\n{\n";
								PrintType(ctx.GlobalHeader, genType->BaseType.Ptr());
								ctx.GlobalHeader << " " << bufName << "[];\n};\n";
								break;
							}
							case BindableResourceType::Texture:
							{
								if (useVulkanBinding)
								{
									if (!useSingleDescSet)
										ctx.GlobalHeader << "layout(set = " << module.Value->DescriptorSetId << ", binding = " << slotId << ")";
									else
									{
										ctx.GlobalHeader << "layout(set = 0, binding = " << oneDescBindingLoc << ")";
										param.Value->BindingPoints.Clear();
										param.Value->BindingPoints.Add(oneDescBindingLoc);
										oneDescBindingLoc++;
									}
								}
								else
									ctx.GlobalHeader << "layout(binding = " << param.Value->BindingPoints.First() << ")";
								ctx.GlobalHeader << " uniform ";
								PrintType(ctx.GlobalHeader, param.Value->Type.Ptr());
								ctx.GlobalHeader << " " << EscapeCodeName(module.Value->BindingName + "_" + param.Value->Name) << ";\n";
								break;
							}
							case BindableResourceType::Sampler:
							{
								if (useVulkanBinding)
								{
									if (!useSingleDescSet)
										ctx.GlobalHeader << "layout(set = " << module.Value->DescriptorSetId << ", binding = " << slotId << ")";
									else
									{
										ctx.GlobalHeader << "layout(set = 0, binding = " << oneDescBindingLoc << ")";
										param.Value->BindingPoints.Clear();
										param.Value->BindingPoints.Add(oneDescBindingLoc);
										oneDescBindingLoc++;
									}
									ctx.GlobalHeader << " uniform ";
									PrintType(ctx.GlobalHeader, param.Value->Type.Ptr());
									ctx.GlobalHeader << " " << EscapeCodeName(module.Value->BindingName + "_" + param.Value->Name) << ";\n";
								}
								break;
							}
							break;
							default:
								continue;
							}
							slotId++;
						}
					}
				}
			}

			virtual void GenerateShaderMetaData(ShaderMetaData & result, ILProgram* program, ILShader* shader, DiagnosticSink* err) override
			{
				EnumerableDictionary<ILModuleParameterInstance*, List<ILModuleParameterInstance*>> samplerTextures;
				for (auto & w : shader->Worlds)
				{
					if (w.Value->Code)
						AnalyzeSamplerUsage(samplerTextures, program, w.Value->Code.Ptr(), err);
				}
				if (!useVulkanBinding)
				{
					for (auto & sampler : samplerTextures)
					{
						sampler.Key->BindingPoints.Clear();
						for (auto & tex : sampler.Value)
							sampler.Key->BindingPoints.AddRange(tex->BindingPoints);
					}
				}
				CLikeCodeGen::GenerateShaderMetaData(result, program, shader, err);
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
				GenerateShaderParameterDefinition(ctx, shader);

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
				GenerateShaderParameterDefinition(ctx, shader);
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
		public:
			GLSLCodeGen(bool vulkanBinding, bool pUseSingleDescSet)
			{
				useVulkanBinding = vulkanBinding;
				useSingleDescSet = pUseSingleDescSet;
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
				int location = 0;
				for (auto & field : world->OutputType->Members)
				{
					if (field.Value.Attributes.ContainsKey("FragDepth"))
						continue;
					ctx.GlobalHeader << "layout(location = " << location << ") ";
					if (declPrefix.Length())
						ctx.GlobalHeader << declPrefix << " ";
					if (field.Value.Type->IsIntegral())
						ctx.GlobalHeader << "flat ";
					ctx.GlobalHeader << "out ";
					String declName = field.Key;
					codeGen->PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), AddWorldNameSuffix(declName, world->OutputType->TypeName));
					ctx.GlobalHeader << ";\n";
					location++;
				}
			}
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) override
			{
				if (world->OutputType->Members[instr->ComponentName]().Attributes.ContainsKey("FragDepth"))
					ctx.Body << "gl_FragDepth = ";
				else
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
			return new GLSLCodeGen(false, false);
		}
		CodeGenBackend * CreateGLSL_VulkanCodeGen()
		{
			return new GLSLCodeGen(true, false);
		}
		CodeGenBackend * CreateGLSL_VulkanOneDescCodeGen()
		{
			return new GLSLCodeGen(true, true);
		}
	}
}