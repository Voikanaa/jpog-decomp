#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/memory/TFreeList.h"
#include "toshi/kernel/memory/TMemory.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <doctest/doctest.h>
#include <vector>

using Toshi::TFreeList;
using Toshi::TMemory;
using namespace jpog::test;

namespace
{

bool IsAligned(const void* memory, std::size_t alignment)
{
	return reinterpret_cast<std::uintptr_t>(memory) % alignment == 0;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TMemory")
{
	TEST_CASE("allocation contract")
	{
		TMemory& memory = TMemory::GetMemManager();
		CHECK(TMemory::IsInitialised());
		for (unsigned int alignment : {4u, 8u, 16u, 64u, 256u})
		{
			void* block = memory.Alloc(3, alignment, TMemory::GetGlobalBlock(), nullptr, -1);
			REQUIRE(block != nullptr);
			CHECK(IsAligned(block, alignment));
			std::memset(block, 0xAB, 8); // the minimum size is 8 bytes
			CHECK(memory.Free(block));
		}
		CHECK(memory.Alloc(16, 2, nullptr, nullptr, -1) == nullptr);
		CHECK_FALSE(memory.Free(nullptr));

		auto* zeroed = static_cast<unsigned char*>(tcalloc(7, 9));
		REQUIRE(zeroed != nullptr);
		CHECK(IsAligned(zeroed, 16));
		for (int i = 0; i < 63; ++i)
			CHECK(zeroed[i] == 0);
		tfree(zeroed);

		void* block = tmalloc(32, nullptr, -1);
		CHECK(trealloc(block, 64) == nullptr); // unimplemented in the original
		tfree(block);
	}

	TEST_CASE("statistics track live allocations")
	{
		TMemory::MemInfo before;
		TMemory::GetMemInfo(before, nullptr);
		void* a = tmalloc(13, nullptr, -1); // rounded to 16
		void* b = tmalloc(1000, nullptr, -1);
		TMemory::MemInfo during;
		TMemory::GetMemInfo(during, nullptr);
		CHECK(during.numProcesses == before.numProcesses + 2);
		CHECK(during.totalUsed == before.totalUsed + 16 + 1000);
		CHECK(during.largestProcess >= 1000);
		CHECK(during.smallestProcess <= 16);
		CHECK(during.numHoles == 0);
		tfree(a);
		tfree(b);
		TMemory::MemInfo after;
		TMemory::GetMemInfo(after, nullptr);
		CHECK(after.numProcesses == before.numProcesses);
		CHECK(after.totalUsed == before.totalUsed);
		CHECK(TMemory::DebugTestMemoryBlock(nullptr));
		CHECK(TMemory::TestMemIntegrity(nullptr));
	}

	TEST_CASE("TFreeList reuses items")
	{
		TFreeList list(24, 4, 0, nullptr);
		CHECK(list.GetCapacity() == 4);
		CHECK(list.GetGrowSize() == 8);
		std::vector<void*> items;
		for (int i = 0; i < 4; ++i)
			items.push_back(list.New(24));
		CHECK(list.GetCapacity() == 4);
		list.Delete(items[1]);
		CHECK(list.New(24) == items[1]);
		list.New(24); // grows by GetGrowSize()
		CHECK(list.GetCapacity() == 12);
		void* other = list.New(100); // other sizes come from tmalloc
		tfree(other);
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

// Field layout shared by both implementations (asserted in TFreeList.h).
struct FreeListView
{
	int capacity;
	int itemSize;
	TFreeList::Node* freeList;
	TFreeList::Node* blockList;
	int growSize;
};

// Position of an item: (block number counted from the oldest block, slot within the block). The
// owning block is the nearest block start at or below the item: no other block begins inside it.
std::pair<int, int> Locate(const FreeListView& view, const void* item)
{
	std::vector<const char*> starts;
	for (const TFreeList::Node* block = view.blockList; block != nullptr; block = block->next)
		starts.push_back(reinterpret_cast<const char*>(block + 1));
	const auto* address = static_cast<const char*>(item);
	int best = -1;
	for (int i = 0; i < static_cast<int>(starts.size()); ++i)
		if (starts[i] <= address && (best < 0 || starts[i] > starts[best]))
			best = i;
	if (best < 0)
		return {-1, -1};
	const std::ptrdiff_t offset = address - starts[best];
	if (offset % view.itemSize != 0)
		return {-2, -2};
	return {static_cast<int>(starts.size()) - 1 - best, static_cast<int>(offset / view.itemSize)};
}

std::vector<std::pair<int, int>> FreeChain(const FreeListView& view)
{
	std::vector<std::pair<int, int>> chain;
	for (const TFreeList::Node* node = view.freeList; node != nullptr && chain.size() < 4096; node = node->next)
		chain.push_back(Locate(view, node));
	return chain;
}

} // namespace

TEST_SUITE("TMemory differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("TMemory failure cases")
	{
		using GetManager = TMemory*(__stdcall*)();
		auto* original = Original<GetManager>("?GetMemManager@TMemory@Toshi@@SGAAV12@XZ")();
		const auto free = Original<Method<bool, void*>>("?Free@TMemory@Toshi@@QAE_NPAX@Z");
		const auto realloc = Original<Method<void*, void*, unsigned int, TMemory::MemBlock*>>(
		    "?Realloc@TMemory@Toshi@@QAEPAXPAXIPAVMemBlock@12@@Z");
		char buffer[16] = {};
		CHECK(free(original, nullptr) == TMemory::GetMemManager().Free(nullptr));
		CHECK(free(original, buffer + 1) == TMemory::GetMemManager().Free(buffer + 1));
		CHECK(realloc(original, buffer, 64, nullptr) == TMemory::GetMemManager().Realloc(buffer, 64, nullptr));

		using HalFn = void(__stdcall*)(TMemory::HALMemInfo*);
		TMemory::HALMemInfo originalHal, ourHal;
		std::memset(&originalHal, 0xCD, sizeof(originalHal));
		std::memset(&ourHal, 0xCD, sizeof(ourHal));
		Original<HalFn>("?GetHALMemInfo@TMemory@Toshi@@SGXAAUHALMemInfo@12@@Z")(&originalHal);
		TMemory::GetHALMemInfo(ourHal);
		CHECK(std::memcmp(&originalHal, &ourHal, sizeof(ourHal)) == 0);
		CHECK(Original<unsigned int(__stdcall*)()>("?GetMarkedHALPhysical@TMemory@Toshi@@SGIXZ")() ==
		      TMemory::GetMarkedHALPhysical());
		CHECK(Original<bool(__stdcall*)(TMemory::MemBlock*)>("?TestMemIntegrity@TMemory@Toshi@@SG_NPAVMemBlock@12@@Z")(
		          nullptr) == TMemory::TestMemIntegrity(nullptr));
	}

	TEST_CASE("TFreeList behaves like the original")
	{
		using Construct = Method<void, int, int, int, char*>;
		const auto construct = Original<Construct>("??0TFreeList@Toshi@@QAE@HHHPAD@Z");
		const auto destruct = Original<Method<void>>("??1TFreeList@Toshi@@QAE@XZ");
		const auto newItem = Original<Method<void*, unsigned int>>("?New@TFreeList@Toshi@@QAEPAXI@Z");
		const auto deleteItem = Original<Method<void, void*>>("?Delete@TFreeList@Toshi@@QAEXPAX@Z");
		const auto setCapacity = Original<Method<void, int>>("?SetCapacity@TFreeList@Toshi@@QAEXH@Z");
		const auto setGrowSize = Original<Method<void, int>>("?SetGrowSize@TFreeList@Toshi@@QAEXH@Z");

		InputGenerator gen;
		for (int round = 0; round < 50; ++round)
		{
			const int itemSize = 4 * gen.NextInt(1, 16);
			const int capacity = gen.NextInt(-2, 20);
			const int grow = gen.NextInt(-1, 10);
			alignas(TFreeList) unsigned char storage[sizeof(TFreeList)];
			construct(storage, itemSize, capacity, grow, nullptr);
			auto& original = *reinterpret_cast<FreeListView*>(storage);
			TFreeList list(itemSize, capacity, grow, nullptr);
			auto& ours = *reinterpret_cast<FreeListView*>(&list);

			std::vector<void*> liveOriginal, liveOurs;
			for (int step = 0; step < 200; ++step)
			{
				const int action = gen.NextInt(0, 9);
				if (action < 5 || liveOriginal.empty())
				{
					liveOriginal.push_back(newItem(storage, static_cast<unsigned int>(itemSize)));
					liveOurs.push_back(list.New(static_cast<unsigned int>(itemSize)));
					CHECK(Locate(original, liveOriginal.back()) == Locate(ours, liveOurs.back()));
				}
				else if (action < 9)
				{
					const std::size_t index = gen.NextU32() % liveOriginal.size();
					deleteItem(storage, liveOriginal[index]);
					list.Delete(liveOurs[index]);
					liveOriginal.erase(liveOriginal.begin() + static_cast<std::ptrdiff_t>(index));
					liveOurs.erase(liveOurs.begin() + static_cast<std::ptrdiff_t>(index));
				}
				else if (gen.NextU32() & 1)
				{
					const int newCapacity = gen.NextInt(0, original.capacity + 10);
					setCapacity(storage, newCapacity);
					list.SetCapacity(newCapacity);
				}
				else
				{
					const int newGrow = gen.NextInt(-1, 12);
					setGrowSize(storage, newGrow);
					list.SetGrowSize(newGrow);
				}
				REQUIRE(original.capacity == ours.capacity);
				REQUIRE(original.growSize == ours.growSize);
				REQUIRE(original.itemSize == ours.itemSize);
			}
			CHECK(FreeChain(original) == FreeChain(ours));
			destruct(storage);
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
