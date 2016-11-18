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
		class HLSLCodeGen : public CLikeCodeGen
		{
		protected:
			OutputStrategy * CreateStandardOutputStrategy(ILWorld * world, String layoutPrefix) override;
			OutputStrategy * CreatePackedBufferOutputStrategy(ILWorld * world) override;
			OutputStrategy * CreateArrayOutputStrategy(ILWorld * world, bool pIsPatch, int pArraySize, String arrayIndex) override;

			void PrintRasterPositionOutputWrite(CodeGenContext & ctx, ILOperand * operand) override
			{
				ctx.Body << L"stage_output.sv_position = ";
				PrintOp(ctx, operand);
				ctx.Body << L";\n";
			}

			void PrintMatrixMulInstrExpr(CodeGenContext & ctx, ILOperand* op0, ILOperand* op1) override
			{
				ctx.Body << L"mul(";
				PrintOp(ctx, op0);
				ctx.Body << L", ";
				PrintOp(ctx, op1);
				ctx.Body << L")";
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
				sb << componentName;
			}

			void PrintArrayBufferInputReference(StringBuilder& sb, String inputName, String componentName) override
			{
				sb << L"blk" << inputName << L".content";
			}

			void PrintPackedBufferInputReference(StringBuilder& sb, String inputName, String componentName) override
			{
				sb << L"blk" << inputName << L".content";
			}

			void PrintStandardInputReference(StringBuilder& sb, ILRecordType* /*recType*/, String inputName, String componentName) override
			{
				String declName = componentName;
				sb << L"stage_input." << declName;
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

			String RemapFuncNameForTarget(String name) override
			{
				// Currently, all types are internally named based on their GLSL equivalent, so
				// for HLSL output we go ahead and maintain a big table to remap the names.
				//
				// Note: for right now, this is just a linear array, with no particular sorting.
				// Eventually it should be turned into a hash table for performance, or at least
				// just be kept sorted so that we can use a binary search.
				//
				// Note 2: Well, actually, the Right Answer is for the type representation to
				// be better than just a string, so that we don't have to do this string->string map.
				static const struct {
					wchar_t const* glslName;
					wchar_t const* hlslName;
				} kNameRemaps[] =
				{
					{ L"vec2", L"float2" },
					{ L"vec3", L"float3" },
					{ L"vec4", L"float4" },

					{ L"ivec2", L"int2" },
					{ L"ivec3", L"int3" },
					{ L"ivec4", L"int4" },

					{ L"uvec2", L"uint2" },
					{ L"uvec3", L"uint3" },
					{ L"uvec4", L"uint4" },

					{ L"mat3", L"float3x3" },
					{ L"mat4", L"float4x4" },
				};

				for(auto remap : kNameRemaps)
				{
					if(wcscmp(name.Buffer(), remap.glslName) == 0)
					{
						return remap.hlslName;
					}
				}

				return name;
			}

			void PrintTextureCall(CodeGenContext & ctx, CallInstruction * instr)
			{
				// texture functions are defined based on HLSL, so this is trivial
				// internally, texObj.Sample(sampler_obj, uv, ..) is represented as Sample(texObj, sampler_obj, uv, ...)
				// so we need to lift first argument to the front
				PrintOp(ctx, instr->Arguments[0].Ptr(), true);
				ctx.Body << L"." << instr->Function;
				ctx.Body << L"(";
				for (int i = 1; i < instr->Arguments.Count(); i++)
				{
					PrintOp(ctx, instr->Arguments[i].Ptr());
					if (i < instr->Arguments.Count() - 1)
						ctx.Body << L", ";
				}
				ctx.Body << L")";
			}

			void PrintProjectInstrExpr(CodeGenContext & ctx, ProjectInstruction * proj) override
			{
				// project component out of record type. 
				PrintOp(ctx, proj->Operand.Ptr());
				ctx.Body << L"." << proj->ComponentName;
			}

			void PrintTypeName(StringBuilder& sb, ILType* type) override
			{
				// Currently, all types are internally named based on their GLSL equivalent, so
				// for HLSL output we go ahead and maintain a big table to remap the names.
				//
				// Note: for right now, this is just a linear array, with no particular sorting.
				// Eventually it should be turned into a hash table for performance, or at least
				// just be kept sorted so that we can use a binary search.
				//
				// Note 2: Well, actually, the Right Answer is for the type representation to
				// be better than just a string, so that we don't have to do this string->string map.
				static const struct {
					wchar_t const* glslName;
					wchar_t const* hlslName;
				} kNameRemaps[] =
				{
					{ L"vec2", L"float2" },
					{ L"vec3", L"float3" },
					{ L"vec4", L"float4" },

					{ L"ivec2", L"int2" },
					{ L"ivec3", L"int3" },
					{ L"ivec4", L"int4" },

					{ L"uvec2", L"uint2" },
					{ L"uvec3", L"uint3" },
					{ L"uvec4", L"uint4" },

					{ L"mat3", L"float3x3" },
					{ L"mat4", L"float4x4" },
				};

				String typeName = type->ToString();
				for(auto remap : kNameRemaps)
				{
					if(wcscmp(typeName.Buffer(), remap.glslName) == 0)
					{
						sb << remap.hlslName;
						return;
					}
				}

				// If we don't find the type in our map, then that either means we missed a case,
				// or this is a user-defined type. I don't see an obvious way to check which of
				// those cases we are in, so we will just fall back to outputting the "GLSL name" here.
				sb << type->ToString();
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

				sb.GlobalHeader << L"cbuffer " << input.Name;
				if (info.Binding != -1)
					sb.GlobalHeader << L" : register(b" << info.Binding << L")";
				sb.GlobalHeader << L"\n{\n";

				// We declare an inline struct inside the `cbuffer` to ensure that
				// the members have an appropriate prefix on their name.
				sb.GlobalHeader << L"struct {\n";

				int index = 0;
				for (auto & field : recType->Members)
				{
					if (field.Value.Type->IsTexture())
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
					return;
				}

				sb.GlobalHeader << L"} blk" << input.Name << L";\n";
				sb.GlobalHeader << L"};\n";

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

			void DeclareArrayBuffer(CodeGenContext & /*sb*/, const ILObjectDefinition & /*input*/, bool /*isVertexShader*/) override
			{

			}
			void DeclarePackedBuffer(CodeGenContext & /*sb*/, const ILObjectDefinition & /*input*/, bool /*isVertexShader*/) override
			{

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
						// TODO(tfoley): texture binding allocation needs to be per-stage in D3D11, but should be global for D3D12
						int slotIndex = sb.TextureBindingsAllocator++;

						sb.GlobalHeader << L"Texture2D " << field.Key;
						sb.GlobalHeader << L" : register(t" << slotIndex << L")";
						sb.GlobalHeader << L";\n";

						sb.GlobalHeader << L"SamplerState " << field.Key << "_sampler";
						sb.GlobalHeader << L" : register(s" << slotIndex << L")";
						sb.GlobalHeader << L";\n";
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
				extCompInfo[input.Name] = info;
				auto recType = ExtractRecordType(input.Type.Ptr());
				assert(recType);

				// In order to handle ordinary per-stage shader inputs, we need to
				// declare a `struct` type over all the fields.

				sb.GlobalHeader << L"struct T" << recType->TypeName << L"\n{\n";

				int index = 0;
				for (auto & field : recType->Members)
				{
					if (!isVertexShader && (input.Attributes.ContainsKey(L"Flat") || field.Value.Type->IsIntegral()))
						sb.GlobalHeader << L"noperspective ";

					String declName = field.Key;
					PrintDef(sb.GlobalHeader, field.Value.Type.Ptr(), declName);
					if (info.IsArray)
					{
						sb.GlobalHeader << L"[";
						if (info.ArrayLength)
							sb.GlobalHeader << String(info.ArrayLength);
						sb.GlobalHeader << L"]";
					}

					// We synthesize a dummy semantic for every component, just to make things easy
					// TODO(tfoley): This won't work in presence of `struct`-type fields
					sb.GlobalHeader << " : A" << index;

					sb.GlobalHeader << L";\n";

					index++;
				}

				sb.GlobalHeader << L"};\n";
			}

			void DeclarePatchInputRecord(CodeGenContext & sb, const ILObjectDefinition & input, bool isVertexShader) override
			{
				// In HLSL, both standard input/output and per-patch input/output are passed as ordinary `struct` types.
				DeclareStandardInputRecord(sb, input, isVertexShader);
			}


			StageSource GenerateSingleWorldShader(ILProgram * program, ILShader * shader, ILStage * stage) override
			{
				useBindlessTexture = stage->Attributes.ContainsKey(L"BindlessTexture");
				StageSource rs;
				CodeGenContext ctx;

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
				ILRecordType* stageInputType = nullptr;
				for (auto & input : world->Inputs)
				{
					DeclareInput(ctx, input, stage->StageType == L"VertexShader");

					// We need to detect the world that represents the ordinary stage input...
					// TODO(tfoley): It seems like this is logically part of the stage definition.
					auto info = ExtractExternComponentInfo(input);
					if(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StandardInput)
					{
						auto recType = ExtractRecordType(input.Type.Ptr());
						stageInputType = recType;
					}
				}
				if(!stageInputType)
				{
					errWriter->Error(99999, L"'" + stage->StageType + L"' doesn't appear to have any input world", stage->Position);
				}
		
				outputStrategy->DeclareOutput(ctx, stage);
				ctx.codeGen = this;
				world->Code->NameAllInstructions();
				GenerateCode(ctx, world->Code.Ptr());
				if (stage->StageType == L"VertexShader" || stage->StageType == L"DomainShader")
					GenerateVertexShaderEpilog(ctx, world.Ptr(), stage);

				StringBuilder sb;
				sb << ctx.GlobalHeader.ProduceString();

				sb << L"struct T" << world->OutputType->TypeName << "Ext\n{\n";
				sb << L"T" << world->OutputType->TypeName << " user;\n";
				if(stage->Attributes.TryGetValue(L"Position"))
				{
					sb << L"float4 sv_position : SV_Position;\n";
				}
				sb << L"};\n";

				sb << L"T" << world->OutputType->TypeName << L"Ext main(";
				sb << L"T" << stageInputType->TypeName << " stage_input";
				sb << ")\n{ \n";
				sb << "T" << world->OutputType->TypeName << "Ext stage_output;\n";
				sb << ctx.Header.ProduceString() << ctx.Body.ProduceString();
				sb << "return stage_output;\n";
				sb << L"}";
				rs.MainCode = sb.ProduceString();
				return rs;
			}

			StageSource GenerateHullShader(ILProgram * program, ILShader * shader, ILStage * stage) override
			{
				// TODO(tfoley): This is just copy-pasted from the GLSL case, and needs a lot of work

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

				//GenerateHeader(ctx.GlobalHeader, stage);

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

		class HLSLStandardOutputStrategy : public OutputStrategy
		{
		private:
			String declPrefix;
		public:
			HLSLStandardOutputStrategy(HLSLCodeGen * pCodeGen, ILWorld * world, String prefix)
				: OutputStrategy(pCodeGen, world), declPrefix(prefix)
			{}
			virtual void DeclareOutput(CodeGenContext & ctx, ILStage * stage) override
			{
				ctx.GlobalHeader << L"struct T" << world->OutputType->TypeName << L"\n{\n";
				int index = 0;
				for (auto & field : world->OutputType->Members)
				{
					if (declPrefix.Length())
						ctx.GlobalHeader << declPrefix << L" ";
					if (field.Value.Type->IsIntegral())
						ctx.GlobalHeader << L"noperspective ";
					String declName = field.Key;
					codeGen->PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), AddWorldNameSuffix(declName, world->OutputType->TypeName));

					// We synthesize a dummy semantic for every component, just to make things easy
					// TODO(tfoley): This won't work in presence of `struct`-type fields

					// Note(tfoley): The fragment shader outputs needs to use the `SV_Target` semantic
					// instead of a user-defined semantic. This is annoyingly non-orthogonal.
					if(stage->StageType == L"FragmentShader")
					{
						ctx.GlobalHeader << " : SV_Target" << index;
					}
					else
					{
						ctx.GlobalHeader << " : A" << index;
					}

					ctx.GlobalHeader << L";\n";

					index++;
				}
				ctx.GlobalHeader << L"};\n";
			}
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) override
			{
				ctx.Body << "stage_output.user." << AddWorldNameSuffix(instr->ComponentName, world->OutputType->TypeName) << L" = ";
				codeGen->PrintOp(ctx, instr->Operand.Ptr());
				ctx.Body << L";\n";
			}
		};

		class HLSLArrayOutputStrategy : public OutputStrategy
		{
		protected:
			bool isPatch = false;
			int arraySize = 0;
		public:
			String outputIndex;
			HLSLArrayOutputStrategy(HLSLCodeGen * pCodeGen, ILWorld * world, bool pIsPatch, int pArraySize, String pOutputIndex)
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

		class HLSLPackedBufferOutputStrategy : public OutputStrategy
		{
		public:
			HLSLPackedBufferOutputStrategy(HLSLCodeGen * pCodeGen, ILWorld * world)
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

		OutputStrategy * HLSLCodeGen::CreateStandardOutputStrategy(ILWorld * world, String layoutPrefix)
		{
			return new HLSLStandardOutputStrategy(this, world, layoutPrefix);
		}
		OutputStrategy * HLSLCodeGen::CreatePackedBufferOutputStrategy(ILWorld * world)
		{
			return new HLSLPackedBufferOutputStrategy(this, world);
		}
		OutputStrategy * HLSLCodeGen::CreateArrayOutputStrategy(ILWorld * world, bool pIsPatch, int pArraySize, String arrayIndex)
		{
			return new HLSLArrayOutputStrategy(this, world, pIsPatch, pArraySize, arrayIndex);
		}

		CodeGenBackend * CreateHLSLCodeGen()
		{
			return new HLSLCodeGen();
		}
	}
}