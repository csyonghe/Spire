#include "PTexMipmapBuilder.h"

namespace RealtimeEngine
{
	void PTexMipmapBuilder::Init(DeviceResourcePool * pEngine)
	{
		engine = pEngine;
		auto hw = pEngine->GetHardwareRenderer();
		for (int i = 0; i < 13; i++)
		{
			auto tex = hw->CreateTexture2D();
			auto rb = hw->CreateRenderBuffer(StorageFormat::Depth24Stencil8, 1<<i, 1<<i, 1);
			tex.SetData(StorageFormat::RGBA_F16, 1 << i, 1 << i, 1, DataType::Float4, nullptr);
			tmpTex.Add(tex);
			renderBuffer.Add(rb);
		}
		fbo = hw->CreateFrameBuffer();
		const char * vsSrc = R"(
			#version 440
			layout(location = 0) in vec2 vert_Position;
			void main()
			{
				gl_Position = vec4(vert_Position, 0.0, 1.0);
			}
		)";
		const char * dilateFS = R"(
			#version 440
			layout(pixel_center_integer) in vec4 gl_FragCoord;
			layout(location = 0) out vec4 color;
			layout(location = 0) uniform sampler2D srcTex;	
			layout(location = 1) uniform int lod;
			void main()
			{
				vec4 p;
				p = texelFetch(srcTex, ivec2(gl_FragCoord.x-1.0, gl_FragCoord.y), lod);
				if (p != 0.0)
				{
					color = p;
				}
				p = texelFetch(srcTex, ivec2(gl_FragCoord.x+1.0, gl_FragCoord.y), lod);
				if (p != 0.0)
				{
					color = p;
				}
				p = texelFetch(srcTex, ivec2(gl_FragCoord.x, gl_FragCoord.y-1.0), lod);
				if (p != 0.0)
				{
					color = p;
				}
				p = texelFetch(srcTex, ivec2(gl_FragCoord.x, gl_FragCoord.y+1.0), lod);
				if (p != 0.0)
				{
					color = p;
				}
				p = texelFetch(srcTex, ivec2(gl_FragCoord.xy), lod);
				if (p != vec4(0.0))
				{
					color = p;
				}
			}
		)";
		const char * downSampleFS = R"(
			#version 440
			layout(pixel_center_integer) in vec4 gl_FragCoord;
			layout(location = 0) out vec4 color;
			layout(location = 0) uniform sampler2D srcTex;	
			layout(location = 1) uniform int lod;
			void main()
			{
				ivec2 baseCoord = ivec2(gl_FragCoord.xy) * 2;
				vec4 sum = vec4(0.0);
				int count = 0;
				vec4 p = texelFetch(srcTex, baseCoord, lod);
				if (p != 0.0) { sum += p; count++; }
				p = texelFetch(srcTex, baseCoord + ivec2(1,0), lod);
				if (p != 0.0) { sum += p; count++; }
				p = texelFetch(srcTex, baseCoord + ivec2(1,1), lod);
				if (p != 0.0) { sum += p; count++; }
				p = texelFetch(srcTex, baseCoord + ivec2(0,1), lod);
				if (p != 0.0) { sum += p; count++; }
				if (count > 1)
				{
					sum *= (1.0/count);
				}
				color = sum;
			}
		)";
		dilateProgram = engine->LoadProgram(engine->LoadShader(ShaderType::VertexShader, vsSrc), engine->LoadShader(ShaderType::FragmentShader, dilateFS));
		downSampleProgram = engine->LoadProgram(engine->LoadShader(ShaderType::VertexShader, vsSrc), engine->LoadShader(ShaderType::FragmentShader, downSampleFS));
	}

	void PTexMipmapBuilder::Free()
	{
		auto hw = engine->GetHardwareRenderer();
		hw->DestroyFrameBuffer(fbo);
		for (auto & rb : renderBuffer)
			hw->DestroyRenderBuffer(rb);
		renderBuffer.Clear();
		for (auto & tex : tmpTex)
			hw->DestroyTexture(tex);
		tmpTex.Clear();
	}

	void PTexMipmapBuilder::BuildMimap(GL::Texture2D tex)
	{
		int w, h;
		tex.GetSize(w, h);
		int level = Math::Log2Ceil(w);
		
		auto hw = engine->GetHardwareRenderer();
		hw->SetWriteFrameBuffer(fbo);
		auto nearestSampler = engine->GetPresetSampler(PresetSamplers::NearestClamp);
		hw->SetZTestMode(BlendOperator::Disabled);
	
		dilateProgram.Use();
		for (int i = 0; i <= level; i++)
		{
			int size = w >> i;
			fbo.SetColorRenderTarget(0, tmpTex[level - i]);
			fbo.SetDepthStencilRenderTarget(renderBuffer[level - i]);
			hw->SetViewport(0, 0, size, size);
			hw->UseTexture(0, tex, nearestSampler);
			dilateProgram.SetUniform(0, 0);
			dilateProgram.SetUniform(1, i);
			engine->DrawFullScreenQuad(-1.0f, -1.0f, 2.0f, 2.0f);
			fbo.SetColorRenderTarget(0, tex, i);
			hw->UseTexture(0, tmpTex[level - i], nearestSampler);
			engine->DrawFullScreenQuad(-1.0f, -1.0f, 2.0f, 2.0f);

		}
		downSampleProgram.Use();
		for (int i = 0; i < level; i++)
		{
			fbo.SetColorRenderTarget(0, tex, i + 1);
			fbo.SetDepthStencilRenderTarget(renderBuffer[level - i - 1]);
			int size = w >> i >> 1;
			hw->SetViewport(0, 0, size, size);
			hw->UseTexture(0, tex, nearestSampler);
			downSampleProgram.SetUniform(0, 0);
			downSampleProgram.SetUniform(1, i);
			engine->DrawFullScreenQuad(-1.0f, -1.0f, 2.0f, 2.0f);
		}
	}
}