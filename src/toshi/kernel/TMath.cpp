#include "toshi/kernel/TMath.h"

#include <bit>
#include <cmath>

namespace Toshi
{

namespace
{

// Precision note: the original evaluates float expressions on the x87 FPU, which the CRT runs
// at 53-bit precision, and rounds to float only when a value is stored or returned. Computing
// intermediates in double and rounding once reproduces those results bit for bit.

constexpr double PI_D = TMath::PI;
constexpr double TWO_PI_D = TMath::TWO_PI;

// Unbiased exponent of a float's bit pattern; the int helpers below use the exponent of the
// value converted to float as a fast floor(log2(x)).
int FloatExponent(float value)
{
	return (std::bit_cast<int32_t>(value) >> 23) - 127;
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
	return static_cast<float>(double(to) * t + (1.0 - t) * from);
}

float TSTDCALL TMath::DecrMinf(float value, float decrement, float min)
{
	const double result = double(value) - decrement;
	return result >= min ? static_cast<float>(result) : min;
}

float TSTDCALL TMath::IncrMaxf(float value, float increment, float max)
{
	const double result = double(value) + increment;
	return result > max ? max : static_cast<float>(result);
}

float TSTDCALL TMath::Sqrt(float value)
{
	return std::sqrt(value);
}

float TSTDCALL TMath::RSqrt(float value)
{
	return static_cast<float>(1.0 / std::sqrt(static_cast<double>(value)));
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
	constexpr double ONE_BITS = 1065353216.0; // std::bit_cast<int32_t>(1.0f)
	const double bits = std::bit_cast<int32_t>(base) * static_cast<double>(exponent) +
		(1.0 - static_cast<double>(exponent)) * ONE_BITS;
	return std::bit_cast<float>(static_cast<int32_t>(bits));
}

float TSTDCALL TMath::FMod(float value, float divisor)
{
	return static_cast<float>(std::fmod(static_cast<double>(value), static_cast<double>(divisor)));
}

float TSTDCALL TMath::DegToRad(float degrees)
{
	return static_cast<float>(PI_D * (1.0f / 180.0f) * degrees);
}

float TSTDCALL TMath::RadToDeg(float radians)
{
	return static_cast<float>(180.0 / PI_D * radians);
}

// Wraps an angle into [-PI, PI].
void TSTDCALL TMath::NormaliseAngle(float& angle)
{
	if (angle > PI)
		angle = static_cast<float>(std::fmod(angle + PI_D, TWO_PI_D) - PI_D);
	else if (angle < -PI)
		angle = static_cast<float>(std::fmod(angle - PI_D, TWO_PI_D) + PI_D);
}

// Shortest signed difference from `from` to `to`, for angles in [-PI, PI].
float TSTDCALL TMath::GetAngleDifference(float from, float to)
{
	const double difference = double(to) - from;
	if (std::fabs(difference) > PI_D)
	{
		const double wrapped = TWO_PI_D - std::fabs(difference);
		return static_cast<float>(difference > 0.0 ? -wrapped : wrapped);
	}
	return static_cast<float>(difference);
}

// Difference from `from` to `to` going in the requested direction.
float TSTDCALL TMath::GetDirectedAngleDifference(float from, float to, bool positive)
{
	const double difference = double(to) - from;
	if ((difference >= 0.0) == positive)
		return static_cast<float>(difference);

	const double wrapped = TWO_PI_D - std::fabs(difference);
	return static_cast<float>(difference > 0.0 ? -wrapped : wrapped);
}

// Interpolates along the shorter arc between two angles.
float TSTDCALL TMath::LERPAngle(float from, float to, float t)
{
	const double distance = std::fabs(double(to) - from);
	if (distance > PI_D)
	{
		double step = (TWO_PI_D - distance) * t;
		if (to >= from)
			step = -step;

		float result = static_cast<float>(from + step);
		NormaliseAngle(result);
		return result;
	}
	return LERP(from, to, t);
}

namespace
{

// Bias before rounding to float; Gain scales this unrounded value, as the original does with
// the x87 result.
double BiasUnrounded(float bias, float t)
{
	// The original caches 1 / log(0.5) in a float on first use.
	static const float s_invLogHalf = static_cast<float>(1.0 / std::log(0.5));
	return std::pow(static_cast<double>(t), static_cast<double>(s_invLogHalf) * std::log(static_cast<double>(bias)));
}

} // namespace

float TSTDCALL TMath::Bias(float bias, float t)
{
	return static_cast<float>(BiasUnrounded(bias, t));
}

float TSTDCALL TMath::Gain(float gain, float t)
{
	if (t < 0.5f)
		return static_cast<float>(0.5 * BiasUnrounded(1.0f - gain, t + t));

	return static_cast<float>(-0.5 * BiasUnrounded(1.0f - gain, 2.0f - (t + t)) + 1.0);
}

float TSTDCALL TMath::Gain075(float t)
{
	// log(1 - 0.75) / log(0.5), cached in floats like the original.
	static const float s_exponent =
		static_cast<float>(std::log(0.25)) * static_cast<float>(1.0 / std::log(0.5));

	if (t < 0.5f)
		return static_cast<float>(0.5 * std::pow(static_cast<double>(t + t), static_cast<double>(s_exponent)));

	return static_cast<float>(
		-0.5 * std::pow(static_cast<double>(2.0f - (t + t)), static_cast<double>(s_exponent)) + 1.0);
}

} // namespace Toshi
