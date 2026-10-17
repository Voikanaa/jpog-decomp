#pragma once

#include "common/abi.h"

// Toshi memory manager (TKernelInterface.dll).
//
// The original is a 32-bit heap with packed node headers that cannot be carried to x64, and no
// other module depends on its internal layout (they only use tmalloc/tfree, TFreeList and a few
// statistics functions). This reimplementation keeps the public API and its observable behaviour
// (minimum size, alignment, zeroing, failure cases) on top of the C runtime heap. See
// docs/plan.md ("Memory").

#include "common/layout.h"

namespace Toshi
{

class TFile;

class TMemory
{
public:
	// Heap regions and nodes of the original allocator. Opaque here: callers only pass them back.
	class MemBlock;
	class MemNode;

	// Heap statistics. Sizes are in bytes; "holes" are free regions and "processes" allocations.
	// On this heap there are no holes: only live allocations are reported.
	struct MemInfo
	{
		unsigned int totalSize;
		unsigned int totalFree;
		unsigned int totalUsed;
		unsigned int numHoles;
		unsigned int numProcesses;
		unsigned int largestHole;
		unsigned int largestProcess;
		unsigned int smallestHole;    // 0 when there are no holes
		unsigned int smallestProcess; // 0xFFFFFFFF when there are no allocations
	};

	// Platform memory statistics; always zero on PC (the original only clears it).
	struct HALMemInfo
	{
		unsigned int values[21];
	};

	TMemory();
	~TMemory();

	// Allocates `size` bytes (at least 8) aligned to `alignment` (a power of two, at least 4;
	// smaller values fail and return null). `block`, `file` and `line` are accepted for API
	// compatibility.
	void* Alloc(unsigned int size, unsigned int alignment, MemBlock* block, char* file, int line);
	// Alloc, then zero the `size` bytes.
	void* Calloc(unsigned int size, unsigned int alignment, MemBlock* block, char* file, int line);
	// Frees memory from Alloc/Calloc. Returns false (and does nothing) for null or pointers that
	// are not 4-byte aligned.
	bool Free(void* memory);
	// Unimplemented in the original: both always return null and leave `memory` untouched.
	void* Realloc(void* memory, unsigned int size, MemBlock* block);
	void* Recalloc(void* memory, unsigned int size, MemBlock* block);

	static TMemory& TSTDCALL GetMemManager();
	static MemBlock* TSTDCALL GetGlobalBlock();
	static bool TSTDCALL IsInitialised();
	static bool TSTDCALL IsDebug();
	// Zero-initialised allocation straight from the system (GlobalAlloc in the original).
	static void* TSTDCALL SysAlloc(unsigned int size);
	static void TSTDCALL SysFree(void* memory);

	static void TSTDCALL GetMemInfo(MemInfo& info, MemBlock* block);
	static void TSTDCALL GetHALMemInfo(HALMemInfo& info);
	// Always true here: the heap is the C runtime's.
	static bool TSTDCALL TestMemIntegrity(MemBlock* block);
	// Allocation/free stress test with integrity checks (returns true, as the original).
	static bool TSTDCALL DebugTestMemoryBlock(MemBlock* block);
	static unsigned int TSTDCALL GetMarkedHALPhysical();
	static void TSTDCALL MarkHALPhysical();
	// Debug output; empty in the shipped original except for DumpMemInfo's dump counter.
	static void TSTDCALL DumpMemInfo(MemInfo& info);
	static void TSTDCALL DebugPrintFreeMemoryAt(const char* label);
	static void TSTDCALL DebugPrintHALMemInfo(const char* label);
	static void TSTDCALL DebugPrintHALMemInfoToFile(HALMemInfo& info, TFile* file, const char* label);
	static void TSTDCALL DumpAllocations(MemBlock* block, int flags);
	static void TSTDCALL DumpAllocationsToFile(TFile* file, MemBlock* block, int flags);

protected:
	static bool TSTDCALL Initialise();
	// A fixed 128 MB in the original.
	static unsigned int TSTDCALL GetFreePhysicalMemory();
	static void TSTDCALL DumpSKUMemoryInfo();
	void Deinitialise();

private:
	static bool m_bInitialised;
	static bool m_bIsDebug;
};

JPOG_ASSERT_SIZE(TMemory::MemInfo, 0x24);
JPOG_ASSERT_SIZE(TMemory::HALMemInfo, 0x54);

} // namespace Toshi

void* TSTDCALL tmalloc(int size, char* file, int line);
void* TSTDCALL tcalloc(int count, int size);
void* TSTDCALL tmemalign(int alignment, int size);
void* TSTDCALL trealloc(void* memory, int size);
void TSTDCALL tfree(void* memory);
