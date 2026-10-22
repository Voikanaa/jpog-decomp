#include "platform/Thread.h"

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <sched.h>

namespace jpog::platform
{

// --- Locks -----------------------------------------------------------------------------------

namespace
{

std::recursive_mutex*& Lock(unsigned char* storage)
{
	static_assert(sizeof(std::recursive_mutex*) == kLockStorageSize);
	return *reinterpret_cast<std::recursive_mutex**>(storage);
}

} // namespace

void LockInit(unsigned char* storage)
{
	Lock(storage) = new std::recursive_mutex();
}

void LockDestroy(unsigned char* storage)
{
	delete Lock(storage);
	Lock(storage) = nullptr;
}

void LockEnter(unsigned char* storage)
{
	Lock(storage)->lock();
}

void LockLeave(unsigned char* storage)
{
	Lock(storage)->unlock();
}

// --- Semaphores and mutexes ------------------------------------------------------------------

namespace
{

struct Semaphore
{
	std::mutex mutex;
	std::condition_variable available;
	int count;
	int maxCount;
};

} // namespace

void* SemaphoreCreate(int initialCount, int maxCount)
{
	if (maxCount <= 0 || initialCount < 0 || initialCount > maxCount)
		return nullptr;
	return new Semaphore{{}, {}, initialCount, maxCount};
}

bool SemaphoreDestroy(void* semaphore)
{
	if (semaphore == nullptr)
		return false;
	delete static_cast<Semaphore*>(semaphore);
	return true;
}

// Fails, like ReleaseSemaphore, when the count would exceed its maximum.
bool SemaphoreSignal(void* semaphore)
{
	auto& s = *static_cast<Semaphore*>(semaphore);
	{
		std::lock_guard lock(s.mutex);
		if (s.count >= s.maxCount)
			return false;
		++s.count;
	}
	s.available.notify_one();
	return true;
}

bool SemaphoreWait(void* semaphore, bool poll)
{
	auto& s = *static_cast<Semaphore*>(semaphore);
	std::unique_lock lock(s.mutex);
	if (poll && s.count == 0)
		return false;
	s.available.wait(lock, [&] { return s.count > 0; });
	--s.count;
	return true;
}

void* MutexCreate()
{
	return new std::recursive_mutex();
}

bool MutexDestroy(void* mutex)
{
	if (mutex == nullptr)
		return false;
	delete static_cast<std::recursive_mutex*>(mutex);
	return true;
}

bool MutexLock(void* mutex, bool poll)
{
	auto* m = static_cast<std::recursive_mutex*>(mutex);
	if (poll)
		return m->try_lock();
	m->lock();
	return true;
}

bool MutexUnlock(void* mutex)
{
	static_cast<std::recursive_mutex*>(mutex)->unlock();
	return true;
}

// --- Threads ---------------------------------------------------------------------------------

namespace
{

// A thread's shared state. The handle owns one reference; the running thread holds another in
// a thread_local until it ends.
struct ThreadState
{
	ThreadEntry entry = nullptr;
	void* argument = nullptr;
	unsigned int id = 0;
	int priority = 0;
	pthread_t thread{};
	std::mutex mutex;
	std::condition_variable resumed;
	bool running = false;
};

struct ThreadHandle
{
	std::shared_ptr<ThreadState> state;
};

std::atomic<unsigned int> s_nextThreadId{1};

// The calling thread's handle (also for threads we did not create, e.g. the main thread).
thread_local ThreadHandle t_current;

ThreadHandle& Current()
{
	if (t_current.state == nullptr)
	{
		t_current.state = std::make_shared<ThreadState>();
		t_current.state->id = s_nextThreadId++;
		t_current.state->thread = pthread_self();
		t_current.state->running = true;
	}
	return t_current;
}

void* Start(void* argument)
{
	auto* start = static_cast<std::shared_ptr<ThreadState>*>(argument);
	t_current.state = std::move(*start);
	delete start;
	ThreadState& state = *t_current.state;
	{
		std::unique_lock lock(state.mutex);
		state.resumed.wait(lock, [&] { return state.running; });
	}
	state.entry(state.argument);
	return nullptr;
}

} // namespace

void* ThreadCreate(unsigned int stackSize, ThreadEntry entry, void* argument, bool suspended, unsigned int& id)
{
	auto* handle = new ThreadHandle{std::make_shared<ThreadState>()};
	ThreadState& state = *handle->state;
	state.entry = entry;
	state.argument = argument;
	state.id = s_nextThreadId++;
	state.running = !suspended;
	id = state.id;

	pthread_attr_t attributes;
	pthread_attr_init(&attributes);
	if (stackSize != 0)
		pthread_attr_setstacksize(&attributes, stackSize);
	auto* start = new std::shared_ptr<ThreadState>(handle->state);
	const int result = pthread_create(&state.thread, &attributes, Start, start);
	pthread_attr_destroy(&attributes);
	if (result != 0)
	{
		delete start;
		delete handle;
		id = 0;
		return nullptr;
	}
	pthread_detach(state.thread);
	return handle;
}

// POSIX cannot end another thread safely; it keeps running.
void ThreadTerminate(void*)
{
}

void ThreadClose(void* thread)
{
	if (thread != &t_current)
		delete static_cast<ThreadHandle*>(thread);
}

bool ThreadSuspend(void*)
{
	return false;
}

bool ThreadResume(void* thread)
{
	ThreadState& state = *static_cast<ThreadHandle*>(thread)->state;
	{
		std::lock_guard lock(state.mutex);
		state.running = true;
	}
	state.resumed.notify_one();
	return true;
}

// Priorities are recorded but not applied: raising them needs privileges on POSIX systems.
int ThreadGetPriority(void* thread)
{
	return static_cast<ThreadHandle*>(thread)->state->priority;
}

void ThreadSetPriority(void* thread, int priority)
{
	static_cast<ThreadHandle*>(thread)->state->priority = priority;
}

void* ThreadCurrent()
{
	return &Current();
}

unsigned int ThreadCurrentId()
{
	return Current().state->id;
}

void ThreadYield()
{
	sched_yield();
}

void ThreadExit()
{
	pthread_exit(nullptr);
}

} // namespace jpog::platform
