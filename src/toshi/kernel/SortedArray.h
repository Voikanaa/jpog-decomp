#pragma once

// The original's inline sorted-array container (used by the file lexers' define sets and the
// TMap template). Elements live in an array new'd with a count cookie, so its whole capacity is
// constructed and destroyed; removed tail slots keep their old values (and references) until
// the array is reallocated or destroyed, as in the original. Lives outside namespace Toshi so it
// adds no symbols there.

#include <cstddef>

namespace jpog::kernel
{

template <typename T> struct SortedArray
{
	int growSize;
	int count = 0;
	int capacity = 0;
	T* data = nullptr;

	explicit SortedArray(int grow, int initialCapacity = 0) : growSize(grow), capacity(initialCapacity)
	{
		if (initialCapacity > 0)
			data = new T[static_cast<std::size_t>(initialCapacity)];
	}
	SortedArray(const SortedArray&) = delete;
	SortedArray& operator=(const SortedArray&) = delete;
	~SortedArray() { delete[] data; }

	T& operator[](int index) { return data[index]; }
	const T& operator[](int index) const { return data[index]; }

	// Upper bound of `key` (one past the last equal element); `found` tells whether one exists.
	// `compare(key, element)` returns <0, 0 or >0.
	template <typename Key, typename Compare> int Search(const Key& key, Compare compare, bool& found) const
	{
		found = false;
		int low = 0;
		int high = count - 1;
		while (low <= high)
		{
			int middle = (low + high) / 2;
			const int order = compare(key, data[middle]);
			if (order < 0)
				high = middle - 1;
			else if (order > 0)
				low = middle + 1;
			else
			{
				for (++middle; middle <= high && compare(key, data[middle]) == 0; ++middle)
				{
				}
				found = true;
				return middle;
			}
		}
		return low;
	}

	// Index of the first element equal to `key`, or -1.
	template <typename Key, typename Compare> int FindFirst(const Key& key, Compare compare) const
	{
		int low = 0;
		int high = count - 1;
		while (low <= high)
		{
			int middle = (low + high) / 2;
			const int order = compare(key, data[middle]);
			if (order < 0)
				high = middle - 1;
			else if (order > 0)
				low = middle + 1;
			else
			{
				for (--middle; middle >= low && compare(key, data[middle]) == 0; --middle)
				{
				}
				return middle + 1;
			}
		}
		return -1;
	}

	// Opens `slots` elements at `index` by copy-assigning the tail upwards. When full, a new
	// array of capacity + max(growSize, slots) is allocated and the elements are assigned over.
	void InsertSlots(int index, int slots)
	{
		if (count + slots <= capacity)
		{
			for (int i = count - 1; i >= index; --i)
				data[i + slots] = data[i];
			count += slots;
			return;
		}
		const int newCapacity = capacity + (growSize <= slots ? slots : growSize);
		T* grown = new T[static_cast<std::size_t>(newCapacity)];
		if (data != nullptr)
		{
			for (int i = 0; i < index; ++i)
				grown[i] = data[i];
			for (int i = index; i < count; ++i)
				grown[i + slots] = data[i];
			delete[] data;
		}
		capacity = newCapacity;
		data = grown;
		count += slots;
	}

	// Removes the element at `index` by shifting the rest down (no range check, as the
	// original).
	void RemoveAt(int index)
	{
		const int last = count - 1;
		for (; index < last; ++index)
			data[index] = data[index + 1];
		count = last;
	}
};

} // namespace jpog::kernel
