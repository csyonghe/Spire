#Vulkan Engine

This is an engine that uses both Vulkan and OpenGL as the rendering API. The engine uses Spire as its shading language, and use Spire compiler to generate GLSL code for the OpenGL rendering API as well as SPIRV for the Vulkan rendering API.

To get started, compile the "GameEngine.sln" using Visual Studio 2015, and run GameEngine project with the following command line:
```
GameEngine.exe -dir "$(SolutionDir)ExampleGame" -enginedir "$(SolutionDir)EngineContent"
```
By default, the engine uses OpenGL as the rendering API, to use Vulkan and SPIRV, start GameEngine with the following command line instead:
```
GameEngine.exe -dir "$(SolutionDir)ExampleGame" -enginedir "$(SolutionDir)EngineContent" -vk -gpu 0
```
If your system has more than one GPU installed, you can change the -gpu argument to another index to use a different GPU.
