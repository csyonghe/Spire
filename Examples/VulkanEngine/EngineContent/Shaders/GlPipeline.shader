pipeline StandardPipeline
{
    [Pinned]
    input world MeshVertex;
    
    [Pinned]
    input world ModelInstance;
    
    [Pinned]
    input world SkeletonData;
    
    [Pinned]
    
    input world ViewUniform;
    
    [Pinned]
    input world MaterialUniform;
    
    world CoarseVertex;// : "glsl(vertex:projCoord)" using projCoord export standardExport;
    world Fragment;// : "glsl" export fragmentExport;
    
    require @CoarseVertex vec4 projCoord; 
    
    [Binding: "2"]
    extern @(CoarseVertex*, Fragment*) Uniform<MaterialUniform> MaterialUniformBlock; 
    import(MaterialUniform->CoarseVertex) uniformImport()
    {
        return MaterialUniformBlock;
    }
    import(MaterialUniform->Fragment) uniformImport()
    {
        return MaterialUniformBlock;
    }

    [Binding: "1"]
    extern @(CoarseVertex*, Fragment*) Uniform<ViewUniform> ViewUniformBlock;
    import(ViewUniform->CoarseVertex) uniformImport()
    {
        return ViewUniformBlock;
    }
    import(ViewUniform->Fragment) uniformImport()
    {
        return ViewUniformBlock;
    }
    
    [Binding: "0"]
    extern @(CoarseVertex*, Fragment*) Uniform<ModelInstance> ModelInstanceBlock;
    import(ModelInstance->CoarseVertex) uniformImport()
    {
        return ModelInstanceBlock;
    }
    import(ModelInstance->Fragment) uniformImport()
    {
        return ModelInstanceBlock;
    }
    
    [Binding: "3"]
    extern @(CoarseVertex*) StorageBuffer<SkeletonData> SkeletonDataBlock;
    import(SkeletonData->CoarseVertex) uniformImport()
    {
        return SkeletonDataBlock;
    }
    
    [VertexInput]
    extern @CoarseVertex MeshVertex vertAttribIn;
    import(MeshVertex->CoarseVertex) vertexImport()
    {
        return vertAttribIn;
    }
    
    extern @Fragment CoarseVertex CoarseVertexIn;
    import(CoarseVertex->Fragment) standardImport()
        require trait IsTriviallyPassable(CoarseVertex)
    {
        return CoarseVertexIn;
    }
    
    stage vs : VertexShader
    {
        World: CoarseVertex;
        Position: projCoord;
    }
    
    stage fs : FragmentShader
    {
        World: Fragment;
    }
}


pipeline TessellationPipeline : StandardPipeline
{
    [Pinned]
    input world MeshVertex;
    
    [Pinned]
    [Binding: "0"]
    input world ModelInstance;
   
    [Pinned]
    [Binding: "1"]
    input world ViewUniform;
    
    [Pinned]
    [Binding: "2"]
    input world MaterialUniform;
    
    [Pinned]
    input world SkeletonData;
    
    world CoarseVertex;
    world ControlPoint;
    world CornerPoint;
    world TessPatch;
    world FineVertex;
    world Fragment;
    
    [Binding: "3"]
    extern @(CoarseVertex*, Fragment*, ControlPoint*, FineVertex*) StorageBuffer<SkeletonData> SkeletonDataBlock;
    import(SkeletonData->CoarseVertex) uniformImport()
    {
        return SkeletonDataBlock;
    }
    
    require @FineVertex vec4 projCoord; 
    require @ControlPoint vec2 tessLevelInner;
    require @ControlPoint vec4 tessLevelOuter;
    [Binding: "2"]
    extern @(CoarseVertex*, Fragment*, ControlPoint*, FineVertex*, TessPatch*) Uniform<MaterialUniform> instanceUniformBlock;
    
    import(MaterialUniform->CoarseVertex) uniformImport() { return instanceUniformBlock; }
    import(MaterialUniform->Fragment) uniformImport() { return instanceUniformBlock; }
    import(MaterialUniform->ControlPoint) uniformImport() { return instanceUniformBlock; }
    import(MaterialUniform->FineVertex) uniformImport() { return instanceUniformBlock; }
    import(MaterialUniform->TessPatch) uniformImport() { return instanceUniformBlock; }
    
    [Binding: "1"]
    extern @(CoarseVertex*, Fragment*, ControlPoint*, FineVertex*, TessPatch*) Uniform<ViewUniform> ViewUniformBlock;
    
    import(ViewUniform->CoarseVertex) uniformImport() { return ViewUniformBlock; }
    import(ViewUniform->Fragment) uniformImport() { return ViewUniformBlock; }
    import(ViewUniform->ControlPoint) uniformImport() { return ViewUniformBlock; }
    import(ViewUniform->FineVertex) uniformImport() { return ViewUniformBlock; }
    import(ViewUniform->TessPatch) uniformImport() { return ViewUniformBlock; }
    
    [Binding: "0"]
    extern @(CoarseVertex*, Fragment*, ControlPoint*, FineVertex*, TessPatch*) Uniform<ModelInstance> modelUniformBlock;
    
    import(ModelInstance->CoarseVertex) uniformImport() { return modelUniformBlock; }
    import(ModelInstance->ControlPoint) uniformImport() { return modelUniformBlock; }
    import(ModelInstance->FineVertex) uniformImport() { return modelUniformBlock; }
    import(ModelInstance->TessPatch) uniformImport() { return modelUniformBlock; }
    
    import(ModelInstance->Fragment) uniformImport() { return modelUniformBlock; }
    
    [VertexInput]
    extern @CoarseVertex MeshVertex vertAttribs;
    import(MeshVertex->CoarseVertex) vertexImport() { return vertAttribs; }
    
    // implicit import operator CoarseVertex->CornerPoint
    extern @CornerPoint CoarseVertex[] CoarseVertex_ControlPoint;
    [PerCornerIterator]
    extern @CornerPoint int sysLocalIterator;
    import (CoarseVertex->CornerPoint) standardImport()
        require trait IsTriviallyPassable(CoarseVertex)
    {
        return CoarseVertex_ControlPoint[sysLocalIterator];
    } 
    
    // implicit import operator FineVertex->Fragment
    extern @Fragment FineVertex tes_Fragment;
    import(FineVertex->Fragment) standardImport()
        require trait IsTriviallyPassable(FineVertex)
    {
        return tes_Fragment;
    } 

    extern @ControlPoint CoarseVertex[] CoarseVertex_ControlPoint;
    extern @TessPatch CoarseVertex[] CoarseVertex_ControlPoint;
    [InvocationId]
    extern @ControlPoint int invocationId;
    import(CoarseVertex->ControlPoint) indexImport(int id)
        require trait IsTriviallyPassable(CoarseVertex)
    {
        return CoarseVertex_ControlPoint[id];
    }
    import(CoarseVertex->TessPatch) indexImport(int id)
        require trait IsTriviallyPassable(CoarseVertex)
    {
        return CoarseVertex_ControlPoint[id];
    }
    extern @FineVertex ControlPoint[] ControlPoint_tes;
    import(ControlPoint->FineVertex) indexImport(int id)
        require trait IsTriviallyPassable(CoarseVertex)
    {
        return ControlPoint_tes[id];
    }
    extern @FineVertex Patch<TessPatch> perPatch_tes;
    import (TessPatch->FineVertex) standardImport()
        require trait IsTriviallyPassable(CoarseVertex)
    {
        return perPatch_tes;
    }
    
    extern @FineVertex Patch<CornerPoint[3]> perCorner_tes;
    [TessCoord]
    extern @FineVertex vec3 tessCoord;
    import(CornerPoint->FineVertex) standardImport()
        require CornerPoint operator + (CornerPoint, CornerPoint)
        require CornerPoint operator * (CornerPoint, float)
    {
        return perCorner_tes[0] * tessCoord.x +
               perCorner_tes[1] * tessCoord.y +
               perCorner_tes[2] * tessCoord.z;
    }
      
    stage vs : VertexShader
    {
        VertexInput: vertAttribs;
        World: CoarseVertex;
    }

    stage tcs : HullShader
    {
        PatchWorld: TessPatch;
        ControlPointWorld: ControlPoint;
        CornerPointWorld: CornerPoint;
        ControlPointCount: 1;
        Domain: triangles;
        TessLevelOuter: tessLevelOuter;
        TessLevelInner: tessLevelInner;
    }
    
    stage tes : DomainShader
    {
        World : FineVertex;
        Position : projCoord;
        Domain: triangles;
    }
    
    stage fs : FragmentShader
    {
        World: Fragment;
        CoarseVertexInput: vertexOutputBlock;
    }
}