#pragma once

#include "common/abi.h"
#include "toshi/kernel/TScheduler.h"

class AOptions;

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

	// Store the display and the graphics settings in the options and back. Quirk: the
	// "ScreenResolution" option goes through AOptionLogic as a quality level, so only the width
	// is saved, and loading it sets the resolution to that width with no height or depth.
	bool SerialiseOptions(AOptions& options);
	bool DeserialiseOptions(AOptions& options);

	static Toshi::TClass m_sClass;

	float m_fFarClip = 0.0f; // original +0x38, set by ARootTask::ResetOptions

protected:
	bool OnUpdate(float deltaTime) override;

private:
	static Toshi::TObject* TSTDCALL CreateObject();
	static Toshi::TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();
};
