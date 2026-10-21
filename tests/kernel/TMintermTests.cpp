#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TMinterm.h"
#include "toshi/kernel/strings/TCString.h"
#include "toshi/kernel/strings/TCStringPool.h"

#include <cstring>
#include <doctest/doctest.h>
#include <string>

using Toshi::TArray;
using Toshi::TCString;
using Toshi::TCStringPool;
using Toshi::TMinterm;
using Toshi::TPCString;
using Toshi::TSumOfMinterms;
using namespace jpog::test;

namespace
{

std::string Describe(const TSumOfMinterms& sum, const char** opNames, const TArray<TPCString>& names)
{
	TCString text;
	sum.MakeFunctionString(text, opNames, names);
	return text.GetString(0);
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TMinterm")
{
	TEST_CASE("truth and function strings")
	{
		TCStringPool pool;
		TArray<TPCString> names(4, 0);
		names.Push(pool.Get("a"));
		names.Push(pool.Get("b"));
		names.Push(pool.Get("c"));
		const char** english = TMinterm::ms_aEnglishOpNames;
		const char** math = TMinterm::ms_aMathOpNames;

		TSumOfMinterms sum;
		CHECK(sum.GetTruth(0));
		CHECK(Describe(sum, english, names) == "true");

		TMinterm aNotB(3, 1);
		CHECK(aNotB.GetTruth(5));
		CHECK_FALSE(aNotB.GetTruth(3));
		sum + aNotB;
		CHECK(Describe(sum, english, names) == "a and not b");
		CHECK(Describe(sum, math, names) == "a & ~b");
		sum + TMinterm(4, 4) + TMinterm(false);
		CHECK(Describe(sum, english, names) == "(a and not b) or (c) or (false)");
		CHECK(sum.GetTruth(4));
		CHECK(sum.GetTruth(0)); // the false minterm holds for no inputs set
		CHECK_FALSE(sum.GetTruth(2));

		TSumOfMinterms copy(sum);
		CHECK(Describe(copy, math, names) == "(a & ~b) + (c) + (0)");
		TSumOfMinterms single(TMinterm(true));
		CHECK(Describe(single, english, names) == "true");
		copy = single;
		CHECK(copy.MakeFunctionString(math, names).GetCString().GetString(0) == std::string("1"));
		CHECK_FALSE(copy.ParseFunctionString(TCString("a"), english, names));
		CHECK(TMinterm(1, 0) == TMinterm(1, 0));
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

TEST_SUITE("TMinterm differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("sums, truth and strings match the original")
	{
		using P = void*;
		const auto constructSize = Original<Method<void, int>>("??0TSumOfMinterms@Toshi@@QAE@H@Z");
		const auto constructOne =
		    Original<Method<void, const TMinterm*>>("??0TSumOfMinterms@Toshi@@QAE@ABVTMinterm@1@@Z");
		const auto copy = Original<Method<void, P>>("??0TSumOfMinterms@Toshi@@QAE@ABV01@@Z");
		const auto destruct = Original<Method<void>>("??1TSumOfMinterms@Toshi@@QAE@XZ");
		const auto add = Original<Method<P, const TMinterm*>>("??HTSumOfMinterms@Toshi@@QAEAAV01@ABVTMinterm@1@@Z");
		const auto truth = Original<Method<bool, unsigned int>>("?GetTruth@TSumOfMinterms@Toshi@@QAE_NI@Z");
		const auto make = Original<Method<void, P, const char**, const TArray<TPCString>*>>(
		    "?MakeFunctionString@TSumOfMinterms@Toshi@@QBEXAAVTCString@2@QAPBDABV?$TArray@VTPCString@Toshi@@@2@@Z");
		const auto stringCtor = Original<Method<void>>("??0TCString@Toshi@@QAE@XZ");
		const auto stringDtor = Original<Method<void>>("??1TCString@Toshi@@QAE@XZ");
		const auto getString = Original<Method<const char*, int>>("?GetString@TCString@Toshi@@QBEPBDH@Z");
		const auto english = static_cast<const char**>(Original<void*>("?ms_aEnglishOpNames@TMinterm@Toshi@@2PAPBDA"));
		for (int i = 0; i < 7; ++i)
			CHECK(std::string(english[i]) == TMinterm::ms_aEnglishOpNames[i]);

		TCStringPool pool;
		TArray<TPCString> names(8, 0);
		for (int i = 0; i < 34; ++i)
			names.Push(pool.Get(("in" + std::to_string(i)).c_str()));

		// Raw view: grow size, count, capacity, minterm data.
		const auto view = [](const void* sum)
		{
			int fields[4];
			std::memcpy(fields, sum, sizeof(fields));
			std::string text =
			    std::to_string(fields[0]) + " " + std::to_string(fields[1]) + " " + std::to_string(fields[2]) + ":";
			const auto* data = *reinterpret_cast<const unsigned int* const*>(static_cast<const char*>(sum) + 12);
			for (int i = 0; i < fields[1]; ++i)
				text += " " + std::to_string(data[2 * i]) + "/" + std::to_string(data[2 * i + 1]);
			return text;
		};

		InputGenerator gen;
		for (int round = 0; round < 300; ++round)
		{
			INFO("round ", round);
			const auto randomMinterm = [&]
			{
				const int kind = static_cast<int>(gen.NextU32() % 8);
				if (kind == 0)
					return TMinterm(kind == 0 && gen.NextU32() % 2 == 0);
				const unsigned int mask = gen.NextU32() & gen.NextU32();
				return TMinterm(mask, (gen.NextU32() & mask) ^ (kind == 1 ? 1u : 0u));
			};
			alignas(void*) unsigned char original[sizeof(TSumOfMinterms)];
			const int start = gen.NextInt(-1, 3);
			TSumOfMinterms ours = start == 3 ? TSumOfMinterms(TMinterm(5, 1)) : TSumOfMinterms(start);
			if (start == 3)
			{
				const TMinterm first(5, 1);
				constructOne(original, &first);
			}
			else
				constructSize(original, start);
			const int count = gen.NextInt(0, 9);
			for (int i = 0; i < count; ++i)
			{
				const TMinterm minterm = randomMinterm();
				add(original, &minterm);
				ours + minterm;
			}
			alignas(void*) unsigned char originalCopy[sizeof(TSumOfMinterms)];
			copy(originalCopy, original);
			const TSumOfMinterms oursCopy(ours);
			REQUIRE(view(original) == view(&ours));
			CHECK(view(originalCopy) == view(&oursCopy));
			for (int i = 0; i < 64; ++i)
			{
				const unsigned int inputs = gen.NextU32() & gen.NextU32();
				CHECK(truth(original, inputs) == ours.GetTruth(inputs));
			}
			for (const char** opNames : {TMinterm::ms_aEnglishOpNames, TMinterm::ms_aMathOpNames})
			{
				alignas(void*) unsigned char text[sizeof(TCString)];
				stringCtor(text);
				make(original, text, opNames, &names);
				CHECK(std::string(getString(text, 0)) == Describe(ours, opNames, names));
				stringDtor(text);
			}
			destruct(originalCopy);
			destruct(original);
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
