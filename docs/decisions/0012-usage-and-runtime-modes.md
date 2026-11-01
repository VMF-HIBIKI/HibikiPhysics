# ADR-0012: Usage and runtime modes

**English** | [简体中文](0012-usage-and-runtime-modes.zh-CN.md)

Status: Accepted

Date: 2026-11-01

Related work: [Plugin architecture and lifecycle](0010-plugin-architecture-and-lifecycle.md), [multi-World batch simulation](0011-multi-world-batch-simulation.md).

## Context

HibikiPhysics serves physics research and provides simulation capabilities for embodied-intelligence applications. The current product scope covers the engine, local scene construction and inspection, and reproducible simulation experiments. A future Lab project may build training workflows on these capabilities.

## Alternatives

| Approach | Trade-off |
| --- | --- |
| Application-specific entry points with separate behavior | Local customization is straightforward; maintaining common scene and simulation semantics requires extra coordination. |
| Shared simulation semantics with specialized entry points | Selected approach; C++, Python, command-line and visualization serve distinct users while using compatible plugin compositions and model/state semantics. |
| Engine and integrated training Lab | Broader application coverage with additional task, reward, curriculum and training-framework responsibilities. |

## Decision

### Entry-point roles

| Entry point | Role |
| --- | --- |
| C++ | Independently usable engine and extension foundation for native integration, algorithm research and plugin development; supports individual and batched Worlds. |
| Python | First-class local scripting entry for creating/loading scenes, configuring models, controlling simulation, opening the viewer, inspecting state and exporting/analyzing experiments. Batch simulation capabilities can be exposed as engine operations. |
| Command line | Reproducible headless runs, regression scenarios, parameter studies and performance measurements from recorded configuration. |
| Visualization | Optional observation and interaction, including bodies, joints, contacts, forces and solver diagnostics, with selection of individual Worlds or subsets. |

All entry points use scene meanings and configuration/result protocols defined by the selected composition. Numerical behavior is defined by the selected composition. Native C++ applications can use the engine independently; Python and GUI integration are optional capabilities of a composition. Equivalent runs record any differences in backend or configuration.

### Python and Lab boundary

The short-term Python deliverable supports local scene construction, simulation control and viewing. Research scripts may perform batch runs, partial resets and pre-compilation parameter variation through supported engine capabilities.

Training tasks, reward definitions, episode-management frameworks, curricula, RL algorithm integration and distributed training orchestration belong to a higher-level application. A future separate Lab project may provide them through public engine and plugin contracts. The current Python deliverable is evaluated against local simulation workflows.

Multi-World isolation, partial reset and per-instance parameters remain core engine requirements under ADR-0011. Future Lab applications can build on those primitives and define their own action, observation and task semantics.

### Runtime modes and priority

| Mode | Purpose and priority |
| --- | --- |
| Headless multi-World batch | Primary delivery and performance path for simulation experiments and future embodied applications. Execution advances simulation time under the selected stepping policy. |
| Interactive research | Core development workflow for a scene or a small selection of Worlds, with pause, single-step, reset and controlled interaction. |
| Batch observation | Inspect selected Worlds and sampled diagnostics from a batch; observation frequency is configured separately from physics advancement. |
| Replay and inspection | Analyze recorded trajectories or restore supported snapshots. Playback data and resumable simulation state have explicit, distinct guarantees. |

Viewing and control respect declared snapshot/read rules and declared runtime-input and reset boundaries. Visual layout offsets retain the per-World physics semantics from ADR-0011. [ADR-0016](0016-time-advancement.md) establishes default synchronous batch interaction and extensible asynchronous advancement; concrete scheduling remains pending.

### Viewer and visual observations

The viewer serves human inspection and interaction. Visual sensors produce observations for applications. Both are extensible capabilities with separately defined requirements. Headless execution can host offscreen observation providers when the selected composition supports them.

[ADR-0014](0014-visualization-and-diagnostics.md) assigns rendering to external renderers, defines plugin-based integration and establishes renderer-independent diagnostics. The local viewer is a reference application using an external renderer.

Rendering backend, image modalities, fidelity and offscreen-sensor delivery priority remain undecided. Local viewing is part of the short-term Python workflow; the application-facing visual-sensor scope requires further discussion.

### Reproducible operation

Command-line and scripted runs record scene/model identities, plugin implementations and versions, parameters, randomization configuration, execution backend, stepping policy, per-World outcomes and failure diagnostics. Output supports subsequent analysis, playback or restoration according to the capabilities actually recorded.

Applications can choose C++, Python or command-line entry points for an equivalent simulation configuration. The viewer consumes the selected simulation state and diagnostics through public contracts.

## Consequences

Headless batch capability and local interactive investigation guide the initial delivery. C++ remains the independent engine foundation, and Python makes local construction and inspection accessible. Lab responsibilities have a separate application boundary while the engine retains the batch capabilities required for future integration.

Concrete C++/Python interfaces, binding libraries, tensor/array interchange, command syntax, scene file format, viewer implementation, rendering backend and replay storage format remain undecided. Implementation is subsequent work.

## Validation

Validation status: Pending implementation and workflow acceptance.

- Build and run a native headless simulation through the C++ engine.
- Construct a local scene in Python, advance and reset it, view it interactively, inspect state and export results.
- Run a recorded multi-World configuration headlessly, including partial reset and pre-compilation parameter variation.
- Compare equivalent entry-point runs under the same composition and documented numerical criteria.
- Observe selected Worlds and validate safe interaction, state isolation and presentation-only offsets.
- Verify recorded outcomes and declared playback/restoration capabilities. Group related workflow cases to control test-file count.
