#include <HibikiPhysics/HeightField.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace HibikiPhysics
{
HeightField::HeightField(std::size_t inRows, std::size_t inColumns, std::array<double, 2> inSpacing, double inBase, std::vector<double> inSamples)
	: mSamples(std::move(inSamples)), mRows(inRows), mColumns(inColumns), mSpacing(inSpacing), mBase(inBase)
{
	if (mRows < 2 || mColumns < 2 || mRows > std::numeric_limits<std::size_t>::max() / mColumns || mRows * mColumns != mSamples.size())
		throw std::invalid_argument("Expected a complete height field of at least two rows and columns");
	for (double spacing : mSpacing)
		if (!std::isfinite(spacing) || spacing <= 0)
			throw std::invalid_argument("Expected positive finite height field spacing");
	if (!std::isfinite(static_cast<double>(mColumns - 1) * mSpacing[0]) || !std::isfinite(static_cast<double>(mRows - 1) * mSpacing[1]))
		throw std::overflow_error("Height field footprint exceeds finite range");
	if (!std::isfinite(mBase))
		throw std::invalid_argument("Expected a finite height field base");
	for (double elevation : mSamples)
		if (!std::isfinite(elevation) || elevation < mBase)
			throw std::invalid_argument("Expected finite elevations at or above the base");
}

namespace
{
struct CellCoordinate
{
	std::size_t mIndex;
	double		mFraction;
};

CellCoordinate Locate(double position, double spacing, std::size_t count)
{
	std::size_t i = std::min(static_cast<std::size_t>(position / spacing), count - 2);
	const auto	coordinate = [spacing](std::size_t index)
	{ return static_cast<double>(index) * spacing; };
	if (i > 0 && position < coordinate(i))
		--i;
	else if (i < count - 2 && position > coordinate(i + 1))
		++i;
	return {i, (position - coordinate(i)) / (coordinate(i + 1) - coordinate(i))};
}
} // namespace

std::optional<double> HeightField::SurfaceHeight(double inX, double inY) const
{
	if (!std::isfinite(inX) || !std::isfinite(inY))
		throw std::invalid_argument("Expected finite height field coordinates");
	if (inX < 0 || inY < 0 || inX > static_cast<double>(mColumns - 1) * mSpacing[0] || inY > static_cast<double>(mRows - 1) * mSpacing[1])
		return std::nullopt;
	const auto [col, u] = Locate(inX, mSpacing[0], mColumns);
	const auto [row, v] = Locate(inY, mSpacing[1], mRows);
	const std::size_t i = row * mColumns + col;
	const double	  a = mSamples[i], d = mSamples[i + mColumns + 1];
	// Interpolate from the origin corner to the opposite edge of the selected triangle.
	if (u >= v)
		return u == 0 ? a : std::lerp(a, std::lerp(mSamples[i + 1], d, v / u), u);
	return std::lerp(a, std::lerp(mSamples[i + mColumns], d, u / v), v);
}
} // namespace HibikiPhysics
