#pragma once

#include "common/abi.h"
#include "common/layout.h"

namespace Toshi
{

// 3D vector (TKernelInterface.dll). Members are defined out of line, as in the original.
class TVector3
{
public:
	TVector3();
	TVector3(float x, float y, float z);
	explicit TVector3(const float* const xyz);
	TVector3(const TVector3& other);
	TVector3& operator=(const TVector3& other);

	void Set(float x, float y, float z);
	void Set(const float* const xyz);

	float& operator()(unsigned int index);
	const float& operator()(unsigned int index) const;

	void Add(const TVector3& vec);
	void Add(const TVector3& a, const TVector3& b);
	void Subtract(const TVector3& vec);
	void Subtract(const TVector3& a, const TVector3& b);
	void Multiply(float scalar);
	void Multiply(const TVector3& vec);
	void Multiply(const TVector3& vec, float scalar);
	void Multiply(const TVector3& a, const TVector3& b);
	void Divide(float scalar);
	void Divide(const TVector3& vec);
	void Divide(const TVector3& vec, float scalar);
	void Divide(const TVector3& a, const TVector3& b);
	void Negate();
	void Negate(const TVector3& vec);
	void Abs();
	void Abs(const TVector3& vec);
	void Clip(float min, float max);
	void Lerp(const TVector3& to, float t);
	void Lerp(const TVector3& from, const TVector3& to, float t);
	void CrossProduct(const TVector3& a, const TVector3& b);
	void RotateX(float angle);
	void RotateY(float angle);
	void RotateZ(float angle);
	void Normalise();
	// Sets this vector to its projection onto `unit` (assumed normalised) / onto `vec`.
	// The original declares these (and GetScalarProjection*) with non-const parameters and as
	// non-const members; kept so the signatures match the original exports.
	void SetUnitVectorProjection(TVector3& unit);
	void SetVectorProjection(TVector3& vec);

	float Magnitude() const;
	float MagnitudeSq() const;
	bool IsNormalised(float tolerance) const;
	// True when every component lies in [-limit, limit).
	bool IsReasonable(float limit) const;
	bool IsEqual(const TVector3& vec) const;
	float DotProduct(const TVector3& vec) const;
	float GetScalarProjectionOnUnit(TVector3& unit);
	float GetScalarProjection(TVector3& vec);

	TVector3& operator+=(const TVector3& vec);
	TVector3& operator-=(const TVector3& vec);
	TVector3& operator*=(float scalar);
	TVector3& operator*=(const TVector3& vec);
	TVector3& operator/=(float scalar);
	TVector3& operator/=(const TVector3& vec);
	TVector3 operator+(const TVector3& vec) const;
	TVector3 operator-(const TVector3& vec) const;
	TVector3 operator*(float scalar) const;
	TVector3 operator*(const TVector3& vec) const;
	TVector3 operator/(float scalar) const;
	TVector3 operator/(const TVector3& vec) const;
	bool operator==(const TVector3& vec) const;
	bool operator!=(const TVector3& vec) const;

	static float TSTDCALL DotProduct(const TVector3& a, const TVector3& b);
	static float TSTDCALL Distance(const TVector3& a, const TVector3& b);
	static float TSTDCALL DistanceSq(const TVector3& a, const TVector3& b);

	static const TVector3 VEC_ZERO;
	static const TVector3 VEC_POSX;
	static const TVector3 VEC_POSY;
	static const TVector3 VEC_POSZ;
	static const TVector3 VEC_NEGX;
	static const TVector3 VEC_NEGY;
	static const TVector3 VEC_NEGZ;
	static const TVector3 VEC_POSXPOSZ;
	static const TVector3 VEC_POSXNEGZ;
	static const TVector3 VEC_NEGXPOSZ;
	static const TVector3 VEC_NEGXNEGZ;

	float x;
	float y;
	float z;
};

JPOG_ASSERT_SIZE(TVector3, 12);

} // namespace Toshi
