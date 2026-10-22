#include "platform/CRuntime.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#if defined(_WIN32)
#include <cwchar>
#endif

namespace jpog::platform
{

namespace
{

template <typename Char> unsigned int AsciiLower(Char c)
{
	const unsigned int value = static_cast<std::make_unsigned_t<Char>>(c);
	return value >= 'A' && value <= 'Z' ? value + ('a' - 'A') : value;
}

#if defined(_WIN32)
static_assert(sizeof(wchar_t) == sizeof(unsigned short), "wide strings are UTF-16 on Windows");

const wchar_t* Wide(const unsigned short* string)
{
	return reinterpret_cast<const wchar_t*>(string);
}

wchar_t* Wide(unsigned short* string)
{
	return reinterpret_cast<wchar_t*>(string);
}
#endif

} // namespace

#if defined(_WIN32)

int StrICmp(const char* a, const char* b)
{
	return _stricmp(a, b);
}

int StrNICmp(const char* a, const char* b, std::size_t count)
{
	return _strnicmp(a, b, count);
}

std::size_t WcsLen(const unsigned short* string)
{
	return std::wcslen(Wide(string));
}

int WcsCmp(const unsigned short* a, const unsigned short* b)
{
	return std::wcscmp(Wide(a), Wide(b));
}

int WcsNCmp(const unsigned short* a, const unsigned short* b, std::size_t count)
{
	return std::wcsncmp(Wide(a), Wide(b), count);
}

int WcsICmp(const unsigned short* a, const unsigned short* b)
{
	return _wcsicmp(Wide(a), Wide(b));
}

int WcsNICmp(const unsigned short* a, const unsigned short* b, std::size_t count)
{
	return _wcsnicmp(Wide(a), Wide(b), count);
}

unsigned short* WcsCpy(unsigned short* destination, const unsigned short* source)
{
#pragma warning(suppress : 4996) // unbounded, as the original
	std::wcscpy(Wide(destination), Wide(source));
	return destination;
}

unsigned short* WcsNCpy(unsigned short* destination, const unsigned short* source, std::size_t count)
{
#pragma warning(suppress : 4996) // the original relies on wcsncpy's padding and missing terminator
	std::wcsncpy(Wide(destination), Wide(source), count);
	return destination;
}

const unsigned short* WcsChr(const unsigned short* string, unsigned short character)
{
	return reinterpret_cast<const unsigned short*>(std::wcschr(Wide(string), static_cast<wchar_t>(character)));
}

const unsigned short* WcsStr(const unsigned short* string, const unsigned short* substring)
{
	return reinterpret_cast<const unsigned short*>(std::wcsstr(Wide(string), Wide(substring)));
}

int VSNPrintf(char* buffer, std::size_t count, const char* format, va_list args)
{
#pragma warning(suppress : 4996) // _vsnprintf's truncation semantics are the point
	return _vsnprintf(buffer, count, format, args);
}

int VSNWPrintf(unsigned short* buffer, std::size_t count, const unsigned short* format, va_list args)
{
#pragma warning(suppress : 4996)
	return _vsnwprintf(Wide(buffer), count, Wide(format), args);
}

int VSWPrintf(unsigned short* buffer, std::size_t count, const unsigned short* format, va_list args)
{
	return std::vswprintf(Wide(buffer), count, Wide(format), args);
}

#else

int StrICmp(const char* a, const char* b)
{
	return StrNICmp(a, b, static_cast<std::size_t>(-1));
}

int StrNICmp(const char* a, const char* b, std::size_t count)
{
	for (; count != 0; --count, ++a, ++b)
	{
		const unsigned int ca = AsciiLower(*a);
		const unsigned int cb = AsciiLower(*b);
		if (ca != cb || ca == 0)
			return static_cast<int>(ca) - static_cast<int>(cb);
	}
	return 0;
}

std::size_t WcsLen(const unsigned short* string)
{
	std::size_t length = 0;
	while (string[length] != 0)
		++length;
	return length;
}

// MSVC's wcscmp returns -1/0/1; wcsncmp returns the difference.
int WcsCmp(const unsigned short* a, const unsigned short* b)
{
	const int difference = WcsNCmp(a, b, static_cast<std::size_t>(-1));
	return difference < 0 ? -1 : difference > 0 ? 1 : 0;
}

int WcsNCmp(const unsigned short* a, const unsigned short* b, std::size_t count)
{
	for (; count != 0; --count, ++a, ++b)
		if (*a != *b || *a == 0)
			return static_cast<int>(*a) - static_cast<int>(*b);
	return 0;
}

int WcsICmp(const unsigned short* a, const unsigned short* b)
{
	return WcsNICmp(a, b, static_cast<std::size_t>(-1));
}

int WcsNICmp(const unsigned short* a, const unsigned short* b, std::size_t count)
{
	for (; count != 0; --count, ++a, ++b)
	{
		const unsigned int ca = AsciiLower(*a);
		const unsigned int cb = AsciiLower(*b);
		if (ca != cb || ca == 0)
			return static_cast<int>(ca) - static_cast<int>(cb);
	}
	return 0;
}

unsigned short* WcsCpy(unsigned short* destination, const unsigned short* source)
{
	unsigned short* out = destination;
	while ((*out++ = *source++) != 0)
	{
	}
	return destination;
}

unsigned short* WcsNCpy(unsigned short* destination, const unsigned short* source, std::size_t count)
{
	std::size_t i = 0;
	for (; i < count && source[i] != 0; ++i)
		destination[i] = source[i];
	for (; i < count; ++i)
		destination[i] = 0;
	return destination;
}

const unsigned short* WcsChr(const unsigned short* string, unsigned short character)
{
	for (;; ++string)
	{
		if (*string == character)
			return string;
		if (*string == 0)
			return nullptr;
	}
}

const unsigned short* WcsStr(const unsigned short* string, const unsigned short* substring)
{
	if (*substring == 0)
		return string;
	for (; *string != 0; ++string)
	{
		std::size_t i = 0;
		while (substring[i] != 0 && string[i] == substring[i])
			++i;
		if (substring[i] == 0)
			return string;
	}
	return nullptr;
}

int VSNPrintf(char* buffer, std::size_t count, const char* format, va_list args)
{
	va_list copy;
	va_copy(copy, args);
	const int length = std::vsnprintf(nullptr, 0, format, copy);
	va_end(copy);
	if (length < 0)
		return -1;
	std::vector<char> text(static_cast<std::size_t>(length) + 1);
	std::vsnprintf(text.data(), text.size(), format, args);
	const std::size_t written = static_cast<std::size_t>(length) < count ? static_cast<std::size_t>(length) + 1 : count;
	std::memcpy(buffer, text.data(), written);
	return static_cast<std::size_t>(length) <= count ? length : -1;
}

int VSNWPrintf(unsigned short* buffer, std::size_t count, const unsigned short* format, va_list args)
{
	const int length = FormatWide(buffer, count, format, args);
	if (static_cast<std::size_t>(length) < count)
		buffer[length] = 0;
	return static_cast<std::size_t>(length) <= count ? length : -1;
}

int VSWPrintf(unsigned short* buffer, std::size_t count, const unsigned short* format, va_list args)
{
	if (count == 0)
		return -1;
	const int length = FormatWide(buffer, count - 1, format, args);
	if (static_cast<std::size_t>(length) >= count)
	{
		buffer[count - 1] = 0;
		return -1;
	}
	buffer[length] = 0;
	return length;
}

#endif

// --- FormatWide ------------------------------------------------------------------------------

namespace
{

class WideSink
{
public:
	WideSink(unsigned short* buffer, std::size_t count) : m_pBuffer(buffer), m_uCount(count) {}

	void Put(unsigned int character)
	{
		if (m_uLength < m_uCount)
			m_pBuffer[m_uLength] = static_cast<unsigned short>(character);
		++m_uLength;
	}
	void Repeat(unsigned int character, int times)
	{
		for (; times > 0; --times)
			Put(character);
	}
	int Length() const { return static_cast<int>(m_uLength); }

private:
	unsigned short* m_pBuffer;
	std::size_t m_uCount;
	std::size_t m_uLength = 0;
};

enum class Size
{
	Default,
	Char,  // hh
	Short, // h
	Long,  // l, w (32 bits, as on Windows)
	LongLong,
	Pointer, // I, z, t, j on 64-bit
};

struct Spec
{
	char flags[6] = {};
	int flagCount = 0;
	bool left = false;
	bool zero = false;
	int width = 0;
	int precision = -1;
	Size size = Size::Default;
};

// Pads and writes a converted field of `length` characters produced by `write`.
template <typename Write> void Field(WideSink& out, const Spec& spec, int length, Write write)
{
	const int padding = spec.width - length;
	if (!spec.left && !spec.zero)
		out.Repeat(' ', padding);
	if (!spec.left && spec.zero)
		out.Repeat('0', padding);
	write();
	if (spec.left)
		out.Repeat(' ', padding);
}

// Formats a numeric conversion with the C library (ASCII output) and widens it.
template <typename T> void Numeric(WideSink& out, const Spec& spec, const char* length, char conversion, T value)
{
	char format[32];
	int n = 0;
	format[n++] = '%';
	for (int i = 0; i < spec.flagCount; ++i)
		format[n++] = spec.flags[i];
	format[n++] = '*';
	format[n++] = '.';
	format[n++] = '*';
	for (; *length != '\0'; ++length)
		format[n++] = *length;
	format[n++] = conversion;
	format[n] = '\0';
	const int width = spec.left ? -spec.width : spec.width;
	const int size = std::snprintf(nullptr, 0, format, width, spec.precision, value);
	std::vector<char> text(static_cast<std::size_t>(size < 0 ? 0 : size) + 1);
	std::snprintf(text.data(), text.size(), format, width, spec.precision, value);
	for (int i = 0; i < size; ++i)
		out.Put(static_cast<unsigned char>(text[static_cast<std::size_t>(i)]));
}

} // namespace

int FormatWide(unsigned short* buffer, std::size_t count, const unsigned short* format, va_list args)
{
	WideSink out(buffer, count);
	va_list ap;
	va_copy(ap, args);
	while (*format != 0)
	{
		if (*format != '%')
		{
			out.Put(*format++);
			continue;
		}
		const unsigned short* start = format++;
		Spec spec;
		for (;; ++format)
		{
			const unsigned short c = *format;
			if (c != '-' && c != '+' && c != ' ' && c != '#' && c != '0')
				break;
			spec.left |= c == '-';
			spec.zero |= c == '0';
			if (spec.flagCount < 5)
				spec.flags[spec.flagCount++] = static_cast<char>(c);
		}
		if (*format == '*')
		{
			spec.width = va_arg(ap, int);
			if (spec.width < 0)
			{
				spec.left = true;
				spec.width = -spec.width;
			}
			++format;
		}
		else
			for (; *format >= '0' && *format <= '9'; ++format)
				spec.width = spec.width * 10 + (*format - '0');
		if (*format == '.')
		{
			++format;
			spec.precision = 0;
			if (*format == '*')
			{
				spec.precision = va_arg(ap, int);
				++format;
			}
			else
				for (; *format >= '0' && *format <= '9'; ++format)
					spec.precision = spec.precision * 10 + (*format - '0');
		}
		if (spec.left)
			spec.zero = false;

		// Length modifiers.
		bool wideArgument = false;
		bool narrowArgument = false;
		switch (*format)
		{
		case 'h':
			++format;
			narrowArgument = true;
			spec.size = Size::Short;
			if (*format == 'h')
			{
				++format;
				spec.size = Size::Char;
			}
			break;
		case 'l':
			++format;
			wideArgument = true;
			spec.size = Size::Long;
			if (*format == 'l')
			{
				++format;
				spec.size = Size::LongLong;
			}
			break;
		case 'w':
			++format;
			wideArgument = true;
			spec.size = Size::Long;
			break;
		case 'L':
			++format;
			break;
		case 'I':
			++format;
			if (format[0] == '6' && format[1] == '4')
			{
				format += 2;
				spec.size = Size::LongLong;
			}
			else if (format[0] == '3' && format[1] == '2')
			{
				format += 2;
				spec.size = Size::Long;
			}
			else
				spec.size = Size::Pointer;
			break;
		case 'j':
			++format;
			spec.size = Size::LongLong;
			break;
		case 'z':
		case 't':
			++format;
			spec.size = Size::Pointer;
			break;
		default:
			break;
		}

		const unsigned short conversion = *format;
		if (conversion == 0)
			break;
		++format;
		switch (conversion)
		{
		case '%':
			out.Put('%');
			break;
		case 'd':
		case 'i':
		{
			long long value;
			if (spec.size == Size::LongLong || (spec.size == Size::Pointer && sizeof(void*) == 8))
				value = va_arg(ap, long long);
			else
				value = va_arg(ap, int);
			if (spec.size == Size::Short)
				value = static_cast<short>(value);
			else if (spec.size == Size::Char)
				value = static_cast<signed char>(value);
			Numeric(out, spec, "ll", 'd', value);
			break;
		}
		case 'o':
		case 'u':
		case 'x':
		case 'X':
		{
			unsigned long long value;
			if (spec.size == Size::LongLong || (spec.size == Size::Pointer && sizeof(void*) == 8))
				value = va_arg(ap, unsigned long long);
			else
				value = va_arg(ap, unsigned int);
			if (spec.size == Size::Short)
				value = static_cast<unsigned short>(value);
			else if (spec.size == Size::Char)
				value = static_cast<unsigned char>(value);
			Numeric(out, spec, "ll", static_cast<char>(conversion), value);
			break;
		}
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
		case 'a':
		case 'A':
			Numeric(out, spec, "", static_cast<char>(conversion), va_arg(ap, double));
			break;
		case 'p':
		{
			// Printing the address needs its integer value.
			const auto address = reinterpret_cast<std::uintptr_t>(va_arg(ap, void*));
			Spec hex = spec;
			hex.flagCount = 0;
			hex.precision = static_cast<int>(sizeof(void*) * 2);
			Numeric(out, hex, "ll", 'X', static_cast<unsigned long long>(address));
			break;
		}
		case 'c':
		case 'C':
		{
			// %c is wide in the wide functions unless `h`; %C is narrow unless `l`/`w`.
			const bool narrow = conversion == 'c' ? narrowArgument : !wideArgument;
			const int value = va_arg(ap, int);
			const unsigned int character =
			    narrow ? static_cast<unsigned char>(value) : static_cast<unsigned short>(value);
			Field(out, spec, 1, [&] { out.Put(character); });
			break;
		}
		case 's':
		case 'S':
		{
			const bool narrow = conversion == 's' ? narrowArgument : !wideArgument;
			const void* argument = va_arg(ap, const void*);
			static const char kNull[] = "(null)";
			const bool isNull = argument == nullptr;
			const bool narrowText = narrow || isNull;
			const auto* narrowString = isNull ? kNull : static_cast<const char*>(argument);
			const auto* wideString = static_cast<const unsigned short*>(argument);
			int length = 0;
			if (narrowText)
				while ((spec.precision < 0 || length < spec.precision) && narrowString[length] != '\0')
					++length;
			else
				while ((spec.precision < 0 || length < spec.precision) && wideString[length] != 0)
					++length;
			Field(out, spec, length,
			      [&]
			      {
				      for (int i = 0; i < length; ++i)
					      out.Put(narrowText ? static_cast<unsigned char>(narrowString[i]) : wideString[i]);
			      });
			break;
		}
		case 'n':
			// Disabled in MSVC's runtime; consumes the argument and writes nothing.
			(void)va_arg(ap, int*);
			break;
		default:
			// Unknown conversions are copied as they are.
			for (const unsigned short* c = start; c != format; ++c)
				out.Put(*c);
			break;
		}
	}
	va_end(ap);
	return out.Length();
}

} // namespace jpog::platform
