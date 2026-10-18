#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TFile.h"
#include "toshi/kernel/TMemFile.h"
#include "toshi/kernel/TNativeFile.h"
#include "toshi/kernel/TNullFile.h"

#include <cstring>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using Toshi::TCString;
using Toshi::TFile;
using Toshi::TFileManager;
using Toshi::TFileSystem;
using namespace jpog::test;

namespace
{

// Records CreateFile calls; returns a marker for paths it "has".
struct Call
{
	int system;
	std::string path;
	unsigned int flags;
	bool operator==(const Call&) const = default;
};
std::vector<Call> s_calls;

class TRecordingFileSystem : public TFileSystem
{
public:
	TRecordingFileSystem(const char* name, int index, const char* owned)
	    : TFileSystem(name), m_index(index), m_owned(owned)
	{
	}
	TFile* CreateFile(const TCString& path, unsigned int flags) override
	{
		s_calls.push_back({m_index, static_cast<const char*>(path), flags});
		return std::strcmp(path, m_owned) == 0 ? reinterpret_cast<TFile*>(this) : nullptr;
	}
	void DestroyFile(TFile*) override {}
	TCString MakeInternalPath(const TCString& path) const override { return path; }

private:
	int m_index;
	const char* m_owned;
};

std::string RandomPath(InputGenerator& gen)
{
	static const char* const parts[] = {"a", "bb", "..", ".", "", "dir", "C:", "x.txt", "\\", "/"};
	std::string path;
	const int count = gen.NextInt(0, 6);
	for (int i = 0; i < count; ++i)
	{
		path += parts[gen.NextU32() % (sizeof(parts) / sizeof(parts[0]))];
		if (gen.NextU32() % 3)
			path += (gen.NextU32() & 1) ? "/" : "\\";
	}
	return path;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TFile")
{
	TEST_CASE("path helpers")
	{
		CHECK(TFile::SimplifyPath("a/./b/../c\\d") == "a/c/d");
		CHECK(TFile::SimplifyPath("/x/") == "/x/"); // as the original: one trailing slash survives
		CHECK(TFile::ConcatPath("data/", "units/rex.tmd") == "data/units/rex.tmd");
		CHECK(TFile::ConcatPath("data", "/abs") == "/abs");
		TCString directory, file;
		TFile::SplitPath("Data/Models/rex.tmd", directory, file);
		CHECK(directory == "Data/Models/");
		CHECK(file == "rex.tmd");
	}

	TEST_CASE("file manager lookup")
	{
		TFileManager manager;
		CHECK(TFileManager::GetFileManager() == &manager);
		TRecordingFileSystem a("a", 0, "x"), b("b", 1, "y");
		manager.MountFileSystem(&a);
		manager.MountFileSystem(&b);
		manager.SetSystemPath("b;a");
		s_calls.clear();
		CHECK(manager.CreateFile("x", 7) == reinterpret_cast<TFile*>(&a));
		CHECK(s_calls == std::vector<Call>{{1, "x", 7}, {0, "x", 7}});
		s_calls.clear();
		CHECK(manager.CreateFile("a:y", 1) == nullptr);
		CHECK(s_calls == std::vector<Call>{{0, "y", 1}});
		CHECK(manager.FindFileSystem("b") == &b);
		manager.UnmountFileSystem(&a);
		manager.UnmountFileSystem(&b);
	}

	TEST_CASE("null files")
	{
		TFileManager manager;
		Toshi::TNullFileSystem system("null");
		CHECK(Toshi::TNullFileSystem::GetNullFileSystem() == &system);
		auto* file = static_cast<Toshi::TNullFile*>(system.CreateFile("anything", 2));
		REQUIRE(file != nullptr);
		CHECK(file->Write("abc", 3) == 3);
		CHECK(file->PutCChar('x') == 'x');
		CHECK(file->CPrintf("%d", 1234) == 4);
		CHECK(file->GetBytesWritten() == 8);
		char buffer[4];
		CHECK(file->Read(buffer, 4) == -1);
		CHECK(file->GetCChar() == -1);
		CHECK(file->Tell() == 0);
		CHECK(system.MakeInternalPath("x") == "");
		system.DestroyFile(file);
	}

	TEST_CASE("memory files")
	{
		namespace fs = std::filesystem;
		const fs::path directory = fs::temp_directory_path() / "jpog_mem_file_test";
		fs::create_directories(directory);
		{
			std::ofstream out(directory / "data.bin", std::ios::binary);
			out << "0123456789";
		}
		TFileManager manager;
		Toshi::TNativeFileSystem local("local");
		local.SetPrefix(directory.string().c_str());
		Toshi::TMemFileSystem memory("mem");
		CHECK(memory.MakeInternalPath("a") == local.MakeInternalPath("a"));

		TFile* source = local.CreateFile("data.bin", 1);
		REQUIRE(source != nullptr);
		source->Seek(4, TFile::TSEEK_SET);
		TFile* file = memory.CreateFile(source); // copies from the start
		local.DestroyFile(source);
		REQUIRE(file != nullptr);
		CHECK(file->GetSize() == 10);
		char buffer[8] = {};
		CHECK(file->Read(buffer, 3) == 3);
		CHECK(std::string(buffer, 3) == "012");
		CHECK(file->Read(buffer, 8) == -1); // all or nothing
		CHECK(file->Tell() == 3);
		CHECK(file->Seek(20, TFile::TSEEK_SET));
		CHECK(file->Tell() == 0); // past the end goes back to the start
		file->Seek(0, TFile::TSEEK_END);
		CHECK(file->Tell() == 10);
		CHECK(file->GetCChar() == -1);
		CHECK(file->PutCChar('a') == 'a');
		memory.DestroyFile(file);
		fs::remove_all(directory);
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

TEST_SUITE("TFile differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("path helpers match the original")
	{
		using Unary = TCString*(__stdcall*)(TCString*, const TCString*);
		using Binary = TCString*(__stdcall*)(TCString*, const TCString*, const TCString*);
		using Split = void(__stdcall*)(const TCString*, TCString*, TCString*);
		const auto simplify = Original<Unary>("?SimplifyPath@TFile@Toshi@@SG?AVTCString@2@ABV32@@Z");
		const auto concat = Original<Binary>("?ConcatPath@TFile@Toshi@@SG?AVTCString@2@ABV32@0@Z");
		const auto split = Original<Split>("?SplitPath@TFile@Toshi@@SGXABVTCString@2@AAV32@1@Z");
		// Strings returned or filled by the original live on its heap: copy them out, then free
		// them through its own destructor.
		const auto destroy = Original<Method<void>>("??1TCString@Toshi@@QAE@XZ");
		const auto construct = Original<Method<void>>("??0TCString@Toshi@@QAE@XZ");
		auto take = [&](TCString* original)
		{
			std::string text = static_cast<const char*>(*original);
			destroy(original);
			return text;
		};

		InputGenerator gen;
		for (int i = 0; i < 3000; ++i)
		{
			const TCString a(RandomPath(gen).c_str()), b(RandomPath(gen).c_str());
			INFO("a='", static_cast<const char*>(a), "' b='", static_cast<const char*>(b), "'");
			alignas(void*) unsigned char storage[3][sizeof(TCString)];
			auto* r0 = reinterpret_cast<TCString*>(storage[0]);
			simplify(r0, &a);
			CHECK(take(r0) == std::string(static_cast<const char*>(TFile::SimplifyPath(a))));
			concat(r0, &a, &b);
			CHECK(take(r0) == std::string(static_cast<const char*>(TFile::ConcatPath(a, b))));

			auto* directory = reinterpret_cast<TCString*>(storage[1]);
			auto* file = reinterpret_cast<TCString*>(storage[2]);
			construct(directory);
			construct(file);
			split(&a, directory, file);
			TCString ourDirectory, ourFile;
			TFile::SplitPath(a, ourDirectory, ourFile);
			CHECK(take(directory) == std::string(static_cast<const char*>(ourDirectory)));
			CHECK(take(file) == std::string(static_cast<const char*>(ourFile)));
		}
	}

	TEST_CASE("file manager matches the original")
	{
		using P = void*;
		const auto construct = Original<Method<void>>("??0TFileManager@Toshi@@QAE@XZ");
		const auto destruct = Original<Method<void>>("??1TFileManager@Toshi@@QAE@XZ");
		const auto mount = Original<Method<void, P>>("?MountFileSystem@TFileManager@Toshi@@QAEXPAVTFileSystem@2@@Z");
		const auto unmount =
		    Original<Method<void, P>>("?UnmountFileSystem@TFileManager@Toshi@@QAEXPAVTFileSystem@2@@Z");
		const auto setPath =
		    Original<Method<void, const TCString*>>("?SetSystemPath@TFileManager@Toshi@@QAEXABVTCString@2@@Z");
		const auto create = Original<Method<TFile*, const TCString*, unsigned int>>(
		    "?CreateFile@TFileManager@Toshi@@QAEPAVTFile@2@ABVTCString@2@I@Z");
		const auto find = Original<Method<P, const TCString*>>(
		    "?FindFileSystem@TFileManager@Toshi@@QAEPAVTFileSystem@2@ABVTCString@2@@Z");

		static const char* const names[] = {"nat", "iso", "mem", "pak"};
		static const char* const owned[] = {"a", "b", "c", "a"};
		InputGenerator gen;
		for (int round = 0; round < 100; ++round)
		{
			// Separate file system objects for each manager, so their links never mix.
			std::vector<TRecordingFileSystem*> originalSystems, ourSystems;
			for (int i = 0; i < 4; ++i)
			{
				originalSystems.push_back(new TRecordingFileSystem(names[i], i, owned[i]));
				ourSystems.push_back(new TRecordingFileSystem(names[i], i, owned[i]));
			}
			alignas(void*) unsigned char storage[sizeof(TFileManager)];
			construct(storage);
			auto* ours = new TFileManager();
			std::vector<bool> mounted(4, false);
			for (int step = 0; step < 30; ++step)
			{
				const int which = gen.NextInt(0, 3);
				switch (gen.NextU32() % 5)
				{
				case 0:
					if (!mounted[which])
					{
						mount(storage, originalSystems[which]);
						ours->MountFileSystem(ourSystems[which]);
						mounted[which] = true;
					}
					break;
				case 1:
					if (mounted[which])
					{
						unmount(storage, originalSystems[which]);
						ours->UnmountFileSystem(ourSystems[which]);
						mounted[which] = false;
					}
					break;
				case 2:
				{
					std::string path;
					const int count = gen.NextInt(0, 4);
					for (int i = 0; i < count; ++i)
						path += std::string(i ? ";" : "") + ((gen.NextU32() % 5) ? names[gen.NextU32() % 4] : "x");
					const TCString text(path.c_str());
					setPath(storage, &text);
					ours->SetSystemPath(text);
					break;
				}
				case 3:
				{
					std::string path = (gen.NextU32() % 3 == 0) ? std::string(names[gen.NextU32() % 4]) + ":" : "";
					path += "abcz"[gen.NextU32() % 4];
					const TCString text(path.c_str());
					const unsigned int flags = gen.NextU32() & 7;
					s_calls.clear();
					TFile* originalFile = create(storage, &text, flags);
					const auto originalCalls = s_calls;
					s_calls.clear();
					TFile* ourFile = ours->CreateFile(text, flags);
					INFO("path ", path.c_str());
					CHECK(originalCalls == s_calls);
					// Results are each manager's own objects: compare which system answered.
					auto index = [](const std::vector<TRecordingFileSystem*>& systems, TFile* file)
					{
						for (int i = 0; i < 4; ++i)
							if (reinterpret_cast<TFile*>(systems[i]) == file)
								return i;
						return -1;
					};
					CHECK(index(originalSystems, originalFile) == index(ourSystems, ourFile));
					break;
				}
				default:
				{
					const TCString name(names[which]);
					const P found = find(storage, &name);
					TFileSystem* ourFound = ours->FindFileSystem(name);
					CHECK((found == originalSystems[which]) == (ourFound == ourSystems[which]));
					CHECK((found == nullptr) == (ourFound == nullptr));
					break;
				}
				}
			}
			for (int i = 0; i < 4; ++i)
				if (mounted[i])
				{
					unmount(storage, originalSystems[i]);
					ours->UnmountFileSystem(ourSystems[i]);
				}
			destruct(storage);
			delete ours;
			for (int i = 0; i < 4; ++i)
			{
				delete originalSystems[i];
				delete ourSystems[i];
			}
		}
	}

	TEST_CASE("native files match the original")
	{
		namespace fs = std::filesystem;
		const fs::path directory = fs::temp_directory_path() / "jpog_native_file_test";
		fs::create_directories(directory);
		InputGenerator gen;
		std::vector<unsigned char> content(5000 + gen.NextInt(0, 3000));
		for (auto& byte : content)
			byte = static_cast<unsigned char>(gen.NextU32());
		// One copy per side: files are opened exclusively (share mode 0).
		for (const char* file : {"data_original.bin", "data_ours.bin"})
		{
			std::ofstream out(directory / file, std::ios::binary);
			out.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
		}

		using P = void*;
		const auto managerCtor = Original<Method<void>>("??0TFileManager@Toshi@@QAE@XZ");
		const auto managerDtor = Original<Method<void>>("??1TFileManager@Toshi@@QAE@XZ");
		const auto fsCtor = Original<Method<void, const char*>>("??0TNativeFileSystem@Toshi@@QAE@PBD@Z");
		const auto fsDtor = Original<Method<void>>("??1TNativeFileSystem@Toshi@@UAE@XZ");
		const auto setPrefix =
		    Original<Method<void, const TCString*>>("?SetPrefix@TFileSystem@Toshi@@UAEXABVTCString@2@@Z");
		const auto createFile = Original<Method<TFile*, const TCString*, unsigned int>>(
		    "?CreateFile@TNativeFileSystem@Toshi@@UAEPAVTFile@2@ABVTCString@2@I@Z");
		const auto destroyFile =
		    Original<Method<void, TFile*>>("?DestroyFile@TNativeFileSystem@Toshi@@UAEXPAVTFile@2@@Z");

		alignas(void*) unsigned char originalManager[sizeof(TFileManager)];
		alignas(void*) unsigned char originalSystem[sizeof(Toshi::TNativeFileSystem)];
		managerCtor(originalManager);
		fsCtor(originalSystem, "local");
		const TCString prefix(directory.string().c_str());
		setPrefix(originalSystem, &prefix);
		auto* ourManager = new TFileManager();
		auto* ourSystem = new Toshi::TNativeFileSystem("local");
		ourSystem->SetPrefix(prefix);

		struct NativeView
		{
			void* vtable;
			void* system;
			void* handle;
			int position, filePointer, bufferStart, bufferSize;
			void *readBuffer, *writeBuffer;
			int writeUsed;
			bool writeBuffered;
		};
		auto sameState = [](const TFile* a, const TFile* b)
		{
			const auto* x = reinterpret_cast<const NativeView*>(a);
			const auto* y = reinterpret_cast<const NativeView*>(b);
			return x->position == y->position && x->filePointer == y->filePointer && x->bufferStart == y->bufferStart &&
			       x->bufferSize == y->bufferSize && x->writeUsed == y->writeUsed &&
			       x->writeBuffered == y->writeBuffered;
		};

		const TCString originalData("data_original.bin"), ourData("data_ours.bin");
		for (int round = 0; round < 20; ++round)
		{
			const unsigned int flags = (round % 4 == 3) ? 0x11 : 1; // also unbuffered
			TFile* original = createFile(originalSystem, &originalData, flags);
			TFile* ours = ourSystem->CreateFile(ourData, flags);
			REQUIRE(original != nullptr);
			REQUIRE(ours != nullptr);
			CHECK(sameState(original, ours));
			std::vector<unsigned char> a(4000), b(4000);
			for (int step = 0; step < 200; ++step)
			{
				INFO("round ", round, " step ", step);
				switch (gen.NextU32() % 6)
				{
				case 0:
				case 1:
				{
					const int size = gen.NextInt(-1, 3000);
					const int readOriginal = original->Read(a.data(), size);
					const int readOurs = ours->Read(b.data(), size);
					REQUIRE(readOriginal == readOurs);
					CHECK(std::memcmp(a.data(), b.data(), readOurs > 0 ? readOurs : 0) == 0);
					break;
				}
				case 2:
				{
					const int offset = gen.NextInt(-200, static_cast<int>(content.size()) + 200);
					const auto origin = static_cast<TFile::TSEEK>(gen.NextInt(0, 2));
					const int adjusted =
					    origin == TFile::TSEEK_SET ? offset : offset - static_cast<int>(content.size());
					CHECK(original->Seek(adjusted, origin) == ours->Seek(adjusted, origin));
					break;
				}
				case 3:
					CHECK(original->GetCChar() == ours->GetCChar());
					break;
				case 4:
					CHECK(original->GetWChar() == ours->GetWChar());
					break;
				default:
					CHECK(original->Tell() == ours->Tell());
					if (step % 20 == 0)
						CHECK(original->GetSize() == ours->GetSize());
					break;
				}
				CHECK(sameState(original, ours));
			}
			destroyFile(originalSystem, original);
			ourSystem->DestroyFile(ours);
		}

		// Writing: both create their own file from the same calls; compare the results.
		for (int round = 0; round < 10; ++round)
		{
			const TCString originalName("out_original.bin"), ourName("out_ours.bin");
			TFile* original = createFile(originalSystem, &originalName, 2 | 8);
			TFile* ours = ourSystem->CreateFile(ourName, 2 | 8);
			REQUIRE(original != nullptr);
			REQUIRE(ours != nullptr);
			for (int step = 0; step < 50; ++step)
			{
				const int size = gen.NextInt(0, 3000);
				const int offset = gen.NextInt(0, static_cast<int>(content.size()) - size);
				if (step % 7 == 0)
				{
					const int position = gen.NextInt(0, 4000);
					original->Seek(position, TFile::TSEEK_SET);
					ours->Seek(position, TFile::TSEEK_SET);
				}
				CHECK(original->Write(content.data() + offset, size) == ours->Write(content.data() + offset, size));
				CHECK(original->PutCChar(static_cast<char>(step)) == ours->PutCChar(static_cast<char>(step)));
				CHECK(original->CPrintf("%d|", step) == ours->CPrintf("%d|", step));
				CHECK(sameState(original, ours));
			}
			destroyFile(originalSystem, original);
			ourSystem->DestroyFile(ours);
			std::ifstream x(directory / "out_original.bin", std::ios::binary),
			    y(directory / "out_ours.bin", std::ios::binary);
			const std::string xs((std::istreambuf_iterator<char>(x)), {}), ys((std::istreambuf_iterator<char>(y)), {});
			CHECK(xs == ys);
		}

		delete ourSystem;
		delete ourManager;
		fsDtor(originalSystem);
		managerDtor(originalManager);
		fs::remove_all(directory);
	}

	TEST_CASE("memory files match the original")
	{
		namespace fs = std::filesystem;
		const fs::path directory = fs::temp_directory_path() / "jpog_mem_file_diff";
		fs::create_directories(directory);
		InputGenerator gen;
		std::vector<unsigned char> content(3000 + gen.NextInt(0, 2000));
		for (auto& byte : content)
			byte = static_cast<unsigned char>(gen.NextU32());
		// Path-based opens leak the source handle (share mode 0), so each side gets its own files.
		for (const char* file : {"src_original.bin", "src_ours.bin", "path_original.bin", "path_ours.bin"})
		{
			std::ofstream out(directory / file, std::ios::binary);
			out.write(reinterpret_cast<const char*>(content.data()), static_cast<std::streamsize>(content.size()));
		}

		const auto managerCtor = Original<Method<void>>("??0TFileManager@Toshi@@QAE@XZ");
		const auto managerDtor = Original<Method<void>>("??1TFileManager@Toshi@@QAE@XZ");
		const auto nativeCtor = Original<Method<void, const char*>>("??0TNativeFileSystem@Toshi@@QAE@PBD@Z");
		const auto nativeDtor = Original<Method<void>>("??1TNativeFileSystem@Toshi@@UAE@XZ");
		const auto memCtor = Original<Method<void, const char*>>("??0TMemFileSystem@Toshi@@QAE@PBD@Z");
		const auto memDtor = Original<Method<void>>("??1TMemFileSystem@Toshi@@UAE@XZ");
		const auto setPrefix =
		    Original<Method<void, const TCString*>>("?SetPrefix@TFileSystem@Toshi@@UAEXABVTCString@2@@Z");
		const auto nativeCreate = Original<Method<TFile*, const TCString*, unsigned int>>(
		    "?CreateFile@TNativeFileSystem@Toshi@@UAEPAVTFile@2@ABVTCString@2@I@Z");
		const auto nativeDestroy =
		    Original<Method<void, TFile*>>("?DestroyFile@TNativeFileSystem@Toshi@@UAEXPAVTFile@2@@Z");
		const auto memCreatePath = Original<Method<TFile*, const TCString*, unsigned int>>(
		    "?CreateFile@TMemFileSystem@Toshi@@UAEPAVTFile@2@ABVTCString@2@I@Z");
		const auto memCreateCopy =
		    Original<Method<TFile*, TFile*>>("?CreateFile@TMemFileSystem@Toshi@@QAEPAVTFile@2@PAV32@@Z");
		const auto memDestroy = Original<Method<void, TFile*>>("?DestroyFile@TMemFileSystem@Toshi@@UAEXPAVTFile@2@@Z");

		alignas(void*) unsigned char originalManager[sizeof(TFileManager)];
		alignas(void*) unsigned char originalLocal[sizeof(Toshi::TNativeFileSystem)];
		alignas(void*) unsigned char originalMemory[sizeof(Toshi::TMemFileSystem)];
		managerCtor(originalManager);
		nativeCtor(originalLocal, "local");
		const TCString prefix(directory.string().c_str());
		setPrefix(originalLocal, &prefix);
		memCtor(originalMemory, "mem");
		auto* ourManager = new TFileManager();
		auto* ourLocal = new Toshi::TNativeFileSystem("local");
		ourLocal->SetPrefix(prefix);
		auto* ourMemory = new Toshi::TMemFileSystem("mem");

		auto exercise = [&](TFile* original, TFile* ours)
		{
			REQUIRE(original != nullptr);
			REQUIRE(ours != nullptr);
			CHECK(original->GetSize() == ours->GetSize());
			std::vector<unsigned char> a(4000), b(4000);
			for (int step = 0; step < 300; ++step)
			{
				INFO("step ", step);
				switch (gen.NextU32() % 5)
				{
				case 0:
				case 1:
				{
					const int size = gen.NextInt(-1, 1500);
					const int readOriginal = original->Read(a.data(), size);
					REQUIRE(readOriginal == ours->Read(b.data(), size));
					CHECK(std::memcmp(a.data(), b.data(), readOriginal > 0 ? readOriginal : 0) == 0);
					break;
				}
				case 2:
				{
					// Negative positions are not clamped and would read outside the buffer.
					const auto origin = static_cast<TFile::TSEEK>(gen.NextInt(0, 2));
					const int offset = origin == TFile::TSEEK_CUR
					                       ? gen.NextInt(-original->Tell(), 2000)
					                       : gen.NextInt(0, static_cast<int>(content.size()) + 200);
					CHECK(original->Seek(offset, origin) == ours->Seek(offset, origin));
					break;
				}
				case 3:
					CHECK(original->GetCChar() == ours->GetCChar());
					CHECK(original->GetWChar() == ours->GetWChar());
					CHECK(original->PutWChar(7) == ours->PutWChar(7));
					break;
				default:
					CHECK(original->CPrintf("%d", step) == ours->CPrintf("%d", step));
					break;
				}
				CHECK(original->Tell() == ours->Tell());
			}
		};

		const TCString originalSource("src_original.bin"), ourSource("src_ours.bin");
		for (int round = 0; round < 5; ++round)
		{
			TFile* originalNative = nativeCreate(originalLocal, &originalSource, 1);
			TFile* ourNative = ourLocal->CreateFile(ourSource, 1);
			REQUIRE(originalNative != nullptr);
			REQUIRE(ourNative != nullptr);
			const int position = gen.NextInt(0, 1000);
			originalNative->Seek(position, TFile::TSEEK_SET);
			ourNative->Seek(position, TFile::TSEEK_SET);
			TFile* original = memCreateCopy(originalMemory, originalNative);
			TFile* ours = ourMemory->CreateFile(ourNative);
			nativeDestroy(originalLocal, originalNative);
			ourLocal->DestroyFile(ourNative);
			exercise(original, ours);
			memDestroy(originalMemory, original);
			ourMemory->DestroyFile(ours);
		}

		// The first path-based open loads the file; the leaked handle makes the second one empty.
		const TCString originalPath("local:path_original.bin"), ourPath("local:path_ours.bin");
		for (int round = 0; round < 2; ++round)
		{
			TFile* original = memCreatePath(originalMemory, &originalPath, 1);
			TFile* ours = ourMemory->CreateFile(ourPath, 1);
			exercise(original, ours);
			CHECK(ours->GetSize() == (round == 0 ? static_cast<int>(content.size()) : 0));
			memDestroy(originalMemory, original);
			ourMemory->DestroyFile(ours);
		}

		delete ourMemory;
		delete ourLocal;
		delete ourManager;
		memDtor(originalMemory);
		nativeDtor(originalLocal);
		managerDtor(originalManager);
		std::error_code ignored; // the leaked handles keep the path files open
		fs::remove_all(directory, ignored);
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
