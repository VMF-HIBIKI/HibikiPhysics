#include <HibikiPhysics/BatchedOverlap.h>
#include <HibikiPhysics/MortonSpatialIndex.h>
#include "../fixtures/SpatialConformance.h"
#include <chrono>
#include <iostream>

using namespace HibikiPhysics;
using namespace HibikiPhysics::Test::SpatialReference;

namespace
{
class FailingIndex final : public SpatialOverlapIndex
{
  public:
	Ids	  Overlaps(const AABB &) const override { throw std::runtime_error("injected query failure"); }
	Pairs OverlapPairs() const override { return {}; }
};
} // namespace
int main()
{
	const AABB				  box({0, 0, 0}, {1, 1, 1});
	std::vector<SpatialEntry> entries;
	for (std::size_t i = 0; i < 132; ++i)
		entries.push_back({(i + 1) * 19, box});
	const auto	   index = MakeMortonSpatialOverlapProvider()->Build(entries);
	const auto	   before = std::chrono::steady_clock::now();
	const auto	   baseline = index->OverlapPairs();
	const auto	   middle = std::chrono::steady_clock::now();
	PairBatchStats stats;
	const auto	   parallel = CollectOverlapPairs(*index, entries, {4, 2}, stats);
	const auto	   after = std::chrono::steady_clock::now();
	Check(parallel == baseline && parallel == AllPairs(entries), "batched output vs serial and brute force");
	Check(stats.mPasses == 2 && stats.mCapacity == parallel.size() && stats.mWorkers == 4, "full count and grow/retry");
	Check(CollectOverlapPairs(*index, entries, {1, parallel.size()}, stats) == parallel && stats.mPasses == 1, "ready buffer");
	FailingIndex failing;
	try
	{
		(void)CollectOverlapPairs(failing, entries, {3, 1}, stats);
		throw std::logic_error("missing failure");
	}
	catch (const std::runtime_error &e)
	{
		Check(std::string_view(e.what()) == "injected query failure", "original failure");
	}
	Check(stats.mPasses == 1 && stats.mWorkers == 1, "failed collection preserves caller statistics");
	CapabilityConfig config;
	config.Add(CpuSpatialOverlapPlugin());
	config.Replace(BatchedMortonOverlapPlugin({3, 0}));
	Dependencies deps;
	SpatialOverlapSlot().Declare(deps);
	const auto ctx = config.Build(deps);
	HibikiPhysics::Test::CheckOverlapProvider(SpatialOverlapSlot().Get(ctx));
	std::cout << "dense pairs=" << parallel.size() << " serial_ms=" << std::chrono::duration<double, std::milli>(middle - before).count() << " batch_ms=" << std::chrono::duration<double, std::milli>(after - middle).count() << '\n';
}
