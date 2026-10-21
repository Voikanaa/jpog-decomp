#include "toshi/kernel/memory/TFreeList.h"

#include "toshi/kernel/memory/TMemory.h"

#include <cstddef>

namespace Toshi
{

namespace
{

TFreeList::Node* ItemAt(TFreeList::Node* first, int index, unsigned int itemSize)
{
	return reinterpret_cast<TFreeList::Node*>(reinterpret_cast<char*>(first) +
	                                          static_cast<std::ptrdiff_t>(index) * itemSize);
}

} // namespace

TFreeList::Node& TFreeList::Node::operator=(const Node& other)
{
	next = other.next;
	return *this;
}

// `name` is only a debugging label in the original and is not stored.
TFreeList::TFreeList(int itemSize, int initialCapacity, int growSize, char*)
    : m_iCapacity(0), m_iItemSize(itemSize), m_pFreeList(nullptr), m_pBlockList(nullptr)
{
	m_iGrowSize = growSize < 1 ? 8 : growSize;
	SetCapacity(initialCapacity);
}

TFreeList::~TFreeList()
{
	while (Node* block = m_pBlockList)
	{
		m_pBlockList = block->next;
		tfree(block);
	}
}

void* TFreeList::New(unsigned int size)
{
	if (size != static_cast<unsigned int>(m_iItemSize))
		return tmalloc(static_cast<int>(size), nullptr, -1);
	if (Node* item = m_pFreeList)
	{
		m_pFreeList = item->next;
		return item;
	}
	return Allocate(m_iGrowSize, size);
}

void TFreeList::Delete(void* item)
{
	Node* node = static_cast<Node*>(item);
	node->next = m_pFreeList;
	m_pFreeList = node;
}

int TFreeList::GetCapacity() const
{
	return m_iCapacity;
}

void TFreeList::SetCapacity(int capacity)
{
	if (capacity <= m_iCapacity)
		return;
	Node* last = Allocate(capacity - m_iCapacity, static_cast<unsigned int>(m_iItemSize));
	last->next = m_pFreeList;
	m_pFreeList = last;
}

int TFreeList::GetGrowSize() const
{
	return m_iGrowSize;
}

void TFreeList::SetGrowSize(int growSize)
{
	m_iGrowSize = growSize < 1 ? 8 : growSize;
}

TFreeList::Node* TFreeList::Allocate(int count, unsigned int itemSize)
{
	m_iCapacity += count;
	// Each block starts with a link to the previous block, followed by the items.
	Node* block = static_cast<Node*>(tmalloc(static_cast<int>(itemSize * count + sizeof(Node)), nullptr, -1));
	block->next = m_pBlockList;
	m_pBlockList = block;

	Node* first = block + 1;
	Node* previous = nullptr;
	for (int i = 0; i < count - 1; ++i)
	{
		Node* item = ItemAt(first, i, itemSize);
		item->next = previous;
		previous = item;
	}
	m_pFreeList = previous;
	return ItemAt(first, count - 1, itemSize);
}

} // namespace Toshi
