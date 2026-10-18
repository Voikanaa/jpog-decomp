#pragma once

// Helpers shared by the differential tests against the original TKernelInterface.dll.

#include "support/OriginalModule.h"

#include <bit>
#include <cmath>
#include <cstdint>
#include <doctest/doctest.h>

namespace jpog::test
{

// Bitwise equality (also distinguishes +0/-0); any two NaNs compare equal.
inline bool Same(float a, float b)
{
	return std::bit_cast<uint32_t>(a) == std::bit_cast<uint32_t>(b) || (std::isnan(a) && std::isnan(b));
}

#if JPOG_HAS_ORIGINAL_MODULES

// Used as `doctest::skip(KernelMissing())` on differential suites.
inline bool KernelMissing()
{
	return !OriginalModule::Kernel().IsLoaded();
}

// Looks up an export of the original kernel; fails the test when it is missing.
template <typename Fn> Fn Original(const char* mangledName)
{
	const Fn fn = OriginalModule::Kernel().Get<Fn>(mangledName);
	REQUIRE_MESSAGE(fn != nullptr, "export not found: " << doctest::String(mangledName));
	return fn;
}

// The same for PPropertyParser.dll.
inline bool PPropertyParserMissing()
{
	return !OriginalModule::PPropertyParser().IsLoaded();
}

template <typename Fn> Fn OriginalPP(const char* mangledName)
{
	const Fn fn = OriginalModule::PPropertyParser().Get<Fn>(mangledName);
	REQUIRE_MESSAGE(fn != nullptr, "export not found: " << doctest::String(mangledName));
	return fn;
}

#endif // JPOG_HAS_ORIGINAL_MODULES

} // namespace jpog::test
