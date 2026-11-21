#include <HibikiPhysics/SpatialIndex.h>
#include "../fixtures/SpatialConformance.h"

#include <algorithm>
#include <cmath>
#include <future>
#include <limits>
#include <random>
#include <stdexcept>

namespace
{
using namespace HibikiPhysics;
using namespace HibikiPhysics::Test::SpatialReference;

void CheckBoundaries(const SpatialQueryProvider &provider)
{
	const AABB						  unit({0, 0, 0}, {1, 1, 1}), point({1, 1, 1}, {1, 1, 1});
	const std::array<SpatialEntry, 5> entries{{{17, unit}, {9, unit}, {2, point}, {80, {{1, 0, 0}, {2, 1, 1}}}, {51, {{10, 0, 0}, {11, 1, 1}}}}};
	const auto						  index = provider.Build(entries);
	Check(index->OverlapPairs() == AllPairs(entries), "Coincident, corner and face overlaps");
	const Ids touching{2, 9, 17, 80};
	Check(index->Overlaps(point) == touching, "Point AABB on boundary");
	Check(index->RayCandidates({1, 1, 1}, {-0.0, 0, 0}) == touching, "Stationary ray");
	Check(index->RayCandidates({2, 2, 2}, {-1, -1, -1}) == touching, "Negative direction hits a shared corner");
	Check(index->RayCandidates({2, 2, 2}, {1, 1, 1}).empty(), "Boxes behind ray");
	Check(index->RayCandidates({1, 1, 1}, {1, 0, 0}) == Ids{2, 9, 17, 51, 80}, "Origin and parallel boundary included");
	Check(index->RayCandidates({1, 1, std::nextafter(1.0, 2.0)}, {1, 0, -0.0}).empty(), "Parallel ray outside slab");

	std::vector<SpatialEntry> sameCenter;
	for (std::size_t i = 0; i < 50; ++i)
	{
		const double halfWidth = static_cast<double>(i % 4);
		sameCenter.push_back({100 - i, {{-halfWidth, -1, 0}, {halfWidth, 1, 0}}});
	}
	const auto coincident = provider.Build(sameCenter);
	Check(coincident->OverlapPairs() == AllPairs(sameCenter), "Unique pairs for equal split keys");
	auto trace = [&](const SpatialQueryIndex &built)
	{
		Ids		   visited;
		const auto best = built.Nearest({}, [&](std::size_t id)
										{ visited.push_back(id); return 0.0; });
		Check(best && best->mId == 51, "Coincident tie selects lowest stable ID");
		return visited;
	};
	std::ranges::reverse(sameCenter);
	Check(trace(*coincident) == trace(*provider.Build(sameCenter)), "Reproducible traversal for permuted input");
}

void CheckNearest(const SpatialQueryProvider &provider)
{
	const std::array<SpatialEntry, 4> entries{{{40, {{-10, 0, 0}, {0, 0, 0}}},
											   {1, {{1, 0, 0}, {1, 0, 0}}},
											   {15, {{-1, 0, 0}, {-1, 0, 0}}},
											   {8, {{100, 0, 0}, {101, 1, 1}}}}};
	const auto						  index = provider.Build(entries);
	Ids								  visited;
	const auto						  nearest = index->Nearest({}, [&](std::size_t id)
															   {
		visited.push_back(id);
		if (id == 40)
			return 10.0;
		if (id == 8)
			throw std::logic_error("Distant leaf should have been pruned");
		return 1.0; });
	Check(nearest && nearest->mId == 1 && nearest->mDistance == 1, "Exact hit wins and lowest tied ID retained");
	Check(std::ranges::find(visited, 15) != visited.end(), "Equal lower bounds were explored");
	const auto skipped = index->Nearest({}, [](std::size_t id)
										{ return id == 8 ? 100.0 : std::numeric_limits<double>::infinity(); });
	Check(skipped && skipped->mId == 8 && skipped->mDistance == 100, "Excluded leaves cannot prune valid hits");
	Check(!index->Nearest({}, [](std::size_t)
						  { return std::numeric_limits<double>::infinity(); }),
		  "All leaves excluded");
}

void CheckExtremes(const SpatialQueryProvider &provider)
{
	const double					  max = std::numeric_limits<double>::max(), tiny = std::numeric_limits<double>::denorm_min();
	const std::array<SpatialEntry, 3> entries{{{3, {{max, max, 0}, {max, max, 0}}},
											   {7, {{max / 2, max, 0}, {max / 2, max, 0}}},
											   {11, {{-max, -max, 0}, {-max, -max, 0}}}}};
	const auto						  index = provider.Build(entries);
	Check(index->RayCandidates({-max, -max, 0}, {max, max, 0}) == Ids{3, 11}, "Overflowing coordinate differences");
	Check(index->RayCandidates({}, {tiny, tiny, 0}) == Ids{3}, "Ray times above double range remain ordered");
	const std::array<SpatialEntry, 2> subnormals{{{21, {{tiny, tiny, 0}, {tiny, tiny, 0}}}, {22, {{tiny * 2, tiny, 0}, {tiny * 2, tiny, 0}}}}};
	Check(provider.Build(subnormals)->RayCandidates({}, {max, max, 0}) == Ids{21}, "Ray times below double range remain ordered");
	const auto best = index->Nearest({max, max, 0}, [](std::size_t id)
									 {
		if (id != 3)
			throw std::logic_error("Far extreme leaf should be pruned");
		return 0.0; });
	Check(best && best->mId == 3, "Extreme coordinates preserve zero nearest distance");
}

void CheckInvalid(const SpatialQueryProvider &provider)
{
	const double inf = std::numeric_limits<double>::infinity(), nan = std::numeric_limits<double>::quiet_NaN();
	const AABB	 unit({0, 0, 0}, {1, 1, 1}), unbounded({-inf, 0, 0}, {inf, 0, 0});
	const auto	 empty = provider.Build({});
	Check(empty->Overlaps(unit).empty() && empty->OverlapPairs().empty() && empty->RayCandidates({}, {}).empty(), "Empty candidates");
	Check(!empty->Nearest({}, [](std::size_t) -> double
						  { throw std::logic_error("Empty leaf"); }),
		  "Empty nearest");
	ExpectInvalid([&]
				  { (void)empty->Overlaps(unbounded); });
	ExpectInvalid([&]
				  { (void)empty->RayCandidates({0, inf, 0}, {}); });
	ExpectInvalid([&]
				  { (void)empty->RayCandidates({}, {0, nan, 0}); });
	ExpectInvalid([&]
				  { (void)empty->Nearest({0, 0, inf}, [](std::size_t)
										 { return 0.0; }); });
	ExpectInvalid([&]
				  { (void)empty->Nearest({}, {}); });
	ExpectInvalid([&]
				  { (void)provider.Build(std::array<SpatialEntry, 2>{{{10, unit}, {10, unit}}}); });
	ExpectInvalid([&]
				  { (void)provider.Build(std::array<SpatialEntry, 1>{{{0, unbounded}}}); });
	const auto one = provider.Build(std::array<SpatialEntry, 1>{{{std::numeric_limits<std::size_t>::max(), unit}}});
	Check(one->OverlapPairs().empty() && one->Overlaps(unit) == Ids{std::numeric_limits<std::size_t>::max()}, "All caller ID bits usable");
	for (double bad : {-inf, -1.0, nan})
		ExpectInvalid([&]
					  { (void)one->Nearest({}, [bad](std::size_t)
										   { return bad; }); });
}

class LinearOverlapIndex final : public SpatialOverlapIndex
{
  public:
	explicit LinearOverlapIndex(std::span<const SpatialEntry> entries) : mEntries(entries.begin(), entries.end()) {}
	Ids Overlaps(const AABB &bounds) const override
	{
		return Matching(mEntries, [&](const AABB &entry)
						{ return Overlap(bounds, entry); });
	}
	Pairs OverlapPairs() const override { return AllPairs(mEntries); }

  private:
	std::vector<SpatialEntry> mEntries;
};

class LinearOverlapProvider final : public SpatialOverlapProvider
{
  public:
	std::unique_ptr<const SpatialOverlapIndex> Build(std::span<const SpatialEntry> entries) const override { return std::make_unique<LinearOverlapIndex>(entries); }
};

class CountingQueryProvider final : public SpatialQueryProvider
{
  public:
	explicit CountingQueryProvider(std::size_t &builds) : mBuilds(builds) {}
	std::unique_ptr<const SpatialQueryIndex> Build(std::span<const SpatialEntry> entries) const override
	{
		++mBuilds;
		return MakeCpuSpatialQueryProvider()->Build(entries);
	}

  private:
	std::size_t &mBuilds;
};

void CheckRegistration()
{
	CapabilityConfig defaults;
	defaults.Add(CpuSpatialOverlapPlugin());
	Dependencies overlapOnly;
	SpatialOverlapSlot().Declare(overlapOnly);
	const auto defaultScope = defaults.Build(overlapOnly);
	HibikiPhysics::Test::CheckOverlapProvider(SpatialOverlapSlot().Get(defaultScope));
	const AABB						  point({}, {});
	const std::array<SpatialEntry, 2> entries{{{12, point}, {5, point}}};
	const Pairs						  expected{{5, 12}};
	Check(SpatialOverlapSlot().Get(defaultScope).Build(entries)->OverlapPairs() == expected, "Default overlap capability alone");
	auto				  modified = defaults;
	LinearOverlapProvider linear;
	modified.Replace(SpatialOverlapSlot().Borrow("test.linear", linear));
	const auto replaced = modified.Build(overlapOnly);
	Check(&SpatialOverlapSlot().Get(replaced) == &linear && replaced.GetSelections().size() == 1, "Pair-only provider selected");
	Check(linear.Build(entries)->OverlapPairs() == expected, "Pair-only implementation");

	defaults.Add(CpuSpatialQueryPlugin());
	Dependencies rayAndNearest;
	SpatialQuerySlot().Declare(rayAndNearest);
	const auto cpu = defaults.Build(rayAndNearest);
	Check(cpu.GetSelections().size() == 1 && !cpu.GetSelections()[0].mReplaced, "Ray and nearest require one explicit provider");
	auto		experiment = defaults;
	std::size_t builds = 0;
	experiment.Replace(SpatialQuerySlot().Factory("test.counting", [&]
												  { return std::make_unique<CountingQueryProvider>(builds); }));
	const auto				   scoped = experiment.Build(rayAndNearest);
	const auto				   index = SpatialQuerySlot().Get(scoped).Build(entries);
	const SpatialOverlapIndex &overlap = *index;
	Check(overlap.OverlapPairs() == expected && index->RayCandidates({}, {}) == Ids{5, 12}, "One selected index serves overlap and ray");
	Check(index->Nearest({}, [](std::size_t)
						 { return 0.0; })
					  ->mId == 5 &&
			  builds == 1,
		  "Same index serves nearest");
	Check(!SpatialQuerySlot().Get(cpu).Build({})->Nearest({}, [](std::size_t)
														  { return 0.0; }) &&
			  builds == 1,
		  "Independent default scope");
}
} // namespace

int main()
{
	const auto provider = HibikiPhysics::MakeCpuSpatialQueryProvider();
	HibikiPhysics::Test::CheckQueryProvider(*provider);
	CheckBoundaries(*provider);
	CheckNearest(*provider);
	CheckExtremes(*provider);
	CheckInvalid(*provider);
	CheckRegistration();
}
