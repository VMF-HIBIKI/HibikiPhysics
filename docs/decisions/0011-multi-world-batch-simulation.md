# ADR-0011: Multi-World batch simulation

**English** | [简体中文](0011-multi-world-batch-simulation.zh-CN.md)

Status: Accepted

Date: 2026-11-01

Related work: [Default rigid-body research configuration](0009-incremental-potential-rigid-contact.md), [plugin architecture and lifecycle](0010-plugin-architecture-and-lifecycle.md).

## Context

Embodied-intelligence applications require many independent simulation instances for training, evaluation, system identification and parameter studies. Multi-World batch simulation is a core architecture and delivery requirement. MotrixSim's separation of shared model data from independent simulation data provides a reference for this organization.

A World is a logically isolated simulation instance. A robot and other bodies interacting inside one World form one physical scene. Cross-World batching groups independent instances for execution.

## Alternatives

| Approach | Trade-off |
| --- | --- |
| Duplicate a complete model for every instance | Straightforward ownership; repeated topology and assets increase memory and setup cost. |
| Share a model and maintain independent instance state | Selected default batch organization; enables model reuse with explicit isolation and parameter overrides. |
| Batch heterogeneous models and configurations | Supported architectural direction; compatible groups can share execution strategies, with separate performance characterization. |

## Decision

### Shared model and independent state

Compatible Worlds share read-only geometry assets, mechanism topology and default parameters. Each World owns its mutable motion state, controls, sensor state, contact and friction history, integration history, solver state, random state, simulation time and diagnostics. Per-World model parameters are fixed during scene compilation under [ADR-0019](0019-world-configuration-and-runtime-inputs.md). Mutable scratch storage follows explicit ownership or synchronization rules.

Physical interactions remain within a World. Rendering offsets affect presentation only. Shared services and caches declare their concurrency and lifetime contracts. The logical model/state distinction permits plugin-defined storage under ADR-0010.

### Batch scope and application boundary

Homogeneous topology with independent state is the primary performance path. Per-World parameters sampled before scene compilation support domain randomization on that path. Different models or plugin compositions can coexist in one process and be scheduled in compatible groups.

The core workflow supports batch control submission, simulation advancement, state/observation reads and selection of Worlds by identity, index or mask. Tasks, rewards, episode termination and curricula belong to higher-level applications, potentially provided by a future separate Lab project. Their interaction with stepping and resets has explicit ordering semantics. [ADR-0012](0012-usage-and-runtime-modes.md) defines short-term Python support for local scene construction, simulation and viewing, alongside headless batch engine capabilities.

CPU multi-World parallel execution is a required delivery capability. Warp remains an extensible backend and acceleration research direction. Plugin and batch contracts account for backend requirements; actual backend coverage is declared by each composition.

### Partial reset and recovery

Selected Worlds can reset independently while the states of unselected Worlds are preserved. Reset covers all participating plugin state according to a declared reset policy, including contact history, friction variables, integrator history, controls, sensors, clocks, solver caches and randomization state. Reset retains compiled model parameters and restores the configured initial state. Random-stream restoration follows the declared reset policy.

Reset and explicitly supported runtime inputs take effect at defined safe execution boundaries. Observations of a completed or terminated step remain distinguishable from observations after reset. Automatic-reset behavior is declared by the application composition.

The official example flow reports isolatable numerical failures per World and demonstrates retaining the last accepted state under its rollback policy. The caller chooses subsequent actions. Users define failure propagation, state validity and recovery in their own flows under [ADR-0010](0010-plugin-architecture-and-lifecycle.md). Process/device failure scope follows the selected backend and flow.

### Domain randomization

Supported parameters are configured per World before scene compilation while shared model defaults remain read-only. Initial-state configuration, mass, inertia, friction and actuator parameters are target categories, with actual support and admissible ranges declared by the responsible plugin. Scene compilation fixes these values and prepares the corresponding derived data.

Model-parameter resampling and structural changes enter a new configuration and scene-compilation cycle. Randomization distributions and sampling policies are extensible. Per-World sampling provenance and seeds support reproduction independently of worker assignment. Runtime interaction uses inputs explicitly enabled by the compiled configuration.

### Plugin parallel isolation

Plugins declare instance scope, shared resources, concurrent access rules and supported execution modes. Stateful simulation data belongs to the corresponding World. Cross-World sharing uses explicit immutable or synchronized resources.

The official default composition must pass multi-World isolation, partial-reset and parallel-execution validation. User plugins can provide single-instance execution under compatible scheduling or specialized batch execution. Users manage composition and scheduling; selected components define checks for their supported concurrent use.

Plugin lifecycle, reset, rollback and teardown coordinate outstanding work before releasing or modifying state. Scheduling and storage decisions preserve the open protocol, data and flow extension principles of ADR-0010.

### Reproduction and performance

Run records identify model and asset versions, plugin composition, compiled per-World parameters, per-World random-state provenance, stepping/reset sequence and execution policy. Official experiments state reproduction claims for their configurations and backends and evaluate cross-platform/backend comparisons under their own criteria. Users define application acceptance under ADR-0015.

Performance evaluation includes batch throughput, core scaling, per-World memory, setup/reset cost, uneven contact and solver workloads, and transfer cost to the training application. World-count and throughput targets are set using representative robots and measured hardware.

## Consequences

Isolation, partial reset and parameter variation influence architecture from the beginning. Sharing immutable models reduces repeated data while independent histories and compiled per-World parameters require explicit ownership. Variable solver work across Worlds requires performance evaluation of the eventual scheduling policy.

Concrete C++/Python interfaces, batch memory layout, thread pools, scheduling algorithms, synchronization mechanisms and device transfer implementation remain undecided. [ADR-0016](0016-time-advancement.md) defines synchronous batch interaction as the default and supports extensible asynchronous advancement. This record establishes capability and behavioral requirements. Implementation remains pending.

## Validation

Validation status: Pending implementation and batch experiments.

- Compare a World run alone and in a batch under identical model, inputs and random state, using the declared numerical reproducibility criteria.
- Reset a selected subset and verify every relevant plugin state plus preservation of unselected Worlds.
- Compile shared-model instances with different per-World parameters; verify derived data, model immutability and independent behavior.
- Repeat pre-compilation sampling under different worker assignments, and verify that partial resets preserve compiled parameters.
- Exercise recoverable per-World solver failures, rollback and subsequent reset or retry.
- Run different models and plugin compositions concurrently and validate resource lifetime and parallel access declarations.
- Measure throughput, memory, reset overhead, load imbalance and training-side data transfer. Related cases use parameterized tests to control test-file count.

## References

- [MotrixSim simulation data](https://motrixsim.readthedocs.io/en/latest/user_guide/main_function/scene_data.html)
- [MotrixSim multi-environment simulation](https://motrixsim.readthedocs.io/en/v0.9.0/user_guide/main_function/parallel.html)
- [MotrixSim batch data and model API](https://motrixsim.readthedocs.io/en/latest/api_reference/core/motrixsim.html)
