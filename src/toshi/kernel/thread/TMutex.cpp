#include "toshi/kernel/thread/TMutex.h"

#include "platform/Thread.h"

namespace Toshi
{

TMutex& TMutex::operator=(const TMutex& other)
{
	m_hMutex = other.m_hMutex;
	return *this;
}

bool TMutex::Create()
{
	m_hMutex = jpog::platform::MutexCreate();
	return true;
}

bool TMutex::Destroy()
{
	const bool closed = jpog::platform::MutexDestroy(m_hMutex);
	m_hMutex = nullptr;
	return closed;
}

bool TMutex::Lock(unsigned int flags)
{
	return jpog::platform::MutexLock(m_hMutex, (flags & 1) != 0);
}

bool TMutex::Unlock()
{
	return jpog::platform::MutexUnlock(m_hMutex);
}

TMutexLock::TMutexLock() : m_pMutex(nullptr)
{
}

TMutexLock::TMutexLock(TMutex* mutex)
{
	Create(mutex);
}

TMutexLock::TMutexLock(const TMutexLock& other) : m_pMutex(other.m_pMutex)
{
}

TMutexLock::~TMutexLock()
{
	Destroy();
}

TMutexLock& TMutexLock::operator=(const TMutexLock& other)
{
	m_pMutex = other.m_pMutex;
	return *this;
}

void TMutexLock::Create(TMutex* mutex)
{
	m_pMutex = mutex;
	mutex->Lock(0);
}

void TMutexLock::Destroy()
{
	if (m_pMutex != nullptr)
	{
		m_pMutex->Unlock();
		m_pMutex = nullptr;
	}
}

} // namespace Toshi
