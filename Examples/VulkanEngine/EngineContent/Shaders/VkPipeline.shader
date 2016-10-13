pipeline EnginePipeline
{
    [Pinned]
    [Packed]
    input world rootVert;

    [Pinned]
    [InterfaceBlockIndex: "0"]
    input world modelTransform;

    [Pinned]
    [InterfaceBlockIndex: "3"]
    [ShaderStorageBlock]
    input world skeletalTransform;

    [Pinned]
    [InterfaceBlockIndex: "1"]
    input world viewUniform;

    [Pinned]
    [InterfaceBlockIndex: "2"]
    input world perInstanceUniform;

    world vs : "spirv(vertex:projCoord; TextureBindingStart:4)" using projCoord export standardExport;
    world fs : "spirv(TextureBindingStart:4)" export fragmentExport;

    require @vs vec4 projCoord;

    import uniformImport(perInstanceUniform->vs);
    import uniformImport(perInstanceUniform->fs);

    import uniformImport(viewUniform->vs);
    import uniformImport(viewUniform->fs);

    import uniformImport(modelTransform->vs);
    import uniformImport(modelTransform->fs);

    import uniformImport(skeletalTransform->vs);
    import uniformImport(skeletalTransform->fs);

    import vertexImport(rootVert->vs);
    import standardImport(vs->fs);
}
