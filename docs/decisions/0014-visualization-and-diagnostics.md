# ADR-0014: Visualization and diagnostics

**English** | [简体中文](0014-visualization-and-diagnostics.zh-CN.md)

Status: Accepted

Date: 2026-11-01

Related work: [Plugin architecture](0010-plugin-architecture-and-lifecycle.md), [multi-World simulation](0011-multi-world-batch-simulation.md), [usage and runtime modes](0012-usage-and-runtime-modes.md), [scenes and assets](0013-scenes-and-assets.md).

## Context

Local scene inspection and multi-World experiments require physical debugging, solver diagnostics and reproducible failure analysis. Rendering is provided by external renderers or host applications. HibikiPhysics supplies extensible integration and renderer-independent diagnostic capabilities.

## Alternatives

| Approach | Trade-off |
| --- | --- |
| Engine-owned renderer and embedded debugging UI | Direct integration; adds graphics implementation and couples viewing to a particular renderer. |
| External rendering with pluggable adapters and independent diagnostics | Selected approach; supports different host applications and headless analysis through explicit data and interaction contracts. |

## Decision

### Rendering boundary

External renderers own graphics rendering. HibikiPhysics provides scene and geometry references, body transforms, World identity and diagnostic information. Rendering adapters map these data to the selected renderer's resources and update operations. The renderer or host application owns windows, cameras, materials, lighting, drawing and UI.

Users can develop adapters for arbitrary renderers through public extension mechanisms. Actual compatibility is established by each adapter's declared capabilities. The official local viewing application is a reference integration built on an external renderer. The rendering-engine choice, exchange protocol and synchronization mechanism remain undecided.

Render assets and collision representations may differ and are associated through stable identities and local transforms. Physical diagnostics can describe contact points, force arrows, joint axes and other debug content; adapters determine their presentation. Custom plugins can supply their own diagnostic and presentation capabilities.

### Local inspection and interaction

The reference viewing workflow covers camera navigation, object selection, scene/link hierarchy, visibility controls, visual/collision geometry inspection and physical debug overlays. Relevant overlays include bounds, samples, contact normals/forces, joint frames/axes/limits, loop connections, reaction forces and constraint errors.

Simulation controls include run, pause, single-step and reset. Runtime interaction uses inputs explicitly enabled by the compiled configuration under [ADR-0019](0019-world-configuration-and-runtime-inputs.md). Applied forces/torques, drive targets and mouse-applied forces are candidate inputs for subsequent feature design. Plugins declare input validation and effective timing. Model-parameter editing belongs to configuration and a new scene-compilation cycle. Reproducible runs can record the applied commands and their simulation-time ordering.

### Multi-World observation

Viewing supports one-World focus, selected-World layouts and anomaly-based World selection. Display offsets affect presentation only. Intervention and pause commands explicitly identify their target scope.

Displayed World count, observation frequency and diagnostic detail are configurable independently of physics advancement. Viewing consumes consistent snapshots or declared read views. Batch summaries help locate a failed or unusual World for detailed inspection. Scheduling and the consistency mechanism remain subsequent design work.

### Diagnostic capabilities

Diagnostics run independently of rendering and are available to command-line, Python, analysis and viewing consumers.

| Level | Content and use |
| --- | --- |
| Runtime summary | Per-World outcomes, timing, contact counts, iteration counts and failure reasons for batch monitoring. |
| Physical inspection | Poses, velocities, contact and joint forces, closure errors, sticking drift and model-specific physical measures. |
| Solver trace | Residual history, line-search behavior, rank diagnostics, constraint-state transitions and detailed stopping reasons. |

Plugins can define metrics, events, debug data and specialized analysis or display consumers. They declare meaning, units, sampling time, World association and applicability. A particular solver's diagnostic vocabulary belongs to its plugin protocol under ADR-0010.

Collection is selectable by World, metric, time interval and detail, with explicit sampling and capacity controls. Runtime summaries support ongoing monitoring; detailed traces are enabled on demand. Export and automated analysis are available uses of the same diagnostic information. Capture limits and any truncation are reported so that retained evidence can be interpreted correctly.

### Failure analysis and replay

Failure records associate the World, model/plugin composition, parameters and stopping reason with available history and restoration data. Capture can retain a bounded interval around an anomaly and supported state snapshots.

Trajectory playback records selected poses, display data and diagnostics. Resumable snapshots contain the compatible model/configuration references and authoritative plugin state required to continue simulation. Their distinct guarantees follow ADR-0012 and ADR-0013. Storage format, recording frequency and trigger implementation remain undecided.

### Viewer and sensor boundary

The viewer serves human observation and interaction. Visual sensors serve application observations and can connect to external rendering through adapters. Sensor coverage, sampling and delivery requirements receive a separate decision. Local viewing and headless physical diagnostics guide the current delivery.

## Consequences

The engine can support local inspection and headless batch analysis while external renderers provide graphics. Adapter development and explicit snapshot/command contracts are required. Detailed collection has configurable performance and memory cost, which is measured during validation.

Concrete interfaces, diagnostic schemas, event transport, file formats, storage tools, UI layout, reference renderer and rendering synchronization remain undecided. This record establishes accepted boundaries and capabilities; implementation remains pending.

## Validation

Validation status: Pending implementation and visualization/diagnostic acceptance.

- Connect an external renderer through an adapter and validate geometry identities, transforms and debug presentation.
- Collect and export physical and solver diagnostics during headless runs.
- Inspect selected Worlds and verify presentation-only offsets, state isolation and explicit intervention scope.
- Add custom diagnostics and presentation from an independent plugin.
- Check sampling, capacity/truncation reporting and the overhead of enabled collection levels.
- Capture a failing World, locate its configuration and stopping reason, and exercise the declared playback or restoration path.
- Validate safe interactive commands and their ordering. Related cases share parameterized workflows to control test-file count.
