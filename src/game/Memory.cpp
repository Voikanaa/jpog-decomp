// Like SimJP.exe, the game allocates through the kernel heap: global new/delete go to the
// kernel's tmalloc/tfree, so objects can be created by the kernel (TClass::CreateObject) and
// deleted by the game or the other way round.
#include "toshi/kernel/TMemory.h"

#include <cstddef>

void* operator new(std::size_t size)
{
	return tmalloc(static_cast<int>(size), nullptr, -1);
}

void* operator new[](std::size_t size)
{
	return tmalloc(static_cast<int>(size), nullptr, -1);
}

void operator delete(void* memory) noexcept
{
	tfree(memory);
}

void operator delete[](void* memory) noexcept
{
	tfree(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
	tfree(memory);
}

void operator delete[](void* memory, std::size_t) noexcept
{
	tfree(memory);
}
