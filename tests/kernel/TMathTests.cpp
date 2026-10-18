#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TMath.h"

#include <bit>
#include <cmath>
#include <doctest/doctest.h>
#include <limits>
#include <vector>

using Toshi::TMath;
using namespace jpog::test;

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TMath")
{
	TEST_CASE("rounding to int")
	{
		CHECK(TMath::FloorToInt(-1.5f) == -2);
		CHECK(TMath::FloorToInt(1.5f) == 1);
		CHECK(TMath::CeilToInt(-1.5f) == -1);
		CHECK(TMath::CeilToInt(1.25f) == 2);
		CHECK(TMath::TruncToInt(-1.75f) == -1);
		CHECK(TMath::TruncToInt(1.75f) == 1);
		CHECK(TMath::RoundToInt(2.5f) == 2); // round half to even
		CHECK(TMath::RoundToInt(3.5f) == 4);
		CHECK(TMath::Round(-2.5f) == -2.0f);
	}

	TEST_CASE("raw bit conversions")
	{
		CHECK(TMath::FloatToIntRaw(1.0f) == 0x3F800000);
		CHECK(TMath::IntToFloatRaw(0x3F800000) == 1.0f);
		CHECK(TMath::ShrinkFloat(1.0f) == std::nextafter(1.0f, 0.0f));
	}

	TEST_CASE("integer helpers")
	{
		CHECK(TMath::Abs(-5) == 5);
		CHECK(TMath::Abs(std::numeric_limits<int>::min()) == std::numeric_limits<int>::min());
		CHECK(TMath::Sign(-7) == -1);
		CHECK(TMath::Sign(0) == 0);
		CHECK(TMath::FastMod(13, 8) == 5);
		CHECK(TMath::BitShiftMod(13, 3) == 5);
		CHECK(TMath::IntLog2(1) == 0);
		CHECK(TMath::IntLog2(1024) == 10);
		CHECK(TMath::IntLog2(1025) == 10);
		CHECK(TMath::IntLog2Up(1025) == 11);
		CHECK(TMath::RoundDownPow2(100) == 64);
		CHECK(TMath::RoundUpPow2(100) == 128);
		CHECK(TMath::RoundUpPow2(64) == 64);

		int value = 15;
		TMath::Clip(value, 0, 10);
		CHECK(value == 10);
	}

	TEST_CASE("float helpers")
	{
		CHECK(TMath::Sign(-0.5f) == -1.0f);
		CHECK(TMath::Sign(0.0f) == 0.0f);
		CHECK(TMath::Sign(std::numeric_limits<float>::quiet_NaN()) == 1.0f);
		CHECK(TMath::LERP(2.0f, 4.0f, 0.5f) == 3.0f);
		CHECK(TMath::DecrMinf(5.0f, 3.0f, 4.0f) == 4.0f);
		CHECK(TMath::IncrMaxf(5.0f, 3.0f, 7.0f) == 7.0f);
		CHECK(TMath::ApproxPow(4.0f, 0.5f) == 2.0f);
	}

	TEST_CASE("angles")
	{
		float angle = 3.0f * TMath::PI;
		TMath::NormaliseAngle(angle);
		CHECK(std::fabs(std::fabs(angle) - TMath::PI) < 1e-5f);

		CHECK(TMath::GetAngleDifference(0.1f, -0.1f) == doctest::Approx(-0.2f));
		// Crossing the +-PI seam takes the short way round.
		CHECK(TMath::GetAngleDifference(3.0f, -3.0f) == doctest::Approx(TMath::TWO_PI - 6.0f));
		CHECK(TMath::GetDirectedAngleDifference(0.0f, -1.0f, true) == doctest::Approx(TMath::TWO_PI - 1.0f));
		CHECK(TMath::RadToDeg(TMath::PI) == doctest::Approx(180.0f));
	}

	TEST_CASE("gain")
	{
		CHECK(TMath::Gain075(0.5f) == doctest::Approx(0.5f));
		CHECK(TMath::Gain(0.75f, 0.25f) == doctest::Approx(TMath::Gain075(0.25f)));
		CHECK(TMath::Gain(0.5f, 0.3f) == doctest::Approx(0.3f)); // gain 0.5 is the identity
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

std::vector<float> FloatInputs(float min, float max, int randomCount = 2000)
{
	std::vector<float> values = {0.0f, -0.0f, 0.5f, -0.5f, 1.0f, -1.0f, 1.5f, -1.5f, 2.5f, -2.5f};
	InputGenerator gen;
	for (int i = 0; i < randomCount; ++i)
		values.push_back(gen.NextFloat(min, max));
	std::erase_if(values, [&](float v) { return v < min || v > max; });
	return values;
}

std::vector<int> IntInputs(int min, int max, int randomCount = 2000)
{
	std::vector<int> values;
	for (int v : {0, 1, 2, 3, 63, 64, 65, 1023, 1024, 1025, -1, -2, -64})
		if (v >= min && v <= max)
			values.push_back(v);
	InputGenerator gen;
	for (int i = 0; i < randomCount; ++i)
		values.push_back(gen.NextInt(min, max));
	return values;
}

using FloatToFloat = float(__stdcall*)(float);
using FloatToInt = int(__stdcall*)(float);
using IntToInt = int(__stdcall*)(int);
using IntToFloatFn = float(__stdcall*)(int);
using Float2ToFloat = float(__stdcall*)(float, float);
using Float3ToFloat = float(__stdcall*)(float, float, float);

void CompareUnary(const char* name, FloatToFloat original, float(TSTDCALL* ours)(float),
                  const std::vector<float>& inputs, int64_t maxUlps)
{
	for (float x : inputs)
	{
		const float expected = original(x);
		const float actual = ours(x);
		INFO(name, "(", x, "): original=", expected, " ours=", actual);
		CHECK(NearlyEqual(expected, actual, maxUlps));
	}
}

} // namespace

TEST_SUITE("TMath differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("FPU control word round-trips with the original")
	{
		using GetFn = unsigned short(__stdcall*)();
		using SetFn = void(__stdcall*)(unsigned short);
		const auto get = Original<GetFn>("?GetFPUControlWord@TMath@Toshi@@SGGXZ");
		const auto set = Original<SetFn>("?SetFPUControlWord@TMath@Toshi@@SGXG@Z");
		const unsigned short saved = TMath::GetFPUControlWord();
		const unsigned short words[] = {0x007F, 0x027F, 0x0C7F, 0x047F};
		for (const unsigned short word : words)
		{
			TMath::SetFPUControlWord(word);
			CHECK(get() == word);
			set(saved);
			set(word);
			CHECK(TMath::GetFPUControlWord() == word);
		}
		TMath::SetFPUControlWord(saved);
		CHECK(get() == saved);
	}

	TEST_CASE("float to int conversions match exactly")
	{
		GamePrecisionScope precision;
		const auto inputs = FloatInputs(-1.0e6f, 1.0e6f);
		const struct
		{
			const char* name;
			int(TSTDCALL* ours)(float);
		} cases[] = {
		    {"?FloorToInt@TMath@Toshi@@SGHM@Z", &TMath::FloorToInt},
		    {"?CeilToInt@TMath@Toshi@@SGHM@Z", &TMath::CeilToInt},
		    {"?TruncToInt@TMath@Toshi@@SGHM@Z", &TMath::TruncToInt},
		    {"?RoundToInt@TMath@Toshi@@SGHM@Z", &TMath::RoundToInt},
		    {"?FloatToInt@TMath@Toshi@@SGHM@Z", &TMath::FloatToInt},
		    {"?FloatToIntRaw@TMath@Toshi@@SGHM@Z", &TMath::FloatToIntRaw},
		};
		for (const auto& c : cases)
		{
			const auto original = Original<FloatToInt>(c.name);
			for (float x : inputs)
			{
				INFO(c.name, "(", x, ")");
				CHECK(original(x) == c.ours(x));
			}
		}
	}

	TEST_CASE("float rounding functions match exactly")
	{
		GamePrecisionScope precision;
		const auto inputs = FloatInputs(-1.0e6f, 1.0e6f);
		CompareUnary("Floor", Original<FloatToFloat>("?Floor@TMath@Toshi@@SGMM@Z"), &TMath::Floor, inputs, 0);
		CompareUnary("Ceil", Original<FloatToFloat>("?Ceil@TMath@Toshi@@SGMM@Z"), &TMath::Ceil, inputs, 0);
		CompareUnary("Trunc", Original<FloatToFloat>("?Trunc@TMath@Toshi@@SGMM@Z"), &TMath::Trunc, inputs, 0);
		CompareUnary("Round", Original<FloatToFloat>("?Round@TMath@Toshi@@SGMM@Z"), &TMath::Round, inputs, 0);
		CompareUnary("ShrinkFloat", Original<FloatToFloat>("?ShrinkFloat@TMath@Toshi@@SGMM@Z"), &TMath::ShrinkFloat,
		             FloatInputs(0.001f, 1.0e6f), 0);
		CompareUnary("Abs", Original<FloatToFloat>("?Abs@TMath@Toshi@@SGMM@Z"),
		             static_cast<float(TSTDCALL*)(float)>(&TMath::Abs), inputs, 0);
		CompareUnary("Sign", Original<FloatToFloat>("?Sign@TMath@Toshi@@SGMM@Z"),
		             static_cast<float(TSTDCALL*)(float)>(&TMath::Sign), inputs, 0);
	}

	TEST_CASE("integer helpers match exactly")
	{
		GamePrecisionScope precision;
		const struct
		{
			const char* name;
			int(TSTDCALL* ours)(int);
			int min;
			int max;
		} cases[] = {
		    {"?Abs@TMath@Toshi@@SGHH@Z", static_cast<int(TSTDCALL*)(int)>(&TMath::Abs), -1000000, 1000000},
		    {"?Sign@TMath@Toshi@@SGHH@Z", static_cast<int(TSTDCALL*)(int)>(&TMath::Sign), -1000000, 1000000},
		    {"?IntLog2@TMath@Toshi@@SGHH@Z", &TMath::IntLog2, 1, 1 << 30},
		    {"?IntLog2Up@TMath@Toshi@@SGHH@Z", &TMath::IntLog2Up, 1, 1 << 29},
		    {"?RoundDownPow2@TMath@Toshi@@SGHH@Z", &TMath::RoundDownPow2, 1, 1 << 30},
		    {"?RoundUpPow2@TMath@Toshi@@SGHH@Z", &TMath::RoundUpPow2, 1, 1 << 29},
		};
		for (const auto& c : cases)
		{
			const auto original = Original<IntToInt>(c.name);
			for (int x : IntInputs(c.min, c.max))
			{
				INFO(c.name, "(", x, ")");
				CHECK(original(x) == c.ours(x));
			}
		}

		const auto fastMod = Original<int(__stdcall*)(int, int)>("?FastMod@TMath@Toshi@@SGHHH@Z");
		const auto bitShiftMod = Original<int(__stdcall*)(int, int)>("?BitShiftMod@TMath@Toshi@@SGHHH@Z");
		InputGenerator gen;
		for (int i = 0; i < 2000; ++i)
		{
			const int value = gen.NextInt(-1000000, 1000000);
			const int shift = gen.NextInt(0, 31);
			CHECK(fastMod(value, 1 << shift) == TMath::FastMod(value, 1 << shift));
			CHECK(bitShiftMod(value, shift) == TMath::BitShiftMod(value, shift));
		}

		const auto intToFloat = Original<IntToFloatFn>("?IntToFloat@TMath@Toshi@@SGMH@Z");
		const auto intToFloatRaw = Original<IntToFloatFn>("?IntToFloatRaw@TMath@Toshi@@SGMH@Z");
		for (int x : IntInputs(-(1 << 30), 1 << 30))
		{
			CHECK(intToFloat(x) == TMath::IntToFloat(x));
			CHECK(std::bit_cast<int32_t>(intToFloatRaw(x)) == std::bit_cast<int32_t>(TMath::IntToFloatRaw(x)));
		}
	}

	TEST_CASE("clip, lerp and increments")
	{
		GamePrecisionScope precision;
		const auto clipInt = Original<void(__stdcall*)(int&, int, int)>("?Clip@TMath@Toshi@@SGXAAHHH@Z");
		const auto clipFloat = Original<void(__stdcall*)(float&, float, float)>("?Clip@TMath@Toshi@@SGXAAMMM@Z");
		const auto lerp = Original<Float3ToFloat>("?LERP@TMath@Toshi@@SGMMMM@Z");
		const auto decrMinf = Original<Float3ToFloat>("?DecrMinf@TMath@Toshi@@SGMMMM@Z");
		const auto incrMaxf = Original<Float3ToFloat>("?IncrMaxf@TMath@Toshi@@SGMMMM@Z");

		InputGenerator gen;
		for (int i = 0; i < 2000; ++i)
		{
			const float a = gen.NextFloat(-100.0f, 100.0f);
			const float b = gen.NextFloat(-100.0f, 100.0f);
			const float c = gen.NextFloat(-100.0f, 100.0f);
			const float t = gen.NextFloat(0.0f, 1.0f);

			int intOriginal = static_cast<int>(a), intOurs = static_cast<int>(a);
			clipInt(intOriginal, -20, 20);
			TMath::Clip(intOurs, -20, 20);
			CHECK(intOriginal == intOurs);

			// Every 50th input is NaN (Clip turns it into the minimum).
			const float nan = std::numeric_limits<float>::quiet_NaN();
			const float clipped = i % 50 == 0 ? nan : a;
			float floatOriginal = clipped, floatOurs = clipped;
			clipFloat(floatOriginal, -20.0f, 20.0f);
			TMath::Clip(floatOurs, -20.0f, 20.0f);
			CHECK(Same(floatOriginal, floatOurs));

			INFO("a=", a, " b=", b, " c=", c, " t=", t);
			CHECK(NearlyEqual(lerp(a, b, t), TMath::LERP(a, b, t), 0));
			CHECK(NearlyEqual(decrMinf(a, b, c), TMath::DecrMinf(a, b, c), 0));
			CHECK(NearlyEqual(incrMaxf(a, b, c), TMath::IncrMaxf(a, b, c), 0));
			CHECK(Same(decrMinf(clipped, b, c), TMath::DecrMinf(clipped, b, c)));
			CHECK(Same(incrMaxf(clipped, b, c), TMath::IncrMaxf(clipped, b, c)));
		}
	}

	TEST_CASE("transcendental functions")
	{
		GamePrecisionScope precision;
		const auto angles = FloatInputs(-100.0f, 100.0f);
		const auto unit = FloatInputs(-1.0f, 1.0f);
		const auto positive = FloatInputs(0.001f, 1000.0f);

		CompareUnary("Sqrt", Original<FloatToFloat>("?Sqrt@TMath@Toshi@@SGMM@Z"), &TMath::Sqrt, positive, 0);
		CompareUnary("RSqrt", Original<FloatToFloat>("?RSqrt@TMath@Toshi@@SGMM@Z"), &TMath::RSqrt, positive, 0);
		CompareUnary("Sin", Original<FloatToFloat>("?Sin@TMath@Toshi@@SGMM@Z"), &TMath::Sin, angles, 0);
		CompareUnary("Cos", Original<FloatToFloat>("?Cos@TMath@Toshi@@SGMM@Z"), &TMath::Cos, angles, 0);
		CompareUnary("Tan", Original<FloatToFloat>("?Tan@TMath@Toshi@@SGMM@Z"), &TMath::Tan, FloatInputs(-1.5f, 1.5f),
		             0);
		CompareUnary("ATan", Original<FloatToFloat>("?ATan@TMath@Toshi@@SGMM@Z"), &TMath::ATan, angles, 0);
		CompareUnary("ASin", Original<FloatToFloat>("?ASin@TMath@Toshi@@SGMM@Z"), &TMath::ASin, unit, 0);
		CompareUnary("ACos", Original<FloatToFloat>("?ACos@TMath@Toshi@@SGMM@Z"), &TMath::ACos, unit, 0);
		CompareUnary("Exp", Original<FloatToFloat>("?Exp@TMath@Toshi@@SGMM@Z"), &TMath::Exp, FloatInputs(-50.0f, 50.0f),
		             0);
		CompareUnary("Log", Original<FloatToFloat>("?Log@TMath@Toshi@@SGMM@Z"), &TMath::Log, positive, 0);
		CompareUnary("DegToRad", Original<FloatToFloat>("?DegToRad@TMath@Toshi@@SGMM@Z"), &TMath::DegToRad, angles, 0);
		CompareUnary("RadToDeg", Original<FloatToFloat>("?RadToDeg@TMath@Toshi@@SGMM@Z"), &TMath::RadToDeg, angles, 0);
		CompareUnary("Gain075", Original<FloatToFloat>("?Gain075@TMath@Toshi@@SGMM@Z"), &TMath::Gain075,
		             FloatInputs(0.0f, 1.0f), 0);

		const auto sinCos = Original<void(__stdcall*)(float, float&, float&)>("?SinCos@TMath@Toshi@@SGXMAAM0@Z");
		for (float x : angles)
		{
			float sinOriginal, cosOriginal, sinOurs, cosOurs;
			sinCos(x, sinOriginal, cosOriginal);
			TMath::SinCos(x, sinOurs, cosOurs);
			CHECK(NearlyEqual(sinOriginal, sinOurs, 0));
			CHECK(NearlyEqual(cosOriginal, cosOurs, 0));
		}

		const auto atan2 = Original<Float2ToFloat>("?ATan2@TMath@Toshi@@SGMMM@Z");
		const auto pow = Original<Float2ToFloat>("?Pow@TMath@Toshi@@SGMMM@Z");
		const auto approxPow = Original<Float2ToFloat>("?ApproxPow@TMath@Toshi@@SGMMM@Z");
		const auto fmod = Original<Float2ToFloat>("?FMod@TMath@Toshi@@SGMMM@Z");
		const auto gain = Original<Float2ToFloat>("?Gain@TMath@Toshi@@SGMMM@Z");
		InputGenerator gen;
		for (int i = 0; i < 2000; ++i)
		{
			const float a = gen.NextFloat(-100.0f, 100.0f);
			const float b = gen.NextFloat(-100.0f, 100.0f);
			const float base = gen.NextFloat(0.01f, 10.0f);
			const float exponent = gen.NextFloat(-4.0f, 4.0f);
			const float g = gen.NextFloat(0.05f, 0.95f);
			const float t = gen.NextFloat(0.0f, 1.0f);
			INFO("a=", a, " b=", b, " base=", base, " exponent=", exponent, " g=", g, " t=", t);
			CHECK(NearlyEqual(atan2(a, b), TMath::ATan2(a, b), 0));
			CHECK(NearlyEqual(pow(base, exponent), TMath::Pow(base, exponent), 0));
			CHECK(NearlyEqual(approxPow(base, exponent), TMath::ApproxPow(base, exponent), 0));
			CHECK(NearlyEqual(fmod(a, b), TMath::FMod(a, b), 0));
			CHECK(NearlyEqual(gain(g, t), TMath::Gain(g, t), 0));
		}
	}

	TEST_CASE("angle helpers")
	{
		GamePrecisionScope precision;
		const auto normalise = Original<void(__stdcall*)(float&)>("?NormaliseAngle@TMath@Toshi@@SGXAAM@Z");
		const auto difference = Original<Float2ToFloat>("?GetAngleDifference@TMath@Toshi@@SGMMM@Z");
		const auto directed =
		    Original<float(__stdcall*)(float, float, bool)>("?GetDirectedAngleDifference@TMath@Toshi@@SGMMM_N@Z");
		const auto lerpAngle = Original<Float3ToFloat>("?LERPAngle@TMath@Toshi@@SGMMMM@Z");

		InputGenerator gen;
		for (int i = 0; i < 4000; ++i)
		{
			float angleOriginal = gen.NextFloat(-50.0f, 50.0f);
			float angleOurs = angleOriginal;
			INFO("angle=", angleOriginal);
			normalise(angleOriginal);
			TMath::NormaliseAngle(angleOurs);
			CHECK(NearlyEqual(angleOriginal, angleOurs, 0));

			const float a = gen.NextFloat(-TMath::PI, TMath::PI);
			const float b = gen.NextFloat(-TMath::PI, TMath::PI);
			const float t = gen.NextFloat(0.0f, 1.0f);
			const bool positive = (gen.NextU32() & 1) != 0;
			INFO("a=", a, " b=", b, " t=", t, " positive=", positive);
			CHECK(NearlyEqual(difference(a, b), TMath::GetAngleDifference(a, b), 0));
			CHECK(NearlyEqual(directed(a, b, positive), TMath::GetDirectedAngleDifference(a, b, positive), 0));
			CHECK(NearlyEqual(lerpAngle(a, b, t), TMath::LERPAngle(a, b, t), 0));
		}
	}

	TEST_CASE("classification")
	{
		GamePrecisionScope precision;
		const auto isNaN = Original<bool(__stdcall*)(float)>("?IsNaN@TMath@Toshi@@SG_NM@Z");
		const auto isFinite = Original<bool(__stdcall*)(float)>("?IsFinite@TMath@Toshi@@SG_NM@Z");
		const float values[] = {0.0f,
		                        1.0f,
		                        -1.0f,
		                        std::numeric_limits<float>::infinity(),
		                        -std::numeric_limits<float>::infinity(),
		                        std::numeric_limits<float>::quiet_NaN(),
		                        TMath::MAXFLOAT,
		                        TMath::MINFLOAT};
		for (float v : values)
		{
			CHECK(isNaN(v) == TMath::IsNaN(v));
			CHECK(isFinite(v) == TMath::IsFinite(v));
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
