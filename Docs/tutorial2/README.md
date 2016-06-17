#Using Shader Modules

In [previous tutorial](https://github.com/csyonghe/Spire/tree/master/Docs/tutorial1), you've seen what a basic Spire shader looks like. Our first shader contains many different type of shading logic, 
such as vertex input declaration, vertex and normal transformation, pattern generation and lighting. In practice, we often want
to encapsulate different types of shading concerns into separate modules for code re-use or just making shader code cleaner and easier to read.
In this tutorial, we are going to talk about the modularity features supported by Spire.

##Shader Modules
A shader module can be defined with the syntax
```glsl
module ModuleName
{
	...Definitions...
}
```
A `module` is treated almost the same as a `shader` by the compiler, except that modules do not triger shader code generation - the compiler is only going
to generate final shader code for a `shader`. Modules are supposed to be a part of a shader, and do not need to contain all pipeline-required components.

##Using Modules as "#include"
The simplest way to use modules is to "include" them in another module or shader.
For example you can define all vertex attribute inputs of `Demo1Shader` from tutorial 1 as a module:
```glsl
module VertexInput
{
	// define vertex inputs
    public @rootVert vec3 vert_pos;
    public @rootVert vec3 vert_normal;
    public @rootVert vec3 vert_tangent;
    public @rootVert vec2 vert_uv;
}
```
And `Demo1Shader` can be changed to use the `VertexInput` module instead of re-defining all the vertex attributes:
```glsl
shader DemoShader
{
	using VertexInput;

	... rest of shader definition ...
}
```
The using statement above can be thought as C++'s `#include`: it imports all definitions in `VertexInput` module into the current namespace, so the rest of 
`Demo1Shader` can reference the vertex attributes directly as if they are defined in `DemoShader`.

Alternatively, you can import all definitions from `VertexInput` into a separate namespace, as demostrated in the following code
```glsl
shader DemoShader
{
	using vertex = VertexInput;
	...
	vec4 projPos = viewProjectionMatrix * vec4(vertex.vert_pos, 1.0);
}
```
This places all definitions of `VertexInput` into its own namespace `vertex`, and to reference to a vertex attribute, you can write
```
vertex.vert_pos
``` 
Note that in this example, all components in `VertexInput` are marked as `public`. Anything that is not marked as `public` will not be visible from other modules.


##Using Modules as Functions

Modules can also be used as functions. For example, you can define a module that performs tangent frame transformation on a normal vector:
```glsl
module TangentFrameTransform
{
    require vec3 inputVector;
    require vec3 vNormal;
    require vec3 vTangent;
    require vec3 vBiNormal;
    public vec3 result = normalize(vTangent * inputVector.x 
                                   + vBiNormal * inputVector.y 
                                   + vNormal * inputVector.z);
}
```
As you can see, parameters of a module are defined via `require` clause. Return values of a module are defined as normal public components.
This module can then be used like a function in another module or shader like the following:
```glsl
shader DemoShader
{
	vec3 normal = ...;
	vec3 vNormal = ...;
	vec3 vTangent = ...;
	vec3 vBiNormal = ...;
	using nt = TangentFrameTransform(normal, vNormal, vTangent, vBiNormal);
	vec3 transformedNormal = nt.result;
}
```
###Named Argument Passing
You can also use the named argument syntax (similar to C#):
```glsl
shader DemoShader
{
	vec3 normal = ...;
	vec3 vNormal = ...;
	vec3 vTangent = ...;
	vec3 vBiNormal = ...;
	using nt = TangentFrameTransform(inputVector: normal,
	                                 vNormal: vNormal, 
									 vTangent: vTangent, 
									 vBiNormal: vBiNormal);
	vec3 transformedNormal = nt.result;
}
```
###Implicit Argument Passing
As a syntactic convenience, Spire allows you to omit argument specification if there is already a component with the same name as the parameter in current
name scope. For example in the above case, `DemoShader` defines `vNormal`, `vTangent` and `vBiNormal` component, and they have the same name of 
`TangentFrameTransform`'s paremeters. Therefore you can omit the specification for these arguments, like the following:
```glsl
shader DemoShader
{
	vec3 normal = ...;
	vec3 vNormal = ...;
	vec3 vTangent = ...;
	vec3 vBiNormal = ...;
	using nt = TangentFrameTransform(inputVector: normal);
	vec3 transformedNormal = nt.result;
}
```
Since the only parameter that requires an explicit argument is `inputVector`, which is the first parameter, you can use the short syntax as well:
```glsl
shader DemoShader
{
	vec3 normal = ...;
	vec3 vNormal = ...;
	vec3 vTangent = ...;
	vec3 vBiNormal = ...;
	using nt = TangentFrameTransform(normal);
	vec3 transformedNormal = nt.result;
}
```
In both cases, the compiler searches in the scope of DemoShader to find components with the same name and use them as arguments. However if you are 
using the short syntax, you must specify arguments according the same order as the corresponding parameters. If at some point you want to skip
some parameters in the middle, the rest of the parameters must be specified in the explicitly named form. For example
```glsl
shader DemoShader
{
	vec3 normal = ...;
	vec3 vNormal = ...;
	vec3 vTangent = ...;
	vec3 vBiNormal = ...;
	using nt = TangentFrameTransform(normal, // we are skipping vNormal parameter here
			vTangent: vTangent // use named argument to specify rest of arguments
			vBiNormal: vBiNormal
			);
	vec3 transformedNormal = nt.result;
}
```

##Examples of Using Modules
So that is all the syntax basics you need to know about modules. Now let's rewrite our `Demo1Shader` using several sub-modules. You can find the 
full source code for the rewritten shader at [Docs/tutorial2/Demo.shader](https://github.com/csyonghe/Spire/blob/master/Docs/tutorial2/Demo.shader). To see
the demo shader, compile `Examples/DemoEngine/DemoEngine.sln` and run `SceneViewer`, in `SceneViewer`, select File->Open and choose 
`Docs/tutorial2/Demo.world`.

First, we can define all vertex attributes in a module:
```glsl
module VertexInput
{
    // define vertex inputs
    public @rootVert vec3 vert_pos;
    public @rootVert vec3 vert_normal;
    public @rootVert vec3 vert_tangent;
    public @rootVert vec2 vert_uv;   
}
```
Next, we use another module to hold all engine-provided uniform inputs:
```glsl
module SystemUniforms
{
    // define engine provided uniforms
    public @modelTransform mat4 modelMatrix; 
    public @modelTransform mat4 normalMatrix;
    
    // define view and environment uniform inputs
    public @viewUniform mat4 viewProjectionMatrix;
    public @viewUniform mat4 viewMatrix;
    public @viewUniform mat4 projectionMatrix;
    public @viewUniform mat4 invViewMatrix;
    public @viewUniform mat4 invViewProjectionMatrix;
    public @viewUniform vec3 cameraPos;
    public @viewUniform vec3 lightColor;
    public @viewUniform vec3 lightDir;
}
``` 
And then, we define a module that does vertex transform and computes the `projCoord` `texSpaceVert` and `screenCoord` that are required 
`MultiRatePipeline`.
```glsl
module VertexTransform
{
    require mat4 modelMatrix;
    require mat4 viewProjectionMatrix;
    require vec3 vert_pos;
    require vec2 vert_uv;
      
    // compute projected vertex position
    public vec4 position = modelMatrix * vec4(vert_pos, 1.0);
    public vec4 projCoord = viewProjectionMatrix * position;
    
    // pipeline requires a texture space vertex for object space rendering
    // here we require the mesh provides a unique parameterization stored in vert_uv
    public vec4 texSpaceVert = vec4(vert_uv*2.0 - vec2(1.0), 0.0, 1.0);           
    public vec2 screenCoord = projCoord.xy/vec2(projCoord.w)*0.5 + vec2(0.5);
}
```
Note that in `VertexTransform` module,
all required components are declared as parameters using the `require` statement, instead of providing the actual definition of those components.

Next, we can define another module that contains all the material pattern generation logic:
```glsl
module Material
{
    require vec2 vert_uv;
    
    // define material uniform inputs
    @perInstanceUniform sampler2D ground_pebble_map;
    @perInstanceUniform sampler2D ground_pebble_Nmap;    
    
    vec2 uv = vert_uv * 4.0; // tile the texture
    public vec3 albedo = texture(ground_pebble_map, uv).xyz;

    public vec3 normal
    {
        // fetch normal map
        vec3 normalTex = (texture(ground_pebble_Nmap, uv).xyz - 0.5) * 2.0;
        return normalTex;
    }
}
```
The `Material` module requires a `vert_uv` parameterization and returns the `albedo` and `normal` (in tangent space) for the given surface position.

Next, we have a module that does tangent space to world space normal transformation:
```glsl
module TangentFrameTransform
{
    require vec3 inputVector;
    require mat4 normalMatrix;
    require vec3 vert_normal;
    require vec3 vert_tangent;
    
    vec3 vNormal = (normalMatrix * vec4(vert_normal, 1.0)).xyz;
    vec3 vTangent = (normalMatrix * vec4(vert_tangent, 1.0)).xyz;
    vec3 vBiTangent = cross(vTangent, vNormal);
    
    public vec3 result = normalize(inputVector.x * vTangent 
                    + inputVector.y * vBiTangent 
                    + inputVector.z * vNormal);
}
```
And last, we define a lighting module:
```glsl
module Lighting
{
    require vec3 albedo;
    require vec3 normal;
    require vec3 lightColor;
    require vec3 lightDir;
    require vec4 position;
    require vec3 cameraPos;
    
    vec3 view = normalize(cameraPos - position.xyz);
    // compute lighting
    float diffuse = clamp(dot(lightDir, normal), 0.0, 1.0);
    float specular = ComputeHighlightPhong(lightDir, normal, view, 0.5, 0.5, 0.4);  
    public vec4 result = vec4(lightColor * 
         (albedo * (diffuse * 0.7 + 0.5) * 0.6 + 
         mix(albedo, vec3(1.0), 0.6) * specular), 1.0);  
}
```
Again, in the defintion of `Lighting` module, we are just declaring all we need as parameters, using the `require` statement. We do not need to care
where do those parameters actually come from.

Now that we have all the modules defined, let's piece them together into a final shader!

First, we define our shader and include both `VertexInput` and `SystemUniforms`:
```glsl
shader DemoShader
{
    using VertexInput;
    using SystemUniforms;
```
After using these two modules, the current name scope of `DemoShader` contains all public components defined in
`VertexInput` and `SystemUniforms`.

We can continue to include the vertex transform logic by using `VertexTransform` module:
```glsl
    using VertexTransform;
```
Note that `VertexTransform` module requires `modelMatrix` `viewProjectionMatrix` `vert_pos` and `vert_uv`. Since we have already included 
`VertexInput` and `SystemUniforms`, the current name scope of `DemoShader` already contains component definitions of the same names. So 
we do not need to explicitly specify those arguments here.

Next, we use the `Material` module to compute material properties (`albedo` and `normal`):
```glsl
    using material = Material();
```
Again, `Material` module requires `vert_uv`, but we do not need to explicitly specify that argument because `vert_uv` is already in current scope
due to inclusion of `VertexInput`.

Then, we use the `TangentFrameTransform` module to transform the tangent space normal returned by `Material` module into world space, so we can use it
for lighting:
```glsl
    using normalTransform = TangentFrameTransform(material.normal);
```
Here, we explicitly specify the `inputVector` parameter, but leave all other parameters implicitly specified.

Now we have everything ready, let's use the `Lighting` module to compute final lighting result and use it as our color output:
```glsl
    using lighting = Lighting(material.albedo, normalTransform.result);
    vec4 outputColor = lighting.result;
}
```

So that is `Demo1Shader` in tutorial 1 rewritten using modules. Note that many modules we defined here can be re-used over different shaders, and 
these modules can be replaced with a slightly different implementation if needed. For example, you can define different lighting modules, and 
different VertexInput modules to work with different shading requirements.

Also note that the by using implicit argument passing syntax properly, we can save ourselves a lot of effort in explicitly specifying arguments over
and over again.

##Modules vs Plain Functions
Besides modules, Spire also support plain functions. Functions in spire are the same as in GLSL. The difference is that
the entire body of the function will be executed at the world where the function is called, 
but a module can contain components that are computed at different worlds. You can view modules as an enhanced version of
functions with multi-rate logic. And the compiler supports additional named argument and implicit argument syntax for modules.