#include "toshi/kernel/math/TSphere.h"

#include <cmath>

namespace Toshi
{

TSphere::TSphere()
{
}

TSphere::TSphere(float x, float y, float z, float radius) : m_oCentre(x, y, z), m_fRadius(radius)
{
}

TSphere::TSphere(const TVector3& centre, float radius) : m_oCentre(centre), m_fRadius(radius)
{
}

TSphere::TSphere(const TVector4& sphere) : m_oCentre(sphere.x, sphere.y, sphere.z), m_fRadius(sphere.w)
{
}

TSphere::TSphere(const TSphere& other) = default;

TSphere& TSphere::operator=(const TSphere& other) = default;

void TSphere::Set(float x, float y, float z, float radius)
{
	m_oCentre = TVector3(x, y, z);
	m_fRadius = radius;
}

void TSphere::Set(const TVector3& centre, float radius)
{
	m_oCentre = centre;
	m_fRadius = radius;
}

void TSphere::Set(const TVector4& sphere)
{
	m_oCentre = TVector3(sphere.x, sphere.y, sphere.z);
	m_fRadius = sphere.w;
}

void TSphere::Set(const TSphere& sphere)
{
	*this = sphere;
}

TVector4& TSphere::AsVector4()
{
	return *reinterpret_cast<TVector4*>(this);
}

const TVector4& TSphere::AsVector4() const
{
	return *reinterpret_cast<const TVector4*>(this);
}

// The radius is stored before `a` is read again, so `this` may alias either sphere with the
// original's results.
void TSphere::Union(TSphere& a, TSphere& b)
{
	TVector3 offset;
	offset.Subtract(b.m_oCentre, a.m_oCentre);
	const float distanceSq = offset.MagnitudeSq();
	const float radiusDifference = b.m_fRadius - a.m_fRadius;
	if (radiusDifference > 0.0f)
	{
		if (radiusDifference * radiusDifference > distanceSq)
		{
			*this = b;
			return;
		}
	}
	else if (radiusDifference * radiusDifference > distanceSq)
	{
		*this = a;
		return;
	}
	const float distance = std::sqrt(distanceSq);
	m_fRadius = ((a.m_fRadius + b.m_fRadius) + distance) * 0.5f;
	offset.Multiply((m_fRadius - a.m_fRadius) / distance);
	m_oCentre.Add(a.m_oCentre, offset);
}

TPlane::PlaneComparison TSphere::ComparePlane(TPlane& plane)
{
	const float distance = TVector3::DotProduct(plane.AsNormal(), m_oCentre) - plane.d;
	if (!(m_fRadius >= distance)) // also for NaN
		return TPlane::PlaneComparison_Negative;
	if (-m_fRadius > distance)
		return TPlane::PlaneComparison_Positive;
	return TPlane::PlaneComparison_Intersects;
}

} // namespace Toshi
