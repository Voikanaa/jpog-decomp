#include "toshi/kernel/TKernelInterface.h"

#include "platform/System.h"
#include "toshi/kernel/TDLL.h"
#include "toshi/kernel/TScheduler.h"
#include "toshi/kernel/TUser.h"
#include "toshi/kernel/file/TFile.h"
#include "toshi/kernel/file/TMemFile.h"
#include "toshi/kernel/file/TNativeFile.h"
#include "toshi/kernel/file/TNullFile.h"

namespace Toshi
{

TClass TKernelInterface::m_sClass("TKernelInterface", &TObject::m_sClass, CreateObject, CreateObjectInPlace,
                                  InitialiseStatic, DeinitialiseStatic, 0x10000);

namespace
{

// The engine registers itself as user "Toshi" (once) and remembers the latest kernel.
struct TKernelUser : TUser
{
	TKernelUser() : TUser("Toshi") { Register(); }
	TKernelInterface* m_pKernel = nullptr;
};

TKernelUser& KernelUser()
{
	static TKernelUser user;
	return user;
}

// The module list head as the module pointer its links use: where a module's list field would
// be the list itself (it is not a real module), as in the original.
TKernelInterfaceDLL* ModuleListHead(void** list)
{
	return reinterpret_cast<TKernelInterfaceDLL*>(reinterpret_cast<unsigned char*>(list) - sizeof(TDLL));
}

// The scheduler pointer counts as a reference to the scheduler.
void SetScheduler(TScheduler*& slot, TScheduler* scheduler)
{
	if (slot == scheduler)
		return;
	if (slot != nullptr && slot->DecRefCount(nullptr) == 0)
		slot->Delete();
	slot = scheduler;
	if (scheduler != nullptr)
		scheduler->IncRefCount(nullptr);
}

} // namespace

// The file systems and the manager are never deleted, as in the original.
TKernelInterface::TKernelInterface(int, char** const, bool verbose)
    : m_pScheduler(nullptr), m_pModuleList(nullptr), m_pFirstModule(ModuleListHead(&m_pModuleList)),
      m_pLastModule(ModuleListHead(&m_pModuleList)), m_iModuleCount(0), m_fSecondTime(0.0f), m_fAverageFPS(0.0f),
      m_bVerbose(verbose), m_hInstance(nullptr), m_bPaused(false)
{
	KernelUser().m_pKernel = this;

	TCString workingDirectory;
	char buffer[0x104];
	if (jpog::platform::WorkingDirectory(buffer, sizeof(buffer)))
		workingDirectory = buffer;
	auto* manager = new TFileManager();
	(new TNativeFileSystem("local"))->SetPrefix(workingDirectory);
	(new TNativeFileSystem("abs"))->SetPrefix("");
	new TNullFileSystem("null");
	manager->SetSystemPath("local");
	new TMemFileSystem("mem");
	if (verbose)
		DumpInfo();
	SetScheduler(m_pScheduler, new TScheduler(this));
}

TKernelInterface::~TKernelInterface()
{
	m_pScheduler->DestroyAllTasks();
	SetScheduler(m_pScheduler, nullptr);
	// The head's links are m_pFirstModule/m_pLastModule: write them directly, not through the head
	// pointer (it does not point at a module).
	const auto nextSlot = [this](TKernelInterfaceDLL* node) -> TKernelInterfaceDLL*&
	{ return node == ModuleListHead(&m_pModuleList) ? m_pFirstModule : node->m_pNext; };
	const auto prevSlot = [this](TKernelInterfaceDLL* node) -> TKernelInterfaceDLL*&
	{ return node == ModuleListHead(&m_pModuleList) ? m_pLastModule : node->m_pPrev; };
	for (TKernelInterfaceDLL* module = m_pFirstModule; module != ModuleListHead(&m_pModuleList);
	     module = m_pFirstModule)
	{
		module->m_pList = nullptr;
		nextSlot(module->m_pPrev) = module->m_pNext;
		prevSlot(module->m_pNext) = module->m_pPrev;
		module->m_pNext = module;
		module->m_pPrev = module;
		--m_iModuleCount;
		delete module;
	}
	SetScheduler(m_pScheduler, nullptr);
}

TClass& TKernelInterface::GetClass() const
{
	return m_sClass;
}

// The average is updated once per second: the mean of the previous average and the frame
// rate of the frame that crossed the second.
bool TKernelInterface::Update()
{
	if (m_bPaused)
	{
		m_pScheduler->UpdateKernelPaused();
		return true;
	}
	m_oSystemTimer.Update();
	const float delta = GetSystemTimer()->GetDelta();
	const float time = m_fSecondTime + delta;
	if (time <= 1.0f)
		m_fSecondTime = time;
	else
	{
		m_fAverageFPS = (m_fAverageFPS + 1.0f / delta) * 0.5f;
		m_fSecondTime = 0.0f;
	}
	m_pScheduler->Update();
	return true;
}

bool TKernelInterface::Pause(bool paused)
{
	const bool previous = m_bPaused;
	m_bPaused = paused;
	return previous;
}

bool TKernelInterface::IsPaused()
{
	return m_bPaused;
}

TKernelInterfaceDLL* TKernelInterface::LoadInterface(const TCString& name)
{
	if (TKernelInterfaceDLL* loaded = FindInterface(name))
		return loaded;
	auto* module = new TKernelInterfaceDLL();
	if (!module->Load(name))
	{
		delete module;
		return nullptr;
	}
	// The head's previous link is m_pLastModule (the head pointer does not point at a module).
	const auto prevSlot = [this](TKernelInterfaceDLL* node) -> TKernelInterfaceDLL*&
	{ return node == ModuleListHead(&m_pModuleList) ? m_pLastModule : node->m_pPrev; };
	module->m_pList = &m_pModuleList;
	module->m_pNext = m_pFirstModule;
	module->m_pPrev = ModuleListHead(&m_pModuleList);
	prevSlot(m_pFirstModule) = module;
	m_pFirstModule = module;
	++m_iModuleCount;
	return module;
}

TKernelInterfaceDLL* TKernelInterface::FindInterface(const TCString& name)
{
	for (TKernelInterfaceDLL* module = m_pFirstModule; module != ModuleListHead(&m_pModuleList);
	     module = module->m_pNext)
		if (module->GetFilename() == name)
			return module;
	return nullptr;
}

void TKernelInterface::UnloadInterface(TKernelInterfaceDLL& module)
{
	module.Unload();
	// The head's links are m_pFirstModule/m_pLastModule: write them directly, not through the head
	// pointer (it does not point at a module).
	const auto nextSlot = [this](TKernelInterfaceDLL* node) -> TKernelInterfaceDLL*&
	{ return node == ModuleListHead(&m_pModuleList) ? m_pFirstModule : node->m_pNext; };
	const auto prevSlot = [this](TKernelInterfaceDLL* node) -> TKernelInterfaceDLL*&
	{ return node == ModuleListHead(&m_pModuleList) ? m_pLastModule : node->m_pPrev; };
	module.m_pList = nullptr;
	nextSlot(module.m_pPrev) = module.m_pNext;
	prevSlot(module.m_pNext) = module.m_pPrev;
	module.m_pNext = &module;
	module.m_pPrev = &module;
	--m_iModuleCount;
	delete &module;
}

void TKernelInterface::UnloadInterface(const TCString& name)
{
	if (TKernelInterfaceDLL* module = FindInterface(name))
		UnloadInterface(*module);
}

void TKernelInterface::DumpInfo()
{
	jpog::platform::QuerySystemInfo();
}

THPTimer* TKernelInterface::GetSystemTimer()
{
	return &m_oSystemTimer;
}

TScheduler* TKernelInterface::GetScheduler() const
{
	return m_pScheduler;
}

float TKernelInterface::GetFPS()
{
	return 1.0f / m_oSystemTimer.GetDelta();
}

float TKernelInterface::GetAverageFPS()
{
	return m_fAverageFPS;
}

void TKernelInterface::SetInstance(HINSTANCE__* instance)
{
	m_hInstance = instance;
}

HINSTANCE__* TKernelInterface::GetInstance()
{
	return m_hInstance;
}

TObject* TSTDCALL TKernelInterface::CreateObject()
{
	return nullptr;
}

TObject* TSTDCALL TKernelInterface::CreateObjectInPlace(void*)
{
	return nullptr;
}

void TSTDCALL TKernelInterface::InitialiseStatic()
{
}

void TSTDCALL TKernelInterface::DeinitialiseStatic()
{
}

} // namespace Toshi
