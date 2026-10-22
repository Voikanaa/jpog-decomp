#include "PropertyFiles.h"
#include "ppropertyparser/PPropertyWriter.h"

#include <doctest/doctest.h>

using namespace jpog::test;

namespace
{

// Reads one set from `text` with our reader.
void Read(const std::string& text, PProperties& properties)
{
	PPropertyReader reader;
	REQUIRE(reader.Open(TCString("test.ini"), NewStringFile(text)));
	reader.LoadPropertyBlock(properties);
}

const unsigned short kFileName[] = {'o', 'u', 't', '.', 'i', 'n', 'i', 0};

// A block that writes its header and one property set.
class SavingBlock : public PPropertyBlock
{
public:
	SavingBlock(const char* name, int priority, std::function<bool(PPropertyWriter*, const TPCString&)> save)
	    : PPropertyBlock(Toshi::TSystem::GetCStringPool()->Get(name), static_cast<Priority>(priority)),
	      m_save(std::move(save))
	{
	}
	bool Load(PPropertyReader*) override { return true; }
	bool Save(PPropertyWriter* writer) override { return m_save(writer, GetName()); }

private:
	std::function<bool(PPropertyWriter*, const TPCString&)> m_save;
};

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("PPropertyWriter")
{
	TEST_CASE("property sets")
	{
		const std::string text = "a = 12; b[2] = \"s t\"; c { d = 2.5; e; f = { 1, 4u, \"q\\\"\" }; } g = n[x];"
		                         "h = true; i = \"true\";";
		PProperties properties;
		Read(text, properties);

		TStringFile file;
		PPropertyWriter writer;
		REQUIRE(writer.Open(Toshi::TWString(kFileName), &file));
		CHECK(writer.PutPropertyBlock(&properties));
		writer.Close();
		// The integer sub-name is a string that needs quotes.
		CHECK(file.Written() ==
		      "a = 12;\nb[\"2\"] = \"s t\";\nc {\n\td = 2.5f;\n\te;\n\tf = {1,0x00000004u,\"q\\\"\"};\n}\n"
		      "g = n[x];\nh = true;\ni = \"true\";\n");

		// The output reads back the same.
		PProperties again;
		Read(file.Written(), again);
		CHECK(Describe(again) == Describe(properties));
	}

	TEST_CASE("scalars and quoting")
	{
		TStringFile file;
		PPropertyWriter writer;
		REQUIRE(writer.Open(Toshi::TWString(kFileName), &file));
		CHECK(writer.Put(4000000000u));
		writer.PrintUnsignedAsHex(false);
		CHECK(writer.Put(4000000000u));
		CHECK(writer.Put(-7));
		CHECK(writer.Put(0.1f));
		CHECK(writer.Put("plain_1"));
		CHECK(writer.Put("1st"));
		CHECK(writer.Put("false"));
		CHECK(writer.Put(""));
		CHECK(writer.Put('-', 3));
		CHECK_FALSE(writer.Put('\xe9', 1));
		CHECK(writer.PutBlockHeader(Toshi::TSystem::GetCStringPool()->Get("Block")));
		CHECK_FALSE(writer.PutToken(TFileLexer::TOKEN_IDENT));
		writer.Close();
		CHECK(file.Written() == "0xEE6B2800u-294967296u-70.1fplain_1\"1st\"\"false\"\"\"---\xe9\n[Block]\n");
	}

	TEST_CASE("definition blocks are saved by priority")
	{
		TStringFile file;
		PPropertyWriter writer;
		const auto save = [](PPropertyWriter* w, const TPCString& name) { return w->PutBlockHeader(name); };
		for (const auto& [name, priority] : {std::pair{"C", 3}, {"A", 1}, {"B", 3}, {"D", 2}})
			writer.AddDefinitionBlock(Toshi::TManagedPtr<PPropertyBlock>(new SavingBlock(name, priority, save)));
		REQUIRE(writer.Open(Toshi::TWString(kFileName), &file));
		CHECK(writer.Save());
		CHECK(file.Written() == "\n[A]\n\n[D]\n\n[C]\n\n[B]\n");
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests (x86, original DLLs)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

struct OriginalWriter
{
	const Method<void, const unsigned short*> constructWString =
	    Original<Method<void, const unsigned short*>>("??0TWString@Toshi@@QAE@PBG@Z");
	const Method<void> destructWString = Original<Method<void>>("??1TWString@Toshi@@QAE@XZ");
	const Method<void> construct = OriginalPP<Method<void>>("??0PPropertyWriter@@QAE@XZ");
	const Method<void> destruct = OriginalPP<Method<void>>("??1PPropertyWriter@@UAE@XZ");
	const Method<bool, const void*, TFile*> open =
	    OriginalPP<Method<bool, const void*, TFile*>>("?Open@PPropertyWriter@@QAE_NABVTWString@Toshi@@PAVTFile@3@@Z");
	const Method<void> close = OriginalPP<Method<void>>("?Close@PPropertyWriter@@QAEXXZ");
	const Method<void, bool> hex = OriginalPP<Method<void, bool>>("?PrintUnsignedAsHex@PPropertyWriter@@QAEX_N@Z");
	const Method<bool, int, const void*> putBlock =
	    OriginalPP<Method<bool, int, const void*>>("?PutPropertyBlock@PPropertyWriter@@QAE_NHPBVPProperties@@@Z");
	const Method<bool, const char*> putString =
	    OriginalPP<Method<bool, const char*>>("?Put@PPropertyWriter@@QAE_NPBD@Z");
	const Method<bool, char, int> putChars = OriginalPP<Method<bool, char, int>>("?Put@PPropertyWriter@@QAE_NDH@Z");
	const Method<bool, unsigned int> putUInt = OriginalPP<Method<bool, unsigned int>>("?Put@PPropertyWriter@@QAE_NI@Z");
	const Method<bool, float> putFloat = OriginalPP<Method<bool, float>>("?Put@PPropertyWriter@@QAE_NM@Z");
	const Method<bool, const TPCString*> putHeader =
	    OriginalPP<Method<bool, const TPCString*>>("?PutBlockHeader@PPropertyWriter@@QAE_NABVTPCString@Toshi@@@Z");
	const Method<void, void*> addBlock = OriginalPP<Method<void, void*>>(
	    "?AddDefinitionBlock@PPropertyWriter@@QAEXV?$TManagedPtr@VPPropertyBlock@@@Toshi@@@Z");
	const Method<bool> save = OriginalPP<Method<bool>>("?Save@PPropertyWriter@@QAE_NXZ");

	void Open(unsigned char* writer, TFile* file) const
	{
		construct(writer);
		alignas(void*) unsigned char name[8];
		constructWString(name, kFileName);
		REQUIRE(open(writer, name, file));
		destructWString(name);
	}
};

} // namespace

TEST_SUITE("PPropertyWriter differential" * doctest::skip(PPropertyParserMissing()))
{
	TEST_CASE("written property sets match the original")
	{
		const OriginalReader reader;
		const OriginalWriter writer;
		InputGenerator gen;
		for (int round = 0; round < 1000; ++round)
		{
			const std::string text = PropertySource(gen).Statements(0, false);
			INFO("round ", round, "\n", text);
			const bool hex = round % 3 != 0;

			alignas(void*) unsigned char originalReader[0x58];
			reader.Open(originalReader, NewStringFile(text), false);
			alignas(void*) unsigned char originalSet[0x18];
			reader.constructProperties(originalSet);
			reader.loadBlock(originalReader, originalSet);
			reader.destruct(originalReader);
			PProperties ourSet;
			Read(text, ourSet);
			REQUIRE(reader.DescribeProperties(originalSet) == Describe(ourSet));

			TStringFile originalFile, ourFile;
			alignas(void*) unsigned char originalWriter[0x24];
			writer.Open(originalWriter, &originalFile);
			writer.hex(originalWriter, hex);
			const bool originalResult = writer.putBlock(originalWriter, 1, originalSet);
			writer.close(originalWriter);
			writer.destruct(originalWriter);
			reader.destructProperties(originalSet);

			PPropertyWriter ours;
			REQUIRE(ours.Open(Toshi::TWString(kFileName), &ourFile));
			ours.PrintUnsignedAsHex(hex);
			CHECK(originalResult == ours.PutPropertyBlock(1, &ourSet));
			ours.Close();
			REQUIRE(originalFile.Written() == ourFile.Written());
		}
	}

	TEST_CASE("scalars match the original")
	{
		const OriginalWriter writer;
		InputGenerator gen;
		TStringFile originalFile, ourFile;
		alignas(void*) unsigned char original[0x24];
		writer.Open(original, &originalFile);
		PPropertyWriter ours;
		REQUIRE(ours.Open(Toshi::TWString(kFileName), &ourFile));
		const char alphabet[] = "aZ_09 \"\\\xe9\x7f-.";
		for (int step = 0; step < 20000; ++step)
		{
			INFO("step ", step);
			switch (gen.NextU32() % 5)
			{
			case 0:
			{
				std::string text;
				const int length = gen.NextInt(0, 6);
				for (int i = 0; i < length; ++i)
					text += alphabet[gen.NextU32() % (sizeof(alphabet) - 1)];
				if (gen.NextU32() % 4 == 0)
					text = gen.NextU32() % 2 ? "true" : "false";
				CHECK(writer.putString(original, text.c_str()) == ours.Put(text.c_str()));
				break;
			}
			case 1:
			{
				const char c = alphabet[gen.NextU32() % (sizeof(alphabet) - 1)];
				const int count = gen.NextInt(-1, 3);
				CHECK(writer.putChars(original, c, count) == ours.Put(c, count));
				break;
			}
			case 2:
			{
				const unsigned int value = gen.NextU32();
				const bool hex = gen.NextU32() % 2 == 0;
				writer.hex(original, hex);
				ours.PrintUnsignedAsHex(hex);
				CHECK(writer.putUInt(original, value) == ours.Put(value));
				break;
			}
			case 3:
			{
				const float value = gen.NextFloat(-1e6f, 1e6f) * (gen.NextU32() % 2 ? 1e-6f : 1.0f);
				CHECK(writer.putFloat(original, value) == ours.Put(value));
				break;
			}
			default:
			{
				const TPCString name = Toshi::TSystem::GetCStringPool()->Get(gen.NextU32() % 2 ? "Block" : "");
				CHECK(writer.putHeader(original, &name) == ours.PutBlockHeader(name));
				break;
			}
			}
		}
		writer.close(original);
		writer.destruct(original);
		ours.Close();
		CHECK(originalFile.Written().size() > 100000);
		REQUIRE(originalFile.Written() == ourFile.Written());
	}

	TEST_CASE("definition block order matches the original")
	{
		const OriginalWriter writer;
		InputGenerator gen;
		for (int round = 0; round < 200; ++round)
		{
			INFO("round ", round);
			TStringFile originalFile, ourFile;
			alignas(void*) unsigned char original[0x24];
			writer.Open(original, &originalFile);
			PPropertyWriter ours;
			REQUIRE(ours.Open(Toshi::TWString(kFileName), &ourFile));
			std::vector<Toshi::TManagedPtr<PPropertyBlock>> blocks;
			const int count = gen.NextInt(0, 12);
			for (int i = 0; i < count; ++i)
			{
				const std::string name = "B" + std::to_string(i);
				const int priority = gen.NextInt(0, 3);
				// One block in ten fails, which stops the save.
				const bool fails = gen.NextU32() % 10 == 0;
				blocks.emplace_back(new SavingBlock(name.c_str(), priority,
				                                    [&writer, &ours, fails](PPropertyWriter* w, const TPCString& n)
				                                    {
					                                    if (w == &ours)
						                                    ours.PutBlockHeader(n);
					                                    else
						                                    writer.putHeader(w, &n);
					                                    return !fails;
				                                    }));
				// The callee releases its by-value TManagedPtr.
				blocks.back()->IncRefCount(nullptr);
				writer.addBlock(original, blocks.back().Get());
				ours.AddDefinitionBlock(blocks.back());
			}
			CHECK(writer.save(original) == ours.Save());
			writer.close(original);
			writer.destruct(original);
			ours.Close();
			REQUIRE(originalFile.Written() == ourFile.Written());
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
