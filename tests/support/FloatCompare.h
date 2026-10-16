#pragma once

#include <bit>
#include <cmath>
#include <cstdint>

namespace jpog::test
{

// Distance between two floats in units in the last place. Implementations that follow the
// gameplay precision rules (docs/plan.md) match the original at 0 ULPs; a non-zero tolerance
// needs a comment explaining why.
inline int64_t UlpDistance(float a, float b)
{
	auto ordered = [](float value)
	{
		const int32_t bits = std::bit_cast<int32_t>(value);
		return bits < 0 ? static_cast<int64_t>(INT32_MIN) - bits : static_cast<int64_t>(bits);
	};
	const int64_t distance = ordered(a) - ordered(b);
	return distance < 0 ? -distance : distance;
}

// True when both are NaN, or both are equal within `maxUlps`.
inline bool NearlyEqual(float a, float b, int64_t maxUlps)
{
	if (std::isnan(a) || std::isnan(b))
		return std::isnan(a) && std::isnan(b);
	return a == b || UlpDistance(a, b) <= maxUlps;
}

// Deterministic pseudo-random inputs so failures are reproducible.
class InputGenerator
{
public:
	explicit InputGenerator(uint32_t seed = 0x4A504F47u) : m_state(seed) {}

	uint32_t NextU32()
	{
		m_state = m_state * 1664525u + 1013904223u;
		return m_state;
	}

	float NextFloat(float min, float max)
	{
		const float unit = static_cast<float>(NextU32() >> 8) / static_cast<float>(1u << 24);
		return min + (max - min) * unit;
	}

	int NextInt(int min, int max) { return min + static_cast<int>(NextU32() % static_cast<uint32_t>(max - min + 1)); }

private:
	uint32_t m_state;
};

} // namespace jpog::test
