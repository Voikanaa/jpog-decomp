#include "toshi/kernel/strings/TCString.h"

#include "toshi/kernel/TSystem.h"
#include "toshi/kernel/memory/TMemory.h"
#include "toshi/kernel/strings/TWString.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <type_traits>

namespace Toshi
{

#if defined(_MSC_VER)
static_assert(std::is_same_v<va_list, char*>, "the original passes argument lists as char* (MSVC's va_list)");
#endif

char* TCString::m_aNull = nullptr;

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

// The C runtime's _strlwr/_strupr in the "C" locale: only ASCII letters change.
void ChangeCase(char* string, char from, char to)
{
	for (; *string != '\0'; ++string)
		if (*string >= from && *string <= static_cast<char>(from + 25))
			*string = static_cast<char>(*string - from + to);
}

} // namespace

TCString::TCString()
{
	Reset();
	AllocBuffer(0, true);
}

TCString::TCString(int length)
{
	Reset();
	AllocBuffer(length, true);
}

TCString::TCString(const char* string)
{
	Reset();
	Copy(string, -1);
}

TCString::TCString(const unsigned short* string)
{
	Reset();
	Copy(string, -1);
}

TCString::TCString(const TCString& other)
{
	Reset();
	Copy(other, -1);
}

TCString::TCString(const TWString& other)
{
	Reset();
	Copy(other, -1);
}

TCString::~TCString()
{
	FreeBuffer();
}

TCString& TCString::operator=(TCString& other)
{
	Copy(other, -1);
	return *this;
}

TCString& TCString::operator=(TWString& other)
{
	Copy(other, -1);
	return *this;
}

TCString& TCString::operator=(const char* string)
{
	Copy(string, -1);
	return *this;
}

TCString& TCString::operator=(const unsigned short* string)
{
	Copy(string, -1);
	return *this;
}

void TCString::Copy(const TCString& other, int count)
{
	if (this == &other)
		return;
	if (other.Length() < count || count == -1)
		count = other.Length();
	AllocBuffer(count, true);
	TSystem::MemCopy(m_pBuffer, other.m_pBuffer, static_cast<unsigned int>(count));
	m_pBuffer[count] = '\0';
}

void TCString::Copy(const TWString& other, int count)
{
	if (other.Length() < count || count == -1)
		count = other.Length();
	AllocBuffer(count, true);
	TSystem::StringUnicodeToChar(m_pBuffer, other, count);
	m_pBuffer[count] = '\0';
}

void TCString::Copy(const char* string, int count)
{
	if (string == m_pBuffer)
		return;
	const int length = string == nullptr ? 0 : TSystem::StringLength(string);
	if (length < count || count == -1)
		count = length;
	AllocBuffer(count, true);
	TSystem::MemCopy(m_pBuffer, string, static_cast<unsigned int>(count));
	m_pBuffer[count] = '\0';
}

void TCString::Copy(const unsigned short* string, int count)
{
	const int length = string == nullptr ? 0 : TSystem::StringLength(string);
	if (length < count || count == -1)
		count = length;
	AllocBuffer(count, true);
	TSystem::StringUnicodeToChar(m_pBuffer, string, count);
	m_pBuffer[count] = '\0';
}

TCString& TCString::Concat(const TCString& other, int count)
{
	if (other.Length() < count || count == -1)
		count = other.Length();
	const int length = Length();
	char* old = m_pBuffer;
	const bool allocated = AllocBuffer(length + count, false);
	if (allocated)
		TSystem::StringCopy(m_pBuffer, old, -1);
	TSystem::StringCopy(m_pBuffer + length, other.m_pBuffer, count);
	m_pBuffer[length + count] = '\0';
	if (allocated && length != 0)
		tfree(old);
	return *this;
}

TCString& TCString::Concat(const TWString& other, int count)
{
	if (other.Length() < count || count == -1)
		count = other.Length();
	const int length = Length();
	char* old = m_pBuffer;
	const bool allocated = AllocBuffer(length + count, false);
	if (allocated)
		TSystem::StringCopy(m_pBuffer, old, -1);
	TSystem::StringUnicodeToChar(m_pBuffer + length, other.GetString(0), count);
	m_pBuffer[length + count] = '\0';
	if (allocated && length != 0)
		tfree(old);
	return *this;
}

TCString& TCString::Concat(const char* string, int count)
{
	const int stringLength = TSystem::StringLength(string);
	if (stringLength < count || count == -1)
		count = stringLength;
	const int length = Length();
	char* old = m_pBuffer;
	const bool allocated = AllocBuffer(length + count, false);
	if (allocated)
		TSystem::StringCopy(m_pBuffer, old, -1);
	TSystem::StringCopy(m_pBuffer + length, string, count);
	m_pBuffer[length + count] = '\0';
	if (allocated && length != 0)
		tfree(old);
	return *this;
}

TCString& TCString::Format(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	VFormat(format, args);
	va_end(args);
	return *this;
}

TCString& TCString::VFormat(const char* format, va_list args)
{
	char buffer[0x400];
	std::vsnprintf(buffer, sizeof(buffer), format, args);
	Copy(buffer, -1);
	return *this;
}

void TCString::Truncate(int length)
{
	const int current = Length();
	if (current < length)
		length = current;
	char* old = m_pBuffer;
	const bool allocated = AllocBuffer(length, false);
	if (allocated)
		TSystem::StringCopy(m_pBuffer, old, length);
	m_pBuffer[length] = '\0';
	if (allocated && current != 0)
		tfree(old);
}

TCString TCString::Mid(int start, int count) const
{
	const int length = Length();
	if (start < 0)
		start = 0;
	else if (length <= start)
		return TCString();
	if (count < 0 || length < count + start)
		count = length - start;
	TCString part(count);
	TSystem::MemCopy(part.m_pBuffer, m_pBuffer + start, static_cast<unsigned int>(count));
	part.m_pBuffer[count] = '\0';
	return TCString(part);
}

TCString& TCString::MakeLower()
{
	if (m_pBuffer != nullptr)
		ChangeCase(m_pBuffer, 'A', 'a');
	return *this;
}

TCString& TCString::MakeUpper()
{
	if (m_pBuffer != nullptr)
		ChangeCase(m_pBuffer, 'a', 'A');
	return *this;
}

void TCString::ForceSetData(char* data, int length)
{
	m_pBuffer = data;
	SetLength(m_uiLengthAndExcess, length < 0 ? TSystem::StringLength(data) : length);
	SetExcess(m_uiLengthAndExcess, 0);
}

void TCString::UndoForceSetData()
{
	Reset();
}

int TCString::Compare(const char* string, int count) const
{
	return TSystem::StringCompare(GetString(0), string, count);
}

int TCString::CompareNoCase(const char* string, int count) const
{
	return TSystem::StringCompareNoCase(GetString(0), string, count);
}

int TCString::Find(char character, int start) const
{
	if (!IsIndexValid(start))
		return -1;
	const char* found = std::strchr(GetString(0) + start, character);
	return found == nullptr ? -1 : static_cast<int>(found - GetString(0));
}

int TCString::Find(const char* string, int start) const
{
	if (!IsIndexValid(start))
		return -1;
	const char* found = std::strstr(GetString(0) + start, string);
	return found == nullptr ? -1 : static_cast<int>(found - GetString(0));
}

int TCString::FindReverse(char character, int start) const
{
	if (start == -1)
		start = Length();
	else if (!IsIndexValid(start))
		return -1;
	const char* string = GetString(0);
	for (; start >= 0; --start)
		if (string[start] == character)
			return start;
	return -1;
}

bool TCString::IsAllLowerCase() const
{
	for (int i = 0; i < Length(); ++i)
		if (m_pBuffer[i] >= 'A' && m_pBuffer[i] <= 'Z')
			return false;
	return true;
}

bool TCString::IsAllUpperCase() const
{
	for (int i = 0; i < Length(); ++i)
		if (m_pBuffer[i] >= 'a' && m_pBuffer[i] <= 'z')
			return false;
	return true;
}

bool TCString::IsEmpty() const
{
	return Length() == 0;
}

bool TCString::IsIndexValid(int index) const
{
	return index <= Length() && index >= 0;
}

bool TCString::IsUnicode() const
{
	return false;
}

int TCString::Length() const
{
	return LengthOf(m_uiLengthAndExcess);
}

int TCString::ExcessLength() const
{
	return static_cast<int>(m_uiLengthAndExcess >> 24);
}

const char* TCString::GetString(int index) const
{
	return IsIndexValid(index) ? m_pBuffer + index : nullptr;
}

// Debug output; empty in the shipped original.
const TCString& TCString::Print() const
{
	return *this;
}

TCString TCString::operator+(const TCString& other) const
{
	TCString result(*this);
	result.Concat(other, -1);
	return result;
}

TCString TCString::operator+(const TWString& other) const
{
	TCString result(*this);
	result.Concat(other, -1);
	return result;
}

TCString TCString::operator+(const char* string) const
{
	TCString result(*this);
	const TCString suffix(string);
	result.Concat(suffix, -1);
	return result;
}

TCString TCString::operator+(const unsigned short* string) const
{
	TCString result(*this);
	const TCString suffix(string);
	result.Concat(suffix, -1);
	return result;
}

TCString& TCString::operator+=(const TCString& other)
{
	return Concat(other, -1);
}

TCString& TCString::operator+=(const TWString& other)
{
	return Concat(other, -1);
}

TCString& TCString::operator+=(const char* string)
{
	return Concat(string, -1);
}

bool TCString::operator==(const TCString& other) const
{
	return Compare(other.m_pBuffer, -1) == 0;
}

bool TCString::operator==(const char* string) const
{
	return Compare(string, -1) == 0;
}

bool TCString::operator!=(const TCString& other) const
{
	return Compare(other.m_pBuffer, -1) != 0;
}

bool TCString::operator!=(const char* string) const
{
	return Compare(string, -1) != 0;
}

bool TCString::operator<(const TCString& other) const
{
	return Compare(other.m_pBuffer, -1) < 0;
}

bool TCString::operator<(const char* string) const
{
	return Compare(string, -1) < 0;
}

bool TCString::operator<=(const TCString& other) const
{
	return Compare(other.m_pBuffer, -1) <= 0;
}

bool TCString::operator<=(const char* string) const
{
	return Compare(string, -1) <= 0;
}

bool TCString::operator>(const TCString& other) const
{
	return Compare(other.m_pBuffer, -1) > 0;
}

bool TCString::operator>(const char* string) const
{
	return Compare(string, -1) > 0;
}

bool TCString::operator>=(const TCString& other) const
{
	return Compare(other.m_pBuffer, -1) >= 0;
}

bool TCString::operator>=(const char* string) const
{
	return Compare(string, -1) >= 0;
}

bool TCString::operator!() const
{
	return Length() == 0;
}

char& TCString::operator[](int index)
{
	return m_pBuffer[index];
}

const char& TCString::operator[](int index) const
{
	return m_pBuffer[index];
}

TCString::operator const char*() const
{
	return m_pBuffer;
}

bool TCString::AllocBuffer(int length, bool freeOld)
{
	const int current = Length();
	bool allocated = false;
	if (length != current)
	{
		if (length == 0)
		{
			if (freeOld)
				tfree(m_pBuffer);
			m_pBuffer = reinterpret_cast<char*>(&m_aNull);
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
				m_pBuffer = static_cast<char*>(tmalloc(length + 1, nullptr, -1));
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
		m_pBuffer[0] = '\0';
	return allocated;
}

void TCString::FreeBuffer()
{
	if (Length() != 0)
	{
		tfree(m_pBuffer);
		m_pBuffer = nullptr;
	}
	Reset();
}

void TCString::Reset()
{
	m_pBuffer = reinterpret_cast<char*>(&m_aNull);
	m_uiLengthAndExcess = 0;
}

TCString TSTDCALL operator+(const char* string, const TCString& other)
{
	TCString result;
	result.Copy(string, -1);
	result.Concat(other, -1);
	return result;
}

bool TSTDCALL operator==(const char* string, const TCString& other)
{
	return other.Compare(string, -1) == 0;
}

bool TSTDCALL operator!=(const char* string, const TCString& other)
{
	return other.Compare(string, -1) != 0;
}

bool TSTDCALL operator<(const char* string, const TCString& other)
{
	return other.Compare(string, -1) < 0;
}

bool TSTDCALL operator<=(const char* string, const TCString& other)
{
	return other.Compare(string, -1) <= 0;
}

bool TSTDCALL operator>(const char* string, const TCString& other)
{
	return other.Compare(string, -1) > 0;
}

bool TSTDCALL operator>=(const char* string, const TCString& other)
{
	return other.Compare(string, -1) >= 0;
}

} // namespace Toshi
