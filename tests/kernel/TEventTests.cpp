#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TEvent.h"

#include <doctest/doctest.h>
#include <vector>

using Toshi::TGenericEmitter;
using Toshi::TGenericListener;
using namespace jpog::test;

namespace
{

struct TTestEmitter : TGenericEmitter
{
	explicit TTestEmitter(void* owner) : TGenericEmitter(owner) {}
	using TGenericEmitter::Throw;
};

struct TTestListener : TGenericListener
{
	using TGenericListener::Connect;
	using TGenericListener::Disconnect;
};

struct Receiver
{
	int id;
	bool stop;       // return false from the callback
	bool disconnect; // disconnect the own listener from inside the callback
	void (*disconnectFn)(void* listener);
	void* listener;
};

struct Call
{
	int id;
	void* owner;
	void* event;
	bool operator==(const Call&) const = default;
};
std::vector<Call> s_calls;

bool TSTDCALL Record(void* receiver, void* owner, void* event)
{
	auto* r = static_cast<Receiver*>(receiver);
	s_calls.push_back({r->id, owner, event});
	if (r->disconnect)
		r->disconnectFn(r->listener);
	return !r->stop;
}

void OurDisconnect(void* listener)
{
	static_cast<TTestListener*>(listener)->Disconnect();
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TEvent")
{
	TEST_CASE("listeners run in priority order until one stops")
	{
		int owner = 0, event = 0;
		TTestEmitter emitter(&owner);
		TTestListener listeners[3];
		Receiver receivers[3] = {{0, false, false, nullptr, nullptr},
		                         {1, true, false, nullptr, nullptr},
		                         {2, false, false, nullptr, nullptr}};
		listeners[0].Connect(&emitter, &receivers[0], Record, 5);
		listeners[1].Connect(&emitter, &receivers[1], Record, 1);
		listeners[2].Connect(&emitter, &receivers[2], Record, 0);
		s_calls.clear();
		emitter.Throw(&event);
		CHECK(s_calls == std::vector<Call>{{2, &owner, &event}, {1, &owner, &event}}); // 1 stops the chain
		listeners[1].Disconnect();
		s_calls.clear();
		emitter.Throw(nullptr);
		CHECK(s_calls == std::vector<Call>{{2, &owner, nullptr}, {0, &owner, nullptr}});
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

using Callback = TGenericListener::Callback;
Method<void> s_originalDisconnect;

void OriginalDisconnect(void* listener)
{
	s_originalDisconnect(listener);
}

} // namespace

TEST_SUITE("TEvent differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("emitters and listeners match the original")
	{
		const auto emitterCtor = Original<Method<void, void*>>("??0TGenericEmitter@Toshi@@IAE@PAX@Z");
		const auto emitterDtor = Original<Method<void>>("??1TGenericEmitter@Toshi@@IAE@XZ");
		const auto doThrow = Original<Method<void, void*>>("?Throw@TGenericEmitter@Toshi@@IAEXPAX@Z");
		const auto listenerCtor = Original<Method<void>>("??0TGenericListener@Toshi@@IAE@XZ");
		const auto listenerDtor = Original<Method<void>>("??1TGenericListener@Toshi@@IAE@XZ");
		const auto connect = Original<Method<void, void*, void*, Callback, int>>(
		    "?Connect@TGenericListener@Toshi@@IAEXPAVTGenericEmitter@2@PAXP6G_N111@ZH@Z");
		s_originalDisconnect = Original<Method<void>>("?Disconnect@TGenericListener@Toshi@@IAEXXZ");

		constexpr int kCount = 8;
		InputGenerator gen;
		int owner = 0, event = 0;
		for (int round = 0; round < 200; ++round)
		{
			alignas(void*) unsigned char originalEmitter[sizeof(TGenericEmitter)];
			alignas(void*) unsigned char originalListeners[kCount][sizeof(TGenericListener)];
			emitterCtor(originalEmitter, &owner);
			auto* ourEmitter = new TTestEmitter(&owner);
			TTestListener ourListeners[kCount];
			Receiver originalReceivers[kCount], ourReceivers[kCount];
			bool connected[kCount] = {};
			for (int i = 0; i < kCount; ++i)
			{
				listenerCtor(originalListeners[i]);
				originalReceivers[i] = {i, false, false, OriginalDisconnect, originalListeners[i]};
				ourReceivers[i] = {i, false, false, OurDisconnect, &ourListeners[i]};
			}
			for (int step = 0; step < 40; ++step)
			{
				INFO("round ", round, " step ", step);
				const int which = gen.NextInt(0, kCount - 1);
				switch (gen.NextU32() % 4)
				{
				case 0:
				case 1:
					if (!connected[which])
					{
						const int priority = gen.NextInt(-3, 3);
						connect(originalListeners[which], originalEmitter, &originalReceivers[which], Record, priority);
						ourListeners[which].Connect(ourEmitter, &ourReceivers[which], Record, priority);
						connected[which] = true;
					}
					break;
				case 2:
					s_originalDisconnect(originalListeners[which]);
					ourListeners[which].Disconnect();
					connected[which] = false;
					break;
				default:
				{
					for (int i = 0; i < kCount; ++i)
					{
						const bool stop = gen.NextU32() % 6 == 0, disconnect = gen.NextU32() % 8 == 0;
						originalReceivers[i].stop = ourReceivers[i].stop = stop;
						originalReceivers[i].disconnect = ourReceivers[i].disconnect = disconnect;
					}
					s_calls.clear();
					doThrow(originalEmitter, &event);
					const auto originalCalls = s_calls;
					s_calls.clear();
					ourEmitter->Throw(&event);
					CHECK(originalCalls == s_calls);
					for (const auto& call : s_calls)
						if (ourReceivers[call.id].disconnect)
							connected[call.id] = false;
					break;
				}
				}
			}
			for (int i = 0; i < kCount; ++i)
				listenerDtor(originalListeners[i]);
			emitterDtor(originalEmitter);
			delete ourEmitter;
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
