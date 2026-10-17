#pragma once

#include "common/abi.h"

namespace Toshi
{

// System helpers (TKernelInterface.dll): memory and string primitives. Wide strings are UTF-16
// code units (`unsigned short`, as in the original exports). A count of -1 means "whole string".
// String pools, locale, time and scratch memory follow with their own classes.
struct TSystem
{
	static void* TSTDCALL MemCopy(void* destination, const void* source, unsigned int size);
	static void* TSTDCALL MemSet(void* destination, int value, int size);

	static int TSTDCALL StringLength(const char* string);
	static int TSTDCALL StringLength(const unsigned short* string);
	// With a count, strncpy semantics (pads with zeros, may not terminate).
	static const char* TSTDCALL StringCopy(char* destination, const char* source, int count);
	static const unsigned short* TSTDCALL StringCopy(unsigned short* destination, const unsigned short* source,
	                                                 int count);
	// The narrow whole-string compare returns exactly -1, 0 or 1; the others return the C runtime's
	// result, whose sign is what matters.
	static int TSTDCALL StringCompare(const char* a, const char* b, int count);
	static int TSTDCALL StringCompare(const unsigned short* a, const unsigned short* b, int count);
	static int TSTDCALL StringCompareNoCase(const char* a, const char* b, int count);
	static int TSTDCALL StringCompareNoCase(const unsigned short* a, const unsigned short* b, int count);
	static const char* TSTDCALL StringFindChar(const char* string, char character);
	static const unsigned short* TSTDCALL StringFindChar(const unsigned short* string, unsigned short character);
	// Byte <-> code unit conversion without any code page (zero-extends / truncates), terminated.
	static const unsigned short* TSTDCALL StringCharToUnicode(unsigned short* destination, const char* source,
	                                                          int count);
	static const char* TSTDCALL StringUnicodeToChar(char* destination, const unsigned short* source, int count);
	// Radix 8, 10 or 16; any other radix leaves `buffer` untouched.
	static const char* TSTDCALL StringIntToString(int value, char* buffer, int radix);
	static int __cdecl StringFormat(char* buffer, const char* format, ...);
	static int __cdecl StringFormat(unsigned short* buffer, const unsigned short* format, ...);

	// hash = hash * 5 + character (narrow characters are signed).
	static unsigned int TSTDCALL HashString(const char* string);
	static unsigned int TSTDCALL HashString(const unsigned short* string);
	// Packed BCD (one decimal digit per nibble) to binary.
	static unsigned int TSTDCALL BCDToInteger(unsigned int bcd);
};

} // namespace Toshi
