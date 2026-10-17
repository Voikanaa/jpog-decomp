#include "toshi/kernel/TMemory.h"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <mutex>
#include <new>

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
int s_dumpCount = 0;

// Allocator for the statistics map. It must bypass operator new, which itself allocates
// through TMemory (see the global operators at the end of this file).
template <typename T> struct SystemAllocator
{
	using value_type = T;
	SystemAllocator() = default;
	template <typename U> SystemAllocator(const SystemAllocator<U>&) {}
	T* allocate(std::size_t count) { return static_cast<T*>(std::malloc(count * sizeof(T))); }
	void deallocate(T* memory, std::size_t) { std::free(memory); }
	template <typename U> bool operator==(const SystemAllocator<U>&) const { return true; }
};

// Live allocation statistics for GetMemInfo.
struct Statistics
{
	std::mutex mutex;
	// size -> number of live allocations
	std::map<unsigned int, unsigned int, std::less<>, SystemAllocator<std::pair<const unsigned int, unsigned int>>>
	    sizes;
	unsigned int count = 0;
	unsigned int bytes = 0;
};

// Never destroyed: static objects (such as free lists) may still free memory during exit.
Statistics& Stats()
{
	static Statistics* stats = new (std::malloc(sizeof(Statistics))) Statistics;
	return *stats;
}

// Stored just before every aligned allocation.
struct Header
{
	void* base;
	unsigned int size;
};

Header& HeaderOf(void* memory)
{
	return reinterpret_cast<Header*>(memory)[-1];
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
	if (alignment < alignof(Header))
		alignment = alignof(Header);
	size = (size + 3) & ~3u; // the original rounds sizes to 4 bytes

	const std::size_t total = std::size_t{size} + alignment + sizeof(Header);
	void* base = std::malloc(total);
	if (base == nullptr)
		return nullptr;
	void* aligned = static_cast<char*>(base) + sizeof(Header);
	std::size_t space = total - sizeof(Header);
	aligned = std::align(alignment, size, aligned, space);
	HeaderOf(aligned) = {base, size};

	Statistics& stats = Stats();
	const std::lock_guard lock(stats.mutex);
	++stats.sizes[size];
	++stats.count;
	stats.bytes += size;
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
	const Header header = HeaderOf(memory);
	{
		Statistics& stats = Stats();
		const std::lock_guard lock(stats.mutex);
		const auto it = stats.sizes.find(header.size);
		if (it != stats.sizes.end() && --it->second == 0)
			stats.sizes.erase(it);
		--stats.count;
		stats.bytes -= header.size;
	}
	std::free(header.base);
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
	static TMemory& manager = *new (std::malloc(sizeof(TMemory))) TMemory; // never destroyed
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

void TSTDCALL TMemory::GetMemInfo(MemInfo& info, MemBlock*)
{
	Initialise();
	Statistics& stats = Stats();
	const std::lock_guard lock(stats.mutex);
	info = {};
	info.totalSize = stats.bytes;
	info.totalUsed = stats.bytes;
	info.numProcesses = stats.count;
	info.smallestProcess = 0xFFFFFFFF;
	if (!stats.sizes.empty())
	{
		info.smallestProcess = stats.sizes.begin()->first;
		info.largestProcess = stats.sizes.rbegin()->first;
	}
}

void TSTDCALL TMemory::GetHALMemInfo(HALMemInfo& info)
{
	info = {};
}

bool TSTDCALL TMemory::TestMemIntegrity(MemBlock*)
{
	Initialise();
	return true;
}

bool TSTDCALL TMemory::DebugTestMemoryBlock(MemBlock* block)
{
	MemInfo info;
	GetMemInfo(info, block);
	void* allocations[200];
	const unsigned int size = info.totalFree / 150 + 8;
	for (int pass = 0; pass < 2; ++pass)
	{
		for (void*& allocation : allocations)
			allocation = tmalloc(static_cast<int>(size), nullptr, -1);
		TestMemIntegrity(nullptr);
		for (void*& allocation : allocations)
		{
			GetMemManager().Free(allocation);
			allocation = nullptr;
		}
		TestMemIntegrity(nullptr);
	}
	return true;
}

unsigned int TSTDCALL TMemory::GetMarkedHALPhysical()
{
	Initialise();
	return 0;
}

void TSTDCALL TMemory::MarkHALPhysical()
{
	Initialise();
}

void TSTDCALL TMemory::DumpMemInfo(MemInfo&)
{
	++s_dumpCount;
}

void TSTDCALL TMemory::DebugPrintFreeMemoryAt(const char*)
{
}

void TSTDCALL TMemory::DebugPrintHALMemInfo(const char*)
{
}

void TSTDCALL TMemory::DebugPrintHALMemInfoToFile(HALMemInfo&, TFile*, const char*)
{
}

void TSTDCALL TMemory::DumpAllocations(MemBlock*, int)
{
}

void TSTDCALL TMemory::DumpAllocationsToFile(TFile*, MemBlock*, int)
{
}

unsigned int TSTDCALL TMemory::GetFreePhysicalMemory()
{
	return 0x8000000;
}

void TSTDCALL TMemory::DumpSKUMemoryInfo()
{
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

// Toshi code allocates with operator new and frees with operator delete, which the original
// inlines as tmalloc/tfree (TObject's deleting destructor calls tfree directly). Objects from
// TClass::CreateObject (tmalloc) are therefore released by delete.
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
