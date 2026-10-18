#include "game/AApplication.h"

#include "game/ADummyTask.h"
#include "game/ALocale.h"
#include "game/ARootTask.h"
#include "ppropertyparser/PProperties.h"
#include "toshi/gui/TGUIWidgetAction.h"
#include "toshi/kernel/TCString.h"
#include "toshi/kernel/TCStringPool.h"
#include "toshi/kernel/TFreeList.h"
#include "toshi/kernel/TKernelInterface.h"
#include "toshi/kernel/TLocale.h"
#include "toshi/kernel/TMemory.h"
#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/TWStringPool.h"
#include "toshi/renderd3d/TRenderD3DResources.h"

using namespace Toshi;

AApplication g_oTheApp;

// SimJP.exe 0x4846C0. The original sizes the free lists through inline access to their static
// members; the exported GetFreeList functions reach the same objects.
bool AApplication::OnCreate(int argc, char** const argv)
{
	TSystem::CreateCStringPoolExplicit("data/strpool.dat", 0x400, 12000);
	PProperties::GetFreeList().SetCapacity(5500);
	PProperties::GetFreeList().SetGrowSize(250);
	PProperties::PProperty::GetFreeList().SetCapacity(40000);
	PProperties::PProperty::GetFreeList().SetGrowSize(2000);
	TGUIWidgetAction::GetFreeList().SetCapacity(32);
	TLocalisedString::GetFreeList().SetCapacity(5000);
	TPooledCString::GetFreeList().SetCapacity(2000);
	TPooledWString::GetFreeList().SetCapacity(4000);
	TTextureResourceHAL::GetFreeList().SetCapacity(256);
	TIndexBlockResource::GetFreeList().SetCapacity(200);
	TVertexBlockResource::GetFreeList().SetCapacity(450);

	TSystem::GetLocale()->LoadInfo(TCString("Data/GUI/Locales/Locales.txt"));
	TMemory::DebugPrintHALMemInfo(nullptr);
	ALocale::SetMeasurementSystem(ALocale::MeasurementSystem_Metric);

	GetKernel()->LoadInterface(TCString("TGUIInterface"));
	GetKernel()->LoadInterface(TCString("PGUICore"));
	const_cast<TClass*>(TClass::Find("TGUIInterface", nullptr))->InitialiseStatic();

	m_pRenderTask = CreateRootTask(const_cast<char*>("RenderTask"));
	m_pUpdate3Task = CreateRootTask(const_cast<char*>("Update3"));
	m_pUpdate2Task = CreateRootTask(const_cast<char*>("Update2"));
	m_pUpdate1Task = CreateRootTask(const_cast<char*>("Update1"));
	m_pInputTask = CreateRootTask(const_cast<char*>("InputTask"));

	TScheduler* scheduler = GetKernel()->GetScheduler();
	m_pRootTask = static_cast<ARootTask*>(scheduler->CreateTask(ARootTask::m_sClass, m_pUpdate2Task));
	if (m_pRootTask != nullptr)
	{
		if (!m_pRootTask->Create())
			return false;
		m_pRootTask->Activate(true);
	}
	scheduler->Debug_PrintTaskTree();
	return TApplication::OnCreate(argc, argv);
}

// Creates, activates and names one of the top-level task groups.
ADummyTask* AApplication::CreateRootTask(char* name)
{
	auto* task = static_cast<ADummyTask*>(GetKernel()->GetScheduler()->CreateTask(ADummyTask::m_sClass, nullptr));
	task->Create();
	task->Activate(true);
	task->SetName(name);
	return task;
}

// SimJP.exe 0x484DC0. The tasks are only marked dying; the scheduler deletes them with the kernel
// when the application object goes away at process exit.
bool AApplication::OnDestroy()
{
	m_pRootTask->DestroyTask();
	m_pInputTask->DestroyTask();
	m_pRenderTask->DestroyTask();
	return true;
}

ARootTask* AApplication::GetRootTask() const
{
	return m_pRootTask;
}

ADummyTask* AApplication::GetInputRootTask()
{
	return m_pInputTask;
}

ADummyTask* AApplication::GetUpdate1RootTask()
{
	return m_pUpdate1Task;
}

ADummyTask* AApplication::GetUpdate2RootTask()
{
	return m_pUpdate2Task;
}

ADummyTask* AApplication::GetUpdate3RootTask()
{
	return m_pUpdate3Task;
}

ADummyTask* AApplication::GetRenderRootTask()
{
	return m_pRenderTask;
}
