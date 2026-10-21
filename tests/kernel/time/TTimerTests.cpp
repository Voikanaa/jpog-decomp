#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TEvent.h"
#include "toshi/kernel/TProfiler.h"
#include "toshi/kernel/time/TTimer.h"

#include <cstring>
#include <doctest/doctest.h>
#include <string>
#include <vector>

using Toshi::TGenericEmitter;
using Toshi::TGenericListener;
using Toshi::TProfiler;
using Toshi::TProfilerStarter;
using Toshi::TTimer;
using namespace jpog::test;

namespace
{

// Exposes the timer's update hook and emitter.
struct TTestTimer : TTimer
{
	using TTimer::OnUpdate;
	TGenericEmitter* Emitter() { return &m_oEmitter; }
};

struct TTestListener : TGenericListener
{
	using TGenericListener::Connect;
	using TGenericListener::Disconnect;
};

// Received trigger events as text ("2 3 0 1.5").
std::vector<std::string> s_events;

bool TSTDCALL RecordEvent(void*, void*, void* event)
{
	const auto& trigger = *static_cast<const TTimer::TriggerEvent*>(event);
	s_events.push_back(std::to_string(trigger.m_iMode) + " " + std::to_string(trigger.m_iLoops) + " " +
	                   std::to_string(trigger.m_fWaitTime) + " " + std::to_string(trigger.m_fTime));
	return true;
}

// The timer fields after the emitter, the same for both sides (x86 layout).
struct TimerView
{
	int mode, loops, updates;
	float waitTime, time;
	bool operator==(const TimerView&) const = default;
};

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TTimer")
{
	TEST_CASE("modes trigger their events")
	{
		TTestTimer timer;
		TTestListener listener;
		listener.Connect(timer.Emitter(), nullptr, RecordEvent, 0);
		s_events.clear();

		timer.OnUpdate(0.5f); // stopped: time runs, no event
		CHECK(s_events.empty());

		timer.SetModeOnce();
		CHECK(timer.OnUpdate(0.25f));
		REQUIRE(s_events.size() == 1);
		CHECK(s_events.back() == "1 0 0.000000 0.250000");

		timer.SetModeLoop(2);
		s_events.clear();
		timer.OnUpdate(1.0f);
		timer.OnUpdate(1.0f);
		CHECK(s_events.empty());
		timer.OnUpdate(1.0f);
		REQUIRE(s_events.size() == 1);
		CHECK(s_events.back() == "2 2 0.000000 3.000000");

		timer.SetModeWaitFor(1.5f);
		s_events.clear();
		timer.OnUpdate(1.0f);
		CHECK(s_events.empty());
		timer.OnUpdate(0.5f);
		timer.OnUpdate(0.5f);
		CHECK(s_events.size() == 2);
		CHECK(s_events.front() == "3 0 1.500000 1.500000");

		timer.SetModeContinuous();
		s_events.clear();
		timer.OnUpdate(0.5f);
		CHECK(s_events.back() == "4 0 0.000000 0.500000");
		listener.Disconnect();
	}
}

TEST_SUITE("TProfiler")
{
	TEST_CASE("profilers form a chain with ids")
	{
		static TProfiler first(const_cast<char*>("First"), nullptr, 1);
		static TProfiler second(const_cast<char*>("Second"), const_cast<char*>(__FILE__), __LINE__);
		CHECK(TProfiler::GetProfiler(second.m_sCurrentID) == &second);
		CHECK(TProfiler::GetProfiler(second.m_sCurrentID - 1) == &first);
		CHECK(TProfiler::GetProfiler(-5) == nullptr);
	}

	TEST_CASE("starters record the parent from the second nesting on")
	{
		static TProfiler outer(const_cast<char*>("Outer"), nullptr, 0);
		static TProfiler inner(const_cast<char*>("Inner"), nullptr, 0);
		const int outerID = TProfiler::GetProfiler(TProfiler::m_sCurrentID) == &inner ? TProfiler::m_sCurrentID - 1
		                                                                              : TProfiler::m_sCurrentID;
		REQUIRE(TProfiler::GetProfiler(outerID) == &outer);

		{
			TProfilerStarter stopped(&inner); // the profiler is stopped: nothing is recorded
		}
		outer.Start();
		CHECK(outer.IsRunning());
		for (int pass = 0; pass < 2; ++pass)
		{
			// The first nesting only marks the profiler as having a parent.
			CHECK(TProfiler::GetProfilerWithParentID(outerID) == nullptr);
			TProfilerStarter outerScope(&outer);
			TProfilerStarter innerScope(&inner);
		}
		CHECK(TProfiler::GetProfilerWithParentID(outerID) == &inner);
		outer.Stop();
		CHECK_FALSE(outer.IsRunning());
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

TEST_SUITE("TTimer differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("modes and updates match the original")
	{
		using P = void*;
		const auto construct = Original<Method<void>>("??0TTimer@Toshi@@QAE@XZ");
		const auto destruct = Original<Method<void>>("??1TTimer@Toshi@@UAE@XZ");
		const auto onUpdate = Original<Method<bool, float>>("?OnUpdate@TTimer@Toshi@@MAE_NM@Z");
		const auto stopped = Original<Method<void>>("?SetModeStopped@TTimer@Toshi@@QAEXXZ");
		const auto once = Original<Method<void>>("?SetModeOnce@TTimer@Toshi@@QAEXXZ");
		const auto loop = Original<Method<void, int>>("?SetModeLoop@TTimer@Toshi@@QAEXH@Z");
		const auto waitFor = Original<Method<void, float>>("?SetModeWaitFor@TTimer@Toshi@@QAEXM@Z");
		const auto continuous = Original<Method<void>>("?SetModeContinuous@TTimer@Toshi@@QAEXXZ");

		alignas(void*) unsigned char original[sizeof(TTimer)];
		construct(original);
		TTestTimer ours;
		void* emitters[2] = {original + 0x24, ours.Emitter()};
		TTestListener listeners[2];
		for (int side = 0; side < 2; ++side)
			listeners[side].Connect(static_cast<TGenericEmitter*>(emitters[side]), nullptr, RecordEvent, 0);
		const auto view = [](const void* timer)
		{
			TimerView result;
			std::memcpy(&result, static_cast<const unsigned char*>(timer) + 0x30, sizeof(result));
			return result;
		};
		CHECK(view(original) == view(&ours));

		InputGenerator gen;
		GamePrecisionScope precision;
		for (int step = 0; step < 2000; ++step)
		{
			INFO("step ", step);
			const int op = static_cast<int>(gen.NextU32() % 8);
			const int loops = gen.NextInt(-1, 6);
			const float value = gen.NextInt(-4, 40) * 0.0625f + gen.NextFloat(0.0f, 0.01f);
			std::vector<std::string> events[2];
			for (int side = 0; side < 2; ++side)
			{
				s_events.clear();
				P timer = side == 0 ? static_cast<P>(original) : static_cast<P>(&ours);
				switch (op)
				{
				case 0:
					side == 0 ? stopped(timer) : ours.SetModeStopped();
					break;
				case 1:
					side == 0 ? once(timer) : ours.SetModeOnce();
					break;
				case 2:
					side == 0 ? loop(timer, loops) : ours.SetModeLoop(loops);
					break;
				case 3:
					side == 0 ? waitFor(timer, value) : ours.SetModeWaitFor(value);
					break;
				case 4:
					side == 0 ? continuous(timer) : ours.SetModeContinuous();
					break;
				default:
					CHECK((side == 0 ? onUpdate(timer, value) : ours.OnUpdate(value)));
					break;
				}
				events[side] = s_events;
			}
			CHECK(events[0] == events[1]);
			REQUIRE(view(original) == view(&ours));
		}
		for (TTestListener& listener : listeners)
			listener.Disconnect();
		destruct(original);
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
