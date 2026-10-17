#include "toshi/kernel/TEvent.h"

namespace Toshi
{

// --- TGenericEmitter -----------------------------------------------------------------------

TGenericEmitter::TGenericEmitter(void* owner)
{
	Create(owner);
}

// Copies the raw links, as the original.
TGenericEmitter::TGenericEmitter(const TGenericEmitter& other) : TGenericPriList(), m_pOwner(other.m_pOwner)
{
	TGenericPriList::operator=(other);
}

TGenericEmitter& TGenericEmitter::operator=(const TGenericEmitter& other)
{
	TGenericPriList::operator=(other);
	m_pOwner = other.m_pOwner;
	return *this;
}

TGenericEmitter::~TGenericEmitter()
{
	Destroy();
}

void TGenericEmitter::Create(void* owner)
{
	m_pOwner = owner;
}

void TGenericEmitter::Destroy()
{
	while (!IsEmpty())
		static_cast<TGenericListener*>(Head())->Disconnect();
	m_pOwner = nullptr;
}

void TGenericEmitter::Throw(void* event)
{
	const TNode* end = End();
	for (TNode* node = Begin(); node != end;)
	{
		auto* listener = static_cast<TGenericListener*>(node);
		node = listener->Next(); // the callback may disconnect its listener
		if (!listener->m_fnCallback(listener->m_pReceiver, m_pOwner, event))
			return;
	}
}

// --- TGenericListener ----------------------------------------------------------------------

TGenericListener::TGenericListener() : m_pReceiver(nullptr), m_fnCallback(nullptr)
{
}

TGenericListener::TGenericListener(TGenericEmitter* emitter, void* receiver, Callback callback, int priority)
{
	Connect(emitter, receiver, callback, priority);
}

TGenericListener::~TGenericListener()
{
	Disconnect();
}

TGenericListener& TGenericListener::operator=(const TGenericListener& other)
{
	TNode::operator=(other);
	m_pReceiver = other.m_pReceiver;
	m_fnCallback = other.m_fnCallback;
	return *this;
}

void TGenericListener::Connect(TGenericEmitter* emitter, void* receiver, Callback callback, int priority)
{
	m_pReceiver = receiver;
	m_fnCallback = callback;
	SetPriority(priority);
	emitter->Insert(this);
}

void TGenericListener::Disconnect()
{
	Remove();
	m_pReceiver = nullptr;
	m_fnCallback = nullptr;
}

// --- TNullEvent ----------------------------------------------------------------------------

TNullEvent& TNullEvent::operator=(const TNullEvent&)
{
	return *this;
}

} // namespace Toshi
