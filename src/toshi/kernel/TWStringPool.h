#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "toshi/kernel/StringPoolArray.h"
#include "toshi/kernel/TFreeList.h"
#include "toshi/kernel/TWString.h"

#include <cstddef>

namespace Toshi
{

class TWStringPool;

// The UTF-16 counterpart of TPooledCString (TKernelInterface.dll).
class TPooledWString
{
	friend class TWStringPool;
	friend class TPWString;

public:
	static void* TSTDCALL operator new(std::size_t size);
	static void* TSTDCALL operator new(std::size_t size, char* file, int line);
	static void* TSTDCALL operator new(std::size_t size, void* place);
	static void TSTDCALL operator delete(void* memory);
	static TFreeList& TSTDCALL GetFreeList();

private:
	TPooledWString();
	TPooledWString(const unsigned short* string, TWStringPool* pool);
	~TPooledWString();
	void Delete();

	static TFreeList ms_oFreelist;

	int m_iRefCount;
	TWString m_oString;
	TWStringPool* m_pPool; // null for volatile strings
};

JPOG_ASSERT_SIZE(TPooledWString, 16);

// Handle to a pooled wide string. The original defines it entirely inline (only ms_sEmpty is
// exported), so it stays header-only here; tools/coverage.py knows its members are not exports.
class TPWString
{
	friend class TWStringPool;

public:
	TPWString() : m_pString(nullptr) {}
	TPWString(const TPWString& other) : m_pString(other.m_pString)
	{
		if (m_pString != nullptr)
			++m_pString->m_iRefCount;
	}
	~TPWString()
	{
		if (m_pString != nullptr && --m_pString->m_iRefCount == 0)
			m_pString->Delete();
		m_pString = nullptr;
	}
	TPWString& operator=(const TPWString& other)
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

	int Compare(const TPWString& other) const
	{
		if (m_pString == nullptr)
			return other.m_pString != nullptr ? -1 : 0;
		if (other.m_pString == nullptr)
			return 1;
		if (m_pString == other.m_pString)
			return 0;
		return m_pString->m_oString.Compare(other.m_pString->m_oString.GetString(0), -1);
	}
	bool IsEmpty() const { return m_pString == nullptr || m_pString->m_oString.IsEmpty(); }
	const TWString& GetWString() const { return m_pString == nullptr ? ms_sEmpty : m_pString->m_oString; }
	TWStringPool* GetStringPool() const { return m_pString->m_pPool; }

	bool operator==(const TPWString& other) const { return m_pString == other.m_pString; }
	bool operator!=(const TPWString& other) const { return m_pString != other.m_pString; }
	bool operator<(const TPWString& other) const { return Compare(other) < 0; }
	const TWString* operator->() const { return &GetWString(); }
	const TWString& operator*() const { return GetWString(); }

	static const TWString ms_sEmpty;

private:
	explicit TPWString(TPooledWString* string) : m_pString(string)
	{
		if (string != nullptr)
			++string->m_iRefCount;
	}

	TPooledWString* m_pString;
};

JPOG_ASSERT_SIZE(TPWString, 4);

// Interns wide strings, like TCStringPool (without the optional lookups).
class TWStringPool
{
	friend class TPooledWString;

public:
	TWStringPool();
	TWStringPool(const TWStringPool& other);
	~TWStringPool();
	TWStringPool& operator=(const TWStringPool& other);

	TPWString Get(const unsigned short* string);
	TPWString Get(const TWString& string);
	TPWString Get(int value);
	TPWString GetVolatile(const unsigned short* string);
	TPWString GetVolatile(const TWString& string);
	TPWString GetVolatile(int value);
	// Formats one line per entry and passes it to TWString::Print (which is empty when shipped).
	void Dump() const;

protected:
	void Remove(TPooledWString& string);

private:
	int m_iGrowSize;
	int m_iCount;
	int m_iCapacity;
	jpog::kernel::PoolEntry<unsigned short, TPooledWString>* m_pEntries; // sorted by content
};

JPOG_ASSERT_SIZE(TWStringPool, 16);

} // namespace Toshi
