#pragma once

#include <HibikiPhysics/Math.h>
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace HibikiPhysics
{
// Mesh-local, node-centered distance samples in metres. X varies fastest, then Y, then Z.
class DenseSdfGrid
{
  public:
	DenseSdfGrid(Vec3 inOrigin, double inSpacing, std::array<std::size_t, 3> inNodeCounts, std::vector<double> inValues);
	DenseSdfGrid(const DenseSdfGrid &) noexcept = default;
	DenseSdfGrid &operator=(const DenseSdfGrid &) noexcept = default;

	[[nodiscard]] Vec3						 GetOrigin() const noexcept { return mOrigin; }
	[[nodiscard]] double					 GetSpacing() const noexcept { return mSpacing; }
	[[nodiscard]] std::array<std::size_t, 3> GetNodeCounts() const noexcept { return mNodeCounts; }
	[[nodiscard]] const std::vector<double> &GetValues() const noexcept { return *mValues; }

	[[nodiscard]] double GetNode(std::size_t inX, std::size_t inY, std::size_t inZ) const;
	// The outer node planes are included. Finite points outside the grid return an empty optional.
	[[nodiscard]] std::optional<double> Sample(Vec3 inPoint) const;

  private:
	Vec3									   mOrigin;
	double									   mSpacing;
	std::array<std::size_t, 3>				   mNodeCounts;
	Vec3									   mLastNode;
	std::shared_ptr<const std::vector<double>> mValues;
};
} // namespace HibikiPhysics
