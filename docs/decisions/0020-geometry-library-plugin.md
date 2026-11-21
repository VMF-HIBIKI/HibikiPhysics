# ADR-0020: Geometry library plugin

**English** | [简体中文](0020-geometry-library-plugin.zh-CN.md)

Status: Accepted

Date: 2026-11-21

Related work: [Issue #17](https://github.com/VMF-HIBIKI/HibikiPhysics/issues/17) and [ADR-0010](0010-plugin-architecture-and-lifecycle.md).

## Context

HibikiPhysics needs geometry definitions that research plugins can share. Jolt exposes compound shapes and tapered primitives, MuJoCo defines a set of model geoms, MotrixSim distinguishes source geometry from the colliders generated for it, and Isaac Sim offers several collision representations for one mesh. These choices motivate tracking geometry representation, import and collision coverage independently. [Jolt shape types](https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Physics/Collision/Shape/Shape.h) · [MuJoCo geoms](https://mujoco.readthedocs.io/en/stable/XMLreference.html#body-geom) · [MotrixSim geometry API](https://motrixsim.readthedocs.io/en/v0.10.0/api_reference/core/motrixsim.html) · [Isaac Sim colliders](https://docs.isaacsim.omniverse.nvidia.com/latest/physics/simulation_fundamentals.html)

## Key choices

One official geometry library plugin can supply multiple geometry types. The plugin owns their definitions, local coordinate systems and size conventions. Format adapters own source conversion. Collision plugins own shape-pair selection and contact algorithms. The plugin core continues to assemble capabilities using ADR-0010.

The following families form the agreed candidate list. Implementation and validation will be recorded per type.

| Family | Candidates |
| --- | --- |
| Basic analytic geometry | Plane, closed half-space, finite rectangular patch, sphere, box, capsule, cylinder, ellipsoid |
| Extended analytic geometry | Cone, conical frustum, tapered capsule |
| Discrete and convex geometry | Triangle, convex hull, triangle mesh |
| Terrain and fields | Height field, signed distance field (SDF) |
| Instances and composition | Geometry with a local pose, compound geometry, shape-specific scaling rules |

## Local analytic geometry

The eight basic shapes use metres and shape-local coordinates. Instances will carry placement. The SDF representation and generation method remain research decisions. [Issue #19](https://github.com/VMF-HIBIKI/HibikiPhysics/issues/19).

| Shape | Definition and constructor dimensions |
| --- | --- |
| `Plane` | Infinite oriented XY surface through the origin, with unit normal +Z. Signed distance equals the query point's Z coordinate. |
| `HalfSpace` | Closed region with that plane as its boundary. A point belongs to it when Z ≤ 0. |
| `RectangularPatch` | Closed, zero-thickness rectangle centred at the origin in XY. Its two half extents determine the four edges. The nearest point clamps X and Y to those intervals and has Z = 0. |
| `Sphere` | Centre at the origin, with one radius. |
| `Box` | Centre at the origin, with three half extents along local X, Y and Z. Half extents (1, 0.5, 0.2) give full side lengths (2, 1, 0.4). |
| `Capsule` | Radius r and cylinder half-length h. Cap centres lie at z = ±h. Full axial length is 2(h + r). At h = 0 the capsule occupies a sphere of radius r. |
| `Cylinder` | Radius and half-height h. Its flat caps lie at z = ±h. |
| `Ellipsoid` | Three semi-axis lengths in local X, Y, Z order. |

Dimensions are finite and positive, with zero also valid for the capsule's cylinder half-length. Construction and point queries report invalid inputs with `std::invalid_argument`. Capsule half-extent queries report an unrepresentable axial extent with `std::overflow_error`. The API uses double-precision coordinates. A capsule with r = 0.2 and h = 0.5 has a 1.0 m spine and measures 1.4 m tip to tip.

## Registration and validation

`GeometryLibrary::Contains` tests local points against the five closed solids and the closed half-space. Boundaries are included and the double-precision comparisons apply zero contact margin. With radius 0.2 m and axial half-length 0.5 m, a capsule contains (0, 0, 0.6) and a cylinder ends at z = 0.5 m. A capsule with a zero-length spine uses the same membership criterion as a sphere.

`LocalBounds` returns a local axis-aligned bounding box (`AABB`) for each of the eight shapes. Its min/max endpoints define closed intervals. The plane spans both horizontal axes with Z fixed at zero. The half-space spans both horizontal axes and extends down to negative infinity in Z. A patch has finite XY intervals and zero thickness. Infinite interval ends use IEEE infinities. A finite capsule's axial bound rounds outward when needed, and an unrepresentable finite bound raises `std::overflow_error`. Use `AABB::Contains` for box membership and the shape overload for membership in the actual solid.

An application linking `hibiki::physics` includes `<HibikiPhysics/Geometry.h>` and registers `GeometryLibraryPlugin()` with `CapabilityConfig::Add`. Consumers request `GeometryLibrarySlot()`. The official factory supplies one `GeometryLibrary` per requesting scope. It exposes plane signed distance, half-space containment and the closest point on a finite patch. For a patch with half extents (1, 0.5), the point (2, 0, 0) lies on the infinite plane and its nearest patch point is (1, 0, 0). [Installed example](../../examples/install-consumer/AnalyticGeometry.cpp) · [Analytic geometry tests](../../tests/install-consumer/AnalyticShapes.cpp).

The installed-package test checks explicit registration, injection of the selected object, independent factory instances and a configuration-local replacement. [Geometry plugin test](../../tests/install-consumer/GeometryPlugin.cpp)
