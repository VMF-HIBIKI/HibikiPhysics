# ADR-0006: Environment initialization and reproduction

**English** | [简体中文](0006-environment-initialization-and-reproduction.zh-CN.md)

Status: Accepted

Date: 2026-10-31

Validation status: Windows preparation, diagnostics and offline reconstruction passed.
Current Windows evidence is recorded in the [acceptance record](../environment/acceptance.md).

## Context and decision

The accepted toolchain and dependencies require explicit selection, recoverable preparation and repeatable verification. Tools are stored by version at user scope. Each checkout owns its virtual environment and build outputs. Repository declarations identify requirements; local reports identify the actual environment.

## Storage

The default Windows root is %LOCALAPPDATA%/HibikiPhysics. It contains tools/<tool>/<version>, python for managed interpreters, cache/downloads for content-addressed archives, and cache/uv for uv-managed cache data. Local configuration can override this root.

MSVC and Windows SDK remain in official installation locations and are selected by version and component identity. Explicit overrides may reuse existing tools after identity verification. CI and first-time reproduction prefer manifest-defined distributions.

| Checkout directory | Role |
| --- | --- |
| .local/ | Resolved paths, local settings and environment identity |
| .venv/ | Checkout-specific Python environment |
| build/<preset>/ | Separate build configurations and dependency build trees |
| install/<preset>/ | Installation acceptance artifacts |
| .cache/warp/<profile>/ | Profile-specific Warp kernel cache |
| artifacts/ | Environment reports, test logs and diagnostics |

Generated directories are ignored by Git. Source files, committed settings and handwritten local notes retain their own lifecycle.

## Initialization entry points

| Entry | Responsibility | Effects |
| --- | --- | --- |
| doctor | Inspect versions, paths and environment variables | Console report by default; file report through an explicit output option |
| prepare | Obtain verified tools, prepare Python/dependencies and generate local settings | User tool storage and checkout-generated files |
| verify | Build, test, validate installed-library consumption and tool integration | Build outputs and acceptance reports |

The initial Windows entry uses PowerShell. Platform entry points share these semantics. Preparation validates required manifest fields, resolves installed components, verifies downloaded archives, prepares the specified Python distribution with pinned uv, synchronizes uv.lock, obtains verified C++ archives and generates local paths and CMake User Presets.

Git hook activation, editor-profile preparation and system-component installation have separate explicit options. MSVC/SDK preparation uses the official installation mechanism. Session launchers and subprocess environments provide the selected PATH and variables.

Preparation is idempotent: matching components are reused. Temporary directories hold incomplete downloads and extraction; verified content is promoted to its versioned location. Steps record inputs, outcomes and artifacts for recovery.

## Failure handling

| Condition | Response |
| --- | --- |
| Interrupted download | Completed steps remain reusable; the current acquisition is retried |
| Hash mismatch | The artifact is isolated and preparation of that component fails with source information |
| Version mismatch | Actual and required versions and paths are reported |
| Missing MSVC/SDK | Required components and preparation steps are reported |
| Python synchronization failure | Logs are preserved; retry uses the same lockfile |
| Build configuration change | A separate build directory or explicit reconfiguration is selected |
| Acceptance failure | Diagnostic artifacts are retained and validation status records failure |

Environment states are Pending preparation, Prepared, Validated and Validation failed. Prepared means components are available; Validated means the requested acceptance checks passed.

## Version records

| Record | Responsibility |
| --- | --- |
| tools/toolchain-lock.json | Platform, versions, distributions, URLs, SHA-256 and component identity |
| cmake/Dependencies.cmake | Reads C++ source identities and hashes from the toolchain manifest; declares build options |
| .python-version | Interpreter version |
| pyproject.toml and uv.lock | Python declarations and resolved dependencies |
| CMakePresets.json | Shared build configurations |
| CMakeUserPresets.json | Machine-resolved paths and local configuration |
| .local/environment.json | Detected paths, results and baseline digest |

Each field has an authoritative source. Repeated values such as the interpreter version receive consistency checks. Required hashes, interpreter build identity, SDK servicing details and VSIX identity must be complete before formal preparation.

## Cache policy

Download archives are shared by SHA-256. uv manages its cache using supported interfaces; installed interpreters are persistent tools. FetchContent extraction and dependency builds stay inside each build tree, while verified archives can be shared. C++ outputs are separated by platform, toolchain, configuration and dependency identity.

Warp uses WARP_CACHE_PATH or kernel_cache_dir with profile-specific project paths. Warp adds its own version subdirectory. Diagnostic and ordinary kernel configurations remain identifiable.

Cleanup scopes include one preset, project Warp cache, reclaimable uv cache or a specified archive hash. Cleanup displays resolved targets and verifies scope before mutation. Source files and handwritten local records remain outside generated-artifact cleanup.

## Daily commands and network boundary

The planned project interface exposes doctor, prepare, configure <preset>, build <preset>, test <preset>, check, format, verify and clean <scope>. Wrappers resolve the selected tools and preserve actual commands in logs. VS Code tasks and CI invoke the same project entry points.

Network acquisition belongs to prepare. Daily build, test and check operations consume prepared inputs. Missing content produces preparation guidance. Offline mode verifies archive and package availability before proceeding. CI restores caches, prepares the environment, verifies it and saves reports/artifacts.

Toolchain upgrades update versions, origins and hashes, rebuild affected dependencies, verify editor/CLI paths, and run build, debugging and numerical regressions. Older tool directories may remain available for older commits. Each commit's declarations select its baseline.

## Acceptance

| Level | Evidence |
| --- | --- |
| Environment identity | Versions, paths, hashes, architecture, SDK and runtime match declarations |
| Tool integration | CMake/Ninja builds, clangd indexing, formatting and analysis operate together |
| Library consumption | Build, tests and a separate consumer of the installed library succeed |
| Debugging and computation | C++/Python breakpoints and stacks work; Warp CPU/GPU smoke checks pass where required |
| Reconstruction | A fresh checkout, clean build and offline reconstruction from prepared inputs succeed |

Reports contain a machine-readable record and a readable summary with commit identity, baseline digest, resolved tools, commands, results and timings. Requested CPU/GPU acceptance profiles explicitly identify required checks; omitted optional checks are recorded.

The initial reproducibility contract covers tool/dependency identity, repeatable build steps and specified test outcomes. Byte-identical artifacts require additional treatment of paths, timestamps, debug information and archive ordering and remain a later goal.

ENV-001 records the selected identities. ENV-002 implements [read-only diagnostics](../environment/doctor.md), with its validation scope documented there. Preparation, machine settings and acceptance execution follow subsequent stages.

## References

- [Toolchain and dependencies](0001-development-toolchain-and-dependencies.md)
- [uv storage](https://docs.astral.sh/uv/reference/storage/)
- [uv caching](https://docs.astral.sh/uv/concepts/cache/)
- [CMake FetchContent](https://cmake.org/cmake/help/latest/module/FetchContent.html)
- [Warp cache settings](https://github.com/NVIDIA/warp/blob/v1.18.0/warp/config.py)
