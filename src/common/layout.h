#pragma once

#include <cstddef>

// Layout checks for classes shared with the original 32-bit binaries. They only
// apply to the x86 build, where our objects must be binary-compatible with the
// original code; the x64 build uses natural layout.
#if defined(_M_IX86)
#define JPOG_ASSERT_SIZE(type, size) static_assert(sizeof(type) == (size), #type " size mismatch")
#define JPOG_ASSERT_OFFSET(type, member, offset) \
	static_assert(offsetof(type, member) == (offset), #type "::" #member " offset mismatch")
#else
#define JPOG_ASSERT_SIZE(type, size)
#define JPOG_ASSERT_OFFSET(type, member, offset)
#endif

// For the few conversions that reproduce the original's layout tricks, such as a list root used
// as a pointer to an item it is not part of: the address is only stored and compared, never
// dereferenced as that type. UBSan's vptr check would flag the conversion itself.
#if defined(__clang__) || defined(__GNUC__)
#define JPOG_NO_SANITIZE_VPTR __attribute__((no_sanitize("vptr")))
#else
#define JPOG_NO_SANITIZE_VPTR
#endif
