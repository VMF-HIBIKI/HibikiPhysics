#include <HibikiPhysics/FilteredSpatialIndex.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>

using namespace HibikiPhysics;

namespace
{
constexpr int kSamples = 21;
constexpr int kRepeats = 10;

std::vector<TaggedSpatialEntry> Grid(bool homogeneous)
{
	std::vector<TaggedSpatialEntry> entries;
	for (std::size_t i = 0; i < 512; ++i)
	{
		const auto			  x = static_cast<double>(i % 32);
		const auto			  y = static_cast<double>(i / 32);
		const SpatialMetadata tags = homogeneous ? SpatialMetadata{0, 0} : SpatialMetadata{(i % 32) / 4, (i / 32) / 4};
		entries.push_back({{1024 - i, {{x, y, 0}, {x + 2, y + 2, 1}}}, tags});
	}
	return entries;
}

const SpatialPairPredicate kAccept = [](const SpatialMetadata &a, const SpatialMetadata &b)
{ return a.mOwner < b.mOwner || a.mCategory != b.mCategory; };

std::size_t Batch(const FilteredSpatialIndex &index, std::span<const TaggedSpatialEntry> entries, bool outsideRoot, SpatialFilterStats &stats)
{
	if (!outsideRoot)
		return index.FilteredPairs(kAccept, stats).size();
	const AABB miss({100, 100, 100}, {101, 101, 101});
	stats = {};
	std::size_t hits = 0;
	for (const auto &entry : entries)
	{
		SpatialFilterStats call{};
		hits += index.FilteredOverlaps(entry.mEntry.mId, miss, kAccept, call).size();
		stats.mNodeVisits += call.mNodeVisits;
		stats.mQueryReads += call.mQueryReads;
		stats.mLeafTests += call.mLeafTests;
		stats.mPrunedSubtrees += call.mPrunedSubtrees;
	}
	return hits;
}

void Measure(const char *scene, bool homogeneous, bool outsideRoot)
{
	const auto						  entries = Grid(homogeneous);
	const std::array				  indexes{MakeFilteredMortonProvider()->Build(entries), MakeCachedFilteredMortonProvider()->Build(entries)};
	std::array<SpatialFilterStats, 2> stats{};
	const auto						  hits = Batch(*indexes[0], entries, outsideRoot, stats[0]);
	if (Batch(*indexes[1], entries, outsideRoot, stats[1]) != hits ||
		stats[0].mNodeVisits != stats[1].mNodeVisits || stats[0].mLeafTests != stats[1].mLeafTests ||
		stats[0].mPrunedSubtrees != stats[1].mPrunedSubtrees)
		throw std::logic_error("Provider results or traversal work differ");
	if (outsideRoot && hits != 0)
		throw std::logic_error("Outside-root query returned a candidate");
	if (!outsideRoot && indexes[0]->FilteredPairs(kAccept, stats[0]) != indexes[1]->FilteredPairs(kAccept, stats[1]))
		throw std::logic_error("Provider candidate sets differ");

	using Clock = std::chrono::steady_clock;
	std::array<std::array<double, kSamples>, 2> times{};
	for (int sample = 0; sample < kSamples; ++sample)
		for (int order = 0; order < 2; ++order)
		{
			const auto	provider = (sample + order) % 2;
			std::size_t total = 0;
			const auto	start = Clock::now();
			for (int i = 0; i < kRepeats; ++i)
				total += Batch(*indexes[provider], entries, outsideRoot, stats[provider]);
			const double us = std::chrono::duration<double, std::micro>(Clock::now() - start).count();
			times[provider][sample] = us / kRepeats;
			if (total != kRepeats * hits)
				throw std::logic_error("Candidate count changed during timing");
		}
	for (std::size_t p = 0; p < indexes.size(); ++p)
	{
		std::ranges::sort(times[p]);
		std::cout << scene << ',' << (p == 0 ? "baseline" : "cached") << ',' << times[p][kSamples / 2]
				  << ',' << hits << ',' << stats[p].mQueryReads << ',' << stats[p].mNodeVisits
				  << ',' << stats[p].mLeafTests << ',' << stats[p].mPrunedSubtrees << '\n';
	}
}
} // namespace

int main()
{
	std::cout << "scene,provider,median_us,hits,query_reads,node_visits,leaf_tests,pruned_subtrees\n";
	Measure("uniform_pairs", true, false);
	Measure("mixed_pairs", false, false);
	Measure("outside_root_boxes", false, true);
}
