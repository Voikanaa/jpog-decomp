#pragma once

#include "common/layout.h"
#include "toshi/kernel/THPTimer.h"
#include "toshi/kernel/TObject.h"
#include "toshi/kernel/TRefCounted.h"

struct HINSTANCE__;

namespace Toshi
{

class TCString;
class TKernelInterfaceDLL;
class TScheduler;

// The engine core (TKernelInterface.dll): sets up the file systems ("local" at the working
// directory, "abs", "null", "mem"), owns the scheduler and the system timer, and loads the
// engine modules. Instances are created by TApplication::Create.
class TKernelInterface : public TObject, public TRefCounted
{
public:
	TKernelInterface(int argc, char** const argv, bool verbose);
	~TKernelInterface() override;

	TClass& GetClass() const override;

	// Samples the timer, tracks the frame rate and updates the scheduler (paused: only tasks'
	// OnUpdateKernelPaused).
	bool Update();
	// Returns the previous state.
	bool Pause(bool paused);
	bool IsPaused();
	// Loads an engine module ("TRenderD3DInterface", "TGUIInterface", ...) once.
	TKernelInterfaceDLL* LoadInterface(const TCString& name);
	TKernelInterfaceDLL* FindInterface(const TCString& name);
	void UnloadInterface(TKernelInterfaceDLL& module);
	void UnloadInterface(const TCString& name);
	// Queries the OS and machine information (prints nothing in the shipped build).
	void DumpInfo();

	THPTimer* GetSystemTimer();
	TScheduler* GetScheduler() const;
	float GetFPS();
	// A running average over whole seconds.
	float GetAverageFPS();
	void SetInstance(HINSTANCE__* instance);
	HINSTANCE__* GetInstance();

	static TClass m_sClass;

private:
	static TObject* TSTDCALL CreateObject();
	static TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	THPTimer m_oSystemTimer;
	TScheduler* m_pScheduler; // holds a reference
	// Module list: head links as in TKernelInterfaceDLL, then the count.
	void* m_pModuleList;
	TKernelInterfaceDLL* m_pFirstModule;
	TKernelInterfaceDLL* m_pLastModule;
	int m_iModuleCount;
	float m_fSecondTime; // time accumulated towards the next whole second
	float m_fAverageFPS;
	bool m_bVerbose;
	HINSTANCE__* m_hInstance;
	bool m_bPaused;
};

JPOG_ASSERT_SIZE(TKernelInterface, 0x58);

} // namespace Toshi
