#include "toshi/kernel/containers/TGenericNodeList.h"

namespace Toshi
{

using TNode = TGenericNodeList::TNode;
using Iterator = TGenericNodeList::Iterator;

// --- TNode ---------------------------------------------------------------------------------

TNode::TNode() : m_pNext(this), m_pPrev(this)
{
	SetList(nullptr);
}

TNode::TNode(const TNode& other) : m_pList(other.m_pList), m_pNext(other.m_pNext), m_pPrev(other.m_pPrev)
{
}

TNode::~TNode()
{
}

TNode& TNode::operator=(const TNode& other)
{
	m_pList = other.m_pList;
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

TGenericNodeList* TNode::GetList() const
{
	return m_pList;
}

bool TNode::IsLinked() const
{
	return GetList() != nullptr;
}

TGenericNodeList* TNode::SetList(TGenericNodeList* list)
{
	TGenericNodeList* previous = m_pList;
	m_pList = list;
	return previous;
}

// --- Iterator ------------------------------------------------------------------------------

Iterator::Iterator() : m_pNode(nullptr)
{
}

Iterator::Iterator(TNode* node) : m_pNode(node)
{
}

Iterator::Iterator(const Iterator& other) : m_pNode(other.m_pNode)
{
}

Iterator::~Iterator()
{
}

Iterator& Iterator::operator=(const Iterator& other)
{
	m_pNode = other.m_pNode;
	return *this;
}

bool Iterator::IsNull()
{
	return m_pNode == nullptr;
}

bool Iterator::operator!()
{
	return IsNull();
}

bool Iterator::operator==(Iterator& other) const
{
	return m_pNode == other.m_pNode;
}

bool Iterator::operator!=(Iterator& other) const
{
	return m_pNode != other.m_pNode;
}

Iterator& Iterator::operator++()
{
	if (m_pNode != nullptr)
		m_pNode = m_pNode->Next();
	return *this;
}

Iterator Iterator::operator++(int)
{
	const Iterator previous = *this;
	if (m_pNode != nullptr)
		m_pNode = m_pNode->Next();
	return previous;
}

Iterator& Iterator::operator--()
{
	if (m_pNode != nullptr)
		m_pNode = m_pNode->Prev();
	return *this;
}

TNode& Iterator::operator*() const
{
	return *m_pNode;
}

TNode* Iterator::operator->() const
{
	return m_pNode;
}

Iterator::operator TNode*() const
{
	return m_pNode;
}

// --- TGenericNodeList ----------------------------------------------------------------------

TGenericNodeList::TGenericNodeList() : m_iCount(0)
{
}

TGenericNodeList::TGenericNodeList(TGenericNodeList&)
{
}

TGenericNodeList& TGenericNodeList::operator=(TGenericNodeList&)
{
	return *this;
}

TGenericNodeList::~TGenericNodeList()
{
	DeleteAll();
}

void TGenericNodeList::InsertHead(TNode& node)
{
	InsertAfter(m_oRoot, node);
}

void TGenericNodeList::InsertTail(TNode& node)
{
	InsertBefore(m_oRoot, node);
}

void TGenericNodeList::InsertBefore(TNode& position, TNode& node)
{
	node.SetList(this);
	node.m_pNext = &position;
	node.m_pPrev = position.m_pPrev;
	position.m_pPrev->m_pNext = &node;
	position.m_pPrev = &node;
	++m_iCount;
}

void TGenericNodeList::InsertAfter(TNode& position, TNode& node)
{
	node.SetList(this);
	node.m_pNext = position.m_pNext;
	node.m_pPrev = &position;
	position.m_pNext->m_pPrev = &node;
	position.m_pNext = &node;
	++m_iCount;
}

void TGenericNodeList::Append(TGenericNodeList& list)
{
	TNode* tail = m_oRoot.m_pPrev;
	TNode* otherHead = list.m_oRoot.m_pNext;
	TNode* otherTail = list.m_oRoot.m_pPrev;
	if (list.IsEmpty())
		return;
	for (TNode* node = otherHead; node != &list.m_oRoot; node = node->m_pNext)
		node->SetList(this);
	m_iCount += list.Count();
	tail->m_pNext = otherHead;
	otherHead->m_pPrev = tail;
	otherTail->m_pNext = &m_oRoot;
	m_oRoot.m_pPrev = otherTail;
	list.m_iCount = 0;
	list.m_oRoot.m_pPrev = &list.m_oRoot;
	list.m_oRoot.m_pNext = &list.m_oRoot;
}

TNode* TGenericNodeList::Remove(TNode& node)
{
	node.SetList(nullptr);
	node.m_pPrev->m_pNext = node.m_pNext;
	node.m_pNext->m_pPrev = node.m_pPrev;
	node.m_pNext = &node;
	node.m_pPrev = &node;
	--m_iCount;
	return &node;
}

TNode* TGenericNodeList::RemoveHead()
{
	return IsEmpty() ? nullptr : Remove(*m_oRoot.m_pNext);
}

TNode* TGenericNodeList::RemoveTail()
{
	return IsEmpty() ? nullptr : Remove(*m_oRoot.m_pPrev);
}

void TGenericNodeList::RemoveAll()
{
	while (!IsEmpty())
		Remove(*m_oRoot.m_pNext);
}

void TGenericNodeList::Delete(TNode& node)
{
	Remove(node);
	delete &node;
}

void TGenericNodeList::DeleteHead()
{
	if (TNode* node = RemoveHead())
		delete node;
}

void TGenericNodeList::DeleteTail()
{
	if (TNode* node = RemoveTail())
		delete node;
}

void TGenericNodeList::DeleteAll()
{
	while (!IsEmpty())
		Delete(*m_oRoot.m_pNext);
}

TNode* TGenericNodeList::Head() const
{
	return m_oRoot.m_pNext;
}

TNode* TGenericNodeList::Tail() const
{
	return m_oRoot.m_pPrev;
}

Iterator TGenericNodeList::Begin() const
{
	return Iterator(m_oRoot.m_pNext);
}

Iterator TGenericNodeList::End() const
{
	return Iterator(const_cast<TNode*>(&m_oRoot));
}

Iterator TGenericNodeList::RBegin() const
{
	return Iterator(m_oRoot.m_pPrev);
}

Iterator TGenericNodeList::REnd() const
{
	return Iterator(const_cast<TNode*>(&m_oRoot));
}

Iterator TGenericNodeList::MakeIterator(TNode* node)
{
	return Iterator(node);
}

bool TGenericNodeList::IsEmpty() const
{
	return m_oRoot.m_pNext == &m_oRoot;
}

bool TGenericNodeList::IsValid(const TNode* node) const
{
	return node != nullptr && node != &m_oRoot;
}

int TGenericNodeList::Count() const
{
	return m_iCount;
}

} // namespace Toshi
