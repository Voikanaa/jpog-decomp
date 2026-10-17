#pragma once

#include "common/abi.h"
#include "common/layout.h"

namespace Toshi
{

class TPlane;
class TQuaternion;
class TVector3;
class TVector4;

// 4x4 matrix (TKernelInterface.dll), row-major with row vectors: rows 0-2 are the basis vectors
// and row 3 the translation, so a point transforms as p' = p * M.
class TMatrix44
{
public:
	TMatrix44();
	TMatrix44(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20,
	          float m21, float m22, float m23, float m30, float m31, float m32, float m33);
	TMatrix44(const TMatrix44& other);
	TMatrix44& operator=(const TMatrix44& other);

	void Set(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20,
	         float m21, float m22, float m23, float m30, float m31, float m32, float m33);
	void Identity();
	TMatrix44& SetFromQuaternion(const TQuaternion& quat);
	// this = rotation(quat) with translation `translation`, applied after `matrix`.
	TMatrix44& PushQuaternion(const TQuaternion& quat, const TMatrix44& matrix, const TVector3& translation);

	float& operator()(unsigned int row, unsigned int column);
	const float& operator()(unsigned int row, unsigned int column) const;
	void SetBasisVector3(int row, const TVector3& vec);
	void SetBasisVector4(int row, const TVector4& vec);
	void GetBasisVector3(int row, TVector3& vec) const;
	void GetBasisVector4(int row, TVector4& vec) const;
	TVector3& AsBasisVector3(int row);
	const TVector3& AsBasisVector3(int row) const;
	TVector4& AsBasisVector4(int row);
	const TVector4& AsBasisVector4(int row) const;
	void SetTranslate(float x, float y, float z);
	void SetTranslate(const TVector3& translation);
	void SetTranslate(const TVector4& translation);
	void GetTranslate(TVector3& translation) const;
	void GetTranslate(TVector4& translation) const;

	// Moves the translation by `offset` expressed in this matrix's space.
	void Translate(float x, float y, float z);
	void Translate(const TVector3& offset);
	void Translate(const TVector4& offset);
	void Scale(float scale);
	void Scale(float x, float y, float z);
	void Scale(const TVector4& scale);
	void RotateX(float angle);
	void RotateY(float angle);
	void RotateZ(float angle);
	// RotateX, then RotateY, then RotateZ.
	void Rotate(float x, float y, float z);
	// Points row 2 from `eye` towards `target` and rebuilds rows 0 and 1 (row 0 stays horizontal).
	// The original takes non-const references; kept for identical signatures.
	void LookAtTarget(TVector3& target, TVector3& eye);
	void LookAtTarget(TVector3& target);

	void Add(const TMatrix44& matrix);
	void Add(const TMatrix44& a, const TMatrix44& b);
	void Subtract(const TMatrix44& matrix);
	void Subtract(const TMatrix44& a, const TMatrix44& b);
	// this = this * matrix (apply this, then `matrix`).
	void Multiply(const TMatrix44& matrix);
	// this = b * a (apply b, then a).
	void Multiply(const TMatrix44& a, const TMatrix44& b);
	void Multiply(float scalar);
	void Multiply(const TMatrix44& matrix, float scalar);
	void Divide(float scalar);
	void Divide(const TMatrix44& matrix, float scalar);
	void Transpose(const TMatrix44& matrix);
	// Affine inverse that assumes a 3x3 rotation part (transpose plus rotated translation).
	void InvertOrthogonal();
	void InvertOrthogonal(const TMatrix44& matrix);
	// Affine inverse: general 3x3 inverse, column 3 copied, translation transformed back.
	bool Invert();
	bool Invert(const TMatrix44& matrix);
	// Full 4x4 inverses (cofactor expansions with different evaluation orders).
	bool Invert4(const TMatrix44& matrix);
	bool InvertArbitrary(const TMatrix44& matrix);

	float Determinant() const;
	float Determinant4() const;
	bool IsEqual(const TMatrix44& matrix) const;
	bool IsIdentity() const;
	bool IsOrthogonal() const;
	bool IsOrthonormal() const;
	bool IsReflected() const;
	void DebugPrint(const char* label) const;

	TMatrix44& operator+=(const TMatrix44& matrix);
	TMatrix44& operator-=(const TMatrix44& matrix);
	TMatrix44& operator*=(const TMatrix44& matrix);
	TMatrix44& operator*=(float scalar);
	TMatrix44& operator/=(float scalar);
	TMatrix44 operator+(const TMatrix44& matrix) const;
	TMatrix44 operator-(const TMatrix44& matrix) const;
	TMatrix44 operator*(const TMatrix44& matrix) const;
	TMatrix44 operator*(float scalar) const;
	TMatrix44 operator/(float scalar) const;
	TVector3 operator*(const TVector3& vec) const;
	TVector4 operator*(const TVector4& vec) const;
	TPlane operator*(const TPlane& plane) const;
	bool operator==(const TMatrix44& matrix) const;
	bool operator!=(const TMatrix44& matrix) const;

	static void TSTDCALL TransformVector(TVector3& result, const TMatrix44& matrix, const TVector3& vec);
	static void TSTDCALL TransformVector(TVector4& result, const TMatrix44& matrix, const TVector4& vec);
	static void TSTDCALL RotateVector(TVector3& result, const TMatrix44& matrix, const TVector3& vec);
	// Rotates xyz; result.w = vec.w.
	static void TSTDCALL RotateVector(TVector4& result, const TMatrix44& matrix, const TVector4& vec);
	static void TSTDCALL TransformPlane(TPlane& result, const TMatrix44& matrix, const TPlane& plane);
	// Faster TransformPlane for matrices without scale or shear.
	static void TSTDCALL TransformPlaneOrthogonal(TPlane& result, const TMatrix44& matrix, const TPlane& plane);
	static void TSTDCALL MatToQuat(TQuaternion& result, const TMatrix44& matrix);

	static const TMatrix44 IDENTITY;
	static const float MAX_ORTHODEVIATION;
	static const float QUATSCALAR;

	float m[4][4];
};

JPOG_ASSERT_SIZE(TMatrix44, 64);

} // namespace Toshi
