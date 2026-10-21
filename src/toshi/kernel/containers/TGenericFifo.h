#pragma once

#include "common/layout.h"

namespace Toshi
{

// Thread-safe ring of fixed-size items over a caller's buffer (TKernelInterface.dll). Two
// semaphores count free and filled slots; flag 1 makes Push/Pop return false instead of waiting.
class TGenericFifo
{
public:
	TGenericFifo& operator=(const TGenericFifo& other);

	bool Create(void* buffer, int capacity, int itemSize);
	bool Destroy();
	bool Push(const void* item, unsigned int flags);
	bool Pop(void* item, unsigned int flags);

private:
	// A CRITICAL_SECTION (kept opaque so the header needs no windows.h).
	alignas(void*) unsigned char m_aLock[sizeof(void*) == 4 ? 24 : 40];
	void* m_hFreeSlots;
	void* m_hFilledSlots;
	int m_iItemSize;
	int m_iCapacity;
	unsigned char* m_pBuffer;
	unsigned char* m_pBufferEnd;
	unsigned char* m_pRead;
	unsigned char* m_pWrite;
};

JPOG_ASSERT_SIZE(TGenericFifo, 0x38);

} // namespace Toshi
