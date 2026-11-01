# ADR-0013: Scenes and assets

**English** | [简体中文](0013-scenes-and-assets.zh-CN.md)

Status: Accepted

Date: 2026-11-01

Related work: [Plugin architecture and lifecycle](0010-plugin-architecture-and-lifecycle.md), [multi-World batch simulation](0011-multi-world-batch-simulation.md), [usage and runtime modes](0012-usage-and-runtime-modes.md).

## Context

HibikiPhysics needs programmatic scene construction, reusable native scene descriptions and robot/geometry asset import. Different source formats express distinct structures and physical semantics. A common intermediate layer provides a shared path from those inputs to models and plugin compositions, while preserving user-defined extensions.

## Alternatives

| Approach | Trade-off |
| --- | --- |
| Each importer directly constructs runtime internals | Format-specific integration is straightforward; validation and runtime assumptions become distributed across importers. |
| A source format defines the engine's scene model | Reuses an existing vocabulary; extension coverage follows that format's conventions. |
| Native extensible intermediate layer with format plugins | Selected approach; common validation and construction serve multiple formats and custom model semantics. |

## Decision

### Scene sources and native intermediate layer

The engine provides a native intermediate parsing and scene-description layer. C++/Python programmatic construction, native scene loading and external format adapters converge on the same scene semantics and validation/model-building path.

The conceptual flow is:

```text
Source format -> format adapter plugin -> extensible intermediate scene description
Programmatic construction / native scene loading -> intermediate scene description
Intermediate scene description -> validation and model building -> model and plugin composition
Model -> independent World instances
```

The intermediate layer represents bodies/links, joints and connections, geometry, mass properties, materials, actuators, initial conditions and asset references. It expresses authored scene and physical-model semantics. Selected downstream plugins determine solver coordinates, runtime layout and assembly.

### Format adapters

Official URDF and MJCF adapters are the primary robot-import targets, with incremental acceptance against documented feature coverage. USD remains a subsequent scene-exchange direction. Mesh assets have their own import path; concrete file-format coverage is specified later.

Official and user adapters use the same public registration and conversion mechanisms. Users can add formats in independent packages and reuse downstream validation, model construction and multi-World capabilities. Adapter identity, version, options and source provenance are available in import reports.

Each adapter defines mappings for units, coordinate conventions, references, topology and physical attributes. Parsing coverage and end-to-end physical support are reported separately. Unsupported or differing semantics produce diagnostics with an explicit error, warning or user-selected substitution policy. Reports retain the source location/name and the applied conversion or substitution.

### User-defined extensions

Plugins may define new scene elements, properties, model data and resource kinds, together with their validation and construction handlers. Extension identities and dependency information let the pipeline resolve the appropriate handlers without changing built-in core type lists.

Native scene persistence supports plugin-owned configuration and model content through declared save/load capabilities. Missing handlers or incompatible versions produce explicit diagnostics. Preservation of opaque extension data and executable simulation support have distinct declared meanings. Concrete extension schemas and registration APIs remain undecided.

### Scene, snapshot and experiment records

| Artifact | Responsibility |
| --- | --- |
| Scene/model description | Structure, asset references, physical parameters, initial conditions and selected plugin configuration. |
| State snapshot | A World's current authoritative state and plugin histories required for supported continuation, with references to the compatible model and configuration. |
| Experiment record | Actual model/plugin identities, inputs, parameters, randomization, execution policy, outcomes and diagnostics. |

Multiple Worlds can share one model and its immutable assets while maintaining their independent state and per-World parameters fixed during scene compilation. Asset identity and plugin-version provenance connect these artifacts. Playback and resumable restoration retain the distinct guarantees established in ADR-0012.

### Asset preprocessing

Mesh processing, mass-property calculation, convex decomposition and SDF generation are extensible preprocessing capabilities. Generated artifacts record source asset identity, preprocessing implementation/version and parameters so that cached results can be identified and reproduced.

Original geometry remains available for uses such as surface sampling, visualization, validation and regeneration. Collision representation and mass-property sources are explicitly selected. The specific SDF storage, construction, interpolation and derivative routes remain undecided for later investigation.

## Consequences

A shared intermediate layer separates source-format concerns from runtime organization and supports a common validation path. Format adapters and user extensions require clear conversion semantics, provenance and handler compatibility. A source asset's supported simulation behavior is established through the selected plugins and acceptance cases.

Concrete data structures, native file syntax, schema/versioning mechanisms, parser libraries, resource-resolution rules, preprocessing algorithms and C++/Python interfaces remain undecided. This record establishes the accepted architecture and responsibilities; implementation remains pending.

## Validation

Validation status: Pending implementation and import/extension acceptance.

- Construct equivalent supported scenes programmatically and through native/external inputs; compare model semantics and representative simulation behavior.
- Add a format adapter from an independent project through the public extension mechanism.
- Add a user-defined scene element with validation, construction and persistence handlers; check save/load under a compatible composition.
- Check source-linked diagnostics for unsupported properties, unit/frame mappings, missing assets, handlers and version mismatches.
- Verify that supported robot hierarchies, connections and mass properties survive conversion and model construction.
- Instantiate multiple independent Worlds from one constructed model and check shared asset identity and state isolation.
- Check preprocessing cache provenance and the declared distinction between scene loading, snapshot restoration and experiment replay. Group related cases to control test-file count.
