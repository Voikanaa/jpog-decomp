#pragma once

#include "common/layout.h"
#include "toshi/kernel/TPlane.h"
#include "toshi/kernel/TVector3.h"

namespace Toshi
{

// Axis-aligned box (TKernelInterface.dll): centre and half extents ("bounds").
class TAABox
{
public:
	TAABox();
	TAABox(const float& x, const float& y, const float& z, const float& boundsX, const float& boundsY,
	       const float& boundsZ);
	TAABox(const TVector3& centre, const TVector3& bounds);
	TAABox(const TAABox& other);
	TAABox& operator=(const TAABox& other);

	void SetBox(const float& x, const float& y, const float& z, const float& boundsX, const float& boundsY,
	            const float& boundsZ);
	void SetCentre(const TVector3& centre);
	void SetBounds(const TVector3& bounds);
	const TVector3& GetCentre() const;
	const TVector3& GetBounds() const;

	// Becomes the box containing the `count` boxes (unchanged for none).
	void BoxUnion(TAABox* boxes, unsigned int count);
	// Negative: entirely behind the plane; positive: entirely on the side the normal points to
	// (the opposite of TSphere).
	TPlane::PlaneComparison ComparePlane(TPlane& plane);

private:
	TVector3 m_oCentre;
	TVector3 m_oBounds;
};

JPOG_ASSERT_SIZE(TAABox, 24);

} // namespace Toshi
