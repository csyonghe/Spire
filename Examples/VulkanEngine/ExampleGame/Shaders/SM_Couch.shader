using "DefaultGeometry.shader";

module MaterialPattern
{
    @MaterialUniform sampler2D maskMap;
    @MaterialUniform sampler2D leatherNormalMap;
    @MaterialUniform sampler2D baseNormalMap;
    @MaterialUniform sampler2D aoMap;
    @MaterialUniform sampler2D leatherSpecularMap;
    @MaterialUniform sampler2D leatherMap;
    
    inline vec3 mask = texture(maskMap, vec2(vertUV.x, vertUV.y)).xyz;
    inline vec2 normalCoord = vertUV * 5.79;

    public vec3 normal
    {
        vec2 macroNormalCoord = vertUV * 0.372;
        vec3 macroNormal = (texture(leatherNormalMap, macroNormalCoord).xyz*2.0-vec3(1.0,1.0,1.0)) * vec3(0.274,0.274, 0.0);
        vec3 leatherNormal = (texture(leatherNormalMap, normalCoord).xyz*2.0-vec3(1.0,1.0,1.0)) * vec3(1.0,1.0,0.0);
        return normalize(texture(baseNormalMap, vertUV).xyz*2.0-vec3(1.0,1.0,1.0) + (leatherNormal + macroNormal)*mask.x);
    }
    
    inline vec3 aoTex = texture(aoMap, vertUV).xyz;
    inline vec3 specTex = texture(leatherSpecularMap, normalCoord).xyz;
    inline float wearFactor = mask.z * 0.381;
    
    public float roughness = mix(mix(mix(0.2, mix(mix(0.659,2.01, specTex.x), 
                                -0.154, wearFactor), mask.x), 0.0, mask.y), 0.0, aoTex.y);
    public float metallic = mix(0.5,0.1, specTex.x);
    public float specular = 1.0;
    public vec3 albedo
    {
		float ao = aoTex.x * 0.5 + 0.5;
        vec3 Color1 = vec3(0.0,0.0,0.0);
        float Desaturation2 = 0.0;
        float Desaturation2WearSpot = 0.0896;
        vec3 Color2 = vec3(1.0, 0.86,0.833);
        vec3 Color2WearSpot = vec3(0.628,0.584, 0.584);
        vec3 Color3 = vec3(0.823,0.823,0.823);
        vec3 SeamColor = vec3(0.522,0.270,0.105);
        return mix(mix(mix(Color1, desaturate(texture(leatherMap, normalCoord).xyz,
            mix(Desaturation2, Desaturation2WearSpot, wearFactor)) * 
            mix(Color2, Color2WearSpot, wearFactor), mask.x), 
            Color3, mask.y), SeamColor, aoTex.y) * ao;
    }
    
    public float selfShadow(vec3 lightDir)
    {
        return 1.0;
    }
}

