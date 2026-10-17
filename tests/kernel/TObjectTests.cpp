#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TObject.h"

#include <array>
#include <cstring>
#include <doctest/doctest.h>
#include <new>
#include <string>
#include <vector>

using Toshi::TClass;
using Toshi::TObject;
using namespace jpog::test;

namespace
{

// A small class derived from TObject for the unit tests.
class TTestObject : public TObject
{
public:
	TClass& GetClass() const override { return s_class; }
	static TObject* TSTDCALL Create() { return new TTestObject(); }
	static TClass s_class;
};

TClass TTestObject::s_class("TTestObject", &TObject::m_sClass, &TTestObject::Create, nullptr, nullptr, nullptr,
                            0x20003);

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TObject")
{
	TEST_CASE("class registration and queries")
	{
		CHECK(TTestObject::s_class.IsAttached());
		CHECK(TTestObject::s_class.IsA(TObject::m_sClass));
		CHECK_FALSE(TObject::m_sClass.IsA(TTestObject::s_class));
		CHECK(TClass::Find("ttestobject", nullptr) == &TTestObject::s_class); // case-insensitive
		CHECK(TClass::FindCommonBaseClass(TTestObject::s_class, TObject::m_sClass) == &TObject::m_sClass);
		CHECK(TTestObject::s_class.GetVersionMajor() == 2);
		CHECK(TTestObject::s_class.GetVersionMinor() == 3);

		TObject* object = TTestObject::s_class.CreateObject();
		REQUIRE(object != nullptr);
		CHECK(object->IsExactly(TTestObject::s_class));
		CHECK(object->IsA(TObject::m_sClass));
		object->Delete();
		TClass::DumpObjectClassTree();
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

// Records RecurseTree callbacks as (kind, class index).
struct Recorder
{
	std::vector<const void*> classes; // index -> class
	std::vector<std::pair<char, int>> events;
	int IndexOf(const void* cls) const
	{
		for (std::size_t i = 0; i < classes.size(); ++i)
			if (classes[i] == cls)
				return static_cast<int>(i);
		return -1;
	}
};

bool __stdcall RecordEnter(TClass* cls, void* data)
{
	auto* recorder = static_cast<Recorder*>(data);
	recorder->events.push_back({'>', recorder->IndexOf(cls)});
	return true;
}

bool __stdcall RecordLeave(TClass* cls, void* data)
{
	auto* recorder = static_cast<Recorder*>(data);
	recorder->events.push_back({'<', recorder->IndexOf(cls)});
	return true;
}

bool __stdcall RecordEach(TClass* cls, void* data)
{
	auto* recorder = static_cast<Recorder*>(data);
	recorder->events.push_back({'.', recorder->IndexOf(cls)});
	return true;
}

} // namespace

TEST_SUITE("TObject differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("TObject class descriptor")
	{
		const auto* original =
		    static_cast<const unsigned char*>(OriginalModule::Kernel().Find("?m_sClass@TObject@Toshi@@2VTClass@2@A"));
		REQUIRE(original != nullptr);
		const char* originalName = *reinterpret_cast<const char* const*>(original);
		CHECK(std::strcmp(originalName, TObject::m_sClass.GetName()) == 0);
		CHECK(*reinterpret_cast<const unsigned int*>(original + 0x20) == TObject::m_sClass.GetVersion());
		CHECK(*reinterpret_cast<const void* const*>(original + 0x14) == nullptr); // no parent
	}

	TEST_CASE("class trees behave like the original")
	{
		using Construct = Method<void, const char*, void*, void*, void*, void*, void*, unsigned int>;
		const auto construct =
		    Original<Construct>("??0TClass@Toshi@@QAE@PBDPAV01@P6GPAVTObject@1@XZP6GPAV21@PAX@ZP6GXXZ5I@Z");
		const auto destruct = Original<Method<void>>("??1TClass@Toshi@@QAE@XZ");
		const auto isA = Original<Method<bool, const void*>>("?IsA@TClass@Toshi@@QBE_NABV12@@Z");
		const auto isAttached = Original<Method<bool>>("?IsAttached@TClass@Toshi@@QBE_NXZ");
		const auto minor = Original<Method<unsigned short>>("?GetVersionMinor@TClass@Toshi@@QBEGXZ");
		const auto major = Original<Method<unsigned short>>("?GetVersionMajor@TClass@Toshi@@QBEGXZ");
		using CommonFn = const void*(__stdcall*)(const void*, const void*);
		const auto common = Original<CommonFn>("?FindCommonBaseClass@TClass@Toshi@@SGPBV12@ABV12@0@Z");
		using FindFn = const void*(__stdcall*)(const char*, const void*);
		const auto find = Original<FindFn>("?Find@TClass@Toshi@@SGPBV12@PBDPBV12@@Z");
		using Callback = bool(__stdcall*)(TClass*, void*);
		const auto recurse = Original<Method<void, Callback, Callback, Callback, void*>>(
		    "?RecurseTree@TClass@Toshi@@QAEXP6G_NPAV12@PAX@Z221@Z");

		static const char* const names[] = {"Root",  "Animal", "Dino", "Rex",  "Raptor",
		                                    "Plant", "Fern",   "Tree", "rex2", "Gate"};
		InputGenerator gen;
		for (int round = 0; round < 40; ++round)
		{
			const int count = gen.NextInt(1, 10);
			std::vector<int> parents(count, -1);
			for (int i = 1; i < count; ++i)
				parents[i] = gen.NextInt(0, i - 1);

			// Zeroed storage, like static TClass objects.
			std::vector<std::array<unsigned char, sizeof(TClass)>> originalStorage(count);
			std::vector<std::array<unsigned char, sizeof(TClass)>> ourStorage(count);
			for (int i = 0; i < count; ++i)
			{
				originalStorage[i].fill(0);
				ourStorage[i].fill(0);
			}
			std::vector<TClass*> ours(count);
			Recorder originalRecorder, ourRecorder;
			for (int i = 0; i < count; ++i)
			{
				const unsigned int version = gen.NextU32();
				construct(originalStorage[i].data(), names[i],
				          parents[i] < 0 ? nullptr : originalStorage[parents[i]].data(), nullptr, nullptr, nullptr,
				          nullptr, version);
				ours[i] = new (ourStorage[i].data()) TClass(names[i], parents[i] < 0 ? nullptr : ours[parents[i]],
				                                            nullptr, nullptr, nullptr, nullptr, version);
				originalRecorder.classes.push_back(originalStorage[i].data());
				ourRecorder.classes.push_back(ours[i]);
			}
			auto originalIndex = [&](const void* cls) { return originalRecorder.IndexOf(cls); };
			auto ourIndex = [&](const void* cls) { return ourRecorder.IndexOf(cls); };

			for (int i = 0; i < count; ++i)
			{
				void* o = originalStorage[i].data();
				INFO("class ", i);
				CHECK(isAttached(o) == ours[i]->IsAttached());
				CHECK(minor(o) == ours[i]->GetVersionMinor());
				CHECK(major(o) == ours[i]->GetVersionMajor());
				CHECK(originalIndex(find(names[i], originalStorage[0].data())) ==
				      ourIndex(TClass::Find(names[i], ours[0])));
				for (int j = 0; j < count; ++j)
				{
					CHECK(isA(o, originalStorage[j].data()) == ours[i]->IsA(*ours[j]));
					CHECK(originalIndex(common(o, originalStorage[j].data())) ==
					      ourIndex(TClass::FindCommonBaseClass(*ours[i], *ours[j])));
				}
			}
			CHECK(originalIndex(find("REX", originalStorage[0].data())) == ourIndex(TClass::Find("REX", ours[0])));
			recurse(originalStorage[0].data(), &RecordEnter, &RecordLeave, &RecordEach, &originalRecorder);
			ours[0]->RecurseTree(&RecordEnter, &RecordLeave, &RecordEach, &ourRecorder);
			CHECK(originalRecorder.events == ourRecorder.events);

			for (int i = count - 1; i >= 0; --i)
			{
				destruct(originalStorage[i].data());
				ours[i]->~TClass();
			}
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
