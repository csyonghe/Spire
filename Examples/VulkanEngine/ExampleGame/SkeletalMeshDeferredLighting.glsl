name SkeletalMeshDeferredLighting
source
{
	vs
	text
	{
		#version 440
		struct BoneTransform
		{
			mat4 transformMatrix;
			mat3 normalMatrix;
		};
		struct SkinningResult
		{
			vec3 pos;
			vec3 normal;
			vec3 tangent;
		};
		vec4 QuaternionMul(vec4 p_q1, vec4 p_q2);
		vec4 QuaternionConjugate(vec4 p_q);
		vec3 QuaternionRotate(vec4 p_q, vec3 p_pos);
		vec4 QuaternionMul(vec4 p_q1, vec4 p_q2)
		{
			vec4 v0_rs;
			vec4 t23;
			vec4 t35;
			vec4 t47;
			vec4 t59;
			t23 = v0_rs;
			t23[0] = ((((p_q1.w * p_q2.x) + (p_q1.x * p_q2.w)) + (p_q1.y * p_q2.z)) - (p_q1.z * p_q2.y));
			v0_rs = t23;
			t35 = v0_rs;
			t35[1] = ((((p_q1.w * p_q2.y) + (p_q1.y * p_q2.w)) + (p_q1.z * p_q2.x)) - (p_q1.x * p_q2.z));
			v0_rs = t35;
			t47 = v0_rs;
			t47[2] = ((((p_q1.w * p_q2.z) + (p_q1.z * p_q2.w)) + (p_q1.x * p_q2.y)) - (p_q1.y * p_q2.x));
			v0_rs = t47;
			t59 = v0_rs;
			t59[3] = ((((p_q1.w * p_q2.w) - (p_q1.x * p_q2.x)) - (p_q1.y * p_q2.y)) - (p_q1.z * p_q2.z));
			v0_rs = t59;
			return v0_rs;
		}
		vec4 QuaternionConjugate(vec4 p_q)
		{
			return vec4((-p_q.x), (-p_q.y), (-p_q.z), p_q.w);
		}
		vec3 QuaternionRotate(vec4 p_q, vec3 p_pos)
		{
			return QuaternionMul(QuaternionMul(p_q, vec4(p_pos, 0.000000000000e+00)), QuaternionConjugate(p_q)).xyz;
		}
		layout(std430, binding = 3) buffer skeletalTransformBlock
		{
			BoneTransform[] boneTransforms;
		} blkskeletalTransformBlock;
		layout(binding = 0) uniform sampler2D albedoMap;
		layout(binding = 1) uniform sampler2D normalMap;
		layout(binding = 2) uniform sampler2D displacementMap;
		layout(std140, binding = 1) uniform viewUniformBlock
		{
			mat4 viewTransform;
			mat4 viewProjectionTransform;
			mat4 invViewTransform;
			mat4 invViewProjTransform;
			vec3 cameraPos;
		} blkviewUniformBlock;
		layout(location = 0) in vec3 vertPos_atrootVert;
		layout(location = 1) in vec2 vertUV0_atrootVert;
		layout(location = 2) in uint tangentFrame_atrootVert;
		out vec3 coarseVertPos_atvs;
		out vec3 coarseVertNormal_atvs;
		out vec3 vertPosI_at_Ivs_atvs;
		out vec3 vertNormal_atvs;
		out vec3 vertTangent_atvs;
		out vec2 vertUV0I_at_Ivs_atvs;
		void main()
		{
			uint _vcmpboneIds;
			uint _vcmpboneWeights;
			vec3 t879;
			vec3 _vcmpvertPosI_at_Ivs;
			uint t87d;
			uint _vcmptangentFrameI_at_Ivs;
			vec2 t880;
			vec2 _vcmpvertUV0I_at_Ivs;
			vec4 v0_result;
			float v1_inv255;
			vec4 t88a;
			vec4 t892;
			vec4 t89a;
			vec4 t8a2;
			vec4 _vcmpSkinnedVertexI_IMeshVertexI_ItangentFrameQuaternion;
			vec3 t8a8;
			vec3 _vcmpvertNormal;
			vec3 t8ad;
			vec3 _vcmpvertTangent;
			SkinningResult v2_result;
			SkinningResult t8b1;
			SkinningResult t8b5;
			SkinningResult t8b9;
			int v_3i;
			uint v4_boneId;
			float v5_boneWeight;
			vec3 v6_tp;
			BoneTransform t8d2;
			SkinningResult t8da;
			BoneTransform t8dd;
			SkinningResult t8e4;
			BoneTransform t8e7;
			SkinningResult t8ee;
			SkinningResult _vcmpskinning;
			vec3 t8f2;
			vec3 _vcmpcoarseVertPos;
			vec3 t8f5;
			vec3 _vcmpcoarseVertNormal;
			_vcmpboneIds = 255;
			_vcmpboneWeights = 0;
			t879 = vertPos_atrootVert;
			vertPosI_at_Ivs_atvs = t879;
			_vcmpvertPosI_at_Ivs = t879;
			t87d = tangentFrame_atrootVert;
			_vcmptangentFrameI_at_Ivs = t87d;
			t880 = vertUV0_atrootVert;
			vertUV0I_at_Ivs_atvs = t880;
			_vcmpvertUV0I_at_Ivs = t880;
			v1_inv255 = (2.000000000000e+00 / 2.550000000000e+02);
			t88a = v0_result;
			t88a[0] = ((float((_vcmptangentFrameI_at_Ivs & 255)) * v1_inv255) - 1.000000000000e+00);
			v0_result = t88a;
			t892 = v0_result;
			t892[1] = ((float(((_vcmptangentFrameI_at_Ivs >> 8) & 255)) * v1_inv255) - 1.000000000000e+00);
			v0_result = t892;
			t89a = v0_result;
			t89a[2] = ((float(((_vcmptangentFrameI_at_Ivs >> 16) & 255)) * v1_inv255) - 1.000000000000e+00);
			v0_result = t89a;
			t8a2 = v0_result;
			t8a2[3] = ((float(((_vcmptangentFrameI_at_Ivs >> 24) & 255)) * v1_inv255) - 1.000000000000e+00);
			v0_result = t8a2;
			_vcmpSkinnedVertexI_IMeshVertexI_ItangentFrameQuaternion = v0_result;
			t8a8 = normalize(QuaternionRotate(_vcmpSkinnedVertexI_IMeshVertexI_ItangentFrameQuaternion, vec3(0.000000000000e+00, 1.000000000000e+00, 0.000000000000e+00)));
			vertNormal_atvs = t8a8;
			_vcmpvertNormal = t8a8;
			t8ad = normalize(QuaternionRotate(_vcmpSkinnedVertexI_IMeshVertexI_ItangentFrameQuaternion, vec3(1.000000000000e+00, 0.000000000000e+00, 0.000000000000e+00)));
			vertTangent_atvs = t8ad;
			_vcmpvertTangent = t8ad;
			t8b1 = v2_result;
			t8b1.pos = vec3(0.000000000000e+00);
			v2_result = t8b1;
			t8b5 = v2_result;
			t8b5.normal = vec3(0.000000000000e+00);
			v2_result = t8b5;
			t8b9 = v2_result;
			t8b9.tangent = vec3(0.000000000000e+00);
			v2_result = t8b9;
			v_3i = 0;
			for (;bool((v_3i <= 3)); (v_3i = (v_3i + 1)))
			{
				v4_boneId = ((_vcmpboneIds >> (v_3i * 8)) & 255);
				if (bool((v4_boneId == 255)))
				{
					continue;
				}
				v5_boneWeight = (float(((_vcmpboneWeights >> (v_3i * 8)) & 255)) * (1.000000000000e+00 / 2.550000000000e+02));
				t8d2 = blkskeletalTransformBlock.boneTransforms[v4_boneId];
				v6_tp = (t8d2.transformMatrix * vec4(_vcmpvertPosI_at_Ivs, 1.000000000000e+00)).xyz;
				t8da = v2_result;
				t8da.pos = (v2_result.pos + (v6_tp * v5_boneWeight));
				v2_result = t8da;
				t8dd = blkskeletalTransformBlock.boneTransforms[v4_boneId];
				v6_tp = (t8dd.normalMatrix * _vcmpvertNormal);
				t8e4 = v2_result;
				t8e4.normal = (v2_result.normal + (v6_tp * v5_boneWeight));
				v2_result = t8e4;
				t8e7 = blkskeletalTransformBlock.boneTransforms[v4_boneId];
				v6_tp = (t8e7.normalMatrix * _vcmpvertTangent);
				t8ee = v2_result;
				t8ee.tangent = (v2_result.tangent + (v6_tp * v5_boneWeight));
				v2_result = t8ee;
			}
			_vcmpskinning = v2_result;
			t8f2 = _vcmpskinning.pos;
			coarseVertPos_atvs = t8f2;
			_vcmpcoarseVertPos = t8f2;
			t8f5 = _vcmpskinning.normal;
			coarseVertNormal_atvs = t8f5;
			_vcmpcoarseVertNormal = t8f5;
		}
	}
	fs
	text
	{
		#version 440
		struct BoneTransform
		{
			mat4 transformMatrix;
			mat3 normalMatrix;
		};
		struct SkinningResult
		{
			vec3 pos;
			vec3 normal;
			vec3 tangent;
		};
		vec3 TangentSpaceTransformXWorldSpaceToTangentSpaceXvec3(vec3 t148, vec3 t149, vec3 t14a, vec3 t14b);
		vec3 TangentSpaceTransformXTangentSpaceToWorldSpaceXvec3(vec3 t151, vec3 t152, vec3 t153, vec3 t154);
		vec3 SkinnedVertexXworldTransformPosXvec3(vec3 t633);
		vec3 SkinnedVertexXworldTransformNormalXvec3(vec3 t635);
		vec3 TangentSpaceTransformXWorldSpaceToTangentSpaceXvec3(vec3 t148, vec3 t149, vec3 t14a, vec3 t14b)
		{
			return vec3(dot(t14b, t148), dot(t14b, t149), dot(t14b, t14a));
		}
		vec3 TangentSpaceTransformXTangentSpaceToWorldSpaceXvec3(vec3 t151, vec3 t152, vec3 t153, vec3 t154)
		{
			return (((t154.x * t151) + (t154.y * t152)) + (t154.z * t153));
		}
		vec3 SkinnedVertexXworldTransformPosXvec3(vec3 t633)
		{
			return t633;
		}
		vec3 SkinnedVertexXworldTransformNormalXvec3(vec3 t635)
		{
			return t635;
		}
		layout(std430, binding = 3) buffer skeletalTransformBlock
		{
			BoneTransform[] boneTransforms;
		} blkskeletalTransformBlock;
		layout(binding = 0) uniform sampler2D albedoMap;
		layout(binding = 1) uniform sampler2D normalMap;
		layout(binding = 2) uniform sampler2D displacementMap;
		layout(std140, binding = 1) uniform viewUniformBlock
		{
			mat4 viewTransform;
			mat4 viewProjectionTransform;
			mat4 invViewTransform;
			mat4 invViewProjTransform;
			vec3 cameraPos;
		} blkviewUniformBlock;
		in vec3 vertPosI_at_Ites_attes;
		in vec3 vertNormalI_at_Ites_attes;
		in vec3 vertTangentI_at_Ites_attes;
		in vec3 fineVertPos_attes;
		in vec2 vertUV0I_at_Ites_attes;
		in vec4 projCoord_attes;
		out vec3 outputPbr_atfs;
		out vec3 outputAlbedo_atfs;
		out vec3 outputNormal_atfs;
		void main()
		{
			uint _vcmpboneIds;
			uint _vcmpboneWeights;
			vec3 _vcmpdisplacement;
			float _vcmpSurfacePatternI_Iarg11_pomparallaxScale;
			float _vcmproughness;
			float _vcmpmetallic;
			float _vcmpspecular;
			vec3 t906;
			vec3 _vcmpvertPosI_at_Ifs;
			vec3 t909;
			vec3 _vcmpvertNormalI_at_Ifs;
			vec3 t90c;
			vec3 _vcmpvertTangentI_at_Ifs;
			vec3 t90f;
			vec3 _vcmpfineVertPosI_at_Ifs;
			vec3 t912;
			vec3 _vcmpcameraPosI_at_Ifs;
			vec2 t917;
			vec2 _vcmpvertUV0I_at_Ifs;
			vec3 _vcmplightParam;
			vec3 _vcmpoutputPbr;
			SkinningResult v0_result;
			SkinningResult t920;
			SkinningResult t924;
			SkinningResult t928;
			int v_1i;
			uint v2_boneId;
			float v3_boneWeight;
			vec3 v4_tp;
			BoneTransform t941;
			SkinningResult t949;
			BoneTransform t94c;
			SkinningResult t953;
			BoneTransform t956;
			SkinningResult t95d;
			SkinningResult _vcmpskinning;
			vec3 _vcmpcoarseVertNormal;
			vec3 _vcmpcoarseVertTangent;
			vec3 _vcmpvNormal;
			vec3 _vcmpvTangent;
			vec3 _vcmpvBiTangent;
			vec3 _vcmppos;
			vec3 _vcmpSurfacePatternI_Iarg9_pomviewDirTangentSpace;
			vec3 v5_V;
			vec2 v6_T;
			float v7_parallaxHeight;
			float v8_minLayers;
			float v9_maxLayers;
			float v10_numLayers;
			float v11_layerHeight;
			float v12_curLayerHeight;
			vec2 v13_dtex;
			vec2 t987;
			vec2 v14_currentTextureCoords;
			float v15_heightFromTexture;
			vec4 t98b;
			vec4 t996;
			vec2 v16_deltaTexCoord;
			float v17_deltaHeight;
			int v18_numSearches;
			int v_19i;
			vec4 t9ae;
			vec3 _vcmpSurfacePatternI_IpomI_IparallaxMapping;
			vec2 _vcmppomI_IuvOut;
			vec3 _vcmpalbedo;
			vec3 _vcmpnormal;
			vec3 _vcmpoutputAlbedo;
			vec3 t9cd;
			vec3 _vcmpoutputNormal;
			_vcmpboneIds = 255;
			_vcmpboneWeights = 0;
			_vcmpdisplacement = vec3(0.000000000000e+00);
			_vcmpSurfacePatternI_Iarg11_pomparallaxScale = 1.999999955297e-02;
			_vcmproughness = 5.000000000000e-01;
			_vcmpmetallic = 3.000000119209e-01;
			_vcmpspecular = 1.000000000000e+00;
			t906 = vertPosI_at_Ites_attes;
			_vcmpvertPosI_at_Ifs = t906;
			t909 = vertNormalI_at_Ites_attes;
			_vcmpvertNormalI_at_Ifs = t909;
			t90c = vertTangentI_at_Ites_attes;
			_vcmpvertTangentI_at_Ifs = t90c;
			t90f = fineVertPos_attes;
			_vcmpfineVertPosI_at_Ifs = t90f;
			t912 = blkviewUniformBlock.cameraPos;
			_vcmpcameraPosI_at_Ifs = t912;
			t917 = vertUV0I_at_Ites_attes;
			_vcmpvertUV0I_at_Ifs = t917;
			_vcmplightParam = vec3(_vcmproughness, _vcmpmetallic, _vcmpspecular);
			outputPbr_atfs = _vcmplightParam;
			_vcmpoutputPbr = _vcmplightParam;
			t920 = v0_result;
			t920.pos = vec3(0.000000000000e+00);
			v0_result = t920;
			t924 = v0_result;
			t924.normal = vec3(0.000000000000e+00);
			v0_result = t924;
			t928 = v0_result;
			t928.tangent = vec3(0.000000000000e+00);
			v0_result = t928;
			v_1i = 0;
			for (;bool((v_1i <= 3)); (v_1i = (v_1i + 1)))
			{
				v2_boneId = ((_vcmpboneIds >> (v_1i * 8)) & 255);
				if (bool((v2_boneId == 255)))
				{
					continue;
				}
				v3_boneWeight = (float(((_vcmpboneWeights >> (v_1i * 8)) & 255)) * (1.000000000000e+00 / 2.550000000000e+02));
				t941 = blkskeletalTransformBlock.boneTransforms[v2_boneId];
				v4_tp = (t941.transformMatrix * vec4(_vcmpvertPosI_at_Ifs, 1.000000000000e+00)).xyz;
				t949 = v0_result;
				t949.pos = (v0_result.pos + (v4_tp * v3_boneWeight));
				v0_result = t949;
				t94c = blkskeletalTransformBlock.boneTransforms[v2_boneId];
				v4_tp = (t94c.normalMatrix * _vcmpvertNormalI_at_Ifs);
				t953 = v0_result;
				t953.normal = (v0_result.normal + (v4_tp * v3_boneWeight));
				v0_result = t953;
				t956 = blkskeletalTransformBlock.boneTransforms[v2_boneId];
				v4_tp = (t956.normalMatrix * _vcmpvertTangentI_at_Ifs);
				t95d = v0_result;
				t95d.tangent = (v0_result.tangent + (v4_tp * v3_boneWeight));
				v0_result = t95d;
			}
			_vcmpskinning = v0_result;
			_vcmpcoarseVertNormal = _vcmpskinning.normal;
			_vcmpcoarseVertTangent = _vcmpskinning.tangent;
			_vcmpvNormal = SkinnedVertexXworldTransformNormalXvec3(_vcmpcoarseVertNormal).xyz;
			_vcmpvTangent = SkinnedVertexXworldTransformNormalXvec3(_vcmpcoarseVertTangent).xyz;
			_vcmpvBiTangent = cross(_vcmpvTangent, _vcmpvNormal);
			_vcmppos = SkinnedVertexXworldTransformPosXvec3((_vcmpfineVertPosI_at_Ifs + _vcmpdisplacement));
			_vcmpSurfacePatternI_Iarg9_pomviewDirTangentSpace = TangentSpaceTransformXWorldSpaceToTangentSpaceXvec3(_vcmpvTangent, _vcmpvBiTangent, _vcmpvNormal, normalize((_vcmpcameraPosI_at_Ifs - _vcmppos)));
			v5_V = _vcmpSurfacePatternI_Iarg9_pomviewDirTangentSpace;
			v6_T = _vcmpvertUV0I_at_Ifs;
			v8_minLayers = 10;
			v9_maxLayers = 15;
			v10_numLayers = mix(v9_maxLayers, v8_minLayers, abs(v5_V.z));
			v11_layerHeight = (1.000000000000e+00 / v10_numLayers);
			v12_curLayerHeight = 9.999999776483e-03;
			v13_dtex = (((_vcmpSurfacePatternI_Iarg11_pomparallaxScale * v5_V.xy) / v5_V.z) / v10_numLayers);
			t987 = v13_dtex;
			t987[1] = (-v13_dtex.y);
			v13_dtex = t987;
			v14_currentTextureCoords = v6_T;
			t98b = texture(displacementMap, v14_currentTextureCoords);
			v15_heightFromTexture = (1.000000000000e+00 - t98b.x);
			while (bool((v15_heightFromTexture > v12_curLayerHeight)))
			{
				v12_curLayerHeight = (v12_curLayerHeight + v11_layerHeight);
				v14_currentTextureCoords = (v14_currentTextureCoords - v13_dtex);
				t996 = texture(displacementMap, v14_currentTextureCoords);
				v15_heightFromTexture = (1.000000000000e+00 - t996.x);
			}
			v16_deltaTexCoord = (v13_dtex / 2);
			v17_deltaHeight = (v11_layerHeight / 2);
			v14_currentTextureCoords = (v14_currentTextureCoords + v16_deltaTexCoord);
			v12_curLayerHeight = (v12_curLayerHeight - v17_deltaHeight);
			v18_numSearches = 5;
			v_19i = 0;
			for (;bool((v_19i <= v18_numSearches)); (v_19i = (v_19i + 1)))
			{
				v16_deltaTexCoord = (v16_deltaTexCoord / 2);
				v17_deltaHeight = (v17_deltaHeight / 2);
				t9ae = texture(displacementMap, v14_currentTextureCoords);
				v15_heightFromTexture = (1.000000000000e+00 - t9ae.x);
				if (bool((v15_heightFromTexture > v12_curLayerHeight)))
				{
					v14_currentTextureCoords = (v14_currentTextureCoords - v16_deltaTexCoord);
					v12_curLayerHeight = (v12_curLayerHeight + v17_deltaHeight);
				}
				else
				{
					v14_currentTextureCoords = (v14_currentTextureCoords + v16_deltaTexCoord);
					v12_curLayerHeight = (v12_curLayerHeight - v17_deltaHeight);
				}
			}
			v7_parallaxHeight = v12_curLayerHeight;
			_vcmpSurfacePatternI_IpomI_IparallaxMapping = vec3(v14_currentTextureCoords, v7_parallaxHeight);
			_vcmppomI_IuvOut = _vcmpSurfacePatternI_IpomI_IparallaxMapping.xy;
			_vcmpalbedo = (texture(albedoMap, _vcmppomI_IuvOut).xyz * 6.999999880791e-01);
			_vcmpnormal = normalize(((texture(normalMap, _vcmppomI_IuvOut).xyz * 2.000000000000e+00) - 1.000000000000e+00));
			outputAlbedo_atfs = _vcmpalbedo;
			_vcmpoutputAlbedo = _vcmpalbedo;
			t9cd = TangentSpaceTransformXTangentSpaceToWorldSpaceXvec3(_vcmpvTangent, _vcmpvBiTangent, _vcmpvNormal, _vcmpnormal);
			outputNormal_atfs = t9cd;
			_vcmpoutputNormal = t9cd;
		}
	}
	tcs
	text
	{
		#version 440
		layout(vertices = 1) out;
		struct BoneTransform
		{
			mat4 transformMatrix;
			mat3 normalMatrix;
		};
		struct SkinningResult
		{
			vec3 pos;
			vec3 normal;
			vec3 tangent;
		};
		vec3 PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(vec3 t2a7, vec3 t2a8, vec3 t2a9);
		vec3 PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(vec3 t2a7, vec3 t2a8, vec3 t2a9)
		{
			vec3 v2_v;
			float v3_Len;
			vec3 v4_d;
			v2_v = (t2a7 - t2a8);
			v3_Len = dot(v2_v, t2a9);
			v4_d = (v3_Len * t2a9);
			return (t2a7 - v4_d);
		}
		layout(binding = 0) uniform sampler2D albedoMap;
		layout(binding = 1) uniform sampler2D normalMap;
		layout(binding = 2) uniform sampler2D displacementMap;
		layout(std140, binding = 1) uniform viewUniformBlock
		{
			mat4 viewTransform;
			mat4 viewProjectionTransform;
			mat4 invViewTransform;
			mat4 invViewProjTransform;
			vec3 cameraPos;
		} blkviewUniformBlock;
		in vec3 coarseVertPos_atvs[];
		in vec3 coarseVertNormal_atvs[];
		in vec3 vertPosI_at_Ivs_atvs[];
		in vec3 vertNormal_atvs[];
		in vec3 vertTangent_atvs[];
		in vec2 vertUV0I_at_Ivs_atvs[];
		layout(std430, binding = 3) buffer skeletalTransformBlock
		{
			BoneTransform[] boneTransforms;
		} blkskeletalTransformBlock;
		out vec4 tessLevelOuter_attcs[]; 
		out vec2 tessLevelInner_attcs[]; 
		out vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B300_attcs[]; 
		out vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B030_attcs[]; 
		out vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B003_attcs[]; 
		out vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B210_attcs[]; 
		out vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B120_attcs[]; 
		out vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B201_attcs[]; 
		out vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B021_attcs[]; 
		out vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B102_attcs[]; 
		out vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B012_attcs[]; 
		out vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B111_attcs[]; 
		patch out vec3 vertPosI_at_IperCornerPoint_atperCornerPoint[3]; 
		patch out vec3 vertNormalI_at_IperCornerPoint_atperCornerPoint[3]; 
		patch out vec3 vertTangentI_at_IperCornerPoint_atperCornerPoint[3]; 
		patch out vec2 vertUV0I_at_IperCornerPoint_atperCornerPoint[3]; 
		void main()
		{
			vec4 t9d9;
			vec4 _vcmptessLevelOuter;
			vec2 t9dc;
			vec2 _vcmptessLevelInner;
			vec3 t9df;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B030;
			vec3 t9e4;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B003;
			vec3 t9e9;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B300;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IEdgeB300;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IEdgeB030;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IEdgeB003;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B021t;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B012t;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B102t;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B201t;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B210t;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B120t;
			vec3 ta09;
			vec3 ta0c;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B021;
			vec3 ta0f;
			vec3 ta12;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B012;
			vec3 ta15;
			vec3 ta18;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B102;
			vec3 ta1b;
			vec3 ta1e;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B201;
			vec3 ta21;
			vec3 ta24;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B210;
			vec3 ta27;
			vec3 ta2a;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B120;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_ICenter;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B111t;
			vec3 ta3a;
			vec3 _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B111;
			vec3 ta3f;
			vec3 _vcmpvertPosI_at_IperCornerPoint;
			vec3 ta44;
			vec3 _vcmpvertNormalI_at_IperCornerPoint;
			vec3 ta49;
			vec3 _vcmpvertTangentI_at_IperCornerPoint;
			vec2 ta4e;
			vec2 _vcmpvertUV0I_at_IperCornerPoint;
			t9d9 = vec4(3, 3, 3, 0);
			tessLevelOuter_attcs[gl_InvocationID] = t9d9;
			_vcmptessLevelOuter = t9d9;
			t9dc = vec2(3.000000000000e+00);
			tessLevelInner_attcs[gl_InvocationID] = t9dc;
			_vcmptessLevelInner = t9dc;
			t9df = coarseVertPos_atvs[0];
			SurfaceGeometryI_IPN_TessellationI_IWorldPos_B030_attcs[gl_InvocationID] = t9df;
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B030 = t9df;
			t9e4 = coarseVertPos_atvs[1];
			SurfaceGeometryI_IPN_TessellationI_IWorldPos_B003_attcs[gl_InvocationID] = t9e4;
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B003 = t9e4;
			t9e9 = coarseVertPos_atvs[2];
			SurfaceGeometryI_IPN_TessellationI_IWorldPos_B300_attcs[gl_InvocationID] = t9e9;
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B300 = t9e9;
			_vcmpSurfaceGeometryI_IPN_TessellationI_IEdgeB300 = (_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B003 - _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B030);
			_vcmpSurfaceGeometryI_IPN_TessellationI_IEdgeB030 = (_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B300 - _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B003);
			_vcmpSurfaceGeometryI_IPN_TessellationI_IEdgeB003 = (_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B030 - _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B300);
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B021t = (_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B030 + (_vcmpSurfaceGeometryI_IPN_TessellationI_IEdgeB300 / 3.000000000000e+00));
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B012t = (_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B030 + ((_vcmpSurfaceGeometryI_IPN_TessellationI_IEdgeB300 * 2.000000000000e+00) / 3.000000000000e+00));
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B102t = (_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B003 + (_vcmpSurfaceGeometryI_IPN_TessellationI_IEdgeB030 / 3.000000000000e+00));
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B201t = (_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B003 + ((_vcmpSurfaceGeometryI_IPN_TessellationI_IEdgeB030 * 2.000000000000e+00) / 3.000000000000e+00));
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B210t = (_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B300 + (_vcmpSurfaceGeometryI_IPN_TessellationI_IEdgeB003 / 3.000000000000e+00));
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B120t = (_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B300 + ((_vcmpSurfaceGeometryI_IPN_TessellationI_IEdgeB003 * 2.000000000000e+00) / 3.000000000000e+00));
			ta09 = coarseVertNormal_atvs[0];
			ta0c = PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B021t, _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B030, ta09);
			SurfaceGeometryI_IPN_TessellationI_IWorldPos_B021_attcs[gl_InvocationID] = ta0c;
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B021 = ta0c;
			ta0f = coarseVertNormal_atvs[1];
			ta12 = PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B012t, _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B003, ta0f);
			SurfaceGeometryI_IPN_TessellationI_IWorldPos_B012_attcs[gl_InvocationID] = ta12;
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B012 = ta12;
			ta15 = coarseVertNormal_atvs[1];
			ta18 = PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B102t, _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B003, ta15);
			SurfaceGeometryI_IPN_TessellationI_IWorldPos_B102_attcs[gl_InvocationID] = ta18;
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B102 = ta18;
			ta1b = coarseVertNormal_atvs[2];
			ta1e = PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B201t, _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B300, ta1b);
			SurfaceGeometryI_IPN_TessellationI_IWorldPos_B201_attcs[gl_InvocationID] = ta1e;
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B201 = ta1e;
			ta21 = coarseVertNormal_atvs[2];
			ta24 = PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B210t, _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B300, ta21);
			SurfaceGeometryI_IPN_TessellationI_IWorldPos_B210_attcs[gl_InvocationID] = ta24;
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B210 = ta24;
			ta27 = coarseVertNormal_atvs[0];
			ta2a = PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B120t, _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B030, ta27);
			SurfaceGeometryI_IPN_TessellationI_IWorldPos_B120_attcs[gl_InvocationID] = ta2a;
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B120 = ta2a;
			_vcmpSurfaceGeometryI_IPN_TessellationI_ICenter = (((_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B003 + _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B030) + _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B300) / 3.000000000000e+00);
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B111t = ((((((_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B021 + _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B012) + _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B102) + _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B201) + _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B210) + _vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B120) / 6.000000000000e+00);
			ta3a = (_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B111t + ((_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B111t - _vcmpSurfaceGeometryI_IPN_TessellationI_ICenter) / 2.000000000000e+00));
			SurfaceGeometryI_IPN_TessellationI_IWorldPos_B111_attcs[gl_InvocationID] = ta3a;
			_vcmpSurfaceGeometryI_IPN_TessellationI_IWorldPos_B111 = ta3a;
			for (int sysLocalIterator = 0; sysLocalIterator < gl_PatchVerticesIn; sysLocalIterator++)
			{
				ta3f = vertPosI_at_Ivs_atvs[sysLocalIterator];
				vertPosI_at_IperCornerPoint_atperCornerPoint[sysLocalIterator] = ta3f;
				_vcmpvertPosI_at_IperCornerPoint = ta3f;
				ta44 = vertNormal_atvs[sysLocalIterator];
				vertNormalI_at_IperCornerPoint_atperCornerPoint[sysLocalIterator] = ta44;
				_vcmpvertNormalI_at_IperCornerPoint = ta44;
				ta49 = vertTangent_atvs[sysLocalIterator];
				vertTangentI_at_IperCornerPoint_atperCornerPoint[sysLocalIterator] = ta49;
				_vcmpvertTangentI_at_IperCornerPoint = ta49;
				ta4e = vertUV0I_at_Ivs_atvs[sysLocalIterator];
				vertUV0I_at_IperCornerPoint_atperCornerPoint[sysLocalIterator] = ta4e;
				_vcmpvertUV0I_at_IperCornerPoint = ta4e;
			}
			gl_TessLevelInner[0] = _vcmptessLevelInner[0];
			gl_TessLevelInner[1] = _vcmptessLevelInner[1];
			gl_TessLevelOuter[0] = _vcmptessLevelOuter[0];
			gl_TessLevelOuter[1] = _vcmptessLevelOuter[1];
			gl_TessLevelOuter[2] = _vcmptessLevelOuter[2];
			gl_TessLevelOuter[3] = _vcmptessLevelOuter[3];
		}
	}
	tes
	text
	{
		#version 440
		layout(triangles) in;
		struct BoneTransform
		{
			mat4 transformMatrix;
			mat3 normalMatrix;
		};
		struct SkinningResult
		{
			vec3 pos;
			vec3 normal;
			vec3 tangent;
		};
		vec3 SkinnedVertexXworldTransformPosXvec3(vec3 t633);
		vec3 SkinnedVertexXworldTransformPosXvec3(vec3 t633)
		{
			return t633;
		}
		layout(std430, binding = 3) buffer skeletalTransformBlock
		{
			BoneTransform[] boneTransforms;
		} blkskeletalTransformBlock;
		layout(binding = 0) uniform sampler2D albedoMap;
		layout(binding = 1) uniform sampler2D normalMap;
		layout(binding = 2) uniform sampler2D displacementMap;
		layout(std140, binding = 1) uniform viewUniformBlock
		{
			mat4 viewTransform;
			mat4 viewProjectionTransform;
			mat4 invViewTransform;
			mat4 invViewProjTransform;
			vec3 cameraPos;
		} blkviewUniformBlock;
		in vec4 tessLevelOuter_attcs[];
		in vec2 tessLevelInner_attcs[];
		in vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B300_attcs[];
		in vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B030_attcs[];
		in vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B003_attcs[];
		in vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B210_attcs[];
		in vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B120_attcs[];
		in vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B201_attcs[];
		in vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B021_attcs[];
		in vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B102_attcs[];
		in vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B012_attcs[];
		in vec3 SurfaceGeometryI_IPN_TessellationI_IWorldPos_B111_attcs[];
		patch in vec3 vertPosI_at_IperCornerPoint_atperCornerPoint[3];
		patch in vec3 vertNormalI_at_IperCornerPoint_atperCornerPoint[3];
		patch in vec3 vertTangentI_at_IperCornerPoint_atperCornerPoint[3];
		patch in vec2 vertUV0I_at_IperCornerPoint_atperCornerPoint[3];
		out vec3 vertPosI_at_Ites_attes;
		out vec3 vertNormalI_at_Ites_attes;
		out vec3 vertTangentI_at_Ites_attes;
		out vec3 fineVertPos_attes;
		out vec2 vertUV0I_at_Ites_attes;
		out vec4 projCoord_attes;
		void main()
		{
			vec3 _vcmpdisplacement;
			float _vcmpSurfaceGeometryI_IPN_TessellationI_Iu;
			float _vcmpSurfaceGeometryI_IPN_TessellationI_Iv;
			float _vcmpSurfaceGeometryI_IPN_TessellationI_Iw;
			float _vcmpSurfaceGeometryI_IPN_TessellationI_IvPow2;
			float _vcmpSurfaceGeometryI_IPN_TessellationI_IuPow2;
			float _vcmpSurfaceGeometryI_IPN_TessellationI_IwPow2;
			float _vcmpSurfaceGeometryI_IPN_TessellationI_IuPow3;
			float _vcmpSurfaceGeometryI_IPN_TessellationI_IvPow3;
			float _vcmpSurfaceGeometryI_IPN_TessellationI_IwPow3;
			vec3 ta6f;
			vec3 ta76;
			vec3 ta7c;
			vec3 ta82;
			vec3 ta88;
			vec3 ta8e;
			vec3 ta94;
			vec3 ta9a;
			vec3 ta9e;
			vec3 taa2;
			vec3 taae;
			vec3 _vcmpfineVertPos;
			vec3 _vcmppos;
			vec3 tab4;
			vec3 _vcmpvertPosI_at_Ites;
			vec3 tac3;
			vec3 _vcmpvertNormalI_at_Ites;
			vec3 tad2;
			vec3 _vcmpvertTangentI_at_Ites;
			mat4 tae1;
			mat4 _vcmpviewProjectionTransformI_at_Ites;
			vec2 tae4;
			vec2 _vcmpvertUV0I_at_Ites;
			vec4 taf4;
			vec4 _vcmpprojCoord;
			_vcmpdisplacement = vec3(0.000000000000e+00);
			_vcmpSurfaceGeometryI_IPN_TessellationI_Iu = gl_TessCoord.x;
			_vcmpSurfaceGeometryI_IPN_TessellationI_Iv = gl_TessCoord.y;
			_vcmpSurfaceGeometryI_IPN_TessellationI_Iw = gl_TessCoord.z;
			_vcmpSurfaceGeometryI_IPN_TessellationI_IvPow2 = (_vcmpSurfaceGeometryI_IPN_TessellationI_Iv * _vcmpSurfaceGeometryI_IPN_TessellationI_Iv);
			_vcmpSurfaceGeometryI_IPN_TessellationI_IuPow2 = (_vcmpSurfaceGeometryI_IPN_TessellationI_Iu * _vcmpSurfaceGeometryI_IPN_TessellationI_Iu);
			_vcmpSurfaceGeometryI_IPN_TessellationI_IwPow2 = (_vcmpSurfaceGeometryI_IPN_TessellationI_Iw * _vcmpSurfaceGeometryI_IPN_TessellationI_Iw);
			_vcmpSurfaceGeometryI_IPN_TessellationI_IuPow3 = (_vcmpSurfaceGeometryI_IPN_TessellationI_IuPow2 * _vcmpSurfaceGeometryI_IPN_TessellationI_Iu);
			_vcmpSurfaceGeometryI_IPN_TessellationI_IvPow3 = (_vcmpSurfaceGeometryI_IPN_TessellationI_IvPow2 * _vcmpSurfaceGeometryI_IPN_TessellationI_Iv);
			_vcmpSurfaceGeometryI_IPN_TessellationI_IwPow3 = (_vcmpSurfaceGeometryI_IPN_TessellationI_IwPow2 * _vcmpSurfaceGeometryI_IPN_TessellationI_Iw);
			ta6f = SurfaceGeometryI_IPN_TessellationI_IWorldPos_B111_attcs[0];
			ta76 = SurfaceGeometryI_IPN_TessellationI_IWorldPos_B012_attcs[0];
			ta7c = SurfaceGeometryI_IPN_TessellationI_IWorldPos_B102_attcs[0];
			ta82 = SurfaceGeometryI_IPN_TessellationI_IWorldPos_B021_attcs[0];
			ta88 = SurfaceGeometryI_IPN_TessellationI_IWorldPos_B201_attcs[0];
			ta8e = SurfaceGeometryI_IPN_TessellationI_IWorldPos_B120_attcs[0];
			ta94 = SurfaceGeometryI_IPN_TessellationI_IWorldPos_B210_attcs[0];
			ta9a = SurfaceGeometryI_IPN_TessellationI_IWorldPos_B003_attcs[0];
			ta9e = SurfaceGeometryI_IPN_TessellationI_IWorldPos_B030_attcs[0];
			taa2 = SurfaceGeometryI_IPN_TessellationI_IWorldPos_B300_attcs[0];
			taae = ((((((((((taa2 * _vcmpSurfaceGeometryI_IPN_TessellationI_IwPow3) + (ta9e * _vcmpSurfaceGeometryI_IPN_TessellationI_IuPow3)) + (ta9a * _vcmpSurfaceGeometryI_IPN_TessellationI_IvPow3)) + (((ta94 * 3.000000000000e+00) * _vcmpSurfaceGeometryI_IPN_TessellationI_IwPow2) * _vcmpSurfaceGeometryI_IPN_TessellationI_Iu)) + (((ta8e * 3.000000000000e+00) * _vcmpSurfaceGeometryI_IPN_TessellationI_Iw) * _vcmpSurfaceGeometryI_IPN_TessellationI_IuPow2)) + (((ta88 * 3.000000000000e+00) * _vcmpSurfaceGeometryI_IPN_TessellationI_IwPow2) * _vcmpSurfaceGeometryI_IPN_TessellationI_Iv)) + (((ta82 * 3.000000000000e+00) * _vcmpSurfaceGeometryI_IPN_TessellationI_IuPow2) * _vcmpSurfaceGeometryI_IPN_TessellationI_Iv)) + (((ta7c * 3.000000000000e+00) * _vcmpSurfaceGeometryI_IPN_TessellationI_Iw) * _vcmpSurfaceGeometryI_IPN_TessellationI_IvPow2)) + (((ta76 * 3.000000000000e+00) * _vcmpSurfaceGeometryI_IPN_TessellationI_Iu) * _vcmpSurfaceGeometryI_IPN_TessellationI_IvPow2)) + ((((ta6f * 6.000000000000e+00) * _vcmpSurfaceGeometryI_IPN_TessellationI_Iw) * _vcmpSurfaceGeometryI_IPN_TessellationI_Iu) * _vcmpSurfaceGeometryI_IPN_TessellationI_Iv));
			fineVertPos_attes = taae;
			_vcmpfineVertPos = taae;
			_vcmppos = SkinnedVertexXworldTransformPosXvec3((_vcmpfineVertPos + _vcmpdisplacement));
			tab4 = (((vertPosI_at_IperCornerPoint_atperCornerPoint[0] * gl_TessCoord.x) + (vertPosI_at_IperCornerPoint_atperCornerPoint[1] * gl_TessCoord.y)) + (vertPosI_at_IperCornerPoint_atperCornerPoint[2] * gl_TessCoord.z));
			vertPosI_at_Ites_attes = tab4;
			_vcmpvertPosI_at_Ites = tab4;
			tac3 = (((vertNormalI_at_IperCornerPoint_atperCornerPoint[0] * gl_TessCoord.x) + (vertNormalI_at_IperCornerPoint_atperCornerPoint[1] * gl_TessCoord.y)) + (vertNormalI_at_IperCornerPoint_atperCornerPoint[2] * gl_TessCoord.z));
			vertNormalI_at_Ites_attes = tac3;
			_vcmpvertNormalI_at_Ites = tac3;
			tad2 = (((vertTangentI_at_IperCornerPoint_atperCornerPoint[0] * gl_TessCoord.x) + (vertTangentI_at_IperCornerPoint_atperCornerPoint[1] * gl_TessCoord.y)) + (vertTangentI_at_IperCornerPoint_atperCornerPoint[2] * gl_TessCoord.z));
			vertTangentI_at_Ites_attes = tad2;
			_vcmpvertTangentI_at_Ites = tad2;
			tae1 = blkviewUniformBlock.viewProjectionTransform;
			_vcmpviewProjectionTransformI_at_Ites = tae1;
			tae4 = (((vertUV0I_at_IperCornerPoint_atperCornerPoint[0] * gl_TessCoord.x) + (vertUV0I_at_IperCornerPoint_atperCornerPoint[1] * gl_TessCoord.y)) + (vertUV0I_at_IperCornerPoint_atperCornerPoint[2] * gl_TessCoord.z));
			vertUV0I_at_Ites_attes = tae4;
			_vcmpvertUV0I_at_Ites = tae4;
			taf4 = (_vcmpviewProjectionTransformI_at_Ites * vec4(_vcmppos, 1));
			projCoord_attes = taf4;
			_vcmpprojCoord = taf4;
			gl_Position = _vcmpprojCoord;
		}
	}
}
