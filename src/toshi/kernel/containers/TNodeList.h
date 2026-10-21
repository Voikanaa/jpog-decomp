#pragma once

namespace Toshi
{

// Intrusive circular list of polymorphic nodes (the original defines it inline; the renderer's
// adapter and mode lists use it). T derives from TNode after its vtable pointer, and the links
// point at the T objects. The root is a bare TNode: End() is the root seen as a T (it does not
// point at a real T and must only be compared).
template <class T> class TNodeList
{
public:
	class TNode
	{
	public:
		TNodeList* List() const { return m_pList; }
		T* Next() const { return m_pNext; }
		T* Prev() const { return m_pPrev; }

	private:
		friend class TNodeList;

		TNodeList* m_pList = nullptr;
		T* m_pNext = nullptr;
		T* m_pPrev = nullptr;
	};

	T* Begin() const { return m_oRoot.m_pNext; }
	T* End() const { return static_cast<T*>(const_cast<TNode*>(&m_oRoot)); }
	int Count() const { return m_iCount; }

private:
	TNode m_oRoot;
	int m_iCount = 0;
};

} // namespace Toshi
