#include <HibikiPhysics/MortonSpatialIndex.h>
#include "MortonTree.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numeric>

namespace HibikiPhysics
{
namespace
{
using namespace Detail;

struct MortonLeaf
{
	std::uint64_t		mCode;
	const SpatialEntry *mEntry;
};

Point Center(const AABB &bounds)
{
	const auto lo = bounds.GetMin(), hi = bounds.GetMax();
	return {std::midpoint(lo[0], hi[0]), std::midpoint(lo[1], hi[1]), std::midpoint(lo[2], hi[2])};
}

constexpr unsigned AxisBits = 21;

std::uint64_t Quantize(double value, double lo, double hi)
{
	if (lo == hi)
		return 0;
	// Halve only when necessary, preserving the full scale of subnormal extents.
	if (std::isinf(hi - lo))
	{
		value *= 0.5;
		lo *= 0.5;
		hi *= 0.5;
	}
	constexpr std::uint64_t bins = std::uint64_t{1} << AxisBits;
	const auto				bin = static_cast<std::uint64_t>(((value - lo) / (hi - lo)) * bins);
	return std::min(bin, bins - 1);
}

std::uint64_t MortonCode(const Point &center, const Point &lo, const Point &hi)
{
	std::uint64_t result = 0;
	for (unsigned axis = 0; axis < 3; ++axis)
	{
		const auto coordinate = Quantize(center[axis], lo[axis], hi[axis]);
		for (unsigned bit = 0; bit < AxisBits; ++bit)
			result |= ((coordinate >> bit) & 1) << (3 * bit + axis);
	}
	return result;
}

template <class Key>
std::size_t PrefixSplit(std::span<const MortonLeaf> leaves, Key key)
{
	// In a sorted range the first differing bit is zero on the left and one on the right.
	const auto mask = std::bit_floor(key(leaves.front()) ^ key(leaves.back()));
	const auto split = std::ranges::partition_point(leaves, [&](const MortonLeaf &leaf)
													{ return (key(leaf) & mask) == 0; });
	return static_cast<std::size_t>(split - leaves.begin());
}

AABB UnionBounds(const AABB &a, const AABB &b)
{
	auto	   lo = a.GetMin(), hi = a.GetMax();
	const auto bLo = b.GetMin(), bHi = b.GetMax();
	for (unsigned axis = 0; axis < 3; ++axis)
	{
		lo[axis] = std::min(lo[axis], bLo[axis]);
		hi[axis] = std::max(hi[axis], bHi[axis]);
	}
	return {lo, hi};
}

std::size_t BuildRange(std::span<const MortonLeaf> leaves, std::vector<BvhNode> &nodes)
{
	const auto	root = nodes.size();
	const auto &first = *leaves.front().mEntry;
	nodes.push_back({first.mBounds});
	if (leaves.size() == 1)
	{
		nodes[root].mId = first.mId;
		return root;
	}
	// Equal Morton codes extend their common prefix with the caller's unique ID bits.
	const auto split = leaves.front().mCode != leaves.back().mCode
						   ? PrefixSplit(leaves, [](const MortonLeaf &leaf)
										 { return leaf.mCode; })
						   : PrefixSplit(leaves, [](const MortonLeaf &leaf)
										 { return leaf.mEntry->mId; });
	const auto left = BuildRange(leaves.first(split), nodes);
	const auto right = BuildRange(leaves.subspan(split), nodes);
	nodes[root].mRight = right;
	nodes[root].mBounds = UnionBounds(nodes[left].mBounds, nodes[right].mBounds);
	return root;
}

template <class Provider, class Index>
class MortonProvider final : public Provider
{
  public:
	std::unique_ptr<const Index> Build(std::span<const SpatialEntry> entries) const override
	{
		return std::make_unique<SpatialTreeIndex>(BuildMortonTree(entries));
	}
};
} // namespace

std::vector<Detail::BvhNode> Detail::BuildMortonTree(std::span<const SpatialEntry> entries)
{
	if (entries.empty())
		return {};
	const auto owned = CopySpatialEntries(entries);
	auto	   lo = Center(owned.front().mBounds), hi = lo;
	for (const auto &entry : owned)
	{
		const auto center = Center(entry.mBounds);
		for (unsigned axis = 0; axis < 3; ++axis)
		{
			lo[axis] = std::min(lo[axis], center[axis]);
			hi[axis] = std::max(hi[axis], center[axis]);
		}
	}
	std::vector<MortonLeaf> leaves;
	leaves.reserve(owned.size());
	for (const auto &entry : owned)
		leaves.push_back({MortonCode(Center(entry.mBounds), lo, hi), &entry});
	std::ranges::sort(leaves, {}, [](const MortonLeaf &leaf)
					  { return std::pair(leaf.mCode, leaf.mEntry->mId); });

	std::vector<BvhNode> nodes;
	nodes.reserve(owned.size() + (owned.size() - 1));
	BuildRange(leaves, nodes);
	return nodes;
}

std::unique_ptr<SpatialQueryProvider> MakeMortonSpatialQueryProvider()
{
	return std::make_unique<MortonProvider<SpatialQueryProvider, SpatialQueryIndex>>();
}

std::unique_ptr<SpatialOverlapProvider> MakeMortonSpatialOverlapProvider()
{
	return std::make_unique<MortonProvider<SpatialOverlapProvider, SpatialOverlapIndex>>();
}

CapabilityRegistration MortonSpatialQueryPlugin()
{
	return SpatialQuerySlot().Factory("hibiki.morton_lbvh", MakeMortonSpatialQueryProvider);
}

CapabilityRegistration MortonSpatialOverlapPlugin()
{
	return SpatialOverlapSlot().Factory("hibiki.morton_lbvh", MakeMortonSpatialOverlapProvider);
}
} // namespace HibikiPhysics
