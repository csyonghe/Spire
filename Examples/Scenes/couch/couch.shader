using "../DemoEngine.pipeline";

shader Couch
{
    using Header;
    
    @perInstanceUniform sampler2D maskMap;
    @perInstanceUniform sampler2D leatherNormalMap;
    @perInstanceUniform sampler2D baseNormalMap;
    @perInstanceUniform sampler2D aoMap;
    @perInstanceUniform sampler2D leatherSpecularMap;
    @perInstanceUniform sampler2D leatherMap;
    
    inline vec2 normalCoord = vert_uv * 5.79;
    
    inline vec3 mask = texture(maskMap, vert_uv).xyz;
    
    vec3 Normal
    {
        vec2 macroNormalCoord = vert_uv * 0.372;
        vec3 macroNormal = (texture(leatherNormalMap, macroNormalCoord).xyz*2.0-vec3(1.0,1.0,1.0)) * vec3(0.274,0.274, 0.0);
        vec3 leatherNormal = (texture(leatherNormalMap, normalCoord).xyz*2.0-vec3(1.0,1.0,1.0)) * vec3(1.0,1.0,0.0);
        return normalize(texture(baseNormalMap, vert_uv).xyz*2.0-vec3(1.0,1.0,1.0) + (leatherNormal + macroNormal)*mask.x);
    }
    
    inline vec3 aoTex = texture(aoMap, vert_uv).xyz;
    inline vec3 specTex = texture(leatherSpecularMap, normalCoord).xyz;
    inline float wearFactor = mask.z * 0.381;
    
    float Roughness = mix(mix(mix(0.2, mix(mix(0.659,2.01, specTex.x), 
                                -0.154, wearFactor), mask.x), 0.0, mask.y), 0.0, aoTex.y);
    float Metallic = mix(0.5,0.1, specTex.x);
    float Specular = 1.0;
    [RGB8]
    vec3 Albedo
    {
        float ao = aoTex.x;
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
    
    using Footer;
}

