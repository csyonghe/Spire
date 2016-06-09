#include "Pipeline.h"
#include "CoreLib/WinForm.h"
#include "ModelResource.h"

using namespace CoreLib::Imaging;
using namespace CoreLib::IO;
using namespace CoreLib::Text;
using namespace GL;

using namespace Spire::Compiler;
using namespace SpireLib;

//#define USE_COMMAND_LIST

namespace DemoEngine
{
	RefPtr<DeviceMesh> EnginePipeline::AddDrawableMesh(Mesh pMesh, const Matrix4 & matrix)
	{
		List<unsigned char> meshBuffer;
		RefPtr<DeviceMesh> rs = new DeviceMesh(engine->GetHardwareRenderer());
		rs->Bounds = pMesh.Bounds;
		auto vertBuffer = (VertexData*)pMesh.VertexData.Buffer();
		int vertCount = pMesh.VertexData.Count() / pMesh.GetVertexSize();
		Matrix4 normalMat = matrix;
		normalMat.values[12] = normalMat.values[13] = normalMat.values[14] = 0.0f;
		Matrix4 invTransMat;
		normalMat.Inverse3D(invTransMat);
		invTransMat.Transpose();
		for (int i = 0; i < vertCount; i++)
		{
			auto vert = vertBuffer[i];
			Vec4 pos;
			matrix.Transform(pos, Vec4::Create(vert.Position, 1.0f));
			float inv = 1.0f / pos.w;
			pos *= inv;
			vert.Position = pos.xyz();
			Vec4 normal;
			invTransMat.Transform(normal, Vec4::Create(vert.Normal, 1.0f));
			vert.Normal = normal.xyz();
			Vec4 tangent;
			invTransMat.Transform(tangent, Vec4::Create(vert.Tangent, 1.0f));
			vert.Tangent = tangent.xyz();

			meshBuffer.AddRange((unsigned char *)&vert, sizeof(VertexData));
		}
		for (auto & desc : pMesh.Attributes)
		{
			rs->Attributes.Add(desc);
		}
		rs->BufferSize = meshBuffer.Count();
		rs->MeshBuffer.SetData(meshBuffer.Buffer(), meshBuffer.Count());
		rs->MeshBuffer.MakeResident(true);
		return rs;
	}

	int EnginePipeline::LoadShader(String shaderFileName, String sourceFileName)
	{
		String shaderName = Path::GetFileNameWithoutEXT(shaderFileName);
		int shaderId = 0;
		RefPtr<ShaderLib> shaderLib;
		if (shaderIds.TryGetValue(shaderName, shaderId))
		{
			shaderLib = shaderLibs[shaderId];
		}
		else
		{
			shaderId = shaderLibs.Count();
			shaderLib = new ShaderLib();
			shaderIds[shaderName] = shaderId;
			shaderLibs.Add(shaderLib);
			shaderFileNames[shaderName] = sourceFileName;
			gpuShaders.Add(MaterialContext());
			if (File::Exists(shaderFileName))
				shaderLib->Reload(shaderFileName);
			else if (File::Exists(sourceFileName))
				shaderLib->CompileFrom(shaderName, sourceFileName, L"");
			else
				throw InvalidOperationException(L"cannot find compiled or source shader \'" + shaderName + L"\'.");
			ReloadShader(gpuShaders[shaderId], *shaderLib);
		}
		return shaderId;
	}

	void EnginePipeline::ReloadShader(MaterialContext & ctx, const ShaderLib & shader)
	{
		ctx.Reload(engine->GetHardwareRenderer(), gpuShaderStore, shader);
		for (int i = 0; i < renderPasses.Count(); i++)
			ctx.Programs[i] = renderPasses[i]->LoadProgram(gpuShaderStore, shader);
	}

	RefPtr<DeviceMesh> EnginePipeline::LoadMesh(String meshFileName)
	{
		RefPtr<DeviceMesh> rs;
		printf("loading %s\n", meshFileName.ToMultiByteString());
		if (!meshBuffers.TryGetValue(meshFileName, rs))
		{
			Mesh m;
			m.LoadFromFile(meshFileName);
			
			Matrix4 identity;
			Matrix4::CreateIdentityMatrix(identity);
			rs = AddDrawableMesh(m, identity);
		}
		return rs;
	}
	void EnginePipeline::InitializeShader(int shaderId)
	{
		printf("Initializing material %d\n", shaderId);
		BuildUniformBuffer(shaderId);
		Precompute(shaderId);
		AllocObjectSpaceBuffers(shaderId);
		CompileCommandLists();
	}
	void AlignBuffer(List<unsigned char> & buffer)
	{
		int uniformBufferAlignSize = 16;
		glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &uniformBufferAlignSize);
		if ((buffer.Count() & (uniformBufferAlignSize - 1)))
			buffer.SetSize(buffer.Count() + uniformBufferAlignSize - (buffer.Count() & (uniformBufferAlignSize - 1)));
	}
	void EnginePipeline::BuildUniformBuffer(int shaderId)
	{
		auto & gpuShader = gpuShaders[shaderId];
		auto hw = engine->GetHardwareRenderer();
		if (gpuShader.UniformBuffer.Handle)
			hw->DestroyBuffer(gpuShader.UniformBuffer);
		gpuShader.UniformBuffer = hw->CreateBuffer(BufferUsage::UniformBuffer);
		
		List<unsigned char> buffer;
		for (int i = 0; i < objects.Count(); i++)
		{
			auto & obj = objects[i];
			for (int j = 0; j < obj.Techniques.Count(); j++)
			{
				if (obj.Techniques[j].ShaderId != shaderId)
					continue;
				if (j > 0 && obj.Techniques[j - 1].ShaderId == obj.Techniques[j].ShaderId)
				{
					obj.Techniques[j].UniformRange = obj.Techniques[j - 1].UniformRange;
					obj.Techniques[j].ModelUniformRange = obj.Techniques[j - 1].ModelUniformRange;
					obj.Techniques[j].RootTexUniformRange = obj.Techniques[j - 1].RootTexUniformRange;
					continue;
				}
				auto fillUniformBuffer = [&](String uniformBlockName)
				{
					BufferRange rs;
					if (auto ublock = shaderLibs[obj.Techniques[j].ShaderId]->MetaData.InterfaceBlocks.TryGetValue(uniformBlockName))
					{
						AlignBuffer(buffer);
						
						rs.IndexStart = (uint64_t)buffer.Count();
						int start = (int)rs.IndexStart;
						for (auto & entry : ublock->Entries)
						{
							int ptr = start + entry.Offset;
							buffer.SetSize(ptr);
							UniformValue val;
							if (!obj.Uniforms.TryGetValue(entry.Name, val))
							{
								for (int k = 0; k < 16; k++)
									val.IntValues[k] = 0;
							}

							if (entry.Type != Spire::Compiler::ILBaseType::Float3x3)
							{
								buffer.AddRange((unsigned char*)val.IntValues, entry.Size);
							}
							else
								throw NotImplementedException();
						}
						rs.IndexEnd = (uint64_t)buffer.Count();
					}
					return rs;
				};
				obj.Techniques[j].UniformRange = fillUniformBuffer(L"perInstanceUniform");
				obj.Techniques[j].ModelUniformRange = fillUniformBuffer(L"modelTransform");
				obj.Techniques[j].RootTexUniformRange = fillUniformBuffer(L"rootTex");
			}
		}
		gpuShader.UniformBuffer.SetData(buffer.Buffer(), buffer.Count());
		if (buffer.Count())
		{
			uniformAddr = gpuShader.UniformBuffer.GetGpuAddress();
			gpuShader.UniformBuffer.MakeResident(true);
		}
		else
			uniformAddr = 0;
		for (auto & obj : objects)
		{
			for (auto & tech : obj.Techniques)
			{
				if (tech.ShaderId == shaderId)
				{
					tech.ModelUniformRange.IndexStart += uniformAddr;
					tech.ModelUniformRange.IndexEnd += uniformAddr;
					tech.UniformRange.IndexStart += uniformAddr;
					tech.UniformRange.IndexEnd += uniformAddr;
					tech.RootTexUniformRange.IndexStart += uniformAddr;
					tech.RootTexUniformRange.IndexEnd += uniformAddr;
				}
			}
		}
	}

	List<String> EnginePipeline::GetShaders()
	{
		List<String> shaders;
		for (auto shader : shaderFileNames)
			shaders.Add(shader.Key);
		return shaders;
	}
	String GenerateSchedule(const EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue> & choices, const EnumerableDictionary<String, EnumerableDictionary<String, String>> & attribs)
	{
		StringBuilder sb;
		List<String> lines;
		for (auto & choice : choices)
		{
			lines.Add(choice.Key + L" = \"" + choice.Value.ToString() + L"\";");
		}
		for (auto & attrib : attribs)
		{
			StringBuilder asb;
			asb << L"attrib " << attrib.Key << L" = ";
			int count = 0;
			for (auto & kv : attrib.Value)
			{
				asb << kv.Key << L": \"" << kv.Value << L"\"";
				count++;
				if (count != attrib.Value.Count())
					asb << L", ";
			}
			asb << L";";
			lines.Add(asb.ToString());
		}
		for (int i = lines.Count() - 1; i >= 0; i--)
			sb << lines[i] << "\n";
		return sb.ProduceString();
	}
	List<ShaderChoice> EnginePipeline::GetChoices(String shaderName, const EnumerableDictionary<String, Spire::Compiler::ShaderChoiceValue>& existingChoices)
	{
		CompileOptions options;
		CompileResult result;
		options.ScheduleSource = GenerateSchedule(existingChoices, EnumerableDictionary<String, EnumerableDictionary<String, String>>());
		options.Mode = Spire::Compiler::CompilerMode::GenerateChoice;
		options.SymbolToCompile = shaderName;
		String shaderFileName;
		if (shaderFileNames.TryGetValue(shaderName, shaderFileName))
		{
			SpireLib::CompileShaderSourceFromFile(result, Path::ReplaceExt(shaderFileName, L"shader"), options);
			result.PrintError();
		}
		return result.Choices;
	}
	void EnginePipeline::RecompileShader(String shaderName, String schedule)
	{
		engine->GetHardwareRenderer()->Finish();
		int shaderId = shaderIds[shaderName];
		shaderLibs[shaderId]->CompileFrom(shaderName, Path::ReplaceExt(shaderFileNames[shaderName].GetValue(), L"shader"), schedule);
		ReloadShader(gpuShaders[shaderId], *shaderLibs[shaderId]);
		InitializeShader(shaderId);
	}
	BBox EnginePipeline::GetBounds()
	{
		BBox rs;
		rs.Init();
		for (auto & inst : objects)
		{
			rs.Union(inst.Bounds);
		}
		return rs;
	}

	void EnginePipeline::SetFoveatedRendering(bool enable)
	{
		enableFoveatedRendering = enable;
		CompileCommandLists();
	}
	
	void EnginePipeline::SetUseShadowMap(bool enable)
	{
		shadowMap->Enabled = enable;
		for (int shaderId = 0; shaderId < gpuShaders.Count(); shaderId++)
		{
			ReloadShader(gpuShaders[shaderId], *shaderLibs[shaderId]);
			InitializeShader(shaderId);
		}
	}

	void EnginePipeline::CompileCommandLists()
	{
		FreeCommandLists();
		auto hw = engine->GetHardwareRenderer();
		bounds = GetBounds();
		sceneGridObjLists.SetSize(sceneGridSize * sceneGridSize);
		renderPassContexts.SetSize(renderPasses.Count());
		Array<SizeI, 16> viewports;
		for (int p = 0; p < renderPasses.Count(); p++)
		{
			renderPassContexts[p].CompiledCommandLists.SetSize(sceneGridSize * sceneGridSize);
			viewports.Add(renderPasses[p]->GetViewport());
		}
		engine->GetHardwareRenderer()->SetZTestMode(BlendOperator::LessEqual);
		hw->SetClearColor(Vec4::Create(58 / 255.0f, 131 / 255.0f, 198 / 255.0f, 1.0));
		float cellSizeX = (bounds.xMax - bounds.xMin) / sceneGridSize;
		float cellSizeZ = (bounds.zMax - bounds.zMin) / sceneGridSize;
		for (int i = 0; i < sceneGridSize; i++)
		{
			for (int j = 0; j < sceneGridSize; j++)
			{
				BBox cellBounds;
				cellBounds.Min = Vec3::Create(bounds.xMin + cellSizeX * i, bounds.yMin,
					bounds.zMin + cellSizeZ * j);
				cellBounds.Max = Vec3::Create(cellBounds.xMin + cellSizeX, bounds.yMax, cellBounds.zMin + cellSizeZ);
				auto & objList = sceneGridObjLists[j*sceneGridSize + i];
				objList.Clear();
				int lodSize = 0;
				for (auto & obj : objects)
				{
					if (cellBounds.ContainsNoOverlap((obj.Bounds.Min + obj.Bounds.Max)*0.5f))
					{
						objList.Add(&obj);
						lodSize = Math::Max(lodSize, obj.Techniques.Count());
					}
				}
				for (int p = 0; p < renderPasses.Count(); p++)
					renderPassContexts[p].CompiledCommandLists[j*sceneGridSize + i].SetSize(lodSize);
#ifdef USE_COMMAND_LIST	
				for (int l = 0; l < lodSize; l++)
				{
					for (int p = 0; p < renderPasses.Count(); p++)
					{
						auto & commandLists = renderPassContexts[p].CompiledCommandLists[j*sceneGridSize + i];
						GL::Program lastHandle;
						GL::CommandListBuilder cmdBuilder;
						cmdBuilder.SetSegments(1);
						GL::CommandBuffer *cmdBuffer = nullptr;
						commandLists[l].Clear();
						bool nonEmpty = false;
						for (auto obj : objList)
						{
							auto & tech = obj->Techniques[Math::Min(l, obj->Techniques.Count() - 1)];
							auto program = gpuShaders[tech.ShaderId].Programs[p];
							if (program.Handle != lastHandle.Handle)
							{
								lastHandle = program;
								auto fb = renderPasses[p]->GetFramebuffer();
								auto renderTargetMask = renderPasses[p]->GetRenderTargetMask(tech);
								StencilMode stencilMode;
								if (enableFoveatedRendering)
								{
									if (p == 1)
									{
										stencilMode.Bits = 8;
										stencilMode.StencilFunc = CompareFunc::Equal;
										stencilMode.StencilMask = 255;
										stencilMode.DepthFail = stencilMode.DepthPass = stencilMode.Fail = StencilOp::Keep;
										stencilMode.StencilReference = 1;
									}
									else if (p == 2)
									{
										stencilMode.Bits = 8;
										stencilMode.StencilFunc = CompareFunc::NotEqual;
										stencilMode.StencilMask = 255;
										stencilMode.DepthFail = stencilMode.DepthPass = stencilMode.Fail = StencilOp::Keep;
										stencilMode.StencilReference = 1;
									}
								}
								hw->SetStencilMode(stencilMode);
								fb.EnableRenderTargets(renderTargetMask);
								hw->SetCullMode(CullMode::CullBackFace);
								hw->SetViewport(0, 0, viewports[p].Width, viewports[p].Height);
								hw->SetWriteFrameBuffer(fb);
								tech.PrebakedMesh->ConfigVertexArray();
								program.Use();
								renderPasses[p]->SetRenderState(hw);
								auto state = hw->CaptureState(PrimitiveType::Triangles);
								stateObjects.Add(state);
								cmdBuffer = cmdBuilder.NewCommandBuffer(0, fb, state);
								cmdBuffer->Viewport(0, 0, viewports[p].Width, viewports[p].Height);
								CompiledCommands compiledList;
								compiledList.Program = lastHandle;
								compiledList.RenderTargets = renderTargetMask;
								commandLists[l].Add(compiledList);
								if (!drawCommandSortBuffer.ContainsKey(lastHandle.Handle))
								{
									drawCommandSortBuffer[lastHandle.Handle] = List<CompiledCommands>();
								}
							}
							if (program.Handle)
							{
								cmdBuffer->UniformAddress(1, ShaderType::VertexShader, viewUniformAddr);
								cmdBuffer->UniformAddress(1, ShaderType::FragmentShader, viewUniformAddr);

								cmdBuffer->UniformAddress(2, ShaderType::VertexShader, tech.UniformRange.IndexStart);
								cmdBuffer->UniformAddress(2, ShaderType::FragmentShader, tech.UniformRange.IndexStart);

								cmdBuffer->UniformAddress(3, ShaderType::VertexShader, tech.PrecomputeTexUniformRange.IndexStart);
								cmdBuffer->UniformAddress(3, ShaderType::FragmentShader, tech.PrecomputeTexUniformRange.IndexStart);

								cmdBuffer->UniformAddress(4, ShaderType::VertexShader, tech.RootTexUniformRange.IndexStart);
								cmdBuffer->UniformAddress(4, ShaderType::FragmentShader, tech.RootTexUniformRange.IndexStart);

								cmdBuffer->UniformAddress(5, ShaderType::VertexShader, tech.ObjTexUniformRange.IndexStart);
								cmdBuffer->UniformAddress(5, ShaderType::FragmentShader, tech.ObjTexUniformRange.IndexStart);

								cmdBuffer->UniformAddress(6, ShaderType::VertexShader, tech.LowResUniformRange.IndexStart);
								cmdBuffer->UniformAddress(6, ShaderType::FragmentShader, tech.LowResUniformRange.IndexStart);

								cmdBuffer->UniformAddress(7, ShaderType::VertexShader, tech.PrecomputeUniformRange.IndexStart);
								cmdBuffer->UniformAddress(7, ShaderType::FragmentShader, tech.PrecomputeUniformRange.IndexStart);

								cmdBuffer->UniformAddress(0, ShaderType::VertexShader, tech.ModelUniformRange.IndexStart);
								cmdBuffer->UniformAddress(0, ShaderType::FragmentShader, tech.ModelUniformRange.IndexStart);

								shadowMap->BindShadowMapResources(*cmdBuffer);

								tech.PrebakedMesh->Draw(*cmdBuffer);
								nonEmpty = true;
							}
						}
						if (nonEmpty)
						{
							List<GL::CommandList> lists;
							cmdBuilder.CompileInto(lists);
							for (int k = 0; k < lists.Count(); k++)
							{
								commandLists[l][k].Commands = lists[k];
							}
						}
					}
				}
#endif
			}
		}
	}
	void EnginePipeline::Initialize(DeviceResourcePool * pEngine)
	{
		engine = pEngine;
		gpuShaderStore.Init(engine->GetHardwareRenderer());
		ViewUniformBuffer = pEngine->GetHardwareRenderer()->CreateBuffer(BufferUsage::UniformBuffer);
		ViewUniformBufferContent bufferContent;
		bufferContent.CameraPos.SetZero();
		Matrix4::CreateIdentityMatrix(bufferContent.ViewProjectionMatrix);
		Matrix4::CreateIdentityMatrix(bufferContent.ViewMatrix);
		Matrix4::CreateIdentityMatrix(bufferContent.InvViewMatrix);
		Matrix4::CreateIdentityMatrix(bufferContent.InvViewProjectionMatrix);
		bufferContent.LightDir = Vec4::Create(0.0f, 1.0f, 0.0f, 0.0f);
		bufferContent.LightColor = Vec4::Create(1.0f);

		bufferContent.ViewProjectionMatrix.values[0] = 1.0f;
		bufferContent.ViewProjectionMatrix.values[1] = 2.0f;

		ViewUniformBuffer.SetData(&bufferContent, sizeof(bufferContent));
		ViewUniformBuffer.MakeResident(true);
		viewUniformAddr = ViewUniformBuffer.GetGpuAddress();

		glEnableClientState(GL_UNIFORM_BUFFER_UNIFIED_NV);
		glEnableClientState(GL_VERTEX_ATTRIB_ARRAY_UNIFIED_NV);

		auto texDData = CoreLib::Graphics::ComputeTextureD(1.0f, 512);
		auto texFVData = CoreLib::Graphics::ComputeTextureFV(1.0f, 512);
		auto texD = engine->LoadTexture2D(L"texGGX_D", StorageFormat::Float16, 512, 512, 1, DataType::Float, texDData.Buffer());
		auto texFV = engine->LoadTexture2D(L"texGGX_FV", StorageFormat::RG_F16, 512, 512, 1, DataType::Float2, texFVData.Buffer());
		texGGX_D = texD.GetTextureHandle(engine->GetPresetSampler(PresetSamplers::LinearClamp));
		texGGX_FV = texFV.GetTextureHandle(engine->GetPresetSampler(PresetSamplers::LinearClamp));
		texGGX_D.MakeResident();
		texGGX_FV.MakeResident();

		mipmapBuilder.Init(engine);

		shadowMap = CreateCascadedShadowMapAlgorithm(engine);
		shadowMap->SetShadowMapSize(shadowMapSize);
		renderPasses.Add(CreateLowResRenderPass(engine, shadowMap.Ptr()));
		renderPasses.Add(CreateMainRenderPass(engine, shadowMap.Ptr()));
		renderPasses.Add(CreateLowQualityRenderPass(engine, shadowMap.Ptr()));
		int shadowPassStartId = renderPasses.Count();
		renderPasses.AddRange(shadowMap->GetRenderPasses());
		FrameResized();
		shadowMap->DrawShadowMap = [this, shadowPassStartId](const Matrix4 & projMatrix, int passId)
		{
			ViewUniformBufferContent content;
			content.ViewProjectionMatrix = content.ProjectionMatrix = projMatrix;
			Matrix4::CreateIdentityMatrix(content.ViewMatrix);
			content.InvViewMatrix = content.ViewMatrix;
			content.InvViewProjectionMatrix = content.ViewMatrix;
			content.texGGX_D = texGGX_D.Handle;
			content.texGGX_FV = texGGX_FV.Handle;
			ViewUniformBuffer.SubData(0, sizeof(content), &content);
#ifdef USE_COMMAND_LIST
			SubmitGeometryCommandList(shadowPassStartId + passId);
#else
			SubmitGeometry(shadowPassStartId + passId);
#endif
		};
	}

	UniformValue ParseUniform(String & name, String dir, DeviceResourcePool * engine, CoreLib::Text::Parser & parser)
	{
		auto parseTextureStorage = [](CoreLib::Text::Parser & parser)
		{
			StorageFormat storage = StorageFormat::RGBA_I8;
			auto storageStr = parser.ReadToken().Str;
			if (storageStr == L"RGB8")
				storage = StorageFormat::RGB_I8;
			else if (storageStr == L"RGB16F")
				storage = StorageFormat::RGB_F16;
			else if (storageStr == L"RGBA16F")
				storage = StorageFormat::RGBA_F16;
			else if (storageStr == L"RGB10_A2")
				storage = StorageFormat::RGB10_A2;
			else if (storageStr == L"R8")
				storage = StorageFormat::Int8;
			else if (storageStr == L"RG8")
				storage = StorageFormat::RG_I8;
			else if (storageStr == L"RGBA8")
				storage = StorageFormat::RGBA_I8;
			else if (storageStr == L"CompressedRGBA8")
				storage = StorageFormat::RGBA_Compressed;
			return storage;
		};
		parser.ReadToken();
		auto type = parser.ReadToken().Str;
		name = parser.ReadToken().Str;
		UniformValue val;
		if (type == L"float")
		{
			val.Type = UniformType::ConstantFloat;
			parser.Read(L"=");
			val.FloatValues[0] = (float)parser.ReadDouble();
		}
		else if (type == L"vec2")
		{
			val.Type = UniformType::ConstantFloat2;
			parser.Read(L"=");
			parser.Read(L"(");
			val.FloatValues[0] = (float)parser.ReadDouble();
			parser.Read(L",");
			val.FloatValues[1] = (float)parser.ReadDouble();
			parser.Read(L")");
		}
		else if (type == L"vec3")
		{
			val.Type = UniformType::ConstantFloat3;
			parser.Read(L"=");
			parser.Read(L"(");
			val.FloatValues[0] = (float)parser.ReadDouble();
			parser.Read(L",");
			val.FloatValues[1] = (float)parser.ReadDouble();
			parser.Read(L",");
			val.FloatValues[2] = (float)parser.ReadDouble();
			parser.Read(L")");
		}
		else if (type == L"vec4")
		{
			val.Type = UniformType::ConstantFloat4;
			parser.Read(L"=");
			parser.Read(L"(");
			val.FloatValues[0] = (float)parser.ReadDouble();
			parser.Read(L",");
			val.FloatValues[1] = (float)parser.ReadDouble();
			parser.Read(L",");
			val.FloatValues[2] = (float)parser.ReadDouble();
			parser.Read(L",");
			val.FloatValues[3] = (float)parser.ReadDouble();
			parser.Read(L")");
		}
		else if (type == L"int")
		{
			val.Type = UniformType::ConstantInt;
			parser.Read(L"=");
			val.IntValues[0] = parser.ReadInt();
		}
		else if (type == L"ivec2")
		{
			val.Type = UniformType::ConstantInt2;
			parser.Read(L"=");
			parser.Read(L"(");
			val.IntValues[0] = parser.ReadInt();
			parser.Read(L",");
			val.IntValues[1] = parser.ReadInt();
			parser.Read(L")");
		}
		else if (type == L"ivec3")
		{
			val.Type = UniformType::ConstantInt3;
			parser.Read(L"=");
			parser.Read(L"(");
			val.IntValues[0] = parser.ReadInt();
			parser.Read(L",");
			val.IntValues[1] = parser.ReadInt();
			parser.Read(L",");
			val.IntValues[2] = parser.ReadInt();
			parser.Read(L")");
		}
		else if (type == L"ivec4")
		{
			val.Type = UniformType::ConstantInt4;
			parser.Read(L"=");
			parser.Read(L"(");
			val.IntValues[0] = parser.ReadInt();
			parser.Read(L",");
			val.IntValues[1] = parser.ReadInt();
			parser.Read(L",");
			val.IntValues[2] = parser.ReadInt();
			parser.Read(L",");
			val.IntValues[3] = parser.ReadInt();
			parser.Read(L")");
		}
		else if (type == L"mat4")
		{
			val.Type = UniformType::ConstantMatrix4;
			parser.Read(L"=");
			parser.Read(L"(");
			for (int i = 0; i < 16; i++)
			{
				val.FloatValues[i] = (float)parser.ReadDouble();
				if (parser.LookAhead(L","))
					parser.Read(L",");
			}
			parser.Read(L")");
		}
		else if (type == L"texture2D")
		{
			val.Type = UniformType::Texture2D;
			parser.Read(L"=");
			auto textureName = parser.ReadStringLiteral();
			StorageFormat storage = StorageFormat::RGBA_I8;
			if (parser.LookAhead(L","))
			{
				parser.Read(L",");
				storage = parseTextureStorage(parser);
			}
			printf("loading %s\n", textureName.ToMultiByteString());

			textureName = Path::Combine(dir, textureName);
			val.TextureName = textureName;
			auto tex = engine->LoadTexture2D(textureName, storage);
			val.TextureHandle = tex.GetTextureHandle(engine->GetPresetSampler(PresetSamplers::Aniso16xRepeat));
			val.TextureHandle.MakeResident();
		}
		else if (type == L"textureCube")
		{
			Array<String, 6> faces;
			parser.Read(L"=");
			parser.Read(L"(");
			printf("loading %s\n", name.ToMultiByteString());
			for (int i = 0; i < 6; i++)
			{
				auto textureName = parser.ReadStringLiteral();
				textureName = Path::Combine(dir, textureName);
				faces.Add(textureName);
				if (i != 5)
					parser.Read(L",");
			}
			parser.Read(L")");
			StorageFormat storage = StorageFormat::RGBA_I8;
			if (parser.LookAhead(L","))
			{
				parser.Read(L",");
				storage = parseTextureStorage(parser);
			}
			auto tex = engine->LoadTextureCube(faces.GetArrayView(), storage);
			val.TextureHandle = tex.GetTextureHandle(engine->GetPresetSampler(PresetSamplers::Aniso16xRepeat));
			val.TextureHandle.MakeResident();
			val.TextureName = faces[0];
			val.Type = UniformType::TextureCube;
		}

		parser.Read(L";");
		return val;
	}

	void EnginePipeline::LoadFromFile(String fileName)
	{
		auto dir = Path::GetDirectoryName(fileName);
		CoreLib::Text::Parser parser(File::ReadAllText(fileName));
		Random random(381723);
		while (!parser.IsEnd())
		{
			auto fieldName = parser.ReadWord();
			if (fieldName == L"terrain")
			{
				DrawableInstance obj;
				auto heightMap = parser.ReadStringLiteral();
				cellSpace = (float)parser.ReadDouble();
				auto heightScale = (float)parser.ReadDouble();

				terrain.Init(engine->GetHardwareRenderer(), cellSpace, heightScale, heightMap);
				
				parser.Read(L"{");
				while (!parser.LookAhead(L"}"))
				{
					if (parser.LookAhead(L"shader"))
					{
						parser.ReadToken();
						auto shaderName = parser.ReadStringLiteral();
						String srcName;
						if (parser.LookAhead(L"src"))
						{
							parser.ReadToken();
							srcName = Path::Combine(dir, parser.ReadStringLiteral());
							shaderFileNames[Path::GetFileNameWithoutEXT(shaderName)] = srcName;
						}
						int shaderId = LoadShader(Path::Combine(dir, shaderName), srcName);
						obj.Techniques.Add(ShadingTechnique(shaderId));
						if (parser.LookAhead(L","))
						{
							parser.ReadToken();
							obj.Techniques.Last().LodDistance = (float)parser.ReadDouble();
						}
					}
					else if (parser.LookAhead(L"uniform"))
					{
						String name;
						auto val = ParseUniform(name, dir, engine, parser);
						obj.Uniforms[name] = val;
					}
					else
						parser.ReadToken();
					obj.Position.SetZero();
				}
				parser.Read(L"}");
				int blockId = 0;
				for (auto & block : terrain.Blocks)
				{
					DrawableInstance blockInst = obj;
					for (int t = 0; t < blockInst.Techniques.Count(); t++)
					{
						blockInst.Techniques[t].SourceMesh = block.RangeLods[Math::Min(t, block.RangeLods.Count() - 1)];
					}
					blockInst.Position = (block.Bounds.Max + block.Bounds.Min) * 0.5f;
					blockInst.Bounds = block.Bounds;
					Matrix4 identity;
					Matrix4::CreateIdentityMatrix(identity);
					UniformValue val;
					val.Type = UniformType::ConstantMatrix4;
					for (int j = 0; j < 16; j++)
						val.FloatValues[j] = identity.values[j];
					blockInst.Uniforms[L"normalMatrix"] = val;
					blockInst.Uniforms[L"uv0"] = UniformValue(UniformType::ConstantFloat2, block.UV0);
					blockInst.Uniforms[L"uv_size"] = UniformValue(UniformType::ConstantFloat2, block.UVSize);
					blockInst.GUID = L"TB" + String(blockId++);
					objects.Add(blockInst);
				}
			}
			else if (fieldName == L"mesh")
			{
				DrawableInstance obj;
				auto meshName = parser.ReadStringLiteral();
				auto mesh = LoadMesh(meshName);
				obj.Position.SetZero();
				parser.Read(L"{");
				while (!parser.LookAhead(L"}"))
				{
					if (parser.LookAhead(L"shader"))
					{
						parser.ReadToken();
						auto shaderName = parser.ReadStringLiteral();
						String srcName;
						if (parser.LookAhead(L"src"))
						{
							parser.ReadToken();
							srcName = Path::Combine(dir, parser.ReadStringLiteral());
							shaderFileNames[Path::GetFileNameWithoutEXT(shaderName)] = srcName;
						}
						int shaderId = LoadShader(Path::Combine(dir, shaderName), srcName);
						obj.Techniques.Add(ShadingTechnique(shaderId));
						obj.Techniques.Last().SourceMesh = mesh;
						if (parser.LookAhead(L","))
						{
							parser.ReadToken();
							obj.Techniques.Last().LodDistance = (float)parser.ReadDouble();
						}
					}
					else if (parser.LookAhead(L"instance"))
					{
						DrawableInstance newInst = obj;
						parser.ReadToken();
						parser.Read(L"{");
						Matrix4 modelView;
						Matrix4::CreateIdentityMatrix(modelView);
						while (!parser.LookAhead(L"}"))
						{
							String name;
							auto val = ParseUniform(name, dir, engine, parser);
							newInst.Uniforms[name] = val;
							if (name == L"modelMatrix")
							{
								newInst.Position.x = val.FloatValues[12];
								newInst.Position.y = val.FloatValues[13];
								newInst.Position.z = val.FloatValues[14];
								for (int i = 0; i < 16; i++)
									modelView.values[i] = val.FloatValues[i];
							}
						}
						parser.Read(L"}");
						UniformValue val;
						val.Type = UniformType::ConstantMatrix4;
						for (int j = 0; j < 16; j++)
							val.FloatValues[j] = modelView.values[j];
						newInst.Uniforms[L"modelMatrix"] = val;
						val.FloatValues[12] = val.FloatValues[13] = val.FloatValues[14] = 0.0f;
						newInst.Uniforms[L"normalMatrix"] = val;
						TransformBBox(newInst.Bounds, modelView, mesh->Bounds);
						StringBuilder guidGen;
						guidGen << meshName << L"|";
						for (auto &uniform : newInst.Uniforms)
							if (uniform.Key != L"modelMatrix" && uniform.Key != L"normalMatrix")
								guidGen << uniform.Value.ToString();
						newInst.GUID = guidGen.ProduceString();
						objects.Add(newInst);
					}
					else if (parser.LookAhead(L"procedural"))
					{
						parser.ReadToken();
						float xMin = (float)parser.ReadDouble();
						float xMax = (float)parser.ReadDouble();
						float zMin = (float)parser.ReadDouble();
						float zMax = (float)parser.ReadDouble();
						int count = parser.ReadInt();
						parser.Read(L"{");
						Matrix4 modelView;
						Matrix4::CreateIdentityMatrix(modelView);
						DrawableInstance objInst = obj;
						while (!parser.LookAhead(L"}"))
						{
							String name;
							auto val = ParseUniform(name, dir, engine, parser);
							objInst.Uniforms[name] = val;
						}
						parser.Read(L"}");
						for (int i = 0; i < count; i++)
						{
							float posX = random.NextFloat(xMin, xMax);
							float posZ = random.NextFloat(zMin, zMax);
							float posY = terrain.GetAltitude(Vec3::Create(posX, 0.0f, posZ));
							float rotY = random.NextFloat(0.0f, Math::Pi*2.0f);

							Matrix4::RotationY(modelView, rotY);
							modelView.values[12] = posX;
							modelView.values[13] = posY;
							modelView.values[14] = posZ;
							DrawableInstance newInst = objInst;
							UniformValue val;
							val.Type = UniformType::ConstantMatrix4;
							for (int j = 0; j < 16; j++)
								val.FloatValues[j] = modelView.values[j];
							newInst.Uniforms[L"modelMatrix"] = val;
							newInst.Uniforms[L"normalMatrix"] = val;
							newInst.Position = Vec3::Create(posX, posY, posZ);
							TransformBBox(newInst.Bounds, modelView, mesh->Bounds);
							StringBuilder guidGen;
							guidGen << meshName << L"|";
							for (auto &uniform : newInst.Uniforms)
								if (uniform.Key != L"modelMatrix" && uniform.Key != L"normalMatrix")
									guidGen << uniform.Value.ToString();
							newInst.GUID = guidGen.ProduceString();
							objects.Add(newInst);
						}
					}
					
					else
						parser.ReadToken();
				}
				parser.Read(L"}");
			}
			else if (fieldName == L"sky")
			{
				background = new Background(engine, parser.ReadStringLiteral());
			}
		}

		for (int i = 0; i < gpuShaders.Count(); i++)
		{
			BuildUniformBuffer(i);
			AllocObjectSpaceBuffers(i);
			Precompute(i);
		}
		int totalTextures = 0;
		for (auto & s : gpuShaders)
			totalTextures += s.PrebakedTextures.Count();
		printf("%d textures baked.\n", totalTextures);
		CompileCommandLists();
	}

	void EnginePipeline::UpdateSysUniform(SystemUniforms & sysUniform, int viewId)
	{
		ViewUniformBufferContent viewUniformContent;
		auto & view = sysUniform.Views[viewId];
		viewUniformContent.ProjectionMatrix = view.GetProjectionTransform();
		viewUniformContent.ViewMatrix = view.GetViewTransform();
		viewUniformContent.ViewProjectionMatrix = viewUniformContent.ProjectionMatrix * viewUniformContent.ViewMatrix;
		viewUniformContent.ViewMatrix.Inverse(viewUniformContent.InvViewMatrix);
		viewUniformContent.ViewProjectionMatrix.Inverse(viewUniformContent.InvViewProjectionMatrix);
		viewUniformContent.CameraPos = Vec4::Create(view.CamPos, 1.0f);
		viewUniformContent.LightDir = Vec4::Create(sysUniform.LightDir, 1.0f);
		viewUniformContent.LightColor = Vec4::Create(sysUniform.LightColor, 1.0f);
		viewUniformContent.LightParams = Vec4::Create(0.0f);
		viewUniformContent.texGGX_D = texGGX_D.Handle;
		viewUniformContent.texGGX_FV = texGGX_FV.Handle;
		viewUniformContent.Time = sysUniform.Time;
		ViewUniformBuffer.SubData(0, sizeof(ViewUniformBufferContent), &viewUniformContent);
		camPos = view.CamPos;
		this->sysUniforms = sysUniform;
	}

	void EnginePipeline::SubmitGeometryCommandList(int programIndex)
	{
		float cellSizeX = (bounds.xMax - bounds.xMin) / sceneGridSize;
		float cellSizeZ = (bounds.zMax - bounds.zMin) / sceneGridSize;
		auto &commandLists = renderPassContexts[programIndex].CompiledCommandLists;
		auto fb = renderPasses[programIndex]->GetFramebuffer();
		for (auto & list : drawCommandSortBuffer)
			list.Value.Clear();
		for (int i = 0; i < sceneGridSize; i++)
		{
			for (int j = 0; j < sceneGridSize; j++)
			{
				BBox cellBounds;
				cellBounds.Min = Vec3::Create(bounds.xMin + cellSizeX * i, bounds.yMin,
					bounds.zMin + cellSizeZ * j);
				cellBounds.Max = Vec3::Create(cellBounds.xMin + cellSizeX, bounds.yMax, cellBounds.zMin + cellSizeZ);
				auto dist = cellBounds.Distance(camPos);
				int lod = 0;
				if (dist > 12000.0)
					lod = 4;
				else if (dist > 9000.0)
					lod = 3;
				else if (dist > 2500.0)
					lod = 2;
				else if (dist > 1000.0)
					lod = 1;
				if (SelectedLOD >= 0)
					lod = SelectedLOD;
				lod = Math::Min(lod, commandLists[j * sceneGridSize + i].Count() - 1);
				if (lod >= 0)
				{
					auto & cmdBuf = commandLists[j*sceneGridSize + i][lod];
					for (auto & cmd : cmdBuf)
					{
						drawCommandSortBuffer[cmd.Program.Handle].GetValue().Add(cmd);
					}
				}
			}
		}

		for (auto & list : drawCommandSortBuffer)
		{
			if (list.Value.Count())
			{
				fb.EnableRenderTargets(list.Value.First().RenderTargets);
			}
			for (auto & cmd : list.Value)
			{
				cmd.Commands.Execute();
				CommandListsDrawn++;
			}
		}
	}

	void EnginePipeline::SubmitGeometry(int programIndex)
	{
		float cellSizeX = (bounds.xMax - bounds.xMin) / sceneGridSize;
		float cellSizeZ = (bounds.zMax - bounds.zMin) / sceneGridSize;
		auto viewportSize = renderPasses[programIndex]->GetViewport();
		shadowMap->BindShadowMapResources();
		auto fb = renderPasses[programIndex]->GetFramebuffer();
		for (int i = 0; i < sceneGridSize; i++)
		{
			for (int j = 0; j < sceneGridSize; j++)
			{
				BBox cellBounds;
				cellBounds.Min = Vec3::Create(bounds.xMin + cellSizeX * i, bounds.yMin,
					bounds.zMin + cellSizeZ * j);
				cellBounds.Max = Vec3::Create(cellBounds.xMin + cellSizeX, bounds.yMax, cellBounds.zMin + cellSizeZ);
				auto dist = cellBounds.Distance(camPos);
				int lod = 0;
				if (dist > 12000.0)
					lod = 4;
				else if (dist > 9000.0)
					lod = 3;
				else if (dist > 2500.0)
					lod = 2;
				else if (dist > 1000.0)
					lod = 1;
				if (SelectedLOD >= 0)
					lod = SelectedLOD;
				if (lod >= 0)
				{
					auto &objList = sceneGridObjLists[j*sceneGridSize + i];
					auto hw = engine->GetHardwareRenderer();
					GLuint lastHandle = 0;
					for (auto obj : objList)
					{
						auto & tech = obj->Techniques[Math::Min(lod, obj->Techniques.Count() - 1)];
						auto program = gpuShaders[tech.ShaderId].Programs[programIndex];
						if (program.Handle == 0)
							continue;
						if (program.Handle != lastHandle)
						{
							hw->SetWriteFrameBuffer(fb);
							fb.EnableRenderTargets(renderPasses[programIndex]->GetRenderTargetMask(tech));
							hw->SetCullMode(CullMode::CullBackFace);
							hw->SetViewport(0, 0, viewportSize.Width, viewportSize.Height);
							lastHandle = program.Handle;
							program.Use();
						}
						glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, 0, tech.ModelUniformRange.IndexStart,
							(GLsizeiptr)(tech.ModelUniformRange.IndexEnd - tech.ModelUniformRange.IndexStart));
						glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, 1, viewUniformAddr, (GLsizeiptr)sizeof(ViewUniformBufferContent));
						glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, 2, tech.UniformRange.IndexStart,
							(GLsizeiptr)(tech.UniformRange.IndexEnd - tech.UniformRange.IndexStart));
						glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, 3, tech.PrecomputeTexUniformRange.IndexStart,
							(GLsizeiptr)(tech.PrecomputeTexUniformRange.IndexEnd - tech.PrecomputeTexUniformRange.IndexStart));
						glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, 4, tech.RootTexUniformRange.IndexStart,
							(GLsizeiptr)(tech.RootTexUniformRange.IndexEnd - tech.RootTexUniformRange.IndexStart));
						glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, 5, tech.ObjTexUniformRange.IndexStart,
							(GLsizeiptr)(tech.ObjTexUniformRange.IndexEnd - tech.ObjTexUniformRange.IndexStart));
						glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, 6, tech.LowResUniformRange.IndexStart,
							(GLsizeiptr)(tech.LowResUniformRange.IndexEnd - tech.LowResUniformRange.IndexStart));
						glBufferAddressRangeNV(GL_UNIFORM_BUFFER_ADDRESS_NV, 7, tech.PrecomputeUniformRange.IndexStart,
							(GLsizeiptr)(tech.PrecomputeUniformRange.IndexEnd - tech.PrecomputeUniformRange.IndexStart));
						
						tech.PrebakedMesh->Draw();
					}
				}
			}
		}
	}

	void EnginePipeline::Draw(bool stereo)
	{
		auto hw = engine->GetHardwareRenderer();
		auto blitFB = [&](GL::FrameBuffer fb, int x, int y, int w, int h)
		{
			hw->SetReadFrameBuffer(fb);
			hw->SetWriteFrameBuffer(GL::FrameBuffer());
			glBlitFramebuffer(0, 0, engine->GetScreenWidth(), engine->GetScreenHeight(),
				x, y, x+w, y+h, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		};
		auto frameBuffer = GetMainRenderPass()->GetFramebuffer();

		if (shadowMap->Enabled)
			shadowMap->GenerateShadowMap(sysUniforms.LightDir, bounds, sysUniforms.Views[0]);

		if (stereo)
		{
			int w = engine->GetScreenWidth();
			int h = engine->GetScreenHeight();
			DrawMainPass();
			blitFB(frameBuffer, 0, h/4, w/2, h/2);
			UpdateSysUniform(sysUniforms, 1);
			DrawMainPass();
			blitFB(frameBuffer, w/2, h / 4, w / 2, h / 2);
			UpdateSysUniform(sysUniforms, 0);
		}
		else
		{
			UpdateSysUniform(sysUniforms, 0);
			DrawMainPass();
			blitFB(frameBuffer, 0, 0, engine->GetScreenWidth(), engine->GetScreenHeight());
		}
	}

	void EnginePipeline::DrawMainPass()
	{
		auto hw = engine->GetHardwareRenderer();
		hw->SetZTestMode(BlendOperator::LessEqual);
		CommandListsDrawn = 0;
		
		StencilMode stencilGenMode;
		
		for (int p = 0; p < 3; p++)
		{
			auto viewportSize = renderPasses[p]->GetViewport();
			auto timePoint = CoreLib::Diagnostics::PerformanceCounter::Start();
			GL::FrameBuffer fb = renderPasses[p]->GetFramebuffer();
			hw->SetWriteFrameBuffer(fb);
			hw->SetViewport(0, 0, viewportSize.Width, viewportSize.Height);
			if (p == 1)
			{
				hw->Clear(true, true, true);
				if (background)
				{
					stencilGenMode.StencilFunc = CompareFunc::Disabled;
					stencilGenMode.StencilMask = 0;
					stencilGenMode.Bits = 8;
					hw->SetStencilMode(stencilGenMode);
					background->Draw(engine, sysUniforms.Views[0]);
				}
				if (enableFoveatedRendering)
				{
					stencilGenMode.Bits = 8;
					stencilGenMode.StencilFunc = CompareFunc::Always;
					stencilGenMode.StencilMask = 255;
					stencilGenMode.DepthFail = stencilGenMode.DepthPass = stencilGenMode.Fail = StencilOp::Replace;
					stencilGenMode.StencilReference = 1;
					hw->SetColorMask(false, false, false, false);
					hw->SetDepthMask(false);
					hw->SetStencilMode(stencilGenMode);
					engine->DrawScreenSpaceCircle((float)FoveaX, (float)FoveaY, (float)FoveaRad);
					stencilGenMode.StencilFunc = CompareFunc::Equal;
					stencilGenMode.DepthFail = stencilGenMode.DepthPass = stencilGenMode.Fail = StencilOp::Keep;
					hw->SetStencilMode(stencilGenMode);
					hw->SetColorMask(true, true, true, true);
					hw->SetDepthMask(true);

				}
			}
			else if (p == 2)
			{
				if (!enableFoveatedRendering)
					break;
				stencilGenMode.StencilFunc = CompareFunc::NotEqual;
				hw->SetStencilMode(stencilGenMode);
			}
			else
			{
				hw->Clear(true, true, false);
			}
#ifdef USE_COMMAND_LIST
			SubmitGeometryCommandList(p);
#else
			SubmitGeometry(p);
#endif
		}
	}

	void EnginePipeline::Precompute(int shaderId)
	{
		auto & gpuShader = gpuShaders[shaderId];
		auto hw = engine->GetHardwareRenderer();
		List<unsigned char> precomputeTexUniformBuffer;
	
		for (auto & obj : objects)
		{
			for (auto & tech : obj.Techniques)
			{
				if (tech.ShaderId != shaderId)
					continue;
				PrecomputeVertex(obj, tech);
				PrecomputeUniform(obj, precomputeTexUniformBuffer, tech);
				PrecomputeTextures(obj, tech, precomputeTexUniformBuffer);

			}
		}
		if (gpuShader.PrecomputeTexUniformBuffer.Handle)
			hw->DestroyBuffer(gpuShader.PrecomputeTexUniformBuffer);
		if (precomputeTexUniformBuffer.Count())
		{
			gpuShader.PrecomputeTexUniformBuffer = hw->CreateBuffer(BufferUsage::UniformBuffer);
			gpuShader.PrecomputeTexUniformBuffer.SetData(precomputeTexUniformBuffer.Buffer(), precomputeTexUniformBuffer.Count());
			gpuShader.PrecomputeTexUniformBuffer.MakeResident(true);
			printf("buffer %d: %d\n", shaderId, gpuShader.PrecomputeTexUniformBuffer.Handle);
			auto addr = gpuShader.PrecomputeTexUniformBuffer.GetGpuAddress();
			// fill in true gpu address with precomputeTex uniforms
			for (auto & obj : objects)
			{
				for (auto & tech : obj.Techniques)
				{
					if (tech.ShaderId != shaderId)
						continue;
					tech.PrecomputeTexUniformRange.IndexStart += addr;
					tech.PrecomputeTexUniformRange.IndexEnd += addr;
					tech.PrecomputeUniformRange.IndexStart += addr;
					tech.PrecomputeUniformRange.IndexEnd += addr;
				}
			}
		}	
	}
	void EnginePipeline::PrecomputeVertex(DrawableInstance & inst, ShadingTechnique & tech)
	{
		Spire::Compiler::WorldMetaData wmeta;
		if (!shaderLibs[tech.ShaderId]->MetaData.Worlds.TryGetValue(L"precomputeVert", wmeta))
		{
			tech.PrebakedMesh = tech.SourceMesh;
			return;
		}
	
		RefPtr<DeviceMesh> rsMesh;
		if (gpuShaders[tech.ShaderId].PrebakedVertices.TryGetValue(inst.GUID, rsMesh))
		{
			tech.PrebakedMesh = rsMesh;
			return;
		}

		auto hw = engine->GetHardwareRenderer();
		rsMesh = new DeviceMesh(hw);
		rsMesh->Bounds = tech.SourceMesh->Bounds;
		rsMesh->IndexBuffer = tech.SourceMesh->IndexBuffer;
		rsMesh->IndexCount = tech.SourceMesh->IndexCount;
		tech.PrebakedMesh = rsMesh;
		gpuShaders[tech.ShaderId].PrebakedVertices[inst.GUID] = rsMesh;
		for (auto & comp : shaderLibs[tech.ShaderId]->MetaData.InterfaceBlocks[wmeta.OutputBlock].GetValue().Entries)
		{
			VertexDataType type = VertexDataType::Float;
			int components = 1;
			bool normalize = false;
			switch (comp.Type)
			{
			case ILBaseType::Float:
				components = 1;
				break;
			case ILBaseType::Float2:
				components = 2;
				break;
			case ILBaseType::Float3:
				components = 3;
				break;
			case ILBaseType::Float4:
				components = 4;
				break;
			default:
				throw NotImplementedException(L"Engine does not support precomputing vertex attributes of type " + ILBaseTypeToString(comp.Type));
			}
			rsMesh->Attributes.Add(VertexAttribute(comp.Name, type, components, normalize));
		}
		auto originalVformat = CreateVertexFormat(tech.SourceMesh->Attributes);
		auto newVformat = CreateVertexFormat(rsMesh->Attributes);
		auto vertCount = tech.SourceMesh->BufferSize / originalVformat.Size;
		rsMesh->BufferSize = newVformat.Size * vertCount;

		const int workGroupSize = 256;
		rsMesh->MeshBuffer.SetData(nullptr, rsMesh->BufferSize);
		rsMesh->MeshBuffer.MakeResident(false);

		auto &prog = gpuShaders[tech.ShaderId].PrecomputeVertProgram;
		prog.Use();
		hw->BindBufferAddr(GL::BufferType::UniformBuffer, 2, tech.UniformRange.IndexStart,
			(int)(tech.UniformRange.IndexEnd - tech.UniformRange.IndexStart));
		hw->BindBufferAddr(GL::BufferType::UniformBuffer, 4, tech.RootTexUniformRange.IndexStart,
			(int)(tech.RootTexUniformRange.IndexEnd - tech.RootTexUniformRange.IndexStart));
		hw->BindBufferAddr(GL::BufferType::UniformBuffer, 0, tech.ModelUniformRange.IndexStart,
			(int)(tech.ModelUniformRange.IndexEnd - tech.ModelUniformRange.IndexStart));
		prog.SetUniform(L"rootVert", tech.SourceMesh->MeshBuffer.GetGpuAddress());
		prog.SetUniform(L"precomputeVert", rsMesh->MeshBuffer.GetGpuAddress());
		prog.SetUniform(L"sys_thread_count", (unsigned int)vertCount);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
		hw->ExecuteComputeShader(vertCount / workGroupSize + (vertCount%workGroupSize == 0 ? 0 : 1), 1, 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);
	}
	void EnginePipeline::PrecomputeUniform(DrawableInstance & inst, List<unsigned char> & uniformBuffer, ShadingTechnique & tech)
	{
		Spire::Compiler::WorldMetaData wmeta, vtxMeta;
		if (!shaderLibs[tech.ShaderId]->MetaData.Worlds.TryGetValue(L"precomputeUniform", wmeta))
		{
			tech.PrecomputeUniformRange.IndexStart = 0;
			tech.PrecomputeUniformRange.IndexEnd = 0;
			return;
		}
		if (!shaderLibs[tech.ShaderId]->MetaData.Worlds.TryGetValue(L"precomputeVert", vtxMeta))
		{
			return;
		}
		auto guid = inst.GUID;
		BufferRange rs;
		if (gpuShaders[tech.ShaderId].PrebakedUniforms.TryGetValue(guid, rs))
		{
			tech.PrecomputeUniformRange = rs;
			return;
		}
		auto outBlock = shaderLibs[tech.ShaderId]->MetaData.InterfaceBlocks[wmeta.OutputBlock].GetValue();
		if (outBlock.Size)
		{
			auto hw = engine->GetHardwareRenderer();

			auto newVformat = CreateVertexFormat(tech.PrebakedMesh->Attributes);
			auto vertCount = tech.PrebakedMesh->BufferSize / newVformat.Size;

			const int workGroupSize = 256;
			GL::BufferObject gpuBuffer = hw->CreateBuffer(BufferUsage::ShadeStorageBuffer);
			gpuBuffer.SetData(nullptr, outBlock.Size * vertCount);
			gpuBuffer.MakeResident(false);

			auto &prog = gpuShaders[tech.ShaderId].PrecomputeUniformProgram;
			prog.Use();
			hw->BindBufferAddr(GL::BufferType::UniformBuffer, 1, viewUniformAddr, sizeof(ViewUniformBufferContent));
			hw->BindBufferAddr(GL::BufferType::UniformBuffer, 2, tech.UniformRange.IndexStart,
				(int)(tech.UniformRange.IndexEnd - tech.UniformRange.IndexStart));
			hw->BindBufferAddr(GL::BufferType::UniformBuffer, 4, tech.RootTexUniformRange.IndexStart,
				(int)(tech.RootTexUniformRange.IndexEnd - tech.RootTexUniformRange.IndexStart));
			hw->BindBufferAddr(GL::BufferType::UniformBuffer, 0, tech.ModelUniformRange.IndexStart,
				(int)(tech.ModelUniformRange.IndexEnd - tech.ModelUniformRange.IndexStart));
			prog.SetUniform(L"precomputeVert", tech.PrebakedMesh->MeshBuffer.GetGpuAddress());
			prog.SetUniform(L"precomputeUniform", gpuBuffer.GetGpuAddress());
			prog.SetUniform(L"sys_thread_count", (unsigned int)vertCount);
			hw->ExecuteComputeShader(vertCount / workGroupSize + (vertCount%workGroupSize == 0 ? 0 : 1), 1, 1);
			hw->Finish();
			glMemoryBarrier(GL_ALL_BARRIER_BITS);
			List<float> bufferData;
			bufferData.SetSize(outBlock.Size * vertCount);
			int size = bufferData.Count();
			gpuBuffer.GetData(bufferData.Buffer(), size);
			int stride = outBlock.Size / 4;
			for (int i = 1; i < vertCount; i++)
			{
				for (int j = 0; j < stride; j++)
				{
					bufferData[j] += bufferData[i*stride + j];
				}
			}
			float invCount = 1.0f / vertCount;
			for (int j = 0; j < stride; j++)
			{
				bufferData[j] *= invCount;
			}
			AlignBuffer(uniformBuffer);
			
			tech.PrecomputeUniformRange.IndexStart = uniformBuffer.Count();
			uniformBuffer.AddRange((unsigned char *)bufferData.Buffer(), stride * sizeof(float));
			tech.PrecomputeUniformRange.IndexEnd = uniformBuffer.Count();
			gpuShaders[tech.ShaderId].PrebakedUniforms[guid] = tech.PrecomputeUniformRange;
			hw->DestroyBuffer(gpuBuffer);
		}
	}
	void EnginePipeline::PrecomputeTextures(DrawableInstance & inst, ShadingTechnique & tech, List<unsigned char> & precomputeTexUniformBuffer)
	{
		int PrebakeTexSize = 512;

		auto hw = engine->GetHardwareRenderer();
		auto &gpuShader = gpuShaders[tech.ShaderId];
		Spire::Compiler::WorldMetaData wmeta;
		if (shaderLibs[tech.ShaderId]->MetaData.Worlds.TryGetValue(L"precomputeTex", wmeta))
		{
			hw->SetCullMode(CullMode::Disabled);
			hw->SetZTestMode(BlendOperator::Disabled);
			int frameBufferSize = 0;
			auto & interfaceBlock = shaderLibs[tech.ShaderId]->MetaData.InterfaceBlocks[wmeta.OutputBlock].GetValue();
			for (auto & comp : interfaceBlock.Entries)
			{
				int size = PrebakeTexSize;
				String strSize;
				if (comp.Attributes.TryGetValue(L"TextureResolution", strSize))
					size = StringToInt(strSize);
				if (size == 0)
					size = PrebakeTexSize;
				frameBufferSize = Math::Max(frameBufferSize, size);
			}
			if (frameBufferSize != 0)
			{
				GL::FrameBuffer fb = hw->CreateFrameBuffer();
				GL::RenderBuffer rb = hw->CreateRenderBuffer(StorageFormat::Depth24Stencil8, frameBufferSize, frameBufferSize, 1);
				hw->SetViewport(0, 0, frameBufferSize, frameBufferSize);
				fb.SetDepthStencilRenderTarget(rb);
				EnumerableDictionary<String, GL::Texture2D> renderTargets, renderTargetsToRender;
				for (auto & comp : interfaceBlock.Entries)
				{
					String guid = comp.Name + L"#" + inst.GUID;
					GL::Texture2D tex;
					if (!gpuShader.PrebakedTextures.TryGetValue(guid, tex))
					{
						tex = hw->CreateTexture2D();
						gpuShader.PrebakedTextures[guid] = tex;
						renderTargetsToRender[comp.Name] = tex;
						StorageFormat sformat = StorageFormat::RGBA_F16;
						String storage = L"RGBA_16F";
						if (comp.Attributes.TryGetValue(L"Storage", storage))
						{
							if (storage == L"RGB8")
								sformat = StorageFormat::RGB_I8;
							else if (storage == L"R8")
								sformat = StorageFormat::Int8;
							else if (storage == L"RGB10_A2")
								sformat = StorageFormat::RGB10_A2;
							else if (storage == L"RGBA8")
								sformat = StorageFormat::RGBA_I8;
							else if (storage == L"RGBA_16F")
								sformat = StorageFormat::RGBA_F16;
							else if (storage == L"RGBA_32F")
								sformat = StorageFormat::RGBA_F32;
							else
							{
								printf("Unknown storage format for component %s - %s", comp.Name.ToMultiByteString(), storage.ToMultiByteString());
								storage = L"RGBA_16F";
							}
						}
						if (comp.Attributes.ContainsKey(L"Normal"))
						{
							sformat = StorageFormat::RGB10_A2;
							storage = L"RGB10_A2 (Normal)";
						}
						printf("Prebaking texture '%s', format: %s...\n", comp.Name.ToMultiByteString(), storage.ToMultiByteString());
						tex.SetData(sformat, frameBufferSize, frameBufferSize, 1, DataType::Float, nullptr);
						tex.BuildMipmaps();
					}
					renderTargets[comp.Name] = tex;
				}
				auto precomputeTexProgram = gpuShader.PrecomputeTexProgram;
				precomputeTexProgram.Use();
				int mask = 0;
				for (auto & target : renderTargetsToRender)
				{
					int loc = precomputeTexProgram.GetOutputLocation(target.Key);
					if (loc != -1)
					{
						mask |= (1 << loc);
						fb.SetColorRenderTarget(loc, target.Value);
					}
				}
				if (mask)
				{
					fb.EnableRenderTargets(mask);
					hw->SetWriteFrameBuffer(fb);
					hw->SetClearColor(Vec4::Create(0.0f));
					hw->Clear(true, true, true);
					hw->BindBufferAddr(GL::BufferType::UniformBuffer, 2, tech.UniformRange.IndexStart,
						(int)(tech.UniformRange.IndexEnd - tech.UniformRange.IndexStart));
					hw->BindBufferAddr(GL::BufferType::UniformBuffer, 4, tech.RootTexUniformRange.IndexStart,
						(int)(tech.RootTexUniformRange.IndexEnd - tech.RootTexUniformRange.IndexStart));
					hw->BindBufferAddr(GL::BufferType::UniformBuffer, 0, tech.ModelUniformRange.IndexStart,
						(int)(tech.ModelUniformRange.IndexEnd - tech.ModelUniformRange.IndexStart));
					tech.PrebakedMesh->Draw();
					hw->SetWriteFrameBuffer(GL::FrameBuffer());
					fb.SetColorRenderTarget(0, GL::Texture2D());
					for (auto & target : renderTargets)
					{
						target.Value.BuildMipmaps();
						mipmapBuilder.BuildMimap(target.Value);
					}
				}
				hw->DestroyRenderBuffer(rb);
				hw->DestroyFrameBuffer(fb);

				// create uniform fields
				AlignBuffer(precomputeTexUniformBuffer);
				int bufferStart = precomputeTexUniformBuffer.Count();
				for (auto & entry : interfaceBlock.Entries)
				{
					auto entryName = entry.Name;
					GL::Texture2D val;
					GL::TextureHandle handle;
					if (renderTargets.TryGetValue(entryName, val))
					{
						handle = val.GetTextureHandle(engine->GetPresetSampler(PresetSamplers::TrilinearClamp));
						handle.MakeResident();
					}
					precomputeTexUniformBuffer.AddRange((unsigned char*)&handle, sizeof(uint64_t));
				}
				tech.PrecomputeTexUniformRange.IndexStart = bufferStart;
				tech.PrecomputeTexUniformRange.IndexEnd = precomputeTexUniformBuffer.Count();

			}
			hw->SetCullMode(CullMode::CullBackFace);
			hw->SetZTestMode(BlendOperator::LessEqual);
		}
		
	}
	void EnginePipeline::DrawObjSpace()
	{
		auto hw = engine->GetHardwareRenderer();
		engine->GetHardwareRenderer()->SetZTestMode(BlendOperator::Disabled);
		hw->SetClearColor(Vec4::Create(0.0f));
		engine->GetHardwareRenderer()->SetCullMode(CullMode::Disabled);
		for (auto & obj : objects)
		{
			for (auto & tech : obj.Techniques)
			{
				if (tech.ObjSpaceTextures.Count())
				{
					hw->SetWriteFrameBuffer(tech.ObjSpaceFBO);
					int w = 0, h = 0;
					tech.DepthRenderBuffer.GetSize(w, h);
					hw->SetViewport(0, 0, w, h);
					hw->Clear(true, true, true);
					gpuShaders[tech.ShaderId].ObjSpaceProgram.Use();
					hw->BindBufferAddr(GL::BufferType::UniformBuffer, 1, viewUniformAddr, sizeof(ViewUniformBufferContent));
					hw->BindBufferAddr(GL::BufferType::UniformBuffer, 2, tech.UniformRange.IndexStart,
						(int)(tech.UniformRange.IndexEnd - tech.UniformRange.IndexStart));
					hw->BindBufferAddr(GL::BufferType::UniformBuffer, 3, tech.PrecomputeTexUniformRange.IndexStart,
						(int)(tech.PrecomputeTexUniformRange.IndexEnd - tech.PrecomputeTexUniformRange.IndexStart));
					hw->BindBufferAddr(GL::BufferType::UniformBuffer, 4, tech.RootTexUniformRange.IndexStart,
						(int)(tech.RootTexUniformRange.IndexEnd - tech.RootTexUniformRange.IndexStart));
					hw->BindBufferAddr(GL::BufferType::UniformBuffer, 0, tech.ModelUniformRange.IndexStart,
						(int)(tech.ModelUniformRange.IndexEnd - tech.ModelUniformRange.IndexStart));
					tech.PrebakedMesh->Draw();
					for (auto & tex : tech.ObjSpaceTextures)
					{
						mipmapBuilder.BuildMimap(tex);
					}
				}
			}
		}
		hw->SetViewport(0, 0, engine->GetScreenWidth(), engine->GetScreenHeight());
	}
	void EnginePipeline::FreeCommandLists()
	{
		auto hw = engine->GetHardwareRenderer();
		for (auto & passCtx : renderPassContexts)
			for (auto &cmdLists : passCtx.CompiledCommandLists)
			{
#ifdef USE_COMMAND_LIST
				for (auto &cmdList : cmdLists)
				{
					for (auto cmd : cmdList)
						hw->DestroyCommandList(cmd.Commands);
				}
#endif
				cmdLists.Clear();
			}

		for (auto & stateObj : stateObjects)
			hw->DestroyState(stateObj);
		stateObjects.Clear();
	}

	void EnginePipeline::PrintStats()
	{
		float cellSizeX = (bounds.Max.x - bounds.Min.x) / sceneGridSize;
		float cellSizeZ = (bounds.Max.z - bounds.Min.z) / sceneGridSize;
		printf("CPU time: %f\n", cpuFrameTime);
		for (int i = 0; i < sceneGridSize; i++)
		{
			for (int j = 0; j < sceneGridSize; j++)
			{
				BBox cellBounds;
				cellBounds.Min = Vec3::Create(bounds.xMin + cellSizeX * i, bounds.yMin,
					bounds.zMin + cellSizeZ * j);
				cellBounds.Max = Vec3::Create(cellBounds.xMin + cellSizeX, bounds.yMax, cellBounds.zMin + cellSizeZ);
				if (cellBounds.Contains(sysUniforms.Views[0].CamPos))
				{
					auto &objs = sceneGridObjLists[j*sceneGridSize + i];
					printf("Object count: %d\n", objs.Count());
					auto & lists = GetMainRenderPassContext().CompiledCommandLists[j*sceneGridSize + i];
					for (int k = 0; k < lists.Count(); k++)
					{
						printf("lod %d: %d lists\n", k, lists[k].Count());
						HashSet<GLuint> programHandles;
						for (auto obj : objs)
						{
							programHandles.Add(gpuShaders[obj->Techniques[Math::Min(k, obj->Techniques.Count() - 1)].ShaderId].Programs[MaterialContext::MainProgramId].Handle);
						}
						printf("       %d shaders types\n", programHandles.Count());
					}
				}
			}
		}
	}

	Spire::Compiler::ShaderMetaData EnginePipeline::GetShaderMetaData(String shaderName)
	{
		int id = -1;
		if (shaderIds.TryGetValue(shaderName, id))
		{
			return shaderLibs[id]->MetaData;
		}
		return ShaderMetaData();
	}

	EnumerableDictionary<String, GL::Texture2D> EnginePipeline::GetPrecomputedTextures(String shader)
	{
		auto inst = objects[0];
		for (auto & tech : inst.Techniques)
		{
			if (shaderLibs[tech.ShaderId]->MetaData.ShaderName == shader)
			{
				EnumerableDictionary<String, GL::Texture2D> result;
				for (auto & tex : gpuShaders[tech.ShaderId].PrebakedTextures)
				{
					auto name = tex.Key.SubString(0, tex.Key.IndexOf(L'#'));
					result[name] = tex.Value;
				}
				return result;
			}
		}
		return EnumerableDictionary<String, GL::Texture2D>();
	}

	void EnginePipeline::AllocObjectSpaceBuffers(int shaderId)
	{
		auto & gpuShader = gpuShaders[shaderId];
		auto hw = engine->GetHardwareRenderer();
		if (gpuShader.ObjSpaceUniformBuffer.Handle)
			hw->DestroyBuffer(gpuShader.ObjSpaceUniformBuffer);
		gpuShader.ObjSpaceUniformBuffer = hw->CreateBuffer(BufferUsage::UniformBuffer);

		List<unsigned char> uniformBuffer;
		for (int i = 0; i < objects.Count(); i++)
		{
			auto & obj = objects[i];
			for (int j = 0; j < obj.Techniques.Count(); j++)
			{
				auto & tech = obj.Techniques[j];
				if (tech.ShaderId != shaderId)
					continue;
				Spire::Compiler::WorldMetaData wmeta;
				tech.FreeObjectSpaceTextures(hw);
				if (shaderLibs[shaderId]->MetaData.Worlds.TryGetValue(L"objSurface", wmeta))
				{
					int DefaultTextureSize = 512;
					int frameBufferSize = 0;
					auto & interfaceBlock = shaderLibs[tech.ShaderId]->MetaData.InterfaceBlocks[wmeta.OutputBlock].GetValue();
					for (auto & comp : interfaceBlock.Entries)
					{
						int size = DefaultTextureSize;
						String strSize;
						if (comp.Attributes.TryGetValue(L"TextureResolution", strSize))
							size = StringToInt(strSize);
						if (size == 0)
							size = DefaultTextureSize;
						frameBufferSize = Math::Max(frameBufferSize, size);
					}
					if (frameBufferSize != 0)
					{
						tech.ObjSpaceFBO = hw->CreateFrameBuffer();
						tech.DepthRenderBuffer = hw->CreateRenderBuffer(StorageFormat::Depth24Stencil8, frameBufferSize, frameBufferSize, 1);
						tech.ObjSpaceFBO.SetDepthStencilRenderTarget(tech.DepthRenderBuffer);
						EnumerableDictionary<String, GL::Texture2D> renderTargets;
						for (auto & comp : interfaceBlock.Entries)
						{
							auto tex = hw->CreateTexture2D();
							tech.ObjSpaceTextures.Add(tex);
							tex.SetData(StorageFormat::RGBA_F16, frameBufferSize, frameBufferSize, 1, DataType::Float, nullptr);
							tex.BuildMipmaps();
							renderTargets[comp.Name] = tex;
						}

						int mask = 0;
						for (auto & target : renderTargets)
						{
							int loc = gpuShader.ObjSpaceProgram.GetOutputLocation(target.Key);
							if (loc != -1)
							{
								mask |= (1 << loc);
								tech.ObjSpaceFBO.SetColorRenderTarget(loc, target.Value);
							}
						}
						tech.ObjSpaceFBO.EnableRenderTargets(mask);
						// create uniform fields
						AlignBuffer(uniformBuffer);
						int bufferStart = uniformBuffer.Count();
						for (auto & entry : interfaceBlock.Entries)
						{
							auto entryName = entry.Name;
							GL::Texture2D val;
							GL::TextureHandle handle;
							if (renderTargets.TryGetValue(entryName, val))
							{
								handle = val.GetTextureHandle(engine->GetPresetSampler(PresetSamplers::TrilinearClamp));
								handle.MakeResident();
							}
							uniformBuffer.AddRange((unsigned char*)&handle, sizeof(uint64_t));
						}
						tech.ObjTexUniformRange.IndexStart = bufferStart;
						tech.ObjTexUniformRange.IndexEnd = uniformBuffer.Count();
					}
				}
				if (shaderLibs[shaderId]->MetaData.Worlds.TryGetValue(L"lowRes", wmeta))
				{
					EnumerableDictionary<String, GL::Texture2D> renderTargets;
					int mask = 0;
					auto & interfaceBlock = shaderLibs[tech.ShaderId]->MetaData.InterfaceBlocks[wmeta.OutputBlock].GetValue();

					for (auto & comp : interfaceBlock.Entries)
					{
						int loc = gpuShader.Programs[MaterialContext::LowResProgramId].GetOutputLocation(comp.Name);
						auto tex = engine->LoadGBufferTexture(L"s" + String(loc), StorageFormat::RGBA_F16, DataType::Float4, 0.5f, 1);
						mask |= (1 << loc);
						renderTargets[comp.Name] = tex;
					}
					tech.LowResRenderTargetMask = mask;
			
					AlignBuffer(uniformBuffer);
					int bufferStart = uniformBuffer.Count();
					for (auto & entry : interfaceBlock.Entries)
					{
						GL::Texture2D val;
						GL::TextureHandle handle;
						auto entryName = entry.Name;
						if (renderTargets.TryGetValue(entryName, val))
						{
							handle = val.GetTextureHandle(engine->GetPresetSampler(PresetSamplers::LinearClamp));
							handle.MakeResident();
						}
						uniformBuffer.AddRange((unsigned char*)&handle, sizeof(uint64_t));
					}
					tech.LowResUniformRange.IndexStart = bufferStart;
					tech.LowResUniformRange.IndexEnd = uniformBuffer.Count();
				}
			}
		}
		if (uniformBuffer.Count())
		{
			gpuShader.ObjSpaceUniformBuffer.SetData(uniformBuffer.Buffer(), uniformBuffer.Count());
			gpuShader.ObjSpaceUniformBuffer.MakeResident(true);
			auto addr = gpuShader.ObjSpaceUniformBuffer.GetGpuAddress();
			for (int i = 0; i < objects.Count(); i++)
			{
				auto & obj = objects[i];
				for (int j = 0; j < obj.Techniques.Count(); j++)
				{
					auto & tech = obj.Techniques[j];
					if (tech.ShaderId != shaderId)
						continue;
					tech.ObjTexUniformRange.IndexStart += addr;
					tech.ObjTexUniformRange.IndexEnd += addr;
					tech.LowResUniformRange.IndexStart += addr;
					tech.LowResUniformRange.IndexEnd += addr;
				}
			}
		}
	}

	void EnginePipeline::FrameResized()
	{
		for (auto & pass : renderPasses)
			pass->FrameResized();
		for (int i = 0; i < gpuShaders.Count(); i++)
			this->AllocObjectSpaceBuffers(i);
		CompileCommandLists();
	}

	

}