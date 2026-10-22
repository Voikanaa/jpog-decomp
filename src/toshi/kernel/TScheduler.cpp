#include "toshi/kernel/TScheduler.h"

#include "toshi/kernel/TKernelInterface.h"

namespace Toshi
{

TClass TTask::m_sClass("TTask", &TObject::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                       DeinitialiseStatic, 0x10000);
TClass TScheduler::m_sClass("TScheduler", &TObject::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                            DeinitialiseStatic, 0x10000);
const float TScheduler::sm_cfMaxTimeDeltaAllowed = 1.0f;
const float TScheduler::sm_cfDebugSlowMaxTimeDeltaAllowed = 0.01f;

namespace
{

// The scheduler pointer of a task counts as a reference to the scheduler.
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

// --- TTask ---------------------------------------------------------------------------------

TTask::TTask() : m_uiFlags(0), m_pScheduler(nullptr)
{
	m_sClass.GetVersion(); // the original reads it and discards it
}

TTask::~TTask()
{
	SetScheduler(m_pScheduler, nullptr);
}

TClass& TTask::GetClass() const
{
	return m_sClass;
}

bool TTask::Create()
{
	if (IsCreated())
		return true;
	if (!OnCreate())
	{
		OnDestroy();
		TNodeTree<TTask>* tree = Tree();
		if (tree != nullptr)
			tree->Remove(this, false);
		else
			TNodeTree<TTask>::Unlink(this);
		delete this;
		return false;
	}
	m_uiFlags |= FLAG_CREATED;
	Activate(true);
	return true;
}

bool TTask::CreateFailed()
{
	if (!IsCreated())
	{
		TNodeTree<TTask>* tree = Tree();
		if (tree != nullptr)
			tree->Remove(this, false);
		else
			TNodeTree<TTask>::Unlink(this);
		delete this;
	}
	return false;
}

void TTask::Activate(bool active)
{
	const unsigned int old = m_uiFlags;
	m_uiFlags = (old & ~FLAG_ACTIVE) | (active ? static_cast<unsigned int>(FLAG_ACTIVE) : 0u);
	if (old == m_uiFlags)
		return;
	if (active)
		OnActivate();
	else
		OnDeactivate();
}

void TTask::DestroyTask()
{
	m_pScheduler->DestroyTask(*this);
}

unsigned int TTask::GetFlags() const
{
	return m_uiFlags;
}

bool TTask::IsCreated() const
{
	return (m_uiFlags & FLAG_CREATED) != 0;
}

bool TTask::IsActive() const
{
	return (m_uiFlags & FLAG_ACTIVE) != 0;
}

bool TTask::IsDying() const
{
	return (m_uiFlags & FLAG_DYING) != 0;
}

TScheduler* TTask::GetScheduler()
{
	return m_pScheduler;
}

TKernelInterface* TTask::GetKernel()
{
	return GetScheduler()->GetKernelInterface();
}

bool TTask::OnCreate()
{
	return true;
}

bool TTask::OnUpdate(float)
{
	return true;
}

bool TTask::OnUpdateKernelPaused()
{
	return true;
}

void TTask::OnDestroy()
{
}

bool TTask::OnChildDying(const TTask*)
{
	return true;
}

void TTask::OnChildDied(const TClass&, const TTask*)
{
}

void TTask::OnActivate()
{
}

void TTask::OnDeactivate()
{
}

TObject* TSTDCALL TTask::CreateObject()
{
	return nullptr;
}

TObject* TSTDCALL TTask::CreateObjectInPlace(void*)
{
	return nullptr;
}

void TSTDCALL TTask::InitialiseStatic()
{
}

void TSTDCALL TTask::DeinitialiseStatic()
{
}

// --- TScheduler ----------------------------------------------------------------------------

TScheduler::TScheduler(TKernelInterface* kernel)
    : m_pKernel(kernel), m_fCurrentTimeDelta(0.0f), m_fTotalTime(0.0f), m_fMaxTimeDelta(sm_cfMaxTimeDeltaAllowed),
      m_iFrameCount(0)
{
}

TScheduler::~TScheduler()
{
	for (TTask* task = m_oTree.Attached(); task != nullptr; task = m_oTree.Attached())
	{
		m_oTree.Remove(task, false);
		m_oTree.DeleteRecurse(task);
	}
}

TClass& TScheduler::GetClass() const
{
	return m_sClass;
}

TTask* TScheduler::CreateTask(const TClass& cls, TTask* parent)
{
	auto* task = static_cast<TTask*>(cls.CreateObject());
	SetScheduler(task->m_pScheduler, this);
	m_oTree.InsertHead(task);
	if (parent != nullptr)
	{
		m_oTree.Remove(task, false);
		m_oTree.InsertTail(parent, task);
	}
	return task;
}

void TScheduler::DestroyTask(TTask& task)
{
	if (task.IsDying())
		return;
	task.m_uiFlags |= TTask::FLAG_DYING;
	DestroyTaskRecurse(task.Attached());
}

void TScheduler::DestroyAllTasks()
{
	if (m_oTree.Attached() == nullptr)
		return;
	DestroyTask(*m_oTree.Attached());
	DestroyDyingTasks(m_oTree.Attached());
}

// Walks the siblings backwards from the last one.
void TScheduler::DestroyDyingTasks(TTask* first)
{
	if (first == nullptr)
		return;
	for (TTask* task = first->Prev(); task != nullptr;)
	{
		TTask* next = task == first ? nullptr : task->Prev();
		if (task->IsDying())
			DeleteTask(task);
		else if (task->Attached() != nullptr)
			DestroyDyingTasks(task->Attached());
		task = next;
	}
}

void TScheduler::Update()
{
	const float delta = m_pKernel->GetSystemTimer()->GetDelta();
	m_fTotalTime += delta;
	++m_iFrameCount;
	if (0.0f <= delta)
		m_fCurrentTimeDelta = delta <= m_fMaxTimeDelta ? delta : m_fMaxTimeDelta;
	else
		m_fCurrentTimeDelta = 0.0f;
	DestroyDyingTasks(m_oTree.Attached());
	UpdateActiveTasks(m_oTree.Attached());
}

void TScheduler::UpdateKernelPaused()
{
	UpdateActiveTasksKernelPaused(m_oTree.Attached());
}

void TScheduler::SetDebugSlowTime(bool slow)
{
	m_fMaxTimeDelta = slow ? sm_cfDebugSlowMaxTimeDeltaAllowed : sm_cfMaxTimeDeltaAllowed;
}

void TScheduler::Debug_PrintTaskTree() const
{
}

float TScheduler::GetCurrentTimeDelta()
{
	return m_fCurrentTimeDelta;
}

float TScheduler::GetTotalTime()
{
	return m_fTotalTime;
}

int TScheduler::GetFrameCount()
{
	return m_iFrameCount;
}

TKernelInterface* TScheduler::GetKernelInterface()
{
	return m_pKernel;
}

TNodeTree<TTask>* TScheduler::GetTree()
{
	return &m_oTree;
}

// Stops at once when a task pauses the kernel.
void TScheduler::UpdateActiveTasks(TTask* first)
{
	for (TTask* task = first; task != nullptr;)
	{
		TTask* next = task->Next() == first ? nullptr : task->Next();
		bool updateChildren = true;
		if (task->IsCreated() && task->IsActive())
		{
			updateChildren = task->OnUpdate(m_fCurrentTimeDelta);
			if (m_pKernel->IsPaused())
				return;
		}
		if (task->Attached() != nullptr && updateChildren)
			UpdateActiveTasks(task->Attached());
		task = next;
	}
}

void TScheduler::UpdateActiveTasksKernelPaused(TTask* first)
{
	for (TTask* task = first; task != nullptr;)
	{
		TTask* next = task->Next() == first ? nullptr : task->Next();
		bool updateChildren = true;
		if (task->IsCreated() && task->IsActive())
			updateChildren = task->OnUpdateKernelPaused();
		if (task->Attached() != nullptr && updateChildren)
			UpdateActiveTasksKernelPaused(task->Attached());
		task = next;
	}
}

void TScheduler::DestroyTaskRecurse(TTask* first)
{
	for (TTask* task = first; task != nullptr;)
	{
		task->m_uiFlags |= TTask::FLAG_DYING;
		if (task->Attached() != nullptr)
			DestroyTaskRecurse(task->Attached());
		task = task->Next() == first ? nullptr : task->Next();
	}
}

void TScheduler::DeleteTask(TTask* task)
{
	DeleteTaskRecurse(task->Attached());
	DeleteTaskAtomic(task);
}

// Walks the siblings backwards from the last one; tasks kept alive by OnChildDying stay.
void TScheduler::DeleteTaskRecurse(TTask* first)
{
	if (first == nullptr)
		return;
	for (TTask* task = first->Prev(); task != nullptr;)
	{
		TTask* next = task->Prev() == task ? nullptr : task->Prev();
		DeleteTaskAtomic(task);
		task = next;
	}
}

void TScheduler::DeleteTaskAtomic(TTask* task)
{
	if (task == nullptr)
		return;
	TTask* parent = task->Parent();
	if (parent == nullptr || (task->Tree() != nullptr && parent == task->Tree()->Root()))
		parent = nullptr;
	const bool hasParent = parent != nullptr && parent != m_oTree.Root();
	if (hasParent && !parent->IsDying() && !parent->OnChildDying(task))
		return;
	if (task->Attached() != nullptr)
		DeleteTaskRecurse(task->Attached());
	const TClass& cls = task->GetClass();
	task->OnDestroy();
	m_oTree.Remove(task, false);
	delete task;
	if (hasParent)
		parent->OnChildDied(cls, task);
}

void TScheduler::Debug_PrintTaskTree_Rescurse(const TTask* first, int depth) const
{
	for (const TTask* task = first; task != nullptr;)
	{
		const TTask* next = task->Next() == first ? nullptr : task->Next();
		if (task->Attached() != nullptr)
			Debug_PrintTaskTree_Rescurse(task->Attached(), depth + 1);
		task = next;
	}
}

TObject* TSTDCALL TScheduler::CreateObject()
{
	return nullptr;
}

TObject* TSTDCALL TScheduler::CreateObjectInPlace(void*)
{
	return nullptr;
}

void TSTDCALL TScheduler::InitialiseStatic()
{
}

void TSTDCALL TScheduler::DeinitialiseStatic()
{
}

} // namespace Toshi
