#pragma once

#include <HibikiPhysics/SpatialIndex.h>

namespace HibikiPhysics
{
struct PairBatchSettings
{
	std::size_t mWorkers = 0;
	std::size_t mInitialCapacity = 256;
};
struct PairBatchStats
{
	std::size_t mWorkers = 0;
	std::size_t mPasses = 0;
	std::size_t mCapacity = 0;
};

// Entries are the same unique IDs and closed finite boxes used to build the index.
// A completed call publishes statistics; scratch and output belong to this query.
[[nodiscard]] std::vector<std::pair<std::size_t, std::size_t>> CollectOverlapPairs(
	const SpatialOverlapIndex &inIndex, std::span<const SpatialEntry> inEntries, PairBatchSettings inSettings, PairBatchStats &outStats);
[[nodiscard]] std::unique_ptr<SpatialOverlapProvider> MakeBatchedMortonOverlapProvider(PairBatchSettings inSettings = {});
[[nodiscard]] CapabilityRegistration				  BatchedMortonOverlapPlugin(PairBatchSettings inSettings = {});
} // namespace HibikiPhysics
