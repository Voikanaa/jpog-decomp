#pragma once

#include "common/layout.h"
#include "toshi/kernel/strings/TCString.h"

namespace Toshi
{

class TKernelInterface;

// Application base class (TApplication.dll): Create() builds the kernel and calls OnCreate,
// Execute() runs the main loop (kernel update, then OnUpdate with the frame delta) until
// OnUpdate returns false, then calls OnDestroy.
// Declaration only so far; implemented by the original DLL in the x86 build. The virtual
// order matches the original vtable: destructor, OnCreate, OnUpdate, OnDestroy.
class TApplication
{
public:
	TApplication();
	virtual ~TApplication();
	virtual bool OnCreate(int argc, char** const argv);
	virtual bool OnUpdate(float deltaTime);
	virtual bool OnDestroy();

	bool Create(const char* name, int argc, char** const argv);
	bool Execute();
	void Destroy();
	TKernelInterface* GetKernel();

private:
	TKernelInterface* m_pKernel;
	TCString m_oName;
	unsigned int m_uiFlags;
	bool m_bVerbose;
	bool m_bShowConsole;
	void* m_pConsoleWindow;
};

JPOG_ASSERT_SIZE(TApplication, 0x1C);

} // namespace Toshi
