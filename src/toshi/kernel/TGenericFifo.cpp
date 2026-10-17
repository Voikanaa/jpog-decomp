#include "toshi/kernel/TGenericFifo.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace Toshi
{

namespace
{

CRITICAL_SECTION* Lock(unsigned char* storage)
{
	static_assert(sizeof(CRITICAL_SECTION) == (sizeof(void*) == 4 ? 24 : 40));
	return reinterpret_cast<CRITICAL_SECTION*>(storage);
}

} // namespace

TGenericFifo& TGenericFifo::operator=(const TGenericFifo& other) = default;

bool TGenericFifo::Create(void* buffer, int capacity, int itemSize)
{
	m_iItemSize = itemSize;
	m_iCapacity = capacity;
	m_pBuffer = static_cast<unsigned char*>(buffer);
	m_pBufferEnd = m_pBuffer + itemSize * capacity;
	m_pRead = m_pBuffer;
	m_pWrite = m_pBuffer;
	m_hFreeSlots = CreateSemaphoreA(nullptr, capacity, capacity, nullptr);
	m_hFilledSlots = CreateSemaphoreA(nullptr, 0, m_iCapacity, nullptr);
	InitializeCriticalSection(Lock(m_aLock));
	return true;
}

bool TGenericFifo::Destroy()
{
	DeleteCriticalSection(Lock(m_aLock));
	CloseHandle(m_hFilledSlots);
	m_hFilledSlots = nullptr;
	CloseHandle(m_hFreeSlots);
	m_hFreeSlots = nullptr;
	return true;
}

bool TGenericFifo::Push(const void* item, unsigned int flags)
{
	if (WaitForSingleObject(m_hFreeSlots, (flags & 1) != 0 ? 0 : INFINITE) != WAIT_OBJECT_0)
		return false;
	EnterCriticalSection(Lock(m_aLock));
	const auto* in = static_cast<const unsigned char*>(item);
	for (int i = 0; i < m_iItemSize; ++i)
		m_pWrite[i] = in[i];
	m_pWrite = m_pWrite + m_iItemSize == m_pBufferEnd ? m_pBuffer : m_pWrite + m_iItemSize;
	LeaveCriticalSection(Lock(m_aLock));
	ReleaseSemaphore(m_hFilledSlots, 1, nullptr);
	return true;
}

bool TGenericFifo::Pop(void* item, unsigned int flags)
{
	if (WaitForSingleObject(m_hFilledSlots, (flags & 1) != 0 ? 0 : INFINITE) != WAIT_OBJECT_0)
		return false;
	EnterCriticalSection(Lock(m_aLock));
	auto* out = static_cast<unsigned char*>(item);
	for (int i = 0; i < m_iItemSize; ++i)
		out[i] = m_pRead[i];
	m_pRead = m_pRead + m_iItemSize == m_pBufferEnd ? m_pBuffer : m_pRead + m_iItemSize;
	LeaveCriticalSection(Lock(m_aLock));
	ReleaseSemaphore(m_hFreeSlots, 1, nullptr);
	return true;
}

} // namespace Toshi
