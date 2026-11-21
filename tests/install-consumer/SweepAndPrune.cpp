#include <HibikiPhysics/SweepAndPrune.h>

#include "../fixtures/SpatialConformance.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <future>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{
using namespace HibikiPhysics;
using Ids = std::vector<std::size_t>;
using Pairs = std::vector<std::pair<std::size_t, std::size_t>>;

void Check(bool condition, const char *message)
{
	if (!condition)
		throw std::logic_error(message);
}

template <class Operation>
void ExpectInvalid(Operation operation)
{
	try
	{
		operation();
	}
	catch (const std::invalid_argument &)
	{
		return;
	}
	throw std::logic_error("Expected invalid sweep-and-prune input to throw");
}

void CheckEndpointOrder(const SpatialOverlapProvider &provider)
{
	// One interval closes where two begin and two zero-width points meet.
	const double					  neighbor = std::nextafter(0.0, 1.0);
	const std::array<SpatialEntry, 6> entries{{
		{40, {{-1, -1, -1}, {-0.0, 0, 0}}},
		{10, {{0, 0, 0}, {1, 1, 1}}},
		{20, {{-0.0, 0, 0}, {0, 0, 0}}},
		{50, {{0, 0, 0}, {-0.0, 0, 0}}},
		{30, {{0, 0, 2}, {0, 0, 3}}},
		{60, {{neighbor, -1, -1}, {neighbor, -1, -1}}},
	}};
	const auto						  index = provider.Build(entries);
	Check(index->OverlapPairs() == Pairs{{10, 20}, {10, 40}, {10, 50}, {20, 40}, {20, 50}, {40, 50}}, "Starts precede ends at signed zero");
	Check(index->Overlaps({{}, {}}) == Ids{10, 20, 40, 50}, "Point query includes each coincident start and end");
	Check(index->Overlaps({{neighbor, 0, 0}, {neighbor, 0, 0}}) == Ids{10}, "One representable step separates intervals");
}

void CheckActiveSet(const SpatialOverlapProvider &provider)
{
	std::vector<SpatialEntry> entries;
	Pairs					  expected;
	for (std::size_t i = 0; i < 72; ++i)
	{
		const double y = static_cast<double>(i % 3), z = static_cast<double>(i % 2);
		entries.push_back({i, {{-1, y, z}, {static_cast<double>(i % 5), y, z}}});
		for (std::size_t j = 0; j < i; ++j)
			if (j % 6 == i % 6)
				expected.emplace_back(j, i);
	}
	std::ranges::sort(expected);
	std::ranges::reverse(entries);
	const auto index = provider.Build(entries);
	entries.clear();
	auto pending = std::async(std::launch::async, [&]
							  { return index->OverlapPairs(); });
	Check(index->OverlapPairs() == expected && pending.get() == expected, "Dense projection filters both secondary axes with concurrent readers");

	// Gaps separate successive point boxes. Queries sample the beginning, interior, and end.
	for (std::size_t i = 0; i < 4096; ++i)
	{
		const double x = 2 * static_cast<double>(i);
		entries.push_back({i, {{x, 0, 0}, {x, 0, 0}}});
	}
	const auto sparse = provider.Build(entries);
	Check(sparse->OverlapPairs().empty(), "X-separated points produce no pairs");
	for (std::size_t i : {0, 2037, 4095})
	{
		const double x = 2 * static_cast<double>(i);
		Check(sparse->Overlaps({{x, 0, 0}, {x, 0, 0}}) == Ids{i}, "Narrow query inside a sparse set");
		Check(sparse->Overlaps({{x + 1, 0, 0}, {x + 1, 0, 0}}).empty(), "Query in a gap");
	}
}

void CheckExtremesAndErrors(const SpatialOverlapProvider &provider)
{
	const double					  limit = std::numeric_limits<double>::max(), inf = std::numeric_limits<double>::infinity();
	const auto						  maxId = std::numeric_limits<std::size_t>::max();
	const AABB						  allFinite({-limit, -limit, -limit}, {limit, limit, limit});
	const std::array<SpatialEntry, 2> entries{{{maxId, allFinite}, {0, {{limit, -limit, limit}, {limit, -limit, limit}}}}};
	const auto						  index = provider.Build(entries);
	Check(index->OverlapPairs() == Pairs{{0, maxId}} && index->Overlaps(allFinite) == Ids{0, maxId}, "Finite extremes and full ID range");

	const auto empty = provider.Build({});
	Check(empty->OverlapPairs().empty() && empty->Overlaps(allFinite).empty(), "Empty operations");
	ExpectInvalid([&]
				  { (void)provider.Build(std::array{entries[0], entries[0]}); });
	for (const AABB &unbounded : {AABB({-inf, 0, 0}, {}), AABB({}, {0, inf, 0}), AABB({}, {0, 0, inf})})
	{
		ExpectInvalid([&]
					  { (void)provider.Build(std::array{SpatialEntry{42, unbounded}}); });
		ExpectInvalid([&]
					  { (void)empty->Overlaps(unbounded); });
	}
}

void CheckRegistration()
{
	CapabilityConfig config;
	config.Add(CpuSpatialOverlapPlugin());
	config.Replace(SweepAndPrunePlugin());
	Dependencies overlap;
	SpatialOverlapSlot().Declare(overlap);
	const auto scope = config.Build(overlap);
	const auto selected = scope.GetSelections();
	Check(selected.size() == 1 && selected[0].mSlot == "spatial.overlap" && selected[0].mImplementation == "hibiki.sweep_and_prune" && selected[0].mReplaced, "Overlap slot replacement");
	Test::CheckOverlapProvider(SpatialOverlapSlot().Get(scope));
}
} // namespace

int main()
{
	try
	{
		const auto provider = HibikiPhysics::MakeSweepAndPruneProvider();
		CheckEndpointOrder(*provider);
		CheckActiveSet(*provider);
		CheckExtremesAndErrors(*provider);
		CheckRegistration();
	}
	catch (const std::exception &error)
	{
		std::cerr << error.what() << '\n';
		return 1;
	}
}
