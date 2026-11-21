#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

namespace HibikiPhysics
{
// Row-major samples in local length units. Columns advance along X and rows along Y.
class HeightField
{
  public:
	HeightField(std::size_t inRows, std::size_t inColumns, std::array<double, 2> inSpacing, double inBase, std::vector<double> inSamples);
	// Copies preserve the validated grid; construction takes ownership of the sample buffer.
	HeightField(const HeightField &) = default;
	HeightField								&operator=(const HeightField &) = default;
	[[nodiscard]] std::size_t				 GetRowCount() const noexcept { return mRows; }
	[[nodiscard]] std::size_t				 GetColumnCount() const noexcept { return mColumns; }
	[[nodiscard]] std::array<double, 2>		 GetSpacing() const noexcept { return mSpacing; }
	[[nodiscard]] double					 GetBase() const noexcept { return mBase; }
	[[nodiscard]] const std::vector<double> &GetSamples() const noexcept { return mSamples; }
	// Closed XY footprint. A finite point outside the footprint has no surface height.
	[[nodiscard]] std::optional<double> SurfaceHeight(double inX, double inY) const;

  private:
	std::vector<double>	  mSamples;
	std::size_t			  mRows, mColumns;
	std::array<double, 2> mSpacing;
	double				  mBase;
};
} // namespace HibikiPhysics
