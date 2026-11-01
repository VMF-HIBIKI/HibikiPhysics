# ADR-0010: Plugin architecture and lifecycle

**English** | [简体中文](0010-plugin-architecture-and-lifecycle.zh-CN.md)

Status: Accepted

Date: 2026-11-01

Related work: [ADR-0009: Default rigid-body research configuration](0009-incremental-potential-rigid-contact.md).

## Context

HibikiPhysics supports research involving alternative physical models, state representations, numerical methods and execution backends. Users need to replace existing implementations, extend existing systems and introduce capabilities beyond the engine's original vocabulary.

DeepSeek Harness provides an architectural reference for plugin-defined services, dependency management and replaceable execution loops. HibikiPhysics applies these principles to its own C++ runtime and physics requirements.

## Alternatives

| Approach | Trade-off |
| --- | --- |
| Fixed engine with predefined algorithm slots | Simple integration for anticipated alternatives; extension scope follows the slots designed into the engine. |
| Universal mathematical interface | Common assembly vocabulary; its assumptions constrain new formulations and data requirements. |
| Open plugin protocols with a shared lifecycle | Selected approach; users define new capabilities, with interoperability rules defined by the participating users and components. |

## Decision

### Open extension

Physical capabilities are supplied by plugins. Official and user plugins use the same public extension mechanisms. A plugin may provide a small service, a physical model with its state, a complete subsystem or a composition of other plugins.

Users can define and register new protocols, configuration types, model data, runtime state, diagnostics and execution flows in independently developed packages. Registration supports new capability identities while keeping the kernel's built-in type lists stable. Third-party plugins can define a protocol and collaborate through that protocol's shared package.

The extension contract covers three uses: replacing an existing provider, contributing to an existing subsystem, and supplying a new subsystem with its own protocols and execution flow.

### Protocol ownership and composition

Protocols belong to the plugin ecosystems that use them. Users manage plugin identities, versions, dependencies, provider selection, composition and upgrades. Participants define their own interoperability, data ownership and execution rules. Scene compilation executes checks required by the selected components and reports their results. Concrete protocol and checking mechanisms remain open.

Potential, residual, constraint, projection and matrix-free operator interfaces remain candidates for particular algorithm families. Concrete mathematical interfaces, mandatory capability categories and universal model/discretization/solver layering remain undecided. These concepts can organize the default implementation as its interfaces mature through actual extension cases.

Simulation orchestration is itself replaceable. The default prediction/contact/solve/commit sequence belongs to the default time-advancement implementation. Plugins can expose internal extension points or provide a complete alternative flow. Data-access rules, synchronization, ordering and mutation boundaries are explicit parts of each flow's contract.

### Shared lifecycle

The runtime provides a common lifecycle mechanism for registration, initialization, resource ownership, teardown and resource cleanup. Users or their selected orchestration components arrange dependencies and initialization order. Teardown coordinates outstanding work and dependent users before releasing owned resources. Registrations are released with their owning plugin scope.

Plugins declare instance scope and distinguish immutable model data, persistent simulation state and reconstructible caches. Separate scenes can select different implementations and hold independent mutable state. Cross-plugin access follows explicit ownership and lifetime contracts.

The selected execution flow defines checkpoint, rollback, commit and restoration participation. Components supporting restart define their own state checks and reconstruction rules. Users design failure propagation and recovery; lifecycle cleanup follows resource ownership.

Uniform lifecycle denotes common management rules. Callback signatures, state-storage mechanisms, concurrency primitives and binary loading behavior remain subsequent design work.

### Error-handling extension

Error handling is extensible through plugins. The project will provide an official example plugin and application demonstrating diagnostic collection, outcome summaries and caller-selected actions. Users define error categories, information formats, processing protocols, state-validity rules and recovery strategies. Concrete interfaces remain pending. The example records a failed World and its time/solver context; the application chooses its response.

### Default research configuration

[ADR-0009](0009-incremental-potential-rigid-contact.md) defines the official default rigid-body research configuration: hybrid multibody coordinates, incremental-potential inertia, compliant normal contact, ideal hard closures and nonsmooth static friction with semismooth Newton.

Its physical and numerical requirements apply to that configuration. Alternative plugin compositions can select other models and solvers and declare their own behavior and validation requirements. The default composition is assembled through public extension mechanisms and serves as a reproducible reference.

Runtime records identify the actual plugin composition, implementation versions, parameters and execution policy. Friction and damping are extensible capabilities; the default joint-friction direction is nonsmooth dry friction with zero-speed load support and separately additive viscous damping. Detailed joint-friction protocols and discretization remain subsequent specifications.

## Consequences

Extension freedom includes protocol, data and flow definition. Shared protocols provide reuse and substitution within a compatible ecosystem. Users choose dependency management, adapters and validation appropriate to their combinations. Official examples record their actual compositions for reproduction.

[ADR-0011](0011-multi-world-batch-simulation.md) establishes multi-World batch simulation and plugin parallel isolation as core requirements. Specialized data layouts and build-time binding remain available design choices. Concrete C++ interfaces, registration identifiers, static/shared loading, binary ABI and loading mechanisms remain subsequent design work. Runtime compositions remain fixed under [ADR-0019](0019-world-configuration-and-runtime-inputs.md). This record defines accepted principles; implementation remains pending.

## Validation

Validation status: Pending architecture prototypes and implementation.

- Build the official default composition entirely through public extension mechanisms.
- Add a friction plugin with custom history state from an independent project and verify lifecycle and rollback behavior.
- Add a multibody representation and solver that reuse geometry services while owning their state and mathematics.
- Define a previously unknown service protocol in a user package and connect two user plugins through it.
- Replace the entire time-advancement flow using a plugin composition.
- Run two scenes with different implementations and independent state in the same process.
- Exercise component-defined checks, initialization cleanup, teardown and the official error-handling example; record the actual composition for reproduction.

## References

- [DeepSeek Harness architecture](https://github.com/deepseek-ai/deepseek-harness/blob/master/docs/architecture.zh.md)
- [Cordis primer](https://github.com/deepseek-ai/deepseek-harness/blob/master/docs/cordis-primer.zh.md)
