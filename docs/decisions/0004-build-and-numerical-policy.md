# ADR-0004: Build and numerical policy

**English** | [简体中文](0004-build-and-numerical-policy.zh-CN.md)

Status: Accepted

Date: 2026-10-31

Validation status: Windows toolchain numerical cases passed; engine-specific checks follow implementation.
Current Windows evidence is recorded in the [acceptance record](../environment/acceptance.md).

## Context

[ADR-0001](0001-development-toolchain-and-dependencies.md) defines the toolchain. The engine needs explicit optimization, floating-point and instruction-set policies so correctness and performance measurements have identifiable conditions.

## Alternatives and decision

The options considered include changing floating-point behavior with build type, enabling host-specific instructions by default, and using an explicit portable baseline with separate optimization experiments. The selected approach keeps the baseline floating-point semantics consistent across build types. FMA, AVX2 and LTO experiments receive separately identified configurations and measurements.

## Build configurations

| Configuration | Windows clang-cl optimization | Debug symbols | Engine internal assertions | Purpose |
| --- | --- | --- | --- | --- |
| Debug | /Od | Enabled | Enabled | Stepping and local correctness checks |
| RelWithDebInfo | /O2 | Enabled | Enabled | Daily execution and optimized debugging |
| Release | /O2 | Enabled and retained | Disabled | Performance measurement and release validation |

Release symbols can be retained as separate artifacts associated with the executable. LTO remains disabled in the initial baseline. A later LTO configuration records link time, artifact size, runtime performance and numerical regression results.

These optimization flags apply to the Windows baseline. Linux and macOS mappings will be defined with their toolchains. Project assertions have a dedicated switch independent of third-party NDEBUG behavior and preserve compatible public layouts across configurations.

## Floating-point baseline

All three baseline configurations preserve NaN, infinity and signed-zero semantics, retain floating-point expression ordering constraints, and use ordinary division and square-root accuracy requirements. Fast-math and automatic multiply-add contraction are disabled.

The implementation will specify and verify the effective clang-cl options and their ordering. Precise floating-point mode and contraction settings are configured separately. Release optimization follows the same selected numerical policy.

## Fused multiply-add

FMA evaluates a multiply-add with one rounding step. Separate multiply and add operations can produce different results through two rounding steps. The baseline disables automatic contraction to establish explicit comparison conditions.

Explicit std::fma calls or FMA intrinsics are part of the algorithm contract and receive dedicated tests. A later FMA optimization configuration compares runtime, residuals and stability. Warp comparisons record their own fusion settings.

Cross-platform bitwise agreement requires additional control over algorithm order, parallel reductions, mathematical functions and runtime state. This decision establishes numerical settings and comparison conditions; reproducibility guarantees will be defined through separate validation evidence.

## CPU instruction sets

The Windows/Linux x64 baseline targets SSE2. A later AVX2 configuration explicitly identifies its additional CPU requirements. ARM64 requirements will be defined with the corresponding platform baseline.

Release artifacts and comparable benchmarks use explicit instruction-set settings. Native-host builds serve local investigations and record the CPU and effective compiler options. Scalar source implementations may be automatically vectorized within the selected target's instruction set.

## Assertions and error handling

| Check | Debug | RelWithDebInfo | Release |
| --- | --- | --- | --- |
| Internal invariants | Enabled | Enabled | Disabled |
| Public API input validation | Enabled | Enabled | Enabled |
| Expensive full-state checks | Opt-in | Opt-in | Dedicated validation configuration |
| Floating-point traps | Dedicated diagnostic configuration | Dedicated diagnostic configuration | Ordinary runtime mode |

Invalid handles, invalid parameters and resource failures use explicit error handling. Assertion expressions are free of side effects so necessary operations remain consistent across configurations. The exact error-reporting API is a later interface decision.

Floating-point trap diagnostics require compiler and platform verification. They are separate from the ordinary numerical baseline.

## Thread floating-point state

The proposed implementation of the accepted baseline uses round-to-nearest, ties-to-even and gradual underflow. On x86 this includes checking FTZ/DAZ state. Relevant engine entry points and worker-thread initialization verify the required floating-point state. Any scoped adjustment restores the caller's state afterward.

Platform-layer design will establish the exact checking and restoration boundaries. CPU and GPU runtime-state behavior is validated separately.

[ADR-0005](0005-sanitizers-memory-and-runtime-diagnostics.md) specifies instrumented builds, allocator checks and GPU diagnostic workflows.

## Validation

- [x] Effective compiler and linker commands match the selected build configuration.
- [ ] Symbols, stepping and optimized crash diagnosis work for the selected toolchain.
- [x] Numerical cases cover NaN, infinity, signed zero, underflow and multiply-add rounding.
- [ ] Automatic contraction settings are checked with representative generated code.
- [ ] Baseline artifacts respect the stated instruction-set requirements.
- [ ] Internal assertion behavior matches each configuration and public validation remains active.
- [ ] Caller and worker-thread floating-point states follow the checking/restoration contract.
- [ ] Debug and optimized runs satisfy scene-specific numerical acceptance criteria.
- [ ] Benchmark reports record optimization, floating-point, ISA, assertion and runtime-state settings.

The Windows environment baseline passed build compatibility and toolchain numerical checks. Engine numerical behavior is validated alongside its implementation.

## References

- [Jolt build configuration](https://github.com/jrouwe/JoltPhysics/blob/master/Build/CMakeLists.txt)
- [Jolt target configuration](https://github.com/jrouwe/JoltPhysics/blob/master/Jolt/Jolt.cmake)
- [Genesis initialization](https://github.com/Genesis-Embodied-AI/genesis-world/blob/main/genesis/__init__.py)
- [Quadrants host compiler configuration](https://github.com/Genesis-Embodied-AI/quadrants/blob/main/cmake/QuadrantsCXXFlags.cmake)
- [Clang floating-point behavior](https://clang.llvm.org/docs/UsersManual.html#controlling-floating-point-behavior)
