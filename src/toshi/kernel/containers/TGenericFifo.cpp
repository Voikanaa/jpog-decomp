#include "toshi/kernel/containers/TGenericFifo.h"

#include "platform/Thread.h"

namespace Toshi
{

using namespace jpog::platform;

TGenericFifo& TGenericFifo::operator=(const TGenericFifo& other) = default;

bool TGenericFifo::Create(void* buffer, int capacity, int itemSize)
{
	m_iItemSize = itemSize;
	m_iCapacity = capacity;
	m_pBuffer = static_cast<unsigned char*>(buffer);
	m_pBufferEnd = m_pBuffer + itemSize * capacity;
	m_pRead = m_pBuffer;
	m_pWrite = m_pBuffer;
	m_hFreeSlots = SemaphoreCreate(capacity, capacity);
	m_hFilledSlots = SemaphoreCreate(0, m_iCapacity);
	LockInit(m_aLock);
	return true;
}

bool TGenericFifo::Destroy()
{
	LockDestroy(m_aLock);
	SemaphoreDestroy(m_hFilledSlots);
	m_hFilledSlots = nullptr;
	SemaphoreDestroy(m_hFreeSlots);
	m_hFreeSlots = nullptr;
	return true;
}

bool TGenericFifo::Push(const void* item, unsigned int flags)
{
	if (!SemaphoreWait(m_hFreeSlots, (flags & 1) != 0))
		return false;
	LockEnter(m_aLock);
	const auto* in = static_cast<const unsigned char*>(item);
	for (int i = 0; i < m_iItemSize; ++i)
		m_pWrite[i] = in[i];
	m_pWrite = m_pWrite + m_iItemSize == m_pBufferEnd ? m_pBuffer : m_pWrite + m_iItemSize;
	LockLeave(m_aLock);
	SemaphoreSignal(m_hFilledSlots);
	return true;
}

bool TGenericFifo::Pop(void* item, unsigned int flags)
{
	if (!SemaphoreWait(m_hFilledSlots, (flags & 1) != 0))
		return false;
	LockEnter(m_aLock);
	auto* out = static_cast<unsigned char*>(item);
	for (int i = 0; i < m_iItemSize; ++i)
		out[i] = m_pRead[i];
	m_pRead = m_pRead + m_iItemSize == m_pBufferEnd ? m_pBuffer : m_pRead + m_iItemSize;
	LockLeave(m_aLock);
	SemaphoreSignal(m_hFreeSlots);
	return true;
}

} // namespace Toshi
