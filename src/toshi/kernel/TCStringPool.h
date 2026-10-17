#pragma once

#include "common/abi.h"
#include "common/layout.h"
#include "toshi/kernel/StringPoolArray.h"
#include "toshi/kernel/TCString.h"
#include "toshi/kernel/TFreeList.h"

#include <cstddef>

namespace Toshi
{
class TPooledCString;
}

namespace Toshi
{

class TCStringPool;

// A reference-counted string owned by a TCStringPool (TKernelInterface.dll). Allocated from a
// TFreeList; deleted when the last TPCString releases it, which also removes it from its pool.
class TPooledCString
{
	friend class TCStringPool;
	friend class TPCString;

public:
	static void* TSTDCALL operator new(std::size_t size);
	static void* TSTDCALL operator new(std::size_t size, char* file, int line);
	static void* TSTDCALL operator new(std::size_t size, void* place);
	static void TSTDCALL operator delete(void* memory);
	static TFreeList& TSTDCALL GetFreeList();

private:
	TPooledCString();
	TPooledCString(const char* string, TCStringPool* pool);
	~TPooledCString();
	// Removes the string from its pool, destroys it and returns its memory to the free list.
	void Delete();

	static TFreeList ms_oFreelist;

	int m_iRefCount;
	TCString m_oString;
	TCStringPool* m_pPool; // null for volatile (unpooled) strings
};

JPOG_ASSERT_SIZE(TPooledCString, 16);

// Handle to a pooled string. Equal pooled strings share one TPooledCString, so equality is
// pointer equality; a null handle reads as the empty string.
class TPCString
{
	friend class TCStringPool;

public:
	TPCString();
	TPCString(const TPCString& other);
	~TPCString();
	TPCString& operator=(const TPCString& other);

	// Orders by string content (null first); equal handles compare equal without a string compare.
	int Compare(const TPCString& other) const;
	bool IsEmpty() const;
	const TCString& GetCString() const;
	// The string itself; the handle must not be null.
	TCString& GetVolatileCString() const;
	// The owning pool; the handle must not be null.
	TCStringPool* GetStringPool() const;

	bool operator==(const TPCString& other) const;
	bool operator!=(const TPCString& other) const;
	bool operator<(const TPCString& other) const;
	operator const TCString*() const;
	const TCString* operator->() const;
	const TCString& operator*() const;

	static const TCString ms_sEmpty;

private:
	TPCString(TPooledCString* string);

	TPooledCString* m_pString;
};

JPOG_ASSERT_SIZE(TPCString, 4);

// Interns strings: Get returns the shared TPooledCString for equal contents. Entries are kept in
// an array sorted by content and grow by `growSize` entries at a time.
class TCStringPool
{
	friend class TPooledCString;

public:
	TCStringPool(int growSize = 0x400, int capacity = 0);
	TCStringPool(const TCStringPool& other);
	~TCStringPool();
	TCStringPool& operator=(const TCStringPool& other);

	// The pooled string (created if absent). Null or empty strings give a null handle.
	TPCString Get(const char* string);
	TPCString Get(const TCString& string);
	TPCString Get(int value); // decimal
	// The pooled string if present, otherwise a null handle.
	TPCString GetOptional(const char* string);
	TPCString GetOptional(const TCString& string);
	TPCString GetOptional(int value);
	// A fresh, unpooled string with the given contents.
	TPCString GetVolatile(const char* string);
	TPCString GetVolatile(const TCString& string);
	TPCString GetVolatile(int value);
	bool IsPresent(const char* string);
	bool IsPresent(const TCString& string);
	bool IsPresent(int value);
	void Dump() const;

protected:
	void Remove(TPooledCString& string);

private:
	int m_iGrowSize;
	int m_iCount;
	int m_iCapacity;
	jpog::kernel::PoolEntry<char, TPooledCString>* m_pEntries; // sorted by content
};

JPOG_ASSERT_SIZE(TCStringPool, 16);

} // namespace Toshi
