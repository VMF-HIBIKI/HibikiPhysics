#include <HibikiPhysics/StacklessSpatialIndex.h>
#include "MortonTree.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <stdexcept>

namespace
{
using namespace HibikiPhysics;
using Detail::BvhNode;
using Detail::Pairs;

// Reference DFS always pushes the right child first so each leaf sees the
// roots of the remaining forest on its stack, in reverse visit order.
class ExplicitStackIndex
{
  public:
	explicit ExplicitStackIndex(std::span<const SpatialEntry> entries) : mNodes(Detail::BuildMortonTree(entries)) {}

	std::vector<std::size_t> Overlaps(const AABB &bounds, std::size_t &visited) const
	{
		Detail::RequireFinite(bounds);
		std::vector<std::size_t> ids, pending;
		if (!mNodes.empty())
			pending.push_back(0);
		visited = Walk(bounds, pending, [&](std::size_t id)
					   { ids.push_back(id); });
		return ids;
	}

	Pairs OverlapPairs(std::size_t &visited) const
	{
		visited = 0;
		Pairs					 pairs;
		std::vector<std::size_t> pending, remaining;
		if (!mNodes.empty())
			pending.push_back(0);
		while (!pending.empty())
		{
			const auto i = pending.back();
			pending.pop_back();
			const auto &node = mNodes[i];
			if (node.IsLeaf())
			{
				remaining = pending;
				visited += Walk(node.mBounds, remaining, [&](std::size_t id)
								{ pairs.emplace_back(std::minmax(node.mId, id)); });
			}
			else
			{
				pending.push_back(node.mRight);
				pending.push_back(i + 1);
			}
		}
		return pairs;
	}

  private:
	template <class Emit>
	std::size_t Walk(const AABB &bounds, std::vector<std::size_t> &pending, Emit emit) const
	{
		std::size_t visited = 0;
		while (!pending.empty())
		{
			const auto i = pending.back();
			pending.pop_back();
			const auto &node = mNodes[i];
			++visited;
			if (!Detail::Intersects(bounds, node.mBounds))
				continue;
			if (node.IsLeaf())
				emit(node.mId);
			else
			{
				pending.push_back(node.mRight);
				pending.push_back(i + 1);
			}
		}
		return visited;
	}

	std::vector<BvhNode> mNodes;
};

std::vector<SpatialEntry> Boxes()
{
	std::mt19937			  random(52);
	std::vector<SpatialEntry> boxes;
	for (std::size_t i = 0; i < 1024; ++i)
	{
		std::array<double, 3> lo{}, hi{};
		for (std::size_t axis = 0; axis < 3; ++axis)
		{
			lo[axis] = static_cast<double>(random() % 64);
			hi[axis] = lo[axis] + static_cast<double>(random() % 9);
		}
		boxes.push_back({7 * i + 13, {lo, hi}});
	}
	return boxes;
}

template <class Range>
void Compare(Range a, Range b, std::size_t aVisited, std::size_t bVisited)
{
	std::ranges::sort(a);
	std::ranges::sort(b);
	if (a != b || aVisited != bVisited)
		throw std::logic_error("Stackless and explicit-stack candidates or node visits differ");
}

struct Totals
{
	std::size_t mCandidates = 0;
	std::size_t mVisits = 0;
	bool		operator==(const Totals &) const = default;
};

template <class Operation, class Index>
double Time(Operation operation, const Index &index, Totals expected)
{
	const auto start = std::chrono::steady_clock::now();
	const auto actual = operation(index);
	const auto stop = std::chrono::steady_clock::now();
	if (actual != expected)
		throw std::logic_error("Candidate or node totals changed between timings");
	return std::chrono::duration<double, std::micro>(stop - start).count();
}

template <class Operation>
void Measure(const char *label, Operation operation, const StacklessSpatialOverlapIndex &stackless, const ExplicitStackIndex &stack)
{
	const auto expected = operation(stack);
	if (operation(stackless) != expected)
		throw std::logic_error("Warm-up totals differ");
	std::array<double, 9> stacklessTimes, stackTimes;
	for (std::size_t i = 0; i < stacklessTimes.size(); ++i)
	{
		if (i % 2 == 0)
		{
			stackTimes[i] = Time(operation, stack, expected);
			stacklessTimes[i] = Time(operation, stackless, expected);
		}
		else
		{
			stacklessTimes[i] = Time(operation, stackless, expected);
			stackTimes[i] = Time(operation, stack, expected);
		}
	}
	std::ranges::sort(stacklessTimes);
	std::ranges::sort(stackTimes);
	std::cout << label << " candidates=" << expected.mCandidates << " visited=" << expected.mVisits
			  << " stackless_median_us=" << stacklessTimes[stacklessTimes.size() / 2] << " explicit_stack_median_us=" << stackTimes[stackTimes.size() / 2] << '\n';
}
} // namespace

int main()
{
	const auto						   boxes = Boxes();
	const StacklessSpatialOverlapIndex stackless(boxes);
	const ExplicitStackIndex		   stack(boxes);
	std::size_t						   aVisited, bVisited;
	for (const auto &entry : boxes)
	{
		auto a = stackless.Overlaps(entry.mBounds, aVisited);
		auto b = stack.Overlaps(entry.mBounds, bVisited);
		Compare(std::move(a), std::move(b), aVisited, bVisited);
	}
	auto a = stackless.OverlapPairs(aVisited);
	auto b = stack.OverlapPairs(bVisited);
	Compare(std::move(a), std::move(b), aVisited, bVisited);

	auto boxQueries = [&](const auto &index)
	{
		Totals totals;
		for (const auto &entry : boxes)
		{
			std::size_t visited;
			totals.mCandidates += index.Overlaps(entry.mBounds, visited).size();
			totals.mVisits += visited;
		}
		return totals;
	};
	auto selfPairs = [](const auto &index)
	{
		std::size_t visited;
		const auto	pairs = index.OverlapPairs(visited);
		return Totals{pairs.size(), visited};
	};
	Measure("box_queries", boxQueries, stackless, stack);
	Measure("self_pairs", selfPairs, stackless, stack);
}
