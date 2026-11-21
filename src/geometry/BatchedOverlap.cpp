#include <HibikiPhysics/BatchedOverlap.h>
#include <HibikiPhysics/MortonSpatialIndex.h>
#include <algorithm>
#include <atomic>
#include <exception>
#include <limits>
#include <stdexcept>
#include <thread>

namespace HibikiPhysics
{
namespace
{
using Pair = std::pair<std::size_t, std::size_t>;
using Pairs = std::vector<Pair>;

// Reserve a contiguous block even when the caller's buffer only fits a prefix.
// The final cursor is the exact capacity needed for a retry.
void Flush(std::span<const Pair> batch, std::span<Pair> output, std::atomic<std::size_t> &cursor)
{
	if (batch.empty())
		return;
	auto offset = cursor.load(std::memory_order_relaxed);
	do
	{
		if (batch.size() > std::numeric_limits<std::size_t>::max() - offset)
			throw std::length_error("Spatial pair count overflow");
	} while (!cursor.compare_exchange_weak(offset, offset + batch.size(), std::memory_order_relaxed));
	if (offset < output.size())
		std::ranges::copy(batch.first(std::min(batch.size(), output.size() - offset)), output.begin() + static_cast<std::ptrdiff_t>(offset));
}

std::size_t CollectPass(const SpatialOverlapIndex &index, std::span<const SpatialEntry> entries, std::span<Pair> output, std::size_t workers)
{
	std::atomic<std::size_t> cursor{0};
	std::atomic<bool>		 cancelled{false};
	std::exception_ptr		 failure;
	const auto				 recordFailure = [&]
	{
		if (!cancelled.exchange(true, std::memory_order_relaxed))
			failure = std::current_exception();
	};
	const auto collect = [&](std::size_t start)
	{
		try
		{
			std::array<Pair, 64> batch;
			std::size_t			 size = 0;
			for (std::size_t i = start; i < entries.size() && !cancelled.load(std::memory_order_relaxed); i += workers)
			{
				const auto &entry = entries[i];
				for (const auto id : index.Overlaps(entry.mBounds))
				{
					if (entry.mId >= id)
						continue;
					batch[size++] = {entry.mId, id};
					if (size == batch.size())
					{
						Flush(batch, output, cursor);
						size = 0;
					}
				}
			}
			Flush(std::span(batch).first(size), output, cursor);
		}
		catch (...)
		{
			recordFailure();
		}
	};
	std::vector<std::jthread> threads;
	threads.reserve(workers - 1);
	try
	{
		for (std::size_t i = 1; i < workers && !cancelled.load(std::memory_order_relaxed); ++i)
			threads.emplace_back(collect, i);
		collect(0);
	}
	catch (...)
	{
		recordFailure();
	}
	for (auto &thread : threads)
		thread.join();
	if (failure)
		std::rethrow_exception(failure);
	return cursor.load(std::memory_order_relaxed);
}

class BatchedIndex final : public SpatialOverlapIndex
{
  public:
	BatchedIndex(std::span<const SpatialEntry> entries, PairBatchSettings settings)
		: mEntries(entries.begin(), entries.end()), mIndex(MakeMortonSpatialOverlapProvider()->Build(mEntries)), mSettings(settings) {}
	std::vector<std::size_t> Overlaps(const AABB &bounds) const override { return mIndex->Overlaps(bounds); }
	Pairs					 OverlapPairs() const override
	{
		PairBatchStats stats;
		return CollectOverlapPairs(*mIndex, mEntries, mSettings, stats);
	}

  private:
	std::vector<SpatialEntry>				   mEntries;
	std::unique_ptr<const SpatialOverlapIndex> mIndex;
	PairBatchSettings						   mSettings;
};
class BatchedProvider final : public SpatialOverlapProvider
{
  public:
	explicit BatchedProvider(PairBatchSettings settings) : mSettings(settings) {}
	std::unique_ptr<const SpatialOverlapIndex> Build(std::span<const SpatialEntry> entries) const override { return std::make_unique<BatchedIndex>(entries, mSettings); }

  private:
	PairBatchSettings mSettings;
};
} // namespace

std::vector<std::pair<std::size_t, std::size_t>> CollectOverlapPairs(const SpatialOverlapIndex &inIndex, std::span<const SpatialEntry> inEntries, PairBatchSettings inSettings, PairBatchStats &outStats)
{
	if (inEntries.empty())
	{
		outStats = {};
		return {};
	}
	const auto	   requested = inSettings.mWorkers ? inSettings.mWorkers : std::max(1u, std::thread::hardware_concurrency());
	PairBatchStats stats{std::min(inEntries.size(), requested), 0, inSettings.mInitialCapacity};
	Pairs		   output;
	for (;;)
	{
		output.resize(stats.mCapacity);
		++stats.mPasses;
		const auto required = CollectPass(inIndex, inEntries, output, stats.mWorkers);
		if (required <= output.size())
		{
			output.resize(required);
			std::ranges::sort(output);
			outStats = stats;
			return output;
		}
		stats.mCapacity = required;
	}
}

std::unique_ptr<SpatialOverlapProvider> MakeBatchedMortonOverlapProvider(PairBatchSettings inSettings)
{
	return std::make_unique<BatchedProvider>(inSettings);
}
CapabilityRegistration BatchedMortonOverlapPlugin(PairBatchSettings inSettings)
{
	return SpatialOverlapSlot().Factory("hibiki.morton_batch", [inSettings]
										{ return MakeBatchedMortonOverlapProvider(inSettings); });
}
} // namespace HibikiPhysics
