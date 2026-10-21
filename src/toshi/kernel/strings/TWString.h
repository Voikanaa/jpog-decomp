#pragma once

#include "common/abi.h"
#include "common/layout.h"

#include <cstdint>

namespace Toshi
{

class TCString;

// 16-bit string (TKernelInterface.dll), the UTF-16 counterpart of TCString with the same buffer
// scheme. Indices and counts are in code units; a count of -1 means "all".
class TWString
{
public:
	TWString();
	explicit TWString(int length);
	TWString(const char* string);
	TWString(const unsigned short* string);
	TWString(const TCString& other);
	TWString(const TWString& other);
	~TWString();

	// Non-const references as in the original. Copying a TWString onto itself clears its
	// characters but keeps the length: the original has no self-assignment check here.
	TWString& operator=(TCString& other);
	TWString& operator=(TWString& other);
	TWString& operator=(const char* string);
	TWString& operator=(const unsigned short* string);

	void Copy(const TCString& other, int count);
	void Copy(const TWString& other, int count);
	void Copy(const char* string, int count);
	void Copy(const unsigned short* string, int count);
	TWString& Concat(const TWString& other, int count);
	TWString& Concat(const unsigned short* string, int count);
	TWString& __cdecl Format(const unsigned short* format, ...);
	TWString& VFormat(const unsigned short* format, char* args);
	void Truncate(int length);
	TWString Mid(int start, int count) const;
	TWString& MakeLower();
	TWString& MakeUpper();

	int Compare(const unsigned short* string, int count) const;
	int CompareNoCase(const unsigned short* string, int count) const;
	int Find(unsigned short character, int start) const;
	int Find(const unsigned short* string, int start) const;
	int FindReverse(unsigned short character, int start) const;
	bool IsEmpty() const;
	bool IsIndexValid(int index) const;
	bool IsUnicode() const;
	int Length() const;
	int ExcessLength() const;
	const unsigned short* GetString(int index) const;
	const TWString& Print() const;

	TWString operator+(const TWString& other) const;
	TWString operator+(const unsigned short* string) const;
	TWString& operator+=(const TWString& other);
	TWString& operator+=(const unsigned short* string);
	bool operator==(const TWString& other) const;
	bool operator==(const unsigned short* string) const;
	bool operator!=(const TWString& other) const;
	bool operator!=(const unsigned short* string) const;
	bool operator<(const TWString& other) const;
	bool operator<(const unsigned short* string) const;
	bool operator<=(const TWString& other) const;
	bool operator<=(const unsigned short* string) const;
	bool operator>(const TWString& other) const;
	bool operator>(const unsigned short* string) const;
	bool operator>=(const TWString& other) const;
	bool operator>=(const unsigned short* string) const;
	bool operator!() const;
	// Const but returns a mutable reference, as in the original.
	unsigned short& operator[](int index) const;
	operator const unsigned short*() const;

private:
	bool AllocBuffer(int length, bool freeOld);
	void FreeBuffer();
	void Reset();

	static unsigned short* m_aNull; // its address is the shared empty string

	unsigned short* m_pBuffer;
	uint32_t m_uiLengthAndExcess; // low 24 bits: length, high 8 bits: spare capacity
};

JPOG_ASSERT_SIZE(TWString, 8);

TWString TSTDCALL operator+(const unsigned short* string, const TWString& other);
// The original compares `other` against `string`, so the ordering operators are reversed
// ("a" < x is x < "a").
bool TSTDCALL operator==(const unsigned short* string, const TWString& other);
bool TSTDCALL operator!=(const unsigned short* string, const TWString& other);
bool TSTDCALL operator<(const unsigned short* string, const TWString& other);
bool TSTDCALL operator<=(const unsigned short* string, const TWString& other);
bool TSTDCALL operator>(const unsigned short* string, const TWString& other);
bool TSTDCALL operator>=(const unsigned short* string, const TWString& other);

} // namespace Toshi
