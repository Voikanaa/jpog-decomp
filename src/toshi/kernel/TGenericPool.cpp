#include "toshi/kernel/TGenericPool.h"

#include "toshi/kernel/TMemory.h"

#include <cstdint>
#include <memory>

namespace Toshi
{

namespace
{

// Block links and free items hold a pointer (4 bytes on x86, as the original).
constexpr int kLinkSize = static_cast<int>(sizeof(void*));

} // namespace

TGenericPool::TNode& TGenericPool::TNode::operator=(const TNode& other)
{
	m_pNext = other.m_pNext;
	return *this;
}

TGenericPool::TBlock& TGenericPool::TBlock::operator=(const TBlock& other)
{
	m_pNext = other.m_pNext;
	return *this;
}

TGenericPool::TGenericPool(int itemSize, int alignment, int blockSize, int maxSize)
    : m_iAlignment(alignment < kLinkSize ? kLinkSize : alignment), m_iMaxSize(maxSize), m_iTotalSize(0),
      m_pBlocks(nullptr), m_pFreeList(nullptr)
{
	m_iItemSize = (itemSize + m_iAlignment - 1) & ~(m_iAlignment - 1);
	m_iBlockSize = blockSize - blockSize % m_iItemSize;
}

TGenericPool::~TGenericPool()
{
	Reinit();
}

TGenericPool& TGenericPool::operator=(const TGenericPool& other)
{
	m_iItemSize = other.m_iItemSize;
	m_iAlignment = other.m_iAlignment;
	m_iBlockSize = other.m_iBlockSize;
	m_iMaxSize = other.m_iMaxSize;
	m_iTotalSize = other.m_iTotalSize;
	m_pBlocks = other.m_pBlocks;
	m_pFreeList = other.m_pFreeList;
	return *this;
}

void* TGenericPool::Alloc()
{
	if (m_pFreeList == nullptr && !AllocBlock())
		return nullptr;
	TNode* item = m_pFreeList;
	m_pFreeList = item->m_pNext;
	return item;
}

void TGenericPool::Free(void* item)
{
	if (item == nullptr)
		return;
	auto* node = static_cast<TNode*>(item);
	node->m_pNext = m_pFreeList;
	m_pFreeList = node;
}

bool TGenericPool::AllocBlock()
{
	const int total = m_iBlockSize + m_iTotalSize;
	if (m_iMaxSize != 0 && total > m_iMaxSize)
		return false;
	auto* block = static_cast<TBlock*>(tmalloc(m_iAlignment + m_iBlockSize + (kLinkSize - 4), nullptr, -1));
	if (block == nullptr)
		return false;
	block->m_pNext = m_pBlocks;
	m_pBlocks = block;
	m_iTotalSize = total;
	ResetBlock(block);
	return true;
}

void TGenericPool::Reset()
{
	m_pFreeList = nullptr;
	for (TBlock* block = m_pBlocks; block != nullptr; block = block->m_pNext)
		ResetBlock(block);
}

void TGenericPool::Reinit()
{
	for (TBlock* block = m_pBlocks; block != nullptr;)
	{
		TBlock* next = block->m_pNext;
		tfree(block);
		block = next;
	}
	m_iTotalSize = 0;
	m_pBlocks = nullptr;
	m_pFreeList = nullptr;
}

// Items start at the first aligned address after the block link; the block's items are linked
// in address order in front of the free list.
void TGenericPool::ResetBlock(TBlock* block)
{
	void* aligned = reinterpret_cast<unsigned char*>(block) + kLinkSize;
	std::size_t space = SIZE_MAX;
	std::align(static_cast<std::size_t>(m_iAlignment), 1, aligned, space);
	auto* first = static_cast<unsigned char*>(aligned);
	unsigned char* last = first + (m_iBlockSize - m_iItemSize);
	for (unsigned char* item = first; item != last; item += m_iItemSize)
		reinterpret_cast<TNode*>(item)->m_pNext = reinterpret_cast<TNode*>(item + m_iItemSize);
	reinterpret_cast<TNode*>(last)->m_pNext = m_pFreeList;
	m_pFreeList = reinterpret_cast<TNode*>(first);
}

} // namespace Toshi
