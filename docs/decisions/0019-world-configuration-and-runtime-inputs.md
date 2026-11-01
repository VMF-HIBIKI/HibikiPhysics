# ADR-0019: World configuration and runtime inputs

**English** | [简体中文](0019-world-configuration-and-runtime-inputs.zh-CN.md)

Status: Accepted

Date: 2026-11-01

Related work: [Multi-World simulation](0011-multi-world-batch-simulation.md), [visualization](0014-visualization-and-diagnostics.md), [snapshots](0017-snapshots-and-restoration.md).

## Context

Batch simulation needs per-World parameter variation and domain randomization. Stable runtime models provide a defined basis for derived data, plugin execution and restoration. Interactive applications also need controlled input channels.

## Alternatives

| Approach | Trade-off |
| --- | --- |
| Runtime model editing with dependency updates | Requires mutation, invalidation and reconstruction contracts during execution. |
| Pre-compilation configuration and explicit runtime inputs | Selected approach; fixes model data for each compiled instance and defines interaction through declared inputs. |

## Decision

### Lifecycle and ownership

A World follows configuration, scene compilation and execution. Scene compilation builds executable simulation instances from scenes, parameters and plugin configuration. Source-code compilation remains a separate development operation.

Configuration defines structure, per-World parameters, initial state, plugin composition and domain-randomization rules. Scene compilation fixes model structure and physical parameters, performs component-defined checks and prepares collision, dynamics and solver data. Compatible immutable assets and model data can be shared; each World owns its evolving simulation state and plugin history.

Compiled model structure and parameters remain fixed throughout execution, including pauses and resets. Changes enter a new configuration and scene-compilation cycle. Concrete rebuilding, resource reuse and state-transfer mechanisms remain pending.

### Domain randomization

Users can assign or sample different parameters for each World before scene compilation. Plugins declare supported parameters and valid ranges. Compilation prepares derived data for the actual values of each World. Sampling provenance, seeds and resulting values support reproduction.

Model-parameter resampling, including mass, inertia and friction changes, starts a new configuration and compilation cycle. Partial reset retains compiled parameters and restores the configured initial state and plugin state under the reset policy. Other Worlds preserve their state.

### Runtime inputs

Execution permits state advancement, result reads and inputs explicitly enabled by the compiled configuration. Plugins declare input semantics, allowed ranges, validation and effective timing. Candidate inputs include actuation, control targets and future mouse-applied external forces. Concrete input coverage is determined during the corresponding feature design.

Viewer interaction uses these same declared channels. Model edits belong to the configuration phase. Input channels operate within the fixed model and its declared capabilities. Run records can identify applied inputs and their simulation times.

### Restoration and scope

Snapshots identify compiled configuration and per-World parameter values. Restoration uses a compatible compiled configuration. A branch with changed model parameters or structure goes through configuration and compilation with an explicit state-transfer policy.

This record updates earlier descriptions of parameter overrides, reset-time randomization, viewer parameter editing and runtime mechanism attachment. Per-World overrides mean pre-compilation configuration. Interfaces, data layouts, compilation algorithms and the concrete runtime-input list remain pending.

## Consequences

Domain randomization supports heterogeneous parameter sets within a batch. Runtime derived data is tied to a stable compiled configuration. Parameter studies account for configuration and compilation costs. User-defined plugins retain extensibility through configuration data, compilation behavior and declared runtime inputs.

## Validation

Validation status: Pending implementation.

Development checks will cover distinct per-World compiled parameters, consistent derived data, reset-time parameter preservation, input validation and compatible snapshot restoration. Model-edit requests during execution produce a diagnostic directing callers to configuration and compilation. Official experiments select their own evaluation criteria; users define application acceptance under ADR-0015.
