#include "toshi/kernel/TProfiler.h"

#include "toshi/kernel/file/TFile.h"
#include "toshi/kernel/strings/TCString.h"

#include <windows.h>
#include <cstdio>
#include <cstring>

namespace Toshi
{

TFile* TProfiler::m_pFile = nullptr;
int TProfiler::m_sCurrentID = -1;
int TProfiler::m_sOutput = 3;
int TProfiler::m_sOutputFilter = 0;
char* TProfiler::m_sOutFile = const_cast<char*>("TProfile.txt");
int TProfiler::m_sOutFileMaxSize = 20000;
bool TProfiler::m_sStartImmediately = false;
TProfiler_Sorting TProfiler::m_sSortBy = TProfiler_SortNone;
TProfiler* TProfiler::m_sChainHead = nullptr;
__int64 TProfiler::m_sFrequency = 0;
__int64 TProfiler::m_sProgramTime = 0;
bool TProfiler::m_sProfilerStopped = true;
TProfilerStarter* TProfilerStarter::m_sLastActive = nullptr;

namespace
{

// Stands for the original's parent value 1 ("first parent seen").
unsigned char s_firstParentMarker;

TProfiler* FirstParentMarker()
{
	return reinterpret_cast<TProfiler*>(&s_firstParentMarker);
}

__int64 Counter()
{
	LARGE_INTEGER value;
	QueryPerformanceCounter(&value);
	return value.QuadPart;
}

__int64 Frequency()
{
	LARGE_INTEGER value;
	QueryPerformanceFrequency(&value);
	return value.QuadPart;
}

bool IsRealParent(const TProfiler* parent)
{
	return parent != nullptr && parent != FirstParentMarker();
}

constexpr __int64 kNoMinimum = 0x7FFFFFFFFFFFFFFF;

} // namespace

// --- TProfiler -----------------------------------------------------------------------------

TProfiler::TProfiler(char* name, char* file, int line) : m_bUnused(false), m_pParent(nullptr)
{
	if (name == nullptr)
		m_szName[0] = '\0';
	else
		std::strcpy(m_szName, name);
	Reset();
	m_pNext = m_sChainHead;
	m_iID = ++m_sCurrentID;
	m_sChainHead = this;
	m_iDepth = 0;
	m_iUnused = 0;
	m_szFile = file;
	m_iLine = line;
	m_sFrequency = Frequency();
	m_oCounters = {};
	m_oChildCounters = {};
}

TProfiler::~TProfiler()
{
}

TProfiler& TProfiler::operator=(const TProfiler& other) = default;

void TProfiler::Reset()
{
	m_iTotalTime = 0;
	m_iChildTime = 0;
	m_oStart = {};
	m_iMaxTime = 0;
	m_iMinTime = kNoMinimum;
	m_iHits = 0;
	m_bReported = false;
}

void TProfiler::Start()
{
	m_sProfilerStopped = false;
	m_oStart.m_iPerformanceCounter = Counter();
}

// The hardware counters stay zero here; the original subtracts the start from uninitialised
// values.
void TProfiler::Stop()
{
	m_sProfilerStopped = true;
	++m_iHits;
	COUNTER_STRUCT now = {};
	GetHardwareCounterValues(now);
	m_iTotalTime = m_iTotalTime + now.m_iPerformanceCounter - m_oStart.m_iPerformanceCounter;
	m_oCounters.m_iCounter0 = now.m_iCounter0 - m_oStart.m_iCounter0;
	m_oCounters.m_iCounter1 = now.m_iCounter1 - m_oStart.m_iCounter1;
}

bool TProfiler::IsRunning()
{
	return !m_sProfilerStopped;
}

void TProfiler::Add(__int64& time, __int64& childTime)
{
	m_iTotalTime += time;
	m_iChildTime += childTime;
	++m_iHits;
}

void TProfiler::AddCounterStructs(COUNTER_STRUCT& counters, COUNTER_STRUCT& childCounters)
{
	m_oCounters.m_iCounter0 += counters.m_iCounter0;
	m_oCounters.m_iCounter1 += counters.m_iCounter1;
	m_oChildCounters.m_iCounter0 += childCounters.m_iCounter0;
	m_oChildCounters.m_iCounter1 += childCounters.m_iCounter1;
}

double TProfiler::ElapsedInMS()
{
	return static_cast<double>(m_iTotalTime) / (static_cast<double>(m_sFrequency) * 1000.0);
}

// Reported profilers sort last; division by zero hits is not guarded, as in the original.
bool TProfiler::operator>(TProfiler& other)
{
	if (other.m_bReported)
		return true;
	if (m_bReported)
		return false;
	switch (m_sSortBy)
	{
	case TProfiler_SortByTotal:
		if (m_pParent != other.m_pParent)
			return !IsRealParent(m_pParent);
		return m_iTotalTime > other.m_iTotalTime;
	case TProfiler_SortBySelf:
		return m_iTotalTime - m_iChildTime > other.m_iTotalTime - other.m_iChildTime;
	case TProfiler_SortByHits:
		return m_iHits > other.m_iHits;
	case TProfiler_SortByTimePerCall:
		return m_iTotalTime / m_iHits > other.m_iTotalTime / other.m_iHits;
	default:
		return false;
	}
}

void TSTDCALL TProfiler::GetHardwareCounterValues(COUNTER_STRUCT& counters)
{
	counters.m_iPerformanceCounter = Counter();
}

void TSTDCALL TProfiler::RestartPC()
{
}

bool TSTDCALL TProfiler::Out(char* text, char*)
{
	if ((m_sOutput & 1) == 0)
		return true;
	if (m_pFile != nullptr)
		m_pFile->CPrintf("%s", text);
	return true;
}

void TSTDCALL TProfiler::PrintHeader()
{
	char empty[] = "";
	Out(const_cast<char*>("------------------------------------------------------\n"), empty);
	Out(const_cast<char*>("|-Child|Total "), empty);
	if ((m_sOutputFilter & 1) == 0)
		Out(const_cast<char*>("|Time (ms) "), nullptr);
	if ((m_sOutputFilter & 8) == 0)
		Out(const_cast<char*>("|  Hits  "), nullptr);
	if ((m_sOutputFilter & 4) == 0)
		Out(const_cast<char*>("|Time/call "), nullptr);
	Out(const_cast<char*>("|   MIN   |   MAX   "), nullptr);
	Out(const_cast<char*>("| Function    \n"), nullptr);
	Out(const_cast<char*>("------------------------------------------------------\n"), empty);
}

// Appends to Profile.txt (created if missing). The profilers are reported in sort order, then
// reset. The file pointer is left dangling after the file is destroyed, as in the original.
void TSTDCALL TProfiler::PrintSummary()
{
	m_sFrequency = Frequency();
	TCString name;
	name.Format("Profile.txt");
	TFile* file = TFile::Create(name, 2);
	if (file == nullptr)
		file = TFile::Create(name, 10);
	else
	{
		file->GetSize();
		file->Seek(0, TFile::TSEEK_END);
	}
	m_pFile = file;
	PrintHeader();
	for (;;)
	{
		TProfiler* best = m_sChainHead;
		for (TProfiler* profiler = m_sChainHead; profiler != nullptr; profiler = profiler->m_pNext)
			if (!(*best > *profiler))
				best = profiler;
		if (best == nullptr || best->m_bReported)
			break;
		best->Report(0);
	}
	for (TProfiler* profiler = m_sChainHead; profiler != nullptr; profiler = profiler->m_pNext)
		profiler->Reset();
	Out(const_cast<char*>("\n"), nullptr);
	if (file != nullptr)
		TFile::Destroy(file);
	TProfilerStarter::m_sLastActive = nullptr;
}

void TProfiler::Report(int indent)
{
	if (m_bReported)
		return;
	m_bReported = true;
	char line[0x100];
	char location[0x100];
	const double milliseconds = (static_cast<double>(m_iTotalTime) / static_cast<double>(m_sFrequency)) * 1000.0;
	// m_sProgramTime is never set, so the shares come out infinite or NaN, as in the original.
	const double total = static_cast<double>(m_iTotalTime) * (1.0 / static_cast<double>(m_sProgramTime)) * 100.0;
	const double self =
	    100.0 * static_cast<double>(m_iTotalTime - m_iChildTime) * (1.0 / static_cast<double>(m_sProgramTime));
	if (self == total)
		std::sprintf(line, m_sSortBy == TProfiler_SortByTotal ? "|      |%6.2f" : "|%6.2f|      ", self);
	else
		std::sprintf(line, "|%6.2f|%6.2f", self, total);
	if (m_szFile == nullptr)
		std::sprintf(location, " ");
	else
		std::sprintf(location, "%-*.*s(%3d) :", 0x50, 0x50, m_szFile, m_iLine);
	Out(line, location);
	if ((m_sOutputFilter & 1) == 0)
	{
		std::sprintf(line, "|%10.2f", milliseconds);
		Out(line, nullptr);
	}
	if ((m_sOutputFilter & 8) == 0)
	{
		std::sprintf(line, "|%8I64d", m_iHits);
		Out(line, nullptr);
	}
	if ((m_sOutputFilter & 4) == 0)
	{
		std::sprintf(line, "|%10.3f", m_iHits == 0 ? 0.0 : milliseconds / static_cast<double>(m_iHits));
		Out(line, nullptr);
	}
	Out(const_cast<char*>("|"), nullptr);
	if (m_iMinTime == kNoMinimum)
		m_iMinTime = 0;
	const float minimum = static_cast<float>(m_iMinTime) * (1.0f / static_cast<float>(m_sFrequency)) * 1000.0f;
	const float maximum = 1000.0f * static_cast<float>(m_iMaxTime) * (1.0f / static_cast<float>(m_sFrequency));
	std::sprintf(line, "%9.3f|%9.3f|", static_cast<double>(minimum), static_cast<double>(maximum));
	Out(line, nullptr);
	for (int i = 0; i < indent; ++i)
		Out(const_cast<char*>("  "), nullptr);
	std::sprintf(line, "ID:(%d):", m_iID);
	Out(line, nullptr);
	Out(m_szName, nullptr);
	if (IsRealParent(m_pParent))
	{
		const double share =
		    m_pParent->m_iTotalTime <= 0
		        ? 0.0
		        : (static_cast<double>(m_iTotalTime) / static_cast<double>(m_pParent->m_iTotalTime)) * 100.0;
		std::sprintf(line, "(%3.1f%%) PID:%d", share, m_pParent->m_iID);
		Out(line, nullptr);
	}
	Out(const_cast<char*>("\n"), nullptr);
}

// Unknown IDs and the first-parent marker are skipped (the original dereferences them).
void TSTDCALL TProfiler::RecursiveReport(int id)
{
	TProfiler* profiler = GetProfiler(id);
	if (profiler != nullptr && !profiler->m_bReported)
		profiler->Report(0);
	for (TProfiler* child = m_sChainHead; child != nullptr; child = child->m_pNext)
		if (IsRealParent(child->m_pParent) && id == child->m_pParent->m_iID)
			RecursiveReport(child->m_iID);
}

TProfiler* TSTDCALL TProfiler::GetProfiler(int id)
{
	for (TProfiler* profiler = m_sChainHead; profiler != nullptr; profiler = profiler->m_pNext)
		if (id == profiler->m_iID)
			return profiler;
	return nullptr;
}

TProfiler* TSTDCALL TProfiler::GetProfilerWithParentID(int parentID)
{
	for (TProfiler* profiler = m_sChainHead; profiler != nullptr; profiler = profiler->m_pNext)
		if (IsRealParent(profiler->m_pParent) && parentID == profiler->m_pParent->m_iID)
			return profiler;
	return nullptr;
}

// --- TProfilerStarter ----------------------------------------------------------------------

TProfilerStarter::TProfilerStarter(TProfiler* profiler) : m_pProfiler(profiler)
{
	if (TProfiler::m_sProfilerStopped)
		return;
	m_oStart = {};
	TProfiler::GetHardwareCounterValues(m_oStart);
	m_pPrevious = m_sLastActive;
	m_sLastActive = this;
	m_iChildTime = 0;
	m_oChildCounters = {};
	++profiler->m_iDepth;
}

// Adds the scope's time to its profiler (time inside a recursive call of the same profiler is
// credited once) and as child time to the enclosing scope.
TProfilerStarter::~TProfilerStarter()
{
	if (TProfiler::m_sProfilerStopped)
		return;
	COUNTER_STRUCT now = {};
	TProfiler::GetHardwareCounterValues(now);
	const __int64 elapsed = now.m_iPerformanceCounter - m_oStart.m_iPerformanceCounter;
	COUNTER_STRUCT counters = {0, now.m_iCounter0 - m_oStart.m_iCounter0, now.m_iCounter1 - m_oStart.m_iCounter1};
	m_sLastActive = m_pPrevious;
	TProfiler& profiler = *m_pProfiler;
	const int depth = profiler.m_iDepth;
	profiler.m_iDepth = depth - 1;
	if (elapsed > profiler.m_iMaxTime)
		profiler.m_iMaxTime = elapsed;
	if (elapsed < profiler.m_iMinTime)
		profiler.m_iMinTime = elapsed;

	auto addToProfiler = [&]
	{
		profiler.m_iTotalTime += elapsed;
		profiler.m_iChildTime += m_iChildTime;
		++profiler.m_iHits;
		profiler.m_oCounters.m_iCounter0 += counters.m_iCounter0;
		profiler.m_oCounters.m_iCounter1 += counters.m_iCounter1;
		profiler.m_oChildCounters.m_iCounter0 += m_oChildCounters.m_iCounter0;
		profiler.m_oChildCounters.m_iCounter1 += m_oChildCounters.m_iCounter1;
	};
	auto addChild = [](TProfilerStarter& to, __int64 time, const COUNTER_STRUCT& values)
	{
		to.m_iChildTime += time;
		to.m_oChildCounters.m_iCounter0 += values.m_iCounter0;
		to.m_oChildCounters.m_iCounter1 += values.m_iCounter1;
	};

	if (m_pPrevious == nullptr)
	{
		addToProfiler();
		return;
	}
	if (profiler.m_pParent == nullptr)
	{
		if (m_pPrevious->m_pProfiler != nullptr)
			profiler.m_pParent = FirstParentMarker();
	}
	else
		profiler.m_pParent = m_pPrevious->m_pProfiler;

	TProfilerStarter* recursive = nullptr;
	if (depth - 1 != 0 && depth > 0)
		recursive = RecursiveCallFrom();
	if (recursive != nullptr)
	{
		++profiler.m_iHits;
		if (recursive != m_pPrevious)
		{
			addChild(*m_pPrevious, elapsed, counters);
			const COUNTER_STRUCT own = {0, counters.m_iCounter0 - m_oChildCounters.m_iCounter0,
			                            counters.m_iCounter1 - m_oChildCounters.m_iCounter1};
			recursive->m_iChildTime -= elapsed - m_iChildTime;
			recursive->m_oChildCounters.m_iCounter0 -= own.m_iCounter0;
			recursive->m_oChildCounters.m_iCounter1 -= own.m_iCounter1;
			return;
		}
		addChild(*m_pPrevious, m_iChildTime, m_oChildCounters);
		return;
	}
	addToProfiler();
	addChild(*m_pPrevious, elapsed, counters);
}

TProfilerStarter& TProfilerStarter::operator=(const TProfilerStarter& other) = default;

TProfilerStarter* TProfilerStarter::RecursiveCallFrom()
{
	for (TProfilerStarter* starter = m_pPrevious; starter != nullptr; starter = starter->m_pPrevious)
		if (m_pProfiler == starter->m_pProfiler)
			return starter;
	return nullptr;
}

} // namespace Toshi
