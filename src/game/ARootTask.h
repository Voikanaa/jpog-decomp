#pragma once

#include "common/abi.h"
#include "toshi/kernel/TScheduler.h"
#include "toshi/kernel/containers/TManagedPtr.h"
#include "toshi/render/TRenderInterface.h"

class AOptions;
class ARenderer;
class AParticleSystem;

// The game's root task (SimJP.exe), a child of AApplication's Update2 task. Its constructor
// allocates the game's systems and OnCreate brings them up.
//
// So far the options, the constants, the renderer (with its material library manager), ARenderer
// and the subsystem tasks in the original order (placeholders, ASystemTasks.h): the original
// OnCreate also initialises the APrim statics, loads the material libraries, the movie player
// and the front end (issue #21). The layout is not the original's.
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
	void AllocateCameraManager();
	void AllocateGUISystem();
	void AllocateAnimationSystem();
	void AllocateInputSystem();
	void AllocateGameStateController();
	void AllocateMissionController();
	void AllocateSoundSystem();
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
	void CreateAnimationSystem();
	// Creates the input handler and the vibration manager, which then starts inactive.
	void CreateInputSystem();
	// Drops the GUI system if it cannot be created.
	void CreateGUISystem();
	void CreateGameStateController();
	void CreateMissionController();
	void CreateSoundSystem();
	void CreateCameraManager();

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
	Toshi::TTask* m_pAnimationSystem = nullptr;                     // original +0x30
	Toshi::TTask* m_pGUISystem = nullptr;                           // original +0x34
	Toshi::TTask* m_pInputHandler = nullptr;                        // original +0x38
	Toshi::TTask* m_pRootStateController = nullptr;                 // original +0xD8
	Toshi::TTask* m_pSoundManager = nullptr;                        // original +0xDC
	Toshi::TTask* m_pMissionController = nullptr;                   // original +0xE0
	Toshi::TTask* m_pVibrationManager = nullptr;                    // original +0xE8
	Toshi::TTask* m_pCameraManager = nullptr;                       // original +0xEC
	AParticleSystem* m_pParticleSystem = nullptr;                   // original +0xF4
};
