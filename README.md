# Spire
Spire is a shading language and compiler framework that facilitates rapid exploration of shader optimization choices (such as frequency reduction and algorithmic approximation) afforded by modern real-time graphics engines.

Paper: http://graphics.cs.cmu.edu/projects/spire/

##Tutorials:

[Understanding Spire](https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1) <br/>
[Using Spire in your engine](https://github.com/csyonghe/Spire/blob/master/Docs/UserGuide.md)


## Getting Started
To experience the benefit of using Spire as your engine's shading language, we have build a rendering engine that supports offline  texture prebaking, object space shading, screen space multi-resolution shading and shadow map generation. The engine uses the Spire compiler to generate GPU compute, vertex and fragment shaders for all these rendering stages. To see it working, compile the demo engine from `"Examples/DemoEngine/DemoEngine.sln"` and run the `SceneViewer` project. In `SceneViewer`, select "File"->"Open", and open "Examples/Scenes/couch/couch.world". This loads up a couch scene. The couch is rendered using [Examples/Scenes/couch/couch.shader](https://github.com/csyonghe/Spire/blob/master/Examples/Scenes/couch/couch.shader).

![](https://github.com/csyonghe/Spire/blob/master/Docs/sceneViewer.jpg)

The Choice Control window allows you to dynamically recompile the modified shader and explore different rate placement choices in real-time.

Currently the demo engine runs only on Windows with an NVIDIA Kepler or later GPU. AMD and Intel GPUs are not supported. 
However, the compiler is platform independent and is compatible with both msvc and g++ 5.0 (with `-fpermissive -std=c++14 -msse2` flag). Spire currently supports generating GLSL compute, vertex and fragment shaders. 

## Understanding Spire
<a href="https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1">Read this tutorial</a> to learn the basics of Spire.
