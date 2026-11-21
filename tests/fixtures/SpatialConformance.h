#pragma once

#include <HibikiPhysics/SpatialIndex.h>
#include <algorithm>
#include <cmath>
#include <future>
#include <limits>
#include <random>
#include <stdexcept>

namespace HibikiPhysics::Test
{
namespace SpatialReference
{
using Point = std::array<double, 3>;
using Ids = std::vector<std::size_t>;
using Pairs = std::vector<std::pair<std::size_t, std::size_t>>;

inline void Check(bool condition, const char *message)
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
	throw std::logic_error("Expected invalid spatial input to be rejected");
}

inline bool Overlap(const AABB &a, const AABB &b)
{
	for (int axis = 0; axis < 3; ++axis)
		if (std::min(a.GetMax()[axis], b.GetMax()[axis]) < std::max(a.GetMin()[axis], b.GetMin()[axis]))
			return false;
	return true;
}

template <class Predicate>
Ids Matching(std::span<const SpatialEntry> entries, Predicate matches)
{
	Ids ids;
	for (const auto &entry : entries)
		if (matches(entry.mBounds))
			ids.push_back(entry.mId);
	std::ranges::sort(ids);
	return ids;
}

inline Pairs AllPairs(std::span<const SpatialEntry> entries)
{
	Pairs pairs;
	for (std::size_t a = 0; a < entries.size(); ++a)
		for (std::size_t b = 0; b < a; ++b)
			if (Overlap(entries[a].mBounds, entries[b].mBounds))
				pairs.emplace_back(std::minmax(entries[a].mId, entries[b].mId));
	std::ranges::sort(pairs);
	return pairs;
}

// Exhaustive face intersection for small integral boxes and rays. A forward ray
// meeting a closed box starts inside or crosses one of its six boundary faces.
inline bool RayHits(const Point &origin, const Point &direction, const AABB &bounds)
{
	if (bounds.Contains(origin))
		return true;
	for (int axis = 0; axis < 3; ++axis)
	{
		if (direction[axis] == 0)
			continue;
		for (auto face : {bounds.GetMin(), bounds.GetMax()})
		{
			const double t = (face[axis] - origin[axis]) / direction[axis];
			if (t < 0)
				continue;
			Point hit = origin;
			for (int j = 0; j < 3; ++j)
				hit[j] += t * direction[j];
			if (bounds.Contains(hit))
				return true;
		}
	}
	return false;
}

template <class Range>
Range Sorted(Range values)
{
	std::ranges::sort(values);
	return values;
}
inline std::vector<SpatialEntry> Boxes()
{
	std::mt19937 random(8729);
	auto		 coordinate = [&]
	{ return static_cast<double>(random() % 21) - 10; };
	std::vector<SpatialEntry> entries;
	for (std::size_t i = 0; i < 96; ++i)
	{
		Point lo{coordinate(), coordinate(), coordinate()}, hi = lo;
		for (auto &value : hi)
			value += static_cast<double>(random() % 5);
		entries.push_back({i * 3 + 7, {lo, hi}});
	}
	return entries;
}

template <class Provider>
void OverlapContract(const Provider &provider)
{
	auto verify = [&](std::vector<SpatialEntry> entries)
	{
		const auto index = provider.Build(entries);
		const auto pairs = AllPairs(entries);
		Check(Sorted(index->OverlapPairs()) == pairs, "unordered self-pairs agree with exhaustive closed-box overlaps");
		for (const auto &query : entries)
		{
			const auto expected = Matching(entries, [&](const AABB &bounds)
										   { return Overlap(query.mBounds, bounds); });
			Check(Sorted(index->Overlaps(query.mBounds)) == expected, "overlap candidates agree with brute force");
		}
		auto concurrent = std::async(std::launch::async, [&]
									 { return Sorted(index->OverlapPairs()); });
		std::ranges::reverse(entries);
		const auto rebuilt = provider.Build(entries);
		entries.clear();
		Check(Sorted(rebuilt->OverlapPairs()) == pairs && concurrent.get() == pairs, "snapshots own entries and support concurrent reads");
	};
	verify(Boxes());
	const AABB unit({0, 0, 0}, {1, 1, 1}), corner({1, 1, 1}, {1, 1, 1});
	verify({{9, unit}, {17, unit}, {3, corner}, {2, {{1, 0, 0}, {2, 1, 1}}}, {39, {{1, 1, 0}, {2, 2, 1}}}, {88, {{1, 1, 1}, {2, 2, 2}}}});
	std::vector<SpatialEntry> coincident;
	for (std::size_t i = 0; i < 59; ++i)
	{
		const double t = static_cast<double>(i % 7);
		coincident.push_back({101 - i, {{-t, -1, 0}, {t, 1, 0}}});
	}
	verify(std::move(coincident));
	const auto empty = provider.Build({});
	Check(empty->Overlaps(unit).empty() && empty->OverlapPairs().empty(), "empty overlap index");
	const auto maxId = std::numeric_limits<std::size_t>::max();
	const auto one = provider.Build(std::array<SpatialEntry, 1>{{{maxId, unit}}});
	Check(one->OverlapPairs().empty() && one->Overlaps(corner) == Ids{maxId}, "all ID bits usable");
	const double inf = std::numeric_limits<double>::infinity();
	const AABB	 unbounded({-inf, 0, 0}, {inf, 1, 1});
	ExpectInvalid([&]
				  { (void)empty->Overlaps(unbounded); });
	ExpectInvalid([&]
				  { (void)provider.Build(std::array<SpatialEntry, 1>{{{7, unbounded}}}); });
	ExpectInvalid([&]
				  { (void)provider.Build(std::array<SpatialEntry, 2>{{{7, unit}, {7, corner}}}); });
}
} // namespace SpatialReference

inline void CheckOverlapProvider(const SpatialOverlapProvider &provider)
{
	SpatialReference::OverlapContract(provider);
}

inline void CheckQueryProvider(const SpatialQueryProvider &provider)
{
	using namespace SpatialReference;
	OverlapContract(provider);
	const auto	 entries = Boxes();
	const auto	 index = provider.Build(entries);
	std::mt19937 random(7171);
	auto		 coordinate = [&]
	{ return static_cast<double>(random() % 21) - 10; };
	for (int i = 0; i < 48; ++i)
	{
		const Point p{coordinate(), coordinate(), coordinate()};
		const Point d{i % 2 ? 1. : -1., i % 3 ? 0. : -2., i % 5 ? .5 : 0.};
		Check(Sorted(index->RayCandidates(p, d)) == Matching(entries, [&](const AABB &a)
															 { return RayHits(p, d, a); }),
			  "exhaustive ray intersection");
		auto exact = [&](std::size_t id)
		{
			const auto &box = entries.at((id - 7) / 3).mBounds;
			const auto	site = id % 2 ? box.GetMin() : box.GetMax();
			return std::hypot(site[0] - p[0], site[1] - p[1], site[2] - p[2]);
		};
		std::pair<double, std::size_t> expected{std::numeric_limits<double>::infinity(), 0};
		for (const auto &v : entries)
			expected = std::min(expected, {exact(v.mId), v.mId});
		const auto best = index->Nearest(p, exact);
		Check(best && best->mId == expected.second && best->mDistance == expected.first, "nearest agrees with exhaustive leaf distances");
	}
	const AABB unit({0, 0, 0}, {1, 1, 1});
	const auto tied = provider.Build(std::array<SpatialEntry, 2>{{{9, unit}, {2, unit}}});
	Check(Sorted(tied->RayCandidates({1, 1, 1}, {0, 0, 0})) == Ids{2, 9}, "stationary ray includes touching boxes");
	Check(tied->Nearest({}, [](std::size_t)
						{ return 0.; })
				  ->mId == 2,
		  "ties select minimum caller ID");
	Check(!tied->Nearest({}, [](std::size_t)
						 { return std::numeric_limits<double>::infinity(); }),
		  "infinite distance skips a leaf");
}
} // namespace HibikiPhysics::Test
