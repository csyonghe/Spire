pipeline Pipe
{
    abstract world uniform;
    abstract world vertex;
    world vs : "glsl" using projPos export standardExport ;
    world fs : "glsl" export fragmentExport;
    require @vs vec4 projPos;
    require out @fs vec4 fragColor;
    
    import vertexImport(vertex->vs);
    import uniformImport(uniform->vs);
    import uniformImport(uniform->fs);
    import standardImport(vs->fs);
}

module VertexTransform
{
    @vertex vec3 vert_pos;
    @vertex vec2 vert_uv;
    @vertex vec3 vert_normal;
    @uniform mat3 normalMatrix;
    @uniform mat4 viewProjectionMatrix;
    @uniform mat4 modelMatrix;
    public vec4 projPos = viewProjectionMatrix * vec4(vert_pos, 1.0);
    public vec3 pos = (modelMatrix * vec4(vert_pos, 1.0)).xyz;
    public vec3 normal = normalMatrix * vert_normal;
}

module LambertianLighting
{
    require vec3 pos;
    require vec3 lightPos;
    require vec3 normal;
    public float lightingResult
    {
        vec3 L = normalize(lightPos - pos);
        return clamp(dot(normal, L), 0.0, 1.0) * 0.7 + 0.3;
    }
}

module Func0
{
    require int p0;
    require int p1;
    require vec3 pos;
    int sum = 2*p0 + p1;
    public int result = sum + 5;
}

module Func1
{
    require int p0;
    require vec3 pos;
    int p1 = 1;
    int sum = 4;
    using f = Func0(p0: p1, p1: p0); // long syntax of invoking a shader module
    // when invoking a shader, named arguments can come after positional arguments:
    // this is also valid: Func0(p1, p1: p0)
    // but this is not valid: Func0(p0: p1, p0)
    public int result = f.result + sum;
}

module Func2
{
    require vec3 pos; // this parameter will be filled in automatically
    float nameClashTest = 0.5;
    public float publicNameClashTest = 0.7;
    using f0 = Func1((int)pos.x); // short syntax of invoking a shader module (pos is auto-filled)
    public using Func1(f0.result); // "public using" makes public components of Func1 transitively available to users of Func2 
}

shader StandardShader
{
    using VertexTransform; // mixin
    @uniform vec3 lightPos;
    using light = LambertianLighting; // aggregation
    using Func2; // testing mixin
    float nameClashTest = 0.6; // this will not clash with Func2::nameClashTest because it is private
    // float publicNameClashTest = 0.8; // this will clash with Func2::publicNameClashTest
    vec4 fragColor = vec4(vec3(light.lightingResult), (float)result); // result is transitively imported from Func1 via Func2
}



/*
pipeline EnginePipeline
{
    [Pinned]
    [Packed]
    abstract world rootVert;
    
    [InterfaceBlockIndex: "4"]
    abstract world rootTex;
    [Pinned]
    [InterfaceBlockIndex: "1"]
    abstract world viewUniform;
    [InterfaceBlock: "perInstanceUniform:2"]
    abstract world perInstanceUniform;
    [InterfaceBlock: "modelTransform:0"]
    abstract world modelTransform;
    [Packed]
    world precomputeVert : "glsl" export bufferExport;
    [InterfaceBlockIndex: "7"]
    world precomputeUniform : "glsl" export bufferExport;
    world precomputeTexVs : "glsl(vertex:texSpaceVert)" using texSpaceVert export standardExport;
    [InterfaceBlock: "PrebakedAssets:3"]
    world precomputeTex : "glsl" export fragmentExport;
    world shadowVs : "glsl(vertex:projCoord;command_list)" using projCoord export standardExport;
    world shadowFs : "glsl(command_list)" using opacity, projCoord export fragmentExport;
    world vs : "glsl(vertex:projCoord;command_list)" using projCoord export standardExport;
    world lqfs : "glsl(command_list)" using opacity export fragmentExport;
    world fs : "glsl(command_list)" using opacity export fragmentExport;
    
    require @(vs*, shadowVs*) vec4 projCoord; 
    require @(lqfs*, fs*, shadowFs*) float opacity;
    require @precomputeTexVs vec4 texSpaceVert;
    require out @(fs*, lqfs*) vec4 outputColor;
    require @(vs,fs,lqfs, shadowFs, shadowVs, precomputeTex) vec2 vert_uv;
    
    import uniformImport(perInstanceUniform->precomputeVert);
    import uniformImport(perInstanceUniform->precomputeTexVs);
    import uniformImport(perInstanceUniform->precomputeTex);
    import uniformImport(perInstanceUniform->vs);
    import uniformImport(perInstanceUniform->fs);
    import uniformImport(perInstanceUniform->lqfs);
    
    import uniformImport(perInstanceUniform->shadowVs);
    import uniformImport(perInstanceUniform->shadowFs);    
        

    import uniformImport(viewUniform->vs);
    import uniformImport(viewUniform->fs);
    import uniformImport(viewUniform->lqfs);
    
    import uniformImport(viewUniform->shadowVs);
    import uniformImport(viewUniform->shadowFs);
    import uniformImport(modelTransform->vs);
    import uniformImport(modelTransform->fs);
    import uniformImport(modelTransform->lqfs);
    
    import uniformImport(modelTransform->shadowVs);
    import uniformImport(modelTransform->shadowFs);
    import bufferImport(rootVert->precomputeVert);
    import vertexImport(precomputeVert->precomputeTexVs);
    import vertexImport(precomputeVert->vs);
    import vertexImport(precomputeVert->shadowVs);
    
    import uniformImport(viewUniform->precomputeUniform);
    import uniformImport(perInstanceUniform->precomputeUniform);
    import bufferImport(precomputeVert->precomputeUniform);
    import uniformImport(precomputeUniform->vs);
    import uniformImport(precomputeUniform->fs);
    import uniformImport(precomputeUniform->lqfs);
    
    import uniformImport(precomputeUniform->shadowVs);
    import uniformImport(precomputeUniform->shadowFs);
    
    import standardImport(precomputeTexVs->precomputeTex);
    import standardImport(vs->fs);
    import standardImport(vs->lqfs);
    
    import standardImport(shadowVs->shadowFs);
    
    import textureImport(precomputeTex->vs) using vert_uv;
    import textureImport(precomputeTex->fs) using vert_uv;
    import textureImport(precomputeTex->lqfs) using vert_uv;
    
    import textureImport(precomputeTex->shadowVs) using vert_uv;
    import textureImport(precomputeTex->shadowFs) using vert_uv;
    import textureImport(rootTex->precomputeTex) using vert_uv;
}

shader TestShader
{
    @modelTransform mat4 modelMatrix; 
    @viewUniform mat4 viewProjectionMatrix;
    @viewUniform mat4 projectionMatrix;
    @rootVert vec3 vert_pos;
    @rootVert vec3 vert_normal;
    @rootVert vec3 vert_tangent;
    @rootVert vec2 vert_uv; 
    
    vec3 positionOffset = vec3(0.0);
    inline vec4 position = modelMatrix * vec4(vert_pos + positionOffset, 1.0);
    vec4 projCoord = viewProjectionMatrix * position;
    vec4 texSpaceVert = vec4(vert_uv*2.0 - vec2(1.0), 0.0, 1.0);
    float opacity = 1.0;
    vec4 outputColor = vec4(1.0);
}
*/