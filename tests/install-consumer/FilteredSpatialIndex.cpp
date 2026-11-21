#include <HibikiPhysics/FilteredSpatialIndex.h>
#include "../fixtures/SpatialConformance.h"

using namespace HibikiPhysics;
using namespace HibikiPhysics::Test::SpatialReference;

namespace
{
void SameTraversal(const SpatialFilterStats &baseline, const SpatialFilterStats &cached)
{
	Check(baseline.mNodeVisits == cached.mNodeVisits && baseline.mLeafTests == cached.mLeafTests &&
			  baseline.mPrunedSubtrees == cached.mPrunedSubtrees,
		  "cache preserves node visits, leaf tests and pruned subtrees");
}

void CheckProviders(const std::vector<TaggedSpatialEntry> &entries, const SpatialPairPredicate &accept, bool homogeneous)
{
	CapabilityConfig config;
	config.Add(FilteredMortonPlugin());
	Dependencies deps;
	FilteredSpatialSlot().Declare(deps);
	const auto baselineContext = config.Build(deps);
	const auto baseline = FilteredSpatialSlot().Get(baselineContext).Build(entries);
	config.Replace(CachedFilteredMortonPlugin());
	const auto cachedContext = config.Build(deps);
	const auto cached = FilteredSpatialSlot().Get(cachedContext).Build(entries);

	const auto queryCachedPairs = [&]
	{
		SpatialFilterStats stats{};
		return cached->FilteredPairs(accept, stats);
	};
	auto			   concurrentPairs = std::async(std::launch::async, queryCachedPairs);
	SpatialFilterStats baseStats{}, cachedStats{};
	Pairs			   expected, unfiltered;
	for (const auto &a : entries)
	{
		Ids ids;
		for (const auto &b : entries)
		{
			if (!Overlap(a.mEntry.mBounds, b.mEntry.mBounds))
				continue;
			if (a.mEntry.mId < b.mEntry.mId)
				unfiltered.emplace_back(a.mEntry.mId, b.mEntry.mId);
			if (accept(a.mMetadata, b.mMetadata))
			{
				ids.push_back(b.mEntry.mId);
				if (a.mEntry.mId < b.mEntry.mId)
					expected.emplace_back(a.mEntry.mId, b.mEntry.mId);
			}
		}
		std::ranges::sort(ids);
		Check(baseline->FilteredOverlaps(a.mEntry.mId, a.mEntry.mBounds, accept, baseStats) == ids, "baseline box vs post-filter");
		Check(cached->FilteredOverlaps(a.mEntry.mId, a.mEntry.mBounds, accept, cachedStats) == ids, "cached box vs post-filter");
		Check(cachedStats.mQueryReads == 1, "one source read per filtered box");
		SameTraversal(baseStats, cachedStats);
	}
	std::ranges::sort(expected);
	Check(baseline->FilteredPairs(accept, baseStats) == expected && cached->FilteredPairs(accept, cachedStats) == expected &&
			  concurrentPairs.get() == expected,
		  "canonical filtered pairs agree with post-filter including concurrent queries");
	SameTraversal(baseStats, cachedStats);
	Check(cachedStats.mQueryReads == entries.size(), "one source read per leaf in pair traversal");
	if (homogeneous)
		Check(expected.empty() && baseStats.mQueryReads == entries.size() && baseStats.mPrunedSubtrees == entries.size() &&
				  baseStats.mLeafTests == 0,
			  "both providers reject the root after one read per source");
	else
		Check(baseStats.mQueryReads > cachedStats.mQueryReads && baseStats.mLeafTests > 0, "mixed leaves require repeated baseline reads");
	std::ranges::sort(unfiltered);
	Check(baseline->OverlapPairs() == unfiltered && cached->OverlapPairs() == unfiltered, "unfiltered pairs ignore metadata");

	const AABB miss({100, 100, 100}, {101, 101, 101});
	const auto id = entries.front().mEntry.mId;
	Check(baseline->FilteredOverlaps(id, miss, accept, baseStats).empty() && cached->FilteredOverlaps(id, miss, accept, cachedStats).empty(), "box disjoint from root");
	SameTraversal(baseStats, cachedStats);
	Check(baseStats.mNodeVisits == 1 && baseStats.mQueryReads == 0 && cachedStats.mQueryReads == 1, "eager cache reads even when the root misses");
	for (const auto *index : {baseline.get(), cached.get()})
	{
		ExpectInvalid([&]
					  { (void)index->FilteredOverlaps(1000, miss, accept, cachedStats); });
		ExpectInvalid([&]
					  { (void)index->FilteredPairs({}, cachedStats); });
	}
}
} // namespace

int main()
{
	std::vector<TaggedSpatialEntry> entries;
	for (std::size_t i = 0; i < 75; ++i)
	{
		const double x = static_cast<double>(i % 8);
		entries.push_back({{101 - i, {{x, 0, 0}, {x + 3, 1, 1}}}, {i % 3, (i / 4) % 2}});
	}
	const SpatialPairPredicate accept = [](const SpatialMetadata &a, const SpatialMetadata &b)
	{ return a.mOwner < b.mOwner || a.mCategory != b.mCategory; };
	CheckProviders(entries, accept, false);
	for (auto &entry : entries)
		entry.mMetadata = {std::numeric_limits<std::size_t>::max(), 7};
	CheckProviders(entries, accept, true);

	SpatialFilterStats stats{7, 7, 7, 7};
	const auto		   empty = MakeCachedFilteredMortonProvider()->Build({});
	Check(empty->FilteredPairs(accept, stats).empty() && stats.mQueryReads == 0 && stats.mNodeVisits == 0 &&
			  stats.mLeafTests == 0 && stats.mPrunedSubtrees == 0,
		  "empty filtered call replaces prior counters");
}
