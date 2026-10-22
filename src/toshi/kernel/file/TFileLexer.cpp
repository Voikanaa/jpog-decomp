#include "toshi/kernel/file/TFileLexer.h"

#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/file/TFile.h"
#include "toshi/kernel/memory/TMemory.h"

#include <bit>
#include <cstdio>
#include <cwctype>
#include <new>

namespace jpog::kernel
{

using Toshi::TCString;
using Toshi::TFile;
using Toshi::TFileLexer;
using Toshi::TPCString;
using Toshi::TPWString;
using Toshi::TSystem;

// Character classes through the wide C runtime, as the original (wint_t truncates to 16 bits,
// so EOF is 0xFFFF and in no class).
bool IsSpace(int c)
{
	return std::iswspace(static_cast<wint_t>(c)) != 0;
}
bool IsAlpha(int c)
{
	return std::iswalpha(static_cast<wint_t>(c)) != 0;
}
bool IsAlnum(int c)
{
	return std::iswalnum(static_cast<wint_t>(c)) != 0;
}
bool IsDigit(int c)
{
	return std::iswdigit(static_cast<wint_t>(c)) != 0;
}
bool IsXDigit(int c)
{
	return std::iswxdigit(static_cast<wint_t>(c)) != 0;
}

// IDENT, STRING and COMMENT tokens carry a pooled string.
bool HasString(TFileLexer::TokenType type)
{
	return type == TFileLexer::TOKEN_IDENT || type == TFileLexer::TOKEN_STRING || type == TFileLexer::TOKEN_COMMENT;
}

// Smallest power of two >= n, taken from the float exponent of 2n - 1 as the original does.
int NextPowerOfTwo(int n)
{
	const int bits = std::bit_cast<int>(static_cast<float>(n * 2 - 1));
	return 1 << (((bits >> 23) + 0x81) & 0x1F);
}

// Per character type: input, pooling and number scanning.
int ReadChar(TFile* file, char)
{
	return file->GetCChar();
}
int ReadChar(TFile* file, unsigned short)
{
	return file->GetWChar();
}
TPCString Pool(const char* string)
{
	return TSystem::GetCStringPool()->Get(string);
}
TPWString Pool(const unsigned short* string)
{
	return TSystem::GetWStringPool()->Get(string);
}
const char* Text(const TPCString& string)
{
	return string.GetCString().GetString(0);
}
const unsigned short* Text(const TPWString& string)
{
	return string.GetWString().GetString(0);
}
TCString Narrow(const TPCString& string)
{
	return string.GetCString();
}
TCString Narrow(const TPWString& string)
{
	return TCString(string.GetWString());
}
// The scans ignore failures, so `value` keeps its zero (the original's is uninitialised).
template <typename T> void Scan(const char* text, const char* format, T* value)
{
	std::sscanf(text, format, value);
}
// Number tokens are ASCII, so the wide text is narrowed (anything else stops the scan, as it
// would swscanf's).
template <typename T> void Scan(const unsigned short* text, const char* format, T* value)
{
	char narrow[0x100];
	std::size_t i = 0;
	for (; i + 1 < sizeof(narrow) && text[i] != 0; ++i)
		narrow[i] = text[i] < 0x80 ? static_cast<char>(text[i]) : '?';
	narrow[i] = '\0';
	std::sscanf(narrow, format, value);
}
template <typename Char> bool Equals(const Char* text, const char* ascii)
{
	Char literal[8] = {};
	for (int i = 0; ascii[i] != '\0'; ++i)
		literal[i] = static_cast<Char>(ascii[i]);
	return TSystem::StringCompare(text, literal, -1) == 0;
}

template <typename Lexer> struct LexerCore
{
	using Char = typename Lexer::Char;
	using Token = typename Lexer::Token;
	using String = typename Lexer::PooledString;
	using Type = TFileLexer::TokenType;

	// Token text is collected in one static buffer per lexer type (as the original). Writes
	// past its end are dropped (the original overruns it).
	static constexpr int kBufferSize = 0x800;
	static inline Char s_buffer[kBufferSize];

	static void Put(int index, int c)
	{
		if (index >= 0 && index < kBufferSize)
			s_buffer[index] = static_cast<Char>(c);
	}

	// --- Token ---------------------------------------------------------------------------

	static void Construct(Token& token, Type type, int line)
	{
		token.m_eType = type;
		token.m_iLine = line;
	}

	static void ConstructString(Token& token, Type type, int line, const String& string)
	{
		Construct(token, type, line);
		new (&token.m_oString) String();
		token.m_oString = string;
	}

	static void Destroy(Token& token)
	{
		if (HasString(token.m_eType))
			token.m_oString.~String();
	}

	static void Assign(Token& token, const Token& other)
	{
		const bool had = HasString(token.m_eType);
		const bool has = HasString(other.m_eType);
		if (had && !has)
			token.m_oString.~String();
		else if (!had && has)
			new (&token.m_oString) String();
		token.m_eType = other.m_eType;
		token.m_iLine = other.m_iLine;
		if (has)
			token.m_oString = other.m_oString;
		else if (other.m_eType == TFileLexer::TOKEN_INTEGER || other.m_eType == TFileLexer::TOKEN_UINTEGER ||
		         other.m_eType == TFileLexer::TOKEN_FLOAT)
			token.m_iInteger = other.m_iInteger;
	}

	static TCString Describe(const Token& token)
	{
		TCString text;
		switch (token.m_eType)
		{
		case TFileLexer::TOKEN_IDENT:
			text = "IDENT:" + Narrow(token.m_oString);
			break;
		case TFileLexer::TOKEN_STRING:
			text = "STRING:" + Narrow(token.m_oString);
			break;
		case TFileLexer::TOKEN_INTEGER:
			text.Format("%sINTEGER:%d", text.GetString(0), token.m_iInteger);
			break;
		case TFileLexer::TOKEN_UINTEGER:
			text.Format("%sUINTEGER:%d", text.GetString(0), token.m_uiInteger);
			break;
		case TFileLexer::TOKEN_FLOAT:
			text.Format("%sFLOAT:%f", text.GetString(0), static_cast<double>(token.m_fFloat));
			break;
		default:
			text = Lexer::tostring(token.m_eType);
			break;
		}
		TCString result;
		result.Format("Token[Line: %d, %s]", token.m_iLine, text.GetString(0));
		return result;
	}

	// --- Lexer state -----------------------------------------------------------------------

	// The token ring gets at least size (and mask + 1) slots: the original allocates exactly
	// `lookahead` and can index past it.
	static void Initialise(Lexer& lexer, TFile* file, int lookahead, int size, int mask)
	{
		lexer.m_pFile = file;
		lexer.m_bOutputComments = false;
		lexer.m_iCharLookaheadSize = 0;
		lexer.m_iCharLookaheadMask = 0;
		lexer.m_piCharLookahead = nullptr;
		lexer.m_iCharRead = 0;
		lexer.m_iCharWrite = 0;
		lexer.m_iLine = 0;
		lexer.m_iTokenLookaheadSize = size;
		lexer.m_iTokenLookaheadMask = mask;
		int slots = lookahead;
		if (slots < size)
			slots = size;
		if (slots < mask + 1)
			slots = mask + 1;
		lexer.m_pTokenLookahead = new Token[static_cast<std::size_t>(slots)];
		lexer.m_iTokenCount = 0;
		lexer.m_iTokenRead = 0;
		lexer.m_iTokenWrite = 0;
		lexer.m_iIfDepth = 0;
		for (bool& state : lexer.m_abIfStack)
			state = false;
		lexer.m_bAllowPreprocessor = true;
		lexer.m_bStopped = false;
	}

	static void SetInputStream(Lexer& lexer, TFile* file)
	{
		lexer.m_pFile = file;
		lexer.m_iLine = 1;
		lexer.m_iIfDepth = 0;
		lexer.m_abIfStack[0] = true;
		lexer.m_bAllowPreprocessor = lexer.ComputePreprocessorAllow();
		lexer.SetCharacterLookaheadSize(3);
	}

	static void SetCharacterLookaheadSize(Lexer& lexer, int size)
	{
		const int ring = NextPowerOfTwo(size);
		lexer.m_iCharLookaheadSize = ring;
		lexer.m_iCharLookaheadMask = ring - 1;
		lexer.m_iCharRead = 0;
		lexer.m_iCharWrite = 0;
		lexer.m_piCharLookahead = static_cast<int*>(tmalloc(ring * 4, nullptr, -1));
		for (int i = 0; i < lexer.m_iCharLookaheadSize; ++i)
			lexer.m_piCharLookahead[i] = ReadChar(lexer.m_pFile, Char());
	}

	// Entries past the stack (nesting of 32 or more) read as true; the original reads the
	// following members.
	static bool ComputePreprocessorAllow(const Lexer& lexer)
	{
		for (int i = 0; i < lexer.m_iIfDepth; ++i)
		{
			const int level = lexer.m_iIfDepth - i;
			if (level < 0x20 && !lexer.m_abIfStack[level])
				return false;
		}
		return true;
	}

	static void PushIf(Lexer& lexer, bool state)
	{
		const int previous = lexer.m_iIfDepth++;
		if (lexer.m_iIfDepth < 0x20)
			lexer.m_abIfStack[previous + 1] = state;
	}

	// --- Characters ------------------------------------------------------------------------

	static int Peek(const Lexer& lexer) { return lexer.m_piCharLookahead[lexer.m_iCharRead]; }

	static int Peek(const Lexer& lexer, int offset)
	{
		return lexer.m_piCharLookahead[(lexer.m_iCharRead + offset) & lexer.m_iCharLookaheadMask];
	}

	static void Advance(Lexer& lexer)
	{
		lexer.m_iCharRead = (lexer.m_iCharRead + 1) & lexer.m_iCharLookaheadMask;
		lexer.m_piCharLookahead[lexer.m_iCharWrite] = ReadChar(lexer.m_pFile, Char());
		lexer.m_iCharWrite = lexer.m_iCharRead;
	}

	static void Advance(Lexer& lexer, int count)
	{
		lexer.m_iCharRead = (lexer.m_iCharRead + count) & lexer.m_iCharLookaheadMask;
		FillLookAhead(lexer);
	}

	static void FillLookAhead(Lexer& lexer)
	{
		while (lexer.m_iCharWrite != lexer.m_iCharRead)
		{
			lexer.m_piCharLookahead[lexer.m_iCharWrite] = ReadChar(lexer.m_pFile, Char());
			lexer.m_iCharWrite = (lexer.m_iCharWrite + 1) & lexer.m_iCharLookaheadMask;
		}
	}

	static void Error(Lexer& lexer, const char* message)
	{
		typename Lexer::ParseError error{lexer.m_iLine, message};
		lexer.m_oParseErrorEmitter.Throw(&error);
	}

	// --- Defines -----------------------------------------------------------------------------

	static int CompareStrings(const String& a, const String& b) { return a.Compare(b); }

	static void Define(Lexer& lexer, const Char* name)
	{
		const String string = Pool(name);
		bool found = false;
		const int index = lexer.m_oDefines.Search(string, CompareStrings, found);
		if (found)
			return;
		lexer.m_oDefines.InsertSlots(index, 1);
		lexer.m_oDefines[index] = string;
	}

	static void Undef(Lexer& lexer, const Char* name)
	{
		const String string = Pool(name);
		const int index = lexer.m_oDefines.FindFirst(string, CompareStrings);
		if (index != -1)
			lexer.m_oDefines.RemoveAt(index);
	}

	static bool IfDef(const Lexer& lexer, const Char* name)
	{
		for (int i = 0; i < lexer.m_oDefines.count; ++i)
			if (TSystem::StringCompare(name, Text(lexer.m_oDefines[i]), -1) == 0)
				return true;
		return false;
	}

	// --- Scanning ----------------------------------------------------------------------------

	enum class Directive
	{
		IfDef,
		IfNDef,
		Else,
		EndIf,
		Stop,
		Unknown
	};

	// Reads non-space characters into the buffer (stops at EOF); returns the next character.
	static int ReadWord(Lexer& lexer)
	{
		int length = 0;
		int c = Peek(lexer);
		while (!IsSpace(c) && c != -1)
		{
			Put(length++, c);
			Advance(lexer);
			c = Peek(lexer);
		}
		Put(length, 0);
		return c;
	}

	// Skips white space, comments (unless they are output as tokens) and preprocessor lines, and
	// skips input disabled by #if blocks. Directives are only recognised right after a newline
	// (not on the first line). Where the original loops forever at EOF (inside a disabled
	// block), this stops.
	static void SkipWhiteSpace(Lexer& lexer)
	{
		int c = Peek(lexer);
		for (;;)
		{
			bool newline = false;
			while (IsSpace(c) || c == 0xFEFF)
			{
				Advance(lexer);
				newline = c == '\n';
				if (newline)
					++lexer.m_iLine;
				c = Peek(lexer);
			}
			if (!lexer.m_bAllowPreprocessor)
			{
				for (int current = Peek(lexer); current != '#' || !newline; current = Peek(lexer))
				{
					if (current == -1)
						break;
					Advance(lexer);
					newline = current == '\n';
					if (newline)
						++lexer.m_iLine;
				}
			}
			if (newline && Peek(lexer) == '#')
			{
				Advance(lexer);
				ReadWord(lexer);
				Directive directive = Directive::Unknown;
				if (Equals(s_buffer, "ifdef"))
					directive = Directive::IfDef;
				else if (Equals(s_buffer, "ifndef"))
					directive = Directive::IfNDef;
				else if (Equals(s_buffer, "else"))
					directive = Directive::Else;
				else if (Equals(s_buffer, "endif"))
					directive = Directive::EndIf;
				else if (Equals(s_buffer, "stop"))
					directive = Directive::Stop;

				if (directive == Directive::Unknown)
				{
					// The wide lexer passes its wide buffer to %s too (so only its first
					// character shows), as the original.
					TCString message;
					message.Format("Unknown preprocessor command '#%s'", s_buffer);
					Error(lexer, message);
					c = Peek(lexer);
					continue;
				}
				if (directive == Directive::IfDef || directive == Directive::IfNDef)
				{
					while (Peek(lexer) == ' ')
						Advance(lexer);
					if (ReadWord(lexer) == -1)
					{
						Error(lexer, "Unexpected EOF while processing preprocessor directive");
						c = Peek(lexer);
						continue;
					}
					const bool defined = lexer.IfDef(s_buffer);
					PushIf(lexer, directive == Directive::IfDef ? defined : !defined);
					lexer.m_bAllowPreprocessor = lexer.ComputePreprocessorAllow();
				}
				else if (directive == Directive::Else)
				{
					if (lexer.m_iIfDepth == 0)
					{
						Error(lexer, "Unexpected #else directive");
						c = Peek(lexer);
						continue;
					}
					if (lexer.m_iIfDepth < 0x20)
						lexer.m_abIfStack[lexer.m_iIfDepth] = !lexer.m_abIfStack[lexer.m_iIfDepth];
					lexer.m_bAllowPreprocessor = lexer.ComputePreprocessorAllow();
				}
				else if (directive == Directive::EndIf)
				{
					if (lexer.m_iIfDepth == 0)
					{
						Error(lexer, "Unexpected #endif directive");
						c = Peek(lexer);
						continue;
					}
					lexer.m_iIfDepth = lexer.m_iIfDepth - 1 < 0 ? 0 : lexer.m_iIfDepth - 1;
					lexer.m_bAllowPreprocessor = lexer.ComputePreprocessorAllow();
				}
				else if (lexer.m_bAllowPreprocessor) // #stop: the input ends here
				{
					for (int i = 0; i < lexer.m_iTokenLookaheadSize; ++i)
						lexer.m_pTokenLookahead[i].assign(Token(lexer.m_iLine));
					lexer.m_piCharLookahead[lexer.m_iCharRead] = -1;
					lexer.m_bStopped = true;
					return;
				}
				for (c = Peek(lexer); c != '\n' && c != -1; c = Peek(lexer))
					Advance(lexer);
				continue;
			}
			if (lexer.m_bOutputComments)
				return;
			if (Peek(lexer, 0) == '/' && Peek(lexer, 1) == '/')
			{
				Advance(lexer, 1);
				Advance(lexer);
				for (c = Peek(lexer); c != '\n' && c != -1; c = Peek(lexer))
					Advance(lexer);
				continue;
			}
			if (Peek(lexer, 0) != '/' || Peek(lexer, 1) != '*')
				return;
			Advance(lexer, 2);
			for (;;)
			{
				const int current = Peek(lexer);
				if (current == '*' && Peek(lexer, 1) == '/')
					break;
				if (current == -1)
				{
					Error(lexer, "Unterminated block comment /* ... */");
					return;
				}
				if (current == '\n')
					++lexer.m_iLine;
				Advance(lexer);
			}
			Advance(lexer, 2);
			c = Peek(lexer);
		}
	}

	static Token StringToken(Lexer& lexer, Type type)
	{
		return Token(type, lexer.m_iLine, Pool(static_cast<const Char*>(s_buffer)));
	}

	// Comment tokens (when output). The original's line comment loops forever at EOF; this
	// stops.
	static Token LineComment(Lexer& lexer)
	{
		Advance(lexer, 2);
		int length = 0;
		for (int c = Peek(lexer); c != '\n' && c != '\r' && c != -1; c = Peek(lexer))
		{
			Put(length++, c);
			Advance(lexer);
		}
		Put(length, 0);
		return StringToken(lexer, TFileLexer::TOKEN_COMMENT);
	}

	static Token BlockComment(Lexer& lexer)
	{
		Advance(lexer, 2);
		int length = 0;
		for (;;)
		{
			const int c = Peek(lexer);
			if ((c == '*' && Peek(lexer, 1) == '/') || c == -1)
				break;
			if (c == '\n')
				++lexer.m_iLine;
			Put(length++, c);
			Advance(lexer);
		}
		Put(length, 0);
		Advance(lexer, 2);
		return StringToken(lexer, TFileLexer::TOKEN_COMMENT);
	}

	// "..." with \ escapes (\n is a newline, anything else stands for itself). The original
	// loops forever at EOF; this ends the string there.
	static Token QuotedString(Lexer& lexer, int quote)
	{
		Advance(lexer);
		Char previous = static_cast<Char>(quote);
		int length = 0;
		int c = Peek(lexer);
		for (;;)
		{
			if (c == '"')
			{
				if (previous != '\\')
				{
					Advance(lexer);
					Put(length, 0);
					return StringToken(lexer, TFileLexer::TOKEN_STRING);
				}
			}
			else if (c == '\\')
			{
				Advance(lexer);
				int escaped = Peek(lexer);
				if (escaped == '\n')
					++lexer.m_iLine;
				else if (escaped == 'n')
					escaped = '\n';
				Put(length++, escaped);
				Advance(lexer);
				previous = 0;
				c = Peek(lexer);
				continue;
			}
			else if (c == '\n')
				++lexer.m_iLine;
			else if (c == -1)
			{
				Put(length, 0);
				return StringToken(lexer, TFileLexer::TOKEN_STRING);
			}
			previous = static_cast<Char>(c);
			Put(length++, c);
			Advance(lexer);
			c = Peek(lexer);
		}
	}

	// Decimal, hex (0x..., any digit before the x), float (. or e+/e- or f suffix, or a
	// "1.#INF" style # suffix) and unsigned (u suffix) numbers; a leading - is allowed.
	static Token Number(Lexer& lexer, int first)
	{
		bool hex = false;
		bool isFloat = first == '.';
		bool isUnsigned = false;
		if (isFloat)
			Put(0, '0');
		int end = isFloat ? 1 : 0;
		Put(end, first);
		Advance(lexer);
		int c = Peek(lexer);
		if (c == 'x' || c == 'X')
		{
			hex = true;
			end = 0;
			Advance(lexer);
			for (c = Peek(lexer); IsXDigit(c); c = Peek(lexer))
			{
				Put(end++, c);
				Advance(lexer);
			}
			if (c == 'u')
			{
				Advance(lexer);
				isUnsigned = true;
			}
		}
		else
		{
			int last = end; // index of the last character written
			for (;;)
			{
				for (; IsDigit(c); c = Peek(lexer))
				{
					Put(++last, c);
					Advance(lexer);
				}
				end = last + 1;
				if (c != '.')
					break;
				if (isFloat)
				{
					Error(lexer, "Floating point number has multiple decimal places");
					return Token(0);
				}
				Put(end, '.');
				Advance(lexer);
				c = Peek(lexer);
				isFloat = true;
				last = end;
			}
			if (c == 'f')
			{
				Advance(lexer);
				isFloat = true;
			}
			else if (c == 'u')
			{
				Advance(lexer);
				isUnsigned = true;
			}
			else if (c == 'e' || c == 'E')
			{
				Put(end, c);
				Advance(lexer);
				const int sign = Peek(lexer);
				if (sign != '+' && sign != '-')
				{
					Error(lexer, "Exponential number not of form e+ or e-");
					return Token(0);
				}
				Put(end + 1, sign);
				end += 2;
				Advance(lexer);
				for (c = Peek(lexer); IsDigit(c); c = Peek(lexer))
				{
					Put(end++, c);
					Advance(lexer);
				}
				if (c == 'f')
					Advance(lexer);
				isFloat = true;
			}
			else if (c == '#')
			{
				Put(end, '#');
				Advance(lexer);
				++end;
				for (c = Peek(lexer); IsAlpha(c); c = Peek(lexer))
				{
					Put(end++, c);
					Advance(lexer);
				}
				isFloat = true;
			}
		}
		Put(end, 0);

		const int line = lexer.m_iLine;
		if (isUnsigned)
		{
			unsigned int value = 0;
			Scan(s_buffer, hex ? "%x" : "%u", &value);
			return Token(line, value);
		}
		if (hex)
		{
			int value = 0;
			Scan(s_buffer, "%x", &value);
			return Token(line, value);
		}
		if (isFloat)
		{
			float value = 0.0f;
			Scan(s_buffer, "%f", &value);
			return Token(line, value);
		}
		// Decimal, with wrapping arithmetic.
		unsigned int value = 0;
		for (int i = s_buffer[0] == '-' ? 1 : 0; s_buffer[i] != 0; ++i)
			value = static_cast<unsigned int>(CharValue(s_buffer[i])) + value * 10 - '0';
		if (s_buffer[0] == '-')
			value = 0 - value;
		return Token(line, static_cast<int>(value));
	}

	// The narrow lexer's characters are signed.
	static int CharValue(char c) { return static_cast<signed char>(c); }
	static int CharValue(unsigned short c) { return c; }

	static Token NextToken(Lexer& lexer)
	{
		if (lexer.m_bStopped)
			return Token(lexer.m_iLine);
		SkipWhiteSpace(lexer);
		const int c = Peek(lexer);
		if (c == -1)
			return Token(lexer.m_iLine);
		if (lexer.m_bOutputComments && c == '/' && Peek(lexer, 1) == '/')
			return LineComment(lexer);
		if (lexer.m_bOutputComments && c == '/' && Peek(lexer, 1) == '*')
			return BlockComment(lexer);

		if (IsAlpha(c) || c == '_')
		{
			Put(0, c);
			Advance(lexer);
			int length = 1;
			for (int next = Peek(lexer); IsAlnum(next) || next == '_'; next = Peek(lexer))
			{
				Put(length++, next);
				Advance(lexer);
			}
			Put(length, 0);
			return StringToken(lexer, TFileLexer::TOKEN_IDENT);
		}
		if (IsDigit(c) || c == '-' || (c == '.' && IsDigit(Peek(lexer, 1))))
			return Number(lexer, c);
		if (c == '"')
			return QuotedString(lexer, c);

		Type type;
		switch (c)
		{
		case '$':
			type = TFileLexer::TOKEN_DOLLAR;
			break;
		case '(':
			type = TFileLexer::TOKEN_OPENPAREN;
			break;
		case ')':
			type = TFileLexer::TOKEN_CLOSEPAREN;
			break;
		case '*':
			type = TFileLexer::TOKEN_ASTERISK;
			break;
		case ',':
			type = TFileLexer::TOKEN_COMMA;
			break;
		case '.':
			type = TFileLexer::TOKEN_DOT;
			break;
		case ':':
			type = TFileLexer::TOKEN_COLON;
			break;
		case ';':
			type = TFileLexer::TOKEN_SEMI;
			break;
		case '<':
			type = TFileLexer::TOKEN_LESSTHAN;
			break;
		case '=':
			type = TFileLexer::TOKEN_EQUAL;
			break;
		case '>':
			type = TFileLexer::TOKEN_GREATERTHAN;
			break;
		case '[':
			type = TFileLexer::TOKEN_OPENSQR;
			break;
		case ']':
			type = TFileLexer::TOKEN_CLOSESQR;
			break;
		case '{':
			type = TFileLexer::TOKEN_OPENBRACE;
			break;
		case '}':
			type = TFileLexer::TOKEN_CLOSEBRACE;
			break;
		default:
			// The character is not consumed, so the lexer stays here.
			Error(lexer, "Invalid character");
			return Token(0);
		}
		Advance(lexer);
		return Token(type, lexer.m_iLine);
	}

	// --- Token queue ---------------------------------------------------------------------------

	static Token GetNextToken(Lexer& lexer)
	{
		if (lexer.m_iTokenCount < 1)
			lexer.m_oLastToken.assign(lexer.get_next_token());
		else
		{
			lexer.m_oLastToken.assign(lexer.m_pTokenLookahead[lexer.m_iTokenRead]);
			lexer.m_iTokenRead = (lexer.m_iTokenRead + 1) & lexer.m_iTokenLookaheadMask;
			--lexer.m_iTokenCount;
		}
		return lexer.m_oLastToken;
	}

	static Token PeekNextToken(Lexer& lexer, int index)
	{
		if (index >= lexer.m_iTokenCount)
		{
			do
			{
				lexer.m_pTokenLookahead[lexer.m_iTokenWrite].assign(lexer.get_next_token());
				lexer.m_iTokenWrite = (lexer.m_iTokenWrite + 1) & lexer.m_iTokenLookaheadMask;
			} while (++lexer.m_iTokenCount <= index);
		}
		lexer.m_oLastToken.assign(lexer.m_pTokenLookahead[(index + lexer.m_iTokenRead) & lexer.m_iTokenLookaheadMask]);
		return lexer.m_oLastToken;
	}
};

const char* TokenName(TFileLexer::TokenType type)
{
	static const char* const names[] = {"EOF",        "SEMI",     "COLON",       "COMMA",     "DOT",        nullptr,
	                                    "DOLLAR",     "OPENSQR",  "CLOSESQR",    "OPENBRACE", "CLOSEBRACE", "OPENPAREN",
	                                    "CLOSEPAREN", "LESSTHAN", "GREATERTHAN", "EQUAL"};
	if (type < 0 || type >= static_cast<int>(sizeof(names) / sizeof(names[0])) || names[type] == nullptr)
		return "?????";
	return names[type];
}

// Token ring sizes from the constructor argument.
int TokenRingSize(int lookahead)
{
	return NextPowerOfTwo(lookahead);
}
int TokenRingMask(int size)
{
	return size - 1 < 2 ? 1 : size - 1;
}

} // namespace jpog::kernel

namespace Toshi
{

using jpog::kernel::LexerCore;

// Both lexers register as "LexerT" (the original's name for both).
TClass TFileLexerUTF8::m_sClass("LexerT", &TObject::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                                DeinitialiseStatic, 0x10000);
TClass TFileLexerUTF16::m_sClass("LexerT", &TObject::m_sClass, CreateObject, CreateObjectInPlace, InitialiseStatic,
                                 DeinitialiseStatic, 0x10000);

// The two lexers share their implementation (LexerCore); only the character type differs.
#define JPOG_DEFINE_LEXER(Lexer, PooledString, SkuName) \
	Lexer::Token::Token() \
	{ \
		LexerCore<Lexer>::Construct(*this, TFileLexer::TOKEN_EOF, 0); \
	} \
	Lexer::Token::Token(int line) \
	{ \
		LexerCore<Lexer>::Construct(*this, TFileLexer::TOKEN_EOF, line); \
	} \
	Lexer::Token::Token(TFileLexer::TokenType type, int line) \
	{ \
		LexerCore<Lexer>::Construct(*this, type, line); \
	} \
	Lexer::Token::Token(TFileLexer::TokenType type, int line, const PooledString& string) \
	{ \
		LexerCore<Lexer>::ConstructString(*this, type, line, string); \
	} \
	Lexer::Token::Token(int line, int value) \
	{ \
		LexerCore<Lexer>::Construct(*this, TFileLexer::TOKEN_INTEGER, line); \
		m_iInteger = value; \
	} \
	Lexer::Token::Token(int line, unsigned int value) \
	{ \
		LexerCore<Lexer>::Construct(*this, TFileLexer::TOKEN_UINTEGER, line); \
		m_uiInteger = value; \
	} \
	Lexer::Token::Token(int line, float value) \
	{ \
		LexerCore<Lexer>::Construct(*this, TFileLexer::TOKEN_FLOAT, line); \
		m_fFloat = value; \
	} \
	Lexer::Token::Token(const Token& other) \
	{ \
		m_eType = TFileLexer::TOKEN_EOF; \
		LexerCore<Lexer>::Assign(*this, other); \
	} \
	Lexer::Token::~Token() \
	{ \
		LexerCore<Lexer>::Destroy(*this); \
	} \
	Lexer::Token& Lexer::Token::operator=(const Token& other) \
	{ \
		LexerCore<Lexer>::Assign(*this, other); \
		return *this; \
	} \
	void Lexer::Token::assign(const Token& other) \
	{ \
		LexerCore<Lexer>::Assign(*this, other); \
	} \
	TFileLexer::TokenType Lexer::Token::GetType() const \
	{ \
		return m_eType; \
	} \
	int Lexer::Token::GetLine() const \
	{ \
		return m_iLine; \
	} \
	int Lexer::Token::GetInteger() const \
	{ \
		return m_iInteger; \
	} \
	unsigned int Lexer::Token::GetUInteger() const \
	{ \
		return m_uiInteger; \
	} \
	float Lexer::Token::GetFloat() const \
	{ \
		return m_fFloat; \
	} \
	PooledString& Lexer::Token::GetString() \
	{ \
		return m_oString; \
	} \
	const PooledString& Lexer::Token::GetString() const \
	{ \
		return m_oString; \
	} \
	TCString Lexer::Token::tostring() const \
	{ \
		return LexerCore<Lexer>::Describe(*this); \
	} \
	Lexer::ParseError& Lexer::ParseError::operator=(const ParseError& other) \
	{ \
		m_iLine = other.m_iLine; \
		m_szMessage = other.m_szMessage; \
		return *this; \
	} \
	Lexer::Lexer() : m_oDefines(10), m_oParseErrorEmitter(this) \
	{ \
		LexerCore<Lexer>::Initialise(*this, nullptr, 1, 1, 1); \
		Define(SkuName); \
	} \
	Lexer::Lexer(TFile* file, int tokenLookahead) : m_oDefines(10), m_oParseErrorEmitter(this) \
	{ \
		const int size = jpog::kernel::TokenRingSize(tokenLookahead); \
		LexerCore<Lexer>::Initialise(*this, file, tokenLookahead, size, jpog::kernel::TokenRingMask(size)); \
		SetInputStream(m_pFile); \
		Define(SkuName); \
	} \
	Lexer::~Lexer() \
	{ \
		if (m_piCharLookahead != nullptr) \
			tfree(m_piCharLookahead); \
		delete[] m_pTokenLookahead; \
	} \
	TClass& Lexer::GetClass() const \
	{ \
		return m_sClass; \
	} \
	void Lexer::SetInputStream(TFile* file) \
	{ \
		LexerCore<Lexer>::SetInputStream(*this, file); \
	} \
	void Lexer::SetCharacterLookaheadSize(int size) \
	{ \
		LexerCore<Lexer>::SetCharacterLookaheadSize(*this, size); \
	} \
	void Lexer::SetOutputComments(bool output) \
	{ \
		m_bOutputComments = output; \
	} \
	void Lexer::Define(const Char* name) \
	{ \
		LexerCore<Lexer>::Define(*this, name); \
	} \
	void Lexer::Undef(const Char* name) \
	{ \
		LexerCore<Lexer>::Undef(*this, name); \
	} \
	bool Lexer::IfDef(const Char* name) \
	{ \
		return LexerCore<Lexer>::IfDef(*this, name); \
	} \
	Lexer::Token Lexer::GetNextToken() \
	{ \
		return LexerCore<Lexer>::GetNextToken(*this); \
	} \
	Lexer::Token Lexer::PeekNextToken(int index) \
	{ \
		return LexerCore<Lexer>::PeekNextToken(*this, index); \
	} \
	Lexer::Token Lexer::GetLastToken() \
	{ \
		return m_oLastToken; \
	} \
	bool Lexer::Expect(TFileLexer::TokenType type) \
	{ \
		return GetNextToken().GetType() == type; \
	} \
	bool Lexer::Expect(TFileLexer::TokenType type, Token& token) \
	{ \
		token.assign(GetNextToken()); \
		return type == token.GetType(); \
	} \
	TEmitter<Lexer, Lexer::ParseError>* Lexer::GetParseErrorEmitter() \
	{ \
		return &m_oParseErrorEmitter; \
	} \
	const char* TSTDCALL Lexer::tostring(TFileLexer::TokenType type) \
	{ \
		return jpog::kernel::TokenName(type); \
	} \
	Lexer::Token Lexer::get_next_token() \
	{ \
		return LexerCore<Lexer>::NextToken(*this); \
	} \
	void Lexer::skipWhiteSpace() \
	{ \
		LexerCore<Lexer>::SkipWhiteSpace(*this); \
	} \
	void Lexer::fillLookAhead() \
	{ \
		LexerCore<Lexer>::FillLookAhead(*this); \
	} \
	void Lexer::advance() \
	{ \
		LexerCore<Lexer>::Advance(*this); \
	} \
	void Lexer::advance(int count) \
	{ \
		LexerCore<Lexer>::Advance(*this, count); \
	} \
	int Lexer::peek() \
	{ \
		return LexerCore<Lexer>::Peek(*this); \
	} \
	int Lexer::peek(int offset) \
	{ \
		return LexerCore<Lexer>::Peek(*this, offset); \
	} \
	void Lexer::ThrowError(const char* message) \
	{ \
		LexerCore<Lexer>::Error(*this, message); \
	} \
	bool Lexer::ComputePreprocessorAllow() \
	{ \
		return LexerCore<Lexer>::ComputePreprocessorAllow(*this); \
	} \
	TObject* TSTDCALL Lexer::CreateObject() \
	{ \
		return new Lexer(); \
	} \
	TObject* TSTDCALL Lexer::CreateObjectInPlace(void* memory) \
	{ \
		return memory == nullptr ? nullptr : new (memory) Lexer(); \
	} \
	void TSTDCALL Lexer::InitialiseStatic() \
	{ \
	} \
	void TSTDCALL Lexer::DeinitialiseStatic() \
	{ \
	}

namespace
{

const unsigned short kSkuWindows[] = {'T', 'O', 'S', 'H', 'I', '_', 'S', 'K', 'U',
                                      '_', 'W', 'I', 'N', 'D', 'O', 'W', 'S', 0};

} // namespace

JPOG_DEFINE_LEXER(TFileLexerUTF8, TPCString, "TOSHI_SKU_WINDOWS")
JPOG_DEFINE_LEXER(TFileLexerUTF16, TPWString, kSkuWindows)

#undef JPOG_DEFINE_LEXER

} // namespace Toshi
