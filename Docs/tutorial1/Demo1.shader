// import pipeline definition
using "ObjSpace.pipeline";

// define the shader for our demo
shader Demo1Shader
{
    // define vertex inputs
    @rootVert vec3 vert_pos;
    @rootVert vec3 vert_normal;
    @rootVert vec3 vert_tangent;
    @rootVert vec2 vert_uv;
    
    // define transform matrix inputs 
    @modelTransform mat4 modelMatrix; 
    @modelTransform mat4 normalMatrix;
    
    // define material uniform inputs
    @perInstanceUniform sampler2D ground_pebble_map;
    @perInstanceUniform sampler2D ground_pebble_Nmap;
    
    // define view and environment uniform inputs
    @viewUniform mat4 viewProjectionMatrix;
    @viewUniform mat4 viewMatrix;
    @viewUniform mat4 projectionMatrix;
    @viewUniform mat4 invViewMatrix;
    @viewUniform mat4 invViewProjectionMatrix;
    @viewUniform vec3 cameraPos;
    @viewUniform vec3 lightColor;
    @viewUniform vec3 lightDir;
    
    // compute projected vertex position
    vec4 position = modelMatrix * vec4(vert_pos, 1.0);
    vec4 projCoord = viewProjectionMatrix * position;
    
    // pipeline requires a texture space vertex for object space rendering
    // here we require the mesh provides a unique parameterization stored in vert_uv
    vec4 texSpaceVert = vec4(vert_uv*2.0 - vec2(1.0), 0.0, 1.0);                    
    
    vec2 uv = vert_uv * 2.0; // tile the texture
    vec3 albedo = texture(ground_pebble_map, uv).xyz;
    
    // compute normal from normal map
    vec3 normal
    {
        vec3 vNormal = (normalMatrix * vec4(vert_normal, 1.0)).xyz;
        vec3 vTangent = (normalMatrix * vec4(vert_tangent, 1.0)).xyz;
        vec3 vBiTangent = cross(vTangent, vNormal);
        // fetch normal map
        vec3 normalTex = (texture(ground_pebble_Nmap, uv).xyz - 0.5) * 2.0;
        // transform to world space
        return normalize(normalTex.x * vTangent 
                    + normalTex.y * vBiTangent 
                    + normalTex.z * vNormal);
    }
    
    // uncomment the following line to add an @vs overload for normal
    @vs vec3 normal = (normalMatrix * vec4(vert_normal, 1.0)).xyz;

    vec3 view = normalize(cameraPos - position.xyz);
    
    // compute lighting
    float diffuse = clamp(dot(lightDir, normal), 0.0, 1.0);
    float specular = ComputeHighlightPhong(lightDir, normal, view, 0.5, 0.5, 0.4);
    // uncomment the following lines to provide explicit alternatives of specular
    //float specular:ggx = ComputeHighlightGGX(lightDir, normal, view, 0.5, 0.5, 0.4);
    //float specular:phong = ComputeHighlightPhong(lightDir, normal, view, 0.5, 0.5, 0.4);

    // the final shader output
    // (The pipeline declaration has declared outputColor as the final output)
    vec4 outputColor = vec4(lightColor * 
         (albedo * (diffuse * 0.7 + 0.5) * 0.6 + 
         mix(albedo, vec3(1.0), 0.6) * specular), 1.0);
}