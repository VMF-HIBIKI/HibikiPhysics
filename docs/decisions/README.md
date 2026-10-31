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
