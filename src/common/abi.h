#pragma once

// Calling conventions of the original 32-bit code. On x86 our replacements must use the same
// convention as the functions they replace so they can be hooked in directly; on x64 there is a
// single calling convention and the macros expand to nothing.
#if defined(_M_IX86)
#define TSTDCALL __stdcall
#else
#define TSTDCALL
#endif

// Data (not functions) that our x86 jpog.exe or PPropertyParser.dll imports from an engine DLL
// must be declared dllimport; functions go through import thunks. Only those modules define
// JPOG_IMPORT_ENGINE_DATA: in our own kernel the same declarations define the data.
#if defined(JPOG_IMPORT_ENGINE_DATA)
#define TIMPORT_DATA __declspec(dllimport)
#else
#define TIMPORT_DATA
#endif
