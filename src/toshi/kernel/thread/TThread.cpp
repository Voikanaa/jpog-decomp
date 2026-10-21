#include "toshi/kernel/thread/TThread.h"

#include <windows.h>
#include <process.h>

#undef Yield

namespace Toshi
{

TThreadManager* TThreadManager::s_pThreadManager = nullptr;

namespace
{

CRITICAL_SECTION* Lock(unsigned char* storage)
{
	static_assert(sizeof(CRITICAL_SECTION) == (sizeof(void*) == 4 ? 24 : 40));
	return reinterpret_cast<CRITICAL_SECTION*>(storage);
}

} // namespace

// --- TThread -------------------------------------------------------------------------------

TThread::TThread() : m_hThread(nullptr), m_uiThreadID(0)
{
}

TThread::TThread(const TThread& other) : TNode(other), m_hThread(other.m_hThread), m_uiThreadID(other.m_uiThreadID)
{
}

TThread& TThread::operator=(const TThread& other)
{
	TNode::operator=(other);
	m_hThread = other.m_hThread;
	m_uiThreadID = other.m_uiThreadID;
	return *this;
}

bool TThread::Create(STACKSIZE stackSize, PRIORITY priority, unsigned int flags)
{
	m_hThread = nullptr;
	m_uiThreadID = 0xFFFFFFFF;
	// _beginthreadex returns the handle as an integer.
	m_hThread = reinterpret_cast<void*>(
	    _beginthreadex(nullptr, static_cast<unsigned int>(stackSize), Entry, this, CREATE_SUSPENDED, &m_uiThreadID));
	SetThreadPriority(m_hThread, priority);
	TThreadManager::GetThreadManager()->InsertThread(this);
	if ((flags & 1) == 0)
		ResumeThread(m_hThread);
	return true;
}

bool TThread::Destroy()
{
	if (m_hThread == nullptr)
		return false;
#pragma warning(suppress : 6258) // TerminateThread is what the original does
	TerminateThread(m_hThread, 0);
	CloseHandle(m_hThread);
	TThreadManager::GetThreadManager()->RemoveThread(this);
	m_hThread = nullptr;
	m_uiThreadID = 0xFFFFFFFF;
	return true;
}

bool TThread::Suspend()
{
	return SuspendThread(m_hThread) != 0xFFFFFFFF;
}

bool TThread::Resume()
{
	return ResumeThread(m_hThread) != 0xFFFFFFFF;
}

bool TThread::GetPriority(PRIORITY& priority)
{
	return GetPriority(m_hThread, priority);
}

bool TThread::SetPriority(PRIORITY priority)
{
	return SetPriority(m_hThread, priority);
}

bool TSTDCALL TThread::GetPriority(void* handle, PRIORITY& priority)
{
	priority = static_cast<PRIORITY>(GetThreadPriority(handle));
	return true;
}

bool TSTDCALL TThread::SetPriority(void* handle, PRIORITY priority)
{
	SetThreadPriority(handle, priority);
	return true;
}

bool TSTDCALL TThread::GetCurrentPriority(PRIORITY& priority)
{
	return GetPriority(GetCurrentThread(), priority);
}

bool TSTDCALL TThread::SetCurrentPriority(PRIORITY priority)
{
	return SetPriority(GetCurrentThread(), priority);
}

bool TSTDCALL TThread::Yield()
{
	Sleep(0);
	return true;
}

void TSTDCALL TThread::Exit()
{
	Exit(TThreadManager::GetThreadManager()->FindThread(GetCurrentThreadId()));
}

unsigned int TSTDCALL TThread::Entry(void* thread)
{
	static_cast<TThread*>(thread)->Main();
	Exit(static_cast<TThread*>(thread));
	return 0;
}

void TSTDCALL TThread::Exit(TThread* thread)
{
	CloseHandle(thread->m_hThread);
	TThreadManager::GetThreadManager()->RemoveThread(thread);
	thread->m_hThread = nullptr;
	thread->m_uiThreadID = 0xFFFFFFFF;
	_endthreadex(0);
}

// --- TThreadManager ------------------------------------------------------------------------

TThreadManager::TThreadManager()
{
	Create();
}

TThreadManager::~TThreadManager()
{
	Destroy();
}

TThreadManager& TThreadManager::operator=(const TThreadManager& other)
{
	for (std::size_t i = 0; i < sizeof(m_aLock); ++i)
		m_aLock[i] = other.m_aLock[i];
	m_oThreads = other.m_oThreads;
	return *this;
}

void TThreadManager::Create()
{
	s_pThreadManager = this;
	InitializeCriticalSection(Lock(m_aLock));
}

void TThreadManager::Destroy()
{
	s_pThreadManager = nullptr;
	DeleteCriticalSection(Lock(m_aLock));
}

void TThreadManager::InsertThread(TThread* thread)
{
	EnterCriticalSection(Lock(m_aLock));
	m_oThreads.InsertHead(thread);
	LeaveCriticalSection(Lock(m_aLock));
}

void TThreadManager::RemoveThread(TThread* thread)
{
	EnterCriticalSection(Lock(m_aLock));
	TDList<TThread>::Remove(thread);
	LeaveCriticalSection(Lock(m_aLock));
}

TThread* TThreadManager::FindThread(unsigned int threadID)
{
	EnterCriticalSection(Lock(m_aLock));
	TThread* found = nullptr;
	for (TThread* thread = m_oThreads.Head(); !m_oThreads.IsEnd(thread); thread = TDList<TThread>::NextOf(thread))
		if (thread->m_uiThreadID == threadID)
		{
			found = thread;
			break;
		}
	LeaveCriticalSection(Lock(m_aLock));
	return found;
}

TThreadManager* TSTDCALL TThreadManager::GetThreadManager()
{
	return s_pThreadManager;
}

// --- TSemaphore ----------------------------------------------------------------------------

TSemaphore& TSemaphore::operator=(const TSemaphore& other)
{
	m_hSemaphore = other.m_hSemaphore;
	return *this;
}

bool TSemaphore::Create(int initialCount, int maxCount)
{
	m_hSemaphore = CreateSemaphoreA(nullptr, initialCount, maxCount, nullptr);
	return true;
}

bool TSemaphore::Destroy()
{
	const BOOL closed = CloseHandle(m_hSemaphore);
	m_hSemaphore = nullptr;
	return closed != FALSE;
}

bool TSemaphore::Signal()
{
	return ReleaseSemaphore(m_hSemaphore, 1, nullptr) != FALSE;
}

bool TSemaphore::Wait()
{
	return WaitForSingleObject(m_hSemaphore, INFINITE) == WAIT_OBJECT_0;
}

bool TSemaphore::Poll()
{
	return WaitForSingleObject(m_hSemaphore, 0) == WAIT_OBJECT_0;
}

} // namespace Toshi
