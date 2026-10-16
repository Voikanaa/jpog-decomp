#pragma once

#include "common/abi.h"

#include <cstdint>

namespace Toshi
{

// Scalar math helpers (TKernelInterface.dll). All members are static, as in the original.
// Geometry helpers operating on TVector2/TVector3 live with the vector classes.
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

private:
	static float TSTDCALL Bias(float bias, float t);
};

} // namespace Toshi
