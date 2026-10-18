#pragma once

#include "toshi/application/TApplication.h"

class ADummyTask;
class ARootTask;

// The game's application object (SimJP.exe: AApplication, global g_oTheApp). OnCreate prepares
// the engine (string pool, free lists, locales, GUI modules), builds the task tree (five
// ADummyTask groups and ARootTask, which brings up the game) and OnDestroy tears it down.
class AApplication : public Toshi::TApplication
{
public:
	bool OnCreate(int argc, char** const argv) override;
	bool OnDestroy() override;

	ARootTask* GetRootTask() const;
	ADummyTask* GetInputRootTask();
	ADummyTask* GetUpdate1RootTask();
	ADummyTask* GetUpdate2RootTask();
	ADummyTask* GetUpdate3RootTask();
	ADummyTask* GetRenderRootTask();

private:
	ADummyTask* CreateRootTask(char* name);

	ARootTask* m_pRootTask = nullptr;
	ADummyTask* m_pInputTask = nullptr;
	ADummyTask* m_pUpdate1Task = nullptr;
	ADummyTask* m_pUpdate2Task = nullptr;
	ADummyTask* m_pUpdate3Task = nullptr;
	ADummyTask* m_pRenderTask = nullptr;
};

extern AApplication g_oTheApp;
