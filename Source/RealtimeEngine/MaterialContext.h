#ifndef REALTIME_ENGINE_MATERIAL_CONTEXT_H
#define REALTIME_ENGINE_MATERIAL_CONTEXT_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "DeviceMarshalling.h"
#include "GpuShaderStore.h"
#include "SpireLib.h"

namespace RealtimeEngine
{
	class MaterialContext
	{
	public:
		GL::BufferObject UniformBuffer, PrecomputeTexUniformBuffer, ObjSpaceUniformBuffer, PrecomputeUniformBuffer;
		Array<GL::Program, 16> Programs;
		GL::Program PrecomputeTexProgram, PrecomputeVertProgram, PrecomputeUniformProgram, ObjSpaceProgram;
		static const int LowResProgramId = 0;
		static const int MainProgramId = 1;
		static const int LowQualityProgramId = 2;
		static const int ShadowProgramId = 3;
		String PrecomputeUniformSource, PrecomputeVertSource;
		EnumerableDictionary<String, GL::Texture2D> PrebakedTextures;
		EnumerableDictionary<String, BufferRange> PrebakedUniforms;
		EnumerableDictionary<String, RefPtr<DeviceMesh>> PrebakedVertices;

		void Reload(GL::HardwareRenderer * hw, GpuShaderStore & shaderStore, const SpireLib::ShaderLib & shaderLib);
		void Free(GL::HardwareRenderer * hw, GpuShaderStore & shaderStore)
		{
			for (auto & program : Programs)
				if (program.Handle)
					shaderStore.DestroyProgram(program);
			if (PrecomputeTexProgram.Handle)
				shaderStore.DestroyProgram(PrecomputeTexProgram);
			if (PrecomputeVertProgram.Handle)
				shaderStore.DestroyProgram(PrecomputeVertProgram);
			if (PrecomputeUniformProgram.Handle)
				shaderStore.DestroyProgram(PrecomputeUniformProgram);
			if (ObjSpaceProgram.Handle)
				shaderStore.DestroyProgram(ObjSpaceProgram);
			if (UniformBuffer.Handle)
				hw->DestroyBuffer(UniformBuffer);
			if (PrecomputeTexUniformBuffer.Handle)
				hw->DestroyBuffer(PrecomputeTexUniformBuffer);
			if (PrecomputeUniformBuffer.Handle)
				hw->DestroyBuffer(PrecomputeUniformBuffer);
			if (ObjSpaceUniformBuffer.Handle)
				hw->DestroyBuffer(ObjSpaceUniformBuffer);
			for (auto & tex : PrebakedTextures)
				hw->DestroyTexture(tex.Value);
			PrebakedTextures.Clear();
			PrebakedUniforms.Clear();
			for (auto & mesh : PrebakedVertices)
				mesh.Value = nullptr;
			PrebakedVertices.Clear();
		}
	};
}

#endif