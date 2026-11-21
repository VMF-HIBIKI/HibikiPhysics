#include <HibikiPhysics/MortonSpatialIndex.h>
#include <HibikiPhysics/SdfBaker.h>
#include "../fixtures/SpatialConformance.h"
#include "../fixtures/ThroughHolePlate.h"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <stdexcept>

namespace
{
using namespace HibikiPhysics;

void Check(bool condition, const char *message)
{
	if (!condition)
		throw std::logic_error(message);
}

std::vector<std::size_t> VisitAll(const SpatialQueryIndex &index, std::array<double, 3> point)
{
	std::vector<std::size_t> visited;
	const auto				 result = index.Nearest(point, [&](std::size_t id)
													{
										 visited.push_back(id);
										 return std::numeric_limits<double>::infinity(); });
	Check(!result, "All exact distances excluded");
	return visited;
}

void CheckPrefixSplit(const SpatialQueryProvider &provider)
{
	// The top differing bit isolates the distant point. Nearest visits the entire
	// near cluster before entering that subtree, even when exact hits are excluded.
	std::vector<SpatialEntry> entries;
	for (const auto [id, x] : {std::pair{45u, 1.0}, {80u, 100.0}, {19u, 0.0}, {72u, 2.0}})
		entries.push_back({id, {{x, 0, 0}, {x, 0, 0}}});
	const std::vector<std::size_t> expected{72, 45, 19, 80};
	Check(VisitAll(*provider.Build(entries), {3, 0, 0}) == expected, "Morton prefix separates the remote leaf");
	std::ranges::reverse(entries);
	Check(VisitAll(*provider.Build(entries), {3, 0, 0}) == expected, "Prefix split is independent of input order");
}

void CheckQuantization(const SpatialQueryProvider &provider)
{
	const auto highestId = std::numeric_limits<std::size_t>::max();
	const auto maximum = std::numeric_limits<double>::max();
	// Three distinct centers share a quantization cell near zero. Their boxes all
	// contain the query, so the stable ID suffix determines their visit order.
	std::vector<SpatialEntry> entries{{highestId, {{0, 0, 0}, {4, 0, 0}}},
									  {0, {{0, 0, 0}, {8, 0, 0}}},
									  {27, {{0, 0, 0}, {2, 0, 0}}},
									  {13, {{-maximum, 0, 0}, {-maximum, 0, 0}}},
									  {5, {{maximum, 0, 0}, {maximum, 0, 0}}}};
	const auto				  visited = VisitAll(*provider.Build(entries), {});
	std::vector<std::size_t>  tied;
	for (const auto id : visited)
		if (id != 13 && id != 5)
			tied.push_back(id);
	Check(tied == std::vector<std::size_t>{0, 27, highestId}, "Duplicate Morton keys use every stable ID bit");
	std::ranges::reverse(entries);
	Check(visited == VisitAll(*provider.Build(entries), {}), "Overflowing center extent is reproducible");
}

void CheckReplacement()
{
	CapabilityConfig config;
	config.Add(CpuSpatialQueryPlugin());
	config.Add(CpuSpatialOverlapPlugin());
	config.Replace(MortonSpatialQueryPlugin());
	config.Replace(MortonSpatialOverlapPlugin());
	Dependencies requirements;
	SpatialQuerySlot().Declare(requirements);
	SpatialOverlapSlot().Declare(requirements);
	const auto scope = config.Build(requirements);
	const auto selections = scope.GetSelections();
	Check(selections.size() == 2 && std::ranges::all_of(selections, [](const auto &selection)
														{ return selection.mReplaced && selection.mImplementation == "hibiki.morton_lbvh"; }),
		  "Both Morton registrations selected");
	Test::CheckOverlapProvider(SpatialOverlapSlot().Get(scope));

	const auto			  mesh = Test::MakeThroughHolePlate();
	const SdfBakeSettings settings{.005, {.005, .005, .005}, {.25, .5, 0}, 2};
	const auto			  reference = BakeDenseSdf(mesh, settings);
	const auto			  actual = BakeDenseSdf(mesh, settings, SpatialQuerySlot().Get(scope));
	const auto			  bits = [](double value)
	{ return std::bit_cast<std::uint64_t>(value); };
	const auto a = reference.GetOrigin(), b = actual.GetOrigin();
	Check(reference.GetNodeCounts() == actual.GetNodeCounts() &&
			  bits(a.mX) == bits(b.mX) && bits(a.mY) == bits(b.mY) && bits(a.mZ) == bits(b.mZ) &&
			  bits(reference.GetSpacing()) == bits(actual.GetSpacing()),
		  "Identical baked grid placement");
	Check(std::ranges::equal(reference.GetValues(), actual.GetValues(), {}, bits, bits), "Bitwise identical through-hole plate samples");
}
} // namespace

int main()
{
	const auto provider = HibikiPhysics::MakeMortonSpatialQueryProvider();
	HibikiPhysics::Test::CheckQueryProvider(*provider);
	CheckPrefixSplit(*provider);
	CheckQuantization(*provider);
	CheckReplacement();
}
