#include "toshi/kernel/TWStringPool.h"

#include "toshi/kernel/TMemory.h"
#include "toshi/kernel/TSystem.h"

namespace Toshi
{

using Entry = jpog::kernel::PoolEntry<unsigned short, TPooledWString>;
using jpog::kernel::AllocEntries;
using jpog::kernel::FindFirstEntry;
using jpog::kernel::FreeEntries;
using jpog::kernel::InsertEntrySlots;
using jpog::kernel::RemoveEntry;
using jpog::kernel::SearchEntries;

TFreeList TPooledWString::ms_oFreelist(sizeof(TPooledWString), 0, 8, nullptr);
const TWString TPWString::ms_sEmpty;

namespace
{

// L"%d" and L"[%4d] 0x%p %s\n" as UTF-16.
const unsigned short kDecimalFormat[] = {'%', 'd', 0};
const unsigned short kDumpFormat[] = {'[', '%', '4', 'd', ']', ' ', '0', 'x', '%', 'p', ' ', '%', 's', '\n', 0};

} // namespace

// --- TPooledWString ------------------------------------------------------------------------

void* TSTDCALL TPooledWString::operator new(std::size_t size)
{
	return ms_oFreelist.New(static_cast<unsigned int>(size));
}

void* TSTDCALL TPooledWString::operator new(std::size_t size, char*, int)
{
	return ms_oFreelist.New(static_cast<unsigned int>(size));
}

void* TSTDCALL TPooledWString::operator new(std::size_t, void* place)
{
	return place;
}

void TSTDCALL TPooledWString::operator delete(void* memory)
{
	ms_oFreelist.Delete(memory);
}

TFreeList& TSTDCALL TPooledWString::GetFreeList()
{
	return ms_oFreelist;
}

TPooledWString::TPooledWString() : m_iRefCount(0), m_pPool(nullptr)
{
}

TPooledWString::TPooledWString(const unsigned short* string, TWStringPool* pool)
    : m_iRefCount(0), m_oString(string), m_pPool(pool)
{
}

TPooledWString::~TPooledWString()
{
	if (m_pPool != nullptr)
		m_pPool->Remove(*this);
}

// The destructor and the free list release written out, as in the original.
void TPooledWString::Delete()
{
	if (m_pPool != nullptr)
		m_pPool->Remove(*this);
	m_oString.~TWString();
	ms_oFreelist.Delete(this);
}

// --- TWStringPool --------------------------------------------------------------------------

TWStringPool::TWStringPool() : m_iGrowSize(0x80), m_iCount(0), m_iCapacity(4000), m_pEntries(AllocEntries<Entry>(4000))
{
}

TWStringPool::TWStringPool(const TWStringPool& other) : m_pEntries(nullptr)
{
	*this = other;
}

// Quirk kept from the original: removing an entry shifts the array while the loop moves on, so
// every other pooled string is skipped (and stays allocated).
TWStringPool::~TWStringPool()
{
	int index = m_iCount == 0 ? -1 : 0;
	while (index != -1)
	{
		TPooledWString* string = m_pEntries[index].string;
		if (string != nullptr)
		{
			if (string->m_pPool != nullptr)
				string->m_pPool->Remove(*string);
			string->m_oString.~TWString();
			TPooledWString::ms_oFreelist.Delete(string);
		}
		++index;
		if (m_iCount <= index || index == 0)
			index = -1;
	}
	if (m_pEntries != nullptr)
		FreeEntries(m_pEntries);
}

// A shallow copy: the entries (and their pooled strings) are shared, capacity becomes the count.
TWStringPool& TWStringPool::operator=(const TWStringPool& other)
{
	if (m_pEntries != nullptr)
		FreeEntries(m_pEntries);
	m_iCount = other.m_iCount;
	m_iGrowSize = other.m_iGrowSize;
	m_iCapacity = other.m_iCount;
	m_pEntries = AllocEntries<Entry>(m_iCapacity);
	for (int i = 0; i < m_iCount; ++i)
		m_pEntries[i] = other.m_pEntries[i];
	return *this;
}

TPWString TWStringPool::Get(const unsigned short* string)
{
	if (string == nullptr || TSystem::StringLength(string) == 0)
		return TPWString();
	bool found;
	const int index = SearchEntries(m_pEntries, m_iCount, string, found);
	if (found)
		return TPWString(m_pEntries[index - 1].string);
	auto* pooled = new TPooledWString(string, this);
	InsertEntrySlots(m_iGrowSize, m_iCount, m_iCapacity, m_pEntries, index, 1);
	m_pEntries[index] = {pooled->m_oString.GetString(0), pooled};
	return TPWString(pooled);
}

TPWString TWStringPool::Get(const TWString& string)
{
	return Get(string.GetString(0));
}

TPWString TWStringPool::Get(int value)
{
	TWString text;
	text.Format(kDecimalFormat, value);
	return Get(text);
}

TPWString TWStringPool::GetVolatile(const unsigned short* string)
{
	return TPWString(new TPooledWString(string, nullptr));
}

TPWString TWStringPool::GetVolatile(const TWString& string)
{
	return GetVolatile(string.GetString(0));
}

TPWString TWStringPool::GetVolatile(int value)
{
	TWString text;
	text.Format(kDecimalFormat, value);
	return GetVolatile(text);
}

void TWStringPool::Dump() const
{
	for (int i = 0; i < m_iCount; ++i)
	{
		TWString line;
		const unsigned short* string = m_pEntries[i].string->m_oString.GetString(0);
		line.Format(kDumpFormat, i, string, string).Print();
	}
}

// Dumps the pool when the string is missing; the removal then proceeds unchecked, as in the
// original.
void TWStringPool::Remove(TPooledWString& string)
{
	const int index = FindFirstEntry(m_pEntries, m_iCount, string.m_oString.GetString(0));
	if (index == -1)
		Dump();
	RemoveEntry(m_iCount, m_pEntries, index);
}

} // namespace Toshi
