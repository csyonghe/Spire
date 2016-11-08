name StaticMeshDeferredLighting
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
	comp VertexTransformI_Iposition;
	comp projCoord;
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
	comp vNormal;
	comp vTangent;
	comp vBiTangent;
	comp SurfacePatternI_Imask;
	comp SurfacePatternI_InormalCoord;
	comp normal;
	comp SurfacePatternI_IaoTex;
	comp SurfacePatternI_IspecTex;
	comp SurfacePatternI_IwearFactor;
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
interface rootVert size 32
{
	vec3 vertPos : 0,12;
	vec2 vertUV0 : 12,8;
	vec2 vertUV1 : 20,8;
	uint tangentFrame : 28,4;
}
interface modelTransform size 128
{
	mat4 modelMatrix : 0,64;
	mat4 normalMatrix : 64,64;
}
interface skeletalTransform size 0
{
}
interface viewUniform size 272
{
	mat4 viewTransform : 0,64;
	mat4 viewProjectionTransform : 64,64;
	mat4 invViewTransform : 128,64;
	mat4 invViewProjTransform : 192,64;
	vec3 cameraPos : 256,12;
}
interface perInstanceUniform size 48
{
	sampler2D maskMap : 0,8;
	sampler2D leatherNormalMap : 8,8;
	sampler2D baseNormalMap : 16,8;
	sampler2D aoMap : 24,8;
	sampler2D leatherSpecularMap : 32,8;
	sampler2D leatherMap : 40,8;
}
interface vs size 48
{
	vec2 vertUV0 : 0,8;
	vec3 vertNormal : 16,12;
	vec3 vertTangent : 32,12;
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
		//$modelMatrix$blkmodelTransform.modelMatrix
		//$viewProjectionTransform$blkviewUniform.viewProjectionTransform
		//$MeshVertexI_ItangentFrameQuaternion$_vcmpMeshVertexI_ItangentFrameQuaternion
		//$vertNormal$_vcmpvertNormal
		//$vertTangent$_vcmpvertTangent
		//$VertexTransformI_Iposition$_vcmpVertexTransformI_Iposition
		//$projCoord$_vcmpprojCoord
		//! input from viewUniform
		layout(std140, binding = 1) uniform viewUniform
		{
			mat4 viewTransform;
			mat4 viewProjectionTransform;
			mat4 invViewTransform;
			mat4 invViewProjTransform;
			vec3 cameraPos;
		} blkviewUniform;
		//! input from modelTransform
		layout(std140, binding = 0) uniform modelTransform
		{
			mat4 modelMatrix;
			mat4 normalMatrix;
		} blkmodelTransform;
		//! input from rootVert
		layout(location = 0) in vec3 vertPos;
		layout(location = 1) in vec2 vertUV0;
		layout(location = 2) in vec2 vertUV1;
		layout(location = 3) in uint tangentFrame;

		//! output declarations
		out vs
		{
			vec2 vertUV0;
			vec3 vertNormal;
			vec3 vertTangent;
		} blkvs;

		//! global declarations
		vec3 QuaternionRotate(vec4 p_q, vec3 p_pos);
		vec4 QuaternionMul(vec4 p_q1, vec4 p_q2);
		vec4 QuaternionConjugate(vec4 p_q);
		vec3 QuaternionRotate(vec4 p_q, vec3 p_pos)
		{
			vec4 t135;
			vec3 t136;
			vec3 t137;
			t135 = QuaternionMul(QuaternionMul(p_q, vec4(p_pos, 0.000000000000e+00)), QuaternionConjugate(p_q));
			t137 = t136;
			t137[0] = t135.x;
			t137[1] = t135.y;
			t137[2] = t135.z;
			t136 = t137;
			return t136;
		}
		vec4 QuaternionMul(vec4 p_q1, vec4 p_q2)
		{
			vec4 v0_rs;
			vec4 t150;
			vec4 t162;
			vec4 t174;
			vec4 t186;
			t150 = v0_rs;
			t150[0] = ((((p_q1.w * p_q2.x) + (p_q1.x * p_q2.w)) + (p_q1.y * p_q2.z)) - (p_q1.z * p_q2.y));
			v0_rs = t150;
			t162 = v0_rs;
			t162[1] = ((((p_q1.w * p_q2.y) + (p_q1.y * p_q2.w)) + (p_q1.z * p_q2.x)) - (p_q1.x * p_q2.z));
			v0_rs = t162;
			t174 = v0_rs;
			t174[2] = ((((p_q1.w * p_q2.z) + (p_q1.z * p_q2.w)) + (p_q1.x * p_q2.y)) - (p_q1.y * p_q2.x));
			v0_rs = t174;
			t186 = v0_rs;
			t186[3] = ((((p_q1.w * p_q2.w) - (p_q1.x * p_q2.x)) - (p_q1.y * p_q2.y)) - (p_q1.z * p_q2.z));
			v0_rs = t186;
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
			vec4 _vcmpVertexTransformI_Iposition;
			vec4 _vcmpprojCoord;

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
			blkvs.vertNormal = _vcmpvertNormal;
			_vcmpvertTangent = normalize(QuaternionRotate(_vcmpMeshVertexI_ItangentFrameQuaternion, vec3(1.000000000000e+00, 0.000000000000e+00, 0.000000000000e+00)));
			blkvs.vertTangent = _vcmpvertTangent;
			_vcmpVertexTransformI_Iposition = (blkmodelTransform.modelMatrix * vec4(vertPos, 1));
			_vcmpprojCoord = (blkviewUniform.viewProjectionTransform * _vcmpVertexTransformI_Iposition);
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
		//$normalMatrix$blkmodelTransform.normalMatrix
		//$maskMap$maskMap
		//$leatherNormalMap$leatherNormalMap
		//$baseNormalMap$baseNormalMap
		//$aoMap$aoMap
		//$leatherSpecularMap$leatherSpecularMap
		//$leatherMap$leatherMap
		//$vertNormal$blkvs.vertNormal
		//$vertTangent$blkvs.vertTangent
		//$vertUV$_vcmpvertUV
		//$vNormal$_vcmpvNormal
		//$vTangent$_vcmpvTangent
		//$vBiTangent$_vcmpvBiTangent
		//$SurfacePatternI_Imask$_vcmpSurfacePatternI_Imask
		//$SurfacePatternI_InormalCoord$_vcmpSurfacePatternI_InormalCoord
		//$normal$_vcmpnormal
		//$SurfacePatternI_IaoTex$_vcmpSurfacePatternI_IaoTex
		//$SurfacePatternI_IspecTex$_vcmpSurfacePatternI_IspecTex
		//$SurfacePatternI_IwearFactor$_vcmpSurfacePatternI_IwearFactor
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
		layout(binding = 0) uniform sampler2D maskMap;
		layout(binding = 1) uniform sampler2D leatherNormalMap;
		layout(binding = 2) uniform sampler2D baseNormalMap;
		layout(binding = 3) uniform sampler2D aoMap;
		layout(binding = 4) uniform sampler2D leatherSpecularMap;
		layout(binding = 5) uniform sampler2D leatherMap;
		//! input from modelTransform
		layout(std140, binding = 0) uniform modelTransform
		{
			mat4 modelMatrix;
			mat4 normalMatrix;
		} blkmodelTransform;
		//! input from vs
		in vs
		{
			vec2 vertUV0;
			vec3 vertNormal;
			vec3 vertTangent;
		} blkvs;

		//! output declarations
		layout(location = 0) out vec3 outputAlbedo;
		layout(location = 1) out vec3 outputPbr;
		layout(location = 2) out vec3 outputNormal;

		//! global declarations
		vec3 desaturate(vec3 p_color, float p_factor);
		float ComputeLuminance(vec3 p_color);
		vec3 desaturate(vec3 p_color, float p_factor)
		{
			float v1_lum;
			v1_lum = ComputeLuminance(p_color);
			return mix(p_color, vec3(v1_lum, v1_lum, v1_lum), p_factor);
		}
		float ComputeLuminance(vec3 p_color)
		{
			return (((p_color.x * 3.000000119209e-01) + (p_color.y * 5.899999737740e-01)) + (p_color.z * 1.099999994040e-01));
		}


		//! end declarations
		void main()
		{
			//! local declarations
			vec2 _vcmpvertUV;
			vec3 _vcmpvNormal;
			vec4 t6;
			vec3 t7;
			vec3 t8;
			vec3 _vcmpvTangent;
			vec4 t13;
			vec3 t14;
			vec3 t15;
			vec3 _vcmpvBiTangent;
			vec3 _vcmpSurfacePatternI_Imask;
			vec4 t24;
			vec3 t25;
			vec3 t26;
			vec2 _vcmpSurfacePatternI_InormalCoord;
			vec3 _vcmpnormal;
			vec2 v0_macroNormalCoord;
			vec3 v1_macroNormal;
			vec4 t36;
			vec3 t37;
			vec3 t38;
			vec3 v2_leatherNormal;
			vec4 t47;
			vec3 t48;
			vec3 t49;
			vec4 t5a;
			vec3 t5b;
			vec3 t5c;
			vec3 _vcmpSurfacePatternI_IaoTex;
			vec4 t6a;
			vec3 t6b;
			vec3 t6c;
			vec3 _vcmpSurfacePatternI_IspecTex;
			vec4 t76;
			vec3 t77;
			vec3 t78;
			float _vcmpSurfacePatternI_IwearFactor;
			float _vcmproughness;
			float _vcmpmetallic;
			float _vcmpspecular;
			vec3 _vcmpalbedo;
			float v3_ao;
			vec3 v4_Color1;
			float v5_Desaturation2;
			float v6_Desaturation2WearSpot;
			vec3 v7_Color2;
			vec3 v8_Color2WearSpot;
			vec3 v9_Color3;
			vec3 v10_SeamColor;
			vec4 ta3;
			vec3 ta4;
			vec3 ta5;
			vec3 _vcmplightParam;
			vec3 _vcmparg2_transformedNormalnormal_in;
			vec3 _vcmpoutputAlbedo;
			vec3 _vcmpoutputPbr;
			vec3 _vcmptransformedNormalI_Inormal;
			vec3 _vcmpoutputNormal;

			//! main code


			_vcmpvertUV = blkvs.vertUV0;
			t6 = (blkmodelTransform.normalMatrix * vec4(blkvs.vertNormal, 0.000000000000e+00));
			t8 = t7;
			t8[0] = t6.x;
			t8[1] = t6.y;
			t8[2] = t6.z;
			t7 = t8;
			_vcmpvNormal = t7;
			t13 = (blkmodelTransform.normalMatrix * vec4(blkvs.vertTangent, 0.000000000000e+00));
			t15 = t14;
			t15[0] = t13.x;
			t15[1] = t13.y;
			t15[2] = t13.z;
			t14 = t15;
			_vcmpvTangent = t14;
			_vcmpvBiTangent = cross(_vcmpvTangent, _vcmpvNormal);
			t24 = texture(maskMap, vec2(_vcmpvertUV.x, _vcmpvertUV.y));
			t26 = t25;
			t26[0] = t24.x;
			t26[1] = t24.y;
			t26[2] = t24.z;
			t25 = t26;
			_vcmpSurfacePatternI_Imask = t25;
			_vcmpSurfacePatternI_InormalCoord = (_vcmpvertUV * 5.789999961853e+00);
			v0_macroNormalCoord = (_vcmpvertUV * 3.720000088215e-01);
			t36 = texture(leatherNormalMap, v0_macroNormalCoord);
			t38 = t37;
			t38[0] = t36.x;
			t38[1] = t36.y;
			t38[2] = t36.z;
			t37 = t38;
			v1_macroNormal = (((t37 * 2.000000000000e+00) - vec3(1.000000000000e+00, 1.000000000000e+00, 1.000000000000e+00)) * vec3(2.739999890327e-01, 2.739999890327e-01, 0.000000000000e+00));
			t47 = texture(leatherNormalMap, _vcmpSurfacePatternI_InormalCoord);
			t49 = t48;
			t49[0] = t47.x;
			t49[1] = t47.y;
			t49[2] = t47.z;
			t48 = t49;
			v2_leatherNormal = (((t48 * 2.000000000000e+00) - vec3(1.000000000000e+00, 1.000000000000e+00, 1.000000000000e+00)) * vec3(1.000000000000e+00, 1.000000000000e+00, 0.000000000000e+00));
			t5a = texture(baseNormalMap, _vcmpvertUV);
			t5c = t5b;
			t5c[0] = t5a.x;
			t5c[1] = t5a.y;
			t5c[2] = t5a.z;
			t5b = t5c;
			_vcmpnormal = normalize((((t5b * 2.000000000000e+00) - vec3(1.000000000000e+00, 1.000000000000e+00, 1.000000000000e+00)) + ((v2_leatherNormal + v1_macroNormal) * _vcmpSurfacePatternI_Imask.x)));
			t6a = texture(aoMap, _vcmpvertUV);
			t6c = t6b;
			t6c[0] = t6a.x;
			t6c[1] = t6a.y;
			t6c[2] = t6a.z;
			t6b = t6c;
			_vcmpSurfacePatternI_IaoTex = t6b;
			t76 = texture(leatherSpecularMap, _vcmpSurfacePatternI_InormalCoord);
			t78 = t77;
			t78[0] = t76.x;
			t78[1] = t76.y;
			t78[2] = t76.z;
			t77 = t78;
			_vcmpSurfacePatternI_IspecTex = t77;
			_vcmpSurfacePatternI_IwearFactor = (_vcmpSurfacePatternI_Imask.z * 3.810000121593e-01);
			_vcmproughness = mix(mix(mix(2.000000029802e-01, mix(mix(6.589999794960e-01, 2.009999990463e+00, _vcmpSurfacePatternI_IspecTex.x), (-1.539999991655e-01), _vcmpSurfacePatternI_IwearFactor), _vcmpSurfacePatternI_Imask.x), 0.000000000000e+00, _vcmpSurfacePatternI_Imask.y), 0.000000000000e+00, _vcmpSurfacePatternI_IaoTex.y);
			_vcmpmetallic = mix(5.000000000000e-01, 1.000000014901e-01, _vcmpSurfacePatternI_IspecTex.x);
			_vcmpspecular = 1.000000000000e+00;
			v3_ao = _vcmpSurfacePatternI_IaoTex.x;
			v4_Color1 = vec3(0.000000000000e+00, 0.000000000000e+00, 0.000000000000e+00);
			v5_Desaturation2 = 0.000000000000e+00;
			v6_Desaturation2WearSpot = 8.959999680519e-02;
			v7_Color2 = vec3(1.000000000000e+00, 8.600000143051e-01, 8.330000042915e-01);
			v8_Color2WearSpot = vec3(6.280000209808e-01, 5.839999914169e-01, 5.839999914169e-01);
			v9_Color3 = vec3(8.230000138283e-01, 8.230000138283e-01, 8.230000138283e-01);
			v10_SeamColor = vec3(5.220000147820e-01, 2.700000107288e-01, 1.049999967217e-01);
			ta3 = texture(leatherMap, _vcmpSurfacePatternI_InormalCoord);
			ta5 = ta4;
			ta5[0] = ta3.x;
			ta5[1] = ta3.y;
			ta5[2] = ta3.z;
			ta4 = ta5;
			_vcmpalbedo = (mix(mix(mix(v4_Color1, (desaturate(ta4, mix(v5_Desaturation2, v6_Desaturation2WearSpot, _vcmpSurfacePatternI_IwearFactor)) * mix(v7_Color2, v8_Color2WearSpot, _vcmpSurfacePatternI_IwearFactor)), _vcmpSurfacePatternI_Imask.x), v9_Color3, _vcmpSurfacePatternI_Imask.y), v10_SeamColor, _vcmpSurfacePatternI_IaoTex.y) * v3_ao);
			_vcmplightParam = vec3(_vcmproughness, _vcmpmetallic, _vcmpspecular);
			_vcmparg2_transformedNormalnormal_in = _vcmpnormal;
			_vcmpoutputAlbedo = _vcmpalbedo;
			outputAlbedo = _vcmpoutputAlbedo;
			_vcmpoutputPbr = _vcmplightParam;
			outputPbr = _vcmpoutputPbr;
			_vcmptransformedNormalI_Inormal = normalize((((_vcmparg2_transformedNormalnormal_in.x * _vcmpvTangent) + (_vcmparg2_transformedNormalnormal_in.y * _vcmpvBiTangent)) + (_vcmparg2_transformedNormalnormal_in.z * _vcmpvNormal)));
			_vcmpoutputNormal = _vcmptransformedNormalI_Inormal;
			outputNormal = _vcmpoutputNormal;



			//! end code
		}

	}
}
