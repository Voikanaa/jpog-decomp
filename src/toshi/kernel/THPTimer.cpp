#include "toshi/kernel/THPTimer.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace Toshi
{

namespace
{

__int64 Counter()
{
	LARGE_INTEGER value;
	QueryPerformanceCounter(&value);
	return value.QuadPart;
}

} // namespace

// The original samples the counter once before Reset and discards it.
THPTimer::THPTimer()
{
	Counter();
	Reset();
}

THPTimer::~THPTimer()
{
}

void THPTimer::Reset()
{
	m_iNow = Counter();
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	m_iFrequency = frequency.QuadPart;
	m_iPrevious = m_iNow;
	m_fDelta = 0.0f;
}

// The original multiplies the exact tick counts by 1 / frequency on the x87 stack; double
// arithmetic matches it except for rare double rounding.
void THPTimer::Update()
{
	m_iPrevious = m_iNow;
	m_iNow = Counter();
	const double inverse = 1.0 / static_cast<double>(m_iFrequency);
	m_fDelta = static_cast<float>(static_cast<double>(m_iNow - m_iPrevious) * inverse);
	m_fCurrent = static_cast<float>(static_cast<double>(m_iNow) * inverse);
}

float THPTimer::GetDelta()
{
	return m_fDelta;
}

float THPTimer::GetCurrent()
{
	return m_fCurrent;
}

unsigned int THPTimer::GetRaw32()
{
	return static_cast<unsigned int>(Counter());
}

unsigned __int64 THPTimer::GetRaw64()
{
	return static_cast<unsigned __int64>(Counter());
}

} // namespace Toshi
