#include "toshi/kernel/TPlane.h"

#include "toshi/kernel/TVector3.h"
#include "toshi/kernel/TVector4.h"

namespace Toshi
{

// The default constructor leaves the plane uninitialised, like the original.
TPlane::TPlane()
{
}

TPlane::TPlane(float newX, float newY, float newZ, float newD) : x(newX), y(newY), z(newZ), d(newD)
{
}

TPlane::TPlane(const TVector3& normal, float newD) : x(normal.x), y(normal.y), z(normal.z), d(newD)
{
}

TPlane::TPlane(const TVector3& normal, const TVector3& point) : x(normal.x), y(normal.y), z(normal.z)
{
	d = TVector3::DotProduct(normal, point);
}

TPlane::TPlane(const TVector4& plane) : x(plane.x), y(plane.y), z(plane.z), d(plane.w)
{
}

TPlane::TPlane(const TPlane& other) : x(other.x), y(other.y), z(other.z), d(other.d)
{
}

TPlane& TPlane::operator=(const TPlane& other)
{
	Set(other);
	return *this;
}

void TPlane::Set(float newX, float newY, float newZ, float newD)
{
	x = newX;
	y = newY;
	z = newZ;
	d = newD;
}

void TPlane::Set(const TVector3& normal, float newD)
{
	SetNormal(normal);
	d = newD;
}

void TPlane::Set(const TVector3& normal, const TVector3& point)
{
	SetNormal(normal);
	d = TVector3::DotProduct(normal, point);
}

void TPlane::Set(const TVector4& plane)
{
	Set(plane.x, plane.y, plane.z, plane.w);
}

void TPlane::Set(const TPlane& plane)
{
	Set(plane.x, plane.y, plane.z, plane.d);
}

void TPlane::SetNormal(float newX, float newY, float newZ)
{
	x = newX;
	y = newY;
	z = newZ;
}

void TPlane::SetNormal(const TVector3& normal)
{
	SetNormal(normal.x, normal.y, normal.z);
}

const TVector3& TPlane::GetNormal() const
{
	return AsNormal();
}

void TPlane::SetD(float newD)
{
	d = newD;
}

float TPlane::GetD() const
{
	return d;
}

void TPlane::SetPoint(const TVector3& point)
{
	d = TVector3::DotProduct(AsNormal(), point);
}

void TPlane::GetPoint(TVector3& point) const
{
	point.Multiply(AsNormal(), d);
}

void TPlane::Negate()
{
	AsVector4().Negate4();
}

void TPlane::Negate(const TPlane& plane)
{
	AsVector4().Negate4(plane.AsVector4());
}

// The normal and the full plane share the TVector3 / TVector4 layout; the original returns `this`.
TVector3& TPlane::AsNormal()
{
	return *reinterpret_cast<TVector3*>(this);
}

const TVector3& TPlane::AsNormal() const
{
	return *reinterpret_cast<const TVector3*>(this);
}

TVector4& TPlane::AsVector4()
{
	return *reinterpret_cast<TVector4*>(this);
}

const TVector4& TPlane::AsVector4() const
{
	return *reinterpret_cast<const TVector4*>(this);
}

} // namespace Toshi
