#pragma once

#include "common/abi.h"

namespace Toshi
{

class TFile;

// Debug output (TKernelInterface.dll). Most of it is stubbed out in the shipped build; only the
// debug file and the message level remain.
struct TDebug
{
	enum MSGLEVEL
	{
		MSGLEVEL_0,
		MSGLEVEL_1,
		MSGLEVEL_2,
		MSGLEVEL_3,
	};

	TDebug& operator=(const TDebug& other);

	// Opens (mode 0xC: create, write) and enables the debug file, closing any previous one.
	static bool TSTDCALL OpenDebugFile(char* fileName);
	static void TSTDCALL CloseDebugFile();
	// Returns the previous state.
	static bool TSTDCALL EnableDebugFile(bool enable);
	static bool TSTDCALL IsDebugFileEnabled();
	static TFile* TSTDCALL GetDebugFile();
	// To the debug file when it is open and enabled (the text is used as the format there),
	// otherwise to stdout.
	static void TSTDCALL DebugFilePrintString(char* text);
	static bool TSTDCALL IsValidAddress(const void* address);
	static void TSTDCALL PrintIndent();
	static void TSTDCALL ResetOutput();

	static bool m_bEnableDebugFile;
	static TFile* m_pDebugFile;
	static MSGLEVEL s_iMessageLevel;
};

// Prints to stdout when `level` is at least the message level.
void TDebug_Message(TDebug::MSGLEVEL level, const char* format, ...);

// Stubs in the shipped build.
void TSTDCALL GetCallStack(int depth, int* addresses);
void TSTDCALL DumpCallStack(int depth, int* addresses, int skip);
void TSTDCALL DumpCallStackToFile(TFile* file, int depth, int* addresses, int skip);

} // namespace Toshi
