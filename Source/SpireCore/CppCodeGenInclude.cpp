#include "CppCodeGenInclude.h"

const char * CppCodeIncludeString1 = R"(
#ifndef SHADER_RUNTIME_H
#define SHADER_RUNTIME_H
#include <math.h>
#include <vector>
template<typename T, int dim>
class Vec
{
public:
	T vals[dim];
	Vec() = default;
	Vec(const Vec<T, dim> & other) = default;
	inline Vec(T val)
	{
		for (int i = 0; i < dim; i++)
			vals[i] = val;
	}
	inline Vec(T x, T y)
	{
		vals[0] = x;
		vals[1] = y;
	}
	inline Vec(T x, T y, T z)
	{
		vals[0] = x;
		vals[1] = y;
		vals[2] = z;
	}
	inline Vec(T x, T y, T z, T w)
	{
		vals[0] = x;
		vals[1] = y;
		vals[2] = z;
		vals[3] = w;
	}
	inline T & x()
	{
		return vals[0];
	}
	inline T & y()
	{
		return vals[1];
	}
	inline T & z()
	{
		return vals[2];
	}
	inline T & w()
	{
		return vals[3];
	}
	inline T x() const
	{
		return vals[0];
	}
	inline T y() const
	{
		return vals[1];
	}
	inline T z() const
	{
		return vals[2];
	}
	inline T w() const
	{
		return vals[3];
	}
	inline Vec<T, 2> xy() const
	{
		return Vec<T, 2>(vals[0], vals[1]);
	}
	inline Vec<T, 3> xyz() const
	{
		return Vec<T, 3>(vals[0], vals[1], vals[2]);
	}
	inline Vec<T, dim> operator +(const Vec<T, dim> & other) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] + other.vals[i];
		return rs;
	}
	inline Vec<T, dim> operator -() const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = -vals[i];
		return rs;
	}
	inline Vec<int, dim> operator !() const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] != 0;
		return rs;
	}
	inline Vec<T, dim> operator -(const Vec<T, dim> & other) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] - other.vals[i];
		return rs;
	}
	inline Vec<T, dim> operator *(T s) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] * s;
		return rs;
	}
	inline Vec<T, dim> operator *(const Vec<T, dim> & other) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] * other.vals[i];
		return rs;
	}
	inline Vec<T, dim> operator /(const Vec<T, dim> & other) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] / other.vals[i];
		return rs;
	}
	inline Vec<T, dim> operator %(const Vec<T, dim> & other) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] % other.vals[i];
		return rs;
	}
	inline Vec<int, dim> operator >(const Vec<T, dim> & other) const
	{
		Vec<int, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] > other.vals[i]>1:0;
		return rs;
	}
	inline Vec<int, dim> operator <(const Vec<T, dim> & other) const
	{
		Vec<int, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] < other.vals[i]>1:0;
		return rs;
	}
	inline Vec<int, dim> operator >=(const Vec<T, dim> & other) const
	{
		Vec<int, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] >= other.vals[i]>1:0;
		return rs;
	}
	inline Vec<int, dim> operator <=(const Vec<T, dim> & other) const
	{
		Vec<int, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] <= other.vals[i]>1:0;
		return rs;
	}
	inline Vec<int, dim> operator ==(const Vec<T, dim> & other) const
	{
		Vec<int, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] == other.vals[i]>1:0;
		return rs;
	}
	inline Vec<int, dim> operator !=(const Vec<T, dim> & other) const
	{
		Vec<int, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = vals[i] != other.vals[i]>1:0;
		return rs;
	}
	inline const T & operator [](int i) const
	{
		return vals[i];
	}
	inline T & operator [](int i)
	{
		return vals[i];
	}
	template<typename Func>
	inline Vec<T, dim> map(const Func & f) const
	{
		Vec<T, dim> rs;
		for (int i = 0; i < dim; i++)
			rs.vals[i] = f(rs.vals[i]);
		return rs;
	}
};

typedef Vec<float, 2> vec2;
typedef Vec<float, 3> vec3;
typedef Vec<float, 4> vec4;
typedef Vec<int, 2> ivec2;
typedef Vec<int, 3> ivec3;
typedef Vec<int, 4> ivec4;

inline vec4 make_vec4(float v)
{
	vec4 rs;
	rs[0] = v;
	return rs;
}

inline vec4 make_vec4(vec2 v)
{
	vec4 rs;
	rs[0] = v[0];
	rs[1] = v[1];
	return rs;
}

inline vec4 make_vec4(vec3 v)
{
	vec4 rs;
	rs[0] = v[0];
	rs[1] = v[1];
	rs[2] = v[2];
	return rs;
}

inline vec4 make_vec4(vec4 v)
{
	return v;
}

class mat3 : public Vec<float, 9>
{
public:
	mat3() = default;
	mat3(vec3 x, vec3 y, vec3 z)
	{
		vals[0] = x.vals[0];
		vals[1] = x.vals[1];
		vals[2] = x.vals[2];
		vals[3] = y.vals[0];
		vals[4] = y.vals[1];
		vals[5] = y.vals[2];
		vals[6] = z.vals[0];
		vals[7] = z.vals[1];
		vals[8] = z.vals[2];
	}
	mat3(float v0, float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8)
	{
		vals[0] = v0;
		vals[1] = v1;
		vals[2] = v2;
		vals[3] = v3;
		vals[4] = v4;
		vals[5] = v5;
		vals[6] = v6;
		vals[7] = v7;
		vals[8] = v8;
	}
	inline mat3 operator * (const mat3 & other) const
	{
		mat3 result;
		for (int i = 0; i < 3; i++)
			for (int j = 0; j < 3; j++)
			{
				float dot = 0.0f;
				for (int k = 0; k < 3; k++)
					dot += vals[k * 3 + j] * other.vals[i * 3 + k];
				result.vals[i * 3 + j] = dot;
			}
		return result;
	}
	inline vec3 operator * (const vec3 & other) const
	{
		vec3 result;
		for (int j = 0; j < 3; j++)
		{
			float dot = 0.0f;
			for (int k = 0; k < 3; k++)
				dot += vals[k * 3 + j] * other.vals[k];
			result.vals[j] = dot;
		}
		return result;
	}
	inline vec3 & operator [](int i)
	{
		return *((vec3*)(void*)this + i);
	}
	inline vec3 operator [](int i) const
	{
		return *((vec3*)(void*)this + i);
	}
	inline vec3 x() const
	{
		return this->operator[](0);
	}
	inline vec3 y() const
	{
		return this->operator[](1);
	}
	inline vec3 z() const
	{
		return this->operator[](2);
	}
	inline vec3 & x()
	{
		return this->operator[](0);
	}
	inline vec3 & y()
	{
		return this->operator[](1);
	}
	inline vec3 & z()
	{
		return this->operator[](2);
	}
};

class mat4 : public Vec<float, 16>
{
public:
	mat4() = default;
	mat4(vec4 x, vec4 y, vec4 z, vec4 w)
	{
		vals[0] = x.vals[0];
		vals[1] = x.vals[1];
		vals[2] = x.vals[2];
		vals[3] = x.vals[3];

		vals[4] = y.vals[0];
		vals[5] = y.vals[1];
		vals[6] = y.vals[2];
		vals[7] = y.vals[3];

		vals[8] = z.vals[0];
		vals[9] = z.vals[1];
		vals[10] = z.vals[2];
		vals[11] = z.vals[3];

		vals[12] = w.vals[0];
		vals[13] = w.vals[1];
		vals[14] = w.vals[2];
		vals[15] = w.vals[3];
	}
	mat4(float v0, float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8, float v9, float v10, float v11, float v12, float v13, float v14, float v15)
	{
		vals[0] = v0;
		vals[1] = v1;
		vals[2] = v2;
		vals[3] = v3;
		vals[4] = v4;
		vals[5] = v5;
		vals[6] = v6;
		vals[7] = v7;
		vals[8] = v8;
		vals[9] = v9;
		vals[10] = v10;
		vals[11] = v11;
		vals[12] = v12;
		vals[13] = v13;
		vals[14] = v14;
		vals[15] = v15;
	}
	inline vec4 operator [](int i) const
	{
		return *((vec4*)(void*)this + i);
	}
	inline vec4 & operator [](int i)
	{
		return *((vec4*)(void*)this + i);
	}
	inline vec4 x() const
	{
		return this->operator[](0);
	}
	inline vec4 y() const
	{
		return this->operator[](1);
	}
	inline vec4 z() const
	{
		return this->operator[](2);
	}
	inline vec4 w() const
	{
		return this->operator[](3);
	}
	inline vec4 & x()
	{
		return this->operator[](0);
	}
	inline vec4 & y()
	{
		return this->operator[](1);
	}
	inline vec4 & z()
	{
		return this->operator[](2);
	}
	inline vec4 & w()
	{
		return this->operator[](32);
	}
	inline mat4 operator * (const mat4 & other) const
	{
		mat4 result;
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
			{
				float dot = 0.0f;
				for (int k = 0; k < 4; k++)
					dot += vals[k * 4 + j] * other.vals[i * 4 + k];
				result.vals[i * 4 + j] = dot;
			}
		return result;
	}
	inline vec4 operator * (const vec4 & other) const
	{
		vec4 result;
		for (int j = 0; j < 4; j++)
		{
			float dot = 0.0f;
			for (int k = 0; k < 4; k++)
				dot += vals[k * 4 + j] * other.vals[k];
			result.vals[j] = dot;
		}
		return result;
	}
};

inline mat3 transpose(mat3 m)
{
	mat3 rs;
	rs.vals[0] = m.vals[0];
	rs.vals[1] = m.vals[3];
	rs.vals[2] = m.vals[6];

	rs.vals[3] = m.vals[1];
	rs.vals[4] = m.vals[4];
	rs.vals[5] = m.vals[7];

	rs.vals[6] = m.vals[2];
	rs.vals[7] = m.vals[5];
	rs.vals[8] = m.vals[8];
	return rs;
}

inline mat4 transpose(mat4 m)
{
	mat4 rs;
	rs.vals[0] = m.vals[0];
	rs.vals[1] = m.vals[4];
	rs.vals[2] = m.vals[8];
	rs.vals[3] = m.vals[12];

	rs.vals[4] = m.vals[1];
	rs.vals[5] = m.vals[5];
	rs.vals[6] = m.vals[9];
	rs.vals[7] = m.vals[13];

	rs.vals[8] = m.vals[2];
	rs.vals[9] = m.vals[6];
	rs.vals[10] = m.vals[10];
	rs.vals[11] = m.vals[14];

	rs.vals[12] = m.vals[3];
	rs.vals[13] = m.vals[7];
	rs.vals[14] = m.vals[11];
	rs.vals[15] = m.vals[15];
	return rs;
}

inline mat4 inverse(mat4 m)
{
	mat4 rs;
	double Result[4][4];
	double tmp[12];
	double src[16];
	double det;
	for (int i = 0; i < 4; i++)
	{
		src[i + 0] = m[i][0];
		src[i + 4] = m[i][1];
		src[i + 8] = m[i][2];
		src[i + 12] = m[i][3];
	}
	tmp[0] = src[10] * src[15];
	tmp[1] = src[11] * src[14];
	tmp[2] = src[9] * src[15];
	tmp[3] = src[11] * src[13];
	tmp[4] = src[9] * src[14];
	tmp[5] = src[10] * src[13];
	tmp[6] = src[8] * src[15];
	tmp[7] = src[11] * src[12];
	tmp[8] = src[8] * src[14];
	tmp[9] = src[10] * src[12];
	tmp[10] = src[8] * src[13];
	tmp[11] = src[9] * src[12];
	Result[0][0] = tmp[0] * src[5] + tmp[3] * src[6] + tmp[4] * src[7];
	Result[0][0] -= tmp[1] * src[5] + tmp[2] * src[6] + tmp[5] * src[7];
	Result[0][1] = tmp[1] * src[4] + tmp[6] * src[6] + tmp[9] * src[7];
	Result[0][1] -= tmp[0] * src[4] + tmp[7] * src[6] + tmp[8] * src[7];
	Result[0][2] = tmp[2] * src[4] + tmp[7] * src[5] + tmp[10] * src[7];
	Result[0][2] -= tmp[3] * src[4] + tmp[6] * src[5] + tmp[11] * src[7];
	Result[0][3] = tmp[5] * src[4] + tmp[8] * src[5] + tmp[11] * src[6];
	Result[0][3] -= tmp[4] * src[4] + tmp[9] * src[5] + tmp[10] * src[6];
	Result[1][0] = tmp[1] * src[1] + tmp[2] * src[2] + tmp[5] * src[3];
	Result[1][0] -= tmp[0] * src[1] + tmp[3] * src[2] + tmp[4] * src[3];
	Result[1][1] = tmp[0] * src[0] + tmp[7] * src[2] + tmp[8] * src[3];
	Result[1][1] -= tmp[1] * src[0] + tmp[6] * src[2] + tmp[9] * src[3];
	Result[1][2] = tmp[3] * src[0] + tmp[6] * src[1] + tmp[11] * src[3];
	Result[1][2] -= tmp[2] * src[0] + tmp[7] * src[1] + tmp[10] * src[3];
	Result[1][3] = tmp[4] * src[0] + tmp[9] * src[1] + tmp[10] * src[2];
	Result[1][3] -= tmp[5] * src[0] + tmp[8] * src[1] + tmp[11] * src[2];
	tmp[0] = src[2] * src[7];
	tmp[1] = src[3] * src[6];
	tmp[2] = src[1] * src[7];
	tmp[3] = src[3] * src[5];
	tmp[4] = src[1] * src[6];
	tmp[5] = src[2] * src[5];
	tmp[6] = src[0] * src[7];
	tmp[7] = src[3] * src[4];
	tmp[8] = src[0] * src[6];
	tmp[9] = src[2] * src[4];
	tmp[10] = src[0] * src[5];
	tmp[11] = src[1] * src[4];
	Result[2][0] = tmp[0] * src[13] + tmp[3] * src[14] + tmp[4] * src[15];
	Result[2][0] -= tmp[1] * src[13] + tmp[2] * src[14] + tmp[5] * src[15];
	Result[2][1] = tmp[1] * src[12] + tmp[6] * src[14] + tmp[9] * src[15];
	Result[2][1] -= tmp[0] * src[12] + tmp[7] * src[14] + tmp[8] * src[15];
	Result[2][2] = tmp[2] * src[12] + tmp[7] * src[13] + tmp[10] * src[15];
	Result[2][2] -= tmp[3] * src[12] + tmp[6] * src[13] + tmp[11] * src[15];
	Result[2][3] = tmp[5] * src[12] + tmp[8] * src[13] + tmp[11] * src[14];
	Result[2][3] -= tmp[4] * src[12] + tmp[9] * src[13] + tmp[10] * src[14];
	Result[3][0] = tmp[2] * src[10] + tmp[5] * src[11] + tmp[1] * src[9];
	Result[3][0] -= tmp[4] * src[11] + tmp[0] * src[9] + tmp[3] * src[10];
	Result[3][1] = tmp[8] * src[11] + tmp[0] * src[8] + tmp[7] * src[10];
	Result[3][1] -= tmp[6] * src[10] + tmp[9] * src[11] + tmp[1] * src[8];
	Result[3][2] = tmp[6] * src[9] + tmp[11] * src[11] + tmp[3] * src[8];
	Result[3][2] -= tmp[10] * src[11] + tmp[2] * src[8] + tmp[7] * src[9];
	Result[3][3] = tmp[10] * src[10] + tmp[4] * src[8] + tmp[9] * src[9];
	Result[3][3] -= tmp[8] * src[9] + tmp[11] * src[10] + tmp[5] * src[8];
	det = src[0] * Result[0][0] + src[1] * Result[0][1] + src[2] * Result[0][2] + src[3] * Result[0][3];
	det = 1.0f / det;
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			rs.vals[i * 4 + j] = (float)(Result[i][j] * det);
		}
	}
	return rs;
}
)";
const char * CppCodeIncludeString2 = R"(
template<typename T, int dim>
inline T dot(Vec<T, dim> v0, Vec<T, dim> v1)
{
	T rs = 0;
	for (int i = 0; i < dim; i++)
	{
		rs += v0.vals[i] * v1.vals[i];
	}
	return rs;
}

template<int dim>
inline Vec<float, dim> normalize(Vec<float, dim> v0)
{
	Vec<float, dim> rs;
	float length = 0.0f;
	for (int i = 0; i < dim; i++)
	{
		length += v0.vals[i] * v0.vals[i];
	}
	length = 1.0f / sqrt(length);
	for (int i = 0; i < dim; i++)
		rs.vals[i] *= length;
	return rs;
}

template<typename T, int dim>
inline float length(const Vec<T, dim> & v0)
{
	float length = 0.0f;
	for (int i = 0; i < dim; i++)
	{
		length += v0.vals[i] * v0.vals[i];
	}
	return length;
}

template<typename T, int dim>
inline float distance(const Vec<T, dim> & v0, const Vec<T, dim> & v1)
{
	float length = 0.0f;
	for (int i = 0; i < dim; i++)
	{
		float diff = (v0.vals[i] - v1.vals[i]);
		length += diff * diff;
	}
	return length;
}

template<int dim>
inline Vec<float, dim> sin(const Vec<float, dim> & x)
{
	return x.map(sinf);
}
template<int dim>
Vec<float, dim> sinh(const Vec<float, dim> & x)
{
	return x.map(sinhf);
}
template<int dim>
inline Vec<float, dim> cos(const Vec<float, dim> & x)
{
	return x.map(cosf);
}
template<int dim>
inline Vec<float, dim> sqrt(const Vec<float, dim> & x)
{
	return x.map(sqrtf);
}
template<int dim>
inline Vec<float, dim> log(const Vec<float, dim> & x)
{
	return x.map(logf);
}
template<int dim>
inline Vec<float, dim> log2(const Vec<float, dim> & x)
{
	return x.map(log2f);
}
template<int dim>
inline Vec<float, dim> exp(const Vec<float, dim> & x)
{
	return x.map(expf);
}
template<int dim>
inline Vec<float, dim> tan(const Vec<float, dim> & x)
{
	return x.map(tanf);
}
template<int dim>
inline Vec<float, dim> tanh(const Vec<float, dim> & x)
{
	return x.map(tanhf);
}
template<int dim>
Vec<float, dim> asin(const Vec<float, dim> & x)
{
	return x.map(asinf);
}
template<int dim>
Vec<float, dim> acos(const Vec<float, dim> & x)
{
	return x.map(acosf);
}
template<int dim>
Vec<float, dim> atan(const Vec<float, dim> & x)
{
	return x.map(atanf);
}
template<int dim>
Vec<float, dim> floor(const Vec<float, dim> & x)
{
	return x.map(floorf);
}
template<int dim>
Vec<float, dim> ceil(const Vec<float, dim> & x)
{
	return x.map(ceilf);
}
template<int dim>
Vec<float, dim> round(const Vec<float, dim> & x)
{
	return x.map(roundf);
}
template<int dim>
Vec<float, dim> abs(const Vec<float, dim> & x)
{
	return x.map(fabsf);
}
template<int dim>
Vec<float, dim> fract(const Vec<float, dim> & x)
{
	return x - floor(x);
}
template<int dim>
Vec<float, dim> exp2(const Vec<float, dim> & x)
{
	return x.map(exp2f);
}
template<int dim>
Vec<float, dim> pow(const Vec<float, dim> & x, const Vec<float, dim> & y)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = powf(x.vals[i], y.vals[i]);
	return rs;
}
template<int dim>
Vec<float, dim> atan2(const Vec<float, dim> & x, const Vec<float, dim> & y)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = atan2(x.vals[i], y.vals[i]);
	return rs;
}

template<int dim>
Vec<float, dim> min(const Vec<float, dim> & x, const Vec<float, dim> & y)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = x.vals[i] < y.vals[i] ? x.vals[i] : y.vals[i];
	return rs;
}

template<int dim>
Vec<float, dim> max(const Vec<float, dim> & x, const Vec<float, dim> & y)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = x.vals[i] > y.vals[i] ? x.vals[i] : y.vals[i];
	return rs;
}

inline float min(float a, float b)
{
	return a < b ? a : b;
}

inline float max(float a, float b)
{
	return a > b ? a : b;
}

inline float clamp(float a, float v0, float v1)
{
	if (a < v0) return v0; else if (a>v1) return v1; else return a;
}

template<int dim>
inline Vec<float, dim> clamp(const Vec<float, dim> & a, const Vec<float, dim> & v0, const Vec<float, dim> & v1)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = clamp(a.vals[i], v0.vals[i], v1.vals[i]);
	return rs;
}
inline float radians(float degrees)
{
	return (3.141592653f / 180.0f) * degrees;
}

inline float mix(float a, float b, float t)
{
	return a*(1.0f - t) + b*t;
}

template<int dim>
inline Vec<float, dim> mix(const Vec<float, dim> & a, const Vec<float, dim> &  b, float t)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = mix(a.vals[i], b.vals[i], t);
	return rs;
}

template<int dim>
inline Vec<float, dim> mix(const Vec<float, dim> & a, const Vec<float, dim> &  b, const Vec<float, dim> & t)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = mix(a.vals[i], b.vals[i], t.vals[i]);
	return rs;
}

template<typename T, int dim>
inline Vec<T, dim> reflect(const Vec<T, dim> & I, const Vec<T, dim> & N)
{
	return I - N * (2 * dot(I, N));
}

template<int dim>
inline Vec<float, dim> reflect(const Vec<float, dim> & I, const Vec<float, dim> & N, float eta)
{
	float d = dot(N, I);
	float k = 1.0f - eta*eta *(1.0f - d*d);
	if (k < 0.0)
		return Vec<float, dim>(0.0f);
	else
		return I * eta - N * (eta * d + sqrt(k));
}

inline float sign(float x)
{
	if (x < 0.0f)
		return -1.0f;
	else
		return 1.0f;
}

template<int dim>
Vec<float, dim> sign(const Vec<float, dim> & x)
{
	return x.map([](float v) {return sign(v); });
}

inline float step(float edge, float x)
{
	if (x < edge)
		return 0.0f;
	else
		return 1.0f;
}


template<int dim>
inline Vec<float, dim> step(float edge, const Vec<float, dim> & x)
{
	return x.map([](float v) {return step(edge, v); });
}

template<int dim>
inline Vec<float, dim> step(const Vec<float, dim> & edge, const Vec<float, dim> & x)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = step(edge.vals[i], x.vals[i]);
	return rs;
}

inline float smoothstep(float edge0, float edge1, float x)
{
	float t = clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
	return (float)(t*t*(3.0f - 2.0*t));
}

template<int dim>
Vec<float, dim> smoothstep(const Vec<float, dim> & edge0, const Vec<float, dim> & edge1, const Vec<float, dim> & x)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = smoothstep(edge0.vals[i], edge1.vals[i], x.vals[i]);
	return rs;
}

template<int dim>
Vec<float, dim> smoothstep(float edge0, float edge1, const Vec<float, dim> & x)
{
	Vec<float, dim> rs;
	for (int i = 0; i < dim; i++)
		rs.vals[i] = smoothstep(edge0, edge1, x.vals[i]);
	return rs;
}

class ITexture2D
{
public:
	virtual vec4 GetValue(vec2 uv, vec2 ddx, vec2 ddy) = 0;
};

typedef ITexture2D* sampler2D;
typedef ITexture2D* sampler2DShadow;

inline vec4 texture(sampler2D sampler, vec2 uv)
{
	return sampler->GetValue(uv, vec2(0.0f, 0.0f), vec2(0.0f, 0.0f));
}

inline vec4 texture(sampler2D sampler, vec2 uv, vec2 ddx, vec2 ddy)
{
	return sampler->GetValue(uv, ddx, ddy);
}

class ITexture3D
{
public:
	virtual vec4 GetValue(vec3 uvw, vec3 ddx, vec3 ddy) = 0;
};

typedef ITexture3D * samplerCube;
typedef ITexture3D * samplerCubeShadow;

inline vec4 texture(samplerCube sampler, vec3 uv)
{
	return sampler->GetValue(uv, vec3(0.0f), vec3(0.0f));
}

inline vec4 texture(samplerCube sampler, vec3 uv, vec3 ddx, vec3 ddy)
{
	return sampler->GetValue(uv, ddx, ddy);
}


class IShader
{
public:
	virtual bool SetInput(const char * name, void * buffer) = 0;
	virtual void SetInputSize(int n) = 0;
	virtual bool GetOutput(const char * name, void * buffer, int & bufferSize) = 0;
	virtual void Run() = 0;
	virtual void Dispose() = 0;
};


#ifdef _MSC_VER
#pragma warning(disable:4100; disable:4189)
#endif

#define DLL_EXPORT extern "C" __declspec(dllexport) 

#endif
)";