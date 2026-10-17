#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TRandom.h"

#include <cstring>
#include <doctest/doctest.h>

using Toshi::TRandom;
using namespace jpog::test;

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TRandom")
{
	TEST_CASE("seeded sequences repeat and stay in range")
	{
		TRandom a, b;
		a.SetSeed(1234);
		b.SetSeed(1234);
		CHECK(a.GetSeed() == 1234);
		for (int i = 0; i < 1000; ++i)
		{
			CHECK(a.GetInt() == b.GetInt());
			const int value = a.GetInt(-5, 5);
			b.GetInt(-5, 5);
			CHECK(value >= -5);
			CHECK(value < 5);
			const float f = a.GetFloat();
			b.GetFloat();
			CHECK(f >= 0.0f);
			CHECK(f < 1.0f);
		}
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

TEST_SUITE("TRandom differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("random numbers match the original")
	{
		const auto construct = Original<Method<void>>("??0TRandom@Toshi@@QAE@XZ");
		const auto setSeed = Original<Method<void, unsigned int>>("?SetSeed@TRandom@Toshi@@QAEXI@Z");
		const auto getInt = Original<Method<int>>("?GetInt@TRandom@Toshi@@QAEHXZ");
		const auto getRange = Original<Method<int, int>>("?GetInt@TRandom@Toshi@@QAEHH@Z");
		const auto getMinMax = Original<Method<int, int, int>>("?GetInt@TRandom@Toshi@@QAEHHH@Z");
		const auto getFloat = Original<Method<float>>("?GetFloat@TRandom@Toshi@@QAEMXZ");
		const auto getFloat2 = Original<Method<float>>("?GetFloat2@TRandom@Toshi@@QAEMXZ");
		const auto getFloatRange = Original<Method<float, float, float>>("?GetFloatMinRange@TRandom@Toshi@@QAEMMM@Z");
		const auto getFloatMinMax = Original<Method<float, float, float>>("?GetFloatMinMax@TRandom@Toshi@@QAEMMM@Z");

		GamePrecisionScope precision;
		InputGenerator gen;
		for (int round = 0; round < 50; ++round)
		{
			const unsigned int seed = round < 3 ? static_cast<unsigned int>(round) : gen.NextU32();
			alignas(void*) unsigned char original[sizeof(TRandom)];
			construct(original);
			setSeed(original, seed);
			TRandom ours;
			ours.SetSeed(seed);
			REQUIRE(std::memcmp(original, &ours, sizeof(TRandom)) == 0);
			for (int step = 0; step < 500; ++step)
			{
				INFO("seed ", seed, " step ", step);
				const int a = gen.NextInt(-1000, 1000), b = gen.NextInt(-1000, 1000);
				const float x = gen.NextFloat(-100.0f, 100.0f), y = gen.NextFloat(-100.0f, 100.0f);
				switch (step % 7)
				{
				case 0:
					CHECK(getInt(original) == ours.GetInt());
					break;
				case 1:
					CHECK(getRange(original, a) == ours.GetInt(a));
					break;
				case 2:
					CHECK(getMinMax(original, a, b) == ours.GetInt(a, b));
					break;
				case 3:
					CHECK(Same(getFloat(original), ours.GetFloat()));
					break;
				case 4:
					CHECK(Same(getFloat2(original), ours.GetFloat2()));
					break;
				case 5:
					CHECK(Same(getFloatRange(original, x, y), ours.GetFloatMinRange(x, y)));
					break;
				default:
					CHECK(Same(getFloatMinMax(original, x, y), ours.GetFloatMinMax(x, y)));
					break;
				}
			}
			CHECK(std::memcmp(original, &ours, sizeof(TRandom)) == 0);
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
