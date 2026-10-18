#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TMath.h"
#include "toshi/kernel/TVector2.h"
#include "toshi/kernel/TVector3.h"

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
}

#endif // JPOG_HAS_ORIGINAL_MODULES
