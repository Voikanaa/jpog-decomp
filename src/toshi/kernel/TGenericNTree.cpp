#include "toshi/kernel/TGenericNTree.h"

namespace Toshi
{

using TNode = TGenericNTree::TNode;

// The child slots continue past the declared one-element array in derived node types, so they
// are addressed through a pointer to its first element.

TNode& TNode::operator=(const TNode& other)
{
	m_pParent = other.m_pParent;
	m_apChildren[0] = other.m_apChildren[0];
	return *this;
}

TNode* TNode::Parent() const
{
	return m_pParent;
}

TNode* TNode::Child(int index) const
{
	return (&m_apChildren[0])[index];
}

int TNode::FindChild(const TNode* child, int childCount) const
{
	TNode* const* children = &m_apChildren[0];
	for (int i = 0; i < childCount; ++i)
		if (children[i] == child)
			return i;
	return -1;
}

void TNode::InsertChild(int index, TNode* child)
{
	(&m_apChildren[0])[index] = child;
	child->m_pParent = this;
}

void TNode::RemoveChild(int index)
{
	TNode** slot = &m_apChildren[0] + index;
	if (*slot != nullptr)
	{
		(*slot)->m_pParent = nullptr;
		*slot = nullptr;
	}
}

void TNode::RemoveChild(TNode* child, int childCount)
{
	const int index = FindChild(child, childCount);
	if (index >= 0)
		RemoveChild(index);
}

void TNode::Remove(int childCount)
{
	if (m_pParent != nullptr)
	{
		const int index = m_pParent->FindChild(this, childCount);
		if (index >= 0)
			m_pParent->RemoveChild(index);
	}
}

void TNode::Reset(int childCount)
{
	m_pParent = nullptr;
	TNode** children = &m_apChildren[0];
	for (int i = 0; i < childCount; ++i)
		children[i] = nullptr;
}

TGenericNTree::TGenericNTree() : m_pRoot(nullptr)
{
}

TGenericNTree& TGenericNTree::operator=(const TGenericNTree& other)
{
	m_pRoot = other.m_pRoot;
	return *this;
}

TNode* TGenericNTree::Root() const
{
	return m_pRoot;
}

void TGenericNTree::InsertRoot(TNode* node)
{
	m_pRoot = node;
	node->m_pParent = nullptr;
}

void TGenericNTree::RemoveRoot()
{
	m_pRoot = nullptr;
}

} // namespace Toshi
