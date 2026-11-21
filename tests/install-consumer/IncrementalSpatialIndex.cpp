#include <HibikiPhysics/FourWaySpatialIndex.h>
#include <HibikiPhysics/IncrementalSpatialIndex.h>
#include <HibikiPhysics/SweepAndPrune.h>

#include "../fixtures/SpatialConformance.h"

#include <iostream>
#include <latch>
#include <thread>

namespace
{
using namespace HibikiPhysics;
using namespace HibikiPhysics::Test::SpatialReference;

AABB At(double x)
{
	return {{x, -1, 0}, {x + 1, 1, 0}};
}

void CheckFrame(const SpatialOverlapIndex &updated, const SpatialOverlapProvider &freshProvider, std::span<const SpatialEntry> entries)
{
	const auto rebuilt = freshProvider.Build(entries);
	const auto expected = AllPairs(entries);
	Check(updated.OverlapPairs() == expected && rebuilt->OverlapPairs() == expected, "Updated and rebuilt pairs agree with brute force");
	auto query = [&](const AABB &probe)
	{
		const auto ids = Matching(entries, [&](const AABB &bounds)
								  { return Overlap(probe, bounds); });
		Check(updated.Overlaps(probe) == ids && rebuilt->Overlaps(probe) == ids, "Updated and rebuilt box queries agree with brute force");
	};
	for (const auto &entry : entries)
		query(entry.mBounds);
	query(At(-100));
	query(At(100));
}

void CheckMotion(const IncrementalSpatialProvider &provider, const SpatialOverlapProvider &fresh, bool sweep)
{
	std::vector<SpatialEntry> entries;
	for (std::size_t i = 0; i < 64; ++i)
		entries.push_back({i * 3 + 7, At(static_cast<double>(i))});
	auto			   frame = provider.Build(entries);
	SpatialUpdateStats stats;
	const auto		   oldEntries = entries;
	auto			   old = std::shared_ptr<const IncrementalSpatialIndex>(std::move(frame));
	std::latch		   started(1);
	bool			   readerConsistent = true;
	std::jthread	   reader([&](std::stop_token stop)
							  {
		const auto pairs = AllPairs(oldEntries);
		started.count_down();
		do
		{
			if (old->OverlapPairs() != pairs || old->Overlaps(At(0)) != Ids{7, 10})
				readerConsistent = false;
		} while (!stop.stop_requested()); });
	started.wait();
	frame = old->Updated({}, stats);
	for (std::size_t i = 0; i < 90; ++i)
	{
		const std::size_t slot = (i * 29) % entries.size();
		auto			 &entry = entries[slot];
		const double	  x = i == 64 ? 3000 : entry.mBounds.GetMin()[0] + 0.03125;
		entry.mBounds = At(x);
		frame = frame->Updated({{}, std::span(&entry, 1), {}}, stats);
		Check(stats.mTopologyRebuilds == 0, "Motion reused the topology");
		if (sweep)
		{
			Check(stats.mEndpointsChanged == 2 && stats.mRefitNodes == 0, "Sweep repairs moved endpoints");
			Check(i == 64 ? stats.mEndpointSwaps > 60 : stats.mEndpointSwaps <= 2, "Endpoint crossings track teleport and coherent motion");
		}
		else
			Check(stats.mRefitNodes == 3 && stats.mEndpointSwaps == 0, "One of 64 leaves refits only its three ancestors");
		CheckFrame(*frame, fresh, entries);
	}
	reader.request_stop();
	reader.join();
	Check(readerConsistent, "Old frame changed while reading");
	CheckFrame(*old, fresh, oldEntries);
}

void CheckBatchMotion(const IncrementalSpatialProvider &provider, const SpatialOverlapProvider &fresh, bool sweep)
{
	auto		 entries = Boxes();
	auto		 frame = provider.Build(entries);
	std::mt19937 random(159);
	for (std::size_t step = 0; step < 36; ++step)
	{
		std::shuffle(entries.begin(), entries.end(), random);
		const auto moved = std::span(entries).first(12);
		for (auto &entry : moved)
		{
			auto	   lo = entry.mBounds.GetMin(), hi = entry.mBounds.GetMax();
			const auto axis = step % 3;
			lo[axis] = static_cast<double>(random() % 25) - 12;
			hi[axis] = lo[axis] + static_cast<double>(random() % 4);
			entry.mBounds = {lo, hi};
		}
		SpatialUpdateStats stats;
		frame = frame->Updated({{}, moved, {}}, stats);
		Check(stats.mTopologyRebuilds == 0, "A multi-leaf motion batch preserves topology");
		if (sweep && step % 3 != 0)
			Check(stats.mEndpointsChanged == 0 && stats.mEndpointSwaps == 0, "Moving along Y or Z keeps all X endpoints");
		CheckFrame(*frame, fresh, entries);
	}
}

void CheckMembership(const IncrementalSpatialProvider &provider, const SpatialOverlapProvider &fresh, bool sweep)
{
	auto			   entries = Boxes();
	auto			   frame = provider.Build(entries);
	SpatialUpdateStats stats;
	for (std::size_t step = 0; step < 20; ++step)
	{
		std::rotate(entries.begin(), entries.begin() + 7, entries.end());
		const auto removed = std::array{entries.back().mId, entries[0].mId};
		const auto inserted = std::array{SpatialEntry{2000 + step, At(7)}, SpatialEntry{1000 + step, At(8)}};
		const auto moved = std::array{SpatialEntry{entries[1].mId, At(7)}, SpatialEntry{entries[2].mId, At(7)}};
		auto	   next = frame->Updated({inserted, moved, removed}, stats);
		Check(stats.mTopologyRebuilds == (sweep ? 0u : 1u), "Membership rebuild accounting");
		CheckFrame(*frame, fresh, entries);
		entries[0] = inserted[0];
		entries.back() = inserted[1];
		entries[1] = moved[0];
		entries[2] = moved[1];
		CheckFrame(*next, fresh, entries);
		frame = std::move(next);
	}
	Ids ids;
	for (const auto &entry : entries)
		ids.push_back(entry.mId);
	auto empty = frame->Updated({{}, {}, ids}, stats);
	CheckFrame(*empty, fresh, {});
	const std::array singleton{SpatialEntry{std::numeric_limits<std::size_t>::max(), At(0)}};
	auto			 one = empty->Updated({singleton, {}, {}}, stats);
	CheckFrame(*one, fresh, singleton);
}

void CheckRejected(const IncrementalSpatialProvider &provider)
{
	const std::array   entries{SpatialEntry{1, At(0)}, SpatialEntry{2, At(1)}};
	const auto		   frame = provider.Build(entries);
	const auto		   added = std::array{SpatialEntry{3, At(4)}};
	const auto		   repeatedInsert = std::array{added[0], added[0]};
	const auto		   repeatedMove = std::array{entries[1], entries[1]};
	SpatialUpdateStats stats{9, 8, 7, 6};
	auto			   reject = [&](SpatialUpdateBatch batch)
	{
		ExpectInvalid([&]
					  { (void)frame->Updated(batch, stats); });
		Check(stats.mEndpointSwaps == 9 && stats.mRefitNodes == 8 && stats.mTopologyRebuilds == 7 && stats.mEndpointsChanged == 6, "Failed update changed stats");
		Check(frame->OverlapPairs() == Pairs{{1, 2}} && frame->Overlaps(At(0)) == Ids{1, 2}, "Failed update changed snapshot");
	};
	reject({entries, {}, {}});
	reject({{}, added, {}});
	reject({{}, {}, std::array<std::size_t, 1>{3}});
	reject({repeatedInsert, {}, {}});
	reject({added, repeatedMove, {}});
	reject({{}, {}, std::array<std::size_t, 2>{1, 1}});
	reject({added, entries, std::array<std::size_t, 1>{2}});
	reject({added, added, {}});
	reject({added, {}, std::array<std::size_t, 1>{3}});
	reject({{}, {}, std::array<std::size_t, 3>{1, 2, 3}});
	const AABB unbounded({}, {0, std::numeric_limits<double>::infinity(), 0});
	reject({std::array{SpatialEntry{3, unbounded}}, {}, {}});
	reject({added, std::array{SpatialEntry{1, At(4)}, SpatialEntry{2, unbounded}}, {}});
	const auto unchanged = frame->Updated({}, stats);
	Check(unchanged->OverlapPairs() == Pairs{{1, 2}} && stats.mEndpointSwaps == 0 && stats.mRefitNodes == 0 && stats.mTopologyRebuilds == 0 && stats.mEndpointsChanged == 0, "Empty update reports zero work");
}

void CheckProvider(bool sweep)
{
	CapabilityConfig config;
	config.Add(FourWayUpdatePlugin());
	if (sweep)
		config.Replace(SweepAndPruneUpdatePlugin());
	Dependencies required;
	IncrementalSpatialSlot().Declare(required);
	const auto scope = config.Build(required);
	const auto selected = scope.GetSelections();
	Check(selected.size() == 1 && selected[0].mSlot == "spatial.incremental_overlap" &&
			  selected[0].mImplementation == (sweep ? "hibiki.sweep_and_prune" : "hibiki.four_way_bvh"),
		  "Incremental capability selection");
	const auto &provider = IncrementalSpatialSlot().Get(scope);
	const auto	fresh = sweep ? MakeSweepAndPruneProvider() : MakeFourWaySpatialOverlapProvider();
	OverlapContract(provider);
	CheckRejected(provider);
	CheckMotion(provider, *fresh, sweep);
	CheckBatchMotion(provider, *fresh, sweep);
	CheckMembership(provider, *fresh, sweep);
}
} // namespace

int main()
{
	try
	{
		CheckProvider(true);
		CheckProvider(false);
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
