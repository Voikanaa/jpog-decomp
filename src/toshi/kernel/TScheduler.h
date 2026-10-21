#pragma once

#include "common/layout.h"
#include "toshi/kernel/TObject.h"
#include "toshi/kernel/TRefCounted.h"
#include "toshi/kernel/containers/TNodeTree.h"

namespace Toshi
{

class TKernelInterface;
class TScheduler;

// A unit of per-frame work in the scheduler's task tree (TKernelInterface.dll). Subclasses
// override the On* hooks. Flags: 1 created, 2 active, 4 dying.
class TTask : public TObject, public TRefCounted, public TNodeTree<TTask>::TNode
{
	friend class TScheduler;

public:
	enum Flags : unsigned int
	{
		FLAG_CREATED = 1,
		FLAG_ACTIVE = 2,
		FLAG_DYING = 4,
	};

	TTask();
	~TTask() override;

	TClass& GetClass() const override;
	// Runs OnCreate once; a failed create deletes the task.
	virtual bool Create();
	// Deletes a task that was never created.
	virtual bool CreateFailed();

	void Activate(bool active);
	// Marks the task and its children dying; the scheduler deletes them on its next update.
	void DestroyTask();
	unsigned int GetFlags() const;
	bool IsCreated() const;
	bool IsActive() const;
	bool IsDying() const;
	TScheduler* GetScheduler();
	TKernelInterface* GetKernel();

	TIMPORT_DATA static TClass m_sClass;

protected:
	virtual bool OnCreate();
	// Returning false skips the children this frame.
	virtual bool OnUpdate(float deltaTime);
	virtual bool OnUpdateKernelPaused();
	virtual void OnDestroy();
	// Returning false keeps the child alive.
	virtual bool OnChildDying(const TTask* child);
	// `child` has already been deleted; only its address is passed.
	virtual void OnChildDied(const TClass& childClass, const TTask* child);
	virtual void OnActivate();
	virtual void OnDeactivate();

private:
	static TObject* TSTDCALL CreateObject();
	static TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	unsigned int m_uiFlags;
	TScheduler* m_pScheduler; // holds a reference
};

JPOG_ASSERT_SIZE(TTask, 0x24);

// Owns and updates the task tree (TKernelInterface.dll). Each frame it deletes dying tasks,
// then updates active ones depth first with the frame time (clamped to [0, max]).
class TScheduler : public TObject, public TRefCounted
{
	friend class TTask;

public:
	explicit TScheduler(TKernelInterface* kernel);
	~TScheduler() override;

	TClass& GetClass() const override;

	// Creates a task of `cls` (top level, or as the last child of `parent`).
	TTask* CreateTask(const TClass& cls, TTask* parent);
	void DestroyTask(TTask& task);
	// Only destroys the first top-level task's subtree, as the original.
	void DestroyAllTasks();
	void DestroyDyingTasks(TTask* first);
	void Update();
	void UpdateKernelPaused();
	// Limits the frame time to 0.01 s instead of 1 s.
	void SetDebugSlowTime(bool slow);
	// Empty in the shipped build.
	void Debug_PrintTaskTree() const;

	float GetCurrentTimeDelta();
	float GetTotalTime();
	int GetFrameCount();
	TKernelInterface* GetKernelInterface();
	TNodeTree<TTask>* GetTree();

	static TClass m_sClass;

private:
	void UpdateActiveTasks(TTask* first);
	void UpdateActiveTasksKernelPaused(TTask* first);
	void DestroyTaskRecurse(TTask* first);
	void DeleteTask(TTask* task);
	void DeleteTaskRecurse(TTask* first);
	void DeleteTaskAtomic(TTask* task);
	void Debug_PrintTaskTree_Rescurse(const TTask* first, int depth) const;

	static TObject* TSTDCALL CreateObject();
	static TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	// Time delta limits (normal, debug slow time).
	static const float sm_cfMaxTimeDeltaAllowed;
	static const float sm_cfDebugSlowMaxTimeDeltaAllowed;

	TKernelInterface* m_pKernel;
	TNodeTree<TTask> m_oTree;
	float m_fCurrentTimeDelta;
	float m_fTotalTime;
	float m_fMaxTimeDelta;
	int m_iFrameCount;
};

JPOG_ASSERT_SIZE(TScheduler, 0x34);

} // namespace Toshi
