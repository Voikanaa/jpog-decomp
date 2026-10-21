#pragma once

#include "common/abi.h"
#include "common/layout.h"

namespace Toshi
{

class TVector3;

// Homogeneous 3D vector (TKernelInterface.dll). Most operations work on x, y and z only: the
// in-place forms leave w untouched and the two-operand forms copy w from the first operand. The
// "4" variants (Negate4, Multiply4, Subtract4, DotProduct4, Lerp) include w.
class TVector4
{
public:
	TVector4();
	TVector4(float x, float y, float z, float w);
	explicit TVector4(const float* const xyzw);
	TVector4(const TVector4& other);
	TVector4& operator=(const TVector4& other);

	void Set(float x, float y, float z, float w);
	void Set(const float* const xyzw);
	// Copies x, y and z and sets w to 1.
	void Set(const TVector3& vec);

	float& operator()(unsigned int index);
	const float& operator()(unsigned int index) const;
	TVector3& AsVector3();
	const TVector3& AsVector3() const;

	void Add(const TVector4& vec);
	void Add(const TVector4& a, const TVector4& b);
	void Subtract(const TVector4& vec);
	void Subtract(const TVector4& a, const TVector4& b);
	void Subtract4(const TVector4& a, const TVector4& b);
	void Multiply(float scalar);
	void Multiply(const TVector4& vec);
	void Multiply(const TVector4& vec, float scalar);
	void Multiply(const TVector4& a, const TVector4& b);
	void Multiply4(const TVector4& vec);
	void Divide(float scalar);
	void Divide(const TVector4& vec);
	void Divide(const TVector4& vec, float scalar);
	void Divide(const TVector4& a, const TVector4& b);
	void Negate();
	void Negate(const TVector4& vec);
	void Negate3();
	void Negate3(const TVector4& vec);
	void Negate4();
	void Negate4(const TVector4& vec);
	void Abs();
	void Abs(const TVector4& vec);
	void Sin(const TVector4& vec);
	void Cos(const TVector4& vec);
	void Max(float value);
	void Max(const TVector4& vec);
	void Max(const TVector4& vec, float value);
	void Max(const TVector4& a, const TVector4& b);
	void Min(float value);
	void Min(const TVector4& vec);
	void Min(const TVector4& vec, float value);
	void Min(const TVector4& a, const TVector4& b);
	void Lerp(const TVector4& to, float t);
	void Lerp(const TVector4& from, const TVector4& to, float t);
	void Lerp3(const TVector4& to, float t);
	void Lerp3(const TVector4& from, const TVector4& to, float t);
	void CrossProduct(const TVector4& a, const TVector4& b);
	void RotateX(float angle);
	void RotateY(float angle);
	void RotateZ(float angle);
	void Normalise();

	float Magnitude() const;
	float MagnitudeSq() const;
	bool IsEqual(const TVector4& vec) const;
	float DotProduct(const TVector4& vec) const;
	float DotProduct3(const TVector4& vec) const;
	float DotProduct4(const TVector4& vec) const;

	TVector4& operator+=(const TVector4& vec);
	TVector4& operator-=(const TVector4& vec);
	TVector4& operator*=(float scalar);
	TVector4& operator*=(const TVector4& vec);
	TVector4& operator/=(float scalar);
	TVector4& operator/=(const TVector4& vec);
	TVector4 operator+(const TVector4& vec) const;
	TVector4 operator-(const TVector4& vec) const;
	TVector4 operator*(float scalar) const;
	TVector4 operator*(const TVector4& vec) const;
	TVector4 operator/(float scalar) const;
	TVector4 operator/(const TVector4& vec) const;
	bool operator==(const TVector4& vec) const;
	bool operator!=(const TVector4& vec) const;

	// DotProduct is the three-component product.
	static float TSTDCALL DotProduct(const TVector4& a, const TVector4& b);
	static float TSTDCALL DotProduct3(const TVector4& a, const TVector4& b);
	static float TSTDCALL DotProduct4(const TVector4& a, const TVector4& b);
	static float TSTDCALL Distance(const TVector4& a, const TVector4& b);
	static float TSTDCALL DistanceSq(const TVector4& a, const TVector4& b);

	static const TVector4 VEC_ZERO;
	static const TVector4 VEC_POSX;
	static const TVector4 VEC_POSY;
	static const TVector4 VEC_POSZ;
	static const TVector4 VEC_NEGX;
	static const TVector4 VEC_NEGY;
	static const TVector4 VEC_NEGZ;

	float x;
	float y;
	float z;
	float w;
};

JPOG_ASSERT_SIZE(TVector4, 16);

} // namespace Toshi
