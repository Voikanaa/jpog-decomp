#pragma once

namespace Toshi
{

// Intrusive tree (TKernelInterface.dll; the original defines it inline). Each T derives from
// TNodeTree<T>::TNode; siblings form a circular list and a parent points at its first child.
// Links hold T pointers, as in the original. The tree's root node is not inside a T, but other
// nodes still refer to it as a T pointer (Root()); converting that back with Node() gives the
// root node again, which is how the original's code works too.
template <typename T> class TNodeTree
{
public:
	class TNode
	{
		friend class TNodeTree;

	public:
		TNode() : m_pTree(nullptr), m_pNext(Self()), m_pPrev(Self()), m_pParent(nullptr), m_pAttached(nullptr) {}

		TNodeTree* Tree() const { return m_pTree; }
		T* Next() const { return m_pNext; }
		T* Prev() const { return m_pPrev; }
		T* Parent() const { return m_pParent; }
		// The first child.
		T* Attached() const { return m_pAttached; }

	private:
		T* Self() { return static_cast<T*>(this); }

		TNodeTree* m_pTree;
		T* m_pNext;
		T* m_pPrev;
		T* m_pParent;
		T* m_pAttached;
	};

	TNodeTree() : m_iCount(0)
	{
		m_oRoot.m_pNext = Root();
		m_oRoot.m_pPrev = Root();
	}

	// The root as the T pointer the nodes store (it does not point at a real T).
	T* Root() { return static_cast<T*>(&m_oRoot); }
	static TNode* Node(T* node) { return static_cast<TNode*>(node); }
	// The first top-level node.
	T* Attached() const { return m_oRoot.m_pAttached; }
	int Count() const { return m_iCount; }

	// Unlinks `node` (it must be in this tree, or in none). With `reattachChildren` its children
	// move up to its parent first.
	T* Remove(T* node, bool reattachChildren)
	{
		TNode* entry = Node(node);
		if (entry->m_pTree != nullptr)
		{
			if (entry->m_pTree != this)
				return node;
			--m_iCount;
		}
		if (reattachChildren)
		{
			for (T* child = entry->m_pAttached; child != nullptr; child = entry->m_pAttached)
			{
				T* parent = entry->m_pParent;
				if (parent == Root())
					parent = nullptr;
				Remove(child, false);
				InsertTail(parent == nullptr ? Root() : parent, child);
			}
		}
		Unlink(node);
		return node;
	}

	// Makes `node` the first top-level node (inserted before the previous first).
	void InsertHead(T* node)
	{
		Remove(node, false);
		T* first = m_oRoot.m_pAttached;
		if (first != nullptr)
			LinkBefore(first, node);
		++m_iCount;
		Node(node)->m_pParent = Root();
		m_oRoot.m_pAttached = node;
		Node(node)->m_pTree = this;
	}

	// Appends `node` to the children of `parent` (Root() for the top level).
	void InsertTail(T* parent, T* node)
	{
		Remove(node, false);
		TNode* parentNode = Node(parent);
		T* first = parentNode->m_pAttached;
		if (first == nullptr)
			parentNode->m_pAttached = node;
		else
		{
			Node(node)->m_pTree = this;
			LinkBefore(first, node);
		}
		++m_iCount;
		Node(node)->m_pParent = parent;
		Node(node)->m_pTree = this;
	}

	// Deletes `node`, its siblings and all their children.
	void DeleteRecurse(T* node)
	{
		while (node != nullptr)
		{
			T* next = Node(node)->m_pNext;
			if (next == node)
				next = nullptr;
			if (Node(node)->m_pAttached != nullptr)
				DeleteRecurse(Node(node)->m_pAttached);
			Remove(node, false);
			delete node;
			node = next;
		}
	}

	// Unlinking without a tree (the original also calls it through a null tree pointer).
	static void Unlink(T* node)
	{
		TNode* entry = Node(node);
		if (T* parent = entry->m_pParent; parent != nullptr)
		{
			TNode* parentNode = Node(parent);
			if (parentNode->m_pAttached == node)
				parentNode->m_pAttached = entry->m_pNext == node ? nullptr : entry->m_pNext;
			entry->m_pParent = nullptr;
		}
		Node(entry->m_pPrev)->m_pNext = entry->m_pNext;
		Node(entry->m_pNext)->m_pPrev = entry->m_pPrev;
		entry->m_pNext = node;
		entry->m_pPrev = node;
		entry->m_pTree = nullptr;
	}

private:
	// Links `node` before `first` in its sibling ring, taking its parent.
	static void LinkBefore(T* first, T* node)
	{
		TNode* entry = Node(node);
		TNode* firstNode = Node(first);
		entry->m_pNext = first;
		entry->m_pPrev = firstNode->m_pPrev;
		Node(firstNode->m_pPrev)->m_pNext = node;
		firstNode->m_pPrev = node;
		entry->m_pParent = firstNode->m_pParent;
	}

	TNode m_oRoot;
	int m_iCount;
};

} // namespace Toshi
