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
float PhongApprox(float p_Roughness, float p_RoL);
float3 EnvBRDFApprox(float3 p_SpecularColor, float p_Roughness, float p_NoV);
float DeferredLighting_selfShadow_vec3(float3 p0_x);
float4 SubModuleWithParam_Eval_vec2(Texture2D p0_albedoTex, SamplerState p1_samplerState, float2 p2_uv);
float4 SubModuleWithParam1_Eval_vec2(Texture2D p0_albedoTex, SamplerState p1_samplerState, float2 p2_uv);
float PhongApprox(float p_Roughness, float p_RoL)
{
float a;
float a2;
float rcp_a2;
float c;
float p;
a = (p_Roughness * p_Roughness);
a = max(a, 8.000000379980e-03);
a2 = (a * a);
rcp_a2 = (1.000000000000e+00 / a2);
c = ((7.213475108147e-01 * rcp_a2) + 3.967411220074e-01);
p = (rcp_a2 * exp2(((c * p_RoL) - c)));
return min(p, rcp_a2);
}
float3 EnvBRDFApprox(float3 p_SpecularColor, float p_Roughness, float p_NoV)
{
float4 c0;
float4 c1;
float4 r;
float a004;
float2 AB;
c0 = float4((-1), (-2.749999985099e-02), (-5.720000267029e-01), 2.199999988079e-02);
c1 = float4(1, 4.250000044703e-02, 1.039999961853e+00, (-3.999999910593e-02));
r = ((p_Roughness * c0) + c1);
a004 = ((min((r.x * r.x), exp2(((-9.279999732971e+00) * p_NoV))) * r.x) + r.y);
AB = ((float2((-1.039999961853e+00), 1.039999961853e+00) * a004) + r.zw);
AB[1] = (AB.y * min((5.000000000000e+01 * p_SpecularColor.y), 1.000000000000e+00));
return ((p_SpecularColor * AB.x) + AB.y);
}
float DeferredLighting_selfShadow_vec3(float3 p0_x)
{
return 1.000000000000e+00;
}
float4 SubModuleWithParam_Eval_vec2(Texture2D p0_albedoTex, SamplerState p1_samplerState, float2 p2_uv)
{
return p0_albedoTex.Sample(p1_samplerState, p2_uv);
}
float4 SubModuleWithParam1_Eval_vec2(Texture2D p0_albedoTex, SamplerState p1_samplerState, float2 p2_uv)
{
return p0_albedoTex.Sample(p1_samplerState, p2_uv);
}
struct TCoarseVertex
{
float2 vertUV_CoarseVertex : A0A;
};
struct TFragment
{
float4 outputColor : A0A;
};
struct TFragmentExt
{
TFragment user : SV_Target;
};
TFragmentExt main(
    TCoarseVertex stage_input)
{ 
TFragmentExt stage_output;
float2 vertUV;
float4 normalSample;
float3 normal;
float4 pbr;
float3 albedo;
float3 lightParam;
float4 t4D;
float4 position;
float3 pos;
float3 view;
float roughness_in;
float metallic_in;
float shadow;
float3 viewPos;
int i = 0;
int t62 = 0;
float4 t63;
int t64 = 0;
float4 lightSpacePosT;
float3 lightSpacePos;
float val;
float3 lNormal;
float dielectricSpecluar;
float3 diffuseColor;
float3 specularColor;
float NoV;
float3 R;
float RoL;
float3 result;
float3 specularIBL;
float3 diffuseIBL;
float4 outputColor;
vertUV = stage_input/*standard*/.vertUV_CoarseVertex;
normalSample = DeferredLightingParams_normalTex.Sample(DeferredLightingParams_nearestSampler, vertUV);
normal = ((normalSample.xyz * 2.000000000000e+00) - 1.000000000000e+00);
pbr = DeferredLightingParams_pbrTex.Sample(DeferredLightingParams_nearestSampler, vertUV);
albedo = DeferredLightingParams_albedoTex.Sample(DeferredLightingParams_nearestSampler, vertUV).xyz;
lightParam = float3(pbr.x, pbr.y, pbr.z);
t4D = DeferredLightingParams_depthTex.Sample(DeferredLightingParams_nearestSampler, vertUV);
position = mul(float4(((vertUV.x * 2) - 1), ((vertUV.y * 2) - 1), t4D.x, 1.000000000000e+00), ForwardBasePassParams.invViewProjTransform);
pos = (position.xyz / position.w);
view = normalize((ForwardBasePassParams.cameraPos - pos));
roughness_in = lightParam.x;
metallic_in = lightParam.y;
shadow = DeferredLighting_selfShadow_vec3(lighting.lightDir);
if (bool(lighting.numCascades))
{
viewPos = mul(float4(pos, 1.000000000000e+00), ForwardBasePassParams.viewTransform).xyz;
i = 0;
for (; (i < lighting.numCascades); (i = (i + 1)))
{
t62 = (i >> 2);
t63 = lighting.zPlanes[t62];
t64 = (i & 3);
if (bool(((-viewPos.z) < t63[t64])))
{
lightSpacePosT = mul(float4(pos, 1.000000000000e+00), lighting.lightMatrix[i]);
lightSpacePos = (lightSpacePosT.xyz / lightSpacePosT.w);
val = lighting_shadowMapArray.SampleCmp(lighting_shadowMapSampler, float3(lightSpacePos.xy, (i + lighting.shadowMapId)), lightSpacePos.z);
shadow = (shadow * val);
break;
}
}
}
if (bool((normalSample.w != 0.000000000000e+00)))
{
lNormal = ((dot(normal, view) < 0)?(-normal):normal);
}
else
{
lNormal = normal;
}
dielectricSpecluar = (1.999999955297e-02 * lightParam.z);
diffuseColor = (albedo - (albedo * metallic_in));
specularColor = (((float3) (dielectricSpecluar - (dielectricSpecluar * metallic_in))) + (albedo * metallic_in));
NoV = max(dot(lNormal, view), 0.000000000000e+00);
specularColor = EnvBRDFApprox(specularColor, roughness_in, NoV);
R = reflect((-view), lNormal);
RoL = max(0, dot(R, lighting.lightDir));
result = (((lighting.lightColor * clamp(dot(lNormal, lighting.lightDir), 9.999999776483e-03, 9.900000095367e-01)) * (diffuseColor + (specularColor * PhongApprox(roughness_in, RoL)))) * shadow);
specularIBL = (specularColor * lighting_envMap.SampleLevel(ForwardBasePassParams_textureSampler, R, (clamp(roughness_in, 0.000000000000e+00, 1.000000000000e+00) * 8.000000000000e+00)).xyz);
diffuseIBL = ((diffuseColor * lighting_envMap.SampleLevel(ForwardBasePassParams_textureSampler, lNormal, 8.000000000000e+00).xyz) * lighting.ambient);
result = (result + (specularIBL + diffuseIBL));
result = (result * pbr.w);
outputColor = (float4(result, 1.000000000000e+00) + ((SubModuleWithParam_Eval_vec2(layers_l0_albedoTex, layers_l0_samplerState, ((float2) 0.000000000000e+00)) + SubModuleWithParam_Eval_vec2(layers_l1_albedoTex, layers_l1_samplerState, ((float2) 0.000000000000e+00))) + SubModuleWithParam1_Eval_vec2(layers_l2_albedoTex, layers_l2_samplerState, ((float2) 0.000000000000e+00))));
stage_output.user.outputColor = outputColor;
return stage_output;
}