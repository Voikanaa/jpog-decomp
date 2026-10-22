#pragma once

#include <cstdarg>
#include <cstddef>

// The MSVC C runtime string functions the kernel relies on, with their results in the "C"
// locale. Windows forwards to the CRT; elsewhere they are reimplemented, since wchar_t is 32 bits
// there while the game's wide strings are UTF-16 (`unsigned short`).
namespace jpog::platform
{

// _stricmp/_strnicmp: compare ASCII-lowercased bytes, returning their difference.
int StrICmp(const char* a, const char* b);
int StrNICmp(const char* a, const char* b, std::size_t count);

std::size_t WcsLen(const unsigned short* string);
int WcsCmp(const unsigned short* a, const unsigned short* b);
int WcsNCmp(const unsigned short* a, const unsigned short* b, std::size_t count);
int WcsICmp(const unsigned short* a, const unsigned short* b);
int WcsNICmp(const unsigned short* a, const unsigned short* b, std::size_t count);
unsigned short* WcsCpy(unsigned short* destination, const unsigned short* source);
// Zero-pads up to `count`, with no terminator when the source is that long (wcsncpy).
unsigned short* WcsNCpy(unsigned short* destination, const unsigned short* source, std::size_t count);
const unsigned short* WcsChr(const unsigned short* string, unsigned short character);
const unsigned short* WcsStr(const unsigned short* string, const unsigned short* substring);

// _vsnprintf/_vsnwprintf: writes at most `count` characters, terminated only if there is room;
// returns the length, or -1 when the output was truncated.
int VSNPrintf(char* buffer, std::size_t count, const char* format, va_list args);
int VSNWPrintf(unsigned short* buffer, std::size_t count, const unsigned short* format, va_list args);
// vswprintf: always terminated; returns the length, or -1 when the output does not fit.
int VSWPrintf(unsigned short* buffer, std::size_t count, const unsigned short* format, va_list args);

// The portable wide formatter behind VSNWPrintf/VSWPrintf outside Windows, with MSVC's
// conversions: %s and %c take wide arguments, %hs/%S and %hc/%C narrow ones, `l` is 32 bits,
// I64/ll 64 bits, I/z/t pointer-sized, %p prints the address as zero-padded uppercase hex.
// Writes up to `count` characters without a terminator and returns the full length.
// (Exposed so the tests can compare it with the CRT on Windows.)
int FormatWide(unsigned short* buffer, std::size_t count, const unsigned short* format, va_list args);

} // namespace jpog::platform
