#Writing Pipeline Declarations
As we have mentioned in [tutorial 1](https://github.com/csyonghe/Spire/tree/master/Docs/tutorial1),
you need to write a pipeline declaration so that the Spire compiler knows how to generate shader code
that your engine can use. [Docs/tutorial1/MultiRate.pipeline](https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/MultiRate.pipeline)
and [Docs/tutorial1/ObjSpace.pipeline](https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/ObjSpace.pipeline)
are examples of pipeline declarations. In this tutorial, we will examine all the details in a pipeline declaration.

In Spire, a pipeline declaration begins by
```
pipeline PipelineName
{
    ...
```

A pipeline declaration consists of three parts:
*World declarations - define worlds supported by the pipeline
*Requirement delcarations - declarate components required by the pipeline
*Import operator declarations - declarate data flow pathes between worlds

Now let's go through each part of a pipeline delcaration. Assume you have implemented a simple rendering engine
that supports vertex and fragment shading. Your engine also pass in addtional uniform inputs using a uniform buffer object.
Let's say your uniform buffer object has the following content
```c++
struct ViewUniforms
{
    Matrix4 modelTransform;
    Matrix4 viewProjectionTransform;
    Vec3 lightDir;  
};
```
and your engine uses the following vertex format
```c++
struct Vertex
{
    Vec3 vertPos;
    Vec2 vertUv;
    Vec3 vertNormal; 
};
```
Let's say you use Spire as library (as in [this tutorial](https://github.com/csyonghe/Spire/blob/master/Docs/UserGuide.md)),
and at initialization time, you compile Spire shaders and set up OpenGL program objects (assuming the Spire shader
defines `vs` world for vertex shader and `fs` world for fragment shader):
```
    Spire::Compiler::CompileResult result;
    Spire::Compiler:CompileOptions options;
    auto compiledShaders = SpireLib::CompileShaderSource(result, shaderSrc, options);
    
    auto vs = createGLshader(compiledShaders[0].Sources["vs"], GL_VERTEX_SHADER);
    auto fs = createGLshader(compiledShaders[0].Sources["fs"], GL_FRAGMENT_SHADER);
    auto program = createGLprogram(vs, fs);
```
At each frame, the engine binds the uniform buffer at binding point 0 and draws a mesh:
```
    glBindVertexArray(vao);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, uniformBufferObj);
    glUseProgram(program);
    glDrawElements(...);
```
Now let's see how to declare this render pipeline in Spire, so the Spire compiler can generate proper shader code
for this engine.


##World Declarations

###Input Worlds

First of all, we need to tell Spire what are the worlds supported by the pipeline. Worlds can have arbitrary names and
Spire do not need to know the meaning of a world (or how your engine is going to use it). Spire only need to know there
is a world named X. There are two types of worlds in Spire: input worlds and regular worlds. Input worlds are used to 
define shader inputs. For example, you can define a world that holds all vertex attributes:
```glsl
[Pinned][Packed]
input world rootVert;
``` 
By default, Spire compiler generates one shader interface block per world, so all components defined in `rootVert` world
will be in the same interface block. Additionally, Spire supports several attributes that control the code generation of
an input world. These attributes are:
*`Pinned` - informs the compiler not to eliminate unsed components from the resulting interface block.
*`Packed` - informs the compiler the components defined in the input world are tightly packed. If this attribute is not declared,
the compiler uses `std140` layout.
*`InterfaceBlockIndex` - specify the binding index of the resulting interface block from this input world.
*`InterfaceBlock` - specify the resulting interface block name (and optionally the binding index) of the resulting interface block.

Therefore, the above declaration says that there is an input world called `rootVert`, and the input comes in a tightly packed
buffer.

So to declare our uniform buffer input, we need another input world:
```
[Pinned]
[InterfaceBlock: "view_uniforms: 0"]
input world viewUniforms;
```
Declares an input world named `viewUniforms`, and informs the backend of Spire to pack all components defined in 
`viewUniforms` world into a buffer called `view_uniforms`, and the buffer should have `std140` layout, 
and a component should appear in the interface block even if it is not used in the shader. The actual type of the buffer
(e.g. whether it is an uniform buffer, shader storage buffer or vertex buffer) is determined by the type of import operator
associated with the data path to the user world, which we will talk about later in this tutorial. 
For now you can assume that compiling the following shader with the above pipeline definition:
```glsl
shader Test
{
    @viewUniform mat4 modelTransform;
    @viewUniform mat4 modelViewTransform;
    @viewUniform vec3 lightDir;
}
```
yields the following interface block declaration in the generated GLSL source:
```glsl
layout(std140, binding = 0)
uniform view_uniforms
{
    mat4 modelTransform;
    mat4 modelViewTransform;
    vec3 lightDir;
} blkview_Uniforms;
```
Which matches the engine's definition of the uniform buffer.

###Regular Worlds
A regular world represent a compute stage that the compiler can generate shdaer code for, it corresponds to a compute,
vertex or fragment shader of the compiled shader source. 
For example, because we want to use Spire to generate vertex and fragment shader
from a Spire shader, we should define two worlds
```
world vs;
world fs;
```
This tells Spire to generate code for two worlds named `vs` and `fs`, but we need to tell more about what `vs` and
`fs` is.

First, we need to tell the compiler to generate GLSL code for `vs` world, and the output of 'vs' world should be defined
using the standard `out` syntax in GLSL:
```
world vs: "glsl" export standardExport;
``` 
Here the `export` clause specifies the **export operator** of `vs` world. Export operators are used to tell Spire how 
to treat the output of the world. `standardExport` is the export operator that generates the standard `out` declarations
in GLSL.

Similarly the `fs` world can be declared as:
```
world fs: "glsl" export fragmentExport;
```
Here we are using `fragmentExport` instead of `standardExport` for fragment shader outputs, this tells Spire that the output
the `fs` world will be stored in framebuffers or textures.

Note that we are still missing one important piece of information in the `vs` declaration: we haven't tell the Spire compiler
what the projected vertex output (`gl_Position`) is for the vertex shader. To do that we need the second part of pipeline
declaration: pipeline requirements.

##Requirement Declarations
Pipeline requirements are declared using `require` statement (same as the `require` statement used to define shader module
parameters). Declaring a requirement in the pipeline means all shaders written against the pipeline must provide a definition
of the required component at specified world. For example, with the statement
```glsl
require @vs vec4 projCoord;
```
the Spire compiler guarantees
all verified shaders will have a definition of `projCoord` at `vs` world. Now, we can modify our `vs` world declaration
to inform the compiler that `vs` correspond to a vertex shader, and should use `projCoord` as the `gl_Position` output:
```glsl
world vs : "glsl(vertex:projCoord)" using projCoord export standardExport;
```
Here `"glsl(vertex:projCoord)"` means Spire should use the `"glsl"` backend to generate code for `vs` world, and 
`"vertex:projCoord"` is an additional argument passed to that backend, informing the GLSL backend to emit vertex shader code
by using `projCoord` component as the `gl_Position` output. The `using projCoord` clause tells the Spire compiler that the 
backend will reference `projCoord` component, so the compiler should make sure `projCoord` is always available at this world.

Right now, our pipeline definition should look like the following:
```glsl
pipeline DemoPipeline
{
    [Pinned] [Packed] 
    input world rootVert; 
    
    [Pinned] [InterfaceBlock: "view_uniforms: 0"] 
    input world viewUniform; 
   
    world vs : "glsl(vertex:projCoord)" using projCoord export standardExport;
    world fs: "glsl" export fragmentExport;
    
    require @vs vec4 projCoord;
}
```

##Import Operator Declarations

So far we have declared the input worlds, the `vs` and `fs` world as well as the `projCoord` requirement in `DemoPipeline`.
There is one more thing we need to do: declare the data pathes between worlds. In this pipeline, the data pathes are:
*`rootVert` -> `vs`: vertex shader(`vs`) should have access to the input vertex stream(`rootVert`)
*`viewUniform` -> `vs`: vertex shader(`vs`) should have access to the uniform buffer(`viewUniform`)
*`viewUniform` -> `fs`: fragment shader(`fs`) should also have access to the uniform buffer(`viewUniform`)
*`vs`->`fs`: fragment shader(`fs`) should have access to vertex shader outputs(`vs`)

Note that all of the listed data pathes are direct pathes: we do not need to declare transitive pathes (such as `rootVert`->`fs`),
these transitive pathes will be implemnted automatically by the compiler.
 
Besides the source and destination worlds, Spire also need to know the type of the data path (e.g. is it importing a uniform buffer,
or is it reading from a vertex stream?), because different type of data path requires slightly different code generation logic.

Spire models different type of data path using import operators. Currently Spire implements five import operators:
* `uniformImport` is used to define dependency between an uniform input world and a regular world. 
  the source world of an uniformImport dependency will be compiled to an uniform buffer definition. 
* `vertexImport` is used to define dependency between an vertex buffer input world and a regular world. 
  the source world of an vertexImport dependency will be regarded as the vertex attribute definition. 
  the destination world must be used as a vertex shader. 
* `standardImport` is used to define dependency between GPU pipeline stages,  
  such as from vertex shader to fragment shader 
* `textureImport` is used to define dependency on a fragment shading world. 
  Spire compiler will inject texture fetch operations in the user world to fetch the textures produced 
  by the source world. 
* `bufferImport` is used to inform the compiler that the output of source world is stored in a general 
  buffer. Spire will inject buffer fetch instruction to load the value at gl_InvocationIndex. 

For our `DemoPipeline`, we should use these import operators:
*`rootVert`->`vs` should use `vertexImport`
*`viewUniform`->`vs` and `viewUniform`->`fs` should use `uniformImport`
*`vs`->`fs` should use `standardImport`

After declaring the import operators, our final pipeline delcaration is:
```glsl
pipeline DemoPipeline
{
    [Pinned] [Packed] 
    input world rootVert; 
    
    [Pinned] [InterfaceBlock: "view_uniforms: 0"] 
    input world viewUniform; 
   
    world vs : "glsl(vertex:projCoord)" using projCoord export standardExport;
    world fs: "glsl" export fragmentExport;
    
    require @vs vec4 projCoord;
    
    import vertexImport(rootVert->vs);
    import uniformImport(viewUniform->vs);
    import uniformImport(viewUniform->fs);
    import standardImport(vs->fs);
}
```

##Declaring Engine-provided Inputs in Shader Modules
Since the engine defines the data in vertex stream and uniform buffer, we can define them in separate shader modules
so other shaders can just use these modules without repeatedly declaring them.

In this case we can define those inputs as following:
```glsl
module VertexInput
{
    public @rootVert vec3 vertPos;
    public @rootVert vec2 vertUv;
    public @rootVert vec3 vertNormal;
}

module ViewUniforms
{
    public @viewUniform mat4 modelTransform;
    public @viewUniform mat4 viewProjectionTransform;
    public @viewUniforms vec3 lightDir;
}
```

That is everything you need to declare the engine pipeline in Spire! You should now understand the all the 
basics regarding the pipeline declaration and how Spire's code generator works. We have not covered more advanced 
import operators such as `textureImport` and `bufferImport` in this tutorial, and you are encouraged to look
at our DemoEngine code as well as [Docs/tutorial1/MultiRate.pipeline](https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/MultiRate.pipeline)
to learn how to use them to declare complex pipelines that involve object space and screen space multi-rate rendering.