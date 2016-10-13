pipeline EnginePipeline
{
    [Pinned]
    input world rootVert;
    
    [Pinned]
    input world modelTransform;
    
    [Pinned]
    input world skeletalTransform;
    
    [Pinned]
    
    input world viewUniform;
    
    [Pinned]
    input world perInstanceUniform;
    
    world vs;// : "glsl(vertex:projCoord)" using projCoord export standardExport;
    world fs;// : "glsl" export fragmentExport;
    
    require @vs vec4 projCoord; 
    
    [Binding: "2"]
    extern @(vs*, fs*) Uniform<perInstanceUniform> perInstanceUniformBlock; 
    import(perInstanceUniform->vs) uniformImport()
    {
        return perInstanceUniformBlock;
    }
    import(perInstanceUniform->fs) uniformImport()
    {
        return perInstanceUniformBlock;
    }

    [Binding: "1"]
    extern @(vs*, fs*) Uniform<viewUniform> viewUniformBlock;
    import(viewUniform->vs) uniformImport()
    {
        return viewUniformBlock;
    }
    import(viewUniform->fs) uniformImport()
    {
        return viewUniformBlock;
    }
    
    [Binding: "0"]
    extern @(vs*, fs*) Uniform<modelTransform> modelTransformBlock;
    import(modelTransform->vs) uniformImport()
    {
        return modelTransformBlock;
    }
    import(modelTransform->fs) uniformImport()
    {
        return modelTransformBlock;
    }
    
    [Binding: "3"]
    extern @(vs*, fs*) StorageBuffer<skeletalTransform> skeletalTransformBlock;
    import(skeletalTransform->vs) uniformImport()
    {
        return skeletalTransformBlock;
    }
    import(skeletalTransform->fs) uniformImport()
    {
        return skeletalTransformBlock;
    }
    
    [VertexInput]
    extern @vs rootVert vertAttribIn;
    import(rootVert->vs) vertexImport()
    {
        return vertAttribIn;
    }
    
    extern @fs vs vsIn;
    import(vs->fs) standardImport()
    {
        return vsIn;
    }
    
    stage vs : VertexShader
    {
        World: vs;
        Position: projCoord;
    }
    
    stage fs : FragmentShader
    {
        World: fs;
    }
}