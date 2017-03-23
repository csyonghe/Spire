#pragma warning(disable: 3576)
#pragma pack_matrix( row_major )
struct SkinningResult
{
float3 pos;
float3 tangent;
float3 binormal;
};
Texture2D DeferredLightingParams_albedoTex: register(t0, space-1);
Texture2D DeferredLightingParams_pbrTex: register(t1, space-1);
Texture2D DeferredLightingParams_normalTex: register(t2, space-1);
Texture2D DeferredLightingParams_depthTex: register(t3, space-1);
SamplerState DeferredLightingParams_nearestSampler: register(s0, space-1);
SamplerState ForwardBasePassParams_textureSampler: register(s1, space-1);
Texture2DArray lighting_shadowMapArray: register(t4, space-1);
SamplerComparisonState lighting_shadowMapSampler: register(s2, space-1);
TextureCube lighting_envMap: register(t5, space-1);
Texture2D layers_l0_albedoTex: register(t6, space-1);
SamplerState layers_l0_samplerState: register(s3, space-1);
Texture2D layers_l1_albedoTex: register(t7, space-1);
SamplerState layers_l1_samplerState: register(s4, space-1);
Texture2D layers_l2_albedoTex: register(t8, space-1);
SamplerState layers_l2_samplerState: register(s5, space-1);
Texture2D DeferredLightingParams_albedoTex: register(t0, space0);
Texture2D DeferredLightingParams_pbrTex: register(t1, space0);
Texture2D DeferredLightingParams_normalTex: register(t2, space0);
Texture2D DeferredLightingParams_depthTex: register(t3, space0);
SamplerState DeferredLightingParams_nearestSampler: register(s0, space0);
cbuffer bufForwardBasePassParams : register(b1)
{
struct {
float4x4 viewTransform;
float4x4 viewProjectionTransform;
float4x4 invViewTransform;
float4x4 invViewProjTransform;
float3 cameraPos;
float time;
} ForwardBasePassParams;
};
SamplerState ForwardBasePassParams_textureSampler: register(s0, space1);
cbuffer buflighting : register(b2)
{
struct {
float3 lightDir;
float3 lightColor;
float ambient;
int shadowMapId;
int numCascades;
float4x4 lightMatrix[8];
float4 zPlanes[2];
} lighting;
};
Texture2DArray lighting_shadowMapArray: register(t0, space2);
SamplerComparisonState lighting_shadowMapSampler: register(s0, space2);
TextureCube lighting_envMap: register(t1, space2);
Texture2D layers_l0_albedoTex: register(t0, space3);
SamplerState layers_l0_samplerState: register(s0, space3);
Texture2D layers_l1_albedoTex: register(t1, space3);
SamplerState layers_l1_samplerState: register(s1, space3);
Texture2D layers_l2_albedoTex: register(t2, space3);
SamplerState layers_l2_samplerState: register(s2, space3);
struct TMeshVertex
{
float2 vertPos : A0A;
float2 vertUV : A1A;
};
struct TCoarseVertex
{
float2 vertUV_CoarseVertex : A0A;
};
struct TCoarseVertexExt
{
TCoarseVertex user;
float4 sv_position : SV_Position;
};
TCoarseVertexExt main(
    TMeshVertex stage_input : A)
{ 
TCoarseVertexExt stage_output;
float2 vertPos;
float2 vertUV;
vertPos = stage_input/*standard*/.vertPos;
vertUV = stage_input/*standard*/.vertUV;
stage_output.user.vertUV_CoarseVertex = vertUV;
stage_output.sv_position = float4(vertPos.xy, 0.000000000000e+00, 1.000000000000e+00);
return stage_output;
}