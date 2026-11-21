# ADR-0010: Plugin architecture and lifecycle

**English** | [简体中文](0010-plugin-architecture-and-lifecycle.zh-CN.md)

Status: Accepted

Date: 2026-11-01

Updated: 2026-11-21

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

### Plugin unit and capabilities

A plugin joins a scope through the shared lifecycle and can provide builders, model data, mathematical interfaces, task graphs or complete execution flows. A service plugin can also define how other plugins extend it.

For example, a collision service defines how to register a detector for a pair of shapes. During scene compilation, the selected sphere–plane detector registers with the same collision service instance that will handle queries during a step. Contacts produced during the step belong to the current World.

### Capability declarations and automatic binding

A plugin declares each capability it provides, the protocol it implements, the scene content it handles and the services it requires. The owner of the extension point defines the matching rules. For the collision service, sphere–plane detection is one capability regardless of the order in which the two shapes appear in the file. A single plugin can contribute detectors for several pairs.

The engine supplies a default implementation for each supported capability. Loading a scene uses those defaults automatically. An extension registers a new capability or explicitly replaces a named capability in an experiment configuration. Replacing sphere–plane detection preserves the selection for sphere–sphere detection, even when one plugin supplies both defaults. Every scene built with the experiment configuration uses the replacement. Other configurations retain their own selections.

The native scene and asset layer translates file content into the intermediate scene description defined in [ADR-0013](0013-scenes-and-assets.md). Scene compilation derives required capabilities from those facts, including shapes and collision filters. It selects the experiment replacement where specified and the default otherwise. Once the selected providers are ready, the builder binds their instances to the consumers and mounts them through the validated lifecycle entry point. The compiled scene records which provider supplies each selected capability and whether it came from the default or an experiment replacement. Actual contact detection takes place during simulation.

A missing capability produces a build diagnostic identifying the scene content that requires it. Competing replacements for one capability in the same configuration produce a build diagnostic identifying the capability and the conflicting plugins. Both conditions require resolution before the scene can run.

### Registration interface for plugin authors

Each extension point supplies its own typed declaration interface. The author of a sphere–plane detector supplies an implementation identifier, a shape pair and a detection function through the collision service's interface. The pair is declared once and serves both to match scene content and to check the function signature. The collision service passes the sphere and plane to the function in the order specified by its protocol.

An experiment author can register a declaration in the experiment configuration before loading a scene. The configuration supports two operations.

- `Add` registers a new capability. When an implementation already occupies that entry, the configuration reports the conflict and identifies `Replace` as the operation for an intentional replacement.
- `Replace` takes the new implementation's declaration and derives the target capability from it. For example, a sphere–plane detector declaration already identifies its target pair, so the experiment author supplies the declaration once.

Multiple scenes can share the declarations in a configuration. The builder mounts the selected implementations for each scene. The collision extension point supplies an adapter that declares the collision service dependency, registers the function with that scene's collision service instance and transfers the registration handle to the mounting scope for cleanup.

The installed C++ API uses `CapabilitySlot<T>` to associate a capability name with its protocol type and const qualification. Its `Factory` and `Borrow` declarations carry the target slot and implementation name into `Add` or `Replace`. A configuration copy has an independent selection. The first replacement must match the registered reference type. A second replacement for the same slot reports a conflict. An explicit composition function can supply the defaults for an application.

`Build` and `Mount` validate the requested slot types before running factories. Each scope constructs each requested factory provider once and owns the returned object. Borrowed providers remain caller-owned. Mounting registrations are released before owned providers, in reverse acquisition order, including when construction or mounting fails. `MakePlugin` derives the requirements and callback argument order from the same typed slots. The mounting context is available to callbacks that retain registration handles.

### Dependency declaration and binding

Plugins declare the capabilities they need. The builder resolves providers from the defaults and explicit experiment replacements and runs component-defined checks. Bindings and runtime composition remain fixed during execution. Changes follow the reconfiguration and compilation cycle in ADR-0019. Users continue to manage plugin versions, dependency acquisition and distribution.

For example, a friction plugin declares a need for linear-solving capability, and users can bind different solver providers for different experiments. The capability protocol defines their calling contract. The C++ interface identifies slots by name. `Require<T>` records the type of `std::reference_wrapper<T>`, including the const qualification of `T`. The caller supplies a binding using `std::ref` or `std::cref`. `Plugin::Mount` collects declarations, calls `PluginContext::Validate`, then invokes the protected `OnMount` callback. The first missing or incompatible binding raises `std::invalid_argument` with the slot name and reason. The caller chooses how to handle the failure.

### Dependencies and execution order

Construction dependencies describe the capabilities or data needed to prepare and bind plugins; per-step execution order describes when computations occur. The selected execution flow arranges order, loops and repeated calls. For example, contact-data access is bound during construction, while contact updates and friction solving execute according to the step flow, which may request repeated contact updates during iteration.

Execution flows can themselves be supplied by plugins. Dependency connections and computational ordering are expressed separately; concrete task graphs and scheduling protocols belong to the corresponding plugins.

### Plugin objects and World data

Algorithm implementations and read-only configuration may serve multiple Worlds when their sharing conditions are satisfied. Mutable runtime data such as contact history, friction state and solver workspace is isolated by World. Plugins define data creation, allocation and release and integrate these operations with the shared lifecycle.

Per-World objects and batched arrays are both available storage choices. Isolation concerns logical state; concrete memory layout and plugin object counts are implementation choices. Capability references held across plugins follow provider lifetime and concurrent-access contracts, with batch isolation governed by ADR-0011.

### Protocol ownership and composition

Protocols belong to the plugin ecosystems that use them. Users manage plugin identities, versions, dependencies, provider selection, composition and upgrades. Participants define their own interoperability, data ownership and execution rules. Scene compilation executes checks required by the selected components and reports their results. Concrete protocol and checking mechanisms remain open.

Potential, residual, constraint, projection and matrix-free operator interfaces remain candidates for particular algorithm families. Concrete mathematical interfaces, mandatory capability categories and universal model/discretization/solver layering remain undecided. These concepts can organize the default implementation as its interfaces mature through actual extension cases.

Simulation orchestration is itself replaceable. The default prediction/contact/solve/commit sequence belongs to the default time-advancement implementation. Plugins can expose internal extension points or provide a complete alternative flow. Data-access rules, synchronization, ordering and mutation boundaries are explicit parts of each flow's contract.

### Shared lifecycle

The runtime provides a common lifecycle for registration, initialization and cleanup. Users or their selected orchestration components arrange dependencies and initialization order. The scope of the plugin that registered a function or subscribed to an event owns that registration. Once outstanding calls have finished, teardown removes the scope's entries while their providers are still available. The same cleanup applies to registrations made before a mount fails.

Plugins declare instance scope and distinguish immutable model data, persistent simulation state and reconstructible caches. Separate scenes can select different implementations and hold independent mutable state. Cross-plugin access follows explicit ownership and lifetime contracts.

The selected execution flow defines checkpoint, rollback, commit and restoration participation. Components supporting restart define their own state checks and reconstruction rules. Users design failure propagation and recovery; lifecycle cleanup follows resource ownership.

Uniform lifecycle denotes common management rules. State-storage mechanisms, concurrency primitives and binary loading behavior remain subsequent design work.

### Error-handling extension

Error handling is extensible through plugins. The project will provide an official example plugin and application demonstrating diagnostic collection, outcome summaries and caller-selected actions. Users define error categories, information formats, processing protocols, state-validity rules and recovery strategies. Concrete interfaces remain pending. The example records a failed World and its time/solver context; the application chooses its response.

### Default research configuration

[ADR-0009](0009-incremental-potential-rigid-contact.md) defines the official default rigid-body research configuration: hybrid multibody coordinates, incremental-potential inertia, compliant normal contact, ideal hard closures and nonsmooth static friction with semismooth Newton.

Its physical and numerical requirements apply to that configuration. Alternative plugin compositions can select other models and solvers and declare their own behavior and validation requirements. The default composition is assembled through public extension mechanisms and serves as a reproducible reference.

Runtime records identify the actual plugin composition, implementation versions, parameters and execution policy. Friction and damping are extensible capabilities; the default joint-friction direction is nonsmooth dry friction with zero-speed load support and separately additive viscous damping. Detailed joint-friction protocols and discretization remain subsequent specifications.

## Consequences

Extension freedom includes protocol, data and flow definition. Shared protocols provide reuse and substitution within a compatible ecosystem. Users choose dependency management, adapters and validation appropriate to their combinations. Official examples record their actual compositions for reproduction.

[ADR-0011](0011-multi-world-batch-simulation.md) establishes multi-World batch simulation and plugin parallel isolation as core requirements. Specialized data layouts remain available design choices; dependency binding occurs during scene compilation. Additional C++ interfaces, registration identifiers, static/shared loading, binary ABI and loading mechanisms remain subsequent design work. Runtime compositions remain fixed under [ADR-0019](0019-world-configuration-and-runtime-inputs.md). The installed package implements the plugin base class and binding checks.

## Validation

Validation status: Installed-package tests cover default registration, experiment replacements, binding validation and mounting-scope cleanup. Deriving requirements from a bridged scene, collision adapters, multi-World behavior and scheduling await implementation.

- Build the official default composition entirely through public extension mechanisms.
- Add a friction plugin with custom history state from an independent project and verify lifecycle and rollback behavior.
- Add a multibody representation and solver that reuse geometry services while owning their state and mathematics.
- Connect two user plugins through a service protocol defined in their package. Verify that releasing one scope removes its registrations and preserves entries contributed by other scopes.
- Replace the entire time-advancement flow using a plugin composition.
- Bind different capability providers to the same consumer plugin and verify that actual connections match user selections.
- Exercise construction dependencies and per-step ordering separately in an example flow, including repeated calls within one step.
- Run multiple instances with shared read-only content and independent World state; check reset and release isolation.
- Run two scenes with different implementations and independent state in the same process.
- Exercise component-defined checks, initialization cleanup, teardown and the official error-handling example; record the actual composition for reproduction.

## References

- [DeepSeek Harness architecture](https://github.com/deepseek-ai/deepseek-harness/blob/master/docs/architecture.zh.md)
- [Cordis primer](https://github.com/deepseek-ai/deepseek-harness/blob/master/docs/cordis-primer.zh.md)
