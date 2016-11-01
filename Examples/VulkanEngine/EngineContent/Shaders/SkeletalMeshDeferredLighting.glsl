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
			vec4 t1e;
			vec4 t30;
			vec4 t42;
			vec4 t54;
			t1e = v0_rs;
			t1e[0] = ((((p_q1.w * p_q2.x) + (p_q1.x * p_q2.w)) + (p_q1.y * p_q2.z)) - (p_q1.z * p_q2.y));
			v0_rs = t1e;
			t30 = v0_rs;
			t30[1] = ((((p_q1.w * p_q2.y) + (p_q1.y * p_q2.w)) + (p_q1.z * p_q2.x)) - (p_q1.x * p_q2.z));
			v0_rs = t30;
			t42 = v0_rs;
			t42[2] = ((((p_q1.w * p_q2.z) + (p_q1.z * p_q2.w)) + (p_q1.x * p_q2.y)) - (p_q1.y * p_q2.x));
			v0_rs = t42;
			t54 = v0_rs;
			t54[3] = ((((p_q1.w * p_q2.w) - (p_q1.x * p_q2.x)) - (p_q1.y * p_q2.y)) - (p_q1.z * p_q2.z));
			v0_rs = t54;
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
		layout(std430, binding = 3) buffer SkeletonDataBlock
		{
			BoneTransform[] boneTransforms;
		} blkSkeletonDataBlock;
		layout(binding = 0) uniform sampler2D albedoMap;
		layout(binding = 1) uniform sampler2D normalMap;
		layout(binding = 2) uniform sampler2D displacementMap;
		layout(std140, binding = 1) uniform ViewUniformBlock
		{
			mat4 viewTransform;
			mat4 viewProjectionTransform;
			mat4 invViewTransform;
			mat4 invViewProjTransform;
			vec3 cameraPos;
		} blkViewUniformBlock;
		layout(location = 0) in vec3 vertPos_atMeshVertex;
		layout(location = 1) in vec2 vertUV0_atMeshVertex;
		layout(location = 2) in uint tangentFrame_atMeshVertex;
		out vec3 coarseVertPos_atCoarseVertex;
		out vec3 coarseVertNormal_atCoarseVertex;
		out vec3 coarseVertTangent_atCoarseVertex;
		out vec2 vertUV0I_at_ICoarseVertex_atCoarseVertex;
		void main()
		{
			uint _vcmpboneIds;
			uint _vcmpboneWeights;
			vec3 t810;
			vec3 _vcmpvertPosI_at_ICoarseVertex;
			uint t813;
			uint _vcmptangentFrameI_at_ICoarseVertex;
			vec2 t816;
			vec2 _vcmpvertUV0I_at_ICoarseVertex;
			vec4 v0_result;
			float v1_inv255;
			vec4 t820;
			vec4 t828;
			vec4 t830;
			vec4 t838;
			vec4 _vcmpSkeletalAnimationI_IVertexAttributesI_ItangentFrameQuaternion;
			vec3 _vcmpvertNormal;
			vec3 _vcmpvertTangent;
			SkinningResult v2_result;
			SkinningResult t845;
			SkinningResult t849;
			SkinningResult t84d;
			int v_3i;
			uint v4_boneId;
			float v5_boneWeight;
			vec3 v6_tp;
			BoneTransform t866;
			SkinningResult t86e;
			BoneTransform t871;
			SkinningResult t878;
			BoneTransform t87b;
			SkinningResult t882;
			SkinningResult _vcmpskinning;
			vec3 t886;
			vec3 _vcmpcoarseVertPos;
			vec3 t889;
			vec3 _vcmpcoarseVertNormal;
			vec3 t88c;
			vec3 _vcmpcoarseVertTangent;
			_vcmpboneIds = 255;
			_vcmpboneWeights = 0;
			t810 = vertPos_atMeshVertex;
			_vcmpvertPosI_at_ICoarseVertex = t810;
			t813 = tangentFrame_atMeshVertex;
			_vcmptangentFrameI_at_ICoarseVertex = t813;
			t816 = vertUV0_atMeshVertex;
			vertUV0I_at_ICoarseVertex_atCoarseVertex = t816;
			_vcmpvertUV0I_at_ICoarseVertex = t816;
			v1_inv255 = (2.000000000000e+00 / 2.550000000000e+02);
			t820 = v0_result;
			t820[0] = ((float((_vcmptangentFrameI_at_ICoarseVertex & 255)) * v1_inv255) - 1.000000000000e+00);
			v0_result = t820;
			t828 = v0_result;
			t828[1] = ((float(((_vcmptangentFrameI_at_ICoarseVertex >> 8) & 255)) * v1_inv255) - 1.000000000000e+00);
			v0_result = t828;
			t830 = v0_result;
			t830[2] = ((float(((_vcmptangentFrameI_at_ICoarseVertex >> 16) & 255)) * v1_inv255) - 1.000000000000e+00);
			v0_result = t830;
			t838 = v0_result;
			t838[3] = ((float(((_vcmptangentFrameI_at_ICoarseVertex >> 24) & 255)) * v1_inv255) - 1.000000000000e+00);
			v0_result = t838;
			_vcmpSkeletalAnimationI_IVertexAttributesI_ItangentFrameQuaternion = v0_result;
			_vcmpvertNormal = normalize(QuaternionRotate(_vcmpSkeletalAnimationI_IVertexAttributesI_ItangentFrameQuaternion, vec3(0.000000000000e+00, 1.000000000000e+00, 0.000000000000e+00)));
			_vcmpvertTangent = normalize(QuaternionRotate(_vcmpSkeletalAnimationI_IVertexAttributesI_ItangentFrameQuaternion, vec3(1.000000000000e+00, 0.000000000000e+00, 0.000000000000e+00)));
			t845 = v2_result;
			t845.pos = vec3(0.000000000000e+00);
			v2_result = t845;
			t849 = v2_result;
			t849.normal = vec3(0.000000000000e+00);
			v2_result = t849;
			t84d = v2_result;
			t84d.tangent = vec3(0.000000000000e+00);
			v2_result = t84d;
			v_3i = 0;
			for (;bool((v_3i <= 3)); (v_3i = (v_3i + 1)))
			{
				v4_boneId = ((_vcmpboneIds >> (v_3i * 8)) & 255);
				if (bool((v4_boneId == 255)))
				{
					continue;
				}
				v5_boneWeight = (float(((_vcmpboneWeights >> (v_3i * 8)) & 255)) * (1.000000000000e+00 / 2.550000000000e+02));
				t866 = blkSkeletonDataBlock.boneTransforms[v4_boneId];
				v6_tp = (t866.transformMatrix * vec4(_vcmpvertPosI_at_ICoarseVertex, 1.000000000000e+00)).xyz;
				t86e = v2_result;
				t86e.pos = (v2_result.pos + (v6_tp * v5_boneWeight));
				v2_result = t86e;
				t871 = blkSkeletonDataBlock.boneTransforms[v4_boneId];
				v6_tp = (t871.normalMatrix * _vcmpvertNormal);
				t878 = v2_result;
				t878.normal = (v2_result.normal + (v6_tp * v5_boneWeight));
				v2_result = t878;
				t87b = blkSkeletonDataBlock.boneTransforms[v4_boneId];
				v6_tp = (t87b.normalMatrix * _vcmpvertTangent);
				t882 = v2_result;
				t882.tangent = (v2_result.tangent + (v6_tp * v5_boneWeight));
				v2_result = t882;
			}
			_vcmpskinning = v2_result;
			t886 = _vcmpskinning.pos;
			coarseVertPos_atCoarseVertex = t886;
			_vcmpcoarseVertPos = t886;
			t889 = _vcmpskinning.normal;
			coarseVertNormal_atCoarseVertex = t889;
			_vcmpcoarseVertNormal = t889;
			t88c = _vcmpskinning.tangent;
			coarseVertTangent_atCoarseVertex = t88c;
			_vcmpcoarseVertTangent = t88c;
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
		vec3 PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(vec3 t13c, vec3 t13d, vec3 t13e);
		vec3 PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(vec3 t13c, vec3 t13d, vec3 t13e)
		{
			vec3 v2_v;
			float v3_Len;
			vec3 v4_d;
			v2_v = (t13c - t13d);
			v3_Len = dot(v2_v, t13e);
			v4_d = (v3_Len * t13e);
			return (t13c - v4_d);
		}
		layout(binding = 0) uniform sampler2D albedoMap;
		layout(binding = 1) uniform sampler2D normalMap;
		layout(binding = 2) uniform sampler2D displacementMap;
		layout(std140, binding = 1) uniform ViewUniformBlock
		{
			mat4 viewTransform;
			mat4 viewProjectionTransform;
			mat4 invViewTransform;
			mat4 invViewProjTransform;
			vec3 cameraPos;
		} blkViewUniformBlock;
		in vec3 coarseVertPos_atCoarseVertex[];
		in vec3 coarseVertNormal_atCoarseVertex[];
		in vec3 coarseVertTangent_atCoarseVertex[];
		in vec2 vertUV0I_at_ICoarseVertex_atCoarseVertex[];
		out vec4 tessLevelOuter_atControlPoint[]; 
		out vec2 tessLevelInner_atControlPoint[]; 
		out vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B300_atControlPoint[]; 
		out vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B030_atControlPoint[]; 
		out vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B003_atControlPoint[]; 
		out vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B210_atControlPoint[]; 
		out vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B120_atControlPoint[]; 
		out vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B201_atControlPoint[]; 
		out vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B021_atControlPoint[]; 
		out vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B102_atControlPoint[]; 
		out vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B012_atControlPoint[]; 
		out vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B111_atControlPoint[]; 
		patch out vec3 coarseVertNormalI_at_ICornerPoint_atCornerPoint[3]; 
		patch out vec3 coarseVertTangentI_at_ICornerPoint_atCornerPoint[3]; 
		patch out vec2 vertUV0I_at_ICornerPoint_atCornerPoint[3]; 
		void main()
		{
			vec4 t897;
			vec4 _vcmptessLevelOuter;
			vec2 t89a;
			vec2 _vcmptessLevelInner;
			vec3 t89d;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B030;
			vec3 t8a2;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B003;
			vec3 t8a7;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B300;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IEdgeB300;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IEdgeB030;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IEdgeB003;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B021t;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B012t;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B102t;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B201t;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B210t;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B120t;
			vec3 t8c7;
			vec3 t8ca;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B021;
			vec3 t8cd;
			vec3 t8d0;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B012;
			vec3 t8d3;
			vec3 t8d6;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B102;
			vec3 t8d9;
			vec3 t8dc;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B201;
			vec3 t8df;
			vec3 t8e2;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B210;
			vec3 t8e5;
			vec3 t8e8;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B120;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_ICenter;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B111t;
			vec3 t8f8;
			vec3 _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B111;
			vec3 t8fd;
			vec3 _vcmpcoarseVertNormalI_at_ICornerPoint;
			vec3 t902;
			vec3 _vcmpcoarseVertTangentI_at_ICornerPoint;
			vec2 t907;
			vec2 _vcmpvertUV0I_at_ICornerPoint;
			t897 = vec4(3, 3, 3, 0);
			tessLevelOuter_atControlPoint[gl_InvocationID] = t897;
			_vcmptessLevelOuter = t897;
			t89a = vec2(3.000000000000e+00);
			tessLevelInner_atControlPoint[gl_InvocationID] = t89a;
			_vcmptessLevelInner = t89a;
			t89d = coarseVertPos_atCoarseVertex[0];
			MaterialGeometryI_IPN_TessellationI_IWorldPos_B030_atControlPoint[gl_InvocationID] = t89d;
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B030 = t89d;
			t8a2 = coarseVertPos_atCoarseVertex[1];
			MaterialGeometryI_IPN_TessellationI_IWorldPos_B003_atControlPoint[gl_InvocationID] = t8a2;
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B003 = t8a2;
			t8a7 = coarseVertPos_atCoarseVertex[2];
			MaterialGeometryI_IPN_TessellationI_IWorldPos_B300_atControlPoint[gl_InvocationID] = t8a7;
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B300 = t8a7;
			_vcmpMaterialGeometryI_IPN_TessellationI_IEdgeB300 = (_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B003 - _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B030);
			_vcmpMaterialGeometryI_IPN_TessellationI_IEdgeB030 = (_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B300 - _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B003);
			_vcmpMaterialGeometryI_IPN_TessellationI_IEdgeB003 = (_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B030 - _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B300);
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B021t = (_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B030 + (_vcmpMaterialGeometryI_IPN_TessellationI_IEdgeB300 / 3.000000000000e+00));
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B012t = (_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B030 + ((_vcmpMaterialGeometryI_IPN_TessellationI_IEdgeB300 * 2.000000000000e+00) / 3.000000000000e+00));
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B102t = (_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B003 + (_vcmpMaterialGeometryI_IPN_TessellationI_IEdgeB030 / 3.000000000000e+00));
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B201t = (_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B003 + ((_vcmpMaterialGeometryI_IPN_TessellationI_IEdgeB030 * 2.000000000000e+00) / 3.000000000000e+00));
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B210t = (_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B300 + (_vcmpMaterialGeometryI_IPN_TessellationI_IEdgeB003 / 3.000000000000e+00));
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B120t = (_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B300 + ((_vcmpMaterialGeometryI_IPN_TessellationI_IEdgeB003 * 2.000000000000e+00) / 3.000000000000e+00));
			t8c7 = coarseVertNormal_atCoarseVertex[0];
			t8ca = PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B021t, _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B030, t8c7);
			MaterialGeometryI_IPN_TessellationI_IWorldPos_B021_atControlPoint[gl_InvocationID] = t8ca;
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B021 = t8ca;
			t8cd = coarseVertNormal_atCoarseVertex[1];
			t8d0 = PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B012t, _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B003, t8cd);
			MaterialGeometryI_IPN_TessellationI_IWorldPos_B012_atControlPoint[gl_InvocationID] = t8d0;
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B012 = t8d0;
			t8d3 = coarseVertNormal_atCoarseVertex[1];
			t8d6 = PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B102t, _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B003, t8d3);
			MaterialGeometryI_IPN_TessellationI_IWorldPos_B102_atControlPoint[gl_InvocationID] = t8d6;
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B102 = t8d6;
			t8d9 = coarseVertNormal_atCoarseVertex[2];
			t8dc = PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B201t, _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B300, t8d9);
			MaterialGeometryI_IPN_TessellationI_IWorldPos_B201_atControlPoint[gl_InvocationID] = t8dc;
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B201 = t8dc;
			t8df = coarseVertNormal_atCoarseVertex[2];
			t8e2 = PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B210t, _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B300, t8df);
			MaterialGeometryI_IPN_TessellationI_IWorldPos_B210_atControlPoint[gl_InvocationID] = t8e2;
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B210 = t8e2;
			t8e5 = coarseVertNormal_atCoarseVertex[0];
			t8e8 = PNXTessellationXProjectToPlaneXvec3Xvec3Xvec3(_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B120t, _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B030, t8e5);
			MaterialGeometryI_IPN_TessellationI_IWorldPos_B120_atControlPoint[gl_InvocationID] = t8e8;
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B120 = t8e8;
			_vcmpMaterialGeometryI_IPN_TessellationI_ICenter = (((_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B003 + _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B030) + _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B300) / 3.000000000000e+00);
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B111t = ((((((_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B021 + _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B012) + _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B102) + _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B201) + _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B210) + _vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B120) / 6.000000000000e+00);
			t8f8 = (_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B111t + ((_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B111t - _vcmpMaterialGeometryI_IPN_TessellationI_ICenter) / 2.000000000000e+00));
			MaterialGeometryI_IPN_TessellationI_IWorldPos_B111_atControlPoint[gl_InvocationID] = t8f8;
			_vcmpMaterialGeometryI_IPN_TessellationI_IWorldPos_B111 = t8f8;
			for (int sysLocalIterator = 0; sysLocalIterator < gl_PatchVerticesIn; sysLocalIterator++)
			{
				t8fd = coarseVertNormal_atCoarseVertex[sysLocalIterator];
				coarseVertNormalI_at_ICornerPoint_atCornerPoint[sysLocalIterator] = t8fd;
				_vcmpcoarseVertNormalI_at_ICornerPoint = t8fd;
				t902 = coarseVertTangent_atCoarseVertex[sysLocalIterator];
				coarseVertTangentI_at_ICornerPoint_atCornerPoint[sysLocalIterator] = t902;
				_vcmpcoarseVertTangentI_at_ICornerPoint = t902;
				t907 = vertUV0I_at_ICoarseVertex_atCoarseVertex[sysLocalIterator];
				vertUV0I_at_ICornerPoint_atCornerPoint[sysLocalIterator] = t907;
				_vcmpvertUV0I_at_ICornerPoint = t907;
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
		vec3 SkeletalAnimationXworldTransformPosXvec3(vec3 t6aa);
		vec3 SkeletalAnimationXworldTransformPosXvec3(vec3 t6aa)
		{
			return t6aa;
		}
		layout(binding = 0) uniform sampler2D albedoMap;
		layout(binding = 1) uniform sampler2D normalMap;
		layout(binding = 2) uniform sampler2D displacementMap;
		layout(std140, binding = 1) uniform ViewUniformBlock
		{
			mat4 viewTransform;
			mat4 viewProjectionTransform;
			mat4 invViewTransform;
			mat4 invViewProjTransform;
			vec3 cameraPos;
		} blkViewUniformBlock;
		in vec4 tessLevelOuter_atControlPoint[];
		in vec2 tessLevelInner_atControlPoint[];
		in vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B300_atControlPoint[];
		in vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B030_atControlPoint[];
		in vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B003_atControlPoint[];
		in vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B210_atControlPoint[];
		in vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B120_atControlPoint[];
		in vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B201_atControlPoint[];
		in vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B021_atControlPoint[];
		in vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B102_atControlPoint[];
		in vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B012_atControlPoint[];
		in vec3 MaterialGeometryI_IPN_TessellationI_IWorldPos_B111_atControlPoint[];
		patch in vec3 coarseVertNormalI_at_ICornerPoint_atCornerPoint[3];
		patch in vec3 coarseVertTangentI_at_ICornerPoint_atCornerPoint[3];
		patch in vec2 vertUV0I_at_ICornerPoint_atCornerPoint[3];
		out vec4 projCoord_atFineVertex;
		out vec3 coarseVertNormalI_at_IFineVertex_atFineVertex;
		out vec3 coarseVertTangentI_at_IFineVertex_atFineVertex;
		out vec3 fineVertPos_atFineVertex;
		out vec2 vertUV0I_at_IFineVertex_atFineVertex;
		void main()
		{
			vec3 _vcmpdisplacement;
			float _vcmpMaterialGeometryI_IPN_TessellationI_Iu;
			float _vcmpMaterialGeometryI_IPN_TessellationI_Iv;
			float _vcmpMaterialGeometryI_IPN_TessellationI_Iw;
			float _vcmpMaterialGeometryI_IPN_TessellationI_IvPow2;
			float _vcmpMaterialGeometryI_IPN_TessellationI_IuPow2;
			float _vcmpMaterialGeometryI_IPN_TessellationI_IwPow2;
			float _vcmpMaterialGeometryI_IPN_TessellationI_IuPow3;
			float _vcmpMaterialGeometryI_IPN_TessellationI_IvPow3;
			float _vcmpMaterialGeometryI_IPN_TessellationI_IwPow3;
			vec3 t927;
			vec3 t92e;
			vec3 t934;
			vec3 t93a;
			vec3 t940;
			vec3 t946;
			vec3 t94c;
			vec3 t952;
			vec3 t956;
			vec3 t95a;
			vec3 t966;
			vec3 _vcmpfineVertPos;
			vec3 _vcmppos;
			vec3 t96c;
			vec3 _vcmpcoarseVertNormalI_at_IFineVertex;
			vec3 t97b;
			vec3 _vcmpcoarseVertTangentI_at_IFineVertex;
			mat4 t98a;
			mat4 _vcmpviewProjectionTransformI_at_IFineVertex;
			vec2 t98d;
			vec2 _vcmpvertUV0I_at_IFineVertex;
			vec4 t99d;
			vec4 _vcmpprojCoord;
			_vcmpdisplacement = vec3(0.000000000000e+00);
			_vcmpMaterialGeometryI_IPN_TessellationI_Iu = gl_TessCoord.x;
			_vcmpMaterialGeometryI_IPN_TessellationI_Iv = gl_TessCoord.y;
			_vcmpMaterialGeometryI_IPN_TessellationI_Iw = gl_TessCoord.z;
			_vcmpMaterialGeometryI_IPN_TessellationI_IvPow2 = (_vcmpMaterialGeometryI_IPN_TessellationI_Iv * _vcmpMaterialGeometryI_IPN_TessellationI_Iv);
			_vcmpMaterialGeometryI_IPN_TessellationI_IuPow2 = (_vcmpMaterialGeometryI_IPN_TessellationI_Iu * _vcmpMaterialGeometryI_IPN_TessellationI_Iu);
			_vcmpMaterialGeometryI_IPN_TessellationI_IwPow2 = (_vcmpMaterialGeometryI_IPN_TessellationI_Iw * _vcmpMaterialGeometryI_IPN_TessellationI_Iw);
			_vcmpMaterialGeometryI_IPN_TessellationI_IuPow3 = (_vcmpMaterialGeometryI_IPN_TessellationI_IuPow2 * _vcmpMaterialGeometryI_IPN_TessellationI_Iu);
			_vcmpMaterialGeometryI_IPN_TessellationI_IvPow3 = (_vcmpMaterialGeometryI_IPN_TessellationI_IvPow2 * _vcmpMaterialGeometryI_IPN_TessellationI_Iv);
			_vcmpMaterialGeometryI_IPN_TessellationI_IwPow3 = (_vcmpMaterialGeometryI_IPN_TessellationI_IwPow2 * _vcmpMaterialGeometryI_IPN_TessellationI_Iw);
			t927 = MaterialGeometryI_IPN_TessellationI_IWorldPos_B111_atControlPoint[0];
			t92e = MaterialGeometryI_IPN_TessellationI_IWorldPos_B012_atControlPoint[0];
			t934 = MaterialGeometryI_IPN_TessellationI_IWorldPos_B102_atControlPoint[0];
			t93a = MaterialGeometryI_IPN_TessellationI_IWorldPos_B021_atControlPoint[0];
			t940 = MaterialGeometryI_IPN_TessellationI_IWorldPos_B201_atControlPoint[0];
			t946 = MaterialGeometryI_IPN_TessellationI_IWorldPos_B120_atControlPoint[0];
			t94c = MaterialGeometryI_IPN_TessellationI_IWorldPos_B210_atControlPoint[0];
			t952 = MaterialGeometryI_IPN_TessellationI_IWorldPos_B003_atControlPoint[0];
			t956 = MaterialGeometryI_IPN_TessellationI_IWorldPos_B030_atControlPoint[0];
			t95a = MaterialGeometryI_IPN_TessellationI_IWorldPos_B300_atControlPoint[0];
			t966 = ((((((((((t95a * _vcmpMaterialGeometryI_IPN_TessellationI_IwPow3) + (t956 * _vcmpMaterialGeometryI_IPN_TessellationI_IuPow3)) + (t952 * _vcmpMaterialGeometryI_IPN_TessellationI_IvPow3)) + (((t94c * 3.000000000000e+00) * _vcmpMaterialGeometryI_IPN_TessellationI_IwPow2) * _vcmpMaterialGeometryI_IPN_TessellationI_Iu)) + (((t946 * 3.000000000000e+00) * _vcmpMaterialGeometryI_IPN_TessellationI_Iw) * _vcmpMaterialGeometryI_IPN_TessellationI_IuPow2)) + (((t940 * 3.000000000000e+00) * _vcmpMaterialGeometryI_IPN_TessellationI_IwPow2) * _vcmpMaterialGeometryI_IPN_TessellationI_Iv)) + (((t93a * 3.000000000000e+00) * _vcmpMaterialGeometryI_IPN_TessellationI_IuPow2) * _vcmpMaterialGeometryI_IPN_TessellationI_Iv)) + (((t934 * 3.000000000000e+00) * _vcmpMaterialGeometryI_IPN_TessellationI_Iw) * _vcmpMaterialGeometryI_IPN_TessellationI_IvPow2)) + (((t92e * 3.000000000000e+00) * _vcmpMaterialGeometryI_IPN_TessellationI_Iu) * _vcmpMaterialGeometryI_IPN_TessellationI_IvPow2)) + ((((t927 * 6.000000000000e+00) * _vcmpMaterialGeometryI_IPN_TessellationI_Iw) * _vcmpMaterialGeometryI_IPN_TessellationI_Iu) * _vcmpMaterialGeometryI_IPN_TessellationI_Iv));
			fineVertPos_atFineVertex = t966;
			_vcmpfineVertPos = t966;
			_vcmppos = SkeletalAnimationXworldTransformPosXvec3((_vcmpfineVertPos + _vcmpdisplacement));
			t96c = (((coarseVertNormalI_at_ICornerPoint_atCornerPoint[0] * gl_TessCoord.x) + (coarseVertNormalI_at_ICornerPoint_atCornerPoint[1] * gl_TessCoord.y)) + (coarseVertNormalI_at_ICornerPoint_atCornerPoint[2] * gl_TessCoord.z));
			coarseVertNormalI_at_IFineVertex_atFineVertex = t96c;
			_vcmpcoarseVertNormalI_at_IFineVertex = t96c;
			t97b = (((coarseVertTangentI_at_ICornerPoint_atCornerPoint[0] * gl_TessCoord.x) + (coarseVertTangentI_at_ICornerPoint_atCornerPoint[1] * gl_TessCoord.y)) + (coarseVertTangentI_at_ICornerPoint_atCornerPoint[2] * gl_TessCoord.z));
			coarseVertTangentI_at_IFineVertex_atFineVertex = t97b;
			_vcmpcoarseVertTangentI_at_IFineVertex = t97b;
			t98a = blkViewUniformBlock.viewProjectionTransform;
			_vcmpviewProjectionTransformI_at_IFineVertex = t98a;
			t98d = (((vertUV0I_at_ICornerPoint_atCornerPoint[0] * gl_TessCoord.x) + (vertUV0I_at_ICornerPoint_atCornerPoint[1] * gl_TessCoord.y)) + (vertUV0I_at_ICornerPoint_atCornerPoint[2] * gl_TessCoord.z));
			vertUV0I_at_IFineVertex_atFineVertex = t98d;
			_vcmpvertUV0I_at_IFineVertex = t98d;
			t99d = (_vcmpviewProjectionTransformI_at_IFineVertex * vec4(_vcmppos, 1));
			projCoord_atFineVertex = t99d;
			_vcmpprojCoord = t99d;
			gl_Position = _vcmpprojCoord;
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
		vec3 TangentSpaceTransformXWorldSpaceToTangentSpaceXvec3(vec3 t267, vec3 t268, vec3 t269, vec3 t26a);
		vec3 TangentSpaceTransformXTangentSpaceToWorldSpaceXvec3(vec3 t270, vec3 t271, vec3 t272, vec3 t273);
		vec3 SkeletalAnimationXworldTransformPosXvec3(vec3 t6aa);
		vec3 SkeletalAnimationXworldTransformNormalXvec3(vec3 t740);
		vec3 TangentSpaceTransformXWorldSpaceToTangentSpaceXvec3(vec3 t267, vec3 t268, vec3 t269, vec3 t26a)
		{
			return vec3(dot(t26a, t267), dot(t26a, t268), dot(t26a, t269));
		}
		vec3 TangentSpaceTransformXTangentSpaceToWorldSpaceXvec3(vec3 t270, vec3 t271, vec3 t272, vec3 t273)
		{
			return (((t273.x * t270) + (t273.y * t271)) + (t273.z * t272));
		}
		vec3 SkeletalAnimationXworldTransformPosXvec3(vec3 t6aa)
		{
			return t6aa;
		}
		vec3 SkeletalAnimationXworldTransformNormalXvec3(vec3 t740)
		{
			return t740;
		}
		layout(binding = 0) uniform sampler2D albedoMap;
		layout(binding = 1) uniform sampler2D normalMap;
		layout(binding = 2) uniform sampler2D displacementMap;
		layout(std140, binding = 1) uniform ViewUniformBlock
		{
			mat4 viewTransform;
			mat4 viewProjectionTransform;
			mat4 invViewTransform;
			mat4 invViewProjTransform;
			vec3 cameraPos;
		} blkViewUniformBlock;
		in vec4 projCoord_atFineVertex;
		in vec3 coarseVertNormalI_at_IFineVertex_atFineVertex;
		in vec3 coarseVertTangentI_at_IFineVertex_atFineVertex;
		in vec3 fineVertPos_atFineVertex;
		in vec2 vertUV0I_at_IFineVertex_atFineVertex;
		out vec3 outputPbr_atFragment;
		out vec3 outputAlbedo_atFragment;
		out vec3 outputNormal_atFragment;
		void main()
		{
			vec3 _vcmpdisplacement;
			float _vcmpMaterialPatternI_Iarg11_pomparallaxScale;
			float _vcmproughness;
			float _vcmpmetallic;
			float _vcmpspecular;
			vec3 t9aa;
			vec3 _vcmpcoarseVertNormalI_at_IFragment;
			vec3 t9ad;
			vec3 _vcmpcoarseVertTangentI_at_IFragment;
			vec3 t9b0;
			vec3 _vcmpfineVertPosI_at_IFragment;
			vec3 t9b3;
			vec3 _vcmpcameraPosI_at_IFragment;
			vec2 t9b8;
			vec2 _vcmpvertUV0I_at_IFragment;
			vec3 _vcmplightParam;
			vec3 _vcmpoutputPbr;
			vec3 _vcmpvNormal;
			vec3 _vcmpvTangent;
			vec3 _vcmpvBiTangent;
			vec3 _vcmppos;
			vec3 _vcmpMaterialPatternI_Iarg9_pomviewDirTangentSpace;
			vec3 v0_V;
			vec2 v1_T;
			float v2_parallaxHeight;
			float v3_minLayers;
			float v4_maxLayers;
			float v5_numLayers;
			float v6_layerHeight;
			float v7_curLayerHeight;
			vec2 v8_dtex;
			vec2 t9e2;
			vec2 v9_currentTextureCoords;
			float v10_heightFromTexture;
			vec4 t9e6;
			vec4 t9f1;
			vec2 v11_deltaTexCoord;
			float v12_deltaHeight;
			int v13_numSearches;
			int v_14i;
			vec4 ta09;
			vec3 _vcmpMaterialPatternI_IpomI_IparallaxMapping;
			vec2 _vcmppomI_IuvOut;
			vec3 _vcmpalbedo;
			vec3 _vcmpnormal;
			vec3 _vcmpoutputAlbedo;
			vec3 ta28;
			vec3 _vcmpoutputNormal;
			_vcmpdisplacement = vec3(0.000000000000e+00);
			_vcmpMaterialPatternI_Iarg11_pomparallaxScale = 1.999999955297e-02;
			_vcmproughness = 5.000000000000e-01;
			_vcmpmetallic = 3.000000119209e-01;
			_vcmpspecular = 1.000000000000e+00;
			t9aa = coarseVertNormalI_at_IFineVertex_atFineVertex;
			_vcmpcoarseVertNormalI_at_IFragment = t9aa;
			t9ad = coarseVertTangentI_at_IFineVertex_atFineVertex;
			_vcmpcoarseVertTangentI_at_IFragment = t9ad;
			t9b0 = fineVertPos_atFineVertex;
			_vcmpfineVertPosI_at_IFragment = t9b0;
			t9b3 = blkViewUniformBlock.cameraPos;
			_vcmpcameraPosI_at_IFragment = t9b3;
			t9b8 = vertUV0I_at_IFineVertex_atFineVertex;
			_vcmpvertUV0I_at_IFragment = t9b8;
			_vcmplightParam = vec3(_vcmproughness, _vcmpmetallic, _vcmpspecular);
			outputPbr_atFragment = _vcmplightParam;
			_vcmpoutputPbr = _vcmplightParam;
			_vcmpvNormal = SkeletalAnimationXworldTransformNormalXvec3(_vcmpcoarseVertNormalI_at_IFragment).xyz;
			_vcmpvTangent = SkeletalAnimationXworldTransformNormalXvec3(_vcmpcoarseVertTangentI_at_IFragment).xyz;
			_vcmpvBiTangent = cross(_vcmpvTangent, _vcmpvNormal);
			_vcmppos = SkeletalAnimationXworldTransformPosXvec3((_vcmpfineVertPosI_at_IFragment + _vcmpdisplacement));
			_vcmpMaterialPatternI_Iarg9_pomviewDirTangentSpace = TangentSpaceTransformXWorldSpaceToTangentSpaceXvec3(_vcmpvTangent, _vcmpvBiTangent, _vcmpvNormal, normalize((_vcmpcameraPosI_at_IFragment - _vcmppos)));
			v0_V = _vcmpMaterialPatternI_Iarg9_pomviewDirTangentSpace;
			v1_T = _vcmpvertUV0I_at_IFragment;
			v3_minLayers = 10;
			v4_maxLayers = 15;
			v5_numLayers = mix(v4_maxLayers, v3_minLayers, abs(v0_V.z));
			v6_layerHeight = (1.000000000000e+00 / v5_numLayers);
			v7_curLayerHeight = 9.999999776483e-03;
			v8_dtex = (((_vcmpMaterialPatternI_Iarg11_pomparallaxScale * v0_V.xy) / v0_V.z) / v5_numLayers);
			t9e2 = v8_dtex;
			t9e2[1] = (-v8_dtex.y);
			v8_dtex = t9e2;
			v9_currentTextureCoords = v1_T;
			t9e6 = texture(displacementMap, v9_currentTextureCoords);
			v10_heightFromTexture = (1.000000000000e+00 - t9e6.x);
			while (bool((v10_heightFromTexture > v7_curLayerHeight)))
			{
				v7_curLayerHeight = (v7_curLayerHeight + v6_layerHeight);
				v9_currentTextureCoords = (v9_currentTextureCoords - v8_dtex);
				t9f1 = texture(displacementMap, v9_currentTextureCoords);
				v10_heightFromTexture = (1.000000000000e+00 - t9f1.x);
			}
			v11_deltaTexCoord = (v8_dtex / 2);
			v12_deltaHeight = (v6_layerHeight / 2);
			v9_currentTextureCoords = (v9_currentTextureCoords + v11_deltaTexCoord);
			v7_curLayerHeight = (v7_curLayerHeight - v12_deltaHeight);
			v13_numSearches = 5;
			v_14i = 0;
			for (;bool((v_14i <= v13_numSearches)); (v_14i = (v_14i + 1)))
			{
				v11_deltaTexCoord = (v11_deltaTexCoord / 2);
				v12_deltaHeight = (v12_deltaHeight / 2);
				ta09 = texture(displacementMap, v9_currentTextureCoords);
				v10_heightFromTexture = (1.000000000000e+00 - ta09.x);
				if (bool((v10_heightFromTexture > v7_curLayerHeight)))
				{
					v9_currentTextureCoords = (v9_currentTextureCoords - v11_deltaTexCoord);
					v7_curLayerHeight = (v7_curLayerHeight + v12_deltaHeight);
				}
				else
				{
					v9_currentTextureCoords = (v9_currentTextureCoords + v11_deltaTexCoord);
					v7_curLayerHeight = (v7_curLayerHeight - v12_deltaHeight);
				}
			}
			v2_parallaxHeight = v7_curLayerHeight;
			_vcmpMaterialPatternI_IpomI_IparallaxMapping = vec3(v9_currentTextureCoords, v2_parallaxHeight);
			_vcmppomI_IuvOut = _vcmpMaterialPatternI_IpomI_IparallaxMapping.xy;
			_vcmpalbedo = (texture(albedoMap, _vcmppomI_IuvOut).xyz * 6.999999880791e-01);
			_vcmpnormal = normalize(((texture(normalMap, _vcmppomI_IuvOut).xyz * 2.000000000000e+00) - 1.000000000000e+00));
			outputAlbedo_atFragment = _vcmpalbedo;
			_vcmpoutputAlbedo = _vcmpalbedo;
			ta28 = TangentSpaceTransformXTangentSpaceToWorldSpaceXvec3(_vcmpvTangent, _vcmpvBiTangent, _vcmpvNormal, _vcmpnormal);
			outputNormal_atFragment = ta28;
			_vcmpoutputNormal = ta28;
		}
	}
}
