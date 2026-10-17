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

// Handle to a pooled wide string, like TPCString. A null handle reads as the empty string.
class TPWString
{
	friend class TWStringPool;

public:
	TPWString();
	TPWString(const TPWString& other);
	~TPWString();
	TPWString& operator=(const TPWString& other);

	// Orders by string content (null first); equal handles compare equal without a string compare.
	int Compare(const TPWString& other) const;
	const TWString& GetWString() const;
	// The string itself; the handle must not be null.
	TWString& GetVolatileWString() const;
	// The owning pool; the handle must not be null.
	TWStringPool* GetStringPool() const;

	bool operator==(const TPWString& other) const;
	bool operator!=(const TPWString& other) const;
	bool operator<(const TPWString& other) const;
	operator const TWString*() const;
	const TWString* operator->() const;
	const TWString& operator*() const;

	static const TWString ms_sEmpty;

private:
	explicit TPWString(TPooledWString* string);

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

TWString TSTDCALL operator+(const TWString& string, const TPWString& other);

} // namespace Toshi
