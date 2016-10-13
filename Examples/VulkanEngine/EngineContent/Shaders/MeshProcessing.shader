shader StaticMeshForwardLighting
{
    public using SystemUniforms;
    public using MeshVertex;
    public inline vec2 vertUV = vertUV0;
    public @fs vec2 shit = vertUV;
    public using VertexTransform;
    public using SurfacePattern;
    vec3 lightParam = vec3(roughness, metallic, specular);
    using transformedNormal = TangentSpaceTransform(normal);
    using lighting = Lighting(transformedNormal.normal);
    public out @fs vec4 outputColor = vec4(lighting.result + shit.x*0.0, 1.0);
}

shader StaticMeshDeferredLighting
{
    public using SystemUniforms;
    public using MeshVertex;
    public vec2 vertUV = vertUV0;

    public using VertexTransform;
    public using SurfacePattern;
    vec3 lightParam = vec3(roughness, metallic, specular);
    using transformedNormal = TangentSpaceTransform(normal);

    public out @fs vec3 outputAlbedo = albedo;
    public out @fs vec3 outputPbr = lightParam;
    public out @fs vec3 outputNormal = transformedNormal.normal;
}

shader SkeletalMeshForwardLighting
{
    public using SystemUniforms;
    public using MeshVertex;
    public vec2 vertUV = vertUV0;
    public using SkeletalVertexTransform;
    public using SurfacePattern;
    vec3 lightParam = vec3(roughness, metallic, specular);
    using transformedNormal = TangentSpaceTransform(normal);
    using lighting = Lighting(transformedNormal.normal);
    public out @fs vec4 outputColor = vec4(lighting.result, 1.0);
}

shader SkeletalMeshDeferredLighting
{
    public using SystemUniforms;
    public using MeshVertex;
    public vec2 vertUV = vertUV0;
    public using SkeletalVertexTransform;
    public using SurfacePattern;
    vec3 lightParam = vec3(roughness, metallic, specular);
    using transformedNormal = TangentSpaceTransform(normal);
    
    public out @fs vec3 outputAlbedo = albedo;
    public out @fs vec3 outputPbr = lightParam;
    public out @fs vec3 outputNormal = transformedNormal.normal;
}
