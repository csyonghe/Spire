using "../DemoEngine.pipeline";

shader Plane
{
    using ViewUniforms;
    using StandardVertexFormat;
    using VertexTransform; 
    
    @perInstanceUniform sampler2D baseMap;
    @perInstanceUniform sampler2D baseNormalMap;
    
    vec2 uvCoord = vert_uv * 3.0;
    
    vec3 Normal
    {
        vec3 normalMap = texture(baseNormalMap, vec2(uvCoord.y, uvCoord.x)).xyz;
        return normalMap;
    }
       
    vec3 Albedo = texture(baseMap, uvCoord).xyz * 2.0;
    using normalTransform = TangentSpaceTransform(Normal);
    using lighting = Lighting(normalTransform.normal, Albedo, vec3(0.8, 0.3, 0.7));
    
    vec4 outputColor = vec4(lighting.result, opacity);
    float opacity = 1.0;
}