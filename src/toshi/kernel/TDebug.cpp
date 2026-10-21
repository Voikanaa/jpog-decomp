#include "toshi/kernel/TDebug.h"

#include "toshi/kernel/file/TFile.h"
#include "toshi/kernel/strings/TCString.h"

#include <cstdarg>
#include <cstdio>

namespace Toshi
{

bool TDebug::m_bEnableDebugFile = false;
TFile* TDebug::m_pDebugFile = nullptr;
TDebug::MSGLEVEL TDebug::s_iMessageLevel = TDebug::MSGLEVEL_2;

TDebug& TDebug::operator=(const TDebug&)
{
	return *this;
}

// The original also formats the current date into a buffer it never uses.
bool TSTDCALL TDebug::OpenDebugFile(char* fileName)
{
	CloseDebugFile();
	m_pDebugFile = TFile::Create(TCString(fileName), 0xC);
	if (m_pDebugFile == nullptr)
		return false;
	EnableDebugFile(true);
	return true;
}

void TSTDCALL TDebug::CloseDebugFile()
{
	if (m_pDebugFile == nullptr)
		return;
	TFile::Destroy(m_pDebugFile);
	m_pDebugFile = nullptr;
	EnableDebugFile(false);
}

bool TSTDCALL TDebug::EnableDebugFile(bool enable)
{
	const bool previous = m_bEnableDebugFile;
	m_bEnableDebugFile = enable;
	return previous;
}

bool TSTDCALL TDebug::IsDebugFileEnabled()
{
	return m_bEnableDebugFile;
}

TFile* TSTDCALL TDebug::GetDebugFile()
{
	return m_pDebugFile;
}

void TSTDCALL TDebug::DebugFilePrintString(char* text)
{
	if (m_pDebugFile != nullptr && m_bEnableDebugFile)
	{
		m_pDebugFile->CPrintf(text);
		return;
	}
	std::printf("%s", text);
}

bool TSTDCALL TDebug::IsValidAddress(const void*)
{
	return true;
}

void TSTDCALL TDebug::PrintIndent()
{
}

void TSTDCALL TDebug::ResetOutput()
{
}

void TDebug_Message(TDebug::MSGLEVEL level, const char* format, ...)
{
	if (level < TDebug::s_iMessageLevel)
		return;
	va_list args;
	va_start(args, format);
	std::vprintf(format, args);
	va_end(args);
}

void TSTDCALL GetCallStack(int, int*)
{
}

void TSTDCALL DumpCallStack(int, int*, int)
{
}

void TSTDCALL DumpCallStackToFile(TFile*, int, int*, int)
{
}

} // namespace Toshi
