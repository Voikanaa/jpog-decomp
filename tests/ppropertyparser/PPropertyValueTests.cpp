#include "ppropertyparser/PProperties.h"
#include "ppropertyparser/PPropertyValue.h"
#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/math/TVector3.h"

#include <cmath>
#include <doctest/doctest.h>
#include <limits>
#include <string>
#include <vector>

using Toshi::TPCString;
using Toshi::TSystem;
using namespace jpog::test;

namespace
{

TPCString Pooled(const char* text)
{
	return TSystem::GetCStringPool()->Get(text);
}

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("PPropertyValue")
{
	TEST_CASE("types, conversions and assignment")
	{
		PPropertyValue value;
		CHECK_FALSE(value.IsDefined());
		CHECK(value.IsEmpty());
		value = 42;
		CHECK(value.IsType(PPropertyValue::TYPE_INT));
		CHECK(value.GetFloat() == 42.0f);
		CHECK(value.CanBeType(PPropertyValue::TYPE_FLOAT));
		value = 2.5f;
		CHECK(value.GetInteger() == 2);
		CHECK_FALSE(value.CanBeType(PPropertyValue::TYPE_INT));
		value = 3.0f;
		CHECK(value.CanBeType(PPropertyValue::TYPE_INT));
		value = Pooled("text");
		CHECK(value == "text");
		CHECK(value.GetInteger() == 0);
		CHECK(std::string(PPropertyValue::ToString(value.GetType())) == "TPCString");
		CHECK(std::string(PPropertyValue::ToString(PPropertyValue::TYPE_UNDEF)) == "TNULL");

		const PPropertyValue copy(value);
		CHECK(copy == value);
		value.SetUndef();
		CHECK(copy != value);

		auto* array = new PPropertyValueArray(3);
		array->GetValues().Push(PPropertyValue(1));
		array->GetValues().Push(PPropertyValue(2.0f));
		array->GetValues().Push(PPropertyValue(3u));
		value = array;
		Toshi::TVector3 vector;
		CHECK(value.GetArray()->GetTVector3(vector));
		CHECK(vector.x == 1.0f);
		CHECK(vector.z == 3.0f);
		value.SetUndef(); // releases the array
	}

	TEST_CASE("property sets")
	{
		auto* properties = new PProperties();
		const PPropertyValue holder(properties); // holds a reference
		properties->PutProperty(Pooled("Width"), PPropertyValue(640));
		properties->PutProperty(Pooled("Screen[Depth]"), PPropertyValue(32));
		properties->PutProperty(Pooled("Width"), PPropertyValue(800)); // duplicate
		CHECK(properties->GetPropertyCount() == 3);
		CHECK(properties->GetProperty(Pooled("Width"))->GetInteger() == 640);
		CHECK(properties->GetProperty(Pooled("Screen"), Pooled("Depth"))->GetInteger() == 32);
		CHECK(properties->GetProperty(Pooled("Screen")) == nullptr);
		CHECK(properties->GetRequiredProperty(Pooled("Width"), PPropertyValue::TYPE_FLOAT) == nullptr);

		properties->PutPropertyUnique(Pooled("Width"), PPropertyValue(1024));
		CHECK(properties->GetPropertyCount() == 3);
		CHECK(properties->GetProperty(Pooled("Width"))->GetInteger() == 1024);

		properties->PutProperty(Pooled("Item"), Pooled("2"), PPropertyValue(true));
		CHECK(properties->GetProperty(Pooled("Item"), 2)->GetBoolean());

		auto position = properties->Begin();
		const auto* first = properties->FindProperty(Pooled("Width"), position);
		REQUIRE(first != nullptr);
		CHECK(first->GetValue()->GetInteger() == 1024);
		const auto* second = properties->FindProperty(Pooled("Width"), position);
		REQUIRE(second != nullptr);
		CHECK(second->GetValue()->GetInteger() == 800);
		CHECK(properties->FindProperty(Pooled("Width"), position) == nullptr);
		CHECK(position == properties->End());
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original PPropertyParser.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

struct RawValue
{
	const void* type;
	unsigned int value;
};

} // namespace

TEST_SUITE("PPropertyValue differential" * doctest::skip(PPropertyParserMissing()))
{
	TEST_CASE("numeric conversions match the original")
	{
		const auto constructInt = OriginalPP<Method<void, int>>("??0PPropertyValue@@QAE@H@Z");
		const auto constructUInt = OriginalPP<Method<void, unsigned int>>("??0PPropertyValue@@QAE@I@Z");
		const auto constructFloat = OriginalPP<Method<void, float>>("??0PPropertyValue@@QAE@M@Z");
		const auto constructBool = OriginalPP<Method<void, bool>>("??0PPropertyValue@@QAE@_N@Z");
		const auto getInteger = OriginalPP<Method<int>>("?GetInteger@PPropertyValue@@QBEHXZ");
		const auto getUInt = OriginalPP<Method<unsigned int>>("?GetUINT32@PPropertyValue@@QBEIXZ");
		const auto getFloat = OriginalPP<Method<float>>("?GetFloat@PPropertyValue@@QBEMXZ");
		const auto getBoolean = OriginalPP<Method<bool>>("?GetBoolean@PPropertyValue@@QBE_NXZ");
		const auto canBe = OriginalPP<Method<bool, const void*>>("?CanBeType@PPropertyValue@@QBE_NPBVTClass@Toshi@@@Z");
		const auto isEmpty = OriginalPP<Method<bool>>("?IsEmpty@PPropertyValue@@QBE_NXZ");
		const auto toString =
		    OriginalPP<const char*(TSTDCALL*)(const void*)>("?ToString@PPropertyValue@@SGPBDPBVTClass@Toshi@@@Z");

		const char* const typeNames[] = {"INT", "UINT32", "FLOAT", "BOOL", "TPCSTRING"};
		const void* originalTypes[5];
		const Toshi::TClass* ourTypes[] = {PPropertyValue::TYPE_INT, PPropertyValue::TYPE_UINT32,
		                                   PPropertyValue::TYPE_FLOAT, PPropertyValue::TYPE_BOOL,
		                                   PPropertyValue::TYPE_TPCSTRING};
		for (int i = 0; i < 5; ++i)
		{
			const std::string name = std::string("?TYPE_") + typeNames[i] + "@PPropertyValue@@2PBVTClass@Toshi@@B";
			originalTypes[i] = *static_cast<const void* const*>(OriginalPP<void*>(name.c_str()));
			CHECK(std::string(toString(originalTypes[i])) == PPropertyValue::ToString(ourTypes[i]));
		}

		std::vector<float> floats = {0.0f,
		                             -0.0f,
		                             0.5f,
		                             -0.5f,
		                             1.0f,
		                             -1.0f,
		                             2.5f,
		                             3.5f,
		                             -2.5f,
		                             1e8f,
		                             99999992.0f,
		                             2147483648.0f,
		                             -2147483648.0f,
		                             4294967296.0f,
		                             3e9f,
		                             -3e9f,
		                             1e20f,
		                             -1e20f,
		                             std::numeric_limits<float>::infinity(),
		                             std::numeric_limits<float>::quiet_NaN(),
		                             1.17549435e-38f};
		std::vector<int> ints = {0, 1, -1, 99999999, 100000000, 100000001, -100000000, 2147483647, -2147483647 - 1};
		InputGenerator gen;
		for (int i = 0; i < 2000; ++i)
		{
			floats.push_back(gen.NextFloat(-1e10f, 1e10f));
			floats.push_back(static_cast<float>(gen.NextInt(-1000, 1000)) * 0.5f);
			ints.push_back(static_cast<int>(gen.NextU32()));
		}

		const auto compare = [&](const RawValue& original, const PPropertyValue& ours, const char* what, double input)
		{
			INFO(what, " ", input);
			CHECK(getInteger(&original) == ours.GetInteger());
			CHECK(getUInt(&original) == ours.GetUINT32());
			CHECK(Same(getFloat(&original), ours.GetFloat()));
			CHECK(getBoolean(&original) == ours.GetBoolean());
			CHECK(isEmpty(&original) == ours.IsEmpty());
			for (int t = 0; t < 5; ++t)
				CHECK(canBe(&original, originalTypes[t]) == ours.CanBeType(ourTypes[t]));
		};
		for (const float f : floats)
		{
			RawValue original;
			constructFloat(&original, f);
			compare(original, PPropertyValue(f), "float", f);
		}
		for (const int n : ints)
		{
			RawValue original;
			constructInt(&original, n);
			compare(original, PPropertyValue(n), "int", n);
			constructUInt(&original, static_cast<unsigned int>(n));
			compare(original, PPropertyValue(static_cast<unsigned int>(n)), "uint", static_cast<unsigned int>(n));
		}
		for (const bool b : {false, true})
		{
			RawValue original;
			constructBool(&original, b);
			compare(original, PPropertyValue(b), "bool", b);
		}
	}
}

TEST_SUITE("PProperties differential" * doctest::skip(PPropertyParserMissing()))
{
	TEST_CASE("puts, lookups and sorting match the original")
	{
		using P = void*;
		const auto pool = Original<P(TSTDCALL*)()>("?GetCStringPool@TSystem@Toshi@@SGPAVTCStringPool@2@XZ");
		const auto get = Original<Method<P*, P*, const char*>>("?Get@TCStringPool@Toshi@@QAE?AVTPCString@2@PBD@Z");
		const auto construct = OriginalPP<Method<void>>("??0PProperties@@QAE@XZ");
		const auto destruct = OriginalPP<Method<void>>("??1PProperties@@UAE@XZ");
		const auto constructInt = OriginalPP<Method<void, int>>("??0PPropertyValue@@QAE@H@Z");
		const auto destructValue = OriginalPP<Method<void>>("??1PPropertyValue@@QAE@XZ");
		const auto put = OriginalPP<Method<void, P*, const RawValue*>>(
		    "?PutProperty@PProperties@@QAEXABVTPCString@Toshi@@ABVPPropertyValue@@@Z");
		const auto putUnique = OriginalPP<Method<void, P*, const RawValue*>>(
		    "?PutPropertyUnique@PProperties@@QAEXABVTPCString@Toshi@@ABVPPropertyValue@@@Z");
		const auto sort = OriginalPP<Method<void>>("?Sort@PProperties@@QAEXXZ");
		const auto getProperty = OriginalPP<Method<const RawValue*, P*>>(
		    "?GetProperty@PProperties@@QBEPBVPPropertyValue@@ABVTPCString@Toshi@@@Z");

		const char* const names[] = {"a", "b", "a[x]", "a[y]", "b[x]", "c", "a[x]", "B", "a[", "c[z]"};
		const auto text = [](const void* handle) -> std::string
		{
			if (handle == nullptr)
				return "";
			const char* buffer = *reinterpret_cast<const char* const*>(static_cast<const char*>(handle) + 4);
			return buffer ? buffer : "";
		};
		// The list from raw memory (both layouts): node {next, prev, name, sub, value type, value}.
		const auto describeOriginal = [&](const unsigned char* properties)
		{
			std::string out;
			const auto* root = properties + 0xC;
			for (auto* node = *reinterpret_cast<unsigned char* const*>(root); node != root;
			     node = *reinterpret_cast<unsigned char* const*>(node))
				out += text(*reinterpret_cast<void* const*>(node + 8)) + "[" +
				       text(*reinterpret_cast<void* const*>(node + 0xC)) +
				       "]=" + std::to_string(*reinterpret_cast<const int*>(node + 0x14)) + " ";
			return out + "#" + std::to_string(*reinterpret_cast<const int*>(properties + 0x14));
		};
		const auto describeOurs = [](const PProperties& properties)
		{
			std::string out;
			for (auto it = properties.Begin(); it != properties.End(); ++it)
				out += std::string(it->GetName().GetName().GetCString().GetString(0)) + "[" +
				       it->GetName().GetSubName().GetCString().GetString(0) +
				       "]=" + std::to_string(it->GetValue()->GetInteger()) + " ";
			return out + "#" + std::to_string(properties.GetPropertyCount());
		};

		InputGenerator gen;
		for (int round = 0; round < 100; ++round)
		{
			alignas(void*) unsigned char original[0x18];
			construct(original);
			PProperties ours;
			for (int step = 0; step < 40; ++step)
			{
				INFO("round ", round, " step ", step);
				const int op = static_cast<int>(gen.NextU32() % 6);
				const char* name = names[gen.NextU32() % std::size(names)];
				const int number = step;
				P handle = nullptr;
				get(pool(), &handle, name);
				if (op == 5)
				{
					sort(original);
					ours.Sort();
				}
				else
				{
					RawValue value;
					constructInt(&value, number);
					if (op == 4)
						putUnique(original, &handle, &value);
					else
						put(original, &handle, &value);
					destructValue(&value);
					if (op == 4)
						ours.PutPropertyUnique(Pooled(name), PPropertyValue(number));
					else
						ours.PutProperty(Pooled(name), PPropertyValue(number));
				}
				REQUIRE(describeOriginal(original) == describeOurs(ours));
				const RawValue* found = getProperty(original, &handle);
				const PPropertyValue* ourFound = ours.GetProperty(Pooled(name));
				REQUIRE((found == nullptr) == (ourFound == nullptr));
				if (found != nullptr)
					CHECK(static_cast<int>(found->value) == ourFound->GetInteger());
			}
			destruct(original);
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
