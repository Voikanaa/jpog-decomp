#include "game/ARootTask.h"

#include "game/AApplication.h"
#include "game/AConstants.h"
#include "game/ADummyTask.h"
#include "game/AFadeDist.h"
#include "game/AOptionLogic.h"
#include "game/AOptions.h"
#include "game/AParticleSystem.h"
#include "game/ARenderer.h"
#include "game/ASystemTasks.h"
#include "game/AWeatherManager.h"
#include "toshi/kernel/TKernelInterface.h"
#include "toshi/kernel/strings/TCString.h"
#include "toshi/render/TMaterialLibraryManager.h"
#include "toshi/render/TResource.h"
#include "toshi/terrain/TTerrainInterface.h"

#include <new>

using namespace Toshi;

TClass ARootTask::m_sClass("ARootTask", &TTask::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                           DeinitialiseStatic, 0x20000);

// The original's initial values (ResetOptions replaces them).
float ARootTask::sm_fFarClip = 0x1.541f9ap+8f; // 340.12344
int ARootTask::sm_iSkySegs = 20;
int ARootTask::sm_iSkySlices = 20;
float ARootTask::sm_fSkyHeight = 100.0f;
int ARootTask::sm_iWaterSegs = 24;
int ARootTask::sm_iWaterRings = 24;

ARootTask::ARootTask()
{
	m_pOptions = new AOptions();
	AllocateRenderInterface();
	AllocateCameraManager();
	AllocateGUISystem();
	AllocateARenderer();
	AllocateAnimationSystem();
	AllocateInputSystem();
	AllocateGameStateController();
	AllocateMissionController();
	AllocateSoundSystem();
	m_pParticleSystem = static_cast<AParticleSystem*>(
	    g_oTheApp.GetKernel()->GetScheduler()->CreateTask(AParticleSystem::m_sClass, this));
	// TODO(#21): the original also allocates AFeatureInfo and loads the Units interface, which
	// imports from SimJP.exe and so needs jpog.exe's exports first.
}

// The original only drops its renderer reference. Deviation: deleting the renderer cannot work
// (issue #20: its teardown frees every resource before DestroyDisplay uses the display texture,
// and DestroyDisplay cannot run twice), so as the last holder we keep it alive and leave the
// device and window to process exit. Fix this properly in our own TRenderD3DInterface.
ARootTask::~ARootTask()
{
	delete m_pOptions;
	if (m_pRenderInterface && m_pRenderInterface->GetRefCount() == 1)
		m_pRenderInterface->IncRefCount(nullptr); // deliberately never released
}

TClass& ARootTask::GetClass() const
{
	return m_sClass;
}

TManagedPtr<TRenderInterface> ARootTask::GetRenderInterface()
{
	return m_pRenderInterface;
}

AOptions* ARootTask::GetOptions()
{
	return m_pOptions;
}

bool ARootTask::OnCreate()
{
	AOptionLogic logic;
	logic.EnumerateModes();
	m_pOptions->LoadOptions();
	DeserialiseOptions();
	SerialiseOptions();
	InitialiseConstants();
	if (!CreateRenderInterface())
		return false;
	CreateARenderer();
	CreateAnimationSystem();
	CreateInputSystem();
	CreateGUISystem();
	CreateGameStateController();
	CreateMissionController();
	CreateSoundSystem();
	CreateCameraManager();
	// TODO(#21): the material libraries (7, 11 and 8) load here.
	m_pParticleSystem->Create();
	// The original then creates the tac map textures and the terrain shader, saves the options,
	// creates the movie player and applies the options.
	m_pOptions->SaveOptions();
	ResetOptions();
	return TTask::OnCreate();
}

namespace
{

struct QualityOption
{
	const char* name;
	AOptionLogic::OPTION option;
};

const QualityOption kQualityOptions[] = {
    {"TextureQuality", AOptionLogic::OPTION_TEXTUREQUALITY},
    {"TerrainDetail", AOptionLogic::OPTION_TERRAINDETAIL},
    {"TerrainQuality", AOptionLogic::OPTION_TERRAINQUALITY},
    {"ModelQuality", AOptionLogic::OPTION_MODELQUALITY},
    {"Effects", AOptionLogic::OPTION_EFFECTS},
    {"DrawDistance", AOptionLogic::OPTION_DRAWDISTANCE},
};

} // namespace

void TSTDCALL ARootTask::DeserialiseOptions()
{
	ARootTask* root = g_oTheApp.GetRootTask();
	// The original asks for the display parameters and ignores them.
	root->GetRenderInterface()->GetCurrentDisplayParams();
	AOptions* options = root->m_pOptions;
	AOptionLogic logic;

	int width;
	int height;
	int depth;
	if (options->GetOption("ScreenWidth", width) && options->GetOption("ScreenHeight", height) &&
	    options->GetOption("ScreenDepth", depth))
	{
		AOptionLogic::AScreenRes resolution;
		logic.GetOption(AOptionLogic::OPTION_SCREENRES, resolution);
		resolution.uiWidth = static_cast<unsigned int>(width);
		resolution.uiHeight = static_cast<unsigned int>(height);
		resolution.uiColourDepth = static_cast<unsigned int>(depth);
		logic.SetOption(AOptionLogic::OPTION_SCREENRES, resolution);
	}
	bool flag;
	if (options->GetOption("ScreenWindowed", flag))
		logic.SetOption(AOptionLogic::OPTION_WINDOWED, flag);
	for (const QualityOption& quality : kQualityOptions)
	{
		int value;
		if (options->GetOption(quality.name, value))
			logic.SetOption(quality.option, static_cast<AOptionLogic::LMH>(value));
	}
	if (options->GetOption("Grass", flag))
		logic.SetOption(AOptionLogic::OPTION_GRASS, flag);
	if (options->GetOption("WaterReflection", flag))
		logic.SetOption(AOptionLogic::OPTION_WATERREFLECTION, flag);
	float value;
	if (options->GetOption("ScreenBrightess", value))
		logic.SetOption(AOptionLogic::OPTION_BRIGHTNESS, value);
	if (options->GetOption("ScreenContrast", value))
		logic.SetOption(AOptionLogic::OPTION_CONTRAST, value);
	if (options->GetOption("ScreenGamma", value))
		logic.SetOption(AOptionLogic::OPTION_GAMMA, value);
}

void TSTDCALL ARootTask::SerialiseOptions()
{
	ARootTask* root = g_oTheApp.GetRootTask();
	root->GetRenderInterface()->GetCurrentDisplayParams();
	AOptions* options = root->m_pOptions;
	AOptionLogic logic;

	AOptionLogic::AScreenRes resolution;
	if (logic.GetOption(AOptionLogic::OPTION_SCREENRES, resolution))
	{
		options->PutOption("ScreenWidth", resolution.uiWidth);
		options->PutOption("ScreenHeight", resolution.uiHeight);
		options->PutOption("ScreenDepth", resolution.uiColourDepth);
	}
	for (const QualityOption& quality : kQualityOptions)
	{
		AOptionLogic::LMH value;
		logic.GetOption(quality.option, value);
		const int stored = value;
		options->PutOption(quality.name, stored);
	}
	bool flag;
	logic.GetOption(AOptionLogic::OPTION_GRASS, flag);
	options->PutOption("Grass", flag);
	logic.GetOption(AOptionLogic::OPTION_WATERREFLECTION, flag);
	options->PutOption("WaterReflection", flag);
	float value;
	logic.GetOption(AOptionLogic::OPTION_BRIGHTNESS, value);
	options->PutOption("ScreenBrightness", value);
	logic.GetOption(AOptionLogic::OPTION_CONTRAST, value);
	options->PutOption("ScreenContrast", value);
	logic.GetOption(AOptionLogic::OPTION_GAMMA, value);
	options->PutOption("ScreenGamma", value);
}

void TSTDCALL ARootTask::ResetOptions()
{
	using C = AConstantTypes;
	AOptions* options = g_oTheApp.GetRootTask()->m_pOptions;

	int drawDistance = 2;
	options->GetOption("DrawDistance", drawDistance);
	sm_fFarClip = AConstants::GetFloatConstant(drawDistance == 0   ? C::LOW_TerrainDrawDistance
	                                           : drawDistance == 1 ? C::MEDIUM_TerrainDrawDistance
	                                                               : C::HIGH_TerrainDrawDistance);
	AFadeDist::LoadFadeData(drawDistance);
	g_oTheApp.GetRootTask()->m_pARenderer->m_fFarClip = sm_fFarClip;

	int terrainDetail = 2;
	options->GetOption("TerrainDetail", terrainDetail);
	TTerrainInterface::SetLODDetail(terrainDetail == 0   ? TTerrainInterface::TERRAINLOD_LOW
	                                : terrainDetail == 1 ? TTerrainInterface::TERRAINLOD_MEDIUM
	                                                     : TTerrainInterface::TERRAINLOD_HIGH);

	// Water reflections take the high water settings; low and medium share the low ones. The
	// conversions truncate.
	bool reflection = false;
	options->GetOption("WaterReflection", reflection);
	const int water = reflection ? 2 : terrainDetail;
	if (water == 0 || water == 1)
		SetWaterSettings(static_cast<int>(AConstants::GetFloatConstant(C::LOW_WaterSegs)),
		                 static_cast<int>(AConstants::GetFloatConstant(C::LOW_WaterRings)));
	else
		SetWaterSettings(static_cast<int>(AConstants::GetFloatConstant(C::HIGH_WaterSegs)),
		                 static_cast<int>(AConstants::GetFloatConstant(C::HIGH_WaterRings)));

	int terrainQuality = 0;
	options->GetOption("TerrainQuality", terrainQuality);
	if (terrainQuality == 0 || terrainQuality == 1)
		SetSkySettings(static_cast<int>(AConstants::GetFloatConstant(C::LOW_SkySegs)),
		               static_cast<int>(AConstants::GetFloatConstant(C::LOW_SkySlices)),
		               AConstants::GetFloatConstant(C::LOW_SkyHeight));
	else
		SetSkySettings(static_cast<int>(AConstants::GetFloatConstant(C::HIGH_SkySegs)),
		               static_cast<int>(AConstants::GetFloatConstant(C::HIGH_SkySlices)),
		               AConstants::GetFloatConstant(C::HIGH_SkyHeight));
	g_oTheApp.GetRootTask()->InitTerrain();

	int effects = 2;
	options->GetOption("Effects", effects);
	AParticleSystem::SetDetail(effects == 0   ? AParticleSystem::PARTICLEDETAIL_LOW
	                           : effects == 1 ? AParticleSystem::PARTICLEDETAIL_MEDIUM
	                                          : AParticleSystem::PARTICLEDETAIL_HIGH);

	// Quirk: the low offset compares the low to the medium draw distance whatever they are.
	if (drawDistance == 0)
	{
		const float ratio = -(AConstants::GetFloatConstant(C::LOW_TerrainDrawDistance) /
		                      AConstants::GetFloatConstant(C::MEDIUM_TerrainDrawDistance)) +
		                    1.0f;
		AWeatherManager::SetScatteringNearFarOffset(ratio * 0.6f);
	}
	else
		AWeatherManager::SetScatteringNearFarOffset(0.0f);
	// TODO(#21): with a created game controller, the original also re-initialises the weather
	// and the tree and wall fade distances.
}

void TSTDCALL ARootTask::GetWaterSettings(int& segments, int& rings)
{
	segments = sm_iWaterSegs;
	rings = sm_iWaterRings;
}

void TSTDCALL ARootTask::SetWaterSettings(int segments, int rings)
{
	sm_iWaterSegs = segments;
	sm_iWaterRings = rings;
}

void TSTDCALL ARootTask::GetSkySettings(int& segments, int& slices, float& height)
{
	segments = sm_iSkySegs;
	slices = sm_iSkySlices;
	height = sm_fSkyHeight;
}

void TSTDCALL ARootTask::SetSkySettings(int segments, int slices, float height)
{
	sm_iSkySegs = segments;
	sm_iSkySlices = slices;
	sm_fSkyHeight = height;
}

bool ARootTask::OnUpdate(float)
{
	return true;
}

void ARootTask::AllocateRenderInterface()
{
	TKernelInterface* kernel = g_oTheApp.GetKernel();
	kernel->LoadInterface(TCString("TRenderD3DInterface"));
	const TClass* renderClass = TClass::Find("TRenderD3DInterface", nullptr);
	m_pRenderInterface = static_cast<TRenderInterface*>(renderClass->CreateObject());
	m_pRenderInterface->Create(kernel);
}

void ARootTask::AllocateARenderer()
{
	m_pARenderer = static_cast<ARenderer*>(
	    g_oTheApp.GetKernel()->GetScheduler()->CreateTask(ARenderer::m_sClass, g_oTheApp.GetRenderRootTask()));
}

void ARootTask::AllocateCameraManager()
{
	m_pCameraManager =
	    g_oTheApp.GetKernel()->GetScheduler()->CreateTask(ACameraManager::m_sClass, g_oTheApp.GetUpdate3RootTask());
}

void ARootTask::AllocateGUISystem()
{
	m_pGUISystem =
	    g_oTheApp.GetKernel()->GetScheduler()->CreateTask(AGUISystem::m_sClass, g_oTheApp.GetUpdate3RootTask());
}

void ARootTask::AllocateAnimationSystem()
{
	m_pAnimationSystem = g_oTheApp.GetKernel()->GetScheduler()->CreateTask(AToshiAnimationSystem::m_sClass, this);
}

void ARootTask::AllocateInputSystem()
{
	TScheduler* scheduler = g_oTheApp.GetKernel()->GetScheduler();
	m_pInputHandler = scheduler->CreateTask(AInputHandler::m_sClass, g_oTheApp.GetInputRootTask());
	m_pVibrationManager = scheduler->CreateTask(AVibrationManager::m_sClass, g_oTheApp.GetInputRootTask());
}

void ARootTask::AllocateGameStateController()
{
	m_pRootStateController = g_oTheApp.GetKernel()->GetScheduler()->CreateTask(ARootStateController::m_sClass, this);
}

void ARootTask::AllocateMissionController()
{
	m_pMissionController = g_oTheApp.GetKernel()->GetScheduler()->CreateTask(AMissionController::m_sClass, this);
}

void ARootTask::AllocateSoundSystem()
{
	m_pSoundManager = g_oTheApp.GetKernel()->GetScheduler()->CreateTask(ASoundManager::m_sClass, this);
}

bool ARootTask::CreateRenderInterface()
{
	int width = 640;
	int height = 480;
	int depth = 32;
	bool windowed = false;
	bool bestDepth = true;
	m_pOptions->GetOption("ScreenWidth", width);
	m_pOptions->GetOption("ScreenHeight", height);
	m_pOptions->GetOption("ScreenDepth", depth);
	m_pOptions->GetOption("ScreenBestZ", bestDepth);
	m_pOptions->GetOption("ScreenWindowed", windowed);
	TRenderInterface::DisplayParams params{};
	params.uiWidth = static_cast<unsigned int>(width);
	params.uiHeight = static_cast<unsigned int>(height);
	params.uiColourDepth = static_cast<unsigned int>(depth);
	params.eDepthStencilFormat = 0;
	params.bWindowed = windowed;

	const TRenderAdapter::Mode::Device* device = CreateDisplayDevice(params, bestDepth);
	if (device == nullptr && params.uiColourDepth == 32)
	{
		params.uiColourDepth = 16;
		const int fallback = 16;
		m_pOptions->PutOption("ScreenDepth", fallback);
		device = CreateDisplayDevice(params, bestDepth);
	}
	if (device == nullptr)
	{
		TRenderInterface::GetInterface()->FailedDisplayMessageBox();
		return false;
	}

	// The display result is not checked: only the report depends on it.
	if (m_pRenderInterface->CreateDisplay(&params))
	{
		TCString text;
		text.Format("--------------\n").Print();
		text.Format("Found Device [%s]:\n", device->GetTypeString()).Print();
		text.Format("  Width=%d\n", device->GetMode()->GetWidth()).Print();
		text.Format("  Height=%d\n", device->GetMode()->GetHeight()).Print();
		text.Format("  ColourDepth=Supports[%s], Using [%d]\n", device->GetMode()->Is32Bit() ? "32" : "16",
		            params.uiColourDepth)
		    .Print();
		text.Format("  SupportsHardwareTnL=%s\n", device->SupportsHardwareTransfomations() ? "Yes" : "No").Print();
		text.Format("  SupportsWindowed=%s, Using %s\n", device->CanRenderWindowed() ? "Yes" : "No",
		            params.bWindowed ? "Windowed" : "Fullscreen")
		    .Print();
		text.Format("--------------\n").Print();
	}
	m_pRenderInterface->GetMaterialLibraryManager()->Create();
	m_pARenderer->DeserialiseOptions(*m_pOptions);
	return true;
}

const TRenderAdapter::Mode::Device* ARootTask::CreateDisplayDevice(TRenderInterface::DisplayParams& params,
                                                                   bool bestDepth)
{
	// Depth/stencil formats in the order tried; `params` keeps the format of the device found.
	static const unsigned int s_aBest[] = {4, 2, 3, 0, 1};
	static const unsigned int s_aDefault[] = {0, 1, 2, 4, 3};
	TRenderInterface* renderer = TRenderInterface::GetInterface();
	for (const unsigned int format : bestDepth ? s_aBest : s_aDefault)
	{
		params.eDepthStencilFormat = format;
		if (const TRenderAdapter::Mode::Device* device = renderer->FindDevice(&params))
			return device;
	}
	return nullptr;
}

void ARootTask::InitialiseConstants()
{
	AConstants::Initialise();
}

void ARootTask::InitTerrain()
{
	// TODO(#21): with a created game controller, the original first passes the sky, water,
	// terrain fade and grass settings to the park's terrain.
	TRenderInterface* renderer = TRenderInterface::GetInterface();
	auto* terrain = static_cast<TTerrainShader*>(renderer->FindOrphanedResource("SHTERRAIN", nullptr));
	if (terrain == nullptr || terrain->IsHighEndTerrain())
		return;
	auto* skin = static_cast<TSkinShader*>(renderer->GetSystemResource(static_cast<TRenderInterface::SYSRESOURCES>(9)));
	auto* system =
	    static_cast<TSysShader*>(renderer->GetSystemResource(static_cast<TRenderInterface::SYSRESOURCES>(8)));
	auto* grass = static_cast<TGrassShader*>(renderer->FindOrphanedResource("SHGRASS", nullptr));
	if (skin != nullptr)
		skin->SetLightScattering(false);
	if (system != nullptr)
		system->SetLightScattering(false);
	if (grass != nullptr)
		grass->SetLightScattering(false);
}

void ARootTask::CreateARenderer()
{
	if (m_pARenderer == nullptr || !m_pARenderer->Create())
		m_pARenderer = nullptr;
}

void ARootTask::CreateAnimationSystem()
{
	m_pAnimationSystem->Create();
}

void ARootTask::CreateInputSystem()
{
	m_pInputHandler->Create();
	m_pVibrationManager->Create();
	m_pVibrationManager->Activate(false);
}

void ARootTask::CreateGUISystem()
{
	if (m_pGUISystem == nullptr || !m_pGUISystem->Create())
		m_pGUISystem = nullptr;
}

void ARootTask::CreateGameStateController()
{
	m_pRootStateController->Create();
}

void ARootTask::CreateMissionController()
{
	m_pMissionController->Create();
}

void ARootTask::CreateSoundSystem()
{
	m_pSoundManager->Create();
}

void ARootTask::CreateCameraManager()
{
	m_pCameraManager->Create();
}

TObject* TSTDCALL ARootTask::CreateObject()
{
	return new ARootTask();
}

TObject* TSTDCALL ARootTask::CreateObjectInPlace(void* memory)
{
	return memory == nullptr ? nullptr : new (memory) ARootTask();
}

void TSTDCALL ARootTask::InitialiseStatic()
{
}

void TSTDCALL ARootTask::DeinitialiseStatic()
{
}
