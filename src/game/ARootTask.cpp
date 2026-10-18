#include "game/ARootTask.h"

#include "game/AApplication.h"
#include "game/ADummyTask.h"
#include "game/ARenderer.h"
#include "toshi/kernel/TCString.h"
#include "toshi/kernel/TKernelInterface.h"

#include <new>

using namespace Toshi;

TClass ARootTask::m_sClass("ARootTask", &TTask::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                           DeinitialiseStatic, 0x20000);

ARootTask::ARootTask()
{
	AllocateRenderInterface();
	AllocateARenderer();
}

// The original only drops its renderer reference. Deviation: deleting the renderer cannot work
// (issue #20: its teardown frees every resource before DestroyDisplay uses the display texture,
// and DestroyDisplay cannot run twice), so as the last holder we keep it alive and leave the
// device and window to process exit. Fix this properly in our own TRenderD3DInterface.
ARootTask::~ARootTask()
{
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

bool ARootTask::OnCreate()
{
	if (!CreateRenderInterface())
		return false;
	CreateARenderer();
	return TTask::OnCreate();
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

// The display part of the original: it takes the mode from the options, which are not loaded
// yet, so this uses the game's defaults (640x480, 32-bit, windowed), falling back to 16-bit
// colour like the game.
bool ARootTask::CreateRenderInterface()
{
	TRenderInterface::DisplayParams params{};
	params.uiWidth = 640;
	params.uiHeight = 480;
	params.uiColourDepth = 32;
	params.eDepthStencilFormat = 0;
	params.bWindowed = true;
	if (m_pRenderInterface->CreateDisplay(&params))
		return true;
	params.uiColourDepth = 16;
	return m_pRenderInterface->CreateDisplay(&params);
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
