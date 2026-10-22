#pragma once

#include "common/layout.h"

namespace Toshi
{

// High-resolution timer over QueryPerformanceCounter (TKernelInterface.dll). Update samples the
// counter; times are in seconds.
class THPTimer
{
public:
	THPTimer();
	~THPTimer();

	void Reset();
	void Update();
	// Seconds between the last two updates.
	float GetDelta();
	// Seconds since the counter's origin at the last update.
	float GetCurrent();
	unsigned int GetRaw32();
	unsigned long long GetRaw64();

private:
	long long m_iFrequency;
	float m_fCurrent;
	long long m_iPrevious;
	long long m_iNow;
	float m_fDelta;
};

JPOG_ASSERT_SIZE(THPTimer, 0x28);

} // namespace Toshi
