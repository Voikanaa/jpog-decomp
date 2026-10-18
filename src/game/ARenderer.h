#pragma once

#include "toshi/kernel/TScheduler.h"

// The game's render task (SimJP.exe), under AApplication's render task. Placeholder: it only
// pumps the window and presents empty frames; the original renders the world, GUI and movies
// (issue #21). Ending the game when the window closes is ours too: the original's path for that
// is not traced yet.
class ARenderer : public Toshi::TTask
{
public:
	Toshi::TClass& GetClass() const override;

	static Toshi::TClass m_sClass;

protected:
	bool OnUpdate(float deltaTime) override;

private:
	static Toshi::TObject* __stdcall CreateObject();
	static Toshi::TObject* __stdcall CreateObjectInPlace(void* memory);
	static void __stdcall InitialiseStatic();
	static void __stdcall DeinitialiseStatic();
};
