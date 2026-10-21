#pragma once

#include "common/layout.h"

namespace Toshi
{

// Bump allocator over a chain of fixed-size blocks (TKernelInterface.dll). Memory is only
// returned all at once (Reset reuses the blocks, Free releases them). Flag 1: 32-byte aligned
// blocks.
class TPile
{
public:
	// `maxSize` / `blockSize` blocks at most (0: unlimited).
	TPile(int blockSize, int maxSize, unsigned int flags);
	~TPile();

	void* RawAlloc(int size, int alignment);
	// Whether `memory` lies in a block. Quirk kept: the block range is taken as four times the
	// block size.
	bool RawIsInside(void* memory);
	void Reset();
	void Free();
	int GetNumAllocs() const;
	int GetNumBlocks() const;
	int GetWastedSpace() const;

private:
	// Blocks start with the link to the next block.
	struct Block
	{
		Block* m_pNext;
	};

	int m_iBlockSize;
	int m_iMaxBlocks;
	unsigned int m_uiFlags;
	int m_iNumBlocks;
	int m_iBlockIndex; // 1-based index of the current block, 0 before the first
	unsigned char* m_pCursor;
	unsigned char* m_pBlockEnd;
	Block* m_pFirstBlock;
	Block* m_pCurrentBlock;
	int m_iNumAllocs;
	int m_iWastedSpace;
};

JPOG_ASSERT_SIZE(TPile, 0x2C);

} // namespace Toshi
