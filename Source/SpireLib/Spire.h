#ifndef SPIRE_H
#define SPIRE_H

namespace Spire
{
	class CompilationContext;
	class Shader;
	class CompileResult;
	
	enum class CodeGenTarget
	{
		GLSL, HLSL, SPIRV
	};
	enum class MessageType
	{
		Warning, Error
	};

	CompilationContext * CreateCompilationContext(const char * cacheDir);
	void SetCodeGenTarget(CompilationContext * ctx, CodeGenTarget target);
	void AddSearchPath(CompilationContext * ctx, const char * searchDir);
	void SetBackendParameter(CompilationContext * ctx, const char * paramName, const char * value);
	void DestroyCompilationContext(CompilationContext * ctx);

	bool LoadModuleLibrary(CompilationContext * ctx, const char * fileName);
	bool LoadModuleLibrary(CompilationContext * ctx, const char * source, const char * fileName);
	Shader* CreateShader(CompilationContext * ctx, const char * name);
	void ShaderAddModule(Shader * shader, const char * moduleName);
	void ShaderSetPipeline(Shader * shader, const char * pipelineName);
	
	void DestroyShader(Shader * shader);
	CompileResult* CompileShader(CompilationContext * ctx, Shader * shader);
	CompileResult* CompileShader(CompilationContext * ctx, const char * source, const char * fileName);

	bool IsCompileSucessful(CompileResult * result);
	int GetMessageCount(CompileResult * result, MessageType type);
	bool GetMessageContent(CompileResult * result, char * buffer, int & bufferSize, MessageType type, int index);
	int GetCompiledShaderNames(CompileResult * result, char * buffer, int & bufferSize);
	int GetCompiledShaderStageNames(CompileResult * result, const char * shaderName, char * buffer, int & bufferSize);
	char * GetShaderStageSource(CompileResult * result, const char * shaderName, const char * stage, int & bufferSize);
	void DestroyCompileResult(CompileResult * result);
}

#endif