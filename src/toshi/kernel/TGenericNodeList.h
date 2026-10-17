#pragma once

#include "common/layout.h"

namespace Toshi
{

// Intrusive, circular doubly linked list (TKernelInterface.dll). Nodes know their list; the list
// holds a sentinel node (m_oRoot) whose next/prev are the head/tail. Typed lists derive from it
// and expose the protected interface.
class TGenericNodeList
{
public:
	class TNode
	{
		friend class TGenericNodeList;

	public:
		TNode();
		// Copies the links as they are (the copy is not inserted anywhere), as in the original.
		TNode(const TNode& other);
		virtual ~TNode();
		TNode& operator=(const TNode& other);

		TNode* Next() const;
		TNode* Prev() const;
		TGenericNodeList* GetList() const;
		bool IsLinked() const;

	protected:
		// Sets the owning list and returns the previous one.
		TGenericNodeList* SetList(TGenericNodeList* list);

	private:
		TGenericNodeList* m_pList;
		TNode* m_pNext;
		TNode* m_pPrev;
	};

	class Iterator
	{
	public:
		Iterator();
		Iterator(TNode* node);
		Iterator(const Iterator& other);
		~Iterator();
		Iterator& operator=(const Iterator& other);

		bool IsNull();
		bool operator!();
		// The original takes non-const references here.
		bool operator==(Iterator& other) const;
		bool operator!=(Iterator& other) const;
		Iterator& operator++();
		Iterator operator++(int);
		Iterator& operator--();
		TNode& operator*() const;
		TNode* operator->() const;
		operator TNode*() const;

	private:
		TNode* m_pNode;
	};

protected:
	TGenericNodeList();
	// Deletes every node still in the list.
	~TGenericNodeList();

	void InsertHead(TNode& node);
	void InsertTail(TNode& node);
	void InsertBefore(TNode& position, TNode& node);
	void InsertAfter(TNode& position, TNode& node);
	// Moves all of `list`'s nodes to the end of this list.
	void Append(TGenericNodeList& list);
	TNode* Remove(TNode& node);
	TNode* RemoveHead();
	TNode* RemoveTail();
	void RemoveAll();
	// Remove, then destroy and free the node (virtual destructor, operator delete).
	void Delete(TNode& node);
	void DeleteHead();
	void DeleteTail();
	void DeleteAll();

	TNode* Head() const;
	TNode* Tail() const;
	// Begin/RBegin start at the head/tail; End and REnd are the sentinel.
	Iterator Begin() const;
	Iterator End() const;
	Iterator RBegin() const;
	Iterator REnd() const;
	Iterator MakeIterator(TNode* node);
	bool IsEmpty() const;
	// Not null and not the sentinel.
	bool IsValid(const TNode* node) const;
	int Count() const;

private:
	// Copying a list is disabled; the original's private copy constructor leaves the count unset
	// and its assignment does nothing.
	TGenericNodeList(TGenericNodeList& other);
	TGenericNodeList& operator=(TGenericNodeList& other);

	TNode m_oRoot;
	int m_iCount;
};

JPOG_ASSERT_SIZE(TGenericNodeList::TNode, 16);
JPOG_ASSERT_SIZE(TGenericNodeList, 20);

} // namespace Toshi
