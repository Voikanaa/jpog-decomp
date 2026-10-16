#include "game/AApplication.h"

#include "toshi/kernel/TCString.h"
#include "toshi/kernel/TKernelInterface.h"
#include "toshi/kernel/TObject.h"
#include "toshi/render/TRenderInterface.h"

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

bool AApplication::OnDestroy()
{
	// Like ~ARootTask: the game never calls Destroy()/DestroyDisplay() on the renderer, it only
	// releases its reference, and the last release deletes the object (which tears itself down).
	if (m_pRenderInterface)
	{
		m_pRenderInterface->Delete();
		m_pRenderInterface = nullptr;
	}
	return TApplication::OnDestroy();
}
