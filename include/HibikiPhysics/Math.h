#pragma once

#include <array>

namespace HibikiPhysics
{
struct Vec3
{
	double mX = 0, mY = 0, mZ = 0;

	[[nodiscard]] constexpr Vec3		operator+(Vec3 inRhs) const noexcept { return {mX + inRhs.mX, mY + inRhs.mY, mZ + inRhs.mZ}; }
	[[nodiscard]] constexpr Vec3		operator-(Vec3 inRhs) const noexcept { return {mX - inRhs.mX, mY - inRhs.mY, mZ - inRhs.mZ}; }
	[[nodiscard]] constexpr Vec3		operator-() const noexcept { return {-mX, -mY, -mZ}; }
	[[nodiscard]] constexpr Vec3		operator*(double inScale) const noexcept { return {mX * inScale, mY * inScale, mZ * inScale}; }
	[[nodiscard]] friend constexpr Vec3 operator*(double inScale, Vec3 inVector) noexcept { return inVector * inScale; }
	[[nodiscard]] constexpr double		Dot(Vec3 inRhs) const noexcept { return mX * inRhs.mX + mY * inRhs.mY + mZ * inRhs.mZ; }
	[[nodiscard]] constexpr Vec3		Cross(Vec3 inRhs) const noexcept
	{
		return {mY * inRhs.mZ - mZ * inRhs.mY, mZ * inRhs.mX - mX * inRhs.mZ, mX * inRhs.mY - mY * inRhs.mX};
	}
	[[nodiscard]] double Length() const noexcept;
};

// A right-handed unit rotation. The default is identity.
class Quat
{
  public:
	constexpr Quat() noexcept = default;
	[[nodiscard]] static Quat					  FromXYZW(std::array<double, 4> inComponents);
	[[nodiscard]] static Quat					  FromWXYZ(std::array<double, 4> inComponents);
	[[nodiscard]] static Quat					  FromAxisAngle(Vec3 inAxis, double inRadians);
	[[nodiscard]] constexpr std::array<double, 4> ToXYZW() const noexcept { return mXYZW; }
	[[nodiscard]] constexpr std::array<double, 4> ToWXYZ() const noexcept { return {mXYZW[3], mXYZW[0], mXYZW[1], mXYZW[2]}; }
	[[nodiscard]] Quat							  Inverse() const noexcept;
	[[nodiscard]] Quat							  operator*(const Quat &inRhs) const;
	[[nodiscard]] Vec3							  Rotate(Vec3 inVector) const noexcept;

  private:
	explicit constexpr Quat(std::array<double, 4> inUnitXYZW) noexcept : mXYZW(inUnitXYZW) {}
	std::array<double, 4> mXYZW{0, 0, 0, 1};
};

// Local to parent. A * B maps a point through B, then A.
class Transform
{
  public:
	constexpr Transform() noexcept = default;
	Transform(Vec3 inPosition, Quat inRotation);
	[[nodiscard]] constexpr Vec3 GetPosition() const noexcept { return mPosition; }
	[[nodiscard]] constexpr Quat GetRotation() const noexcept { return mRotation; }
	[[nodiscard]] Vec3			 TransformPoint(Vec3 inPoint) const noexcept { return mRotation.Rotate(inPoint) + mPosition; }
	[[nodiscard]] Vec3			 TransformDirection(Vec3 inDirection) const noexcept { return mRotation.Rotate(inDirection); }
	[[nodiscard]] Transform		 Inverse() const;
	[[nodiscard]] Transform		 operator*(const Transform &inRhs) const;

  private:
	Vec3 mPosition;
	Quat mRotation;
};
} // namespace HibikiPhysics
