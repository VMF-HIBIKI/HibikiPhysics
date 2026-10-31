# Build and install consumption

**English** | [简体中文](build.zh-CN.md)

The root CMake project builds the C++20 static target `hibiki::physics`. Its build-information API provides a linkable symbol for installation and debugger checks. Toolchain tests exercise GoogleTest, Eigen, spdlog, Google Benchmark and floating-point behavior.

`CMakePresets.json` defines Windows Debug, RelWithDebInfo, Release, ASan and MSVC compatibility configurations. Each configuration has its own build and install directories. The Windows toolchain reads selected paths from `.local/environment.json`, and resolves the SDK revision from the committed component manifest. C and C++ compiler paths select the same compiler driver.

With the environment paths prepared, the following commands build and test Debug:

```powershell
$environment = Get-Content .local/environment.json -Raw | ConvertFrom-Json
& $environment.paths.cmake --preset windows-debug
& $environment.paths.cmake --build --preset windows-debug
$ctest = Join-Path (Split-Path $environment.paths.cmake) ctest.exe
& $ctest --preset windows-debug
```

`scripts/environment/VerifyCpp.ps1` builds the configuration matrix, executes CTest, installs the static library and builds the independent `examples/install-consumer` project against each install tree. The ASan configuration additionally requires a failing child process with a symbolized heap-buffer-overflow report. Logs and JUnit results are written under `artifacts/environment/`.

```powershell
& ./scripts/environment/VerifyCpp.ps1
& ./scripts/environment/VerifyCpp.ps1 -Presets windows-debug
```

The consumer uses `find_package(HibikiPhysics CONFIG REQUIRED)` and links the exported `hibiki::physics` target. C++ dependency archives are selected by immutable identity from the manifest and consumed from a local directory. The project package exports its own static library and headers.

Windows Debug uses the debug dynamic CRT; optimized configurations use the release dynamic CRT. ASan uses the release dynamic CRT with disabled optimization. LLVM builds explicitly disable fast math and contraction and select the x86-64 baseline. MSVC compatibility builds use strict floating-point mode. Internal-assertion configuration is independently represented in the build-information API.

Current evidence: all five configurations passed 11 CTest cases each and independent installed-library consumer compilation and execution. The ASan child produced the expected symbolized heap-buffer-overflow report. MSVC clean rebuilding and header dependency recording were verified. Environment preparation, editor/debugger integration and reconstruction passed; see the [acceptance record](acceptance.md).
