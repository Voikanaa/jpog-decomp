#include "toshi/kernel/TMutex.h"

#include <windows.h>

namespace Toshi
{

TMutex& TMutex::operator=(const TMutex& other)
{
	m_hMutex = other.m_hMutex;
	return *this;
}

bool TMutex::Create()
{
	m_hMutex = CreateMutexA(nullptr, FALSE, nullptr);
	return true;
}

bool TMutex::Destroy()
{
	const BOOL closed = CloseHandle(m_hMutex);
	m_hMutex = nullptr;
	return closed != FALSE;
}

bool TMutex::Lock(unsigned int flags)
{
	return WaitForSingleObject(m_hMutex, (flags & 1) ? 0 : INFINITE) == WAIT_OBJECT_0;
}

bool TMutex::Unlock()
{
	return ReleaseMutex(m_hMutex) != FALSE;
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
