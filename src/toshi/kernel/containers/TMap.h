#pragma once

#include "toshi/kernel/containers/SortedArray.h"

namespace Toshi
{

// Default map ordering. The original compares by subtraction, so keys more than 2^31 apart
// order the wrong way round.
template <typename T> struct TComparator
{
	static int Compare(const T& a, const T& b)
	{
		if (static_cast<int>(static_cast<unsigned int>(a) - static_cast<unsigned int>(b)) < 0)
			return -1;
		return a == b ? 0 : 1;
	}
};

// Sorted array map with duplicate keys (TKernelInterface.dll; the original defines it inline).
// Lookups are binary searches; Insert places a pair after any equal keys.
template <typename Key, typename Value, typename Comparator> class TMap
{
public:
	class TPair
	{
	public:
		TPair() : first(), second() {}
		~TPair() {} // user-declared, so arrays of pairs carry a count cookie like the original's

		Key first;
		Value second;
	};

	explicit TMap(int growSize, int capacity = 0) : m_oPairs(growSize, capacity) {}
	TMap(const TMap& other) : m_oPairs(other.m_oPairs.growSize) { m_oPairs.CopyFrom(other.m_oPairs); }
	TMap& operator=(const TMap& other)
	{
		m_oPairs.CopyFrom(other.m_oPairs);
		return *this;
	}

	int Count() const { return m_oPairs.count; }
	TPair& operator[](int index) { return m_oPairs[index]; }
	const TPair& operator[](int index) const { return m_oPairs[index]; }

	// Index after the last pair with `key`; `found` tells whether there is one.
	int Search(const Key& key, bool& found) const { return m_oPairs.Search(key, ComparePair, found); }
	// Index of the first pair with `key`, or -1.
	int FindFirst(const Key& key) const { return m_oPairs.FindFirst(key, ComparePair); }

	void Insert(const Key& key, const Value& value)
	{
		bool found = false;
		const int index = Search(key, found);
		m_oPairs.InsertSlots(index, 1);
		m_oPairs[index].first = key;
		m_oPairs[index].second = value;
	}

	void RemoveAt(int index) { m_oPairs.RemoveAt(index); }

	// Drops all pairs and the array (the growth size stays).
	void Clear()
	{
		delete[] m_oPairs.data;
		m_oPairs.data = nullptr;
		m_oPairs.capacity = 0;
		m_oPairs.count = 0;
	}

private:
	static int ComparePair(const Key& key, const TPair& pair) { return Comparator::Compare(key, pair.first); }

	jpog::kernel::SortedArray<TPair> m_oPairs;
};

} // namespace Toshi
