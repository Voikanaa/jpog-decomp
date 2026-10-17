#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TGenericFifo.h"
#include "toshi/kernel/TGenericPool.h"
#include "toshi/kernel/TPile.h"

#include <cstring>
#include <doctest/doctest.h>
#include <string>
#include <vector>

using Toshi::TGenericFifo;
using Toshi::TGenericPool;
using Toshi::TPile;
using namespace jpog::test;

namespace
{

// Exposes the protected pool interface.
struct TTestPool : TGenericPool
{
	TTestPool(int itemSize, int alignment, int blockSize, int maxSize)
	    : TGenericPool(itemSize, alignment, blockSize, maxSize)
	{
	}
	using TGenericPool::Alloc;
	using TGenericPool::Free;
	using TGenericPool::Reinit;
	using TGenericPool::Reset;
};

// Raw views (x86 layouts), the same for the original's objects and ours.
struct PileView
{
	int blockSize, maxBlocks;
	unsigned int flags;
	int numBlocks, blockIndex;
	unsigned char *cursor, *blockEnd;
	void** firstBlock;
	void** currentBlock;
	int numAllocs, wasted;
};

struct PoolView
{
	int itemSize, alignment, blockSize, maxSize, totalSize;
	void** blocks;
	void** freeList;
};

// "block:offset" of `memory` among the `blockBytes`-sized blocks linked from `first` (blocks link through their
// first word), or "-" for null.
std::string Locate(void** first, const void* memory, int blockBytes)
{
	if (memory == nullptr)
		return "-";
	int index = 0;
	for (void** block = first; block != nullptr; block = static_cast<void**>(*block), ++index)
	{
		const auto* start = reinterpret_cast<const unsigned char*>(block);
		const auto* address = static_cast<const unsigned char*>(memory);
		if (address >= start && address <= start + blockBytes)
			return std::to_string(index) + ":" + std::to_string(address - start);
	}
	return "?";
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TPool")
{
	TEST_CASE("pile")
	{
		TPile pile(256, 1024, 0);
		void* a = pile.RawAlloc(10, 4);
		void* b = pile.RawAlloc(10, 8);
		CHECK(a != nullptr);
		CHECK(static_cast<unsigned char*>(b) >= static_cast<unsigned char*>(a) + 10);
		CHECK(pile.RawAlloc(250, 4) != nullptr); // second block
		CHECK(pile.GetNumBlocks() == 2);
		CHECK(pile.GetNumAllocs() == 3);
		pile.Reset();
		CHECK(pile.RawAlloc(10, 4) == a); // blocks are reused
		pile.Free();
		CHECK(pile.GetNumBlocks() == 0);
	}

	TEST_CASE("pool")
	{
		TTestPool pool(12, 8, 64, 128);
		std::vector<void*> items;
		for (void* item = pool.Alloc(); item != nullptr; item = pool.Alloc())
			items.push_back(item);
		CHECK(items.size() == 8); // two blocks of 64 / 16
		pool.Free(items[3]);
		CHECK(pool.Alloc() == items[3]);
		pool.Reinit();
	}

	TEST_CASE("fifo")
	{
		int buffer[4];
		TGenericFifo fifo;
		fifo.Create(buffer, 4, sizeof(int));
		for (int i = 0; i < 4; ++i)
			CHECK(fifo.Push(&i, 1));
		const int extra = 9;
		CHECK_FALSE(fifo.Push(&extra, 1)); // full
		int value = -1;
		CHECK(fifo.Pop(&value, 1));
		CHECK(value == 0);
		fifo.Destroy();
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

TEST_SUITE("TPool differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("pile allocations match the original")
	{
		const auto construct = Original<Method<void, int, int, unsigned int>>("??0TPile@Toshi@@QAE@HHI@Z");
		const auto rawAlloc = Original<Method<void*, int, int>>("?RawAlloc@TPile@Toshi@@QAEPAXHH@Z");
		const auto isInside = Original<Method<bool, void*>>("?RawIsInside@TPile@Toshi@@QAE_NPAX@Z");
		const auto reset = Original<Method<void>>("?Reset@TPile@Toshi@@QAEXXZ");
		const auto release = Original<Method<void>>("?Free@TPile@Toshi@@QAEXXZ");

		InputGenerator gen;
		for (int round = 0; round < 100; ++round)
		{
			const int blockSize = gen.NextInt(16, 600);
			const int maxSize = gen.NextU32() % 3 == 0 ? 0 : blockSize * gen.NextInt(1, 6);
			const unsigned int flags = gen.NextU32() % 2;
			alignas(void*) unsigned char original[sizeof(TPile)];
			construct(original, blockSize, maxSize, flags);
			TPile ours(blockSize, maxSize, flags);
			auto& a = *reinterpret_cast<PileView*>(original);
			auto& b = *reinterpret_cast<PileView*>(&ours);
			for (int step = 0; step < 200; ++step)
			{
				INFO("round ", round, " step ", step);
				const int choice = static_cast<int>(gen.NextU32() % 20);
				if (choice == 0)
				{
					reset(original);
					ours.Reset();
				}
				else if (choice == 1)
				{
					release(original);
					ours.Free();
				}
				else
				{
					const int size = gen.NextInt(0, blockSize);
					// Blocks are 16-aligned (32 with flag 1), so larger alignments would depend on
					// the block addresses.
					const int alignment = 1 << gen.NextInt(0, flags != 0 ? 5 : 4);
					void* x = rawAlloc(original, size, alignment);
					void* y = ours.RawAlloc(size, alignment);
					CHECK(Locate(a.firstBlock, x, blockSize + 4) == Locate(b.firstBlock, y, blockSize + 4));
					if (x != nullptr && y != nullptr)
						CHECK(isInside(original, x) == ours.RawIsInside(y));
				}
				CHECK(a.numBlocks == b.numBlocks);
				CHECK(a.blockIndex == b.blockIndex);
				CHECK(a.numAllocs == b.numAllocs);
				// The cursor may overshoot the block (failed allocations), so compare it to the block end.
				CHECK((a.cursor == nullptr) == (b.cursor == nullptr));
				CHECK(a.cursor - a.blockEnd == b.cursor - b.blockEnd);
				CHECK(Locate(a.firstBlock, a.blockEnd, blockSize + 4) ==
				      Locate(b.firstBlock, b.blockEnd, blockSize + 4));
			}
			release(original);
			ours.Free();
		}
	}

	TEST_CASE("pool allocations match the original")
	{
		const auto construct = Original<Method<void, int, int, int, int>>("??0TGenericPool@Toshi@@IAE@HHHH@Z");
		const auto destruct = Original<Method<void>>("??1TGenericPool@Toshi@@IAE@XZ");
		const auto alloc = Original<Method<void*>>("?Alloc@TGenericPool@Toshi@@IAEPAXXZ");
		const auto release = Original<Method<void, void*>>("?Free@TGenericPool@Toshi@@IAEXPAX@Z");
		const auto reset = Original<Method<void>>("?Reset@TGenericPool@Toshi@@IAEXXZ");

		InputGenerator gen;
		for (int round = 0; round < 100; ++round)
		{
			const int itemSize = gen.NextInt(1, 40);
			const int alignment = 1 << gen.NextInt(0, 4);
			const int blockSize = gen.NextInt(64, 400);
			const int maxSize = gen.NextU32() % 2 == 0 ? 0 : blockSize * gen.NextInt(1, 4);
			alignas(void*) unsigned char original[sizeof(TGenericPool)];
			construct(original, itemSize, alignment, blockSize, maxSize);
			TTestPool ours(itemSize, alignment, blockSize, maxSize);
			const auto& a = *reinterpret_cast<PoolView*>(original);
			const auto& b = *reinterpret_cast<PoolView*>(&ours);
			REQUIRE(std::memcmp(&a, &b, 5 * sizeof(int)) == 0);
			std::vector<void*> originalItems, ourItems;
			for (int step = 0; step < 300; ++step)
			{
				INFO("round ", round, " step ", step);
				const int choice = static_cast<int>(gen.NextU32() % 10);
				if (choice < 6 || originalItems.empty())
				{
					void* x = alloc(original);
					void* y = ours.Alloc();
					CHECK(Locate(a.blocks, x, a.alignment + a.blockSize) ==
					      Locate(b.blocks, y, a.alignment + a.blockSize));
					if (x != nullptr && y != nullptr)
					{
						originalItems.push_back(x);
						ourItems.push_back(y);
					}
				}
				else if (choice < 9)
				{
					const std::size_t index = gen.NextU32() % originalItems.size();
					release(original, originalItems[index]);
					ours.Free(ourItems[index]);
					originalItems.erase(originalItems.begin() + static_cast<std::ptrdiff_t>(index));
					ourItems.erase(ourItems.begin() + static_cast<std::ptrdiff_t>(index));
				}
				else
				{
					reset(original);
					ours.Reset();
					originalItems.clear();
					ourItems.clear();
				}
				CHECK(a.totalSize == b.totalSize);
				CHECK(Locate(a.blocks, a.freeList, a.alignment + a.blockSize) ==
				      Locate(b.blocks, b.freeList, a.alignment + a.blockSize));
			}
			destruct(original);
		}
	}

	TEST_CASE("fifo matches the original")
	{
		const auto create = Original<Method<bool, void*, int, int>>("?Create@TGenericFifo@Toshi@@QAE_NPAXHH@Z");
		const auto destroy = Original<Method<bool>>("?Destroy@TGenericFifo@Toshi@@QAE_NXZ");
		const auto push = Original<Method<bool, const void*, unsigned int>>("?Push@TGenericFifo@Toshi@@QAE_NPBXI@Z");
		const auto pop = Original<Method<bool, void*, unsigned int>>("?Pop@TGenericFifo@Toshi@@QAE_NPAXI@Z");

		InputGenerator gen;
		for (int round = 0; round < 50; ++round)
		{
			const int capacity = gen.NextInt(1, 8);
			const int itemSize = gen.NextInt(1, 12);
			std::vector<unsigned char> originalBuffer(static_cast<std::size_t>(capacity * itemSize));
			std::vector<unsigned char> ourBuffer(originalBuffer.size());
			alignas(void*) unsigned char original[sizeof(TGenericFifo)];
			TGenericFifo ours;
			CHECK(create(original, originalBuffer.data(), capacity, itemSize) ==
			      ours.Create(ourBuffer.data(), capacity, itemSize));
			for (int step = 0; step < 100; ++step)
			{
				unsigned char item[12], x[12] = {}, y[12] = {};
				for (unsigned char& byte : item)
					byte = static_cast<unsigned char>(gen.NextU32());
				if (gen.NextU32() % 2 == 0)
					CHECK(push(original, item, 1) == ours.Push(item, 1));
				else
				{
					CHECK(pop(original, x, 1) == ours.Pop(y, 1));
					CHECK(std::memcmp(x, y, sizeof(x)) == 0);
				}
			}
			CHECK(originalBuffer == ourBuffer);
			destroy(original);
			ours.Destroy();
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
