#include "CLikeCodeGen.h"
#include "../CoreLib/Tokenizer.h"
#include "Syntax.h"
#include "Naming.h"
#include "TypeLayout.h"
#include <cassert>

using namespace CoreLib::Basic;

namespace Spire
{
	namespace Compiler
	{
		class HLSLCodeGen : public CLikeCodeGen
		{
		private:
			bool useD3D12Registers = false;
		protected:
			OutputStrategy * CreateStandardOutputStrategy(ILWorld * world, String layoutPrefix) override;
			OutputStrategy * CreatePackedBufferOutputStrategy(ILWorld * world) override;
			OutputStrategy * CreateArrayOutputStrategy(ILWorld * world, bool pIsPatch, int pArraySize, String arrayIndex) override;

            LayoutRule GetDefaultLayoutRule() override
            {
                return LayoutRule::HLSL;
            }

			void PrintRasterPositionOutputWrite(CodeGenContext & ctx, ILOperand * operand) override
			{
				ctx.Body << "stage_output.sv_position = ";
				PrintOp(ctx, operand);
				ctx.Body << ";\n";
			}

			void PrintMatrixMulInstrExpr(CodeGenContext & ctx, ILOperand* op0, ILOperand* op1) override
			{
				// The matrix-vector, vector-matrix, and matrix-matrix product
				// operation is written with the `*` operator in GLSL, but
				// is handled by the built-in function `mul()` in HLSL.
				//
				// This function is called by the code generator for that op
				// and allows us to print it appropriately.

				ctx.Body << "mul(";
				PrintOp(ctx, op1);
				ctx.Body << ", ";
				PrintOp(ctx, op0);
				ctx.Body << ")";
			}
			
			void PrintStandardInputReference(StringBuilder& sb, ILRecordType* /*recType*/, String inputName, String componentName) override
			{
				sb << "stage_input/*standard*/";
			}

			void PrintStandardArrayInputReference(StringBuilder& sb, ILRecordType* /*recType*/, String inputName, String componentName) override
			{
				sb << "stage_input/*array*/";
			}

			void PrintPatchInputReference(StringBuilder& sb, ILRecordType* /*recType*/, String inputName, String componentName) override
			{
				sb << "stage_input_patch/*patch*/." << inputName;
			}

			void PrintDefaultInputReference(StringBuilder& sb, ILRecordType* /*recType*/, String inputName, String componentName) override
			{
				String declName = componentName;
				sb << declName;
			}

			void PrintSystemVarReference(CodeGenContext & ctx, StringBuilder& sb, String inputName, ExternComponentCodeGenInfo::SystemVarType systemVar) override
			{
				ctx.UsedSystemInputs.Add(systemVar);
				switch(systemVar)
				{
				case ExternComponentCodeGenInfo::SystemVarType::FragCoord:
					sb << "sv_FragPosition";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::TessCoord:
					sb << "sv_DomainLocation";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::InvocationId:
					sb << "sv_ThreadID";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::ThreadId:
					sb << "sv_GlobalThreadID.x";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::PatchVertexCount:
					// TODO(tfoley): there is no equivalent of this in HLSL
					sb << "sv_InputControlPointCount";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::PrimitiveId:
					sb << "sv_PrimitiveID";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::InstanceId:
					sb << "sv_InstanceID";
					break;
				default:
					sb << inputName;
					break;
				}
			}

			void PrintCallInstrExprForTarget(CodeGenContext & ctx, CallInstruction * instr, String const& name) override
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
					char const* glslName;
					char const* hlslName;
				} kNameRemaps[] =
				{
					{ "vec2", "*float2" },
					{ "vec3", "*float3" },
					{ "vec4", "*float4" },

					{ "ivec2", "*int2" },
					{ "ivec3", "*int3" },
					{ "ivec4", "*int4" },

					{ "uvec2", "*uint2" },
					{ "uvec3", "*uint3" },
					{ "uvec4", "*uint4" },

					{ "mat3", "float3x3" },
					{ "mat4", "float4x4" },

					{ "sampler2D", "Texture2D" },
					{ "sampler2DArray", "Texture2DArray" },
					{ "samplerCube", "TextureCube" },
					{ "sampler2DShadow", "Texture2D" },
					{ "sampler2DArrayShadow", "Texture2DArray" },
					{ "samplerCubeShadow", "TextureCube" },
				};

				for(auto remap : kNameRemaps)
				{
					if(strcmp(name.Buffer(), remap.glslName) == 0)
					{
						char const* hlslName = remap.hlslName;
						if(*hlslName == '*')
						{
							hlslName++;

							// Note(tfoley): The specific case we are dealing with right
							// now is that constructing a vector from a scalar value
							// *must* be expressed as a cast in HLSL, while in GLSL
							// it *must* be expressed as a constructor call. We
							// intercept the call to a constructor here in the
							// specific case where it has one argument, and print
							// it differently
							if(instr->Arguments.Count() == 1)
							{
								ctx.Body << "((" << hlslName << ") ";
								PrintOp(ctx, instr->Arguments[0].Ptr());
								ctx.Body << ")";
								return;
							}
						}

						PrintDefaultCallInstrExpr(ctx, instr, hlslName);
						return;
					}
				}

				PrintDefaultCallInstrExpr(ctx, instr, name);
			}

			void PrintTextureCall(CodeGenContext & ctx, CallInstruction * instr)
			{
				// texture functions are defined based on HLSL, so this is trivial
				// internally, texObj.Sample(sampler_obj, uv, ..) is represented as Sample(texObj, sampler_obj, uv, ...)
				// so we need to lift first argument to the front
				PrintOp(ctx, instr->Arguments[0].Ptr(), true);
				ctx.Body << "." << instr->Function;
				ctx.Body << "(";
				for (int i = 1; i < instr->Arguments.Count(); i++)
				{
					PrintOp(ctx, instr->Arguments[i].Ptr());
					if (i < instr->Arguments.Count() - 1)
						ctx.Body << ", ";
				}
				ctx.Body << ")";
			}

			void PrintProjectInstrExpr(CodeGenContext & ctx, ProjectInstruction * proj) override
			{
				// project component out of record type. 
				PrintOp(ctx, proj->Operand.Ptr());
				ctx.Body << "." << proj->ComponentName;
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
					char const* glslName;
					char const* hlslName;
				} kNameRemaps[] =
				{
					{ "vec2", "float2" },
					{ "vec3", "float3" },
					{ "vec4", "float4" },

					{ "ivec2", "int2" },
					{ "ivec3", "int3" },
					{ "ivec4", "int4" },

					{ "uvec2", "uint2" },
					{ "uvec3", "uint3" },
					{ "uvec4", "uint4" },

					{ "mat3", "float3x3" },
					{ "mat4", "float4x4" },

					{ "sampler2D", "Texture2D" },
					{ "sampler2DArray", "Texture2DArray" },
					{ "samplerCube", "TextureCube" },
					{ "sampler2DShadow", "Texture2D" },
					{ "sampler2DArrayShadow", "Texture2DArray" },
					{ "samplerCubeShadow", "TextureCube" },
					{ "sampler3D", "Texture3D" }
				};

				String typeName = type->ToString();
				for(auto remap : kNameRemaps)
				{
					if(strcmp(typeName.Buffer(), remap.glslName) == 0)
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

			String GetLastSegment(String accessName)
			{
				int idx = accessName.LastIndexOf('.');
				if (idx == -1)
					return accessName;
				return accessName.SubString(idx + 1, accessName.Length() - idx - 1);
			}

			void DefineCBufferParameterFields(CodeGenContext & sb, ILModuleParameterSet * module, int & itemsDeclaredInBlock)
			{
				// We declare an inline struct inside the `cbuffer` to ensure that
				// the members have an appropriate prefix on their name.
				sb.GlobalHeader << "struct {\n";

				int index = 0;
				for (auto & field : module->Parameters)
				{
					auto bindableResType = field.Value->Type->GetBindableResourceType();
					if (bindableResType != BindableResourceType::NonBindable)
						continue;
					String declName = field.Key;
					PrintDef(sb.GlobalHeader, field.Value->Type.Ptr(), declName);
					itemsDeclaredInBlock++;
					sb.GlobalHeader << ";\n";
					index++;
				}

				// define sub parameter fields
				for (auto & subParam : module->SubModules)
				{
					int subItemsDeclaredInBlock = 0;
					int declarationStart = sb.GlobalHeader.Length();
					DefineCBufferParameterFields(sb, subParam.Ptr(), subItemsDeclaredInBlock);
					if (subItemsDeclaredInBlock == 0)
					{
						sb.GlobalHeader.Remove(declarationStart, sb.GlobalHeader.Length() - declarationStart);
					}
				}
				sb.GlobalHeader << "} " << GetLastSegment(module->BindingName) << ";\n";
			}
			void DefineBindableParameterFields(CodeGenContext & sb, ILModuleParameterSet * module, int descSetId, int & tCount, int & sCount, int & uCount, int & cCount)
			{
				module->TextureBindingStartIndex = tCount;
				module->SamplerBindingStartIndex = sCount;
				module->StorageBufferBindingStartIndex = uCount;
				module->UniformBindingStartIndex = cCount;
				for (auto & field : module->Parameters)
				{
					auto bindableResType = field.Value->Type->GetBindableResourceType();
					if (bindableResType == BindableResourceType::NonBindable)
						continue;
					PrintDef(sb.GlobalHeader, field.Value->Type.Ptr(), EscapeCodeName(module->BindingName + "_" + field.Key));
					if (field.Value->BindingPoints.Count())
					{
						sb.GlobalHeader << ": register(";
						switch (bindableResType)
						{
						case BindableResourceType::Texture:
							sb.GlobalHeader << "t";
							break;
						case BindableResourceType::Sampler:
							sb.GlobalHeader << "s";
							break;
						case BindableResourceType::StorageBuffer:
							sb.GlobalHeader << "u";
							break;
						case BindableResourceType::Buffer:
							sb.GlobalHeader << "c";
							break;
						default:
							throw NotImplementedException();
						}
						if (useD3D12Registers)
						{
							switch (bindableResType)
							{
							case BindableResourceType::Texture:
								sb.GlobalHeader << tCount;
								tCount++;
								break;
							case BindableResourceType::Sampler:
								sb.GlobalHeader << sCount;
								sCount++;
								break;
							case BindableResourceType::StorageBuffer:
								sb.GlobalHeader << uCount;
								uCount++;
								break;
							case BindableResourceType::Buffer:
								sb.GlobalHeader << cCount;
								cCount++;
								break;
							}
							sb.GlobalHeader << ", space" << descSetId;
						}
						else
						{
							sb.GlobalHeader << field.Value->BindingPoints.First();
						}
						sb.GlobalHeader << ")";
					}
					sb.GlobalHeader << ";\n";
				}
				for (auto & subModule : module->SubModules)
					DefineBindableParameterFields(sb, subModule.Ptr(), descSetId, tCount, sCount, uCount, cCount);
			}
			bool DetermineParameterFieldOffset(ILModuleParameterSet * module, int & ptr)
			{
				bool firstFieldEncountered = false;
				module->UniformBufferOffset = ptr;
				auto layout = GetLayoutRulesImpl(LayoutRule::HLSL);
				auto structInfo = layout->BeginStructLayout();
				for (auto & field : module->Parameters)
				{
					if (field.Value->Type->GetBindableResourceType() == BindableResourceType::NonBindable)
					{
						auto flayout = GetLayout(field.Value->Type.Ptr(), LayoutRule::HLSL);
						field.Value->BufferOffset = (int)(layout->AddStructField(&structInfo, flayout));
						field.Value->Size = (int)flayout.size;
						if (!firstFieldEncountered)
						{
							module->UniformBufferOffset = field.Value->BufferOffset;
							firstFieldEncountered = true;
						}
					}
					for (auto & subModule : module->SubModules)
						firstFieldEncountered = DetermineParameterFieldOffset(subModule.Ptr(), ptr) | firstFieldEncountered;
				}
				layout->EndStructLayout(&structInfo);
				module->BufferSize = (int)structInfo.size;
				return firstFieldEncountered;
			}
			void GenerateShaderParameterDefinition(CodeGenContext & sb, ILShader * shader)
			{
				// first pass: figure out buffer offsets and alignments
				for (auto module : shader->ModuleParamSets)
				{
					if (!module.Value->IsTopLevel)
						continue;
					int ptr = 0;
					DetermineParameterFieldOffset(module.Value.Ptr(), ptr);
				}
				for (auto module : shader->ModuleParamSets)
				{
					if (!module.Value->IsTopLevel)
						continue;
					// TODO: this generates D3D11 style binding, should update to generate D3D12 root signature declaration
					auto moduleName = EscapeCodeName(module.Value->BindingName);
					
					int declarationStart = sb.GlobalHeader.Length();
					int itemsDeclaredInBlock = 0;

					sb.GlobalHeader << "cbuffer buf" << moduleName;
					if (module.Value->DescriptorSetId != -1)
						sb.GlobalHeader << " : register(b" << module.Value->DescriptorSetId << ")";
					sb.GlobalHeader << "\n{\n";
					DefineCBufferParameterFields(sb, module.Value.Ptr(), itemsDeclaredInBlock);
					sb.GlobalHeader << "};\n";

					if (itemsDeclaredInBlock == 0)
					{
						sb.GlobalHeader.Remove(declarationStart, sb.GlobalHeader.Length() - declarationStart);
					}
					int textureReg = 0; 
					int samplerReg = 0;
					int uReg = 0;
					int cReg = 0;
					DefineBindableParameterFields(sb, module.Value.Ptr(), module.Value->DescriptorSetId, textureReg, samplerReg, uReg, cReg);
				}
				
			}

			virtual void PrintParameterReference(StringBuilder& sb, ILModuleParameterInstance * param) override
			{
				if (param->Type->GetBindableResourceType() == BindableResourceType::NonBindable)
				{
					sb << param->Module->BindingName << "." << param->Name;
				}
				else
				{
					sb << EscapeCodeName(param->Module->BindingName + "_" + param->Name);
				}
			}

			void DeclareStandardInputRecord(CodeGenContext & sb, const ILObjectDefinition & input, bool /*isVertexShader*/) override
			{
				auto info = ExtractExternComponentInfo(input);
				extCompInfo[input.Name] = info;
				auto recType = ExtractRecordType(input.Type.Ptr());
				assert(recType);

				DeclareRecordTypeStruct(sb, recType);
			}

			void DeclarePatchInputRecord(CodeGenContext & sb, const ILObjectDefinition & input, bool isVertexShader) override
			{
				// In HLSL, both standard input/output and per-patch input/output are passed as ordinary `struct` types.
				DeclareStandardInputRecord(sb, input, isVertexShader);
			}

			void GenerateDomainShaderAttributes(StringBuilder & sb, ILStage * stage)
			{
				StageAttribute val;
				if (stage->Attributes.TryGetValue("Domain", val))
					sb << "[domain(\"" << ((val.Value == "quads") ? "quad" : "tri") << "\")]\n";
				else
					sb << "[domain(\"tri\")]\n";
                if (val.Value != "triangles" && val.Value != "quads")
                    getSink()->diagnose(val.Position, Diagnostics::invalidTessellationDomain);
			}

			void PrintHeaderBoilerplate(CodeGenContext& ctx)
			{
				// The way that we assign semantics may generate a warning,
				// and rather than clear it up with more complicated codegen,
				// we choose to just disable it (since we control all the
				// semantics anyway).
				ctx.GlobalHeader << "#pragma warning(disable: 3576)\n";

				// In order to be a portable shading language, Spire needs
				// to make some basic decisions about the semantics of
				// matrix operations so they are consistent between APIs.
				//
				// The default interpretation in each language is:
				//   GLSL: column-major storage, column-major semantics
				//   HLSL: column-major storage, row-major semantics
				//
				// We can't change the semantics, but we *can* change
				// the storage layout, and making it be row-major in
				// HLSL ensures that the actual behavior of a shader
				// is consistent between APIs.
				ctx.GlobalHeader << "#pragma pack_matrix( row_major )\n";
			}

			StageSource GenerateSingleWorldShader(ILProgram * program, ILShader * shader, ILStage * stage) override
			{
				// This entry point is used to generate a Vertex, Fragment,
				// Domain, or Compute Shader, since they all amount to
				// a single world.
				//
				// TODO(tfoley): This code actually doesn't work for compute,
				// since it currently assumes there is always going to be
				// "varying" stage input/output.
				//
				// TODO(tfoley): Honestly, there is almost zero value in trying
				// to share this code, and what little sharing there is could
				// be expressed just as well by having a differentry codegen
				// entry point per stage type, with lower-level shared routines
				// they can call into.

				// TODO(tfoley): Ther are no bindles textures in HLSL, so I'm
				// not sure what to do with this flag.
				useBindlessTexture = stage->Attributes.ContainsKey("BindlessTexture");

				StageSource rs;
				CodeGenContext ctx;

				PrintHeaderBoilerplate(ctx);

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
				ILRecordType* stageInputType = nullptr;
				ILRecordType* dsCornerPointType = nullptr;
				int dsCornerPointCount = 0;
				ILRecordType* dsPatchType = nullptr;

				ILObjectDefinition* dsCornerPointInput = nullptr;
				ILObjectDefinition* dsPatchInput = nullptr;

				// We start by emitting whatever declarations the input worlds
				// need, and along the way we try to capture the components/
				// records being used as input, so that we can refer to them
				// appropriately.
				//
				// Note(tfoley): It seems awkward to find these worlds/records
				// that we need by search, whereas the "primary" world for the
				// shader is passed to us more explicitly.
				for (auto & input : world->Inputs)
				{
					DeclareInput(ctx, input, stage->StageType == "VertexShader");

					// We need to detect the world that represents the ordinary stage input...
					// TODO(tfoley): It seems like this is logically part of the stage definition.
					auto info = ExtractExternComponentInfo(input);
					if(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StandardInput)
					{
						auto recType = ExtractRecordType(input.Type.Ptr());
						if(recType)
						{
							stageInputType = recType;
						}
					}
					else if(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::Patch)
					{
						auto recType = ExtractRecordType(input.Type.Ptr());
						if(recType)
						{
							if(info.IsArray)
							{
								dsCornerPointInput = &input;
								dsCornerPointType = recType;
								dsCornerPointCount = info.ArrayLength;
							}
							else
							{
								dsPatchInput = &input;
								dsPatchType = recType;
							}
						}
					}
				}
				if(!stageInputType)
				{
					errWriter->diagnose(stage->Position, Diagnostics::stageDoesntHaveInputWorld, stage->StageType);
				}
		
				// For a domain shader, we need to know how many corners the
				// domain has (triangle or quadrilateral), so that we can
				// declare an output array of appropriate size.
				StageAttribute controlPointCount;
				int cornerCount = 3;
				if(stage->StageType == "DomainShader")
				{
					if (!stage->Attributes.TryGetValue("ControlPointCount", controlPointCount))
					{
						errWriter->diagnose(stage->Position, Diagnostics::domainShaderRequiresControlPointCount);
					}
					StageAttribute val;
					if(stage->Attributes.TryGetValue("Domain", val))
					{
						if(val.Value == "quads")			cornerCount = 4;
						else if(val.Value == "triangles")	cornerCount = 3;
					}
				}

				outputStrategy->DeclareOutput(ctx, stage);
				ctx.codeGen = this;
				world->Code->NameAllInstructions();
				GenerateCode(ctx, world->Code.Ptr());

				// For shader types that might output the special `SV_Position`
				// output, we check if the stage in the pipeline actually
				// declares this output, and emit the logic as needed.
				if (stage->StageType == "VertexShader" || stage->StageType == "DomainShader")
					GenerateVertexShaderEpilog(ctx, world.Ptr(), stage);

				StringBuilder sb;
				sb << ctx.GlobalHeader.ProduceString();

				// We always declare our shader entry point as outputting a
				// single `struct` value, for simplicity. To make this
				// work, we generate a combined `struct` that comprises
				// the user-declared outputs (in a nested `struct`) along
				// with any system-interpreted outputs we need.
				sb << "struct T" << world->OutputType->TypeName << "Ext\n{\n";
				sb << "T" << world->OutputType->TypeName << " user";

				// The fragment shader needs to use the specific output
				// semantic `SV_Target` as expected by the HLSL compiler.
				// Because the `user` field is a `struct` this semantic
				// will recursively propagate to all of its fields.
				//
				// All other stage types will just use the default semantics
				// already applied to the fields of the output `struct`.
				if(stage->StageType == "FragmentShader")
				{
					sb << " : SV_Target";
				}
				sb << ";\n";

				// We emit any required system-output semantics here.
				// For now we are just handling `SV_Position`, but
				// values like fragment shader depth output, etc.
				// would also go here.
				if(stage->Attributes.TryGetValue("Position"))
				{
					sb << "float4 sv_position : SV_Position;\n";
				}
				sb << "};\n";

				if(dsPatchType || dsCornerPointType)
				{
					// A domain shader receives two kinds of input: per-patch
					// and per-control-point. The per-control-point input
					// appears as the ordinary input, from the perspective
					// of the Spire front-end, so we need to declare the
					// per-patch input more explicitly.
					//
					// Similar to what we do with the `*Ext` contrivance
					// above, we are going to output a single `struct`
					// that combines user-defined and system inputs.

					sb << "struct TStageInputPatch\n{\n";
					if(dsPatchType)
					{
						// In order to ensure consistent semantics, we apply
						// a blanket `P` semantic here to the per-patch input.
						// This semantic will override any per-field semantics
						// that got emitted for the record type itself.
						sb << "T" << dsPatchType->TypeName << " "
							<< dsPatchInput->Name << " : P;\n";
					}
					if(dsCornerPointType)
					{
						// Similar to the per-patch case, we declare an array
						// of records for the per-corner-point data, and
						// apply a single blanket semantic that will go and
						// recursively enumerate unique semantics for all
						// the array elements and fields.
						sb << "T" << dsCornerPointType->TypeName << " "
							<< dsCornerPointInput->Name
							<< "[" << cornerCount << "] : C;\n";
					}

					// Note: HLSL requires tessellation level to be declared
					// as an input to the Domain Shader, even if it is unused

					// TODO(tfoley): This repeated matching on the `Domain`
					// attribute by string comparison is dangerous, and needs
					// to be handled more centrally and robustly.
					StageAttribute val;
					if(stage->Attributes.TryGetValue("Domain", val) && (val.Value == "quads"))
					{
						sb << "    float sv_TessFactors[4] : SV_TessFactor;\n";
						sb << "    float sv_InsideTessFactors[2] : SV_InsideTessFactor;\n";
					}
					else
					{
						sb << "    float sv_TessFactors[3] : SV_TessFactor;\n";
						sb << "    float sv_InsideTessFactors[1] : SV_InsideTessFactor;\n";
					}

					sb << "};\n";
				}

				// The domain shader has a few required attributes that need
				// to be emitted in front of the declaration of `main()`.
				if(stage->StageType == "DomainShader")
				{
					GenerateDomainShaderAttributes(sb, stage);
				}

				sb << "T" << world->OutputType->TypeName << "Ext main(";

				if(stageInputType)
				{
					// We need to declare our inputs a bit differently,
					// depending on the stage we are emitting:

					// TODO(tfoley): All this string-based matching on stage type seems wrong
					if(stage->StageType == "DomainShader")
					{
						// A domain shader needs to declare an array of input
						// control points, using the special-purpose generic type
						// provided by HLSL.
						sb << "OutputPatch<T" << stageInputType->TypeName << ", " << controlPointCount.Value << "> stage_input";
					}
                    /* FALCOR Don't treat vertex shader specially...
					else if (stage->StageType == "VertexShader")
					{
						// A vertex shader can declare its input as normal, but
						// to make matching over vertex attributes with host
						// code simpler, we apply a blanket `A` semantic here,
						// so that the individual vertex elements in the input
						// layout will all get the semantic "A" with sequential
						// indices starting at zero.
						sb << "\n    T" << stageInputType->TypeName << " stage_input : A";
					}
                    FALCOR */
					else
					{
						// Finally, the default case just uses the semantics
						// that were automatically assigned to the fields
						// of the input record type.
						sb << "\n    T" << stageInputType->TypeName << " stage_input";
					}
				}

				if(dsPatchType || dsCornerPointType)
				{
					// For a domain shader, we also need to declare
					// the per-patch (and per-corner point) input.
					sb << ",\n    TStageInputPatch stage_input_patch";
				}

				// Next we declare any addition system inputs that ended up
				// being used during code generation.
				if(ctx.UsedSystemInputs.Contains(ExternComponentCodeGenInfo::SystemVarType::TessCoord))
				{
					sb << ",\n    ";

					StageAttribute val;
					if(stage->Attributes.TryGetValue("Domain", val))
						sb << ((val.Value == "quads") ? "float2" : "float3");
					else
						sb << "float3";

					sb << " sv_DomainLocation : SV_DomainLocation";
				}
				if(ctx.UsedSystemInputs.Contains(ExternComponentCodeGenInfo::SystemVarType::FragCoord))
				{
					sb << ",\n    float4 sv_FragPosition : SV_Position";
				}
				if(ctx.UsedSystemInputs.Contains(ExternComponentCodeGenInfo::SystemVarType::InstanceId))
				{
					sb << ",\n    uint sv_InstanceID : SV_InstanceID";
				}

				sb << ")\n{ \n";
				sb << "T" << world->OutputType->TypeName << "Ext stage_output;\n";
				sb << ctx.Header.ProduceString() << ctx.Body.ProduceString();
				sb << "return stage_output;\n";
				sb << "}";
				rs.MainCode = sb.ProduceString();
				return rs;
			}

			void DeclareRecordTypeStruct(CodeGenContext& ctx, ILRecordType* recType)
			{
				// By convention, the name of the generated `struct` is
				// "T" prefixed onto the name of the record type.
				ctx.GlobalHeader << "struct T" << recType->TypeName << "\n{\n";

				int index = 0;
				for (auto & field : recType->Members)
				{
					// As a catch-all, we apply the `nointerpolation`
					// modifier to all integral types, even though
					// this really only affects records that flow
					// through rasterization/setup/interpolation.
					if (field.Value.Type->IsIntegral())
						ctx.GlobalHeader << "nointerpolation ";

					// Declare the field as a `struct` member
					String declName = field.Key;
					PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), declName);

					// We automatically synthesize a semantic for every
					// field. This will need to match any equivalent
					// declaration on the input side.
					//
					// The semantic must be unique across fields.
					// We can't simply use a convention like "A0", "A1", ...
					// because these semantics with a numeric suffix
					// will not interact nicely with fields of `struct`
					// or array type.
					//
					// We could use the field name to generate a unique
					// semantic, but this might be long and ugly, and we'd
					// need to decorate it to avoid accidentally having a
					// numeric suffix, or an "SV_" prefix.
					//
					// Ultimately, the easiest thing to do is to take the
					// simple "A0", "A1", ... idea and simply add another
					// "A" onto the end, so that it isn't techically a
					// numeric suffix. So: "A0A", "A1A", "A2A", ...
					//
					// In the case where the field is a `struct` or array
					// type, the HLSL compiler will then automatically
					// generate per-field/-element semantics based on
					// the prefix we gave it, e.g.: "A0A0", "A0A1", ...
//FALCOR					ctx.GlobalHeader << " : A" << index << "A";

                    // FALCOR: just use the name instead...
                    ctx.GlobalHeader << " : " << declName;

					ctx.GlobalHeader << ";\n";
					index++;
				}

				ctx.GlobalHeader << "};\n";
			}

			// Most of our generated HLSL code can use a single simple output
			// strategy, which simply declares the output as a `struct` type
			// (to be used in the declaration of `main()`, and then output
			// writes so that they reference the fields of that type with
			// a simple prefix.
			struct SimpleOutputStrategy : OutputStrategy
			{
				HLSLCodeGen* hlslCodeGen;
				String prefix;

				SimpleOutputStrategy(HLSLCodeGen* hlslCodeGen, ILWorld* world, String const& prefix)
					: OutputStrategy(hlslCodeGen, world)
					, hlslCodeGen(hlslCodeGen)
					, prefix(prefix)
				{}

				virtual void DeclareOutput(CodeGenContext & ctx, ILStage * /*stage*/) override
				{
					hlslCodeGen->DeclareRecordTypeStruct(ctx, world->OutputType.Ptr());
				}

				virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) override
				{
					ctx.Body << prefix << "." << instr->ComponentName << " = ";
					codeGen->PrintOp(ctx, instr->Operand.Ptr());
					ctx.Body << ";\n";
				}
			};


			StageSource GenerateHullShader(ILProgram * program, ILShader * shader, ILStage * stage) override
			{
				// As a first step, we validate the various attributes required
				// on a `HullShader` stage declaration.
				//
				// Note(tfoley): This logic is mostly copy-pasted from the GLSL
				// case, and there is a reasonable case to be made that it
				// should be unified.
				//
				StageSource rs;
				StageAttribute patchWorldName, controlPointWorldName, cornerPointWorldName, domain, innerLevel, outerLevel, numControlPoints;
				StageAttribute inputControlPointCount;
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
					errWriter->diagnose(domain.Position, Diagnostics::invalidTessellationDomian);
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
				if (!stage->Attributes.TryGetValue("InputControlPointCount", inputControlPointCount))
				{
					errWriter->diagnose(stage->Position, Diagnostics::hullShaderRequiresInputControlPointCount);
					return rs;
				}
				if (!stage->Attributes.TryGetValue("ControlPointCount", numControlPoints))
				{
					errWriter->diagnose(stage->Position, Diagnostics::hullShaderRequiresControlPointCount);
					return rs;
				}

				// Note(tfoley): The needs of HLSL codegen forced me to add
				// a few more required attributes, and we probably need to
				// decide whether to always require these (for portability)
				// or only require them when generating HLSL.
				//
				StageAttribute partitioning;
				if(!stage->Attributes.TryGetValue("Partitioning", partitioning))
				{
					errWriter->diagnose(stage->Position, Diagnostics::hullShaderRequiresPartitioning);
					return rs;
				}
				StageAttribute outputTopology;
				if(!stage->Attributes.TryGetValue("OutputTopology", outputTopology))
				{
					errWriter->diagnose(stage->Position, Diagnostics::hullShaderRequiresOutputTopology);
					return rs;
				}
				// TODO(tfoley): Any reason to include an optional
				// `maxtessfactor` attribute?

				CodeGenContext ctx;
				ctx.codeGen = this;
				List<ILWorld*> worlds;
				worlds.Add(patchWorld.Ptr());
				worlds.Add(controlPointWorld.Ptr());
				worlds.Add(cornerPointWorld.Ptr());

				PrintHeaderBoilerplate(ctx);

				int cornerCount = 3;
				if(domain.Value == "triangles")
					cornerCount = 3;
				else if(domain.Value == "quads")
					cornerCount = 4;


				GenerateStructs(ctx.GlobalHeader, program);
				GenerateShaderParameterDefinition(ctx, shader);
				GenerateReferencedFunctions(ctx.GlobalHeader, program, worlds.GetArrayView());

				// As in the single-world case, we need to emit declarations
				// for any inputs to the stage, but unlike that case we have
				// multiple worlds to deal with.
				//
				// We maintain a set of inputs encountered so far, so that
				// don't re-declare any given input.
				HashSet<String> declaredInputs;

				// Similar to the single-world case, we try to capture
				// some information about inputs so that we can use it
				// to inform code generation later.
				String perCornerIteratorInputName = "perCornerIterator";
				ILRecordType* coarseVertexType = nullptr;
				//
				for (auto & input : controlPointWorld->Inputs)
				{
					if(declaredInputs.Add(input.Name))
					{
						DeclareInput(ctx, input, false);

						auto info = ExtractExternComponentInfo(input);
						if(info.DataStructure == ExternComponentCodeGenInfo::DataStructureType::StandardInput)
						{
							auto recType = ExtractRecordType(input.Type.Ptr());
							if(recType)
								coarseVertexType = recType;
						}
					}
				}
				for (auto & input : patchWorld->Inputs)
				{
					if (declaredInputs.Add(input.Name))
						DeclareInput(ctx, input, false);
				}
				for (auto & input : cornerPointWorld->Inputs)
				{
					if(declaredInputs.Add(input.Name))
					{
						DeclareInput(ctx, input, false);

						if(input.Attributes.ContainsKey("PerCornerIterator"))
						{
							perCornerIteratorInputName = input.Name;
						}
					}
				}


				// HLSL requires two entry points for the Hull Shader: a
				// "patch-constant" function and the ordinary `main()`
				// entry point (which runs per-control-point).
				//
				// We start code generation with the "patch-constant"
				// function, which we use for per-patch and per-corner
				// computation.


				// Perform per-corner computation
				cornerPointWorld->Code->NameAllInstructions();

				StringBuilder cornerPointOutputPrefix;
				cornerPointOutputPrefix << "stage_output.corners[" << perCornerIteratorInputName << "]";

				outputStrategy = new SimpleOutputStrategy(this, cornerPointWorld.Ptr(), cornerPointOutputPrefix.ProduceString());
				outputStrategy->DeclareOutput(ctx, stage);

				// Note(tfoley): We use the `[unroll]` attribute here, because
				// the HLSL compiler will end up unrolling this loop anyway,
				// and we'd rather not get their warning about it.
				ctx.Body << "[unroll] for (uint " << perCornerIteratorInputName << " = 0; "
					<< perCornerIteratorInputName << " < " << cornerCount << "; "
					<< perCornerIteratorInputName << "++)\n{\n";
				GenerateCode(ctx, cornerPointWorld->Code.Ptr());
				auto debugStr = cornerPointWorld->Code->ToString();
				ctx.Body << "}\n";
				outputStrategy = NULL;

				// Perform per-patch computation
				patchWorld->Code->NameAllInstructions();
				outputStrategy = CreateStandardOutputStrategy(patchWorld.Ptr(), "patch");
				outputStrategy->DeclareOutput(ctx, stage);
				GenerateCode(ctx, patchWorld->Code.Ptr());

				// Compute the number of edges and interior axes we need to deal with.
				StageAttribute val;
				int tessFactorCount = 3;
				int insideFactorCount = 1;
				if(stage->Attributes.TryGetValue("Domain", val) && (val.Value == "quads"))
				{
					tessFactorCount = 4;
					insideFactorCount = 2;
				}
				else
				{
					tessFactorCount = 3;
					insideFactorCount = 1;
				}

				// Generate code to set tessellation factors.
				//
				// TODO(tfoley): This is written as a search over worlds,
				// whereas I would have expected the tess factors to
				// be expected in a fixed world (e.g., @PatchEdge,
				// and then @PatchInterior). This should probalby get
				// cleaned up.
				//
				// Note(tfoley): I swapped the order from what the GLSL
				// case does, so that we output the edge factors before
				// the interior one(s). This doesn't matter right now,
				// but in practice many adaptive schemes will want to
				// compute the interior factor(s) from the edge ones,
				// so this ordering would in theory be conducive to that.
				bool found = false;
				for (auto & world : worlds)
				{
					ILOperand * operand;
					if (world->Components.TryGetValue(outerLevel.Value, operand))
					{
						for (int i = 0; i < tessFactorCount; i++)
						{
							// TODO(tfoley): is this needlessly re-computing the operand multiple times?

							ctx.Body << "stage_output.sv_TessFactors[" << i << "] = ";
							PrintOp(ctx, operand);
							ctx.Body << "[" << i << "];\n";
						}
						found = true;
						break;
					}

				}
				if (!found)
					errWriter->diagnose(outerLevel.Position, Diagnostics::componentNotDefined, outerLevel.Value);


				found = false;
				for (auto & world : worlds)
				{
					ILOperand * operand;
					if (world->Components.TryGetValue(innerLevel.Value, operand))
					{
						for (int i = 0; i < insideFactorCount; i++)
						{
							ctx.Body << "stage_output.sv_InsideTessFactors[" << i << "] = ";
							PrintOp(ctx, operand);
							ctx.Body << "[" << i << "];\n";
						}
						found = true;
						break;
					}
				}
				if (!found)
					errWriter->diagnose(innerLevel.Position, Diagnostics::componentNotDefined, innerLevel.Value);

				// Now surround the code with the boilerplate needed to
				// make a real Hull Shader "patch constant function"

				StringBuilder patchMain;

				patchMain << "struct SPIRE_PatchOutput\n{\n";
				patchMain << "T" << patchWorld->OutputType->TypeName << " user : P;\n";
				patchMain << "T" << cornerPointWorld->OutputType->TypeName << " corners[" << cornerCount << "] : C;\n";

				patchMain << "    float sv_TessFactors[" << tessFactorCount << "] : SV_TessFactor;\n";
				patchMain << "    float sv_InsideTessFactors[" << insideFactorCount << "] : SV_InsideTessFactor;\n";

				patchMain << "};\n";


				patchMain << "SPIRE_PatchOutput SPIRE_patchOutput(";

				if (coarseVertexType)
				{
					patchMain << "    InputPatch<T" << coarseVertexType->TypeName << ", " << inputControlPointCount.Value << "> stage_input\n";
				}
				// TODO(tfoley): provide other input shere like SV_PrimitiveID


				patchMain << ")\n{\n";
				patchMain << "SPIRE_PatchOutput stage_output;\n";
				patchMain << ctx.Header.ProduceString() << ctx.Body.ProduceString();
				patchMain << "return stage_output;\n";
				patchMain << "}\n";

				// After we are done outputting the per-patch entry point,
				// we move on to the per-control-point one.
				//
				// Note that calling `ProduceString()` on the `Header` and
				// `Body` builders above has cleared them out for us.

				controlPointWorld->Code->NameAllInstructions();
				outputStrategy = new SimpleOutputStrategy(this, controlPointWorld.Ptr(), "stage_output");
				outputStrategy->DeclareOutput(ctx, stage);
				GenerateCode(ctx, controlPointWorld->Code.Ptr());


				StringBuilder controlPointMain;

				// HLSL requires a bunch of attributres in front of the
				// Hull Shader `main()` (the per-control-point function)
				// These are effectively binding the state of the
				// fixed-function tessellator (rather than have a bunch of API
				// state for it).

				// Name of the entry point to use for the patch-constant phase
				controlPointMain << "[patchconstantfunc(\"SPIRE_patchOutput\")]\n";

				// Domain for tessellation.
				controlPointMain << "[domain(\"";
				if(domain.Value == "quads")
				{
					controlPointMain << "quad";
				}
				else if(domain.Value == "triangles")
				{
					controlPointMain << "tri";
				}
				else
				{
					errWriter->diagnose(domain.Position, Diagnostics::invalidTessellationDomain);
					return rs;
				}
				controlPointMain << "\")]\n";

				// Parititoning mode (integer, fractional, etc.)
				controlPointMain << "[partitioning(\"";
				if(partitioning.Value == "integer")
				{
					controlPointMain << "integer";
				}
				else if(partitioning.Value == "pow2")
				{
					controlPointMain << "pow2";
				}
				else if(partitioning.Value == "fractional_even")
				{
					controlPointMain << "fractional_even";
				}
				else if(partitioning.Value == "fractional_odd")
				{
					controlPointMain << "fractional_odd";
				}
				else
				{
					errWriter->diagnose(partitioning.Position, Diagnostics::invalidTessellationPartitioning);
					return rs;
				}
				controlPointMain << "\")]\n";

				// Desired output topology, including winding order
				// for triangles.
				controlPointMain << "[outputtopology(\"";
				if(outputTopology.Value == "point")
				{
					controlPointMain << "point";
				}
				else if(outputTopology.Value == "line")
				{
					controlPointMain << "line";
				}
				else if(outputTopology.Value == "triangle_cw")
				{
					controlPointMain << "triangle_cw";
				}
				else if(outputTopology.Value == "triangle_ccw")
				{
					controlPointMain << "triangle_ccw";
				}
				else
				{
					errWriter->diagnose(partitioning.Position, Diagnostics::invalidTessellationOutputTopology);
					return rs;
				}
				controlPointMain << "\")]\n";

				// Number of output control points
				controlPointMain << "[outputcontrolpoints(" << numControlPoints.Value << ")]\n";

				// With all the attributes dealt with, we can emit the actual `main()` routine

				controlPointMain << "T" << controlPointWorld->OutputType->TypeName << " main(";

				controlPointMain << "    InputPatch<T" << coarseVertexType->TypeName << ", " << inputControlPointCount.Value << "> stage_input";
				controlPointMain << ",\n    uint sv_ControlPointID : SV_OutputControlPointID";

				controlPointMain << ")\n{\n";
				controlPointMain << "T" << controlPointWorld->OutputType->TypeName << " stage_output;\n";
				controlPointMain << ctx.Header.ProduceString() << ctx.Body.ProduceString();
				controlPointMain << "return stage_output;\n";
				controlPointMain << "}\n";

				StringBuilder sb;
				sb << ctx.GlobalHeader.ProduceString();
				sb << patchMain.ProduceString();
				sb << controlPointMain.ProduceString();
				rs.MainCode = sb.ProduceString();
				return rs;
			}
		};

		// TODO(tfoley): This code has not been ported or tested.
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
				ctx.GlobalHeader << "struct T" << world->OutputType->TypeName << "\n{\n";

				for (auto & field : world->OutputType->Members)
				{
					codeGen->PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), field.Key);
					ctx.GlobalHeader << ";\n";
				}

				ctx.GlobalHeader << "};\n";
			}
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) override
			{
				ctx.Body << AddWorldNameSuffix(instr->ComponentName, world->Name) << "[" << outputIndex << "] = ";
				codeGen->PrintOp(ctx, instr->Operand.Ptr());
				ctx.Body << ";\n";
			}
		};

		// TODO(tfoley): This code has not been ported or tested.
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

		OutputStrategy * HLSLCodeGen::CreateStandardOutputStrategy(ILWorld * world, String layoutPrefix)
		{
			return new HLSLCodeGen::SimpleOutputStrategy(this, world, "stage_output.user");
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