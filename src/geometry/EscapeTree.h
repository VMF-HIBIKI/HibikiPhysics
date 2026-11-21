#pragma once

#include "SpatialTree.h"

namespace HibikiPhysics::Detail
{
// Preorder children use index + 1 and mRight. Each escape is the first index
// after that node's subtree, including the one-past-end sentinel.
class EscapeTree
{
  public:
	explicit EscapeTree(std::vector<BvhNode> nodes) : mNodes(std::move(nodes)), mEscape(mNodes.size(), mNodes.size())
	{
		for (std::size_t i = 0; i < mNodes.size(); ++i)
			if (!mNodes[i].IsLeaf())
			{
				mEscape[i + 1] = mNodes[i].mRight;
				mEscape[mNodes[i].mRight] = mEscape[i];
			}
	}

	std::span<const BvhNode> Nodes() const { return mNodes; }

	// Starting immediately after a source leaf walks only the remaining forest.
	template <class AcceptNode, class EmitLeaf>
	std::size_t Walk(std::size_t first, AcceptNode accept, EmitLeaf emit) const
	{
		std::size_t visited = 0;
		for (auto i = first; i < mNodes.size();)
		{
			const auto &node = mNodes[i];
			++visited;
			if (!accept(i, node))
				i = mEscape[i];
			else
			{
				if (node.IsLeaf())
					emit(i, node);
				++i;
			}
		}
		return visited;
	}

  private:
	std::vector<BvhNode>	 mNodes;
	std::vector<std::size_t> mEscape;
};
} // namespace HibikiPhysics::Detail
