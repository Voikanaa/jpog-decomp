#pragma once

#include "common/abi.h"
#include "common/layout.h"

namespace Toshi
{

class TVector3;

// 2D vector (TKernelInterface.dll). Members are defined out of line, as in the original, so each
// one exists as a symbol that can be hooked and checked against the original export.
class TVector2
{
public:
	TVector2();
	TVector2(float x, float y);
	explicit TVector2(const float* const xy);
	TVector2(const TVector2& other);
	TVector2& operator=(const TVector2& other);

	void Set(float x, float y);
	void Set(const float* const xy);
	void SetXYFromVector3(const TVector3& vec);
	void SetXZFromVector3(const TVector3& vec);
	TVector3 GetVector3XY() const;
	TVector3 GetVector3XZ() const;

	float& operator()(unsigned int index);
	const float& operator()(unsigned int index) const;

	void Add(const TVector2& vec);
	void Add(const TVector2& a, const TVector2& b);
	void Subtract(const TVector2& vec);
	void Subtract(const TVector2& a, const TVector2& b);
	void Multiply(float scalar);
	void Multiply(const TVector2& vec);
	void Multiply(const TVector2& vec, float scalar);
	void Multiply(const TVector2& a, const TVector2& b);
	void Divide(float scalar);
	void Divide(const TVector2& vec);
	void Divide(const TVector2& vec, float scalar);
	void Divide(const TVector2& a, const TVector2& b);
	void Negate();
	void Negate(const TVector2& vec);
	void Abs();
	void Abs(const TVector2& vec);
	void Lerp(const TVector2& from, const TVector2& to, float t);
	void Rotate(float angle);
	void SetPerpendicular();
	void Normalise();

	float Magnitude() const;
	float MagnitudeSq() const;
	bool IsNormalised(float tolerance) const;
	bool IsEqual(const TVector2& vec) const;
	float DotProduct(const TVector2& vec) const;
	float CrossProduct(const TVector2& vec) const;

	TVector2& operator+=(const TVector2& vec);
	TVector2& operator-=(const TVector2& vec);
	TVector2& operator*=(float scalar);
	TVector2& operator*=(const TVector2& vec);
	TVector2& operator/=(float scalar);
	TVector2& operator/=(const TVector2& vec);
	TVector2 operator+(const TVector2& vec) const;
	TVector2 operator-(const TVector2& vec) const;
	TVector2 operator*(float scalar) const;
	TVector2 operator*(const TVector2& vec) const;
	TVector2 operator/(float scalar) const;
	TVector2 operator/(const TVector2& vec) const;
	bool operator==(const TVector2& vec) const;
	bool operator!=(const TVector2& vec) const;

	static float TSTDCALL DotProduct(const TVector2& a, const TVector2& b);
	static float TSTDCALL CrossProduct(const TVector2& a, const TVector2& b);
	static float TSTDCALL Distance(const TVector2& a, const TVector2& b);
	static float TSTDCALL DistanceSq(const TVector2& a, const TVector2& b);
	// Sets result to vec3 with its X and Z offset by the 2D vector (Y is copied).
	static void TSTDCALL AddXZ(TVector3& result, const TVector3& vec3, const TVector2& offset);
	static void TSTDCALL SubXZ(TVector3& result, const TVector3& vec3, const TVector2& offset);

	static const TVector2 VEC_ZERO;
	static const TVector2 VEC_POSX;
	static const TVector2 VEC_POSY;
	static const TVector2 VEC_NEGX;
	static const TVector2 VEC_NEGY;

	float x;
	float y;
};

JPOG_ASSERT_SIZE(TVector2, 8);

} // namespace Toshi
