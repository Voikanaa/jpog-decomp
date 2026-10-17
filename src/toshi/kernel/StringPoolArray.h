#pragma once

// Sorted entry arrays shared by TCStringPool and TWStringPool. The original inlines one template
// instance per pool; these helpers live outside namespace Toshi so they add no symbols there.

#include "toshi/kernel/TMemory.h"
#include "toshi/kernel/TSystem.h"

#include <cstddef>

namespace jpog::kernel
{

// One pool entry: the string's characters (the sort key) and its pooled string.
template <typename Char, typename Pooled> struct PoolEntry
{
	const Char* key;
	Pooled* string;
};

// Entry arrays carry their element count in front, like the original's array new.
template <typename Entry>
constexpr std::size_t kEntryCookie = alignof(Entry) > sizeof(int) ? alignof(Entry) : sizeof(int);

template <typename Entry> Entry* AllocEntries(int count)
{
	auto* raw = static_cast<unsigned char*>(
	    tmalloc(static_cast<int>(count * sizeof(Entry) + kEntryCookie<Entry>), nullptr, -1));
	if (raw == nullptr)
		return nullptr;
	*reinterpret_cast<int*>(raw) = count;
	return reinterpret_cast<Entry*>(raw + kEntryCookie<Entry>);
}

template <typename Entry> void FreeEntries(Entry* entries)
{
	tfree(reinterpret_cast<unsigned char*>(entries) - kEntryCookie<Entry>);
}

// Binary search for `key`. Returns the insertion index, or (with `found`) one past the last
// entry equal to `key`.
template <typename Char, typename Entry>
int SearchEntries(const Entry* entries, int count, const Char* key, bool& found)
{
	found = false;
	int low = 0;
	int high = count - 1;
	while (low <= high)
	{
		int middle = (low + high) / 2;
		const int order = Toshi::TSystem::StringCompare(key, entries[middle].key, -1);
		if (order < 0)
			high = middle - 1;
		else if (order > 0)
			low = middle + 1;
		else
		{
			do
				++middle;
			while (middle <= high && Toshi::TSystem::StringCompare(key, entries[middle].key, -1) == 0);
			found = true;
			return middle;
		}
	}
	return low;
}

// Index of the first entry equal to `key`, or -1.
template <typename Char, typename Entry> int FindFirstEntry(const Entry* entries, int count, const Char* key)
{
	int low = 0;
	int high = count - 1;
	while (low <= high)
	{
		int middle = (low + high) / 2;
		const int order = Toshi::TSystem::StringCompare(key, entries[middle].key, -1);
		if (order < 0)
			high = middle - 1;
		else if (order > 0)
			low = middle + 1;
		else
		{
			while (middle - 1 >= low && Toshi::TSystem::StringCompare(key, entries[middle - 1].key, -1) == 0)
				--middle;
			return middle;
		}
	}
	return -1;
}

// Opens `slots` entries at `index`, growing the array by max(growSize, slots) when full.
template <typename Entry>
void InsertEntrySlots(int growSize, int& count, int& capacity, Entry*& entries, int index, int slots)
{
	if (capacity < count + slots)
	{
		const int newCapacity = capacity + (growSize <= slots ? slots : growSize);
		Entry* grown = AllocEntries<Entry>(newCapacity);
		if (entries != nullptr)
		{
			for (int i = 0; i < index; ++i)
				grown[i] = entries[i];
			for (int i = index; i < count; ++i)
				grown[i + slots] = entries[i];
			FreeEntries(entries);
		}
		capacity = newCapacity;
		entries = grown;
		count += slots;
		return;
	}
	for (int i = count - 1; i >= index; --i)
		entries[i + slots] = entries[i];
	count += slots;
}

// Removes the entry at `index` by shifting the rest down. As in the original there is no check
// that `index` is valid (-1 when the string was not found).
template <typename Entry> void RemoveEntry(int& count, Entry* entries, int index)
{
	const int last = count - 1;
	for (; index < last; ++index)
		entries[index] = entries[index + 1];
	count = last;
}

} // namespace jpog::kernel
