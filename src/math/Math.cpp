#include <HibikiPhysics/Math.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace HibikiPhysics
{
namespace
{
template <std::size_t N>
std::array<double, N> Normalize(std::array<double, N> inValues, const char *inError)
{
	double largest = 0;
	for (double value : inValues)
	{
		if (!std::isfinite(value))
			throw std::invalid_argument(inError);
		largest = std::max(largest, std::abs(value));
	}
	if (largest == 0)
		throw std::invalid_argument(inError);

	// Scale before squaring so both subnormal and very large inputs retain their direction.
	double sum = 0;
	for (double &value : inValues)
	{
		value /= largest;
		sum += value * value;
	}
	const double length = std::sqrt(sum);
	for (double &value : inValues)
		value /= length;
	return inValues;
}
} // namespace

double Vec3::Length() const noexcept
{
	return std::hypot(mX, mY, mZ);
}

Quat Quat::FromXYZW(std::array<double, 4> inComponents)
{
	return Quat(Normalize(inComponents, "Expected finite nonzero quaternion"));
}

Quat Quat::FromWXYZ(std::array<double, 4> inComponents)
{
	const auto [w, x, y, z] = inComponents;
	return FromXYZW({x, y, z, w});
}

Quat Quat::FromAxisAngle(Vec3 inAxis, double inRadians)
{
	if (!std::isfinite(inRadians))
		throw std::invalid_argument("Expected finite angle in radians");
	const auto	 axis = Normalize<3>({inAxis.mX, inAxis.mY, inAxis.mZ}, "Expected finite nonzero rotation axis");
	const double sine = std::sin(0.5 * inRadians);
	return FromXYZW({axis[0] * sine, axis[1] * sine, axis[2] * sine, std::cos(0.5 * inRadians)});
}

Quat Quat::Inverse() const noexcept
{
	const auto [x, y, z, w] = mXYZW;
	return Quat({-x, -y, -z, w});
}

Quat Quat::operator*(const Quat &inRhs) const
{
	const auto [ax, ay, az, aw] = mXYZW;
	const auto [bx, by, bz, bw] = inRhs.mXYZW;
	const Vec3 a{ax, ay, az}, b{bx, by, bz};
	const Vec3 v = aw * b + bw * a + a.Cross(b);
	return FromXYZW({v.mX, v.mY, v.mZ, aw * bw - a.Dot(b)});
}

Vec3 Quat::Rotate(Vec3 inVector) const noexcept
{
	const auto [x, y, z, w] = mXYZW;
	// Columns are the rotated local basis vectors expressed in the parent frame.
	const Vec3 basisX{1 - 2 * (y * y + z * z), 2 * (x * y + w * z), 2 * (x * z - w * y)};
	const Vec3 basisY{2 * (x * y - w * z), 1 - 2 * (x * x + z * z), 2 * (y * z + w * x)};
	const Vec3 basisZ{2 * (x * z + w * y), 2 * (y * z - w * x), 1 - 2 * (x * x + y * y)};
	return inVector.mX * basisX + inVector.mY * basisY + inVector.mZ * basisZ;
}

Transform::Transform(Vec3 inPosition, Quat inRotation) : mPosition(inPosition), mRotation(inRotation)
{
	if (!std::isfinite(inPosition.mX) || !std::isfinite(inPosition.mY) || !std::isfinite(inPosition.mZ))
		throw std::invalid_argument("Expected a finite local origin in parent coordinates");
}

Transform Transform::Inverse() const
{
	const Quat inverse = mRotation.Inverse();
	return {inverse.Rotate(-mPosition), inverse};
}

Transform Transform::operator*(const Transform &inRhs) const
{
	return {TransformPoint(inRhs.mPosition), mRotation * inRhs.mRotation};
}
} // namespace HibikiPhysics
