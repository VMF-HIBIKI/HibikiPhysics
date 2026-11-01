# Project roadmap

**English** | [简体中文](roadmap.zh-CN.md)

## Release objective

v0.1.0 targets a runnable, extensible C++20 CPU research release delivered as a static library, connecting scene compilation, multi-World execution, default rigid-body solving and local viewing. Eight effective development weeks begin at implementation kickoff, using the existing Windows environment as the baseline.

## Priorities and scope

1. C++ runtime skeleton, open plugins, scene compilation and independent World state.
2. Free rigid bodies, incremental potentials, contact and zero-slip static friction.
3. Articulated trees, basic joints, simple loops and contact coupling.
4. Local Python operation, an external viewer and reproducible experiments.

Broad shape coverage remains the long-term direction. Necessary sphere, plane and box combinations come first, followed by convex geometry and further shapes as progress allows. Warp simulation, CCD, differentiability, concrete sensors, the SDF approach and full joint/robot-format coverage remain subsequent research.

## Milestones

| Milestone | Target | Delivery | Official development and research records |
| --- | --- | --- | --- |
| M1: Runtime skeleton | Week 1 | Scene configuration/compilation, plugin lifecycle, single-World/synchronous batches, external plugin and diagnostic examples | Lifecycle, isolation and fixed compiled configuration |
| M2: Free rigid bodies | Week 2 | Math/state, inertial potentials, Newton/line search and force inputs | Free fall/rotation, derivative and step-size studies |
| M3: Contact and static friction | Weeks 3–4 | Initial collision pairs, normal potentials, semismooth Newton, contact redundancy and diagnostics | Static loading, sliding, inclines, simple stacks and failures |
| M4: Multibodies and joints | Weeks 5–6 | Trees, fixed/revolute/prismatic connections, loops, force/torque drives and intermediate parsing | Pendulums, serial mechanisms, four-bar linkage, mechanism contact and batches |
| M5: Usage and research delivery | Weeks 7–8 | Python, CLI, one format subset, external viewer, partial reset, snapshots, error-handling example and integration fixes | Same-build restoration, reproduction, actual coverage and known issues |

The [detailed eight-week plan](plans/0002-cpu-research-release.md) lists deliverables, optional additions and pending decisions. A week-4 review adjusts later scope using contact/friction research progress. Multi-World execution and plugin extension run throughout the plan; users define application acceptance criteria.

## Current implementation plan

The [Windows environment implementation plan](plans/0001-windows-environment.md) details the engineering-foundation work. The initial delivery uses a static C++ library, Windows-first environment preparation and independent small acceptance cases. Tasks ENV-001 through ENV-006 cover provenance, doctor, build sources, preparation, editor integration and verification. The Windows scope has passed [acceptance](environment/acceptance.md).

## Initial backlog

The identifiers below are local planning references. Remote issues can link back to these entries after hosting is configured.

- [x] HP-001: CPU research scope and representative experiments are recorded in the eight-week plan; Windows is the initial platform.
- [x] HP-002: CPU and Warp responsibilities are recorded in ADR-0018.
- [x] HP-003: Establish CMake configuration, build, tests, installation and a consumer example.
- [ ] HP-004: Formatting and initial CI cover the Windows clang-cl baseline, MSVC compatibility checks, and the subsequently specified Linux/macOS baselines.
- [ ] HP-005: Define units, coordinate system, rotation representation and contact-normal conventions.
- [ ] HP-006: Design body handles, lifetime rules and state storage.
- [ ] HP-007: Implement free-motion integration with analytical validation.
- [ ] HP-008: Implement the first contact-generation and response scenario.
- [ ] HP-009: Establish a reproducible benchmark and environment report.
- [ ] HP-010: Publish the first development log and milestone demonstration.
- [ ] HP-011: Apache-2.0 is selected; review third-party attribution for the actual release contents before public release.
- [x] HP-012: Toolchain and dependency selections are recorded in [ADR-0001](decisions/0001-development-toolchain-and-dependencies.md); installation and compatibility validation are tracked separately in that record.

## Completion and change control

Each milestone has a review of acceptance evidence, remaining risks and known limitations. Scope adjustments record the reason, affected deliverables and revised milestone placement.

The roadmap owns release scope. Issues own task acceptance criteria. ADRs own architecture decisions. Pull requests own change-specific evidence. Development logs explain the investigation and learning behind the work.
