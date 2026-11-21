#pragma once

#include "SpatialConformance.h"
#include <HibikiPhysics/FilteredSpatialIndex.h>
#include <string_view>

namespace HibikiPhysics::Test::SpatialBenchmark
{
using SpatialReference::Point;
constexpr unsigned Seed = 20261121;

inline AABB Centered(Point center, Point radius)
{
	Point lo = center, hi = center;
	for (std::size_t axis = 0; axis < 3; ++axis)
	{
		lo[axis] -= radius[axis];
		hi[axis] += radius[axis];
	}
	return {lo, hi};
}

inline std::vector<SpatialEntry> Scene(std::string_view name, std::size_t count)
{
	std::mt19937			  random(Seed);
	std::vector<SpatialEntry> entries;
	for (std::size_t i = 0; i < count; ++i)
	{
		const auto cluster = random() % 8;
		Point	   center{}, radius{};
		for (std::size_t axis = 0; axis < 3; ++axis)
		{
			// Use the generator's integer output directly so samples are portable.
			const auto position = random(), size = random();
			if (name == "sparse")
			{
				center[axis] = position % 81;
				radius[axis] = 1 + size % 2;
			}
			else if (name == "dense")
			{
				center[axis] = position % 13;
				radius[axis] = 2 + size % 3;
			}
			else if (name == "thin")
			{
				center[axis] = axis == 2 ? (position % 13) * 0.125 : position % 81;
				radius[axis] = axis == 2 ? (size % 2) * 0.0625 : 2 + size % 7;
			}
			else if (name == "clustered" || name == "moving")
			{
				center[axis] = ((cluster >> axis) & 1) * 36 + position % 13;
				radius[axis] = 1 + size % 3;
			}
			else
				throw std::invalid_argument("unknown spatial workload");
		}
		entries.push_back({(count - 1 - i) * 7 + 3, Centered(center, radius)});
	}
	return entries;
}

// The same quarter of the bodies drift by one eighth or one quarter unit on each axis.
inline std::vector<SpatialEntry> Move(std::vector<SpatialEntry> &entries)
{
	std::vector<SpatialEntry> changed;
	for (std::size_t i = 0; i < entries.size(); i += 4)
	{
		auto &entry = entries[i];
		auto  lo = entry.mBounds.GetMin(), hi = entry.mBounds.GetMax();
		for (std::size_t axis = 0; axis < 3; ++axis)
		{
			const double step = ((entry.mId >> axis) & 1) ? 0.25 : -0.125;
			lo[axis] += step;
			hi[axis] += step;
		}
		entry.mBounds = {lo, hi};
		changed.push_back(entry);
	}
	return changed;
}

inline std::vector<SpatialEntry> Queries(const std::vector<SpatialEntry> &entries, std::size_t count)
{
	std::vector<SpatialEntry> queries;
	for (std::size_t i = 0; i < count; ++i)
	{
		auto query = entries[(i * 73) % entries.size()];
		if (i % 8 == 0)
			query.mBounds = Centered({200., 200., 200.}, {1., 1., 1.});
		else if (i % 8 == 1)
			query.mBounds = {query.mBounds.GetMax(), query.mBounds.GetMax()};
		queries.push_back(query);
	}
	return queries;
}

inline std::vector<TaggedSpatialEntry> Tag(const std::vector<SpatialEntry> &entries)
{
	std::vector<TaggedSpatialEntry> tagged;
	for (const auto &entry : entries)
	{
		// Most neighbours have a common owner. Every seventh ordinal gets a distinct owner.
		const auto ordinal = (entry.mId - 3) / 7;
		const auto owner = ordinal % 7 ? static_cast<std::size_t>((entry.mBounds.GetMin()[0] + 32) / 16) : 100 + ordinal;
		tagged.push_back({entry, {owner, ordinal % 3}});
	}
	return tagged;
}

inline bool Allowed(const SpatialMetadata &left, const SpatialMetadata &right)
{
	return left.mOwner != right.mOwner && left.mCategory == right.mCategory;
}
} // namespace HibikiPhysics::Test::SpatialBenchmark
