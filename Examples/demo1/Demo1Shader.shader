using "../MultiRatePipeline.shader";

shader Demo1Shader
{
    @rootVert vec3 vert_pos;
    @rootVert vec3 vert_normal;
    @rootVert vec3 vert_tangent;
    @rootVert vec2 vert_uv; 
    @modelTransform mat4 modelMatrix; 
    @modelTransform mat4 normalMatrix;
    @perInstanceUniform sampler2D ground_pebble_map;
    @perInstanceUniform sampler2D ground_pebble_Nmap;
    @viewUniform mat4 viewProjectionMatrix;
    @viewUniform mat4 viewMatrix;
    @viewUniform mat4 projectionMatrix;
    @viewUniform mat4 invViewMatrix;
    @viewUniform mat4 invViewProjectionMatrix;
    @viewUniform vec3 cameraPos;
    @viewUniform vec3 lightColor;
    @viewUniform vec3 lightDir;
    
    vec4 position = modelMatrix * vec4(vert_pos, 1.0);
    vec4 projCoord = viewProjectionMatrix * position;
    vec2 screenCoord = projCoord.xy/vec2(projCoord.w)*0.5 + vec2(0.5);
    
    vec4 texSpaceVert = vec4(vert_uv*2.0 - vec2(1.0), 0.0, 1.0);                    
    
    vec2 uv = vert_pos.xz*(1.0/30.0);
    vec3 albedo = texture(ground_pebble_map, uv).xyz;
    vec3 normalTex = (texture(ground_pebble_Nmap, uv).xyz - 0.5) * 2.0;
   
    vec3 vNormal = (normalMatrix * vec4(vert_normal, 1.0)).xyz;
    vec3 vTangent = (normalMatrix * vec4(vert_tangent, 1.0)).xyz;
    vec3 vBiTangent = cross(vTangent, vNormal);
    vec3 normal = (normalMatrix * vec4(normalize(normalTex.x * vTangent 
                    + normalTex.y * vBiTangent 
                    + normalTex.z * vNormal), 1)).xyz;
                    
    vec3 view = normalize(cameraPos - position.xyz);
    float roughness = 0.5;
    float specular = 0.5;
    float metallic = 0.4;
    float highlight = ComputeHighlightPhong(lightDir, normal, view, roughness, specular, metallic);
    float lambert = clamp(dot(lightDir, normal), 0.0, 1.0);
    vec4 outputColor = vec4(lightColor * 
						(albedo * (lambert*0.7 + 0.5)*(1.0-metallic) + 
						mix(albedo, vec3(1.0), 1.0 - metallic) * highlight), 1.0);
    float opacity = 1.0;
}
