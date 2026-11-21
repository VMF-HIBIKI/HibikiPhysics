#include <HibikiPhysics/SdfBaker.h>
#include <HibikiPhysics/ClosedMeshSdf.h>
#include <HibikiPhysics/Geometry.h>
#include <HibikiPhysics/SpatialIndex.h>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <exception>
#include <new>
#include <stdexcept>
#include <string>
#include <thread>

namespace HibikiPhysics
{
namespace
{
struct GridLayout
{
	std::array<double, 3>	   origin{};
	std::array<std::size_t, 3> counts{};
	std::size_t				   total = 1;
};

GridLayout MakeLayout(const AABB &bounds, const SdfBakeSettings &settings)
{
	const double h = settings.mSpacing;
	if (!std::isfinite(h) || h <= 0)
		throw std::invalid_argument("Expected positive finite SDF spacing in metres");
	const std::array padding{settings.mPadding.mX, settings.mPadding.mY, settings.mPadding.mZ};
	const std::array phase{settings.mPhase.mX, settings.mPhase.mY, settings.mPhase.mZ};
	const auto		 low = bounds.GetMin(), high = bounds.GetMax();
	const auto		 capacity = std::vector<double>{}.max_size();
	GridLayout		 grid;
	for (std::size_t a = 0; a < 3; ++a)
	{
		if (!std::isfinite(padding[a]) || padding[a] < 0)
			throw std::invalid_argument("Expected nonnegative finite SDF padding in metres");
		if (!std::isfinite(phase[a]) || phase[a] < 0 || phase[a] >= 1)
			throw std::invalid_argument("Expected SDF phase in [0,1)");
		const double first = (low[a] - padding[a]) - phase[a] * h, last = high[a] + padding[a];
		const double intervals = (last - first) / h;
		if (!std::isfinite(intervals) || intervals >= static_cast<double>(capacity))
			throw std::overflow_error("SDF grid extent exceeds addressable storage");
		const auto coordinate = [&](std::size_t i)
		{ return first + static_cast<double>(i) * h; };
		std::size_t cells = std::max<std::size_t>(1, static_cast<std::size_t>(intervals));
		// Check the actual node formula: rounding a quotient near an integer may shift the endpoint.
		while (cells > 1 && coordinate(cells - 1) >= last)
			--cells;
		while (coordinate(cells) < last)
		{
			if (cells >= capacity - 1)
				throw std::overflow_error("SDF grid axis exceeds addressable storage");
			++cells;
		}
		if (!std::isfinite(coordinate(cells)) || !std::isfinite(coordinate(cells) - first))
			throw std::overflow_error("SDF node coordinates exceed finite range");
		if (coordinate(1) <= first || coordinate(cells) <= coordinate(cells - 1))
			throw std::invalid_argument("SDF spacing must resolve distinct local coordinates");
		const auto nodes = cells + 1;
		if (nodes > capacity / grid.total)
			throw std::overflow_error("SDF sample count exceeds addressable storage");
		grid.origin[a] = first;
		grid.counts[a] = nodes;
		grid.total *= nodes;
	}
	return grid;
}

void FillSlice(const ClosedMeshSdf &surface, const GridLayout &grid, double h, std::size_t z, std::vector<double> &values)
{
	const auto	 nx = grid.counts[0], ny = grid.counts[1];
	const double pz = grid.origin[2] + static_cast<double>(z) * h;
	for (std::size_t y = 0; y < ny; ++y)
	{
		const double py = grid.origin[1] + static_cast<double>(y) * h;
		for (std::size_t x = 0; x < nx; ++x)
		{
			const double px = grid.origin[0] + static_cast<double>(x) * h;
			values[(z * ny + y) * nx + x] = surface.SignedDistance({px, py, pz});
		}
	}
}

void FillGrid(const ClosedMeshSdf &surface, const GridLayout &grid, const SdfBakeSettings &settings, std::vector<double> &values)
{
	const std::size_t nz = grid.counts[2];
	const std::size_t requested = settings.mWorkerCount == 0 ? std::max(1u, std::thread::hardware_concurrency()) : settings.mWorkerCount;
	const std::size_t workers = std::min(nz, requested);
	if (workers == 1)
	{
		for (std::size_t z = 0; z < nz; ++z)
			FillSlice(surface, grid, settings.mSpacing, z, values);
		return;
	}

	std::atomic<bool>  cancelled{false};
	std::exception_ptr failure;
	auto			   recordFailure = [&]
	{
		// Only the first failing thread writes the exception. The caller reads it after joining.
		if (!cancelled.exchange(true, std::memory_order_relaxed))
			failure = std::current_exception();
	};
	auto fill = [&](std::size_t firstSlice)
	{
		try
		{
			for (std::size_t z = firstSlice; z < nz && !cancelled.load(std::memory_order_relaxed); z += workers)
				FillSlice(surface, grid, settings.mSpacing, z, values);
		}
		catch (...)
		{
			recordFailure();
		}
	};

	std::vector<std::thread> threads;
	threads.reserve(workers - 1);
	try
	{
		for (std::size_t i = 1; i < workers && !cancelled.load(std::memory_order_relaxed); ++i)
			threads.emplace_back(fill, i);
		fill(0);
	}
	catch (...)
	{
		// Thread creation may fail after some workers have started.
		recordFailure();
	}
	for (auto &thread : threads)
		thread.join();
	if (failure)
		std::rethrow_exception(failure);
}
} // namespace

DenseSdfGrid BakeDenseSdf(const TriangleMesh &inMesh, const SdfBakeSettings &inSettings)
{
	return BakeDenseSdf(inMesh, inSettings, *MakeCpuSpatialQueryProvider());
}

DenseSdfGrid BakeDenseSdf(const TriangleMesh &inMesh, const SdfBakeSettings &inSettings, const SpatialQueryProvider &inProvider)
{
	const GridLayout	grid = MakeLayout(GeometryLibrary().LocalBounds(inMesh), inSettings);
	const ClosedMeshSdf surface(inMesh, inProvider);
	std::vector<double> values;
	try
	{
		values.resize(grid.total);
	}
	catch (const std::bad_alloc &)
	{
		throw std::length_error("Could not allocate dense SDF sample buffer of " + std::to_string(grid.total) + " doubles");
	}
	FillGrid(surface, grid, inSettings, values);
	const auto [x, y, z] = grid.origin;
	return {{x, y, z}, inSettings.mSpacing, grid.counts, std::move(values)};
}
} // namespace HibikiPhysics
