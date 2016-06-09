#Understanding a Spire Shader and Its Space of Optimization Choices

In this tutorial, we are going to walk through a basic Spire shader and cover the basics of Spire. You will learn how Spire shaders can be compiled into different GLSL shader implementations for use by a simple rendering engine. The different implementations reflect different performance-quality trade-offs.
##The Demo Engine
To start with the tutorial, we have created a demo rendering engine that consists of two rendering phases: object space shading and full screen resolution (per fragment) shading.

<img src="https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/img/1.png" width="500px"/><br/>
Figure 1. Rendering phases supported by the Demo Engine

At each frame, the engine renders object-space textures for each object, and these textures are made available to subsequent phases. The engine implements object space shading by rasterizing the mesh using its texture coordinate (assuming the texture coordinates represent an unique parameterization of the mesh surface), and storing each object space shading output in a separate texture.
Since all rendering phases are done on the GPU, each of them includes a vertex shading stage and fragment shading stage, hence the rendering pipeline has four rendering stages in total, as illustrated in Figure 2. 

<img src="https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/img/2.png" width="500px"/><br/>
Figure 2. Rendering stages of our Demo Engine

In Spire’s terminology, a shading stage in a rendering pipeline corresponds to a `world`. Spire sees a pipeline as a set of worlds and the dependency between the worlds. When compiling a Spire shader, the compiler generates one GPU shader for each world. In this example pipeline, four GLSL shaders will be produced from compilation of one Spire shader.

If we write GLSL shaders directly for this pipeline, in most cases we need to create four shaders for each type of material and carefully define the input/output interfaces for each stage. If we want to try computing some terms in a different shading stage (world), for example computing high frequency specular highlight at object space to reduce aliasing, all four shaders would need to change.

Spire allows you to define the shading logic across the entire pipeline and quickly explore different placement decisions without altering the shader code. In the rest of the tutorial, we will go through a simple Spire shader that fetches normal map and computes Phong lighting, and learn how Spire makes it easy to explore different choices such as moving specular lighting to object space.

To use Spire to compile shaders for our pipeline, we need to do two things. First, we need to delcare the rendering pipeline so the compiler knows how to generate code. Then, we can write Spire shaders containing logic for all these worlds. The Spire declaration of the above pipeline can be found at https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/ObjSpace.pipeline
We will cover the details of pipeline declaration in later tutorials. For now you only need to know that a pipeline declaration tells Spire the set of worlds as well as the dependency between the worlds. ObjSpace.pipeline declares a pipeline with four worlds: objSurfaceVs, objSurface, vs, and fs, which correspond to the rendering stages illustrated in Figure 2. In addition, we also defined four input worlds: rootVert, viewUniform, modelTransform and perInstanceUniform. Input worlds do not correspond to a shading stage, but are instead used as sources of shader inputs. For example, rootVert is for holding vertex inputs and viewUniform for passing in per-frame uniforms. 


##The First Spire Shader

We can write our first Spire shader for this pipeline, which can be found at https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/Demo1Shader.shader

You can load the scene and play with the shader by starting SceneViewer, and open “Docs/tutorial1/demo1.world”.

Demo1Shader computes projected vertex positions, samples albedo and normal map, and computes directional lighting using the phong model. Here is a screenshot of the shader:

<img src="https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/img/3.png"/><br/>
Figure 3. Screenshot of our first shader

In the first line, we reference the file that contains the pipeline declaration:
```c++
using "../ObjSpace.pipeline";
```
Then we can immediately start to define our shader:
```glsl
shader Demo1Shader
{
```
A Spire shader consists of a set of components. A shader component is either an input, or a intermediate shading effect, or the final output. We first define all the inputs to the shader:
```glsl
    // define vertex inputs
    @rootVert vec3 vert_pos;
    @rootVert vec3 vert_normal;
    @rootVert vec3 vert_tangent;
    @rootVert vec2 vert_uv;
    
    // define transofrm matrix inputs 
    @modelTransform mat4 modelMatrix; 
    @modelTransform mat4 normalMatrix;
    
    // define material uniform inputs
    @perInstanceUniform sampler2D ground_pebble_map;
    @perInstanceUniform sampler2D ground_pebble_Nmap;
    
    // define view and environment uniform inputs
    @viewUniform mat4 viewProjectionMatrix;
    @viewUniform mat4 viewMatrix;
    @viewUniform mat4 projectionMatrix;
    @viewUniform mat4 invViewMatrix;
    @viewUniform mat4 invViewProjectionMatrix;
    @viewUniform vec3 cameraPos;
    @viewUniform vec3 lightColor;
    @viewUniform vec3 lightDir;
```
These components stand for inputs from vertex stream, per-object or per-frame uniform buffers.
You may want to ask, since the GLSL code of defining and accessing vertex attributes is different from that of a uniform, how does Spire compiler generate the correct GLSL definitions? The answer is we have already specified all of these code generation details in the pipeline declaration. In the pipeline declaration, we define the set of worlds, and specify the type of world dependencies that defines how to generate correct GLSL code.
After defining all the inputs, we can start to define our shading logic. First we compute the projected vertex coordinates:
```glsl
     // compute projected vertex position
    vec4 position = modelMatrix * vec4(vert_pos, 1.0);
    vec4 projCoord = viewProjectionMatrix * position;
```
Note that instead of writing `gl_Position = ...`, we are only defining a shader component called `projCoord`. The compiler is able to generate the correct vertex shader from this because the pipeline declaration has required every Spire shader to provide a component named `projCoord`, and also specified that `projCoord` is the projected vertex output for `vs`, thus the GLSL backend has everything needed to produce a valid vertex shader.
Next we have:
```glsl
    vec4 texSpaceVert = vec4(vert_uv*2.0 - vec2(1.0), 0.0, 1.0); 
```
This defined the `texSpaceVert` component, which is used by the pipeline as the projected vertex position for object space rendering phase. The pipeline declaration has told the GLSL backend to look for `texSpaceVert` as the `gl_Position` value for the vertex shader of object space shading phase.
Next comes some simple pattern generation logic that fetches albedo and normal textures:
```glsl
    vec2 uv = vert_uv * 10.0; // tile the texture
    vec3 albedo = texture(ground_pebble_map, uv).xyz;
    
    // compute normal from normal map
    vec3 normal
    {
        vec3 vNormal = (normalMatrix * vec4(vert_normal, 1.0)).xyz;
        vec3 vTangent = (normalMatrix * vec4(vert_tangent, 1.0)).xyz;
        vec3 vBiTangent = cross(vTangent, vNormal);
        // fetch normal map
        vec3 normalTex = (texture(ground_pebble_Nmap, uv).xyz - 0.5) * 2.0;
        // transform to world space
        return normalize(normalTex.x * vTangent 
                    + normalTex.y * vBiTangent 
                    + normalTex.z * vNormal);
    }
```
Here, the normal component involves more complex tangent frame transformations, so instead of writing it as an expression, we write it in a code block.
Next, we define the lighting logic:
```glsl
    vec3 view = normalize(cameraPos - position.xyz);
    
    // compute lighting
    float diffuse = clamp(dot(lightDir, normal), 0.0, 1.0);
    float specular = ComputeHighlightPhong(lightDir, normal, view, 0.5, 0.5, 0.4);
```
And finally, we combine everything into the final fragment output and completes our first shader:
```glsl
    vec4 outputColor = vec4(lightColor * 
         (albedo * (diffuse * 0.7 + 0.5) * 0.6 + 
         mix(albedo, vec3(1.0), 0.6) * specular), 1.0);
}
```
Note that we didn’t provide any specifier marking `outputColor` as output, as this has already been stated in the pipeline declaration.

##Shader Variants

In Demo1Shader, we defined a set of shading terms, e.g. `position`, `normal`, `specular` etc., which are called shader components. Note that in the shader definition, we have not specified at which world to place each shader component - the shader definition is simply a DAG of components. The compiled shaders could be different if we decide to compute a component (e.g. `specular`) at different worlds. The set of compiled shaders after applying world placement decisions is called a shader variant. 

###The Default Variant

By default, the Spire compiler will prefer to compute a shader component at latest possible world (e.g. `fs` in this pipeline) if the world to compute the component is not explictly specified. In most cases, this is the most expensive but highest quality choice (compute everyting at fragment shader). For example, directly compiling the above Spire shader yields the following shader variant visualized in Figure 4.

<img src="https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/img/4.png" width="650px"/><br/>
Figure 4. The default shader variant result from compiling Demo1Shader directly. 

In Figure 4, each column correspond to a world, and each box in the column correspond to a shader component. The shader component in yellow text are components appear in inter-world interfaces, for example, position is computed in vs and passed to fs.  

The shader variant shown in Figure 4 is equivalent to the following GLSL shaders:

```glsl
// vs:
/**************************************************************************/
#version 450
layout(std140, binding = 1) uniform viewUniform
{
	mat4 viewProjectionMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 invViewMatrix;
	mat4 invViewProjectionMatrix;
	vec3 cameraPos;
	vec3 lightColor;
	vec3 lightDir;
} blkviewUniform;

layout(std140, binding = 0) uniform modelTransform
{
	mat4 modelMatrix;
	mat4 normalMatrix;
} blkmodelTransform;

layout(location = 0) in vec3 vert_pos;
layout(location = 1) in vec3 vert_normal;
layout(location = 2) in vec3 vert_tangent;
layout(location = 3) in vec2 vert_uv;

out vs
{
	vec3 vert_normal;
	vec3 vert_tangent;
	vec2 vert_uv;
	vec4 position;
} blkvs;

void main()
{
	blkvs.vert_normal = vert_normal;
	blkvs.vert_tangent = vert_tangent;
	blkvs.vert_uv = vert_uv;
	vec4  = (blkmodelTransform.modelMatrix * vec4(vert_pos, 1.0));
	blkvs.position = _vcmpposition;
	vec4 _vcmpprojCoord = (blkviewUniform.viewProjectionMatrix * _vcmpposition);
	gl_Position = _vcmpprojCoord;
}
/*********************************************************************************/
```
```glsl
// fs:
/********************************************************************************/
#version 450
layout(std140, binding = 2) uniform perInstanceUniform
{
	sampler2D ground_pebble_map;
	sampler2D ground_pebble_Nmap;
} blkperInstanceUniform;

layout(std140, binding = 1) uniform viewUniform
{
	mat4 viewProjectionMatrix;
	mat4 viewMatrix;
	mat4 projectionMatrix;
	mat4 invViewMatrix;
	mat4 invViewProjectionMatrix;
	vec3 cameraPos;
	vec3 lightColor;
	vec3 lightDir;
} blkviewUniform;

layout(std140, binding = 0) uniform modelTransform
{
	mat4 modelMatrix;
	mat4 normalMatrix;
} blkmodelTransform;

in vs
{
	vec3 vert_normal;
	vec3 vert_tangent;
	vec2 vert_uv;
	vec4 position;
} blkvs;

layout(location = 0) out vec4 outputColor;

float ComputeHighlightPhong(vec3 p_L, vec3 p_N, vec3 p_V, float p_roughness, float p_metallic, float p_specular);

void main()
{
	vec2 _vcmpuv = (blkvs.vert_uv * 10.0);
	vec3 _vcmpalbedo = texture(blkperInstanceUniform.ground_pebble_map, _vcmpuv).xyz;
	vec3 v0_vNormal = (blkmodelTransform.normalMatrix * vec4(blkvs.vert_normal, 1.0)).xyz;
	vec3 v1_vTangent = (blkmodelTransform.normalMatrix * vec4(blkvs.vert_tangent, 1.0)).xyz;
	vec3 v2_vBiTangent = cross(v1_vTangent, v0_vNormal);
	vec3 t2a = texture(blkperInstanceUniform.ground_pebble_Nmap, _vcmpuv).xyz;
	vec3 v3_normalTex = (t2a - 0.5) * 2.0;
	vec3 _vcmpnormal = normalize((((v3_normalTex.x * v1_vTangent) + (v3_normalTex.y * v2_vBiTangent)) + (v3_normalTex.z * v0_vNormal)));
	vec3 _vcmpview = normalize((blkviewUniform.cameraPos - blkvs.position.xyz));
	float _vcmpdiffuse = clamp(dot(blkviewUniform.lightDir, _vcmpnormal), 0.0, 1.0);
	float _vcmpspecular = ComputeHighlightPhong(blkviewUniform.lightDir, _vcmpnormal, _vcmpview, 0.5, 0.5, 0.4);
	vec4 _vcmpoutputColor = vec4((blkviewUniform.lightColor * (((_vcmpalbedo * ((_vcmpdiffuse * 0.7) + 0.5)) * 0.6) + (mix(_vcmpalbedo, vec3(1.0), 0.6) * _vcmpspecular))), 1.0);
	outputColor = _vcmpoutputColor;
}

/********************************************************************************/
```

###Specifying Other Variants

To change the default schedule, you can add explicit world specifiers to the component definition by changing the line
```glsl
float specular = ComputeHighlightPhong(lightDir, normal, view, roughness, specular, metallic);
```
To the following:
```glsl
@objSurface float specular = ComputeHighlightPhong(lightDir, normal, view, roughness, specular, metallic);
```
This forces specular to be computed in objSurface world, which results in the shader variant shown in Figure 5.

<img src="https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/img/6.png" width="650px"/><br/>
Figure 5. Shader variant after placing specular at objSurface world. Note that only specular appears as the output of objSurface world.

###Create Different Variants via Schedule File

You have already learnt how to use world specifiers to control the world placement decisions of shader components. However, when exploring different choices, frequently changing the shader code can be inconvenient. Spire allows you to specify world placements in a separate schedule file. For example, you can create a file named “schedule.txt” that contains the following line:
```
specular = objSurface;
```
Then compile the shader using the following command will produce the same shader variant as Figure 5.
```
SpireCompiler “Demo1Shader.shader” -schedule “schedule.txt”
```
##Exploring Different Choices via Interactive Tools
A more intuitive way of exploring different placement choices is to visualize all choices afforded by a shader and change them graphically. The Spire compiler can enumerates all world placement choices exposed by the shader. With this data, you can build GUI tools to explore different choices. The demo engine implements such a tool as shown below. To play with it, run SceneViewer and open “Examples/demo1/demo1.world”, then try selecting different worlds for some components in the Choice Control window.

<img src="https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/img/7.png" width="500px"/><br/>
Figure 6. The choice exploration tool included in the demo engine. All components of the selected shader is listed, with ComboBoxes displaying allowed placement choices for each component.  Clicking Save button will save the currently selected choices to a schedule file.

With the knowledge of the choice space, it is even possible to build an autotuner that automatically searches the space for the most performant choice. We have implemented a prototype autotuner that enumerates the entire choice space to find the combination of world placement choices that yields the highest render quality under desired time budget. To experiement with the tool, you can select the components you wish to explore in the choice control window, then change the time budget (default is 10ms) and click Autotune button.

##Exposing Additional Choices via Component Overloading

Spire supports expressing more choices than just world placements. A shader may expose additional choices of algorithmic approximations or simplifications by providing component overloads.
For example, you can add the following line to Demo1Shader:
```glsl
@vs vec3 normal = vNormal;
```
This results Demo1Shader containing two definitions of normal, one without an explicit world specifier, one with the @vs specifier. 
This means that if “normal” is scheduled to “vs” world, then the compiler should pick the overloaded implementation that uses vertex normal only and not fetching the normal map. Compiling the new shader with the schedule 
```
normal = vs;
```
yields the following shader variant:

<img src="https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/img/8.png"  width="650px"/><br/>
Figure 7. Shader variant result from using the @vs overload of normal component. This shader uses only vertex normal for lighting.

##Component Overloading for Algorithmic Choices

A component can be overloaded with different algorithmic alternatives. For example, you can change the definition of the specular component to the following:
```glsl
float specular:phong = ComputeHighlightPhong(lightDir, normal, view, roughness, specular, metallic);
float specular:ggx = ComputeHighlightGGX(lightDir, normal, view, roughness, specular, metallic);
```
This exposes two named alternatives of the specular component that compute lighting using phong and ggx model respectively.
The actual choice can be provided at compile time. In “schedule.txt”, add the following line:
```
specular = fs:ggx;
```
Tells the compiler to compute specular using the `ggx` model and compute it at `fs` world.


##Targeting a More Complex Pipeline

The choice of world placements can be used to carry out many interesting optimizations other than object space shading.
We have also implemented a more complex multi-rate rendering pipeline that features screen space half resolution (coarse pixel) shading, as shown in Figure 8.

<img src="https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/img/9.png" width="700px"/><br/>
Figure 8. The multi-rate rendering pipeline featuring object space and screen space half resolution rendering phases.

Similar to the object space rendering phase that computes and stores shading result in object space textures, half resolution rendering phase performs another rendering pass but stores the shading results in half resolution screen-space textures. These screen-space textures can then be used by full resolution rendering phase. In practice, low frequency shader components such as diffuse lighting and fog can usually be sampled at sparser than once per pixel rate, making half resolution rendering an ideal trade-off for performance.

Same as the previous object space shading pipeline, we need to declare the multi-rate pipeline in Spire before the compiler can generate shaders for us. The declaration of the new multi-rate pipeline is mostly similar to the previous object spacing shading pipeline, except the addition of two new worlds: lowResVs and lowRes, that correspond to the vertex and fragment shading stages of the half resolution rendering phase. The full declaration of the pipeline can be found at https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/MultiRate.pipeline 

Since the shading results from half resolution rendering phase is stored in screen space textures, they need to be fetched with the correct texture coordinates (in this case, screen space coordinates) in the full resolution fragment shader. In a Spire shader, if you write:
```glsl
@lowRes vec3 color = ...;
@fs vec3 result = color * 2;
```
The compiler will inject the necessary texture fetch operation in the fragment shader to access color computed at lowRes world. The generated shader for lowRes and fs world are like the following:
```glsl
// generated lowRes shader:
out vec3 color;
void main()
{
    color = …;
}
```
```glsl
// generated fs shader:
uniform lowRes
{
    sampler2D tex_color;
} blkLowRes;
void main()
{
    vec3 color = texture(blkLowRes.tex_color, screenCoord);
    vec3 result = color * 2; 
}
```
As you can see, the compiler need the extra screenCoord to complete the texture fetch operation. In Spire, the compiler provides the capability to inject the texture fetch operation automatically, but it remains your responsibility to provide proper texture coordinate definition.  In the pipeline definition, we have told the compiler that when accessing a component computed at lowRes world from fs world, it need to inject a texture fetch operation using screenCoord as the texture coordinate, and every shader written against the multi-rate pipeline must provide a definition of screenCoord.

With this pipeline declaration, our previous Demo1Shader can be ported to the new multi-rate pipeline without much change in code, except that it should provide a definition of screenCoord, as follows:
```glsl
vec2 screenCoord = projCoord.xy / projCoord.w * 0.5 + 0.5;
```
This is the only change needed to make Demo1Shader run on the new multi-rate pipeline. You can find the ported shader at https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/Demo2Shader.shader . To play with it, run SceneViewer and open “Examples/demo2/demo2.world”. Notice the extra choices available from the Choice Control window. Figure 9 shows the shader variant that computes both diffuse and specular lighting at half resolution.

<img src="https://github.com/csyonghe/Spire/blob/master/Docs/tutorial1/img/10.png"/><br/>
Figure 9. Rendering result of the ported shader with both diffuse and specular computed at half screen resolution.
