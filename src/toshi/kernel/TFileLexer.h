#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "toshi/kernel/SortedArray.h"
#include "toshi/kernel/TCStringPool.h"
#include "toshi/kernel/TEvent.h"
#include "toshi/kernel/TObject.h"
#include "toshi/kernel/TWStringPool.h"

namespace jpog::kernel
{
template <typename Lexer> struct LexerCore;
}

namespace Toshi
{

class TFile;

class TFileLexer
{
public:
	enum TokenType
	{
		TOKEN_EOF,
		TOKEN_SEMI,
		TOKEN_COLON,
		TOKEN_COMMA,
		TOKEN_DOT,
		TOKEN_ASTERISK, // has no name in tostring
		TOKEN_DOLLAR,
		TOKEN_OPENSQR,
		TOKEN_CLOSESQR,
		TOKEN_OPENBRACE,
		TOKEN_CLOSEBRACE,
		TOKEN_OPENPAREN,
		TOKEN_CLOSEPAREN,
		TOKEN_LESSTHAN,
		TOKEN_GREATERTHAN,
		TOKEN_EQUAL,
		TOKEN_IDENT,
		TOKEN_STRING,
		TOKEN_INTEGER,
		TOKEN_UINTEGER,
		TOKEN_FLOAT,
		TOKEN_COMMENT,
	};
};

// Tokenizer for the engine's text formats (TKernelInterface.dll), reading 8-bit characters with
// TFile::GetCChar. Supports // and /* */ comments (optionally returned as tokens), "strings"
// with \ escapes, decimal/hex/float numbers with u/f suffixes, and a line-start preprocessor:
// #ifdef, #ifndef, #else, #endif and #stop (end of input). TOSHI_SKU_WINDOWS is predefined.
// Errors are reported through the parse error emitter. Lookahead buffers are power-of-two
// rings.
class TFileLexerUTF8 : public TObject
{
	friend struct jpog::kernel::LexerCore<TFileLexerUTF8>;

public:
	using Char = char;
	using PooledString = TPCString;

	class Token
	{
	public:
		Token();
		explicit Token(int line); // EOF
		Token(TFileLexer::TokenType type, int line);
		Token(TFileLexer::TokenType type, int line, const TPCString& string);
		Token(int line, int value);
		Token(int line, unsigned int value);
		Token(int line, float value);
		Token(const Token& other);
		~Token();
		Token& operator=(const Token& other);

		// Releases or takes over the string as the types change; values of other types are only
		// copied for numbers.
		void assign(const Token& other);
		TFileLexer::TokenType GetType() const;
		int GetLine() const;
		int GetInteger() const;
		unsigned int GetUInteger() const;
		float GetFloat() const;
		TPCString& GetString();
		const TPCString& GetString() const;
		// "Token[Line: n, ...]" for debugging.
		TCString tostring() const;

	private:
		friend struct jpog::kernel::LexerCore<TFileLexerUTF8>;

		TFileLexer::TokenType m_eType;
		int m_iLine;
		// IDENT, STRING and COMMENT tokens hold the string.
		union
		{
			TPCString m_oString;
			int m_iInteger;
			unsigned int m_uiInteger;
			float m_fFloat;
		};
	};

	class ParseError
	{
	public:
		ParseError& operator=(const ParseError& other);

		int m_iLine;
		const char* m_szMessage;
	};

	TFileLexerUTF8();
	// Starts reading `file` immediately; `tokenLookahead` sizes the PeekNextToken ring.
	TFileLexerUTF8(TFile* file, int tokenLookahead);
	~TFileLexerUTF8() override;

	TClass& GetClass() const override;

	void SetInputStream(TFile* file);
	// Allocates a fresh character ring (the old one is not freed, as in the original) and
	// fills it from the stream.
	void SetCharacterLookaheadSize(int size);
	void SetOutputComments(bool output);
	void Define(const char* name);
	void Undef(const char* name);
	bool IfDef(const char* name);
	Token GetNextToken();
	// The token `index` positions ahead (0 = next); also becomes the last token.
	Token PeekNextToken(int index);
	Token GetLastToken();
	bool Expect(TFileLexer::TokenType type);
	bool Expect(TFileLexer::TokenType type, Token& token);
	TEmitter<TFileLexerUTF8, ParseError>* GetParseErrorEmitter();

	static const char* TSTDCALL tostring(TFileLexer::TokenType type);

	static TClass m_sClass;

protected:
	Token get_next_token();
	void skipWhiteSpace();
	void fillLookAhead();
	void advance();
	void advance(int count);
	int peek();
	int peek(int offset);
	void ThrowError(const char* message);

private:
	bool ComputePreprocessorAllow();

	static TObject* TSTDCALL CreateObject();
	static TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	TFile* m_pFile;
	bool m_bOutputComments;
	int m_iCharLookaheadSize;
	int m_iCharLookaheadMask;
	int* m_piCharLookahead;
	int m_iCharRead;  // ring index of the current character
	int m_iCharWrite; // next ring slot to refill
	int m_iLine;
	int m_iTokenLookaheadSize;
	int m_iTokenLookaheadMask;
	Token m_oLastToken;
	Token* m_pTokenLookahead;
	int m_iTokenCount;
	int m_iTokenRead;
	int m_iTokenWrite;
	int m_iIfDepth;
	bool m_abIfStack[0x20]; // [depth] is the state of the innermost #if; [0] is always true
	bool m_bAllowPreprocessor;
	bool m_bStopped;
	jpog::kernel::SortedArray<TPCString> m_oDefines;
	TEmitter<TFileLexerUTF8, ParseError> m_oParseErrorEmitter;
};

JPOG_ASSERT_SIZE(TFileLexerUTF8::Token, 0xC);
JPOG_ASSERT_SIZE(TFileLexerUTF8, 0x8C);

// The same tokenizer for UTF-16 input (TFile::GetWChar), with wide pooled strings.
class TFileLexerUTF16 : public TObject
{
	friend struct jpog::kernel::LexerCore<TFileLexerUTF16>;

public:
	using Char = unsigned short;
	using PooledString = TPWString;

	class Token
	{
	public:
		Token();
		explicit Token(int line);
		Token(TFileLexer::TokenType type, int line);
		Token(TFileLexer::TokenType type, int line, const TPWString& string);
		Token(int line, int value);
		Token(int line, unsigned int value);
		Token(int line, float value);
		Token(const Token& other);
		~Token();
		Token& operator=(const Token& other);

		void assign(const Token& other);
		TFileLexer::TokenType GetType() const;
		int GetLine() const;
		int GetInteger() const;
		unsigned int GetUInteger() const;
		float GetFloat() const;
		TPWString& GetString();
		const TPWString& GetString() const;
		TCString tostring() const;

	private:
		friend struct jpog::kernel::LexerCore<TFileLexerUTF16>;

		TFileLexer::TokenType m_eType;
		int m_iLine;
		union
		{
			TPWString m_oString;
			int m_iInteger;
			unsigned int m_uiInteger;
			float m_fFloat;
		};
	};

	class ParseError
	{
	public:
		ParseError& operator=(const ParseError& other);

		int m_iLine;
		const char* m_szMessage;
	};

	TFileLexerUTF16();
	TFileLexerUTF16(TFile* file, int tokenLookahead);
	~TFileLexerUTF16() override;

	TClass& GetClass() const override;

	void SetInputStream(TFile* file);
	void SetCharacterLookaheadSize(int size);
	void SetOutputComments(bool output);
	void Define(const unsigned short* name);
	void Undef(const unsigned short* name);
	bool IfDef(const unsigned short* name);
	Token GetNextToken();
	Token PeekNextToken(int index);
	Token GetLastToken();
	bool Expect(TFileLexer::TokenType type);
	bool Expect(TFileLexer::TokenType type, Token& token);
	TEmitter<TFileLexerUTF16, ParseError>* GetParseErrorEmitter();

	static const char* TSTDCALL tostring(TFileLexer::TokenType type);

	static TClass m_sClass;

protected:
	Token get_next_token();
	void skipWhiteSpace();
	void fillLookAhead();
	void advance();
	void advance(int count);
	int peek();
	int peek(int offset);
	void ThrowError(const char* message);

private:
	bool ComputePreprocessorAllow();

	static TObject* TSTDCALL CreateObject();
	static TObject* TSTDCALL CreateObjectInPlace(void* memory);
	static void TSTDCALL InitialiseStatic();
	static void TSTDCALL DeinitialiseStatic();

	TFile* m_pFile;
	bool m_bOutputComments;
	int m_iCharLookaheadSize;
	int m_iCharLookaheadMask;
	int* m_piCharLookahead;
	int m_iCharRead;
	int m_iCharWrite;
	int m_iLine;
	int m_iTokenLookaheadSize;
	int m_iTokenLookaheadMask;
	Token m_oLastToken;
	Token* m_pTokenLookahead;
	int m_iTokenCount;
	int m_iTokenRead;
	int m_iTokenWrite;
	int m_iIfDepth;
	bool m_abIfStack[0x20];
	bool m_bAllowPreprocessor;
	bool m_bStopped;
	jpog::kernel::SortedArray<TPWString> m_oDefines;
	TEmitter<TFileLexerUTF16, ParseError> m_oParseErrorEmitter;
};

JPOG_ASSERT_SIZE(TFileLexerUTF16::Token, 0xC);
JPOG_ASSERT_SIZE(TFileLexerUTF16, 0x8C);

} // namespace Toshi
