#include "toshi/kernel/TVector4.h"

#include "toshi/kernel/TMath.h"
#include "toshi/kernel/TVector3.h"

#include <cmath>

namespace Toshi
{

const TVector4 TVector4::VEC_ZERO(0.0f, 0.0f, 0.0f, 1.0f);
const TVector4 TVector4::VEC_POSX(1.0f, 0.0f, 0.0f, 1.0f);
const TVector4 TVector4::VEC_POSY(0.0f, 1.0f, 0.0f, 1.0f);
const TVector4 TVector4::VEC_POSZ(0.0f, 0.0f, 1.0f, 1.0f);
const TVector4 TVector4::VEC_NEGX(-1.0f, 0.0f, 0.0f, 1.0f);
const TVector4 TVector4::VEC_NEGY(0.0f, -1.0f, 0.0f, 1.0f);
const TVector4 TVector4::VEC_NEGZ(0.0f, 0.0f, -1.0f, 1.0f);

namespace
{

// Component selection for Max/Min. The original copies the chosen component bit for bit and
// keeps `keep` when `keep >= other` (so an unordered comparison picks `other`).
const float& Select(const float& keep, const float& other)
{
	return keep >= other ? keep : other;
}

} // namespace

// The default constructor leaves the components uninitialised, like the original.
TVector4::TVector4()
{
}

TVector4::TVector4(float x, float y, float z, float w)
{
	Set(x, y, z, w);
}

TVector4::TVector4(const float* const xyzw)
{
	Set(xyzw);
}

TVector4::TVector4(const TVector4& other) : x(other.x), y(other.y), z(other.z), w(other.w)
{
}

TVector4& TVector4::operator=(const TVector4& other)
{
	x = other.x;
	y = other.y;
	z = other.z;
	w = other.w;
	return *this;
}

void TVector4::Set(float newX, float newY, float newZ, float newW)
{
	x = newX;
	y = newY;
	z = newZ;
	w = newW;
}

void TVector4::Set(const float* const xyzw)
{
	x = xyzw[0];
	y = xyzw[1];
	z = xyzw[2];
	w = xyzw[3];
}

void TVector4::Set(const TVector3& vec)
{
	x = vec.x;
	y = vec.y;
	z = vec.z;
	w = 1.0f;
}

float& TVector4::operator()(unsigned int index)
{
	return (&x)[index];
}

const float& TVector4::operator()(unsigned int index) const
{
	return (&x)[index];
}

// x, y and z have the same layout as a TVector3; the original returns `this` reinterpreted.
TVector3& TVector4::AsVector3()
{
	return *reinterpret_cast<TVector3*>(this);
}

const TVector3& TVector4::AsVector3() const
{
	return *reinterpret_cast<const TVector3*>(this);
}

void TVector4::Add(const TVector4& vec)
{
	x += vec.x;
	y += vec.y;
	z += vec.z;
}

void TVector4::Add(const TVector4& a, const TVector4& b)
{
	x = a.x + b.x;
	y = a.y + b.y;
	z = a.z + b.z;
	w = a.w;
}

void TVector4::Subtract(const TVector4& vec)
{
	x -= vec.x;
	y -= vec.y;
	z -= vec.z;
}

void TVector4::Subtract(const TVector4& a, const TVector4& b)
{
	x = a.x - b.x;
	y = a.y - b.y;
	z = a.z - b.z;
	w = a.w;
}

void TVector4::Subtract4(const TVector4& a, const TVector4& b)
{
	x = a.x - b.x;
	y = a.y - b.y;
	z = a.z - b.z;
	w = a.w - b.w;
}

void TVector4::Multiply(float scalar)
{
	x *= scalar;
	y *= scalar;
	z *= scalar;
}

void TVector4::Multiply(const TVector4& vec)
{
	x *= vec.x;
	y *= vec.y;
	z *= vec.z;
}

void TVector4::Multiply(const TVector4& vec, float scalar)
{
	x = vec.x * scalar;
	y = vec.y * scalar;
	z = vec.z * scalar;
	w = vec.w;
}

void TVector4::Multiply(const TVector4& a, const TVector4& b)
{
	x = a.x * b.x;
	y = a.y * b.y;
	z = a.z * b.z;
	w = a.w;
}

void TVector4::Multiply4(const TVector4& vec)
{
	x *= vec.x;
	y *= vec.y;
	z *= vec.z;
	w *= vec.w;
}

// Division by a scalar multiplies by the reciprocal, as the original does.
void TVector4::Divide(float scalar)
{
	const float reciprocal = 1.0f / scalar;
	x *= reciprocal;
	y *= reciprocal;
	z *= reciprocal;
}

void TVector4::Divide(const TVector4& vec)
{
	x /= vec.x;
	y /= vec.y;
	z /= vec.z;
}

void TVector4::Divide(const TVector4& vec, float scalar)
{
	const float reciprocal = 1.0f / scalar;
	x = vec.x * reciprocal;
	y = vec.y * reciprocal;
	z = vec.z * reciprocal;
	w = vec.w;
}

void TVector4::Divide(const TVector4& a, const TVector4& b)
{
	x = a.x / b.x;
	y = a.y / b.y;
	z = a.z / b.z;
	w = a.w;
}

void TVector4::Negate()
{
	Negate3();
}

void TVector4::Negate(const TVector4& vec)
{
	Negate3(vec);
}

void TVector4::Negate3()
{
	x = -x;
	y = -y;
	z = -z;
}

void TVector4::Negate3(const TVector4& vec)
{
	x = -vec.x;
	y = -vec.y;
	z = -vec.z;
	w = vec.w;
}

void TVector4::Negate4()
{
	x = -x;
	y = -y;
	z = -z;
	w = -w;
}

void TVector4::Negate4(const TVector4& vec)
{
	x = -vec.x;
	y = -vec.y;
	z = -vec.z;
	w = -vec.w;
}

void TVector4::Abs()
{
	x = std::fabs(x);
	y = std::fabs(y);
	z = std::fabs(z);
	w = std::fabs(w);
}

void TVector4::Abs(const TVector4& vec)
{
	x = std::fabs(vec.x);
	y = std::fabs(vec.y);
	z = std::fabs(vec.z);
	w = std::fabs(vec.w);
}

void TVector4::Sin(const TVector4& vec)
{
	x = TMath::Sin(vec.x);
	y = TMath::Sin(vec.y);
	z = TMath::Sin(vec.z);
	w = TMath::Sin(vec.w);
}

void TVector4::Cos(const TVector4& vec)
{
	x = TMath::Cos(vec.x);
	y = TMath::Cos(vec.y);
	z = TMath::Cos(vec.z);
	w = TMath::Cos(vec.w);
}

void TVector4::Max(float value)
{
	x = Select(x, value);
	y = Select(y, value);
	z = Select(z, value);
	w = Select(w, value);
}

void TVector4::Max(const TVector4& vec)
{
	x = Select(x, vec.x);
	y = Select(y, vec.y);
	z = Select(z, vec.z);
	w = Select(w, vec.w);
}

void TVector4::Max(const TVector4& vec, float value)
{
	x = Select(vec.x, value);
	y = Select(vec.y, value);
	z = Select(vec.z, value);
	w = Select(vec.w, value);
}

void TVector4::Max(const TVector4& a, const TVector4& b)
{
	x = Select(a.x, b.x);
	y = Select(a.y, b.y);
	z = Select(a.z, b.z);
	w = Select(a.w, b.w);
}

// Min keeps the first operand when `other >= first`.
void TVector4::Min(float value)
{
	x = value >= x ? x : value;
	y = value >= y ? y : value;
	z = value >= z ? z : value;
	w = value >= w ? w : value;
}

void TVector4::Min(const TVector4& vec)
{
	x = vec.x >= x ? x : vec.x;
	y = vec.y >= y ? y : vec.y;
	z = vec.z >= z ? z : vec.z;
	w = vec.w >= w ? w : vec.w;
}

void TVector4::Min(const TVector4& vec, float value)
{
	x = value >= vec.x ? vec.x : value;
	y = value >= vec.y ? vec.y : value;
	z = value >= vec.z ? vec.z : value;
	w = value >= vec.w ? vec.w : value;
}

void TVector4::Min(const TVector4& a, const TVector4& b)
{
	x = b.x >= a.x ? a.x : b.x;
	y = b.y >= a.y ? a.y : b.y;
	z = b.z >= a.z ? a.z : b.z;
	w = b.w >= a.w ? a.w : b.w;
}

void TVector4::Lerp(const TVector4& to, float t)
{
	Lerp(*this, to, t);
}

void TVector4::Lerp(const TVector4& from, const TVector4& to, float t)
{
	x = (to.x - from.x) * t + from.x;
	y = (to.y - from.y) * t + from.y;
	z = (to.z - from.z) * t + from.z;
	w = (to.w - from.w) * t + from.w;
}

void TVector4::Lerp3(const TVector4& to, float t)
{
	Lerp3(*this, to, t);
}

void TVector4::Lerp3(const TVector4& from, const TVector4& to, float t)
{
	x = (to.x - from.x) * t + from.x;
	y = (to.y - from.y) * t + from.y;
	z = (to.z - from.z) * t + from.z;
	w = from.w;
}

void TVector4::CrossProduct(const TVector4& a, const TVector4& b)
{
	const float newX = a.y * b.z - a.z * b.y;
	const float newY = a.z * b.x - a.x * b.z;
	const float newZ = a.x * b.y - a.y * b.x;
	const float newW = a.w;
	Set(newX, newY, newZ, newW);
}

void TVector4::RotateX(float angle)
{
	float sin;
	float cos;
	TMath::SinCos(angle, sin, cos);
	const float oldY = y;
	y = cos * oldY + -sin * z;
	z = oldY * sin + z * cos;
}

void TVector4::RotateY(float angle)
{
	float sin;
	float cos;
	TMath::SinCos(angle, sin, cos);
	const float oldX = x;
	x = cos * oldX + sin * z;
	z = z * cos + -sin * oldX;
}

void TVector4::RotateZ(float angle)
{
	float sin;
	float cos;
	TMath::SinCos(angle, sin, cos);
	const float oldX = x;
	x = cos * oldX + -sin * y;
	y = sin * oldX + y * cos;
}

// Normalises x, y and z; a zero vector becomes (0, 0, 1). w is left untouched.
void TVector4::Normalise()
{
	const float magnitudeSq = MagnitudeSq();
	if (magnitudeSq == 0.0f)
	{
		x = 0.0f;
		y = 0.0f;
		z = 1.0f;
		return;
	}
	const float reciprocal = 1.0f / std::sqrt(magnitudeSq);
	x *= reciprocal;
	y *= reciprocal;
	z *= reciprocal;
}

float TVector4::Magnitude() const
{
	return std::sqrt(MagnitudeSq());
}

float TVector4::MagnitudeSq() const
{
	return x * x + y * y + z * z;
}

bool TVector4::IsEqual(const TVector4& vec) const
{
	return x == vec.x && y == vec.y && z == vec.z && w == vec.w;
}

float TVector4::DotProduct(const TVector4& vec) const
{
	return DotProduct3(*this, vec);
}

float TVector4::DotProduct3(const TVector4& vec) const
{
	return DotProduct3(*this, vec);
}

float TVector4::DotProduct4(const TVector4& vec) const
{
	return DotProduct4(*this, vec);
}

TVector4& TVector4::operator+=(const TVector4& vec)
{
	Add(vec);
	return *this;
}

TVector4& TVector4::operator-=(const TVector4& vec)
{
	Subtract(vec);
	return *this;
}

TVector4& TVector4::operator*=(float scalar)
{
	Multiply(scalar);
	return *this;
}

TVector4& TVector4::operator*=(const TVector4& vec)
{
	Multiply(vec);
	return *this;
}

TVector4& TVector4::operator/=(float scalar)
{
	Divide(scalar);
	return *this;
}

TVector4& TVector4::operator/=(const TVector4& vec)
{
	Divide(vec);
	return *this;
}

TVector4 TVector4::operator+(const TVector4& vec) const
{
	TVector4 result;
	result.Add(*this, vec);
	return result;
}

TVector4 TVector4::operator-(const TVector4& vec) const
{
	TVector4 result;
	result.Subtract(*this, vec);
	return result;
}

TVector4 TVector4::operator*(float scalar) const
{
	TVector4 result;
	result.Multiply(*this, scalar);
	return result;
}

TVector4 TVector4::operator*(const TVector4& vec) const
{
	TVector4 result;
	result.Multiply(*this, vec);
	return result;
}

TVector4 TVector4::operator/(float scalar) const
{
	TVector4 result;
	result.Divide(*this, scalar);
	return result;
}

TVector4 TVector4::operator/(const TVector4& vec) const
{
	TVector4 result;
	result.Divide(*this, vec);
	return result;
}

bool TVector4::operator==(const TVector4& vec) const
{
	return IsEqual(vec);
}

bool TVector4::operator!=(const TVector4& vec) const
{
	return !IsEqual(vec);
}

float TSTDCALL TVector4::DotProduct(const TVector4& a, const TVector4& b)
{
	return DotProduct3(a, b);
}

float TSTDCALL TVector4::DotProduct3(const TVector4& a, const TVector4& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

float TSTDCALL TVector4::DotProduct4(const TVector4& a, const TVector4& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

float TSTDCALL TVector4::Distance(const TVector4& a, const TVector4& b)
{
	return std::sqrt(DistanceSq(a, b));
}

float TSTDCALL TVector4::DistanceSq(const TVector4& a, const TVector4& b)
{
	const float dx = b.x - a.x;
	const float dy = b.y - a.y;
	const float dz = b.z - a.z;
	return dx * dx + dy * dy + dz * dz;
}

} // namespace Toshi
