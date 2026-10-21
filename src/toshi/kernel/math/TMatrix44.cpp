#include "toshi/kernel/math/TMatrix44.h"

#include "toshi/kernel/math/TMath.h"
#include "toshi/kernel/math/TPlane.h"
#include "toshi/kernel/math/TQuaternion.h"
#include "toshi/kernel/math/TVector3.h"
#include "toshi/kernel/math/TVector4.h"

#include <cmath>
#include <cstring>
#include <utility>

// The evaluation order of every floating-point expression below follows the original code
// (recovered with tools/x87_expr.py), which is required for bit-exact results.

namespace Toshi
{

const TMatrix44 TMatrix44::IDENTITY(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                                    0.0f, 1.0f);
const float TMatrix44::MAX_ORTHODEVIATION = 1e-5f;
const float TMatrix44::QUATSCALAR = 2.0f;

// The default constructor leaves the matrix uninitialised, like the original.
TMatrix44::TMatrix44()
{
}

TMatrix44::TMatrix44(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20,
                     float m21, float m22, float m23, float m30, float m31, float m32, float m33)
{
	Set(m00, m01, m02, m03, m10, m11, m12, m13, m20, m21, m22, m23, m30, m31, m32, m33);
}

TMatrix44::TMatrix44(const TMatrix44& other)
{
	std::memcpy(m, other.m, sizeof(m));
}

TMatrix44& TMatrix44::operator=(const TMatrix44& other)
{
	std::memcpy(m, other.m, sizeof(m));
	return *this;
}

void TMatrix44::Set(float m00, float m01, float m02, float m03, float m10, float m11, float m12, float m13, float m20,
                    float m21, float m22, float m23, float m30, float m31, float m32, float m33)
{
	m[0][0] = m00, m[0][1] = m01, m[0][2] = m02, m[0][3] = m03;
	m[1][0] = m10, m[1][1] = m11, m[1][2] = m12, m[1][3] = m13;
	m[2][0] = m20, m[2][1] = m21, m[2][2] = m22, m[2][3] = m23;
	m[3][0] = m30, m[3][1] = m31, m[3][2] = m32, m[3][3] = m33;
}

void TMatrix44::Identity()
{
	*this = IDENTITY;
}

TMatrix44& TMatrix44::SetFromQuaternion(const TQuaternion& quat)
{
	const float x = quat.x, y = quat.y, z = quat.z, w = quat.w;
	const float y2 = QUATSCALAR * y;
	const float z2 = QUATSCALAR * z;
	const float xw2 = QUATSCALAR * x * w;
	const float xx2 = QUATSCALAR * x * x;
	const float zz2 = z * z2;
	m[0][0] = 1.0f - (y2 * y + zz2);
	m[1][0] = y2 * x - w * z2;
	m[2][0] = x * z2 + y2 * w;
	m[0][1] = y2 * x + w * z2;
	m[1][1] = 1.0f - (zz2 + xx2);
	m[2][1] = y * z2 - xw2;
	m[0][2] = x * z2 - y2 * w;
	m[1][2] = y * z2 + xw2;
	m[2][2] = 1.0f - (xx2 + y2 * y);
	m[0][3] = 0.0f;
	m[1][3] = 0.0f;
	m[2][3] = 0.0f;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
	return *this;
}

TMatrix44& TMatrix44::PushQuaternion(const TQuaternion& quat, const TMatrix44& matrix, const TVector3& translation)
{
	TMatrix44 rotation;
	rotation.SetFromQuaternion(quat);
	rotation.SetTranslate(translation);
	Multiply(matrix, rotation);
	return *this;
}

float& TMatrix44::operator()(unsigned int row, unsigned int column)
{
	return m[row][column];
}

const float& TMatrix44::operator()(unsigned int row, unsigned int column) const
{
	return m[row][column];
}

void TMatrix44::SetBasisVector3(int row, const TVector3& vec)
{
	m[row][0] = vec.x;
	m[row][1] = vec.y;
	m[row][2] = vec.z;
}

void TMatrix44::SetBasisVector4(int row, const TVector4& vec)
{
	m[row][0] = vec.x;
	m[row][1] = vec.y;
	m[row][2] = vec.z;
	m[row][3] = vec.w;
}

void TMatrix44::GetBasisVector3(int row, TVector3& vec) const
{
	vec.Set(m[row][0], m[row][1], m[row][2]);
}

void TMatrix44::GetBasisVector4(int row, TVector4& vec) const
{
	vec.Set(m[row][0], m[row][1], m[row][2], m[row][3]);
}

// Each row has the TVector3 / TVector4 layout; the original returns the row reinterpreted.
TVector3& TMatrix44::AsBasisVector3(int row)
{
	return *reinterpret_cast<TVector3*>(m[row]);
}

const TVector3& TMatrix44::AsBasisVector3(int row) const
{
	return *reinterpret_cast<const TVector3*>(m[row]);
}

TVector4& TMatrix44::AsBasisVector4(int row)
{
	return *reinterpret_cast<TVector4*>(m[row]);
}

const TVector4& TMatrix44::AsBasisVector4(int row) const
{
	return *reinterpret_cast<const TVector4*>(m[row]);
}

void TMatrix44::SetTranslate(float x, float y, float z)
{
	m[3][0] = x;
	m[3][1] = y;
	m[3][2] = z;
}

void TMatrix44::SetTranslate(const TVector3& translation)
{
	SetBasisVector3(3, translation);
}

void TMatrix44::SetTranslate(const TVector4& translation)
{
	SetBasisVector4(3, translation);
}

void TMatrix44::GetTranslate(TVector3& translation) const
{
	GetBasisVector3(3, translation);
}

void TMatrix44::GetTranslate(TVector4& translation) const
{
	GetBasisVector4(3, translation);
}

void TMatrix44::Translate(float x, float y, float z)
{
	const TVector4 offset(x, y, z, 1.0f);
	TransformVector(AsBasisVector4(3), *this, offset);
}

void TMatrix44::Translate(const TVector3& offset)
{
	TransformVector(AsBasisVector3(3), *this, offset);
}

void TMatrix44::Translate(const TVector4& offset)
{
	TransformVector(AsBasisVector4(3), *this, offset);
}

void TMatrix44::Scale(float scale)
{
	Scale(scale, scale, scale);
}

void TMatrix44::Scale(float x, float y, float z)
{
	for (int column = 0; column < 3; ++column)
	{
		m[0][column] *= x;
		m[1][column] *= y;
		m[2][column] *= z;
	}
}

void TMatrix44::Scale(const TVector4& scale)
{
	Scale(scale.x, scale.y, scale.z);
}

void TMatrix44::RotateX(float angle)
{
	float sin;
	float cos;
	TMath::SinCos(angle, sin, cos);
	for (int column = 0; column < 3; ++column)
	{
		const float row1 = m[1][column];
		const float row2 = m[2][column];
		m[1][column] = sin * row2 + cos * row1;
		m[2][column] = row1 * -sin + row2 * cos;
	}
}

void TMatrix44::RotateY(float angle)
{
	float sin;
	float cos;
	TMath::SinCos(angle, sin, cos);
	for (int column = 0; column < 3; ++column)
	{
		const float row0 = m[0][column];
		const float row2 = m[2][column];
		m[0][column] = -sin * row2 + cos * row0;
		m[2][column] = row0 * sin + row2 * cos;
	}
}

void TMatrix44::RotateZ(float angle)
{
	float sin;
	float cos;
	TMath::SinCos(angle, sin, cos);
	for (int column = 0; column < 3; ++column)
	{
		const float row0 = m[0][column];
		const float row1 = m[1][column];
		m[0][column] = sin * row1 + cos * row0;
		m[1][column] = row0 * -sin + row1 * cos;
	}
}

void TMatrix44::Rotate(float x, float y, float z)
{
	RotateX(x);
	RotateY(y);
	RotateZ(z);
}

void TMatrix44::LookAtTarget(TVector3& target, TVector3& eye)
{
	TVector3& forward = AsBasisVector3(2);
	forward.Subtract(target, eye);
	forward.Normalise();
	TVector3& right = AsBasisVector3(0);
	right.Set(forward.z, 0.0f, -forward.x);
	right.Normalise();
	AsBasisVector3(1).CrossProduct(forward, right);
}

void TMatrix44::LookAtTarget(TVector3& target)
{
	LookAtTarget(target, AsBasisVector3(3));
}

void TMatrix44::Add(const TMatrix44& matrix)
{
	Add(*this, matrix);
}

void TMatrix44::Add(const TMatrix44& a, const TMatrix44& b)
{
	for (int row = 0; row < 4; ++row)
		for (int column = 0; column < 4; ++column)
			m[row][column] = a.m[row][column] + b.m[row][column];
}

void TMatrix44::Subtract(const TMatrix44& matrix)
{
	Subtract(*this, matrix);
}

void TMatrix44::Subtract(const TMatrix44& a, const TMatrix44& b)
{
	for (int row = 0; row < 4; ++row)
		for (int column = 0; column < 4; ++column)
			m[row][column] = a.m[row][column] - b.m[row][column];
}

void TMatrix44::Multiply(const TMatrix44& matrix)
{
	TMatrix44 result;
	result.Multiply(*this, matrix);
	*this = result;
}

// Writes each element as soon as it is computed, like the original (no aliasing protection).
void TMatrix44::Multiply(const TMatrix44& a, const TMatrix44& b)
{
	for (int row = 0; row < 4; ++row)
	{
		for (int column = 0; column < 4; ++column)
		{
			m[row][column] =
			    ((a.m[0][column] * b.m[row][0] + a.m[1][column] * b.m[row][1]) + a.m[2][column] * b.m[row][2]) +
			    a.m[3][column] * b.m[row][3];
		}
	}
}

void TMatrix44::Multiply(float scalar)
{
	Multiply(*this, scalar);
}

void TMatrix44::Multiply(const TMatrix44& matrix, float scalar)
{
	for (int row = 0; row < 4; ++row)
		for (int column = 0; column < 4; ++column)
			m[row][column] = matrix.m[row][column] * scalar;
}

void TMatrix44::Divide(float scalar)
{
	Multiply(*this, 1.0f / scalar);
}

void TMatrix44::Divide(const TMatrix44& matrix, float scalar)
{
	Multiply(matrix, 1.0f / scalar);
}

void TMatrix44::Transpose(const TMatrix44& matrix)
{
	const TMatrix44 source = matrix;
	for (int row = 0; row < 4; ++row)
		for (int column = 0; column < 4; ++column)
			m[row][column] = source.m[column][row];
}

void TMatrix44::InvertOrthogonal()
{
	std::swap(m[0][1], m[1][0]);
	std::swap(m[0][2], m[2][0]);
	std::swap(m[1][2], m[2][1]);
	TVector4& translation = AsBasisVector4(3);
	translation.Negate3();
	RotateVector(translation, *this, translation);
}

void TMatrix44::InvertOrthogonal(const TMatrix44& matrix)
{
	const TMatrix44 source = matrix;
	for (int row = 0; row < 3; ++row)
	{
		for (int column = 0; column < 3; ++column)
			m[row][column] = source.m[column][row];
		m[row][3] = source.m[row][3];
	}
	m[3][0] = -source.m[3][0];
	m[3][1] = -source.m[3][1];
	m[3][2] = -source.m[3][2];
	m[3][3] = source.m[3][3];
	TVector4& translation = AsBasisVector4(3);
	RotateVector(translation, *this, translation);
}

bool TMatrix44::Invert()
{
	return Invert(*this);
}

bool TMatrix44::Invert(const TMatrix44& matrix)
{
	const auto& s = matrix.m;
	const float c00 = s[2][2] * s[1][1] - s[2][1] * s[1][2];
	const float c01 = -(s[2][2] * s[0][1] - s[2][1] * s[0][2]);
	const float c02 = s[1][2] * s[0][1] - s[1][1] * s[0][2];
	const float determinant = (c00 * s[0][0] + c01 * s[1][0]) + c02 * s[2][0];
	if (determinant == 0.0f)
		return false;

	const float inv = 1.0f / determinant;
	const TMatrix44 source = matrix;
	const auto& a = source.m;
	m[0][0] = c00 * inv;
	m[0][1] = c01 * inv;
	m[0][2] = c02 * inv;
	m[0][3] = a[0][3];
	m[1][0] = -(a[2][2] * a[1][0] - a[2][0] * a[1][2]) * inv;
	m[1][1] = (a[2][2] * a[0][0] - a[2][0] * a[0][2]) * inv;
	m[1][2] = -(a[1][2] * a[0][0] - a[0][2] * a[1][0]) * inv;
	m[1][3] = a[1][3];
	m[2][0] = (a[2][1] * a[1][0] - a[2][0] * a[1][1]) * inv;
	m[2][1] = -(a[2][1] * a[0][0] - a[2][0] * a[0][1]) * inv;
	m[2][2] = (a[1][1] * a[0][0] - a[1][0] * a[0][1]) * inv;
	m[2][3] = a[2][3];
	m[3][0] = -a[3][0];
	m[3][1] = -a[3][1];
	m[3][2] = -a[3][2];
	m[3][3] = a[3][3];
	TVector4& translation = AsBasisVector4(3);
	RotateVector(translation, *this, translation);
	return true;
}

bool TMatrix44::Invert4(const TMatrix44& matrix)
{
	const float determinant = matrix.Determinant4();
	if (determinant == 0.0f)
		return false;

	const float inv = 1.0f / determinant;
	const TMatrix44 source = matrix;
	const auto& a = source.m;
	// clang-format off
	m[0][0] = (((a[3][3] * a[2][2] - a[2][3] * a[3][2]) * a[1][1] + (a[3][2] * a[1][3] - a[3][3] * a[1][2]) * a[2][1]) + (a[2][3] * a[1][2] - a[1][3] * a[2][2]) * a[3][1]) * inv;
	m[0][1] = (((a[3][3] * a[0][2] - a[0][3] * a[3][2]) * a[2][1] + (a[0][3] * a[2][2] - a[0][2] * a[2][3]) * a[3][1]) + (a[2][3] * a[3][2] - a[3][3] * a[2][2]) * a[0][1]) * inv;
	m[0][2] = (((a[1][3] * a[0][2] - a[0][3] * a[1][2]) * a[3][1] + (a[3][3] * a[1][2] - a[3][2] * a[1][3]) * a[0][1]) + (a[0][3] * a[3][2] - a[3][3] * a[0][2]) * a[1][1]) * inv;
	m[0][3] = (((a[1][3] * a[2][2] - a[2][3] * a[1][2]) * a[0][1] + (a[0][2] * a[2][3] - a[0][3] * a[2][2]) * a[1][1]) + (a[0][3] * a[1][2] - a[1][3] * a[0][2]) * a[2][1]) * inv;
	m[1][0] = (((a[3][3] * a[2][0] - a[2][3] * a[3][0]) * a[1][2] + (a[1][3] * a[3][0] - a[3][3] * a[1][0]) * a[2][2]) + (a[2][3] * a[1][0] - a[2][0] * a[1][3]) * a[3][2]) * inv;
	m[1][1] = (((a[3][3] * a[0][0] - a[0][3] * a[3][0]) * a[2][2] + (a[0][3] * a[2][0] - a[2][3] * a[0][0]) * a[3][2]) + (a[2][3] * a[3][0] - a[3][3] * a[2][0]) * a[0][2]) * inv;
	m[1][2] = (((a[0][0] * a[1][3] - a[0][3] * a[1][0]) * a[3][2] + (a[3][3] * a[1][0] - a[1][3] * a[3][0]) * a[0][2]) + (a[0][3] * a[3][0] - a[3][3] * a[0][0]) * a[1][2]) * inv;
	m[1][3] = (((a[2][0] * a[1][3] - a[2][3] * a[1][0]) * a[0][2] + (a[2][3] * a[0][0] - a[0][3] * a[2][0]) * a[1][2]) + (a[0][3] * a[1][0] - a[0][0] * a[1][3]) * a[2][2]) * inv;
	m[2][0] = (((a[3][1] * a[2][0] - a[2][1] * a[3][0]) * a[1][3] + (a[1][1] * a[3][0] - a[3][1] * a[1][0]) * a[2][3]) + (a[2][1] * a[1][0] - a[2][0] * a[1][1]) * a[3][3]) * inv;
	m[2][1] = (((a[3][1] * a[0][0] - a[3][0] * a[0][1]) * a[2][3] + (a[2][0] * a[0][1] - a[2][1] * a[0][0]) * a[3][3]) + (a[2][1] * a[3][0] - a[3][1] * a[2][0]) * a[0][3]) * inv;
	m[2][2] = (((a[0][0] * a[1][1] - a[1][0] * a[0][1]) * a[3][3] + (a[3][1] * a[1][0] - a[1][1] * a[3][0]) * a[0][3]) + (a[3][0] * a[0][1] - a[3][1] * a[0][0]) * a[1][3]) * inv;
	m[2][3] = (((a[2][0] * a[1][1] - a[2][1] * a[1][0]) * a[0][3] + (a[2][1] * a[0][0] - a[2][0] * a[0][1]) * a[1][3]) + (a[1][0] * a[0][1] - a[0][0] * a[1][1]) * a[2][3]) * inv;
	m[3][0] = (((a[2][2] * a[3][1] - a[3][2] * a[2][1]) * a[1][0] + (a[1][1] * a[3][2] - a[1][2] * a[3][1]) * a[2][0]) + (a[2][1] * a[1][2] - a[2][2] * a[1][1]) * a[3][0]) * inv;
	m[3][1] = (((a[3][1] * a[0][2] - a[3][2] * a[0][1]) * a[2][0] + (a[2][2] * a[0][1] - a[2][1] * a[0][2]) * a[3][0]) + (a[3][2] * a[2][1] - a[2][2] * a[3][1]) * a[0][0]) * inv;
	m[3][2] = (((a[0][2] * a[1][1] - a[0][1] * a[1][2]) * a[3][0] + (a[1][2] * a[3][1] - a[1][1] * a[3][2]) * a[0][0]) + (a[3][2] * a[0][1] - a[3][1] * a[0][2]) * a[1][0]) * inv;
	m[3][3] = (((a[2][2] * a[1][1] - a[2][1] * a[1][2]) * a[0][0] + (a[2][1] * a[0][2] - a[2][2] * a[0][1]) * a[1][0]) + (a[0][1] * a[1][2] - a[0][2] * a[1][1]) * a[2][0]) * inv;
	// clang-format on
	return true;
}

bool TMatrix44::InvertArbitrary(const TMatrix44& matrix)
{
	const TMatrix44 source = matrix;
	const auto& a = source.m;
	// 2x2 minors of rows 0-1 x rows 1-3 (first two columns) and of the last two columns.
	const float t0 = a[1][1] * a[0][0] - a[0][1] * a[1][0];
	const float t1 = a[2][1] * a[0][0] - a[0][1] * a[2][0];
	const float t2 = a[0][0] * a[3][1] - a[0][1] * a[3][0];
	const float t3 = a[2][1] * a[1][0] - a[1][1] * a[2][0];
	const float t4 = a[1][0] * a[3][1] - a[1][1] * a[3][0];
	const float t5 = a[3][1] * a[2][0] - a[2][1] * a[3][0];
	const float t6 = t0 * a[2][2] + (t3 * a[0][2] - t1 * a[1][2]);
	const float t7 = (t2 * a[1][2] - t4 * a[0][2]) - t0 * a[3][2];
	const float t8 = (a[0][2] * t5 - t2 * a[2][2]) + t1 * a[3][2];
	const float t9 = (a[2][2] * t4 - t5 * a[1][2]) - a[3][2] * t3;
	const float t10 = (t1 * a[1][3] - t0 * a[2][3]) - t3 * a[0][3];
	const float t11 = (t0 * a[3][3] - t2 * a[1][3]) + t4 * a[0][3];
	const float t12 = (t2 * a[2][3] - t1 * a[3][3]) - t5 * a[0][3];
	const float t13 = t5 * a[1][3] + (t3 * a[3][3] - t4 * a[2][3]);
	const float t14 = a[1][3] * a[0][2] - a[0][3] * a[1][2];
	const float t15 = a[2][3] * a[0][2] - a[0][3] * a[2][2];
	const float t16 = a[0][2] * a[3][3] - a[0][3] * a[3][2];
	const float t17 = a[2][3] * a[1][2] - a[2][2] * a[1][3];
	const float t18 = a[3][3] * a[2][2] - a[2][3] * a[3][2];
	const float u = a[1][2] * a[3][3] - a[1][3] * a[3][2];
	const float t19 = (t15 * a[1][1] - t14 * a[2][1]) - t17 * a[0][1];
	const float t20 = (t14 * a[3][1] - t16 * a[1][1]) + u * a[0][1];
	const float t21 = a[1][1] * t18 + (a[3][1] * t17 - a[2][1] * u);
	const float t22 = (t16 * a[2][1] - t15 * a[3][1]) - a[0][1] * t18;
	const float determinant = ((t21 * a[0][0] + t22 * a[1][0]) + t20 * a[2][0]) + t19 * a[3][0];
	if (determinant == 0.0f)
		return false;

	const float inv = 1.0f / determinant;
	const float t24 = t17 * a[0][0] + (t14 * a[2][0] - t15 * a[1][0]);
	m[0][0] = t21 * inv;
	m[1][0] = ((u * a[2][0] - a[3][0] * t17) - t18 * a[1][0]) * inv;
	m[0][1] = t22 * inv;
	m[2][0] = t13 * inv;
	m[0][2] = t20 * inv;
	m[3][0] = t9 * inv;
	m[0][3] = t19 * inv;
	m[1][1] = (a[0][0] * t18 + (t15 * a[3][0] - t16 * a[2][0])) * inv;
	m[2][1] = t12 * inv;
	m[1][2] = ((t16 * a[1][0] - t14 * a[3][0]) - u * a[0][0]) * inv;
	m[3][1] = t8 * inv;
	m[1][3] = t24 * inv;
	m[2][2] = t11 * inv;
	m[3][2] = t7 * inv;
	m[2][3] = t10 * inv;
	m[3][3] = inv * t6;
	return true;
}

float TMatrix44::Determinant() const
{
	return ((m[2][2] * m[1][1] - m[2][1] * m[1][2]) * m[0][0] - (m[2][2] * m[0][1] - m[2][1] * m[0][2]) * m[1][0]) +
	       (m[1][2] * m[0][1] - m[1][1] * m[0][2]) * m[2][0];
}

float TMatrix44::Determinant4() const
{
	const float p1 = (m[3][3] * m[0][2] - m[3][2] * m[0][3]) * (m[2][1] * m[1][0] - m[1][1] * m[2][0]);
	const float p2 = (m[2][3] * m[1][2] - m[1][3] * m[2][2]) * (m[0][0] * m[3][1] - m[0][1] * m[3][0]);
	const float p3 = (m[1][1] * m[0][0] - m[0][1] * m[1][0]) * (m[3][3] * m[2][2] - m[2][3] * m[3][2]);
	const float p4 = (m[3][3] * m[1][2] - m[1][3] * m[3][2]) * (m[2][1] * m[0][0] - m[0][1] * m[2][0]);
	const float p5 = (m[1][0] * m[3][1] - m[1][1] * m[3][0]) * (m[2][3] * m[0][2] - m[2][2] * m[0][3]);
	const float p6 = (m[0][2] * m[1][3] - m[0][3] * m[1][2]) * (m[3][1] * m[2][0] - m[3][0] * m[2][1]);
	return ((p1 + (p2 + (p3 - p4))) - p5) + p6;
}

bool TMatrix44::IsEqual(const TMatrix44& matrix) const
{
	for (int row = 0; row < 4; ++row)
		for (int column = 0; column < 4; ++column)
			if (matrix.m[row][column] != m[row][column])
				return false;
	return true;
}

bool TMatrix44::IsIdentity() const
{
	return IsEqual(IDENTITY);
}

// The squared deviation of the three row dot products from 0 is below MAX_ORTHODEVIATION.
bool TMatrix44::IsOrthogonal() const
{
	const TVector3 dots(TVector3::DotProduct(AsBasisVector3(0), AsBasisVector3(1)),
	                    TVector3::DotProduct(AsBasisVector3(0), AsBasisVector3(2)),
	                    TVector3::DotProduct(AsBasisVector3(1), AsBasisVector3(2)));
	return TVector3::DotProduct(dots, dots) < MAX_ORTHODEVIATION;
}

// As IsOrthogonal, plus the deviation of each row's squared length from 1.
bool TMatrix44::IsOrthonormal() const
{
	const TVector3 dots(TVector3::DotProduct(AsBasisVector3(0), AsBasisVector3(1)),
	                    TVector3::DotProduct(AsBasisVector3(0), AsBasisVector3(2)),
	                    TVector3::DotProduct(AsBasisVector3(1), AsBasisVector3(2)));
	const TVector3 lengths(TVector3::DotProduct(AsBasisVector3(0), AsBasisVector3(0)) + -1.0f,
	                       TVector3::DotProduct(AsBasisVector3(1), AsBasisVector3(1)) + -1.0f,
	                       TVector3::DotProduct(AsBasisVector3(2), AsBasisVector3(2)) + -1.0f);
	return TVector3::DotProduct(lengths, lengths) + TVector3::DotProduct(dots, dots) < MAX_ORTHODEVIATION;
}

bool TMatrix44::IsReflected() const
{
	TVector3 cross;
	cross.CrossProduct(AsBasisVector3(0), AsBasisVector3(1));
	return TVector3::DotProduct(cross, AsBasisVector3(2)) < 0.0f;
}

// Empty in the shipped build.
void TMatrix44::DebugPrint(const char*) const
{
}

TMatrix44& TMatrix44::operator+=(const TMatrix44& matrix)
{
	Add(matrix);
	return *this;
}

TMatrix44& TMatrix44::operator-=(const TMatrix44& matrix)
{
	Subtract(matrix);
	return *this;
}

TMatrix44& TMatrix44::operator*=(const TMatrix44& matrix)
{
	Multiply(matrix);
	return *this;
}

TMatrix44& TMatrix44::operator*=(float scalar)
{
	Multiply(scalar);
	return *this;
}

TMatrix44& TMatrix44::operator/=(float scalar)
{
	Divide(scalar);
	return *this;
}

TMatrix44 TMatrix44::operator+(const TMatrix44& matrix) const
{
	TMatrix44 result;
	result.Add(*this, matrix);
	return result;
}

TMatrix44 TMatrix44::operator-(const TMatrix44& matrix) const
{
	TMatrix44 result;
	result.Subtract(*this, matrix);
	return result;
}

TMatrix44 TMatrix44::operator*(const TMatrix44& matrix) const
{
	TMatrix44 result;
	result.Multiply(*this, matrix);
	return result;
}

TMatrix44 TMatrix44::operator*(float scalar) const
{
	TMatrix44 result;
	result.Multiply(*this, scalar);
	return result;
}

TMatrix44 TMatrix44::operator/(float scalar) const
{
	TMatrix44 result;
	result.Divide(*this, scalar);
	return result;
}

TVector3 TMatrix44::operator*(const TVector3& vec) const
{
	TVector3 result;
	TransformVector(result, *this, vec);
	return result;
}

TVector4 TMatrix44::operator*(const TVector4& vec) const
{
	TVector4 result;
	TransformVector(result, *this, vec);
	return result;
}

TPlane TMatrix44::operator*(const TPlane& plane) const
{
	TPlane result;
	TransformPlane(result, *this, plane);
	return result;
}

bool TMatrix44::operator==(const TMatrix44& matrix) const
{
	return IsEqual(matrix);
}

bool TMatrix44::operator!=(const TMatrix44& matrix) const
{
	return !IsEqual(matrix);
}

// The translation is added after the x term, as in the original.
void TSTDCALL TMatrix44::TransformVector(TVector3& result, const TMatrix44& matrix, const TVector3& vec)
{
	const auto& a = matrix.m;
	const float x = vec.x, y = vec.y, z = vec.z;
	const float rx = ((a[0][0] * x + a[3][0]) + a[1][0] * y) + a[2][0] * z;
	const float ry = ((a[0][1] * x + a[3][1]) + a[1][1] * y) + a[2][1] * z;
	const float rz = ((a[0][2] * x + a[3][2]) + a[1][2] * y) + a[2][2] * z;
	result.Set(rx, ry, rz);
}

void TSTDCALL TMatrix44::TransformVector(TVector4& result, const TMatrix44& matrix, const TVector4& vec)
{
	const auto& a = matrix.m;
	const float x = vec.x, y = vec.y, z = vec.z, w = vec.w;
	float r[4];
	for (int column = 0; column < 4; ++column)
		r[column] = ((a[0][column] * x + a[1][column] * y) + a[2][column] * z) + a[3][column] * w;
	result.Set(r[0], r[1], r[2], r[3]);
}

void TSTDCALL TMatrix44::RotateVector(TVector3& result, const TMatrix44& matrix, const TVector3& vec)
{
	const auto& a = matrix.m;
	const float x = vec.x, y = vec.y, z = vec.z;
	float r[3];
	for (int column = 0; column < 3; ++column)
		r[column] = (a[0][column] * x + a[1][column] * y) + a[2][column] * z;
	result.Set(r[0], r[1], r[2]);
}

void TSTDCALL TMatrix44::RotateVector(TVector4& result, const TMatrix44& matrix, const TVector4& vec)
{
	const auto& a = matrix.m;
	const float x = vec.x, y = vec.y, z = vec.z, w = vec.w;
	float r[3];
	for (int column = 0; column < 3; ++column)
		r[column] = (a[0][column] * x + a[1][column] * y) + a[2][column] * z;
	result.Set(r[0], r[1], r[2], w);
}

// Transforms two vectors lying in the plane and a point on it, then rebuilds the plane.
void TSTDCALL TMatrix44::TransformPlane(TPlane& result, const TMatrix44& matrix, const TPlane& plane)
{
	const TVector4& normal = plane.AsVector4();
	TVector4 tangent;
	if (std::fabs(plane.y) <= std::fabs(plane.x))
		tangent.Set(-plane.z, 0.0f, plane.x, 1.0f);
	else
		tangent.Set(0.0f, plane.z, -plane.y, 1.0f);
	TVector4 bitangent;
	bitangent.CrossProduct(normal, tangent);
	bitangent.w = 1.0f;
	TVector4 point;
	point.Multiply(normal, plane.d);
	point.w = 1.0f;

	RotateVector(tangent, matrix, tangent);
	RotateVector(bitangent, matrix, bitangent);
	TVector4& out = result.AsVector4();
	out.CrossProduct(tangent, bitangent);
	out.Normalise();
	TransformVector(point, matrix, point);
	result.d = TVector4::DotProduct3(out, point);
	if (matrix.IsReflected())
		out.Negate4();
}

void TSTDCALL TMatrix44::TransformPlaneOrthogonal(TPlane& result, const TMatrix44& matrix, const TPlane& plane)
{
	RotateVector(result.AsVector4(), matrix, plane.AsVector4());
	result.d = TVector4::DotProduct3(result.AsVector4(), matrix.AsBasisVector4(3)) + plane.d;
}

// Shepperd's method: picks the largest of w, x, y, z to divide by.
void TSTDCALL TMatrix44::MatToQuat(TQuaternion& result, const TMatrix44& matrix)
{
	const auto& a = matrix.m;
	const float trace = (a[1][1] + a[0][0]) + a[2][2];
	if (0.0f < trace)
	{
		const float s = std::sqrt(trace + 1.0f) * 0.5f;
		result.w = s;
		const float r = 0.25f / s;
		result.x = (a[1][2] - a[2][1]) * r;
		result.y = (a[2][0] - a[0][2]) * r;
		result.z = (a[0][1] - a[1][0]) * r;
		return;
	}
	const bool xLargest = a[1][1] <= a[0][0] && a[2][2] < a[0][0];
	const bool yLargest = !(a[1][1] <= a[0][0]) && !(a[1][1] < a[2][2]);
	if (xLargest)
	{
		const float s = std::sqrt(((a[0][0] + 1.0f) - a[1][1]) - a[2][2]) * 0.5f;
		result.x = s;
		const float r = 0.25f / s;
		result.w = (a[1][2] - a[2][1]) * r;
		result.y = (a[0][1] + a[1][0]) * r;
		result.z = (a[0][2] + a[2][0]) * r;
	}
	else if (yLargest)
	{
		const float s = std::sqrt(((a[1][1] + 1.0f) - a[2][2]) - a[0][0]) * 0.5f;
		result.y = s;
		const float r = 0.25f / s;
		result.w = (a[2][0] - a[0][2]) * r;
		result.z = (a[1][2] + a[2][1]) * r;
		result.x = (a[1][0] + a[0][1]) * r;
	}
	else
	{
		const float s = std::sqrt(((a[2][2] + 1.0f) - a[0][0]) - a[1][1]) * 0.5f;
		result.z = s;
		const float r = 0.25f / s;
		result.w = (a[0][1] - a[1][0]) * r;
		result.x = (a[2][0] + a[0][2]) * r;
		result.y = (a[2][1] + a[1][2]) * r;
	}
}

// --- TMatrix44ID ----------------------------------------------------------------------------

TMatrix44ID::TMatrix44ID()
{
}

TMatrix44ID::TMatrix44ID(const TMatrix44ID& other) = default;

TMatrix44ID& TMatrix44ID::operator=(const TMatrix44ID& other) = default;

unsigned int TMatrix44ID::GetID()
{
	return m_uiID;
}

void TMatrix44ID::SetID(unsigned int id)
{
	m_uiID = id;
}

} // namespace Toshi
