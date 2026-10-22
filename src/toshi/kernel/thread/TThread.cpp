#include "toshi/kernel/thread/TThread.h"

#include "platform/Thread.h"

namespace Toshi
{

using namespace jpog::platform;

TThreadManager* TThreadManager::s_pThreadManager = nullptr;

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
	m_hThread = ThreadCreate(static_cast<unsigned int>(stackSize), Entry, this, true, m_uiThreadID);
	ThreadSetPriority(m_hThread, priority);
	TThreadManager::GetThreadManager()->InsertThread(this);
	if ((flags & 1) == 0)
		ThreadResume(m_hThread);
	return true;
}

bool TThread::Destroy()
{
	if (m_hThread == nullptr)
		return false;
	ThreadTerminate(m_hThread);
	ThreadClose(m_hThread);
	TThreadManager::GetThreadManager()->RemoveThread(this);
	m_hThread = nullptr;
	m_uiThreadID = 0xFFFFFFFF;
	return true;
}

bool TThread::Suspend()
{
	return ThreadSuspend(m_hThread);
}

bool TThread::Resume()
{
	return ThreadResume(m_hThread);
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
	priority = static_cast<PRIORITY>(ThreadGetPriority(handle));
	return true;
}

bool TSTDCALL TThread::SetPriority(void* handle, PRIORITY priority)
{
	ThreadSetPriority(handle, priority);
	return true;
}

bool TSTDCALL TThread::GetCurrentPriority(PRIORITY& priority)
{
	return GetPriority(ThreadCurrent(), priority);
}

bool TSTDCALL TThread::SetCurrentPriority(PRIORITY priority)
{
	return SetPriority(ThreadCurrent(), priority);
}

bool TSTDCALL TThread::Yield()
{
	ThreadYield();
	return true;
}

void TSTDCALL TThread::Exit()
{
	Exit(TThreadManager::GetThreadManager()->FindThread(ThreadCurrentId()));
}

unsigned int TSTDCALL TThread::Entry(void* thread)
{
	static_cast<TThread*>(thread)->Main();
	Exit(static_cast<TThread*>(thread)); // never returns
}

void TSTDCALL TThread::Exit(TThread* thread)
{
	ThreadClose(thread->m_hThread);
	TThreadManager::GetThreadManager()->RemoveThread(thread);
	thread->m_hThread = nullptr;
	thread->m_uiThreadID = 0xFFFFFFFF;
	ThreadExit();
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
	LockInit(m_aLock);
}

void TThreadManager::Destroy()
{
	s_pThreadManager = nullptr;
	LockDestroy(m_aLock);
}

void TThreadManager::InsertThread(TThread* thread)
{
	LockEnter(m_aLock);
	m_oThreads.InsertHead(thread);
	LockLeave(m_aLock);
}

void TThreadManager::RemoveThread(TThread* thread)
{
	LockEnter(m_aLock);
	TDList<TThread>::Remove(thread);
	LockLeave(m_aLock);
}

TThread* TThreadManager::FindThread(unsigned int threadID)
{
	LockEnter(m_aLock);
	TThread* found = nullptr;
	for (TThread* thread = m_oThreads.Head(); thread; thread = m_oThreads.Next(thread))
		if (thread->m_uiThreadID == threadID)
		{
			found = thread;
			break;
		}
	LockLeave(m_aLock);
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
	m_hSemaphore = SemaphoreCreate(initialCount, maxCount);
	return true;
}

bool TSemaphore::Destroy()
{
	const bool closed = SemaphoreDestroy(m_hSemaphore);
	m_hSemaphore = nullptr;
	return closed;
}

bool TSemaphore::Signal()
{
	return SemaphoreSignal(m_hSemaphore);
}

bool TSemaphore::Wait()
{
	return SemaphoreWait(m_hSemaphore, false);
}

bool TSemaphore::Poll()
{
	return SemaphoreWait(m_hSemaphore, true);
}

} // namespace Toshi
