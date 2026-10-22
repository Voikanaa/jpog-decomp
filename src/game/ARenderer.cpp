#include "game/ARenderer.h"

#include "game/AApplication.h"
#include "game/ARootTask.h"

#include <new>

using namespace Toshi;

TClass ARenderer::m_sClass("ARenderer", &TTask::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                           DeinitialiseStatic, 0x10000);

TClass& ARenderer::GetClass() const
{
	return m_sClass;
}

bool ARenderer::OnUpdate(float deltaTime)
{
	const TManagedPtr<TRenderInterface> renderer = g_oTheApp.GetRootTask()->GetRenderInterface();
	if (!renderer->Update(deltaTime))
	{
		g_oTheApp.Destroy();
		return true;
	}
	renderer->BeginScene();
	renderer->EndScene();
	return true;
}

TObject* TSTDCALL ARenderer::CreateObject()
{
	return new ARenderer();
}

TObject* TSTDCALL ARenderer::CreateObjectInPlace(void* memory)
{
	return memory == nullptr ? nullptr : new (memory) ARenderer();
}

void TSTDCALL ARenderer::InitialiseStatic()
{
}

void TSTDCALL ARenderer::DeinitialiseStatic()
{
}
