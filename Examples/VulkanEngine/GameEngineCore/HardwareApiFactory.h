#ifndef HARDWARE_API_FACTORY_H
#define HARDWARE_API_FACTORY_H

#include "CoreLib/Basic.h"
#include "Engine.h"
#include "Spire/Spire.h"

namespace GameEngine
{
	class HardwareRenderer;

	class ShaderCompilationError
	{
	public:
		CoreLib::String Message;
		int ErrorId;
		CoreLib::String FileName;
		int Line, Col;
		ShaderCompilationError() {}
		ShaderCompilationError(const SpireErrorMessage & msg)
		{
			Message = msg.Message;
			ErrorId = msg.ErrorId;
			FileName = msg.FileName;
			Line = msg.Line;
			Col = msg.Col;
		}
	};

	class ShaderCompilationResult
	{
	public:
		CoreLib::EnumerableDictionary<CoreLib::String, CoreLib::List<unsigned char>> Shaders;
		CoreLib::List<ShaderCompilationError> Errors, Warnings;
		void LoadFromFile(CoreLib::String fileName);
		void SaveToFile(CoreLib::String fileName, bool codeIsText);
	};

	class HardwareApiFactory : public CoreLib::Object
	{
	protected:
		CoreLib::String engineShaderDir;
		CoreLib::String pipelineShaderDef;
		CoreLib::String defaultShader;
		CoreLib::String meshProcessingDef;
	public:
		void LoadShaderLibrary();
		virtual HardwareRenderer * CreateRenderer(int gpuId) = 0;
		virtual bool CompileShader(ShaderCompilationResult & src, const CoreLib::String & filename, const CoreLib::String & vertexDef, const CoreLib::String & symbol) = 0;
	};

	HardwareApiFactory * CreateOpenGLFactory();
	HardwareApiFactory * CreateVulkanFactory();
}

#endif