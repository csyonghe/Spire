#include "StdInclude.h"
#include "Syntax.h"

const char * LibIncludeString = R"(
__intrinsic float dFdx(float v);
__intrinsic float dFdy(float v);
__intrinsic float fwidth(float v);
__intrinsic vec2 dFdx(vec2 v);
__intrinsic vec2 dFdy(vec2 v);
__intrinsic vec2 fwidth(vec2 v);
__intrinsic vec3 dFdx(vec3 v);
__intrinsic vec3 dFdy(vec3 v);
__intrinsic vec3 fwidth(vec3 v);
__intrinsic vec4 dFdx(vec4 v);
__intrinsic vec4 dFdy(vec4 v);
__intrinsic vec4 fwidth(vec4 v);

__intrinsic float intBitsToFloat(int x);
__intrinsic int floatBitsToInt(float x);

__intrinsic vec3 normalize(vec3 v);
__intrinsic float dot(vec2 v0, vec2 v1);
__intrinsic float dot(vec3 v0, vec3 v1);
__intrinsic float dot(vec4 v0, vec4 v1);
__intrinsic float sin(float v);
__intrinsic float cos(float v);
__intrinsic float tan(float v);
__intrinsic float sqrt(float v);
__intrinsic vec2 sin(vec2 v);
__intrinsic vec2 cos(vec2 v);
__intrinsic vec2 tan(vec2 v);
__intrinsic vec2 sqrt(vec2 v);
__intrinsic vec3 sin(vec3 v);
__intrinsic vec3 cos(vec3 v);
__intrinsic vec3 tan(vec3 v);
__intrinsic vec3 sqrt(vec3 v);
__intrinsic vec4 sin(vec4 v);
__intrinsic vec4 cos(vec4 v);
__intrinsic vec4 tan(vec4 v);
__intrinsic vec4 sqrt(vec4 v);
__intrinsic float abs(float v);
__intrinsic vec2 abs(vec2 v);
__intrinsic vec3 abs(vec3 v);
__intrinsic vec4 abs(vec4 v);

__intrinsic float exp(float v);
__intrinsic vec2 exp(vec2 v);
__intrinsic vec3 exp(vec3 v);
__intrinsic vec4 exp(vec4 v);

__intrinsic float log(float v);
__intrinsic vec2 log(vec2 v);
__intrinsic vec3 log(vec3 v);
__intrinsic vec4 log(vec4 v);

__intrinsic float exp2(float v);
__intrinsic vec2 exp2(vec2 v);
__intrinsic vec3 exp2(vec3 v);
__intrinsic vec4 exp2(vec4 v);

__intrinsic float log2(float v);
__intrinsic vec2 log2(vec2 v);
__intrinsic vec3 log2(vec3 v);
__intrinsic vec4 log2(vec4 v);

__intrinsic float asin(float v);
__intrinsic vec2 asin(vec2 v);
__intrinsic vec3 asin(vec3 v);
__intrinsic vec4 asin(vec4 v);

__intrinsic float acos(float v);
__intrinsic vec2 acos(vec2 v);
__intrinsic vec3 acos(vec3 v);
__intrinsic vec4 acos(vec4 v);

__intrinsic float atan(float v);
__intrinsic vec2 atan(vec2 v);
__intrinsic vec3 atan(vec3 v);
__intrinsic vec4 atan(vec4 v);

__intrinsic float sign(float x);
__intrinsic vec2 sign(vec2 x);
__intrinsic vec3 sign(vec3 x);
__intrinsic vec4 sign(vec4 x);

__intrinsic float pow(float base, float e);
__intrinsic vec2 pow(vec2 base, vec2 e);
__intrinsic vec3 pow(vec3 base, vec3 e);
__intrinsic vec4 pow(vec4 base, vec4 e);
__intrinsic float atan2(float x, float y);
__intrinsic float floor(float v);
__intrinsic vec2 floor(vec2 v);
__intrinsic vec3 floor(vec3 v);
__intrinsic vec4 floor(vec4 v);
__intrinsic float fract(float v);
__intrinsic vec2 fract(vec2 v);
__intrinsic vec3 fract(vec3 v);
__intrinsic vec4 fract(vec4 v);
__intrinsic float ceil(float v);
__intrinsic vec2 ceil(vec2 v);
__intrinsic vec3 ceil(vec3 v);
__intrinsic vec4 ceil(vec4 v);
__intrinsic float step(float v, float y);
__intrinsic vec2 step(vec2 v, vec2 v1);
__intrinsic vec3 step(vec3 v, vec3 v1);
__intrinsic vec4 step(vec4 v, vec4 v1);
__intrinsic float smoothstep(float e0, float e1, float v);
__intrinsic vec2 smoothstep(vec2 e0, vec2 e1, vec2 v);
__intrinsic vec3 smoothstep(vec3 e0, vec3 e1, vec3 v);
__intrinsic vec4 smoothstep(vec4 e0, vec4 e1, vec4 v);
__intrinsic vec4 Sample(Texture2D tex, SamplerState sampler, vec2 uv);
__intrinsic vec4 Sample(Texture2D tex, SamplerState sampler, vec2 uv, ivec2 offset);
__intrinsic vec4 Sample(TextureCube tex, SamplerState sampler, vec3 uv);
__intrinsic vec4 Sample(Texture3D tex, SamplerState sampler, vec3 uv);
__intrinsic vec4 SampleLevel(Texture2D tex, SamplerState sampler, vec2 uv, float lod);
__intrinsic vec4 SampleLevel(TextureCube tex, SamplerState sampler, vec3 uv, float lod);
__intrinsic vec4 SampleLevel(Texture3D tex, SamplerState sampler, vec3 uv, float lod);
__intrinsic float SampleCmp(Texture2DShadow tex, SamplerComparisonState s, vec2 location, float compareValue, ivec2 offset);
__intrinsic float SampleCmp(Texture2DShadow tex, SamplerComparisonState s, vec2 location, float compareValue);
__intrinsic float SampleCmp(Texture2DArrayShadow tex, SamplerComparisonState s, vec3 location, float compareValue, ivec2 offset);
__intrinsic float SampleCmp(Texture2DArrayShadow tex, SamplerComparisonState s, vec3 location, float compareValue);
__intrinsic vec4 SampleGrad(Texture2D tex, SamplerState sampler, vec2 uv, vec2 ddx, vec2 ddy);
__intrinsic vec4 SampleGrad(Texture2D tex, SamplerState sampler, vec2 uv, vec2 ddx, vec2 ddy, ivec2 offset);
__intrinsic vec4 SampleGrad(TextureCube tex, SamplerState sampler, vec3 uv, vec3 ddx, vec3 ddy);
__intrinsic vec4 SampleBias(Texture2D tex, SamplerState sampler, vec2 uv, float bias);
__intrinsic vec4 SampleBias(Texture2D tex, SamplerState sampler, vec2 uv, float bias, ivec2 offset);
__intrinsic vec4 SampleBias(TextureCube tex, SamplerState sampler, vec3 uv, float bias);
__intrinsic float diff(float v);
__intrinsic float mod(float x, float y);
__intrinsic float max(float v);
__intrinsic float min(float v);
__intrinsic float max(float v, float v1);
__intrinsic float min(float v, float v1);
__intrinsic vec2 max(vec2 v, vec2 v1);
__intrinsic vec2 min(vec2 v, vec2 v1);
__intrinsic vec3 max(vec3 v, vec3 v1);
__intrinsic vec3 min(vec3 v, vec3 v1);
__intrinsic vec4 max(vec4 v, vec4 v1);
__intrinsic vec4 min(vec4 v, vec4 v1);
__intrinsic vec2 max(vec2 v, float v1);
__intrinsic vec2 min(vec2 v, float v1);
__intrinsic vec3 max(vec3 v, float v1);
__intrinsic vec3 min(vec3 v, float v1);
__intrinsic vec4 max(vec4 v, float v1);
__intrinsic vec4 min(vec4 v, float v1);
__intrinsic float clamp(float v, float v1, float v2);
__intrinsic vec2 clamp(vec2 v, vec2 v1, vec2 v2);
__intrinsic vec3 clamp(vec3 v, vec3 v1, vec3 v2);
__intrinsic vec4 clamp(vec4 v, vec4 v1, vec4 v2);
__intrinsic vec2 clamp(vec2 v, float v1, float v2);
__intrinsic vec3 clamp(vec3 v, float v1, float v2);
__intrinsic vec4 clamp(vec4 v, float v1, float v2);

__intrinsic vec3 reflect(vec3 I, vec3 N);
__intrinsic vec3 refract(vec3 I, vec3 N, float eta);

__intrinsic float length(vec2 v);
__intrinsic float length(vec3 v);
__intrinsic float length(vec4 v);

__intrinsic void alphaTest(float alpha, float threshold);
__intrinsic vec3 mix(vec3 v0, vec3 v1, float t);
__intrinsic vec4 mix(vec4 v0, vec4 v1, float t);
__intrinsic vec2 mix(vec2 v0, vec2 v1, float t);
__intrinsic float mix(float v0, float v1, float t);
__intrinsic vec3 mix(vec3 v0, vec3 v1, vec3 t);
__intrinsic vec4 mix(vec4 v0, vec4 v1, vec4 t);
__intrinsic vec2 mix(vec2 v0, vec2 v1, vec2 t);
__intrinsic mat3 mat3(vec3 a, vec3 b, vec3 c);
__intrinsic mat3 mat3(float a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8);
__intrinsic mat4 mat4(vec4 a, vec4 b, vec4 c, vec4 d);
__intrinsic mat4 mat4(float a0, float a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, float a12, float a13, float a14, float a15);
__intrinsic vec3 cross(vec3 v1, vec3 v2);
__intrinsic float float(float v);
__intrinsic int int(int v);
__intrinsic uint uint(uint v);
__intrinsic bool bool(bool v);
__intrinsic vec2 vec2(float v);
__intrinsic vec3 vec3(float v);
__intrinsic vec4 vec4(float v);
__intrinsic vec2 vec2(float x, float y);
__intrinsic vec3 vec3(float x, float y, float z);
__intrinsic vec3 vec3(vec2 v, float z);
__intrinsic vec4 vec4(float x, float y, float z, float w);
__intrinsic vec4 vec4(vec3 v, float w);
__intrinsic vec4 vec4(vec2 v, float z, float w);
__intrinsic vec4 vec4(vec2 v, vec2 w);
__intrinsic ivec2 ivec2(int x, int y);
__intrinsic ivec3 ivec3(int x, int y, int z);
__intrinsic ivec3 ivec3(ivec2 v, int z);
__intrinsic ivec4 ivec4(int x, int y, int z, int w);
__intrinsic ivec4 ivec4(ivec3 v, int w);
__intrinsic ivec4 ivec4(ivec2 v, int z, int w);
__intrinsic ivec4 ivec4(ivec2 v, ivec2 w);

__intrinsic uvec2 uvec2(uint x, uint y);
__intrinsic uvec3 uvec3(uint x, uint y, uint z);
__intrinsic uvec3 uvec3(uvec2 v, uint z);
__intrinsic uvec4 uvec4(uint x, uint y, uint z, uint w);
__intrinsic uvec4 uvec4(uvec3 v, uint w);
__intrinsic uvec4 uvec4(uvec2 v, uint z, uint w);
__intrinsic uvec4 uvec4(uvec2 v, uvec2 w);

__intrinsic int int(uint val);
__intrinsic int int(float val);
__intrinsic ivec2 ivec2(uvec2 val);
__intrinsic ivec2 ivec2(vec2 val);
__intrinsic ivec3 ivec3(uvec3 val);
__intrinsic ivec3 ivec3(vec3 val);
__intrinsic ivec4 ivec4(uvec4 val);
__intrinsic ivec4 ivec4(vec4 val);

__intrinsic uint uint(int val);
__intrinsic uint uint(float val);
__intrinsic uvec2 uvec2(ivec2 val);
__intrinsic uvec2 uvec2(vec2 val);
__intrinsic uvec3 uvec3(ivec3 val);
__intrinsic uvec3 uvec3(vec3 val);
__intrinsic uvec4 uvec4(ivec4 val);
__intrinsic uvec4 uvec4(vec4 val);

__intrinsic float float(int val);
__intrinsic float float(uint val);
__intrinsic vec2 vec2(ivec2 val);
__intrinsic vec2 vec2(uvec2 val);
__intrinsic vec3 vec3(ivec3 val);
__intrinsic vec3 vec3(uvec3 val);
__intrinsic vec4 vec4(ivec4 val);
__intrinsic vec4 vec4(uvec4 val);

__intrinsic mat3 transpose(mat3 in);
__intrinsic mat4 transpose(mat4 in);
__intrinsic mat3 mat3(mat4 in);

struct trait __intrinsic {};
__intrinsic trait IsTriviallyPassable(float);
__intrinsic trait IsTriviallyPassable(vec2);
__intrinsic trait IsTriviallyPassable(vec3);
__intrinsic trait IsTriviallyPassable(vec4);
__intrinsic trait IsTriviallyPassable(mat3);
__intrinsic trait IsTriviallyPassable(mat4);
__intrinsic trait IsTriviallyPassable(int);
__intrinsic trait IsTriviallyPassable(ivec2);
__intrinsic trait IsTriviallyPassable(ivec3);
__intrinsic trait IsTriviallyPassable(ivec4);
__intrinsic trait IsTriviallyPassable(uint);
__intrinsic trait IsTriviallyPassable(uvec2);
__intrinsic trait IsTriviallyPassable(uvec3);
__intrinsic trait IsTriviallyPassable(uvec4);
__intrinsic trait IsTriviallyPassable(bool);
#line default
)";

using namespace CoreLib::Basic;

namespace Spire
{
	namespace Compiler
	{
		String SpireStdLib::code;

		String SpireStdLib::GetCode()
		{
			if (code.Length() > 0)
				return code;
			StringBuilder sb;
			// generate operator overloads
			Operator floatUnaryOps[] = { Operator::Neg, Operator::Not, Operator::PreInc, Operator::PreDec };
			Operator intUnaryOps[] = { Operator::Neg, Operator::Not, Operator::BitNot, Operator::PreInc, Operator::PreDec};
			Operator floatOps[] = { Operator::Mul, Operator::Div,
				Operator::Add, Operator::Sub, Operator::And, Operator::Or,
				Operator::Eql, Operator::Neq, Operator::Greater, Operator::Less, Operator::Geq, Operator::Leq };
			Operator intOps[] = {  Operator::Mul, Operator::Div, Operator::Mod,
				Operator::Add, Operator::Sub,
				Operator::Lsh, Operator::Rsh,
				Operator::Eql, Operator::Neq, Operator::Greater, Operator::Less, Operator::Geq, Operator::Leq,
				Operator::BitAnd, Operator::BitXor, Operator::BitOr,
				Operator::And,
				Operator::Or };
			String floatTypes[] = { "float", "vec2", "vec3", "vec4" };
			String intTypes[] = { "int", "ivec2", "ivec3", "ivec4" };
			String uintTypes[] = { "uint", "uvec2", "uvec3", "uvec4" };

			sb << "__intrinsic vec3 operator * (vec3, mat3);\n";
			sb << "__intrinsic vec3 operator * (mat3, vec3);\n";

			sb << "__intrinsic vec4 operator * (vec4, mat4);\n";
			sb << "__intrinsic vec4 operator * (mat4, vec4);\n";

			sb << "__intrinsic mat3 operator * (mat3, mat3);\n";
			sb << "__intrinsic mat4 operator * (mat4, mat4);\n";

			sb << "__intrinsic bool operator && (bool, bool);\n";
			sb << "__intrinsic bool operator || (bool, bool);\n";

			for (auto type : intTypes)
			{
				sb << "__intrinsic bool operator && (bool, " << type << ");\n";
				sb << "__intrinsic bool operator || (bool, " << type << ");\n";
				sb << "__intrinsic bool operator && (" << type << ", bool);\n";
				sb << "__intrinsic bool operator || (" << type << ", bool);\n";
			}

			for (auto op : intUnaryOps)
			{
				String opName = GetOperatorFunctionName(op);
				for (int i = 0; i < 4; i++)
				{
					auto itype = intTypes[i];
					auto utype = uintTypes[i];
					for (int j = 0; j < 2; j++)
					{
						auto retType = (op == Operator::Not) ? "bool" : j == 0 ? itype : utype;
						sb << "__intrinsic " << retType << " operator " << opName << "(" << (j == 0 ? itype : utype) << ");\n";
					}
				}
			}

			for (auto op : floatUnaryOps)
			{
				String opName = GetOperatorFunctionName(op);
				for (int i = 0; i < 4; i++)
				{
					auto type = floatTypes[i];
					auto retType = (op == Operator::Not) ? "bool" : type;
					sb << "__intrinsic " << retType << " operator " << opName << "(" << type << ");\n";
				}
			}

			for (auto op : floatOps)
			{
				String opName = GetOperatorFunctionName(op);
				for (int i = 0; i < 4; i++)
				{
					auto type = floatTypes[i];
					auto itype = intTypes[i];
					auto utype = uintTypes[i];
					auto retType = ((op >= Operator::Eql && op <= Operator::Leq) || op == Operator::And || op == Operator::Or) ? "bool" : type;
					sb << "__intrinsic " << retType << " operator " << opName << "(" << type << ", " << type << ");\n";
					sb << "__intrinsic " << retType << " operator " << opName << "(" << itype << ", " << type << ");\n";
					sb << "__intrinsic " << retType << " operator " << opName << "(" << utype << ", " << type << ");\n";
					sb << "__intrinsic " << retType << " operator " << opName << "(" << type << ", " << itype << ");\n";
					sb << "__intrinsic " << retType << " operator " << opName << "(" << type << ", " << utype << ");\n";
					if (i > 0)
					{
						sb << "__intrinsic " << retType << " operator " << opName << "(" << type << ", " << floatTypes[0] << ");\n";
						sb << "__intrinsic " << retType << " operator " << opName << "(" << floatTypes[0] << ", " << type << ");\n";

						sb << "__intrinsic " << retType << " operator " << opName << "(" << type << ", " << intTypes[0] << ");\n";
						sb << "__intrinsic " << retType << " operator " << opName << "(" << intTypes[0] << ", " << type << ");\n";

						sb << "__intrinsic " << retType << " operator " << opName << "(" << type << ", " << uintTypes[0] << ");\n";
						sb << "__intrinsic " << retType << " operator " << opName << "(" << uintTypes[0] << ", " << type << ");\n";
					}
				}
			}

			for (auto op : intOps)
			{
				String opName = GetOperatorFunctionName(op);
				for (int i = 0; i < 4; i++)
				{
					auto type = intTypes[i];
					auto utype = uintTypes[i];
					auto retType = ((op >= Operator::Eql && op <= Operator::Leq) || op == Operator::And || op == Operator::Or) ? "bool" : type;
					sb << "__intrinsic " << retType << " operator " << opName << "(" << type << ", " << type << ");\n";
					sb << "__intrinsic " << retType << " operator " << opName << "(" << utype << ", " << type << ");\n";
					sb << "__intrinsic " << retType << " operator " << opName << "(" << type << ", " << utype << ");\n";
					sb << "__intrinsic " << retType << " operator " << opName << "(" << utype << ", " << utype << ");\n";
					if (i > 0)
					{
						sb << "__intrinsic " << retType << " operator " << opName << "(" << type << ", " << intTypes[0] << ");\n";
						sb << "__intrinsic " << retType << " operator " << opName << "(" << intTypes[0] << ", " << type << ");\n";

						sb << "__intrinsic " << retType << " operator " << opName << "(" << type << ", " << uintTypes[0] << ");\n";
						sb << "__intrinsic " << retType << " operator " << opName << "(" << uintTypes[0] << ", " << type << ");\n";
					}
				}
			}
			sb << LibIncludeString;
			code = sb.ProduceString();
			return code;
		}

		void SpireStdLib::Finalize()
		{
			code = nullptr;
		}

	}
}

