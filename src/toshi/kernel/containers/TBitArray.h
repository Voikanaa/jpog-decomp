#pragma once

#include "common/layout.h"

namespace Toshi
{

// Fixed-size bit set in 32-bit words (TKernelInterface.dll). Bit indices are not range checked.
class TBitArray
{
public:
	TBitArray();
	// Create(numBits, initialValue).
	TBitArray(int numBits, int initialValue);
	~TBitArray();
	// Shallow copy: both arrays then share (and would both free) the words.
	TBitArray& operator=(const TBitArray& other);

	// Allocates the words and fills them with the initial value (non-zero means all ones).
	bool Create(int numBits, int initialValue);
	void Destroy();
	// Refills every word, including the padding bits past numBits, with the initial value.
	void Reset();

	int GetBit(int index);
	void SetBit(int index);
	void SetBit(int index, int value);
	void ClearBit(int index);
	void ToggleBit(int index);
	// Index of the first set/clear bit, or -1. Whole words are scanned, so padding bits past
	// numBits can be reported, as in the original.
	int FindFirstSetBit();
	int FindFirstClearBit();

private:
	unsigned int* m_puiWords;
	int m_iNumBits;
	unsigned int m_uiInitialValue; // 0 or 0xFFFFFFFF
};

JPOG_ASSERT_SIZE(TBitArray, 12);

} // namespace Toshi
