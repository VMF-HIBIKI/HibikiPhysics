#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numeric>

namespace HibikiPhysics::Detail
{
struct ScaledDeterminant
{
	double fraction = 0;
	int	   exponent = 0;
};

// Exact sign of the homogeneous orientation determinant for finite binary64 coordinates.
// Each determinant term is a product of two or three integer significands and a power of two.
// A common exponent of -1074 per factor lets us add the signed terms in fixed base-1024 bins.
template <std::size_t Dimension>
int ExactOrientation(const std::array<std::array<double, Dimension>, Dimension + 1> &points, ScaledDeterminant *value = nullptr)
{
	static_assert(std::numeric_limits<double>::is_iec559 && sizeof(double) == sizeof(std::uint64_t));
	constexpr int radixBits = 10;
	constexpr int radix = 1 << radixBits;
	constexpr int mask = radix - 1;
	// A finite double in units of the smallest subnormal occupies at most 2098 bits.
	std::array<std::int64_t, (Dimension * 2098 + 9) / 10 + 2> bins{};
	std::array<std::size_t, Dimension + 1>					  order{};
	std::iota(order.begin(), order.end(), std::size_t{0});
	do
	{
		int sign = 1;
		for (std::size_t i = 0; i < order.size(); ++i)
			for (std::size_t j = i + 1; j < order.size(); ++j)
				if (order[i] > order[j])
					sign = -sign;
		std::array<std::uint64_t, (Dimension * 53 + 9) / 10> digits{1};
		std::size_t											 length = 1;
		unsigned											 shift = 0;
		for (std::size_t axis = 0; axis < Dimension; ++axis)
		{
			const auto	   bits = std::bit_cast<std::uint64_t>(points[order[axis]][axis]);
			const auto	   exponent = static_cast<unsigned>((bits >> 52) & 0x7ff);
			constexpr auto hiddenBit = std::uint64_t{1} << 52;
			const auto	   significand = (bits & (hiddenBit - 1)) | (exponent ? hiddenBit : 0);
			if (significand == 0)
			{
				length = 0;
				break;
			}
			if (bits >> 63)
				sign = -sign;
			if (exponent)
				shift += exponent - 1;
			std::uint64_t carry = 0;
			for (std::size_t i = 0; i < length; ++i)
			{
				const auto product = digits[i] * significand + carry;
				digits[i] = product & mask;
				carry = product >> radixBits;
			}
			while (carry != 0)
			{
				digits[length++] = carry & mask;
				carry >>= radixBits;
			}
		}
		for (std::size_t i = 0; i < length; ++i)
			bins[shift / radixBits + i] += sign * static_cast<std::int64_t>(digits[i] << (shift % radixBits));
	} while (std::next_permutation(order.begin(), order.end()));

	bool nonzero = false;
	for (std::size_t i = 0; i + 1 < bins.size(); ++i)
	{
		nonzero |= (bins[i] & mask) != 0;
		bins[i + 1] += bins[i] >> radixBits;
		bins[i] &= mask;
	}
	const int sign = bins.back() < 0 ? -1 : (bins.back() > 0 || nonzero);
	if (value && sign != 0)
	{
		if (sign < 0)
		{
			// Convert the negative, sign-extended digit array to its magnitude.
			std::int64_t carry = 0;
			for (auto &digit : bins)
			{
				const auto next = carry - digit;
				digit = next & mask;
				carry = next >> radixBits;
			}
		}
		std::size_t high = bins.size() - 1;
		while (bins[high] == 0)
			--high;
		// Retain at least 61 leading bits. Keep the scale separate for determinants
		// outside binary64 range whose ratio to a face area is still representable.
		const auto low = high > 6 ? high - 6 : 0;
		double	   leading = 0;
		for (std::size_t j = high + 1; j > low; --j)
			leading = leading * radix + static_cast<double>(bins[j - 1]);
		value->fraction = sign * std::frexp(leading, &value->exponent);
		value->exponent += static_cast<int>(low) * radixBits - 1074 * static_cast<int>(Dimension);
	}
	else if (value)
		*value = {};
	return sign;
}

using Point2 = std::array<double, 2>;
using Point3 = std::array<double, 3>;

inline int Orientation(Point2 a, Point2 b, Point2 c)
{
	if (a == b || a == c || b == c || (a[0] == b[0] && a[0] == c[0]) || (a[1] == b[1] && a[1] == c[1]))
		return 0;
	const double lhs = (a[0] - c[0]) * (b[1] - c[1]);
	const double rhs = (b[0] - c[0]) * (a[1] - c[1]);
	const double determinant = lhs - rhs;
	const double bound = 16 * std::numeric_limits<double>::epsilon() * (std::abs(lhs) + std::abs(rhs));
	if (std::isnormal(bound) && std::abs(determinant) > bound)
		return determinant > 0 ? 1 : -1;
	return ExactOrientation<2>({a, b, c});
}

inline int Orientation(Point3 a, Point3 b, Point3 c, Point3 d)
{
	if (a == b || a == c || a == d || b == c || b == d || c == d)
		return 0;
	for (std::size_t axis = 0; axis < 3; ++axis)
		if (a[axis] == b[axis] && a[axis] == c[axis] && a[axis] == d[axis])
			return 0;

	// Bound intermediate exponents as well as cancellation in the fast determinant.
	// Outside this range an underflow in a minor could be magnified by its third factor.
	Point3 u{}, v{}, w{};
	for (std::size_t i = 0; i < 3; ++i)
	{
		u[i] = a[i] - d[i];
		v[i] = b[i] - d[i];
		w[i] = c[i] - d[i];
		for (double value : {u[i], v[i], w[i]})
			if (value != 0 && (std::abs(value) < 0x1p-250 || std::abs(value) > 0x1p250))
				return ExactOrientation<3>({a, b, c, d});
	}
	double determinant = 0, permanent = 0;
	for (std::size_t i = 0; i < 3; ++i)
	{
		const auto	 j = (i + 1) % 3, k = (i + 2) % 3;
		const double lhs = v[j] * w[k], rhs = v[k] * w[j];
		determinant += u[i] * (lhs - rhs);
		permanent += std::abs(u[i]) * (std::abs(lhs) + std::abs(rhs));
	}
	if (std::abs(determinant) > 64 * std::numeric_limits<double>::epsilon() * permanent)
		return determinant > 0 ? 1 : -1;
	return ExactOrientation<3>({a, b, c, d});
}
} // namespace HibikiPhysics::Detail
