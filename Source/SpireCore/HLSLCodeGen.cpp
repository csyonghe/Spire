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
				// The matrix-vector, vector-matrix, and matrix-matrix product
				// operation is written with the `*` operator in GLSL, but
				// is handled by the built-in function `mul()` in HLSL.
				//
				// This function is called by the code generator for that op
				// and allows us to print it appropriately.

				ctx.Body << L"mul(";
				PrintOp(ctx, op1);
				ctx.Body << L", ";
				PrintOp(ctx, op0);
				ctx.Body << L")";
			}

			void PrintUniformBufferInputReference(StringBuilder& sb, String inputName, String componentName) override
			{
				if (!currentImportInstr->Type->IsTexture() || useBindlessTexture)
					sb << L"blk" << inputName;
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
				sb << L"stage_input/*standard*/";
			}

			void PrintStandardArrayInputReference(StringBuilder& sb, ILRecordType* /*recType*/, String inputName, String componentName) override
			{
				sb << L"stage_input/*array*/";
			}

			void PrintPatchInputReference(StringBuilder& sb, ILRecordType* /*recType*/, String inputName, String componentName) override
			{
				sb << L"stage_input_patch/*patch*/." << inputName;
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
					sb << L"sv_FragPosition";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::TessCoord:
					sb << L"sv_DomainLocation";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::InvocationId:
					sb << L"sv_ThreadID";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::ThreadId:
					sb << L"sv_GlobalThreadID.x";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::PatchVertexCount:
					// TODO(tfoley): there is no equivalent of this in HLSL
					sb << L"sv_InputControlPointCount";
					break;
				case ExternComponentCodeGenInfo::SystemVarType::PrimitiveId:
					sb << L"sv_PrimitiveID";
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
					wchar_t const* glslName;
					wchar_t const* hlslName;
				} kNameRemaps[] =
				{
					{ L"vec2", L"*float2" },
					{ L"vec3", L"*float3" },
					{ L"vec4", L"*float4" },

					{ L"ivec2", L"*int2" },
					{ L"ivec3", L"*int3" },
					{ L"ivec4", L"*int4" },

					{ L"uvec2", L"*uint2" },
					{ L"uvec3", L"*uint3" },
					{ L"uvec4", L"*uint4" },

					{ L"mat3", L"float3x3" },
					{ L"mat4", L"float4x4" },
				};

				for(auto remap : kNameRemaps)
				{
					if(wcscmp(name.Buffer(), remap.glslName) == 0)
					{
						wchar_t const* hlslName = remap.hlslName;
						if(*hlslName == L'*')
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
								ctx.Body << L"((" << hlslName << L") ";
								PrintOp(ctx, instr->Arguments[0].Ptr());
								ctx.Body << L")";
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
				if (stage->Attributes.TryGetValue(L"Domain", val))
					sb << L"[domain(\"" << ((val.Value == L"quads") ? L"quad" : L"tri") << L"\")]\n";
				else
					sb << L"[domain(\"tri\")]\n";
				if (val.Value != L"triangles" && val.Value != L"quads")
					Error(50093, L"'Domain' should be either 'triangles' or 'quads'.", val.Position);
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
				useBindlessTexture = stage->Attributes.ContainsKey(L"BindlessTexture");

				StageSource rs;
				CodeGenContext ctx;

				PrintHeaderBoilerplate(ctx);

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
					DeclareInput(ctx, input, stage->StageType == L"VertexShader");

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
					errWriter->Error(99999, L"'" + stage->StageType + L"' doesn't appear to have any input world", stage->Position);
				}
		
				// For a domain shader, we need to know how many corners the
				// domain has (triangle or quadrilateral), so that we can
				// declare an output array of appropriate size.
				StageAttribute controlPointCount;
				int cornerCount = 3;
				if(stage->StageType == "DomainShader")
				{
					if (!stage->Attributes.TryGetValue(L"ControlPointCount", controlPointCount))
					{
						errWriter->Error(50052, L"'DomainShader' requires attribute 'ControlPointCount'.", stage->Position);
					}
					StageAttribute val;
					if(stage->Attributes.TryGetValue(L"Domain", val))
					{
						if(val.Value == L"quads")			cornerCount = 4;
						else if(val.Value == L"triangles")	cornerCount = 3;
					}
				}

				outputStrategy->DeclareOutput(ctx, stage);
				ctx.codeGen = this;
				world->Code->NameAllInstructions();
				GenerateCode(ctx, world->Code.Ptr());

				// For shader types that might output the special `SV_Position`
				// output, we check if the stage in the pipeline actually
				// declares this output, and emit the logic as needed.
				if (stage->StageType == L"VertexShader" || stage->StageType == L"DomainShader")
					GenerateVertexShaderEpilog(ctx, world.Ptr(), stage);

				StringBuilder sb;
				sb << ctx.GlobalHeader.ProduceString();

				// We always declare our shader entry point as outputting a
				// single `struct` value, for simplicity. To make this
				// work, we generate a combined `struct` that comprises
				// the user-declared outputs (in a nested `struct`) along
				// with any system-interpreted outputs we need.
				sb << L"struct T" << world->OutputType->TypeName << "Ext\n{\n";
				sb << L"T" << world->OutputType->TypeName << " user";

				// The fragment shader needs to use the specific output
				// semantic `SV_Target` as expected by the HLSL compiler.
				// Because the `user` field is a `struct` this semantic
				// will recursively propagate to all of its fields.
				//
				// All other stage types will just use the default semantics
				// already applied to the fields of the output `struct`.
				if(stage->StageType == "FragmentShader")
				{
					sb << L" : SV_Target";
				}
				sb << L";\n";

				// We emit any required system-output semantics here.
				// For now we are just handling `SV_Position`, but
				// values like fragment shader depth output, etc.
				// would also go here.
				if(stage->Attributes.TryGetValue(L"Position"))
				{
					sb << L"float4 sv_position : SV_Position;\n";
				}
				sb << L"};\n";

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

					sb << L"struct TStageInputPatch\n{\n";
					if(dsPatchType)
					{
						// In order to ensure consistent semantics, we apply
						// a blanket `P` semantic here to the per-patch input.
						// This semantic will override any per-field semantics
						// that got emitted for the record type itself.
						sb << L"T" << dsPatchType->TypeName << L" "
							<< dsPatchInput->Name << L" : P;\n";
					}
					if(dsCornerPointType)
					{
						// Similar to the per-patch case, we declare an array
						// of records for the per-corner-point data, and
						// apply a single blanket semantic that will go and
						// recursively enumerate unique semantics for all
						// the array elements and fields.
						sb << L"T" << dsCornerPointType->TypeName << L" "
							<< dsCornerPointInput->Name
							<< L"[" << cornerCount << L"] : C;\n";
					}

					// Note: HLSL requires tessellation level to be declared
					// as an input to the Domain Shader, even if it is unused

					// TODO(tfoley): This repeated matching on the `Domain`
					// attribute by string comparison is dangerous, and needs
					// to be handled more centrally and robustly.
					StageAttribute val;
					if(stage->Attributes.TryGetValue(L"Domain", val) && (val.Value == L"quads"))
					{
						sb << L"    float sv_TessFactors[4] : SV_TessFactor;\n";
						sb << L"    float sv_InsideTessFactors[2] : SV_InsideTessFactor;\n";
					}
					else
					{
						sb << L"    float sv_TessFactors[3] : SV_TessFactor;\n";
						sb << L"    float sv_InsideTessFactors[1] : SV_InsideTessFactor;\n";
					}

					sb << L"};\n";
				}

				// The domain shader has a few required attributes that need
				// to be emitted in front of the declaration of `main()`.
				if(stage->StageType == "DomainShader")
				{
					GenerateDomainShaderAttributes(sb, stage);
				}

				sb << L"T" << world->OutputType->TypeName << L"Ext main(";

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
						sb << L"OutputPatch<T" << stageInputType->TypeName << ", " << controlPointCount.Value << L"> stage_input";
					}
					else if (stage->StageType == "VertexShader")
					{
						// A vertex shader can declare its input as normal, but
						// to make matching over vertex attributes with host
						// code simpler, we apply a blanket `A` semantic here,
						// so that the individual vertex elements in the input
						// layout will all get the semantic "A" with sequential
						// indices starting at zero.
						sb << L"\n    T" << stageInputType->TypeName << " stage_input : A";
					}
					else
					{
						// Finally, the default case just uses the semantics
						// that were automatically assigned to the fields
						// of the input record type.
						sb << L"\n    T" << stageInputType->TypeName << " stage_input";
					}
				}

				if(dsPatchType || dsCornerPointType)
				{
					// For a domain shader, we also need to declare
					// the per-patch (and per-corner point) input.
					sb << L",\n    TStageInputPatch stage_input_patch";
				}

				// Next we declare any addition system inputs that ended up
				// being used during code generation.
				if(ctx.UsedSystemInputs.Contains(ExternComponentCodeGenInfo::SystemVarType::TessCoord))
				{
					sb << L",\n    ";

					StageAttribute val;
					if(stage->Attributes.TryGetValue(L"Domain", val))
						sb << ((val.Value == L"quads") ? L"float2" : L"float3");
					else
						sb << L"float3";

					sb << L" sv_DomainLocation : SV_DomainLocation";
				}
				if(ctx.UsedSystemInputs.Contains(ExternComponentCodeGenInfo::SystemVarType::FragCoord))
				{
					sb << L",\n    float4 sv_FragPosition : SV_Position";
				}

				sb << ")\n{ \n";
				sb << "T" << world->OutputType->TypeName << "Ext stage_output;\n";
				sb << ctx.Header.ProduceString() << ctx.Body.ProduceString();
				sb << "return stage_output;\n";
				sb << L"}";
				rs.MainCode = sb.ProduceString();
				return rs;
			}

			void DeclareRecordTypeStruct(CodeGenContext& ctx, ILRecordType* recType)
			{
				// By convention, the name of the generated `struct` is
				// "T" prefixed onto the name of the record type.
				ctx.GlobalHeader << L"struct T" << recType->TypeName << L"\n{\n";

				int index = 0;
				for (auto & field : recType->Members)
				{
					// As a catch-all, we apply the `noperspective`
					// modifier to all integral types, even though
					// this relaly only affects records that flow
					// through rasterization/setup/interpolation.
					if (field.Value.Type->IsIntegral())
						ctx.GlobalHeader << L"noperspective ";

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
					ctx.GlobalHeader << " : A" << index << "A";

					ctx.GlobalHeader << ";\n";
					index++;
				}

				ctx.GlobalHeader << L"};\n";
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
					ctx.Body << prefix << L"." << instr->ComponentName << L" = ";
					codeGen->PrintOp(ctx, instr->Operand.Ptr());
					ctx.Body << L";\n";
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
				if (!stage->Attributes.TryGetValue(L"InputControlPointCount", inputControlPointCount))
				{
					errWriter->Error(50052, L"'HullShader' requires attribute 'InputControlPointCount'.", stage->Position);
					return rs;
				}
				if (!stage->Attributes.TryGetValue(L"ControlPointCount", numControlPoints))
				{
					errWriter->Error(50052, L"'HullShader' requires attribute 'ControlPointCount'.", stage->Position);
					return rs;
				}

				// Note(tfoley): The needs of HLSL codegen forced me to add
				// a few more required attributes, and we probably need to
				// decide whether to always require these (for portability)
				// or only require them when generating HLSL.
				//
				StageAttribute partitioning;
				if(!stage->Attributes.TryGetValue(L"Partitioning", partitioning))
				{
					errWriter->Error(50052, L"'HullShader' requires attribute 'Partitioning'.", stage->Position);
					return rs;
				}
				StageAttribute outputTopology;
				if(!stage->Attributes.TryGetValue(L"OutputTopology", outputTopology))
				{
					errWriter->Error(50052, L"'HullShader' requires attribute 'OutputTopology'.", stage->Position);
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
				if(domain.Value == L"triangles")
					cornerCount = 3;
				else if(domain.Value == L"quads")
					cornerCount = 4;


				GenerateStructs(ctx.GlobalHeader, program);
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
				String perCornerIteratorInputName = L"perCornerIterator";
				ILRecordType* coarseVertexType;

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
				cornerPointOutputPrefix << L"stage_output.corners[" << perCornerIteratorInputName << L"]";

				outputStrategy = new SimpleOutputStrategy(this, cornerPointWorld.Ptr(), cornerPointOutputPrefix.ProduceString());
				outputStrategy->DeclareOutput(ctx, stage);

				// Note(tfoley): We use the `[unroll]` attribute here, because
				// the HLSL compiler will end up unrolling this loop anyway,
				// and we'd rather not get their warning about it.
				ctx.Body << L"[unroll] for (uint " << perCornerIteratorInputName << " = 0; "
					<< perCornerIteratorInputName << " < " << cornerCount << L"; "
					<< perCornerIteratorInputName << "++)\n{\n";
				GenerateCode(ctx, cornerPointWorld->Code.Ptr());
				auto debugStr = cornerPointWorld->Code->ToString();
				ctx.Body << L"}\n";
				outputStrategy = NULL;

				// Perform per-patch computation
				patchWorld->Code->NameAllInstructions();
				outputStrategy = CreateStandardOutputStrategy(patchWorld.Ptr(), L"patch");
				outputStrategy->DeclareOutput(ctx, stage);
				GenerateCode(ctx, patchWorld->Code.Ptr());

				// Compute the number of edges and interior axes we need to deal with.
				StageAttribute val;
				int tessFactorCount = 3;
				int insideFactorCount = 1;
				if(stage->Attributes.TryGetValue(L"Domain", val) && (val.Value == L"quads"))
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

							ctx.Body << L"stage_output.sv_TessFactors[" << i << L"] = ";
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


				found = false;
				for (auto & world : worlds)
				{
					ILOperand * operand;
					if (world->Components.TryGetValue(innerLevel.Value, operand))
					{
						for (int i = 0; i < insideFactorCount; i++)
						{
							ctx.Body << L"stage_output.sv_InsideTessFactors[" << i << L"] = ";
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

				// Now surround the code with the boilerplate needed to
				// make a real Hull Shader "patch constant function"

				StringBuilder patchMain;

				patchMain << L"struct SPIRE_PatchOutput\n{\n";
				patchMain << L"T" << patchWorld->OutputType->TypeName << " user : P;\n";
				patchMain << L"T" << cornerPointWorld->OutputType->TypeName << L" corners[" << cornerCount << L"] : C;\n";

				patchMain << L"    float sv_TessFactors[" << tessFactorCount << L"] : SV_TessFactor;\n";
				patchMain << L"    float sv_InsideTessFactors[" << insideFactorCount << "] : SV_InsideTessFactor;\n";

				patchMain << L"};\n";


				patchMain << L"SPIRE_PatchOutput SPIRE_patchOutput(";

				if (coarseVertexType)
				{
					patchMain << "    InputPatch<T" << coarseVertexType->TypeName << ", " << inputControlPointCount.Value << "> stage_input\n";
				}
				// TODO(tfoley): provide other input shere like SV_PrimitiveID


				patchMain << L")\n{\n";
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
				controlPointMain << L"[patchconstantfunc(\"SPIRE_patchOutput\")]\n";

				// Domain for tessellation.
				controlPointMain << L"[domain(\"";
				if(domain.Value == L"quads")
				{
					controlPointMain << L"quad";
				}
				else if(domain.Value == L"triangles")
				{
					controlPointMain << L"tri";
				}
				else
				{
					errWriter->Error(50053, L"'Domain' should be either 'triangles' or 'quads'.", domain.Position);
					return rs;
				}
				controlPointMain << L"\")]\n";

				// Parititoning mode (integer, fractional, etc.)
				controlPointMain << L"[partitioning(\"";
				if(partitioning.Value == L"integer")
				{
					controlPointMain << "integer";
				}
				else if(partitioning.Value == L"pow2")
				{
					controlPointMain << "pow2";
				}
				else if(partitioning.Value == L"fractional_even")
				{
					controlPointMain << "fractional_even";
				}
				else if(partitioning.Value == L"fractional_odd")
				{
					controlPointMain << "fractional_odd";
				}
				else
				{
					errWriter->Error(50053, L"'Partitioning' must be one of: 'integer', 'pow2', 'fractional_even', or 'fractional_odd'.", partitioning.Position);
					return rs;
				}
				controlPointMain << L"\")]\n";

				// Desired output topology, including winding order
				// for triangles.
				controlPointMain << L"[outputtopology(\"";
				if(outputTopology.Value == L"point")
				{
					controlPointMain << "point";
				}
				else if(outputTopology.Value == L"line")
				{
					controlPointMain << "line";
				}
				else if(outputTopology.Value == L"triangle_cw")
				{
					controlPointMain << "triangle_cw";
				}
				else if(outputTopology.Value == L"triangle_ccw")
				{
					controlPointMain << "triangle_ccw";
				}
				else
				{
					errWriter->Error(50053, L"'OutputTopology' must be one of: 'point', 'line', 'triangle_cw', or 'triangle_ccw'.", partitioning.Position);
					return rs;
				}
				controlPointMain << L"\")]\n";

				// Number of output control points
				controlPointMain << L"[outputcontrolpoints(" << numControlPoints.Value << ")]\n";

				// With all the attributes dealt with, we can emit the actual `main()` routine

				controlPointMain << L"T" << controlPointWorld->OutputType->TypeName << " main(";

				controlPointMain << L"    InputPatch<T" << coarseVertexType->TypeName << L", " << inputControlPointCount.Value << L"> stage_input";
				controlPointMain << L",\n    uint sv_ControlPointID : SV_OutputControlPointID";

				controlPointMain << L")\n{\n";
				controlPointMain << L"T" << controlPointWorld->OutputType->TypeName << " stage_output;\n";
				controlPointMain << ctx.Header.ProduceString() << ctx.Body.ProduceString();
				controlPointMain << L"return stage_output;\n";
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
				ctx.GlobalHeader << L"struct T" << world->OutputType->TypeName << L"\n{\n";

				for (auto & field : world->OutputType->Members)
				{
					codeGen->PrintDef(ctx.GlobalHeader, field.Value.Type.Ptr(), field.Key);
					ctx.GlobalHeader << L";\n";
				}

				ctx.GlobalHeader << L"};\n";
			}
			virtual void ProcessExportInstruction(CodeGenContext & ctx, ExportInstruction * instr) override
			{
				ctx.Body << AddWorldNameSuffix(instr->ComponentName, world->Name) << L"[" << outputIndex << L"] = ";
				codeGen->PrintOp(ctx, instr->Operand.Ptr());
				ctx.Body << L";\n";
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