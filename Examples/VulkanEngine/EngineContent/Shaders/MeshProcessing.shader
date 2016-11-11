shader StaticMeshForwardLighting : StandardPipeline
{
    public using VertexAttributes;
    public using SystemUniforms;
    public using NoAnimation;
    public inline vec2 vertUV = vertUV0;
    public using TangentSpaceTransform;
    public using MaterialGeometry;
    public using VertexTransform;
    public using MaterialPattern;
    vec3 lightParam = vec3(roughness, metallic, specular);
    using lighting = Lighting(TangentSpaceToWorldSpace(normal));
    public out @Fragment vec4 outputColor = vec4(lighting.result, 1.0);
}

shader StaticMeshDeferredLighting : StandardPipeline
{
    public using VertexAttributes;    
    public using SystemUniforms;
    public using NoAnimation;
    public vec2 vertUV = vertUV0;
    public using TangentSpaceTransform;    
    public using MaterialGeometry;
    public using VertexTransform;
    public using MaterialPattern;
    vec3 lightParam = vec3(roughness, metallic, specular);

    public out @Fragment vec3 outputAlbedo = albedo;
    public out @Fragment vec3 outputPbr = lightParam;
    public out @Fragment vec3 outputNormal = TangentSpaceToWorldSpace(normal);
}

shader SkeletalMeshForwardLighting : StandardPipeline
{
    public using VertexAttributes;    
    public using SystemUniforms;
    public using SkeletalAnimation;
    public vec2 vertUV = vertUV0;
    public using TangentSpaceTransform;    
    public using MaterialGeometry;
    public using VertexTransform;
    public using MaterialPattern;
    vec3 lightParam = vec3(roughness, metallic, specular);
    using lighting = Lighting(TangentSpaceToWorldSpace(normal));
    public out @Fragment vec4 outputColor = vec4(lighting.result, 1.0);
}

shader SkeletalMeshDeferredLighting : StandardPipeline
{
    public using VertexAttributes;    
    public using SystemUniforms;
    public using SkeletalAnimation;
    public vec2 vertUV = vertUV0;
    public using TangentSpaceTransform;
    public using MaterialGeometry;
    public using VertexTransform;
    public using MaterialPattern;
    vec3 lightParam = vec3(roughness, metallic, specular);
    
    public out @Fragment vec3 outputAlbedo = albedo;
    public out @Fragment vec3 outputPbr = lightParam;
    public out @Fragment vec3 outputNormal = TangentSpaceToWorldSpace(normal);
}
