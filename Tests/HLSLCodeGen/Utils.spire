
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

module TangentSpaceTransform
{
    require vec3 coarseVertTangent;
    require vec3 coarseVertBinormal;
    require vec3 worldTransformTangent(vec3 pos);
    public vec3 vTangent = worldTransformTangent(coarseVertTangent);
    public vec3 vBiTangent = worldTransformTangent(coarseVertBinormal);
    public vec3 vNormal = cross(vBiTangent, vTangent);
    
    public vec3 WorldSpaceToTangentSpace(vec3 v)
    {
        return vec3(dot(v, vTangent), dot(v, vBiTangent), dot(v, vNormal));    
    }
    public vec3 TangentSpaceToWorldSpace(vec3 v)
    {
        return v.x * vTangent + v.y * vBiTangent + v.z * vNormal;        
    }
}

module VertexTransform
{
    require vec3 fineVertPos;
    require vec3 displacement;
    require mat4 viewProjectionTransform;
    require vec3 worldTransformPos(vec3 pos);

    public vec3 pos = worldTransformPos(fineVertPos+displacement); 
    public vec4 projCoord
    {
        vec4 rs = viewProjectionTransform * vec4(pos, 1);
        return rs;
    } 
}

module NoAnimation
{
    param mat4 modelMatrix;

    require vec3 vertPos;
    require vec3 vertNormal;
    require vec3 vertTangent;
    require vec3 vertBinormal;
    
    public @CoarseVertex vec3 coarseVertPos = vertPos;
    public @CoarseVertex vec3 coarseVertNormal = vertNormal;
    public @CoarseVertex vec3 coarseVertTangent = vertTangent;
    public @CoarseVertex vec3 coarseVertBinormal = vertBinormal;

    public vec3 worldTransformPos(vec3 pos)
    {
        return (modelMatrix * vec4(pos, 1)).xyz;
    }
    public vec3 worldTransformTangent(vec3 tangent)
    {
        return normalize(mat3(modelMatrix) * tangent);
    }
}

struct SkinningResult
{
    vec3 pos;
    vec3 tangent;
    vec3 binormal;
}

module SkeletalAnimation
{
    require vec3 vertPos;
    require vec3 vertBinormal;
    require vec3 vertTangent;
    require uint boneIds;
    require uint boneWeights;

    require mat4 viewProjectionTransform;

    param mat4[128] boneTransforms;
    
    public SkinningResult skinning
    {
        SkinningResult result;
        result.pos = vec3(0.0);
        result.binormal = vec3(0.0);
        result.tangent = vec3(0.0);
        for (int i = 0; i < 4; i++)
        {
            uint boneId = (boneIds >> (i*8)) & 255;
            if (boneId == 255) continue;
            float boneWeight = float((boneWeights >> (i*8)) & 255) * (1.0/255.0);
            vec3 tp = (boneTransforms[boneId] * vec4(vertPos, 1.0)).xyz;
            result.pos += tp * boneWeight;
            tp = mat3(boneTransforms[boneId]) * vertBinormal;
            result.binormal += tp * boneWeight;
            tp = mat3(boneTransforms[boneId]) * vertTangent;
            result.tangent += tp * boneWeight;
        }
        result.binormal = normalize(result.binormal);
        result.tangent = normalize(result.tangent);
        return result;
    }
    public @CoarseVertex vec3 coarseVertPos = skinning.pos;
    public @CoarseVertex vec3 coarseVertBinormal = skinning.binormal;
    public @CoarseVertex vec3 coarseVertTangent = skinning.tangent;
    public @CoarseVertex vec3 coarseVertNormal = normalize(cross(coarseVertBinormal, coarseVertTangent));

	public vec3 worldTransformPos(vec3 pos)
    {
        return pos;
    }
    public vec3 worldTransformTangent(vec3 tangent)
    {
        return tangent;
    }
}

module NoTessellation
{
    require vec3 coarseVertPos;
    public vec3 fineVertPos = coarseVertPos;
}

module PN_Tessellation targets TessellationPipeline
{
    require vec3 coarseVertPos;
    require vec3 coarseVertNormal;
    
    public @ControlPoint vec4 tessLevelOuter = vec4(3, 3, 3, 0);
    public @ControlPoint vec2 tessLevelInner = vec2(3.0);
    
    vec3 ProjectToPlane(vec3 Point, vec3 PlanePoint, vec3 PlaneNormal)
    {
        vec3 v = Point - PlanePoint;
        float Len = dot(v, PlaneNormal);
        vec3 d = Len * PlaneNormal;
        return (Point - d);
    }
    
    @ControlPoint vec3 WorldPos_B030 = indexImport(coarseVertPos, 0);
    @ControlPoint vec3 WorldPos_B003 = indexImport(coarseVertPos, 1);
    @ControlPoint vec3 WorldPos_B300 = indexImport(coarseVertPos, 2);

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
    @ControlPoint vec3 WorldPos_B021 = ProjectToPlane(WorldPos_B021t, WorldPos_B030,
                                         indexImport(coarseVertNormal, 0));
    @ControlPoint vec3 WorldPos_B012 = ProjectToPlane(WorldPos_B012t, WorldPos_B003,
                                         indexImport(coarseVertNormal, 1));
    @ControlPoint vec3 WorldPos_B102 = ProjectToPlane(WorldPos_B102t, WorldPos_B003,
                                         indexImport(coarseVertNormal, 1));
    @ControlPoint vec3 WorldPos_B201 = ProjectToPlane(WorldPos_B201t, WorldPos_B300,
                                         indexImport(coarseVertNormal, 2));
    @ControlPoint vec3 WorldPos_B210 = ProjectToPlane(WorldPos_B210t, WorldPos_B300,
                                         indexImport(coarseVertNormal, 2));
    @ControlPoint vec3 WorldPos_B120 = ProjectToPlane(WorldPos_B120t, WorldPos_B030,
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

    public @FineVertex float3 fineVertPos = indexImport(WorldPos_B300, 0) * wPow3 +
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

module ParallaxOcclusionMapping
{
    require float GetHeight(vec2 uvCoord);
    require vec3 viewDirTangentSpace;
    require vec2 uv;
    require float parallaxScale;
    require SamplerState textureSampler;

    vec3 parallaxMapping
    {
        vec3 V = viewDirTangentSpace;
        vec2 T = uv;
        
        float parallaxHeight;
        // determine optimal number of layers
        float minLayers = 10;
        float maxLayers = 15;
        float numLayers = mix(maxLayers, minLayers, abs(V.z));

        // height of each layer
        float layerHeight = 1.0 / numLayers;
        // current depth of the layer
        float curLayerHeight = 0.01;
        // shift of texture coordinates for each layer
        vec2 dtex = parallaxScale * V.xy / max(V.z, 1e-5) / numLayers;
        dtex.y = -dtex.y;
        // current texture coordinates
        vec2 currentTextureCoords = T;

        // depth from heightmap
        float heightFromTexture = 1.0 - GetHeight(currentTextureCoords);//heightTexture.Sample(textureSampler, currentTextureCoords).r;

        // while point is above the surface
        while (heightFromTexture > curLayerHeight) 
        {
            // to the next layer
            curLayerHeight += layerHeight; 
            // shift of texture coordinates
            currentTextureCoords -= dtex;
            // new depth from heightmap
            heightFromTexture = 1.0 - GetHeight(currentTextureCoords);//-heightTexture.Sample(textureSampler, currentTextureCoords).r;
        }
         ///////////////////////////////////////////////////////////
        // Start of Relief Parallax Mapping

        // decrease shift and height of layer by half
        vec2 deltaTexCoord = dtex / 2;
        float deltaHeight = layerHeight / 2;

        // return to the mid point of previous layer
        currentTextureCoords += deltaTexCoord;
        curLayerHeight -= deltaHeight;

        // binary search to increase precision of Steep Paralax Mapping
        int numSearches = 5;
        for (int i = 0; i <= numSearches; i++)
        {
            // decrease shift and height of layer by half
            deltaTexCoord /= 2;
            deltaHeight /= 2;

            // new depth from heightmap
            heightFromTexture = 1.0 - GetHeight(currentTextureCoords);//heightTexture.Sample(textureSampler, currentTextureCoords).r;

            // shift along or agains vector V
            if(heightFromTexture > curLayerHeight) // below the surface
            {
                currentTextureCoords -= deltaTexCoord;
                curLayerHeight += deltaHeight;
            }
            else // above the surface
            {
                currentTextureCoords += deltaTexCoord;
                curLayerHeight -= deltaHeight;
            }
        }
        parallaxHeight = curLayerHeight; 
        return vec3(currentTextureCoords, parallaxHeight);
    }

    public vec2 uvOut = parallaxMapping.xy;
    public float heightOut = parallaxMapping.z;

    public float selfShadow(vec3 L_tangentSpace)
    {
        float initialHeight = heightOut - 0.05;
        vec3 L = L_tangentSpace;
        vec2 initialTexCoord = uvOut;
        
        float shadowMultiplier = 1;
        float minLayers = 15;
        float maxLayers = 30;

        // calculate lighting only for surface oriented to the light source
        if (L.z > 0)
        {
            // calculate initial parameters
            float numSamplesUnderSurface = 0;
            shadowMultiplier = 0;
            float numLayers	= mix(maxLayers, minLayers, abs(L.z));
            float layerHeight = max(0.03, abs(initialHeight / numLayers));
            vec2 texStep = parallaxScale * L.xy / L.z / numLayers;
            texStep.y = -texStep.y;        
            // current parameters
            float currentLayerHeight = initialHeight - layerHeight;
            vec2 currentTextureCoords = initialTexCoord + texStep;
            float heightFromTexture	= 1.0 - GetHeight(currentTextureCoords); //heightTexture.Sample(textureSampler, currentTextureCoords).r;
            // while point is below depth 0.0 )
            while(currentLayerHeight > 0)
            {
                // if point is under the surface
                if(heightFromTexture < currentLayerHeight)
                {
                    numSamplesUnderSurface += 1;
                    break;
                }

                // ofFragmentet to the next layer
                currentLayerHeight -= layerHeight;
                currentTextureCoords += texStep;
                heightFromTexture = 1.0 - GetHeight(currentTextureCoords); //-heightTexture.Sample(textureSampler, currentTextureCoords).r;
            }

            // Shadowing factor should be 1 if there were no points under the surface
            if(numSamplesUnderSurface < 1)
            {
                shadowMultiplier = 1;
            }
            else
            {
                shadowMultiplier = 0.0;
            }
        }
        return shadowMultiplier;
    }
}

 float PhongApprox(float Roughness, float RoL)
{
    float a = Roughness * Roughness;	
    a = max(a, 0.008);					
    float a2 = a * a;						
    float rcp_a2 = 1.0/(a2);					
    float c = 0.72134752 * rcp_a2 + 0.39674113;	
    float p = rcp_a2 * exp2(c * RoL - c);	
    // Total 7 instr
    return min(p, rcp_a2);
}
vec3 EnvBRDFApprox( vec3 SpecularColor, float Roughness, float NoV )
{
    vec4 c0 = vec4(-1, -0.0275, -0.572, 0.022);
    vec4 c1 = vec4(1, 0.0425, 1.04, -0.04);
    vec4 r = Roughness * c0 + c1;
    float a004 = min( r.x * r.x, exp2( -9.28 * NoV ) ) * r.x + r.y;
    vec2 AB = vec2( -1.04, 1.04 ) * a004 + r.zw;
    AB.y *= min(50.0 * SpecularColor.g, 1.0);
    return SpecularColor * AB.x + AB.y;
}

module Lighting
{
    public param vec3 lightDir;
    public param vec3 lightColor;
    public param float ambient;
    public param int shadowMapId;
    public param int numCascades;
    public param mat4[8] lightMatrix;
    public param vec4[2] zPlanes;
    public param Texture2DArrayShadow shadowMapArray;
    public param SamplerComparisonState shadowMapSampler;
    public param TextureCube envMap;

    require vec3 normal;   
    require vec3 albedo;
    require vec3 lightParam;
    require float ao;
    require vec3 pos;
    require vec3 cameraPos;
    require float selfShadow(vec3 lightDir);
    require mat4 viewTransform;
    require bool isDoubleSided;
    require SamplerState textureSampler;

    vec3 lNormal
    {
		vec3 result;
        if (isDoubleSided)
        {
            result = dot(normal, view) < 0 ? -normal : normal;
        }
        else
        {
            result = normal;
        }
		return result;
    }

    vec3 view = normalize(cameraPos - pos);
    inline float roughness_in = lightParam.x;
    inline float metallic_in = lightParam.y;
    inline float specular_in = lightParam.z;
    vec3 L = lightDir;
    vec3 H = normalize(view+L);
    float dotNL = clamp(dot(lNormal,L), 0.01, 0.99);
    float dotLH = clamp(dot(L,H), 0.01, 0.99);
    float dotNH = clamp(dot(lNormal,H), 0.01, 0.99);
    
    float Pow4(float x)
    {
        return (x*x)*(x*x);
    }

    vec2 LightingFuncGGX_FV(float dotLH, float roughness)
    {
        float alpha = roughness*roughness;/*sf*/

        // F
        float F_a; float F_b;
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

    float shadow
    {
        float result = selfShadow(lightDir);
        if (numCascades)
        {
            vec3 viewPos = (viewTransform * vec4(pos, 1.0)).xyz;
            for (int i = 0; i < numCascades; i++)
            {
                if (-viewPos.z < zPlanes[i>>2][i&3])
                {
                    vec4 lightSpacePosT = lightMatrix[i] * vec4(pos, 1.0);
                    vec3 lightSpacePos = lightSpacePosT.xyz / lightSpacePosT.w;
                    float val = shadowMapArray.SampleCmp(shadowMapSampler, 
                        vec3(lightSpacePos.xy, i+shadowMapId), lightSpacePos.z);
                    result *= val;
                    break;
                }
            }
        }
        return result;
    }
    
    float brightness = clamp(dot(lightDir, lNormal), 0.0, 1.0) * shadow;

    public vec3 result
    {
        float dielectricSpecluar = 0.02 * specular_in;
        vec3 diffuseColor = albedo - albedo * metallic_in;
        vec3 specularColor = vec3(dielectricSpecluar - dielectricSpecluar * metallic_in) + albedo * metallic_in;
        float NoV = max(dot(lNormal, view), 0.0);
        specularColor = EnvBRDFApprox(specularColor, roughness_in, NoV);
        vec3 R = reflect(-view, lNormal);
        float RoL = max(0, dot(R, lightDir));
        vec3 color = lightColor * dotNL * (diffuseColor + specularColor * PhongApprox(roughness_in, RoL)) * shadow;
        vec3 specularIBL = specularColor * envMap.SampleLevel(textureSampler, R, 
                            clamp(roughness_in, 0.0, 1.0) * 8.0).xyz;
        vec3 diffuseIBL = diffuseColor * envMap.SampleLevel(textureSampler, lNormal, 
                            8.0).xyz * ambient;
        color += specularIBL + diffuseIBL;
        color *= ao;
        return color;
    }
}

module ForwardBasePassParams
{
    public param mat4 viewTransform;
    public param mat4 viewProjectionTransform;
    public param mat4 invViewTransform;
    public param mat4 invViewProjTransform;
    public param vec3 cameraPos;
    public param float time;  
    public param SamplerState textureSampler;  
}

interface IMaterialPattern
{
    vec3 albedo = vec3(1.0);
    vec3 normal = vec3(0.0, 0.0, 1.0);
    float roughness = 0.5;
    float metallic = 0.3;
    float specular = 0.8;
    float opacity = 1.0;
    float ao = 1.0;
    bool isDoubleSided = false;
    float selfShadow(vec3 lightDir)
    {
        return 1.0;        
    }
}

interface IMaterialGeometry
{
    public vec3 fineVertPos;
    public vec3 displacement;
}