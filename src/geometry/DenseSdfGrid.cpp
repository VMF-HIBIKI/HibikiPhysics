#include <HibikiPhysics/DenseSdfGrid.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace HibikiPhysics
{
namespace
{
bool IsFinite(Vec3 point)
{
	return std::isfinite(point.mX) && std::isfinite(point.mY) && std::isfinite(point.mZ);
}

double Coordinate(double first, double spacing, std::size_t index)
{
	return first + static_cast<double>(index) * spacing;
}

double LastCoordinate(double first, double spacing, std::size_t count)
{
	double previous = first;
	for (std::size_t i = 1; i < count; ++i)
	{
		const double next = Coordinate(first, spacing, i);
		if (!std::isfinite(next) || !std::isfinite(next - first))
			throw std::overflow_error("Dense SDF coordinate range exceeds finite precision");
		if (next <= previous)
			throw std::invalid_argument("Dense SDF spacing must produce distinct node coordinates");
		previous = next;
	}
	return previous;
}

struct CellCoordinate
{
	std::size_t mIndex;
	double		mFraction;
};

CellCoordinate Locate(double position, double first, double spacing, std::size_t count)
{
	const auto coordinate = [first, spacing](std::size_t index)
	{ return Coordinate(first, spacing, index); };
	std::size_t i = std::min(static_cast<std::size_t>((position - first) / spacing), count - 2);
	// Rounding a translated node into mesh coordinates can shift the inverse calculation across a cell boundary.
	while (i > 0 && position < coordinate(i))
		--i;
	while (i < count - 2 && position > coordinate(i + 1))
		++i;
	const double lower = coordinate(i);
	return {i, (position - lower) / (coordinate(i + 1) - lower)};
}
} // namespace

DenseSdfGrid::DenseSdfGrid(Vec3 inOrigin, double inSpacing, std::array<std::size_t, 3> inNodeCounts, std::vector<double> inValues)
	: mOrigin(inOrigin), mSpacing(inSpacing), mNodeCounts(inNodeCounts)
{
	if (!IsFinite(mOrigin))
		throw std::invalid_argument("Expected a finite dense SDF origin");
	if (!std::isfinite(mSpacing) || mSpacing <= 0)
		throw std::invalid_argument("Expected positive finite dense SDF spacing");
	std::size_t total = 1;
	for (std::size_t count : mNodeCounts)
	{
		if (count < 2)
			throw std::invalid_argument("Expected at least two dense SDF nodes per axis");
		if (count > inValues.max_size() / total)
			throw std::overflow_error("Dense SDF size exceeds the addressable value buffer");
		total *= count;
	}
	if (total != inValues.size())
		throw std::invalid_argument("Dense SDF value count must match the grid dimensions");
	mLastNode = {LastCoordinate(mOrigin.mX, mSpacing, mNodeCounts[0]), LastCoordinate(mOrigin.mY, mSpacing, mNodeCounts[1]), LastCoordinate(mOrigin.mZ, mSpacing, mNodeCounts[2])};
	for (double value : inValues)
		if (!std::isfinite(value))
			throw std::invalid_argument("Expected finite dense SDF values");
	mValues = std::make_shared<const std::vector<double>>(std::move(inValues));
}

double DenseSdfGrid::GetNode(std::size_t inX, std::size_t inY, std::size_t inZ) const
{
	const auto [nx, ny, nz] = mNodeCounts;
	if (inX >= nx || inY >= ny || inZ >= nz)
		throw std::out_of_range("Dense SDF node index outside grid");
	return (*mValues)[inX + nx * (inY + ny * inZ)];
}

std::optional<double> DenseSdfGrid::Sample(Vec3 inPoint) const
{
	if (!IsFinite(inPoint))
		throw std::invalid_argument("Expected finite dense SDF query coordinates");
	if (inPoint.mX < mOrigin.mX || inPoint.mX > mLastNode.mX || inPoint.mY < mOrigin.mY || inPoint.mY > mLastNode.mY || inPoint.mZ < mOrigin.mZ || inPoint.mZ > mLastNode.mZ)
		return std::nullopt;

	const auto [nx, ny, nz] = mNodeCounts;
	const auto [x, u] = Locate(inPoint.mX, mOrigin.mX, mSpacing, nx);
	const auto [y, v] = Locate(inPoint.mY, mOrigin.mY, mSpacing, ny);
	const auto [z, w] = Locate(inPoint.mZ, mOrigin.mZ, mSpacing, nz);
	const auto		 &values = *mValues;
	const std::size_t first = x + nx * (y + ny * z);
	const auto		  plane = [&](std::size_t i)
	{
		return std::lerp(std::lerp(values[i], values[i + 1], u), std::lerp(values[i + nx], values[i + nx + 1], u), v);
	};
	return std::lerp(plane(first), plane(first + nx * ny), w);
}
} // namespace HibikiPhysics
