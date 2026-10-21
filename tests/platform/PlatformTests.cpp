#include "platform/CRuntime.h"
#include "platform/System.h"
#include "platform/Thread.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <doctest/doctest.h>
#include <filesystem>
#include <string>
#include <vector>

#if defined(_WIN32)
#include <cwchar>
#endif

using namespace jpog::platform;

namespace
{

std::u16string U(const char* ascii)
{
	std::u16string text;
	for (; *ascii != '\0'; ++ascii)
		text.push_back(static_cast<char16_t>(static_cast<unsigned char>(*ascii)));
	return text;
}

const unsigned short* W(const std::u16string& text)
{
	return reinterpret_cast<const unsigned short*>(text.c_str());
}

std::string Narrow(const unsigned short* text, int length)
{
	std::string result;
	for (int i = 0; i < length; ++i)
		result.push_back(text[i] < 0x80 ? static_cast<char>(text[i]) : '?');
	return result;
}

// Formats with the portable formatter; on Windows also with the CRT, which must agree.
std::string Format(const char* format, ...)
{
	const std::u16string wideFormat = U(format);
	unsigned short buffer[256];
	va_list args;
	va_start(args, format);
	const int length = FormatWide(buffer, 256, W(wideFormat), args);
	va_end(args);
	REQUIRE(length >= 0);
	REQUIRE(length < 256);
#if defined(_WIN32)
	wchar_t crt[256];
	va_start(args, format);
#pragma warning(suppress : 4996) // the CRT function under comparison
	const int crtLength = _vsnwprintf(crt, 256, reinterpret_cast<const wchar_t*>(wideFormat.c_str()), args);
	va_end(args);
	CHECK(crtLength == length);
	CHECK(std::wstring(crt, static_cast<std::size_t>(crtLength)) ==
	      std::wstring(reinterpret_cast<const wchar_t*>(buffer), static_cast<std::size_t>(length)));
#endif
	return Narrow(buffer, length);
}

int CallVSNWPrintf(unsigned short* buffer, std::size_t count, const unsigned short* format, ...)
{
	va_list args;
	va_start(args, format);
	const int result = VSNWPrintf(buffer, count, format, args);
	va_end(args);
	return result;
}

int CallVSNPrintf(char* buffer, std::size_t count, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	const int result = VSNPrintf(buffer, count, format, args);
	va_end(args);
	return result;
}

} // namespace

TEST_SUITE("Platform")
{
	TEST_CASE("wide formatter follows the MSVC conversions")
	{
		const std::u16string wide = U("wide");
		CHECK(Format("%d|%5d|%-5d|%05d|%+d|% d", 42, 42, 42, 42, 42, 42) == "42|   42|42   |00042|+42| 42");
		CHECK(Format("%u|%x|%X|%#x|%o|%ld|%lu", 3000000000u, 255u, 255u, 255u, 8u, -7L, 7UL) ==
		      "3000000000|ff|FF|0xff|10|-7|7");
		CHECK(Format("%I64d|%lld|%llx|%I64u", -5LL, 1234567890123LL, 0xABCDEF123456LL, 18446744073709551615ULL) ==
		      "-5|1234567890123|abcdef123456|18446744073709551615");
		CHECK(Format("%hd|%hu|%hhd", 70000, 70000, 300) == "4464|4464|44");
		CHECK(Format("%.3f|%8.2f|%-8.1f|%e|%g|%g", 3.14159, 2.5, 1.25, 12345.678, 0.0001, 1e20) ==
		      "3.142|    2.50|1.2     |1.234568e+04|0.0001|1e+20");
		CHECK(Format("%*d|%-*d|%.*f", 5, 1, 4, 2, 2, 1.0) == "    1|2   |1.00");
		CHECK(Format("%s|%ls|%ws|%5s|%-5s|%.2s", W(wide), W(wide), W(wide), W(wide), W(wide), W(wide)) ==
		      "wide|wide|wide| wide|wide |wi");
		CHECK(Format("%hs|%S|%c|%hc|%C", "narrow", "narrow", 'w', 'n', 'n') == "narrow|narrow|w|n|n");
		CHECK(Format("100%%") == "100%");
	}

	TEST_CASE("_vsnwprintf and _vsnprintf truncation")
	{
		const std::u16string format = U("%d");
		unsigned short wide[4] = {9, 9, 9, 9};
		CHECK(CallVSNWPrintf(wide, 4, W(format), 12) == 2);
		CHECK(wide[2] == 0);
		CHECK(CallVSNWPrintf(wide, 4, W(format), 1234) == 4); // fills the buffer, no terminator
		CHECK(CallVSNWPrintf(wide, 4, W(format), 12345) == -1);

		char narrow[4] = {9, 9, 9, 9};
		CHECK(CallVSNPrintf(narrow, 4, "%d", 12) == 2);
		CHECK(narrow[2] == '\0');
		CHECK(CallVSNPrintf(narrow, 4, "%d", 1234) == 4);
		CHECK(CallVSNPrintf(narrow, 4, "%d", 12345) == -1);
		CHECK(std::string(narrow, 4) == "1234");
	}

	TEST_CASE("wide string functions")
	{
		const std::u16string a = U("Hello"), b = U("hello"), c = U("Help");
		CHECK(WcsLen(W(a)) == 5);
		CHECK(WcsCmp(W(a), W(a)) == 0);
		CHECK(WcsCmp(W(a), W(b)) < 0);
		CHECK(WcsICmp(W(a), W(b)) == 0);
		CHECK(WcsNICmp(W(a), W(c), 3) == 0);
		CHECK(WcsNCmp(W(a), W(c), 4) < 0);
		CHECK(WcsChr(W(a), 'l') == W(a) + 2);
		CHECK(WcsChr(W(a), 'z') == nullptr);
		CHECK(WcsStr(W(a), W(U("llo"))) == W(a) + 2);
		unsigned short copy[8] = {1, 1, 1, 1, 1, 1, 1, 1};
		WcsNCpy(copy, W(U("ab")), 4);
		CHECK((copy[0] == 'a' && copy[1] == 'b' && copy[2] == 0 && copy[3] == 0 && copy[4] == 1));
		CHECK(StrICmp("ABC", "abc") == 0);
		CHECK(StrNICmp("ABCx", "abcy", 3) == 0);
		CHECK(StrICmp("a", "b") < 0);
	}

	TEST_CASE("semaphores, mutexes and locks")
	{
		void* semaphore = SemaphoreCreate(1, 2);
		REQUIRE(semaphore != nullptr);
		CHECK(SemaphoreWait(semaphore, true));
		CHECK_FALSE(SemaphoreWait(semaphore, true));
		CHECK(SemaphoreSignal(semaphore));
		CHECK(SemaphoreSignal(semaphore));
		CHECK_FALSE(SemaphoreSignal(semaphore)); // over the maximum
		CHECK(SemaphoreDestroy(semaphore));

		void* mutex = MutexCreate();
		CHECK(MutexLock(mutex, false));
		CHECK(MutexLock(mutex, true)); // recursive
		CHECK(MutexUnlock(mutex));
		CHECK(MutexUnlock(mutex));
		CHECK(MutexDestroy(mutex));

		alignas(void*) unsigned char lock[kLockStorageSize];
		LockInit(lock);
		LockEnter(lock);
		LockEnter(lock);
		LockLeave(lock);
		LockLeave(lock);
		LockDestroy(lock);
	}

	TEST_CASE("threads start suspended and run")
	{
		struct Context
		{
			void* done;
			unsigned int seenId;
		} context{SemaphoreCreate(0, 1), 0};
		ThreadEntry entry = [](void* argument) -> unsigned int
		{
			auto* context = static_cast<Context*>(argument);
			context->seenId = ThreadCurrentId();
			SemaphoreSignal(context->done);
			return 0;
		};
		unsigned int id = 0;
		void* thread = ThreadCreate(0, entry, &context, true, id);
		REQUIRE(thread != nullptr);
		CHECK(id != 0);
		CHECK(id != ThreadCurrentId());
		ThreadSetPriority(thread, 1);
		CHECK(ThreadGetPriority(thread) == 1);
		CHECK_FALSE(SemaphoreWait(context.done, true));
		CHECK(ThreadResume(thread));
		CHECK(SemaphoreWait(context.done, false));
		CHECK(context.seenId == id);
		ThreadClose(thread);
		SemaphoreDestroy(context.done);
	}

	TEST_CASE("files, case-insensitive paths and listings")
	{
		const std::filesystem::path directory = std::filesystem::temp_directory_path() / "jpog_platform_test";
		std::filesystem::remove_all(directory);
		REQUIRE(DirectoryMake(directory.string().c_str()));
		const std::string file = (directory / "Data.TXT").string();

		void* handle = FileOpen(file.c_str(), FileAccess::Write, true);
		REQUIRE(handle != FileInvalid());
		int written = 0;
		CHECK(FileWrite(handle, "abcdef", 6, written));
		CHECK(written == 6);
		CHECK(FileWriteTime(handle) > 116444736000000000ULL); // after 1970
		FileClose(handle);

		// Different case and `\` separators resolve to the same file everywhere.
		std::string variant = (directory / "data.txt").string();
		for (char& c : variant)
			if (c == '/')
				c = '\\';
		handle = FileOpen(variant.c_str(), FileAccess::Read, false);
		REQUIRE(handle != FileInvalid());
		CHECK(FileSeek(handle, 0, FileOrigin::End) == 6);
		CHECK(FileSeek(handle, 2, FileOrigin::Begin) == 2);
		char buffer[8] = {};
		int read = 0;
		CHECK(FileRead(handle, buffer, 8, read));
		CHECK(read == 4);
		CHECK(std::string(buffer, 4) == "cdef");
		FileClose(handle);

		FindEntry entry;
		void* find = FindFirst((directory.string() + "\\*").c_str(), entry);
		REQUIRE(find != FileInvalid());
		std::vector<std::string> names{entry.name};
		while (FindNext(find, entry))
			names.emplace_back(entry.name);
		CHECK(std::find(names.begin(), names.end(), "Data.TXT") != names.end());
		CHECK(std::find(names.begin(), names.end(), ".") != names.end());

		CHECK(FileDelete(variant.c_str()));
		CHECK(FileOpen(file.c_str(), FileAccess::Read, false) == FileInvalid());
		std::filesystem::remove_all(directory);
	}

	TEST_CASE("timer")
	{
		CHECK(PerformanceFrequency() > 0);
		const long long before = PerformanceCounter();
		CHECK(PerformanceCounter() >= before);
	}
}
