#pragma once

#include "common/layout.h"

namespace Toshi
{

// Recursive OS mutex (TKernelInterface.dll; a Win32 mutex handle). Not created by construction:
// call Create, and Destroy when done.
class TMutex
{
public:
	TMutex() = default;
	TMutex(const TMutex& other) = default; // implicit in the original, not exported
	TMutex& operator=(const TMutex& other);

	bool Create();
	bool Destroy();
	// Bit 0 of `flags`: try once instead of waiting. Returns true when the mutex was acquired.
	bool Lock(unsigned int flags);
	bool Unlock();

private:
	void* m_hMutex;
};

JPOG_ASSERT_SIZE(TMutex, 4);

// Scoped lock: Create locks (waiting), Destroy unlocks.
class TMutexLock
{
public:
	TMutexLock();
	explicit TMutexLock(TMutex* mutex);
	TMutexLock(const TMutexLock& other);
	~TMutexLock();
	TMutexLock& operator=(const TMutexLock& other);

	void Create(TMutex* mutex);
	void Destroy();

private:
	TMutex* m_pMutex;
};

JPOG_ASSERT_SIZE(TMutexLock, 4);

} // namespace Toshi
