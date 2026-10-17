#include "toshi/kernel/TQuaternion.h"

#include "toshi/kernel/TMath.h"
#include "toshi/kernel/TMatrix44.h"
#include "toshi/kernel/TVector4.h"

#include <cmath>

namespace Toshi
{

const TQuaternion TQuaternion::IDENTITY(0.0f, 0.0f, 0.0f, 1.0f);
const TVector3 TQuaternion::VEC3_ZERO_ROTATION(0.0f, 0.0f, 1.0f);
const float TQuaternion::DELTA = 0.001f;

namespace
{

// The original evaluates fsin/fcos at x87 extended precision and rounds only the following
// arithmetic to float (24-bit precision control). Doing the trigonometry in double and rounding
// each product once reproduces that.
float MulRounded(double a, double b)
{
	return static_cast<float>(a * b);
}

} // namespace

// The default constructor leaves the components uninitialised, like the original.
TQuaternion::TQuaternion()
{
}

TQuaternion::TQuaternion(float x, float y, float z, float w)
{
	Set(x, y, z, w);
}

TQuaternion::TQuaternion(const TVector3& axis, float angle)
{
	SetRotation(axis, angle);
}

TQuaternion::TQuaternion(const TQuaternion& other) : x(other.x), y(other.y), z(other.z), w(other.w)
{
}

TQuaternion& TQuaternion::operator=(const TQuaternion& other)
{
	x = other.x;
	y = other.y;
	z = other.z;
	w = other.w;
	return *this;
}

void TQuaternion::Set(float newX, float newY, float newZ, float newW)
{
	x = newX;
	y = newY;
	z = newZ;
	w = newW;
}

void TQuaternion::SetIdentity()
{
	*this = IDENTITY;
}

void TQuaternion::SetRotation(const TVector3& axis, float angle)
{
	float sin;
	float cos;
	TMath::SinCos(angle * 0.5f, sin, cos);
	x = axis.x * sin;
	y = axis.y * sin;
	z = axis.z * sin;
	w = cos;
}

void TQuaternion::SetVectorDirection(const TVector3& to, const TVector3& from)
{
	TVector3 axis;
	axis.CrossProduct(from, to);
	const float magnitude = axis.Magnitude();
	if (TMath::FLOATEPSILON < magnitude)
	{
		axis.Divide(magnitude);
		const float cos = TVector3::DotProduct(from, to);
		float angle = 0.0f;
		if (cos <= 1.0f - TMath::FLOATEPSILON)
		{
			angle = TMath::PI;
			if (-1.0f + TMath::FLOATEPSILON <= cos)
				angle = TMath::ACos(cos);
		}
		SetRotation(axis, angle);
		return;
	}

	// The directions are parallel. Opposite directions need a half turn around any perpendicular
	// axis: `from` turned by 90 degrees around Y (or X, when `from` is nearly vertical).
	if (TVector3::DotProduct(from, to) < 0.0f)
	{
		const float degreesToRadians = TMath::PI * (1.0f / 180.0f);
		TVector3 perpendicular = from;
		if (std::fabs(from.y) < 0.95f)
			perpendicular.RotateY(degreesToRadians * 90.0f);
		else
			perpendicular.RotateX(degreesToRadians * 90.0f);
		*this = IDENTITY;
		RotateAroundAxis(perpendicular, degreesToRadians * 180.0f);
		return;
	}
	Set(0.0f, 0.0f, 0.0f, 1.0f);
}

void TQuaternion::SetFromEulerYX(const float* angles)
{
	float sin;
	float cos;
	TMath::SinCos(angles[0] * 0.5f, sin, cos);
	const TQuaternion rotationX(sin, 0.0f, 0.0f, cos);
	TMath::SinCos(angles[1] * 0.5f, sin, cos);
	Set(0.0f, sin, 0.0f, cos);
	*this *= rotationX;
}

void TQuaternion::SetFromEulerRollPitchYaw(float roll, float pitch, float yaw)
{
	const double cosRoll = std::cos(static_cast<double>(roll * 0.5f));
	const double sinRoll = std::sin(static_cast<double>(roll * 0.5f));
	const double cosPitch = std::cos(static_cast<double>(pitch * 0.5f));
	const double sinPitch = std::sin(static_cast<double>(pitch * 0.5f));
	const double cosYaw = std::cos(static_cast<double>(yaw * 0.5f));
	const double sinYaw = std::sin(static_cast<double>(yaw * 0.5f));
	// The original spills some of the extended-precision terms to float before using them again.
	const float sinPitchF = static_cast<float>(sinPitch);
	const float cosYawF = static_cast<float>(cosYaw);
	w = MulRounded(MulRounded(sinYaw, sinPitch), sinRoll) + MulRounded(MulRounded(cosYaw, cosPitch), cosRoll);
	x = MulRounded(MulRounded(cosYaw, cosPitch), sinRoll) - MulRounded(MulRounded(sinYaw, sinPitch), cosRoll);
	y = MulRounded(MulRounded(cosPitch, sinRoll), sinYaw) + MulRounded(MulRounded(sinPitchF, cosRoll), cosYawF);
	z = MulRounded(MulRounded(cosRoll, cosPitch), sinYaw) - MulRounded(MulRounded(sinRoll, sinPitchF), cosYawF);
}

// angles = (x, y, z), read from the rotation matrix. Near gimbal lock (|cos y| <= 0.001) z is 0.
// The x87 fpatan/asin results are rounded to float only when stored.
void TQuaternion::GetEulerXYZ(float* angles) const
{
	TMatrix44 matrix;
	matrix.SetFromQuaternion(*this);
	const auto& m = matrix.m;
	float sinY = m[2][0];
	if (sinY < -1.0f)
		sinY = -1.0f;
	else if (1.0f < sinY)
		sinY = 1.0f;
	float cosY = std::sqrt(1.0f - sinY * sinY);
	if (m[0][0] < 0.0f && m[2][2] < 0.0f)
		cosY = -cosY;
	if (cosY <= 0.001f && -0.001f <= cosY)
	{
		angles[0] = static_cast<float>(std::atan2(static_cast<double>(-m[2][1]), static_cast<double>(m[1][1])));
		angles[1] = static_cast<float>(-std::asin(static_cast<double>(sinY)));
		angles[2] = 0.0f;
		return;
	}
	const float inv = 1.0f / cosY;
	angles[0] = static_cast<float>(std::atan2(static_cast<double>(m[2][1] * inv), static_cast<double>(inv * m[2][2])));
	angles[1] = static_cast<float>(std::atan2(static_cast<double>(sinY), static_cast<double>(cosY)));
	angles[2] = static_cast<float>(std::atan2(static_cast<double>(m[1][0] * inv), static_cast<double>(inv * m[0][0])));
}

void TQuaternion::GetVectorDirection(TVector3& direction) const
{
	RotateVector(direction, *this, VEC3_ZERO_ROTATION);
}

// The xyzw layout matches TVector4; the original returns `this` reinterpreted.
TVector4& TQuaternion::AsVector4()
{
	return *reinterpret_cast<TVector4*>(this);
}

const TVector4& TQuaternion::AsVector4() const
{
	return *reinterpret_cast<const TVector4*>(this);
}

void TQuaternion::RotateAroundAxis(const TVector3& axis, float angle)
{
	float sin;
	float cos;
	TMath::SinCos(angle * 0.5f, sin, cos);
	const TQuaternion rotation(axis.x * sin, axis.y * sin, axis.z * sin, cos);
	*this *= rotation;
}

void TQuaternion::SetConjugate()
{
	AsVector4().Negate3();
}

void TQuaternion::SetInverse()
{
	AsVector4().Negate3();
}

void TQuaternion::Negate()
{
	x = -x;
	y = -y;
	z = -z;
	w = -w;
}

void TQuaternion::MakePrincple()
{
	const float* components = &x;
	for (int i = 3; i >= 0; --i)
	{
		if (components[i] != 0.0f)
		{
			if (components[i] < 0.0f)
				Negate();
			return;
		}
	}
}

void TQuaternion::Normalise()
{
	*this /= Magnitude();
}

void TQuaternion::Slerp(const TQuaternion& to, float t)
{
	Slerp(*this, to, t);
}

// Takes the shorter arc: `to` is negated when the quaternions point into opposite hemispheres.
void TQuaternion::Slerp(const TQuaternion& from, const TQuaternion& to, float t)
{
	float cos = TVector4::DotProduct4(from.AsVector4(), to.AsVector4());
	TVector4 target;
	if (0.0f <= cos)
	{
		target = to.AsVector4();
	}
	else
	{
		cos = -cos;
		target.Negate4(to.AsVector4());
	}

	float fromWeight;
	float toWeight;
	if (DELTA < 1.0f - cos)
	{
		// TMath::ACos returns its double result on the x87 stack, so the angle is not rounded to
		// float here; only the products below are.
		const double angle = std::acos(static_cast<double>(cos));
		const float invSin = static_cast<float>(1.0 / std::sin(angle));
		fromWeight = MulRounded(std::sin(static_cast<double>(MulRounded(1.0f - t, angle))), invSin);
		toWeight = MulRounded(std::sin(static_cast<double>(MulRounded(t, angle))), invSin);
	}
	else
	{
		fromWeight = 1.0f - t;
		toWeight = t;
	}
	x = from.x * fromWeight + target.x * toWeight;
	y = from.y * fromWeight + target.y * toWeight;
	z = from.z * fromWeight + target.z * toWeight;
	w = from.w * fromWeight + target.w * toWeight;
}

void TQuaternion::Lerp(const TQuaternion& from, const TQuaternion& to, float t)
{
	TVector4 target;
	if (0.0f <= TVector4::DotProduct4(from.AsVector4(), to.AsVector4()))
		target = to.AsVector4();
	else
		target.Negate4(to.AsVector4());

	const float fromWeight = 1.0f - t;
	x = from.x * fromWeight + target.x * t;
	y = from.y * fromWeight + target.y * t;
	z = from.z * fromWeight + target.z * t;
	w = from.w * fromWeight + target.w * t;
}

// Empty in the shipped build.
void TQuaternion::DebugPrint(const char*) const
{
}

float TQuaternion::Magnitude() const
{
	return std::sqrt(MagnitudeSq());
}

float TQuaternion::MagnitudeSq() const
{
	return x * x + y * y + z * z + w * w;
}

TQuaternion& TQuaternion::operator+=(const TQuaternion& quat)
{
	x += quat.x;
	y += quat.y;
	z += quat.z;
	w += quat.w;
	return *this;
}

TQuaternion& TQuaternion::operator-=(const TQuaternion& quat)
{
	x -= quat.x;
	y -= quat.y;
	z -= quat.z;
	w -= quat.w;
	return *this;
}

// Hamilton product this * quat.
TQuaternion& TQuaternion::operator*=(const TQuaternion& quat)
{
	const float x1 = x, y1 = y, z1 = z, w1 = w;
	const float x2 = quat.x, y2 = quat.y, z2 = quat.z, w2 = quat.w;
	x = ((x1 * w2 + z2 * y1) - y2 * z1) + x2 * w1;
	y = (x2 * z1 + (y1 * w2 - z2 * x1)) + y2 * w1;
	z = ((y2 * x1 - x2 * y1) + z1 * w2) + w1 * z2;
	w = (-(y2 * y1 + x2 * x1) - z2 * z1) + w2 * w1;
	return *this;
}

TQuaternion& TQuaternion::operator*=(float scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
	w *= scalar;
	return *this;
}

// Component-wise division. The original divides z by quat.y (not quat.z); kept for fidelity.
TQuaternion& TQuaternion::operator/=(const TQuaternion& quat)
{
	x /= quat.x;
	y /= quat.y;
	z /= quat.y;
	w /= quat.w;
	return *this;
}

// Division by zero leaves the quaternion unchanged.
TQuaternion& TQuaternion::operator/=(float scalar)
{
	if (scalar != 0.0f)
		*this *= 1.0f / scalar;
	return *this;
}

TQuaternion TQuaternion::operator+(const TQuaternion& quat) const
{
	TQuaternion result = *this;
	result += quat;
	return result;
}

TQuaternion TQuaternion::operator-(const TQuaternion& quat) const
{
	TQuaternion result = *this;
	result -= quat;
	return result;
}

TQuaternion TQuaternion::operator*(const TQuaternion& quat) const
{
	TQuaternion result = *this;
	result *= quat;
	return result;
}

TQuaternion TQuaternion::operator*(float scalar) const
{
	TQuaternion result = *this;
	result *= scalar;
	return result;
}

TQuaternion TQuaternion::operator/(const TQuaternion& quat) const
{
	TQuaternion result = *this;
	result /= quat;
	return result;
}

TQuaternion TQuaternion::operator/(float scalar) const
{
	TQuaternion result = *this;
	result /= scalar;
	return result;
}

bool TQuaternion::operator==(const TQuaternion& quat) const
{
	return x == quat.x && y == quat.y && z == quat.z && w == quat.w;
}

bool TQuaternion::operator!=(const TQuaternion& quat) const
{
	return !(*this == quat);
}

void TSTDCALL TQuaternion::Add(TQuaternion& result, const TQuaternion& a, const TQuaternion& b)
{
	result = a;
	result += b;
}

void TSTDCALL TQuaternion::Subtract(TQuaternion& result, const TQuaternion& a, const TQuaternion& b)
{
	result = a;
	result -= b;
}

void TSTDCALL TQuaternion::Multiply(TQuaternion& result, const TQuaternion& a, const TQuaternion& b)
{
	result = a;
	result *= b;
}

void TSTDCALL TQuaternion::Divide(TQuaternion& result, const TQuaternion& a, const TQuaternion& b)
{
	result = a;
	result /= b;
}

// result = quat * (vec, 0) * conjugate(quat).
void TSTDCALL TQuaternion::RotateVector(TVector3& result, const TQuaternion& quat, const TVector3& vec)
{
	TQuaternion conjugate = quat;
	conjugate.SetConjugate();
	const TQuaternion pure(vec.x, vec.y, vec.z, 0.0f);
	TQuaternion rotated = quat;
	rotated *= pure;
	rotated *= conjugate;
	result.Set(rotated.x, rotated.y, rotated.z);
}

void TSTDCALL TQuaternion::RotateVector(TVector4& result, const TQuaternion& quat, const TVector4& vec)
{
	TQuaternion conjugate = quat;
	conjugate.SetConjugate();
	const TQuaternion pure(vec.x, vec.y, vec.z, 0.0f);
	TQuaternion rotated = quat;
	rotated *= pure;
	rotated *= conjugate;
	result.Set(rotated.x, rotated.y, rotated.z, 1.0f);
}

} // namespace Toshi
