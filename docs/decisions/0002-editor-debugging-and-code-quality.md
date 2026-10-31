# ADR-0002: Editor, debugging and code quality

**English** | [简体中文](0002-editor-debugging-and-code-quality.zh-CN.md)

Status: Accepted

Date: 2026-10-31

Confirmed preference: VS Code is the primary editor.

Validation status: Windows editor, CLI and debugger integration passed.
Current Windows evidence is recorded in the [acceptance record](../environment/acceptance.md).

## Context

The project needs consistent editor and command-line diagnostics, reproducible formatting and useful Windows debugging. [ADR-0001](0001-development-toolchain-and-dependencies.md) defines the compiler and dependency baseline.

## Alternatives

Windows native debugging can use the Visual Studio Windows Debugger through cppvsdbg or an LLDB-based adapter. The decision selects cppvsdbg for the existing MSVC-ABI and PDB workflow. Other platforms will receive their own debugger configuration.

Python language services can use Pyright/Pylance, basedpyright or other LSP implementations. The decision selects basedpyright because its CLI and language server share a versioned Python package managed by uv. Ruff handles linting and formatting through its native language server and CLI.

## Selected versions and roles

| Component | Selected version | Role and provenance |
| --- | --- | --- |
| VS Code | 1.143.0, Windows x64 | Installed editor reference; editor version recorded for reproduction |
| C/C++ extension | ms-vscode.cpptools 1.34.4, win32-x64 | Installed baseline; cppvsdbg supplies Windows native debugging |
| clangd client | llvm-vs-code-extensions.vscode-clangd 0.6.0 | Installed client; starts the selected clangd 22.1.6 executable |
| Python extension | ms-python.python 2026.10.0, win32-x64 | Installed baseline for interpreter selection and test integration |
| Python Debugger extension | ms-python.debugpy 2026.6.0, win32-x64 | Installed adapter-client baseline |
| debugpy package | 1.8.22 | Project Python debug server, pinned through uv |
| basedpyright | 1.40.2 | Python completion, navigation and type checking |
| Ruff | 0.17.3 | Python formatting, linting and import organization |
| C++ quality tools | clang-format and clang-tidy 22.1.6 | Existing LLVM baseline |

ENV-001 selects detachhead.basedpyright 1.40.2 universal and charliermarsh.ruff 2026.92.0 win32-x64, with their VSIX hashes recorded in tools/toolchain-lock.json. Their server paths explicitly select project-managed executables. basedpyright brings nodejs-wheel-binaries as a transitive dependency; uv.lock records the resolved package version and artifacts.

The environment record captures extension IDs, versions, platform packages and hashes. A dedicated project editor profile keeps extension changes explicit. Extension recommendations provide discoverability; the environment checker verifies versions.

## Debugging

Windows C++ debugging uses cppvsdbg with the executable and PDB from the selected CMake configuration. Debug provides full debug information with optimization disabled. RelWithDebInfo provides an optimized diagnostic configuration. The acceptance check covers source breakpoints, stepping, stacks, STL values, project types and a crash location. Project types can acquire Natvis descriptions as their layouts become established.

clangd owns C++ completion and diagnostics. The C/C++ extension supplies debugging, with its IntelliSense engine and formatting contribution disabled in the project profile. Formatting selects the pinned clang-format executable.

Python debugging starts the project's locked debugpy package through uv on a loopback endpoint and uses VS Code attach. This makes the server version explicit. Interpreter selection resolves the project's .venv. Debugpy covers Python host execution. Warp kernel-level inspection requires a separate workflow; kernel checks initially use Warp diagnostics and CPU/reference comparisons.

## Python language service

basedpyright owns type diagnostics, navigation, completion and hover. Ruff owns lint diagnostics, formatting and import organization. Each capability has a single configured provider.

The type-check baseline uses Python 3.12 and standard mode. Ordinary scripts and host-side utilities receive strict checking as typed interfaces are established. Warp kernels and their adapters use standard mode initially. Dynamic APIs receive small typed adapters or local, rule-specific suppressions with an explanation. These settings are validated against a representative Warp kernel and host script.

## Formatting

| Setting | C++ | Python |
| --- | --- | --- |
| Tool | clang-format 22.1.6 | Ruff 0.17.3 |
| Basis | Project C++ rules | Ruff stable formatter |
| Indentation | Tabs, width 4 | 4 spaces |
| Line width | Context-based wrapping | 120 columns |
| Line endings | LF | LF |
| Other choices | Allman braces, pointer/reference symbols next to names, preserved include order and comment wrapping | Double quotes, Python 3.12 target, preview features disabled |

UTF-8, a final newline and trimmed trailing whitespace are shared defaults. Markdown retains intentional hard-break spaces. PowerShell files retain the repository's CRLF convention. External sources, generated code, build trees and virtual environments are excluded from project formatting.

The clang-format settings are IndentWidth=4, TabWidth=4, UseTab=Always, ColumnLimit=0, BreakBeforeBraces=Allman, PointerAlignment=Right, ReferenceAlignment=Right, DerivePointerAlignment=false, SortIncludes=Never, ReflowComments=Never and AlignConsecutiveDeclarations=Consecutive. Declaration columns use consistent alignment. Short accessors may remain on one line.

C++ types and ordinary functions use PascalCase. Members use an m prefix, constants a c prefix, and static functions an s prefix. Parameters use in, out or io prefixes to indicate input, output or input/output roles. Local variables use lowerCamelCase. Standard-required names and third-party override signatures retain their required spelling. Python uses snake_case functions and variables, PascalCase classes and UPPER_SNAKE_CASE constants.

## Static checks

### C++

Project targets use /W4 with clang-cl and MSVC. CI treats project warnings as errors after the initial baseline is established. Clang conversion and sign-conversion diagnostics begin as advisory checks because numerical code needs a reviewed conversion policy. Third-party target warning settings remain scoped to those dependencies.

clang-tidy uses the active compilation database and filters diagnostics to project code. The initial checks are:

```text
clang-analyzer-*
bugprone-assert-side-effect
bugprone-dangling-handle
bugprone-sizeof-expression
bugprone-use-after-move
performance-for-range-copy
performance-unnecessary-copy-initialization
modernize-use-nullptr
modernize-use-override
misc-definitions-in-headers
bugprone-unused-raii
```

Each suppression identifies the check and the reason. The initial scan establishes the blocking baseline; new checks enter through reviewed changes. Numerical conventions, allocation behavior and data races also receive dedicated tests and runtime analysis in later validation work.

### Python

Ruff's base rule selection is I, E, F and W, covering import ordering, code style and common errors. The ignored rules are E111, E114, E117 and E741. The E741 exception permits short mathematical symbol names.

Docstring code formatting is enabled (docstring-code-format=true), and diagnostic output uses concise format. First-party package names will be specified explicitly when the Python package structure is established. The __init__.py files that perform re-exports receive path-specific F401 and F403 exceptions. Generated files are excluded by their generation directories; handwritten .pyi files remain within the check scope.

The editor, CLI, CI and future commit hooks use the same uv-managed Ruff 0.17.3. Explicit saves in VS Code apply Ruff formatting, safe fixes and import organization. The hook runner is prek 0.5.9, managed through uv. [ADR-0003](0003-commit-hooks-and-validation.md) defines its accepted workflow; the selected hook is installed and validated.

The project specifies required-version=0.17.3, target-version=py312, line-length=120 and preview=false. Automated fixes use the safe-fix policy; changes affecting behavior receive explicit review. basedpyright handles type errors. CI executes check-only commands, while local formatting produces reviewable edits.

## Configuration and validation

Future implementation places formatting in .clang-format, C++ analysis in .clang-tidy, Python tool settings in pyproject.toml, common whitespace in .editorconfig, and editor templates in a tracked configuration directory. Local editor files use resolved machine paths. The existing .vscode ignore rule will be revisited explicitly when shared editor configuration is implemented.

Expected check commands, once tools and source files are prepared, include:

```text
clang-format --dry-run --Werror <project C++ files>
clang-tidy -p <selected build directory> <project translation units>
uv run --locked ruff format --check .
uv run --locked ruff check .
uv run --locked basedpyright
```

Acceptance includes the actual server versions, editor/CLI diagnostic agreement, repeatable formatting, representative C++ and Python debugging, and scoped handling of Warp annotations. Windows installation, integration and runtime verification passed.

## References

- [VS Code C++ debugging](https://code.visualstudio.com/docs/cpp/cpp-debug)
- [debugpy](https://github.com/microsoft/debugpy)
- [basedpyright installation](https://docs.basedpyright.com/latest/installation/command-line-and-language-server/)
- [basedpyright configuration](https://docs.basedpyright.com/latest/configuration/config-files/)
- [Ruff editor integration](https://docs.astral.sh/ruff/editors/)
- [Ruff settings](https://docs.astral.sh/ruff/settings/)
- [clang-tidy checks](https://clang.llvm.org/extra/clang-tidy/checks/list.html)
