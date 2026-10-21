#pragma once

#include "common/layout.h"

namespace Toshi
{

// Fixed-size item pool over a chain of blocks (TKernelInterface.dll); a base for typed pools.
class TGenericPool
{
public:
	struct TNode
	{
		TNode& operator=(const TNode& other);
		TNode* m_pNext;
	};

	struct TBlock
	{
		TBlock& operator=(const TBlock& other);
		TBlock* m_pNext;
	};

	TGenericPool& operator=(const TGenericPool& other);

protected:
	// Items are rounded up to `alignment` (at least a pointer); blocks hold `blockSize` bytes of items;
	// `maxSize` limits all blocks together (0: unlimited).
	TGenericPool(int itemSize, int alignment, int blockSize, int maxSize);
	~TGenericPool();

	void* Alloc();
	void Free(void* item);
	bool AllocBlock();
	// Puts every item of every block back on the free list.
	void Reset();
	// Releases all blocks.
	void Reinit();

private:
	void ResetBlock(TBlock* block);

	int m_iItemSize;
	int m_iAlignment;
	int m_iBlockSize;
	int m_iMaxSize;
	int m_iTotalSize;
	TBlock* m_pBlocks;
	TNode* m_pFreeList;
};

JPOG_ASSERT_SIZE(TGenericPool, 0x1C);

} // namespace Toshi
