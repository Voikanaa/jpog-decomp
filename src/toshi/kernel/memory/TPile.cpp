#include "toshi/kernel/memory/TPile.h"

#include "toshi/kernel/memory/TMemory.h"

#include <cstdint>
#include <memory>

namespace Toshi
{

namespace
{

// Blocks start with their link (4 bytes on x86, as the original).
constexpr int kLinkSize = static_cast<int>(sizeof(void*));

// The next multiple of `alignment` (a power of two) at or after `pointer`.
unsigned char* AlignUp(unsigned char* pointer, int alignment)
{
	void* aligned = pointer;
	std::size_t space = SIZE_MAX;
	std::align(static_cast<std::size_t>(alignment), 1, aligned, space);
	return static_cast<unsigned char*>(aligned);
}

} // namespace

TPile::TPile(int blockSize, int maxSize, unsigned int flags)
    : m_iBlockSize(blockSize), m_iMaxBlocks(maxSize / blockSize), m_uiFlags(flags), m_iNumBlocks(0), m_iBlockIndex(0),
      m_pCursor(nullptr), m_pBlockEnd(nullptr), m_pFirstBlock(nullptr), m_pCurrentBlock(nullptr), m_iNumAllocs(0),
      m_iWastedSpace(0)
{
}

TPile::~TPile()
{
}

// Allocations that do not fit move on to the next block (reused after a Reset, otherwise new).
// The original also advances the cursor of a failed allocation.
void* TPile::RawAlloc(int size, int alignment)
{
	++m_iNumAllocs;
	unsigned char* memory = nullptr;
	bool fits = false;
	if (m_pCursor != nullptr)
	{
		memory = AlignUp(m_pCursor, alignment);
		m_pCursor = memory + size;
		fits = m_pCursor <= m_pBlockEnd;
	}
	else
		fits = size == 0; // no block yet: a null pointer "fits" an empty request
	if (fits)
		return memory;

	int index = m_iBlockIndex;
	Block* block;
	if (index < m_iNumBlocks)
	{
		block = m_pCurrentBlock->m_pNext;
		m_pCurrentBlock = block;
	}
	else
	{
		if (m_iMaxBlocks != 0 && m_iMaxBlocks <= m_iNumBlocks)
			return nullptr;
		if ((m_uiFlags & 1) == 0)
			block = static_cast<Block*>(tmalloc(m_iBlockSize + kLinkSize, nullptr, -1));
		else
			block = static_cast<Block*>(tmemalign(0x20, (m_iBlockSize + kLinkSize + 0x1F) & ~0x1F));
		block->m_pNext = nullptr;
		if (m_pCurrentBlock == nullptr)
			m_pFirstBlock = block;
		else
			m_pCurrentBlock->m_pNext = block;
		m_pCurrentBlock = block;
		index = m_iBlockIndex;
		++m_iNumBlocks;
	}
	auto* start = reinterpret_cast<unsigned char*>(block);
	m_pBlockEnd = start + m_iBlockSize + kLinkSize;
	memory = AlignUp(start + kLinkSize, alignment);
	m_iBlockIndex = index + 1;
	m_pCursor = memory + size;
	return memory;
}

bool TPile::RawIsInside(void* memory)
{
	const auto* address = static_cast<const unsigned char*>(memory);
	for (const Block* block = m_pFirstBlock; block != nullptr; block = block->m_pNext)
	{
		const auto* start = reinterpret_cast<const unsigned char*>(block);
		if (start < address && address < start + m_iBlockSize * 4)
			return true;
	}
	return false;
}

void TPile::Reset()
{
	if (m_pFirstBlock == nullptr)
	{
		m_pCursor = nullptr;
		m_pBlockEnd = nullptr;
		m_iBlockIndex = 0;
	}
	else
	{
		auto* start = reinterpret_cast<unsigned char*>(m_pFirstBlock);
		m_pCursor = start + kLinkSize;
		m_pBlockEnd = start + m_iBlockSize + kLinkSize;
		m_iBlockIndex = 1;
	}
	m_pCurrentBlock = m_pFirstBlock;
	m_iNumAllocs = 0;
	m_iWastedSpace = 0;
}

void TPile::Free()
{
	while (m_pFirstBlock != nullptr)
	{
		Block* next = m_pFirstBlock->m_pNext;
		tfree(m_pFirstBlock);
		m_pFirstBlock = next;
	}
	m_iNumBlocks = 0;
	Reset();
}

int TPile::GetNumAllocs() const
{
	return m_iNumAllocs;
}

int TPile::GetNumBlocks() const
{
	return m_iNumBlocks;
}

int TPile::GetWastedSpace() const
{
	return m_iWastedSpace;
}

} // namespace Toshi
