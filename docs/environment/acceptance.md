# Windows acceptance record

**English** | [简体中文](acceptance.zh-CN.md)

Date: 2026-10-31. Result: passed.

Scope: Windows x64 development environment and static-library infrastructure. The host used PowerShell 7.6.5, LLVM 22.1.6, MSVC compiler 19.51.36248.0 and SDK 10.0.26100.0 with servicing revision 10.1.26100.8249.

| Check | Evidence |
| --- | --- |
| Tool and dependency identities | 22 archive identities, fixed C++ commits, installed payload receipts and a reviewed `uv.lock` |
| Read-only doctor | 76 checks matched with Warp required; 28 regression checks passed |
| C++ configurations | Debug, RelWithDebInfo, Release, ASan and MSVC: 11 CTest cases per configuration |
| Failure propagation | Five controlled GoogleTest failures propagated through CTest |
| Installed package | Five independent consumers compiled and executed against the install trees |
| Memory diagnostics | LLVM ASan reported the expected heap-buffer-overflow with source filename and line |
| Python and environment checks | Nine tests passed, covering hooks, partial staging, archive recovery and Warp |
| Editor | C++/Python definitions, completion, diagnostics, Ruff formatting, breakpoints, stepping, stacks and variable evaluation passed |
| GPU | Warp 1.18.0 CPU, CUDA and graph results matched references on RTX 5070 Ti, driver 610.88 |
| Reconstruction | An independent copy with a space in its path prepared offline, rebuilt all five configurations and passed consumer and Warp checks |
| Source preservation | Verification and reconstruction retained their input source-file hashes |

The environment tests share one Python module. The test directory contains four C++ source files, three Python source files and one PowerShell regression script. The install consumer remains an example project.

The clean reconstruction exposed ASan library-selection and DLL-search-order differences during CMake feature probes. The toolchain now selects the LLVM import libraries explicitly and places the matching runtime directory first. MSVC include-prefix detection also uses explicit UTF-8 decoding, and Ninja header dependencies were checked after a clean build.

## Repeating verification

```powershell
pwsh -NoProfile -File scripts/dev.ps1 prepare -RequireWarp -WithEditor -EnableHooks
pwsh -NoProfile -File scripts/dev.ps1 verify -RequireWarp -WithEditor -Reproduce
```

The editor run opens the project acceptance host. The reconstruction run creates a new copy under `artifacts/repro/` and restores the original editor settings afterward.

Local evidence is recorded in `artifacts/environment/verification.json`, `doctor-final.json`, `editor-acceptance.json`, `reproduction.json`, JUnit files and command logs. The verification record includes the toolchain digest, Python lock digest and input-file hashes. [Preparation](prepare.md) describes the cache and configuration layout.

Windows PowerShell 5.1 syntax was checked; its runtime execution remains subject to the host execution policy. The validated command host is PowerShell 7.6.5. Linux/macOS and their additional sanitizer configurations remain subsequent platform work.
