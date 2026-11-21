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
| Basic analytic geometry | Sphere, box, capsule, cylinder, ellipsoid, plane or half-space |
| Extended analytic geometry | Cone, conical frustum, tapered capsule, finite rectangular patch |
| Discrete and convex geometry | Triangle, convex hull, triangle mesh |
| Terrain and fields | Height field, signed distance field (SDF) |
| Instances and composition | Geometry with a local pose, compound geometry, shape-specific scaling rules |

The analytic geometry task will settle plane boundary and interior semantics, box half or full extents, capsule cylinder length or end-to-end length, and local axes. The SDF representation and generation method remain research decisions.

## Registration and validation

An application linking `hibiki::physics` includes `<HibikiPhysics/Geometry.h>` and registers `GeometryLibraryPlugin()` with `CapabilityConfig::Add`. Consumers request `GeometryLibrarySlot()`. The official factory supplies one `GeometryLibrary` per requesting scope. Its current public role is the service identity for the module. The first shape definitions and operations are a separate deliverable.

The installed-package test checks explicit registration, injection of the selected object, independent factory instances and a configuration-local replacement. [Geometry plugin test](../../tests/install-consumer/GeometryPlugin.cpp)
