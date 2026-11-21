#pragma once

#include <HibikiPhysics/SpatialIndex.h>

namespace HibikiPhysics
{
namespace Detail
{
struct FourWayTree;
}

// Immutable BVH4 over unique stable IDs and finite, closed 3D bounds.
class FourWaySpatialIndex final : public SpatialOverlapIndex
{
  public:
	explicit FourWaySpatialIndex(std::span<const SpatialEntry> inEntries);
	~FourWaySpatialIndex() override;

	[[nodiscard]] std::vector<std::size_t>						   Overlaps(const AABB &inBounds) const override;
	[[nodiscard]] std::vector<std::pair<std::size_t, std::size_t>> OverlapPairs() const override;

	// Report the number of node bounds tested against the box, or against another
	// node's bounds. The count belongs to this call and includes rejected tests.
	[[nodiscard]] std::vector<std::size_t>						   Overlaps(const AABB &inBounds, std::size_t &outNodeVisits) const;
	[[nodiscard]] std::vector<std::pair<std::size_t, std::size_t>> OverlapPairs(std::size_t &outNodeVisits) const;

  private:
	std::unique_ptr<const Detail::FourWayTree> mImpl;
};

[[nodiscard]] std::unique_ptr<SpatialOverlapProvider> MakeFourWaySpatialOverlapProvider();
[[nodiscard]] CapabilityRegistration				  FourWaySpatialOverlapPlugin();
} // namespace HibikiPhysics
