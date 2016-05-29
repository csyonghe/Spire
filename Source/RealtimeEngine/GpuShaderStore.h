#ifndef REALTIME_ENGINE_GPU_SHADER_STORE_H
#define REALTIME_ENGINE_GPU_SHADER_STORE_H

#include "CoreLib/Basic.h"
#include "CoreLib/VectorMath.h"
#include "LibGL/OpenGLHardwareRenderer.h"

namespace RealtimeEngine
{
	class GpuShaderStore
	{
	private:
		GL::HardwareRenderer * renderer;
		struct GpuProgram
		{
			GL::Shader VS, FS;
			GL::Program Program;
			int RefCount = 0;
		};
		CoreLib::EnumerableDictionary<CoreLib::String, GpuProgram> programs;
		CoreLib::EnumerableDictionary<GLuint, CoreLib::String> programIdentifiers;
	public:
		void Init(GL::HardwareRenderer * pRenderer)
		{
			renderer = pRenderer;
		}
		GL::Program LoadProgram(CoreLib::String vs, CoreLib::List<CoreLib::String> & outputs);
		GL::Program LoadProgram(CoreLib::String vs, CoreLib::String fs);
		GL::Program LoadComputeProgram(CoreLib::String cs);
		void DestroyProgram(GL::Program & program);
		void Free();
	};
}

#endif