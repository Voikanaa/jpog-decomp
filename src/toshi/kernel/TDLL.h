#pragma once

#include "common/layout.h"
#include "toshi/kernel/strings/TCString.h"

namespace Toshi
{

class TKernelInterface;

// A loaded module (TKernelInterface.dll): LoadLibrary/FreeLibrary with the file name kept.
class TDLL
{
public:
	TDLL();
	TDLL(const TDLL& other);
	virtual ~TDLL();
	// The original takes a non-const reference.
	TDLL& operator=(TDLL& other);

	// Unloads any previous module first.
	bool Load(const TCString& fileName);
	void Unload();
	const void* GetDLL();
	const TCString& GetFilename();
	void* GetAddress(const TCString& name);

private:
	TCString m_oFileName;
	void* m_hModule;
};

JPOG_ASSERT_SIZE(TDLL, 0x10);

// An engine module loaded by TKernelInterface::LoadInterface, linked into its module list.
class TKernelInterfaceDLL : public TDLL
{
	friend class TKernelInterface;

public:
	TKernelInterfaceDLL();
	TKernelInterfaceDLL(const TKernelInterfaceDLL& other);
	~TKernelInterfaceDLL() override;
	TKernelInterfaceDLL& operator=(TKernelInterfaceDLL& other);

private:
	// List links; they point at the modules (the list head appears as a module 0x10 bytes before
	// the kernel's list), as in the original.
	void* m_pList;
	TKernelInterfaceDLL* m_pNext;
	TKernelInterfaceDLL* m_pPrev;
};

JPOG_ASSERT_SIZE(TKernelInterfaceDLL, 0x1C);

} // namespace Toshi
