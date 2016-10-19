shader StaticMeshForwardLighting : StandardPipeline
{
    public using SystemUniforms;
    public using StaticVertex;
    public inline vec2 vertUV = vertUV0;
    public using SurfacePattern;
    public using VertexTransform;
    vec3 lightParam = vec3(roughness, metallic, specular);
    using transformedNormal = TangentSpaceTransform(normal);
    using lighting = Lighting(transformedNormal.normal);
    public out @fs vec4 outputColor = vec4(lighting.result, 1.0);
}

shader StaticMeshDeferredLighting : StandardPipeline
{
    public using SystemUniforms;
    public using StaticVertex;
    public vec2 vertUV = vertUV0;

    public using SurfacePattern;
    public using VertexTransform;
    vec3 lightParam = vec3(roughness, metallic, specular);
    using transformedNormal = TangentSpaceTransform(normal);

    public out @fs vec3 outputAlbedo = albedo;
    public out @fs vec3 outputPbr = lightParam;
    public out @fs vec3 outputNormal = transformedNormal.normal;
}

shader SkeletalMeshForwardLighting : StandardPipeline
{
    public using SystemUniforms;
    public using SkinnedVertex;
    public vec2 vertUV = vertUV0;
    public using SurfacePattern;
    public using VertexTransform;
    vec3 lightParam = vec3(roughness, metallic, specular);
    using transformedNormal = TangentSpaceTransform(normal);
    using lighting = Lighting(transformedNormal.normal);
    public out @fs vec4 outputColor = vec4(lighting.result, 1.0);
}

shader SkeletalMeshDeferredLighting : StandardPipeline
{
    public using SystemUniforms;
    public using SkinnedVertex;
    public vec2 vertUV = vertUV0;
    public using SurfacePattern;
    public using VertexTransform;
    vec3 lightParam = vec3(roughness, metallic, specular);
    using transformedNormal = TangentSpaceTransform(normal);
    
    public out @fs vec3 outputAlbedo = albedo;
    public out @fs vec3 outputPbr = lightParam;
    public out @fs vec3 outputNormal = transformedNormal.normal;
}
