#pragma once

#include "common/abi.h"
#include "common/layout.h"

#include <cstdarg>
#include <cstdint>

namespace Toshi
{

class TWString;

// 8-bit string (TKernelInterface.dll). The buffer comes from tmalloc and keeps up to 255
// characters of spare capacity, so shrinking (or growing back) does not reallocate. Empty
// strings point at a shared zero (m_aNull). Indices and counts are in characters; a count of -1
// means "all".
class TCString
{
public:
	TCString();
	// An uninitialised string of `length` characters (only the first is set to 0).
	explicit TCString(int length);
	TCString(const char* string);
	TCString(const unsigned short* string);
	TCString(const TCString& other);
	TCString(const TWString& other);
	~TCString();

	// The original takes non-const references here; kept for identical signatures.
	TCString& operator=(TCString& other);
	TCString& operator=(TWString& other);
	TCString& operator=(const char* string);
	TCString& operator=(const unsigned short* string);

	void Copy(const TCString& other, int count);
	void Copy(const TWString& other, int count);
	void Copy(const char* string, int count);
	void Copy(const unsigned short* string, int count);
	TCString& Concat(const TCString& other, int count);
	TCString& Concat(const TWString& other, int count);
	TCString& Concat(const char* string, int count);
	// Formats into a 1024-character buffer.
	TCString& Format(const char* format, ...);
	TCString& VFormat(const char* format, va_list args);
	void Truncate(int length);
	// Characters [start, start + count), clamped.
	TCString Mid(int start, int count) const;
	TCString& MakeLower();
	TCString& MakeUpper();
	// Points the string at external data without copying (UndoForceSetData before destruction).
	void ForceSetData(char* data, int length);
	void UndoForceSetData();

	int Compare(const char* string, int count) const;
	int CompareNoCase(const char* string, int count) const;
	int Find(char character, int start) const;
	int Find(const char* string, int start) const;
	int FindReverse(char character, int start) const;
	bool IsAllLowerCase() const;
	bool IsAllUpperCase() const;
	bool IsEmpty() const;
	bool IsIndexValid(int index) const;
	bool IsUnicode() const;
	int Length() const;
	int ExcessLength() const;
	// Pointer to character `index`, or null if out of range.
	const char* GetString(int index) const;
	const TCString& Print() const;

	TCString operator+(const TCString& other) const;
	TCString operator+(const TWString& other) const;
	TCString operator+(const char* string) const;
	TCString operator+(const unsigned short* string) const;
	TCString& operator+=(const TCString& other);
	TCString& operator+=(const TWString& other);
	TCString& operator+=(const char* string);
	bool operator==(const TCString& other) const;
	bool operator==(const char* string) const;
	bool operator!=(const TCString& other) const;
	bool operator!=(const char* string) const;
	bool operator<(const TCString& other) const;
	bool operator<(const char* string) const;
	bool operator<=(const TCString& other) const;
	bool operator<=(const char* string) const;
	bool operator>(const TCString& other) const;
	bool operator>(const char* string) const;
	bool operator>=(const TCString& other) const;
	bool operator>=(const char* string) const;
	bool operator!() const;
	char& operator[](int index);
	const char& operator[](int index) const;
	operator const char*() const;

private:
	// Resizes the buffer for `length` characters. Returns true if a new buffer was allocated;
	// with `freeOld` the old buffer is released and the string emptied, otherwise the caller
	// copies the old contents over and frees them.
	bool AllocBuffer(int length, bool freeOld);
	void FreeBuffer();
	void Reset();

	static char* m_aNull; // its address is the shared empty string

	char* m_pBuffer;
	uint32_t m_uiLengthAndExcess; // low 24 bits: length, high 8 bits: spare capacity
};

JPOG_ASSERT_SIZE(TCString, 8);

TCString TSTDCALL operator+(const char* string, const TCString& other);
// The original compares `other` against `string`, so the ordering operators are reversed
// ("a" < x is x < "a").
bool TSTDCALL operator==(const char* string, const TCString& other);
bool TSTDCALL operator!=(const char* string, const TCString& other);
bool TSTDCALL operator<(const char* string, const TCString& other);
bool TSTDCALL operator<=(const char* string, const TCString& other);
bool TSTDCALL operator>(const char* string, const TCString& other);
bool TSTDCALL operator>=(const char* string, const TCString& other);

} // namespace Toshi
