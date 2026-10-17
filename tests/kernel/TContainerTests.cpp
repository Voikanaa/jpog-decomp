#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/TGenericDList.h"
#include "toshi/kernel/TGenericNodeList.h"

#include <array>
#include <doctest/doctest.h>
#include <vector>

using Toshi::TGenericDList;
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

	TEST_CASE("TGenericDList matches the original")
	{
		// Exposes the protected interface of the list and its nodes.
		struct TTestDList : TGenericDList
		{
			using TGenericDList::InsertHead;
			using TGenericDList::InsertSegmentAfter;
			using TGenericDList::InsertSegmentAtHead;
			using TGenericDList::InsertSegmentAtTail;
			using TGenericDList::InsertSegmentBefore;
			using TGenericDList::InsertTail;
			using TGenericDList::RemoveAll;
			using TGenericDList::RemoveHead;
			using TGenericDList::RemoveSegment;
			using TGenericDList::RemoveTail;
		};
		struct DNode
		{
			DNode* next;
			DNode* prev;
		};
		using P = void*;
		const auto listCtor = Original<Method<void>>("??0TGenericDList@Toshi@@IAE@XZ");
		const auto nodeCtor = Original<Method<void>>("??0TNode@TGenericDList@Toshi@@IAE@XZ");
		const auto insertHead = Original<Method<void, P>>("?InsertHead@TGenericDList@Toshi@@IAEXPAVTNode@12@@Z");
		const auto insertTail = Original<Method<void, P>>("?InsertTail@TGenericDList@Toshi@@IAEXPAVTNode@12@@Z");
		const auto removeHead = Original<Method<void>>("?RemoveHead@TGenericDList@Toshi@@IAEXXZ");
		const auto removeTail = Original<Method<void>>("?RemoveTail@TGenericDList@Toshi@@IAEXXZ");
		const auto removeAll = Original<Method<void>>("?RemoveAll@TGenericDList@Toshi@@IAEXXZ");
		const auto atHead =
		    Original<Method<void, P, P>>("?InsertSegmentAtHead@TGenericDList@Toshi@@IAEXPAVTNode@12@0@Z");
		const auto atTail =
		    Original<Method<void, P, P>>("?InsertSegmentAtTail@TGenericDList@Toshi@@IAEXPAVTNode@12@0@Z");
		using Segment3 = void(__stdcall*)(P, P, P);
		const auto after = Original<Segment3>("?InsertSegmentAfter@TGenericDList@Toshi@@KGXPAVTNode@12@00@Z");
		const auto before = Original<Segment3>("?InsertSegmentBefore@TGenericDList@Toshi@@KGXPAVTNode@12@00@Z");
		const auto removeSegment =
		    Original<void(__stdcall*)(P, P)>("?RemoveSegment@TGenericDList@Toshi@@KGXPAVTNode@12@0@Z");

		constexpr int kNodes = 10;
		// Index i < kNodes is a node, kNodes is the list sentinel.
		auto describe = [&](const DNode* base)
		{
			std::vector<int> links;
			for (int i = 0; i <= kNodes; ++i)
			{
				links.push_back(static_cast<int>(base[i].next - base));
				links.push_back(static_cast<int>(base[i].prev - base));
			}
			return links;
		};
		InputGenerator gen;
		for (int round = 0; round < 300; ++round)
		{
			// The list's sentinel is placed right after the nodes, so that all links are indices.
			std::array<DNode, kNodes + 1> original{};
			std::array<DNode, kNodes + 1> ours{};
			for (int i = 0; i < kNodes; ++i)
				nodeCtor(&original[i]);
			listCtor(&original[kNodes]);
			for (auto& node : ours)
				node = {&node, &node};
			auto* ourList = reinterpret_cast<TTestDList*>(&ours[kNodes]);
			auto* o = original.data();
			auto* m = ours.data();
			auto isLinked = [&](int i) { return m[i].next != &m[i]; };

			for (int step = 0; step < 40; ++step)
			{
				const int a = gen.NextInt(0, kNodes - 1);
				switch (gen.NextU32() % 6)
				{
				case 0:
					if (!isLinked(a))
					{
						(step & 1) ? insertHead(&o[kNodes], &o[a]) : insertTail(&o[kNodes], &o[a]);
						(step & 1) ? ourList->InsertHead(reinterpret_cast<TGenericDList::TNode*>(&m[a]))
						           : ourList->InsertTail(reinterpret_cast<TGenericDList::TNode*>(&m[a]));
					}
					break;
				case 1:
					(step & 1) ? removeHead(&o[kNodes]) : removeTail(&o[kNodes]);
					(step & 1) ? ourList->RemoveHead() : ourList->RemoveTail();
					break;
				case 2:
				{
					// Detach a segment of the list (two linked nodes in list order), then reinsert it.
					if (!isLinked(a))
						break;
					int last = a;
					if (m[last].next != &m[kNodes] && gen.NextU32() % 2)
						last = static_cast<int>(m[last].next - m);
					removeSegment(&o[a], &o[last]);
					TTestDList::RemoveSegment(reinterpret_cast<TGenericDList::TNode*>(&m[a]),
					                          reinterpret_cast<TGenericDList::TNode*>(&m[last]));
					// RemoveSegment only closes the outer ends; reconnect the segment's own ends
					// into a list by inserting it again.
					const int where = gen.NextInt(0, 3);
					if (where == 0)
					{
						atHead(&o[kNodes], &o[a], &o[last]);
						ourList->InsertSegmentAtHead(reinterpret_cast<TGenericDList::TNode*>(&m[a]),
						                             reinterpret_cast<TGenericDList::TNode*>(&m[last]));
					}
					else if (where == 1)
					{
						atTail(&o[kNodes], &o[a], &o[last]);
						ourList->InsertSegmentAtTail(reinterpret_cast<TGenericDList::TNode*>(&m[a]),
						                             reinterpret_cast<TGenericDList::TNode*>(&m[last]));
					}
					else
					{
						// Next to the sentinel or a node still in the list.
						int position = gen.NextInt(0, kNodes);
						if (position < kNodes && (!isLinked(position) || position == a || position == last))
							position = kNodes;
						auto* op = &o[position];
						auto* mp = reinterpret_cast<TGenericDList::TNode*>(&m[position]);
						if (where == 2)
						{
							after(&o[a], &o[last], op);
							TTestDList::InsertSegmentAfter(reinterpret_cast<TGenericDList::TNode*>(&m[a]),
							                               reinterpret_cast<TGenericDList::TNode*>(&m[last]), mp);
						}
						else
						{
							before(&o[a], &o[last], op);
							TTestDList::InsertSegmentBefore(reinterpret_cast<TGenericDList::TNode*>(&m[a]),
							                                reinterpret_cast<TGenericDList::TNode*>(&m[last]), mp);
						}
					}
					break;
				}
				case 3:
					if (step % 15 == 0)
					{
						removeAll(&o[kNodes]);
						ourList->RemoveAll();
					}
					break;
				default:
					break;
				}
				INFO("step ", step);
				CHECK(describe(o) == describe(m));
			}
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
