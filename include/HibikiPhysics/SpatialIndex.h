#pragma once

#include <HibikiPhysics/AnalyticShapes.h>
#include <HibikiPhysics/Plugin.h>

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace HibikiPhysics
{
struct SpatialEntry
{
	std::size_t mId;
	AABB		mBounds;
};

struct SpatialNearest
{
	std::size_t mId;
	double		mDistance;
};

// Entries use unique caller IDs and finite closed bounds in one coordinate frame.
// An index outlives its input span and supports concurrent read-only queries.
class SpatialOverlapIndex
{
  public:
	virtual ~SpatialOverlapIndex() = default;
	[[nodiscard]] virtual std::vector<std::size_t> Overlaps(const AABB &inBounds) const = 0;
	// Each unordered pair of distinct IDs occurs once, smaller ID first.
	[[nodiscard]] virtual std::vector<std::pair<std::size_t, std::size_t>> OverlapPairs() const = 0;
};

class SpatialQueryIndex : public SpatialOverlapIndex
{
  public:
	// The ray covers origin + t * direction for t >= 0. A zero direction gives a point query.
	[[nodiscard]] virtual std::vector<std::size_t> RayCandidates(std::array<double, 3> inOrigin, std::array<double, 3> inDirection) const = 0;
	// The callback returns Euclidean distance to the primitive, or +infinity to skip it.
	// Equal distances select the smallest ID. An empty result means no finite distance.
	[[nodiscard]] virtual std::optional<SpatialNearest> Nearest(std::array<double, 3> inPoint, const std::function<double(std::size_t)> &inExactDistance) const = 0;
};

class SpatialOverlapProvider
{
  public:
	virtual ~SpatialOverlapProvider() = default;
	[[nodiscard]] virtual std::unique_ptr<const SpatialOverlapIndex> Build(std::span<const SpatialEntry> inEntries) const = 0;
};

class SpatialQueryProvider
{
  public:
	virtual ~SpatialQueryProvider() = default;
	[[nodiscard]] virtual std::unique_ptr<const SpatialQueryIndex> Build(std::span<const SpatialEntry> inEntries) const = 0;
};

[[nodiscard]] const CapabilitySlot<SpatialOverlapProvider> &SpatialOverlapSlot();
[[nodiscard]] const CapabilitySlot<SpatialQueryProvider>   &SpatialQuerySlot();
// CPU defaults use stackless Morton for overlaps and full Morton for ray/nearest queries.
[[nodiscard]] CapabilityRegistration				CpuSpatialOverlapPlugin();
[[nodiscard]] CapabilityRegistration				CpuSpatialQueryPlugin();
[[nodiscard]] std::unique_ptr<SpatialQueryProvider> MakeCpuSpatialQueryProvider();

// Explicit median-split BVH reference providers for either capability.
[[nodiscard]] CapabilityRegistration				MedianSpatialOverlapPlugin();
[[nodiscard]] CapabilityRegistration				MedianSpatialQueryPlugin();
[[nodiscard]] std::unique_ptr<SpatialQueryProvider> MakeMedianSpatialQueryProvider();
} // namespace HibikiPhysics
