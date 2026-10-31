"""Check repository identity records using the Python standard library."""

import argparse
import hashlib
import json
import re
import sys
import tomllib
from pathlib import Path


def sha256(path: Path) -> str:
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def check(root: Path, archive_directory: Path | None = None) -> list[str]:
    errors: list[str] = []
    manifest = json.loads((root / "tools/toolchain-lock.json").read_text(encoding="utf-8"))
    artifacts = manifest["artifacts"]
    by_id = {item["id"]: item for item in artifacts}
    if len(by_id) != len(artifacts):
        errors.append("Artifact IDs must be unique")
    required = {
        "llvm",
        "cmake",
        "ninja",
        "uv",
        "python",
        "vscode",
        "eigen",
        "googletest",
        "spdlog",
        "benchmark",
        "warp-lang-wheel",
        "pytest-wheel",
        "ruff-wheel",
        "basedpyright-wheel",
        "debugpy-wheel",
        "prek-wheel",
        "ms-vscode.cpptools",
        "llvm-vs-code-extensions.vscode-clangd",
        "ms-python.python",
        "ms-python.debugpy",
        "detachhead.basedpyright",
        "charliermarsh.ruff",
    }
    for name in sorted(required - by_id.keys()):
        errors.append(f"Required artifact missing: {name}")
    for item in artifacts:
        name = item["id"]
        if item["verification"] not in {"publisher_hash_matched", "downloaded_and_hashed"}:
            errors.append(f"Archive verification pending: {name}")
        if re.fullmatch(r"[0-9a-f]{64}", item.get("sha256") or "") is None:
            errors.append(f"SHA-256 required: {name}")
        if "publisher_sha256" in item and item["sha256"] != item["publisher_sha256"]:
            errors.append(f"Publisher hash mismatch: {name}")
        if archive_directory is not None:
            path = archive_directory / item["filename"]
            if not path.is_file():
                errors.append(f"Archive missing: {name}")
            elif path.stat().st_size != item["size"] or sha256(path) != item["sha256"]:
                errors.append(f"Archive content mismatch: {name}")

    system_ref = manifest["system_components"]
    system_path = root / system_ref["path"]
    if sha256(system_path) != system_ref["sha256"]:
        errors.append("Windows component record hash mismatch")
    system = json.loads(system_path.read_text(encoding="utf-8"))
    if not system["files"] or not system["packages"]:
        errors.append("Windows component inventory required")
    for package in system["packages"]:
        for payload in package["payloads"]:
            if re.fullmatch(r"[0-9a-f]{64}", payload["sha256"]) is None:
                errors.append(f"System payload SHA-256 required: {package['id']}")

    interpreter = (root / ".python-version").read_text(encoding="utf-8").strip()
    if interpreter != manifest["python"]["version"] or interpreter != by_id["python"]["version"]:
        errors.append("Python version declarations differ")
    project = tomllib.loads((root / "pyproject.toml").read_text(encoding="utf-8"))
    for dependencies in project["dependency-groups"].values():
        for declaration in dependencies:
            name, version = declaration.split("==")
            if by_id[name + "-wheel"]["version"] != version:
                errors.append(f"Python dependency version differs: {name}")

    notices = json.loads((root / "third_party/licenses/sources.json").read_text(encoding="utf-8"))
    collected = {notice["artifact"] for notice in notices}
    for item in artifacts:
        if item["kind"] in {"cpp_source", "python_wheel"} and item["id"] not in collected:
            errors.append(f"License record required: {item['id']}")
    for notice in notices:
        if notice["archive_sha256"] != by_id[notice["artifact"]]["sha256"]:
            errors.append(f"License archive identity differs: {notice['path']}")
        if sha256(root / notice["path"]) != notice["sha256"]:
            errors.append(f"License text hash mismatch: {notice['path']}")
    return errors


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archives", type=Path, help="Also verify the selected local archive files")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    try:
        errors = check(root, args.archives)
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f"Invalid identity records: {error}", file=sys.stderr)
        return 1
    for error in errors:
        print(error, file=sys.stderr)
    if errors:
        return 1
    print("Identity consistency checks passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
