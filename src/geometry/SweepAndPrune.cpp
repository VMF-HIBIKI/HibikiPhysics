#include <HibikiPhysics/SweepAndPrune.h>

#include <algorithm>
#include <cmath>
#include <compare>
#include <limits>
#include <stdexcept>

namespace HibikiPhysics
{
namespace
{
using Pairs = std::vector<std::pair<std::size_t, std::size_t>>;

void RequireFiniteBounds(const AABB &bounds)
{
	const auto low = bounds.GetMin(), high = bounds.GetMax();
	for (std::size_t axis = 0; axis < low.size(); ++axis)
		if (!std::isfinite(low[axis]) || !std::isfinite(high[axis]))
			throw std::invalid_argument("Sweep-and-prune requires finite AABB endpoints");
}

bool OverlapsYZ(const AABB &a, const AABB &b)
{
	const auto aMin = a.GetMin(), aMax = a.GetMax(), bMin = b.GetMin(), bMax = b.GetMax();
	return aMin[1] <= bMax[1] && bMin[1] <= aMax[1] && aMin[2] <= bMax[2] && bMin[2] <= aMax[2];
}

enum class EndpointKind
{
	Start,
	End
};

// Sorting by kind opens every interval at a coordinate before closing any interval there.
// Entry slots follow stable ID order, including when the caller permutes the input.
struct Endpoint
{
	double		 mCoordinate;
	EndpointKind mKind;
	std::size_t	 mEntry;
	auto		 operator<=>(const Endpoint &) const = default;
};

class SweepAndPruneIndex final : public SpatialOverlapIndex
{
  public:
	explicit SweepAndPruneIndex(std::span<const SpatialEntry> entries) : mEntries(entries.begin(), entries.end())
	{
		std::ranges::sort(mEntries, {}, &SpatialEntry::mId);
		if (std::ranges::adjacent_find(mEntries, {}, &SpatialEntry::mId) != mEntries.end())
			throw std::invalid_argument("Duplicate spatial entry ID");
		if (mEntries.size() > mEndpoints.max_size() / 2)
			throw std::length_error("Too many sweep-and-prune entries");
		mEndpoints.reserve(2 * mEntries.size());
		for (std::size_t i = 0; i < mEntries.size(); ++i)
		{
			const auto &bounds = mEntries[i].mBounds;
			RequireFiniteBounds(bounds);
			mEndpoints.push_back({bounds.GetMin()[0], EndpointKind::Start, i});
			mEndpoints.push_back({bounds.GetMax()[0], EndpointKind::End, i});
		}
		std::ranges::sort(mEndpoints);

		// For a box query, skip the entire prefix once every earlier start ends left of the query.
		double maxEnd = -std::numeric_limits<double>::infinity();
		mPrefixMaxEnd.reserve(mEndpoints.size());
		for (const auto &endpoint : mEndpoints)
		{
			if (endpoint.mKind == EndpointKind::Start)
				maxEnd = std::max(maxEnd, mEntries[endpoint.mEntry].mBounds.GetMax()[0]);
			mPrefixMaxEnd.push_back(maxEnd);
		}
	}

	std::vector<std::size_t> Overlaps(const AABB &bounds) const override
	{
		RequireFiniteBounds(bounds);
		std::vector<std::size_t> result;
		const double			 minX = bounds.GetMin()[0], maxX = bounds.GetMax()[0];
		// All eligible starts lie in [first, last). The prefix maxima are nondecreasing.
		const auto first = std::ranges::lower_bound(mPrefixMaxEnd, minX) - mPrefixMaxEnd.begin();
		const auto last = std::ranges::upper_bound(mEndpoints, maxX, {}, &Endpoint::mCoordinate);
		for (auto it = mEndpoints.begin() + first; it != last; ++it)
		{
			if (it->mKind == EndpointKind::Start)
			{
				const auto &entry = mEntries[it->mEntry];
				if (entry.mBounds.GetMax()[0] >= minX && OverlapsYZ(entry.mBounds, bounds))
					result.push_back(entry.mId);
			}
		}
		std::ranges::sort(result);
		return result;
	}

	Pairs OverlapPairs() const override
	{
		Pairs					 result;
		std::vector<std::size_t> active;
		std::vector<std::size_t> positions(mEntries.size());
		for (const auto &endpoint : mEndpoints)
		{
			const auto slot = endpoint.mEntry;
			if (endpoint.mKind == EndpointKind::Start)
			{
				const auto &entering = mEntries[slot];
				for (auto other : active)
				{
					const auto &candidate = mEntries[other];
					if (OverlapsYZ(entering.mBounds, candidate.mBounds))
						result.emplace_back(std::minmax(entering.mId, candidate.mId));
				}
				positions[slot] = active.size();
				active.push_back(slot);
			}
			else
			{
				// Swap and pop using the slot's current position, even when it is last.
				const auto position = positions[slot];
				active[position] = active.back();
				positions[active.back()] = position;
				active.pop_back();
			}
		}
		std::ranges::sort(result);
		return result;
	}

  private:
	std::vector<SpatialEntry> mEntries;
	std::vector<Endpoint>	  mEndpoints;
	std::vector<double>		  mPrefixMaxEnd;
};

class SweepAndPruneProvider final : public SpatialOverlapProvider
{
  public:
	std::unique_ptr<const SpatialOverlapIndex> Build(std::span<const SpatialEntry> entries) const override { return std::make_unique<SweepAndPruneIndex>(entries); }
};
} // namespace

CapabilityRegistration SweepAndPrunePlugin()
{
	return SpatialOverlapSlot().Factory("hibiki.sweep_and_prune", MakeSweepAndPruneProvider);
}

std::unique_ptr<SpatialOverlapProvider> MakeSweepAndPruneProvider()
{
	return std::make_unique<SweepAndPruneProvider>();
}
} // namespace HibikiPhysics
