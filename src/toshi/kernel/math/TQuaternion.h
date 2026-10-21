#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "toshi/kernel/math/TVector3.h"

namespace Toshi
{

class TVector4;

// Rotation quaternion (x, y, z) + w (TKernelInterface.dll). Angles are in radians.
class TQuaternion
{
public:
	TQuaternion();
	TQuaternion(float x, float y, float z, float w);
	// Rotation of `angle` around `axis` (see SetRotation).
	TQuaternion(const TVector3& axis, float angle);
	TQuaternion(const TQuaternion& other);
	TQuaternion& operator=(const TQuaternion& other);

	void Set(float x, float y, float z, float w);
	void SetIdentity();
	// `axis` is expected to be normalised.
	void SetRotation(const TVector3& axis, float angle);
	// The rotation that turns direction `from` into direction `to`.
	void SetVectorDirection(const TVector3& to, const TVector3& from);
	void SetFromEulerYX(const float* angles);
	void SetFromEulerRollPitchYaw(float roll, float pitch, float yaw);
	void GetEulerXYZ(float* angles) const;
	// Rotates VEC3_ZERO_ROTATION by this quaternion.
	void GetVectorDirection(TVector3& direction) const;
	TVector4& AsVector4();
	const TVector4& AsVector4() const;

	// Applies a further rotation of `angle` around `axis` (this = this * rotation).
	void RotateAroundAxis(const TVector3& axis, float angle);
	// Both negate x, y and z (the original assumes unit quaternions).
	void SetConjugate();
	void SetInverse();
	void Negate();
	// Negates the quaternion unless its last non-zero component (w first) is positive.
	void MakePrincple();
	void Normalise();
	void Slerp(const TQuaternion& to, float t);
	void Slerp(const TQuaternion& from, const TQuaternion& to, float t);
	void Lerp(const TQuaternion& from, const TQuaternion& to, float t);
	void DebugPrint(const char* label) const;

	float Magnitude() const;
	float MagnitudeSq() const;

	TQuaternion& operator+=(const TQuaternion& quat);
	TQuaternion& operator-=(const TQuaternion& quat);
	TQuaternion& operator*=(const TQuaternion& quat);
	TQuaternion& operator*=(float scalar);
	TQuaternion& operator/=(const TQuaternion& quat);
	TQuaternion& operator/=(float scalar);
	TQuaternion operator+(const TQuaternion& quat) const;
	TQuaternion operator-(const TQuaternion& quat) const;
	TQuaternion operator*(const TQuaternion& quat) const;
	TQuaternion operator*(float scalar) const;
	TQuaternion operator/(const TQuaternion& quat) const;
	TQuaternion operator/(float scalar) const;
	bool operator==(const TQuaternion& quat) const;
	bool operator!=(const TQuaternion& quat) const;

	static void TSTDCALL Add(TQuaternion& result, const TQuaternion& a, const TQuaternion& b);
	static void TSTDCALL Subtract(TQuaternion& result, const TQuaternion& a, const TQuaternion& b);
	static void TSTDCALL Multiply(TQuaternion& result, const TQuaternion& a, const TQuaternion& b);
	static void TSTDCALL Divide(TQuaternion& result, const TQuaternion& a, const TQuaternion& b);
	static void TSTDCALL RotateVector(TVector3& result, const TQuaternion& quat, const TVector3& vec);
	// Rotates the xyz part of `vec`; result.w is set to 1.
	static void TSTDCALL RotateVector(TVector4& result, const TQuaternion& quat, const TVector4& vec);

	static const TQuaternion IDENTITY;
	static const TVector3 VEC3_ZERO_ROTATION;
	// Slerp falls back to a linear blend when the angle is this small (1 - cos below DELTA).
	static const float DELTA;

	float x;
	float y;
	float z;
	float w;
};

JPOG_ASSERT_SIZE(TQuaternion, 16);

} // namespace Toshi
