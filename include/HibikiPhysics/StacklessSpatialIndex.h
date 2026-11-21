#pragma once

#include <HibikiPhysics/SpatialIndex.h>

namespace HibikiPhysics
{
namespace Detail
{
class EscapeTree;
}

// Immutable Morton hierarchy with one escape link per node. The overloads
// accepting a count report the number of node bounds tested by that call.
class StacklessSpatialOverlapIndex final : public SpatialOverlapIndex
{
  public:
	explicit StacklessSpatialOverlapIndex(std::span<const SpatialEntry> inEntries);
	~StacklessSpatialOverlapIndex() override;
	[[nodiscard]] std::vector<std::size_t>						   Overlaps(const AABB &inBounds) const override;
	[[nodiscard]] std::vector<std::size_t>						   Overlaps(const AABB &inBounds, std::size_t &outVisitedNodes) const;
	[[nodiscard]] std::vector<std::pair<std::size_t, std::size_t>> OverlapPairs() const override;
	[[nodiscard]] std::vector<std::pair<std::size_t, std::size_t>> OverlapPairs(std::size_t &outVisitedNodes) const;

  private:
	std::unique_ptr<const Detail::EscapeTree> mTree;
};

[[nodiscard]] std::unique_ptr<SpatialOverlapProvider> MakeStacklessSpatialOverlapProvider();
[[nodiscard]] CapabilityRegistration				  StacklessSpatialOverlapPlugin();
} // namespace HibikiPhysics
