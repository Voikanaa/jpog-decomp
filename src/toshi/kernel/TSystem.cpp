#include "toshi/kernel/TSystem.h"

#include "toshi/kernel/TCStringPool.h"
#include "toshi/kernel/TWStringPool.h"

#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <string.h> // _stricmp, _strnicmp
#include <wchar.h>  // _wcsicmp, _wcsnicmp

namespace Toshi
{

static_assert(sizeof(wchar_t) == sizeof(unsigned short), "wide strings are UTF-16 on Windows");

namespace
{

const wchar_t* Wide(const unsigned short* string)
{
	return reinterpret_cast<const wchar_t*>(string);
}

wchar_t* Wide(unsigned short* string)
{
	return reinterpret_cast<wchar_t*>(string);
}

} // namespace

void* TSTDCALL TSystem::MemCopy(void* destination, const void* source, unsigned int size)
{
	auto* out = static_cast<unsigned char*>(destination);
	const auto* in = static_cast<const unsigned char*>(source);
	for (; size != 0; --size)
		*out++ = *in++;
	return destination;
}

void* TSTDCALL TSystem::MemSet(void* destination, int value, int size)
{
	auto* out = static_cast<unsigned char*>(destination);
	for (; size != 0; --size)
		*out++ = static_cast<unsigned char>(value);
	return destination;
}

int TSTDCALL TSystem::StringLength(const char* string)
{
	int length = 0;
	while (string[length] != '\0')
		++length;
	return length;
}

int TSTDCALL TSystem::StringLength(const unsigned short* string)
{
	return static_cast<int>(std::wcslen(Wide(string)));
}

const char* TSTDCALL TSystem::StringCopy(char* destination, const char* source, int count)
{
	if (count != -1)
		return std::strncpy(destination, source, static_cast<size_t>(count));
	char* out = destination;
	while ((*out++ = *source++) != '\0')
	{
	}
	return destination;
}

const unsigned short* TSTDCALL TSystem::StringCopy(unsigned short* destination, const unsigned short* source, int count)
{
	if (count != -1)
		std::wcsncpy(Wide(destination), Wide(source), static_cast<size_t>(count));
	else
		std::wcscpy(Wide(destination), Wide(source));
	return destination;
}

int TSTDCALL TSystem::StringCompare(const char* a, const char* b, int count)
{
	if (count != -1)
		return std::strncmp(a, b, static_cast<size_t>(count));
	for (;; ++a, ++b)
	{
		const auto ca = static_cast<unsigned char>(*a);
		const auto cb = static_cast<unsigned char>(*b);
		if (ca != cb)
			return ca < cb ? -1 : 1;
		if (ca == 0)
			return 0;
	}
}

int TSTDCALL TSystem::StringCompare(const unsigned short* a, const unsigned short* b, int count)
{
	if (count != -1)
		return std::wcsncmp(Wide(a), Wide(b), static_cast<size_t>(count));
	return std::wcscmp(Wide(a), Wide(b));
}

int TSTDCALL TSystem::StringCompareNoCase(const char* a, const char* b, int count)
{
	if (count != -1)
		return _strnicmp(a, b, static_cast<size_t>(count));
	return _stricmp(a, b);
}

int TSTDCALL TSystem::StringCompareNoCase(const unsigned short* a, const unsigned short* b, int count)
{
	if (count != -1)
		return _wcsnicmp(Wide(a), Wide(b), static_cast<size_t>(count));
	return _wcsicmp(Wide(a), Wide(b));
}

const char* TSTDCALL TSystem::StringFindChar(const char* string, char character)
{
	for (; *string != '\0'; ++string)
		if (*string == character)
			return string;
	return nullptr;
}

const unsigned short* TSTDCALL TSystem::StringFindChar(const unsigned short* string, unsigned short character)
{
	for (; *string != 0; ++string)
		if (*string == character)
			return string;
	return nullptr;
}

const unsigned short* TSTDCALL TSystem::StringCharToUnicode(unsigned short* destination, const char* source, int count)
{
	const int length = StringLength(source);
	if (length < count || count == -1)
		count = length;
	int i = 0;
	for (; i < count; ++i)
		destination[i] = static_cast<unsigned char>(source[i]);
	destination[i] = 0;
	return destination;
}

const char* TSTDCALL TSystem::StringUnicodeToChar(char* destination, const unsigned short* source, int count)
{
	const int length = StringLength(source);
	if (length < count || count == -1)
		count = length;
	int i = 0;
	for (; i < count; ++i)
		destination[i] = static_cast<char>(source[i]);
	destination[i] = '\0';
	return destination;
}

const char* TSTDCALL TSystem::StringIntToString(int value, char* buffer, int radix)
{
	const char* format = radix == 8 ? "%o" : radix == 10 ? "%d" : radix == 16 ? "%x" : nullptr;
	if (format != nullptr)
		std::snprintf(buffer, INT_MAX, format, value);
	return buffer;
}

// Unbounded, like the original's vsprintf/vswprintf: the caller provides a large enough buffer.
int __cdecl TSystem::StringFormat(char* buffer, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	const int result = std::vsnprintf(buffer, INT_MAX, format, args);
	va_end(args);
	return result;
}

int __cdecl TSystem::StringFormat(unsigned short* buffer, const unsigned short* format, ...)
{
	va_list args;
	va_start(args, format);
	const int result = std::vswprintf(Wide(buffer), INT_MAX, Wide(format), args);
	va_end(args);
	return result;
}

unsigned int TSTDCALL TSystem::HashString(const char* string)
{
	unsigned int hash = 0;
	for (; *string != '\0'; ++string)
		hash = hash * 5 + static_cast<unsigned int>(static_cast<int>(*string));
	return hash;
}

unsigned int TSTDCALL TSystem::HashString(const unsigned short* string)
{
	unsigned int hash = 0;
	for (; *string != 0; ++string)
		hash = hash * 5 + *string;
	return hash;
}

unsigned int TSTDCALL TSystem::BCDToInteger(unsigned int bcd)
{
	unsigned int result = 0;
	unsigned int scale = 1;
	for (; bcd != 0; bcd >>= 4)
	{
		result += (bcd & 0xF) * scale;
		scale *= 10;
	}
	return result;
}

TCStringPool* TSystem::ms_poTCStringPool = nullptr;
TWStringPool* TSystem::ms_poTWStringPool = nullptr;

TCStringPool* TSystem::GetCStringPool()
{
	if (ms_poTCStringPool == nullptr)
		ms_poTCStringPool = new TCStringPool(0x400, 0);
	return ms_poTCStringPool;
}

TCStringPool* TSystem::CreateCStringPoolExplicit(int growSize, int capacity)
{
	if (ms_poTCStringPool == nullptr)
		ms_poTCStringPool = new TCStringPool(growSize, capacity);
	return ms_poTCStringPool;
}

TCStringPool* TSystem::CreateCStringPoolExplicit(const char* fileName, int growSize, int capacity)
{
	if (ms_poTCStringPool == nullptr)
		ms_poTCStringPool = new TCStringPool(fileName, growSize, capacity);
	return ms_poTCStringPool;
}

TWStringPool* TSystem::GetWStringPool()
{
	if (ms_poTWStringPool == nullptr)
		ms_poTWStringPool = new TWStringPool();
	return ms_poTWStringPool;
}

} // namespace Toshi
