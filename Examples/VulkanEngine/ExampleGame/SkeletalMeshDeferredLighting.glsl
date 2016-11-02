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
			vec4 rs;
			rs[0] = ((((p_q1.w * p_q2.x) + (p_q1.x * p_q2.w)) + (p_q1.y * p_q2.z)) - (p_q1.z * p_q2.y));
			rs[1] = ((((p_q1.w * p_q2.y) + (p_q1.y * p_q2.w)) + (p_q1.z * p_q2.x)) - (p_q1.x * p_q2.z));
			rs[2] = ((((p_q1.w * p_q2.z) + (p_q1.z * p_q2.w)) + (p_q1.x * p_q2.y)) - (p_q1.y * p_q2.x));
			rs[3] = ((((p_q1.w * p_q2.w) - (p_q1.x * p_q2.x)) - (p_q1.y * p_q2.y)) - (p_q1.z * p_q2.z));
			return rs;
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
		layout(location = 0) in vec3 vertPos_MeshVertex;
		layout(location = 1) in vec2 vertUV0_MeshVertex;
		layout(location = 2) in uint tangentFrame_MeshVertex;
		out vec3 coarseVertPos_CoarseVertex;
		out vec3 coarseVertNormal_CoarseVertex;
		out vec3 coarseVertTangent_CoarseVertex;
		out vec2 vertUV0_CoarseVertex;
		void main()
		{
			vec3 vertPos;
			uint tangentFrame = 0;;
			vec2 vertUV0;
			vec4 tangentFrameQuaternion;
			float inv255;
			SkinningResult skinning;
			int i = 0;;
			uint boneId = 0;;
			float boneWeight;
			vec3 tp;
			BoneTransform t2c2a;
			BoneTransform t2c33;
			BoneTransform t2c3b;
			vec3 coarseVertPos;
			vec3 coarseVertNormal;
			vec3 coarseVertTangent;
			vertPos = vertPos_MeshVertex;
			tangentFrame = tangentFrame_MeshVertex;
			vertUV0 = vertUV0_MeshVertex;
			vertUV0_CoarseVertex = vertUV0;
			inv255 = (2.000000000000e+00 / 2.550000000000e+02);
			tangentFrameQuaternion[0] = ((float((tangentFrame & 255)) * inv255) - 1.000000000000e+00);
			tangentFrameQuaternion[1] = ((float(((tangentFrame >> 8) & 255)) * inv255) - 1.000000000000e+00);
			tangentFrameQuaternion[2] = ((float(((tangentFrame >> 16) & 255)) * inv255) - 1.000000000000e+00);
			tangentFrameQuaternion[3] = ((float(((tangentFrame >> 24) & 255)) * inv255) - 1.000000000000e+00);
			skinning.pos = vec3(0.000000000000e+00);
			skinning.normal = vec3(0.000000000000e+00);
			skinning.tangent = vec3(0.000000000000e+00);
			i = 0;
			for (;bool((i <= 3)); (i = (i + 1)))
			{
				boneId = ((255 >> (i * 8)) & 255);
				if (bool((boneId == 255)))
				{
					continue;
				}
				boneWeight = (float(((0 >> (i * 8)) & 255)) * (1.000000000000e+00 / 2.550000000000e+02));
				t2c2a = blkSkeletonDataBlock.boneTransforms[boneId];
				tp = (t2c2a.transformMatrix * vec4(vertPos, 1.000000000000e+00)).xyz;
				skinning.pos = (skinning.pos + (tp * boneWeight));
				t2c33 = blkSkeletonDataBlock.boneTransforms[boneId];
				tp = (t2c33.normalMatrix * normalize(QuaternionRotate(tangentFrameQuaternion, vec3(0.000000000000e+00, 1.000000000000e+00, 0.000000000000e+00))));
				skinning.normal = (skinning.normal + (tp * boneWeight));
				t2c3b = blkSkeletonDataBlock.boneTransforms[boneId];
				tp = (t2c3b.normalMatrix * normalize(QuaternionRotate(tangentFrameQuaternion, vec3(1.000000000000e+00, 0.000000000000e+00, 0.000000000000e+00))));
				skinning.tangent = (skinning.tangent + (tp * boneWeight));
			}
			coarseVertPos = skinning.pos;
			coarseVertPos_CoarseVertex = coarseVertPos;
			coarseVertNormal = skinning.normal;
			coarseVertNormal_CoarseVertex = coarseVertNormal;
			coarseVertTangent = skinning.tangent;
			coarseVertTangent_CoarseVertex = coarseVertTangent;
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
		vec3 PN_Tessellation_ProjectToPlane_vec3_vec3_vec3(vec3 p0_Point, vec3 p1_PlanePoint, vec3 p2_PlaneNormal);
		vec3 PN_Tessellation_ProjectToPlane_vec3_vec3_vec3(vec3 p0_Point, vec3 p1_PlanePoint, vec3 p2_PlaneNormal)
		{
			vec3 v;
			float Len;
			vec3 d;
			v = (p0_Point - p1_PlanePoint);
			Len = dot(v, p2_PlaneNormal);
			d = (Len * p2_PlaneNormal);
			return (p0_Point - d);
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
		in vec3 coarseVertPos_CoarseVertex[];
		in vec3 coarseVertNormal_CoarseVertex[];
		in vec3 coarseVertTangent_CoarseVertex[];
		in vec2 vertUV0_CoarseVertex[];
		out vec4 tessLevelOuter_ControlPoint[]; 
		out vec2 tessLevelInner_ControlPoint[]; 
		out vec3 MaterialGeometry_PN_Tessellation_WorldPos_B300_ControlPoint[]; 
		out vec3 MaterialGeometry_PN_Tessellation_WorldPos_B030_ControlPoint[]; 
		out vec3 MaterialGeometry_PN_Tessellation_WorldPos_B003_ControlPoint[]; 
		out vec3 MaterialGeometry_PN_Tessellation_WorldPos_B210_ControlPoint[]; 
		out vec3 MaterialGeometry_PN_Tessellation_WorldPos_B120_ControlPoint[]; 
		out vec3 MaterialGeometry_PN_Tessellation_WorldPos_B201_ControlPoint[]; 
		out vec3 MaterialGeometry_PN_Tessellation_WorldPos_B021_ControlPoint[]; 
		out vec3 MaterialGeometry_PN_Tessellation_WorldPos_B102_ControlPoint[]; 
		out vec3 MaterialGeometry_PN_Tessellation_WorldPos_B012_ControlPoint[]; 
		out vec3 MaterialGeometry_PN_Tessellation_WorldPos_B111_ControlPoint[]; 
		patch out vec3 coarseVertNormal_CornerPoint[3]; 
		patch out vec3 coarseVertTangent_CornerPoint[3]; 
		patch out vec2 vertUV0_CornerPoint[3]; 
		void main()
		{
			vec4 tessLevelOuter;
			vec2 tessLevelInner;
			vec3 WorldPos_B030;
			vec3 WorldPos_B003;
			vec3 WorldPos_B300;
			vec3 EdgeB300;
			vec3 EdgeB030;
			vec3 EdgeB003;
			vec3 t2c62;
			vec3 WorldPos_B021;
			vec3 t2c66;
			vec3 WorldPos_B012;
			vec3 t2c6a;
			vec3 WorldPos_B102;
			vec3 t2c6e;
			vec3 WorldPos_B201;
			vec3 t2c72;
			vec3 WorldPos_B210;
			vec3 t2c76;
			vec3 WorldPos_B120;
			vec3 WorldPos_B111t;
			vec3 WorldPos_B111;
			vec3 coarseVertNormal;
			vec3 coarseVertTangent;
			vec2 vertUV0;
			tessLevelOuter = vec4(3, 3, 3, 0);
			tessLevelOuter_ControlPoint[gl_InvocationID] = tessLevelOuter;
			tessLevelInner = vec2(3.000000000000e+00);
			tessLevelInner_ControlPoint[gl_InvocationID] = tessLevelInner;
			WorldPos_B030 = coarseVertPos_CoarseVertex[0];
			MaterialGeometry_PN_Tessellation_WorldPos_B030_ControlPoint[gl_InvocationID] = WorldPos_B030;
			WorldPos_B003 = coarseVertPos_CoarseVertex[1];
			MaterialGeometry_PN_Tessellation_WorldPos_B003_ControlPoint[gl_InvocationID] = WorldPos_B003;
			WorldPos_B300 = coarseVertPos_CoarseVertex[2];
			MaterialGeometry_PN_Tessellation_WorldPos_B300_ControlPoint[gl_InvocationID] = WorldPos_B300;
			EdgeB300 = (WorldPos_B003 - WorldPos_B030);
			EdgeB030 = (WorldPos_B300 - WorldPos_B003);
			EdgeB003 = (WorldPos_B030 - WorldPos_B300);
			t2c62 = coarseVertNormal_CoarseVertex[0];
			WorldPos_B021 = PN_Tessellation_ProjectToPlane_vec3_vec3_vec3((WorldPos_B030 + (EdgeB300 / 3.000000000000e+00)), WorldPos_B030, t2c62);
			MaterialGeometry_PN_Tessellation_WorldPos_B021_ControlPoint[gl_InvocationID] = WorldPos_B021;
			t2c66 = coarseVertNormal_CoarseVertex[1];
			WorldPos_B012 = PN_Tessellation_ProjectToPlane_vec3_vec3_vec3((WorldPos_B030 + ((EdgeB300 * 2.000000000000e+00) / 3.000000000000e+00)), WorldPos_B003, t2c66);
			MaterialGeometry_PN_Tessellation_WorldPos_B012_ControlPoint[gl_InvocationID] = WorldPos_B012;
			t2c6a = coarseVertNormal_CoarseVertex[1];
			WorldPos_B102 = PN_Tessellation_ProjectToPlane_vec3_vec3_vec3((WorldPos_B003 + (EdgeB030 / 3.000000000000e+00)), WorldPos_B003, t2c6a);
			MaterialGeometry_PN_Tessellation_WorldPos_B102_ControlPoint[gl_InvocationID] = WorldPos_B102;
			t2c6e = coarseVertNormal_CoarseVertex[2];
			WorldPos_B201 = PN_Tessellation_ProjectToPlane_vec3_vec3_vec3((WorldPos_B003 + ((EdgeB030 * 2.000000000000e+00) / 3.000000000000e+00)), WorldPos_B300, t2c6e);
			MaterialGeometry_PN_Tessellation_WorldPos_B201_ControlPoint[gl_InvocationID] = WorldPos_B201;
			t2c72 = coarseVertNormal_CoarseVertex[2];
			WorldPos_B210 = PN_Tessellation_ProjectToPlane_vec3_vec3_vec3((WorldPos_B300 + (EdgeB003 / 3.000000000000e+00)), WorldPos_B300, t2c72);
			MaterialGeometry_PN_Tessellation_WorldPos_B210_ControlPoint[gl_InvocationID] = WorldPos_B210;
			t2c76 = coarseVertNormal_CoarseVertex[0];
			WorldPos_B120 = PN_Tessellation_ProjectToPlane_vec3_vec3_vec3((WorldPos_B300 + ((EdgeB003 * 2.000000000000e+00) / 3.000000000000e+00)), WorldPos_B030, t2c76);
			MaterialGeometry_PN_Tessellation_WorldPos_B120_ControlPoint[gl_InvocationID] = WorldPos_B120;
			WorldPos_B111t = ((((((WorldPos_B021 + WorldPos_B012) + WorldPos_B102) + WorldPos_B201) + WorldPos_B210) + WorldPos_B120) / 6.000000000000e+00);
			WorldPos_B111 = (WorldPos_B111t + ((WorldPos_B111t - (((WorldPos_B003 + WorldPos_B030) + WorldPos_B300) / 3.000000000000e+00)) / 2.000000000000e+00));
			MaterialGeometry_PN_Tessellation_WorldPos_B111_ControlPoint[gl_InvocationID] = WorldPos_B111;
			for (int sysLocalIterator = 0; sysLocalIterator < gl_PatchVerticesIn; sysLocalIterator++)
			{
				coarseVertNormal = coarseVertNormal_CoarseVertex[sysLocalIterator];
				coarseVertNormal_CornerPoint[sysLocalIterator] = coarseVertNormal;
				coarseVertTangent = coarseVertTangent_CoarseVertex[sysLocalIterator];
				coarseVertTangent_CornerPoint[sysLocalIterator] = coarseVertTangent;
				vertUV0 = vertUV0_CoarseVertex[sysLocalIterator];
				vertUV0_CornerPoint[sysLocalIterator] = vertUV0;
			}
			gl_TessLevelInner[0] = tessLevelInner[0];
			gl_TessLevelInner[1] = tessLevelInner[1];
			gl_TessLevelOuter[0] = tessLevelOuter[0];
			gl_TessLevelOuter[1] = tessLevelOuter[1];
			gl_TessLevelOuter[2] = tessLevelOuter[2];
			gl_TessLevelOuter[3] = tessLevelOuter[3];
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
		vec3 SkeletalAnimation_worldTransformPos_vec3(vec3 p0_pos);
		vec3 SkeletalAnimation_worldTransformPos_vec3(vec3 p0_pos)
		{
			return p0_pos;
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
		in vec4 tessLevelOuter_ControlPoint[];
		in vec2 tessLevelInner_ControlPoint[];
		in vec3 MaterialGeometry_PN_Tessellation_WorldPos_B300_ControlPoint[];
		in vec3 MaterialGeometry_PN_Tessellation_WorldPos_B030_ControlPoint[];
		in vec3 MaterialGeometry_PN_Tessellation_WorldPos_B003_ControlPoint[];
		in vec3 MaterialGeometry_PN_Tessellation_WorldPos_B210_ControlPoint[];
		in vec3 MaterialGeometry_PN_Tessellation_WorldPos_B120_ControlPoint[];
		in vec3 MaterialGeometry_PN_Tessellation_WorldPos_B201_ControlPoint[];
		in vec3 MaterialGeometry_PN_Tessellation_WorldPos_B021_ControlPoint[];
		in vec3 MaterialGeometry_PN_Tessellation_WorldPos_B102_ControlPoint[];
		in vec3 MaterialGeometry_PN_Tessellation_WorldPos_B012_ControlPoint[];
		in vec3 MaterialGeometry_PN_Tessellation_WorldPos_B111_ControlPoint[];
		patch in vec3 coarseVertNormal_CornerPoint[3];
		patch in vec3 coarseVertTangent_CornerPoint[3];
		patch in vec2 vertUV0_CornerPoint[3];
		out vec4 projCoord_FineVertex;
		out vec3 coarseVertNormal_FineVertex;
		out vec3 coarseVertTangent_FineVertex;
		out vec3 fineVertPos_FineVertex;
		out vec2 vertUV0_FineVertex;
		void main()
		{
			float u;
			float v;
			float w;
			float vPow2;
			float uPow2;
			float wPow2;
			vec3 t2c96;
			vec3 t2c9d;
			vec3 t2ca3;
			vec3 t2ca9;
			vec3 t2caf;
			vec3 t2cb5;
			vec3 t2cbb;
			vec3 t2cc1;
			vec3 t2cc5;
			vec3 t2cc9;
			vec3 fineVertPos;
			vec3 coarseVertNormal;
			vec3 coarseVertTangent;
			mat4 viewProjectionTransform;
			vec2 vertUV0;
			vec4 projCoord;
			u = gl_TessCoord.x;
			v = gl_TessCoord.y;
			w = gl_TessCoord.z;
			vPow2 = (v * v);
			uPow2 = (u * u);
			wPow2 = (w * w);
			t2c96 = MaterialGeometry_PN_Tessellation_WorldPos_B111_ControlPoint[0];
			t2c9d = MaterialGeometry_PN_Tessellation_WorldPos_B012_ControlPoint[0];
			t2ca3 = MaterialGeometry_PN_Tessellation_WorldPos_B102_ControlPoint[0];
			t2ca9 = MaterialGeometry_PN_Tessellation_WorldPos_B021_ControlPoint[0];
			t2caf = MaterialGeometry_PN_Tessellation_WorldPos_B201_ControlPoint[0];
			t2cb5 = MaterialGeometry_PN_Tessellation_WorldPos_B120_ControlPoint[0];
			t2cbb = MaterialGeometry_PN_Tessellation_WorldPos_B210_ControlPoint[0];
			t2cc1 = MaterialGeometry_PN_Tessellation_WorldPos_B003_ControlPoint[0];
			t2cc5 = MaterialGeometry_PN_Tessellation_WorldPos_B030_ControlPoint[0];
			t2cc9 = MaterialGeometry_PN_Tessellation_WorldPos_B300_ControlPoint[0];
			fineVertPos = ((((((((((t2cc9 * (wPow2 * w)) + (t2cc5 * (uPow2 * u))) + (t2cc1 * (vPow2 * v))) + (((t2cbb * 3.000000000000e+00) * wPow2) * u)) + (((t2cb5 * 3.000000000000e+00) * w) * uPow2)) + (((t2caf * 3.000000000000e+00) * wPow2) * v)) + (((t2ca9 * 3.000000000000e+00) * uPow2) * v)) + (((t2ca3 * 3.000000000000e+00) * w) * vPow2)) + (((t2c9d * 3.000000000000e+00) * u) * vPow2)) + ((((t2c96 * 6.000000000000e+00) * w) * u) * v));
			fineVertPos_FineVertex = fineVertPos;
			coarseVertNormal = (((coarseVertNormal_CornerPoint[0] * gl_TessCoord.x) + (coarseVertNormal_CornerPoint[1] * gl_TessCoord.y)) + (coarseVertNormal_CornerPoint[2] * gl_TessCoord.z));
			coarseVertNormal_FineVertex = coarseVertNormal;
			coarseVertTangent = (((coarseVertTangent_CornerPoint[0] * gl_TessCoord.x) + (coarseVertTangent_CornerPoint[1] * gl_TessCoord.y)) + (coarseVertTangent_CornerPoint[2] * gl_TessCoord.z));
			coarseVertTangent_FineVertex = coarseVertTangent;
			viewProjectionTransform = blkViewUniformBlock.viewProjectionTransform;
			vertUV0 = (((vertUV0_CornerPoint[0] * gl_TessCoord.x) + (vertUV0_CornerPoint[1] * gl_TessCoord.y)) + (vertUV0_CornerPoint[2] * gl_TessCoord.z));
			vertUV0_FineVertex = vertUV0;
			projCoord = (viewProjectionTransform * vec4(SkeletalAnimation_worldTransformPos_vec3((fineVertPos + vec3(0.000000000000e+00))), 1));
			projCoord_FineVertex = projCoord;
			gl_Position = projCoord;
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
		vec3 TangentSpaceTransform_WorldSpaceToTangentSpace_vec3(vec3 p0_vTangent, vec3 p1_vBiTangent, vec3 p2_vNormal, vec3 p3_v);
		vec3 TangentSpaceTransform_TangentSpaceToWorldSpace_vec3(vec3 p0_vTangent, vec3 p1_vBiTangent, vec3 p2_vNormal, vec3 p3_v);
		vec3 SkeletalAnimation_worldTransformPos_vec3(vec3 p0_pos);
		vec3 SkeletalAnimation_worldTransformNormal_vec3(vec3 p0_norm);
		vec3 TangentSpaceTransform_WorldSpaceToTangentSpace_vec3(vec3 p0_vTangent, vec3 p1_vBiTangent, vec3 p2_vNormal, vec3 p3_v)
		{
			return vec3(dot(p3_v, p0_vTangent), dot(p3_v, p1_vBiTangent), dot(p3_v, p2_vNormal));
		}
		vec3 TangentSpaceTransform_TangentSpaceToWorldSpace_vec3(vec3 p0_vTangent, vec3 p1_vBiTangent, vec3 p2_vNormal, vec3 p3_v)
		{
			return (((p3_v.x * p0_vTangent) + (p3_v.y * p1_vBiTangent)) + (p3_v.z * p2_vNormal));
		}
		vec3 SkeletalAnimation_worldTransformPos_vec3(vec3 p0_pos)
		{
			return p0_pos;
		}
		vec3 SkeletalAnimation_worldTransformNormal_vec3(vec3 p0_norm)
		{
			return p0_norm;
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
		in vec4 projCoord_FineVertex;
		in vec3 coarseVertNormal_FineVertex;
		in vec3 coarseVertTangent_FineVertex;
		in vec3 fineVertPos_FineVertex;
		in vec2 vertUV0_FineVertex;
		out vec3 outputPbr_Fragment;
		out vec3 outputAlbedo_Fragment;
		out vec3 outputNormal_Fragment;
		void main()
		{
			vec3 coarseVertNormal;
			vec3 coarseVertTangent;
			vec3 fineVertPos;
			vec3 cameraPos;
			vec2 vertUV0;
			vec3 outputPbr;
			vec3 vNormal;
			vec3 vTangent;
			vec3 vBiTangent;
			vec3 V;
			vec2 T;
			float parallaxHeight;
			float minLayers;
			float maxLayers;
			float numLayers;
			float layerHeight;
			float curLayerHeight;
			vec2 dtex;
			vec2 currentTextureCoords;
			float heightFromTexture;
			vec4 t2d28;
			vec4 t2d33;
			vec2 deltaTexCoord;
			float deltaHeight;
			int numSearches = 0;;
			int i = 0;;
			vec4 t2d4b;
			vec2 uvOut;
			vec3 outputAlbedo;
			vec3 outputNormal;
			coarseVertNormal = coarseVertNormal_FineVertex;
			coarseVertTangent = coarseVertTangent_FineVertex;
			fineVertPos = fineVertPos_FineVertex;
			cameraPos = blkViewUniformBlock.cameraPos;
			vertUV0 = vertUV0_FineVertex;
			outputPbr = vec3(5.000000000000e-01, 3.000000119209e-01, 1.000000000000e+00);
			outputPbr_Fragment = outputPbr;
			vNormal = SkeletalAnimation_worldTransformNormal_vec3(coarseVertNormal).xyz;
			vTangent = SkeletalAnimation_worldTransformNormal_vec3(coarseVertTangent).xyz;
			vBiTangent = cross(vTangent, vNormal);
			V = TangentSpaceTransform_WorldSpaceToTangentSpace_vec3(vTangent, vBiTangent, vNormal, normalize((cameraPos - SkeletalAnimation_worldTransformPos_vec3((fineVertPos + vec3(0.000000000000e+00))))));
			T = vertUV0;
			minLayers = 10;
			maxLayers = 15;
			numLayers = mix(maxLayers, minLayers, abs(V.z));
			layerHeight = (1.000000000000e+00 / numLayers);
			curLayerHeight = 9.999999776483e-03;
			dtex = (((1.999999955297e-02 * V.xy) / V.z) / numLayers);
			dtex[1] = (-dtex.y);
			currentTextureCoords = T;
			t2d28 = texture(displacementMap, currentTextureCoords);
			heightFromTexture = (1.000000000000e+00 - t2d28.x);
			while (bool((heightFromTexture > curLayerHeight)))
			{
				curLayerHeight = (curLayerHeight + layerHeight);
				currentTextureCoords = (currentTextureCoords - dtex);
				t2d33 = texture(displacementMap, currentTextureCoords);
				heightFromTexture = (1.000000000000e+00 - t2d33.x);
			}
			deltaTexCoord = (dtex / 2);
			deltaHeight = (layerHeight / 2);
			currentTextureCoords = (currentTextureCoords + deltaTexCoord);
			curLayerHeight = (curLayerHeight - deltaHeight);
			numSearches = 5;
			i = 0;
			for (;bool((i <= numSearches)); (i = (i + 1)))
			{
				deltaTexCoord = (deltaTexCoord / 2);
				deltaHeight = (deltaHeight / 2);
				t2d4b = texture(displacementMap, currentTextureCoords);
				heightFromTexture = (1.000000000000e+00 - t2d4b.x);
				if (bool((heightFromTexture > curLayerHeight)))
				{
					currentTextureCoords = (currentTextureCoords - deltaTexCoord);
					curLayerHeight = (curLayerHeight + deltaHeight);
				}
				else
				{
					currentTextureCoords = (currentTextureCoords + deltaTexCoord);
					curLayerHeight = (curLayerHeight - deltaHeight);
				}
			}
			parallaxHeight = curLayerHeight;
			uvOut = vec3(currentTextureCoords, parallaxHeight).xy;
			outputAlbedo = (texture(albedoMap, uvOut).xyz * 6.999999880791e-01);
			outputAlbedo_Fragment = outputAlbedo;
			outputNormal = TangentSpaceTransform_TangentSpaceToWorldSpace_vec3(vTangent, vBiTangent, vNormal, normalize(((texture(normalMap, uvOut).xyz * 2.000000000000e+00) - 1.000000000000e+00)));
			outputNormal_Fragment = outputNormal;
		}
	}
}
