#include "support/Differential.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TLocale.h"
#include "toshi/kernel/file/TNativeFile.h"

#include <cstring>
#include <doctest/doctest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using Toshi::TCString;
using Toshi::TFileManager;
using Toshi::TLocale;
using Toshi::TLString;
using Toshi::TWString;
using namespace jpog::test;

namespace
{

namespace fs = std::filesystem;

void WriteUtf8(const fs::path& path, const std::string& text)
{
	std::ofstream out(path, std::ios::binary);
	out << text;
}

void WriteUtf16(const fs::path& path, const std::u16string& text)
{
	std::ofstream out(path, std::ios::binary);
	const char16_t bom = 0xFEFF;
	out.write(reinterpret_cast<const char*>(&bom), 2);
	out.write(reinterpret_cast<const char*>(text.data()), static_cast<std::streamsize>(text.size() * 2));
}

// Locale files in `directory`: the info list and two string tables.
void WriteLocaleFiles(const fs::path& directory)
{
	WriteUtf8(directory / "locales.txt", "/* Available locales */\n"
	                                     "English \"english.txt\";\n"
	                                     "Japanese \"japanese.txt\";\n"
	                                     "German \"german.txt\";\n"
	                                     "English \"english2.txt\";\n"
	                                     "Empty \"\";\n");
	WriteUtf16(directory / "english.txt",
	           u"// English strings\n"
	           u"HELLO \"Hello\";\n"
	           u"COUNT \"%d dinosaurs\";\n"
	           u"12 \"Number twelve\";\n"
	           u"5 \"Five\nlines\";\n"
	           u"0 \"ignored\";\n"
	           u"HELLO \"Hello again\";\n"
	           u"#ifdef TOSHI_SKU_WINDOWS\nPLATFORM \"Windows\";\n#else\nPLATFORM \"Other\";\n#endif\n"
	           u"PARK \"Jurassic Park é\";\n");
	WriteUtf16(directory / "japanese.txt", u"HELLO \"こんにちは\";\n12 \"十二\";\n");
}

std::u16string Text16(const unsigned short* text)
{
	std::u16string result;
	for (; text != nullptr && *text != 0; ++text)
		result += static_cast<char16_t>(*text);
	return result;
}

// Raw views of the locale's maps, the same for the original's objects and ours.
struct PooledView
{
	int refCount;
	const void* buffer;
};
struct StringView
{
	int refCount;
	int id;
	const PooledView* identifier;
	const PooledView* text;
	const void* locale;
};
struct PairView
{
	const void* key;
	const void* value;
};
struct MapView
{
	int growSize, count, capacity;
	const PairView* pairs;
};
struct InfoView
{
	int index;
	const PooledView* name;
	const PooledView* file;
	int wordBreak;
};
struct LocaleView
{
	void* emitter[3];
	int nextTemporary;
	MapView named, numbered, infos;
	void* directory[2];
	const InfoView* current;
};

std::string Narrow(const PooledView* pooled)
{
	return pooled == nullptr ? "" : static_cast<const char*>(pooled->buffer);
}

std::u16string Wide(const PooledView* pooled)
{
	return pooled == nullptr ? u"" : Text16(static_cast<const unsigned short*>(pooled->buffer));
}

// The string fields that must match (the locale pointer is each side's own).
std::string Describe(const StringView* string)
{
	if (string == nullptr)
		return "(null)";
	std::string text =
	    std::to_string(string->refCount) + "/" + std::to_string(string->id) + "/" + Narrow(string->identifier) + "/";
	for (char16_t c : Wide(string->text))
		text += std::to_string(static_cast<int>(c)) + ",";
	return text;
}

std::vector<std::string> DescribeMap(const MapView& map, bool numbered)
{
	std::vector<std::string> entries{std::to_string(map.growSize) + " " + std::to_string(map.count) + " " +
	                                 std::to_string(map.capacity)};
	for (int i = 0; i < map.count; ++i)
	{
		const PairView& pair = map.pairs[i];
		const std::string key = numbered ? std::to_string(reinterpret_cast<const StringView*>(pair.value)->id)
		                                 : Narrow(static_cast<const PooledView*>(pair.key));
		entries.push_back(key + " -> " + Describe(static_cast<const StringView*>(pair.value)));
	}
	return entries;
}

std::vector<std::string> DescribeInfos(const MapView& map)
{
	std::vector<std::string> entries{std::to_string(map.count)};
	for (int i = 0; i < map.count; ++i)
	{
		const auto* info = static_cast<const InfoView*>(map.pairs[i].value);
		entries.push_back(Narrow(static_cast<const PooledView*>(map.pairs[i].key)) + " " + std::to_string(info->index) +
		                  " " + Narrow(info->name) + " " + Narrow(info->file) + " " + std::to_string(info->wordBreak));
	}
	return entries;
}

void CheckSameLocale(const void* original, const void* ours)
{
	const auto& a = *static_cast<const LocaleView*>(original);
	const auto& b = *static_cast<const LocaleView*>(ours);
	CHECK(a.nextTemporary == b.nextTemporary);
	CHECK(DescribeMap(a.named, false) == DescribeMap(b.named, false));
	const auto numberedA = DescribeMap(a.numbered, true), numberedB = DescribeMap(b.numbered, true);
	for (std::size_t i = 0; i < numberedA.size() && i < numberedB.size(); ++i)
		if (numberedA[i] != numberedB[i])
		{
			MESSAGE("numbered entry ", i, ": original ", numberedA[i], " ours ", numberedB[i]);
			break;
		}
	CHECK(numberedA == numberedB);
	CHECK(DescribeInfos(a.infos) == DescribeInfos(b.infos));
	CHECK((a.current == nullptr) == (b.current == nullptr));
	if (a.current != nullptr && b.current != nullptr)
		CHECK(a.current->index == b.current->index);
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TLocale")
{
	TEST_CASE("loading and lookups")
	{
		const fs::path directory = fs::temp_directory_path() / "jpog_locale_test";
		fs::create_directories(directory);
		WriteLocaleFiles(directory);
		{
			TFileManager manager;
			Toshi::TNativeFileSystem local("local");
			local.SetPrefix(directory.string().c_str());
			TLocale locale;
			CHECK(locale.LoadInfo("local:locales.txt"));
			REQUIRE(locale.FindLocaleInfo("Japanese") != nullptr);
			CHECK(locale.FindLocaleInfo("Japanese")->GetWordBreakStyle() == Toshi::TLocaleInfo::WORDBREAK_ANYWHERE);
			locale.ChangeLocale("English");
			CHECK(locale.GetCurrentLocaleInfo() == locale.FindLocaleInfo("English"));
			CHECK(locale.Get("HELLO").GetLocalisedString() == TWString("Hello again"));
			CHECK(locale.Get(12).GetLocalisedString() == TWString("Number twelve"));
			CHECK(locale.Get("PLATFORM").GetLocalisedString() == TWString("Windows"));
			CHECK_FALSE(locale.IsValid(0));
			// As in the original, Format prints the address of its arguments for %d.
			CHECK(locale.Format("COUNT", 7).GetLocalisedString().Find(TWString(" dinosaurs").GetString(0), 0) > 0);
			CHECK(locale.Format("HELLO", 7).GetLocalisedString() == TWString("Hello again"));
			const TLString temporary = locale.GetTemporary(42);
			CHECK(temporary.IsTemporary());
			CHECK(locale.GetTemporary(42) == temporary); // shared by text
			CHECK(locale.GetVolatile(42) != temporary);
		}
		fs::remove_all(directory);
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

TEST_SUITE("TLocale differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("locale matches the original")
	{
		const fs::path directory = fs::temp_directory_path() / "jpog_locale_diff";
		fs::create_directories(directory);
		WriteLocaleFiles(directory);

		using P = void*;
		const auto managerCtor = Original<Method<void>>("??0TFileManager@Toshi@@QAE@XZ");
		const auto managerDtor = Original<Method<void>>("??1TFileManager@Toshi@@QAE@XZ");
		const auto nativeCtor = Original<Method<void, const char*>>("??0TNativeFileSystem@Toshi@@QAE@PBD@Z");
		const auto nativeDtor = Original<Method<void>>("??1TNativeFileSystem@Toshi@@UAE@XZ");
		const auto setPrefix =
		    Original<Method<void, const TCString*>>("?SetPrefix@TFileSystem@Toshi@@UAEXABVTCString@2@@Z");
		const auto construct = Original<Method<void>>("??0TLocale@Toshi@@QAE@XZ");
		const auto destruct = Original<Method<void>>("??1TLocale@Toshi@@QAE@XZ");
		const auto loadInfo = Original<Method<bool, const TCString*>>("?LoadInfo@TLocale@Toshi@@QAE_NABVTCString@2@@Z");
		const auto changeLocale = Original<Method<void, const char*>>("?ChangeLocale@TLocale@Toshi@@QAEXPBD@Z");
		const auto getNamed = Original<Method<P, P, const char*>>("?Get@TLocale@Toshi@@QAE?AVTLString@2@PBD@Z");
		const auto getNumbered = Original<Method<P, P, int>>("?Get@TLocale@Toshi@@QAE?AVTLString@2@H@Z");
		const auto getTemporary =
		    Original<Method<P, P, const unsigned short*>>("?GetTemporary@TLocale@Toshi@@QAE?AVTLString@2@PBG@Z");
		const auto getVolatile = Original<Method<P, P, int>>("?GetVolatile@TLocale@Toshi@@QAE?AVTLString@2@H@Z");
		const auto isValid = Original<Method<bool, const char*>>("?IsValid@TLocale@Toshi@@QAE_NPBD@Z");
		const auto reset = Original<Method<void>>("?Reset@TLocale@Toshi@@QAEXXZ");
		const auto release = Original<Method<void>>("??1TLString@Toshi@@QAE@XZ");
		using FormatFn = P(__cdecl*)(P, P, const char*, int);
		const auto format = Original<FormatFn>("?Format@TLocale@Toshi@@QAA?AVTLString@2@PBDZZ");

		alignas(void*) unsigned char originalManager[sizeof(TFileManager)];
		alignas(void*) unsigned char originalLocal[sizeof(Toshi::TNativeFileSystem)];
		managerCtor(originalManager);
		nativeCtor(originalLocal, "local");
		const TCString prefix(directory.string().c_str());
		setPrefix(originalLocal, &prefix);
		auto* ourManager = new TFileManager();
		auto* ourLocal = new Toshi::TNativeFileSystem("local");
		ourLocal->SetPrefix(prefix);

		alignas(void*) unsigned char original[sizeof(TLocale)];
		construct(original);
		auto* ours = new TLocale();
		CheckSameLocale(original, ours);

		const TCString infoPath("local:locales.txt");
		CHECK(loadInfo(original, &infoPath) == ours->LoadInfo(infoPath));
		CheckSameLocale(original, ours);

		std::vector<void*> originalHandles;
		std::vector<TLString> ourHandles;
		auto keep = [&](void* handle) { originalHandles.push_back(*static_cast<void**>(handle)); };
		auto sameString = [](void* originalHandle, const TLString& ourHandle)
		{
			const auto* a = *static_cast<const StringView* const*>(originalHandle);
			const auto* b = *reinterpret_cast<const StringView* const*>(&ourHandle);
			if (Describe(a) != Describe(b))
				MESSAGE("original ", Describe(a), " ours ", Describe(b));
			return Describe(a) == Describe(b);
		};

		for (const char* name : {"English", "Japanese", "English"})
		{
			INFO(name);
			changeLocale(original, name);
			ours->ChangeLocale(name);
			CheckSameLocale(original, ours);
			for (const char* key : {"HELLO", "COUNT", "PLATFORM", "PARK", "MISSING"})
			{
				INFO(key);
				CHECK(isValid(original, key) == ours->IsValid(key));
				void* handle = nullptr;
				getNamed(original, &handle, key);
				const TLString string = ours->Get(key);
				CHECK(sameString(&handle, string));
				release(&handle);
			}
			for (int id : {0, 5, 12, 13})
			{
				void* handle = nullptr;
				getNumbered(original, &handle, id);
				const TLString string = ours->Get(id);
				CHECK(sameString(&handle, string));
				release(&handle);
			}
			// Temporaries: some kept alive, so later ones share them; formatted strings too.
			for (int i = 0; i < 6; ++i)
			{
				const unsigned short text[] = {'t', static_cast<unsigned short>('0' + i % 3), 0};
				void* handle = nullptr;
				getTemporary(original, &handle, text);
				TLString string = ours->GetTemporary(text);
				CHECK(sameString(&handle, string));
				if (i % 2 == 0)
				{
					keep(&handle);
					ourHandles.push_back(string);
				}
				else
					release(&handle);
				getVolatile(original, &handle, i);
				string = ours->GetVolatile(i);
				CHECK(sameString(&handle, string));
				release(&handle);
				format(original, &handle, "HELLO", i * 11); // with %d both would print stack addresses
				string = ours->Format("HELLO", i * 11);
				CHECK(sameString(&handle, string));
				release(&handle);
				string = TLString();
				CheckSameLocale(original, ours);
			}
		}

		reset(original);
		ours->Reset();
		CheckSameLocale(original, ours);

		for (void*& handle : originalHandles)
			release(&handle);
		ourHandles.clear();
		CheckSameLocale(original, ours);

		destruct(original);
		delete ours;
		delete ourLocal;
		delete ourManager;
		nativeDtor(originalLocal);
		managerDtor(originalManager);
		fs::remove_all(directory);
	}

	TEST_CASE("game locale files load like the original")
	{
		const fs::path directory = OriginalModuleDirectory() / "Data" / "GUI" / "Locales";
		if (!fs::exists(directory / "Locales.txt"))
			return;
		const auto managerCtor = Original<Method<void>>("??0TFileManager@Toshi@@QAE@XZ");
		const auto managerDtor = Original<Method<void>>("??1TFileManager@Toshi@@QAE@XZ");
		const auto nativeCtor = Original<Method<void, const char*>>("??0TNativeFileSystem@Toshi@@QAE@PBD@Z");
		const auto nativeDtor = Original<Method<void>>("??1TNativeFileSystem@Toshi@@UAE@XZ");
		const auto setPrefix =
		    Original<Method<void, const TCString*>>("?SetPrefix@TFileSystem@Toshi@@UAEXABVTCString@2@@Z");
		const auto construct = Original<Method<void>>("??0TLocale@Toshi@@QAE@XZ");
		const auto destruct = Original<Method<void>>("??1TLocale@Toshi@@QAE@XZ");
		const auto loadInfo = Original<Method<bool, const TCString*>>("?LoadInfo@TLocale@Toshi@@QAE_NABVTCString@2@@Z");
		const auto changeLocale = Original<Method<void, const char*>>("?ChangeLocale@TLocale@Toshi@@QAEXPBD@Z");

		alignas(void*) unsigned char originalManager[sizeof(TFileManager)];
		alignas(void*) unsigned char originalLocal[sizeof(Toshi::TNativeFileSystem)];
		managerCtor(originalManager);
		nativeCtor(originalLocal, "local");
		const TCString prefix(directory.string().c_str());
		setPrefix(originalLocal, &prefix);
		auto* ourManager = new TFileManager();
		auto* ourLocal = new Toshi::TNativeFileSystem("local");
		ourLocal->SetPrefix(prefix);

		alignas(void*) unsigned char original[sizeof(TLocale)];
		construct(original);
		auto* ours = new TLocale();
		const TCString infoPath("local:Locales.txt");
		CHECK(loadInfo(original, &infoPath) == ours->LoadInfo(infoPath));
		CheckSameLocale(original, ours);
		const auto& infos = static_cast<const LocaleView*>(static_cast<void*>(original))->infos;
		REQUIRE(infos.count > 0);
		std::vector<std::string> names;
		for (int i = 0; i < infos.count; ++i)
			names.push_back(Narrow(static_cast<const PooledView*>(infos.pairs[i].key)));
		for (const std::string& name : names)
		{
			INFO(name);
			changeLocale(original, name.c_str());
			ours->ChangeLocale(name.c_str());
			CheckSameLocale(original, ours);
		}
		MESSAGE(static_cast<const LocaleView*>(static_cast<void*>(original))->named.count, " named strings");

		destruct(original);
		delete ours;
		delete ourLocal;
		delete ourManager;
		nativeDtor(originalLocal);
		managerDtor(originalManager);
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
