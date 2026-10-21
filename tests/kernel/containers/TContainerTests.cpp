#include "support/Differential.h"
#include "support/FloatCompare.h"
#include "support/OriginalModule.h"
#include "toshi/kernel/containers/TBitArray.h"
#include "toshi/kernel/containers/TGenericDList.h"
#include "toshi/kernel/containers/TGenericNTree.h"
#include "toshi/kernel/containers/TGenericNodeList.h"
#include "toshi/kernel/containers/TGenericPriList.h"

#include <array>
#include <doctest/doctest.h>
#include <new>
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

	TEST_CASE("TGenericPriList matches the original")
	{
		struct TTestPriList : Toshi::TGenericPriList
		{
			using TGenericPriList::Insert;
			using TGenericPriList::InsertHead;
			using TGenericPriList::InsertTail;
			using TGenericPriList::RemoveAll;
			using TGenericPriList::RemoveHead;
			using TGenericPriList::RemoveTail;
			using TGenericPriList::SetPriority;
		};
		struct PNodeType : Toshi::TGenericPriList::TNode
		{
			PNodeType() = default; // the protected TNode() constructor
		};
		struct PNode
		{
			PNode* next;
			PNode* prev;
			int priority;
		};
		using P = void*;
		const auto listCtor = Original<Method<void>>("??0TGenericPriList@Toshi@@QAE@XZ");
		const auto nodeCtor = Original<Method<void>>("??0TNode@TGenericPriList@Toshi@@IAE@XZ");
		const auto insert = Original<Method<void, P, int>>("?Insert@TGenericPriList@Toshi@@IAEXPAVTNode@12@H@Z");
		const auto insertHead = Original<Method<void, P>>("?InsertHead@TGenericPriList@Toshi@@IAEXPAVTNode@12@@Z");
		const auto insertTail = Original<Method<void, P>>("?InsertTail@TGenericPriList@Toshi@@IAEXPAVTNode@12@@Z");
		const auto setPriority =
		    Original<Method<void, P, int>>("?SetPriority@TGenericPriList@Toshi@@IAEXPAVTNode@12@H@Z");
		const auto removeHead = Original<Method<void>>("?RemoveHead@TGenericPriList@Toshi@@IAEXXZ");
		const auto removeTail = Original<Method<void>>("?RemoveTail@TGenericPriList@Toshi@@IAEXXZ");
		const auto removeAll = Original<Method<void>>("?RemoveAll@TGenericPriList@Toshi@@IAEXXZ");
		const auto nodeRemove = Original<Method<void>>("?Remove@TNode@TGenericPriList@Toshi@@IAEXXZ");

		constexpr int kNodes = 10;
		auto describe = [&](const PNode* base)
		{
			std::vector<int> state;
			for (int i = 0; i <= kNodes; ++i)
			{
				state.push_back(static_cast<int>(base[i].next - base));
				state.push_back(static_cast<int>(base[i].prev - base));
				if (i < kNodes)
					state.push_back(base[i].priority);
			}
			return state;
		};
		InputGenerator gen;
		for (int round = 0; round < 300; ++round)
		{
			// The 8-byte list occupies the slot after the nodes, so links are array indices.
			std::array<PNode, kNodes + 1> original{};
			std::array<PNode, kNodes + 1> ours{};
			for (int i = 0; i < kNodes; ++i)
				nodeCtor(&original[i]);
			listCtor(&original[kNodes]);
			for (int i = 0; i < kNodes; ++i)
				new (&ours[i]) PNodeType();
			auto* ourList = new (&ours[kNodes]) TTestPriList();
			auto* o = original.data();
			auto* m = ours.data();
			auto node = [&](int i)
			{ return static_cast<Toshi::TGenericPriList::TNode*>(reinterpret_cast<PNodeType*>(&m[i])); };
			auto isLinked = [&](int i) { return m[i].next != &m[i]; };

			for (int step = 0; step < 40; ++step)
			{
				const int a = gen.NextInt(0, kNodes - 1);
				const int priority = gen.NextInt(-3, 3);
				switch (gen.NextU32() % 6)
				{
				case 0:
				case 1:
					if (!isLinked(a))
					{
						insert(&o[kNodes], &o[a], priority);
						ourList->Insert(node(a), priority);
					}
					break;
				case 2:
					if (!isLinked(a))
					{
						(step & 1) ? insertHead(&o[kNodes], &o[a]) : insertTail(&o[kNodes], &o[a]);
						(step & 1) ? ourList->InsertHead(node(a)) : ourList->InsertTail(node(a));
					}
					break;
				case 3:
					if (isLinked(a))
					{
						setPriority(&o[kNodes], &o[a], priority);
						ourList->SetPriority(node(a), priority);
					}
					break;
				case 4:
					if (isLinked(a) && (step & 1))
					{
						nodeRemove(&o[a]);
						m[a].prev->next = m[a].next; // TNode::Remove is protected; same unlink
						m[a].next->prev = m[a].prev;
						m[a].next = m[a].prev = &m[a];
					}
					else
					{
						(step & 2) ? removeHead(&o[kNodes]) : removeTail(&o[kNodes]);
						(step & 2) ? ourList->RemoveHead() : ourList->RemoveTail();
					}
					break;
				default:
					if (step % 12 == 0)
					{
						removeAll(&o[kNodes]);
						ourList->RemoveAll();
					}
					break;
				}
				INFO("step ", step);
				CHECK(describe(o) == describe(m));
			}
		}
	}

	TEST_CASE("TBitArray matches the original")
	{
		using Toshi::TBitArray;
		struct BitView
		{
			unsigned int* words;
			int numBits;
			unsigned int initial;
		};
		const auto construct = Original<Method<void, int, int>>("??0TBitArray@Toshi@@QAE@HH@Z");
		const auto destruct = Original<Method<void>>("??1TBitArray@Toshi@@QAE@XZ");
		const auto getBit = Original<Method<int, int>>("?GetBit@TBitArray@Toshi@@QAEHH@Z");
		const auto setBit = Original<Method<void, int>>("?SetBit@TBitArray@Toshi@@QAEXH@Z");
		const auto setBitValue = Original<Method<void, int, int>>("?SetBit@TBitArray@Toshi@@QAEXHH@Z");
		const auto clearBit = Original<Method<void, int>>("?ClearBit@TBitArray@Toshi@@QAEXH@Z");
		const auto toggleBit = Original<Method<void, int>>("?ToggleBit@TBitArray@Toshi@@QAEXH@Z");
		const auto reset = Original<Method<void>>("?Reset@TBitArray@Toshi@@QAEXXZ");
		const auto firstSet = Original<Method<int>>("?FindFirstSetBit@TBitArray@Toshi@@QAEHXZ");
		const auto firstClear = Original<Method<int>>("?FindFirstClearBit@TBitArray@Toshi@@QAEHXZ");

		InputGenerator gen;
		for (int round = 0; round < 300; ++round)
		{
			const int numBits = gen.NextInt(1, 100);
			const int initial = gen.NextInt(0, 1) * gen.NextInt(1, 5);
			BitView original;
			construct(&original, numBits, initial);
			TBitArray ours(numBits, initial);
			const auto& view = reinterpret_cast<const BitView&>(ours);
			for (int step = 0; step < 50; ++step)
			{
				const int bit = gen.NextInt(0, numBits - 1);
				switch (gen.NextU32() % 6)
				{
				case 0:
					setBit(&original, bit);
					ours.SetBit(bit);
					break;
				case 1:
				{
					const int value = gen.NextInt(0, 2);
					setBitValue(&original, bit, value);
					ours.SetBit(bit, value);
					break;
				}
				case 2:
					clearBit(&original, bit);
					ours.ClearBit(bit);
					break;
				case 3:
					toggleBit(&original, bit);
					ours.ToggleBit(bit);
					break;
				case 4:
					if (step % 10 == 0)
					{
						reset(&original);
						ours.Reset();
					}
					break;
				default:
					CHECK(getBit(&original, bit) == ours.GetBit(bit));
					break;
				}
				REQUIRE(original.numBits == view.numBits);
				CHECK(original.initial == view.initial);
				for (int w = 0; w < (numBits + 31) / 32; ++w)
					CHECK(original.words[w] == view.words[w]);
				CHECK(firstSet(&original) == ours.FindFirstSetBit());
				CHECK(firstClear(&original) == ours.FindFirstClearBit());
			}
			destruct(&original);
		}
	}

	TEST_CASE("TGenericNTree matches the original")
	{
		constexpr int kChildren = 4;
		constexpr int kNodes = 9;
		struct TreeNode
		{
			TreeNode* parent;
			TreeNode* children[kChildren];
		};
		struct TTestNode : Toshi::TGenericNTree::TNode
		{
			using TNode::FindChild;
			using TNode::InsertChild;
			using TNode::Remove;
			using TNode::RemoveChild;
			using TNode::Reset;
		};
		using P = void*;
		const auto reset = Original<Method<void, int>>("?Reset@TNode@TGenericNTree@Toshi@@IAEXH@Z");
		const auto insertChild =
		    Original<Method<void, int, P>>("?InsertChild@TNode@TGenericNTree@Toshi@@IAEXHPAV123@@Z");
		const auto removeChildAt = Original<Method<void, int>>("?RemoveChild@TNode@TGenericNTree@Toshi@@IAEXH@Z");
		const auto removeChild =
		    Original<Method<void, P, int>>("?RemoveChild@TNode@TGenericNTree@Toshi@@IAEXPAV123@H@Z");
		const auto remove = Original<Method<void, int>>("?Remove@TNode@TGenericNTree@Toshi@@IAEXH@Z");
		const auto findChild = Original<Method<int, P, int>>("?FindChild@TNode@TGenericNTree@Toshi@@IBEHPBV123@H@Z");

		auto describe = [&](const TreeNode* base)
		{
			auto index = [&](const TreeNode* node) { return node == nullptr ? -1 : static_cast<int>(node - base); };
			std::vector<int> state;
			for (int i = 0; i < kNodes; ++i)
			{
				state.push_back(index(base[i].parent));
				for (const TreeNode* child : base[i].children)
					state.push_back(index(child));
			}
			return state;
		};
		InputGenerator gen;
		for (int round = 0; round < 300; ++round)
		{
			std::array<TreeNode, kNodes> original{};
			std::array<TreeNode, kNodes> ours{};
			auto ourNode = [&](int i) { return reinterpret_cast<TTestNode*>(&ours[i]); };
			for (int i = 0; i < kNodes; ++i)
			{
				reset(&original[i], kChildren);
				ourNode(i)->Reset(kChildren);
			}
			for (int step = 0; step < 40; ++step)
			{
				const int a = gen.NextInt(0, kNodes - 1);
				const int b = gen.NextInt(0, kNodes - 1);
				const int slot = gen.NextInt(0, kChildren - 1);
				switch (gen.NextU32() % 5)
				{
				case 0:
					if (a != b && original[b].parent == nullptr && original[a].children[slot] == nullptr)
					{
						insertChild(&original[a], slot, &original[b]);
						ourNode(a)->InsertChild(slot, ourNode(b));
					}
					break;
				case 1:
					removeChildAt(&original[a], slot);
					ourNode(a)->RemoveChild(slot);
					break;
				case 2:
					removeChild(&original[a], &original[b], kChildren);
					ourNode(a)->RemoveChild(ourNode(b), kChildren);
					break;
				case 3:
					remove(&original[a], kChildren);
					ourNode(a)->Remove(kChildren);
					break;
				default:
					CHECK(findChild(&original[a], &original[b], kChildren) ==
					      ourNode(a)->FindChild(ourNode(b), kChildren));
					break;
				}
				INFO("step ", step);
				CHECK(describe(original.data()) == describe(ours.data()));
			}
		}
	}
}

#endif // JPOG_HAS_ORIGINAL_MODULES
