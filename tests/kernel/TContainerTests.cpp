#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TGenericNodeList.h"

#include <array>
#include <doctest/doctest.h>
#include <vector>

using Toshi::TGenericNodeList;
using namespace jpog::test;

namespace
{

// Exposes the protected list interface, like the engine's typed lists do.
class TTestList : public TGenericNodeList
{
public:
	using TGenericNodeList::Append;
	using TGenericNodeList::Begin;
	using TGenericNodeList::Count;
	using TGenericNodeList::DeleteAll;
	using TGenericNodeList::End;
	using TGenericNodeList::Head;
	using TGenericNodeList::InsertAfter;
	using TGenericNodeList::InsertBefore;
	using TGenericNodeList::InsertHead;
	using TGenericNodeList::InsertTail;
	using TGenericNodeList::IsEmpty;
	using TGenericNodeList::IsValid;
	using TGenericNodeList::Remove;
	using TGenericNodeList::RemoveAll;
	using TGenericNodeList::RemoveHead;
	using TGenericNodeList::RemoveTail;
	using TGenericNodeList::Tail;
	TTestList() = default;
	~TTestList() { RemoveAll(); } // the nodes below are not heap objects
};

int s_destroyed = 0;

struct TCountedNode : TGenericNodeList::TNode
{
	~TCountedNode() override { ++s_destroyed; }
};

} // namespace

// ---------------------------------------------------------------------------------------------
// Unit tests (all platforms)
// ---------------------------------------------------------------------------------------------

TEST_SUITE("TContainers")
{
	TEST_CASE("TGenericNodeList basics")
	{
		TTestList list;
		TGenericNodeList::TNode a, b, c;
		list.InsertTail(a);
		list.InsertTail(c);
		list.InsertBefore(c, b);
		CHECK(list.Count() == 3);
		CHECK(list.Head() == &a);
		CHECK(list.Tail() == &c);
		CHECK(a.Next() == &b);
		CHECK(b.GetList() == &list);
		int visited = 0;
		for (auto it = list.Begin(), end = list.End(); it != end; ++it)
			++visited;
		CHECK(visited == 3);
		CHECK(list.RemoveHead() == &a);
		CHECK_FALSE(a.IsLinked());
		CHECK(a.Next() == &a);

		TTestList other;
		other.InsertTail(a);
		list.Append(other);
		CHECK(other.IsEmpty());
		CHECK(list.Tail() == &a);
		CHECK(a.GetList() == &list);
		list.RemoveAll();
		CHECK(list.IsEmpty());
		CHECK(list.Count() == 0);
	}

	TEST_CASE("TGenericNodeList deletes its nodes")
	{
		s_destroyed = 0;
		{
			class TOwningList : public TGenericNodeList
			{
			public:
				using TGenericNodeList::InsertTail;
			} list;
			list.InsertTail(*new TCountedNode());
			list.InsertTail(*new TCountedNode());
		}
		CHECK(s_destroyed == 2);
	}
}

// ---------------------------------------------------------------------------------------------
// Differential tests against the original TKernelInterface.dll (x86 only)
// ---------------------------------------------------------------------------------------------

#if JPOG_HAS_ORIGINAL_MODULES

namespace
{

struct NodeView
{
	void* vtable;
	void* list;
	NodeView* next;
	NodeView* prev;
};

struct ListView
{
	NodeView root;
	int count;
};

// Forward and backward node order (as indices into `nodes`; -1 for unknown) plus the count.
std::vector<int> Describe(const ListView& list, const NodeView* nodes, int nodeCount)
{
	auto index = [&](const NodeView* node)
	{
		for (int i = 0; i < nodeCount; ++i)
			if (&nodes[i] == node)
				return i;
		return -1;
	};
	std::vector<int> result{list.count};
	for (const NodeView* node = list.root.next; node != &list.root && result.size() < 64; node = node->next)
		result.push_back(index(node));
	result.push_back(-100);
	for (const NodeView* node = list.root.prev; node != &list.root && result.size() < 128; node = node->prev)
		result.push_back(index(node));
	return result;
}

} // namespace

TEST_SUITE("TContainers differential" * doctest::skip(KernelMissing()))
{
	TEST_CASE("TGenericNodeList matches the original")
	{
		using Node = void*;
		const auto nodeCtor = Original<Method<void>>("??0TNode@TGenericNodeList@Toshi@@QAE@XZ");
		const auto listCtor = Original<Method<void>>("??0TGenericNodeList@Toshi@@IAE@XZ");
		const auto insertHead = Original<Method<void, Node>>("?InsertHead@TGenericNodeList@Toshi@@IAEXAAVTNode@12@@Z");
		const auto insertTail = Original<Method<void, Node>>("?InsertTail@TGenericNodeList@Toshi@@IAEXAAVTNode@12@@Z");
		const auto insertBefore =
		    Original<Method<void, Node, Node>>("?InsertBefore@TGenericNodeList@Toshi@@IAEXAAVTNode@12@0@Z");
		const auto insertAfter =
		    Original<Method<void, Node, Node>>("?InsertAfter@TGenericNodeList@Toshi@@IAEXAAVTNode@12@0@Z");
		const auto remove = Original<Method<Node, Node>>("?Remove@TGenericNodeList@Toshi@@IAEPAVTNode@12@AAV312@@Z");
		const auto removeHead = Original<Method<Node>>("?RemoveHead@TGenericNodeList@Toshi@@IAEPAVTNode@12@XZ");
		const auto removeTail = Original<Method<Node>>("?RemoveTail@TGenericNodeList@Toshi@@IAEPAVTNode@12@XZ");
		const auto append = Original<Method<void, void*>>("?Append@TGenericNodeList@Toshi@@IAEXAAV12@@Z");
		const auto removeAll = Original<Method<void>>("?RemoveAll@TGenericNodeList@Toshi@@IAEXXZ");

		constexpr int kNodes = 12;
		InputGenerator gen;
		for (int round = 0; round < 200; ++round)
		{
			std::array<NodeView, kNodes> originalNodes{};
			std::array<ListView, 2> originalLists{};
			for (auto& node : originalNodes)
				nodeCtor(&node);
			for (auto& list : originalLists)
				listCtor(&list);
			std::array<TGenericNodeList::TNode, kNodes> ourNodes;
			std::array<TTestList, 2> ourLists;
			const auto* ourNodeViews = reinterpret_cast<const NodeView*>(ourNodes.data());

			for (int step = 0; step < 60; ++step)
			{
				const int which = gen.NextInt(0, 1);
				const int n = gen.NextInt(0, kNodes - 1);
				const bool linked = ourNodes[n].IsLinked();
				// Positions must be nodes of the same list (or its sentinel).
				const int position = gen.NextInt(0, kNodes - 1);
				const bool positionInList = ourNodes[position].GetList() == &ourLists[which];
				switch (gen.NextU32() % 7)
				{
				case 0:
					if (!linked)
					{
						insertHead(&originalLists[which], &originalNodes[n]);
						ourLists[which].InsertHead(ourNodes[n]);
					}
					break;
				case 1:
					if (!linked)
					{
						insertTail(&originalLists[which], &originalNodes[n]);
						ourLists[which].InsertTail(ourNodes[n]);
					}
					break;
				case 2:
					if (!linked && positionInList)
					{
						if (gen.NextU32() & 1)
						{
							insertBefore(&originalLists[which], &originalNodes[position], &originalNodes[n]);
							ourLists[which].InsertBefore(ourNodes[position], ourNodes[n]);
						}
						else
						{
							insertAfter(&originalLists[which], &originalNodes[position], &originalNodes[n]);
							ourLists[which].InsertAfter(ourNodes[position], ourNodes[n]);
						}
					}
					break;
				case 3:
					if (ourNodes[n].GetList() == &ourLists[which])
					{
						CHECK(remove(&originalLists[which], &originalNodes[n]) == &originalNodes[n]);
						ourLists[which].Remove(ourNodes[n]);
					}
					break;
				case 4:
				{
					const Node removed =
					    (step & 1) ? removeHead(&originalLists[which]) : removeTail(&originalLists[which]);
					auto* ourRemoved = (step & 1) ? ourLists[which].RemoveHead() : ourLists[which].RemoveTail();
					CHECK((removed == nullptr) == (ourRemoved == nullptr));
					break;
				}
				case 5:
					append(&originalLists[which], &originalLists[1 - which]);
					ourLists[which].Append(ourLists[1 - which]);
					break;
				default:
					if (step % 10 == 0)
					{
						removeAll(&originalLists[which]);
						ourLists[which].RemoveAll();
					}
					break;
				}
				for (int l = 0; l < 2; ++l)
				{
					INFO("list ", l, " step ", step);
					CHECK(Describe(originalLists[l], originalNodes.data(), kNodes) ==
					      Describe(*reinterpret_cast<const ListView*>(&ourLists[l]), ourNodeViews, kNodes));
				}
				for (int i = 0; i < kNodes; ++i)
				{
					const int originalOwner =
					    originalNodes[i].list == nullptr ? -1 : (originalNodes[i].list == &originalLists[0] ? 0 : 1);
					const int ourOwner =
					    ourNodes[i].GetList() == nullptr ? -1 : (ourNodes[i].GetList() == &ourLists[0] ? 0 : 1);
					CHECK(originalOwner == ourOwner);
				}
			}
			for (auto& list : originalLists)
				removeAll(&list);
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
