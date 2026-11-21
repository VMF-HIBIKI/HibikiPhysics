#pragma once

#include <HibikiPhysics/SpatialIndex.h>

namespace HibikiPhysics
{
// A move supplies the complete replacement bounds. Each ID may occur once in a batch.
struct SpatialUpdateBatch
{
	std::span<const SpatialEntry> mInserted{};
	std::span<const SpatialEntry> mMoved{};
	std::span<const std::size_t>  mRemoved{};
};

// Work performed by one successful update. A BVH refit counts each recomputed branch once.
struct SpatialUpdateStats
{
	std::size_t mEndpointSwaps = 0;
	std::size_t mRefitNodes = 0;
	std::size_t mTopologyRebuilds = 0;
	std::size_t mEndpointsChanged = 0;
};

class IncrementalSpatialIndex : public SpatialOverlapIndex
{
  public:
	// Readers may keep querying this snapshot while another thread constructs its successor.
	// On failure this snapshot and outStats retain their original values.
	[[nodiscard]] virtual std::unique_ptr<const IncrementalSpatialIndex> Updated(const SpatialUpdateBatch &inBatch, SpatialUpdateStats &outStats) const = 0;
};

class IncrementalSpatialProvider
{
  public:
	virtual ~IncrementalSpatialProvider() = default;
	[[nodiscard]] virtual std::unique_ptr<const IncrementalSpatialIndex> Build(std::span<const SpatialEntry> inEntries) const = 0;
};

[[nodiscard]] const CapabilitySlot<IncrementalSpatialProvider> &IncrementalSpatialSlot();
[[nodiscard]] CapabilityRegistration							SweepAndPruneUpdatePlugin();
[[nodiscard]] CapabilityRegistration							FourWayUpdatePlugin();
[[nodiscard]] std::unique_ptr<IncrementalSpatialProvider>		MakeSweepAndPruneUpdateProvider();
[[nodiscard]] std::unique_ptr<IncrementalSpatialProvider>		MakeFourWayUpdateProvider();
} // namespace HibikiPhysics
