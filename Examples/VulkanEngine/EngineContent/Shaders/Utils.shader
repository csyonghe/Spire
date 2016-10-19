
vec4 QuaternionMul(vec4 q1, vec4 q2)
{
    vec4 rs;
    rs.x = q1.w*q2.x + q1.x*q2.w + q1.y*q2.z - q1.z*q2.y;
    rs.y = q1.w*q2.y + q1.y*q2.w + q1.z*q2.x - q1.x*q2.z;
    rs.z = q1.w*q2.z + q1.z*q2.w + q1.x*q2.y - q1.y*q2.x;
    rs.w = q1.w*q2.w - q1.x*q2.x - q1.y*q2.y - q1.z*q2.z;
    return rs;
}

vec4 QuaternionConjugate(vec4 q)
{
    return vec4(-q.x, -q.y, -q.z, q.w);    
}

vec3 QuaternionRotate(vec4 q, vec3 pos)
{
    return QuaternionMul(QuaternionMul(q, vec4(pos, 0.0)), QuaternionConjugate(q)).xyz;
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

/*
module SimpleStaticMeshVertex
{
    public @rootVert vec3 vertPos;
    public @rootVert vec2 vertUV;
    public @rootVert uint tangentFrame;
    
    vec4 tangentFrameQuaternion
    {
        vec4 result;
        float inv255 = 2.0 / 255.0;
        result.x = float(tangentFrame & 255) * inv255 - 1.0;   
        result.y = float((tangentFrame >> 8) & 255) * inv255 - 1.0;   
        result.z = float((tangentFrame >> 16) & 255) * inv255 - 1.0;
        result.w = float((tangentFrame >> 24) & 255) * inv255 - 1.0;   
        return result;
    }
    public @vs vec3 vertNormal
    {
       return normalize(QuaternionRotate(tangentFrameQuaternion, vec3(0.0, 1.0, 0.0)));
    } 
    public @vs vec3 vertTangent
    {
       return normalize(QuaternionRotate(tangentFrameQuaternion, vec3(1.0, 0.0, 0.0)));
    }
    public vec3 vertBinormal = cross(vertTangent, vertNormal);
}

module SimpleSkeletalMeshVertex
{
    public using SimpleStaticMeshVertex;
    public @rootVert uint boneIds;
    public @rootVert uint boneWeights;
    
}
*/

module SystemUniforms
{
    public @viewUniform mat4 viewTransform;
    public @viewUniform mat4 viewProjectionTransform;
    public @viewUniform mat4 invViewTransform;
    public @viewUniform mat4 invViewProjTransform;
    public @viewUniform vec3 cameraPos;
    public vec3 lightDir = vec3(1.0, 1.0, 0.0);
    public vec3 lightColor = vec3(1.5, 1.5, 1.5);
}

module VertexTransform
{
    require vec3 fineVertPos;
    require vec3 coarseVertTangent;
    require vec3 coarseVertNormal;
    require vec3 displacement;
    require mat4 viewProjectionTransform;
    @modelTransform mat4 modelMatrix; 
    @modelTransform mat4 normalMatrix;
    
    vec4 position = modelMatrix * vec4((fineVertPos+displacement), 1); 
    public vec4 projCoord = viewProjectionTransform * position;
    public vec3 pos = position.xyz;
    public vec3 vNormal = (normalMatrix * vec4(coarseVertNormal, 0.0)).xyz;
    public vec3 vTangent = (normalMatrix * vec4(coarseVertTangent, 0.0)).xyz;
    public vec3 vBiTangent = cross(vTangent, vNormal);
}

struct BoneTransform
{
    mat4 transformMatrix;
    mat3 normalMatrix;
}

module StaticVertex
{
    public using MeshVertex;
    public @vs vec3 coarseVertPos = vertPos;
    public @vs vec3 coarseVertNormal = vertNormal;
    public @vs vec3 coarseVertTangent = vertTangent;
}

module SkinnedVertex
{
    public using MeshVertex;
    require mat4 viewProjectionTransform;
    @skeletalTransform BoneTransform[] boneTransforms;
    
    public @vs vec3 coarseVertPos
    {
        vec3 result = vec3(0.0);
        for (int i = 0 : 3)
        {
            uint boneId = (boneIds >> (i*8)) & 255;
            if (boneId == 255) continue;
            float boneWeight = float((boneWeights >> (i*8)) & 255) * (1.0/255.0);
            vec3 tp = (boneTransforms[boneId].transformMatrix * vec4(vertPos, 1.0)).xyz;
            result += tp * boneWeight;
        }
        return result;
    }
    public @vs vec3 coarseVertNormal
    {
        vec3 result = vec3(0.0);
        for (int i = 0 : 3)
        {
            uint boneId = (boneIds >> (i*8)) & 255;
            if (boneId == 255) continue;            
            float boneWeight = float((boneWeights >> (i*8)) & 255) * (1.0/255.0);
            vec3 tp = boneTransforms[boneId].normalMatrix * vertNormal;
            result += tp * boneWeight;
        }
        return result;
    }
    public @vs vec3 coarseVertTangent
    {
        vec3 result = vec3(0.0);
        for (int i = 0 : 3)
        {
            uint boneId = (boneIds >> (i*8)) & 255;
            if (boneId == 255) continue;            
            float boneWeight = float((boneWeights >> (i*8)) & 255) * (1.0/255.0);
            vec3 tp = boneTransforms[boneId].normalMatrix * vertTangent;
            result += tp * boneWeight;
        }
        return result;
    }
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

module NoTessellation
{
    require vec3 coarseVertPos;
    public vec3 fineVertPos = coarseVertPos;
}

vec3 ProjectToPlane(vec3 Point, vec3 PlanePoint, vec3 PlaneNormal)
{
    vec3 v = Point - PlanePoint;
    float Len = dot(v, PlaneNormal);
    vec3 d = Len * PlaneNormal;
    return (Point - d);
}

module PN_Tessellation : TessellationPipeline
{
    require vec3 coarseVertPos;
    require vec3 coarseVertNormal;
    
    public @tcs vec4 tessLevelOuter = vec4(3, 3, 3, 0);
    public @tcs vec2 tessLevelInner = vec2(3.0);
    
    @tcs vec3 WorldPos_B030 = indexImport(coarseVertPos, 0);
    @tcs vec3 WorldPos_B003 = indexImport(coarseVertPos, 1);
    @tcs vec3 WorldPos_B300 = indexImport(coarseVertPos, 2);

    // Edges are names according to the opposing vertex
    vec3 EdgeB300 = WorldPos_B003 - WorldPos_B030;
    vec3 EdgeB030 = WorldPos_B300 - WorldPos_B003;
    vec3 EdgeB003 = WorldPos_B030 - WorldPos_B300;

    // Generate two midpoints on each edge
    vec3 WorldPos_B021t = WorldPos_B030 + EdgeB300 / 3.0;
    vec3 WorldPos_B012t = WorldPos_B030 + EdgeB300 * 2.0 / 3.0;
    vec3 WorldPos_B102t = WorldPos_B003 + EdgeB030 / 3.0;
    vec3 WorldPos_B201t = WorldPos_B003 + EdgeB030 * 2.0 / 3.0;
    vec3 WorldPos_B210t = WorldPos_B300 + EdgeB003 / 3.0;
    vec3 WorldPos_B120t = WorldPos_B300 + EdgeB003 * 2.0 / 3.0;

    // Project each midpoint on the plane defined by the nearest vertex and its normal
    @tcs vec3 WorldPos_B021 = ProjectToPlane(WorldPos_B021t, WorldPos_B030,
                                         indexImport(coarseVertNormal, 0));
    @tcs vec3 WorldPos_B012 = ProjectToPlane(WorldPos_B012t, WorldPos_B003,
                                         indexImport(coarseVertNormal, 1));
    @tcs vec3 WorldPos_B102 = ProjectToPlane(WorldPos_B102t, WorldPos_B003,
                                         indexImport(coarseVertNormal, 1));
    @tcs vec3 WorldPos_B201 = ProjectToPlane(WorldPos_B201t, WorldPos_B300,
                                         indexImport(coarseVertNormal, 2));
    @tcs vec3 WorldPos_B210 = ProjectToPlane(WorldPos_B210t, WorldPos_B300,
                                         indexImport(coarseVertNormal, 2));
    @tcs vec3 WorldPos_B120 = ProjectToPlane(WorldPos_B120t, WorldPos_B030,
                                         indexImport(coarseVertNormal, 0));

    // Handle the center
    vec3 Center = (WorldPos_B003 + WorldPos_B030 + WorldPos_B300) / 3.0;
    vec3 WorldPos_B111t = (WorldPos_B021 + WorldPos_B012 + WorldPos_B102 +
                          WorldPos_B201 + WorldPos_B210 + WorldPos_B120) / 6.0;
    vec3 WorldPos_B111 = WorldPos_B111t + (WorldPos_B111t - Center) / 2.0;

    
    float u = tessCoord.x;
    float v = tessCoord.y;
    float w = tessCoord.z;

    float vPow2 = v*v;
    float uPow2 = u*u;
    float wPow2 = w*w;
    float uPow3 = uPow2 * u;
    float vPow3 = vPow2 * v;
    float wPow3 = wPow2 * w;

    public @tes float3 fineVertPos = indexImport(WorldPos_B300, 0) * wPow3 +
                    indexImport(WorldPos_B030, 0) * uPow3 +
                    indexImport(WorldPos_B003, 0) * vPow3 +
                    indexImport(WorldPos_B210, 0) * 3.0 * wPow2 * u +
                    indexImport(WorldPos_B120, 0) * 3.0 * w * uPow2 +
                    indexImport(WorldPos_B201, 0) * 3.0 * wPow2 * v +
                    indexImport(WorldPos_B021, 0) * 3.0 * uPow2 * v +
                    indexImport(WorldPos_B102, 0) * 3.0 * w * vPow2 +
                    indexImport(WorldPos_B012, 0) * 3.0 * u * vPow2 +
                    indexImport(WorldPos_B111, 0) * 6.0 * w * u * v;
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
    float shadow = 1.0;
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
