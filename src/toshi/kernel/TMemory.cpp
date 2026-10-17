#include "toshi/kernel/TMemory.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace Toshi
{

bool TMemory::m_bInitialised = false;
bool TMemory::m_bIsDebug = false;

class TMemory::MemBlock
{
};

namespace
{

TMemory::MemBlock s_globalBlock;

// Each allocation stores the pointer returned by malloc just before the aligned address.
void*& BaseSlot(void* memory)
{
	return static_cast<void**>(memory)[-1];
}

bool IsAligned(const void* memory, std::size_t alignment)
{
	return reinterpret_cast<std::uintptr_t>(memory) % alignment == 0;
}

} // namespace

TMemory::TMemory()
{
}

TMemory::~TMemory()
{
}

void* TMemory::Alloc(unsigned int size, unsigned int alignment, MemBlock*, char*, int)
{
	Initialise();
	if (size < 8)
		size = 8;
	if (alignment < 4)
		return nullptr; // the original reports "Out of Toshi memory" and fails
	if (alignment < alignof(void*))
		alignment = alignof(void*);

	const std::size_t total = std::size_t{size} + alignment + sizeof(void*);
	void* base = std::malloc(total);
	if (base == nullptr)
		return nullptr;
	void* aligned = static_cast<char*>(base) + sizeof(void*);
	std::size_t space = total - sizeof(void*);
	aligned = std::align(alignment, size, aligned, space);
	BaseSlot(aligned) = base;
	return aligned;
}

void* TMemory::Calloc(unsigned int size, unsigned int alignment, MemBlock* block, char*, int)
{
	void* memory = Alloc(size, alignment, block, nullptr, -1);
	if (memory != nullptr)
		std::memset(memory, 0, size);
	return memory;
}

bool TMemory::Free(void* memory)
{
	Initialise();
	if (memory == nullptr || !IsAligned(memory, 4))
		return false;
	std::free(BaseSlot(memory));
	return true;
}

void* TMemory::Realloc(void*, unsigned int, MemBlock*)
{
	return nullptr;
}

void* TMemory::Recalloc(void*, unsigned int, MemBlock*)
{
	return nullptr;
}

TMemory& TSTDCALL TMemory::GetMemManager()
{
	static TMemory manager;
	Initialise();
	return manager;
}

TMemory::MemBlock* TSTDCALL TMemory::GetGlobalBlock()
{
	Initialise();
	return &s_globalBlock;
}

bool TSTDCALL TMemory::IsInitialised()
{
	return m_bInitialised;
}

bool TSTDCALL TMemory::IsDebug()
{
	return m_bIsDebug;
}

void* TSTDCALL TMemory::SysAlloc(unsigned int size)
{
	return std::calloc(1, size);
}

void TSTDCALL TMemory::SysFree(void* memory)
{
	std::free(memory);
}

bool TSTDCALL TMemory::Initialise()
{
	m_bInitialised = true;
	return true;
}

void TMemory::Deinitialise()
{
	m_bInitialised = false;
}

} // namespace Toshi

void* TSTDCALL tmalloc(int size, char* file, int line)
{
	return Toshi::TMemory::GetMemManager().Alloc(static_cast<unsigned int>(size), 16, Toshi::TMemory::GetGlobalBlock(),
	                                             file, line);
}

void* TSTDCALL tcalloc(int count, int size)
{
	return Toshi::TMemory::GetMemManager().Calloc(static_cast<unsigned int>(size * count), 16,
	                                              Toshi::TMemory::GetGlobalBlock(), nullptr, -1);
}

void* TSTDCALL trealloc(void* memory, int size)
{
	return Toshi::TMemory::GetMemManager().Realloc(memory, static_cast<unsigned int>(size),
	                                               Toshi::TMemory::GetGlobalBlock());
}

void TSTDCALL tfree(void* memory)
{
	Toshi::TMemory::GetMemManager().Free(memory);
}
