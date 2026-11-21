#include <HibikiPhysics/SweepAndPrune.h>
#include "SpatialUpdate.h"

#include <algorithm>
#include <cmath>
#include <compare>
#include <limits>
#include <stdexcept>
#include <unordered_map>

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
// The entry slot breaks ties between endpoints of the same kind.
struct Endpoint
{
	double		 mCoordinate;
	EndpointKind mKind;
	std::size_t	 mEntry;
	auto		 operator<=>(const Endpoint &) const = default;
};

class SweepAndPruneIndex : public SpatialOverlapIndex
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

		mPrefixMaxEnd.resize(mEndpoints.size());
		RefreshPrefix(0, mEndpoints.size());
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

  protected:
	// Earlier intervals contribute their greatest end to each prefix. After the last
	// changed endpoint the old suffix is valid once its accumulated maximum matches.
	void RefreshPrefix(std::size_t first, std::size_t last)
	{
		double maxEnd = first == 0 ? -std::numeric_limits<double>::infinity() : mPrefixMaxEnd[first - 1];
		for (std::size_t i = first; i < mEndpoints.size(); ++i)
		{
			const auto &endpoint = mEndpoints[i];
			if (endpoint.mKind == EndpointKind::Start)
				maxEnd = std::max(maxEnd, mEntries[endpoint.mEntry].mBounds.GetMax()[0]);
			if (i >= last && maxEnd == mPrefixMaxEnd[i])
				break;
			mPrefixMaxEnd[i] = maxEnd;
		}
	}

	std::vector<SpatialEntry> mEntries;
	std::vector<Endpoint>	  mEndpoints;
	std::vector<double>		  mPrefixMaxEnd;
};

class SweepFrame final : public SweepAndPruneIndex
{
  public:
	explicit SweepFrame(std::span<const SpatialEntry> entries) : SweepAndPruneIndex(entries) { IndexSlots(); }

	void Validate(const SpatialUpdateBatch &batch) const
	{
		Detail::ValidateUpdate(batch, mEntries.size(), std::min(mEntries.max_size(), mEndpoints.max_size() / 2),
							   [&](std::size_t id)
							   { return mSlotById->contains(id); });
	}

	void Apply(const SpatialUpdateBatch &batch, SpatialUpdateStats &stats)
	{
		if (!batch.mRemoved.empty() || !batch.mInserted.empty())
			ChangeMembership(batch);
		std::size_t first = mEndpoints.size(), last = 0;
		for (const auto &entry : batch.mMoved)
		{
			const auto slot = mSlotById->at(entry.mId);
			mEntries[slot].mBounds = entry.mBounds;
			auto repair = [&](EndpointKind kind, double coordinate)
			{
				const auto old = mPositions[slot][static_cast<std::size_t>(kind)];
				if (mEndpoints[old].mCoordinate == coordinate)
					return;
				// A new maximum X also changes the prefix contribution at this entry's start.
				first = std::min({first, old, mPositions[slot][0]});
				const auto position = RepairEndpoint(old, coordinate, stats);
				first = std::min(first, position);
				last = std::max({last, old, position});
			};
			repair(EndpointKind::Start, entry.mBounds.GetMin()[0]);
			repair(EndpointKind::End, entry.mBounds.GetMax()[0]);
		}
		if (first < mEndpoints.size())
			RefreshPrefix(first, last);
	}

  private:
	void IndexSlots()
	{
		auto slots = std::make_shared<std::unordered_map<std::size_t, std::size_t>>();
		slots->reserve(mEntries.size());
		for (std::size_t i = 0; i < mEntries.size(); ++i)
			slots->emplace(mEntries[i].mId, i);
		mSlotById = std::move(slots);
		mPositions.resize(mEntries.size());
		for (std::size_t i = 0; i < mEndpoints.size(); ++i)
			Position(mEndpoints[i]) = i;
	}

	std::size_t &Position(const Endpoint &endpoint)
	{
		return mPositions[endpoint.mEntry][static_cast<std::size_t>(endpoint.mKind)];
	}

	std::size_t RepairEndpoint(std::size_t position, double coordinate, SpatialUpdateStats &stats)
	{
		auto endpoint = mEndpoints[position];
		endpoint.mCoordinate = coordinate;
		++stats.mEndpointsChanged;
		const auto old = position;
		while (position > 0 && endpoint < mEndpoints[position - 1])
		{
			mEndpoints[position] = mEndpoints[position - 1];
			Position(mEndpoints[position]) = position;
			--position;
		}
		while (position + 1 < mEndpoints.size() && mEndpoints[position + 1] < endpoint)
		{
			mEndpoints[position] = mEndpoints[position + 1];
			Position(mEndpoints[position]) = position;
			++position;
		}
		mEndpoints[position] = endpoint;
		Position(endpoint) = position;
		stats.mEndpointSwaps += std::max(old, position) - std::min(old, position);
		return position;
	}

	void ChangeMembership(const SpatialUpdateBatch &batch)
	{
		// Compacting the retained slots monotonically preserves every old endpoint tie.
		const auto				 removed = mEntries.size();
		std::vector<std::size_t> remap(mEntries.size());
		for (auto id : batch.mRemoved)
			remap[mSlotById->at(id)] = removed;
		std::size_t kept = 0;
		for (std::size_t i = 0; i < mEntries.size(); ++i)
			if (remap[i] != removed)
			{
				remap[i] = kept;
				mEntries[kept++] = mEntries[i];
			}
		mEntries.erase(mEntries.begin() + static_cast<std::ptrdiff_t>(kept), mEntries.end());
		std::size_t end = 0;
		for (auto endpoint : mEndpoints)
			if (remap[endpoint.mEntry] != removed)
			{
				endpoint.mEntry = remap[endpoint.mEntry];
				mEndpoints[end++] = endpoint;
			}
		mEndpoints.resize(end);
		for (const auto &entry : batch.mInserted)
		{
			const auto slot = mEntries.size();
			mEntries.push_back(entry);
			mEndpoints.push_back({entry.mBounds.GetMin()[0], EndpointKind::Start, slot});
			mEndpoints.push_back({entry.mBounds.GetMax()[0], EndpointKind::End, slot});
		}
		auto additions = mEndpoints.begin() + static_cast<std::ptrdiff_t>(end);
		std::sort(additions, mEndpoints.end());
		std::inplace_merge(mEndpoints.begin(), additions, mEndpoints.end());
		IndexSlots();
		mPrefixMaxEnd.resize(mEndpoints.size());
		RefreshPrefix(0, mEndpoints.size());
	}

	std::shared_ptr<const std::unordered_map<std::size_t, std::size_t>> mSlotById;
	std::vector<std::array<std::size_t, 2>>								mPositions;
};

class SweepUpdateIndex final : public IncrementalSpatialIndex
{
  public:
	explicit SweepUpdateIndex(std::span<const SpatialEntry> entries) : mFrame(std::make_shared<SweepFrame>(entries)) {}

	std::vector<std::size_t> Overlaps(const AABB &bounds) const override { return mFrame->Overlaps(bounds); }
	Pairs					 OverlapPairs() const override { return mFrame->OverlapPairs(); }

	std::unique_ptr<const IncrementalSpatialIndex> Updated(const SpatialUpdateBatch &batch, SpatialUpdateStats &outStats) const override
	{
		mFrame->Validate(batch);
		auto			   next = std::make_unique<SweepUpdateIndex>(*this);
		SpatialUpdateStats stats;
		if (!batch.mInserted.empty() || !batch.mMoved.empty() || !batch.mRemoved.empty())
		{
			auto frame = std::make_shared<SweepFrame>(*mFrame);
			frame->Apply(batch, stats);
			next->mFrame = std::move(frame);
		}
		outStats = stats;
		return next;
	}

  private:
	std::shared_ptr<const SweepFrame> mFrame;
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

std::unique_ptr<IncrementalSpatialProvider> MakeSweepAndPruneUpdateProvider()
{
	return std::make_unique<Detail::SpatialUpdateProvider<SweepUpdateIndex>>();
}
} // namespace HibikiPhysics
