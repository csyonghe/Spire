shader DeferredLighting
{
	public @rootVert vec2 vertPos;
	public @rootVert vec2 vertUV;
	public using SystemUniforms;

	@perInstanceUniform sampler2D albedoTex;
	@perInstanceUniform sampler2D pbrTex;
	@perInstanceUniform sampler2D normalTex;
	@perInstanceUniform sampler2D depthTex;

    public vec4 projCoord = vec4(vertPos.xy, 0.0, 1.0);

	public vec3 normal = texture(normalTex, vertUV).xyz;
	public float roughness = texture(pbrTex, vertUV).x;
	public float metallic = texture(pbrTex, vertUV).y;
	public float specular = texture(pbrTex, vertUV).z;
	public vec3 albedo = texture(albedoTex, vertUV).xyz;

    vec3 lightParam = vec3(roughness, metallic, specular);
	float z = texture(depthTex, vertUV).r*2-1;
    float x = vertUV.x*2-1;
    float y = vertUV.y*2-1;
	vec4 position = invViewProjTransform * vec4(x, y, -z, 1.0f);
	vec3 pos = position.xyz / position.w;
	using lighting = Lighting();

    public out @fs vec4 outputColor = vec4(lighting.result, 1.0);
    //public out @fs vec4 outputColor = vec4(pos, 1.0);
}
