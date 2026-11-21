#include <HibikiPhysics/SpatialIndex.h>

#include <algorithm>
#include <cmath>
#include <compare>
#include <limits>
#include <numeric>
#include <stdexcept>

namespace HibikiPhysics
{
namespace
{
using Point = std::array<double, 3>;
using Pairs = std::vector<std::pair<std::size_t, std::size_t>>;

void RequireFinite(const Point &point)
{
	for (double coordinate : point)
		if (!std::isfinite(coordinate))
			throw std::invalid_argument("Spatial index coordinates must be finite");
}

void RequireFinite(const AABB &bounds)
{
	RequireFinite(bounds.GetMin());
	RequireFinite(bounds.GetMax());
}

bool Intersects(const AABB &a, const AABB &b)
{
	const auto aMin = a.GetMin(), aMax = a.GetMax(), bMin = b.GetMin(), bMax = b.GetMax();
	for (std::size_t axis = 0; axis < 3; ++axis)
		if (aMin[axis] > bMax[axis] || bMin[axis] > aMax[axis])
			return false;
	return true;
}

AABB Enclose(std::span<const SpatialEntry> entries)
{
	auto lo = entries.front().mBounds.GetMin(), hi = entries.front().mBounds.GetMax();
	for (const auto &entry : entries.subspan(1))
	{
		const auto entryMin = entry.mBounds.GetMin(), entryMax = entry.mBounds.GetMax();
		for (std::size_t axis = 0; axis < 3; ++axis)
		{
			lo[axis] = std::min(lo[axis], entryMin[axis]);
			hi[axis] = std::max(hi[axis], entryMax[axis]);
		}
	}
	return {lo, hi};
}

std::size_t LongestAxis(const AABB &bounds)
{
	const auto lo = bounds.GetMin(), hi = bounds.GetMax();
	Point	   lengths{};
	for (std::size_t axis = 0; axis < 3; ++axis)
		lengths[axis] = hi[axis] - lo[axis];
	if (std::isinf(*std::max_element(lengths.begin(), lengths.end())))
		for (std::size_t axis = 0; axis < 3; ++axis)
			lengths[axis] = hi[axis] * 0.5 - lo[axis] * 0.5;
	return static_cast<std::size_t>(std::max_element(lengths.begin(), lengths.end()) - lengths.begin());
}

// Store positive slab times as a normalized fraction and binary exponent. A finite
// ray can cross a finite box at a parameter outside the range of a double.
struct RayTime
{
	int	   mExponent = std::numeric_limits<int>::min();
	double mFraction = 0;
	auto   operator<=>(const RayTime &) const = default;
};

RayTime SlabTime(double origin, double boundary, double speed, bool upper)
{
	if (boundary <= origin)
		return {};
	double delta = boundary - origin;
	int	   scale = 0;
	if (std::isinf(delta))
	{
		delta = boundary * 0.5 - origin * 0.5;
		scale = 1;
	}
	int	   deltaExponent, speedExponent, quotientExponent;
	double fraction = std::frexp(delta, &deltaExponent) / std::frexp(speed, &speedExponent);
	// Enclose subtraction and division roundoff at faces, edges and corners.
	constexpr double padding = 4 * std::numeric_limits<double>::epsilon();
	fraction = std::frexp(fraction * (upper ? 1 + padding : 1 - padding), &quotientExponent);
	return {deltaExponent + scale - speedExponent + quotientExponent, fraction};
}

bool RayIntersects(const Point &origin, const Point &direction, const AABB &bounds)
{
	const auto lo = bounds.GetMin(), hi = bounds.GetMax();
	RayTime	   enter, exit{std::numeric_limits<int>::max(), 0};
	for (std::size_t axis = 0; axis < 3; ++axis)
	{
		double start = origin[axis], low = lo[axis], high = hi[axis], speed = direction[axis];
		if (speed == 0)
		{
			if (start < low || start > high)
				return false;
			continue;
		}
		if (speed < 0)
		{
			start = -start;
			std::swap(low, high);
			low = -low;
			high = -high;
			speed = -speed;
		}
		if (start > high)
			return false;
		enter = std::max(enter, SlabTime(start, low, speed, false));
		exit = std::min(exit, SlabTime(start, high, speed, true));
		if (enter > exit)
			return false;
	}
	return true;
}

double DistanceLowerBound(const Point &point, const AABB &bounds)
{
	const auto lo = bounds.GetMin(), hi = bounds.GetMax();
	Point	   gap{};
	for (std::size_t axis = 0; axis < 3; ++axis)
		gap[axis] = std::abs(point[axis] - std::clamp(point[axis], lo[axis], hi[axis]));
	const double distance = std::hypot(std::hypot(gap[0], gap[1]), gap[2]);
	// Round toward zero so a boundary tie survives subtraction and norm roundoff.
	return std::nextafter(distance * (1 - 8 * std::numeric_limits<double>::epsilon()), 0.0);
}

class CpuBvh final : public SpatialQueryIndex
{
  public:
	explicit CpuBvh(std::span<const SpatialEntry> input)
	{
		if (input.empty())
			return;
		std::vector<SpatialEntry> entries(input.begin(), input.end());
		for (const auto &entry : entries)
			RequireFinite(entry.mBounds);
		std::ranges::sort(entries, {}, &SpatialEntry::mId);
		if (std::ranges::adjacent_find(entries, {}, &SpatialEntry::mId) != entries.end())
			throw std::invalid_argument("Duplicate spatial entry ID");
		if (entries.size() - 1 > (mNodes.max_size() - 1) / 2)
			throw std::length_error("Too many spatial entries");
		mNodes.reserve(entries.size() + (entries.size() - 1));
		Build(entries);
	}

	std::vector<std::size_t> Overlaps(const AABB &bounds) const override
	{
		RequireFinite(bounds);
		return Candidates([&](const AABB &node)
						  { return Intersects(bounds, node); });
	}

	Pairs OverlapPairs() const override
	{
		Pairs result;
		// Each pair of leaves has exactly one lowest common ancestor.
		for (std::size_t i = 0; i < mNodes.size(); ++i)
			if (!mNodes[i].IsLeaf())
				CrossPairs(i + 1, mNodes[i].mRight, result);
		std::ranges::sort(result);
		return result;
	}

	std::vector<std::size_t> RayCandidates(Point origin, Point direction) const override
	{
		RequireFinite(origin);
		RequireFinite(direction);
		return Candidates([&](const AABB &bounds)
						  { return RayIntersects(origin, direction, bounds); });
	}

	std::optional<SpatialNearest> Nearest(Point point, const std::function<double(std::size_t)> &exactDistance) const override
	{
		RequireFinite(point);
		if (!exactDistance)
			throw std::invalid_argument("Missing exact primitive distance callback");
		std::optional<SpatialNearest> best;
		if (!mNodes.empty())
			VisitNearest(0, 0, point, exactDistance, best);
		return best;
	}

  private:
	// Preorder layout places the left child immediately after its parent.
	struct Node
	{
		AABB		mBounds;
		std::size_t mRight = 0;
		std::size_t mId = 0;
		bool		IsLeaf() const { return mRight == 0; }
	};
	std::vector<Node> mNodes;

	std::size_t Build(std::span<SpatialEntry> entries)
	{
		const auto index = mNodes.size();
		mNodes.push_back({Enclose(entries)});
		if (entries.size() == 1)
		{
			mNodes[index].mId = entries.front().mId;
			return index;
		}
		const auto axis = LongestAxis(mNodes[index].mBounds);
		const auto middle = entries.size() / 2;
		auto	   key = [axis](const SpatialEntry &entry)
		{
			return std::pair(std::midpoint(entry.mBounds.GetMin()[axis], entry.mBounds.GetMax()[axis]), entry.mId);
		};
		std::ranges::nth_element(entries, entries.begin() + static_cast<std::ptrdiff_t>(middle), {}, key);
		Build(entries.first(middle));
		mNodes[index].mRight = Build(entries.subspan(middle));
		return index;
	}

	template <class Predicate>
	std::vector<std::size_t> Candidates(Predicate intersects) const
	{
		std::vector<std::size_t> ids;
		auto					 visit = [&](auto &self, std::size_t index) -> void
		{
			const auto &node = mNodes[index];
			if (!intersects(node.mBounds))
				return;
			if (node.IsLeaf())
				ids.push_back(node.mId);
			else
			{
				self(self, index + 1);
				self(self, node.mRight);
			}
		};
		if (!mNodes.empty())
			visit(visit, 0);
		std::ranges::sort(ids);
		return ids;
	}

	void CrossPairs(std::size_t a, std::size_t b, Pairs &out) const
	{
		if (!Intersects(mNodes[a].mBounds, mNodes[b].mBounds))
			return;
		if (mNodes[a].IsLeaf())
		{
			if (mNodes[b].IsLeaf())
			{
				out.emplace_back(std::minmax(mNodes[a].mId, mNodes[b].mId));
				return;
			}
			std::swap(a, b);
		}
		CrossPairs(b, a + 1, out);
		CrossPairs(b, mNodes[a].mRight, out);
	}

	void VisitNearest(std::size_t index, double lower, const Point &point, const std::function<double(std::size_t)> &exactDistance, std::optional<SpatialNearest> &best) const
	{
		if (best && lower > best->mDistance)
			return;
		const auto &node = mNodes[index];
		if (node.IsLeaf())
		{
			const double distance = exactDistance(node.mId);
			if (!(distance >= 0))
				throw std::invalid_argument("Exact primitive distance must be nonnegative");
			if (std::isfinite(distance) && (!best || distance < best->mDistance || (distance == best->mDistance && node.mId < best->mId)))
				best = SpatialNearest{node.mId, distance};
			return;
		}
		auto   near = index + 1, far = node.mRight;
		double nearDistance = DistanceLowerBound(point, mNodes[near].mBounds), farDistance = DistanceLowerBound(point, mNodes[far].mBounds);
		if (farDistance < nearDistance)
		{
			std::swap(near, far);
			std::swap(nearDistance, farDistance);
		}
		VisitNearest(near, nearDistance, point, exactDistance, best);
		VisitNearest(far, farDistance, point, exactDistance, best);
	}
};

template <class Provider, class Index>
class CpuProvider final : public Provider
{
  public:
	std::unique_ptr<const Index> Build(std::span<const SpatialEntry> entries) const override { return std::make_unique<CpuBvh>(entries); }
};
} // namespace

const CapabilitySlot<SpatialOverlapProvider> &SpatialOverlapSlot()
{
	static const CapabilitySlot<SpatialOverlapProvider> slot("spatial.overlap");
	return slot;
}

const CapabilitySlot<SpatialQueryProvider> &SpatialQuerySlot()
{
	static const CapabilitySlot<SpatialQueryProvider> slot("spatial.ray_nearest");
	return slot;
}

CapabilityRegistration CpuSpatialOverlapPlugin()
{
	return SpatialOverlapSlot().Factory("hibiki.cpu_bvh", []
										{ return std::make_unique<CpuProvider<SpatialOverlapProvider, SpatialOverlapIndex>>(); });
}

CapabilityRegistration CpuSpatialQueryPlugin()
{
	return SpatialQuerySlot().Factory("hibiki.cpu_bvh", MakeCpuSpatialQueryProvider);
}

std::unique_ptr<SpatialQueryProvider> MakeCpuSpatialQueryProvider()
{
	return std::make_unique<CpuProvider<SpatialQueryProvider, SpatialQueryIndex>>();
}
} // namespace HibikiPhysics
