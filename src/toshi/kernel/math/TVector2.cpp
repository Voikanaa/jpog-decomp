#include "toshi/kernel/math/TVector2.h"

#include "toshi/kernel/math/TMath.h"
#include "toshi/kernel/math/TVector3.h"

#include <cmath>

namespace Toshi
{

const TVector2 TVector2::VEC_ZERO(0.0f, 0.0f);
const TVector2 TVector2::VEC_POSX(1.0f, 0.0f);
const TVector2 TVector2::VEC_POSY(0.0f, 1.0f);
const TVector2 TVector2::VEC_NEGX(-1.0f, 0.0f);
const TVector2 TVector2::VEC_NEGY(0.0f, -1.0f);

// The default constructor leaves the components uninitialised, like the original.
TVector2::TVector2()
{
}

TVector2::TVector2(float x, float y)
{
	Set(x, y);
}

TVector2::TVector2(const float* const xy)
{
	Set(xy);
}

TVector2::TVector2(const TVector2& other) : x(other.x), y(other.y)
{
}

TVector2& TVector2::operator=(const TVector2& other)
{
	x = other.x;
	y = other.y;
	return *this;
}

void TVector2::Set(float newX, float newY)
{
	x = newX;
	y = newY;
}

void TVector2::Set(const float* const xy)
{
	x = xy[0];
	y = xy[1];
}

void TVector2::SetXYFromVector3(const TVector3& vec)
{
	x = vec.x;
	y = vec.y;
}

void TVector2::SetXZFromVector3(const TVector3& vec)
{
	x = vec.x;
	y = vec.z;
}

TVector3 TVector2::GetVector3XY() const
{
	return TVector3(x, y, 0.0f);
}

TVector3 TVector2::GetVector3XZ() const
{
	return TVector3(x, 0.0f, y);
}

float& TVector2::operator()(unsigned int index)
{
	return (&x)[index];
}

const float& TVector2::operator()(unsigned int index) const
{
	return (&x)[index];
}

void TVector2::Add(const TVector2& vec)
{
	x += vec.x;
	y += vec.y;
}

void TVector2::Add(const TVector2& a, const TVector2& b)
{
	x = a.x + b.x;
	y = a.y + b.y;
}

void TVector2::Subtract(const TVector2& vec)
{
	x -= vec.x;
	y -= vec.y;
}

void TVector2::Subtract(const TVector2& a, const TVector2& b)
{
	x = a.x - b.x;
	y = a.y - b.y;
}

void TVector2::Multiply(float scalar)
{
	x *= scalar;
	y *= scalar;
}

void TVector2::Multiply(const TVector2& vec)
{
	x *= vec.x;
	y *= vec.y;
}

void TVector2::Multiply(const TVector2& vec, float scalar)
{
	x = vec.x * scalar;
	y = vec.y * scalar;
}

void TVector2::Multiply(const TVector2& a, const TVector2& b)
{
	x = a.x * b.x;
	y = a.y * b.y;
}

// Division by a scalar multiplies by the reciprocal, as the original does.
void TVector2::Divide(float scalar)
{
	const float reciprocal = 1.0f / scalar;
	x *= reciprocal;
	y *= reciprocal;
}

void TVector2::Divide(const TVector2& vec)
{
	x /= vec.x;
	y /= vec.y;
}

void TVector2::Divide(const TVector2& vec, float scalar)
{
	const float reciprocal = 1.0f / scalar;
	x = vec.x * reciprocal;
	y = vec.y * reciprocal;
}

void TVector2::Divide(const TVector2& a, const TVector2& b)
{
	x = a.x / b.x;
	y = a.y / b.y;
}

void TVector2::Negate()
{
	x = -x;
	y = -y;
}

void TVector2::Negate(const TVector2& vec)
{
	x = -vec.x;
	y = -vec.y;
}

void TVector2::Abs()
{
	x = std::fabs(x);
	y = std::fabs(y);
}

void TVector2::Abs(const TVector2& vec)
{
	x = std::fabs(vec.x);
	y = std::fabs(vec.y);
}

void TVector2::Lerp(const TVector2& from, const TVector2& to, float t)
{
	x = (to.x - from.x) * t + from.x;
	y = (to.y - from.y) * t + from.y;
}

// Rotates clockwise by `angle` radians.
void TVector2::Rotate(float angle)
{
	float sin;
	float cos;
	TMath::SinCos(angle, sin, cos);
	const float oldX = x;
	x = cos * oldX + sin * y;
	y = cos * y + -sin * oldX;
}

void TVector2::SetPerpendicular()
{
	const float oldX = x;
	x = -y;
	y = oldX;
}

// A zero vector becomes (1, 0).
void TVector2::Normalise()
{
	const float magnitudeSq = x * x + y * y;
	if (magnitudeSq == 0.0f)
	{
		Set(1.0f, 0.0f);
		return;
	}
	const float reciprocal = 1.0f / std::sqrt(magnitudeSq);
	x *= reciprocal;
	y *= reciprocal;
}

float TVector2::Magnitude() const
{
	return std::sqrt(MagnitudeSq());
}

float TVector2::MagnitudeSq() const
{
	return x * x + y * y;
}

// True when the length is strictly within (1 - tolerance, 1 + tolerance), compared squared.
bool TVector2::IsNormalised(float tolerance) const
{
	const float magnitudeSq = MagnitudeSq();
	const float low = 1.0f - tolerance;
	const float high = tolerance + 1.0f;
	return low * low < magnitudeSq && magnitudeSq < high * high;
}

bool TVector2::IsEqual(const TVector2& vec) const
{
	return x == vec.x && y == vec.y;
}

float TVector2::DotProduct(const TVector2& vec) const
{
	return DotProduct(*this, vec);
}

float TVector2::CrossProduct(const TVector2& vec) const
{
	return CrossProduct(*this, vec);
}

TVector2& TVector2::operator+=(const TVector2& vec)
{
	Add(vec);
	return *this;
}

TVector2& TVector2::operator-=(const TVector2& vec)
{
	Subtract(vec);
	return *this;
}

TVector2& TVector2::operator*=(float scalar)
{
	Multiply(scalar);
	return *this;
}

TVector2& TVector2::operator*=(const TVector2& vec)
{
	Multiply(vec);
	return *this;
}

TVector2& TVector2::operator/=(float scalar)
{
	Divide(scalar);
	return *this;
}

TVector2& TVector2::operator/=(const TVector2& vec)
{
	Divide(vec);
	return *this;
}

TVector2 TVector2::operator+(const TVector2& vec) const
{
	TVector2 result;
	result.Add(*this, vec);
	return result;
}

TVector2 TVector2::operator-(const TVector2& vec) const
{
	TVector2 result;
	result.Subtract(*this, vec);
	return result;
}

TVector2 TVector2::operator*(float scalar) const
{
	TVector2 result;
	result.Multiply(*this, scalar);
	return result;
}

TVector2 TVector2::operator*(const TVector2& vec) const
{
	TVector2 result;
	result.Multiply(*this, vec);
	return result;
}

TVector2 TVector2::operator/(float scalar) const
{
	TVector2 result;
	result.Divide(*this, scalar);
	return result;
}

TVector2 TVector2::operator/(const TVector2& vec) const
{
	TVector2 result;
	result.Divide(*this, vec);
	return result;
}

bool TVector2::operator==(const TVector2& vec) const
{
	return IsEqual(vec);
}

bool TVector2::operator!=(const TVector2& vec) const
{
	return !IsEqual(vec);
}

float TSTDCALL TVector2::DotProduct(const TVector2& a, const TVector2& b)
{
	return a.x * b.x + a.y * b.y;
}

float TSTDCALL TVector2::CrossProduct(const TVector2& a, const TVector2& b)
{
	return a.x * b.y - a.y * b.x;
}

float TSTDCALL TVector2::Distance(const TVector2& a, const TVector2& b)
{
	return std::sqrt(DistanceSq(a, b));
}

float TSTDCALL TVector2::DistanceSq(const TVector2& a, const TVector2& b)
{
	const float dx = b.x - a.x;
	const float dy = b.y - a.y;
	return dx * dx + dy * dy;
}

void TSTDCALL TVector2::AddXZ(TVector3& result, const TVector3& vec3, const TVector2& offset)
{
	result.x = vec3.x + offset.x;
	result.y = vec3.y;
	result.z = vec3.z + offset.y;
}

void TSTDCALL TVector2::SubXZ(TVector3& result, const TVector3& vec3, const TVector2& offset)
{
	result.x = vec3.x - offset.x;
	result.y = vec3.y;
	result.z = vec3.z - offset.y;
}

} // namespace Toshi
