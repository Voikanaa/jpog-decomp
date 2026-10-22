#pragma once

#include "common/abi.h"
#include "toshi/kernel/TScheduler.h"
#include "toshi/kernel/containers/TManagedPtr.h"
#include "toshi/render/TRenderInterface.h"

class AOptions;
class ARenderer;

// The game's root task (SimJP.exe), a child of AApplication's Update2 task. Its constructor
// allocates the game's systems and OnCreate brings them up.
//
// So far the options, the constants, the renderer (with its material library manager) and
// ARenderer: the original OnCreate also initialises the APrim statics, creates the animation,
// input, GUI, game state, mission, sound and camera systems, loads the material libraries, the
// movie player and the front end (issue #21). The layout is not the original's.
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
	static void TSTDCALL DeserialiseOptions();
	static void TSTDCALL SerialiseOptions();
	// Applies the graphics options: far clip and fade distances (DrawDistance), terrain level of
	// detail (TerrainDetail), water (TerrainDetail, high with WaterReflection) and sky
	// (TerrainQuality) settings, particle detail (Effects) and the weather's scattering offset.
	static void TSTDCALL ResetOptions();

	static float GetFarClip() { return sm_fFarClip; }
	static void SetFarClip(float farClip) { sm_fFarClip = farClip; }
	static void TSTDCALL GetWaterSettings(int& segments, int& rings);
	static void TSTDCALL SetWaterSettings(int segments, int rings);
	static void TSTDCALL GetSkySettings(int& segments, int& slices, float& height);
	static void TSTDCALL SetSkySettings(int segments, int slices, float height);

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
	void InitialiseConstants();
	// Passes the water, sky and fade settings to the park's terrain (none yet) and turns light
	// scattering off when the terrain shader is not the high-end one.
	void InitTerrain();
	void CreateARenderer();

	static Toshi::TObject* TSTDCALL CreateObject();
	static Toshi::TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	static float sm_fFarClip;
	static int sm_iSkySegs;
	static int sm_iSkySlices;
	static float sm_fSkyHeight;
	static int sm_iWaterSegs;
	static int sm_iWaterRings;

	AOptions* m_pOptions = nullptr;                                 // original +0x24
	Toshi::TManagedPtr<Toshi::TRenderInterface> m_pRenderInterface; // original +0xD0
	ARenderer* m_pARenderer = nullptr;                              // original +0xD4
};
