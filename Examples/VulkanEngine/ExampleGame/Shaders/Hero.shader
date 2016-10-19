module SurfacePattern
{
    require vec3 coarseVertPos;
    require vec3 coarseVertNormal;
    public using PN_Tessellation;
    //public using NoTessellation;
    
    require vec2 vertUV;
    @perInstanceUniform sampler2D albedoMap;
    public vec3 normal = vec3(0.0, 0.0, 1.0);
    public float roughness = 0.4;
    public float metallic = 0.4;
    public float specular = 1.0;
    public vec3 albedo = texture(albedoMap, vertUV).xyz;
    public vec3 displacement = vec3(0.0);
}
