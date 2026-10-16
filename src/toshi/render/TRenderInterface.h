#pragma once

#include "common/layout.h"
#include "toshi/kernel/TObject.h"

namespace Toshi
{

class TKernelInterface;

// Abstract renderer (TRenderInterface.dll); TRenderD3DInterface.dll implements it.
// Declaration only so far. The virtual order matches the original vtable (symbols/vtables.csv);
// only the leading entries we call are declared, since instances are always created by the
// engine (TClass::CreateObject), never by us.
class TRenderInterface : public TObject
{
public:
	class DisplayParams
	{
	public:
		unsigned int uiWidth;
		unsigned int uiHeight;
		unsigned int uiColourDepth;
		unsigned int eDepthStencilFormat;
		bool bWindowed;
	};

	virtual bool CreateDisplay(const DisplayParams* params) = 0;
	virtual bool DestroyDisplay() = 0;
	// Pumps window messages; returns false once the window has been closed.
	virtual bool Update(float deltaTime) = 0;
	virtual bool BeginScene() = 0;
	virtual bool EndScene() = 0;
	virtual void* GetCurrentDevice() = 0;
	virtual const DisplayParams* GetCurrentDisplayParams() = 0;
	virtual bool Create(TKernelInterface* kernel);
	virtual bool Destroy();
};

JPOG_ASSERT_SIZE(TRenderInterface::DisplayParams, 0x14);

} // namespace Toshi
