#include <HibikiPhysics/FourWaySpatialIndex.h>
#include "SpatialUpdate.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <set>
#include <stdexcept>
#include <unordered_map>

namespace HibikiPhysics
{
namespace
{
using Point = std::array<double, 3>;
using Pairs = std::vector<std::pair<std::size_t, std::size_t>>;

void RequireFinite(const AABB &bounds)
{
	const auto lo = bounds.GetMin(), hi = bounds.GetMax();
	for (std::size_t axis = 0; axis < 3; ++axis)
		if (!std::isfinite(lo[axis]) || !std::isfinite(hi[axis]))
			throw std::invalid_argument("Four-way spatial index requires finite bounds");
}

bool Intersects(const AABB &a, const AABB &b)
{
	const auto alo = a.GetMin(), ahi = a.GetMax(), blo = b.GetMin(), bhi = b.GetMax();
	for (std::size_t axis = 0; axis < 3; ++axis)
		if (ahi[axis] < blo[axis] || bhi[axis] < alo[axis])
			return false;
	return true;
}

struct BuildEntry
{
	std::size_t mId;
	AABB		mBounds;
	Point		mCenter;
};

// Two median partitions form four groups. Each half chooses its own widest
// center axis, so the second cut can separate objects along a different axis.
std::pair<std::span<BuildEntry>, std::span<BuildEntry>> Split(std::span<BuildEntry> entries)
{
	Point lo = entries.front().mCenter, hi = lo;
	for (const auto &entry : entries.subspan(1))
		for (std::size_t axis = 0; axis < 3; ++axis)
		{
			lo[axis] = std::min(lo[axis], entry.mCenter[axis]);
			hi[axis] = std::max(hi[axis], entry.mCenter[axis]);
		}
	Point widths{hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]};
	if (std::isinf(*std::ranges::max_element(widths)))
		for (std::size_t axis = 0; axis < 3; ++axis)
			widths[axis] = hi[axis] * 0.5 - lo[axis] * 0.5;
	const auto axis = static_cast<std::size_t>(std::ranges::max_element(widths) - widths.begin());
	const auto middle = entries.size() / 2;
	std::ranges::nth_element(entries, entries.begin() + static_cast<std::ptrdiff_t>(middle), {}, [axis](const BuildEntry &entry)
							 { return std::pair(entry.mCenter[axis], entry.mId); });
	return {entries.first(middle), entries.subspan(middle)};
}

class FourWayProvider final : public SpatialOverlapProvider
{
  public:
	std::unique_ptr<const SpatialOverlapIndex> Build(std::span<const SpatialEntry> entries) const override
	{
		return std::make_unique<FourWaySpatialIndex>(entries);
	}
};
} // namespace

namespace Detail
{
struct FourWayTree
{
	// Siblings occupy consecutive slots. All descendants follow their parent.
	// A leaf stores the caller's full ID in the same field used for a branch offset.
	struct Node
	{
		AABB		mBounds{{}, {}};
		std::size_t mFirstChildOrId = 0;
		std::size_t mChildCount = 0;
		bool		IsLeaf() const { return mChildCount == 0; }
	};
	std::vector<Node> mNodes;

	explicit FourWayTree(std::span<const SpatialEntry> input)
	{
		if (input.empty())
			return;
		std::vector<BuildEntry> entries;
		entries.reserve(input.size());
		for (const auto &entry : input)
		{
			RequireFinite(entry.mBounds);
			const auto lo = entry.mBounds.GetMin(), hi = entry.mBounds.GetMax();
			entries.push_back({entry.mId, entry.mBounds, {std::midpoint(lo[0], hi[0]), std::midpoint(lo[1], hi[1]), std::midpoint(lo[2], hi[2])}});
		}
		std::ranges::sort(entries, {}, &BuildEntry::mId);
		if (std::ranges::adjacent_find(entries, {}, &BuildEntry::mId) != entries.end())
			throw std::invalid_argument("Duplicate spatial entry ID");
		mNodes.reserve(input.size());
		mNodes.emplace_back();
		Build(0, entries);
	}

	std::span<const Node> Children(const Node &node) const
	{
		return std::span(mNodes).subspan(node.mFirstChildOrId, node.mChildCount);
	}

	void EncloseChildren(Node &node)
	{
		const auto children = Children(node);
		auto	   lo = children.front().mBounds.GetMin(), hi = children.front().mBounds.GetMax();
		for (const auto &child : children.subspan(1))
		{
			const auto childLo = child.mBounds.GetMin(), childHi = child.mBounds.GetMax();
			for (std::size_t axis = 0; axis < 3; ++axis)
			{
				lo[axis] = std::min(lo[axis], childLo[axis]);
				hi[axis] = std::max(hi[axis], childHi[axis]);
			}
		}
		node.mBounds = {lo, hi};
	}

	void Build(std::size_t index, std::span<BuildEntry> entries)
	{
		if (entries.size() == 1)
		{
			mNodes[index] = {entries.front().mBounds, entries.front().mId, 0};
			return;
		}
		std::array<std::span<BuildEntry>, 4> groups;
		const auto							 count = std::min(entries.size(), groups.size());
		if (entries.size() <= groups.size())
		{
			std::ranges::sort(entries, {}, &BuildEntry::mId);
			for (std::size_t i = 0; i < count; ++i)
				groups[i] = entries.subspan(i, 1);
		}
		else
		{
			const auto [left, right] = Split(entries);
			const auto [a, b] = Split(left);
			const auto [c, d] = Split(right);
			groups = {a, b, c, d};
		}
		const auto first = mNodes.size();
		mNodes[index].mFirstChildOrId = first;
		mNodes[index].mChildCount = count;
		for (std::size_t i = 0; i < count; ++i)
			mNodes.emplace_back();
		for (std::size_t i = 0; i < count; ++i)
			Build(first + i, groups[i]);
		EncloseChildren(mNodes[index]);
	}

	void VisitBox(const Node &node, const AABB &bounds, std::vector<std::size_t> &ids, std::size_t &visits) const
	{
		++visits;
		if (!Intersects(node.mBounds, bounds))
			return;
		if (node.IsLeaf())
			ids.push_back(node.mFirstChildOrId);
		else
			for (const auto &child : Children(node))
				VisitBox(child, bounds, ids, visits);
	}

	void Cross(const Node &a, const Node &b, Pairs &pairs, std::size_t &visits) const
	{
		++visits;
		if (!Intersects(a.mBounds, b.mBounds))
			return;
		if (a.IsLeaf() && b.IsLeaf())
		{
			pairs.emplace_back(std::minmax(a.mFirstChildOrId, b.mFirstChildOrId));
			return;
		}
		const Node &branch = a.IsLeaf() ? b : a;
		const Node &other = a.IsLeaf() ? a : b;
		for (const auto &child : Children(branch))
			Cross(other, child, pairs, visits);
	}

	// Every pair separates into different children at one lowest common ancestor.
	void Within(const Node &node, Pairs &pairs, std::size_t &visits) const
	{
		const auto children = Children(node);
		for (std::size_t i = 0; i < children.size(); ++i)
		{
			if (!children[i].IsLeaf())
				Within(children[i], pairs, visits);
			for (const auto &earlier : children.first(i))
				Cross(earlier, children[i], pairs, visits);
		}
	}

	std::vector<std::size_t> Overlaps(const AABB &bounds, std::size_t &visits) const
	{
		RequireFinite(bounds);
		visits = 0;
		std::vector<std::size_t> ids;
		if (!mNodes.empty())
			VisitBox(mNodes.front(), bounds, ids, visits);
		std::ranges::sort(ids);
		return ids;
	}

	Pairs OverlapPairs(std::size_t &visits) const
	{
		visits = 0;
		Pairs pairs;
		if (!mNodes.empty() && !mNodes.front().IsLeaf())
			Within(mNodes.front(), pairs, visits);
		std::ranges::sort(pairs);
		return pairs;
	}
};
} // namespace Detail

namespace
{
using Detail::FourWayTree;

struct FourWayTopology
{
	explicit FourWayTopology(const FourWayTree &tree) : mParent(tree.mNodes.size())
	{
		for (std::size_t i = 0; i < tree.mNodes.size(); ++i)
		{
			const auto &node = tree.mNodes[i];
			if (node.IsLeaf())
				mLeafById.emplace(node.mFirstChildOrId, i);
			else
				for (std::size_t child = 0; child < node.mChildCount; ++child)
					mParent[node.mFirstChildOrId + child] = i;
		}
	}

	std::unordered_map<std::size_t, std::size_t> mLeafById;
	std::vector<std::size_t>					 mParent;
};

class FourWayUpdateIndex final : public IncrementalSpatialIndex
{
  public:
	explicit FourWayUpdateIndex(std::span<const SpatialEntry> entries) : mTree(std::make_shared<FourWayTree>(entries)), mTopology(std::make_shared<FourWayTopology>(*mTree)) {}

	std::vector<std::size_t> Overlaps(const AABB &bounds) const override
	{
		std::size_t visits;
		return mTree->Overlaps(bounds, visits);
	}

	Pairs OverlapPairs() const override
	{
		std::size_t visits;
		return mTree->OverlapPairs(visits);
	}

	std::unique_ptr<const IncrementalSpatialIndex> Updated(const SpatialUpdateBatch &batch, SpatialUpdateStats &outStats) const override
	{
		Detail::ValidateUpdate(batch, mTopology->mLeafById.size(), std::vector<SpatialEntry>().max_size(),
							   [&](std::size_t id)
							   { return mTopology->mLeafById.contains(id); });
		SpatialUpdateStats					stats;
		std::unique_ptr<FourWayUpdateIndex> next;
		if (!batch.mInserted.empty() || !batch.mRemoved.empty())
		{
			const auto entries = ChangedMembership(batch);
			next = std::make_unique<FourWayUpdateIndex>(entries);
			stats.mTopologyRebuilds = 1;
		}
		else
		{
			next = std::make_unique<FourWayUpdateIndex>(*this);
			if (!batch.mMoved.empty())
				next->mTree = Refit(batch.mMoved, stats);
		}
		outStats = stats;
		return next;
	}

  private:
	std::shared_ptr<const FourWayTree> Refit(std::span<const SpatialEntry> moved, SpatialUpdateStats &stats) const
	{
		auto								  tree = std::make_shared<FourWayTree>(*mTree);
		std::set<std::size_t, std::greater<>> affected;
		for (const auto &entry : moved)
		{
			auto index = mTopology->mLeafById.at(entry.mId);
			tree->mNodes[index].mBounds = entry.mBounds;
			while (index != 0)
			{
				index = mTopology->mParent[index];
				if (!affected.insert(index).second)
					break;
			}
		}
		// Descendants always have greater indices. Shared ancestors see every child's new bounds.
		for (auto index : affected)
			tree->EncloseChildren(tree->mNodes[index]);
		stats.mRefitNodes = affected.size();
		return tree;
	}

	std::vector<SpatialEntry> ChangedMembership(const SpatialUpdateBatch &batch) const
	{
		std::unordered_set<std::size_t> replaced(batch.mRemoved.begin(), batch.mRemoved.end());
		for (const auto &entry : batch.mMoved)
			replaced.insert(entry.mId);
		std::vector<SpatialEntry> entries;
		entries.reserve(mTopology->mLeafById.size() - batch.mRemoved.size() + batch.mInserted.size());
		for (const auto &[id, leaf] : mTopology->mLeafById)
			if (!replaced.contains(id))
				entries.push_back({id, mTree->mNodes[leaf].mBounds});
		entries.insert(entries.end(), batch.mMoved.begin(), batch.mMoved.end());
		entries.insert(entries.end(), batch.mInserted.begin(), batch.mInserted.end());
		return entries;
	}

	std::shared_ptr<const FourWayTree>	   mTree;
	std::shared_ptr<const FourWayTopology> mTopology;
};
} // namespace

FourWaySpatialIndex::FourWaySpatialIndex(std::span<const SpatialEntry> entries) : mImpl(std::make_unique<Detail::FourWayTree>(entries))
{
}
FourWaySpatialIndex::~FourWaySpatialIndex() = default;

std::vector<std::size_t> FourWaySpatialIndex::Overlaps(const AABB &bounds) const
{
	std::size_t visits;
	return Overlaps(bounds, visits);
}

std::vector<std::size_t> FourWaySpatialIndex::Overlaps(const AABB &bounds, std::size_t &outNodeVisits) const
{
	return mImpl->Overlaps(bounds, outNodeVisits);
}

Pairs FourWaySpatialIndex::OverlapPairs() const
{
	std::size_t visits;
	return OverlapPairs(visits);
}

Pairs FourWaySpatialIndex::OverlapPairs(std::size_t &outNodeVisits) const
{
	return mImpl->OverlapPairs(outNodeVisits);
}

std::unique_ptr<SpatialOverlapProvider> MakeFourWaySpatialOverlapProvider()
{
	return std::make_unique<FourWayProvider>();
}

CapabilityRegistration FourWaySpatialOverlapPlugin()
{
	return SpatialOverlapSlot().Factory("hibiki.four_way_bvh", MakeFourWaySpatialOverlapProvider);
}

std::unique_ptr<IncrementalSpatialProvider> MakeFourWayUpdateProvider()
{
	return std::make_unique<Detail::SpatialUpdateProvider<FourWayUpdateIndex>>();
}
} // namespace HibikiPhysics
