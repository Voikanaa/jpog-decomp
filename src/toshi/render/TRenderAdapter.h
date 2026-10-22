#pragma once

#include "common/layout.h"
#include "toshi/kernel/containers/TNodeList.h"

namespace Toshi
{

// A display adapter, its modes and each mode's devices (TRenderInterface.dll; TD3DAdapter in
// TRenderD3DInterface.dll implements them). Declaration only: the engine creates them, we
// iterate them. The virtual order matches the original vtables (symbols/vtables.csv). The abstract
// slots have no exported names: ours come from how the engine uses them
// (TRenderInterface::FindDevice) and from what SimJP.exe prints; UnknownN slots are unused.
class TRenderAdapter : public TNodeList<TRenderAdapter>::TNode
{
public:
	class Mode : public TNodeList<Mode>::TNode
	{
	public:
		class Device
		{
		public:
			enum DEPTHSTENCILFORMAT : int
			{
			};

			virtual ~Device();
			virtual Mode* GetMode() const = 0;
			virtual int Unknown2() const = 0;
			virtual const char* GetTypeString() const = 0;
			virtual bool IsSoftware() const = 0;
			virtual bool CanRenderWindowed() const = 0;
			virtual bool SupportsHardwareTransfomations() const = 0;
			virtual bool IsDepthStencilFormatSupported(DEPTHSTENCILFORMAT format) const = 0;
		};

		virtual ~Mode();
		virtual void* Unknown1() const = 0;
		virtual int Unknown2() const = 0;
		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;
		virtual bool Is32Bit() const = 0;
		virtual bool Is16Bit() const = 0;
		virtual unsigned int Unknown7() const = 0; // called by SimJP.exe, result unused
		virtual Device* GetDevice(int index) = 0;
	};

	virtual ~TRenderAdapter();
	virtual void Unknown1() = 0;
	virtual void Unknown2() = 0;
	virtual void Unknown3() = 0;
	virtual void Unknown4() = 0;
	virtual void Unknown5() = 0;
	virtual void Unknown6() = 0;
	virtual void Unknown7() = 0;
	virtual void Unknown8() = 0;
	// Devices per mode (each mode has the same set).
	virtual int GetNumSupportedDevices() = 0;

	TNodeList<Mode>* GetModeList();

private:
	TNodeList<Mode> m_oModes;
};

JPOG_ASSERT_SIZE(TRenderAdapter::Mode, 0x10);
JPOG_ASSERT_SIZE(TRenderAdapter, 0x20);

} // namespace Toshi
