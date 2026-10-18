#include "game/AApplication.h"

#include "toshi/kernel/TCString.h"
#include "toshi/kernel/TKernelInterface.h"
#include "toshi/kernel/TObject.h"

using namespace Toshi;

AApplication g_oTheApp;

bool AApplication::OnCreate(int argc, char** const argv)
{
	if (!CreateRenderInterface())
		return false;

	return TApplication::OnCreate(argc, argv);
}

// Mirrors ARootTask::AllocateRenderInterface and the display part of
// ARootTask::CreateRenderInterface, with the defaults the game uses before options are loaded
// (640x480, 32-bit).
bool AApplication::CreateRenderInterface()
{
	GetKernel()->LoadInterface(TCString("TRenderD3DInterface"));

	const TClass* renderClass = TClass::Find("TRenderD3DInterface", nullptr);
	if (!renderClass)
		return false;

	m_pRenderInterface = static_cast<TRenderInterface*>(renderClass->CreateObject());
	if (!m_pRenderInterface || !m_pRenderInterface->Create(GetKernel()))
		return false;

	TRenderInterface::DisplayParams params{};
	params.uiWidth = 640;
	params.uiHeight = 480;
	params.uiColourDepth = 32;
	params.eDepthStencilFormat = 0;
	params.bWindowed = true;
	if (m_pRenderInterface->CreateDisplay(&params))
		return true;

	// Like the game, fall back to 16-bit colour if 32-bit is unavailable.
	params.uiColourDepth = 16;
	return m_pRenderInterface->CreateDisplay(&params);
}

bool AApplication::OnUpdate(float deltaTime)
{
	if (!m_pRenderInterface->Update(deltaTime))
		return false;

	m_pRenderInterface->BeginScene();
	m_pRenderInterface->EndScene();
	return TApplication::OnUpdate(deltaTime);
}

// The game releases its renderer reference only in ~ARootTask, when the kernel is torn down at
// process exit, and never calls Destroy() or DestroyDisplay() itself. Deleting the renderer cannot
// work (issue #20): its teardown frees every resource (DestroyAllResources) before DestroyDisplay
// uses the display texture, and DestroyDisplay cannot run twice. So as the last holder we keep it
// alive and leave the device and window to process exit. Fix this properly in our own
// TRenderD3DInterface.
bool AApplication::OnDestroy()
{
	if (m_pRenderInterface && m_pRenderInterface->GetRefCount() == 1)
		m_pRenderInterface->IncRefCount(nullptr); // deliberately never released
	m_pRenderInterface.Release();
	return TApplication::OnDestroy();
}
