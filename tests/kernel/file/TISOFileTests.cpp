#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/file/TISOFile.h"
#include "toshi/kernel/file/TNativeFile.h"

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
using Toshi::TISOFileSystem;
using namespace jpog::test;

namespace
{

constexpr int kBlock = 2048;

struct IsoFile
{
	const char* name; // without the ";1" suffix
	int size;
	int sector = 0;
};

struct IsoDirectory
{
	const char* name;
	int parent; // 1-based path table index
	int sector;
	std::vector<IsoFile> files;
	std::vector<int> children; // path table indices (1-based)
};

void Put32(std::vector<unsigned char>& image, std::size_t at, unsigned int value)
{
	for (int i = 0; i < 4; ++i)
	{
		image[at + i] = static_cast<unsigned char>(value >> (8 * i));
		image[at + 7 - i] = static_cast<unsigned char>(value >> (8 * i)); // big-endian half
	}
}

unsigned char FileByte(int sector, int offset)
{
	return static_cast<unsigned char>((sector * 131 + offset * 7 + (offset >> 8)) & 0xFF);
}

// Appends a directory record at `at`; records do not cross sectors.
std::size_t PutRecord(std::vector<unsigned char>& image, std::size_t at, const std::string& name, int sector, int size,
                      bool directory)
{
	const std::size_t length = 33 + name.size() + ((name.size() & 1) == 0 ? 1 : 0);
	if (at / kBlock != (at + length - 1) / kBlock)
		at = (at / kBlock + 1) * kBlock;
	image[at] = static_cast<unsigned char>(length);
	Put32(image, at + 2, static_cast<unsigned int>(sector));
	Put32(image, at + 10, static_cast<unsigned int>(size));
	image[at + 25] = directory ? 2 : 0;
	image[at + 32] = static_cast<unsigned char>(name.size());
	std::memcpy(&image[at + 33], name.data(), name.size());
	return at + length;
}

// A small ISO 9660 image: root (README.TXT, DATA/), DATA (unsorted files, SUB/), DATA/SUB.
std::vector<unsigned char> BuildImage()
{
	std::vector<IsoDirectory> dirs = {
	    {"\0", 1, 20, {{"README.TXT", 100}}, {2}},
	    {"DATA", 1, 21, {{"C.DAT", 2 * 0x2000 + 5}, {"A.BIN", 3000}, {"B.BIN", 20000}, {"EMPTY.TXT", 0}}, {3}},
	    {"SUB", 2, 23, {{"DEEP.DAT", 5000}}, {}},
	};
	int sector = 30;
	for (auto& dir : dirs)
		for (auto& file : dir.files)
		{
			file.sector = sector;
			sector += (file.size + kBlock - 1) / kBlock + 1;
		}
	std::vector<unsigned char> image(static_cast<std::size_t>(sector) * kBlock);

	// Primary volume descriptor.
	std::size_t pvd = 16 * kBlock;
	image[pvd] = 1;
	std::memcpy(&image[pvd + 1], "CD001", 5);
	image[pvd + 6] = 1;
	std::memset(&image[pvd + 8], ' ', 32);
	std::memcpy(&image[pvd + 8], "JPOG TEST", 9);
	std::memset(&image[pvd + 40], ' ', 32);
	std::memcpy(&image[pvd + 40], "VOLUME", 6);
	Put32(image, pvd + 80, static_cast<unsigned int>(sector));
	image[pvd + 120] = 1;
	image[pvd + 123] = 1;
	image[pvd + 124] = 1;
	image[pvd + 127] = 1;
	image[pvd + 128] = kBlock & 0xFF;
	image[pvd + 129] = kBlock >> 8;
	image[pvd + 130] = kBlock >> 8;
	image[pvd + 131] = kBlock & 0xFF;
	std::memcpy(&image[pvd + 190], "SET", 3);

	// Path table (little-endian) at sector 18.
	std::size_t table = 18 * kBlock, at = table;
	for (const auto& dir : dirs)
	{
		const std::size_t length = std::strlen(dir.name) == 0 ? 1 : std::strlen(dir.name);
		image[at] = static_cast<unsigned char>(length);
		image[at + 2] = static_cast<unsigned char>(dir.sector);
		image[at + 6] = static_cast<unsigned char>(dir.parent);
		std::memcpy(&image[at + 8], dir.name, std::strlen(dir.name));
		at += 8 + length + (length & 1);
	}
	Put32(image, pvd + 132, static_cast<unsigned int>(at - table));
	image[pvd + 140] = 18;

	// Directory extents (one sector each, DATA spans two).
	for (std::size_t d = 0; d < dirs.size(); ++d)
	{
		const auto& dir = dirs[d];
		const int size = d == 1 ? 2 * kBlock : kBlock;
		std::size_t record = static_cast<std::size_t>(dir.sector) * kBlock;
		record = PutRecord(image, record, std::string(1, '\0'), dir.sector, size, true);
		record = PutRecord(image, record, std::string(1, '\1'), dirs[static_cast<std::size_t>(dir.parent - 1)].sector,
		                   kBlock, true);
		for (std::size_t f = 0; f < dir.files.size(); ++f)
		{
			const auto& file = dir.files[f];
			record = PutRecord(image, record, std::string(file.name) + ";1", file.sector, file.size, false);
			if (f == 0 && !dir.children.empty())
			{
				const auto& child = dirs[static_cast<std::size_t>(dir.children[0] - 1)];
				record = PutRecord(image, record, child.name, child.sector, kBlock, true);
			}
			if (d == 1 && f == 1) // move the rest to the directory's second sector
				record = (static_cast<std::size_t>(dir.sector) + 1) * kBlock;
			for (int i = 0; i < file.size; ++i)
				image[static_cast<std::size_t>(file.sector) * kBlock + i] = FileByte(file.sector, i);
		}
	}
	return image;
}

void WriteImage(const std::filesystem::path& path)
{
	const auto image = BuildImage();
	std::ofstream out(path, std::ios::binary);
	out.write(reinterpret_cast<const char*>(image.data()), static_cast<std::streamsize>(image.size()));
}

std::string ReadAll(TFile* file)
{
	std::string content(static_cast<std::size_t>(file->GetSize()), '\0');
	if (!content.empty())
		file->Read(content.data(), static_cast<int>(content.size()));
	return content;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TISOFile")
{
	TEST_CASE("reads files from an image")
	{
		namespace fs = std::filesystem;
		const fs::path directory = fs::temp_directory_path() / "jpog_iso_file_test";
		fs::create_directories(directory);
		WriteImage(directory / "test.iso");
		{
			TFileManager manager;
			Toshi::TNativeFileSystem local("local");
			local.SetPrefix(directory.string().c_str());
			TISOFileSystem iso("iso", "local:test.iso");
			REQUIRE(iso.GetISOFile() != nullptr);
			CHECK(std::string(iso.GetPVD().GetVolumeID(), 6) == "VOLUME");
			CHECK(std::string(iso.GetPVD().GetVolumeSetID(), 3) == "SET");
			CHECK(iso.GetBytePos(3) == 3 * kBlock);

			TFile* file = iso.CreateFile("data/sub/deep.dat", 1);
			REQUIRE(file != nullptr);
			CHECK(file->GetSize() == 5000);
			const std::string deep = ReadAll(file);
			CHECK(static_cast<unsigned char>(deep[4999]) == FileByte(57, 4999));
			CHECK(file->GetCChar() == -1); // at the end
			file->Seek(10, TFile::TSEEK_END);
			CHECK(file->Tell() == 4990);
			iso.DestroyFile(file);

			CHECK(iso.CreateFile("README.TXT", 2) == nullptr); // read-only
			CHECK(iso.CreateFile("DATA/NOPE.BIN", 1) == nullptr);
			CHECK(iso.CreateFile("NOPE/A.BIN", 1) == nullptr);
			for (const char* name : {"DATA/A.BIN", "DATA/B.BIN", "DATA/C.DAT", "DATA/EMPTY.TXT", "README.TXT"})
			{
				INFO(name);
				TFile* entry = iso.CreateFile(name, 1);
				REQUIRE(entry != nullptr);
				iso.DestroyFile(entry);
			}
		}
		fs::remove_all(directory);
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

TEST_SUITE("TISOFile differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("ISO files match the original")
	{
		namespace fs = std::filesystem;
		const fs::path directory = fs::temp_directory_path() / "jpog_iso_file_diff";
		fs::create_directories(directory);
		// One image per side: the native file system opens files exclusively.
		WriteImage(directory / "original.iso");
		WriteImage(directory / "ours.iso");

		const auto managerCtor = Original<Method<void>>("??0TFileManager@Toshi@@QAE@XZ");
		const auto managerDtor = Original<Method<void>>("??1TFileManager@Toshi@@QAE@XZ");
		const auto nativeCtor = Original<Method<void, const char*>>("??0TNativeFileSystem@Toshi@@QAE@PBD@Z");
		const auto nativeDtor = Original<Method<void>>("??1TNativeFileSystem@Toshi@@UAE@XZ");
		const auto isoCtor =
		    Original<Method<void, const char*, const TCString*>>("??0TISOFileSystem@Toshi@@QAE@PBDABVTCString@1@@Z");
		const auto isoDtor = Original<Method<void>>("??1TISOFileSystem@Toshi@@UAE@XZ");
		const auto setPrefix =
		    Original<Method<void, const TCString*>>("?SetPrefix@TFileSystem@Toshi@@UAEXABVTCString@2@@Z");

		alignas(void*) unsigned char originalManager[sizeof(TFileManager)];
		alignas(void*) unsigned char originalLocal[sizeof(Toshi::TNativeFileSystem)];
		alignas(void*) unsigned char originalIso[sizeof(TISOFileSystem)];
		managerCtor(originalManager);
		nativeCtor(originalLocal, "local");
		const TCString prefix(directory.string().c_str());
		setPrefix(originalLocal, &prefix);
		const TCString originalPath("local:original.iso"), ourPath("local:ours.iso");
		isoCtor(originalIso, "iso", &originalPath);
		auto* ourManager = new TFileManager();
		auto* ourLocal = new Toshi::TNativeFileSystem("local");
		ourLocal->SetPrefix(prefix);
		auto* ourIso = new TISOFileSystem("iso", ourPath);

		// Our classes share the original vtable layout, so the original's objects can be driven
		// through our declarations.
		auto* original = reinterpret_cast<TISOFileSystem*>(originalIso);
		const auto* originalPvd = reinterpret_cast<const unsigned char*>(&original->GetPVD());
		const auto* ourPvd = reinterpret_cast<const unsigned char*>(&ourIso->GetPVD());
		CHECK(std::memcmp(originalPvd, ourPvd, 0xD2) == 0); // 0xD2-0xD3 is never written
		CHECK(std::memcmp(originalPvd + 0xD4, ourPvd + 0xD4, 0xE4 - 0xD4) == 0);

		struct IsoView
		{
			void* vtable;
			void* system;
			unsigned int offset, size;
			int position;
			unsigned int flags;
			int filePointer, bufferStart, bufferSize;
			void* buffer;
		};
		auto sameState = [](const TFile* a, const TFile* b)
		{
			const auto* x = reinterpret_cast<const IsoView*>(a);
			const auto* y = reinterpret_cast<const IsoView*>(b);
			return x->offset == y->offset && x->size == y->size && x->position == y->position && x->flags == y->flags &&
			       x->filePointer == y->filePointer && x->bufferStart == y->bufferStart &&
			       x->bufferSize == y->bufferSize && (x->buffer == nullptr) == (y->buffer == nullptr);
		};

		static const char* const names[] = {"README.TXT",     "data/a.bin",        "DATA/B.BIN", "Data\\C.dat",
		                                    "DATA/EMPTY.TXT", "DATA/SUB/DEEP.DAT", "DATA/X.BIN", "SUB/DEEP.DAT",
		                                    "data/sub",       "/README.TXT"};
		InputGenerator gen;
		std::vector<unsigned char> a(30000), b(30000);
		for (int round = 0; round < 60; ++round)
		{
			const char* name = names[round % (sizeof(names) / sizeof(names[0]))];
			const unsigned int flags = (round / 10) % 2 == 0 ? 1 : 0x11; // also unbuffered
			INFO("round ", round, " ", name, " flags ", flags);
			const TCString path(name);
			TFile* originalFile = original->CreateFile(path, flags);
			TFile* ourFile = ourIso->CreateFile(path, flags);
			REQUIRE((originalFile == nullptr) == (ourFile == nullptr));
			if (ourFile == nullptr)
				continue;
			CHECK(sameState(originalFile, ourFile));
			const int size = ourFile->GetSize();
			for (int step = 0; step < 150; ++step)
			{
				INFO("step ", step);
				switch (gen.NextU32() % 6)
				{
				case 0:
				case 1:
				{
					const int count = gen.NextInt(-1, 22000);
					const int readOriginal = originalFile->Read(a.data(), count);
					REQUIRE(readOriginal == ourFile->Read(b.data(), count));
					CHECK(std::memcmp(a.data(), b.data(), readOriginal > 0 ? readOriginal : 0) == 0);
					break;
				}
				case 2:
				{
					const auto origin = static_cast<TFile::TSEEK>(gen.NextInt(0, 2));
					const int target = gen.NextInt(0, size);
					const int offset = origin == TFile::TSEEK_SET   ? target
					                   : origin == TFile::TSEEK_CUR ? target - ourFile->Tell()
					                                                : size - target;
					CHECK(originalFile->Seek(offset, origin) == ourFile->Seek(offset, origin));
					break;
				}
				case 3:
					CHECK(originalFile->GetCChar() == ourFile->GetCChar());
					break;
				case 4:
					// Buffered reads at the very end may pick up bytes past the file.
					if (ourFile->Tell() + 2 <= size)
						CHECK(originalFile->GetWChar() == ourFile->GetWChar());
					break;
				default:
					CHECK(originalFile->Tell() == ourFile->Tell());
					CHECK(originalFile->Write("x", 1) == ourFile->Write("x", 1));
					CHECK(originalFile->PutCChar('x') == ourFile->PutCChar('x'));
					break;
				}
				CHECK(sameState(originalFile, ourFile));
			}
			original->DestroyFile(originalFile);
			ourIso->DestroyFile(ourFile);
		}

		delete ourIso;
		delete ourLocal;
		delete ourManager;
		isoDtor(originalIso);
		nativeDtor(originalLocal);
		managerDtor(originalManager);
		fs::remove_all(directory);
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
