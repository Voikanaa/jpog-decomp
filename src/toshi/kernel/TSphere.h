#pragma once

#include "common/layout.h"
#include "toshi/kernel/TPlane.h"
#include "toshi/kernel/TVector3.h"
#include "toshi/kernel/TVector4.h"

namespace Toshi
{

// Bounding sphere (TKernelInterface.dll): centre and radius, laid out like a TVector4.
class TSphere
{
public:
	TSphere();
	TSphere(float x, float y, float z, float radius);
	TSphere(const TVector3& centre, float radius);
	explicit TSphere(const TVector4& sphere);
	TSphere(const TSphere& other);
	TSphere& operator=(const TSphere& other);

	void Set(float x, float y, float z, float radius);
	void Set(const TVector3& centre, float radius);
	void Set(const TVector4& sphere);
	void Set(const TSphere& sphere);

	TVector4& AsVector4();
	const TVector4& AsVector4() const;

	// The smallest sphere containing both (one of them if it contains the other).
	void Union(TSphere& a, TSphere& b);
	// Negative: entirely on the side the normal points to; positive: entirely behind.
	TPlane::PlaneComparison ComparePlane(TPlane& plane);

private:
	TVector3 m_oCentre;
	float m_fRadius;
};

JPOG_ASSERT_SIZE(TSphere, 16);

} // namespace Toshi
