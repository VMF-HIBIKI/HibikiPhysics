# ADR-0016: Time advancement

**English** | [简体中文](0016-time-advancement.zh-CN.md)

Status: Accepted

Date: 2026-11-01

Related work: [Plugin architecture](0010-plugin-architecture-and-lifecycle.md), [multi-World simulation](0011-multi-world-batch-simulation.md), [runtime modes](0012-usage-and-runtime-modes.md).

## Context

Local interaction and batched simulation need explicit simulation-time and state-commit semantics. Simulation time, advancement policy and execution scheduling have separate responsibilities.

## Alternatives

| Approach | Trade-off |
| --- | --- |
| A fixed global loop and clock | Simple coordination; ties different Worlds and algorithms to one advancement policy. |
| Pluggable advancement with per-World time and declared execution semantics | Selected approach; supports default batch operation and extensible asynchronous or independent advancement. |

## Decision

The state/time and failure-reporting behavior below describes the official reference flow. Users can define their own advancement, state validity, failure propagation and recovery protocols under [ADR-0010](0010-plugin-architecture-and-lifecycle.md). Multi-World isolation and fixed compiled configuration remain the boundaries of the selected runtime.

Time advancement is replaceable through plugins. A time-advancement implementation composes per-World simulation clocks, step/substep and target-time policies, solve/retry/commit behavior, pause and single-step control. Execution services may provide task submission, parallel scheduling and completion reporting; a plugin can also supply a complete combined flow.

The default provides synchronous batch interaction semantics with parallel multi-World execution internally. Architecture supports asynchronous submission and independent World advancement. A batch policy explicitly declares whether selected Worlds reach a shared target time or independently reported times.

Simulation time advances with accepted state. Trial solves and retries retain defined commit boundaries. Every World owns its time and advancement state. Controls declare when they take effect; results identify their World, simulation time and advancement request.

Reset, rollback and restoration coordinate outstanding work and restore time together with physical and plugin history. A failed advance reports its accepted progress and stopping reason according to the selected policy. Pending work and external interactions follow the owning flow's contracts.

Wall-clock pacing is a runtime policy: headless experiments can advance as fast as resources allow, and interactive applications can request a playback pace. Simulation-time semantics remain explicit under either policy.

Concrete time representations, APIs, default step/substep selection, retry algorithms, task scheduling, synchronization and clock serialization remain undecided. Users can extend or replace protocols and flows under ADR-0010.

## Consequences

Synchronous and asynchronous entry points can share physical advancement rules. Independent clocks, input timing and result identity require explicit coordination, especially during partial reset or restoration. The accepted scope is behavior and extensibility; implementation remains pending.

## Validation

Validation status: Pending implementation.

- Check that committed time and physical/plugin state advance consistently through successful steps and retries.
- Verify pause, single-step, reset and rollback under the selected batch policy.
- Compare equivalent synchronous and asynchronous runs under documented reproducibility conditions.
- Validate target-time reporting, control application order and isolation of Worlds with different completion times.
- Exercise outstanding work during reset/restoration and verify result identity and state ownership.
