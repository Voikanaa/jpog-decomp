#pragma once

#include "common/abi.h"

#include <cstddef>

// Threads and synchronisation for the kernel: the original's Win32 calls on Windows, POSIX
// threads and standard C++ elsewhere. Handles are opaque pointers. (Names avoid the Win32 API
// names, which <windows.h> redefines as macros.)
namespace jpog::platform
{

// Storage for an in-place recursive lock. On Windows it is a CRITICAL_SECTION, whose size the
// original class layouts include; elsewhere it holds a pointer to a heap-allocated mutex.
#if defined(_WIN32)
inline constexpr std::size_t kLockStorageSize = sizeof(void*) == 4 ? 24 : 40;
#else
inline constexpr std::size_t kLockStorageSize = sizeof(void*);
#endif

void LockInit(unsigned char* storage);
void LockDestroy(unsigned char* storage);
void LockEnter(unsigned char* storage);
void LockLeave(unsigned char* storage);

// Counting semaphore; Wait with `poll` returns at once.
void* SemaphoreCreate(int initialCount, int maxCount);
bool SemaphoreDestroy(void* semaphore);
bool SemaphoreSignal(void* semaphore);
bool SemaphoreWait(void* semaphore, bool poll);

// Recursive mutex owned by the locking thread.
void* MutexCreate();
bool MutexDestroy(void* mutex);
bool MutexLock(void* mutex, bool poll);
bool MutexUnlock(void* mutex);

using ThreadEntry = unsigned int(TSTDCALL*)(void*);

// Creates a thread, suspended until ThreadResume if `suspended`; `id` receives its thread ID.
void* ThreadCreate(unsigned int stackSize, ThreadEntry entry, void* argument, bool suspended, unsigned int& id);
// Ends a thread from outside. Only Windows can; elsewhere the thread is left running.
void ThreadTerminate(void* thread);
void ThreadClose(void* thread);
// Suspending a running thread is only supported on Windows.
bool ThreadSuspend(void* thread);
bool ThreadResume(void* thread);
// Priorities take the Win32 values (THREAD_PRIORITY_*).
int ThreadGetPriority(void* thread);
void ThreadSetPriority(void* thread, int priority);
void* ThreadCurrent();
unsigned int ThreadCurrentId();
void ThreadYield();
// Ends the calling thread.
[[noreturn]] void ThreadExit();

} // namespace jpog::platform
