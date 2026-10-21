#pragma once

namespace Toshi
{

// Intrusive circular doubly-linked list with an element count; the original defines it inline.
// Elements derive from TQList<T>::TNode; the list's root node marks the end.
template <class T> class TQList
{
public:
	class TNode
	{
	public:
		TNode() : m_pNext(this), m_pPrev(this) {}

		T* Next() const { return static_cast<T*>(m_pNext); }
		T* Prev() const { return static_cast<T*>(m_pPrev); }

		// Unlinks the node and makes it a list of its own.
		void Remove()
		{
			m_pPrev->m_pNext = m_pNext;
			m_pNext->m_pPrev = m_pPrev;
			m_pNext = this;
			m_pPrev = this;
		}

	private:
		friend class TQList;

		TNode* m_pNext;
		TNode* m_pPrev;
	};

	// A position in the list; End() is the root node.
	class Iterator
	{
	public:
		Iterator() : m_pNode(nullptr) {}
		explicit Iterator(TNode* node) : m_pNode(node) {}

		T* operator->() const { return static_cast<T*>(m_pNode); }
		T& operator*() const { return *static_cast<T*>(m_pNode); }
		T* Get() const { return static_cast<T*>(m_pNode); }
		Iterator& operator++()
		{
			m_pNode = m_pNode->m_pNext;
			return *this;
		}
		bool operator==(const Iterator& other) const { return m_pNode == other.m_pNode; }
		bool operator!=(const Iterator& other) const { return m_pNode != other.m_pNode; }

	private:
		TNode* m_pNode;
	};

	TQList() : m_iCount(0) {}

	Iterator Begin() const { return Iterator(m_oRoot.m_pNext); }
	Iterator End() const { return Iterator(const_cast<TNode*>(&m_oRoot)); }
	int Count() const { return m_iCount; }
	bool IsEmpty() const { return m_oRoot.m_pNext == &m_oRoot; }

	void PushBack(T* element)
	{
		TNode* node = element;
		node->m_pNext = &m_oRoot;
		node->m_pPrev = m_oRoot.m_pPrev;
		m_oRoot.m_pPrev->m_pNext = node;
		m_oRoot.m_pPrev = node;
		++m_iCount;
	}

	void Remove(T* element)
	{
		static_cast<TNode*>(element)->Remove();
		--m_iCount;
	}

	// Unlinks every element without destroying it.
	void UnlinkAll()
	{
		while (!IsEmpty())
			m_oRoot.m_pNext->Remove();
		m_iCount = 0;
	}

private:
	TNode m_oRoot;
	int m_iCount;
};

} // namespace Toshi
