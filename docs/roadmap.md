# Project roadmap

**English** | [简体中文](roadmap.zh-CN.md)

## Release objective

The first release, `v0.1.0`, targets a verifiable C++20 rigid-body simulation library with runnable examples, documented behavior and reproducible validation.

The planning horizon is eight development weeks. Week numbers are relative to implementation kickoff. Scope and acceptance evidence determine milestone completion.

## Priorities

1. C++ systems engineering: ownership, public interfaces, build and packaging, diagnostics and maintainability.
2. Physics implementation: explicit conventions, contact generation, numerical integration, constraint solving and verification.
3. GPU exploration: an optional Warp experiment with correctness and performance comparisons.

## Planned scope

| Category | Capabilities |
| --- | --- |
| Release requirements | Rigid-body state and integration, a selected set of primitive shapes, collision detection, contact response, basic friction, examples and automated validation |
| Capacity-dependent additions | Sleeping, simple joints, additional shapes and broad-phase improvements |
| Follow-up areas | Soft bodies, fluids, differentiability, general asset import and advanced editing tools |
| Optional experiment | A bounded Warp implementation of a selected stage with a C++ reference comparison |

Specific shape coverage and solver selection will be recorded before their implementation milestones enter Ready.

## Milestones

| Milestone | Target | Deliverable | Acceptance evidence |
| --- | --- | --- | --- |
| M0: Engineering foundation | Week 1 | Build, tests, installation and architecture baseline | Clean build; minimal installed-library consumer; initial automated checks; ownership and numerical-convention decisions |
| M1: Dynamics loop | Week 2 | Body creation, forces, stepping and state access | Analytical trajectory comparison; integration error study; lifecycle tests; runnable example |
| M2: Contact loop | Weeks 3–4 | Selected primitive collisions and contact response | Separation/touching/penetration tests; contact convention checks; reproducible impact scene; regression coverage |
| M3: Stability and measurement | Weeks 5–6 | Selected friction and stacking scenarios, diagnostics and benchmarks | Scene-specific stability thresholds; numerical-quality report; reproducible performance baseline |
| M4: Release readiness | Weeks 7–8 | Installable `v0.1.0`, examples, documentation and retrospective | Clean-environment reproduction; milestone acceptance review; release notes; documented limitations and attribution |

Warp work becomes eligible after the required M3 outcomes are satisfied. Its experiments have a separate dependency environment and evidence report.

## Current implementation plan

The [Windows environment implementation plan](plans/0001-windows-environment.md) details the engineering-foundation work. The initial delivery uses a static C++ library, Windows-first environment preparation and independent small acceptance cases. Tasks ENV-001 through ENV-006 cover provenance, doctor, build sources, preparation, editor integration and verification. The Windows scope has passed [acceptance](environment/acceptance.md).

## Initial backlog

The identifiers below are local planning references. Remote issues can link back to these entries after hosting is configured.

- [ ] HP-001: Define release scope, target platforms and representative acceptance scenes.
- [ ] HP-002: Record the C++ core and Warp experiment boundary in an ADR.
- [x] HP-003: Establish CMake configuration, build, tests, installation and a consumer example.
- [ ] HP-004: Formatting and initial CI cover the Windows clang-cl baseline, MSVC compatibility checks, and the subsequently specified Linux/macOS baselines.
- [ ] HP-005: Define units, coordinate system, rotation representation and contact-normal conventions.
- [ ] HP-006: Design body handles, lifetime rules and state storage.
- [ ] HP-007: Implement free-motion integration with analytical validation.
- [ ] HP-008: Implement the first contact-generation and response scenario.
- [ ] HP-009: Establish a reproducible benchmark and environment report.
- [ ] HP-010: Publish the first development log and milestone demonstration.
- [ ] HP-011: Select the open-source license and establish third-party attribution records before public release.
- [x] HP-012: Toolchain and dependency selections are recorded in [ADR-0001](decisions/0001-development-toolchain-and-dependencies.md); installation and compatibility validation are tracked separately in that record.

## Completion and change control

Each milestone has a review of acceptance evidence, remaining risks and known limitations. Scope adjustments record the reason, affected deliverables and revised milestone placement.

The roadmap owns release scope. Issues own task acceptance criteria. ADRs own architecture decisions. Pull requests own change-specific evidence. Development logs explain the investigation and learning behind the work.
