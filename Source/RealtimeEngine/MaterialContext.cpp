#include "MaterialContext.h"
#include "ShadowMap.h"
using namespace GL;

namespace RealtimeEngine
{
	const wchar_t * fsEpilog = LR"(
		if ($opacity <= 0.0)
			discard;
		)";

	void MaterialContext::Reload(GL::HardwareRenderer * hw, GpuShaderStore & shaderStore, const SpireLib::ShaderLib & shaderLib)
	{
		Free(hw, shaderStore);
		glFinish();
		// reset opengl state 
		for (int i = 0; i < 8; i++)
		{
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, 0);
			glBindSampler(i, 0);
		}
		GL::Program().Use();
		glFinish();

		Programs.SetSize(Programs.GetCapacity());
		PrecomputeTexProgram = shaderStore.LoadProgram(shaderLib.Sources[L"precomputeTexVs"]().GetAllCodeGLSL(), shaderLib.Sources[L"precomputeTex"]().GetAllCodeGLSL());

		Spire::Compiler::WorldMetaData wmeta;
		if (shaderLib.MetaData.Worlds.TryGetValue(L"precomputeVert", wmeta))
		{
			PrecomputeVertSource = shaderLib.Sources[L"precomputeVert"]().GetAllCodeGLSL();
			PrecomputeVertProgram = shaderStore.LoadComputeProgram(PrecomputeVertSource);
		}
		if (shaderLib.MetaData.Worlds.TryGetValue(L"precomputeUniform", wmeta))
		{
			PrecomputeUniformSource = shaderLib.Sources[L"precomputeUniform"]().GetAllCodeGLSL();
			PrecomputeUniformProgram = shaderStore.LoadComputeProgram(PrecomputeUniformSource);
		}
		if (shaderLib.MetaData.Worlds.TryGetValue(L"objSurface", wmeta))
		{
			ObjSpaceProgram = shaderStore.LoadProgram(shaderLib.Sources[L"objSurfaceVs"]().GetAllCodeGLSL(), shaderLib.Sources[L"objSurface"]().GetAllCodeGLSL());
		}
	}
}