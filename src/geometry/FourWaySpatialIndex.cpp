#include <HibikiPhysics/FourWaySpatialIndex.h>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

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

struct FourWaySpatialIndex::Impl
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

	explicit Impl(std::span<const SpatialEntry> input)
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
};

FourWaySpatialIndex::FourWaySpatialIndex(std::span<const SpatialEntry> entries) : mImpl(std::make_unique<Impl>(entries))
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
	RequireFinite(bounds);
	outNodeVisits = 0;
	std::vector<std::size_t> ids;
	if (!mImpl->mNodes.empty())
		mImpl->VisitBox(mImpl->mNodes.front(), bounds, ids, outNodeVisits);
	std::ranges::sort(ids);
	return ids;
}

Pairs FourWaySpatialIndex::OverlapPairs() const
{
	std::size_t visits;
	return OverlapPairs(visits);
}

Pairs FourWaySpatialIndex::OverlapPairs(std::size_t &outNodeVisits) const
{
	outNodeVisits = 0;
	Pairs pairs;
	if (!mImpl->mNodes.empty() && !mImpl->mNodes.front().IsLeaf())
		mImpl->Within(mImpl->mNodes.front(), pairs, outNodeVisits);
	std::ranges::sort(pairs);
	return pairs;
}

std::unique_ptr<SpatialOverlapProvider> MakeFourWaySpatialOverlapProvider()
{
	return std::make_unique<FourWayProvider>();
}

CapabilityRegistration FourWaySpatialOverlapPlugin()
{
	return SpatialOverlapSlot().Factory("hibiki.four_way_bvh", MakeFourWaySpatialOverlapProvider);
}
} // namespace HibikiPhysics
