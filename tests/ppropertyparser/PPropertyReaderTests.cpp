#include "PropertyFiles.h"
#include "ppropertyparser/PPropertyReader.h"

#include <doctest/doctest.h>

using namespace jpog::test;

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("PPropertyReader")
{
	TEST_CASE("property sets, values and comments")
	{
		auto* file = NewStringFile("// first\n"
		                           "a = 12; b[2] = \"s\"; c.d = 2.5; e;\n"
		                           "list = { 1, 4u, { true }, }; ref = n[x];\n"
		                           "set { f = false; g = ident; }\n");
		PPropertyReader reader;
		reader.SetLoadComments(true);
		REQUIRE(reader.Open(TCString("test.ini"), file));
		EventLog log(reader.GetParseErrorEmitter());
		PProperties properties;
		REQUIRE(reader.LoadPropertyBlock(properties));
		CHECK(log.events.empty());
		// After "c.d = ..." the next property goes to the top level again.
		CHECK(Describe(properties) == "a[]# first#=i12; b[2]='s'; c[]={d[]=f40200000; }; e[]=undef; "
		                              "list[]=(i1,u4,(true,),); ref[]=@n[x]; set[]={f[]=false; g[]='ident'; }; ");
	}

	TEST_CASE("errors and blocks")
	{
		PPropertyReader reader;
		REQUIRE(reader.Open(TCString("test.ini"), NewStringFile("x = 1\ny = 2;")));
		EventLog log(reader.GetParseErrorEmitter());
		PProperties properties;
		CHECK_FALSE(reader.LoadPropertyBlock(properties));
		REQUIRE(log.events.size() == 1);
		CHECK(log.events[0] ==
		      Event{TFileLexer::TOKEN_IDENT, 2, "y", "Expected semicolon at end of property assignment"});

		// The original crashes here.
		PPropertyReader scalar;
		REQUIRE(scalar.Open(TCString("test.ini"), NewStringFile("a = 1; a.b = 2;")));
		EventLog scalarLog(scalar.GetParseErrorEmitter());
		PProperties scalarProperties;
		CHECK_FALSE(scalar.LoadPropertyBlock(scalarProperties));
		REQUIRE(scalarLog.events.size() == 1);
		CHECK(scalarLog.events[0].message == "Property 'a' is not a property set");

		std::vector<std::string> loaded;
		Toshi::TManagedPtr<PPropertyBlock> block(new TestBlock("A",
		                                                       [&](PPropertyReader* r)
		                                                       {
			                                                       PProperties set;
			                                                       r->LoadPropertyBlock(set);
			                                                       loaded.push_back(Describe(set));
		                                                       }));
		PPropertyReader blocks;
		blocks.AddDefinitionBlock(block);
		REQUIRE(blocks.Open(TCString("test.ini"), NewStringFile("[A] a = 1; [C] b { c = 2; } d; [A] e = 3;")));
		EventLog blockLog(blocks.GetParseErrorEmitter());
		CHECK(blocks.Load());
		CHECK(loaded == std::vector<std::string>{"a[]=i1; ", "e[]=i3; "});
		REQUIRE(blockLog.events.size() == 1);
		CHECK(blockLog.events[0].message == "Unknown block labelled 'C' found -- skipping");
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests (x86, original DLLs)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

TEST_SUITE("PPropertyReader differential" * doctest::skip(PPropertyParserMissing()))
{
	TEST_CASE("property sets match the original")
	{
		const OriginalReader original;
		InputGenerator gen;
		int failures = 0;
		for (int round = 0; round < 1500; ++round)
		{
			const std::string text = PropertySource(gen).Statements(0, false);
			INFO("round ", round, "\n", text);
			const bool comments = round % 2 == 0;

			alignas(void*) unsigned char reader[0x58];
			original.Open(reader, NewStringFile(text), comments);
			EventLog originalLog(reader + 0x38);
			PPropertyReader ours;
			ours.SetLoadComments(comments);
			REQUIRE(ours.Open(TCString("test.ini"), NewStringFile(text)));
			EventLog ourLog(ours.GetParseErrorEmitter());

			// Repeated loads also cover reading on after a set ends at an unexpected token.
			for (int set = 0; set < 3; ++set)
			{
				bool originalResult = false;
				const std::string originalSet = original.LoadSet(reader, originalResult);
				PProperties properties;
				const bool ourResult = ours.LoadPropertyBlock(properties);
				CHECK(originalResult == ourResult);
				REQUIRE(originalSet == Describe(properties));
				failures += ourResult ? 0 : 1;
			}
			CHECK(originalLog.events == ourLog.events);
			originalLog.Disconnect();
			original.destruct(reader);
		}
		MESSAGE(failures, " failed loads compared");
	}

	TEST_CASE("Open overloads are in the original vtable slots")
	{
		// Slot 3 is Open(name, file), slot 4 Open(name), as in ??_7PPropertyReader@@6B@.
		PPropertyReader ours;
		void* const* vtable = *reinterpret_cast<void* const* const*>(&ours);
		const TCString name("test.ini");
		CHECK((Method<bool, const TCString*, TFile*>{vtable[3]}(&ours, &name, NewStringFile("a = 1;"))));
		CHECK(ours.GetLexer() != nullptr);
	}

	TEST_CASE("block dispatch matches the original")
	{
		const OriginalReader original;
		InputGenerator gen;
		for (int round = 0; round < 500; ++round)
		{
			const std::string text = PropertySource(gen).Statements(0, true);
			INFO("round ", round, "\n", text);

			std::vector<std::string> originalSets, ourSets;
			std::vector<Toshi::TManagedPtr<PPropertyBlock>> originalBlocks, ourBlocks;
			for (const char* name : {"A", "B"})
			{
				originalBlocks.emplace_back(new TestBlock(name,
				                                          [&](PPropertyReader* reader)
				                                          {
					                                          bool result = false;
					                                          const std::string set = original.LoadSet(reader, result);
					                                          originalSets.push_back(set + (result ? "" : "!"));
				                                          }));
				ourBlocks.emplace_back(new TestBlock(name,
				                                     [&](PPropertyReader* reader)
				                                     {
					                                     PProperties properties;
					                                     const bool result = reader->LoadPropertyBlock(properties);
					                                     ourSets.push_back(Describe(properties) + (result ? "" : "!"));
				                                     }));
			}

			alignas(void*) unsigned char reader[0x58];
			original.Open(reader, NewStringFile(text), false);
			for (auto& block : originalBlocks)
			{
				// The callee releases its by-value TManagedPtr.
				block->IncRefCount(nullptr);
				original.addBlock(reader, block.Get());
			}
			EventLog originalLog(reader + 0x38);
			PPropertyReader ours;
			for (auto& block : ourBlocks)
				ours.AddDefinitionBlock(block);
			REQUIRE(ours.Open(TCString("test.ini"), NewStringFile(text)));
			EventLog ourLog(ours.GetParseErrorEmitter());

			CHECK(original.load(reader) == ours.Load());
			const auto join = [](const std::vector<std::string>& sets)
			{
				std::string out;
				for (const auto& set : sets)
					out += set + "\n";
				return out;
			};
			REQUIRE(join(originalSets) == join(ourSets));
			CHECK(originalLog.events == ourLog.events);
			originalLog.Disconnect();
			original.destruct(reader);
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
