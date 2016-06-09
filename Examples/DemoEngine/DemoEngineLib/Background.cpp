#include "Background.h"
#include <algorithm>
#include "Mesh.h"
#include "ModelResource.h"

using namespace CoreLib::IO;
using namespace CoreLib::Text;
using namespace CoreLib::Diagnostics;
using namespace CoreLib::Graphics;
using namespace GL;

namespace RealtimeEngine
{

	const wchar_t * backgroundVertShader = LR"(
		#version 420
		layout(location = 0) in vec2 vert_Position;
		layout(location = 1) in vec2 vert_TexCoords;	
		
		out vec2 uv;
		void main()
		{
			gl_Position = vec4(vert_Position, 0.0, 1.0);
			uv = vert_Position;
		}
	)";
	const wchar_t * backgroundFragShader = LR"(
		#version 420
		layout(binding = 0) uniform sampler2D skyTex;
		uniform mat4 invViewProjMatrix;
		uniform vec3 camPos;
		in vec2 uv;
		layout(location = 0) out vec4 color;
		void main()
		{
			vec4 worldPos = invViewProjMatrix * vec4(uv, 1.0, 1.0);
			vec3 wp3 = worldPos.xyz/worldPos.w;
			vec3 view = normalize(wp3 - camPos);
			view.y = min(0.999, view.y);
			float v = (asin(view.y) + (3.14159*0.5))/3.14159;
			float u = (atan(view.x, view.z) + 3.14159)/(3.14159*2.0);  
			color = texture(skyTex, vec2(u,v));
		}
	)";

	Background::Background(DeviceResourcePool * engine, String fileName)
	{
		program = engine->LoadProgram(engine->LoadShader(ShaderType::VertexShader, backgroundVertShader),
			engine->LoadShader(ShaderType::FragmentShader, backgroundFragShader));
		texture = engine->LoadTexture2D(fileName, StorageFormat::RGBA_I8);
	}
	void Background::Draw(DeviceResourcePool * engine, CoreLib::Graphics::ViewFrustum & view)
	{
		auto renderer = engine->GetHardwareRenderer();
		program.Use();
		program.SetUniform(L"camPos", view.CamPos);
		Matrix4 invViewProjMatrix;
		view.GetViewProjectionTransform().Inverse(invViewProjMatrix);
		program.SetUniform(L"invViewProjMatrix", invViewProjMatrix);
		Array<GL::Texture, 1> textures;
		Array<GL::TextureSampler, 1> samplers;
		textures.Add(texture);
		samplers.Add(engine->GetPresetSampler(PresetSamplers::LinearRepeat));
		renderer->SetZTestMode(BlendOperator::Disabled);
		glDepthMask(GL_FALSE);
		renderer->UseTextures(textures.GetArrayView(), samplers.GetArrayView());
		engine->DrawFullScreenQuad(-1.0f, -1.0f, 2.0f, 2.0f);
		glDepthMask(GL_TRUE);
		renderer->SetZTestMode(BlendOperator::Less);
		renderer->FinishUsingTextures(textures.GetArrayView());
	}
}
