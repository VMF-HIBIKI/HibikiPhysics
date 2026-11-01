# ADR-0017: Snapshots and restoration

**English** | [简体中文](0017-snapshots-and-restoration.zh-CN.md)

Status: Accepted

Date: 2026-11-01

Related work: [Plugin lifecycle](0010-plugin-architecture-and-lifecycle.md), [multi-World simulation](0011-multi-world-batch-simulation.md), [scene artifacts](0013-scenes-and-assets.md), [time advancement](0016-time-advancement.md).

## Context

Simulation snapshots support failure investigation, resumed execution and branching experiments. A snapshot describes a consistent accepted state and the dependencies required to interpret and continue it.

## Alternatives

| Approach | Trade-off |
| --- | --- |
| Save visible motion data | Useful for trajectory playback; continuation depends on additional solver and plugin state. |
| Save continuation state with model/plugin identity | Selected approach; captures necessary physical, temporal and plugin history with declared compatibility. |

## Decision

### Saved state

Snapshots identify the scene/model, assets, physical parameters, plugin implementations/versions and execution configuration. They save positions, orientations, velocities, joint state, compiled per-World parameter identities and values and continuation-relevant contact, friction, integrator and solver history.

Time, control inputs, random state and the execution position of accepted commands are included when they affect continuation. Each plugin declares required persistent state, reconstructible data and temporary workspace. Classification follows influence on future behavior; warm-start data may affect finite-iteration results and requires explicit treatment.

Models and assets can be referenced by stable identity. Self-contained packaging, storage format, compression and incremental saving remain subsequent choices.

### Recovery scope

Supported scenarios include failure rollback, save-and-resume, independent branches from one snapshot, and restoration of selected Worlds. Unselected Worlds preserve their state. A branch records its source snapshot and new run identity; restoration uses a compatible compiled configuration. Parameter changes enter a new configuration and scene-compilation cycle under [ADR-0019](0019-world-configuration-and-runtime-inputs.md), with explicit state-transfer and history policies.

Independent World snapshots record each instance's own simulation time. Coordinated batch capture follows a declared common boundary. Shared application state, when involved, requires its own consistency contract. Inspection-only loading declares which data are available and which continuation capabilities are present.

### Consistent capture and asynchronous execution

Capture uses an accepted-state boundary. Outstanding trials and device work are completed, cancelled or isolated according to the execution policy before a stable snapshot is formed. Concrete waiting and copying mechanisms remain undecided.

Restoration coordinates ownership of state writes with outstanding tasks. Restored runs have an identity that distinguishes obsolete asynchronous results. Input history identifies applied and accepted-pending commands so replay has an explicit execution position. Threads, device pointers and task handles are reconstructed runtime resources; serializable command meaning and external-resource recovery are plugin-declared capabilities.

### Plugin compatibility

The compatibility and failure behavior below describes the official restoration workflow. Users manage plugin versions and dependencies; the selected restoration components define checks, migrations and failure handling under [ADR-0010](0010-plugin-architecture-and-lifecycle.md).

Each stateful plugin declares its snapshot boundary, saved-state version, loading/validation, reconstruction and supported migration behavior. Capture checks participation capabilities before producing a complete snapshot. Restoration validates dependencies and data, constructs and checks the recovered state, then publishes a runnable World.

| Situation | Behavior |
| --- | --- |
| Matching model and plugin configuration | Restore under the declared continuation guarantee. |
| Explicitly compatible plugin version | Load, validate and record the actual implementation identity. |
| Available migration | Apply an explicit conversion and record source/target versions and resulting guarantee. |
| Missing required plugin or uninterpretable state | Report a blocked continuation restore and preserve the snapshot. |
| Missing presentation capability | Permit supported inspection or physical continuation with explicit capability reporting. |

Model/topology changes require a new configuration and scene-compilation cycle, with an explicit migration or branch policy. Default-state initialization of required history is an explicit reinitialization operation. Version identity, compatibility and successful numerical reproduction are separately validated.

## Consequences

The design supports individual and selected-batch recovery with complete plugin participation. State restoration, numerically reproducible continuation and migrated continuation have distinct declared guarantees. Concrete interfaces, file formats, migration APIs, asynchronous capture, synchronization and performance policies remain undecided. Implementation remains pending.

## Validation

Validation status: Pending implementation.

- Compare uninterrupted execution with saved/restored continuation under the declared reproduction criteria.
- Restore or branch selected Worlds and check other instances remain unchanged.
- Verify plugin history, random state, time and input execution positions.
- Exercise capture/restoration with outstanding work and reject obsolete results by run identity.
- Check missing plugins, compatible upgrades, explicit migrations and recovery failure without loss of the previous valid state.
