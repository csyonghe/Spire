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

#ifdef __cplusplus  
extern "C" {  // only need to export C interface if  
			  // used by C++ source code  
#endif  

			  /*!
			  @mainpage Introduction
			  Spire is a shading language and compiler framework that facilitates modular shader authoring and rapid exploration of
			  shader optimization choices (such as frequency reduction and algorithmic approximation) afforded by modern real-time
			  graphics engines. The current implementation of the Spire compiler can generate either GLSL or SPIR-V output for use
			  with OpenGL and Vulkan based engines.

			  Paper: http://graphics.cs.cmu.edu/projects/spire/


			  API Reference: Spire.h

			  @file Spire.h
			  */

#define SPIRE_ERROR 0
#define SPIRE_WARNING 1

#define SPIRE_GLSL 0
#define SPIRE_HLSL 1
#define SPIRE_SPIRV 2

#define SPIRE_LAYOUT_UNIFORM 0
#define SPIRE_LAYOUT_PACKED 1
#define SPIRE_LAYOUT_STORAGE 2

#define SPIRE_ERROR_INSUFFICIENT_BUFFER -1
#define SPIRE_ERROR_INVALID_PARAMETER -2

			  /*!
			  @brief Represents a compilation context. Created by spCreateCompilationContext().

			  Related Functions
			  - spCreateCompilationContext()
			  - spDestroyCompilationContext()
			  - spCreateShader()
			  - spCompileShader()
			  - spSetCodeGenTarget()
			  - spAddSearchPath()
			  - spSetBackendParameter()
			  */
	struct SpireCompilationContext {};

	/*!
	@brief Represents a shader. A SpireShader can be assembled by calling spCreateShader().
	Modules can be added to a shader by calling spShaderAddModule().

	Related Functions
	- spShaderAddModule()
	- spShaderTargetPipeline()
	*/
	struct SpireShader {};

	/*!
	@brief SpireModule objects provide reflection data about a module.
	Module objects can be obtained by calling spFindModule() once a module library is loaded via spLoadModuleLibrary().

	Related Functions
	- spLoadModuleLibrary()
	- spFindModule()
	- spModuleGetAllComponentsByWorld()
	- spModuleGetComponentCountByWorld()
	- spModuleGetComponentByWorld()
	- spModuleGetRequiredComponents()
	*/
	struct SpireModule {};

	/*!
	@brief Represents the compilation result, including error messages and compiled source code for each stage.

	Related Functions
	- spCompileShader()
	- spIsCompilationSucessful()
	- spGetMessageCount()
	- spGetMessageContent()
	- spGetCompiledShaderNames()
	- spGetCompiledShaderStageNames()
	- spGetShaderStageSource()
	- spDestroyCompilationResult()
	*/
	struct SpireCompilationResult {};

	/*!
	@brief Represents an error message from the compiler.
	*/
	struct SpireErrorMessage
	{
		const char * Message;    /**< Content of the message. Storage is owned by SpireCompilationContext.*/
		int ErrorId;             /**< A unique identifier for this type of error.*/
		const char * FileName;   /**< The source file name of this error. Storage is owned by SpireCompilationContext*/
		int Line;                /**< The line number of this error.*/
		int Col;                 /**< The column position of this error.*/
	};

	/*!
	@brief Stores description of a component.
	*/
	struct SpireComponentInfo
	{
		const char * Name;         /**< The name of the component. Storage is owned by SpireCompilationContext.*/
		const char * TypeName;     /**< The type name of the component. Storage is owned by SpireCompilationContext.*/
		const char * Register;     /**< The register binding (if available) of the component. Storage is owned by SpireCompilationContext.*/
		int Size;                  /**< The size (in bytes) of the component. For opaque types (e.g. sampler and texture), this value is 0.*/
		int Alignment;             /**< The alignment (in bytes) of the component. For opaque types (e.g. sampler and texture), this value is 0.*/
		int Offset;				   /**< The offset (in bytes) of the component. For opaque types (e.g. sampler and texture), this value is 0.*/
	};

	/*!
	@brief Represents a collection of SpireComponentInfo.
	*/
	struct SpireComponentInfoCollection {};

	/*!
	@brief Create a compilation context.
	@param cacheDir The directory used to store cached compilation results. Pass NULL to disable caching.
	@return A new compilation context.
	*/
	SPIRE_API SpireCompilationContext * spCreateCompilationContext(const char * cacheDir);

	/*!
	@brief Sets the target for code generation.
	@param ctx The compilation context.
	@param target The code generation target. Possible values are:
	- SPIRE_GLSL. Generates GLSL code.
	- SPIRE_HLSL. Generates HLSL code.
	- SPIRE_SPIRV. Generates SPIR-V code.
	*/
	SPIRE_API void spSetCodeGenTarget(SpireCompilationContext * ctx, int target);

	/*!
	@brief Add a path in which source files are being search. When the programmer specifies @code using <file_name> @endcode in code, the compiler searches the file
	in all search pathes in order.
	@param ctx The compilation context.
	@param searchDir The additional search directory.
	*/
	SPIRE_API void spAddSearchPath(SpireCompilationContext * ctx, const char * searchDir);

	/*!
	@brief Sets a parameter used by the compiler back-end.
	@param ctx The compilation context.
	@param paramName The name of the parameter.
	@param value The value of the parameter.
	*/
	SPIRE_API void spSetBackendParameter(SpireCompilationContext * ctx, const char * paramName, const char * value);

	/*!
	@brief Sets a shader to compile. By default, the compiler will generate code for all shaders in current context. After setting this option,
	the compiler will only generate code for the specified shader.
	@param ctx The compilation context.
	@param shaderName The name of the shader to compile.
	*/
	SPIRE_API void spSetShaderToCompile(SpireCompilationContext * ctx, const char * shaderName);

	/*!
	@brief Destorys the compilation context. Destorying a compilation context will free the memory for all strings owned by the
	SpireComilationContext and all SpireModule objects. These objects will not be available after a call to spDestroyCompilationContext.
	However, all SpireCompilationResult objects will continue to be available until they are destroyed.
	@param ctx The compilation context to destroy.
	*/
	SPIRE_API void spDestroyCompilationContext(SpireCompilationContext * ctx);

	/*!
	@brief Load and precompile spire modules from spire source file. Compilation status and error messages can be obtained via spIsCompilationSucessful(),
	spGetMessageCount() and spGetMessageContent() functions.
	@param ctx The compilation context.
	@param fileName The filename of the spire source code.
	*/
	SPIRE_API void spLoadModuleLibrary(SpireCompilationContext * ctx, const char * fileName);

	/*!
	@brief Load and precompile spire modules from spire source code in memory. Compilation status and error messages can be obtained via spIsCompilationSucessful(),
	spGetMessageCount() and spGetMessageContent() functions.
	@param ctx The compilation context.
	@param source The spire source code to precompile. All strings should be in UTF-8 encoding.
	@param fileName The filename used to report error messages regarding to code in @p source.
	*/
	SPIRE_API void spLoadModuleLibraryFromSource(SpireCompilationContext * ctx, const char * source, const char * fileName);

	/*!
	@brief Create a shader object that can be used to assemble a final shader from modules.
	@param ctx The compilation context.
	@param name The name of the shader.
	*/
	SPIRE_API SpireShader* spCreateShader(SpireCompilationContext * ctx, const char * name);

	/*!
	@brief Adds a module to a shader.
	@param shader A shader object.
	@param moduleName The name of the module to add to @p shader.
	*/
	SPIRE_API void spShaderAddModuleByName(SpireShader * shader, const char * moduleName);

	/*!
	@brief Adds a module to a shader.
	@param shader A shader object.
	@param module The handle of the module to add to @p shader.
	*/
	SPIRE_API void spShaderAddModule(SpireShader * shader, SpireModule * moduleName);

	/*!
	@brief Sets the target pipeline of a shader
	@param shader A shader object.
	@param pipelineName The name of the Pipeline that @p shader targets.
	*/
	SPIRE_API void spShaderSetPipeline(SpireShader * shader, const char * pipelineName);

	/*!
	@brief Find a precompiled module in a SpireCompilationContext.
	@param ctx The compilation context.
	@param moduleName The name of the module to find.
	@return If a module with the specified name exists in the current context, a handle to the module is returned. Otherwise, the return value is NULL.
	@note All SpireModule objects are destroyed when its containing SpireCompilationContext is destroyed.
	*/
	SPIRE_API SpireModule * spFindModule(SpireCompilationContext * ctx, const char * moduleName);

	/*!
	@brief Retrieves components that are qualified with the specified world.
	@param module The module from which to retrieve components.
	@param worldName The world name of requesting components.
	@param layout The layout rule used to compute offsets of the components. Can be SPIRE_LAYOUT_UNIFORM, SPIRE_LAYOUT_STORAGE or SPIRE_LAYOUT_PACKED.
	@return
	A handle to a collection of SpireComponentInfo structures. Individual components can be retrieved from
	the collection using spComponentInfoCollectionGetComponent() function.
	*/
	SPIRE_API SpireComponentInfoCollection * spModuleGetComponentsByWorld(SpireModule * module, const char * worldName, int layout);

	/*!
	@brief Retrieves component info from SpireComponentInfoCollection.
	@param collection The collection from which to retrieve components.
	@param index Index of the requesting component.
	@param result A pointer to a SpireComponentInfo structure used to recieve info on the specified component.
	@return
	If successful, this function returns 0. 
	Otherwise, the return value is one of the following error codes:
	- SPIRE_ERROR_INVALID_PARAMETER if any of the parameters are invalid.
	*/
	SPIRE_API int spComponentInfoCollectionGetComponent(SpireComponentInfoCollection * collection, int index, SpireComponentInfo * result);

	/*!
	@brief Get the number of components contained in a SpireComponentInfoCollection.
	@param collection The collection from which to retrieve components.
	@return
	If successful, this function the number of components in @p collection.
	Otherwise, the return value is one of the following error codes:
	- SPIRE_ERROR_INVALID_PARAMETER if any of the parameters are invalid.
	*/
	SPIRE_API int spComponentInfoCollectionGetCount(SpireComponentInfoCollection * collection);

	
	/*!
	@brief Retrieve a list of components that are required by the specified module.
	@param module The module from where to retrieve components.
	@param buffer A user allocated buffer of SpireComponentInfo for receiving outputs.
	@param bufferSize The size (in number of SpireComponentInfo structs) of the specified buffer.
	@return
	If @p buffer is NULL, the return value is the required size, in number of SpireComponentInfo.
	Otherwise, if the function suceeds, the return value is the number of SpireComponentInfo instances written to
	@p buffer. The function returns a negative value if it does not suceed. Possible error codes are:
	- SPIRE_ERROR_INSUFFICIENT_BUFFER. The supplied buffer size was not large enough.
	- SPIRE_ERROR_INVALID_PARAMETER. Any of the parameter values was invalid.
	*/
	SPIRE_API int spModuleGetRequiredComponents(SpireModule * module, SpireComponentInfo * buffer, int bufferSize);

	/*!
	@brief Destroys a shader object.
	@param shader The shader object to destroy.
	@note You are responsible for destorying a shader object when it is no longer used (e.g. after it has been compiled). Destroying a SpireCompilationContext
	does not automatically destroy SpireShader objects.
	*/
	SPIRE_API void spDestroyShader(SpireShader * shader);

	/*!
	@brief Compiles a shader object.
	@param ctx A shader compilation context.
	@param shader The shader object to compile.
	@return The return value is a handle to a SpireCompilationResult object that contains error messages and compiled source code.
	@note You are responsible for destorying a SpireCompilationResult object when it is no longer used. Destroying a SpireCompilationContext
	does not automatically destroy SpireCompilationResult objects.
	*/
	SPIRE_API SpireCompilationResult* spCompileShader(SpireCompilationContext * ctx, SpireShader * shader);

	/*!
	@brief Compiles a shader object.
	@param ctx A shader compilation context.
	@param source A string that represents the Spire source code that defines a shader.
	@param fileName The filename to use to report error messages regarding to @p source.
	@return The return value is a handle to a SpireCompilationResult object that contains error messages and compiled source code.
	@note You are responsible for destorying a SpireCompilationResult object when it is no longer used. Destroying a SpireCompilationContext
	does not automatically destroy SpireCompilationResult objects.
	@see spDestroyCompilationResult()
	*/
	SPIRE_API SpireCompilationResult* spCompileShaderFromSource(SpireCompilationContext * ctx, const char * source, const char * fileName);

	/*!
	@brief Checks if a compilation operation has succeeded.
	@param result The SpireCompilationResult object returned by spCompileShader().
	@return 1 if compilation is sucessful, 0 otherwise.
	*/
	SPIRE_API int spIsCompilationSucessful(SpireCompilationResult * result);

	/*!
	@brief Retrieve the number of compiler messages in a SpireCompilationResult object.
	@param result A SpireCompilationResult object.
	@param messageType The type of compiler message to check. Possible values are:
	- SPIRE_ERROR. compilation errors.
	- SPIRE_WARNING. compiler warnings.
	@return The number of messages of specified type.
	*/
	SPIRE_API int spGetMessageCount(SpireCompilationResult * result, int messageType);

	/*!
	@brief Retrieve the content of compiler messages in a SpireCompilationResult object.
	@param result A SpireCompilationResult object.
	@param messageType The type of compiler message to check. Possible values are:
	- SPIRE_ERROR. compilation errors.
	- SPIRE_WARNING. compiler warnings.
	@param index The index of the compiler message to retrieve.
	@param pMsg A pointer to a SpireErrorMessage structure to receive the error message.
	@return 1 if successful. SPIRE_ERROR_INVALID_PARAMETER if any of the parameters are invalid.
	*/
	SPIRE_API int spGetMessageContent(SpireCompilationResult * result, int messageType, int index, SpireErrorMessage * pMsg);

	/*!
	@brief Retrieve a list of shader names that has been compiled.
	@param result A SpireCompilationResult object.
	@param buffer A buffer used to receive shader names. Shader names are separated by '\\n'. If this parameter is NULL, the function returns the required buffer size.
	@param bufferSize The size (in bytes) of @p buffer.
	@return If sucessful, the return value is greater or equal to 0 representing the number of charaters required or written to buffer, including the trailing 0.
	Otherwise, it returns one of the following error codes:
	- SPIRE_ERROR_INSUFFICIENT_BUFFER. The supplied buffer size was not large enough.
	- SPIRE_ERROR_INVALID_PARAMETER. Any of the parameter values was invalid.
	*/
	SPIRE_API int spGetCompiledShaderNames(SpireCompilationResult * result, char * buffer, int bufferSize);

	/*!
	@brief Retrieve a list of stage names in a compiled shader.
	@param result A SpireCompilationResult object.
	@param shaderName The name of a shader.
	@param buffer A buffer used to receive stage names. Stage names are separated by '\\n'. If this parameter is NULL, the function returns the required buffer size.
	@param bufferSize The size (in bytes) of @p buffer.
	@return If sucessful, the return value is greater or equal to 0 representing the number of charaters required or written to buffer, including the trailing 0.
	Otherwise, it returns one of the following error codes:
	- SPIRE_ERROR_INSUFFICIENT_BUFFER. The supplied buffer size was not large enough.
	- SPIRE_ERROR_INVALID_PARAMETER. Any of the parameter values was invalid.
	*/
	SPIRE_API int spGetCompiledShaderStageNames(SpireCompilationResult * result, const char * shaderName, char * buffer, int bufferSize);

	/*!
	@brief Retrieve the compiled code (binary or textual, depending on the target language) of a stage in a compiled shader.
	@param result A SpireCompilationResult object.
	@param shaderName The name of a shader. If @p shaderName is NULL, the function returns the source code of the first shader in @p result.
	@param stage The name of a stage.
	@param[out] length A pointer used to receive the length of the compiled code, can be set to NULL.
	@return If sucessful, the return value is a pointer to the buffer storing the compiled code. Otherwise, the return value is NULL.
	@note The backing memory of the returned code buffer is owned by the SpireCompilationResult object. Destroying the SpireCompilationResult object will render this code
	buffer unusable.
	*/
	SPIRE_API char * spGetShaderStageSource(SpireCompilationResult * result, const char * shaderName, const char * stage, int * length);

	/*!
	@brief Destroys the SpireCompilationResult object.
	@param result A SpireCompilationResult object to destroy.
	@note Destroying a SpireCompilationContext object does not automatically destroy SpireCompilationResult objects. You are required to destroy a SpireCompilationResult object
	once it is no longer in use.
	*/
	SPIRE_API void spDestroyCompilationResult(SpireCompilationResult * result);

#ifdef __cplusplus  
}
#endif  

#endif