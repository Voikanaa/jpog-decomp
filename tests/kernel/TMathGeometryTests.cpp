#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TMath.h"
#include "toshi/kernel/TVector2.h"
#include "toshi/kernel/TVector3.h"

#include <cmath>
#include <doctest/doctest.h>

using Toshi::TMath;
using Toshi::TVector2;
using Toshi::TVector3;
using namespace jpog::test;

namespace
{

bool Same(const TVector2& a, const TVector2& b)
{
	return jpog::test::Same(a.x, b.x) && jpog::test::Same(a.y, b.y);
}

// Inputs that make degenerate cases likely: small integers (parallel and collinear lines,
// shared points), zeros and ordinary values.
class GeometryInputs
{
public:
	float Value(float range)
	{
		switch (m_gen.NextU32() % 6)
		{
		case 0:
			return 0.0f;
		case 1:
		case 2:
			return static_cast<float>(m_gen.NextInt(-3, 3));
		default:
			return m_gen.NextFloat(-range, range);
		}
	}
	TVector2 Vector2(float range = 20.0f) { return TVector2(Value(range), Value(range)); }
	TVector3 Vector3(float range = 20.0f) { return TVector3(Value(range), Value(range), Value(range)); }
	InputGenerator& Gen() { return m_gen; }

private:
	InputGenerator m_gen;
};

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TMath geometry")
{
	TEST_CASE("points, planes and lines")
	{
		const TVector2 origin(0.0f, 0.0f), x(1.0f, 0.0f), y(0.0f, 1.0f);
		CHECK(TMath::PointVsPlane2D(x, origin, TVector2(2.0f, 5.0f)));
		CHECK_FALSE(TMath::PointVsPlane2D(x, origin, TVector2(-2.0f, 5.0f)));
		CHECK(TMath::PointVsPlane2DF(x, origin, TVector2(3.0f, 1.0f)) == 3.0f);
		CHECK(
		    TMath::PointVsPlane3D(TVector3(0.0f, 1.0f, 0.0f), TVector3(0.0f, 0.0f, 0.0f), TVector3(0.0f, 0.0f, 0.0f)));
		CHECK(TMath::ClosestPointOnLine2D(origin, TVector2(4.0f, 0.0f), TVector2(1.0f, 7.0f)) == 0.25f);
		CHECK(TMath::DistancePointToLineSq(TVector2(1.0f, 3.0f), origin, x) == 9.0f);
		CHECK(TMath::DistancePointToLine(TVector2(1.0f, 3.0f), origin, x) == 3.0f);

		TVector2 hit;
		CHECK(TMath::LineVsLine2D(TVector2(-1.0f, 0.0f), x, TVector2(0.0f, -1.0f), y, hit));
		CHECK(Same(hit, origin));
		CHECK_FALSE(TMath::LineVsLine2D(origin, x, y, TVector2(1.0f, 1.0f), hit)); // parallel
		CHECK(TMath::LineSegmentVsLineSegment2D(TVector2(-1.0f, 0.0f), x, TVector2(0.0f, -1.0f), y, hit));
		CHECK_FALSE(TMath::LineSegmentVsLineSegment2D(TVector2(1.0f, 0.0f), TVector2(2.0f, 0.0f), TVector2(0.0f, -1.0f),
		                                              y, hit));
		CHECK(TMath::LineSegmentVsLine2D(TVector2(-1.0f, 0.0f), x, TVector2(0.0f, 5.0f), TVector2(0.0f, 6.0f), hit));
		float t = 10.0f;
		CHECK(TMath::RaySegmentVsLine2D(origin, x, TVector2(4.0f, 0.0f), y, t));
		CHECK(t == 4.0f);
		CHECK_FALSE(TMath::RaySegmentVsLine2D(origin, x, TVector2(5.0f, 0.0f), y, t)); // beyond t
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

using V2 = const TVector2&;
using V3 = const TVector3&;

} // namespace

TEST_SUITE("TMath geometry differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("points, planes and lines match the original")
	{
		const auto pointVsPlane2D =
		    Original<bool(__stdcall*)(V2, V2, V2)>("?PointVsPlane2D@TMath@Toshi@@SG_NABVTVector2@2@00@Z");
		const auto pointVsPlane2DF =
		    Original<float(__stdcall*)(V2, V2, V2)>("?PointVsPlane2DF@TMath@Toshi@@SGMABVTVector2@2@00@Z");
		const auto pointVsPlane3D =
		    Original<bool(__stdcall*)(V3, V3, V3)>("?PointVsPlane3D@TMath@Toshi@@SG_NABVTVector3@2@00@Z");
		const auto closest =
		    Original<float(__stdcall*)(V2, V2, V2)>("?ClosestPointOnLine2D@TMath@Toshi@@SGMABVTVector2@2@00@Z");
		const auto distanceSq =
		    Original<float(__stdcall*)(V2, V2, V2)>("?DistancePointToLineSq@TMath@Toshi@@SGMABVTVector2@2@00@Z");
		const auto distance =
		    Original<float(__stdcall*)(V2, V2, V2)>("?DistancePointToLine@TMath@Toshi@@SGMABVTVector2@2@00@Z");
		const auto overTime = Original<float(__stdcall*)(V2, V2, V2, V2, float)>(
		    "?ClosestPointOnLineOverTime2D@TMath@Toshi@@SGMABVTVector2@2@000M@Z");
		const auto lineT =
		    Original<bool(__stdcall*)(V2, V2, V2, V2, float&)>("?LineVsLine2D@TMath@Toshi@@SG_NABVTVector2@2@000AAM@Z");
		const auto lineTU = Original<bool(__stdcall*)(V2, V2, V2, V2, float&, float&)>(
		    "?LineVsLine2D@TMath@Toshi@@SG_NABVTVector2@2@000AAM1@Z");
		const auto linePoint = Original<bool(__stdcall*)(V2, V2, V2, V2, TVector2&)>(
		    "?LineVsLine2D@TMath@Toshi@@SG_NABVTVector2@2@000AAV32@@Z");
		const auto segments = Original<bool(__stdcall*)(V2, V2, V2, V2, TVector2&)>(
		    "?LineSegmentVsLineSegment2D@TMath@Toshi@@SG_NABVTVector2@2@000AAV32@@Z");
		const auto segmentLine = Original<bool(__stdcall*)(V2, V2, V2, V2, TVector2&)>(
		    "?LineSegmentVsLine2D@TMath@Toshi@@SG_NABVTVector2@2@000AAV32@@Z");
		const auto ray = Original<bool(__stdcall*)(V2, V2, V2, V2, float&)>(
		    "?RaySegmentVsLine2D@TMath@Toshi@@SG_NABVTVector2@2@000AAM@Z");

		GeometryInputs in;
		GamePrecisionScope precision;
		for (int i = 0; i < 20000; ++i)
		{
			INFO("iteration ", i);
			const TVector2 a = in.Vector2(), b = in.Vector2(), c = in.Vector2(), d = in.Vector2();
			const float time = in.Value(5.0f);
			CHECK(pointVsPlane2D(a, b, c) == TMath::PointVsPlane2D(a, b, c));
			CHECK(Same(pointVsPlane2DF(a, b, c), TMath::PointVsPlane2DF(a, b, c)));
			const TVector3 p = in.Vector3(), q = in.Vector3(), r = in.Vector3();
			CHECK(pointVsPlane3D(p, q, r) == TMath::PointVsPlane3D(p, q, r));
			CHECK(Same(closest(a, b, c), TMath::ClosestPointOnLine2D(a, b, c)));
			CHECK(Same(distanceSq(a, b, c), TMath::DistancePointToLineSq(a, b, c)));
			CHECK(Same(distance(a, b, c), TMath::DistancePointToLine(a, b, c)));
			CHECK(Same(overTime(a, b, c, d, time), TMath::ClosestPointOnLineOverTime2D(a, b, c, d, time)));

			float t[2] = {time, time}, u[2] = {-time, -time};
			CHECK(lineT(a, b, c, d, t[0]) == TMath::LineVsLine2D(a, b, c, d, t[1]));
			CHECK(Same(t[0], t[1]));
			CHECK(lineTU(a, b, c, d, t[0], u[0]) == TMath::LineVsLine2D(a, b, c, d, t[1], u[1]));
			CHECK(Same(t[0], t[1]));
			CHECK(Same(u[0], u[1]));
			TVector2 hit[2] = {d, d};
			CHECK(linePoint(a, b, c, d, hit[0]) == TMath::LineVsLine2D(a, b, c, d, hit[1]));
			CHECK(Same(hit[0], hit[1]));
			CHECK(segments(a, b, c, d, hit[0]) == TMath::LineSegmentVsLineSegment2D(a, b, c, d, hit[1]));
			CHECK(Same(hit[0], hit[1]));
			CHECK(segmentLine(a, b, c, d, hit[0]) == TMath::LineSegmentVsLine2D(a, b, c, d, hit[1]));
			CHECK(Same(hit[0], hit[1]));
			float limit[2] = {std::abs(time) * 4.0f, std::abs(time) * 4.0f};
			CHECK(ray(a, b, c, d, limit[0]) == TMath::RaySegmentVsLine2D(a, b, c, d, limit[1]));
			CHECK(Same(limit[0], limit[1]));
		}
	}

	TEST_CASE("circles, spheres and capsules match the original")
	{
		using Circle2D = int(__stdcall*)(V2, V2, V2, float, float&, float&);
		using Circle3D = int(__stdcall*)(V3, V3, V3, float, float&, float&);
		using Spheres = bool(__stdcall*)(V3, float, V3, float, float*, TVector3*, float*);
		const auto circle2D = Original<Circle2D>("?RayVsCircle2D@TMath@Toshi@@SGHABVTVector2@2@00MAAM1@Z");
		const auto circle3D = Original<Circle3D>("?RayVsCircle3D@TMath@Toshi@@SGHABVTVector3@2@00MAAM1@Z");
		const auto spheres = Original<Spheres>("?SphereIntersection@TMath@Toshi@@SG_NABVTVector3@2@M0MPAMPAV32@1@Z");
		const auto planar =
		    Original<Spheres>("?SphereIntersectionPlanar@TMath@Toshi@@SG_NABVTVector3@2@M0MPAMPAV32@1@Z");
		const auto dynamic = Original<float(__stdcall*)(V3, float, V3, V3, float, V3)>(
		    "?SphereIntersectionDynamic@TMath@Toshi@@SGMABVTVector3@2@M00M0@Z");
		const auto capsuleNormal = Original<void(__stdcall*)(V2, V2, V2, TVector2&)>(
		    "?CalculateCapsuleNormal2D@TMath@Toshi@@SGXABVTVector2@2@00AAV32@@Z");
		const auto insideCapsule =
		    Original<bool(__stdcall*)(V2, V2, float, V2)>("?PointInsideCapsule2D@TMath@Toshi@@SG_NABVTVector2@2@0M0@Z");
		const auto rayCapsule = Original<bool(__stdcall*)(V2, V2, float&, V2, V2, float)>(
		    "?RayVsCapsule2D@TMath@Toshi@@SG_NABVTVector2@2@0AAM00M@Z");
		const auto pushOut = Original<float(__stdcall*)(V2, V2, float, V2, V2, TVector2&, float&)>(
		    "?PushPointOutOfCapsule2D@TMath@Toshi@@SGMABVTVector2@2@0M00AAV32@AAM@Z");
		const auto axisLimit = Original<void(__stdcall*)(V3, V3, V3, float, TVector3&)>(
		    "?GetAxisChangeLimit3D@TMath@Toshi@@SGXABVTVector3@2@00MAAV32@@Z");

		GeometryInputs in;
		GamePrecisionScope precision;
		int capsuleHits = 0, pushes = 0; // make sure the hit paths are exercised
		for (int i = 0; i < 20000; ++i)
		{
			INFO("iteration ", i);
			TVector2 direction2 = in.Vector2(1.0f);
			if (in.Gen().NextU32() % 4 != 0)
				direction2.Normalise();
			const TVector2 a = in.Vector2(), b = in.Vector2(), c = in.Vector2();
			const float radius = std::abs(in.Value(10.0f));
			float t0[2] = {1.5f, 1.5f}, t1[2] = {-1.5f, -1.5f};
			CHECK(circle2D(a, direction2, b, radius, t0[0], t1[0]) ==
			      TMath::RayVsCircle2D(a, direction2, b, radius, t0[1], t1[1]));
			CHECK(Same(t0[0], t0[1]));
			CHECK(Same(t1[0], t1[1]));

			TVector3 direction3 = in.Vector3(1.0f);
			if (in.Gen().NextU32() % 4 != 0)
				direction3.Normalise();
			const TVector3 p = in.Vector3(), q = in.Vector3(), v = in.Vector3(2.0f), w = in.Vector3(2.0f);
			CHECK(circle3D(p, direction3, q, radius, t0[0], t1[0]) ==
			      TMath::RayVsCircle3D(p, direction3, q, radius, t0[1], t1[1]));
			CHECK(Same(t0[0], t0[1]));
			CHECK(Same(t1[0], t1[1]));

			const float radiusB = std::abs(in.Value(10.0f));
			const unsigned int outputs = in.Gen().NextU32();
			float extra[2] = {7.0f, 7.0f}, depth[2] = {7.0f, 7.0f};
			TVector3 separation[2] = {w, w};
			const auto run = [&](Spheres original, auto ours)
			{
				const bool first =
				    original(p, radius, q, radiusB, (outputs & 1) ? &extra[0] : nullptr,
				             (outputs & 2) ? &separation[0] : nullptr, (outputs & 4) ? &depth[0] : nullptr);
				const bool second = ours(p, radius, q, radiusB, (outputs & 1) ? &extra[1] : nullptr,
				                         (outputs & 2) ? &separation[1] : nullptr, (outputs & 4) ? &depth[1] : nullptr);
				CHECK(first == second);
				CHECK(Same(extra[0], extra[1]));
				CHECK(Same(depth[0], depth[1]));
				CHECK(Same(separation[0].x, separation[1].x));
				CHECK(Same(separation[0].y, separation[1].y));
				CHECK(Same(separation[0].z, separation[1].z));
			};
			run(spheres, TMath::SphereIntersection);
			run(planar, TMath::SphereIntersectionPlanar);
			CHECK(Same(dynamic(p, radius, v, q, radiusB, w),
			           TMath::SphereIntersectionDynamic(p, radius, v, q, radiusB, w)));

			TVector2 normal[2] = {c, c};
			capsuleNormal(a, b, c, normal[0]);
			TMath::CalculateCapsuleNormal2D(a, b, c, normal[1]);
			CHECK(Same(normal[0], normal[1]));
			CHECK(insideCapsule(a, b, radius, c) == TMath::PointInsideCapsule2D(a, b, radius, c));
			float limit[2];
			limit[0] = limit[1] = std::abs(in.Value(30.0f));
			const bool capsuleHit = rayCapsule(c, direction2, limit[0], a, b, radius);
			capsuleHits += capsuleHit ? 1 : 0;
			CHECK(capsuleHit == TMath::RayVsCapsule2D(c, direction2, limit[1], a, b, radius));
			CHECK(Same(limit[0], limit[1]));
			TVector2 side = b - a;
			if (in.Gen().NextU32() % 4 != 0)
			{
				side.Normalise();
				side.SetPerpendicular();
			}
			TVector2 point[2] = {c, c};
			float pushed[2] = {5.0f, 5.0f};
			const float pushResult = pushOut(a, b, radius, side, direction2, point[0], pushed[0]);
			pushes += pushResult != 0.0f ? 1 : 0;
			CHECK(
			    Same(pushResult, TMath::PushPointOutOfCapsule2D(a, b, radius, side, direction2, point[1], pushed[1])));
			CHECK(Same(pushed[0], pushed[1]));
			CHECK(Same(point[0], point[1]));

			TVector3 from = in.Vector3(1.0f), to = in.Vector3(1.0f);
			from.Normalise();
			to = in.Gen().NextU32() % 8 == 0 ? from : to;
			to.Normalise();
			const float angle = in.Value(4.0f);
			TVector3 limited[2] = {q, q};
			axisLimit(from, to, direction3, angle, limited[0]);
			TMath::GetAxisChangeLimit3D(from, to, direction3, angle, limited[1]);
			CHECK(Same(limited[0].x, limited[1].x));
			CHECK(Same(limited[0].y, limited[1].y));
			CHECK(Same(limited[0].z, limited[1].z));
		}
		MESSAGE("capsule hits ", capsuleHits, ", pushes ", pushes);
		CHECK(capsuleHits > 1000);
		CHECK(pushes > 1000);
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
