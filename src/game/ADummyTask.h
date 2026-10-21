#pragma once

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
	static Toshi::TObject* __stdcall CreateObject();
	static Toshi::TObject* __stdcall CreateObjectInPlace(void* memory);
	static void __stdcall InitialiseStatic();
	static void __stdcall DeinitialiseStatic();

	Toshi::TCString m_oName;
};
