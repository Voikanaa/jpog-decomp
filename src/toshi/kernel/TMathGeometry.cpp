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

int TSTDCALL TMath::RayVsCircle2D(const TVector2& origin, const TVector2& direction, const TVector2& centre,
                                  float radius, float& t0, float& t1)
{
	TVector2 offset = centre;
	offset.Subtract(origin);
	const float radiusSq = radius * radius;
	const float distanceSq = offset.MagnitudeSq();
	if (distanceSq >= radiusSq)
	{
		// Outside: the circle must be ahead and close enough to the ray.
		const float along = TVector2::DotProduct(offset, direction);
		if (!(along >= 0.0f) || !(radiusSq >= distanceSq - along * along))
			return 0;
	}
	offset.Multiply(-1.0f);
	const float b = TVector2::DotProduct(offset, direction) + TVector2::DotProduct(offset, direction);
	const float discriminant = b * b + (distanceSq - radiusSq) * -4.0f;
	if (!(discriminant >= 0.0f))
		return 0;
	const float root = std::sqrt(discriminant);
	t0 = -(root + b) * 0.5f;
	t1 = (root - b) * 0.5f;
	return 2;
}

int TSTDCALL TMath::RayVsCircle3D(const TVector3& origin, const TVector3& direction, const TVector3& centre,
                                  float radius, float& t0, float& t1)
{
	TVector3 offset = centre;
	offset.Subtract(origin);
	const float radiusSq = radius * radius;
	const float distanceSq = offset.MagnitudeSq();
	if (distanceSq >= radiusSq)
	{
		const float along = TVector3::DotProduct(offset, direction);
		if (!(along >= 0.0f) || !(radiusSq >= distanceSq - along * along))
			return 0;
	}
	offset.Multiply(-1.0f);
	const float b = TVector3::DotProduct(offset, direction) + TVector3::DotProduct(offset, direction);
	const float discriminant = b * b + (distanceSq - radiusSq) * -4.0f;
	if (!(discriminant >= 0.0f))
		return 0;
	const float root = std::sqrt(discriminant);
	t0 = -(root + b) * 0.5f;
	t1 = (root - b) * 0.5f;
	return 2;
}

// A NaN distance counts as overlapping here (but not in the planar version), as in the original.
bool TSTDCALL TMath::SphereIntersection(const TVector3& centreA, float radiusA, const TVector3& centreB, float radiusB,
                                        float* distanceSqMinusRadiusSq, TVector3* separation, float* penetration)
{
	const float radiusSum = radiusB + radiusA;
	TVector3 offset;
	offset.Subtract(centreB, centreA);
	const float radiusSumSq = radiusSum * radiusSum;
	const float distanceSq = offset.MagnitudeSq();
	if (distanceSq > radiusSumSq)
		return false;
	const float depth = radiusSum - std::sqrt(distanceSq);
	if (penetration != nullptr)
		*penetration = depth;
	if (separation != nullptr)
		separation->Multiply(offset, depth / radiusSum);
	if (distanceSqMinusRadiusSq != nullptr)
		*distanceSqMinusRadiusSq = distanceSq - radiusB * radiusB;
	return true;
}

bool TSTDCALL TMath::SphereIntersectionPlanar(const TVector3& centreA, float radiusA, const TVector3& centreB,
                                              float radiusB, float* distanceSqMinusRadiusSq, TVector3* separation,
                                              float* penetration)
{
	const float radiusSum = radiusB + radiusA;
	const float dx = centreB.x - centreA.x;
	const float dz = centreB.z - centreA.z;
	const float distanceSq = dx * dx + dz * dz;
	if (!(radiusSum * radiusSum >= distanceSq))
		return false;
	const float depth = radiusSum - std::sqrt(distanceSq);
	if (penetration != nullptr)
		*penetration = depth;
	if (separation != nullptr)
	{
		const float scale = depth / radiusSum;
		separation->x = dx * scale;
		separation->z = dz * scale;
		separation->y = 0.0f;
	}
	if (distanceSqMinusRadiusSq != nullptr)
		*distanceSqMinusRadiusSq = distanceSq - radiusB * radiusB;
	return true;
}

float TSTDCALL TMath::SphereIntersectionDynamic(const TVector3& centreA, float radiusA, const TVector3& velocityA,
                                                const TVector3& centreB, float radiusB, const TVector3& velocityB)
{
	const TVector3 velocity = velocityA - velocityB;
	const float speed = velocity.Magnitude();
	const TVector3 direction = velocity / speed;
	const TVector3 offset = centreB - centreA;
	const float distance = offset.Magnitude();
	const float radiusSum = radiusB + radiusA;
	if (!(distance > radiusSum))
		return 0.0f;
	if (!(speed >= TMath::FLOATEPSILON))
		return -1.0f;
	if (!(TVector3::DotProduct(offset, velocity) >= 0.0f))
		return -1.0f;
	const float along = TVector3::DotProduct(offset, direction);
	const float missSq = distance * distance - along * along;
	const float radiusSumSq = radiusSum * radiusSum;
	if (!(radiusSumSq >= missSq))
		return -1.0f;
	const float inside = radiusSumSq - missSq;
	if (!(inside >= 0.0f))
		return -1.0f;
	return (along - std::sqrt(inside)) / speed;
}

// A NaN projection is treated as before the start.
void TSTDCALL TMath::CalculateCapsuleNormal2D(const TVector2& start, const TVector2& end, const TVector2& point,
                                              TVector2& normal)
{
	const float t = ClosestPointOnLine2D(start, end, point);
	if (!(t >= 0.0f))
	{
		normal = point;
		normal.Subtract(start);
		normal.Normalise();
		return;
	}
	if (t > 1.0f)
	{
		normal = point;
		normal.Subtract(end);
		normal.Normalise();
		return;
	}
	normal = end;
	normal.Subtract(start);
	normal.Normalise();
	normal.SetPerpendicular();
	if (!PointVsPlane2D(normal, start, point))
		normal.Negate();
}

bool TSTDCALL TMath::PointInsideCapsule2D(const TVector2& start, const TVector2& end, float radius,
                                          const TVector2& point)
{
	float t = ClosestPointOnLine2D(start, end, point);
	Clip(t, 0.0f, 1.0f);
	TVector2 closest;
	closest.Lerp(start, end, t);
	const TVector2 offset = closest - point;
	return radius * radius >= offset.MagnitudeSq();
}

// fsin/fcos ignore the 24-bit precision: their double results are rounded to float only where
// the original stores them (see TMath.cpp).
void TSTDCALL TMath::GetAxisChangeLimit3D(const TVector3& from, const TVector3& to, const TVector3& fallbackAxis,
                                          float angle, TVector3& result)
{
	const float dot = TVector3::DotProduct(from, to);
	if (!(angle > -PI))
		angle = -PI;
	else if (!(angle < PI))
		angle = PI;
	const double cosine = std::cos(static_cast<double>(angle));
	if (!(cosine > static_cast<double>(dot)))
	{
		result = to;
		return;
	}
	const double sine = std::sin(static_cast<double>(angle));
	TVector3 axis;
	axis.CrossProduct(to, from);
	const float length = axis.Magnitude();
	if (length >= 1e-6f)
		axis.Multiply(1.0f / length);
	else
	{
		axis.CrossProduct(fallbackAxis, from);
		axis.Normalise();
	}
	TVector3 tangent;
	tangent.CrossProduct(from, axis);
	tangent.Multiply(static_cast<float>(sine));
	result = from;
	result.Multiply(static_cast<float>(cosine));
	result.Add(tangent);
}

namespace
{

// The larger hit parameter (the first for NaN).
float LaterHit(float t0, float t1)
{
	return t1 >= t0 ? t1 : t0;
}

} // namespace

// The ray's zones (see GetCapsuleZone) select the parts to test: the sides when either end of the
// ray lies between the caps or the ray crosses from cap to cap, and each cap whose zone it
// touches. Quirks kept: side hits are accepted up to twice the ray length, and a side hit's `t`
// is the ray's length times the parameter (only right for a unit direction).
bool TSTDCALL TMath::RayVsCapsule2D(const TVector2& origin, const TVector2& direction, float& t, const TVector2& start,
                                    const TVector2& end, float radius)
{
	const TVector2 step = direction * t;
	const TVector2 rayEnd = step + origin;
	bool zones[3] = {false, false, false};
	zones[GetCapsuleZone(origin, start, end, radius)] = true;
	zones[GetCapsuleZone(rayEnd, start, end, radius)] = true;
	if (zones[1] && zones[2])
		zones[0] = true;
	bool hit = false;
	if (zones[0])
	{
		TVector2 axis = end;
		axis.Subtract(start);
		axis.Normalise();
		TVector2 side = axis;
		side.SetPerpendicular();
		side.Multiply(radius);
		const TVector2 startLeft = start + side;
		const TVector2 startRight = start - side;
		const TVector2 endLeft = end + side;
		const TVector2 endRight = end - side;
		float best = 2.0f;
		bool sideHit = false;
		float along, across;
		if (LineVsLine2D(origin, rayEnd, startLeft, endLeft, along, across) && across >= 0.0f && !(across > 1.0f) &&
		    along >= 0.0f && !(along >= best))
		{
			best = along;
			sideHit = true;
		}
		if (LineVsLine2D(origin, rayEnd, startRight, endRight, along, across) && across >= 0.0f && !(across > 1.0f) &&
		    along >= 0.0f && !(along >= best))
		{
			best = along;
			sideHit = true;
		}
		if (sideHit)
		{
			TVector2 ray = rayEnd;
			ray.Subtract(origin);
			hit = true;
			t = ray.Magnitude() * best;
		}
	}
	float t0, t1;
	if (zones[1] && RayVsCircle2D(origin, direction, start, radius, t0, t1) > 0 && !(t0 >= t) && t0 >= 0.0f)
	{
		hit = true;
		t = t0;
	}
	if (zones[2] && RayVsCircle2D(origin, direction, end, radius, t0, t1) > 0 && !(t0 >= t) && t0 >= 0.0f)
	{
		hit = true;
		t = t0;
	}
	return hit;
}

// In the middle zone the first ray test (against the offset side) only lowers the limit for the
// cap tests; its result is ignored, as in the original.
float TSTDCALL TMath::PushPointOutOfCapsule2D(const TVector2& start, const TVector2& end, float radius,
                                              const TVector2& normal, const TVector2& direction, TVector2& point,
                                              float& distance)
{
	float limit = 999999.875f;
	TVector2 axis = end;
	axis.Subtract(start);
	distance = 0.0f;
	float t0, t1;
	float hit;
	const bool beyondEnd = PointVsPlane2DF(axis, end, point) > 0.0f;
	if (beyondEnd || !(PointVsPlane2DF(axis, start, point) >= 0.0f))
	{
		// Beyond one of the ends: push out of that end's circle.
		const TVector2& centre = beyondEnd ? end : start;
		TVector2 offset = point;
		offset.Subtract(centre);
		if (!(radius * radius >= offset.MagnitudeSq()))
			return 0.0f;
		if (RayVsCircle2D(point, direction, centre, radius, t0, t1) == 0)
			return 0.0f;
		hit = LaterHit(t0, t1);
	}
	else
	{
		TVector2 side = normal;
		side.Multiply(radius);
		side.Add(start);
		if (PointVsPlane2D(normal, side, point))
			return 0.0f;
		RaySegmentVsLine2D(point, direction, side, axis, limit);
		const TVector2* cap = nullptr;
		if (RaySegmentVsLine2D(point, direction, start, normal, limit))
			cap = &start;
		else if (RaySegmentVsLine2D(point, direction, end, normal, limit))
			cap = &end;
		hit = limit;
		if (cap != nullptr && RayVsCircle2D(point, direction, *cap, radius, t0, t1) > 0)
			hit = LaterHit(t0, t1);
	}
	if (hit > 0.0f)
		hit += 0.01f;
	distance = hit;
	return 1.0f;
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
