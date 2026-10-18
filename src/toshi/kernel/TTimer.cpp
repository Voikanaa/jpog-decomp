#include "toshi/kernel/TTimer.h"

#include <new>

namespace Toshi
{

TClass TTimer::m_sClass("TTimer", &TTask::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                        DeinitialiseStatic, 0x10000);

TTimer::TriggerEvent::TriggerEvent(int mode, int loops, float waitTime, float time)
    : m_iMode(mode), m_iLoops(loops), m_fWaitTime(waitTime), m_fTime(time)
{
}

TTimer::TriggerEvent& TTimer::TriggerEvent::operator=(const TriggerEvent& other) = default;

TTimer::TTimer() : m_oEmitter(this), m_iMode(0), m_iLoops(0), m_iUpdates(0), m_fWaitTime(0.0f), m_fTime(0.0f)
{
	SetModeStopped();
}

TTimer::~TTimer()
{
}

TClass& TTimer::GetClass() const
{
	return m_sClass;
}

void TTimer::SetModeStopped()
{
	m_iMode = MODE_STOPPED;
	m_fTime = 0.0f;
}

void TTimer::SetModeOnce()
{
	m_iMode = MODE_ONCE;
	m_fTime = 0.0f;
}

void TTimer::SetModeLoop(int updates)
{
	m_iMode = MODE_LOOP;
	m_fTime = 0.0f;
	m_iUpdates = 0;
	m_iLoops = updates;
}

void TTimer::SetModeWaitFor(float time)
{
	m_iMode = MODE_WAITFOR;
	m_fTime = 0.0f;
	m_fWaitTime = time;
}

void TTimer::SetModeContinuous()
{
	m_iMode = MODE_CONTINUOUS;
	m_fTime = 0.0f;
}

// The time also runs while stopped.
bool TTimer::OnUpdate(float deltaTime)
{
	m_fTime = deltaTime + m_fTime;
	switch (m_iMode)
	{
	case MODE_ONCE:
	case MODE_CONTINUOUS:
	{
		TriggerEvent event(m_iMode, 0, 0.0f, m_fTime);
		m_oEmitter.Throw(&event);
		break;
	}
	case MODE_LOOP:
		if (m_iLoops < ++m_iUpdates)
		{
			TriggerEvent event(m_iMode, m_iLoops, 0.0f, m_fTime);
			m_oEmitter.Throw(&event);
		}
		break;
	case MODE_WAITFOR:
		if (m_fWaitTime <= m_fTime)
		{
			TriggerEvent event(m_iMode, 0, m_fWaitTime, m_fTime);
			m_oEmitter.Throw(&event);
		}
		break;
	default:
		break;
	}
	return true;
}

void TTimer::OnDestroy()
{
}

TObject* TSTDCALL TTimer::CreateObject()
{
	return new TTimer();
}

TObject* TSTDCALL TTimer::CreateObjectInPlace(void* memory)
{
	return memory == nullptr ? nullptr : new (memory) TTimer();
}

void TSTDCALL TTimer::InitialiseStatic()
{
}

void TSTDCALL TTimer::DeinitialiseStatic()
{
}

} // namespace Toshi
