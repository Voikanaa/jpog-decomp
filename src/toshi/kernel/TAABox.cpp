#include "toshi/kernel/TAABox.h"

namespace Toshi
{

TAABox::TAABox()
{
}

TAABox::TAABox(const float& x, const float& y, const float& z, const float& boundsX, const float& boundsY,
               const float& boundsZ)
    : m_oCentre(x, y, z), m_oBounds(boundsX, boundsY, boundsZ)
{
}

TAABox::TAABox(const TVector3& centre, const TVector3& bounds) : m_oCentre(centre), m_oBounds(bounds)
{
}

TAABox::TAABox(const TAABox& other) = default;

TAABox& TAABox::operator=(const TAABox& other) = default;

void TAABox::SetBox(const float& x, const float& y, const float& z, const float& boundsX, const float& boundsY,
                    const float& boundsZ)
{
	m_oCentre = TVector3(x, y, z);
	m_oBounds = TVector3(boundsX, boundsY, boundsZ);
}

void TAABox::SetCentre(const TVector3& centre)
{
	m_oCentre = centre;
}

void TAABox::SetBounds(const TVector3& bounds)
{
	m_oBounds = bounds;
}

const TVector3& TAABox::GetCentre() const
{
	return m_oCentre;
}

const TVector3& TAABox::GetBounds() const
{
	return m_oBounds;
}

namespace
{

// Ties take the new value; a NaN comparison keeps the current one.
float Min(float current, float value)
{
	return current >= value ? value : current;
}

float Max(float current, float value)
{
	return value >= current ? value : current;
}

} // namespace

// The new bounds are measured from the new centre as stored (max - centre).
void TAABox::BoxUnion(TAABox* boxes, unsigned int count)
{
	if (boxes == nullptr || count == 0)
		return;
	TVector3 minimum = boxes[0].m_oCentre - boxes[0].m_oBounds;
	TVector3 maximum = boxes[0].m_oCentre + boxes[0].m_oBounds;
	for (unsigned int i = 1; i < count; ++i)
	{
		const TVector3 low = boxes[i].m_oCentre - boxes[i].m_oBounds;
		const TVector3 high = boxes[i].m_oCentre + boxes[i].m_oBounds;
		minimum = TVector3(Min(minimum.x, low.x), Min(minimum.y, low.y), Min(minimum.z, low.z));
		maximum = TVector3(Max(maximum.x, high.x), Max(maximum.y, high.y), Max(maximum.z, high.z));
	}
	m_oCentre = (maximum + minimum) * 0.5f;
	m_oBounds = maximum - m_oCentre;
}

TPlane::PlaneComparison TAABox::ComparePlane(TPlane& plane)
{
	TVector3 absNormal;
	absNormal.Abs(plane.AsNormal());
	const float distance = TVector3::DotProduct(m_oCentre, plane.AsNormal()) - plane.d;
	const float extent = TVector3::DotProduct(m_oBounds, absNormal);
	if (!(distance + extent >= 0.0f)) // also for NaN
		return TPlane::PlaneComparison_Negative;
	if (distance - extent > 0.0f)
		return TPlane::PlaneComparison_Positive;
	return TPlane::PlaneComparison_Intersects;
}

} // namespace Toshi
