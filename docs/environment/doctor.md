# Windows environment diagnostics

**English** | [简体中文](doctor.zh-CN.md)

`scripts/dev.ps1 doctor` inspects the selected Windows development environment. It reads repository declarations, PE version resources and architecture, file hashes, Visual Studio installation records, SDK registration, Python distribution metadata and VS Code extension metadata.

## Usage

From the repository root:

```powershell
pwsh -NoProfile -File .\scripts\dev.ps1 doctor
pwsh -NoProfile -File .\scripts\dev.ps1 doctor -RequireWarp
pwsh -NoProfile -File .\scripts\dev.ps1 doctor -ReportPath artifacts/environment/doctor.json
```

The default command writes its report to the terminal. `-ReportPath` explicitly writes a UTF-8 JSON file inside `artifacts/environment/`. The output path uses ordinary directories, with existing reparse points rejected. Exit codes are `0` for matched required checks, `1` for required diagnostic failures, and `2` for invocation, input-reading or report-output errors.

All inspection happens through file and registry reads. Tools, Python, extension services and installer programs remain inactive. Network access, environment preparation and editor integration belong to subsequent implementation stages.

## Path selection

Command-line `-Paths` entries override the selected local configuration. `-ConfigPath` selects a JSON file; its default is `.local/environment.json`. An absent default file uses automatic discovery. An explicitly requested file receives a required validity check. Explicit component paths retain their selection when their targets are missing.

Paths are literal strings. Relative paths resolve against the repository root. A PowerShell session can pass an override directly:

```powershell
& .\scripts\dev.ps1 doctor -Paths @{
    'clang-cl' = 'C:\Development Tools\LLVM\bin\clang-cl.exe'
    visualStudio = 'C:\Program Files\Microsoft Visual Studio\18\Community'
}
```

The local configuration contract is:

```json
{
  "schema_version": 1,
  "paths": {
    "toolsRoot": "C:/Development Tools/HibikiPhysics/tools",
    "clang-cl": "C:/Development Tools/LLVM/bin/clang-cl.exe",
    "extensions": "C:/Development Tools/VSCode/extensions"
  }
}
```

This example describes the input format. ENV-004 owns local configuration generation.

| Path key | Meaning |
| --- | --- |
| `toolsRoot` | Versioned tool root; default `%LOCALAPPDATA%/HibikiPhysics/tools` |
| `clang-cl`, `clangd`, `clang-format`, `clang-tidy`, `lld-link`, `cmake`, `ninja`, `uv`, `python`, `vscode` | Full executable path |
| `visualStudio` | Visual Studio installation root |
| `msvc` | Selected `VC/Tools/MSVC/<version>` directory |
| `sdk` | Windows Kits root containing `Include`, `Lib` and `bin` |
| `extensions` | Directory containing installed extension folders |
| A selected extension ID | Exact installed folder containing its `package.json` |

Automatic tool discovery checks `<toolsRoot>/<artifact>/<version>/bin` and the corresponding version directory. LLVM and VS Code also use their usual installation locations, followed by executable PATH lookup. Python prefers `.venv/Scripts/python.exe`; PATH discovery supplies a diagnostic candidate when the project interpreter is absent. Python package inspection stays scoped to the repository's `.venv`.

Visual Studio discovery prefers the recorded installation revision, then an installation containing the selected toolset directory, then the remaining local instance records. Its compiler/CRT/SDK package revisions and selected file hashes receive separate checks. SDK registration describes the host installation; selected SDK content is checked at its resolved path. Extension discovery prefers the pinned version, with other installed versions available as mismatch evidence. The report records each selected path.

## Report interpretation

| State | Meaning |
| --- | --- |
| `matched` | The stated check matches its expected evidence |
| `missing` | A required file, package or installation record is absent |
| `mismatch` | Observed version, architecture, hash or declaration differs |
| `pending` | The required identity evidence remains incomplete |

Each check includes `required`, `path`, `expected`, `actual`, `detail` and `next_step`. Required states other than `matched` keep `ready` false. Warp package installation is optional by default; `-RequireWarp` makes it required. Direct dependency declarations remain required repository checks. Python declarations are checked against the current exact-pin format in the `dev` and `warp` groups.

The report structure is defined by [doctor-report.schema.json](../../tools/doctor-report.schema.json).

The executable check records its observed SHA-256 and compares it with the verified archive payload receipt. Missing version resources can be resolved through matching pinned payload identity and PE architecture. Installed VSIX files are checked against their archive receipts; side-loaded extension platforms can be identified from the verified selected archive. Python lock identity and installed file hashes are checked against the prepared environment receipt. Evidence that is still missing retains `pending` status. [Preparation](prepare.md) creates these records.

Compiler execution, debugger attachment, LSP operation, dependency imports and GPU checks belong to the acceptance projects and ENV-006. The report describes environment identity at inspection time.

## Validation

```powershell
pwsh -NoProfile -File .\tests\environment\doctor.tests.ps1
```

Tests create isolated fixtures in `artifacts/environment-tests/`. They cover missing tools, wrong versions, architecture reading, explicit overrides, paths containing spaces, declaration conflicts, changed system files, extension metadata, companion-manifest integrity, exit codes and explicit JSON output. File hashes/timestamps and process environment snapshots verify the default read-only behavior. Test fixtures remain available for inspection.

Validation on 2026-10-31 used the existing PowerShell 7.6.5 host; 28 checks passed. Windows PowerShell 5.1 syntax checks passed. Its script execution was blocked by the machine execution policy; runtime validation on that host remains pending. The policy retains its existing setting.

The local diagnostic run found matching selected MSVC/SDK file hashes, a PATH Python candidate differing from CPython 3.12.16, and tools/project packages awaiting preparation. These results describe the current machine. [The implementation plan](../plans/0001-windows-environment.md) tracks subsequent work.
