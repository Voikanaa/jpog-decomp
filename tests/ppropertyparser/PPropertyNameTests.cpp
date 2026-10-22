#include "ppropertyparser/PPropertyName.h"
#include "support/Differential.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TSystem.h"

#include <doctest/doctest.h>
#include <string>

using Toshi::TPCString;
using Toshi::TSystem;
using namespace jpog::test;

namespace
{

TPCString Pooled(const char* text)
{
	return TSystem::GetCStringPool()->Get(text);
}

std::string Text(const TPCString& string)
{
	return string.GetCString().GetString(0);
}

// Names that exercise Split: brackets missing, empty, repeated or out of order.
const char* const kNames[] = {"Name",      "Name[Sub]",     "Name[]", "[Sub]", "Name[Sub",
                              "Name]Sub[", "a[b][c]",       "a[b]c",  "[",     "]",
                              "x[[y]]",    "Screen[Width]", "b",      "a[a]",  ""};

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("PPropertyName")
{
	TEST_CASE("split, string and ordering")
	{
		const PPropertyName plain(Pooled("Width"));
		CHECK(Text(plain.GetName()) == "Width");
		CHECK(plain.GetSubName().IsEmpty());
		CHECK(Text(plain.GetString()) == "Width");

		const PPropertyName split(Pooled("Screen[Width]"));
		CHECK(Text(split.GetName()) == "Screen");
		CHECK(Text(split.GetSubName()) == "Width");
		CHECK(Text(split.GetString()) == "Screen[Width]");
		CHECK(std::string(Toshi::TCString("x ") + split) == "x Screen[Width]");
		CHECK(std::string(Toshi::TCString("x ") + plain) == "x Width");

		const PPropertyName open(Pooled("Screen[Width"));
		CHECK(Text(open.GetName()) == "Screen");
		CHECK(open.GetSubName().IsEmpty());

		PPropertyName copy(split);
		CHECK(copy == split);
		copy.SetSubName(Pooled("Height"));
		CHECK(copy != split);
		CHECK(copy < split);
		CHECK(split > copy);
		copy = plain;
		CHECK(copy == plain);
		CHECK(PPropertyName(Pooled("a"), Pooled("b")) == PPropertyName(Pooled("a[b]")));
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original PPropertyParser.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

// The original's objects hold handles from the original kernel's string pool; both kernels
// share the layout (TPooledCString: count, TCString{buffer, length}, pool).
struct RawName
{
	void* name;
	void* subName;
};

std::string RawText(void* handle)
{
	if (handle == nullptr)
		return "";
	const char* buffer = *reinterpret_cast<const char* const*>(static_cast<const char*>(handle) + 4);
	return buffer ? buffer : "";
}

} // namespace

TEST_SUITE("PPropertyName differential" * doctest::skip(PPropertyParserMissing()))
{
	TEST_CASE("names match the original")
	{
		using P = void*;
		const auto pool = Original<P(TSTDCALL*)()>("?GetCStringPool@TSystem@Toshi@@SGPAVTCStringPool@2@XZ");
		const auto get = Original<Method<P*, P*, const char*>>("?Get@TCStringPool@Toshi@@QAE?AVTPCString@2@PBD@Z");
		const auto construct = OriginalPP<Method<void, P*>>("??0PPropertyName@@QAE@ABVTPCString@Toshi@@@Z");
		const auto getString = OriginalPP<Method<P*, P*>>("?GetString@PPropertyName@@QBE?AVTPCString@Toshi@@XZ");
		const auto equal = OriginalPP<Method<bool, const RawName*>>("??8PPropertyName@@QBE_NABV0@@Z");
		const auto notEqual = OriginalPP<Method<bool, const RawName*>>("??9PPropertyName@@QBE_NABV0@@Z");
		const auto less = OriginalPP<Method<bool, const RawName*>>("??MPPropertyName@@QBE_NABV0@@Z");
		const auto greater = OriginalPP<Method<bool, const RawName*>>("??OPPropertyName@@QBE_NABV0@@Z");
		const auto constructString = Original<Method<void, const char*>>("??0TCString@Toshi@@QAE@PBD@Z");
		const auto destructString = Original<Method<void>>("??1TCString@Toshi@@QAE@XZ");
		const auto append = OriginalPP<void*(TSTDCALL*)(void*, const void*, const RawName*)>(
		    "??H@YG?AVTCString@Toshi@@ABV01@ABVPPropertyName@@@Z");

		// The original names are leaked: their handles belong to the original kernel's pool.
		RawName originals[std::size(kNames)];
		for (size_t i = 0; i < std::size(kNames); ++i)
		{
			INFO("name '", kNames[i], "'");
			P handle = nullptr;
			get(pool(), &handle, kNames[i]);
			construct(&originals[i], &handle);
			const PPropertyName ours(Pooled(kNames[i]));
			CHECK(RawText(originals[i].name) == Text(ours.GetName()));
			CHECK(RawText(originals[i].subName) == Text(ours.GetSubName()));
			P string = nullptr;
			getString(&originals[i], &string);
			CHECK(RawText(string) == Text(ours.GetString()));

			alignas(void*) unsigned char prefix[8], appended[8];
			constructString(prefix, "p:");
			append(appended, prefix, &originals[i]);
			CHECK(std::string(*reinterpret_cast<const char* const*>(appended)) ==
			      static_cast<const char*>(Toshi::TCString("p:") + ours));
			destructString(appended);
			destructString(prefix);
		}
		for (size_t i = 0; i < std::size(kNames); ++i)
			for (size_t j = 0; j < std::size(kNames); ++j)
			{
				INFO("'", kNames[i], "' vs '", kNames[j], "'");
				const PPropertyName a(Pooled(kNames[i])), b(Pooled(kNames[j]));
				CHECK(equal(&originals[i], &originals[j]) == (a == b));
				CHECK(notEqual(&originals[i], &originals[j]) == (a != b));
				CHECK(less(&originals[i], &originals[j]) == (a < b));
				CHECK(greater(&originals[i], &originals[j]) == (a > b));
			}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
