#Vulkan Engine
Developers: Yong He and Teguh Hofstee
##Introduction

This is an engine that uses both Vulkan and OpenGL as the rendering API. The engine uses Spire as its shading language, and use Spire compiler to generate GLSL code for the OpenGL renderer as well as SPIRV for the Vulkan renderer.

To get started, compile "GameEngine.sln" using Visual Studio 2015, and run GameEngine project with the following command line:
```
GameEngine.exe -dir "$(SolutionDir)ExampleGame" -enginedir "$(SolutionDir)EngineContent"
```
By default, the engine uses OpenGL as the rendering API, to use Vulkan and SPIRV, start GameEngine with the following command line instead:
```
GameEngine.exe -dir "$(SolutionDir)ExampleGame" -enginedir "$(SolutionDir)EngineContent" -vk -gpu 0
```
If your system has more than one GPU installed, you can change the -gpu argument to another index to use a different GPU.

##Platform Compatibility Status
OpenGL renderer: runs on NVIDIA, AMD and Intel GPUs.

Vulkan renderer: runs on NVIDIA GPUs.



