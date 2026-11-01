# Architecture decision records

**English** | [简体中文](README.zh-CN.md)

Architecture decision records preserve the reasoning behind consequential choices.

Records use [the template](0000-template.md), with `NNNN-short-title.md` for English and `NNNN-short-title.zh-CN.md` for Chinese. Numbering starts at `0001`. Statuses are Proposed, Accepted and Superseded. A replacement record links to the superseded decision and explains the new evidence.

Each record describes context, alternatives, the decision, consequences and validation. Issue and pull request links provide implementation traceability when available.

Initial decision topics cover the C++/Warp boundary, ownership and handles, numerical conventions, and the first solver approach.

## Records

| Record | Decision status | Validation status |
| --- | --- | --- |
| [ADR-0001: Development toolchain and dependencies](0001-development-toolchain-and-dependencies.md) | Accepted | Windows toolchain and CPU/GPU runtime checks passed. |
| [ADR-0002: Editor, debugging and code quality](0002-editor-debugging-and-code-quality.md) | Accepted | Windows editor, CLI and debugger integration passed. |
| [ADR-0003: Commit hooks and validation](0003-commit-hooks-and-validation.md) | Accepted | Local hooks and checks passed; remote PR CI remains subsequent work. |
| [ADR-0004: Build and numerical policy](0004-build-and-numerical-policy.md) | Accepted | Windows toolchain numerical cases passed; engine-specific checks follow implementation. |
| [ADR-0005: Sanitizers, memory checks and runtime diagnostics](0005-sanitizers-memory-and-runtime-diagnostics.md) | Accepted | Windows ASan and Warp CPU/CUDA/graph checks passed; additional platform diagnostics follow their milestones. |
| [ADR-0006: Environment initialization and reproduction](0006-environment-initialization-and-reproduction.md) | Accepted | Windows preparation, diagnostics and offline reconstruction passed. |
| [ADR-0007: Platform and CI validation](0007-platform-and-ci-validation.md) | Accepted | Pending platform preparation and workflows |
| [ADR-0008: Linux and macOS toolchain versions](0008-linux-and-macos-toolchains.md) | Accepted | Pending package identities and build verification |
| [ADR-0009: Default rigid-body research configuration](0009-incremental-potential-rigid-contact.md) | Accepted | Pending implementation and numerical experiments |
| [ADR-0010: Plugin architecture and lifecycle](0010-plugin-architecture-and-lifecycle.md) | Accepted | Pending architecture prototypes and extension validation |
| [ADR-0011: Multi-World batch simulation](0011-multi-world-batch-simulation.md) | Accepted | Pending implementation, isolation and batch validation |
| [ADR-0012: Usage and runtime modes](0012-usage-and-runtime-modes.md) | Accepted | Pending implementation and workflow acceptance |
| [ADR-0013: Scenes and assets](0013-scenes-and-assets.md) | Accepted | Pending implementation and import/extension acceptance |
| [ADR-0014: Visualization and diagnostics](0014-visualization-and-diagnostics.md) | Accepted | Pending renderer integration and diagnostic acceptance |
| [ADR-0015: Official research experiments and evaluation](0015-experiments-and-acceptance.md) | Accepted | Pending experiment infrastructure and evidence |
| [ADR-0016: Time advancement](0016-time-advancement.md) | Accepted | Pending implementation and time/state validation |
| [ADR-0017: Snapshots and restoration](0017-snapshots-and-restoration.md) | Accepted | Pending implementation and restoration validation |
| [ADR-0018: CPU and Warp backend responsibilities](0018-cpu-and-warp-backends.md) | Accepted | Pending implementation and backend experiments |
| [ADR-0019: World configuration and runtime inputs](0019-world-configuration-and-runtime-inputs.md) | Accepted | Pending implementation |
