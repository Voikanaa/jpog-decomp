#include "toshi/kernel/containers/TBitArray.h"

#include "toshi/kernel/memory/TMemory.h"

namespace Toshi
{

namespace
{

int WordCount(int numBits)
{
	return (numBits + 31) >> 5;
}

unsigned int Mask(int index)
{
	return 1u << (index & 31);
}

} // namespace

TBitArray::TBitArray() : m_puiWords(nullptr), m_iNumBits(0), m_uiInitialValue(0)
{
}

TBitArray::TBitArray(int numBits, int initialValue)
{
	Create(numBits, initialValue);
}

TBitArray::~TBitArray()
{
	Destroy();
}

TBitArray& TBitArray::operator=(const TBitArray& other)
{
	m_puiWords = other.m_puiWords;
	m_iNumBits = other.m_iNumBits;
	m_uiInitialValue = other.m_uiInitialValue;
	return *this;
}

bool TBitArray::Create(int numBits, int initialValue)
{
	m_iNumBits = numBits;
	m_uiInitialValue = initialValue == 0 ? 0u : 0xFFFFFFFFu;
	m_puiWords = static_cast<unsigned int*>(tmalloc(WordCount(numBits) * 4, nullptr, -1));
	Reset();
	return true;
}

void TBitArray::Destroy()
{
	tfree(m_puiWords);
	m_puiWords = nullptr;
}

void TBitArray::Reset()
{
	for (int i = 0; i < WordCount(m_iNumBits); ++i)
		m_puiWords[i] = m_uiInitialValue;
}

int TBitArray::GetBit(int index)
{
	return static_cast<int>((m_puiWords[index >> 5] >> (index & 31)) & 1);
}

void TBitArray::SetBit(int index)
{
	m_puiWords[index >> 5] |= Mask(index);
}

void TBitArray::SetBit(int index, int value)
{
	if (value == 0)
		m_puiWords[index >> 5] &= ~Mask(index);
	else
		m_puiWords[index >> 5] |= Mask(index);
}

void TBitArray::ClearBit(int index)
{
	m_puiWords[index >> 5] &= ~Mask(index);
}

void TBitArray::ToggleBit(int index)
{
	m_puiWords[index >> 5] ^= Mask(index);
}

int TBitArray::FindFirstSetBit()
{
	for (int word = 0; word < WordCount(m_iNumBits); ++word)
	{
		const unsigned int bits = m_puiWords[word];
		if (bits != 0)
			for (int bit = 0; bit < 32; ++bit)
				if (bits & (1u << bit))
					return word * 32 + bit;
	}
	return -1;
}

int TBitArray::FindFirstClearBit()
{
	for (int word = 0; word < WordCount(m_iNumBits); ++word)
	{
		const unsigned int bits = m_puiWords[word];
		if (bits != 0xFFFFFFFFu)
			for (int bit = 0; bit < 32; ++bit)
				if (!(bits & (1u << bit)))
					return word * 32 + bit;
	}
	return -1;
}

} // namespace Toshi
