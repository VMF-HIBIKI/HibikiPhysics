#include <HibikiPhysics/FilteredSpatialIndex.h>
#include "../fixtures/SpatialConformance.h"

using namespace HibikiPhysics;
using namespace HibikiPhysics::Test::SpatialReference;
int main()
{
	const AABB						unit({0, 0, 0}, {1, 1, 1});
	std::vector<TaggedSpatialEntry> tagged;
	for (std::size_t i = 0; i < 75; ++i)
	{
		const double x = static_cast<double>(i % 8);
		tagged.push_back({{101 - i, {{x, 0, 0}, {x + 3, 1, 1}}}, {i % 3, (i / 4) % 2}});
	}
	CapabilityConfig config;
	config.Add(FilteredMortonPlugin());
	Dependencies deps;
	FilteredSpatialSlot().Declare(deps);
	const auto				   ctx = config.Build(deps);
	const auto				   index = FilteredSpatialSlot().Get(ctx).Build(tagged);
	const SpatialPairPredicate accept = [](const SpatialMetadata &a, const SpatialMetadata &b)
	{ return a.mOwner < b.mOwner || a.mCategory != b.mCategory; };
	SpatialFilterStats stats{};
	Pairs			   expected, unfiltered;
	for (const auto &a : tagged)
	{
		Ids ids;
		for (const auto &b : tagged)
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
		Check(Sorted(ids) == index->FilteredOverlaps(a.mEntry.mId, a.mEntry.mBounds, accept, stats), "filtered box vs post-filter");
	}
	Check(Sorted(expected) == index->FilteredPairs(accept, stats), "filtered pairs and asymmetric canonical ID policy");
	Check(stats.mQueryReads > 0 && stats.mNodeVisits > 0 && stats.mLeafTests > 0, "query-local stats");
	Check(Sorted(unfiltered) == index->OverlapPairs(), "unfiltered pairs retained");
	const SpatialMetadata common{std::numeric_limits<std::size_t>::max(), 7};
	for (auto &a : tagged)
		a.mMetadata = common;
	const auto uniform = MakeFilteredMortonProvider()->Build(tagged);
	Check(uniform->FilteredPairs([](const auto &, const auto &)
								 { return false; }, stats)
				  .empty() &&
			  stats.mPrunedSubtrees == tagged.size() && stats.mLeafTests == 0,
		  "reject whole homogeneous subtree");
	Check(uniform->OverlapPairs() == index->OverlapPairs(), "policy is per query");
	ExpectInvalid([&]
				  { (void)uniform->FilteredOverlaps(1000, unit, accept, stats); });
	ExpectInvalid([&]
				  { (void)uniform->FilteredPairs({}, stats); });
}
