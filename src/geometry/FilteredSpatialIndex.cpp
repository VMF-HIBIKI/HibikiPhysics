#include <HibikiPhysics/FilteredSpatialIndex.h>
#include "EscapeTree.h"
#include "MortonTree.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>

namespace HibikiPhysics
{
namespace
{
using Pairs = std::vector<std::pair<std::size_t, std::size_t>>;
using namespace Detail;
std::vector<BvhNode> BuildTaggedTree(std::span<const TaggedSpatialEntry> entries)
{
	std::vector<SpatialEntry> boxes;
	boxes.reserve(entries.size());
	for (const auto &tagged : entries)
		boxes.push_back(tagged.mEntry);
	return BuildMortonTree(boxes);
}
class FilteredIndex final : public FilteredSpatialIndex
{
  public:
	explicit FilteredIndex(std::span<const TaggedSpatialEntry> entries) : mTree(BuildTaggedTree(entries)), mUniform(mTree.Nodes().size())
	{
		mMetadata.reserve(entries.size());
		for (const auto &tagged : entries)
			mMetadata.emplace(tagged.mEntry.mId, tagged.mMetadata);
		const auto nodes = mTree.Nodes();
		for (std::size_t i = nodes.size(); i-- > 0;)
		{
			const auto &node = nodes[i];
			if (node.IsLeaf())
				mUniform[i] = mMetadata.at(node.mId);
			else if (mUniform[i + 1] && mUniform[i + 1] == mUniform[node.mRight])
				mUniform[i] = mUniform[i + 1];
		}
	}
	std::vector<std::size_t> Overlaps(const AABB &bounds) const override
	{
		return QueryBox(0, bounds, nullptr, nullptr);
	}
	Pairs					 OverlapPairs() const override { return QueryPairs(nullptr, nullptr); }
	std::vector<std::size_t> FilteredOverlaps(std::size_t id, const AABB &bounds, const SpatialPairPredicate &accept, SpatialFilterStats &stats) const override
	{
		CheckPolicy(accept);
		if (!mMetadata.contains(id))
			throw std::invalid_argument("Unknown spatial filter source ID");
		SpatialFilterStats result{};
		auto			   ids = QueryBox(id, bounds, &accept, &result);
		stats = result;
		return ids;
	}
	Pairs FilteredPairs(const SpatialPairPredicate &accept, SpatialFilterStats &stats) const override
	{
		CheckPolicy(accept);
		SpatialFilterStats result{};
		auto			   pairs = QueryPairs(&accept, &result);
		stats = result;
		return pairs;
	}

  private:
	static void CheckPolicy(const SpatialPairPredicate &accept)
	{
		if (!accept)
			throw std::invalid_argument("Expected spatial metadata predicate");
	}
	template <class Emit>
	void Visit(std::size_t id, const AABB &bounds, const SpatialPairPredicate *accept, SpatialFilterStats *stats, Emit emit) const
	{
		const auto allowed = [&](const SpatialMetadata &other)
		{
			++stats->mQueryReads;
			return (*accept)(mMetadata.at(id), other);
		};
		const auto visits = mTree.Walk(0, [&](std::size_t i, const BvhNode &node)
									   {
			if(!Intersects(bounds,node.mBounds))return false;
			if(accept && !node.IsLeaf() && mUniform[i] && !allowed(*mUniform[i])){
				++stats->mPrunedSubtrees;return false;
			}
			return true; }, [&](std::size_t i, const BvhNode &node)
									   {
			if(accept){++stats->mLeafTests;if(!allowed(*mUniform[i]))return;}
			emit(node.mId); });
		if (stats)
			stats->mNodeVisits += visits;
	}
	std::vector<std::size_t> QueryBox(std::size_t id, const AABB &bounds, const SpatialPairPredicate *accept, SpatialFilterStats *stats) const
	{
		RequireFinite(bounds);
		std::vector<std::size_t> ids;
		Visit(id, bounds, accept, stats, [&](std::size_t hit)
			  { ids.push_back(hit); });
		std::ranges::sort(ids);
		return ids;
	}
	Pairs QueryPairs(const SpatialPairPredicate *accept, SpatialFilterStats *stats) const
	{
		Pairs pairs;
		for (const auto &node : mTree.Nodes())
		{
			if (!node.IsLeaf())
				continue;
			Visit(node.mId, node.mBounds, accept, stats, [&](std::size_t hit)
				  {if(node.mId<hit)pairs.emplace_back(node.mId,hit); });
		}
		std::ranges::sort(pairs);
		return pairs;
	}
	EscapeTree										 mTree;
	std::vector<std::optional<SpatialMetadata>>		 mUniform;
	std::unordered_map<std::size_t, SpatialMetadata> mMetadata;
};
class FilteredProvider final : public FilteredSpatialProvider
{
  public:
	std::unique_ptr<const FilteredSpatialIndex> Build(std::span<const TaggedSpatialEntry> entries) const override { return std::make_unique<FilteredIndex>(entries); }
};
} // namespace
const CapabilitySlot<FilteredSpatialProvider> &FilteredSpatialSlot()
{
	static const CapabilitySlot<FilteredSpatialProvider> slot("spatial.filtered_overlap");
	return slot;
}
std::unique_ptr<FilteredSpatialProvider> MakeFilteredMortonProvider()
{
	return std::make_unique<FilteredProvider>();
}
CapabilityRegistration FilteredMortonPlugin()
{
	return FilteredSpatialSlot().Factory("hibiki.filtered_morton", MakeFilteredMortonProvider);
}
} // namespace HibikiPhysics
