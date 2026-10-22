#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/strings/TCString.h"
#include "toshi/kernel/strings/TWString.h"

#include <cstring>
#include <doctest/doctest.h>
#include <string>
#include <vector>

using Toshi::TCString;
using Toshi::TSystem;
using Toshi::TWString;
using namespace jpog::test;

namespace
{

using U16 = unsigned short;

std::u16string Widen(const std::string& text)
{
	return std::u16string(text.begin(), text.end());
}

const U16* W(const std::u16string& text)
{
	return reinterpret_cast<const U16*>(text.c_str());
}

// Short strings from a small alphabet (mixed case, so case-insensitive paths and repeats occur).
std::string RandomText(InputGenerator& gen, int maxLength = 12)
{
	static const char alphabet[] = "abcABC xyzXYZ_09";
	std::string text;
	const int length = gen.NextInt(0, maxLength);
	for (int i = 0; i < length; ++i)
		text += alphabet[gen.NextU32() % (sizeof(alphabet) - 1)];
	return text;
}

int Sign(int value)
{
	return (value > 0) - (value < 0);
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TString")
{
	TEST_CASE("TCString basics")
	{
		TCString text("Hello");
		text += ", World";
		CHECK(text == "Hello, World");
		CHECK(text.Length() == 12);
		CHECK(text.Find("World", 0) == 7);
		CHECK(text.FindReverse('o', -1) == 8);
		CHECK(text.Mid(7, 5) == "World");
		text.Truncate(5);
		CHECK(text == "Hello");
		CHECK(text.ExcessLength() == 7); // the buffer is kept
		text.MakeUpper();
		CHECK(text.IsAllUpperCase());
		TCString formatted;
		formatted.Format("%d-%s", 42, "x");
		CHECK(formatted == "42-x");
		const TCString empty;
		CHECK(empty.IsEmpty());
		CHECK(static_cast<const char*>(empty)[0] == '\0');
	}

	TEST_CASE("TWString and conversions")
	{
		const std::u16string hello = u"Hello";
		TWString wide(W(hello));
		TCString narrow(wide);
		CHECK(narrow == "Hello");
		TWString back(narrow);
		CHECK(back == wide);
		CHECK(back.IsUnicode());
		CHECK(TSystem::HashString("ab") == static_cast<unsigned int>('a' * 5 + 'b'));
		CHECK(TSystem::BCDToInteger(0x1234) == 1234);
	}

	TEST_CASE("free comparison operators are reversed")
	{
		const TCString b("b");
		CHECK("b" == b);
		CHECK("a" != b);
		CHECK("a" > b); // b > "a"
		CHECK("c" < b);
		CHECK("b" <= b);
		CHECK("b" >= b);
		const U16 wa[] = {'a', 0};
		const TWString wb("b");
		CHECK(wa > wb);
		CHECK(wa != wb);
		CHECK_FALSE(wa == wb);
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

constexpr int kRounds = 300;

// Field view shared by both implementations (layout asserted in the headers).
struct StringView
{
	void* buffer;
	uint32_t lengthAndExcess;
};

// An original TCString or TWString living in local storage, managed through original exports.
template <typename Char> struct OriginalString
{
	alignas(void*) unsigned char storage[8];
	StringView& view() { return *reinterpret_cast<StringView*>(storage); }
	const Char* data() { return static_cast<const Char*>(view().buffer); }
};

template <typename Ours, typename Char>
void CheckSame(OriginalString<Char>& original, const Ours& ours, const char* step)
{
	const auto& view = reinterpret_cast<const StringView&>(ours);
	INFO("after ", doctest::String(step));
	REQUIRE(original.view().lengthAndExcess == view.lengthAndExcess); // length and spare capacity
	const int length = static_cast<int>(view.lengthAndExcess & 0xFFFFFF);
	CHECK(std::memcmp(original.data(), view.buffer, (length + 1) * sizeof(Char)) == 0);
}

} // namespace

TEST_SUITE("TString differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("TSystem string helpers")
	{
		using LenC = int(TSTDCALL*)(const char*);
		using LenW = int(TSTDCALL*)(const U16*);
		using CmpC = int(TSTDCALL*)(const char*, const char*, int);
		using CmpW = int(TSTDCALL*)(const U16*, const U16*, int);
		using HashC = unsigned int(TSTDCALL*)(const char*);
		using HashW = unsigned int(TSTDCALL*)(const U16*);
		using ToW = const U16*(TSTDCALL*)(U16*, const char*, int);
		using ToC = const char*(TSTDCALL*)(char*, const U16*, int);
		using FindC = const char*(TSTDCALL*)(const char*, char);
		using IntToStr = const char*(TSTDCALL*)(int, char*, int);
		const auto lenC = Original<LenC>("?StringLength@TSystem@Toshi@@SGHPBD@Z");
		const auto lenW = Original<LenW>("?StringLength@TSystem@Toshi@@SGHPBG@Z");
		const auto cmpC = Original<CmpC>("?StringCompare@TSystem@Toshi@@SGHPBD0H@Z");
		const auto cmpW = Original<CmpW>("?StringCompare@TSystem@Toshi@@SGHPBG0H@Z");
		const auto icmpC = Original<CmpC>("?StringCompareNoCase@TSystem@Toshi@@SGHPBD0H@Z");
		const auto icmpW = Original<CmpW>("?StringCompareNoCase@TSystem@Toshi@@SGHPBG0H@Z");
		const auto hashC = Original<HashC>("?HashString@TSystem@Toshi@@SGIPBD@Z");
		const auto hashW = Original<HashW>("?HashString@TSystem@Toshi@@SGIPBG@Z");
		const auto toW = Original<ToW>("?StringCharToUnicode@TSystem@Toshi@@SGPBGPAGPBDH@Z");
		const auto toC = Original<ToC>("?StringUnicodeToChar@TSystem@Toshi@@SGPBDPADPBGH@Z");
		const auto findC = Original<FindC>("?StringFindChar@TSystem@Toshi@@SGPBDPBDD@Z");
		const auto intToStr = Original<IntToStr>("?StringIntToString@TSystem@Toshi@@SGPBDHPADH@Z");
		const auto bcd = Original<unsigned int(TSTDCALL*)(unsigned int)>("?BCDToInteger@TSystem@Toshi@@SGII@Z");

		InputGenerator gen;
		for (int i = 0; i < 2000; ++i)
		{
			const std::string a = RandomText(gen), b = (i % 5 == 0) ? a : RandomText(gen);
			const std::u16string wa = Widen(a), wb = Widen(b);
			const int count = gen.NextInt(-1, 8);
			INFO("a='", a.c_str(), "' b='", b.c_str(), "' count=", count);
			CHECK(lenC(a.c_str()) == TSystem::StringLength(a.c_str()));
			CHECK(lenW(W(wa)) == TSystem::StringLength(W(wa)));
			CHECK(cmpC(a.c_str(), b.c_str(), -1) == TSystem::StringCompare(a.c_str(), b.c_str(), -1)); // exact
			CHECK(Sign(cmpC(a.c_str(), b.c_str(), count)) == Sign(TSystem::StringCompare(a.c_str(), b.c_str(), count)));
			CHECK(Sign(cmpW(W(wa), W(wb), count)) == Sign(TSystem::StringCompare(W(wa), W(wb), count)));
			CHECK(Sign(icmpC(a.c_str(), b.c_str(), count)) ==
			      Sign(TSystem::StringCompareNoCase(a.c_str(), b.c_str(), count)));
			CHECK(Sign(icmpW(W(wa), W(wb), count)) == Sign(TSystem::StringCompareNoCase(W(wa), W(wb), count)));
			CHECK(hashC(a.c_str()) == TSystem::HashString(a.c_str()));
			CHECK(hashW(W(wa)) == TSystem::HashString(W(wa)));

			U16 originalWide[32], ourWide[32];
			std::memset(originalWide, 0xCC, sizeof(originalWide));
			std::memset(ourWide, 0xCC, sizeof(ourWide));
			toW(originalWide, a.c_str(), count);
			TSystem::StringCharToUnicode(ourWide, a.c_str(), count);
			CHECK(std::memcmp(originalWide, ourWide, sizeof(ourWide)) == 0);
			char originalNarrow[32], ourNarrow[32];
			std::memset(originalNarrow, 0xCC, sizeof(originalNarrow));
			std::memset(ourNarrow, 0xCC, sizeof(ourNarrow));
			toC(originalNarrow, W(wa), count);
			TSystem::StringUnicodeToChar(ourNarrow, W(wa), count);
			CHECK(std::memcmp(originalNarrow, ourNarrow, sizeof(ourNarrow)) == 0);
			const char c = "aZ_q"[i % 4];
			const char* foundOriginal = findC(a.c_str(), c);
			const char* foundOurs = TSystem::StringFindChar(a.c_str(), c);
			CHECK(foundOriginal == foundOurs);

			const int value = static_cast<int>(gen.NextU32());
			static constexpr int kRadixes[] = {8, 10, 16, 2};
			const int radix = kRadixes[i % 4];
			char originalNumber[40] = "-", ourNumber[40] = "-";
			intToStr(value, originalNumber, radix);
			TSystem::StringIntToString(value, ourNumber, radix);
			CHECK(std::strcmp(originalNumber, ourNumber) == 0);
			const unsigned int packed = gen.NextU32();
			CHECK(bcd(packed) == TSystem::BCDToInteger(packed));
		}
	}

	TEST_CASE("TCString operations match the original")
	{
		const auto ctorStr = Original<Method<void, const char*>>("??0TCString@Toshi@@QAE@PBD@Z");
		const auto dtor = Original<Method<void>>("??1TCString@Toshi@@QAE@XZ");
		const auto copy = Original<Method<void, const char*, int>>("?Copy@TCString@Toshi@@QAEXPBDH@Z");
		const auto copyStr = Original<Method<void, const void*, int>>("?Copy@TCString@Toshi@@QAEXABV12@H@Z");
		const auto concat = Original<Method<void*, const char*, int>>("?Concat@TCString@Toshi@@QAEAAV12@PBDH@Z");
		const auto truncate = Original<Method<void, int>>("?Truncate@TCString@Toshi@@QAEXH@Z");
		const auto lower = Original<Method<void*>>("?MakeLower@TCString@Toshi@@QAEAAV12@XZ");
		const auto upper = Original<Method<void*>>("?MakeUpper@TCString@Toshi@@QAEAAV12@XZ");
		const auto mid = Original<Method<void*, void*, int, int>>("?Mid@TCString@Toshi@@QBE?AV12@HH@Z");
		const auto find = Original<Method<int, const char*, int>>("?Find@TCString@Toshi@@QBEHPBDH@Z");
		const auto findChar = Original<Method<int, char, int>>("?Find@TCString@Toshi@@QBEHDH@Z");
		const auto findReverse = Original<Method<int, char, int>>("?FindReverse@TCString@Toshi@@QBEHDH@Z");
		const auto compare = Original<Method<int, const char*, int>>("?Compare@TCString@Toshi@@QBEHPBDH@Z");
		const auto compareNoCase = Original<Method<int, const char*, int>>("?CompareNoCase@TCString@Toshi@@QBEHPBDH@Z");
		const auto allLower = Original<Method<bool>>("?IsAllLowerCase@TCString@Toshi@@QBE_NXZ");
		const auto allUpper = Original<Method<bool>>("?IsAllUpperCase@TCString@Toshi@@QBE_NXZ");
		const auto format =
		    Original<void*(__cdecl*)(void*, const char*, ...)>("?Format@TCString@Toshi@@QAAAAV12@PBDZZ");

		InputGenerator gen;
		for (int round = 0; round < kRounds; ++round)
		{
			const std::string initial = RandomText(gen);
			OriginalString<char> original;
			ctorStr(original.storage, initial.c_str());
			TCString ours(initial.c_str());
			CheckSame(original, ours, "construction");
			for (int step = 0; step < 30; ++step)
			{
				const std::string text = RandomText(gen, 20);
				const int count = gen.NextInt(-1, 25);
				switch (gen.NextU32() % 9)
				{
				case 0:
					copy(original.storage, text.c_str(), count);
					ours.Copy(text.c_str(), count);
					CheckSame(original, ours, "Copy");
					break;
				case 1:
				case 2:
					concat(original.storage, text.c_str(), count);
					ours.Concat(text.c_str(), count);
					CheckSame(original, ours, "Concat");
					break;
				case 3:
					truncate(original.storage, count < 0 ? 0 : count);
					ours.Truncate(count < 0 ? 0 : count);
					CheckSame(original, ours, "Truncate");
					break;
				case 4:
					(count & 1) ? lower(original.storage) : upper(original.storage);
					(count & 1) ? ours.MakeLower() : ours.MakeUpper();
					CheckSame(original, ours, "MakeLower/Upper");
					break;
				case 5:
				{
					OriginalString<char> part;
					const int start = gen.NextInt(-2, 15);
					mid(original.storage, part.storage, start, count);
					const TCString ourPart = ours.Mid(start, count);
					CheckSame(part, ourPart, "Mid");
					dtor(part.storage);
					break;
				}
				case 6:
				{
					OriginalString<char> source;
					ctorStr(source.storage, text.c_str());
					const TCString ourSource(text.c_str());
					copyStr(original.storage, source.storage, count);
					ours.Copy(ourSource, count);
					CheckSame(original, ours, "Copy(TCString)");
					dtor(source.storage);
					break;
				}
				case 7:
					format(original.storage, "%s|%d|%x", text.c_str(), count, step);
					ours.Format("%s|%d|%x", text.c_str(), count, step);
					CheckSame(original, ours, "Format");
					break;
				default:
				{
					const int start = gen.NextInt(-2, 22);
					const char c = "aZ x"[step % 4];
					CHECK(find(original.storage, text.c_str(), start) == ours.Find(text.c_str(), start));
					CHECK(findChar(original.storage, c, start) == ours.Find(c, start));
					CHECK(findReverse(original.storage, c, start) == ours.FindReverse(c, start));
					CHECK(findReverse(original.storage, c, -1) == ours.FindReverse(c, -1));
					CHECK(compare(original.storage, text.c_str(), -1) == ours.Compare(text.c_str(), -1));
					CHECK(Sign(compare(original.storage, text.c_str(), count)) ==
					      Sign(ours.Compare(text.c_str(), count)));
					CHECK(Sign(compareNoCase(original.storage, text.c_str(), count)) ==
					      Sign(ours.CompareNoCase(text.c_str(), count)));
					CHECK(allLower(original.storage) == ours.IsAllLowerCase());
					CHECK(allUpper(original.storage) == ours.IsAllUpperCase());
					break;
				}
				}
			}
			dtor(original.storage);
		}
	}

	TEST_CASE("TWString operations match the original")
	{
		const auto ctorStr = Original<Method<void, const U16*>>("??0TWString@Toshi@@QAE@PBG@Z");
		const auto ctorNarrow = Original<Method<void, const char*>>("??0TWString@Toshi@@QAE@PBD@Z");
		const auto dtor = Original<Method<void>>("??1TWString@Toshi@@QAE@XZ");
		const auto copy = Original<Method<void, const U16*, int>>("?Copy@TWString@Toshi@@QAEXPBGH@Z");
		const auto copyNarrow = Original<Method<void, const char*, int>>("?Copy@TWString@Toshi@@QAEXPBDH@Z");
		const auto copySelf = Original<Method<void, const void*, int>>("?Copy@TWString@Toshi@@QAEXABV12@H@Z");
		const auto concat = Original<Method<void*, const U16*, int>>("?Concat@TWString@Toshi@@QAEAAV12@PBGH@Z");
		const auto truncate = Original<Method<void, int>>("?Truncate@TWString@Toshi@@QAEXH@Z");
		const auto mid = Original<Method<void*, void*, int, int>>("?Mid@TWString@Toshi@@QBE?AV12@HH@Z");
		const auto find = Original<Method<int, const U16*, int>>("?Find@TWString@Toshi@@QBEHPBGH@Z");
		const auto findReverse = Original<Method<int, U16, int>>("?FindReverse@TWString@Toshi@@QBEHGH@Z");
		const auto lower = Original<Method<void*>>("?MakeLower@TWString@Toshi@@QAEAAV12@XZ");

		InputGenerator gen;
		for (int round = 0; round < kRounds; ++round)
		{
			const std::u16string initial = Widen(RandomText(gen));
			OriginalString<U16> original;
			ctorStr(original.storage, W(initial));
			TWString ours(W(initial));
			CheckSame(original, ours, "construction");
			for (int step = 0; step < 30; ++step)
			{
				const std::string narrow = RandomText(gen, 20);
				const std::u16string text = Widen(narrow);
				const int count = gen.NextInt(-1, 25);
				switch (gen.NextU32() % 7)
				{
				case 0:
					copy(original.storage, W(text), count);
					ours.Copy(W(text), count);
					CheckSame(original, ours, "Copy");
					break;
				case 1:
					copyNarrow(original.storage, narrow.c_str(), count);
					ours.Copy(narrow.c_str(), count);
					CheckSame(original, ours, "Copy(char)");
					break;
				case 2:
					concat(original.storage, W(text), count);
					ours.Concat(W(text), count);
					CheckSame(original, ours, "Concat");
					break;
				case 3:
					truncate(original.storage, count < 0 ? 0 : count);
					ours.Truncate(count < 0 ? 0 : count);
					CheckSame(original, ours, "Truncate");
					break;
				case 4:
				{
					OriginalString<U16> part;
					const int start = gen.NextInt(-2, 15);
					mid(original.storage, part.storage, start, count);
					const TWString ourPart = ours.Mid(start, count);
					CheckSame(part, ourPart, "Mid");
					dtor(part.storage);
					break;
				}
				case 5:
					if (step % 7 == 0)
					{
						copySelf(original.storage, original.storage, -1); // self-copy quirk
						ours.Copy(ours, -1);
						CheckSame(original, ours, "self Copy");
						// The string now holds zeros with its length kept; a later reallocation copies
						// only up to the first zero, leaving uninitialised heap bytes. Resync.
						copy(original.storage, W(text), -1);
						ours.Copy(W(text), -1);
					}
					else
					{
						lower(original.storage);
						ours.MakeLower();
						CheckSame(original, ours, "MakeLower");
					}
					break;
				default:
				{
					const int start = gen.NextInt(-2, 22);
					CHECK(find(original.storage, W(text), start) == ours.Find(W(text), start));
					const U16 c = u"aZ x"[step % 4];
					CHECK(findReverse(original.storage, c, start) == ours.FindReverse(c, start));
					break;
				}
				}
			}
			dtor(original.storage);
			OriginalString<U16> fromNarrow;
			ctorNarrow(fromNarrow.storage, "Mixed Case 123");
			CheckSame(fromNarrow, TWString("Mixed Case 123"), "TWString(const char*)");
			dtor(fromNarrow.storage);
		}
	}

	TEST_CASE("free comparison operators match the original")
	{
		using Op = bool(TSTDCALL*)(const char*, const TCString*);
		using WideOp = bool(TSTDCALL*)(const U16*, const TWString*);
		const char* codes[] = {"8", "9", "M", "N", "O", "P"};
		const char* texts[] = {"", "a", "b", "ab", "B", "abc"};
		const U16 wide[][4] = {{0}, {'a', 0}, {'b', 0}, {'a', 'b', 0}, {'B', 0}, {'a', 'b', 'c', 0}};
		for (int op = 0; op < 6; ++op)
		{
			const auto original =
			    Original<Op>((std::string("??") + codes[op] + "Toshi@@YG_NPBDABVTCString@0@@Z").c_str());
			const auto originalWide =
			    Original<WideOp>((std::string("??") + codes[op] + "Toshi@@YG_NPBGABVTWString@0@@Z").c_str());
			for (int i = 0; i < 6; ++i)
				for (int j = 0; j < 6; ++j)
				{
					INFO("op ", op, " ", texts[i], " vs ", texts[j]);
					const TCString other(texts[j]);
					const TWString otherWide(wide[j]);
					const bool ours[] = {texts[i] == other, texts[i] != other,
					                     texts[i]<other, texts[i] <= other, texts[i]> other, texts[i] >= other};
					const bool oursWide[] = {wide[i] == otherWide, wide[i] != otherWide,
					                         wide[i]<otherWide, wide[i] <= otherWide, wide[i]> otherWide,
					                         wide[i] >= otherWide};
					CHECK(original(texts[i], &other) == ours[op]);
					CHECK(originalWide(wide[i], &otherWide) == oursWide[op]);
				}
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
