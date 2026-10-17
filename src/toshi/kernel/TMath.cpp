#include "toshi/kernel/TMath.h"

#include <bit>
#include <cmath>

namespace Toshi
{

namespace
{

// Precision note: during gameplay the x87 FPU runs at 24-bit precision, because the renderer
// creates its Direct3D 8 device without D3DCREATE_FPU_PRESERVE. Every add, subtract, multiply,
// divide and square root therefore rounds to float, which is exactly what plain float
// arithmetic does. The exceptions are the x87 transcendental instructions (fsin, fcos, fptan,
// fpatan), which ignore the precision setting, and the CRT functions (pow, log, exp, ...), which
// switch to 53-bit internally. Both produce an unrounded result that is rounded to float once.

// Unbiased exponent of a float's bit pattern; the int helpers below use the exponent of the
// value converted to float as a fast floor(log2(x)).
int FloatExponent(float value)
{
	return (std::bit_cast<int32_t>(value) >> 23) - 127;
}

// value * factor rounded to float once, like an x87 `fild` (exact) followed by `fmul` at 24-bit
// precision; float(value) * factor would round twice. The product of a 32-bit integer and a
// 24-bit mantissa fits exactly in 64 bits.
float MultiplyRounded(int32_t value, float factor)
{
	int exponent = 0;
	const float mantissa = std::frexp(factor, &exponent);
	const auto mantissaBits = static_cast<int64_t>(std::ldexp(mantissa, 24));
	return std::ldexp(static_cast<float>(static_cast<int64_t>(value) * mantissaBits), exponent - 24);
}

} // namespace

// The original rounds with the x87 `fist` instruction under the default round-to-nearest-even
// mode, then corrects by the sign of the remainder; for values within int range that is
// exactly floor/ceil/trunc.

int TSTDCALL TMath::FloorToInt(float value)
{
	return static_cast<int>(std::floor(value));
}

int TSTDCALL TMath::CeilToInt(float value)
{
	return static_cast<int>(std::ceil(value));
}

int TSTDCALL TMath::TruncToInt(float value)
{
	return static_cast<int>(value);
}

int TSTDCALL TMath::RoundToInt(float value)
{
	return static_cast<int>(std::lrint(value));
}

int TSTDCALL TMath::FloatToInt(float value)
{
	return static_cast<int>(std::lrint(value));
}

int TSTDCALL TMath::FloatToIntRaw(float value)
{
	return std::bit_cast<int32_t>(value);
}

float TSTDCALL TMath::IntToFloat(int value)
{
	return static_cast<float>(value);
}

float TSTDCALL TMath::IntToFloatRaw(int value)
{
	return std::bit_cast<float>(value);
}

float TSTDCALL TMath::Floor(float value)
{
	return static_cast<float>(FloorToInt(value));
}

float TSTDCALL TMath::Ceil(float value)
{
	return static_cast<float>(CeilToInt(value));
}

float TSTDCALL TMath::Trunc(float value)
{
	return static_cast<float>(TruncToInt(value));
}

float TSTDCALL TMath::Round(float value)
{
	return static_cast<float>(RoundToInt(value));
}

// Previous representable float towards zero (for positive values).
float TSTDCALL TMath::ShrinkFloat(float value)
{
	return std::bit_cast<float>(std::bit_cast<int32_t>(value) - 1);
}

int TSTDCALL TMath::Abs(int value)
{
	// Wraps for INT_MIN like the original (cdq/xor/sub).
	return value < 0 ? static_cast<int>(0u - static_cast<uint32_t>(value)) : value;
}

unsigned int TSTDCALL TMath::Abs(unsigned int value)
{
	// The original treats the argument as signed.
	return static_cast<unsigned int>(Abs(static_cast<int>(value)));
}

int TSTDCALL TMath::Sign(int value)
{
	return value > 0 ? 1 : (value < 0 ? -1 : 0);
}

int TSTDCALL TMath::FastMod(int value, int powerOfTwo)
{
	return value & (powerOfTwo - 1);
}

int TSTDCALL TMath::BitShiftMod(int value, int shift)
{
	shift &= 31;
	return value - ((value >> shift) << shift);
}

int TSTDCALL TMath::IntLog2(int value)
{
	return FloatExponent(static_cast<float>(value));
}

int TSTDCALL TMath::IntLog2Up(int value)
{
	return IntLog2(static_cast<int>(static_cast<uint32_t>(value) * 2u - 1u));
}

int TSTDCALL TMath::RoundDownPow2(int value)
{
	return 1 << (IntLog2(value) & 31);
}

int TSTDCALL TMath::RoundUpPow2(int value)
{
	return 1 << (IntLog2Up(value) & 31);
}

void TSTDCALL TMath::Clip(int& value, int min, int max)
{
	if (value > max)
		value = max;
	else if (value < min)
		value = min;
}

float TSTDCALL TMath::Abs(float value)
{
	return std::fabs(value);
}

float TSTDCALL TMath::Sign(float value)
{
	// Written so that NaN yields 1.0, matching the original's unordered x87 compare.
	if (!(value <= 0.0f))
		return 1.0f;
	if (value < 0.0f)
		return -1.0f;
	return 0.0f;
}

bool TSTDCALL TMath::IsNaN(float value)
{
	return std::isnan(value);
}

bool TSTDCALL TMath::IsFinite(float value)
{
	return std::isfinite(value);
}

void TSTDCALL TMath::Clip(float& value, float min, float max)
{
	if (value > max)
		value = max;
	else if (value < min)
		value = min;
}

float TSTDCALL TMath::LERP(float from, float to, float t)
{
	return to * t + (1.0f - t) * from;
}

float TSTDCALL TMath::DecrMinf(float value, float decrement, float min)
{
	const float result = value - decrement;
	return result >= min ? result : min;
}

float TSTDCALL TMath::IncrMaxf(float value, float increment, float max)
{
	const float result = value + increment;
	return result > max ? max : result;
}

float TSTDCALL TMath::Sqrt(float value)
{
	return std::sqrt(value);
}

float TSTDCALL TMath::RSqrt(float value)
{
	return 1.0f / std::sqrt(value);
}

float TSTDCALL TMath::Sin(float angle)
{
	return static_cast<float>(std::sin(static_cast<double>(angle)));
}

float TSTDCALL TMath::Cos(float angle)
{
	return static_cast<float>(std::cos(static_cast<double>(angle)));
}

void TSTDCALL TMath::SinCos(float angle, float& sin, float& cos)
{
	sin = Sin(angle);
	cos = Cos(angle);
}

float TSTDCALL TMath::Tan(float angle)
{
	return static_cast<float>(std::tan(static_cast<double>(angle)));
}

// The following call the double-precision CRT functions, as the original does.

float TSTDCALL TMath::ATan(float value)
{
	return static_cast<float>(std::atan(static_cast<double>(value)));
}

float TSTDCALL TMath::ATan2(float y, float x)
{
	return static_cast<float>(std::atan2(static_cast<double>(y), static_cast<double>(x)));
}

float TSTDCALL TMath::ASin(float value)
{
	return static_cast<float>(std::asin(static_cast<double>(value)));
}

float TSTDCALL TMath::ACos(float value)
{
	return static_cast<float>(std::acos(static_cast<double>(value)));
}

float TSTDCALL TMath::Exp(float value)
{
	return static_cast<float>(std::exp(static_cast<double>(value)));
}

float TSTDCALL TMath::Log(float value)
{
	return static_cast<float>(std::log(static_cast<double>(value)));
}

float TSTDCALL TMath::Pow(float base, float exponent)
{
	return static_cast<float>(std::pow(static_cast<double>(base), static_cast<double>(exponent)));
}

// Fast pow approximation that interpolates the float bit pattern linearly in log2 space:
// bits(result) = bits(base) * exponent + bits(1.0f) * (1 - exponent).
float TSTDCALL TMath::ApproxPow(float base, float exponent)
{
	constexpr float ONE_BITS = 1065353216.0f; // std::bit_cast<int32_t>(1.0f)
	const float bits = MultiplyRounded(std::bit_cast<int32_t>(base), exponent) + (1.0f - exponent) * ONE_BITS;
	return std::bit_cast<float>(static_cast<int32_t>(bits));
}

float TSTDCALL TMath::FMod(float value, float divisor)
{
	return static_cast<float>(std::fmod(static_cast<double>(value), static_cast<double>(divisor)));
}

float TSTDCALL TMath::DegToRad(float degrees)
{
	return PI * (1.0f / 180.0f) * degrees;
}

float TSTDCALL TMath::RadToDeg(float radians)
{
	return 180.0f / PI * radians;
}

// Wraps an angle into [-PI, PI].
void TSTDCALL TMath::NormaliseAngle(float& angle)
{
	// fmod of two floats is exact, so the float overload matches the original's double call.
	if (angle > PI)
		angle = std::fmod(angle + PI, TWO_PI) - PI;
	else if (angle < -PI)
		angle = std::fmod(angle - PI, TWO_PI) + PI;
}

// Shortest signed difference from `from` to `to`, for angles in [-PI, PI].
float TSTDCALL TMath::GetAngleDifference(float from, float to)
{
	const float difference = to - from;
	if (std::fabs(difference) > PI)
	{
		const float wrapped = TWO_PI - std::fabs(difference);
		return difference > 0.0f ? -wrapped : wrapped;
	}
	return difference;
}

// Difference from `from` to `to` going in the requested direction.
float TSTDCALL TMath::GetDirectedAngleDifference(float from, float to, bool positive)
{
	const float difference = to - from;
	if ((difference >= 0.0f) == positive)
		return difference;

	const float wrapped = TWO_PI - std::fabs(difference);
	return difference > 0.0f ? -wrapped : wrapped;
}

// Interpolates along the shorter arc between two angles.
float TSTDCALL TMath::LERPAngle(float from, float to, float t)
{
	const float distance = std::fabs(to - from);
	if (distance > PI)
	{
		float step = (TWO_PI - distance) * t;
		if (to >= from)
			step = -step;

		float result = from + step;
		NormaliseAngle(result);
		return result;
	}
	return LERP(from, to, t);
}

float TSTDCALL TMath::Bias(float bias, float t)
{
	// The original caches 1 / log(0.5) in a float on first use.
	static const float s_invLogHalf = static_cast<float>(1.0 / std::log(0.5));
	const float exponent = static_cast<float>(s_invLogHalf * std::log(static_cast<double>(bias)));
	return static_cast<float>(std::pow(static_cast<double>(t), static_cast<double>(exponent)));
}

float TSTDCALL TMath::Gain(float gain, float t)
{
	if (t < 0.5f)
		return 0.5f * Bias(1.0f - gain, t + t);

	return -0.5f * Bias(1.0f - gain, 2.0f - (t + t)) + 1.0f;
}

float TSTDCALL TMath::Gain075(float t)
{
	// log(1 - 0.75) / log(0.5), cached in floats like the original.
	static const float s_exponent = static_cast<float>(std::log(0.25)) * static_cast<float>(1.0 / std::log(0.5));

	if (t < 0.5f)
		return 0.5f * static_cast<float>(std::pow(static_cast<double>(t + t), static_cast<double>(s_exponent)));

	return -0.5f * static_cast<float>(std::pow(static_cast<double>(2.0f - (t + t)), static_cast<double>(s_exponent))) +
	       1.0f;
}

#if defined(_M_IX86)
// clang-format off
unsigned short TSTDCALL TMath::GetFPUControlWord()
{
	unsigned short controlWord;
	__asm { fnstcw controlWord }
	return controlWord;
}

void TSTDCALL TMath::SetFPUControlWord(unsigned short controlWord)
{
	__asm { fldcw controlWord }
}
// clang-format on
#else
namespace
{
unsigned short s_controlWord = 0x027F; // x87 default: 53-bit precision, round to nearest, all masked
}

unsigned short TSTDCALL TMath::GetFPUControlWord()
{
	return s_controlWord;
}

void TSTDCALL TMath::SetFPUControlWord(unsigned short controlWord)
{
	s_controlWord = controlWord;
}
#endif

} // namespace Toshi
