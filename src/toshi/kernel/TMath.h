#pragma once

#include "common/abi.h"

#include <cstdint>

namespace Toshi
{

class TVector2;
class TVector3;

// Math helpers (TKernelInterface.dll). All members are static, as in the original. The 2D/3D
// geometry tests are implemented in TMathGeometry.cpp.
class TMath
{
public:
	static constexpr float PI = 3.14159274f;
	static constexpr float HALF_PI = 1.57079637f;
	static constexpr float TWO_PI = 6.28318548f;
	static constexpr float ONEOVER_PI = 0.318309873f;
	static constexpr float ONEOVERTWO_PI = 0.159154937f;
	static constexpr float E = 2.71828175f;
	static constexpr float SQRT_TWO = 1.41421354f;
	static constexpr float ONEOVER_SQRT_TWO = 0.707106769f;
	static constexpr float MINFLOAT = 1.17549435e-38f;
	static constexpr float MAXFLOAT = 3.40282347e+38f;
	static constexpr float FLOATEPSILON = 1.1920929e-07f;
	static constexpr int MININT = INT32_MIN;
	static constexpr int MAXINT = INT32_MAX;
	static constexpr unsigned int MAXUINT = UINT32_MAX;
	static constexpr char MAXCCHAR = 127;
	static constexpr unsigned short MAXWCHAR = 0xFFFF;

	// Rounding and conversion. The *ToInt functions expect values within int range.
	static int TSTDCALL FloorToInt(float value);
	static int TSTDCALL CeilToInt(float value);
	static int TSTDCALL TruncToInt(float value);
	static int TSTDCALL RoundToInt(float value);
	static int TSTDCALL FloatToInt(float value);
	static int TSTDCALL FloatToIntRaw(float value);
	static float TSTDCALL IntToFloat(int value);
	static float TSTDCALL IntToFloatRaw(int value);
	static float TSTDCALL Floor(float value);
	static float TSTDCALL Ceil(float value);
	static float TSTDCALL Trunc(float value);
	static float TSTDCALL Round(float value);
	static float TSTDCALL ShrinkFloat(float value);

	// Integer helpers.
	static int TSTDCALL Abs(int value);
	static unsigned int TSTDCALL Abs(unsigned int value);
	static int TSTDCALL Sign(int value);
	static int TSTDCALL FastMod(int value, int powerOfTwo);
	static int TSTDCALL BitShiftMod(int value, int shift);
	static int TSTDCALL IntLog2(int value);
	static int TSTDCALL IntLog2Up(int value);
	static int TSTDCALL RoundDownPow2(int value);
	static int TSTDCALL RoundUpPow2(int value);
	static void TSTDCALL Clip(int& value, int min, int max);

	// Float helpers.
	static float TSTDCALL Abs(float value);
	static float TSTDCALL Sign(float value);
	static bool TSTDCALL IsNaN(float value);
	static bool TSTDCALL IsFinite(float value);
	static void TSTDCALL Clip(float& value, float min, float max);
	static float TSTDCALL LERP(float from, float to, float t);
	static float TSTDCALL DecrMinf(float value, float decrement, float min);
	static float TSTDCALL IncrMaxf(float value, float increment, float max);

	// Transcendental functions.
	static float TSTDCALL Sqrt(float value);
	static float TSTDCALL RSqrt(float value);
	static float TSTDCALL Sin(float angle);
	static float TSTDCALL Cos(float angle);
	static void TSTDCALL SinCos(float angle, float& sin, float& cos);
	static float TSTDCALL Tan(float angle);
	static float TSTDCALL ATan(float value);
	static float TSTDCALL ATan2(float y, float x);
	static float TSTDCALL ASin(float value);
	static float TSTDCALL ACos(float value);
	static float TSTDCALL Exp(float value);
	static float TSTDCALL Log(float value);
	static float TSTDCALL Pow(float base, float exponent);
	static float TSTDCALL ApproxPow(float base, float exponent);
	static float TSTDCALL FMod(float value, float divisor);

	// Angles (radians).
	static float TSTDCALL DegToRad(float degrees);
	static float TSTDCALL RadToDeg(float radians);
	static void TSTDCALL NormaliseAngle(float& angle);
	static float TSTDCALL GetAngleDifference(float from, float to);
	static float TSTDCALL GetDirectedAngleDifference(float from, float to, bool positive);
	static float TSTDCALL LERPAngle(float from, float to, float t);

	// Ken Perlin's gain function; Gain075 is Gain(0.75, t) with a cached exponent.
	static float TSTDCALL Gain(float gain, float t);
	static float TSTDCALL Gain075(float t);

	// x87 control word (precision, rounding, exception masks). The game itself never calls these.
	// On x64, where float math uses SSE, the word is only stored and returned.
	static unsigned short TSTDCALL GetFPUControlWord();
	static void TSTDCALL SetFPUControlWord(unsigned short controlWord);

	// Geometry. Lines are given by two points unless noted; parameters are along the first line.
	// True when `point` lies on the side of the plane through `planePoint` that `normal` points to
	// (or on it).
	static bool TSTDCALL PointVsPlane2D(const TVector2& normal, const TVector2& planePoint, const TVector2& point);
	// The signed distance along `normal` (scaled by its length).
	static float TSTDCALL PointVsPlane2DF(const TVector2& normal, const TVector2& planePoint, const TVector2& point);
	static bool TSTDCALL PointVsPlane3D(const TVector3& normal, const TVector3& planePoint, const TVector3& point);
	// Parameter of the projection of `point` on the line from `start` to `end`.
	static float TSTDCALL ClosestPointOnLine2D(const TVector2& start, const TVector2& end, const TVector2& point);
	// Squared distance from `point` to the line through `start` and `end`.
	static float TSTDCALL DistancePointToLineSq(const TVector2& point, const TVector2& start, const TVector2& end);
	static float TSTDCALL DistancePointToLine(const TVector2& point, const TVector2& start, const TVector2& end);
	static float TSTDCALL ClosestPointOnLineOverTime2D(const TVector2& a, const TVector2& b, const TVector2& c,
	                                                   const TVector2& d, float time);
	// Intersection of lines a0-a1 and b0-b1: false when parallel.
	static bool TSTDCALL LineVsLine2D(const TVector2& a0, const TVector2& a1, const TVector2& b0, const TVector2& b1,
	                                  float& t);
	static bool TSTDCALL LineVsLine2D(const TVector2& a0, const TVector2& a1, const TVector2& b0, const TVector2& b1,
	                                  float& t, float& u);
	static bool TSTDCALL LineVsLine2D(const TVector2& a0, const TVector2& a1, const TVector2& b0, const TVector2& b1,
	                                  TVector2& intersection);
	static bool TSTDCALL LineSegmentVsLineSegment2D(const TVector2& a0, const TVector2& a1, const TVector2& b0,
	                                                const TVector2& b1, TVector2& intersection);
	static bool TSTDCALL LineSegmentVsLine2D(const TVector2& a0, const TVector2& a1, const TVector2& b0,
	                                         const TVector2& b1, TVector2& intersection);
	// Ray from `origin` along `direction` against the line through `point` along `lineDirection`;
	// updates `t` only for a hit in [0, t).
	static bool TSTDCALL RaySegmentVsLine2D(const TVector2& origin, const TVector2& direction, const TVector2& point,
	                                        const TVector2& lineDirection, float& t);
	// Ray (origin, direction) against a circle/sphere: 2 with both hit parameters, 0 for a miss
	// (outputs untouched). The parameters assume a unit direction.
	static int TSTDCALL RayVsCircle2D(const TVector2& origin, const TVector2& direction, const TVector2& centre,
	                                  float radius, float& t0, float& t1);
	static int TSTDCALL RayVsCircle3D(const TVector3& origin, const TVector3& direction, const TVector3& centre,
	                                  float radius, float& t0, float& t1);
	// True when the spheres overlap. Optional outputs: `distanceSqMinusRadiusSq` (squared centre
	// distance minus radiusB squared), `separation` (offset A to B scaled by penetration over the
	// radius sum) and `penetration`. Planar ignores Y.
	static bool TSTDCALL SphereIntersection(const TVector3& centreA, float radiusA, const TVector3& centreB,
	                                        float radiusB, float* distanceSqMinusRadiusSq, TVector3* separation,
	                                        float* penetration);
	static bool TSTDCALL SphereIntersectionPlanar(const TVector3& centreA, float radiusA, const TVector3& centreB,
	                                              float radiusB, float* distanceSqMinusRadiusSq, TVector3* separation,
	                                              float* penetration);
	// Time until two moving spheres touch: 0 when already touching, -1 when they never do.
	static float TSTDCALL SphereIntersectionDynamic(const TVector3& centreA, float radiusA, const TVector3& velocityA,
	                                                const TVector3& centreB, float radiusB, const TVector3& velocityB);
	// Capsules are a segment from `start` to `end` plus a radius.
	static void TSTDCALL CalculateCapsuleNormal2D(const TVector2& start, const TVector2& end, const TVector2& point,
	                                              TVector2& normal);
	static bool TSTDCALL PointInsideCapsule2D(const TVector2& start, const TVector2& end, float radius,
	                                          const TVector2& point);
	// `to`, or `from` rotated towards it by `angle` radians (clamped to [-pi, pi]) when the two
	// unit vectors differ by more; `fallbackAxis` gives the rotation plane when they are parallel.
	static void TSTDCALL GetAxisChangeLimit3D(const TVector3& from, const TVector3& to, const TVector3& fallbackAxis,
	                                          float angle, TVector3& result);

private:
	static float TSTDCALL Bias(float bias, float t);
	// 2: beyond `end`, 1: before `start`, 0: in between (the radius is unused).
	static int TSTDCALL GetCapsuleZone(const TVector2& point, const TVector2& start, const TVector2& end, float radius);
};

} // namespace Toshi
