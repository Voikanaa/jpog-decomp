#include "game/ARootTask.h"

#include "game/AApplication.h"
#include "game/ADummyTask.h"
#include "game/AOptionLogic.h"
#include "game/AOptions.h"
#include "game/ARenderer.h"
#include "toshi/kernel/TCString.h"
#include "toshi/kernel/TKernelInterface.h"

#include <new>

using namespace Toshi;

TClass ARootTask::m_sClass("ARootTask", &TTask::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                           DeinitialiseStatic, 0x20000);

ARootTask::ARootTask()
{
	m_pOptions = new AOptions();
	AllocateRenderInterface();
	AllocateARenderer();
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
	if (!CreateRenderInterface())
		return false;
	CreateARenderer();
	// The original saves after creating the other systems and loading the material libraries.
	m_pOptions->SaveOptions();
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

void __stdcall ARootTask::DeserialiseOptions()
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

void __stdcall ARootTask::SerialiseOptions()
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
	m_pARenderer =
	    g_oTheApp.GetKernel()->GetScheduler()->CreateTask(ARenderer::m_sClass, g_oTheApp.GetRenderRootTask());
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
	// TODO(#21): the original continues with TMaterialLibraryManager::Create and
	// ARenderer::DeserialiseOptions.
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

void ARootTask::CreateARenderer()
{
	if (m_pARenderer == nullptr || !m_pARenderer->Create())
		m_pARenderer = nullptr;
}

TObject* __stdcall ARootTask::CreateObject()
{
	return new ARootTask();
}

TObject* __stdcall ARootTask::CreateObjectInPlace(void* memory)
{
	return memory == nullptr ? nullptr : new (memory) ARootTask();
}

void __stdcall ARootTask::InitialiseStatic()
{
}

void __stdcall ARootTask::DeinitialiseStatic()
{
}
