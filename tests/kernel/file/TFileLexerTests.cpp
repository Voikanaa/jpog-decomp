#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/file/TFile.h"
#include "toshi/kernel/file/TFileLexer.h"

#include <cstdarg>
#include <cstring>
#include <doctest/doctest.h>
#include <string>
#include <vector>

using Toshi::TCString;
using Toshi::TFile;
using Toshi::TFileLexer;
using Toshi::TFileLexerUTF16;
using Toshi::TFileLexerUTF8;
using namespace jpog::test;

namespace
{

// A read-only file over a buffer of characters (bytes or UTF-16 units).
class TStringFile : public TFile
{
public:
	explicit TStringFile(std::u16string text) : TFile(nullptr), m_text(std::move(text)) {}
	~TStringFile() override = default;

	int Read(void*, int) override { return 0; }
	int Write(const void*, int) override { return 0; }
	bool Seek(int, TSEEK) override { return false; }
	int Tell() override { return static_cast<int>(m_position); }
	int GetSize() override { return static_cast<int>(m_text.size()); }
	int GetCChar() override { return m_position < m_text.size() ? (m_text[m_position++] & 0xFF) : -1; }
	int GetWChar() override { return m_position < m_text.size() ? m_text[m_position++] : -1; }
	int PutCChar(char) override { return -1; }
	int PutWChar(unsigned short) override { return -1; }
	int VCPrintf(const char*, va_list) override { return 0; }
	int VWPrintf(const unsigned short*, va_list) override { return 0; }

private:
	std::u16string m_text;
	std::size_t m_position = 0;
};

std::u16string Widen(const char* text)
{
	std::u16string result;
	for (; *text != '\0'; ++text)
		result += static_cast<char16_t>(static_cast<unsigned char>(*text));
	return result;
}

std::vector<TFileLexer::TokenType> Types(TFileLexerUTF8& lexer)
{
	std::vector<TFileLexer::TokenType> types;
	for (;;)
	{
		const TFileLexerUTF8::Token token = lexer.GetNextToken();
		types.push_back(token.GetType());
		if (token.GetType() == TFileLexer::TOKEN_EOF)
			return types;
	}
}

// Random but well-formed input: the original loops forever on unterminated strings and on EOF
// inside a disabled #if block, so every construct is closed.
class SourceGenerator
{
public:
	SourceGenerator(InputGenerator& gen, bool wide) : m_gen(gen), m_wide(wide) {}

	std::u16string Generate()
	{
		m_text.clear();
		Block(0, 120);
		m_text += u"\n";
		return m_text;
	}

private:
	void Emit(const char* text) { m_text += Widen(text); }
	void Emit(char16_t c) { m_text += c; }
	int Pick(int count) { return static_cast<int>(m_gen.NextU32() % static_cast<unsigned int>(count)); }

	void Space()
	{
		static const char* const spaces[] = {" ", "  ", "\t", "\n", "\r\n", " \n "};
		Emit(spaces[Pick(6)]);
	}

	void Number()
	{
		static const char* const numbers[] = {
		    "0",    "42",   "-17",       "007",    "123456789012", "0x1F",        "0Xffu", "12u", "3.25",      ".5",
		    "1.5f", "2e+3", "6.02E-23f", "1.#INF", "-0.0",         "4294967295u", "-",     "9f",  "0x7fffffff"};
		Emit(numbers[Pick(sizeof(numbers) / sizeof(numbers[0]))]);
	}

	void Word()
	{
		static const char* const words[] = {"ident", "_x", "Rex2", "a_b_c", "TOSHI_SKU_WINDOWS", "u", "e", "x"};
		Emit(words[Pick(sizeof(words) / sizeof(words[0]))]);
		if (Pick(4) == 0)
			Emit(m_wide ? char16_t(0x3042) : char16_t(0xE9)); // a non-ASCII letter
	}

	void Quoted()
	{
		Emit('"');
		for (int i = Pick(6); i > 0; --i)
		{
			static const char* const parts[] = {"text", " ", "\\n", "\\\"", "\\\\", "\\q", "\n", "#", "//"};
			Emit(parts[Pick(sizeof(parts) / sizeof(parts[0]))]);
		}
		Emit('"');
	}

	void Comment()
	{
		if (Pick(2) == 0)
		{
			Emit("// line comment");
			Emit('\n');
		}
		else
			Emit("/* block\n comment * / */");
	}

	void Item()
	{
		switch (Pick(8))
		{
		case 0:
		case 1:
			Word();
			break;
		case 2:
			Number();
			break;
		case 3:
			Quoted();
			break;
		case 4:
			Comment();
			break;
		case 5:
		{
			static const char* const punctuation[] = {";", ":", ",", ".", "*", "$", "[", "]",
			                                          "{", "}", "(", ")", "<", ">", "="};
			Emit(punctuation[Pick(sizeof(punctuation) / sizeof(punctuation[0]))]);
			break;
		}
		default:
			Space();
			break;
		}
	}

	// Directives always start a line; their blocks are always closed.
	void Block(int depth, int items)
	{
		for (int i = 0; i < items; ++i)
		{
			if (Pick(12) == 0 && depth < 4)
			{
				static const char* const names[] = {"TOSHI_SKU_WINDOWS", "FOO", "BAR"};
				Emit(Pick(2) == 0 ? "\n#ifdef " : "\n#ifndef  ");
				Emit(names[Pick(3)]);
				Emit(" trailing\n");
				Block(depth + 1, Pick(8));
				if (Pick(2) == 0)
				{
					Emit("\n#else\n");
					Block(depth + 1, Pick(8));
				}
				Emit("\n#endif\n");
			}
			else if (Pick(60) == 0)
				Emit(Pick(3) == 0 ? "\n#stop\n" : (Pick(2) == 0 ? "\n#bogus\n" : "\n#endif\n"));
			else if (Pick(150) == 0)
			{
				// Errors that end the token stream.
				static const char* const bad[] = {"@", "#", "1.2.3", "5e3", "`"};
				Emit(bad[Pick(5)]);
			}
			else
			{
				Item();
				Space();
			}
		}
	}

	InputGenerator& m_gen;
	bool m_wide;
	std::u16string m_text;
};

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TFileLexer")
{
	TEST_CASE("tokens")
	{
		TStringFile file(Widen("name = 12; f(0x1F, 2.5f) \"a\\nb\" // c\n-3 4u"));
		TFileLexerUTF8 lexer(&file, 4);
		CHECK(lexer.PeekNextToken(1).GetType() == TFileLexer::TOKEN_EQUAL);
		TFileLexerUTF8::Token token = lexer.GetNextToken();
		CHECK(token.GetType() == TFileLexer::TOKEN_IDENT);
		CHECK(std::string(token.GetString().GetCString()) == "name");
		CHECK(lexer.Expect(TFileLexer::TOKEN_EQUAL));
		CHECK(lexer.Expect(TFileLexer::TOKEN_INTEGER, token));
		CHECK(token.GetInteger() == 12);
		CHECK(Types(lexer) == std::vector<TFileLexer::TokenType>{
		                          TFileLexer::TOKEN_SEMI, TFileLexer::TOKEN_IDENT, TFileLexer::TOKEN_OPENPAREN,
		                          TFileLexer::TOKEN_INTEGER, TFileLexer::TOKEN_COMMA, TFileLexer::TOKEN_FLOAT,
		                          TFileLexer::TOKEN_CLOSEPAREN, TFileLexer::TOKEN_STRING, TFileLexer::TOKEN_INTEGER,
		                          TFileLexer::TOKEN_UINTEGER, TFileLexer::TOKEN_EOF});
		CHECK(std::string(TFileLexerUTF8::tostring(TFileLexer::TOKEN_CLOSEBRACE)) == "CLOSEBRACE");
		CHECK(std::string(TFileLexerUTF8::tostring(TFileLexer::TOKEN_ASTERISK)) == "?????");
	}

	TEST_CASE("preprocessor")
	{
		TStringFile file(Widen("a\n#ifdef TOSHI_SKU_WINDOWS\nb\n#else\nc\n#endif\n#ifndef X\nd\n#endif\n#stop\ne"));
		TFileLexerUTF8 lexer(&file, 2);
		CHECK(lexer.IfDef("TOSHI_SKU_WINDOWS"));
		std::string idents;
		for (TFileLexerUTF8::Token token = lexer.GetNextToken(); token.GetType() == TFileLexer::TOKEN_IDENT;
		     token = lexer.GetNextToken())
			idents += static_cast<const char*>(token.GetString().GetCString());
		CHECK(idents == "abd");
		lexer.Undef("TOSHI_SKU_WINDOWS");
		CHECK_FALSE(lexer.IfDef("TOSHI_SKU_WINDOWS"));
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

struct TokenView
{
	int type;
	int line;
	union
	{
		const void* pooled; // TPooledCString / TPooledWString: refcount, then the string
		int integer;
		float real;
	};
};

template <typename Char> std::u16string PooledText(const void* pooled)
{
	if (pooled == nullptr)
		return {};
	const Char* text = *reinterpret_cast<const Char* const*>(static_cast<const char*>(pooled) + 4);
	std::u16string result;
	for (; *text != 0; ++text)
		result += static_cast<char16_t>(static_cast<std::make_unsigned_t<Char>>(*text));
	return result;
}

struct Error
{
	int line;
	std::string message;
	bool operator==(const Error&) const = default;
};

struct ErrorLog : Toshi::TGenericListener
{
	using TGenericListener::Disconnect;
	ErrorLog(void* emitter) { Connect(static_cast<Toshi::TGenericEmitter*>(emitter), this, Record, 0); }
	~ErrorLog() { Disconnect(); }
	static bool TSTDCALL Record(void* receiver, void*, void* event)
	{
		const auto* error = static_cast<const TFileLexerUTF8::ParseError*>(event);
		static_cast<ErrorLog*>(receiver)->errors.push_back({error->m_iLine, error->m_szMessage});
		return true;
	}
	std::vector<Error> errors;
};

// Drives one original and one reimplemented lexer of the same type over the same input.
template <typename Lexer, typename Char> void CompareLexers(const char* suffix, bool wide)
{
	const std::string lexerName = wide ? "TFileLexerUTF16" : "TFileLexerUTF8";
	auto symbol = [&](const char* pattern)
	{
		std::string s = pattern;
		for (std::size_t at; (at = s.find('#')) != std::string::npos;)
			s.replace(at, 1, lexerName);
		return s;
	};
	const auto construct = Original<Method<void, TFile*, int>>(symbol("??0#@Toshi@@QAE@PAVTFile@1@H@Z").c_str());
	const auto destruct = Original<Method<void>>(symbol("??1#@Toshi@@UAE@XZ").c_str());
	const auto next = Original<Method<void*, void*>>(symbol("?GetNextToken@#@Toshi@@QAE?AVToken@12@XZ").c_str());
	const auto peek = Original<Method<void*, void*, int>>(symbol("?PeekNextToken@#@Toshi@@QAE?AVToken@12@H@Z").c_str());
	const auto releaseToken = Original<Method<void>>(symbol("??1Token@#@Toshi@@QAE@XZ").c_str());
	const auto describe =
	    Original<Method<TCString*, TCString*>>(symbol("?tostring@Token@#@Toshi@@QBE?AVTCString@3@XZ").c_str());
	const auto destroyString = Original<Method<void>>("??1TCString@Toshi@@QAE@XZ");
	const auto setComments = Original<Method<void, bool>>(symbol("?SetOutputComments@#@Toshi@@QAEX_N@Z").c_str());
	const auto define = Original<Method<void, const Char*>>(
	    symbol((std::string("?Define@#@Toshi@@QAEXP") + suffix + "@Z").c_str()).c_str());

	auto same = [&](const TokenView& a, const TokenView& b)
	{
		if (a.type != b.type || a.line != b.line)
			return false;
		if (a.type == TFileLexer::TOKEN_IDENT || a.type == TFileLexer::TOKEN_STRING ||
		    a.type == TFileLexer::TOKEN_COMMENT)
			return PooledText<Char>(a.pooled) == PooledText<Char>(b.pooled);
		if (a.type >= TFileLexer::TOKEN_INTEGER && a.type <= TFileLexer::TOKEN_FLOAT)
			return a.integer == b.integer;
		return true;
	};

	InputGenerator gen;
	int tokens = 0;
	for (int round = 0; round < 400; ++round)
	{
		const std::u16string text = SourceGenerator(gen, wide).Generate();
		TStringFile originalFile(text), ourFile(text);
		// Sizes 2 and 4 only: other sizes make the original index past its token ring.
		const int lookahead = gen.NextU32() % 2 == 0 ? 2 : 4;
		alignas(void*) unsigned char original[sizeof(Lexer)];
		construct(original, &originalFile, lookahead);
		auto* ours = new Lexer(&ourFile, lookahead);
		const bool comments = round % 3 == 0;
		setComments(original, comments);
		ours->SetOutputComments(comments);
		if (round % 4 == 1)
		{
			const Char foo[] = {'F', 'O', 'O', 0};
			define(original, foo);
			ours->Define(foo);
		}
		ErrorLog originalErrors(original + 0x80), ourErrors(ours->GetParseErrorEmitter());

		for (int step = 0; step < 2000; ++step)
		{
			INFO("round ", round, " step ", step);
			alignas(void*) unsigned char storage[sizeof(typename Lexer::Token)];
			const bool peeking = gen.NextU32() % 4 == 0;
			const int index = gen.NextInt(0, lookahead - 1);
			if (peeking)
				peek(original, storage, index);
			else
				next(original, storage);
			const typename Lexer::Token token = peeking ? ours->PeekNextToken(index) : ours->GetNextToken();
			const auto& a = *reinterpret_cast<const TokenView*>(storage);
			const auto& b = *reinterpret_cast<const TokenView*>(&token);
			REQUIRE(same(a, b));
			if (step % 16 == 0)
			{
				alignas(void*) unsigned char described[sizeof(TCString)];
				auto* originalText = reinterpret_cast<TCString*>(described);
				describe(storage, originalText);
				CHECK(std::string(static_cast<const char*>(*originalText)) ==
				      static_cast<const char*>(token.tostring()));
				destroyString(originalText);
			}
			releaseToken(storage);
			++tokens;
			if (b.type == TFileLexer::TOKEN_EOF && !peeking)
				break;
		}
		CHECK(originalErrors.errors == ourErrors.errors);
		originalErrors.Disconnect();
		ourErrors.Disconnect();
		destruct(original);
		delete ours;
	}
	MESSAGE(tokens, " tokens compared");
}

} // namespace

TEST_SUITE("TFileLexer differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("UTF-8 lexer matches the original")
	{
		CompareLexers<TFileLexerUTF8, char>("BD", false);
	}

	TEST_CASE("UTF-16 lexer matches the original")
	{
		CompareLexers<TFileLexerUTF16, unsigned short>("BG", true);
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
