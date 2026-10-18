#pragma once

#include "common/layout.h"

namespace Toshi
{

class TVector3;
class TVector4;

// Plane n·p = d (TKernelInterface.dll): normal (x, y, z) and distance d, laid out like a TVector4.
class TPlane
{
public:
	// Result of TSphere/TAABox::ComparePlane. The sign convention differs between the two (see
	// there), as in the original.
	enum PlaneComparison
	{
		PlaneComparison_Negative = -1,
		PlaneComparison_Intersects = 0,
		PlaneComparison_Positive = 1,
	};

	TPlane();
	TPlane(float x, float y, float z, float d);
	TPlane(const TVector3& normal, float d);
	// The plane with `normal` through `point`.
	TPlane(const TVector3& normal, const TVector3& point);
	explicit TPlane(const TVector4& plane);
	TPlane(const TPlane& other);
	TPlane& operator=(const TPlane& other);

	void Set(float x, float y, float z, float d);
	void Set(const TVector3& normal, float d);
	void Set(const TVector3& normal, const TVector3& point);
	void Set(const TVector4& plane);
	void Set(const TPlane& plane);

	void SetNormal(float x, float y, float z);
	void SetNormal(const TVector3& normal);
	const TVector3& GetNormal() const;
	void SetD(float d);
	float GetD() const;
	// Moves the plane through `point` (d = n·point).
	void SetPoint(const TVector3& point);
	// The point of the plane closest to the origin (n * d, for a unit normal).
	void GetPoint(TVector3& point) const;

	void Negate();
	void Negate(const TPlane& plane);

	TVector3& AsNormal();
	const TVector3& AsNormal() const;
	TVector4& AsVector4();
	const TVector4& AsVector4() const;

	float x;
	float y;
	float z;
	float d;
};

JPOG_ASSERT_SIZE(TPlane, 16);

} // namespace Toshi
