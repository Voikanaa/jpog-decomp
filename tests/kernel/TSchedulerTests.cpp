#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TDLL.h"
#include "toshi/kernel/TKernelInterface.h"
#include "toshi/kernel/TScheduler.h"
#include "toshi/kernel/TUser.h"
#include "toshi/kernel/file/TFile.h"

#include <cstring>
#include <doctest/doctest.h>
#include <map>
#include <string>
#include <vector>

using Toshi::TClass;
using Toshi::TKernelInterface;
using Toshi::TObject;
using Toshi::TScheduler;
using Toshi::TTask;
using namespace jpog::test;

namespace
{

// Hook calls of one side, as text ("3:update 0.5").
std::vector<std::string> s_log;
// Task address -> id, per side; entries stay after deletion (OnChildDied gets a dead pointer).
std::map<const void*, int> s_ids[2];
int s_side = 0;
int s_nextId = 0;

int IdOf(const void* task)
{
	const auto it = s_ids[s_side].find(task);
	return it == s_ids[s_side].end() ? -1 : it->second;
}

void Log(const void* task, const std::string& what)
{
	s_log.push_back(std::to_string(IdOf(task)) + ":" + what);
}

// Behaviour derived from the id, the same on both sides.
bool CreateResult(int id)
{
	return id % 7 != 3;
}
bool UpdateResult(int id)
{
	return id % 5 != 2;
}
bool ChildDyingResult(int id)
{
	return id % 6 != 1;
}

// Our side: a task that records its hooks.
class TRecordTask : public TTask
{
public:
	~TRecordTask() override { Log(this, "deleted"); }

	static TObject* TSTDCALL CreateRecord()
	{
		auto* task = new TRecordTask();
		s_ids[1][task] = s_nextId++;
		return task;
	}

protected:
	bool OnCreate() override
	{
		Log(this, "create");
		return CreateResult(IdOf(this));
	}
	bool OnUpdate(float deltaTime) override
	{
		Log(this, "update " + std::to_string(deltaTime));
		return UpdateResult(IdOf(this));
	}
	bool OnUpdateKernelPaused() override
	{
		Log(this, "paused update");
		return UpdateResult(IdOf(this));
	}
	void OnDestroy() override { Log(this, "destroy"); }
	bool OnChildDying(const TTask* child) override
	{
		Log(this, "child dying " + std::to_string(IdOf(child)));
		return ChildDyingResult(IdOf(this));
	}
	void OnChildDied(const TClass& cls, const TTask* child) override
	{
		Log(this, std::string("child died ") + cls.GetName() + " " + std::to_string(IdOf(child)));
	}
	void OnActivate() override { Log(this, "activate"); }
	void OnDeactivate() override { Log(this, "deactivate"); }
};

// The tree below `first` (sibling ring order), from raw memory: the same for both sides.
struct TaskView
{
	void* vtable;
	int refCount;
	void* tree;
	TaskView* next;
	TaskView* prev;
	TaskView* parent;
	TaskView* attached;
	unsigned int flags;
	void* scheduler;
};

std::string DescribeTree(const TaskView* first)
{
	std::string text;
	for (const TaskView* task = first; task != nullptr;)
	{
		text += std::to_string(IdOf(task)) + "/" + std::to_string(task->flags);
		if (task->attached != nullptr)
			text += "(" + DescribeTree(task->attached) + ")";
		text += " ";
		task = task->next == first ? nullptr : task->next;
	}
	return text;
}

struct SchedulerView
{
	void* vtable;
	int refCount;
	void* kernel;
	void* rootTree;
	void* rootNext;
	void* rootPrev;
	void* rootParent;
	TaskView* rootAttached;
	int count;
	float delta, total, maxDelta;
	int frames;
};

std::string DescribeScheduler(const void* scheduler)
{
	const auto& view = *static_cast<const SchedulerView*>(scheduler);
	return std::to_string(view.count) + " " + std::to_string(view.delta) + " " + std::to_string(view.total) + " " +
	       std::to_string(view.maxDelta) + " " + std::to_string(view.frames) + " [" + DescribeTree(view.rootAttached) +
	       "]";
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TScheduler")
{
	TEST_CASE("task tree and updates")
	{
		alignas(TKernelInterface) unsigned char kernel[sizeof(TKernelInterface)] = {};
		// Classes join the global class tree, so they live until exit.
		static TClass recordClass("TRecordTask", &TTask::m_sClass, TRecordTask::CreateRecord, nullptr, nullptr, nullptr,
		                          1);
		s_side = 1;
		s_log.clear();
		auto* scheduler = new TScheduler(reinterpret_cast<TKernelInterface*>(kernel));
		scheduler->IncRefCount(nullptr); // our own reference
		TTask* parent = scheduler->CreateTask(recordClass, nullptr);
		TTask* child = scheduler->CreateTask(recordClass, parent);
		CHECK(parent->Create());
		CHECK(child->Create());
		CHECK(child->Parent() == parent);
		CHECK(scheduler->GetTree()->Count() == 2);
		scheduler->Update();
		CHECK(scheduler->GetFrameCount() == 1);
		parent->DestroyTask();
		CHECK(child->IsDying());
		scheduler->Update();
		CHECK(scheduler->GetTree()->Count() == 0);
		CHECK(s_log.back() == std::to_string(IdOf(parent)) + ":deleted");
		delete scheduler;
	}

	TEST_CASE("kernel interface")
	{
		// The kernel mounts its file systems on a new (leaked, as in the original) file manager.
		auto* kernel = new TKernelInterface(0, nullptr, false);
		REQUIRE(kernel->GetScheduler() != nullptr);
		CHECK(kernel->GetScheduler()->GetKernelInterface() == kernel);
		CHECK_FALSE(kernel->Pause(true));
		CHECK(kernel->IsPaused());
		CHECK(kernel->Update()); // paused: no timer update
		CHECK(kernel->Pause(false));
		CHECK(kernel->Update());
		CHECK(kernel->GetScheduler()->GetFrameCount() == 1);
		CHECK(Toshi::TFileManager::GetFileManager()->FindFileSystem("mem") != nullptr);
		kernel->DumpInfo(); // only queries the system (debug run-time checks catch buffer overruns)

		const Toshi::TCString name("version.dll");
		Toshi::TKernelInterfaceDLL* module = kernel->LoadInterface(name);
		REQUIRE(module != nullptr);
		CHECK(kernel->LoadInterface(name) == module);
		CHECK(kernel->FindInterface(name) == module);
		CHECK(module->GetAddress("GetFileVersionInfoA") != nullptr);
		CHECK(kernel->LoadInterface("no_such_module.dll") == nullptr);
		kernel->UnloadInterface(name);
		CHECK(kernel->FindInterface(name) == nullptr);
		kernel->LoadInterface(name); // left for the destructor
		delete kernel;
	}

	TEST_CASE("users")
	{
		Toshi::TUser user("Tester");
		CHECK_FALSE(user.IsRegistered());
		CHECK(user.Register() >= 0);
		CHECK(Toshi::TUserHandler::GetSingleton().FindUser("TESTER") == &user);
		user.Deregister();
		CHECK(Toshi::TUserHandler::GetSingleton().FindUser("Tester") == nullptr);
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

// Original tasks: built by the original TTask constructor, then given a copy of its vtable
// whose hooks record like TRecordTask. The stubs use __fastcall to receive `this` in ECX
// (EDX is unused), which matches __thiscall.
void* s_originalVtable[13];
Method<void*, unsigned int> s_originalDeletingDestructor;
Method<void> s_originalTaskConstructor;
using Allocate = void*(__stdcall*)(int, char*, int);
Allocate s_originalAllocate;

void* __fastcall StubDelete(void* self, void*, unsigned int flags)
{
	Log(self, "deleted");
	return s_originalDeletingDestructor(self, flags);
}
bool __fastcall StubCreate(void* self, void*)
{
	Log(self, "create");
	return CreateResult(IdOf(self));
}
bool __fastcall StubUpdate(void* self, void*, float deltaTime)
{
	Log(self, "update " + std::to_string(deltaTime));
	return UpdateResult(IdOf(self));
}
bool __fastcall StubUpdatePaused(void* self, void*)
{
	Log(self, "paused update");
	return UpdateResult(IdOf(self));
}
void __fastcall StubDestroy(void* self, void*)
{
	Log(self, "destroy");
}
bool __fastcall StubChildDying(void* self, void*, const void* child)
{
	Log(self, "child dying " + std::to_string(IdOf(child)));
	return ChildDyingResult(IdOf(self));
}
void __fastcall StubChildDied(void* self, void*, const TClass* cls, const void* child)
{
	Log(self, std::string("child died ") + cls->GetName() + " " + std::to_string(IdOf(child)));
}
void __fastcall StubActivate(void* self, void*)
{
	Log(self, "activate");
}
void __fastcall StubDeactivate(void* self, void*)
{
	Log(self, "deactivate");
}

TObject* TSTDCALL CreateOriginalTask()
{
	void* task = s_originalAllocate(sizeof(TTask), nullptr, -1);
	s_originalTaskConstructor(task);
	*static_cast<void**>(task) = s_originalVtable;
	s_ids[0][task] = s_nextId++;
	return static_cast<TObject*>(task);
}

} // namespace

TEST_SUITE("TScheduler differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("time delta limits match the original")
	{
		CHECK(
		    Same(*static_cast<const float*>(Original<void*>("?sm_cfMaxTimeDeltaAllowed@TScheduler@Toshi@@0MB")), 1.0f));
		CHECK(Same(
		    *static_cast<const float*>(Original<void*>("?sm_cfDebugSlowMaxTimeDeltaAllowed@TScheduler@Toshi@@0MB")),
		    0.01f));
	}

	TEST_CASE("scheduler and tasks match the original")
	{
		s_originalAllocate = Original<Allocate>("?tmalloc@@YGPAXHPADH@Z");
		s_originalTaskConstructor = Original<Method<void>>("??0TTask@Toshi@@QAE@XZ");
		const auto* vtable = static_cast<void* const*>(Original<void*>("??_7TTask@Toshi@@6B@"));
		std::memcpy(s_originalVtable, vtable, sizeof(s_originalVtable));
		s_originalDeletingDestructor = Method<void*, unsigned int>{s_originalVtable[2]};
		s_originalVtable[2] = reinterpret_cast<void*>(StubDelete);
		s_originalVtable[5] = reinterpret_cast<void*>(StubCreate);
		s_originalVtable[6] = reinterpret_cast<void*>(StubUpdate);
		s_originalVtable[7] = reinterpret_cast<void*>(StubUpdatePaused);
		s_originalVtable[8] = reinterpret_cast<void*>(StubDestroy);
		s_originalVtable[9] = reinterpret_cast<void*>(StubChildDying);
		s_originalVtable[10] = reinterpret_cast<void*>(StubChildDied);
		s_originalVtable[11] = reinterpret_cast<void*>(StubActivate);
		s_originalVtable[12] = reinterpret_cast<void*>(StubDeactivate);

		using P = void*;
		const auto construct = Original<Method<void, P>>("??0TScheduler@Toshi@@QAE@PAVTKernelInterface@1@@Z");
		const auto destruct = Original<Method<void>>("??1TScheduler@Toshi@@UAE@XZ");
		const auto createTask =
		    Original<Method<P, const TClass*, P>>("?CreateTask@TScheduler@Toshi@@QAEPAVTTask@2@ABVTClass@2@PAV32@@Z");
		const auto destroyTask = Original<Method<void, P>>("?DestroyTask@TScheduler@Toshi@@QAEXAAVTTask@2@@Z");
		const auto destroyAll = Original<Method<void>>("?DestroyAllTasks@TScheduler@Toshi@@QAEXXZ");
		const auto update = Original<Method<void>>("?Update@TScheduler@Toshi@@QAEXXZ");
		const auto updatePaused = Original<Method<void>>("?UpdateKernelPaused@TScheduler@Toshi@@QAEXXZ");
		const auto taskCreate = Original<Method<bool>>("?Create@TTask@Toshi@@UAE_NXZ");
		const auto taskCreateFailed = Original<Method<bool>>("?CreateFailed@TTask@Toshi@@UAE_NXZ");
		const auto activate = Original<Method<void, bool>>("?Activate@TTask@Toshi@@QAEX_N@Z");

		static TClass originalClass("TOriginalTask", &TTask::m_sClass, CreateOriginalTask, nullptr, nullptr, nullptr,
		                            1);
		static TClass ourClass("TRecordTask2", &TTask::m_sClass, TRecordTask::CreateRecord, nullptr, nullptr, nullptr,
		                       1);

		InputGenerator gen;
		for (int round = 0; round < 60; ++round)
		{
			// One fake kernel: only its timer delta (offset 0x28) and pause flag are read.
			alignas(TKernelInterface) unsigned char kernel[sizeof(TKernelInterface)] = {};
			float& delta = *reinterpret_cast<float*>(kernel + 0x28);
			alignas(void*) unsigned char original[sizeof(TScheduler)];
			construct(original, kernel);
			reinterpret_cast<SchedulerView*>(original)->refCount = 1; // our reference: never deleted
			auto* ours = new TScheduler(reinterpret_cast<TKernelInterface*>(kernel));
			ours->IncRefCount(nullptr);
			std::vector<void*> tasks[2]; // by id, null once deleted

			for (int step = 0; step < 80; ++step)
			{
				INFO("round ", round, " step ", step);
				const int op = static_cast<int>(gen.NextU32() % 10);
				const int pick = static_cast<int>(gen.NextU32());
				const bool flag = gen.NextU32() % 2 == 0;
				const float time = gen.NextInt(-2, 12) * 0.125f;
				const bool destroyAllNow = gen.NextU32() % 8 == 0;
				std::vector<std::string> logs[2];
				std::string trees[2];
				for (int side = 0; side < 2; ++side)
				{
					s_side = side;
					s_log.clear();
					void* scheduler = side == 0 ? static_cast<void*>(original) : static_cast<void*>(ours);
					// Live task with id `pick` modulo the count (the same id on both sides).
					std::vector<int> live;
					for (int id = 0; id < static_cast<int>(tasks[side].size()); ++id)
						if (tasks[side][id] != nullptr)
							live.push_back(id);
					void* target = live.empty() ? nullptr : tasks[side][live[pick % live.size()]];
					s_nextId = static_cast<int>(tasks[side].size());
					switch (op)
					{
					case 0:
					case 1:
					case 2:
					{
						void* parent = flag ? target : nullptr;
						void* task = side == 0 ? createTask(original, &originalClass, parent)
						                       : ours->CreateTask(ourClass, static_cast<TTask*>(parent));
						tasks[side].push_back(task);
						if (pick % 4 != 0)
						{
							if (side == 0)
								taskCreate(task);
							else
								static_cast<TTask*>(task)->Create();
						}
						else if (pick % 8 == 4)
						{
							if (side == 0)
								taskCreateFailed(task);
							else
								static_cast<TTask*>(task)->CreateFailed();
						}
						break;
					}
					case 3:
						if (target != nullptr)
						{
							if (side == 0)
								activate(target, flag);
							else
								static_cast<TTask*>(target)->Activate(flag);
						}
						break;
					case 4:
						if (target != nullptr)
						{
							if (side == 0)
								destroyTask(original, target);
							else
								ours->DestroyTask(*static_cast<TTask*>(target));
						}
						break;
					case 5:
						if (side == 0)
							updatePaused(original);
						else
							ours->UpdateKernelPaused();
						break;
					case 6:
						if (destroyAllNow)
						{
							if (side == 0)
								destroyAll(original);
							else
								ours->DestroyAllTasks();
						}
						break;
					default:
						delta = time;
						if (side == 0)
							update(original);
						else
							ours->Update();
						break;
					}
					for (const std::string& entry : s_log)
						if (entry.size() > 8 && entry.compare(entry.size() - 8, 8, ":deleted") == 0)
						{
							const int id = std::stoi(entry);
							if (id >= 0 && id < static_cast<int>(tasks[side].size()))
								tasks[side][id] = nullptr;
						}
					logs[side] = s_log;
					trees[side] = DescribeScheduler(scheduler);
				}
				if (logs[0] != logs[1] || trees[0] != trees[1])
				{
					std::string a, b;
					for (const auto& e : logs[0])
						a += e + "; ";
					for (const auto& e : logs[1])
						b += e + "; ";
					MESSAGE("op ", op, " pick ", pick, " flag ", flag, " | original: ", a, " | ours: ", b,
					        " | original tree: ", trees[0], " | ours tree: ", trees[1]);
				}
				CHECK(logs[0] == logs[1]);
				REQUIRE(trees[0] == trees[1]);
			}
			s_log.clear();
			s_side = 0;
			destruct(original);
			s_side = 1;
			delete ours;
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
