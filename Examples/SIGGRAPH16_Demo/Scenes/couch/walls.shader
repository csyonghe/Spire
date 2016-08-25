using "../DemoEngine.pipeline";

shader Wall
{
    using Header;
    
	@perInstanceUniform sampler2D baseMap;
	@perInstanceUniform sampler2D baseNormalMap;
	inline vec2 uvCoord = vert_uv * 6.0;
	
	vec3 Normal
	{
		return texture(baseNormalMap, uvCoord).xyz;
	}
	
	float Roughness = 0.5; 
    float Metallic = 0.1;
	float Specular = 1.0;
    [RGB8]
	vec3 Albedo
	{
		return vec3(0.9);
	}
    
    using Footer;
}
