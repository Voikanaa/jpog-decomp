#pragma once

#include "common/abi.h"
#include "common/layout.h"

namespace Toshi
{

template <typename T, int C> class TDList;

// Minimal intrusive circular doubly linked list (TKernelInterface.dll): nodes only hold their
// links (no owner, no count, no vtable). The list is a sentinel node; a "segment" is a chain
// of linked nodes from `first` to `last`.
class TGenericDList
{
public:
	class TNode
	{
		friend class TGenericDList;
		template <typename T, int C> friend class TDList;

	public:
		TNode(const TNode& other) = default; // implicit in the original, not exported
		TNode& operator=(const TNode& other);

	protected:
		enum TUninitialised
		{
			Uninitialised
		};

		// Unlinked: next and prev point at the node itself.
		TNode();
		// Leaves the links untouched.
		explicit TNode(TUninitialised);

		TNode* Next() const;
		TNode* Prev() const;
		bool IsLinked() const;
		void Reset();
		void Remove();
		// Links this node after/before `node`.
		void InsertAfter(TNode* node);
		void InsertBefore(TNode* node);

	private:
		TNode* m_pNext;
		TNode* m_pPrev;
	};

	TGenericDList& operator=(const TGenericDList& other);

protected:
	TGenericDList();
	// Unlinks every node (nodes are not deleted).
	~TGenericDList();

	void InsertHead(TNode* node);
	void InsertTail(TNode* node);
	void InsertSegmentAtHead(TNode* first, TNode* last);
	void InsertSegmentAtTail(TNode* first, TNode* last);
	void RemoveHead();
	void RemoveTail();
	void RemoveAll();
	TNode* Head() const;
	TNode* Tail() const;
	TNode* Begin() const;
	const TNode* End() const;
	TNode* RBegin() const;
	const TNode* REnd() const;
	bool IsEmpty() const;

	static void TSTDCALL InsertSegmentAfter(TNode* first, TNode* last, TNode* position);
	static void TSTDCALL InsertSegmentBefore(TNode* first, TNode* last, TNode* position);
	// Unlinks the segment and closes its ends onto themselves.
	static void TSTDCALL RemoveSegment(TNode* first, TNode* last);

private:
	TNode m_oRoot;
};

JPOG_ASSERT_SIZE(TGenericDList::TNode, 8);
JPOG_ASSERT_SIZE(TGenericDList, 8);

// Typed TGenericDList. T derives from TGenericDList::TNode; the original inlines this template
// (no exported members), so it is header-only here. C distinguishes lists of the same type.
template <typename T, int C = 0> class TDList : public TGenericDList
{
public:
	using Node = TGenericDList::TNode;

	static T* FromNode(Node* node) { return static_cast<T*>(node); }

	T* Head() const { return FromNode(TGenericDList::Head()); }
	T* Tail() const { return FromNode(TGenericDList::Tail()); }
	bool IsEnd(const Node* node) const { return node == TGenericDList::End(); }
	static T* NextOf(T* item) { return FromNode(static_cast<Node*>(item)->m_pNext); }
	static T* PrevOf(T* item) { return FromNode(static_cast<Node*>(item)->m_pPrev); }
	bool IsEmpty() const { return TGenericDList::IsEmpty(); }
	void InsertHead(T* item) { TGenericDList::InsertHead(item); }
	void InsertTail(T* item) { TGenericDList::InsertTail(item); }
	void RemoveAll() { TGenericDList::RemoveAll(); }
	static void Remove(T* item) { static_cast<Node*>(item)->Remove(); }
};

} // namespace Toshi
