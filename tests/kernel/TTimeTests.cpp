#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TCString.h"
#include "toshi/kernel/THPTimer.h"
#include "toshi/kernel/TSystem.h"

#include <cstring>
#include <doctest/doctest.h>
#include <string>

using Toshi::TCString;
using Toshi::TDateTime;
using Toshi::THPTimer;
using Toshi::TSystem;
using namespace jpog::test;

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TTime")
{
	TEST_CASE("date packing")
	{
		TDateTime date{2003, 10, 21, 7, 9, 31};
		unsigned int packedDate = 0, packedTime = 0;
		date.Pack(packedDate, packedTime);
		CHECK(packedDate == (2003u << 16 | 10u << 8 | 31u));
		CHECK(packedTime == (21u << 24 | 7u << 16 | 9u << 8));
		TDateTime unpacked{};
		unpacked.UnPack(packedDate, packedTime);
		CHECK(std::memcmp(&unpacked, &date, 7) == 0);
		TCString text;
		TSystem::StringFormatDateTime(date, text);
		CHECK(std::string(text) == "21:07:09 31/10/2003");
		CHECK(TSystem::GetLocalTime(date));
		CHECK(date.m_usYear >= 2026);
		CHECK(TSystem::GetTempCString() == reinterpret_cast<char*>(TSystem::GetScratchMem()));
	}

	TEST_CASE("high-resolution timer")
	{
		THPTimer timer;
		CHECK(timer.GetDelta() == 0.0f);
		const unsigned __int64 before = timer.GetRaw64();
		timer.Update();
		CHECK(timer.GetDelta() >= 0.0f);
		CHECK(timer.GetCurrent() > 0.0f);
		CHECK(timer.GetRaw64() >= before);
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

TEST_SUITE("TTime differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("date packing and formatting match the original")
	{
		const auto pack = Original<Method<void, unsigned int*, unsigned int*>>("?Pack@TDateTime@Toshi@@QAEXAAI0@Z");
		const auto unpack = Original<Method<void, unsigned int*, unsigned int*>>("?UnPack@TDateTime@Toshi@@QAEXAAI0@Z");
		using Format = void(__stdcall*)(const TDateTime*, TCString*);
		const auto format = Original<Format>("?StringFormatDateTime@TSystem@Toshi@@SGXABUTDateTime@2@AAVTCString@2@@Z");
		using LocalTime = bool(__stdcall*)(TDateTime*);
		const auto localTime = Original<LocalTime>("?GetLocalTime@TSystem@Toshi@@SG_NAAUTDateTime@2@@Z");
		const auto destroy = Original<Method<void>>("??1TCString@Toshi@@QAE@XZ");
		const auto construct = Original<Method<void>>("??0TCString@Toshi@@QAE@XZ");

		InputGenerator gen;
		for (int i = 0; i < 2000; ++i)
		{
			TDateTime date;
			unsigned char bytes[8];
			for (unsigned char& byte : bytes)
				byte = static_cast<unsigned char>(gen.NextU32());
			std::memcpy(&date, bytes, sizeof(date));
			TDateTime copy = date;
			unsigned int a[2] = {}, b[2] = {};
			pack(&date, &a[0], &a[1]);
			copy.Pack(b[0], b[1]);
			CHECK(a[0] == b[0]);
			CHECK(a[1] == b[1]);

			unsigned int date32 = gen.NextU32(), time32 = gen.NextU32();
			TDateTime x = date, y = date;
			unpack(&x, &date32, &time32);
			y.UnPack(date32, time32);
			CHECK(std::memcmp(&x, &y, sizeof(x)) == 0);

			alignas(void*) unsigned char storage[sizeof(TCString)];
			auto* text = reinterpret_cast<TCString*>(storage);
			construct(text);
			format(&date, text);
			TCString ours;
			TSystem::StringFormatDateTime(date, ours);
			CHECK(std::string(*text) == static_cast<const char*>(ours));
			destroy(text);
		}

		TDateTime original{}, ours{};
		CHECK(localTime(&original) == TSystem::GetLocalTime(ours));
		CHECK(original.m_usYear == ours.m_usYear);
		CHECK(original.m_ucMonth == ours.m_ucMonth);
		CHECK(original.m_ucDay == ours.m_ucDay);
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
