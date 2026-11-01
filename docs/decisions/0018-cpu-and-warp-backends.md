# ADR-0018: CPU and Warp backend responsibilities

**English** | [简体中文](0018-cpu-and-warp-backends.zh-CN.md)

Status: Accepted

Date: 2026-11-01

Related work: [Default research configuration](0009-incremental-potential-rigid-contact.md), [plugins](0010-plugin-architecture-and-lifecycle.md), [multi-World simulation](0011-multi-world-batch-simulation.md), [official experiments](0015-experiments-and-acceptance.md).

## Context

The initial research path prioritizes CPU simulation, algorithm development and debugging. Warp provides an optional path for parallel computation. Plugin extensibility requires explicit backend capabilities and compatible compositions.

## Alternatives

| Approach | Trade-off |
| --- | --- |
| Deliver matching algorithms and features on every backend together | Couples development schedules and implementation choices across backends. |
| Develop the CPU research path first and extend backend coverage through plugins | Selected approach; supports incremental implementation and explicit capability coverage. |

## Decision

### Responsibilities and coverage

CPU provides the initial complete research path for simulation, debugging and correctness experiments, and remains a maintained backend. Warp is an optional computing backend whose integration order follows experimental value and suitability for batch parallelism.

A backend determines where computation executes; a solver determines the algorithm. Solver plugins may support one or several backends. The default research configuration is implemented on CPU first. Other compositions may supply different algorithms and feature coverage.

Backends share scene and model semantics for bodies, joints and materials. Each backend builds runtime data suited to its execution needs. Capability declarations describe the supported semantics and configurations.

### Composition and multi-World execution

Users manage backend and plugin combinations. Selected components define their supported execution conditions and perform their own necessary checks during scene compilation. They report capability gaps through their chosen diagnostic mechanisms. [ADR-0010](0010-plugin-architecture-and-lifecycle.md) defines this ownership boundary.

Batch execution groups Worlds by model, plugin composition and backend capabilities, while preserving independent World state. Initial backend selection occurs when creating a runtime instance. Migration during execution remains a subsequent extension.

Compositions spanning CPU and GPU explicitly describe data exchange and synchronization requirements. Performance accounting includes their costs. Concrete composition and scheduling mechanisms remain pending.

### Diagnostics and evaluation ownership

The project provides diagnostic data and extension mechanisms. Users determine application acceptance metrics, thresholds, references and evaluation workflows. Compatibility checks and solver outcome reporting retain their runtime roles.

Official experiments comparing the same model and algorithm examine states, residuals, invariants and numerical errors under recorded precision and execution conditions. Experiments comparing different algorithms select suitable measures of physical behavior, solution quality, stability and cost. Bitwise-reproduction claims specify their applicable configurations. Performance reports include solution-quality evidence. These practices apply to official experiments as described in ADR-0015.

### Deferred details

Device interfaces, data layouts, transfers, scheduling, migration mechanisms and specific Warp implementation milestones remain pending.

## Consequences

The CPU research path can progress independently while Warp and user-defined backends extend the available configurations. Explicit capability declarations make composition limits visible. Each backend owns its runtime representation and implementation costs.

## Validation

Validation status: Pending implementation and backend experiments.

Project development checks will examine capability diagnostics, compatible compositions and World isolation. Official backend experiments will record their own evaluation criteria and evidence.
