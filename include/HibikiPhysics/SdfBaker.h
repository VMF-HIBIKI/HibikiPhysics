#pragma once

#include <HibikiPhysics/DenseSdfGrid.h>
#include <HibikiPhysics/DiscreteShapes.h>
#include <cstddef>

namespace HibikiPhysics
{
class SpatialQueryProvider;

struct SdfBakeSettings
{
	double mSpacing = 0;
	Vec3   mPadding{};
	// Fraction of one cell (per axis, in [0,1)) to extend the negative boundary.
	Vec3 mPhase{};
	// 1 runs on the caller, 0 selects hardware concurrency (at least 1).
	// The total number of workers, including the caller, is capped at the Z slice count.
	std::size_t mWorkerCount = 1;
};

// Bakes the signed distance to a validated local-space material boundary, in metres.
[[nodiscard]] DenseSdfGrid BakeDenseSdf(const TriangleMesh &inMesh, const SdfBakeSettings &inSettings);
[[nodiscard]] DenseSdfGrid BakeDenseSdf(const TriangleMesh &inMesh, const SdfBakeSettings &inSettings, const SpatialQueryProvider &inProvider);
} // namespace HibikiPhysics
