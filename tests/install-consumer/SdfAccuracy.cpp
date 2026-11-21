#include <HibikiPhysics/SdfBaker.h>
#include "../fixtures/ExperimentMetrics.h"
#include "../fixtures/ThroughHolePlate.h"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>

using namespace HibikiPhysics;
namespace
{
constexpr double mm = 0.001;
constexpr double holeHalfWidth = 5 * mm;
constexpr double sphereRadius = 4.8 * mm;

void Require(bool condition, const char *message)
{
	if (!condition)
		throw std::runtime_error(message);
}

double SquareDistance(Vec3 p, double halfWidth)
{
	const double x = std::abs(p.mX) - halfWidth, y = std::abs(p.mY) - halfWidth;
	return std::hypot(std::max(x, 0.0), std::max(y, 0.0)) + std::min(std::max(x, y), 0.0);
}

// The planar square annulus is extruded by [-5,5] mm. Outside the extrusion,
// independent planar and vertical distances combine by Pythagoras.
double Reference(Vec3 p)
{
	const double planar = std::max(SquareDistance(p, 20 * mm), -SquareDistance(p, holeHalfWidth));
	const double vertical = std::abs(p.mZ) - 5 * mm;
	return std::hypot(std::max(planar, 0.0), std::max(vertical, 0.0)) + std::min(std::max(planar, vertical), 0.0);
}

void CheckReference()
{
	auto check = [](Vec3 pointMm, double expectedMm)
	{ Require(std::abs(Reference(pointMm * mm) / mm - expectedMm) < 1e-12, "analytic plate distance"); };
	check({}, 5);
	check({5, -3, 2}, 0);
	check({0, 0, 8}, std::hypot(5.0, 3.0));
	check({6, -6, 0}, -std::sqrt(2.0));
	check({12, 0, 3}, -2);
	check({12, 0, 5}, 0);
	check({21, 21, -6}, std::sqrt(3.0));
}

struct Errors
{
	std::size_t count = 0;
	double		maximum = 0, sum = 0;

	void Add(double error)
	{
		++count;
		maximum = std::max(maximum, std::abs(error));
		sum += std::abs(error);
	}
	double Mean() const { return sum / static_cast<double>(count); }
};

Errors NodeErrors(const DenseSdfGrid &grid)
{
	Errors		 errors;
	const Vec3	 first = grid.GetOrigin();
	const double h = grid.GetSpacing();
	const auto [nx, ny, nz] = grid.GetNodeCounts();
	for (std::size_t z = 0; z < nz; ++z)
		for (std::size_t y = 0; y < ny; ++y)
			for (std::size_t x = 0; x < nx; ++x)
			{
				const Vec3 p = first + Vec3{static_cast<double>(x), static_cast<double>(y), static_cast<double>(z)} * h;
				// The small margin admits boundary nodes under binary rounding.
				constexpr double margin = 1e-10 * mm;
				if (std::abs(p.mX) <= 21 * mm + margin && std::abs(p.mY) <= 21 * mm + margin && std::abs(p.mZ) <= 10 * mm + margin)
					errors.Add(grid.GetNode(x, y, z) - Reference(p));
			}
	return errors;
}

double RadicalInverse(unsigned index, unsigned base)
{
	double value = 0, factor = 1;
	while (index != 0)
	{
		factor /= base;
		value += (index % base) * factor;
		index /= base;
	}
	return value;
}

Errors ProbeErrors(const DenseSdfGrid &grid)
{
	Errors errors;
	auto   probe = [&](Vec3 pointMm)
	{
		const Vec3 p = pointMm * mm;
		errors.Add(grid.Sample(p).value() - Reference(p));
	};
	for (unsigned i = 1; i <= 2048; ++i)
	{
		const double u = RadicalInverse(i, 2), v = RadicalInverse(i, 3), w = RadicalInverse(i, 5);
		probe({42 * u - 21, 42 * v - 21, 20 * w - 10});
		probe({2 * u + 4, 2 * v + 4, 2 * w + 4});
	}
	return errors;
}

Errors WallDisplacements(const DenseSdfGrid &grid)
{
	Errors displacements;
	for (bool xNormal : {false, true})
		for (int side : {-1, 1})
			for (double tangent : {-4.875, -2.375, 0.0, 2.375, 4.875})
				for (double height : {-4.875, 0.0, 4.875})
				{
					auto sample = [&](double radius)
					{
						const Vec3 p = xNormal ? Vec3{side * radius, tangent * mm, height * mm} : Vec3{tangent * mm, side * radius, height * mm};
						return grid.Sample(p).value();
					};
					double		 lo = 0, hi = 10 * mm;
					const double atLo = sample(lo), atHi = sample(hi);
					if (!(atLo > 0 && atHi < 0))
					{
						std::cerr << "No hole-wall crossing on " << (xNormal ? 'X' : 'Y') << " wall, sign " << side
								  << ", tangent " << tangent << " mm, height " << height << " mm. Endpoint distances: "
								  << atLo / mm << ", " << atHi / mm << " mm\n";
						throw std::runtime_error("hole wall crossing must be bracketed");
					}
					for (int k = 0; k < 40; ++k)
					{
						const double r = std::midpoint(lo, hi);
						if (sample(r) > 0)
							lo = r;
						else
							hi = r;
					}
					displacements.Add(std::midpoint(lo, hi) - holeHalfWidth);
				}
	return displacements;
}

void Run(double spacingMm, double phase, std::size_t workers)
{
	CheckReference();
	const auto			  mesh = Test::MakeThroughHolePlate();
	const SdfBakeSettings settings{spacingMm * mm, {5 * mm, 5 * mm, 5 * mm}, {phase, phase, 0}, workers};
	const auto			  start = std::chrono::steady_clock::now();
	const auto			  grid = BakeDenseSdf(mesh, settings);
	const double		  seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
	const auto			  peak = Test::PeakResidentBytes();

	const Errors nodes = NodeErrors(grid), probes = ProbeErrors(grid), walls = WallDisplacements(grid);
	const double centre = grid.Sample({}).value();
	double		 clearance = std::numeric_limits<double>::infinity();
	for (int i = -80; i <= 80; ++i)
		clearance = std::min(clearance, grid.Sample({0, 0, i * 0.125 * mm}).value() - sphereRadius);

	constexpr double tolerance = 1e-10 * mm;
	Require(nodes.maximum < tolerance, "node differs from analytic reference");
	// Jensen's inequality and a 1-Lipschitz SDF bound the trilinear error by sqrt(3)*h/2.
	Require(probes.maximum <= std::sqrt(3.0) * settings.mSpacing / 2 + tolerance, "trilinear distance error bound");
	Require(std::abs(centre - (holeHalfWidth - phase * settings.mSpacing)) < tolerance, "hole centre phase response");
	Require(std::abs(clearance - (centre - sphereRadius)) < tolerance, "axial minimum clearance");

	const auto [nx, ny, nz] = grid.GetNodeCounts();
	std::cout << std::setprecision(17)
			  << "{\"build\":\"" HIBIKI_EXPERIMENT_BUILD "\",\"compiler\":\"" HIBIKI_EXPERIMENT_COMPILER "\""
			  << ",\"spacing_mm\":" << spacingMm << ",\"phase_xy_cells\":" << phase
			  << ",\"requested_workers\":" << workers << ",\"grid_fingerprint\":\"" << std::hex << Test::GridFingerprint(grid) << std::dec << "\""
			  << ",\"grid_shape\":[" << nx << ',' << ny << ',' << nz << "],\"nodes\":" << grid.GetValues().size()
			  << ",\"grid_bytes\":" << grid.GetValues().size() * sizeof(double) << ",\"peak_resident_bytes\":" << peak
			  << ",\"bake_s\":" << seconds << ",\"node_probes\":" << nodes.count
			  << ",\"node_max_mm\":" << nodes.maximum / mm << ",\"node_mean_mm\":" << nodes.Mean() / mm
			  << ",\"probes\":" << probes.count << ",\"probe_max_mm\":" << probes.maximum / mm << ",\"probe_mean_mm\":" << probes.Mean() / mm
			  << ",\"walls\":" << walls.count << ",\"wall_max_mm\":" << walls.maximum / mm << ",\"wall_mean_mm\":" << walls.Mean() / mm
			  << ",\"centre_mm\":" << centre / mm << ",\"axial_min_clearance_mm\":" << clearance / mm << "}\n";
}
} // namespace

int main(int argc, char **argv)
{
	try
	{
		Require(argc == 3 || argc == 4, "usage: hibiki_sdf_accuracy <spacing-mm: 0.25|0.5|1|2.5> <xy-phase: 0|0.5> [workers]");
		const std::string_view spacing = argv[1], phase = argv[2];
		Require(spacing == "0.25" || spacing == "0.5" || spacing == "1" || spacing == "2.5", "unsupported spacing");
		Require(phase == "0" || phase == "0.5", "unsupported phase");
		std::size_t workers = 1;
		if (argc == 4)
		{
			const std::string_view count = argv[3];
			const auto [end, error] = std::from_chars(count.data(), count.data() + count.size(), workers);
			Require(error == std::errc{} && end == count.data() + count.size(), "invalid worker count");
		}
		Run(std::stod(argv[1]), std::stod(argv[2]), workers);
		return 0;
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}
}
