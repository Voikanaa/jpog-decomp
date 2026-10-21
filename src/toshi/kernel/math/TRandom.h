#pragma once

#include "common/layout.h"

namespace Toshi
{

// ISAAC random numbers with a 16-word state (TKernelInterface.dll), seeded from the
// performance counter.
class TRandom
{
public:
	TRandom();
	~TRandom();
	TRandom& operator=(const TRandom& other);

	void SetSeed(unsigned int seed);
	unsigned int GetSeed();
	// Raw 32-bit value.
	int GetInt();
	// [0, range) and [min, max) by 32x32 -> 64-bit scaling.
	int GetInt(int range);
	int GetInt(int min, int max);
	// [0, 1), [-1, 1), [min, min + range) and [min, max), from the top 31 bits.
	float GetFloat();
	float GetFloat2();
	float GetFloatMinRange(float min, float range);
	float GetFloatMinMax(float min, float max);

private:
	void RandInit();
	void Isaac();
	unsigned int RandRaw();

	unsigned int m_uiSeed;
	int m_iRemaining; // unread results
	unsigned int m_auiResults[16];
	unsigned int m_auiMemory[16];
	unsigned int m_uiA;
	unsigned int m_uiB;
	unsigned int m_uiC;
};

JPOG_ASSERT_SIZE(TRandom, 0x94);

} // namespace Toshi
