#pragma once

namespace Toshi
{

class TCString;
class TKernelInterfaceDLL;

// High-performance timer (TKernelInterface.dll). Declaration only so far.
class THPTimer
{
public:
	float GetDelta();
};

// Owns the scheduler, timers and loaded engine modules (TKernelInterface.dll).
// Declaration only so far; instances are created by TApplication::Create.
class TKernelInterface
{
public:
	// Loads an engine module ("TRenderD3DInterface", "TGUIInterface", ...) and registers its classes.
	TKernelInterfaceDLL* LoadInterface(const TCString& name);
	bool Update();
	THPTimer* GetSystemTimer();
};

} // namespace Toshi
