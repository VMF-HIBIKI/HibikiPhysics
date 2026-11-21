# ADR-0021: Quaternions and rigid transforms

**English** | [简体中文](0021-quaternion-and-rigid-transform.zh-CN.md)

Status: Accepted

Date: 2026-11-21

Related work: [Issue #21](https://github.com/VMF-HIBIKI/HibikiPhysics/issues/21), [math dependency choice](0001-development-toolchain-and-dependencies.md) and [local geometry](0020-geometry-library-plugin.md).

## Context

Geometry and future dynamics plugins need a common interpretation of orientations and relative poses. Jolt's component constructor and MotrixSim's runtime pose use XYZW. MuJoCo model quaternions use WXYZ. Isaac Sim uses WXYZ in the Core API and XYZW in PhysX. Explicitly named conversions identify the order at an adapter boundary. [Jolt](https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Math/Quat.h) · [MotrixSim](https://motrixsim.readthedocs.io/en/latest/user_guide/kinematics/sensor/frame.html) · [MuJoCo](https://mujoco.readthedocs.io/en/stable/modeling.html#frame-orientations) · [Isaac Sim](https://docs.isaacsim.omniverse.nvidia.com/latest/reference_material/reference_conventions.html)

## Decision

The installed header `HibikiPhysics/Math.h` provides double-precision scalar value types. `Vec3` defaults to zero and supports addition, subtraction, scalar multiplication, dot product, right-handed cross product and Euclidean length. Eigen supplies independent reference calculations in the test target.

`Quat` represents a unit rotation. Its default is identity, with XYZW components (0, 0, 0, 1). `FromXYZW`, `FromWXYZ`, `ToXYZW` and `ToWXYZ` specify component order explicitly. Input components must be finite and have at least one nonzero value. Construction normalizes them. Axis-angle construction uses a finite nonzero axis, right-handed rotation and radians. Composition uses the Hamilton product and normalizes the result. Inversion takes the conjugate. Opposite quaternion signs represent the same orientation.

`Transform` maps from a local frame into its parent. Its position t is the finite position of the local origin in the parent frame, in metres. Its rotation R expresses the local basis in that frame. The default pose has zero translation and identity rotation. `TransformPoint` evaluates R p + t and `TransformDirection` evaluates R v. `Inverse` returns the reverse map with rotation R⁻¹ and translation −R⁻¹ t.

The right operand acts first. If A maps a link into the world and B maps a box into the link, A * B maps the box into the world. Its rotation is qA qB and its translation is tA + RA tB. The parent's rotation therefore acts on the child's offset as well as its axes.

## Consequences and validation

For a box with half extents (1, 0.5, 0.2) m, a +π/2 turn about +Z followed by translation (3, 2, 0) m takes the +X face centre (1, 0, 0) to (3, 3, 0). The +X direction becomes +Y. Applying the inverse recovers the local face centre. Applying the same translation before the turn takes the point to (−2, 4, 0).

The Eigen reference tests use angle-axis rotations and isometries for the box and for all eight corners under two noncommuting parent and child rotations. Tests also cover scaled inputs, quaternion sign equivalence, repeated composition and invalid input. A standalone installed consumer transforms the face centre. Placed-shape queries are a subsequent geometry task. [Reference tests](../../tests/math/RigidMathTests.cpp) · [Box pose example](../../examples/install-consumer/BoxPose.cpp)
