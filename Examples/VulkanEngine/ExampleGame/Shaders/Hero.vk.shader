name SkeletalMeshDeferredLighting
world vs
{
	target glsl
	in perInstanceUniform;
	in viewUniform;
	in modelTransform;
	in skeletalTransform;
	in rootVert;
	out vs;
	comp MeshVertexI_ItangentFrameQuaternion;
	comp vertNormal;
	comp vertTangent;
	comp SkeletalVertexTransformI_Iposition;
	comp projCoord;
	comp vNormal;
	comp vTangent;
}
world fs
{
	target glsl
	in perInstanceUniform;
	in viewUniform;
	in modelTransform;
	in skeletalTransform;
	in vs;
	out fs;
	comp vertUV;
	comp vBiTangent;
	comp normal;
	comp roughness;
	comp metallic;
	comp specular;
	comp albedo;
	comp lightParam;
	comp arg2_transformedNormalnormal_in;
	comp outputAlbedo;
	comp outputPbr;
	comp transformedNormalI_Inormal;
	comp outputNormal;
}
interface rootVert size 48
{
	vec3 vertPos : 0,12;
	vec2 vertUV0 : 12,8;
	uint tangentFrame : 20,4;
	vec4 vertColor0 : 24,16;
	uint boneIds : 40,4;
	uint boneWeights : 44,4;
}
interface modelTransform size 0
{
}
interface skeletalTransform size 0
{
	BoneTransform[] boneTransforms : 0,0;
}
interface viewUniform size 272
{
	mat4 viewTransform : 0,64;
	mat4 viewProjectionTransform : 64,64;
	mat4 invViewTransform : 128,64;
	mat4 invViewProjTransform : 192,64;
	vec3 cameraPos : 256,12;
}
interface perInstanceUniform size 8
{
	sampler2D albedoMap : 0,8;
}
interface vs size 48
{
	vec3 vNormal : 0,12;
	vec3 vTangent : 16,12;
	vec2 vertUV0 : 32,8;
}
interface fs size 48
{
	vec3 outputAlbedo : 0,12;
	vec3 outputPbr : 16,12;
	vec3 outputNormal : 32,12;
}
source
{
	vs
	{
		#version 450
		struct BoneTransform
		{
			mat4 transformMatrix;
			mat3 normalMatrix;
		};


		//$vertPos$vertPos
		//$vertUV0$vertUV0
		//$tangentFrame$tangentFrame
		//$boneIds$boneIds
		//$boneWeights$boneWeights
		//$boneTransforms$blkskeletalTransform.boneTransforms
		//$viewProjectionTransform$blkviewUniform.viewProjectionTransform
		//$MeshVertexI_ItangentFrameQuaternion$_vcmpMeshVertexI_ItangentFrameQuaternion
		//$vertNormal$_vcmpvertNormal
		//$vertTangent$_vcmpvertTangent
		//$SkeletalVertexTransformI_Iposition$_vcmpSkeletalVertexTransformI_Iposition
		//$projCoord$_vcmpprojCoord
		//$vNormal$_vcmpvNormal
		//$vTangent$_vcmpvTangent
		//! input from viewUniform
		layout(std140, binding = 1) uniform viewUniform
		{
			mat4 viewTransform;
			mat4 viewProjectionTransform;
			mat4 invViewTransform;
			mat4 invViewProjTransform;
			vec3 cameraPos;
		} blkviewUniform;
		//! input from skeletalTransform
		layout(std140, binding = 3) buffer skeletalTransform
		{
			BoneTransform[] boneTransforms;
		} blkskeletalTransform;
		//! input from rootVert
		layout(location = 0) in vec3 vertPos;
		layout(location = 1) in vec2 vertUV0;
		layout(location = 2) in uint tangentFrame;
		layout(location = 3) in vec4 vertColor0;
		layout(location = 4) in uint boneIds;
		layout(location = 5) in uint boneWeights;

		//! output declarations
		out vs
		{
			vec3 vNormal;
			vec3 vTangent;
			vec2 vertUV0;
		} blkvs;

		//! global declarations
		vec3 QuaternionRotate(vec4 p_q, vec3 p_pos);
		vec4 QuaternionMul(vec4 p_q1, vec4 p_q2);
		vec4 QuaternionConjugate(vec4 p_q);
		vec3 QuaternionRotate(vec4 p_q, vec3 p_pos)
		{
			vec4 t190;
			vec3 t191;
			vec3 t192;
			t190 = QuaternionMul(QuaternionMul(p_q, vec4(p_pos, 0.000000000000e+00)), QuaternionConjugate(p_q));
			t192 = t191;
			t192[0] = t190.x;
			t192[1] = t190.y;
			t192[2] = t190.z;
			t191 = t192;
			return t191;
		}
		vec4 QuaternionMul(vec4 p_q1, vec4 p_q2)
		{
			vec4 v0_rs;
			vec4 t1ab;
			vec4 t1bd;
			vec4 t1cf;
			vec4 t1e1;
			t1ab = v0_rs;
			t1ab[0] = ((((p_q1.w * p_q2.x) + (p_q1.x * p_q2.w)) + (p_q1.y * p_q2.z)) - (p_q1.z * p_q2.y));
			v0_rs = t1ab;
			t1bd = v0_rs;
			t1bd[1] = ((((p_q1.w * p_q2.y) + (p_q1.y * p_q2.w)) + (p_q1.z * p_q2.x)) - (p_q1.x * p_q2.z));
			v0_rs = t1bd;
			t1cf = v0_rs;
			t1cf[2] = ((((p_q1.w * p_q2.z) + (p_q1.z * p_q2.w)) + (p_q1.x * p_q2.y)) - (p_q1.y * p_q2.x));
			v0_rs = t1cf;
			t1e1 = v0_rs;
			t1e1[3] = ((((p_q1.w * p_q2.w) - (p_q1.x * p_q2.x)) - (p_q1.y * p_q2.y)) - (p_q1.z * p_q2.z));
			v0_rs = t1e1;
			return v0_rs;
		}
		vec4 QuaternionConjugate(vec4 p_q)
		{
			return vec4((-p_q.x), (-p_q.y), (-p_q.z), p_q.w);
		}


		//! end declarations
		void main()
		{
			//! local declarations
			vec4 _vcmpMeshVertexI_ItangentFrameQuaternion;
			vec4 v0_result;
			float v1_inv255;
			vec4 t107;
			vec4 t10f;
			vec4 t117;
			vec4 t11f;
			vec3 _vcmpvertNormal;
			vec3 _vcmpvertTangent;
			vec4 _vcmpSkeletalVertexTransformI_Iposition;
			vec4 v2_result;
			int v_3i;
			uint v4_boneId;
			float v5_boneWeight;
			vec4 v6_tp;
			vec4 _vcmpprojCoord;
			vec3 _vcmpvNormal;
			vec3 v7_result;
			int v_8i;
			uint v9_boneId;
			float v10_boneWeight;
			vec3 v11_tp;
			vec3 _vcmpvTangent;
			vec3 v12_result;
			int v_13i;
			uint v14_boneId;
			float v15_boneWeight;
			vec3 v16_tp;

			//! main code


			blkvs.vertUV0 = vertUV0;
			v1_inv255 = (2.000000000000e+00 / 2.550000000000e+02);
			t107 = v0_result;
			t107[0] = ((float((tangentFrame & 255)) * v1_inv255) - 1.000000000000e+00);
			v0_result = t107;
			t10f = v0_result;
			t10f[1] = ((float(((tangentFrame >> 8) & 255)) * v1_inv255) - 1.000000000000e+00);
			v0_result = t10f;
			t117 = v0_result;
			t117[2] = ((float(((tangentFrame >> 16) & 255)) * v1_inv255) - 1.000000000000e+00);
			v0_result = t117;
			t11f = v0_result;
			t11f[3] = ((float(((tangentFrame >> 24) & 255)) * v1_inv255) - 1.000000000000e+00);
			v0_result = t11f;
			_vcmpMeshVertexI_ItangentFrameQuaternion = v0_result;
			_vcmpvertNormal = normalize(QuaternionRotate(_vcmpMeshVertexI_ItangentFrameQuaternion, vec3(0.000000000000e+00, 1.000000000000e+00, 0.000000000000e+00)));
			_vcmpvertTangent = normalize(QuaternionRotate(_vcmpMeshVertexI_ItangentFrameQuaternion, vec3(1.000000000000e+00, 0.000000000000e+00, 0.000000000000e+00)));
			v2_result = vec4(0.000000000000e+00);
			v_3i = 0;
			for (;bool(int(v_3i <= 3)); (v_3i = (v_3i + 1)))
			{
				v4_boneId = ((boneIds >> (v_3i * 8)) & 255);
				if (bool(int(v4_boneId == 255)))
				{
					continue;
				}
				v5_boneWeight = (float(((boneWeights >> (v_3i * 8)) & 255)) * (1.000000000000e+00 / 2.550000000000e+02));
				v6_tp = (blkskeletalTransform.boneTransforms[v4_boneId].transformMatrix * vec4(vertPos, 1.000000000000e+00));
				v2_result = (v2_result + (v6_tp * v5_boneWeight));
			}
			_vcmpSkeletalVertexTransformI_Iposition = v2_result;
			_vcmpprojCoord = (blkviewUniform.viewProjectionTransform * _vcmpSkeletalVertexTransformI_Iposition);
			v7_result = vec3(0.000000000000e+00);
			v_8i = 0;
			for (;bool(int(v_8i <= 3)); (v_8i = (v_8i + 1)))
			{
				v9_boneId = ((boneIds >> (v_8i * 8)) & 255);
				if (bool(int(v9_boneId == 255)))
				{
					continue;
				}
				v10_boneWeight = (float(((boneWeights >> (v_8i * 8)) & 255)) * (1.000000000000e+00 / 2.550000000000e+02));
				v11_tp = (blkskeletalTransform.boneTransforms[v9_boneId].normalMatrix * _vcmpvertNormal);
				v7_result = (v7_result + (v11_tp * v10_boneWeight));
			}
			_vcmpvNormal = v7_result;
			blkvs.vNormal = _vcmpvNormal;
			v12_result = vec3(0.000000000000e+00);
			v_13i = 0;
			for (;bool(int(v_13i <= 3)); (v_13i = (v_13i + 1)))
			{
				v14_boneId = ((boneIds >> (v_13i * 8)) & 255);
				if (bool(int(v14_boneId == 255)))
				{
					continue;
				}
				v15_boneWeight = (float(((boneWeights >> (v_13i * 8)) & 255)) * (1.000000000000e+00 / 2.550000000000e+02));
				v16_tp = (blkskeletalTransform.boneTransforms[v14_boneId].normalMatrix * _vcmpvertTangent);
				v12_result = (v12_result + (v16_tp * v15_boneWeight));
			}
			_vcmpvTangent = v12_result;
			blkvs.vTangent = _vcmpvTangent;
			gl_Position = _vcmpprojCoord;



			//! end code
		}

	}
	fs
	{
		#version 450
		struct BoneTransform
		{
			mat4 transformMatrix;
			mat3 normalMatrix;
		};


		//$vertUV0$blkvs.vertUV0
		//$albedoMap$albedoMap
		//$vNormal$blkvs.vNormal
		//$vTangent$blkvs.vTangent
		//$vertUV$_vcmpvertUV
		//$vBiTangent$_vcmpvBiTangent
		//$normal$_vcmpnormal
		//$roughness$_vcmproughness
		//$metallic$_vcmpmetallic
		//$specular$_vcmpspecular
		//$albedo$_vcmpalbedo
		//$lightParam$_vcmplightParam
		//$arg2_transformedNormalnormal_in$_vcmparg2_transformedNormalnormal_in
		//$outputAlbedo$_vcmpoutputAlbedo
		//$outputPbr$_vcmpoutputPbr
		//$transformedNormalI_Inormal$_vcmptransformedNormalI_Inormal
		//$outputNormal$_vcmpoutputNormal
		//! input from perInstanceUniform
		layout(binding = 0) uniform sampler2D albedoMap;
		//! input from vs
		in vs
		{
			vec3 vNormal;
			vec3 vTangent;
			vec2 vertUV0;
		} blkvs;

		//! output declarations
		layout(location = 0) out vec3 outputAlbedo;
		layout(location = 1) out vec3 outputPbr;
		layout(location = 2) out vec3 outputNormal;

		//! global declarations


		//! end declarations
		void main()
		{
			//! local declarations
			vec2 _vcmpvertUV;
			vec3 _vcmpvBiTangent;
			vec3 _vcmpnormal;
			float _vcmproughness;
			float _vcmpmetallic;
			float _vcmpspecular;
			vec3 _vcmpalbedo;
			vec4 t9;
			vec3 ta;
			vec3 tb;
			vec3 _vcmplightParam;
			vec3 _vcmparg2_transformedNormalnormal_in;
			vec3 _vcmpoutputAlbedo;
			vec3 _vcmpoutputPbr;
			vec3 _vcmptransformedNormalI_Inormal;
			vec3 _vcmpoutputNormal;

			//! main code


			_vcmpvertUV = blkvs.vertUV0;
			_vcmpvBiTangent = cross(blkvs.vTangent, blkvs.vNormal);
			_vcmpnormal = vec3(0.000000000000e+00, 0.000000000000e+00, 1.000000000000e+00);
			_vcmproughness = 4.000000059605e-01;
			_vcmpmetallic = 4.000000059605e-01;
			_vcmpspecular = 1.000000000000e+00;
			t9 = texture(albedoMap, _vcmpvertUV);
			tb = ta;
			tb[0] = t9.x;
			tb[1] = t9.y;
			tb[2] = t9.z;
			ta = tb;
			_vcmpalbedo = ta;
			_vcmplightParam = vec3(_vcmproughness, _vcmpmetallic, _vcmpspecular);
			_vcmparg2_transformedNormalnormal_in = _vcmpnormal;
			_vcmpoutputAlbedo = _vcmpalbedo;
			outputAlbedo = _vcmpoutputAlbedo;
			_vcmpoutputPbr = _vcmplightParam;
			outputPbr = _vcmpoutputPbr;
			_vcmptransformedNormalI_Inormal = normalize((((_vcmparg2_transformedNormalnormal_in.x * blkvs.vTangent) + (_vcmparg2_transformedNormalnormal_in.y * _vcmpvBiTangent)) + (_vcmparg2_transformedNormalnormal_in.z * blkvs.vNormal)));
			_vcmpoutputNormal = _vcmptransformedNormalI_Inormal;
			outputNormal = _vcmpoutputNormal;



			//! end code
		}

	}
}
