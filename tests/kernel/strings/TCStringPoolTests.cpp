#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/file/TNativeFile.h"
#include "toshi/kernel/strings/TCStringPool.h"
#include "toshi/kernel/strings/TWStringPool.h"

#include <cstring>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using Toshi::TCString;
using Toshi::TCStringPool;
using Toshi::TFileManager;
using Toshi::TPCString;
using Toshi::TPWString;
using Toshi::TWStringPool;
using namespace jpog::test;

namespace
{

std::string RandomWord(InputGenerator& gen)
{
	static const char* const words[] = {"", "a", "b", "ab", "ba", "dino", "Dino", "park", "rex", "raptor", "gate"};
	return words[gen.NextU32() % (sizeof(words) / sizeof(words[0]))];
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TCStringPool")
{
	TEST_CASE("interning and reference counting")
	{
		TCStringPool pool(4, 0);
		const TPCString a = pool.Get("rex");
		const TPCString b = pool.Get(TCString("rex"));
		CHECK(a == b); // shared
		CHECK(*a == "rex");
		CHECK(pool.IsPresent("rex"));
		CHECK(pool.GetOptional("park").IsEmpty()); // null handle
		CHECK(pool.Get("").IsEmpty());
		CHECK(pool.Get(42) == pool.Get("42"));
		{
			const TPCString temporary = pool.Get("gate");
			CHECK(pool.IsPresent("gate"));
		}
		CHECK_FALSE(pool.IsPresent("gate")); // the last handle removed it
		const TPCString volatileString = pool.GetVolatile("rex");
		CHECK(volatileString != a);
		CHECK(volatileString.GetStringPool() == nullptr);
		CHECK(a.Compare(pool.Get("raptor")) > 0);
		CHECK(TPCString().Compare(a) < 0);
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

struct PooledView
{
	int refCount;
	void* buffer;
	uint32_t lengthAndExcess;
	void* pool;
};

struct EntryView
{
	const char* key;
	PooledView* string;
};

struct PoolView
{
	int growSize;
	int count;
	int capacity;
	EntryView* entries;
};

void CheckSamePool(const PoolView& original, const PoolView& ours)
{
	REQUIRE(original.growSize == ours.growSize);
	REQUIRE(original.count == ours.count);
	REQUIRE(original.capacity == ours.capacity);
	for (int i = 0; i < ours.count; ++i)
	{
		INFO("entry ", i);
		CHECK(std::strcmp(original.entries[i].key, ours.entries[i].key) == 0);
		CHECK(original.entries[i].string->refCount == ours.entries[i].string->refCount);
	}
}

} // namespace

TEST_SUITE("TCStringPool differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("free list parameters")
	{
		const auto* original = static_cast<const int*>(
		    OriginalModule::Kernel().Find("?ms_oFreelist@TPooledCString@Toshi@@0VTFreeList@2@A"));
		REQUIRE(original != nullptr);
		const auto* ours = reinterpret_cast<const int*>(&Toshi::TPooledCString::GetFreeList());
		CHECK(original[1] == ours[1]); // item size
		CHECK(original[4] == ours[4]); // grow size
	}

	TEST_CASE("pool operations match the original")
	{
		using Handle = void*; // a TPCString is one pointer
		const auto construct = Original<Method<void, int, int>>("??0TCStringPool@Toshi@@QAE@HH@Z");
		const auto destruct = Original<Method<void>>("??1TCStringPool@Toshi@@QAE@XZ");
		const auto get =
		    Original<Method<Handle*, Handle*, const char*>>("?Get@TCStringPool@Toshi@@QAE?AVTPCString@2@PBD@Z");
		const auto getInt = Original<Method<Handle*, Handle*, int>>("?Get@TCStringPool@Toshi@@QAE?AVTPCString@2@H@Z");
		const auto getOptional =
		    Original<Method<Handle*, Handle*, const char*>>("?GetOptional@TCStringPool@Toshi@@QAE?AVTPCString@2@PBD@Z");
		const auto getVolatile =
		    Original<Method<Handle*, Handle*, const char*>>("?GetVolatile@TCStringPool@Toshi@@QAE?AVTPCString@2@PBD@Z");
		const auto isPresent = Original<Method<bool, const char*>>("?IsPresent@TCStringPool@Toshi@@QAE_NPBD@Z");
		const auto release = Original<Method<void>>("??1TPCString@Toshi@@QAE@XZ");
		const auto compare = Original<Method<int, const Handle*>>("?Compare@TPCString@Toshi@@QBEHABV12@@Z");

		InputGenerator gen;
		for (int round = 0; round < 60; ++round)
		{
			const int growSize = gen.NextInt(1, 6);
			const int capacity = gen.NextInt(0, 4);
			alignas(void*) unsigned char storage[sizeof(TCStringPool)];
			construct(storage, growSize, capacity);
			auto& original = *reinterpret_cast<PoolView*>(storage);
			// Our pool lives on the heap so that its destructor (with the original's quirk) runs
			// only after all handles are released.
			auto* pool = new TCStringPool(growSize, capacity);
			auto& ours = *reinterpret_cast<PoolView*>(pool);

			std::vector<Handle> originalHandles;
			std::vector<TPCString> ourHandles;
			for (int step = 0; step < 120; ++step)
			{
				const std::string word = RandomWord(gen);
				Handle handle = nullptr;
				switch (gen.NextU32() % 6)
				{
				case 0:
				case 1:
					get(storage, &handle, word.c_str());
					originalHandles.push_back(handle);
					ourHandles.push_back(pool->Get(word.c_str()));
					break;
				case 2:
				{
					const int value = gen.NextInt(-3, 3);
					getInt(storage, &handle, value);
					originalHandles.push_back(handle);
					ourHandles.push_back(pool->Get(value));
					break;
				}
				case 3:
					getOptional(storage, &handle, word.c_str());
					originalHandles.push_back(handle);
					ourHandles.push_back(pool->GetOptional(word.c_str()));
					CHECK((originalHandles.back() == nullptr) ==
					      (*reinterpret_cast<void* const*>(&ourHandles.back()) == nullptr));
					break;
				case 4:
					if (!originalHandles.empty())
					{
						const std::size_t index = gen.NextU32() % originalHandles.size();
						release(&originalHandles[index]);
						originalHandles.erase(originalHandles.begin() + static_cast<std::ptrdiff_t>(index));
						ourHandles.erase(ourHandles.begin() + static_cast<std::ptrdiff_t>(index));
					}
					break;
				default:
					CHECK(isPresent(storage, word.c_str()) == pool->IsPresent(word.c_str()));
					if (originalHandles.size() >= 2)
					{
						const std::size_t a = gen.NextU32() % originalHandles.size();
						const std::size_t b = gen.NextU32() % originalHandles.size();
						const int originalOrder = compare(&originalHandles[a], &originalHandles[b]);
						const int ourOrder = ourHandles[a].Compare(ourHandles[b]);
						CHECK((originalOrder > 0) - (originalOrder < 0) == (ourOrder > 0) - (ourOrder < 0));
					}
					break;
				}
				CheckSamePool(original, ours);
			}
			Handle volatileHandle = nullptr;
			getVolatile(storage, &volatileHandle, "volatile");
			const TPCString ourVolatile = pool->GetVolatile("volatile");
			CheckSamePool(original, ours);
			release(&volatileHandle);

			for (Handle& handle : originalHandles)
				release(&handle);
			ourHandles.clear();
			CheckSamePool(original, ours); // released strings left both pools
			destruct(storage);
			delete pool;
		}
	}

	TEST_CASE("wide pool operations match the original")
	{
		using Handle = void*;
		using U16 = unsigned short;
		const auto construct = Original<Method<void>>("??0TWStringPool@Toshi@@QAE@XZ");
		const auto destruct = Original<Method<void>>("??1TWStringPool@Toshi@@QAE@XZ");
		const auto get =
		    Original<Method<Handle*, Handle*, const U16*>>("?Get@TWStringPool@Toshi@@QAE?AVTPWString@2@PBG@Z");
		const auto getInt = Original<Method<Handle*, Handle*, int>>("?Get@TWStringPool@Toshi@@QAE?AVTPWString@2@H@Z");
		// TPWString is inline in the original: release a handle by hand (refcount at offset 0).
		const auto deleteString = Original<Method<void>>("?Delete@TPooledWString@Toshi@@AAEXXZ");
		auto release = [&](Handle& handle)
		{
			if (handle != nullptr && --*static_cast<int*>(handle) == 0)
				deleteString(handle);
			handle = nullptr;
		};

		InputGenerator gen;
		for (int round = 0; round < 30; ++round)
		{
			alignas(void*) unsigned char storage[sizeof(TWStringPool)];
			construct(storage);
			auto& original = *reinterpret_cast<PoolView*>(storage);
			auto* pool = new TWStringPool();
			auto& ours = *reinterpret_cast<PoolView*>(pool);
			std::vector<Handle> originalHandles;
			std::vector<TPWString> ourHandles;
			for (int step = 0; step < 120; ++step)
			{
				const std::string word = RandomWord(gen);
				const std::u16string wide(word.begin(), word.end());
				const auto* text = reinterpret_cast<const U16*>(wide.c_str());
				Handle handle = nullptr;
				switch (gen.NextU32() % 4)
				{
				case 0:
				case 1:
					get(storage, &handle, text);
					originalHandles.push_back(handle);
					ourHandles.push_back(pool->Get(text));
					break;
				case 2:
				{
					const int value = gen.NextInt(-3, 3);
					getInt(storage, &handle, value);
					originalHandles.push_back(handle);
					ourHandles.push_back(pool->Get(value));
					break;
				}
				default:
					if (!originalHandles.empty())
					{
						const std::size_t index = gen.NextU32() % originalHandles.size();
						release(originalHandles[index]);
						originalHandles.erase(originalHandles.begin() + static_cast<std::ptrdiff_t>(index));
						ourHandles.erase(ourHandles.begin() + static_cast<std::ptrdiff_t>(index));
					}
					break;
				}
				REQUIRE(original.growSize == ours.growSize);
				REQUIRE(original.count == ours.count);
				REQUIRE(original.capacity == ours.capacity);
				for (int i = 0; i < ours.count; ++i)
				{
					const auto* a = reinterpret_cast<const U16*>(original.entries[i].key);
					const auto* b = reinterpret_cast<const U16*>(ours.entries[i].key);
					CHECK(std::u16string(reinterpret_cast<const char16_t*>(a)) ==
					      std::u16string(reinterpret_cast<const char16_t*>(b)));
					CHECK(original.entries[i].string->refCount == ours.entries[i].string->refCount);
				}
			}
			for (Handle& handle : originalHandles)
				release(handle);
			ourHandles.clear();
			REQUIRE(original.count == ours.count);
			destruct(storage);
			delete pool;
		}
	}

	TEST_CASE("pool files match the original")
	{
		namespace fs = std::filesystem;
		const fs::path directory = fs::temp_directory_path() / "jpog_pool_file_diff";
		fs::create_directories(directory);
		const auto managerCtor = Original<Method<void>>("??0TFileManager@Toshi@@QAE@XZ");
		const auto managerDtor = Original<Method<void>>("??1TFileManager@Toshi@@QAE@XZ");
		const auto nativeCtor = Original<Method<void, const char*>>("??0TNativeFileSystem@Toshi@@QAE@PBD@Z");
		const auto nativeDtor = Original<Method<void>>("??1TNativeFileSystem@Toshi@@UAE@XZ");
		const auto setPrefix =
		    Original<Method<void, const TCString*>>("?SetPrefix@TFileSystem@Toshi@@UAEXABVTCString@2@@Z");
		const auto construct = Original<Method<void, int, int>>("??0TCStringPool@Toshi@@QAE@HH@Z");
		const auto constructFromFile =
		    Original<Method<void, const char*, int, int>>("??0TCStringPool@Toshi@@QAE@PBDHH@Z");
		const auto get =
		    Original<Method<void*, void*, const char*>>("?Get@TCStringPool@Toshi@@QAE?AVTPCString@2@PBD@Z");
		const auto writeFile = Original<Method<bool, const char*>>("?WriteFile@TCStringPool@Toshi@@QAE_NPBD@Z");

		alignas(void*) unsigned char originalManager[sizeof(TFileManager)];
		alignas(void*) unsigned char originalLocal[sizeof(Toshi::TNativeFileSystem)];
		managerCtor(originalManager);
		nativeCtor(originalLocal, "local");
		const TCString prefix(directory.string().c_str());
		setPrefix(originalLocal, &prefix);
		auto* ourManager = new TFileManager();
		auto* ourLocal = new Toshi::TNativeFileSystem("local");
		ourLocal->SetPrefix(prefix);

		// Both pools intern the same strings (the handles are dropped without releasing, so
		// every string keeps one reference) and write them out.
		InputGenerator gen;
		alignas(void*) unsigned char originalPool[sizeof(TCStringPool)];
		construct(originalPool, 16, 0);
		auto* ourPool = new TCStringPool(16, 0); // leaked with the original's: handles stay alive
		std::vector<TPCString> ourHandles;
		for (int i = 0; i < 300; ++i)
		{
			std::string text = "s";
			for (int length = gen.NextInt(0, 12); length > 0; --length)
				text += static_cast<char>('a' + gen.NextInt(0, 5));
			void* handle = nullptr;
			get(originalPool, &handle, text.c_str());
			ourHandles.push_back(ourPool->Get(text.c_str()));
		}
		CHECK(writeFile(originalPool, "local:original.pool"));
		CHECK(ourPool->WriteFile("local:ours.pool"));
		auto bytes = [&](const char* name)
		{
			std::ifstream in(directory / name, std::ios::binary);
			return std::string((std::istreambuf_iterator<char>(in)), {});
		};
		const std::string written = bytes("original.pool");
		CHECK(written.size() > 8);
		CHECK(written == bytes("ours.pool"));

		// Each side loads the other's file. Loaded pools are never destroyed (see ReadFile).
		alignas(void*) unsigned char originalLoaded[sizeof(TCStringPool)];
		constructFromFile(originalLoaded, "local:ours.pool", 8, 4);
		auto* ourLoaded = new TCStringPool("local:original.pool", 8, 4);
		CheckSamePool(*reinterpret_cast<const PoolView*>(originalLoaded),
		              *reinterpret_cast<const PoolView*>(ourLoaded));
		CHECK(ourLoaded->IsPresent(reinterpret_cast<const PoolView*>(originalLoaded)->entries[7].key));

		alignas(void*) unsigned char originalMissing[sizeof(TCStringPool)];
		constructFromFile(originalMissing, "local:missing.pool", 8, 4);
		TCStringPool ourMissing("local:missing.pool", 8, 4);
		CheckSamePool(*reinterpret_cast<const PoolView*>(originalMissing),
		              *reinterpret_cast<const PoolView*>(&ourMissing));

		delete ourLocal;
		delete ourManager;
		nativeDtor(originalLocal);
		managerDtor(originalManager);
		fs::remove_all(directory);
	}

	TEST_CASE("string concatenation operators match the original")
	{
		using Narrow = TCString*(__stdcall*)(TCString*, const char*, const TCString*);
		using NarrowPooled = TCString*(__stdcall*)(TCString*, const TCString*, const Toshi::TPCString*);
		using Wide = Toshi::TWString*(__stdcall*)(Toshi::TWString*, const unsigned short*, const Toshi::TWString*);
		const auto narrow = Original<Narrow>("??HToshi@@YG?AVTCString@0@PBDABV10@@Z");
		const auto narrowPooled = Original<NarrowPooled>("??HToshi@@YG?AVTCString@0@ABV10@ABVTPCString@0@@Z");
		const auto wide = Original<Wide>("??HToshi@@YG?AVTWString@0@PBGABV10@@Z");
		const auto destroy = Original<Method<void>>("??1TCString@Toshi@@QAE@XZ");
		const auto destroyWide = Original<Method<void>>("??1TWString@Toshi@@QAE@XZ");

		TCStringPool pool;
		const Toshi::TPCString pooled = pool.Get("tail");
		const Toshi::TPCString empty;
		for (const char* head : {"", "head", "a longer head string"})
		{
			const TCString tail("+tail");
			alignas(void*) unsigned char storage[sizeof(TCString)];
			auto* result = reinterpret_cast<TCString*>(storage);
			narrow(result, head, &tail);
			CHECK(std::string(static_cast<const char*>(*result)) == static_cast<const char*>(head + tail));
			destroy(result);
			const TCString start(head);
			for (const Toshi::TPCString* handle : {&pooled, &empty})
			{
				narrowPooled(result, &start, handle);
				CHECK(std::string(static_cast<const char*>(*result)) == static_cast<const char*>(start + *handle));
				destroy(result);
			}

			const unsigned short wideHead[] = {'w', 'h', 0};
			const Toshi::TWString wideTail(head);
			alignas(void*) unsigned char wideStorage[sizeof(Toshi::TWString)];
			auto* wideResult = reinterpret_cast<Toshi::TWString*>(wideStorage);
			wide(wideResult, wideHead, &wideTail);
			CHECK(*wideResult == wideHead + wideTail);
			destroyWide(wideResult);
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
