#pragma once
#include <HibikiPhysics/SpatialIndex.h>

namespace HibikiPhysics
{
struct SpatialMetadata
{
	std::size_t mOwner;
	std::size_t mCategory;
	bool		operator==(const SpatialMetadata &) const = default;
};
struct TaggedSpatialEntry
{
	SpatialEntry	mEntry;
	SpatialMetadata mMetadata;
};
using SpatialPairPredicate = std::function<bool(const SpatialMetadata &, const SpatialMetadata &)>;
struct SpatialFilterStats
{
	std::size_t mNodeVisits{}, mQueryReads{}, mLeafTests{}, mPrunedSubtrees{};
};

class FilteredSpatialIndex : public SpatialOverlapIndex
{
  public:
	// The source ID selects its metadata. A caller may supply a different query box.
	[[nodiscard]] virtual std::vector<std::size_t> FilteredOverlaps(std::size_t inSourceId, const AABB &inBox, const SpatialPairPredicate &inAccept, SpatialFilterStats &outStats) const = 0;
	// The predicate receives the metadata of the smaller ID first.
	[[nodiscard]] virtual std::vector<std::pair<std::size_t, std::size_t>> FilteredPairs(const SpatialPairPredicate &inAccept, SpatialFilterStats &outStats) const = 0;
};
class FilteredSpatialProvider
{
  public:
	virtual ~FilteredSpatialProvider() = default;
	[[nodiscard]] virtual std::unique_ptr<const FilteredSpatialIndex> Build(std::span<const TaggedSpatialEntry> inEntries) const = 0;
};
[[nodiscard]] const CapabilitySlot<FilteredSpatialProvider> &FilteredSpatialSlot();
[[nodiscard]] std::unique_ptr<FilteredSpatialProvider>		 MakeFilteredMortonProvider();
[[nodiscard]] CapabilityRegistration						 FilteredMortonPlugin();
} // namespace HibikiPhysics
