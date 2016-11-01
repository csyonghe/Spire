using "DefaultGeometry.shader";

module MaterialPattern
{
    require vec2 vertUV;
    @MaterialUniform sampler2D albedoMap;
    public vec3 normal = vec3(0.0, 0.0, 1.0);
    public float roughness = 0.4;
    public float metallic = 0.4;
    public float specular = 1.0;
    public vec3 albedo = texture(albedoMap, vertUV).xyz;
    public float selfShadow(vec3 lightDir)
    {
        return 1.0;
    }
}
