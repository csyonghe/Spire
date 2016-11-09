#ifndef SPIRE_H
#define SPIRE_H

#ifdef _MSC_VER
#ifdef SPIRE_COMPILING_DLL
#define SPIRE_API __declspec(dllexport)
#else
#ifdef SPIRE_DYNAMIC
#define SPIRE_API __declspec(dllimport)
#else
#define SPIRE_API
#endif
#endif
#else
#define SPIRE_API
#endif

#define SPIRE_ERROR 0
#define SPIRE_WARNING 1

#define SPIRE_GLSL 0
#define SPIRE_HLSL 1
#define SPIRE_SPIRV 2

struct SpireCompilationContext;
struct SpireShader;
struct SpireCompileResult;

struct SpireErrorMessage
{
	const char * Message;
	int ErrorId;
	const char * FileName;
	int Line, Col;
};

SPIRE_API SpireCompilationContext * spCreateCompilationContext(const char * cacheDir);
SPIRE_API void spSetCodeGenTarget(SpireCompilationContext * ctx, int target);
SPIRE_API void spAddSearchPath(SpireCompilationContext * ctx, const char * searchDir);
SPIRE_API void spSetBackendParameter(SpireCompilationContext * ctx, const char * paramName, const char * value);
SPIRE_API void spDestroyCompilationContext(SpireCompilationContext * ctx);

SPIRE_API void spLoadModuleLibrary(SpireCompilationContext * ctx, const char * fileName);
SPIRE_API void spLoadModuleLibrary(SpireCompilationContext * ctx, const char * source, const char * fileName);
SPIRE_API SpireShader* spCreateShader(SpireCompilationContext * ctx, const char * name);
SPIRE_API void spShaderAddModule(SpireShader * shader, const char * moduleName);
SPIRE_API void spShaderSetPipeline(SpireShader * shader, const char * pipelineName);
	
SPIRE_API void spDestroyShader(SpireShader * shader);
SPIRE_API SpireCompileResult* spCompileShader(SpireCompilationContext * ctx, SpireShader * shader);
SPIRE_API SpireCompileResult* spCompileShader(SpireCompilationContext * ctx, const char * source, const char * fileName);

SPIRE_API bool spIsCompilationSucessful(SpireCompileResult * result);
SPIRE_API int spGetMessageCount(SpireCompileResult * result, int messageType);
SPIRE_API bool spGetMessageContent(SpireCompileResult * result, int messageType, int index, SpireErrorMessage * pMsg);
SPIRE_API int spGetCompiledShaderNames(SpireCompileResult * result, char * buffer, int * bufferSize);
SPIRE_API int spGetCompiledShaderStageNames(SpireCompileResult * result, const char * shaderName, char * buffer, int * bufferSize);
SPIRE_API char * spGetShaderStageSource(SpireCompileResult * result, const char * shaderName, const char * stage, int * length);
SPIRE_API void spDestroyCompileResult(SpireCompileResult * result);

#endif