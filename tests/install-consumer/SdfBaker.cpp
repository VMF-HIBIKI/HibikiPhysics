#include <HibikiPhysics/SdfBaker.h>
#include <HibikiPhysics/SpatialIndex.h>
#include "../fixtures/ThroughHolePlate.h"
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <thread>
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
enum class FailOn
{
	None,
	Caller,
	BackgroundWorker
};

class FailingIndex final : public SpatialQueryIndex
{
  public:
	FailingIndex(std::unique_ptr<const SpatialQueryIndex> index, FailOn target) : mIndex(std::move(index)), mTarget(target) {}
	std::vector<std::size_t>						 Overlaps(const AABB &bounds) const override { return mIndex->Overlaps(bounds); }
	std::vector<std::pair<std::size_t, std::size_t>> OverlapPairs() const override { return mIndex->OverlapPairs(); }
	std::vector<std::size_t>						 RayCandidates(std::array<double, 3> origin, std::array<double, 3> direction) const override { return mIndex->RayCandidates(origin, direction); }
	std::optional<SpatialNearest>					 Nearest(std::array<double, 3> point, const std::function<double(std::size_t)> &distance) const override
	{
		const auto thread = std::this_thread::get_id() == mCaller ? FailOn::Caller : FailOn::BackgroundWorker;
		if (thread == mTarget)
			throw std::runtime_error("injected nearest-query failure");
		return mIndex->Nearest(point, distance);
	}

  private:
	std::unique_ptr<const SpatialQueryIndex> mIndex;
	FailOn									 mTarget;
	std::thread::id							 mCaller = std::this_thread::get_id();
};

class CountingProvider final : public SpatialQueryProvider
{
  public:
	mutable std::size_t						 mBuilds = 0;
	FailOn									 mFailOn = FailOn::None;
	std::unique_ptr<const SpatialQueryIndex> Build(std::span<const SpatialEntry> entries) const override
	{
		++mBuilds;
		auto index = MakeCpuSpatialQueryProvider()->Build(entries);
		if (mFailOn == FailOn::None)
			return index;
		return std::make_unique<FailingIndex>(std::move(index), mFailOn);
	}
};

void CheckIdentical(const DenseSdfGrid &expected, const DenseSdfGrid &actual)
{
	Check(expected.GetNodeCounts() == actual.GetNodeCounts(), "identical node counts");
	const auto bits = [](double value)
	{ return std::bit_cast<std::uint64_t>(value); };
	const auto a = expected.GetOrigin(), b = actual.GetOrigin();
	Check(bits(a.mX) == bits(b.mX) && bits(a.mY) == bits(b.mY) && bits(a.mZ) == bits(b.mZ), "bitwise identical origin");
	Check(bits(expected.GetSpacing()) == bits(actual.GetSpacing()), "bitwise identical spacing");
	for (std::size_t i = 0; i < expected.GetValues().size(); ++i)
		Check(bits(expected.GetValues()[i]) == bits(actual.GetValues()[i]), "bitwise identical node sample");
}

void CheckParallelBakes(const TriangleMesh &mesh)
{
	SdfBakeSettings settings{.005, {.005, .005, .005}, {.25, .5, 0}};
	Check(settings.mWorkerCount == 1, "single worker by default");
	const auto reference = BakeDenseSdf(mesh, settings);
	CheckIdentical(reference, BakeDenseSdf(mesh, settings, *MakeMedianSpatialQueryProvider()));
	Check(reference.GetNodeCounts()[2] == 5, "uneven slice count for two and three workers");
	CountingProvider				 provider;
	const std::array<std::size_t, 5> counts{0, 1, 2, 3, std::numeric_limits<std::size_t>::max()};
	for (const auto count : counts)
	{
		settings.mWorkerCount = count;
		for (int repeat = 0; repeat < 2; ++repeat)
		{
			const auto builds = provider.mBuilds;
			CheckIdentical(reference, BakeDenseSdf(mesh, settings, provider));
			Check(provider.mBuilds == builds + 1, "each bake shares one spatial index");
		}
	}

	settings.mWorkerCount = 3;
	for (const auto target : {FailOn::BackgroundWorker, FailOn::Caller})
	{
		provider.mFailOn = target;
		std::optional<DenseSdfGrid> result;
		Reject<std::runtime_error>([&]
								   { result.emplace(BakeDenseSdf(mesh, settings, provider)); }, "injected nearest-query failure");
		Check(!result.has_value(), "failure publishes no grid");
		provider.mFailOn = FailOn::None;
		CheckIdentical(reference, BakeDenseSdf(mesh, settings, provider));
	}
}
} // namespace
int main()
{
	const auto mesh = Test::MakeThroughHolePlate();
	CheckParallelBakes(mesh);
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
