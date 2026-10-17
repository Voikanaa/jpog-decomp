#include "toshi/kernel/TCStringPool.h"

#include "toshi/kernel/TFile.h"
#include "toshi/kernel/TMemory.h"
#include "toshi/kernel/TSystem.h"

namespace Toshi
{

using Entry = jpog::kernel::PoolEntry<char, TPooledCString>;

TFreeList TPooledCString::ms_oFreelist(sizeof(TPooledCString), 0, 8, nullptr);
const TCString TPCString::ms_sEmpty;

using jpog::kernel::AllocEntries;
using jpog::kernel::FindFirstEntry;
using jpog::kernel::FreeEntries;
using jpog::kernel::InsertEntrySlots;
using jpog::kernel::RemoveEntry;
using jpog::kernel::SearchEntries;

// --- TPooledCString ------------------------------------------------------------------------

void* TSTDCALL TPooledCString::operator new(std::size_t size)
{
	return ms_oFreelist.New(static_cast<unsigned int>(size));
}

void* TSTDCALL TPooledCString::operator new(std::size_t size, char*, int)
{
	return ms_oFreelist.New(static_cast<unsigned int>(size));
}

void* TSTDCALL TPooledCString::operator new(std::size_t, void* place)
{
	return place;
}

void TSTDCALL TPooledCString::operator delete(void* memory)
{
	ms_oFreelist.Delete(memory);
}

TFreeList& TSTDCALL TPooledCString::GetFreeList()
{
	return ms_oFreelist;
}

TPooledCString::TPooledCString() : m_iRefCount(0), m_pPool(nullptr)
{
}

TPooledCString::TPooledCString(const char* string, TCStringPool* pool)
    : m_iRefCount(0), m_oString(string), m_pPool(pool)
{
}

TPooledCString::~TPooledCString()
{
	if (m_pPool != nullptr)
		m_pPool->Remove(*this);
}

// The destructor and the free list release written out, as in the original.
void TPooledCString::Delete()
{
	if (m_pPool != nullptr)
		m_pPool->Remove(*this);
	m_oString.~TCString();
	ms_oFreelist.Delete(this);
}

// --- TPCString -----------------------------------------------------------------------------

TPCString::TPCString() : m_pString(nullptr)
{
}

TPCString::TPCString(TPooledCString* string) : m_pString(string)
{
	if (string != nullptr)
		++string->m_iRefCount;
}

TPCString::TPCString(const TPCString& other) : m_pString(other.m_pString)
{
	if (m_pString != nullptr)
		++m_pString->m_iRefCount;
}

TPCString::~TPCString()
{
	if (m_pString != nullptr)
	{
		if (--m_pString->m_iRefCount == 0)
			m_pString->Delete();
		m_pString = nullptr;
	}
}

TPCString& TPCString::operator=(const TPCString& other)
{
	if (m_pString != other.m_pString)
	{
		if (m_pString != nullptr && --m_pString->m_iRefCount == 0)
			m_pString->Delete();
		m_pString = other.m_pString;
		if (m_pString != nullptr)
			++m_pString->m_iRefCount;
	}
	return *this;
}

int TPCString::Compare(const TPCString& other) const
{
	if (m_pString == nullptr)
		return other.m_pString != nullptr ? -1 : 0;
	if (other.m_pString == nullptr)
		return 1;
	if (m_pString == other.m_pString)
		return 0;
	return m_pString->m_oString.Compare(other.m_pString->m_oString.GetString(0), -1);
}

bool TPCString::IsEmpty() const
{
	return m_pString == nullptr || m_pString->m_oString.IsEmpty();
}

const TCString& TPCString::GetCString() const
{
	return m_pString == nullptr ? ms_sEmpty : m_pString->m_oString;
}

TCString& TPCString::GetVolatileCString() const
{
	return m_pString->m_oString;
}

TCStringPool* TPCString::GetStringPool() const
{
	return m_pString->m_pPool;
}

bool TPCString::operator==(const TPCString& other) const
{
	return m_pString == other.m_pString;
}

bool TPCString::operator!=(const TPCString& other) const
{
	return m_pString != other.m_pString;
}

bool TPCString::operator<(const TPCString& other) const
{
	return Compare(other) < 0;
}

TPCString::operator const TCString*() const
{
	return &GetCString();
}

const TCString* TPCString::operator->() const
{
	return &GetCString();
}

const TCString& TPCString::operator*() const
{
	return GetCString();
}

// --- TCStringPool --------------------------------------------------------------------------

TCStringPool::TCStringPool(int growSize, int capacity)
    : m_iGrowSize(growSize), m_iCount(0), m_iCapacity(capacity),
      m_pEntries(capacity < 1 ? nullptr : AllocEntries<Entry>(capacity))
{
}

TCStringPool::TCStringPool(const char* fileName, int growSize, int capacity)
    : m_iGrowSize(growSize), m_iCount(0), m_iCapacity(capacity),
      m_pEntries(capacity < 1 ? nullptr : AllocEntries<Entry>(capacity))
{
	ReadFile(fileName);
}

TCStringPool::TCStringPool(const TCStringPool& other) : m_pEntries(nullptr)
{
	*this = other;
}

// Quirk kept from the original: removing an entry shifts the array while the loop moves on, so
// every other pooled string is skipped (and stays allocated).
TCStringPool::~TCStringPool()
{
	int index = m_iCount == 0 ? -1 : 0;
	while (index != -1)
	{
		TPooledCString* string = m_pEntries[index].string;
		if (string != nullptr)
		{
			if (string->m_pPool != nullptr)
				string->m_pPool->Remove(*string);
			string->m_oString.~TCString();
			TPooledCString::ms_oFreelist.Delete(string);
		}
		++index;
		if (m_iCount <= index || index == 0)
			index = -1;
	}
	if (m_pEntries != nullptr)
		FreeEntries(m_pEntries);
}

// A shallow copy: the entries (and their pooled strings) are shared, capacity becomes the count.
TCStringPool& TCStringPool::operator=(const TCStringPool& other)
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

TPCString TCStringPool::Get(const char* string)
{
	if (string == nullptr || TSystem::StringLength(string) == 0)
		return TPCString();
	bool found;
	const int index = SearchEntries(m_pEntries, m_iCount, string, found);
	if (found)
		return TPCString(m_pEntries[index - 1].string);
	auto* pooled = new TPooledCString(string, this);
	InsertEntrySlots(m_iGrowSize, m_iCount, m_iCapacity, m_pEntries, index, 1);
	m_pEntries[index] = {pooled->m_oString.GetString(0), pooled};
	return TPCString(pooled);
}

TPCString TCStringPool::Get(const TCString& string)
{
	return Get(string.GetString(0));
}

TPCString TCStringPool::Get(int value)
{
	TCString text;
	text.Format("%d", value);
	return Get(text);
}

TPCString TCStringPool::GetOptional(const char* string)
{
	if (string == nullptr || TSystem::StringLength(string) == 0)
		return TPCString();
	bool found;
	const int index = SearchEntries(m_pEntries, m_iCount, string, found);
	return found ? TPCString(m_pEntries[index - 1].string) : TPCString();
}

TPCString TCStringPool::GetOptional(const TCString& string)
{
	return GetOptional(string.GetString(0));
}

TPCString TCStringPool::GetOptional(int value)
{
	TCString text;
	text.Format("%d", value);
	return GetOptional(text);
}

TPCString TCStringPool::GetVolatile(const char* string)
{
	return TPCString(new TPooledCString(string, nullptr));
}

TPCString TCStringPool::GetVolatile(const TCString& string)
{
	return GetVolatile(string.GetString(0));
}

TPCString TCStringPool::GetVolatile(int value)
{
	TCString text;
	text.Format("%d", value);
	return GetVolatile(text);
}

bool TCStringPool::IsPresent(const char* string)
{
	if (string == nullptr || TSystem::StringLength(string) == 0)
		return false;
	bool found;
	SearchEntries(m_pEntries, m_iCount, string, found);
	return found;
}

bool TCStringPool::IsPresent(const TCString& string)
{
	return IsPresent(string.GetString(0));
}

bool TCStringPool::IsPresent(int value)
{
	TCString text;
	text.Format("%d", value);
	return IsPresent(text);
}

// Empty in the shipped original.
void TCStringPool::Dump() const
{
}

// Shifts the entries after the string's slot down by one. As in the original there is no check
// that the string is present.
bool TCStringPool::WriteFile(const char* fileName)
{
	TFile* file = TFileManager::GetFileManager()->CreateFile(fileName, 2 | 8);
	if (file == nullptr)
		return false;
	file->Write(&m_iCount, 4);
	int size = 0;
	for (int i = 0; i < m_iCount; ++i)
		size += m_pEntries[i].string->m_oString.Length() + 1;
	file->Write(&size, 4);
	for (int i = 0; i < m_iCount; ++i)
	{
		const TCString& string = m_pEntries[i].string->m_oString;
		file->Write(string.GetString(0), string.Length() + 1);
	}
	TFileManager::GetFileManager()->DestroyFile(file);
	return true;
}

bool TCStringPool::ReadFile(const char* fileName)
{
	TFile* file = TFileManager::GetFileManager()->CreateFile(fileName, 1);
	if (file == nullptr)
		return false;
	int count = 0, size = 0;
	file->Read(&count, 4);
	file->Read(&size, 4);
	if (count == 0 || size == 0)
		return false;
	auto* data = static_cast<char*>(tmalloc(size, nullptr, -1));
	const int read = file->Read(data, size);
	TFileManager::GetFileManager()->DestroyFile(file);
	if (read != size)
	{
		tfree(data);
		return false;
	}
	if (count > m_iCapacity)
	{
		Entry* entries = AllocEntries<Entry>(count);
		for (int i = 0; i < m_iCount; ++i)
			entries[i] = m_pEntries[i];
		if (m_pEntries != nullptr)
			FreeEntries(m_pEntries);
		m_pEntries = entries;
		m_iCapacity = count;
	}
	m_iCount = count;
	auto* strings = new TPooledCString[static_cast<std::size_t>(count)];
	int offset = 0;
	for (int i = 0; i < count; ++i)
	{
		char* string = data + offset;
		int end = offset;
		while (data[end] != '\0')
			++end;
		m_pEntries[i] = {string, &strings[i]};
		strings[i].m_oString.ForceSetData(string, end - offset);
		strings[i].m_pPool = this;
		++strings[i].m_iRefCount;
		offset = end + 1;
	}
	return true;
}

void TCStringPool::Remove(TPooledCString& string)
{
	RemoveEntry(m_iCount, m_pEntries, FindFirstEntry(m_pEntries, m_iCount, string.m_oString.GetString(0)));
}

TCString TSTDCALL operator+(const TCString& string, const TPCString& other)
{
	TCString result(string);
	result += other.GetCString();
	return result;
}

} // namespace Toshi
