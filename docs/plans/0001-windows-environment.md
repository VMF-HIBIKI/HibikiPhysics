# Windows environment implementation plan

**English** | [简体中文](0001-windows-environment.zh-CN.md)

Status: ENV-001 through ENV-006 complete for the Windows scope.

Date: 2026-10-31.

## Objective and scope

The first implementation delivers a Windows x64 development environment, a minimal installable static C++ library and independent toolchain acceptance cases. Windows validation precedes Linux and macOS implementation. Physics algorithms, World/body APIs and simulation scenes belong to subsequent plans.

The initial library uses the public CMake target hibiki::physics. Its minimal callable surface reports a project/build identifier so a real object file can be compiled, linked and consumed. The interface will be reviewed as an infrastructure API. Examples for numerical behavior and diagnostics live in separate toolchain-validation targets.

The static library uses the accepted dynamic CRT policy: /MD for Release and RelWithDebInfo, /MDd for Debug, and /MD for the Windows ASan diagnostic configuration. Shared-library support remains a later extension.

ENV-001 delivers identity declarations, archive verification and bilingual records. Tool installation, environment preparation, hook activation and physics implementation follow their planned stages. Current evidence is linked in the [Windows identity records](../environment/windows-identities.md).

Acceptance evidence is recorded in the [Windows acceptance record](../environment/acceptance.md).

## Governing decisions

- [ADR-0001: Toolchain and dependencies](../decisions/0001-development-toolchain-and-dependencies.md)
- [ADR-0002: Editor, debugging and code quality](../decisions/0002-editor-debugging-and-code-quality.md)
- [ADR-0003: Commit hooks and validation](../decisions/0003-commit-hooks-and-validation.md)
- [ADR-0004: Build and numerical policy](../decisions/0004-build-and-numerical-policy.md)
- [ADR-0005: Memory and runtime diagnostics](../decisions/0005-sanitizers-memory-and-runtime-diagnostics.md)
- [ADR-0006: Initialization and reproduction](../decisions/0006-environment-initialization-and-reproduction.md)
- [ADR-0007: Platform and CI scope](../decisions/0007-platform-and-ci-validation.md)
- [ADR-0008: Linux/macOS follow-up](../decisions/0008-linux-and-macos-toolchains.md)

The ADRs own tool versions and rules. This plan owns implementation sequencing, file responsibilities and acceptance evidence. Compatibility findings that require a version change update the relevant ADR and machine-readable baseline together.

## Tasks and order

The identifiers are local planning references. Future implementation issues and PRs share the exact English titles listed below. Current planning proceeds locally.

| ID | Issue and PR title | Dependency | Deliverable |
| --- | --- | --- | --- |
| ENV-001 | build: Record Windows toolchain and dependency identities | Accepted ADRs | Source records, complete binary identities and schemas |
| ENV-002 | build: Add read-only Windows environment diagnostics | ENV-001 | doctor entry and structured diagnostics |
| ENV-003 | build: Add the static library and acceptance projects | ENV-001 | Build files, install export and small validation cases |
| ENV-004 | build: Add reproducible Windows environment preparation | ENV-001, ENV-002; ENV-003 inputs | prepare, local configuration and dependency preparation |
| ENV-005 | build: Integrate VS Code and project quality tools | ENV-003, ENV-004 | Editor templates, quality entry points and hook configuration |
| ENV-006 | test: Verify Windows environment reconstruction | ENV-002 through ENV-005 | Acceptance execution and bilingual reports |

ENV-002 and ENV-003 can be authored independently after the manifest contract is established. Executable validation follows environment preparation. Each stage records evidence before its completion checkbox is updated.

## Stage 1: Identities and declarations

Planned files:

- tools/toolchain-lock.json and its schema: exact tool versions, platform packages, URLs, hashes, component identity and completeness checks.
- cmake/Dependencies.cmake: immutable C++ source references, archive hashes and dependency options.
- pyproject.toml and .python-version: accepted Python tools, runtime dependencies and interpreter selection.
- third_party/licenses/: license texts and provenance for selected distributions.
- Bilingual environment instructions under docs/.

Required provenance includes LLVM, CMake, Ninja and uv archives; the managed Python build; MSVC installation components and toolset files; SDK servicing details; VSIX packages; and the selected C++ source archives. Each record carries its verification status. Required unresolved identities cause a preparation-readiness failure.

The Python manifest separates quality/development tools from optional Warp use. Exact direct versions follow the ADRs. The first uv.lock is generated explicitly with the selected uv during Stage 4 after that executable is available. Its resolved versions, indexes, markers and hashes are reviewed before locked environment synchronization. Subsequent preparation consumes the committed lockfile.

- [x] ENV-001 declarations and verified provenance are complete for the Windows scope.
- [x] Source archives and bundled notices match their recorded identities.
- [x] Schema checks distinguish complete records from pending information.

## Stage 2: Read-only doctor

Implemented entry: scripts/dev.ps1 doctor. The bootstrap inspection uses Windows PowerShell facilities and can diagnose missing project Python or uv. Shared helper code belongs under scripts/environment/.

Doctor checks executable identities, Visual Studio installations, selected MSVC headers/libraries, SDK, architecture, local overrides, interpreter and extension paths. It detects inconsistent duplicated version fields and reports the proposed resolved toolchain.

The default output is a console report. An explicit output argument writes JSON under artifacts/environment/. Outcomes distinguish matched, missing, mismatched and pending identity. The process fails when required checks fail and describes corrective preparation steps.

- [x] ENV-002 reports required tools and their actual paths accurately.
- [x] Missing-tool and wrong-version fixtures produce meaningful failures.
- [x] Default execution preserves the environment and filesystem.
- [x] Paths containing spaces and explicit overrides are handled correctly.

Usage and validation scope are recorded in the [doctor guide](../environment/doctor.md).

## Stage 3: Static-library and acceptance source

Planned files include root CMakeLists.txt, CMakePresets.json, cmake/HibikiPhysicsConfig.cmake.in, a Windows toolchain module, a minimal public build-information header/implementation, and tests/toolchain/ projects.

The static target exports hibiki::physics. Install rules place headers, libraries and CMake package metadata under install/<preset>/. An independent consumer uses find_package(HibikiPhysics CONFIG REQUIRED) and links the installed target. Its include and library resolution is verified against the installation tree.

Presets cover Windows clang-cl Debug, RelWithDebInfo, Release and ASan. A separate MSVC compatibility configuration is prepared for the planned secondary-compiler check. Each configuration has its own build directory and compilation database.

The core library is configured with tests, benchmarks and tools as explicit options. Eigen/GoogleTest belong to test targets; Google Benchmark belongs to benchmark targets; spdlog belongs to tool targets. External dependency preparation uses verified local archives. Missing content produces a preparation diagnostic. The standalone C++ consumer uses the prepared C++ package independently of the Python environment.

- [x] ENV-003 source defines a real static archive and installed CMake target.
- [x] Presets encode the accepted compiler, CRT, optimization and numerical policy.
- [x] Third-party build options and installation boundaries are explicit.
- [x] Toolchain cases remain separate from future physics implementation.

## Stage 4: Environment preparation

Implemented entry: scripts/dev.ps1 prepare; see the [preparation guide](../environment/prepare.md). Preparation resolves the manifest, acquires archives, checks hashes, extracts into temporary directories and promotes verified artifacts. Existing matching components are reused. MSVC/SDK installation follows the official workflow through an explicit system-component option.

The selected uv executable prepares the exact interpreter distribution. Initial lockfile authoring follows the Stage 1 process; ordinary preparation uses uv sync --locked. Profile selection distinguishes C++/quality checks and Warp acceptance requirements.

Generated files include .local/environment.json and CMakeUserPresets.json. Session/subprocess environments resolve selected tools. Cleanup verifies resolved paths and scopes before changes. Offline preparation checks completeness up front. Installation and hook/editor activation are separately identifiable operations.

- [x] ENV-004 prepares only declared artifacts and settings.
- [x] The reviewed uv.lock is committed before ordinary locked synchronization.
- [x] Repeated preparation reuses matching components.
- [x] Interrupted downloads, invalid hashes and missing offline content have recoverable results.
- [x] Build and test entry points consume prepared inputs and retain actual commands in logs.

## Stage 5: Editor and checks

Planned files include .clang-format, .clang-tidy, .editorconfig, .pre-commit-config.yaml, Python settings in pyproject.toml, tracked editor templates under tools/vscode/, and scripts for check/format/verify.

The root .gitignore will explicitly cover generated caches and reports. Shared VS Code templates remain versioned; generated local settings resolve machine paths. A dedicated profile uses pinned extension identities. Hook activation remains an explicit prepare option.

The editor starts clangd from the selected LLVM installation and uses the matching compilation database. cppvsdbg handles C++ debugging. Python uses project basedpyright/Ruff servers and a locked debugpy server through attach. Formatting, lint and hook commands share the locked tools and accepted rules.

- [x] ENV-005 connects editor features to verified project tools.
- [x] Formatting/check modes follow ADR-0002 and ADR-0003.
- [x] Hook tests cover partial staging, safe fixes and failure reporting.
- [x] Handwritten Python stubs and generated-file exclusions follow the agreed scope.

## Stage 6: Small acceptance cases

| Case | Expected evidence |
| --- | --- |
| Static library and installed consumer | Real symbol compiled into a static archive, found and called from an independent consumer |
| GoogleTest/CTest | A small parameterized arithmetic or container test passes; a controlled failing case verifies failure propagation |
| Eigen | Fixed small matrix operations agree with an explicit expected result |
| spdlog | Console/file sink output and flushing work in a small tool |
| Google Benchmark | A bounded synthetic workload produces machine-readable output |
| Numerical settings | Signed zero, NaN/Inf, underflow and multiply-add examples exercise the accepted configuration |
| Windows ASan | An isolated controlled fault produces a symbolized report and nonzero child exit; a clean control passes |
| Python quality tools | Controlled lint/type fixtures produce expected diagnostics, with clean controls |
| Warp CPU/GPU | A small elementwise kernel matches its reference for the required device profile |
| Editor/debugger | C++ and Python breakpoints, stack frames, variable views and source navigation work |
| Reconstruction | A fresh build/checkout and offline use of verified cached inputs reproduce the required outcomes |

Controlled faulty programs are opt-in child processes. The harness treats expected detection as success and verifies the diagnostic category, making unrelated crashes distinguishable. Ordinary smoke cases remain valid programs. Editor checks retain a manual checklist and evidence; command-line checks record automated results.

The final report records the checkout/commit identity, local changes, manifest digest, tools, commands, configuration, required/optional checks and results. Its readable summary has English and Chinese editions; structured evidence uses stable machine-readable keys. A missing required GPU check remains pending or failed rather than satisfying GPU acceptance.

- [x] ENV-006 validates the required Windows configurations and cases.
- [x] Reports distinguish passed, failed, skipped and pending checks.
- [x] Clean and offline reconstruction cases pass.
- [x] Installation/consumption and diagnostic evidence are retained.

## Completion and follow-up

Windows environment completion requires the selected manifest identities, scripts and settings, passing required acceptance cases, and bilingual reproduction instructions. Pending requirements remain visible in the report.

Linux container preparation and macOS validation follow the Windows milestone using ADR-0008. Remote CI activation follows hosting setup. Physics implementation follows its own scope and design work. API, ABI and shared-library expansion receive explicit subsequent decisions.
