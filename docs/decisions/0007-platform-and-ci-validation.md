# ADR-0007: Platform and CI validation

**English** | [简体中文](0007-platform-and-ci-validation.zh-CN.md)

Status: Accepted

Date: 2026-10-31

Validation status: Platform images, exact Linux/macOS toolchains and workflows pending preparation and validation.

## Context and decision

The initial platform scope is Windows x64, Linux x64 and macOS arm64. Windows is the primary development environment. Linux supplies cross-platform and sanitizer coverage. macOS supplies Apple Silicon source, installation and basic runtime verification.

The considered schedules include running every compiler and diagnostic configuration on every PR, and separating main-platform checks from heavier milestone checks. The selected schedule runs three primary builds on relevant PRs and reserves secondary compilers and heavier suites for explicit or milestone validation.

## Platform responsibilities

| Platform | Toolchain direction | Role |
| --- | --- | --- |
| Windows x64 | Accepted clang-cl baseline with MSVC STL and Windows SDK | Full development validation and ASan |
| Linux x64 | Ubuntu 24.04, Clang 22.1.6 and libstdc++ | Cross-platform tests, ASan/UBSan and leaks |
| macOS arm64 | macOS 15, Apple Clang and libc++ | Build, installation and basic runtime validation |
| Windows secondary compiler | MSVC from the selected toolset | Compiler compatibility |
| Linux secondary compiler | Separately pinned GCC | Compiler compatibility at milestones |

[ADR-0008](0008-linux-and-macos-toolchains.md) records the accepted Linux package and macOS Xcode/SDK selections. Complete artifact identities and compatibility verification remain pending. This record defines platform scope and validation responsibilities.

## PR checks

| Check | Trigger scope |
| --- | --- |
| Title and issue correspondence | PR creation, relevant metadata changes and validation |
| Bilingual documents, links and configuration syntax | Documentation and configuration changes |
| Ruff and basedpyright | Python and related configuration changes |
| Windows clang-cl build and tests | C++, dependency and build changes |
| Linux Clang build and tests | C++, dependency and build changes |
| macOS build and basic tests | C++, dependency and build changes |
| Sanitizers | Memory/algorithm changes; the initial conservative trigger can cover all C++ changes |

Public-header, dependency and build-rule changes trigger the complete platform matrix. A stable aggregate check evaluates required jobs and records explicit reasons for inapplicable checks. Required checks pass only after applicable work succeeds.

## Full and milestone validation

Secondary compiler builds, full physics regressions, long-running stability tests, installed-library consumers, clean reconstruction and offline reconstruction belong to full or milestone validation. TSan, GPU diagnostics and benchmarks follow their feature and resource requirements.

Formal performance comparisons use fixed hardware and record load and environment. Hosted-runner timings describe CI cost and throughput. GPU validation starts as controlled local execution of an identified commit on the RTX 5070 Ti with a reproducible report. Future automated GPU execution receives its own execution environment and external-contribution code boundaries.

## CI environment identity

GitHub-hosted runner labels are platform selectors. Each run records the actual image identity and explicitly selects project tools. Candidate labels are ubuntu-24.04, windows-2025-vs2026 and macos-15 for arm64, subject to availability checks at implementation time.

Linux will use a build container pinned by digest to control user-space dependencies. Reports also capture the host kernel. Windows checks exact MSVC and SDK identities and reports missing components as environment failures. macOS explicitly selects the Xcode path and records Xcode, SDK and OS build identities. External GitHub Actions are pinned to full commit SHAs.

CI uses the project entry points defined in [ADR-0006](0006-environment-initialization-and-reproduction.md). Cache identities include relevant platform, toolchain, configuration and dependency inputs. Artifacts retain environment reports, logs and diagnostic evidence.

## Validation and implementation boundary

- [ ] Linux and macOS exact toolchain/package identities are selected and recorded.
- [ ] The three primary platform builds and installed-library checks pass.
- [ ] Change classification and the aggregate check handle applicable and skipped jobs correctly.
- [ ] Title and implementation-issue correspondence checks follow ADR-0003.
- [ ] Secondary compiler and diagnostic runs produce identifiable reports.
- [ ] Runner image drift and missing pinned tools produce explicit diagnostics.
- [ ] Local GPU reports identify the tested commit and configuration.

This decision accepts the validation scope and schedule. Remote repository creation, workflow activation, installations and environment configuration remain future work.

## References

- [GitHub-hosted runners](https://docs.github.com/en/actions/reference/runners/github-hosted-runners)
- [Runner images](https://github.com/actions/runner-images)
- [Toolchain baseline](0001-development-toolchain-and-dependencies.md)
- [Commit checks](0003-commit-hooks-and-validation.md)
- [Sanitizer policy](0005-sanitizers-memory-and-runtime-diagnostics.md)
