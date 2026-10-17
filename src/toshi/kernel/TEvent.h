#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "toshi/kernel/TGenericPriList.h"

namespace Toshi
{

class TGenericListener;

// Event source (TKernelInterface.dll): a priority list of listeners plus the owner passed to
// every callback.
class TGenericEmitter : public TGenericPriList
{
	friend class TGenericListener;

public:
	TGenericEmitter(const TGenericEmitter& other);
	TGenericEmitter& operator=(const TGenericEmitter& other);

protected:
	explicit TGenericEmitter(void* owner);
	// Disconnects every listener.
	~TGenericEmitter();
	void Create(void* owner);
	void Destroy();
	// Calls the listeners in priority order until one returns false.
	void Throw(void* event);

private:
	void* m_pOwner;
};

JPOG_ASSERT_SIZE(TGenericEmitter, 12);

// Event sink: a priority list node holding a receiver and its callback.
class TGenericListener : public TGenericPriList::TNode
{
public:
	using Callback = bool(TSTDCALL*)(void* receiver, void* owner, void* event);

	TGenericListener& operator=(const TGenericListener& other);

protected:
	TGenericListener();
	TGenericListener(TGenericEmitter* emitter, void* receiver, Callback callback, int priority);
	~TGenericListener();
	void Connect(TGenericEmitter* emitter, void* receiver, Callback callback, int priority);
	// Unlinks and clears the receiver and callback (the priority is kept).
	void Disconnect();

private:
	friend class TGenericEmitter;

	void* m_pReceiver;
	Callback m_fnCallback;
};

JPOG_ASSERT_SIZE(TGenericListener, 0x14);

// Typed emitter; the original defines it inline.
template <class Owner, class Event> class TEmitter : public TGenericEmitter
{
public:
	explicit TEmitter(Owner* owner) : TGenericEmitter(owner) {}
	void Throw(Event* event) { TGenericEmitter::Throw(event); }
};

// Event type without data.
class TNullEvent
{
public:
	TNullEvent& operator=(const TNullEvent& other);
};

} // namespace Toshi
