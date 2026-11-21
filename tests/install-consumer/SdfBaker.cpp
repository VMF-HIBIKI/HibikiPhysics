#include <HibikiPhysics/SdfBaker.h>
#include <HibikiPhysics/SpatialIndex.h>
#include "../fixtures/ThroughHolePlate.h"
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
using namespace HibikiPhysics;
namespace
{
void Check(bool ok, const char *msg)
{
	if (!ok)
		throw std::runtime_error(msg);
}
template <class Error>
void Reject(auto fn, const char *text)
{
	try
	{
		fn();
	}
	catch (const Error &e)
	{
		Check(std::string(e.what()).find(text) != std::string::npos, "expected actionable diagnostic");
		return;
	}
	throw std::logic_error("Expected rejection");
}
class CountingProvider final : public SpatialQueryProvider
{
  public:
	mutable std::size_t						 mBuilds = 0;
	std::unique_ptr<const SpatialQueryIndex> Build(std::span<const SpatialEntry> entries) const override
	{
		++mBuilds;
		return MakeCpuSpatialQueryProvider()->Build(entries);
	}
};
} // namespace
int main()
{
	const auto			  mesh = Test::MakeThroughHolePlate();
	const SdfBakeSettings settings{.001, {.005, .005, .005}, {}};
	CountingProvider	  provider;
	const auto			  grid = BakeDenseSdf(mesh, settings, provider);
	Check(provider.mBuilds == 1, "one spatial hierarchy for validation and all grid nodes");
	Check(grid.GetNodeCounts() == std::array<std::size_t, 3>{51, 51, 21}, "inclusive padded grid dimensions");
	Check(grid.GetOrigin().mX == -.025 && grid.GetOrigin().mZ == -.010, "padded local origin");
	Check(std::abs(grid.Sample({}).value() - .005) < 1e-14, "hole centre in air");
	Check(std::abs(grid.Sample({.007, 0, 0}).value() + .002) < 1e-14, "inside material");
	Check(std::abs(grid.Sample({.005, 0, 0}).value()) < 1e-14, "hole wall");
	const auto identical = BakeDenseSdf(mesh, settings);
	Check(grid.GetValues() == identical.GetValues(), "reproducible serial result");
	auto shifted = settings;
	shifted.mPhase = {.5, .5, 0};
	const auto half = BakeDenseSdf(mesh, shifted);
	Check(std::abs(half.Sample({}).value() - .0045) < 1e-14, "shifted hole interpolation");
	const TriangleMesh open({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}, {{0, 1, 2}});
	Reject<std::invalid_argument>([&]
								  { (void)BakeDenseSdf(open, settings); }, "edge");
	Reject<std::invalid_argument>([&]
								  { (void)BakeDenseSdf(mesh, {-1, {}, {}}); }, "spacing");
	Reject<std::invalid_argument>([&]
								  { (void)BakeDenseSdf(mesh, {.001, {-1, 0, 0}, {}}); }, "padding");
	Reject<std::invalid_argument>([&]
								  { (void)BakeDenseSdf(mesh, {.001, {}, {0, 1, 0}}); }, "phase");
	Reject<std::overflow_error>([&]
								{ (void)BakeDenseSdf(mesh, {1e-20, {}, {}}); }, "storage");
}
