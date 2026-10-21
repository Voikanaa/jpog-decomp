#pragma once

#include "common/layout.h"

namespace Toshi
{

// N-ary tree with intrusive nodes (TKernelInterface.dll). A node holds its parent and a child
// array; the array's length is chosen by the typed tree that derives from it (the generic node
// declares one slot, and the child count is passed to the functions that scan the array).
class TGenericNTree
{
public:
	class TNode
	{
		friend class TGenericNTree;

	public:
		TNode& operator=(const TNode& other);

	protected:
		TNode* Parent() const;
		TNode* Child(int index) const;
		// Index of `child` among the first `childCount` slots, or -1.
		int FindChild(const TNode* child, int childCount) const;
		void InsertChild(int index, TNode* child);
		// Detaches the child (clearing its parent) if the slot is used.
		void RemoveChild(int index);
		void RemoveChild(TNode* child, int childCount);
		// Detaches this node from its parent.
		void Remove(int childCount);
		// Clears the parent and the first `childCount` child slots.
		void Reset(int childCount);

	private:
		TNode* m_pParent;
		TNode* m_apChildren[1]; // extended by derived node types
	};

	TGenericNTree& operator=(const TGenericNTree& other);

protected:
	TGenericNTree();
	TNode* Root() const;
	void InsertRoot(TNode* node);
	void RemoveRoot();

private:
	TNode* m_pRoot;
};

JPOG_ASSERT_SIZE(TGenericNTree::TNode, 8);
JPOG_ASSERT_SIZE(TGenericNTree, 4);

} // namespace Toshi
