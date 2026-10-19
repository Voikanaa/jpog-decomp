#pragma once

// Shared helpers for the PPropertyParser reader and writer tests.

#include "ppropertyparser/PProperties.h"
#include "ppropertyparser/PPropertyReader.h"
#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TFile.h"
#include "toshi/kernel/TSystem.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <doctest/doctest.h>
#include <functional>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace jpog::test
{

using Toshi::TCString;
using Toshi::TFile;
using Toshi::TFileLexer;
using Toshi::TFileLexerUTF8;
using Toshi::TPCString;

// Owns nothing: readers destroy their files through the file's file system.
class TNullFileSystem : public Toshi::TFileSystem
{
public:
	TNullFileSystem() : TFileSystem("null") {}
	TFile* CreateFile(const TCString&, unsigned int) override { return nullptr; }
	void DestroyFile(TFile*) override {}
	TCString MakeInternalPath(const TCString& path) const override { return path; }
};

inline TNullFileSystem g_oFileSystem;

// Reads from a string; writes are collected in Written().
class TStringFile : public TFile
{
public:
	explicit TStringFile(std::string text = {}) : TFile(&g_oFileSystem), m_text(std::move(text)) {}

	const std::string& Written() const { return m_written; }

	int Read(void*, int) override { return 0; }
	int Write(const void* buffer, int size) override
	{
		m_written.append(static_cast<const char*>(buffer), static_cast<std::size_t>(size));
		return size;
	}
	bool Seek(int, TSEEK) override { return false; }
	int Tell() override { return static_cast<int>(m_position); }
	int GetSize() override { return static_cast<int>(m_text.size()); }
	int GetCChar() override
	{
		return m_position < m_text.size() ? static_cast<unsigned char>(m_text[m_position++]) : -1;
	}
	int GetWChar() override { return GetCChar(); }
	int PutCChar(char) override { return -1; }
	int PutWChar(unsigned short) override { return -1; }
	int VCPrintf(const char* format, char* args) override
	{
		char text[1024];
		const int length = std::vsnprintf(text, sizeof(text), format, args);
		m_written += text;
		return length;
	}
	int VWPrintf(const unsigned short*, char*) override { return 0; }

private:
	std::string m_text;
	std::string m_written;
	std::size_t m_position = 0;
};

// The text of a pooled string handle (either kernel's layout: refcount, TCString buffer).
inline std::string HandleText(const void* handle)
{
	if (handle == nullptr)
		return "";
	const char* buffer = *reinterpret_cast<const char* const*>(static_cast<const char*>(handle) + 4);
	return buffer ? buffer : "";
}

inline std::string FloatBits(float value)
{
	unsigned int bits;
	std::memcpy(&bits, &value, sizeof(bits));
	char text[16];
	std::snprintf(text, sizeof(text), "%08x", bits);
	return text;
}

inline std::string Describe(const PProperties& properties);

inline std::string Describe(const PPropertyValue& value)
{
	const Toshi::TClass* type = value.GetType();
	if (type == PPropertyValue::TYPE_UNDEF)
		return "undef";
	if (type == PPropertyValue::TYPE_INT)
		return "i" + std::to_string(value.GetInteger());
	if (type == PPropertyValue::TYPE_UINT32)
		return "u" + std::to_string(value.GetUINT32());
	if (type == PPropertyValue::TYPE_FLOAT)
		return "f" + FloatBits(value.GetFloat());
	if (type == PPropertyValue::TYPE_BOOL)
		return value.GetBoolean() ? "true" : "false";
	if (type == PPropertyValue::TYPE_TPCSTRING)
		return "'" + std::string(value.GetTPCString().GetCString().GetString(0)) + "'";
	if (type == PPropertyValue::TYPE_PROPNAME)
		return "@" + std::string(value.GetPropertyName().GetName().GetCString().GetString(0)) + "[" +
		       value.GetPropertyName().GetSubName().GetCString().GetString(0) + "]";
	if (type == PPropertyValue::TYPE_PROPS)
		return "{" + Describe(*value.GetProperties()) + "}";
	if (type == PPropertyValue::TYPE_ARRAY)
	{
		std::string out = "(";
		const auto& values = value.GetArray()->GetValues();
		for (int i = 0; i < values.GetNumElements(); ++i)
			out += Describe(values[i]) + ",";
		return out + ")";
	}
	return "?";
}

inline std::string Describe(const PProperties& properties)
{
	std::string out;
	for (auto it = properties.Begin(); it != properties.End(); ++it)
	{
		out += std::string(it->GetName().GetName().GetCString().GetString(0)) + "[" +
		       it->GetName().GetSubName().GetCString().GetString(0) + "]";
		const TPCString comment = it->GetComment();
		if (!comment.IsEmpty())
			out += "#" + std::string(comment.GetCString().GetString(0)) + "#";
		out += "=" + Describe(*it->GetValue()) + "; ";
	}
	return out;
}

// One reader event, from either reader's PPropertyReaderParseError.
struct Event
{
	int type;
	int line;
	std::string text;
	std::string message;
	bool operator==(const Event& other) const
	{
		return type == other.type && line == other.line && text == other.text && message == other.message;
	}
};

inline doctest::String toString(const Event& event)
{
	return (std::to_string(event.type) + "@" + std::to_string(event.line) + " '" + event.text + "' " + event.message)
	    .c_str();
}

struct EventLog : Toshi::TGenericListener
{
	using TGenericListener::Disconnect;
	explicit EventLog(void* emitter) { Connect(static_cast<Toshi::TGenericEmitter*>(emitter), this, Record, 0); }
	~EventLog() { Disconnect(); }
	static bool TSTDCALL Record(void* receiver, void*, void* event)
	{
		// The original's events have the same layout (x86).
		const auto& error = *static_cast<const PPropertyReaderParseError*>(event);
		Event recorded;
		recorded.type = error.m_oToken.GetType();
		recorded.line = error.m_oToken.GetLine();
		if (recorded.type == TFileLexer::TOKEN_IDENT || recorded.type == TFileLexer::TOKEN_STRING ||
		    recorded.type == TFileLexer::TOKEN_COMMENT)
			recorded.text = error.m_oToken.GetString().GetCString().GetString(0);
		recorded.message = error.m_szMessage;
		static_cast<EventLog*>(receiver)->events.push_back(recorded);
		return true;
	}
	std::vector<Event> events;
};

// A block that reads its body as one property set and hands it to a callback.
class TestBlock : public PPropertyBlock
{
public:
	TestBlock(const char* name, std::function<void(PPropertyReader*)> load)
	    : PPropertyBlock(Toshi::TSystem::GetCStringPool()->Get(name), static_cast<Priority>(1)), m_load(std::move(load))
	{
	}
	bool Load(PPropertyReader* reader) override
	{
		m_load(reader);
		return true;
	}
	bool Save(PPropertyWriter*) override { return true; }

private:
	std::function<void(PPropertyReader*)> m_load;
};

// Random property text: mostly valid statements with occasional mistakes.
class PropertySource
{
public:
	explicit PropertySource(InputGenerator& gen) : m_gen(gen) {}

	std::string Statements(int depth, bool blocks)
	{
		std::string out;
		const int count = Pick(8);
		for (int i = 0; i < count; ++i)
		{
			if (blocks && Pick(5) == 0)
				out += Pick(10) == 0 ? "[ 3 ]\n" : "[" + std::string(kBlocks[Pick(std::size(kBlocks))]) + "]\n";
			out += Statement(depth, blocks);
		}
		return out;
	}

private:
	static constexpr const char* kIdents[] = {"a", "b", "Name", "true", "false", "x1"};
	static constexpr const char* kBlocks[] = {"A", "B", "C", "A"};
	static constexpr const char* kScalars[] = {
	    "12",     "-3",       "0x1F",        "4u",        "2.5",        "1.5f",       "\"str\"",
	    "\"\"",   "true",     "false",       "ident",     "0",          "-0.0",       "a[b]",
	    "x[3]",   "\"a b\"",  "0xFFFFFFF0u", "1e9",       "\"q\\\"t\"", "\"b\\\\s\"", "\"caf\xe9\"",
	    "\"9x\"", "\"true\"", "\"_u\"",      "123456789", "0.1f",       "x[\"9\"]"};
	// No "$" before an identifier or integer: the tests have no locale to look strings up in.
	static constexpr const char* kJunk[] = {"=", "]", ";", "(", ",", "$ {", "$ 5.5", "$ ;", "[", "}", "{", "."};

	int Pick(std::size_t n) { return static_cast<int>(m_gen.NextU32() % n); }

	std::string Name()
	{
		std::string name = Pick(8) == 0 ? "\"quoted\"" : kIdents[Pick(std::size(kIdents))];
		switch (Pick(8))
		{
		case 0:
			return name + "[sub]";
		case 1:
			return name + "[" + std::to_string(Pick(4)) + "]";
		case 2:
			return name + "[\"s\"]";
		case 3:
			return Pick(4) == 0 ? name + "[2.5]" : name;
		default:
			return name;
		}
	}

	std::string SetName() { return Pick(2) == 0 ? "s" : "t[1]"; }

	std::string Value(int depth)
	{
		if (depth < 3 && Pick(6) == 0)
		{
			std::string out = "{";
			const int count = Pick(4);
			for (int i = 0; i < count; ++i)
				out += Value(depth + 1) + (i + 1 < count || Pick(3) == 0 ? ", " : "");
			if (Pick(12) == 0)
				out += " ; ";
			return out + "}";
		}
		if (Pick(30) == 0)
			return kJunk[Pick(std::size(kJunk))];
		return kScalars[Pick(std::size(kScalars))];
	}

	std::string Statement(int depth, bool blocks)
	{
		std::string out;
		if (Pick(4) == 0)
			out += "// note " + std::to_string(Pick(9)) + "\n";
		if (Pick(40) == 0)
			return out + kJunk[Pick(std::size(kJunk))] + "\n";
		// Only "s" and "t[1]" are dotted into, and they are always sets: dotting into any other
		// value crashes the original.
		std::string name = Pick(6) == 0 ? SetName() : Name();
		if (Pick(5) == 0)
			name = SetName() + (Pick(3) == 0 ? "." + SetName() : "") + "." + Name();
		else if (name == "s" || name == "t[1]")
			return out + name + " {\n" + (depth < 3 ? Statements(depth + 1, blocks) : "") + "}\n";
		switch (Pick(6))
		{
		case 0:
			return out + name + ";\n";
		case 1:
			if (depth < 3)
				return out + name + " {\n" + Statements(depth + 1, blocks) + (Pick(15) == 0 ? "" : "}\n");
			[[fallthrough]];
		default:
			return out + name + " = " + Value(0) + (Pick(25) == 0 ? "\n" : ";\n");
		}
	}

	InputGenerator& m_gen;
};

#if JPOG_HAS_ORIGINAL_MODULES

struct OriginalReader
{
	using P = void*;

	OriginalReader()
	{
		const auto pp = [](const char* name) { return OriginalPP<void*>(name); };
		const char* const typeNames[] = {"INT", "UINT32", "FLOAT", "BOOL", "TPCSTRING", "PROPNAME", "PROPS", "ARRAY"};
		for (int i = 0; i < 8; ++i)
			types[i] = *static_cast<const void* const*>(
			    pp((std::string("?TYPE_") + typeNames[i] + "@PPropertyValue@@2PBVTClass@Toshi@@B").c_str()));
	}

	const Method<void, const char*> constructString =
	    Original<Method<void, const char*>>("??0TCString@Toshi@@QAE@PBD@Z");
	const Method<void> destructString = Original<Method<void>>("??1TCString@Toshi@@QAE@XZ");
	const Method<void> construct = OriginalPP<Method<void>>("??0PPropertyReader@@QAE@XZ");
	const Method<void> destruct = OriginalPP<Method<void>>("??1PPropertyReader@@UAE@XZ");
	const Method<bool, const void*, TFile*> open =
	    OriginalPP<Method<bool, const void*, TFile*>>("?Open@PPropertyReader@@UAE_NABVTCString@Toshi@@PAVTFile@3@@Z");
	const Method<void, bool> setLoadComments =
	    OriginalPP<Method<void, bool>>("?SetLoadComments@PPropertyReader@@QAEX_N@Z");
	const Method<bool, void*> loadBlock =
	    OriginalPP<Method<bool, void*>>("?LoadPropertyBlock@PPropertyReader@@QAE_NAAVPProperties@@@Z");
	const Method<bool> load = OriginalPP<Method<bool>>("?Load@PPropertyReader@@QAE_NXZ");
	const Method<void, void*> addBlock = OriginalPP<Method<void, void*>>(
	    "?AddDefinitionBlock@PPropertyReader@@QAEXV?$TManagedPtr@VPPropertyBlock@@@Toshi@@@Z");
	const Method<void> constructProperties = OriginalPP<Method<void>>("??0PProperties@@QAE@XZ");
	const Method<void> destructProperties = OriginalPP<Method<void>>("??1PProperties@@UAE@XZ");

	const void* types[8];

	std::string DescribeValue(const unsigned char* value) const
	{
		const void* type = *reinterpret_cast<const void* const*>(value);
		const unsigned char* data = value + 4;
		const auto pointer = [&] { return *reinterpret_cast<const unsigned char* const*>(data); };
		if (type == nullptr)
			return "undef";
		if (type == types[0])
			return "i" + std::to_string(*reinterpret_cast<const int*>(data));
		if (type == types[1])
			return "u" + std::to_string(*reinterpret_cast<const unsigned int*>(data));
		if (type == types[2])
			return "f" + FloatBits(*reinterpret_cast<const float*>(data));
		if (type == types[3])
			return *data ? "true" : "false";
		if (type == types[4])
			return "'" + HandleText(pointer()) + "'";
		if (type == types[5])
			return "@" + HandleText(*reinterpret_cast<void* const*>(pointer())) + "[" +
			       HandleText(*reinterpret_cast<void* const*>(pointer() + 4)) + "]";
		if (type == types[6])
			return "{" + DescribeProperties(pointer()) + "}";
		if (type == types[7])
		{
			// Ref count, then TArray {grow, count, size, data}.
			std::string out = "(";
			const int count = *reinterpret_cast<const int*>(pointer() + 8);
			const unsigned char* values = *reinterpret_cast<const unsigned char* const*>(pointer() + 0x10);
			for (int i = 0; i < count; ++i)
				out += DescribeValue(values + i * 8) + ",";
			return out + ")";
		}
		return "?";
	}

	// Node: {next, prev, name, sub-name, value (8), comment}.
	std::string DescribeProperties(const unsigned char* properties) const
	{
		std::string out;
		const auto* root = properties + 0xC;
		for (auto* node = *reinterpret_cast<unsigned char* const*>(root); node != root;
		     node = *reinterpret_cast<unsigned char* const*>(node))
		{
			out += HandleText(*reinterpret_cast<void* const*>(node + 8)) + "[" +
			       HandleText(*reinterpret_cast<void* const*>(node + 0xC)) + "]";
			const std::string comment = HandleText(*reinterpret_cast<void* const*>(node + 0x18));
			if (!comment.empty())
				out += "#" + comment + "#";
			out += "=" + DescribeValue(node + 0x10) + "; ";
		}
		return out;
	}

	void Open(unsigned char* reader, TFile* file, bool comments) const
	{
		construct(reader);
		setLoadComments(reader, comments);
		alignas(void*) unsigned char name[8];
		constructString(name, "test.ini");
		REQUIRE(open(reader, name, file));
		destructString(name);
	}

	// Loads one set and describes it.
	std::string LoadSet(void* reader, bool& result) const
	{
		alignas(void*) unsigned char properties[0x18];
		constructProperties(properties);
		result = loadBlock(reader, properties);
		std::string out = DescribeProperties(properties);
		destructProperties(properties);
		return out;
	}
};

#endif // JPOG_HAS_ORIGINAL_MODULES

} // namespace jpog::test
