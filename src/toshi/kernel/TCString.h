#pragma once

#include "common/layout.h"

#include <cstdint>

namespace Toshi
{

// 8-bit string (TKernelInterface.dll).
// Declaration only so far: matches the original exports, and in the x86 build the original DLL
// provides the implementation until this class is reimplemented.
class TCString
{
public:
	TCString();
	TCString(const char* string);
	~TCString();

private:
	char* m_pBuffer;
	uint32_t m_uiLengthAndExcess; // low 24 bits: length, high 8 bits: spare capacity
};

JPOG_ASSERT_SIZE(TCString, 8);

} // namespace Toshi
