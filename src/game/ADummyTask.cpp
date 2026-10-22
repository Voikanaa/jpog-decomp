#include "game/ADummyTask.h"

#include <new>

using namespace Toshi;

TClass ADummyTask::m_sClass("ADummyTask", &TTask::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                            DeinitialiseStatic, 0x10000);

ADummyTask::ADummyTask()
{
}

TClass& ADummyTask::GetClass() const
{
	return m_sClass;
}

void ADummyTask::SetName(char* name)
{
	m_oName = name;
}

bool ADummyTask::OnUpdate(float deltaTime)
{
	return TTask::OnUpdate(deltaTime);
}

TObject* TSTDCALL ADummyTask::CreateObject()
{
	return new ADummyTask();
}

TObject* TSTDCALL ADummyTask::CreateObjectInPlace(void* memory)
{
	return memory == nullptr ? nullptr : new (memory) ADummyTask();
}

void TSTDCALL ADummyTask::InitialiseStatic()
{
}

void TSTDCALL ADummyTask::DeinitialiseStatic()
{
}
