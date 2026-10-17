#include "toshi/kernel/TWString.h"

#include "toshi/kernel/TCString.h"
#include "toshi/kernel/TMemory.h"
#include "toshi/kernel/TSystem.h"

#include <cstdarg>
#include <cstdio>
#include <cwchar>

namespace Toshi
{

unsigned short* TWString::m_aNull = nullptr;

namespace
{

int LengthOf(uint32_t lengthAndExcess)
{
	return static_cast<int>(lengthAndExcess & 0xFFFFFF);
}

void SetLength(uint32_t& lengthAndExcess, int length)
{
	lengthAndExcess = (lengthAndExcess & 0xFF000000) | (static_cast<uint32_t>(length) & 0xFFFFFF);
}

void SetExcess(uint32_t& lengthAndExcess, int excess)
{
	lengthAndExcess = (lengthAndExcess & 0x00FFFFFF) | (static_cast<uint32_t>(excess) << 24);
}

wchar_t* Wide(unsigned short* string)
{
	return reinterpret_cast<wchar_t*>(string);
}

const wchar_t* Wide(const unsigned short* string)
{
	return reinterpret_cast<const wchar_t*>(string);
}

// _wcslwr/_wcsupr in the "C" locale: only ASCII letters change.
void ChangeCase(unsigned short* string, unsigned short from, unsigned short to)
{
	for (; *string != 0; ++string)
		if (*string >= from && *string <= from + 25)
			*string = static_cast<unsigned short>(*string - from + to);
}

} // namespace

TWString::TWString()
{
	Reset();
	AllocBuffer(0, true);
}

TWString::TWString(int length)
{
	Reset();
	AllocBuffer(length, true);
}

TWString::TWString(const char* string)
{
	Reset();
	Copy(string, -1);
}

TWString::TWString(const unsigned short* string)
{
	Reset();
	Copy(string, -1);
}

TWString::TWString(const TCString& other)
{
	Reset();
	Copy(other, -1);
}

TWString::TWString(const TWString& other)
{
	Reset();
	Copy(other, -1);
}

TWString::~TWString()
{
	FreeBuffer();
}

TWString& TWString::operator=(TCString& other)
{
	Copy(other, -1);
	return *this;
}

TWString& TWString::operator=(TWString& other)
{
	Copy(other, -1);
	return *this;
}

TWString& TWString::operator=(const char* string)
{
	Copy(string, -1);
	return *this;
}

TWString& TWString::operator=(const unsigned short* string)
{
	Copy(string, -1);
	return *this;
}

void TWString::Copy(const TCString& other, int count)
{
	if (other.Length() < count || count == -1)
		count = other.Length();
	AllocBuffer(count, true);
	TSystem::StringCharToUnicode(m_pBuffer, other, count);
	m_pBuffer[count] = 0;
}

void TWString::Copy(const TWString& other, int count)
{
	if (count < 0 || static_cast<unsigned int>(other.Length()) < static_cast<unsigned int>(count))
		count = other.Length();
	AllocBuffer(count, true);
	std::wcsncpy(Wide(m_pBuffer), Wide(other.m_pBuffer), static_cast<size_t>(count));
	m_pBuffer[count] = 0;
}

void TWString::Copy(const char* string, int count)
{
	const int length = string == nullptr ? 0 : TSystem::StringLength(string);
	if (length < count || count == -1)
		count = length;
	AllocBuffer(count, true);
	TSystem::StringCharToUnicode(m_pBuffer, string, count);
	m_pBuffer[count] = 0;
}

void TWString::Copy(const unsigned short* string, int count)
{
	const int length = string == nullptr ? 0 : static_cast<int>(std::wcslen(Wide(string)));
	if (length < count || count == -1)
		count = length;
	AllocBuffer(count, true);
	std::wcsncpy(Wide(m_pBuffer), Wide(string), static_cast<size_t>(count));
	m_pBuffer[count] = 0;
}

TWString& TWString::Concat(const TWString& other, int count)
{
	if (other.Length() < count || count == -1)
		count = other.Length();
	const int length = Length();
	unsigned short* old = m_pBuffer;
	const bool allocated = AllocBuffer(length + count, false);
	if (allocated)
		TSystem::StringCopy(m_pBuffer, old, -1);
	TSystem::StringCopy(m_pBuffer + length, other.m_pBuffer, count);
	m_pBuffer[length + count] = 0;
	if (allocated && length != 0)
		tfree(old);
	return *this;
}

TWString& TWString::Concat(const unsigned short* string, int count)
{
	const int stringLength = TSystem::StringLength(string);
	if (stringLength < count || count == -1)
		count = stringLength;
	const int length = Length();
	unsigned short* old = m_pBuffer;
	const bool allocated = AllocBuffer(length + count, false);
	if (allocated)
		TSystem::StringCopy(m_pBuffer, old, -1);
	TSystem::StringCopy(m_pBuffer + length, string, count);
	m_pBuffer[length + count] = 0;
	if (allocated && length != 0)
		tfree(old);
	return *this;
}

TWString& __cdecl TWString::Format(const unsigned short* format, ...)
{
	va_list args;
	va_start(args, format);
	VFormat(format, args);
	va_end(args);
	return *this;
}

TWString& TWString::VFormat(const unsigned short* format, char* args)
{
	wchar_t buffer[0x400];
	std::vswprintf(buffer, 0x400, Wide(format), args);
	Copy(reinterpret_cast<const unsigned short*>(buffer), -1);
	return *this;
}

void TWString::Truncate(int length)
{
	const int current = Length();
	if (current < length)
		length = current;
	unsigned short* old = m_pBuffer;
	const bool allocated = AllocBuffer(length, false);
	if (allocated)
		TSystem::StringCopy(m_pBuffer, old, length);
	m_pBuffer[length] = 0;
	if (allocated && current != 0)
		tfree(old);
}

TWString TWString::Mid(int start, int count) const
{
	const int length = Length();
	if (start < 0)
		start = 0;
	else if (length <= start)
		return TWString();
	if (count < 0 || length < count + start)
		count = length - start;
	TWString part(count);
	std::wcsncpy(Wide(part.m_pBuffer), Wide(m_pBuffer + start), static_cast<size_t>(count));
	part.m_pBuffer[count] = 0;
	return TWString(part);
}

TWString& TWString::MakeLower()
{
	if (m_pBuffer != nullptr)
		ChangeCase(m_pBuffer, 'A', 'a');
	return *this;
}

TWString& TWString::MakeUpper()
{
	if (m_pBuffer != nullptr)
		ChangeCase(m_pBuffer, 'a', 'A');
	return *this;
}

int TWString::Compare(const unsigned short* string, int count) const
{
	return TSystem::StringCompare(GetString(0), string, count);
}

int TWString::CompareNoCase(const unsigned short* string, int count) const
{
	return TSystem::StringCompareNoCase(GetString(0), string, count);
}

int TWString::Find(unsigned short character, int start) const
{
	if (!IsIndexValid(start))
		return -1;
	const wchar_t* found = std::wcschr(Wide(GetString(0) + start), static_cast<wchar_t>(character));
	return found == nullptr ? -1 : static_cast<int>(found - Wide(GetString(0)));
}

int TWString::Find(const unsigned short* string, int start) const
{
	if (!IsIndexValid(start))
		return -1;
	const wchar_t* found = std::wcsstr(Wide(GetString(0) + start), Wide(string));
	return found == nullptr ? -1 : static_cast<int>(found - Wide(GetString(0)));
}

int TWString::FindReverse(unsigned short character, int start) const
{
	if (start == -1)
		start = Length();
	else if (!IsIndexValid(start))
		return -1;
	const unsigned short* string = GetString(0);
	for (; start >= 0; --start)
		if (string[start] == character)
			return start;
	return -1;
}

bool TWString::IsEmpty() const
{
	return Length() == 0;
}

bool TWString::IsIndexValid(int index) const
{
	return index <= Length() && index >= 0;
}

bool TWString::IsUnicode() const
{
	return true;
}

int TWString::Length() const
{
	return LengthOf(m_uiLengthAndExcess);
}

int TWString::ExcessLength() const
{
	return static_cast<int>(m_uiLengthAndExcess >> 24);
}

const unsigned short* TWString::GetString(int index) const
{
	return IsIndexValid(index) ? m_pBuffer + index : nullptr;
}

// Debug output; empty in the shipped original.
const TWString& TWString::Print() const
{
	return *this;
}

TWString TWString::operator+(const TWString& other) const
{
	TWString result(*this);
	result.Concat(other, -1);
	return result;
}

TWString TWString::operator+(const unsigned short* string) const
{
	TWString result(*this);
	result.Concat(string, -1);
	return result;
}

TWString& TWString::operator+=(const TWString& other)
{
	return Concat(other, -1);
}

TWString& TWString::operator+=(const unsigned short* string)
{
	return Concat(string, -1);
}

bool TWString::operator==(const TWString& other) const
{
	return Compare(other.m_pBuffer, -1) == 0;
}

bool TWString::operator==(const unsigned short* string) const
{
	return Compare(string, -1) == 0;
}

bool TWString::operator!=(const TWString& other) const
{
	return Compare(other.m_pBuffer, -1) != 0;
}

bool TWString::operator!=(const unsigned short* string) const
{
	return Compare(string, -1) != 0;
}

bool TWString::operator<(const TWString& other) const
{
	return Compare(other.m_pBuffer, -1) < 0;
}

bool TWString::operator<(const unsigned short* string) const
{
	return Compare(string, -1) < 0;
}

bool TWString::operator<=(const TWString& other) const
{
	return Compare(other.m_pBuffer, -1) <= 0;
}

bool TWString::operator<=(const unsigned short* string) const
{
	return Compare(string, -1) <= 0;
}

bool TWString::operator>(const TWString& other) const
{
	return Compare(other.m_pBuffer, -1) > 0;
}

bool TWString::operator>(const unsigned short* string) const
{
	return Compare(string, -1) > 0;
}

bool TWString::operator>=(const TWString& other) const
{
	return Compare(other.m_pBuffer, -1) >= 0;
}

bool TWString::operator>=(const unsigned short* string) const
{
	return Compare(string, -1) >= 0;
}

bool TWString::operator!() const
{
	return Length() == 0;
}

unsigned short& TWString::operator[](int index) const
{
	return m_pBuffer[index];
}

TWString::operator const unsigned short*() const
{
	return m_pBuffer;
}

bool TWString::AllocBuffer(int length, bool freeOld)
{
	const int current = Length();
	bool allocated = false;
	if (length != current)
	{
		if (length == 0)
		{
			if (freeOld)
				tfree(m_pBuffer);
			m_pBuffer = reinterpret_cast<unsigned short*>(&m_aNull);
			allocated = true;
			SetExcess(m_uiLengthAndExcess, 0);
		}
		else
		{
			const int spare = current - length + ExcessLength();
			if (spare < 0 || spare > 0xFF)
			{
				if (current != 0 && freeOld)
					tfree(m_pBuffer);
				m_pBuffer = static_cast<unsigned short*>(tmalloc(length * 2 + 2, nullptr, -1));
				allocated = true;
				SetExcess(m_uiLengthAndExcess, 0);
			}
			else
			{
				SetExcess(m_uiLengthAndExcess, spare);
			}
		}
		SetLength(m_uiLengthAndExcess, length);
	}
	if (freeOld)
		m_pBuffer[0] = 0;
	return allocated;
}

void TWString::FreeBuffer()
{
	if (Length() != 0)
	{
		tfree(m_pBuffer);
		m_pBuffer = nullptr;
	}
	Reset();
}

void TWString::Reset()
{
	m_pBuffer = reinterpret_cast<unsigned short*>(&m_aNull);
	m_uiLengthAndExcess = 0;
}

} // namespace Toshi
