#pragma once

#include <HibikiPhysics/IncrementalSpatialIndex.h>

#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace HibikiPhysics::Detail
{
template <class Index>
class SpatialUpdateProvider final : public IncrementalSpatialProvider
{
  public:
	std::unique_ptr<const IncrementalSpatialIndex> Build(std::span<const SpatialEntry> entries) const override
	{
		return std::make_unique<Index>(entries);
	}
};

template <class Contains>
void ValidateUpdate(const SpatialUpdateBatch &batch, std::size_t currentSize, std::size_t maxSize, Contains contains)
{
	if (batch.mRemoved.size() > currentSize)
		throw std::invalid_argument("More removals than spatial entries");
	if (batch.mInserted.size() > maxSize - (currentSize - batch.mRemoved.size()))
		throw std::length_error("Too many spatial entries");
	std::unordered_set<std::size_t> seen;
	auto							checkId = [&](std::size_t id, bool mustExist)
	{
		if (!seen.insert(id).second)
			throw std::invalid_argument("Spatial ID occurs multiple times in one update");
		if (contains(id) != mustExist)
			throw std::invalid_argument("Spatial update ID has incorrect membership");
	};
	auto checkEntry = [&](const SpatialEntry &entry, bool mustExist)
	{
		checkId(entry.mId, mustExist);
		const auto lo = entry.mBounds.GetMin(), hi = entry.mBounds.GetMax();
		for (std::size_t axis = 0; axis < lo.size(); ++axis)
			if (!std::isfinite(lo[axis]) || !std::isfinite(hi[axis]))
				throw std::invalid_argument("Spatial update requires finite bounds");
	};
	for (const auto &entry : batch.mInserted)
		checkEntry(entry, false);
	for (const auto &entry : batch.mMoved)
		checkEntry(entry, true);
	for (auto id : batch.mRemoved)
		checkId(id, true);
}
} // namespace HibikiPhysics::Detail
