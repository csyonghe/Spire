#ifndef HARDWARE_API_FACTORY_H
#define HARDWARE_API_FACTORY_H

#include "CoreLib/Basic.h"
#include "Engine.h"
#include "Spire/Spire.h"

namespace GameEngine
{
	class HardwareRenderer;

	class ShaderCompilationResult
	{
	public:
		CoreLib::EnumerableDictionary<CoreLib::String,  CoreLib::List<unsigned char>> Shaders;
		CoreLib::List<Spire::Compiler::CompileError> Errors, Warnings;
	};

	class HardwareApiFactory : public CoreLib::Object
	{
	public:
		virtual HardwareRenderer * CreateRenderer(int gpuId) = 0;
		virtual bool CompileShader(ShaderCompilationResult & src, const String & filename, const String & additionalDef, const String & vertexDef, const String & symbol) = 0;
	};

	HardwareApiFactory * CreateOpenGLFactory();
	HardwareApiFactory * CreateVulkanFactory();
}

#endif