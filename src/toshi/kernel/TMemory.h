#pragma once

#include "common/abi.h"

// Toshi memory manager (TKernelInterface.dll).
//
// The original is a 32-bit heap with packed node headers that cannot be carried to x64, and no
// other module depends on its internal layout (they only use tmalloc/tfree, TFreeList and a few
// statistics functions). This reimplementation keeps the public API and its observable behaviour
// (minimum size, alignment, zeroing, failure cases) on top of the C runtime heap. See
// docs/plan.md ("Memory").

namespace Toshi
{

class TMemory
{
public:
	// Heap regions and nodes of the original allocator. Opaque here: callers only pass them back.
	class MemBlock;
	class MemNode;

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

protected:
	static bool TSTDCALL Initialise();
	void Deinitialise();

private:
	static bool m_bInitialised;
	static bool m_bIsDebug;
};

} // namespace Toshi

void* TSTDCALL tmalloc(int size, char* file, int line);
void* TSTDCALL tcalloc(int count, int size);
void* TSTDCALL trealloc(void* memory, int size);
void TSTDCALL tfree(void* memory);
