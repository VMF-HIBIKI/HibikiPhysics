# ADR-0008: Linux and macOS toolchain versions

**English** | [简体中文](0008-linux-and-macos-toolchains.zh-CN.md)

Status: Accepted

Date: 2026-10-31

Validation status: Version selection accepted; artifact identities, package resolution and build compatibility pending verification.

## Context and decision

[ADR-0007](0007-platform-and-ci-validation.md) selects Linux x64 and macOS arm64 as initial validation platforms. Each platform uses its corresponding standard library. The selected versions below establish candidate combinations for environment preparation.

## Linux x64

| Component | Selected baseline |
| --- | --- |
| System family | Ubuntu 24.04 LTS, x64 |
| Primary compiler | Clang 22.1.6 |
| Primary linker | LLD 22.1.6 |
| Secondary compiler | GCC 13.3.0 |
| GCC 13 and standard-library development packages | 13.3.0-6ubuntu2~24.04.1 |
| libstdc++6 runtime package | 14.2.0-4ubuntu2~24.04.1 |
| glibc | 2.39-0ubuntu8.9 |
| CMake | 4.4.4 |
| Ninja | 1.13.2 |

Standard-library headers and the dynamically loaded libstdc++ runtime are recorded separately. Their package versions can differ within the distribution's dependency model. Preparation verifies full package resolution, source availability and hashes for the selected combination.

The complete Linux environment identity consists of a build-container digest, system-package manifest and LLVM distribution hashes. The base image identity and package acquisition inputs are retained for reconstruction. The host kernel is recorded separately from the container's user-space packages.

Clang explicitly selects the intended GCC development installation. GCC validation uses a separate build directory. Its linker and binutils revisions remain to be captured in the package manifest. A validated package set and immutable final image digest complete the lock.

## macOS arm64

| Component | Selected baseline |
| --- | --- |
| Validation system | macOS 15, Apple Silicon |
| Xcode | 26.3 |
| Xcode build | 17C529 |
| Compiler | Apple Clang included in the selected Xcode |
| Standard-library headers | libc++ included in the selected Xcode toolchain |
| SDK | macOS 26.2 SDK |
| Linker | Apple linker included in the selected Xcode |
| Initial deployment target | macOS 15.0 |
| CMake | 4.4.4 |
| Ninja | 1.13.2 |

Xcode 26.3 requires a supported host starting at macOS 15.6. The deployment target describes the intended application runtime floor, while the SDK selects the build-time API surface. Validation on newer macOS 15 hosts provides evidence for those tested versions; verification of the minimum runtime remains a separate acceptance item.

The environment explicitly selects the Xcode path. Compiler and linker version strings, SDK build identity and operating-system build number are captured during preparation. libc++ headers come from the selected toolchain, while system runtime libraries follow the running OS. Both identities are retained.

The complete macOS identity includes Xcode version/build, SDK identity, OS build and deployment target. Hosted-runner image identity is recorded on each execution. Tool availability and image changes are surfaced by environment checks.

## Shared tools and configurations

CMake, Ninja, uv, Python, Ruff, basedpyright and dependency-source versions follow the accepted shared baseline. Each platform uses the correct architecture-specific distributions and records their hashes. C++ formatting uses clang-format 22.1.6 across platforms. macOS compilation uses the selected Apple Clang; language-service and analysis compatibility are verified against its compilation database.

Build and numerical behavior follows [ADR-0004](0004-build-and-numerical-policy.md). Sanitizer coverage follows [ADR-0005](0005-sanitizers-memory-and-runtime-diagnostics.md). Initialization and reconstruction follow [ADR-0006](0006-environment-initialization-and-reproduction.md).

## Consequences and validation

Platform-specific library implementations remain versioned dependencies. Linux package revisions and the macOS toolchain bundle require explicit provenance records. A version selection becomes a validated baseline after the following evidence exists:

- [ ] Linux package files, dependency resolution and hashes are verified.
- [ ] Linux base/final container digests and the full package manifest are recorded.
- [ ] Clang uses the selected headers, runtime libraries and linker.
- [ ] GCC and its linker/binutils identities are recorded and tested separately.
- [ ] Xcode build, compiler, linker, SDK and OS identities are verified on a macOS host.
- [ ] Deployment settings and minimum-runtime validation coverage are documented.
- [ ] Builds, tests and installed-library consumers pass on both platforms.
- [ ] Applicable sanitizer and tool-integration checks pass.

Installation, image preparation and environment configuration remain pending. Updated package choices caused by availability or compatibility findings are recorded as explicit baseline revisions.

## References

- [Ubuntu GCC 13](https://packages.ubuntu.com/noble/gcc-13)
- [Ubuntu libstdc++ development package](https://packages.ubuntu.com/noble-updates/libstdc%2B%2B-13-dev)
- [Ubuntu libstdc++ runtime](https://packages.ubuntu.com/noble-updates/libstdc%2B%2B6)
- [Ubuntu glibc](https://packages.ubuntu.com/noble-updates/libc6)
- [Apple Xcode requirements](https://developer.apple.com/xcode/system-requirements)
- [macOS 15 arm64 runner inventory](https://github.com/actions/runner-images/blob/main/images/macos/macos-15-arm64-Readme.md)
