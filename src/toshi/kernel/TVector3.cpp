#include "toshi/kernel/TVector3.h"

#include "toshi/kernel/TMath.h"

#include <cmath>

namespace Toshi
{

const TVector3 TVector3::VEC_ZERO(0.0f, 0.0f, 0.0f);
const TVector3 TVector3::VEC_POSX(1.0f, 0.0f, 0.0f);
const TVector3 TVector3::VEC_POSY(0.0f, 1.0f, 0.0f);
const TVector3 TVector3::VEC_POSZ(0.0f, 0.0f, 1.0f);
const TVector3 TVector3::VEC_NEGX(-1.0f, 0.0f, 0.0f);
const TVector3 TVector3::VEC_NEGY(0.0f, -1.0f, 0.0f);
const TVector3 TVector3::VEC_NEGZ(0.0f, 0.0f, -1.0f);
// The diagonal directions are unit length.
const TVector3 TVector3::VEC_POSXPOSZ(TMath::ONEOVER_SQRT_TWO, 0.0f, TMath::ONEOVER_SQRT_TWO);
const TVector3 TVector3::VEC_POSXNEGZ(TMath::ONEOVER_SQRT_TWO, 0.0f, -TMath::ONEOVER_SQRT_TWO);
const TVector3 TVector3::VEC_NEGXPOSZ(-TMath::ONEOVER_SQRT_TWO, 0.0f, TMath::ONEOVER_SQRT_TWO);
const TVector3 TVector3::VEC_NEGXNEGZ(-TMath::ONEOVER_SQRT_TWO, 0.0f, -TMath::ONEOVER_SQRT_TWO);

// The default constructor leaves the components uninitialised, like the original.
TVector3::TVector3()
{
}

TVector3::TVector3(float x, float y, float z)
{
	Set(x, y, z);
}

TVector3::TVector3(const float* const xyz)
{
	Set(xyz);
}

TVector3::TVector3(const TVector3& other) : x(other.x), y(other.y), z(other.z)
{
}

TVector3& TVector3::operator=(const TVector3& other)
{
	x = other.x;
	y = other.y;
	z = other.z;
	return *this;
}

void TVector3::Set(float newX, float newY, float newZ)
{
	x = newX;
	y = newY;
	z = newZ;
}

void TVector3::Set(const float* const xyz)
{
	x = xyz[0];
	y = xyz[1];
	z = xyz[2];
}

float& TVector3::operator()(unsigned int index)
{
	return (&x)[index];
}

const float& TVector3::operator()(unsigned int index) const
{
	return (&x)[index];
}

void TVector3::Add(const TVector3& vec)
{
	x += vec.x;
	y += vec.y;
	z += vec.z;
}

void TVector3::Add(const TVector3& a, const TVector3& b)
{
	x = a.x + b.x;
	y = a.y + b.y;
	z = a.z + b.z;
}

void TVector3::Subtract(const TVector3& vec)
{
	x -= vec.x;
	y -= vec.y;
	z -= vec.z;
}

void TVector3::Subtract(const TVector3& a, const TVector3& b)
{
	x = a.x - b.x;
	y = a.y - b.y;
	z = a.z - b.z;
}

void TVector3::Multiply(float scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
}

void TVector3::Multiply(const TVector3& vec)
{
	x *= vec.x;
	y *= vec.y;
	z *= vec.z;
}

void TVector3::Multiply(const TVector3& vec, float scalar)
{
	x = vec.x * scalar;
	y = vec.y * scalar;
	z = vec.z * scalar;
}

void TVector3::Multiply(const TVector3& a, const TVector3& b)
{
	x = a.x * b.x;
	y = a.y * b.y;
	z = a.z * b.z;
}

// Division by a scalar multiplies by the reciprocal, as the original does.
void TVector3::Divide(float scalar)
{
	const float reciprocal = 1.0f / scalar;
	x *= reciprocal;
	y *= reciprocal;
	z *= reciprocal;
}

void TVector3::Divide(const TVector3& vec)
{
	x /= vec.x;
	y /= vec.y;
	z /= vec.z;
}

void TVector3::Divide(const TVector3& vec, float scalar)
{
	const float reciprocal = 1.0f / scalar;
	x = vec.x * reciprocal;
	y = vec.y * reciprocal;
	z = vec.z * reciprocal;
}

void TVector3::Divide(const TVector3& a, const TVector3& b)
{
	x = a.x / b.x;
	y = a.y / b.y;
	z = a.z / b.z;
}

void TVector3::Negate()
{
	x = -x;
	y = -y;
	z = -z;
}

void TVector3::Negate(const TVector3& vec)
{
	x = -vec.x;
	y = -vec.y;
	z = -vec.z;
}

void TVector3::Abs()
{
	x = std::fabs(x);
	y = std::fabs(y);
	z = std::fabs(z);
}

void TVector3::Abs(const TVector3& vec)
{
	x = std::fabs(vec.x);
	y = std::fabs(vec.y);
	z = std::fabs(vec.z);
}

void TVector3::Clip(float min, float max)
{
	TMath::Clip(x, min, max);
	TMath::Clip(y, min, max);
	TMath::Clip(z, min, max);
}

void TVector3::Lerp(const TVector3& to, float t)
{
	x = (to.x - x) * t + x;
	y = (to.y - y) * t + y;
	z = (to.z - z) * t + z;
}

void TVector3::Lerp(const TVector3& from, const TVector3& to, float t)
{
	x = (to.x - from.x) * t + from.x;
	y = (to.y - from.y) * t + from.y;
	z = (to.z - from.z) * t + from.z;
}

void TVector3::CrossProduct(const TVector3& a, const TVector3& b)
{
	const float newX = a.y * b.z - a.z * b.y;
	const float newY = a.z * b.x - a.x * b.z;
	const float newZ = a.x * b.y - a.y * b.x;
	Set(newX, newY, newZ);
}

void TVector3::RotateX(float angle)
{
	float sin;
	float cos;
	TMath::SinCos(angle, sin, cos);
	const float oldY = y;
	y = cos * oldY + -sin * z;
	z = oldY * sin + z * cos;
}

void TVector3::RotateY(float angle)
{
	float sin;
	float cos;
	TMath::SinCos(angle, sin, cos);
	const float oldX = x;
	x = cos * oldX + sin * z;
	z = z * cos + -sin * oldX;
}

void TVector3::RotateZ(float angle)
{
	float sin;
	float cos;
	TMath::SinCos(angle, sin, cos);
	const float oldX = x;
	x = cos * oldX + -sin * y;
	y = sin * oldX + y * cos;
}

// A zero vector becomes (0, 0, 1).
void TVector3::Normalise()
{
	const float magnitudeSq = MagnitudeSq();
	if (magnitudeSq == 0.0f)
	{
		Set(0.0f, 0.0f, 1.0f);
		return;
	}
	const float reciprocal = 1.0f / std::sqrt(magnitudeSq);
	x *= reciprocal;
	y *= reciprocal;
	z *= reciprocal;
}

void TVector3::SetUnitVectorProjection(TVector3& unit)
{
	Multiply(unit, GetScalarProjectionOnUnit(unit));
}

void TVector3::SetVectorProjection(TVector3& vec)
{
	Multiply(vec, GetScalarProjection(vec));
}

float TVector3::Magnitude() const
{
	return std::sqrt(MagnitudeSq());
}

float TVector3::MagnitudeSq() const
{
	return x * x + y * y + z * z;
}

// True when the length is strictly within (1 - tolerance, 1 + tolerance), compared squared.
bool TVector3::IsNormalised(float tolerance) const
{
	const float magnitudeSq = MagnitudeSq();
	const float low = 1.0f - tolerance;
	const float high = tolerance + 1.0f;
	return low * low < magnitudeSq && magnitudeSq < high * high;
}

bool TVector3::IsReasonable(float limit) const
{
	return -limit <= x && x < limit && -limit <= y && y < limit && -limit <= z && z < limit;
}

bool TVector3::IsEqual(const TVector3& vec) const
{
	return x == vec.x && y == vec.y && z == vec.z;
}

float TVector3::DotProduct(const TVector3& vec) const
{
	return DotProduct(*this, vec);
}

float TVector3::GetScalarProjectionOnUnit(TVector3& unit)
{
	return DotProduct(*this, unit);
}

float TVector3::GetScalarProjection(TVector3& vec)
{
	return DotProduct(*this, vec) / vec.MagnitudeSq();
}

TVector3& TVector3::operator+=(const TVector3& vec)
{
	Add(vec);
	return *this;
}

TVector3& TVector3::operator-=(const TVector3& vec)
{
	Subtract(vec);
	return *this;
}

TVector3& TVector3::operator*=(float scalar)
{
	Multiply(scalar);
	return *this;
}

TVector3& TVector3::operator*=(const TVector3& vec)
{
	Multiply(vec);
	return *this;
}

TVector3& TVector3::operator/=(float scalar)
{
	Divide(scalar);
	return *this;
}

TVector3& TVector3::operator/=(const TVector3& vec)
{
	Divide(vec);
	return *this;
}

TVector3 TVector3::operator+(const TVector3& vec) const
{
	TVector3 result;
	result.Add(*this, vec);
	return result;
}

TVector3 TVector3::operator-(const TVector3& vec) const
{
	TVector3 result;
	result.Subtract(*this, vec);
	return result;
}

TVector3 TVector3::operator*(float scalar) const
{
	TVector3 result;
	result.Multiply(*this, scalar);
	return result;
}

TVector3 TVector3::operator*(const TVector3& vec) const
{
	TVector3 result;
	result.Multiply(*this, vec);
	return result;
}

TVector3 TVector3::operator/(float scalar) const
{
	TVector3 result;
	result.Divide(*this, scalar);
	return result;
}

TVector3 TVector3::operator/(const TVector3& vec) const
{
	TVector3 result;
	result.Divide(*this, vec);
	return result;
}

bool TVector3::operator==(const TVector3& vec) const
{
	return IsEqual(vec);
}

bool TVector3::operator!=(const TVector3& vec) const
{
	return !IsEqual(vec);
}

float TSTDCALL TVector3::DotProduct(const TVector3& a, const TVector3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float TSTDCALL TVector3::Distance(const TVector3& a, const TVector3& b)
{
	return std::sqrt(DistanceSq(a, b));
}

float TSTDCALL TVector3::DistanceSq(const TVector3& a, const TVector3& b)
{
	const float dx = b.x - a.x;
	const float dy = b.y - a.y;
	const float dz = b.z - a.z;
	return dx * dx + dy * dy + dz * dz;
}

} // namespace Toshi
