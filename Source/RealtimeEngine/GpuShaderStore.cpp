#include "GpuShaderStore.h"
using namespace GL;

namespace RealtimeEngine
{
	GL::Program GpuShaderStore::LoadProgram(String vs, List<String>& outputs)
	{
		if (auto prog = programs.TryGetValue(vs))
		{
			prog->RefCount++;
			return prog->Program;
		}
		GpuProgram gpuProg;
		gpuProg.VS = renderer->CreateShader(ShaderType::VertexShader, vs);
		gpuProg.Program = renderer->CreateTransformFeedbackProgram(gpuProg.VS, outputs, GL::FeedbackStorageMode::Interleaved);
		gpuProg.RefCount = 1;
		programs[vs] = gpuProg;
		programIdentifiers[gpuProg.Program.Handle] = vs;
		return gpuProg.Program;
	}
	GL::Program GpuShaderStore::LoadComputeProgram(String cs)
	{
		if (auto prog = programs.TryGetValue(cs))
		{
			prog->RefCount++;
			return prog->Program;
		}
		GpuProgram gpuProg;
		gpuProg.VS = renderer->CreateShader(ShaderType::ComputeShader, cs);
		gpuProg.Program = renderer->CreateProgram(gpuProg.VS);
		gpuProg.RefCount = 1;
		programs[cs] = gpuProg;
		programIdentifiers[gpuProg.Program.Handle] = cs;
		return gpuProg.Program;
	}
	GL::Program GpuShaderStore::LoadProgram(String vs, String fs)
	{
		if (auto prog = programs.TryGetValue(vs + fs))
		{
			prog->RefCount++;
			return prog->Program;
		}
		GpuProgram gpuProg;
		gpuProg.VS = renderer->CreateShader(ShaderType::VertexShader, vs);
		gpuProg.FS = renderer->CreateShader(ShaderType::FragmentShader, fs);

		gpuProg.Program = renderer->CreateProgram(gpuProg.VS, gpuProg.FS);
		gpuProg.RefCount = 1;
		auto identifier = vs + fs;
		programs[identifier] = gpuProg;
		programIdentifiers[gpuProg.Program.Handle] = identifier;
		return gpuProg.Program;
	}
	void GpuShaderStore::DestroyProgram(GL::Program & program)
	{
		String identifier;
		if (programIdentifiers.TryGetValue(program.Handle, identifier))
		{
			if (auto prog = programs.TryGetValue(identifier))
			{
				prog->RefCount--;
				if (prog->RefCount == 0)
				{
					programIdentifiers.Remove(program.Handle);
					renderer->DestroyShader(prog->VS);
					if (prog->FS.Handle)
						renderer->DestroyShader(prog->FS);
					renderer->DestroyProgram(program);
					programs.Remove(identifier);
				}
			}
		}
		program.Handle = 0;
	}
	void GpuShaderStore::Free()
	{
		for (auto & prog : programs)
		{
			renderer->DestroyShader(prog.Value.VS);
			if (prog.Value.FS.Handle)
				renderer->DestroyShader(prog.Value.FS);
			renderer->DestroyProgram(prog.Value.Program);
		}
		programs.Clear();
		programIdentifiers.Clear();
	}
}