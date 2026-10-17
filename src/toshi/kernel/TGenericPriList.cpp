#include "toshi/kernel/TGenericPriList.h"

namespace Toshi
{

using TNode = TGenericPriList::TNode;

namespace
{

// The list's two links overlay a node's m_pNext/m_pPrev, so the list doubles as the sentinel
// node (its priority field is never read).
TNode* SentinelOf(const TGenericPriList* list)
{
	return reinterpret_cast<TNode*>(const_cast<TGenericPriList*>(list));
}

} // namespace

// --- TNode ---------------------------------------------------------------------------------

TNode::TNode() : m_pNext(this), m_pPrev(this), m_iPriority(0)
{
}

TNode::TNode(TUninitialised)
{
}

TNode& TNode::operator=(const TNode& other)
{
	m_pNext = other.m_pNext;
	m_pPrev = other.m_pPrev;
	m_iPriority = other.m_iPriority;
	return *this;
}

TNode* TNode::Next() const
{
	return m_pNext;
}

TNode* TNode::Prev() const
{
	return m_pPrev;
}

bool TNode::IsLinked() const
{
	return m_pNext != this;
}

int TNode::GetPriority() const
{
	return m_iPriority;
}

void TNode::SetPriority(int priority)
{
	m_iPriority = priority;
}

void TNode::Reset()
{
	m_pNext = this;
	m_pPrev = this;
	m_iPriority = 0;
}

void TNode::Remove()
{
	m_pPrev->m_pNext = m_pNext;
	m_pNext->m_pPrev = m_pPrev;
	m_pNext = this;
	m_pPrev = this;
}

// --- TGenericPriList -----------------------------------------------------------------------

TGenericPriList::TGenericPriList() : m_pHead(SentinelOf(this)), m_pTail(SentinelOf(this))
{
}

TGenericPriList::~TGenericPriList()
{
	RemoveAll();
}

TGenericPriList& TGenericPriList::operator=(const TGenericPriList& other)
{
	m_pHead = other.m_pHead;
	m_pTail = other.m_pTail;
	return *this;
}

void TGenericPriList::Insert(TNode* node)
{
	TNode* root = SentinelOf(this);
	const int priority = node->m_iPriority;
	if (priority < 0)
	{
		TNode* position = m_pHead;
		while (position != root && position->m_iPriority <= priority)
			position = position->m_pNext;
		node->m_pNext = position;
		node->m_pPrev = position->m_pPrev;
		position->m_pPrev = node;
		node->m_pPrev->m_pNext = node;
		return;
	}
	TNode* position = m_pTail;
	while (position != root && priority < position->m_iPriority)
		position = position->m_pPrev;
	node->m_pPrev = position;
	node->m_pNext = position->m_pNext;
	position->m_pNext = node;
	node->m_pNext->m_pPrev = node;
}

void TGenericPriList::Insert(TNode* node, int priority)
{
	node->SetPriority(priority);
	Insert(node);
}

void TGenericPriList::InsertHead(TNode* node)
{
	node->m_iPriority = -0x8000;
	node->m_pPrev = SentinelOf(this);
	node->m_pNext = m_pHead;
	m_pHead = node;
	node->m_pNext->m_pPrev = node;
}

void TGenericPriList::InsertTail(TNode* node)
{
	node->m_iPriority = 0x7FFF;
	node->m_pNext = SentinelOf(this);
	node->m_pPrev = m_pTail;
	m_pTail = node;
	node->m_pPrev->m_pNext = node;
}

void TGenericPriList::SetPriority(TNode* node, int priority)
{
	node->Remove();
	node->m_iPriority = priority;
	Insert(node);
}

void TGenericPriList::RemoveHead()
{
	if (!IsEmpty())
		m_pHead->Remove();
}

void TGenericPriList::RemoveTail()
{
	if (!IsEmpty())
		m_pTail->Remove();
}

void TGenericPriList::RemoveAll()
{
	while (m_pHead != SentinelOf(this))
		m_pHead->Remove();
}

TNode* TGenericPriList::Head() const
{
	return m_pHead;
}

TNode* TGenericPriList::Tail() const
{
	return m_pTail;
}

TNode* TGenericPriList::Begin() const
{
	return m_pHead;
}

const TNode* TGenericPriList::End() const
{
	return SentinelOf(this);
}

TNode* TGenericPriList::RBegin() const
{
	return m_pTail;
}

const TNode* TGenericPriList::REnd() const
{
	return SentinelOf(this);
}

bool TGenericPriList::IsEmpty() const
{
	return m_pHead == SentinelOf(this);
}

} // namespace Toshi
