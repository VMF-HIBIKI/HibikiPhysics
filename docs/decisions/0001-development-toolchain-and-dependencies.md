# ADR-0001: Development toolchain and dependencies

**English** | [简体中文](0001-development-toolchain-and-dependencies.zh-CN.md)

Status: Accepted

Date: 2026-10-31

Validation status: Windows toolchain and CPU/GPU runtime checks passed.
Current Windows evidence is recorded in the [acceptance record](../environment/acceptance.md).

Related work: HP-003, HP-004, HP-011, HP-012.

## Context

HibikiPhysics targets C++20 systems development and physics algorithm implementation. Development requires reproducible tool selection, explicit dependency versions, useful editor diagnostics, and source compatibility across Windows, Linux and macOS.

The initial development environment is Windows x64. Platform-specific standard libraries are part of each platform's versioned toolchain. Windows uses MSVC STL. Linux and macOS toolchain baselines will be specified separately.

## Alternatives considered

| Area | Alternatives | Selected direction |
| --- | --- | --- |
| Windows compiler | MSVC, clang-cl | clang-cl as the primary compiler; MSVC as a compatibility check |
| Standard library | Platform libraries, project-built libc++ on all platforms | Platform libraries with explicit version records |
| C++ dependency acquisition | Conan 2, vcpkg Manifest, CMake FetchContent | FetchContent for the current small dependency set |
| Mathematics | Eigen-backed core, project-owned small types with Eigen references | Project-owned small types and Eigen-based validation |
| C++ tests | doctest, GoogleTest | GoogleTest with CTest |
| Logging | Application callbacks, direct logging-library integration | Core callbacks with spdlog in tools |
| Performance measurement | Library microbenchmarks, application scenarios | Google Benchmark plus a scene runner |

FetchContent allows the selected dependencies to build within the project's configured environment. Dependency options and source identities remain explicit. A future increase in dependency complexity can motivate a separate package-management decision.

## Windows toolchain baseline

The following versions are accepted candidates for implementation. Successful build and runtime checks will establish the validated baseline.

| Component | Version or configuration |
| --- | --- |
| Target | Windows x64 |
| Language | C++20 |
| clang-cl | 22.1.6 |
| clangd | 22.1.6 |
| clang-format | 22.1.6 |
| clang-tidy | 22.1.6 |
| Linker | lld-link 22.1.6 |
| MSVC toolset | 14.51.36231 |
| C++ standard library | MSVC STL supplied by toolset 14.51.36231 |
| Windows SDK selection | 10.0.26100.0 |
| CMake | 4.4.4, upstream distribution |
| Ninja | 1.13.2, upstream distribution |
| Generator | Ninja, single configuration |
| Release / RelWithDebInfo CRT | Dynamic, /MD |
| Debug CRT | Dynamic, /MDd |
| Initial LTO | Disabled |
| Initial exceptions and RTTI | Enabled |

ENV-001 records SDK DisplayVersion 10.1.26100.8249, catalog package version 10.0.26100.16 and MSVC compiler file version 19.51.36248.0. Toolset directories, package revisions and selected file hashes are recorded separately in the [Windows identity records](../environment/windows-identities.md) and their machine-readable manifests.

Each build configuration has a dedicated directory. CMake emits a compilation database for the active configuration. clangd uses the selected development database; clang-tidy uses the database for its check configuration.

Toolchain selection resolves explicit executable, header and library paths. Machine-specific absolute paths belong in local configuration. A future environment-check script compares resolved tools with the baseline and reports mismatches before configuration proceeds.

[ADR-0004](0004-build-and-numerical-policy.md) defines optimization levels, floating-point behavior, instruction-set requirements and assertion policy.

## Python and Warp baseline

| Component | Version or configuration |
| --- | --- |
| uv | 0.13.6 |
| Interpreter | CPython 3.12.16, conventional GIL build, x64 on Windows |
| Initial Python requirement | >=3.12,<3.13 |
| Warp | warp-lang 1.18.0, official Windows x64 wheel |
| Python tests | pytest 9.2.0 |

uv manages the project interpreter and virtual environment. The interpreter distribution identifier, URL and checksum supplement the Python version. The project uses a Python version file, dependency declarations and a committed uv lockfile. Environment synchronization and command execution use locked mode.

The Warp runtime environment records the selected wheel, GPU and actual driver version. The inspected machine has an RTX 5070 Ti and driver 610.88. Warp's release metadata and versioned installation documentation provide compatibility requirements; CPU and GPU execution remain to be verified locally.

Warp source builds require a separate CUDA Toolkit and host-compiler compatibility decision. The selected starting point uses the published wheel and its bundled CUDA components.

## Dependency baseline and boundaries

| Dependency | Version | Scope | License record |
| --- | --- | --- | --- |
| Eigen | 5.0.1 | Mathematical reference tests initially | Primarily MPL-2.0; bundled third-party notices also retained |
| GoogleTest | 1.18.0 | C++ test executables | BSD-3-Clause |
| spdlog | 1.17.0 | Tool executables and their logging adapter | MIT |
| Bundled fmt | 12.1.0 within spdlog | spdlog formatting | MIT |
| Google Benchmark | 1.10.0 | Microbenchmark executables | Apache-2.0 |
| pytest | 9.2.0 | Python and Warp tests | MIT |

Core mathematical types initially cover Vec3, Mat3, Quat and Transform. Scalar implementations establish behavior and layout. Eigen supplies independent reference calculations. More extensive numerical dependencies and SIMD work follow concrete requirements and measurements.

The core exposes a project-owned logging callback. Tools adapt that callback to spdlog. Python code uses standard logging. Scene benchmarks report numerical quality alongside timing and workload statistics.

### Dependency build configuration

| Dependency | Initial configuration |
| --- | --- |
| Eigen | Header-based reference calculations; upstream tests, demos, documentation, BLAS and LAPACK builds disabled |
| GoogleTest | GoogleTest enabled; GoogleMock, Abseil/RE2 integration and installation disabled; dynamic Windows CRT |
| spdlog | Compiled static library with bundled fmt; dynamic Windows CRT; upstream tests, examples, benchmarks and installation disabled |
| Google Benchmark | Library enabled; upstream tests, dependency downloads, Python bindings, libpfm and installation disabled |
| pytest | Base package; plugins introduced through explicit dependency changes |

Dependency options are applied at the appropriate target or dependency scope. Project test execution remains enabled independently of upstream dependency self-tests. Engine targets explicitly require C++20. Third-party diagnostic settings remain separate from project warning policies.

## Locking and provenance

The planned records have distinct responsibilities:

| Record | Responsibility |
| --- | --- |
| tools/toolchain-lock.json | Tool versions, distribution identities, URLs, hashes, architecture and runtime configuration |
| cmake/Dependencies.cmake | Reads immutable C++ source identities and hashes from the toolchain manifest; declares build options |
| .python-version | Project interpreter version |
| pyproject.toml | Python requirement, direct dependency versions and tool configuration |
| uv.lock | Resolved Python dependencies, sources and distribution hashes |
| third_party/licenses/ | License and attribution files from the selected source distributions |
| Validation report | Actual executable paths, versions, build results, GPU and driver details |

Source archives use FetchContent URL_HASH verification. Version labels support human review; resolved commits and archive hashes identify the selected content. The initial configuration obtains sources from declared locations. Source overrides for dependency development are explicit and recorded in validation output.

Source caches are keyed by content identity. Build directories and compiled caches are separated by platform, toolchain and configuration. A source cache can be retained for offline use after the initial verified download.

Dependency upgrades update the source identity, options, license records and validation results in the same reviewable change. Standard-library and compiler upgrades trigger rebuilding affected C++ dependencies.

## Validation and outstanding work

The current evidence consists of local version inspection and upstream release/configuration review. Acceptance of this ADR records the chosen direction and versions.

- [x] Distribution URLs, resolved source commits and verified archive hashes are captured.
- [x] Python distribution identity and Windows SDK servicing information are captured.
- [x] Clean Debug and Release builds use the selected compiler, linker, STL and CRT.
- [x] A consumer program links to an installed library target.
- [x] clangd and clang-tidy resolve project headers through the compilation database.
- [x] Debug symbols, breakpoints, call stacks and variable inspection work.
- [x] GoogleTest executes through CTest with the selected runtime configuration.
- [x] Eigen reference calculations and spdlog console/file output work.
- [x] Google Benchmark produces a machine-readable report.
- [x] A locked Python environment runs pytest and minimal Warp CPU/GPU kernels.
- [x] Binary dependency inspection confirms the intended runtime linkage.
- [x] Source-cache reuse and clean environment reconstruction are verified.

ENV-001 records verified C++ source identities and bundled notices. Python lockfile generation and installation follow the implementation plan. [ADR-0008](0008-linux-and-macos-toolchains.md) defines the Linux/macOS baselines.

[ADR-0002](0002-editor-debugging-and-code-quality.md) records the accepted VS Code, debugging and quality-tool configuration; Windows integration verification passed.

## References

- [CMake 4.4.4](https://github.com/Kitware/CMake/releases/tag/v4.4.4)
- [Ninja 1.13.2](https://github.com/ninja-build/ninja/releases/tag/v1.13.2)
- [uv 0.13.6](https://github.com/astral-sh/uv/releases/tag/0.13.6)
- [Warp 1.18.0 installation](https://github.com/NVIDIA/warp/blob/v1.18.0/docs/user_guide/installation.rst)
- [Eigen 5.0.1 build configuration](https://gitlab.com/libeigen/eigen/-/blob/5.0.1/CMakeLists.txt)
- [Eigen license information](https://gitlab.com/libeigen/eigen/-/blob/5.0.1/COPYING.README)
- [GoogleTest 1.18.0](https://github.com/google/googletest/releases/tag/v1.18.0)
- [spdlog 1.17.0](https://github.com/gabime/spdlog/releases/tag/v1.17.0)
- [Google Benchmark 1.10.0](https://github.com/google/benchmark/releases/tag/v1.10.0)
- [pytest 9.2.0](https://pypi.org/project/pytest/9.2.0/)
