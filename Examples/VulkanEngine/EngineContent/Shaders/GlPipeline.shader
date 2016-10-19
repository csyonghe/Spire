pipeline StandardPipeline
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
    extern @(vs*) StorageBuffer<skeletalTransform> skeletalTransformBlock;
    import(skeletalTransform->vs) uniformImport()
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

pipeline TessellationPipeline : StandardPipeline
{
    [Pinned]
    input world rootVert;
    
    [Pinned]
    [Binding: "0"]
    input world modelTransform;
   
    [Pinned]
    [Binding: "1"]
    input world viewUniform;
    
    [Pinned]
    [Binding: "2"]
    input world perInstanceUniform;
    
    [Pinned]
    input world skeletalTransform;
    
    world vs;
    world tcs;
    world perCornerPoint;
    world perPatch;
    world tes;
    world fs;
    
    [Binding: "3"]
    extern @(vs*, fs*, tcs*, tes*) StorageBuffer<skeletalTransform> skeletalTransformBlock;
    import(skeletalTransform->vs) uniformImport()
    {
        return skeletalTransformBlock;
    }
    import(skeletalTransform->fs) uniformImport()
    {
        return skeletalTransformBlock;
    }
    import(skeletalTransform->tcs) uniformImport()
    {
        return skeletalTransformBlock;
    }
    import(skeletalTransform->tes) uniformImport()
    {
        return skeletalTransformBlock;
    }
    
    require @tes vec4 projCoord; 
    require @tcs vec2 tessLevelInner;
    require @tcs vec4 tessLevelOuter;
    [Binding: "2"]
    extern @(vs*, fs*, tcs*, tes*, perPatch*) Uniform<perInstanceUniform> instanceUniformBlock;
    
    import(perInstanceUniform->vs) uniformImport() { return instanceUniformBlock; }
    import(perInstanceUniform->fs) uniformImport() { return instanceUniformBlock; }
    import(perInstanceUniform->tcs) uniformImport() { return instanceUniformBlock; }
    import(perInstanceUniform->tes) uniformImport() { return instanceUniformBlock; }
    import(perInstanceUniform->perPatch) uniformImport() { return instanceUniformBlock; }
    
    [Binding: "1"]
    extern @(vs*, fs*, tcs*, tes*, perPatch*) Uniform<viewUniform> viewUniformBlock;
    
    import(viewUniform->vs) uniformImport() { return viewUniformBlock; }
    import(viewUniform->fs) uniformImport() { return viewUniformBlock; }
    import(viewUniform->tcs) uniformImport() { return viewUniformBlock; }
    import(viewUniform->tes) uniformImport() { return viewUniformBlock; }
    import(viewUniform->perPatch) uniformImport() { return viewUniformBlock; }
    
    [Binding: "0"]
    extern @(vs*, fs*, tcs*, tes*, perPatch*) Uniform<modelTransform> modelUniformBlock;
    
    import(modelTransform->vs) uniformImport() { return modelUniformBlock; }
    import(modelTransform->tcs) uniformImport() { return modelUniformBlock; }
    import(modelTransform->tes) uniformImport() { return modelUniformBlock; }
    import(modelTransform->perPatch) uniformImport() { return modelUniformBlock; }
    
    import(modelTransform->fs) uniformImport() { return modelUniformBlock; }
    
    [VertexInput]
    extern @vs rootVert vertAttribs;
    import(rootVert->vs) vertexImport() { return vertAttribs; }
    
    // implicit import operator vs->perCornerPoint
    extern @perCornerPoint vs[] vs_tcs;
    [PerCornerIterator]
    extern @perCornerPoint int sysLocalIterator;
    import (vs->perCornerPoint) standardImport()
    {
        return vs_tcs[sysLocalIterator];
    } 
    
    // implicit import operator tes->fs
    extern @fs tes tes_fs;
    import(tes->fs) standardImport()
    {
        return tes_fs;
    } 
    
    //extern @fs vs vertexOutputBlock;
    //import(vs->fs) standardImport() { return vertexOutputBlock; }
    
    stage vs : VertexShader
    {
        VertexInput: vertAttribs;
        World: vs;
    }
    stage fs : FragmentShader
    {
        World: fs;
        VSInput: vertexOutputBlock;
    }
    
    extern @tcs vs[] vs_tcs;
    extern @perPatch vs[] vs_tcs;
    [InvocationID]
    extern @tcs vs invocationId;
    import(vs->tcs) indexImport(int id)
    {
        return vs_tcs[id];
    }
    import(vs->perPatch) indexImport(int id)
    {
        return vs_tcs[id];
    }
    extern @tes tcs[] tcs_tes;
    import(tcs->tes) indexImport(int id)
    {
        return tcs_tes[id];
    }
    extern @tes Patch<perPatch> perPatch_tes;
    import (perPatch->tes) standardImport()
    {
        return perPatch_tes;
    }
    
    extern @tes Patch<perCornerPoint[3]> perCorner_tes;
    [TessCoord]
    extern @tes vec3 tessCoord;
    import(perCornerPoint->tes) standardImport()
    {
        return perCorner_tes[0] * tessCoord.x +
               perCorner_tes[1] * tessCoord.y +
               perCorner_tes[2] * tessCoord.z;
    }
    
    stage tcs : HullShader
    {
        PatchWorld: perPatch;
        ControlPointWorld: tcs;
        CornerPointWorld: perCornerPoint;
        ControlPointCount: 1;
        Domain: triangles;
        TessLevelOuter: tessLevelOuter;
        TessLevelInner: tessLevelInner;
    }
    
    stage tes : DomainShader
    {
        World : tes;
        Position : projCoord;
        Domain: triangles;
    }
}