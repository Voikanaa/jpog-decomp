#pragma once

#include "toshi/kernel/TScheduler.h"

// The game's render task (SimJP.exe), under AApplication's render task. Placeholder: it only
// pumps the window and presents empty frames; the original renders the world, GUI and movies
// (issue #21).
//
// Deviation: closing the window ends the game here. In the original, WM_CLOSE only calls
// TRenderD3DInterface::Exit (Update then returns false, which ARenderer::OnUpdate ignores); the
// game quits through its menu (AFrontEndMainMenuState::OnQuitResponse -> TApplication::Destroy).
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
