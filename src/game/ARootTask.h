#pragma once

#include "toshi/kernel/TScheduler.h"
#include "toshi/kernel/containers/TManagedPtr.h"
#include "toshi/render/TRenderInterface.h"

class AOptions;

// The game's root task (SimJP.exe), a child of AApplication's Update2 task. Its constructor
// allocates the game's systems and OnCreate brings them up.
//
// So far the options, the renderer and ARenderer: the original OnCreate also creates the
// animation, input, GUI, game state, mission, sound and camera systems, loads the material
// libraries, the movie player and the front end (issue #21). The layout is not the original's.
class ARootTask : public Toshi::TTask
{
public:
	ARootTask();
	~ARootTask() override;

	Toshi::TClass& GetClass() const override;
	Toshi::TManagedPtr<Toshi::TRenderInterface> GetRenderInterface();
	AOptions* GetOptions();

	// Copy the graphics settings from the options to AOptionLogic and back. Quirk: the brightness
	// is read as "ScreenBrightess" and written as "ScreenBrightness", and the windowed flag is
	// only read, as in the original.
	static void __stdcall DeserialiseOptions();
	static void __stdcall SerialiseOptions();

	static Toshi::TClass m_sClass;

protected:
	bool OnCreate() override;
	bool OnUpdate(float deltaTime) override;

private:
	void AllocateRenderInterface();
	void AllocateARenderer();
	// Opens the display in the options' mode (640x480x32 fullscreen by default), falling back to
	// 16-bit colour.
	bool CreateRenderInterface();
	// The best device for the mode, trying the depth/stencil formats in the game's order.
	const Toshi::TRenderAdapter::Mode::Device* CreateDisplayDevice(Toshi::TRenderInterface::DisplayParams& params,
	                                                               bool bestDepth);
	void CreateARenderer();

	static Toshi::TObject* __stdcall CreateObject();
	static Toshi::TObject* __stdcall CreateObjectInPlace(void* memory);
	static void __stdcall InitialiseStatic();
	static void __stdcall DeinitialiseStatic();

	AOptions* m_pOptions = nullptr;                                 // original +0x24
	Toshi::TManagedPtr<Toshi::TRenderInterface> m_pRenderInterface; // original +0xD0
	Toshi::TTask* m_pARenderer = nullptr;                           // original +0xD4
};
