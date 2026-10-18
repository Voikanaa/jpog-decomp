#include "toshi/kernel/TMath.h"
#include "toshi/kernel/TVector2.h"
#include "toshi/kernel/TVector3.h"

#include <cmath>

namespace Toshi
{

// Evaluation order follows the original's x87 code (see TMath.cpp for the precision model).
// Comparisons keep the original's NaN behaviour, hence some negated conditions.

bool TSTDCALL TMath::PointVsPlane2D(const TVector2& normal, const TVector2& planePoint, const TVector2& point)
{
	TVector2 offset = point;
	offset.Subtract(planePoint);
	return TVector2::DotProduct(normal, offset) >= 0.0f;
}

float TSTDCALL TMath::PointVsPlane2DF(const TVector2& normal, const TVector2& planePoint, const TVector2& point)
{
	TVector2 offset = point;
	offset.Subtract(planePoint);
	return TVector2::DotProduct(normal, offset);
}

bool TSTDCALL TMath::PointVsPlane3D(const TVector3& normal, const TVector3& planePoint, const TVector3& point)
{
	TVector3 offset = point;
	offset.Subtract(planePoint);
	return TVector3::DotProduct(normal, offset) >= 0.0f;
}

float TSTDCALL TMath::ClosestPointOnLine2D(const TVector2& start, const TVector2& end, const TVector2& point)
{
	TVector2 toPoint = point;
	TVector2 line = end;
	toPoint.Subtract(start);
	line.Subtract(start);
	return TVector2::DotProduct(toPoint, line) / line.MagnitudeSq();
}

float TSTDCALL TMath::DistancePointToLineSq(const TVector2& point, const TVector2& start, const TVector2& end)
{
	TVector2 toPoint = point;
	TVector2 line = end;
	toPoint.Subtract(start);
	line.Subtract(start);
	const float t = TVector2::DotProduct(toPoint, line) / line.MagnitudeSq();
	TVector2 offset = line;
	offset.Multiply(t);
	offset.Add(start);
	offset.Subtract(point);
	return offset.MagnitudeSq();
}

float TSTDCALL TMath::DistancePointToLine(const TVector2& point, const TVector2& start, const TVector2& end)
{
	return std::sqrt(DistancePointToLineSq(point, start, end));
}

float TSTDCALL TMath::ClosestPointOnLineOverTime2D(const TVector2& a, const TVector2& b, const TVector2& c,
                                                   const TVector2& d, float time)
{
	TVector2 line = b;
	line.Subtract(a);
	TVector2 negated = line;
	negated.Negate();
	const float denominator = (TVector2::DotProduct(c, negated) + time) * line.MagnitudeSq();
	TVector2 offset = a;
	offset.Subtract(d);
	offset.Subtract(b);
	const float dot = TVector2::DotProduct(a, offset);
	return ((TVector2::DotProduct(b, d) + dot) * time) / denominator;
}

// Parallel (or NaN) lines fail.
bool TSTDCALL TMath::LineVsLine2D(const TVector2& a0, const TVector2& a1, const TVector2& b0, const TVector2& b1,
                                  float& t)
{
	TVector2 a = a1;
	TVector2 b = b1;
	a.Subtract(a0);
	b.Subtract(b0);
	const float denominator = b.y * a.x - b.x * a.y;
	if (denominator == 0.0f || std::isnan(denominator))
		return false;
	TVector2 offset = a0;
	offset.Subtract(b0);
	t = (b.x * offset.y - b.y * offset.x) / denominator;
	return true;
}

bool TSTDCALL TMath::LineVsLine2D(const TVector2& a0, const TVector2& a1, const TVector2& b0, const TVector2& b1,
                                  float& t, float& u)
{
	TVector2 a = a1;
	TVector2 b = b1;
	a.Subtract(a0);
	b.Subtract(b0);
	const float denominator = b.y * a.x - b.x * a.y;
	if (denominator == 0.0f || std::isnan(denominator))
		return false;
	TVector2 offset = a0;
	offset.Subtract(b0);
	const float inverse = 1.0f / denominator;
	t = (b.x * offset.y - b.y * offset.x) * inverse;
	u = (offset.y * a.x - offset.x * a.y) * inverse;
	return true;
}

bool TSTDCALL TMath::LineVsLine2D(const TVector2& a0, const TVector2& a1, const TVector2& b0, const TVector2& b1,
                                  TVector2& intersection)
{
	float t;
	if (!LineVsLine2D(a0, a1, b0, b1, t))
		return false;
	intersection = a1;
	intersection.Subtract(a0);
	intersection.Multiply(t);
	intersection.Add(a0);
	return true;
}

// A NaN `t` passes the range checks, as in the original.
bool TSTDCALL TMath::LineSegmentVsLineSegment2D(const TVector2& a0, const TVector2& a1, const TVector2& b0,
                                                const TVector2& b1, TVector2& intersection)
{
	float t, u;
	if (!LineVsLine2D(a0, a1, b0, b1, t, u))
		return false;
	if (0.0f > t || !(u >= 0.0f) || t > 1.0f || u > 1.0f)
		return false;
	intersection = a1;
	intersection.Subtract(a0);
	intersection.Multiply(t);
	intersection.Add(a0);
	return true;
}

bool TSTDCALL TMath::LineSegmentVsLine2D(const TVector2& a0, const TVector2& a1, const TVector2& b0, const TVector2& b1,
                                         TVector2& intersection)
{
	float t;
	if (!LineVsLine2D(a0, a1, b0, b1, t))
		return false;
	if (!(t >= 0.0f) || t > 1.0f)
		return false;
	intersection = a1;
	intersection.Subtract(a0);
	intersection.Multiply(t);
	intersection.Add(a0);
	return true;
}

bool TSTDCALL TMath::RaySegmentVsLine2D(const TVector2& origin, const TVector2& direction, const TVector2& point,
                                        const TVector2& lineDirection, float& t)
{
	const float denominator = lineDirection.y * direction.x - lineDirection.x * direction.y;
	if (denominator == 0.0f || std::isnan(denominator))
		return false;
	TVector2 offset = origin;
	offset.Subtract(point);
	const float hit = (lineDirection.x * offset.y + -offset.x * lineDirection.y) / denominator;
	if (hit >= t || !(hit >= 0.0f))
		return false;
	t = hit;
	return true;
}

int TSTDCALL TMath::GetCapsuleZone(const TVector2& point, const TVector2& start, const TVector2& end, float)
{
	TVector2 axis = end;
	axis.Subtract(start);
	if (PointVsPlane2D(axis, end, point))
		return 2;
	return PointVsPlane2D(axis, start, point) ? 0 : 1;
}

} // namespace Toshi
