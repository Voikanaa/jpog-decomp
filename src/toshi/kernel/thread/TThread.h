#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "platform/Thread.h"
#include "toshi/kernel/containers/TGenericDList.h"

namespace Toshi
{

// A thread running Main() (TKernelInterface.dll), registered with the thread manager while it
// runs. The enums take the Win32 values (stack size in bytes, thread priority).
class TThread : public TGenericDList::TNode
{
	friend class TThreadManager;

public:
	enum STACKSIZE
	{
		STACKSIZE_DEFAULT = 0,
	};

	enum PRIORITY
	{
		PRIORITY_IDLE = -15,
		PRIORITY_LOWEST = -2,
		PRIORITY_BELOW_NORMAL = -1,
		PRIORITY_NORMAL = 0,
		PRIORITY_ABOVE_NORMAL = 1,
		PRIORITY_HIGHEST = 2,
		PRIORITY_TIME_CRITICAL = 15,
	};

	TThread();
	TThread(const TThread& other);
	TThread& operator=(const TThread& other);

	virtual void Main() = 0;

	// Starts the thread (flag 1: created suspended).
	bool Create(STACKSIZE stackSize, PRIORITY priority, unsigned int flags);
	// Terminates the thread.
	bool Destroy();
	bool Suspend();
	bool Resume();
	bool GetPriority(PRIORITY& priority);
	bool SetPriority(PRIORITY priority);

	static bool TSTDCALL GetCurrentPriority(PRIORITY& priority);
	static bool TSTDCALL SetCurrentPriority(PRIORITY priority);
	static bool TSTDCALL Yield();
	// Ends the calling thread (which must be a TThread).
	static void TSTDCALL Exit();

private:
	static unsigned int TSTDCALL Entry(void* thread);
	static void TSTDCALL Exit(TThread* thread);
	static bool TSTDCALL GetPriority(void* handle, PRIORITY& priority);
	static bool TSTDCALL SetPriority(void* handle, PRIORITY priority);

	void* m_hThread;
	unsigned int m_uiThreadID;
};

JPOG_ASSERT_SIZE(TThread, 0x14);

// The list of running threads (TKernelInterface.dll), guarded by a critical section.
class TThreadManager
{
public:
	TThreadManager();
	~TThreadManager();
	TThreadManager& operator=(const TThreadManager& other);

	void Create();
	void Destroy();
	void InsertThread(TThread* thread);
	void RemoveThread(TThread* thread);
	TThread* FindThread(unsigned int threadID);

	static TThreadManager* TSTDCALL GetThreadManager();

private:
	static TThreadManager* s_pThreadManager;

	// A CRITICAL_SECTION on Windows (kept opaque so the header needs no windows.h).
	alignas(void*) unsigned char m_aLock[jpog::platform::kLockStorageSize];
	TDList<TThread> m_oThreads;
};

JPOG_ASSERT_SIZE(TThreadManager, 0x20);

// Counting semaphore (TKernelInterface.dll).
class TSemaphore
{
public:
	TSemaphore& operator=(const TSemaphore& other);

	bool Create(int initialCount, int maxCount);
	bool Destroy();
	bool Signal();
	// Waits until the count is above zero.
	bool Wait();
	// Takes the semaphore if it is available.
	bool Poll();

private:
	void* m_hSemaphore;
};

JPOG_ASSERT_SIZE(TSemaphore, 4);

} // namespace Toshi
