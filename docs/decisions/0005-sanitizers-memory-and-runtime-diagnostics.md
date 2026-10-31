# ADR-0005: Sanitizers, memory checks and runtime diagnostics

**English** | [简体中文](0005-sanitizers-memory-and-runtime-diagnostics.zh-CN.md)

Status: Accepted

Date: 2026-10-31

Validation status: Windows ASan and Warp CPU/CUDA/graph checks passed; additional platform diagnostics follow their milestones.
Current Windows evidence is recorded in the [acceptance record](../environment/acceptance.md).

## Context

The engine needs memory-safety checks, numerical fault localization and reproducible failure reports. [ADR-0001](0001-development-toolchain-and-dependencies.md) defines the toolchain, [ADR-0003](0003-commit-hooks-and-validation.md) defines validation layers, and [ADR-0004](0004-build-and-numerical-policy.md) defines the numerical baseline.

## Alternatives and decision

The considered approaches include relying on ordinary debug assertions, enabling a single diagnostic configuration across all platforms, and using platform-specific instrumented builds with separate GPU diagnostics. The selected approach uses Windows ASan as the initial memory-check entry point, Linux for additional undefined-behavior, leak and concurrency checks, and a dedicated Warp diagnostic workflow.

Diagnostic configurations have separate build directories and reports from ordinary Debug/Release configurations. The schedule follows affected changes and milestone requirements.

## C++ check matrix

| Configuration | Platform plan | Main checks | Execution scope |
| --- | --- | --- | --- |
| ASan | Windows x64 first, Linux additionally | Out-of-bounds access, use-after-free, double-free and supported lifetime errors | Memory and lifecycle changes |
| ASan + UBSan | Linux | Memory errors, alignment, signed integer overflow, invalid shifts and other enabled UB checks | Core algorithm changes and full validation |
| LeakSanitizer | Linux, alongside ASan | Memory leaks | Lifecycle tests and milestone acceptance |
| TSan | Linux, separate configuration | CPU data races | Parallel features after their introduction |
| MemorySanitizer | Later dedicated evaluation | Uninitialized-memory reads | Selected difficult investigations |

ThreadSanitizer platform support and runtime availability determine its deployment. MemorySanitizer requires a dedicated instrumented dependency environment. Their supported-platform lists and the selected toolchain are checked during implementation.

## Instrumented build policy

- Engine internal assertions and debug symbols are enabled.
- LTO is disabled. The floating-point and instruction-set baseline follows ADR-0004.
- Windows ASan starts with /Od and dynamic CRT /MD. A minimal executable validates runtime compatibility.
- Linux diagnostic configurations use -O1, debug symbols and frame pointers.
- ASan and TSan use separate builds and executions.
- Reported diagnostic errors fail the affected test process.

ASan runtime and symbolization tools follow LLVM 22.1.6. Validation records the actual runtime paths. Engine targets, test executables and applicable source dependencies use the matching instrumentation configuration. Reports state the instrumented coverage and any external components.

Linux supplies the planned leak acceptance checks. Windows ASan coverage follows the capabilities verified for that configuration. UBSan check selection and failure behavior are made explicit during configuration; additional numerical checks receive their own documented interpretation.

## Engine memory diagnostics

| Capability | Purpose |
| --- | --- |
| Active allocation count and bytes | Resource accounting across creation and destruction |
| Peak usage and per-step allocation statistics | Growth detection and hot-path allocation analysis |
| Debug allocator mode | Suballocation boundary protection |
| Scratch-memory lifetime markers | Detection of references retained across simulation steps |
| Generational handle validation | Detection of references to destroyed objects |
| Repeated creation/destruction tests | Full World, scene and resource lifecycle verification |

Pools allocate large backing blocks, so suballocation boundaries need allocator-specific checks. Allocator design will select ASan poisoning or a debug allocation mode and define alignment, poisoning and reuse behavior. Resource accounting distinguishes active allocations from deliberately retained caches.

## Warp diagnostic modes

The selected Warp version is 1.18.0. Its diagnostic controls include debug compilation mode, line information, verify_fp and verify_cuda. Each control is verified against the selected release and representative kernels.

| Mode | Behavior |
| --- | --- |
| Ordinary diagnostics | Line information, device/kernel/launch dimensions, and result checks at defined stage boundaries |
| Fault localization | Small scenes, debug kernels, applicable floating-point checks and staged synchronization |

verify_cuda is used on ordinary launch paths. CUDA Graph capture uses a separately reproduced diagnostic path because this flag has a documented capture restriction. CPU/reference comparisons and explicit state checks supplement Warp diagnostics. Debugpy continues to cover Python host execution as defined in ADR-0002.

## GPU memory and synchronization checks

NVIDIA Compute Sanitizer supplies the planned GPU checks:

| Tool | Purpose |
| --- | --- |
| memcheck | Device-memory out-of-bounds and misaligned accesses |
| initcheck | Uninitialized device-memory reads in the selected address spaces |
| racecheck | Shared-memory access hazards |
| synccheck | Synchronization-primitive misuse |

Global-memory concurrency correctness also receives algorithm-level tests and review. A clean memcheck run precedes interpretation of initialization-check results.

Compute Sanitizer is an additional development tool. Its exact version and compatibility with the driver, GPU and Warp-generated code will be selected with the GPU diagnostic environment. Warp wheel runtime requirements and diagnostic-tool installation requirements are recorded separately. Diagnostic failures produce a failing test result through explicit exit-code handling.

## Reproducible failure records

Failure artifacts contain:

- Commit SHA, build configuration, toolchain and sanitizer versions.
- Scene settings, random seed and simulation step.
- Stable identifiers for relevant bodies, shapes and constraints.
- Current simulation stage, residuals and the first detected invalid numerical value.
- Stack traces, logs and relevant reproduction inputs.
- GPU device, driver, kernel and synchronization location for GPU cases.

Stage checks can cover finite state after integration, normal and depth conventions after contact generation, and residuals after solving. Diagnostic and performance runs have separate reports so instrumentation costs remain identifiable.

## Implementation order and acceptance

The order is Windows ASan, Linux ASan/UBSan and leak checks, TSan with parallel features, and Compute Sanitizer with Warp work. Each configuration first runs a controlled faulty program in an isolated test process to verify detection and failure propagation, followed by clean controls and real project tests.

- [x] Selected instrumentation runtimes are available and versioned.
- [x] Controlled memory errors produce actionable, symbolized reports and failing test results.
- [x] Clean control cases pass under each enabled configuration.
- [x] Windows /MD, clang-cl and lld-link work together in the ASan build.
- [ ] Linux UB and leak checks cover representative project behavior.
- [ ] Allocator suballocation and lifetime checks behave as documented.
- [ ] Concurrency checks run against the introduced parallel code.
- [x] Warp launch and graph diagnostic paths follow their supported settings.
- [ ] GPU check failures propagate to the test runner.
- [x] Reports preserve reproduction inputs and identify instrumentation coverage.

This decision records the accepted diagnostic strategy. Windows ASan execution passed; additional platform and engine diagnostics follow their milestones.

## References

- [Jolt build configuration](https://github.com/jrouwe/JoltPhysics/blob/master/Build/CMakeLists.txt)
- [Quadrants ASan configuration](https://github.com/Genesis-Embodied-AI/quadrants/blob/main/cmake/QuadrantsCXXFlags.cmake)
- [Genesis debug initialization](https://github.com/Genesis-Embodied-AI/genesis-world/blob/main/genesis/__init__.py)
- [AddressSanitizer](https://clang.llvm.org/docs/AddressSanitizer.html)
- [UndefinedBehaviorSanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html)
- [LeakSanitizer](https://clang.llvm.org/docs/LeakSanitizer.html)
- [ThreadSanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html)
- [MemorySanitizer](https://clang.llvm.org/docs/MemorySanitizer.html)
- [Warp 1.18.0 diagnostic settings](https://github.com/NVIDIA/warp/blob/v1.18.0/warp/config.py)
- [NVIDIA Compute Sanitizer](https://docs.nvidia.com/compute-sanitizer/ComputeSanitizer/index.html)
