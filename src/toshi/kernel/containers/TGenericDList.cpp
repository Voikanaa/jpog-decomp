#include "toshi/kernel/containers/TGenericDList.h"

namespace Toshi
{

using TNode = TGenericDList::TNode;

// --- TNode ---------------------------------------------------------------------------------

TNode::TNode() : m_pNext(this), m_pPrev(this)
{
}

TNode::TNode(TUninitialised)
{
}

TNode& TNode::operator=(const TNode& other)
{
	m_pNext = other.m_pNext;
	m_pPrev = other.m_pPrev;
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

void TNode::Reset()
{
	m_pNext = this;
	m_pPrev = this;
}

void TNode::Remove()
{
	m_pPrev->m_pNext = m_pNext;
	m_pNext->m_pPrev = m_pPrev;
	m_pNext = this;
	m_pPrev = this;
}

void TNode::InsertAfter(TNode* node)
{
	m_pPrev = node;
	m_pNext = node->m_pNext;
	node->m_pNext = this;
	m_pNext->m_pPrev = this;
}

void TNode::InsertBefore(TNode* node)
{
	m_pNext = node;
	m_pPrev = node->m_pPrev;
	node->m_pPrev = this;
	m_pPrev->m_pNext = this;
}

// --- TGenericDList -------------------------------------------------------------------------

TGenericDList::TGenericDList()
{
}

TGenericDList::~TGenericDList()
{
	RemoveAll();
}

TGenericDList& TGenericDList::operator=(const TGenericDList& other)
{
	m_oRoot = other.m_oRoot;
	return *this;
}

void TGenericDList::InsertHead(TNode* node)
{
	node->InsertAfter(&m_oRoot);
}

void TGenericDList::InsertTail(TNode* node)
{
	node->InsertBefore(&m_oRoot);
}

void TGenericDList::InsertSegmentAtHead(TNode* first, TNode* last)
{
	first->m_pPrev = &m_oRoot;
	last->m_pNext = m_oRoot.m_pNext;
	first->m_pPrev->m_pNext = first;
	last->m_pNext->m_pPrev = last;
}

void TGenericDList::InsertSegmentAtTail(TNode* first, TNode* last)
{
	last->m_pNext = &m_oRoot;
	first->m_pPrev = m_oRoot.m_pPrev;
	last->m_pNext->m_pPrev = last;
	first->m_pPrev->m_pNext = first;
}

void TGenericDList::RemoveHead()
{
	if (!IsEmpty())
		m_oRoot.m_pNext->Remove();
}

void TGenericDList::RemoveTail()
{
	if (!IsEmpty())
		m_oRoot.m_pPrev->Remove();
}

void TGenericDList::RemoveAll()
{
	while (m_oRoot.m_pNext != &m_oRoot)
		m_oRoot.m_pNext->Remove();
}

TNode* TGenericDList::Head() const
{
	return m_oRoot.m_pNext;
}

TNode* TGenericDList::Tail() const
{
	return m_oRoot.m_pPrev;
}

TNode* TGenericDList::Begin() const
{
	return m_oRoot.m_pNext;
}

const TNode* TGenericDList::End() const
{
	return &m_oRoot;
}

TNode* TGenericDList::RBegin() const
{
	return m_oRoot.m_pPrev;
}

const TNode* TGenericDList::REnd() const
{
	return &m_oRoot;
}

bool TGenericDList::IsEmpty() const
{
	return m_oRoot.m_pNext == &m_oRoot;
}

void TSTDCALL TGenericDList::InsertSegmentAfter(TNode* first, TNode* last, TNode* position)
{
	first->m_pPrev = position;
	last->m_pNext = position->m_pNext;
	first->m_pPrev->m_pNext = first;
	last->m_pNext->m_pPrev = last;
}

void TSTDCALL TGenericDList::InsertSegmentBefore(TNode* first, TNode* last, TNode* position)
{
	last->m_pNext = position;
	first->m_pPrev = position->m_pPrev;
	last->m_pNext->m_pPrev = last;
	first->m_pPrev->m_pNext = first;
}

void TSTDCALL TGenericDList::RemoveSegment(TNode* first, TNode* last)
{
	first->m_pPrev->m_pNext = last->m_pNext;
	last->m_pNext->m_pPrev = first->m_pPrev;
	first->m_pPrev = first;
	last->m_pNext = last;
}

} // namespace Toshi
