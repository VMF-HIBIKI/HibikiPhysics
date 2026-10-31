# ADR-0003: Commit hooks and validation

**English** | [简体中文](0003-commit-hooks-and-validation.zh-CN.md)

Status: Accepted

Date: 2026-10-31

Validation status: Local hooks and checks passed; remote PR CI remains subsequent work.
Current Windows evidence is recorded in the [acceptance record](../environment/acceptance.md).

## Context

The project needs fast feedback before commits and repeatable validation in CI. [ADR-0002](0002-editor-debugging-and-code-quality.md) defines formatting and analysis rules. Hook execution uses the same versioned tools as the editor and command line.

## Decision

prek 0.5.9 is the selected hook runner. uv manages its exact version in project development dependencies and uv.lock. The configuration uses .pre-commit-config.yaml. Hook activation occurs during a later environment-initialization step.

Local tool hooks use repo: local and language: system. Ruff commands resolve through uv locked mode to Ruff 0.17.3. A project wrapper resolves clang-format 22.1.6 through the toolchain baseline. Environment preparation provides the required tools; hook startup checks their availability and versions and reports preparation steps for mismatches.

Built-in file checks use the pinned prek implementation where suitable. Additional external hook repositories, if introduced, receive explicit revision and provenance records.

## Validation layers

| Layer | Timing | Scope |
| --- | --- | --- |
| Fast checks | Before each commit | Staged-file formatting, Python lint, file validity and conflict checks |
| Full checks | Explicit local execution and PR CI | Formatting verification, basedpyright, clang-tidy, builds and tests |
| Specialized checks | Relevant changes and milestone acceptance | Sanitizers, physics regressions and performance benchmarks |

Fast checks target a few seconds to tens of seconds. Build-dependent analysis belongs to the full layer. Shared configuration changes trigger the corresponding wider checks during full validation.

## Fast checks and local fixes

- Python lint uses ruff check --fix with the safe-fix policy.
- Python formatting uses ruff format.
- C++ formatting uses the pinned clang-format.
- File checks cover JSON, YAML and TOML syntax, merge-conflict markers and case-conflicting filenames.
- Text normalization covers final newlines and trailing whitespace, preserving intentional Markdown hard breaks and the established line-ending policy.
- Documentation checks verify bilingual counterparts and language links. Review covers semantic correspondence.

Local checks receive the staged-file set. Fixes produce reviewable working-tree edits; developers inspect and stage those edits before retrying the commit. Hook validation covers partially staged files and preserves unrelated working-tree changes. Generated files, third-party sources, virtual environments and build artifacts follow the exclusions in ADR-0002.

## Local and CI execution

Local repair and CI verification share rule files. Their hook entries or wrapper modes distinguish writing from checking:

| Local repair | CI verification |
| --- | --- |
| ruff check --fix | ruff check |
| ruff format | ruff format --check |
| clang-format writing mode | clang-format check mode |

CI executes check-only operations and records diagnostics. Full validation also runs basedpyright, clang-tidy, builds and tests. The eventual wrapper exposes the same check-only entry points for local use. Checks requiring complete project context inspect that context even when a smaller changed-file set triggered execution.

PR CI validates the type: description title format, the link to one implementation issue, and exact equality between the issue and PR titles. Local commit hooks cover local files; PR CI resolves remote metadata. Title checks run when relevant PR metadata changes as well as during code validation. Remote workflow configuration is future implementation work.

## Consequences and validation

The project owns the hook wrappers and file-selection rules. A shared tool source keeps editor, CLI and hook versions consistent. Fast feedback and full validation have separate execution costs and schedules.

- [x] The exact prek package and transitive environment are recorded in uv.lock.
- [x] Hook configuration validates under prek 0.5.9.
- [ ] Git hook activation resolves the project environment from terminal and VS Code commits.
- [x] Staged-file checks, local fixes and partially staged changes behave as specified.
- [ ] CI check-only execution preserves the checkout and reports failures.
- [x] Tool-version mismatches produce actionable diagnostics.
- [x] Full checks use the selected compilation database and Python environment.
- [ ] PR metadata validation enforces the title and issue mapping rules.

This record establishes the accepted workflow. Local installation and hook execution passed; remote CI activation follows repository hosting.

## References

- [prek 0.5.9](https://pypi.org/project/prek/0.5.9/)
- [prek configuration](https://prek.j178.dev/configuration/)
- [Local hooks](https://prek.j178.dev/local-hooks/)
