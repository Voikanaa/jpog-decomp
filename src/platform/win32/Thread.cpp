#include "platform/Thread.h"

#include <windows.h>
#include <process.h>

namespace jpog::platform
{

namespace
{

CRITICAL_SECTION* Lock(unsigned char* storage)
{
	static_assert(sizeof(CRITICAL_SECTION) == kLockStorageSize);
	return reinterpret_cast<CRITICAL_SECTION*>(storage);
}

} // namespace

void LockInit(unsigned char* storage)
{
	InitializeCriticalSection(Lock(storage));
}

void LockDestroy(unsigned char* storage)
{
	DeleteCriticalSection(Lock(storage));
}

void LockEnter(unsigned char* storage)
{
	EnterCriticalSection(Lock(storage));
}

void LockLeave(unsigned char* storage)
{
	LeaveCriticalSection(Lock(storage));
}

void* SemaphoreCreate(int initialCount, int maxCount)
{
	return CreateSemaphoreA(nullptr, initialCount, maxCount, nullptr);
}

bool SemaphoreDestroy(void* semaphore)
{
	return CloseHandle(semaphore) != FALSE;
}

bool SemaphoreSignal(void* semaphore)
{
	return ReleaseSemaphore(semaphore, 1, nullptr) != FALSE;
}

bool SemaphoreWait(void* semaphore, bool poll)
{
	return WaitForSingleObject(semaphore, poll ? 0 : INFINITE) == WAIT_OBJECT_0;
}

void* MutexCreate()
{
	return CreateMutexA(nullptr, FALSE, nullptr);
}

bool MutexDestroy(void* mutex)
{
	return CloseHandle(mutex) != FALSE;
}

bool MutexLock(void* mutex, bool poll)
{
	return WaitForSingleObject(mutex, poll ? 0 : INFINITE) == WAIT_OBJECT_0;
}

bool MutexUnlock(void* mutex)
{
	return ReleaseMutex(mutex) != FALSE;
}

void* ThreadCreate(unsigned int stackSize, ThreadEntry entry, void* argument, bool suspended, unsigned int& id)
{
	// _beginthreadex returns the handle as an integer.
	return reinterpret_cast<void*>(
	    _beginthreadex(nullptr, stackSize, entry, argument, suspended ? CREATE_SUSPENDED : 0, &id));
}

void ThreadTerminate(void* thread)
{
#pragma warning(suppress : 6258) // TerminateThread is what the original does
	::TerminateThread(thread, 0);
}

void ThreadClose(void* thread)
{
	CloseHandle(thread);
}

bool ThreadSuspend(void* thread)
{
	return SuspendThread(thread) != 0xFFFFFFFF;
}

bool ThreadResume(void* thread)
{
	return ResumeThread(thread) != 0xFFFFFFFF;
}

int ThreadGetPriority(void* thread)
{
	return GetThreadPriority(thread);
}

void ThreadSetPriority(void* thread, int priority)
{
	SetThreadPriority(thread, priority);
}

void* ThreadCurrent()
{
	return GetCurrentThread();
}

unsigned int ThreadCurrentId()
{
	return GetCurrentThreadId();
}

void ThreadYield()
{
	Sleep(0);
}

void ThreadExit()
{
	_endthreadex(0);
}

} // namespace jpog::platform
