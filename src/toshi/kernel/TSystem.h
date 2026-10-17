#pragma once

#include "common/abi.h"
#include "common/layout.h"

namespace Toshi
{

class TCString;
class TCStringPool;
class TLocale;
class TWStringPool;

// A calendar time (TKernelInterface.dll). The month is 0-based (struct tm's tm_mon).
struct TDateTime
{
	TDateTime& operator=(const TDateTime& other);

	// date = year << 16 | month << 8 | day, time = hour << 24 | minute << 16 | second << 8.
	void Pack(unsigned int& date, unsigned int& time);
	void UnPack(unsigned int& date, unsigned int& time);

	unsigned short m_usYear;
	unsigned char m_ucMonth;
	unsigned char m_ucHour;
	unsigned char m_ucMinute;
	unsigned char m_ucSecond;
	unsigned char m_ucDay;
};

JPOG_ASSERT_SIZE(TDateTime, 8);

// System helpers (TKernelInterface.dll): memory and string primitives. Wide strings are UTF-16
// code units (`unsigned short`, as in the original exports). A count of -1 means "whole string".

struct TSystem
{
	TSystem& operator=(const TSystem& other);

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

	// The global string pools, created on first use. The explicit variants only take effect
	// before the first use.
	static TCStringPool* TSTDCALL GetCStringPool();
	static TCStringPool* TSTDCALL CreateCStringPoolExplicit(int growSize, int capacity);
	static TCStringPool* TSTDCALL CreateCStringPoolExplicit(const char* fileName, int growSize, int capacity);
	static TWStringPool* TSTDCALL GetWStringPool();
	// One shared 1 KB scratch buffer behind all three.
	static unsigned char* TSTDCALL GetScratchMem();
	static char* TSTDCALL GetTempCString();
	static unsigned short* TSTDCALL GetTempWString();
	static bool TSTDCALL GetLocalTime(TDateTime& dateTime);
	// "hh:mm:ss day/month/year".
	static void TSTDCALL StringFormatDateTime(const TDateTime& dateTime, TCString& string);

	// The global string table, created on first use.
	static TLocale* TSTDCALL GetLocale();

private:
	static TCStringPool* ms_poTCStringPool;
	static TWStringPool* ms_poTWStringPool;
	static TLocale* ms_poLocale;
	static unsigned char ms_aScratchMem[0x400];
};

} // namespace Toshi
