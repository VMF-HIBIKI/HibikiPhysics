#include "../fixtures/ExperimentMetrics.h"
#include "../fixtures/SpatialBenchmarkScenes.h"
#include "../fixtures/ThroughHolePlate.h"
#include <HibikiPhysics/BatchedOverlap.h>
#include <HibikiPhysics/FourWaySpatialIndex.h>
#include <HibikiPhysics/IncrementalSpatialIndex.h>
#include <HibikiPhysics/MortonSpatialIndex.h>
#include <HibikiPhysics/SdfBaker.h>
#include <HibikiPhysics/StacklessSpatialIndex.h>
#include <HibikiPhysics/SweepAndPrune.h>
#include <chrono>
#include <iomanip>
#include <iostream>

using namespace HibikiPhysics;
namespace
{
using namespace HibikiPhysics::Test::SpatialBenchmark;
using Test::SpatialReference::Check;
using Test::SpatialReference::Ids;
using Test::SpatialReference::Overlap;
using Test::SpatialReference::Pairs;
using Clock = std::chrono::steady_clock;
using Name = std::string_view;
using Index = std::unique_ptr<const SpatialOverlapIndex>;
constexpr std::array<Name, 9> staticMethods{"median", "morton", "bvh4", "sap", "stackless", "batch1", "batch4", "filter", "cache"};
constexpr std::array<Name, 2> incrementalMethods{"sap-update", "bvh4-update"};

double Milliseconds(Clock::time_point start)
{
	return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

struct Measurements
{
	double					 buildMs = 0, updateMs = 0, boxMs = 0, pairsMs = 0;
	std::uint64_t			 peakBytes = 0;
	std::vector<std::size_t> boxHits, pairHits;
	std::size_t				 moved = 0, pairPasses = 0, pairCapacity = 0, boxVisits = 0, pairVisits = 0;
	SpatialFilterStats		 boxFilter{}, pairFilter{};
	SpatialUpdateStats		 updates{};
};

struct Frame
{
	const std::vector<SpatialEntry> &entries;
	std::vector<SpatialEntry>		 queries;
	std::vector<TaggedSpatialEntry>	 tags;
};

// Enumerate the independent expected set in ascending ID order (input IDs decrease).
// In-place sorting lets verification compare every emitted ID and detect duplicates
// without retaining a second array of all dense pairs in the measured process.
void CheckAnswers(const Frame &frame, std::vector<Ids> &answers, Pairs &pairs)
{
	const auto &[entries, queries, tags] = frame;
	const auto metadata = [&](std::size_t id) -> const SpatialMetadata &
	{ return tags.at(entries.size() - 1 - (id - 3) / 7).mMetadata; };
	const auto accepts = [&](std::size_t a, std::size_t b)
	{ return tags.empty() || Allowed(metadata(a), metadata(b)); };
	for (std::size_t i = 0; i < queries.size(); ++i)
	{
		auto &hits = answers[i];
		std::ranges::sort(hits);
		std::size_t position = 0;
		for (auto entry = entries.rbegin(); entry != entries.rend(); ++entry)
			if (Overlap(queries[i].mBounds, entry->mBounds) && accepts(queries[i].mId, entry->mId))
			{
				Check(position < hits.size() && hits[position] == entry->mId, "box IDs differ from brute force");
				++position;
			}
		Check(position == hits.size(), "extra box candidates");
	}
	std::ranges::sort(pairs);
	std::size_t position = 0;
	for (auto a = entries.rbegin(); a != entries.rend(); ++a)
		for (auto b = a + 1; b != entries.rend(); ++b)
			if (Overlap(a->mBounds, b->mBounds) && accepts(a->mId, b->mId))
			{
				Check(position < pairs.size() && pairs[position] == std::pair{a->mId, b->mId}, "self-pairs differ from brute force");
				++position;
			}
	Check(position == pairs.size(), "extra self-pairs");
}

void Add(SpatialFilterStats &total, const SpatialFilterStats &call)
{
	total.mNodeVisits += call.mNodeVisits;
	total.mQueryReads += call.mQueryReads;
	total.mLeafTests += call.mLeafTests;
	total.mPrunedSubtrees += call.mPrunedSubtrees;
}

template <class BoxQuery, class PairQuery>
void MeasureQueries(const Frame &frame, Measurements &result, BoxQuery boxes, PairQuery selfPairs)
{
	std::vector<Ids> answers;
	answers.reserve(frame.queries.size());
	auto start = Clock::now();
	for (const auto &query : frame.queries)
		answers.push_back(boxes(query));
	result.boxMs += Milliseconds(start);
	start = Clock::now();
	auto pairs = selfPairs();
	result.pairsMs += Milliseconds(start);
	result.peakBytes = std::max(result.peakBytes, Test::PeakResidentBytes());
	std::size_t hits = 0;
	for (const auto &answer : answers)
		hits += answer.size();
	result.boxHits.push_back(hits);
	result.pairHits.push_back(pairs.size());
	CheckAnswers(frame, answers, pairs);
}

Index Build(Name method, const std::vector<SpatialEntry> &entries)
{
	if (method == "median")
		return MakeCpuSpatialQueryProvider()->Build(entries);
	if (method == "morton" || method == "batch1" || method == "batch4")
		return MakeMortonSpatialOverlapProvider()->Build(entries);
	if (method == "bvh4")
		return std::make_unique<FourWaySpatialIndex>(entries);
	if (method == "sap")
		return MakeSweepAndPruneProvider()->Build(entries);
	if (method == "stackless")
		return std::make_unique<StacklessSpatialOverlapIndex>(entries);
	throw std::invalid_argument("unknown index method");
}

void MeasureFiltered(const Frame &frame, const FilteredSpatialIndex &index, Measurements &result)
{
	const SpatialPairPredicate predicate = Allowed;
	auto					   boxes = [&](const SpatialEntry &query)
	{
		SpatialFilterStats stats{};
		auto			   ids = index.FilteredOverlaps(query.mId, query.mBounds, predicate, stats);
		Add(result.boxFilter, stats);
		return ids;
	};
	auto pairs = [&]
	{
		SpatialFilterStats stats{};
		auto			   pairs = index.FilteredPairs(predicate, stats);
		Add(result.pairFilter, stats);
		return pairs;
	};
	MeasureQueries(frame, result, boxes, pairs);
}

template <class Tree>
void MeasureCounted(const Frame &frame, const Tree &tree, Measurements &result)
{
	auto boxes = [&](const SpatialEntry &query)
	{
		std::size_t visits = 0;
		auto		ids = tree.Overlaps(query.mBounds, visits);
		result.boxVisits += visits;
		return ids;
	};
	auto pairs = [&]
	{
		std::size_t visits = 0;
		auto		pairs = tree.OverlapPairs(visits);
		result.pairVisits += visits;
		return pairs;
	};
	MeasureQueries(frame, result, boxes, pairs);
}

void MeasureGeometry(const Frame &frame, const SpatialOverlapIndex &index, std::size_t workers, Measurements &result)
{
	auto pairs = [&]
	{
		if (!workers)
			return index.OverlapPairs();
		PairBatchStats stats{};
		auto		   pairs = CollectOverlapPairs(index, frame.entries, {workers, 256}, stats);
		result.pairPasses += stats.mPasses;
		result.pairCapacity = std::max(result.pairCapacity, stats.mCapacity);
		return pairs;
	};
	MeasureQueries(frame, result, [&](const SpatialEntry &q)
				   { return index.Overlaps(q.mBounds); }, pairs);
}

void WriteMeasurements(Name scene, Name method, std::size_t count, std::size_t queryCount, const Measurements &result, std::ostream &out)
{
	auto list = [&](const std::vector<std::size_t> &values)
	{
		out << '[';
		for (std::size_t i = 0; i < values.size(); ++i)
			out << (i ? "," : "") << values[i];
		out << ']';
	};
	out << "{\"build\":\"" HIBIKI_EXPERIMENT_BUILD "\",\"compiler\":\"" HIBIKI_EXPERIMENT_COMPILER "\""
		<< ",\"scene\":\"" << scene << "\",\"method\":\"" << method << "\",\"entries\":" << count
		<< ",\"queries_per_frame\":" << queryCount << ",\"moved_entries\":" << result.moved << ",\"seed\":" << Seed
		<< ",\"build_ms\":" << result.buildMs << ",\"box_ms\":" << result.boxMs << ",\"pairs_ms\":" << result.pairsMs
		<< ",\"peak_bytes\":" << result.peakBytes << ",\"box_hits\":";
	list(result.boxHits);
	out << ",\"pair_hits\":";
	list(result.pairHits);
	if (result.boxHits.size() > 1)
		out << ",\"update_ms\":" << result.updateMs;
	if (method == "batch1" || method == "batch4")
		out << ",\"pair_passes\":" << result.pairPasses << ",\"max_capacity\":" << result.pairCapacity;
	if (method == "stackless" || method == "bvh4")
		out << ",\"box_node_visits\":" << result.boxVisits << ",\"pair_node_visits\":" << result.pairVisits;
	if (method == "sap-update" || method == "bvh4-update")
		out << ",\"endpoint_swaps\":" << result.updates.mEndpointSwaps << ",\"refit_nodes\":" << result.updates.mRefitNodes
			<< ",\"topology_rebuilds\":" << result.updates.mTopologyRebuilds << ",\"endpoints_changed\":" << result.updates.mEndpointsChanged;
	if (method == "filter" || method == "cache")
	{
		auto counters = [&](Name prefix, const SpatialFilterStats &stats)
		{
			out << ",\"" << prefix << "_node_visits\":" << stats.mNodeVisits << ",\"" << prefix << "_query_reads\":" << stats.mQueryReads
				<< ",\"" << prefix << "_leaf_tests\":" << stats.mLeafTests << ",\"" << prefix << "_pruned_subtrees\":" << stats.mPrunedSubtrees;
		};
		counters("box", result.boxFilter);
		counters("pair", result.pairFilter);
	}
	out << "}\n";
}

void SpatialRun(Name scene, Name method, std::size_t count, std::size_t queryCount, std::size_t frames, std::ostream &out)
{
	auto										   entries = Scene(scene, count);
	Measurements								   result;
	Index										   index;
	std::unique_ptr<const IncrementalSpatialIndex> incremental;
	std::unique_ptr<const FilteredSpatialIndex>	   filtered;
	const bool									   incrementalMode = method == "sap-update" || method == "bvh4-update";
	const bool									   filteredMode = method == "filter" || method == "cache";
	for (std::size_t frame = 0; frame < frames; ++frame)
	{
		std::vector<SpatialEntry> moved;
		if (frame > 0)
			moved = Move(entries);
		result.moved += moved.size();
		const Frame current{entries, Queries(entries, queryCount), filteredMode ? Tag(entries) : std::vector<TaggedSpatialEntry>{}};
		const auto	start = Clock::now();
		if (filteredMode)
		{
			auto provider = method == "filter" ? MakeFilteredMortonProvider() : MakeCachedFilteredMortonProvider();
			filtered = provider->Build(current.tags);
		}
		else if (incrementalMode && frame == 0)
		{
			auto provider = method == "sap-update" ? MakeSweepAndPruneUpdateProvider() : MakeFourWayUpdateProvider();
			incremental = provider->Build(entries);
		}
		else if (incrementalMode)
		{
			SpatialUpdateStats stats{};
			incremental = incremental->Updated({{}, moved, {}}, stats);
			result.updates.mEndpointSwaps += stats.mEndpointSwaps;
			result.updates.mRefitNodes += stats.mRefitNodes;
			result.updates.mTopologyRebuilds += stats.mTopologyRebuilds;
			result.updates.mEndpointsChanged += stats.mEndpointsChanged;
		}
		else
			index = Build(method, entries);
		if (frame == 0)
			result.buildMs = Milliseconds(start);
		else
			result.updateMs += Milliseconds(start);
		if (filteredMode)
			MeasureFiltered(current, *filtered, result);
		else if (method == "stackless")
			MeasureCounted(current, static_cast<const StacklessSpatialOverlapIndex &>(*index), result);
		else if (method == "bvh4")
			MeasureCounted(current, static_cast<const FourWaySpatialIndex &>(*index), result);
		else if (incrementalMode)
			MeasureGeometry(current, *incremental, 0, result);
		else
			MeasureGeometry(current, *index, method == "batch1" ? 1 : (method == "batch4" ? 4 : 0), result);
	}
	WriteMeasurements(scene, method, count, queryCount, result, out);
}

DenseSdfGrid Bake(Name method, bool smoke, std::ostream &out)
{
	Check(method == "median" || method == "morton", "SDF requires a full-query provider");
	const auto			  mesh = Test::MakeThroughHolePlate();
	const SdfBakeSettings settings{smoke ? 0.0025 : 0.0005, {0.005, 0.005, 0.005}, {0.5, 0.5, 0}, 1};
	const auto			  provider = method == "median" ? MakeCpuSpatialQueryProvider() : MakeMortonSpatialQueryProvider();
	const auto			  start = Clock::now();
	auto				  grid = BakeDenseSdf(mesh, settings, *provider);
	const auto			  ms = Milliseconds(start);
	const auto			  peak = Test::PeakResidentBytes();
	const auto [nx, ny, nz] = grid.GetNodeCounts();
	const auto origin = grid.GetOrigin();
	out << "{\"build\":\"" HIBIKI_EXPERIMENT_BUILD "\",\"compiler\":\"" HIBIKI_EXPERIMENT_COMPILER "\""
		<< ",\"scene\":\"plate\",\"method\":\"" << method << "\",\"grid_shape\":[" << nx << ',' << ny << ',' << nz
		<< "],\"grid_origin_m\":[" << origin.mX << ',' << origin.mY << ',' << origin.mZ
		<< "],\"grid_fingerprint\":\"" << std::hex << Test::GridFingerprint(grid) << std::dec
		<< "\",\"spacing_mm\":" << settings.mSpacing * 1000 << ",\"workers\":" << settings.mWorkerCount
		<< ",\"bake_ms\":" << ms << ",\"peak_bytes\":" << peak << "}\n";
	return grid;
}
} // namespace

int main(int argc, char **argv)
{
	try
	{
		std::cout << std::setprecision(9);
		if (argc == 2 && Name(argv[1]) == "--smoke")
		{
			for (Name method : staticMethods)
				SpatialRun("dense", method, 32, 8, 1, std::cout);
			for (Name method : incrementalMethods)
				SpatialRun("moving", method, 32, 8, 3, std::cout);
			const auto a = Bake("median", true, std::cout), b = Bake("morton", true, std::cout);
			Check(a.GetNodeCounts() == b.GetNodeCounts() && std::ranges::equal(a.GetValues(), b.GetValues(), [](double x, double y)
																			   { return std::bit_cast<std::uint64_t>(x) == std::bit_cast<std::uint64_t>(y); }),
				  "plate grids must be bit-identical");
			return 0;
		}
		Check(argc == 3, "usage: hibiki_spatial_benchmark <sparse|dense|clustered|thin|moving|plate> <method>");
		const Name scene = argv[1], method = argv[2];
		if (scene == "plate")
			(void)Bake(method, false, std::cout);
		else
		{
			const bool isIncremental = std::ranges::find(incrementalMethods, method) != incrementalMethods.end();
			Check(!isIncremental || scene == "moving", "incremental method requires moving workload");
			SpatialRun(scene, method, 1024, 128, scene == "moving" ? 9 : 1, std::cout);
		}
		return 0;
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
