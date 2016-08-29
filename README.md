# Spire
Spire is a shading language and compiler framework that facilitates modular shader authoring and rapid exploration of shader optimization choices (such as frequency reduction and algorithmic approximation) afforded by modern real-time graphics engines. The current implementation of the Spire compiler can generate either GLSL or SPIR-V output for use with OpenGL and Vulkan based engines.

Paper: http://graphics.cs.cmu.edu/projects/spire/

##Tutorials:

[1. Understanding Spire](https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1) <br/>
[2. Using Shader Modules](https://github.com/csyonghe/Spire/blob/master/Docs/tutorial2) <br/>
[3. Integrating Spire in your engine](https://github.com/csyonghe/Spire/blob/master/Docs/UserGuide.md) <br/>
[4. Writing Pipeline Declarations](https://github.com/csyonghe/Spire/blob/master/Docs/tutorial3/README.md)

## Getting Started
To experience the benefit of using Spire as your engine's shading language, we have build a rendering engine that supports offline  texture prebaking, object space shading, screen space multi-resolution shading and shadow map generation. The engine uses the Spire compiler to generate GPU compute, vertex and fragment shaders for all these rendering stages. To see it working, compile the demo engine from `"Examples/SIGGRAPH16_Demo/DemoEngine/DemoEngine.sln"` and run the `SceneViewer` project. In `SceneViewer`, select "File"->"Open", and open "Examples/SIGGRAPH16_Demo/Scenes/couch/couch.world". This loads up a couch scene. The couch is rendered using [Examples/SIGGRAPH16_Demo/Scenes/couch/couch.shader](https://github.com/csyonghe/Spire/blob/master/Examples/SIGGRAPH16_Demo/Scenes/couch/couch.shader).

![](https://github.com/csyonghe/Spire/blob/master/Docs/sceneViewer.jpg)

The Choice Control window allows you to dynamically recompile the modified shader and explore different rate placement choices in real-time.

The SIGGRAPH'16 demo engine runs only on Windows with an NVIDIA Kepler or later GPU. AMD and Intel GPUs are not supported. 
However, the compiler is platform independent and is compatible with msvc (level 4 warning free), clang 3.7 and above (warning free) and g++ 5.0 (with `-fpermissive -std=c++14 -msse2` flag). 

Spire currently supports generating GLSL and SPIRV compute, vertex and fragment shaders. 

Now that you can get the demo engine running, <a href="https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1">read this tutorial</a> to learn the basics of Spire.

Also explore Examples/Scenes directory for more interesting shaders!

![](https://github.com/csyonghe/Spire/blob/master/Docs/sceneGallery.jpg)


##Targeting SPIR-V for Vulkan
New: take a look at our [VulkanEngine](https://github.com/csyonghe/Spire/tree/master/Examples/VulkanEngine) project to learn how to build a shading system that uses Spire's services to generate low level shader code for different platforms (SPIR-V for Vulkan and GLSL for OpenGL).
