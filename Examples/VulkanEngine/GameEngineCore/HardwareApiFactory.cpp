#include "HardwareApiFactory.h"
#include "HardwareRenderer.h"
#include "CoreLib/LibIO.h"

namespace GameEngine
{
	class OpenGLFactory : public HardwareApiFactory
	{
	private:
		CoreLib::String pipelineShaderDef;
		CoreLib::String defaultShader;
	public:
		virtual HardwareRenderer * CreateRenderer(int /*gpuId*/) override
		{
			// Load OpenGL shader pipeline
			auto pipelineDefFile = Engine::Instance()->FindFile(L"GlPipeline.shader", ResourceType::Shader);
			if (!pipelineDefFile.Length())
				throw InvalidOperationException(L"'Pipeline.shader' not found. Engine directory is not setup correctly.");
			pipelineShaderDef = CoreLib::IO::File::ReadAllText(pipelineDefFile);
			auto utilDefFile = Engine::Instance()->FindFile(L"Utils.shader", ResourceType::Shader);
			if (!utilDefFile.Length())
				throw InvalidOperationException(L"'Utils.shader' not found. Engine directory is not setup correctly.");
			auto utilsDef = CoreLib::IO::File::ReadAllText(utilDefFile);
			pipelineShaderDef = pipelineShaderDef + utilsDef;
			auto defaultShaderFile = Engine::Instance()->FindFile(L"DefaultPattern.shader", ResourceType::Shader);
			if (!defaultShaderFile.Length())
				throw InvalidOperationException(L"'DefaultPattern.shader' not found. Engine directory is not setup correctly.");
			defaultShader = pipelineShaderDef + CoreLib::IO::File::ReadAllText(defaultShaderFile);

			return CreateGLHardwareRenderer();
		}

		virtual bool CompileShader(ShaderCompilationResult & src, const String & filename, const String & additionalDef, const String & vertexDef, const String & symbol) override
		{
			auto actualFilename = Engine::Instance()->FindFile(filename, ResourceType::Shader);

			// Check disk for shaders
			auto cachedShaderFilename =
				CoreLib::IO::Path::Combine(CoreLib::IO::Path::GetDirectoryName(actualFilename),
					CoreLib::IO::Path::GetFileNameWithoutEXT(actualFilename) + L"_" + symbol + L"_gl.cse");

			/*if (CoreLib::IO::File::Exists(cachedShaderFilename))
			{
				SpireLib::ShaderLibFile cachedShader;
				cachedShader.Load(cachedShaderFilename);

				for (auto world : cachedShader.Sources)
				{
					CoreLib::List<unsigned char> shaderText;
					char * codeStr = world.Value.GetAllCodeGLSL().ToMultiByteString();
					auto len = strlen(codeStr);
					shaderText.SetSize((int)len);
					memcpy(shaderText.Buffer(), codeStr, len);
					shaderText.Add(0);
					src.Shaders.AddIfNotExists(world.Key, shaderText);
				}

				return true;
			}*/

			// Compile shader using Spire	
			String shaderSrc;
			if (actualFilename.Length())
				shaderSrc = pipelineShaderDef + vertexDef + additionalDef + CoreLib::IO::File::ReadAllText(actualFilename);
			else
				shaderSrc = defaultShader + vertexDef + additionalDef;//TODO: remove additionalDef?

			Spire::Compiler::CompileResult compileResult;
			Spire::Compiler::CompileOptions options;
			options.SymbolToCompile = symbol;
			auto compiledShader = SpireLib::CompileShaderSource(compileResult, shaderSrc, L"", options);

			if (compileResult.WarningList.Count() > 0)
			{
				src.Warnings = compileResult.WarningList;
			}
			if (compileResult.ErrorList.Count() > 0)
			{
				src.Errors = compileResult.ErrorList;
				compileResult.PrintError();
				return false;
			}
			for (auto world : compiledShader[0].Sources)
			{
				List<unsigned char> codeBytes;
				auto worldSrc = world.Value.MainCode;
				auto copySrc = worldSrc.ToMultiByteString();
				codeBytes.SetSize((int)strlen(copySrc));
				memcpy(codeBytes.Buffer(), copySrc, codeBytes.Count());
				codeBytes.Add(0);
				src.Shaders.AddIfNotExists(world.Key, codeBytes);
			}
			compiledShader[0].SaveToFile(cachedShaderFilename);

			return true;
		}
	};

	class VulkanFactory : public HardwareApiFactory
	{
	private:
		CoreLib::String pipelineShaderDef;
		CoreLib::String defaultShader;
	public:
		virtual HardwareRenderer * CreateRenderer(int gpuId) override
		{
			// Load Vulkan shader pipeline
			auto pipelineDefFile = Engine::Instance()->FindFile(L"VkPipeline.shader", ResourceType::Shader);
			if (!pipelineDefFile.Length())
				throw InvalidOperationException(L"'VkPipeline.shader' not found. Engine directory is not setup correctly.");
			pipelineShaderDef = CoreLib::IO::File::ReadAllText(pipelineDefFile);
			auto utilDefFile = Engine::Instance()->FindFile(L"Utils.shader", ResourceType::Shader);
			if (!utilDefFile.Length())
				throw InvalidOperationException(L"'Utils.shader' not found. Engine directory is not setup correctly.");
			auto utilsDef = CoreLib::IO::File::ReadAllText(utilDefFile);
			pipelineShaderDef = pipelineShaderDef + utilsDef;
			auto defaultShaderFile = Engine::Instance()->FindFile(L"DefaultPattern.shader", ResourceType::Shader);
			if (!defaultShaderFile.Length())
				throw InvalidOperationException(L"'DefaultPattern.shader' not found. Engine directory is not setup correctly.");
			defaultShader = pipelineShaderDef + CoreLib::IO::File::ReadAllText(defaultShaderFile);

			return CreateVulkanHardwareRenderer(gpuId);
		}

		virtual bool CompileShader(ShaderCompilationResult & src, const String & filename, const String & additionalDef, const String & vertexDef, const String & symbol) override
		{
			// Compile shader using Spire
			auto actualFilename = Engine::Instance()->FindFile(filename, ResourceType::Shader);

			// Check disk for shaders
			auto cachedShaderFilename = 
				CoreLib::IO::Path::Combine(CoreLib::IO::Path::GetDirectoryName(actualFilename),
				CoreLib::IO::Path::GetFileNameWithoutEXT(actualFilename) + L"_" + symbol + L"_vk.cse");

			/*if (CoreLib::IO::File::Exists(cachedShaderFilename))
			{
				SpireLib::ShaderLibFile cachedShader;
				cachedShader.Load(cachedShaderFilename);

				for (auto world : cachedShader.Sources)
				{
					src.Shaders.AddIfNotExists(world.Key, world.Value.BinaryCode);
				}

				return true;
			}*/


			String shaderSrc;
			if (actualFilename.Length())
				shaderSrc = pipelineShaderDef + vertexDef + additionalDef + CoreLib::IO::File::ReadAllText(actualFilename);
			else
				shaderSrc = defaultShader + vertexDef + additionalDef;

			Spire::Compiler::CompileResult compileResult;
			Spire::Compiler::CompileOptions options;
			options.SymbolToCompile = symbol;
			auto compiledShader = SpireLib::CompileShaderSource(compileResult, shaderSrc, L"", options);

			if (compileResult.WarningList.Count() > 0)
			{
				src.Warnings = compileResult.WarningList;
			}
			if (compileResult.ErrorList.Count() > 0)
			{
				src.Errors = compileResult.ErrorList;
				compileResult.PrintError();
				return false;
			}

			for (auto world : compiledShader[0].Sources)
			{
				src.Shaders.AddIfNotExists(world.Key, world.Value.BinaryCode);
			}
			compiledShader[0].SaveToFile(cachedShaderFilename);

			return true;
		}
	};

	HardwareApiFactory * CreateOpenGLFactory()
	{
		return new OpenGLFactory();
	}

	HardwareApiFactory * CreateVulkanFactory()
	{
		return new VulkanFactory();
	}
}