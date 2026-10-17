#pragma once

#include "common/layout.h"

namespace Toshi
{

// Intrusive circular list kept sorted by node priority (TKernelInterface.dll). Equal priorities
// keep insertion order. The list itself is just the sentinel's two links (no priority field), so
// it is addressed as a node only through its links, as in the original.
class TGenericPriList
{
public:
	class TNode
	{
		friend class TGenericPriList;

	public:
		TNode& operator=(const TNode& other);

	protected:
		enum TUninitialised
		{
			Uninitialised
		};

		TNode();
		explicit TNode(TUninitialised);

		TNode* Next() const;
		TNode* Prev() const;
		bool IsLinked() const;
		int GetPriority() const;
		// Only sets the value; use TGenericPriList::SetPriority to re-sort a linked node.
		void SetPriority(int priority);
		void Reset();
		void Remove();

	private:
		TNode* m_pNext;
		TNode* m_pPrev;
		int m_iPriority;
	};

	TGenericPriList();
	~TGenericPriList();
	TGenericPriList& operator=(const TGenericPriList& other);

protected:
	// Inserts by the node's priority: negative priorities after equal ones searching from the
	// head, others after equal ones searching from the tail.
	void Insert(TNode* node);
	void Insert(TNode* node, int priority);
	// Forces the lowest (-32768) / highest (32767) priority.
	void InsertHead(TNode* node);
	void InsertTail(TNode* node);
	// Re-sorts a linked node.
	void SetPriority(TNode* node, int priority);
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

private:
	TNode* m_pHead; // the sentinel's links: m_pHead/m_pTail overlay TNode::m_pNext/m_pPrev
	TNode* m_pTail;
};

JPOG_ASSERT_SIZE(TGenericPriList::TNode, 12);
JPOG_ASSERT_SIZE(TGenericPriList, 8);

} // namespace Toshi
