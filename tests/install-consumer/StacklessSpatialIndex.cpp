#include <HibikiPhysics/StacklessSpatialIndex.h>
#include "../fixtures/SpatialConformance.h"

namespace
{
using namespace HibikiPhysics;
using Test::SpatialReference::Check;
using Test::SpatialReference::Ids;

void CheckEscapes()
{
	// Binary Morton prefixes on this one-dimensional grid form a complete tree.
	std::vector<SpatialEntry> entries;
	for (std::size_t x = 0; x < 8; ++x)
	{
		const auto p = static_cast<double>(x);
		entries.push_back({9 - x, {{p, 0, 0}, {p, 0, 0}}});
	}
	const StacklessSpatialOverlapIndex index(entries);
	const AABB						   all({0, 0, 0}, {7, 0, 0});
	std::size_t						   visited = 99;
	auto							   broad = std::async(std::launch::async, [&]
														  {
		std::size_t count;
		auto ids = index.Overlaps(all, count);
		return std::pair(std::move(ids), count); });
	Check(index.Overlaps({{7, 0, 0}, {7, 0, 0}}, visited) == Ids{2} && visited == 7, "Rightmost path escapes left subtrees");
	Check(index.Overlaps({{3.5, 0, 0}, {3.5, 0, 0}}, visited).empty() && visited == 3, "Both root child bounds reject a gap");
	Check(index.Overlaps({{8, 0, 0}, {8, 0, 0}}, visited).empty() && visited == 1, "Root rejection visits one node");
	Check(index.OverlapPairs(visited).empty() && visited == 12, "Each leaf tests just its right-hand sibling subtrees");
	const auto [ids, count] = broad.get();
	Check(Test::SpatialReference::Sorted(ids) == Ids{2, 3, 4, 5, 6, 7, 8, 9} && count == 15, "Concurrent broad query has its own count");

	const StacklessSpatialOverlapIndex empty({});
	Check(empty.Overlaps(all, visited).empty() && visited == 0, "Empty query resets count");
	Check(empty.OverlapPairs(visited).empty() && visited == 0, "Empty pair walk");
}

void CheckRegistration()
{
	CapabilityConfig config;
	config.Add(CpuSpatialOverlapPlugin());
	config.Replace(StacklessSpatialOverlapPlugin());
	Dependencies requirements;
	SpatialOverlapSlot().Declare(requirements);
	const auto scope = config.Build(requirements);
	const auto selections = scope.GetSelections();
	Check(selections.size() == 1 && selections.front().mReplaced && selections.front().mImplementation == "hibiki.stackless_lbvh", "Stackless overlap replacement selected");
	Test::CheckOverlapProvider(SpatialOverlapSlot().Get(scope));
}
} // namespace

int main()
{
	CheckEscapes();
	CheckRegistration();
}
