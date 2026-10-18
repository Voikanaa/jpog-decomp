#pragma once

#include "common/abi.h"
#include "common/layout.h"

namespace Toshi
{

class TFile;
class TProfilerStarter;

// A performance counter sample; only the first value is filled (the hardware counters are not
// read in the shipped build).
struct COUNTER_STRUCT
{
	__int64 m_iPerformanceCounter;
	__int64 m_iCounter0;
	__int64 m_iCounter1;
};

enum TProfiler_Sorting
{
	TProfiler_SortNone,
	TProfiler_SortByTotal, // top-level profilers first
	TProfiler_SortBySelf,  // total minus child time
	TProfiler_SortByTimePerCall,
	TProfiler_SortByHits,
};

// A named timing counter (TKernelInterface.dll). TProfilerStarter scopes add their time to it;
// PrintSummary appends a report of all profilers to Profile.txt. Profilers form a global chain.
class TProfiler
{
	friend class TProfilerStarter;

public:
	TProfiler(char* name, char* file, int line);
	~TProfiler();
	TProfiler& operator=(const TProfiler& other);

	// Start/Stop time the profiler directly (and switch all starters on/off).
	void Start();
	void Stop();
	bool IsRunning();

	static void TSTDCALL PrintHeader();
	static void TSTDCALL PrintSummary();
	// Reports the profiler and, recursively, the profilers whose parent it is.
	static void TSTDCALL RecursiveReport(int id);
	static TProfiler* TSTDCALL GetProfiler(int id);
	static TProfiler* TSTDCALL GetProfilerWithParentID(int parentID);
	// Writes `text` to the summary file (the second argument is ignored, as in the original).
	static bool TSTDCALL Out(char* text, char* unused);
	static void TSTDCALL GetHardwareCounterValues(COUNTER_STRUCT& counters);
	static void TSTDCALL RestartPC();

	static TFile* m_pFile;
	static int m_sCurrentID;
	static int m_sOutput;
	static int m_sOutputFilter; // 1 no time, 4 no time per call, 8 no hits
	static char* m_sOutFile;
	static int m_sOutFileMaxSize;
	static bool m_sStartImmediately;
	static TProfiler_Sorting m_sSortBy;

protected:
	void Reset();
	void Report(int indent);

private:
	bool operator>(TProfiler& other);
	void Add(__int64& time, __int64& childTime);
	void AddCounterStructs(COUNTER_STRUCT& counters, COUNTER_STRUCT& childCounters);
	// The original divides by frequency * 1000.
	double ElapsedInMS();

	static TProfiler* m_sChainHead;
	static __int64 m_sFrequency;
	static __int64 m_sProgramTime;
	static bool m_sProfilerStopped;

	char m_szName[0x50];
	int m_iID;
	bool m_bUnused;
	__int64 m_iTotalTime;
	__int64 m_iChildTime;
	COUNTER_STRUCT m_oStart;
	COUNTER_STRUCT m_oCounters;
	COUNTER_STRUCT m_oChildCounters;
	__int64 m_iMaxTime;
	__int64 m_iMinTime;
	__int64 m_iHits;
	int m_iUnused;
	TProfiler* m_pNext;
	int m_iDepth; // active starters
	// The first parent seen is recorded as a marker (not a profiler), later ones as the
	// profiler, as the original does with the value 1.
	TProfiler* m_pParent;
	bool m_bReported;
	char* m_szFile;
	int m_iLine;
};

JPOG_ASSERT_SIZE(TProfiler, 0xE8);

// Times a scope for a profiler, separating time spent in nested profiled scopes.
class TProfilerStarter
{
	friend class TProfiler;

public:
	explicit TProfilerStarter(TProfiler* profiler);
	~TProfilerStarter();
	TProfilerStarter& operator=(const TProfilerStarter& other);

private:
	// The innermost enclosing starter of the same profiler.
	TProfilerStarter* RecursiveCallFrom();

	static TProfilerStarter* m_sLastActive;

	TProfiler* m_pProfiler;
	TProfilerStarter* m_pPrevious;
	COUNTER_STRUCT m_oStart;
	__int64 m_iChildTime;
	COUNTER_STRUCT m_oChildCounters;
};

JPOG_ASSERT_SIZE(TProfilerStarter, 0x40);

} // namespace Toshi
