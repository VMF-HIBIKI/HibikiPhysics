#include <HibikiPhysics/StacklessSpatialIndex.h>
#include "EscapeTree.h"
#include "MortonTree.h"

#include <algorithm>

namespace HibikiPhysics
{
namespace
{
class StacklessProvider final : public SpatialOverlapProvider
{
  public:
	std::unique_ptr<const SpatialOverlapIndex> Build(std::span<const SpatialEntry> entries) const override
	{
		return std::make_unique<StacklessSpatialOverlapIndex>(entries);
	}
};
} // namespace

StacklessSpatialOverlapIndex::StacklessSpatialOverlapIndex(std::span<const SpatialEntry> entries)
	: mTree(std::make_unique<Detail::EscapeTree>(Detail::BuildMortonTree(entries)))
{
}

StacklessSpatialOverlapIndex::~StacklessSpatialOverlapIndex() = default;

std::vector<std::size_t> StacklessSpatialOverlapIndex::Overlaps(const AABB &bounds) const
{
	std::size_t visited;
	return Overlaps(bounds, visited);
}

std::vector<std::size_t> StacklessSpatialOverlapIndex::Overlaps(const AABB &bounds, std::size_t &visited) const
{
	Detail::RequireFinite(bounds);
	std::vector<std::size_t> ids;
	visited = mTree->Walk(0, [&](std::size_t, const Detail::BvhNode &node)
						  { return Detail::Intersects(bounds, node.mBounds); }, [&](std::size_t, const Detail::BvhNode &leaf)
						  { ids.push_back(leaf.mId); });
	return ids;
}

Detail::Pairs StacklessSpatialOverlapIndex::OverlapPairs() const
{
	std::size_t visited;
	return OverlapPairs(visited);
}

Detail::Pairs StacklessSpatialOverlapIndex::OverlapPairs(std::size_t &visited) const
{
	Detail::Pairs pairs;
	visited = 0;
	const auto nodes = mTree->Nodes();
	for (std::size_t i = 0; i < nodes.size(); ++i)
	{
		const auto &source = nodes[i];
		if (source.IsLeaf())
			visited += mTree->Walk(i + 1, [&](std::size_t, const Detail::BvhNode &node)
								   { return Detail::Intersects(source.mBounds, node.mBounds); }, [&](std::size_t, const Detail::BvhNode &leaf)
								   { pairs.emplace_back(std::minmax(source.mId, leaf.mId)); });
	}
	return pairs;
}

std::unique_ptr<SpatialOverlapProvider> MakeStacklessSpatialOverlapProvider()
{
	return std::make_unique<StacklessProvider>();
}

CapabilityRegistration StacklessSpatialOverlapPlugin()
{
	return SpatialOverlapSlot().Factory("hibiki.stackless_lbvh", MakeStacklessSpatialOverlapProvider);
}
} // namespace HibikiPhysics
