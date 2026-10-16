#pragma once

// Calling conventions of the original 32-bit code. On x86 our replacements must use the same
// convention as the functions they replace so they can be hooked in directly; on x64 there is a
// single calling convention and the macros expand to nothing.
#if defined(_M_IX86)
#define TSTDCALL __stdcall
#else
#define TSTDCALL
#endif
