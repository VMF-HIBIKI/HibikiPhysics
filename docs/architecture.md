# Architecture direction

**English** | [简体中文](architecture.zh-CN.md)

Status: Proposed baseline for the first implementation milestone.

## Core boundary

The C++20 library owns the production CPU simulation pipeline. It supports headless execution and an independent C++ build. Examples and the sandbox consume its public API. Warp experiments use a separate optional environment and explicitly defined comparison data.

## Modules

| Module | Responsibility |
| --- | --- |
| Core | Handles, storage, resource lifetime and diagnostics |
| Math | Mathematical operations required by the engine |
| Geometry | Shapes, bounds and geometric queries |
| Collision | Broad-phase candidate generation and narrow-phase contact generation |
| Dynamics | Body state, applied forces and integration |
| Constraints | Contact and future joint constraint definitions |
| Solver | Constraint solution and iteration diagnostics |
| World | Simulation lifecycle and stage orchestration |

Lower-level modules expose explicit inputs and outputs. World composes the simulation stages. Rendering and experiment code depend on engine interfaces.

## Initial design principles

- World manages body and shape lifetimes.
- Public handles carry generation information to support stale-handle detection.
- Internal storage layout remains encapsulated behind public interfaces.
- Persistent state and per-step scratch data have distinct lifetimes.
- Simulation stages document their inputs, outputs and synchronization requirements.
- Diagnostics expose relevant counts, iteration statistics and timing data.
- Numerical conventions and tolerances accompany physics behavior.

These principles will be refined through ADRs and small experiments. Build-target separation, data layout and parallel execution will evolve with measured needs.

## Planned build targets

- `hibiki::physics`: public C++ static library target for the initial delivery.
- `hibiki_tests`: automated correctness checks.
- `hibiki_benchmarks`: reproducible performance measurements.
- Example and sandbox executables: API usage and inspection.

The current repository provides the directory structure. Working targets and commands will be introduced by the engineering-foundation milestone.

The [Windows environment implementation plan](plans/0001-windows-environment.md) defines the initial static-library package and independent toolchain cases. Shared-library support is a subsequent extension.
