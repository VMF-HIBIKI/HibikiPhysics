# Windows toolchain identities

**English** | [简体中文](windows-identities.zh-CN.md)

Recorded: 2026-10-31. Scope: ENV-001.

Status: ENV-001 complete. All 22 selected archives pass SHA-256 verification: six tool/editor distributions, six VSIX packages, six direct Python wheels and four C++ source archives. The record includes 18 installed Windows file hashes and 21 original license texts. Schema validation and cross-file consistency checks pass. Negative checks reject incomplete hashes, pending records marked complete, duplicate IDs, publisher-hash conflicts, Python-version drift and changed license bytes. Build and runtime validation follows the implementation plan.

## Records

[toolchain-lock.json](../../tools/toolchain-lock.json) records selected tool archives, Python wheels, VSIX packages and C++ source archives. Each entry contains its version, platform, source URL, SHA-256 and verification state. C++ entries also contain the resolved source commit. [Dependencies.cmake](../../cmake/Dependencies.cmake) reads these source identities and declares dependency options. Its caller supplies a verified local archive directory.

[Windows component records](../../tools/provenance/windows-system-components.json) retain official Visual Studio catalog package identities and payload hashes, together with hashes of selected installed compiler files, STL headers, CRT libraries and SDK files. The inventory includes ancillary installed MSVC targets and resource packages. ENV-004 will derive and validate the installer component selection needed for Windows x64.

| Component | Recorded identity |
| --- | --- |
| MSVC toolset directory | 14.51.36231 |
| Compiler file / product version | 19.51.36248.0 / 14.51.36248.0 |
| CRT headers package | 14.51.36244 |
| x64 desktop CRT package | 14.51.36247 |
| SDK include/library selector | 10.0.26100.0 |
| SDK installed DisplayVersion | 10.1.26100.8249 |
| SDK Visual Studio catalog package | Win11SDK_10.0.26100, 10.0.26100.16 |

These fields identify different layers of the serviced installation. Machine-specific absolute paths remain local. The component record uses paths relative to the selected MSVC and SDK roots.

## Python and editor packages

[.python-version](../../.python-version) selects CPython 3.12.16. [pyproject.toml](../../pyproject.toml) declares exact direct versions in the `dev` and `warp` dependency groups. The interpreter artifact follows the regular-GIL Windows x64 entry in [uv 0.13.6 download metadata](https://github.com/astral-sh/uv/blob/0.13.6/crates/uv-python-managed/download-metadata.json): build 20261026, `install_only_stripped`.

The first `uv.lock` and transitive package review belong to ENV-004. The selected wheel records describe direct package artifacts. basedpyright's Node.js dependency and Warp's transitive dependencies receive their resolved identities through that lock.

VSIX identities include extension version and target platform. The selected Ruff client is `charliermarsh.ruff` 2026.92.0 for win32-x64; the basedpyright client is `detachhead.basedpyright` 1.40.2 universal. ENV-005 connects these clients to the selected project servers.

## Verification states

| State | Evidence |
| --- | --- |
| `publisher_metadata_only` | Official metadata supplies the expected artifact identity; local archive verification remains pending |
| `publisher_hash_matched` | Downloaded bytes match the publisher's SHA-256 and published size when available |
| `downloaded_and_hashed` | Downloaded bytes have a locally calculated SHA-256; the record preserves the source URL |
| `installed_files_hashed` | Selected existing installation files have recorded SHA-256 values |

The [schema](../../tools/toolchain-lock.schema.json) accepts partial records under `identity_recording`. An `identity_complete` record requires verified archive states and complete hashes and sizes. Publisher-hash equality, unique artifact IDs, companion-file integrity and cross-file version agreement receive additional consistency checks. Compiler execution, package installation, debugger integration and clean reconstruction remain ENV-002 through ENV-006 acceptance work.

The local evidence cache is `artifacts/env001/`. It holds downloaded archives and upstream API responses. The versioned manifest retains stable publisher URLs; temporary signed redirect URLs remain in local download reports. Future preparation verifies cached bytes against the committed hashes.

`scripts/environment/check_identities.py` checks cross-file versions, publisher hashes, required artifact IDs and license records. Its optional `--archives artifacts/env001/downloads` argument also hashes cached archives. The initial record checks use the existing host Python and PowerShell; project-tool execution follows environment preparation. JSON Schema validation uses a Draft 2020-12 validator. The initial schema check uses PowerShell 7 `Test-Json`.

## License records

[Third-party notices](../../third_party/licenses/README.md) preserve verbatim license files from the selected C++ archives and direct Python wheels. Their source-member paths and SHA-256 values are recorded separately. Original upstream license texts retain their original language. Tool distributions and extensions retain their accompanying licenses during environment preparation; redistribution packaging receives its own review.
