#pragma once

#include "common/layout.h"
#include "toshi/kernel/TEvent.h"
#include "toshi/kernel/TScheduler.h"

namespace Toshi
{

// A task that counts time and notifies listeners (TKernelInterface.dll). Modes: once (every
// update), loop (after the given number of updates), wait for (once the time is reached, then
// every update) and continuous (every update).
class TTimer : public TTask
{
public:
	enum Mode
	{
		MODE_STOPPED,
		MODE_ONCE,
		MODE_LOOP,
		MODE_WAITFOR,
		MODE_CONTINUOUS,
	};

	class TriggerEvent
	{
	public:
		TriggerEvent(int mode, int loops, float waitTime, float time);
		TriggerEvent& operator=(const TriggerEvent& other);

		int m_iMode;
		int m_iLoops;
		float m_fWaitTime;
		float m_fTime; // since the mode was set
	};

	TTimer();
	~TTimer() override;

	TClass& GetClass() const override;

	void SetModeStopped();
	void SetModeOnce();
	void SetModeLoop(int updates);
	void SetModeWaitFor(float time);
	void SetModeContinuous();

	static TClass m_sClass;

protected:
	bool OnUpdate(float deltaTime) override;
	void OnDestroy() override;

private:
	static TObject* TSTDCALL CreateObject();
	static TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

protected:
	TEmitter<TTimer, TriggerEvent> m_oEmitter;

private:
	int m_iMode;
	int m_iLoops;
	int m_iUpdates;
	float m_fWaitTime;
	float m_fTime;
};

JPOG_ASSERT_SIZE(TTimer, 0x44);

} // namespace Toshi
