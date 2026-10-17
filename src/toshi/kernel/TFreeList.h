#pragma once

#include "common/layout.h"

namespace Toshi
{

// Pool allocator for objects of one size (TKernelInterface.dll). Items are carved out of blocks
// from tmalloc and kept on an intrusive free list; requests of any other size fall through to
// tmalloc. Blocks are only returned to the heap when the free list is destroyed.
class TFreeList
{
public:
	struct Node
	{
		Node& operator=(const Node& other);

		Node* next;
	};

	TFreeList(int itemSize, int initialCapacity, int growSize, char* name);
	~TFreeList();

	void* New(unsigned int size);
	// Returns an item to the free list (no size check, as in the original).
	void Delete(void* item);

	int GetCapacity() const;
	// Grows the pool so that it holds at least `capacity` items.
	void SetCapacity(int capacity);
	int GetGrowSize() const;
	// Items added per block when the free list runs empty; values below 1 become 8.
	void SetGrowSize(int growSize);

protected:
	// Allocates a block of `count` items: all but the last go on the free list (replacing it),
	// and the last one is returned.
	Node* Allocate(int count, unsigned int itemSize);

private:
	int m_iCapacity;
	int m_iItemSize;
	Node* m_pFreeList;
	Node* m_pBlockList;
	int m_iGrowSize;
};

JPOG_ASSERT_SIZE(TFreeList, 20);

} // namespace Toshi
