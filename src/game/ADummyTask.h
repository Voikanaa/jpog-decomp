#pragma once

#include "common/abi.h"
#include "toshi/kernel/TScheduler.h"
#include "toshi/kernel/strings/TCString.h"

// A named task with no behaviour of its own (SimJP.exe): AApplication groups the game's tasks
// under five of these (render, three update stages, input).
class ADummyTask : public Toshi::TTask
{
public:
	ADummyTask();

	Toshi::TClass& GetClass() const override;
	void SetName(char* name);

	static Toshi::TClass m_sClass;

protected:
	bool OnUpdate(float deltaTime) override;

private:
	static Toshi::TObject* TSTDCALL CreateObject();
	static Toshi::TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	Toshi::TCString m_oName;
};
