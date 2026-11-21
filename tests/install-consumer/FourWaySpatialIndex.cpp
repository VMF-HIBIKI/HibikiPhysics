#include <HibikiPhysics/FourWaySpatialIndex.h>
#include "../fixtures/SpatialConformance.h"

namespace
{
using namespace HibikiPhysics;
using namespace HibikiPhysics::Test::SpatialReference;

void CheckRegistration()
{
	CapabilityConfig config;
	config.Add(CpuSpatialOverlapPlugin());
	config.Replace(FourWaySpatialOverlapPlugin());
	Dependencies dependencies;
	SpatialOverlapSlot().Declare(dependencies);
	const auto scope = config.Build(dependencies);
	const auto selections = scope.GetSelections();
	Check(selections.size() == 1 && selections.front().mImplementation == "hibiki.four_way_bvh" && selections.front().mReplaced, "Four-way overlap slot replacement");
	Test::CheckOverlapProvider(SpatialOverlapSlot().Get(scope));
}

void CheckSmallGroups()
{
	std::vector<SpatialEntry> entries;
	const AABB				  unit({0, 0, 0}, {1, 1, 1});
	for (std::size_t n = 0; n <= 19; ++n)
	{
		const FourWaySpatialIndex index(entries);
		std::size_t				  boxVisits = 42, pairVisits = 42;
		const auto				  hits = index.Overlaps(unit, boxVisits);
		const auto				  pairs = index.OverlapPairs(pairVisits);
		Check(hits == Matching(entries, [](const AABB &)
							   { return true; }),
			  "Coincident IDs in ascending order");
		Check(pairs == AllPairs(entries), "Complete and unique pairs for incomplete sibling groups");
		if (n <= 4)
		{
			const std::size_t nodes = n <= 1 ? n : n + 1;
			Check(boxVisits == nodes && pairVisits == (n == 0 ? 0 : n * (n - 1) / 2), "A single wide node covers two to four leaves");
		}
		entries.push_back({1001 - n * 19, unit});
	}
}

void CheckDeepHierarchy()
{
	// Five four-way levels separate 1024 disjoint point boxes along Z.
	std::vector<SpatialEntry> entries;
	for (std::size_t i = 0; i < 1024; ++i)
	{
		Point location{0, 0, static_cast<double>(i)};
		entries.push_back({i * 233 % 1024, {location, location}});
	}
	const auto				 &target = entries[273];
	const FourWaySpatialIndex index(entries);
	std::size_t				  boxVisits, pairVisits;
	auto					  parallel = std::async(std::launch::async, [&]
													{
								   std::size_t visits;
								   Check(index.Overlaps(target.mBounds, visits) == Ids{target.mId}, "Concurrent result");
								   return visits; });
	Check(index.Overlaps(target.mBounds, boxVisits) == Ids{target.mId} && boxVisits == 1 + 4 * 5, "One matching path at each level");
	Check(index.OverlapPairs(pairVisits).empty() && pairVisits == 6 * ((1024 - 1) / 3), "Six disjoint sibling tests per branch");
	Check(parallel.get() == boxVisits, "Concurrent queries report independent visit counts");
	Check(index.Overlaps({{0, 0, -2}, {0, 0, -1}}, boxVisits).empty() && boxVisits == 1, "Root bounds prune a miss");
}

void CheckReorderedInput()
{
	auto					  entries = Boxes();
	const FourWaySpatialIndex first(entries);
	std::ranges::shuffle(entries, std::mt19937(471));
	const FourWaySpatialIndex second(entries);
	std::size_t				  a, b;
	Check(first.OverlapPairs(a) == second.OverlapPairs(b) && a == b, "Input permutation preserves pair traversal");
	for (const auto &entry : entries)
		Check(first.Overlaps(entry.mBounds, a) == second.Overlaps(entry.mBounds, b) && a == b, "Input permutation preserves box traversal");
}

void CheckExtremeCenters()
{
	const double			  limit = std::numeric_limits<double>::max();
	const AABB				  entire({-limit, -limit, -limit}, {limit, limit, limit});
	std::vector<SpatialEntry> entries{{0, entire}};
	for (double x : {-limit, limit})
		for (double y : {-limit, limit})
			for (double z : {-limit, limit})
				entries.push_back({entries.size(), {{x, y, z}, {x, y, z}}});
	const auto index = MakeFourWaySpatialOverlapProvider()->Build(entries);
	Check(index->OverlapPairs() == AllPairs(entries), "Finite extreme centers and overflowing spans");
	for (const auto &entry : entries)
		Check(index->Overlaps(entry.mBounds) == Matching(entries, [&](const AABB &other)
														 { return Overlap(entry.mBounds, other); }),
			  "Extreme closed-box query");
}
} // namespace

int main()
{
	CheckRegistration();
	CheckSmallGroups();
	CheckDeepHierarchy();
	CheckReorderedInput();
	CheckExtremeCenters();
}
