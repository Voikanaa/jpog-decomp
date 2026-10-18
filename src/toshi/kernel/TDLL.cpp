#include "toshi/kernel/TDLL.h"

#include <windows.h>

namespace Toshi
{

// --- TDLL ----------------------------------------------------------------------------------

TDLL::TDLL() : m_hModule(nullptr)
{
}

TDLL::TDLL(const TDLL& other) : m_oFileName(other.m_oFileName), m_hModule(other.m_hModule)
{
}

TDLL::~TDLL()
{
	Unload();
}

TDLL& TDLL::operator=(TDLL& other)
{
	m_oFileName = other.m_oFileName;
	m_hModule = other.m_hModule;
	return *this;
}

bool TDLL::Load(const TCString& fileName)
{
	Unload();
	m_oFileName = static_cast<const char*>(fileName);
	m_hModule = LoadLibraryA(fileName.GetString(0));
	return m_hModule != nullptr;
}

void TDLL::Unload()
{
	if (m_hModule == nullptr)
		return;
	FreeLibrary(static_cast<HMODULE>(m_hModule));
	m_oFileName = "";
	m_hModule = nullptr;
}

const void* TDLL::GetDLL()
{
	return m_hModule;
}

const TCString& TDLL::GetFilename()
{
	return m_oFileName;
}

void* TDLL::GetAddress(const TCString& name)
{
	return reinterpret_cast<void*>(
	    GetProcAddress(static_cast<HMODULE>(const_cast<void*>(GetDLL())), static_cast<const char*>(name)));
}

// --- TKernelInterfaceDLL -------------------------------------------------------------------

TKernelInterfaceDLL::TKernelInterfaceDLL() : m_pList(nullptr), m_pNext(this), m_pPrev(this)
{
}

TKernelInterfaceDLL::TKernelInterfaceDLL(const TKernelInterfaceDLL& other)
    : TDLL(other), m_pList(other.m_pList), m_pNext(other.m_pNext), m_pPrev(other.m_pPrev)
{
}

TKernelInterfaceDLL::~TKernelInterfaceDLL()
{
}

TKernelInterfaceDLL& TKernelInterfaceDLL::operator=(TKernelInterfaceDLL& other)
{
	TDLL::operator=(other);
	m_pList = other.m_pList;
	m_pNext = other.m_pNext;
	m_pPrev = other.m_pPrev;
	return *this;
}

} // namespace Toshi
