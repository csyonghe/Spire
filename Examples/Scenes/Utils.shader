module ViewUniforms
{
    public @viewUniform mat4 viewProjectionMatrix;
    public @viewUniform mat4 viewMatrix;
    public @viewUniform mat4 projectionMatrix;
    public @viewUniform mat4 invViewMatrix;
    public @viewUniform mat4 invViewProjectionMatrix;
    public @viewUniform vec3 cameraPos;
    public @viewUniform vec3 lightColor;
    public @viewUniform vec3 lightDir;
    public @viewUniform vec3 lightParams;
    public @viewUniform sampler2D texGGX_D;
    public @viewUniform sampler2D texGGX_FV;
    public @viewUniform float time;
}

module StandardVertexFormat
{
    public @rootVert vec3 vert_pos;
    public @rootVert vec3 vert_normal;
    public @rootVert vec3 vert_tangent;
    public @rootVert vec2 vert_uv; 
}

module VertexTransform
{
    require vec3 vert_pos;
    require vec2 vert_uv; 
    require vec3 vert_tangent;
    require vec3 vert_normal;
    require mat4 viewProjectionMatrix;
    @modelTransform mat4 modelMatrix; 
    @modelTransform mat4 normalMatrix;
    
    vec4 position = modelMatrix * vec4(vert_pos, 1); 
    public vec4 projCoord = viewProjectionMatrix * position;
    public vec4 texSpaceVert = vec4(vert_uv*2.0 - vec2(1.0), 0.0, 1);
    public vec2 screenCoord = projCoord.xy/vec2(projCoord.w)*0.5 + vec2(0.5);
    public vec3 pos = position.xyz;
    public vec3 vNormal = (normalMatrix * vec4(vert_normal, 1.0)).xyz;
    public vec3 vTangent = (normalMatrix * vec4(vert_tangent, 1.0)).xyz;
    public vec3 vBiTangent = cross(vTangent, vNormal);
}

module TangentSpaceTransform
{
    require vec3 normal_in;
    require vec3 vNormal;
    require vec3 vTangent;
    require vec3 vBiTangent;
    public vec3 normal = normalize(normal_in.x * vTangent 
        + normal_in.y * vBiTangent 
        + normal_in.z * vNormal);
}

extern float computeShadow(vec3 worldPos);

float Pow4(float x)
{
    return (x*x)*(x*x);
}

vec2 LightingFuncGGX_FV(float dotLH, float roughness)
{
    float alpha = roughness*roughness;/*sf*/

    // F
    float F_a, F_b;
    float dotLH5 = Pow4(1.0-dotLH) * (1.0 - dotLH);
    F_a = 1.0;
    F_b = dotLH5;

    // V
    float vis;
    float k = alpha/2.0;
    float k2 = k*k;
    float invK2 = 1.0-k2;
    vis = 1.0/(dotLH*dotLH*invK2 + k2);

    return vec2(F_a*vis, F_b*vis);
}

float LightingFuncGGX_D(float dotNH, float roughness)
{
    float alpha = roughness*roughness;
    float alphaSqr = alpha*alpha;
    float pi = 3.14159;
    float denom = dotNH * dotNH *(alphaSqr-1.0) + 1.0;

    float D = alphaSqr/(pi * denom * denom);
    return D;
}

module Lighting
{
    require vec3 normal;   
    require vec3 albedo;
    require vec3 lightParam;
    require vec3 pos;
    require vec3 lightDir;
    require vec3 lightColor;
    require vec3 cameraPos;
    float shadow = computeShadow(pos);
    float brightness = clamp(dot(lightDir, normal), 0.0, 1.0) * shadow;
    vec3 view = normalize(cameraPos - pos);
    inline float roughness_in = lightParam.x;
    inline float metallic_in = lightParam.y;
    inline float specular_in = lightParam.z;
    vec3 L = lightDir;
    vec3 H = normalize(view+L);
    float dotNL = clamp(dot(normal,L), 0.01, 0.99);
    float dotLH = clamp(dot(L,H), 0.01, 0.99);
    float dotNH = clamp(dot(normal,H), 0.01, 0.99);
    float highlight : phongStandard
    {
        float alpha = roughness_in*roughness_in;
        float p = 6.644/(alpha*alpha) - 6.644;
        float pi = 3.14159;
        return dotNL *exp2(p * dotNH - p) / (pi * (alpha*alpha)) * specular_in;
    }
    float highlight : GGXstandard
    {
        float D = LightingFuncGGX_D(dotNH,roughness_in);
        vec2 FV_helper = LightingFuncGGX_FV(dotLH,roughness_in);
        float FV = metallic_in*FV_helper.x + (1.0-metallic_in)*FV_helper.y;
        float specular = dotNL * D * FV * specular_in;
        return specular;
    }
    public vec3 result = lightColor * 
                        (albedo * (brightness + 0.7)*(1.0-metallic_in) + 
                        mix(albedo, vec3(1.0), 1.0 - metallic_in) * (highlight * shadow));
}

module Fog
{
    require vec3 fogColor;
    require vec3 originalColor;
    require vec3 cameraPos;
    require vec3 pos;
    float fog
    {
        float fogDensity = 0.0002;
        float viewDist = length(cameraPos - pos);
        return 1.0/exp2(viewDist*fogDensity);
    }
    public vec3 result = mix(fogColor, originalColor, fog); 
}

float ComputeLuminance(vec3 color)
{
    return color.x * 0.3 + color.y * 0.59 + color.z * 0.11;
}

vec3 desaturate(vec3 color, float factor)
{
    float lum = ComputeLuminance(color);
    return mix(color, vec3(lum, lum, lum), factor);
}

module Header
{
    public using ViewUniforms;
    public using StandardVertexFormat;
    public using VertexTransform; 
}

module Footer
{
    require vec3 Normal;
    require float Roughness;
    require float Metallic;
    require float Specular;
    require vec3 Albedo;
    require vec3 vNormal;
    require vec3 vTangent;
    require vec3 vBiTangent;
    require vec3 pos;
    require vec3 lightDir;
    require vec3 lightColor;
    require vec3 cameraPos;
    using normalTransform = TangentSpaceTransform(Normal);
    using lighting = Lighting(normalTransform.normal, Albedo, vec3(Roughness, Metallic, Specular));
    public float opacity = 1.0;
    
    float fog
    {
        float fogDensity = 0.0002;
        float viewDist = length(cameraPos - pos);
        return 1.0/exp2(viewDist*fogDensity);
    }
    vec4 fogColor = vec4(0.54, 0.58, 0.67, 1.0);
    public vec4 outputColor = mix(fogColor, vec4(lighting.result, opacity), fog);
}

float ComputeHighlightPhong(vec3 L, vec3 N, vec3 V, float roughness, float metallic, float specular)
{
    vec3 H = normalize(V+L);
    float dotNL = clamp(dot(N,L), 0.01, 0.99);
    float dotNH = clamp(dot(N,H), 0.01, 0.99);

    float alpha = roughness*roughness;
    float p = 6.644/(alpha*alpha) - 6.644;
    float pi = 3.14159;
    return dotNL * metallic * exp2(p * dotNH - p) / (pi * (alpha*alpha)) * specular;
}

float ComputeHighlightGGX(vec3 L, vec3 N, vec3 V, float roughness, float metallic, float specular)
{
    vec3 H = normalize(V+L);
    float dotNL = clamp(dot(N,L), 0.01, 0.99);
    float dotLH = clamp(dot(L,H), 0.01, 0.99);
    float dotNH = clamp(dot(N,H), 0.01, 0.99);

    float D = LightingFuncGGX_D(dotNH,roughness);
    vec2 FV_helper = LightingFuncGGX_FV(dotLH,roughness);
    float FV = metallic;
    return dotNL * D * FV * specular;
}