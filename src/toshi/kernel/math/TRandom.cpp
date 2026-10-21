#include "toshi/kernel/math/TRandom.h"

#include "toshi/kernel/time/THPTimer.h"

namespace Toshi
{

namespace
{

constexpr unsigned int kGoldenRatio = 0x9E3779B9;

void Mix(unsigned int (&x)[8])
{
	unsigned int& a = x[0];
	unsigned int& b = x[1];
	unsigned int& c = x[2];
	unsigned int& d = x[3];
	unsigned int& e = x[4];
	unsigned int& f = x[5];
	unsigned int& g = x[6];
	unsigned int& h = x[7];
	a ^= b << 11;
	d += a;
	b += c;
	b ^= c >> 2;
	e += b;
	c += d;
	c ^= d << 8;
	f += c;
	d += e;
	d ^= e >> 16;
	g += d;
	e += f;
	e ^= f << 10;
	h += e;
	f += g;
	f ^= g >> 4;
	a += f;
	g += h;
	g ^= h << 8;
	b += g;
	h += a;
	h ^= a >> 9;
	c += h;
	a += b;
}

} // namespace

TRandom::TRandom()
{
	THPTimer timer;
	SetSeed(timer.GetRaw32());
}

TRandom::~TRandom()
{
}

TRandom& TRandom::operator=(const TRandom& other) = default;

void TRandom::SetSeed(unsigned int seed)
{
	m_uiSeed = seed;
	RandInit();
}

unsigned int TRandom::GetSeed()
{
	return m_uiSeed;
}

// Bob Jenkins' randinit without a seed array: the seed is added to the golden ratio, which is
// scrambled four times, and the memory is filled from further mixes.
void TRandom::RandInit()
{
	m_uiA = 0;
	m_uiB = 0;
	m_uiC = 0;
	unsigned int x[8];
	for (unsigned int& value : x)
		value = kGoldenRatio + m_uiSeed;
	for (int i = 0; i < 4; ++i)
		Mix(x);
	for (int i = 0; i < 16; i += 8)
	{
		Mix(x);
		for (int j = 0; j < 8; ++j)
			m_auiMemory[i + j] = x[j];
	}
	Isaac();
	m_iRemaining = 16;
}

void TRandom::Isaac()
{
	auto indirect = [this](unsigned int value) { return m_auiMemory[(value >> 2) & 15]; };
	unsigned int a = m_uiA;
	unsigned int b = m_uiB + ++m_uiC;
	int result = 0;
	auto step = [&](int i, unsigned int mixed, int other)
	{
		const unsigned int x = m_auiMemory[i];
		a = mixed + m_auiMemory[other];
		const unsigned int y = indirect(x) + a + b;
		m_auiMemory[i] = y;
		b = indirect(y >> 4) + x;
		m_auiResults[result++] = b;
	};
	for (int half = 0; half < 2; ++half)
		for (int i = half * 8; i < half * 8 + 8; i += 4)
		{
			const int other = (i + 8) & 15;
			step(i, a ^ (a << 13), other);
			step(i + 1, a ^ (a >> 6), other + 1);
			step(i + 2, a ^ (a << 2), other + 2);
			step(i + 3, a ^ (a >> 16), other + 3);
		}
	m_uiB = b;
	m_uiA = a;
}

unsigned int TRandom::RandRaw()
{
	if (m_iRemaining-- != 0)
		return m_auiResults[m_iRemaining];
	Isaac();
	m_iRemaining = 15;
	return m_auiResults[15];
}

int TRandom::GetInt()
{
	return static_cast<int>(RandRaw());
}

int TRandom::GetInt(int range)
{
	return static_cast<int>((static_cast<unsigned long long>(static_cast<unsigned int>(range)) * RandRaw()) >> 32);
}

int TRandom::GetInt(int min, int max)
{
	const auto span = static_cast<unsigned int>(max - min);
	return min + static_cast<int>((static_cast<unsigned long long>(span) * RandRaw()) >> 32);
}

// The scale factors are powers of two, so float arithmetic matches the original's x87 code.
float TRandom::GetFloat()
{
	return static_cast<float>(static_cast<int>(RandRaw() >> 1)) * 4.656612873077393e-10f;
}

float TRandom::GetFloat2()
{
	return static_cast<float>(static_cast<int>(RandRaw() >> 1)) * 9.313225746154785e-10f + -1.0f;
}

float TRandom::GetFloatMinRange(float min, float range)
{
	return static_cast<float>(static_cast<int>(RandRaw() >> 1)) * 4.656612873077393e-10f * range + min;
}

float TRandom::GetFloatMinMax(float min, float max)
{
	return static_cast<float>(static_cast<int>(RandRaw() >> 1)) * 4.656612873077393e-10f * (max - min) + min;
}

} // namespace Toshi
